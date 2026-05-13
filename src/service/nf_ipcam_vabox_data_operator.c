#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nf_ipcam_zmq_utils.h"
#include "nf_ipcam_json_utils.h"
#include "nf_ipcam_vabox_data_operator.h"
// #include "nmf.h"
// #include <nmf_mrtp_pipe.h>
#include <gobjmrtppipe.h>
#include <time.h>
#include "nf_api_dlva.h"

#define IPCAM_VA_OPER_DEBUG (0)
#define BUFFERING_UPDATE_TIME (20)
#define ASPECT_NETWORK_BUFF (5)	//ms
#define BUFFERING_TIME_LIMIT (500) //ms
#define BUFFERING_TIME_DEFAULT (40) //ms
#define MRTP_SYNC_RATE (5) //ms
#define IPCAM_MAX_CHAN	(32)

VB_DATA_OPERATOR operator;
VB_TIME_UPDATOR updator;

static GobjMrtpPipe *_h_mrtp_pipe = NULL;

static unsigned int g_live_buffering_time[IPCAM_MAX_CHAN] = {0,};

typedef struct __PLAY_LIVE_TIME_DELAY_INFO__
{
	unsigned int summed_time;
	unsigned int summed_cnt;
}LIVE_TIME_INFO;

typedef int (*parser_functor)(json_t*);

/////// zmq worker start

typedef struct _ZMQ_WORKER_THREAD_INFO_
{
	GThread *thread_id;
    int id;
    int is_run;
    GAsyncQueue *task_queue;
}ZMQ_WORKER_THREAD_INFO;

typedef struct _ZMQ_WORKER_POOL_MANAGER_
{
    ZMQ_WORKER_THREAD_INFO *workers;  //thread info array
    size_t worker_size;

    GAsyncQueue *task_queue;

	pthread_mutex_t mutex;
}ZMQ_WORKER_POOL_MANAGER;

typedef struct _ZMQ_WORKER_THREAD_TASK_
{
    //typedef int (*parser_functor)(json_t*);
    parser_functor func;
    json_t *data;
}ZMQ_WORKER_THREAD_TASK;

int worker_thread(void *arg)
{
    int rc;
    int i;

    ZMQ_WORKER_THREAD_INFO *info = (ZMQ_WORKER_THREAD_INFO *)arg;
    ZMQ_WORKER_THREAD_TASK *task;

    if(info == NULL){
        printf("[%s:%d] worker_thread_info is null arg[%p]\n", __func__, __LINE__, arg);
        return -1;
    }

    if(info->task_queue == NULL){
        printf("[%s:%d] task_queue is null arg[%p]\n", __func__, __LINE__, info->task_queue);
        return -1;
    }

    if(info->thread_id == NULL){
        return NULL;
    }

    printf("[%s:%d] id[%d] worker is started\n", __func__, __LINE__, info->id);

	while(info->is_run)
	{
		if(g_async_queue_length(info->task_queue) <= 0)
		{
            usleep(100000);
            continue;
        }
        // printf("\e[34m###yanggungg : %s, %d AI_DATA_worker_task_queue length[%d]\e[0m\n", __func__, __LINE__, g_async_queue_length(info->task_queue));

        task = (ZMQ_WORKER_THREAD_TASK *)g_async_queue_try_pop(info->task_queue);
        if(task == NULL) continue;
        // g_message("###yanggungg : %s, %d task poped : %p", __func__, __LINE__, task);

        rc = task->func(task->data);
        if(rc < 0){
            char *debug;
            debug = json_dumps(task->data, JSON_ENCODE_ANY);
            printf("[%s:%d] parse error! task_result[%d] json_data[%s]\n", __func__, __LINE__, rc, debug);
            if(debug) free(debug);
        }
        if(task->data) {
            // g_message("###yanggungg : %s, %d task data free : %p", __func__, __LINE__, task->data);
            json_decref(task->data);
        }
        // g_message("###yanggungg : %s, %d task free : %p", __func__, __LINE__, task);
        free(task);
    }

    return 0;
}

int add_zmq_task(ZMQ_WORKER_POOL_MANAGER *manager, parser_functor func, json_t *json)
{
    ZMQ_WORKER_THREAD_TASK *task = NULL;
    if(manager == NULL || json == NULL || func == NULL){
        printf("[%s:%d] error manager[%p] json[%p] func[%p]\n", __func__, __LINE__, manager, json, func);
        return -1;
    }

    task = (ZMQ_WORKER_THREAD_TASK *)malloc(sizeof(ZMQ_WORKER_THREAD_TASK));
    if(task == NULL){
        printf("[%s:%d] error task object create failed\n", __func__, __LINE__);
        return -1;
    }
    // g_message("###yanggungg : %s, %d task malloc : %p", __func__, __LINE__, task);

    task->func = func;
    task->data = json;

    g_async_queue_push(manager->task_queue, task);
    return 0;
}

int add_worker_pool_task(ZMQ_WORKER_POOL_MANAGER *manager, ZMQ_WORKER_THREAD_TASK *task)
{
    if(manager == NULL || task == NULL) return -1;

    g_async_queue_push(manager->task_queue, task);

    return 0;
}

ZMQ_WORKER_POOL_MANAGER *create_zmq_worker_pool(size_t pool_size)
{
    int i;
    ZMQ_WORKER_POOL_MANAGER *manager = NULL;

    if(pool_size <= 0){
        printf("[%s:%d] error pool_size[%d]\n", __func__, __LINE__, pool_size);
        return NULL;
    }

    //memory alloc manager object
    manager = (ZMQ_WORKER_POOL_MANAGER *)malloc(sizeof(ZMQ_WORKER_POOL_MANAGER));
    if(manager == NULL){
        printf("[%s:%d] error pool_manager malloc failed\n", __func__, __LINE__);
        goto err;
    }
    memset(manager, 0, sizeof(ZMQ_WORKER_POOL_MANAGER));

    //memory alloc workers
    manager->workers = (ZMQ_WORKER_THREAD_INFO *)malloc(sizeof(ZMQ_WORKER_THREAD_INFO) * pool_size);
    if(manager->workers == NULL){
       printf("[%s:%d] error pool_manager->workers malloc failed\n", __func__, __LINE__);
        goto err;
    }
    memset(manager->workers, 0, sizeof(ZMQ_WORKER_THREAD_INFO)*pool_size);

    pthread_mutex_init(&manager->mutex, NULL);
    pthread_mutex_lock(&manager->mutex);

    //create queue
	manager->task_queue = g_async_queue_new();
    if(manager->task_queue == NULL){
        printf("[%s:%d] worker manager, task_queue create failed\n", __func__, __LINE__);
        goto err;
    }

    //create & run thread
    for(i = 0; i < pool_size; i++){
        manager->workers[i].is_run = 1;
        manager->workers[i].task_queue = manager->task_queue;
        manager->workers[i].thread_id = g_thread_create((GThreadFunc)worker_thread, &(manager->workers[i]), TRUE, NULL);
        if(manager->workers[i].thread_id == NULL){
            manager->workers[i].is_run = 0;
            manager->workers[i].task_queue = NULL;
            break;
        }
        manager->workers[i].id = i+1;
        
        printf("[%s:%d] worker_%d created\n", __func__, __LINE__, i);
    }
    manager->worker_size = i;
    pthread_mutex_unlock(&manager->mutex);

    if(manager->worker_size <= 0) goto err;

endl:
    return manager;
err:
    if(manager){
        if(manager->workers){
            free(manager->workers);
        }

        if(manager->task_queue){
            g_async_queue_unref(manager->task_queue);
        }
        free(manager);
    }

    manager = NULL;
    goto endl;
}

int destroy_worker_pool(ZMQ_WORKER_POOL_MANAGER *manager)
{
    ZMQ_WORKER_THREAD_TASK *task;
    int i;

    if(manager == NULL) return -1;
    if(manager->workers == NULL) return -1;

    pthread_mutex_lock(&manager->mutex);
    //thread running flag stop
    for(i = 0; i < manager->worker_size; i++){
        manager->workers[i].is_run = 0;
    }

    //all thread join check
    for(i = 0; i < manager->worker_size; i++){
		g_thread_join(manager->workers[i].thread_id);
    }

    //all queue remove
    while(g_async_queue_length(manager->task_queue)>0)
    {
        task = (ZMQ_WORKER_THREAD_TASK *)g_async_queue_try_pop(manager->task_queue);
        if(task) free(task);
    }
    

    pthread_mutex_unlock(&manager->mutex);
    pthread_mutex_destroy(&manager->mutex);

    //free manager object memory
    if(manager->workers){
        free(manager->workers);
    }
    if(manager->task_queue){
        g_async_queue_unref(manager->task_queue);
    }
    free(manager);
    
    return 0;
}

/////// zmq worker end

ZMQ_WORKER_POOL_MANAGER *worker_manager = NULL;
ZMQ_WORKER_POOL_MANAGER *event_manager = NULL;

static LIVE_TIME_INFO cur_timeinfo[IPCAM_MAX_CHAN];
static LIVE_TIME_INFO past_timeinfo[IPCAM_MAX_CHAN];

static void vabox_metadata_parser_thread_func ();
static int parse_va_eventdata_information(json_t *p_root, VA_EVT_DATA **p_ret_evt_data, VA_GNR_EVT_DATA **p_ret_gnr_evt_data);
static int _nf_ipcam_base64_decode(char *p_text, int numBytes, char *decodedText);
static void nf_ipcam_vabox_live_buffering_time_callback(gint ch, guint *time);

static void vadata_processtime_update_thread_func ();
static int _set_timestamp(json_t *json_timestamp, unsigned int *timestamp, unsigned int *timestampl);
static int _set_topic(VA_EVT_DATA *evt_data, const char *topic);
static int _set_process_time(VA_EVT_DATA *evt_data, json_t *process_time);
static int _parse_metadata(json_t *metadata, VA_EVT_DATA **p_ret_evt_data);
static int _parse_event_data(json_t *event, VA_GNR_EVT_DATA **p_ret_gnr_evt_data);
static void replace_carriage_return(char *src);

static double TimeSpecToSeconds(struct timespec* ts)
{
	    return (double)ts->tv_sec + (double)ts->tv_nsec / 1000000000.0;
}

static double obj_ptime = 0;

static void print_evt_data(VA_EVT_DATA *evt_data)
{
    int i;
    if(evt_data == NULL) return;
#if IPCAM_VA_OPER_DEBUG
    printf("[%s:%d] object[%p] ch[%d] resol width[%d] height[%d]\n", __func__, __LINE__, 
            evt_data, 
            evt_data->ch,
            evt_data->resol_width,
            evt_data->resol_height);

    printf("[%s:%d] topic[%s] timestamp[%u][%u] process_time[%u] meta_count[%d]\n", __func__, __LINE__, 
            evt_data->topic,
            evt_data->timestamp,
            evt_data->timestampl,
            evt_data->process_time,
            evt_data->meta_count);
    for(i = 0; i < evt_data->meta_count; i++){
        printf("[%s:%d] index[%d] id[%d] class[%s] confidence[%f] bbox[%f][%f][%f][%f]\n", __func__, __LINE__, 
                i,
                evt_data->meta_data[i].id,
                evt_data->meta_data[i].class,
                evt_data->meta_data[i].confidence,
                evt_data->meta_data[i].bbx_position[0],
                evt_data->meta_data[i].bbx_position[1],
                evt_data->meta_data[i].bbx_position[2],
                evt_data->meta_data[i].bbx_position[3]);
    }
#endif
}

static void print_gnr_evt_data(VA_GNR_EVT_DATA *evt_gnr_data)
{
    int i;
    if(evt_gnr_data == NULL) return;
#if IPCAM_VA_OPER_DEBUG
    printf("[%s:%d] object[%p] ch[%d] timestamp[%u][%u] trigger_zone_count[%d]\n", __func__, __LINE__, 
            evt_gnr_data, 
            evt_gnr_data->ch,
            evt_gnr_data->timestamp,
            evt_gnr_data->timestampl,
            evt_gnr_data->trigger_zone_count);
    
    printf("[%s:%d] caption    [%s]\n", __func__, __LINE__, evt_gnr_data->caption);
    printf("[%s:%d] title      [%s]\n", __func__, __LINE__, evt_gnr_data->title);
    printf("[%s:%d] description[%s]\n", __func__, __LINE__, evt_gnr_data->description);

    for(i = 0 ; i < evt_gnr_data->trigger_zone_count; i++){
        printf("[%s:%d] index[%d] point[%f][%f]\n", __func__, __LINE__, 
                i,
                evt_gnr_data->trigger_zone_list[i].x,
                evt_gnr_data->trigger_zone_list[i].y);
    }
#endif 
}

static int ai_support_status = 0;

static int is_channel_ai_support(int ch)
{
    char key_value[64];
    int devcam, devbox;

    if(ch < 0 || ch > IPCAM_MAX_CHAN){
        printf("[%s:%d] error ch[%d]\n", __func__, __LINE__, ch);
    }

    snprintf(key_value, 64, "cam.dvabx.cfg.R%d.devcam", ch);
    devcam = nf_sysdb_get_bool(key_value);

    snprintf(key_value, 64, "cam.dvabx.cfg.R%d.devbox", ch);
    devbox = nf_sysdb_get_bool(key_value);

    return(devcam || devbox);
}

static int read_ai_support_status()
{
    int i;
    int ai_support_status = 0;
    for(i = 0; i < IPCAM_MAX_CHAN; i++){
           
    }

    return ai_support_status;
}

static void nf_api_ai_support_status_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    ai_support_status = read_ai_support_status();
}

static void init_ai_support_status()
{
    nf_notify_connect_cb("sysdb_change", nf_api_ai_support_status_change_cb_func, NULL);
    ai_support_status = read_ai_support_status();
}

static int push_onvif_event_data(int ch, json_t *json)
{
    GAsyncQueue *queue = onvif_get_event_queue();
    VA_ONVIF_EVENT_DATA *data = NULL;
    static int onvif_event_malloc_cnt = 0;
    static int json_incref_cnt = 0;

    if(json == NULL) return -1;
    if(ch < 0) return -1;
    if(queue == NULL) return -1;

    data = malloc(sizeof(VA_ONVIF_EVENT_DATA));
    if(data == NULL) return -1;
    // g_message("###yanggungg : %s, %d onvif_event malloc : %p, cnt : %d", __func__, __LINE__, data, onvif_event_malloc_cnt++);

    memset(data, 0, sizeof(VA_ONVIF_EVENT_DATA));
    data->ch = ch;
    data->type = VA_TYPE_GENERIC_EVENT;
    data->json_root = json_incref(json);
    if(data->json_root == NULL){
        printf("[%s:%d] error json_incref failed\n", __func__, __LINE__);
        free(data);
        return -1;
    }
    // g_message("###yanggungg : %s, %d onvif_event json_incref : %p, cnt : %d", __func__, __LINE__, data->json_root, json_incref_cnt++);

    //queue push json_string
    g_async_queue_push(queue, (gpointer)data);
    
    return 1;
}

static int parser_functor_not_found(json_t *json)
{
    if(json == NULL){
        printf("[%s:%d] error! topic command not found json[%p]\n", __func__, __LINE__, json);
    }else{
        char *debug;
        debug = json_dumps(json, JSON_ENCODE_ANY);
        printf("[%s:%d] error! topic command not found json[%s]\n", __func__, __LINE__, debug);
        if(debug) free(debug);
    }

    return -1;
}

static VA_EVT_DATA *parser_evt_data(json_t *json)
{
    VA_EVT_DATA *evt_data = NULL;
    const char *topic = NULL;
    int rc;

	int ch = -1;
    int resol_width;
    int resol_height;
    
    json_t *timestamp = NULL;
    json_t *process_time = NULL;
    json_t *metadata = NULL;
    
    topic = json_string_value(json_object_get(json, "topic"));
	ch = json_integer_value(json_object_get(json, "_nvr_channel"));

	//parse video source
    rc = parse_va_eventdata_information_parse_vsource(json_string_value(json_object_get(json, "source")), &ch, &resol_width, &resol_height);
    if(rc < 0){
        return NULL;
    }

	//timestamp
    timestamp = json_object_get(json, "timestamp");
    if(timestamp == NULL){
        printf("\e[33m [%s][%d][ERROR] timestamp not found... \e[0m\n", __func__, __LINE__);
        return NULL;
    }

	//process_time
    process_time = json_object_get(json, "process_time");
    if(process_time == NULL){
        printf("[%s:%d] process_time is null\n", __func__, __LINE__);
        return NULL;
    }

    //create metadata object
    if(metadata = json_object_get(json, "metadata")){
        rc = _parse_metadata(metadata, &evt_data);
        if(evt_data){
            //set channel data
            evt_data->ch = ch;
            evt_data->resol_width = resol_width;
            evt_data->resol_height = resol_height;

            if(_set_process_time(evt_data, process_time) &&
                    _set_timestamp(timestamp, &(evt_data->timestamp), &(evt_data->timestampl)) &&
                    _set_topic(evt_data, topic))
            {
            }else{
                // g_message("###yanggungg : %s, %d evt_data free : %p", __func__, __LINE__, evt_data);
                free(evt_data);
                evt_data = NULL;
            }
        }
    }
    else{
        printf("\e[33m [%s][%d][ERROR] metadata not found... \e[0m\n", __func__, __LINE__);
    }

    return evt_data;
}

static int parser_functor_meta_data(json_t *json)
{
	int ch;
	VA_EVT_DATA *evt_data = NULL;
	VA_EVT_DATA *onvif_object = NULL;
    static int onvif_obj_malloc_cnt = 0;

    if(json == NULL){
        return -1;
    }

    evt_data = parser_evt_data(json);
    
    if(evt_data){
		ch = evt_data->ch;
        if(is_channel_ai_support(evt_data->ch)){
        	onvif_object = malloc(sizeof(VA_EVT_DATA));
        	if ( onvif_object )
        	{
                // g_message("###yanggungg : %s, %d onvif_object malloc : %p, cnt : %d", __func__, __LINE__, onvif_object, onvif_obj_malloc_cnt++);
        	    memset(onvif_object, 0x00, sizeof(VA_EVT_DATA));
        	    memcpy(onvif_object, evt_data, sizeof(VA_EVT_DATA));
        	    g_async_queue_push(onvif_get_meta_queue(), (gpointer)onvif_object);
        	}
        	g_async_queue_push(operator.vabox_data_queue, (gpointer)evt_data);
		}else{
            // g_message("###yanggungg : %s, %d evt_data free : %p", __func__, __LINE__, evt_data);
			free(evt_data);
		}
    }
    return 0;
}

static VA_GNR_EVT_DATA *parser_gnr_evt_data(json_t *json)
{
    VA_GNR_EVT_DATA *gnr_evt_data = NULL;
    int rc;

	int ch = -1;
    int resol_width;
    int resol_height;

    json_t *timestamp = NULL;
    json_t *eventdata = NULL;

	ch = json_integer_value(json_object_get(json, "_nvr_channel"));

	//parse video source
    rc = parse_va_eventdata_information_parse_vsource(json_string_value(json_object_get(json, "source")), &ch, &resol_width, &resol_height);
    if(rc < 0){
        printf("[%s:%d] parse_va_eventdata_information_parse_vsource[%d] error\n", __func__, __LINE__, ch);
        return NULL;
    }

	//timestamp
    timestamp = json_object_get(json, "timestamp");
    if(timestamp == NULL){
        printf("\e[33m [%s][%d][ERROR] timestamp not found... \e[0m\n", __func__, __LINE__);
        return NULL;
    }

    //create generic eventdata object
    if(eventdata = json_object_get(json, "eventdata")){
        rc = _parse_event_data(eventdata, &gnr_evt_data);
        if(gnr_evt_data){
            gnr_evt_data->ch = ch;
            if(rc > 0 && _set_timestamp(timestamp, &(gnr_evt_data->timestamp), &(gnr_evt_data->timestampl))){
            }else{
				{
					char *debug;
					debug = json_dumps(json, JSON_ENCODE_ANY);
					printf("[%s:%d] error json[%s]\n", __func__, __LINE__, debug);
					if(debug) free(debug);
				}
				if(gnr_evt_data->trigger_zone_list)
					free(gnr_evt_data->trigger_zone_list);
                free(gnr_evt_data);
                gnr_evt_data = NULL;
            }
        }
    }
    else{
        printf("\e[33m [%s][%d][ERROR] eventdata not found... \e[0m\n", __func__, __LINE__);
    }

    return gnr_evt_data;
}

static int parser_functor_event_data(json_t *json)
{
    int ch;
    VA_GNR_EVT_DATA *gnr_evt_data = NULL;
    if(json == NULL){
        return -1;
    }

    gnr_evt_data = parser_gnr_evt_data(json);

    if(gnr_evt_data){
        ch = gnr_evt_data->ch;
        if(is_channel_ai_support(gnr_evt_data->ch)){
            g_async_queue_push(operator.vabox_event_data_queue, (gpointer)gnr_evt_data);
            push_onvif_event_data(ch, json);
        }else{
			if(gnr_evt_data->trigger_zone_list)
				free(gnr_evt_data->trigger_zone_list);
			free(gnr_evt_data);
		}

    }
    return 0;
}

static int parser_functor_heart_beat(json_t *json)
{
    const char *ip_str = NULL;
    unsigned int aibox_ip = 0;
    unsigned int aibox_linklocal_ip = 0;

    json_t *network = NULL;

    if(json == NULL){
        printf("[%s:%d] error json[%p]\n", __func__, __LINE__, json);
        return -1;
    }

    network = json_object_get(json, "network");

    ip_str = json_string_value(json_object_get(network, "ipaddr"));
    if(ip_str){
        aibox_ip = ntohl(inet_addr(ip_str));
    }

    ip_str = json_string_value(json_object_get(network, "linklocal_addr"));
    if(ip_str){
        aibox_linklocal_ip = ntohl(inet_addr(ip_str));
    }

    if(aibox_ip == 0 && aibox_linklocal_ip == 0){
        printf("[%s:%d] error aibox_ip[%08x] linklocal_ip[%08x]\n", __func__, __LINE__, aibox_ip, aibox_linklocal_ip);
        return -3;
    }

    nf_api_aibox_connection_status_update(aibox_ip, json_object_get(json, "vsource"));
    nf_api_aibox_connection_status_update(aibox_linklocal_ip, json_object_get(json, "vsource"));
    return 0;
}

static int parser_functor_aibox_system_data_changed(json_t *json)
{
    int rc = -1;

    unsigned int aibox_ip = 0;
    unsigned int aibox_linklocal_ip = 0;
    const char *ip_str = NULL;

    json_t *network = NULL;

    if(json == NULL){
        return -1;
    }

    //aibox ip check
    {
        char *debug;
        debug = json_dumps(json, JSON_ENCODE_ANY);
#if IPCAM_VA_OPER_DEBUG
        printf("[%s:%d] process json[%s]\n", __func__, __LINE__, debug);
#endif
        if(debug) free(debug);
    }

    //get network info
    network = json_object_get(json, "network");

    ip_str = json_string_value(json_object_get(network, "ipaddr"));
    if(ip_str){
        aibox_ip = ntohl(inet_addr(ip_str));
    }

    ip_str = json_string_value(json_object_get(network, "linklocal_addr"));
    if(ip_str){
        aibox_linklocal_ip = ntohl(inet_addr(ip_str));
    }

    if(aibox_ip == 0 && aibox_linklocal_ip == 0) return -1;
    if(aibox_linklocal_ip == 0) aibox_linklocal_ip = aibox_ip;


	if(aibox_ip){
        rc = nf_api_aibox_update_rules(aibox_ip, get_aibox_ch_mask(aibox_ip));
    }

    if(rc < 0 && aibox_linklocal_ip){
        rc = nf_api_aibox_update_rules(aibox_linklocal_ip, get_aibox_ch_mask(aibox_linklocal_ip));
    }

    if(rc == 0)
	    nf_notify_fire_params("aibox_db_change", NF_AIBOX_DB_RULE, aibox_linklocal_ip, get_aibox_ch_mask(aibox_linklocal_ip), 0);
    return 0;
}

static parser_functor get_command_func(const char *topic)
{
    parser_functor ret = parser_functor_not_found;
    if(topic == NULL){
        printf("[%s:%d] topic is NULL\n", __func__, __LINE__);
        goto endl;
    }

    if(strstr(topic, "Analytics")){
        ret = parser_functor_meta_data;
    }else
    if(strstr(topic, "RuleEngine")){
        ret = parser_functor_event_data;
    }else
    if(strstr(topic, "System/Keepalive/Heartbeat")){
        ret = parser_functor_heart_beat;
    }else
    if(strstr(topic, "System/Database/Changed")){
        ret = parser_functor_aibox_system_data_changed;
    }else
    {
        printf("[%s:%d] error! topic is not defined topic[%s]\n", __func__, __LINE__, topic);
        ret = parser_functor_not_found;
    }

endl:
    return ret;
}

static ZMQ_WORKER_POOL_MANAGER *get_manager(const char *topic)
{
    ZMQ_WORKER_POOL_MANAGER *ret = NULL;

    if(topic == NULL){
        printf("[%s:%d] topic is NULL\n", __func__, __LINE__);
        goto endl;
    }
    // printf("[%s:%d] topic[%s]\n", __func__, __LINE__, topic);

    if(strstr(topic, "RuleEngine")){
#if IPCAM_VA_OPER_DEBUG        
        printf("[%s:%d] topic[%s]\n", __func__, __LINE__, topic);
#endif        
        ret = event_manager;
    }else
    if(strstr(topic, "Analytics")){
        ret = event_manager;
    }else
    {
        ret = worker_manager;
    }
endl:
    return ret;
}

static int json_overflow_except_check(const char *data)
{
    if(strstr(data, "RuleEngine")){
        return 0;
    }
    
    if(strstr(data, "System/Keepalive/Heartbeat")){
        return 0;
    }

    if(strstr(data, "System/Database/Changed")){
        return 0;
    }

    return 1;
}

static void vabox_metadata_parser_thread_func ()
{
	GAsyncQueue* msg_queue = NULL;  
	int rtn;
	void *data = NULL;
	int ch;
	const char *json_str;
    const char *topic = NULL;
    ZMQ_WORKER_THREAD_TASK *task = NULL;
    int sleep_time = 50; //ms

	memset(cur_timeinfo, 0x00, sizeof(LIVE_TIME_INFO) * IPCAM_MAX_CHAN);
	memset(past_timeinfo, 0x00, sizeof(LIVE_TIME_INFO) * IPCAM_MAX_CHAN);
	memset(g_live_buffering_time, 0x00, sizeof(unsigned int) * IPCAM_MAX_CHAN);

	while(operator.is_run)
	{
		msg_queue = get_zmq_msg_queue();

		if(msg_queue == NULL)
		{
#if IPCAM_VA_OPER_DEBUG 
			printf("\e[31m [%s][%d] ERROR! msg queue is NULL\e[0m\n", __func__, __LINE__);
#endif
            usleep(100000); // 100ms
			continue;
		}

		if(g_async_queue_length(msg_queue) > 0)
		{
			data = g_async_queue_try_pop(msg_queue);

			if(data != NULL)
			{
				ch = *(int *)data;
				json_str = (char *)data + 4;
				
#if IPCAM_VA_OPER_DEBUG 
				//parser callback & data push
				printf("\e[34m [%s][%d][popped_data] (%s) \e[0m\n", __func__, __LINE__, json_str);
#endif

                //printf("[%s:%d] zmq metadata queue size[%d]\n", __func__, __LINE__, g_async_queue_length(operator.vabox_data_queue));
                if( (g_async_queue_length(operator.vabox_data_queue) > 512) && (json_overflow_except_check(json_str)) ){
#if IPCAM_VA_OPER_DEBUG                    
                    printf("[%s:%d] zmq_error metadata queue size overflow[%d] except data[%s]\n", __func__, __LINE__, g_async_queue_length(operator.vabox_data_queue), (const char *)data);
#endif                    
                }else{

                    json_t* root = NULL;
                    root = nf_ipcam_load_json((const char *)json_str);

                    if(root){
                        // g_message("###yanggungg : %s, %d json load success : %p", __func__, __LINE__, root);
                        topic = json_string_value(json_object_get(root, "topic"));
#if IPCAM_VA_OPER_DEBUG                        
                        printf("[%s:%d] topic[%s] data[%s]\n", __func__, __LINE__, topic, data);
#endif                   
                        json_object_set_new(root, "_nvr_channel", json_integer(ch));
                        rtn = add_zmq_task(get_manager(topic), get_command_func(topic), root);
                        if(rtn < 0){
                            json_decref(root);
#if IPCAM_VA_OPER_DEBUG                            
                            printf("[%s:%d] worker manager : task add failed rtn[%d]\n", __func__, __LINE__, rtn);
#endif                            
                        }
                    }else{
#if IPCAM_VA_OPER_DEBUG                        
                        printf("[%s:%d] json format error! ch[%d] data[%s]\n", __func__, __LINE__, json_str);
#endif                        
                    }
                }

                if(data) free(data);
			}

            if (g_async_queue_length(msg_queue) > 200) {
                sleep_time = 1;
            } else if (g_async_queue_length(msg_queue) > 150) {
                sleep_time = 10;
            } else if (g_async_queue_length(msg_queue) > 100) {
                sleep_time = 20;
            } else if (g_async_queue_length(msg_queue) > 50) {
                sleep_time = 30;
            } else {
                sleep_time = 50;
            }
        }

		usleep(sleep_time * 100);
	}
}

static char *_mac_to_str(unsigned char *mac, char *buf)
{
	static char ret[50];
	int i;

	if(buf == NULL){
		buf = ret;
	}

	buf[0] = 0;
	for(i = 0; i < 6; i++){
		sprintf(buf + strlen(buf), "%02X:", mac[i]);
	}

	buf[strlen(buf)-1] = 0;

	return buf;
}

typedef struct _aibox_zmq_info
{
    unsigned int ip;
    unsigned char mac[8];
    time_t time;
    int error_count;
}aibox_zmq_info;
aibox_zmq_info aibox_list[IPCAM_MAX_CHAN] = {0, };
unsigned int aloss_status = 0;

/*
aibox_list 는 db 변경시 값 수정진행
최초 keepalive 실행시 init

nf_notify_connect_cb("sysdb_change", nf_api_aibox_url_change_cb_func, NULL);
init
*/

void nf_api_aibox_zmq_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    int i;
	GValue ret_value = {0,};
	char db_mac[100];
	unsigned int db_addr;

    for(i = 0; i < IPCAM_MAX_CHAN; i++){
        //int nf_api_aibox_get_aibox_iplist(unsigned int *list)
		memset(db_mac, 0, sizeof(db_mac));
		db_addr = 0;

		//addr
		if(nf_sysdb_get_key1("cam.ai_box.A%d.addr", i, &ret_value, NULL))
		{
			db_addr = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		
		}
		//mac
		if(nf_sysdb_get_key1("cam.ai_box.A%d.mac", i, &ret_value, NULL))
		{
			//g_stpcpy(db_mac, g_value_get_string(&ret_value));
			strncpy(db_mac, g_value_get_string(&ret_value), sizeof(db_mac) - 1);
			db_mac[sizeof(db_mac) - 1] = 0;
			g_value_unset(&ret_value);
		}
#if IPCAM_VA_OPER_DEBUG
		printf("[%s:%d]index[%d] addr[%s] mac[%s]\n", __func__, __LINE__, i, _ip_to_str(db_addr, NULL), db_mac);
#endif

		//if(db_addr err || db_mac err) continue;
		if(db_addr == 0x00 || db_addr == 0xffffffff){
            memset(&aibox_list[i], 0, sizeof(aibox_zmq_info));
            continue;
        }
		if(db_mac[0] == '\0'){
            memset(&aibox_list[i], 0, sizeof(aibox_zmq_info));
            continue;
        }    

        aibox_list[i].ip = db_addr;
        aibox_list[i].error_count = 0;
        aibox_list[i].time = 0;
        sscanf(db_mac, "%02x:%02x:%02x:%02x:%02x:%02x", &aibox_list[i].mac[0], &aibox_list[i].mac[1], &aibox_list[i].mac[2], &aibox_list[i].mac[3], &aibox_list[i].mac[4], &aibox_list[i].mac[5]);
	}

}

int init_zmq_aibox_list()
{
    int i;
	GValue ret_value = {0,};
	char db_mac[100];
	unsigned int db_addr;

    nf_notify_connect_cb("sysdb_change", nf_api_aibox_zmq_change_cb_func, NULL);
    memset(aibox_list, 0, sizeof(aibox_zmq_info)*IPCAM_MAX_CHAN);

    aloss_status = 0;

    for(i = 0; i < IPCAM_MAX_CHAN; i++){
        //int nf_api_aibox_get_aibox_iplist(unsigned int *list)
		memset(db_mac, 0, sizeof(db_mac));
		db_addr = 0;

		//addr
		if(nf_sysdb_get_key1("cam.ai_box.A%d.addr", i, &ret_value, NULL))
		{
			db_addr = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		
		}
		//mac
		if(nf_sysdb_get_key1("cam.ai_box.A%d.mac", i, &ret_value, NULL))
		{
			//g_stpcpy(db_mac, g_value_get_string(&ret_value));
			strncpy(db_mac, g_value_get_string(&ret_value), sizeof(db_mac) - 1);
			db_mac[sizeof(db_mac) - 1] = 0;
			g_value_unset(&ret_value);
		}
#if IPCAM_VA_OPER_DEBUG
		printf("[%s:%d]index[%d] addr[%s] mac[%s]\n", __func__, __LINE__, i, _ip_to_str(db_addr, NULL), db_mac);
#endif

		//if(db_addr err || db_mac err) continue;
		if(db_addr == 0x00 || db_addr == 0xffffffff) continue;
		if(db_mac[0] == '\0') continue;
    
        aibox_list[i].ip = db_addr;
        aibox_list[i].error_count = 0;
        aibox_list[i].time = 0;
        sscanf(db_mac, "%02x:%02x:%02x:%02x:%02x:%02x", &aibox_list[i].mac[0], &aibox_list[i].mac[1], &aibox_list[i].mac[2], &aibox_list[i].mac[3], &aibox_list[i].mac[4], &aibox_list[i].mac[5]);
	}

}

static int _set_process_time(VA_EVT_DATA *evt_data, json_t *json_process_time)
{
    if(evt_data == NULL || json_process_time == NULL) return 0;

    evt_data->process_time = json_integer_value(json_process_time);

    g_mutex_lock(updator.mutex);
    //process time info update
    cur_timeinfo[evt_data->ch].summed_time += evt_data->process_time;
    cur_timeinfo[evt_data->ch].summed_cnt += 1;
    g_mutex_unlock(updator.mutex);
    
#if IPCAM_VA_OPER_DEBUG
        printf("\e[31m [%s][%d] process time (%d) \e[0m\n", __func__, __LINE__, evt_data->process_time);
#endif

    return 1;
}

static int _set_timestamp(json_t *json_timestamp, unsigned int *timestamp, unsigned int *timestampl)
{
    if(json_timestamp == NULL || timestamp == NULL || timestampl == NULL) return 0;

    char pars_string[32];
	char *rbuf;
    memset(pars_string, 0x00, 32);
    snprintf(pars_string, 32, "%f", json_real_value(json_timestamp));

    char *ptr = strtok_r(pars_string, ".", &rbuf);
    if(ptr != NULL)
        *timestamp = atoi(ptr);
    else
        return 0;

    ptr = strtok_r(NULL, " ", &rbuf);
    if(ptr != NULL)
        *timestampl = atoi(ptr);
    else
        return 0;

    //time dump test code
    //printf("\e[95m json timestamp (%f) \e[0m\n", timestamp);
    //obj_ptime = (*timestamp + (*timestampl / 1000000000.0)) - obj_ptime;
    //if(obj_ptime > 0)
    //  printf("\e[104m json time diff (%f) \e[0m\n", obj_ptime);
    //obj_ptime = (*timestamp + (*timestampl / 1000000000.0));

#if IPCAM_VA_OPER_DEBUG
    printf("\e[31m [%s][%d] obj timestamp (%d) \e[0m\n", __func__, __LINE__, *timestamp);
    printf("\e[31m [%s][%d] obj timestampl (%d) \e[0m\n", __func__, __LINE__, *timestampl);
#endif

    return 1;
}

static int _set_topic(VA_EVT_DATA *evt_data, const char *topic)
{
    if(topic && evt_data){
        strncpy(evt_data->topic, topic, 63);
        return 1;
    }
    return 0;
}

static const char *str_null_to_blank(const char *str)
{
	static const char blank[2] = {0, };
	if(str) return str;
	return blank;
}

static int _parse_event_data(json_t *event, VA_GNR_EVT_DATA **p_ret_gnr_evt_data)
{
    int ret = 0;

    int i;
    const char *str = NULL;

    VA_GNR_EVT_DATA *p_obj = NULL;
    json_t *event_info = NULL;
    json_t *trigger_info = NULL;
    json_t *bbox = NULL;
    json_t *zone_info_list = NULL;
    json_t *direction= NULL;
    json_t *value = NULL;

    //arg check
    if(event == NULL || p_ret_gnr_evt_data == NULL) return 0;

    //malloc
    p_obj = malloc(sizeof(VA_GNR_EVT_DATA));
    if(p_obj == NULL){
        printf("[%s:%d] VA_GNR_EVT_DATA malloc failed\n", __func__, __LINE__);
        return 0;
    }
    memset(p_obj, 0, sizeof(VA_GNR_EVT_DATA));

        //event_info(caption, title, description)
    event_info = json_object_get(event, "event_info");

    //caption
    str = json_string_value(json_object_get(event_info, "caption"));
    if(str == NULL){
        printf("[%s:%d] json_event caption not found\n", __func__, __LINE__);
        goto err;
    }
    snprintf(p_obj->caption, sizeof(p_obj->caption), "%s", str);
	//'\n' remove
	replace_carriage_return(p_obj->caption);

    //title
    str = json_string_value(json_object_get(event_info, "title"));
    if(str == NULL){
        printf("[%s:%d] json_event title not found\n", __func__, __LINE__);
        goto err;
    }
    snprintf(p_obj->title, sizeof(p_obj->title), "%s", str);
	//'\n' remove
	replace_carriage_return(p_obj->title);

    //description
    str = json_string_value(json_object_get(event_info, "description"));
    if(str == NULL){
        printf("[%s:%d] json_event description not found\n", __func__, __LINE__);
        goto err;
    }
    snprintf(p_obj->description, sizeof(p_obj->description), "%s", str);
	//'\n' remove
	replace_carriage_return(p_obj->description);


        //trigger_info(bbox, zone_info)
    trigger_info = json_object_get(event, "trigger_info");

    bbox = json_object_get(
                json_array_get(
                    json_object_get(trigger_info, "object_list"), 
                    0), 
               "bbox");
    //if(bbox == NULL) goto err;
    if(bbox){
        p_obj->event_area[0].x = json_real_value(json_object_get(bbox, "x0"));
        p_obj->event_area[0].y = json_real_value(json_object_get(bbox, "y0"));
        p_obj->event_area[1].x = json_real_value(json_object_get(bbox, "x1"));
        p_obj->event_area[1].y = json_real_value(json_object_get(bbox, "y1"));
    }

    //direction = json_object_get(event, "direction");
    //trigger_type
    snprintf(p_obj->trigger_type, sizeof(p_obj->trigger_type), "%s", str_null_to_blank(json_string_value(json_object_get(trigger_info, "trigger_type"))));
	//'\n' remove
	replace_carriage_return(p_obj->trigger_type);

    //trigger_name
    snprintf(p_obj->trigger_name, sizeof(p_obj->trigger_name), "%s", str_null_to_blank(json_string_value(json_object_get(trigger_info, "trigger_name"))));
	//'\n' remove
	replace_carriage_return(p_obj->trigger_name);

    //trigger zone
    zone_info_list = json_object_get(trigger_info, "zone");

    p_obj->trigger_zone_count = json_array_size(zone_info_list);
    if(p_obj->trigger_zone_count > 0){
        p_obj->trigger_zone_list = malloc(sizeof(VA_POINT) * ((p_obj->trigger_zone_count) + 2));
        if(p_obj->trigger_zone_list){
            memset(p_obj->trigger_zone_list, 0, (sizeof(VA_POINT) * ((p_obj->trigger_zone_count) + 2)));

            json_array_foreach(zone_info_list, i, value){
                p_obj->trigger_zone_list[i].x = json_real_value(json_object_get(value, "x"));
                p_obj->trigger_zone_list[i].y = json_real_value(json_object_get(value, "y"));
            }

            if(!json_is_true(json_object_get(trigger_info, "is_line"))){
                p_obj->trigger_zone_list[i].x = p_obj->trigger_zone_list[0].x;
                p_obj->trigger_zone_list[i].y = p_obj->trigger_zone_list[0].y;
                (p_obj->trigger_zone_count)++;
            }
        }
    }

    ret = 1;
endl:
    *p_ret_gnr_evt_data = p_obj;
    return ret;
err:
    if(event){
        char *debug;
        debug = json_dumps(event, JSON_ENCODE_ANY);
#if IPCAM_VA_OPER_DEBUG        
        printf("[%s:%d] ERROR event[%s]\n", __func__, __LINE__, debug);
#endif        
        if(debug) free(debug);
    }else{
#if IPCAM_VA_OPER_DEBUG        
        printf("[%s:%d] ERROR event object error\n", __func__, __LINE__);
#endif        
    }

    if(p_obj){
        if(p_obj->trigger_zone_list != NULL) free(p_obj->trigger_zone_list);
        free(p_obj);
    }
    p_obj = NULL;

    ret = 0;
    goto endl;
}

static int _parse_metadata(json_t *metadata, VA_EVT_DATA **p_ret_evt_data)
{
    int ret = 0;
    int i = 0, j = 0;
    char *key = NULL;
    json_t* annotations = NULL;
    VA_EVT_DATA *p_obj = NULL;

    //arg check
    if(metadata == NULL || p_ret_evt_data == NULL) return 0;

    //malloc
    p_obj = malloc(sizeof(VA_EVT_DATA));
    if(p_obj == NULL) return 0;
    // g_message("###yanggungg : %s, %d VA_EVT_DATA malloc success : %p", __func__, __LINE__, p_obj);
    memset(p_obj, 0, sizeof(VA_EVT_DATA));

    annotations = json_object_get(metadata, "annotations");
    if(annotations == NULL) goto err;

    p_obj->meta_count = json_array_size(annotations);
#if IPCAM_VA_OPER_DEBUG
    printf("\e[31m [%s][%d] obj event count (%d) \e[0m\n", __func__, __LINE__, p_obj->meta_count);
#endif

    if(p_obj->meta_count == 0) goto err;

    for(i = 0; i < p_obj->meta_count; i++)
    {
        if(i >= VA_META_DATA_MAX)
        {
            printf("\e[33m [%s][%d] recv_meta_count MAX over.. \e[0m\n", __func__, __LINE__);
			p_obj->meta_count = i;
            break;
        }

        json_t *tmp_id = NULL;
        tmp_id = nf_ipcam_get_node_find_str(json_array_get(annotations, i), "track_id");

        if(tmp_id != NULL)
            p_obj->meta_data[i].id = json_integer_value(tmp_id);
        else
            p_obj->meta_data[i].id = -1;

#if IPCAM_VA_OPER_DEBUG
        printf("\e[33m track_id(%d) \e[0m\n", p_obj->meta_data[i].id);
#endif

        //class
        key = nf_ipcam_json_get_string_value(nf_ipcam_get_node_find_str(json_array_get(annotations, i), "class"));
        snprintf(p_obj->meta_data[i].class, 64, key);
#if IPCAM_VA_OPER_DEBUG
        printf("\e[33m class(%s) \e[0m\n", p_obj->meta_data[i].class);
#endif

        //score
        p_obj->meta_data[i].confidence = json_real_value(nf_ipcam_get_node_find_str(json_array_get(annotations, i), "score"));
#if IPCAM_VA_OPER_DEBUG
        printf("\e[33m score(%f) \e[0m\n", p_obj->meta_data[i].confidence);
#endif

        //bbox
        int bbox_size;
        json_t* child_node = NULL;

        bbox_size = json_array_size(nf_ipcam_get_node_find_str(json_array_get(annotations, i), "bbox"));
#if IPCAM_VA_OPER_DEBUG
        printf("\e[95m bbox_size(%d) \e[0m\n", bbox_size);
#endif

        for(j = 0; j < bbox_size; j ++)
        {
            child_node = json_array_get(nf_ipcam_get_node_find_str(json_array_get(annotations, i), "bbox"), j);
            p_obj->meta_data[i].bbx_position[j] = json_real_value(child_node); 
#if IPCAM_VA_OPER_DEBUG
            printf("\e[94m[%s][%d] bbx_position[%d] = (%f) \e[0m\n", __func__, __LINE__, i,  p_obj->meta_data[i].bbx_position[j]);
#endif
        }

        //face data parsing
        p_obj->meta_data[i].is_face_data = FALSE;
        tmp_id = NULL;
        /*
        tmp_id = nf_ipcam_get_node_find_str(json_array_get(annotations, i), "face");
        if(tmp_id != NULL)
        {
            p_obj->meta_data[i].is_face_data = TRUE;
            int group_array_cnt = 0;
            const char *parents_key;
            json_t *p_node;
            const char *child_key;
            json_t *c_node;
            char temp_string[256] = {0, };
            const char *tmp_str = NULL;

            json_object_foreach(tmp_id, parents_key, p_node)
            {

                if(strstr(parents_key, "name"))
                {
                    tmp_str = nf_ipcam_json_get_string_value(p_node);
                    snprintf(p_obj->meta_data[i].face_data.name, 32, tmp_str);
                }

                if(strstr(parents_key, "groups"))
                {
                    group_array_cnt = json_array_size(p_node);
                    p_obj->meta_data[i].face_data.group_cnt = group_array_cnt;

                    for(j = 0; j < group_array_cnt; j++)
                    {
                        c_node = json_array_get(p_node, j);
                        strcpy(p_obj->meta_data[i].face_data.group_name[j], nf_ipcam_json_get_string_value(c_node));
                    }
                }

                if(strstr(parents_key, "attribute"))
                {
                    json_object_foreach(p_node, child_key, c_node)
                    {
                        if(strstr(child_key, "age"))
                        {
                            p_obj->meta_data[i].face_data.age = nf_ipcam_json_get_integer_value(c_node); 
                        }
                        else if(strstr(child_key, "gender"))
                        {
                            char *test = NULL;
                            test = nf_ipcam_json_get_string_value(c_node);
                            strcpy(p_obj->meta_data[i].face_data.gender, test);
                        }
                        else if(strstr(child_key, "headwear"))
                        {
                            char *test = NULL;
                            test = nf_ipcam_json_get_string_value(c_node);
                            strcpy(p_obj->meta_data[i].face_data.headwear, test);
                        }
                        else if(strstr(child_key, "glasses"))
                        {
                            char *test = NULL;
                            test = nf_ipcam_json_get_string_value(c_node);
                            strcpy(p_obj->meta_data[i].face_data.glasses, test);
                        }
                    }
                }
            }

#if IPCAM_VA_OPER_DEBUG
            //face data debug
            printf("\e[33m [%s][%d] face_data name(%s) \e[0m\n",__func__, __LINE__, p_obj->meta_data[i].face_data.name);
            for(j = 0; j < p_obj->meta_data[i].face_data.group_cnt; j++)
                printf("\e[33m [%s][%d] face_data group(%s) \e[0m\n",__func__, __LINE__, p_obj->meta_data[i].face_data.group_name[j]);
            printf("\e[33m [%s][%d] face_data gender(%s) \e[0m\n",__func__, __LINE__, p_obj->meta_data[i].face_data.gender);
            printf("\e[33m [%s][%d] face_data headwear(%s) \e[0m\n",__func__, __LINE__, p_obj->meta_data[i].face_data.headwear);
            printf("\e[33m [%s][%d] face_data glasses(%s) \e[0m\n",__func__, __LINE__, p_obj->meta_data[i].face_data.glasses);
#endif
        }
    */
    }
    ret = 1;
endl:
    *p_ret_evt_data = p_obj;
    return ret;
err:
    if(p_obj){
        free(p_obj);
    }
    p_obj = NULL;
    ret = 0;
    goto endl;
}

int is_cam_ip(const char *rtsp_addr)
{
    int i;
    char ip_buffer[200] = {0, };
	gchar key[128];

    if(nf_api_get_host_from_rtsp_url(rtsp_addr, ip_buffer) == NULL) return -1;
    if(strlen(ip_buffer) <= 0) return -1;

    for(i = 0; i < NUM_ACTIVE_CH; i++){
        //camera ip, vcam ip
        snprintf(key, 128, "cam.logininfo.L%d.hostname", i);
        if(strncmp(ip_buffer, nf_sysdb_get_str_nocopy(key), strlen(ip_buffer)) == 0){
            return i;
        }
    }

    return -1;
}

int parse_va_eventdata_information_parse_vsource(const char *vsource, int *ch, int *resol_width, int *resol_height)
{
	mtable* runtime = get_runtime();
    int channel = -1;
    char *main_str = "main";
    char *second_str = "second";
    uint64_t current_resol;
    int stream_no = 0;
    char key_value[64];
    const char *rtsp_addr = NULL;
    const char *key = NULL;
    
    if(vsource == NULL) return -1;
	if(ch == NULL) return -1;

	if(*ch < 0)
	{
		rtsp_addr = strstr(vsource, "rtsp://");
		if(rtsp_addr != NULL)
		{
			if((channel = is_cam_ip(rtsp_addr)) >= 0){
		}else if(nf_api_is_nvr_stream(rtsp_addr, NULL)){
				//AI BOX PARS SECTION
				//find channel :rtsp addr (current itx addr fix)
				key = strstr(rtsp_addr, main_str);
				if(key == NULL)
				{
					key = strstr(rtsp_addr, second_str);
					key += strlen(second_str);
					stream_no = 1;
				}
				else
				{
					key = strstr(rtsp_addr, main_str);
					key += strlen(main_str);
				}
				channel = atoi(key);
			}else{
#if IPCAM_VA_OPER_DEBUG                
				printf("[%s:%d] stream url not matched rtsp_addr[%s]\n", __func__, __LINE__, rtsp_addr);
#endif                
				return -1;
			}
		}

		*ch = channel;
	}
	else
	{
		channel = *ch;
	}

    if(channel >= 0 && resol_width && resol_height){
        current_resol = runtime[channel].video.resolution.resolution[stream_no];
        if(!(nf_ipcam_change_ipcamres_to_size(current_resol, resol_width, resol_height) == IPCAM_SETUP_RTN_DONE))
            return -1;
    }

#if IPCAM_VA_OPER_DEBUG
    printf("\e[31m [%s][%d] ch (%d) stream_no(%d) width(%d) height(%d) \e[0m\n", __func__, __LINE__, channel, stream_no, resol_width, resol_height);
#endif

    //nf_api_aibox_conn_time_update(channel, 0);
    
    return 1;
}

int is_zmq_running()
{
    return (operator.is_run == 1);
}

int aibox_zmq_recovery_process()
{
    static time_t old = 0;
    time_t now = time(NULL);
    int i;
    //aibox_zmq_connection_info aibox_list;
    //aibox_channel_list setting
    
    //time check
    //current time setting

    old = now;
    return 0;
    for(i = 0; i < IPCAM_MAX_CHAN; i++){
        if(aibox_list[i].ip == 0) continue;

        if((int)(aibox_list[i].time - now) > 5){
            //send_aibox_command(ip, PING);
        }else if((int)(aibox_list[i].time - now) > 30){
            //nf_notify_fire_params("aibox_connection_loss", ...);
            //aibox_loss status mask change
        }
    }
}

int nf_ipcam_vabox_data_operator_start()
{
	int rtn;

	operator.is_run = 1;
	operator.vabox_data_queue = g_async_queue_new();
	operator.vabox_event_data_queue = g_async_queue_new();

	operator.thread_id = g_thread_create((GThreadFunc)vabox_metadata_parser_thread_func, NULL, TRUE, NULL);

	if (operator.thread_id == 0)
	{
		printf("[%s] g_thread_create is failed\n", __FUNCTION__);
		return IPCAM_SETUP_RTN_FAILED;
	}


    worker_manager = create_zmq_worker_pool(2);
    event_manager = create_zmq_worker_pool(1);

    init_zmq_aibox_list();

    //aibox support check
    init_ai_support_status();


#if IPCAM_VA_OPER_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

void nf_ipcam_vabox_data_operator_stop()
{


    if(worker_manager){
        ZMQ_WORKER_POOL_MANAGER *manager = worker_manager;
        worker_manager = NULL;
        destroy_worker_pool(manager);
    }

    if(event_manager){
        ZMQ_WORKER_POOL_MANAGER *manager = event_manager;
        event_manager = NULL;
        destroy_worker_pool(manager);
    }

	if(operator.is_run)
	{
		operator.is_run = 0;
		g_thread_join(operator.thread_id);

		while(g_async_queue_length(operator.vabox_data_queue) > 0)
		{
#if IPCAM_VA_OPER_DEBUG
			printf("\e[33m [%s][%d] vabox data queue_length(%d) \e[0m\n", __func__, __LINE__, 
					g_async_queue_length(operator.vabox_data_queue));
#endif

			void *data = NULL;
			data = g_async_queue_try_pop(operator.vabox_data_queue);

			if(data == NULL)
			{
				g_usleep(5*1000);
				continue;
			}
			//g_object_unref
			free(data);
		}
		g_async_queue_unref(operator.vabox_data_queue);

		while(g_async_queue_length(operator.vabox_event_data_queue) > 0)
		{
#if IPCAM_VA_OPER_DEBUG
			printf("\e[33m [%s][%d] vabox event data queue_length(%d) \e[0m\n", __func__, __LINE__, 
					g_async_queue_length(operator.vabox_event_data_queue));
#endif

			void *data = NULL;
			data = g_async_queue_try_pop(operator.vabox_event_data_queue);

			if(data == NULL)
			{
				g_usleep(5*1000);
				continue;
			}
			//g_object_unref
			free(data);
		}
		g_async_queue_unref(operator.vabox_event_data_queue);
	}

#if IPCAM_VA_OPER_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
}

VA_EVT_DATA* nf_ipcam_get_vabox_popped_data()
{
	if(operator.is_run)
	{
		if(g_async_queue_length(operator.vabox_data_queue) > 0)
		{
            // printf("\e[33m###yanggungg : %s, %d meta queue length[%d]\e[0m\n", __func__, __LINE__, g_async_queue_length(operator.vabox_data_queue));
			return ((VA_EVT_DATA*)g_async_queue_try_pop(operator.vabox_data_queue));
		}
	}
	return NULL; 
}

VA_GNR_EVT_DATA* nf_ipcam_get_vabox_popped_event_data()
{
	if(operator.is_run)
	{
		if(g_async_queue_length(operator.vabox_event_data_queue) > 0)
		{
            // printf("\e[33m###yanggungg : %s, %d event queue length[%d]\e[0m\n", __func__, __LINE__, g_async_queue_length(operator.vabox_event_data_queue));
			return ((VA_GNR_EVT_DATA*)g_async_queue_try_pop(operator.vabox_event_data_queue));
		}
	}
	return NULL; 
}


GAsyncQueue * get_vabox_data_queue()
{
	return operator.vabox_data_queue; 
}

GAsyncQueue * get_vabox_event_data_queue()
{
	return operator.vabox_event_data_queue; 
}

int nf_ipcam_parse_vabox_metadata_stream(int ch, char* p_text)
{
	char va_version[32];
	char *base64_str = NULL;
	char *f_version = "DLVA_V";
	char *f_data = "Value=\"";
	char *s_str = NULL;
	char *e_str = NULL;
	int decoded_len = 0;

	if(p_text == NULL)
		return IPCAM_SETUP_RTN_FAILED; 

	s_str = strstr(p_text, f_version);
	if(!s_str){
#if IPCAM_VA_OPER_DEBUG 
        printf("[%s:%d] [%s] not found text[%s]\n", __func__, __LINE__, f_version, p_text);
#endif
		return IPCAM_SETUP_RTN_FAILED; 
    }
	s_str += strlen(f_version);
	e_str = strstr(s_str, "\"");
	if(!e_str) {
#if IPCAM_VA_OPER_DEBUG
        printf("[%s:%d] [%s] end quote not found text[%s]\n", __func__, __LINE__, f_version, p_text);
#endif
        return IPCAM_SETUP_RTN_FAILED; 
    }

	memset(va_version, 0x00, 32);
	memcpy(va_version, s_str, e_str-s_str);
#if IPCAM_VA_OPER_DEBUG 
	printf("\e[33m [%s][%d] [%s] version (%s) \e[0m\n", __func__, __LINE__, f_version, va_version);
#endif

	s_str = strstr(s_str, f_data);
	if(s_str == NULL)
		return IPCAM_SETUP_RTN_FAILED; 

	s_str += strlen(f_data);
	e_str = strstr(s_str, "\"");
	if(e_str == NULL)
		return IPCAM_SETUP_RTN_FAILED; 

#if IPCAM_VA_OPER_DEBUG 
	printf("\e[95m [%s][%d] base64 encoding data strlen(%d) \e[0m\n", __func__, __LINE__, e_str-s_str);	
#endif
    GAsyncQueue *msg_queue = get_zmq_msg_queue();
    if (msg_queue == NULL)
    {
        printf("[%s:%d] msg_queue is NULL\n", __func__, __LINE__);
        return IPCAM_SETUP_RTN_FAILED;
    }

    int msg_queue_length = g_async_queue_length(msg_queue);
    if (msg_queue_length >= IPCAM_VA_MSG_QUEUE_MAX)
    {
        // printf("\e[31m[%s, %d] msg_queue_length is over max (%d)\e[0m\n", __FUNCTION__, __LINE__, msg_queue_length);
        return IPCAM_SETUP_RTN_FAILED;
    }

	int strlen = (e_str - s_str) + 1;
	char *src_str = malloc(strlen);
    // g_message("###yanggungg : %s, %d src_str malloc : %p", __func__, __LINE__, src_str);
	char *dst_str = malloc(strlen+4);
    // g_message("###yanggungg : %s, %d dst_str malloc : %p", __func__, __LINE__, dst_str);
	memset(src_str, 0x00, strlen);
	memset(dst_str, 0x00, strlen+4);
	memcpy(src_str, s_str, strlen - 1);

	*(int *)dst_str = ch;

	decoded_len = _nf_ipcam_base64_decode(src_str, strlen - 1, dst_str+4); 
	// printf("\e[31m [%s][%d] decoded_len[%d] decodeded_str :\n(%s) \e[0m\n", __func__, __LINE__, decoded_len, dst_str+4);

	g_async_queue_push(msg_queue, (gpointer)dst_str);

	if(src_str) {
        // g_message("###yanggungg : %s, %d src_str free : %p", __func__, __LINE__, src_str);
		free(src_str);
    }

	return IPCAM_SETUP_RTN_DONE;
}

static int _nf_ipcam_base64_decode(char *p_text, int numBytes, char *decodedText)
{

	static int DecodeMimeBase64[256] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 00-0F 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 10-1F 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  // 20-2F 
		52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  // 30-3F 
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  // 40-4F 
		15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  // 50-5F 
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  // 60-6F 
		41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  // 70-7F 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 80-8F 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // 90-9F 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // A0-AF 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // B0-BF 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // C0-CF 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // D0-DF 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  // E0-EF 
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   // F0-FF 
	};

	const char* cp;
	int decoded_len = 0; 
	int phase = 0;
	int d, prev_d = 0;
	unsigned char c;
	
	for ( cp = p_text; *cp != '\0'; ++cp ) {
		d = DecodeMimeBase64[(int) *cp];
		if ( d != -1 ) {
			switch ( phase ) {
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( decoded_len < numBytes )
						decodedText[decoded_len++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( decoded_len < numBytes )
						decodedText[decoded_len++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( decoded_len < numBytes )
						decodedText[decoded_len++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	
	return decoded_len;
}

static void vadata_processtime_update_thread_func ()
{
	int i = 0;
	static unsigned int loop_cnt = 0;

	while(updator.is_run)
	{
		for(i = 0 ; i < IPCAM_MAX_CHAN ; i++)
		{
			if(!memcmp(&cur_timeinfo[i], &past_timeinfo[i], sizeof(LIVE_TIME_INFO)))
			{
				if(!g_mutex_trylock(updator.mutex))
					continue;

				memset(&cur_timeinfo[i], 0x00, sizeof(LIVE_TIME_INFO));
				memset(&past_timeinfo[i], 0x00, sizeof(LIVE_TIME_INFO));
				memset(&g_live_buffering_time[i], 0x00, sizeof(unsigned int));

				g_mutex_unlock(updator.mutex);

				continue;
			}

			if(cur_timeinfo[i].summed_cnt == 0)
				continue;

			if(!g_mutex_trylock(updator.mutex))
				continue;

			memcpy(&past_timeinfo[i], &cur_timeinfo[i], sizeof(LIVE_TIME_INFO));
			g_live_buffering_time[i] = (cur_timeinfo[i].summed_time / cur_timeinfo[i].summed_cnt);
			//g_live_buffering_time[i] += ASPECT_NETWORK_BUFF;

			if((loop_cnt % 5) == 0)
				printf("[[%s][%d] update buffering average CH(%d) time(%d) f_count(%d)\n",__func__, __LINE__, i, g_live_buffering_time[i], cur_timeinfo[i].summed_cnt);

			memset(&cur_timeinfo[i], 0x00, sizeof(LIVE_TIME_INFO));
			memset(&past_timeinfo[i], 0x00, sizeof(LIVE_TIME_INFO));

			//if((g_live_buffering_time[i] / MRTP_SYNC_RATE) <= BUFFERING_TIME_LIMIT)

		
			g_live_buffering_time[i] = BUFFERING_TIME_DEFAULT;
			
			if((g_live_buffering_time[i] ) <= BUFFERING_TIME_LIMIT)
			{
				nmf_mrtp_pipe_update_live_time(_h_mrtp_pipe, i, (g_live_buffering_time[i]));
				//nmf_mrtp_pipe_update_live_time(_h_mrtp_pipe, i, (g_live_buffering_time[i]/MRTP_SYNC_RATE));
				//printf("\e[95m  call this func (%s) ch(%d) buff time(%d)  \e[0m\n", __func__, i, g_live_buffering_time[i]);
			}
			else
				g_live_buffering_time[i] = BUFFERING_TIME_DEFAULT;


			g_mutex_unlock(updator.mutex);
		}

		loop_cnt ++;
		sleep(BUFFERING_UPDATE_TIME);
	}
}

static void replace_carriage_return(char *str)
{
	if(str == NULL)
		return;

	for (; *str != '\0'; str++)
	{
		if (*str == '\n')
		{
			strcpy(str, str + 1);
			str--;
		}
	}
}

int nf_ipcam_vabox_time_updator_start()
{
	int rtn;

	if(updator.mutex == NULL)
		updator.mutex = g_mutex_new();

	updator.is_run = 1;
	updator.thread_id = g_thread_create((GThreadFunc)vadata_processtime_update_thread_func, NULL, TRUE, NULL);

	if (updator.thread_id == 0)
	{
		printf("[%s] g_thread_create is failed\n", __FUNCTION__);
		return IPCAM_SETUP_RTN_FAILED;
	}

	_h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();
#if IPCAM_VA_OPER_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
	return IPCAM_SETUP_RTN_DONE;
}

void nf_ipcam_vabox_time_updator_stop()
{
	if(updator.is_run)
	{
		updator.is_run = 0;
		g_thread_join(updator.thread_id);
	}

#if IPCAM_VA_OPER_DEBUG
	printf("\e[33m [%s][%d] success \e[0m\n",__func__, __LINE__);
#endif
}

