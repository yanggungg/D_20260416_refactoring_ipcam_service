#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nf_api_openmode.h>
#include <nf_ipcam_defs.h>
#include <arpa/inet.h>
#include <glib.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>

#include <linux/if.h>
#include "proxy_cli.h"

#include "nf_api_http.h"
#include "nf_ipcam_utils.h"
#include "nf_api_dlva.h"
#include "jbshell.h"
#include "nfdal.h"
#include "nf_api_ipcam.h"

#define _ADMIN_SVR_PORT					(32679)
#define _ADMIN_CLI_PORT					(32678)

#ifdef DUAL_LAN_NETWORK
	#define ETH_LINKLOCAL "br0:0"
#else
	#define ETH_LINKLOCAL "eth0:0"
#endif

extern int get_openmode_state();
extern void set_openmode_state(int state);
extern void set_discovery_state(int discovery);
extern int get_discovery_state();
extern void nf_openmode_scan_aibox(int sec);
extern gint nf_get_running_mode(void);

extern char *http_strdup(const char *str);


/* for camera api start */

extern const char *tok_iterator_init_(Token_iterator *iter, const char *str, const char *token);

extern int Base64decode_len(const char *bufcoded);
extern int Base64decode(char *bufplain, const char *bufcoded);
extern int Base64encode_len(int len);
extern int Base64encode(char *encoded, const char *string, int len);

#define camres_foreach(iter, string, key, value) \
    for(tok_iterator_init_(&(iter), string, "\r\n&"); (iter).current != NULL && get_key_value((iter).current, &(key), &(value)) > 0; tok_get_next(&(iter)))

/* for camera api end */

typedef struct _aibox_ch_info
{
	int no;
	char name[20];	//name owner
	char url[100];
	unsigned int aibox_ip;
	char ai_algorithm_str[50];//value
	char text[100];
    char algo_type[50];
	char owner[20];

	char mac[8];

	//parse data
	int ch;
	char ip[20];
	char id[50];
	char passwd[50]; 
	int rtsp_port;
}aibox_ch_info;

//trigger
typedef struct sequrinet_event_action
{
    int id;
    int channel;
    int duration;
    int port;
    int proto_checker;
    int recorder_type;

    int is_msg;
    int is_panic;
    int is_ssl;

    char name[100];
	char host[100];
	char authuser[100];
    char authpass[100];
    char caption_msg[100];
	char desc_msg[100];
	char title_msg[100];
    char msg[100];
}sequrinet_event_action;

typedef struct fr_trigger_info
{
	int id;

    char name[250];

	int aibox_ch;
	int nvr_ch;

    char gtype[100];  //attr comparison unregi

    //Facial Attribute Filtering
	int age_max;    //0 ~ 100
    int age_min;    //0 ~ 100
    char gender[100];     //all male female

	//Comparison
	int search_thr; //Min Similarity

    //Comparison & Unregistered
    int liveness;               //Liveliness Detection true or false
    int liveness_thr;           //Liveliness Threshold

    DLVA_POINT zone[MAX_AIBOX_DB_ZONE_SIZE];
    size_t zone_size;

	char group_list[20][250];
    size_t group_size;
}fr_trigger_info;

typedef struct lpr_trigger_info
{
    int id;

    char name[250];
	char rmode[250];
	char policy[250];

	int aibox_ch;
	int nvr_ch;

	DLVA_POINT zone[MAX_AIBOX_DB_ZONE_SIZE];
    size_t zone_size;

	char group_list[20][250];
	size_t group_size;
    //zone info
}lpr_trigger_info;

typedef struct sequrinet_action_rule
{
    int id;
    int vsource;

    char name[100];
	char trigger_name[100];
    char trigger_type[100];
}sequrinet_action_rule;

static void _g_list_free_full (GList *list, GDestroyNotify free_func)
{
  g_list_foreach (list, (GFunc) free_func, NULL);
  g_list_free (list);
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

char *_ip_to_str(unsigned int ip, char *buf)
{
	static char ret[20];
	if(buf == NULL){
		buf = ret;
	}
	snprintf(buf, 16, "%d.%d.%d.%d",
			(ip&0xff000000)>>24,
			(ip&0xff0000)>>16,
			(ip&0xff00)>>8,
			(ip&0xff));
	return buf;
}

unsigned int _netmask_num_to_addr(int num)
{
	unsigned int ret = 0xffffffff;
	ret >>= num;
	return ~ret;
}

int _http_json(char *ipstr, char *path, json_t *json);
int _http_get(char *ipstr, char *path, char *query, char *buffer, int buffer_size);

char *_get_ch_stream_uri(int ch, unsigned int host_ip, char *buf, int high_quality);
char *_get_zmq_uri(char *buf, unsigned int host_ip);
int _send_aicamera_data(unsigned int ch, const char *algorithm);
unsigned int _get_nvr_host_ip(unsigned int ip);

static char *_get_eth_interafce(unsigned char *p_macaddr, unsigned int ip_addr, char *eth_str);
static const char *str_null_to_blank(const char *str);
static int _get_aibox_owner(unsigned int aibox_ip, char *owner, char *date);
static char *_get_aibox_hash(char *body, char *date);
static int _delete_owner(unsigned int aibox_ip, char *date);
static int set_aibox_owner(unsigned int aibox_ip, const char *owner);
static int set_zmq_uri(unsigned int aibox_ip, const char *owner, const char *zmq_uri, char *date);
//static ai_capa_t *_get_aibox_capability(unsigned int aibox_ip, int ch, char *date);
static int _get_aicam_capability(int ch, ai_capa_t **capa);
static int _get_aibox_capability(unsigned int aibox_ip, int ch, char *date, ai_capa_t **capa);
static int _get_aibox_capabilities(unsigned int aibox_ip, char *date, ai_capa_t **capa);

static void _send_aibox_search(void);
static void _init_aibox_socks(void);
static void _close_socks();
static int _get_chinfo_from_aibox_rtsp_url(char *url);
static aibox_search_list *_api_aibox_search_list(aibox_search_list *aibox_list, int timeout);

static void _aibox_info_queue_push(aibox_connection_info *data);
static unsigned int _aibox_list_polling(aibox_search_list *aibox_list, int timeout);
static unsigned int _aibox_ip_polling(int timeout);

static void _host_inf_free(host_inf *list);
static host_inf *_get_host_inf_list(void);
static host_inf *_get_host_inf_from_ip(host_inf *list, unsigned int ip);
static char *get_host_from_url(char *url, char *buf);
static int is_host_ip(host_inf *list, char *ip);

static const char *_get_algorithm_text(char *key, ai_capa_t *capa);
static aibox_algorithm_info *_get_algorithm_info(char *key, ai_capa_t *capa);

static unsigned int get_db_aibox_ip(int ch);
//static json_t *_get_aibox_rule_data(unsigned int aibox_ip);
int _get_aibox_rule_data(unsigned int aibox_ip, json_t **json_ret);

static host_inf *get_host_info_list();
static int get_group_id_aibox_lpr(unsigned int aibox_ip, const char *group_name);
static AIBOX_RULE_TYPE_E _get_aibox_type(const char *string);

int nf_api_aibox_update_rules(unsigned int aibox_ip, unsigned int ch_mask);

int get_mac_from_ip(unsigned char *mac, unsigned int ip);

void aibox_connection_recovery(void *data);
void aibox_cb_work_push(void *func, void *data);
int _discovery_aibox(netconf_msg* received, struct sockaddr_in* cin);
int _is_aibox(netconf_msg* received);

static int get_subnet_host_ip(host_inf *list, unsigned int ip, int is_wan);
static int _is_aibox_connected_wan(aibox_connection_info *conn);
static int set_aibox_zmq_uri(const char *nvr_owner, unsigned int aibox_ip, unsigned int host_ip);
static int is_ip_wan(unsigned int ip);
static int is_mac_wan(const unsigned char *mac);
static unsigned int get_ip_from_zmq_uri(const char *buf_uri);

static int api_aibox_fr_modify(unsigned int aibox_ip, face_info *info);

unsigned int aibox_hub_ipset(aibox_connection_info info);

int aibox_fr_get(unsigned int aibox_ip, int fr_id, face_info *info);

static int http_connection_timeout = 3;
static int http_timeout	= 5;

//queue
GAsyncQueue		*_aibox_search_queue = NULL;
static pthread_mutex_t _search_mutex = PTHREAD_MUTEX_INITIALIZER;

//host inf
static host_inf *host_inf_list = NULL;
static pthread_mutex_t _host_inf_mutex = PTHREAD_MUTEX_INITIALIZER;

static void aibox_ch_info_free(aibox_ch_info *info);

static int set_json_to_lpr_rule(unsigned int aibox_ip, lpr_rule *info, json_t *json);
int aibox_get_fr_trigger(unsigned int aibox_ip, unsigned int trigger_id, const char *trigger_name, fr_trigger_info *info);
static int aibox_modify_fr_trigger(unsigned int aibox_ip, fr_trigger_info *info);
static int set_json_to_fr_trigger(fr_trigger_info *info, json_t *json);
static int get_fr_group(unsigned int aibox_ip, int group_id, fr_group_info *info);
static int aibox_add_fr_trigger(unsigned int aibox_ip, fr_trigger_info *info);
static aibox_ch_info *stream_list_parser(json_t *json);
static int _aibox_get_empty_ch(unsigned int aibox_ip, int ch);

int get_aibox_ch(unsigned int aibox_ip)
{
	GValue ret_value = {0,};
    int ch;
    unsigned int ch_aibox_ip;

    for(ch = 0; ch < AVAILABLE_MAX_CH; ch++){
        if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
        {
            ch_aibox_ip = g_value_get_uint(&ret_value);
            g_value_unset(&ret_value);
            
            if(ch_aibox_ip == aibox_ip) break;
        }
    }

    //printf("[%s:%d] ch[%d] ch_ip[%08x] ip[%08x]\n", __func__, __LINE__, ch, ch_aibox_ip, aibox_ip);

    if(ch == AVAILABLE_MAX_CH)
    {
        return -1;
    }

    return ch;
}

static int _get_aibox_db_ch(unsigned int ip)
{
		//return channel
		GValue ret_value = {0,};
		int ch;
		unsigned int db_ip;
		
		if(ip == 0){
		    return -1;
		}
		    	                            
		for(ch = 0; ch < AVAILABLE_MAX_CH; ch++)
		{
		    //get db ip
			if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
		    {
		    	db_ip = g_value_get_uint(&ret_value);
		    	g_value_unset(&ret_value);
		    }
			else
			{
		    	db_ip = 0;
		    }
		
			if(db_ip == ip){
		    	break;
		    }
		}
		
		if(ch == AVAILABLE_MAX_CH)
		{
		    ch = -1;
		}
		
		return ch;
}

static int _get_aicam_db_ch(unsigned int ip)
{
	//
	//return channel
	
	GValue ret_value = {0,};
	mtable *runtime = get_runtime();

	int ch;

	if(ip == 0){
		return -1;
	}

	for(ch = 0; ch < AVAILABLE_MAX_CH; ch++)
	{
		if(runtime[ch].sys.ipaddr == ntohl(ip)){
			//pro model check
			printf("[%s:%d] jhyoo ch[%d] model_type_support[%08x]\n", __func__, __LINE__, ch, runtime[ch].ai.model_type_support);
										
			if(runtime[ch].ai.model_type_value == NF_AI_MODEL_AICAM_PRO){     /*is built in aibox*/
				break;
			}
#if 0
			if(runtime[ch].ai.model_type_support & NF_AI_MODEL_AICAM_PRO){	/*is built in aibox*/
				break;
			}
#endif
		}
	}

	if(ch == AVAILABLE_MAX_CH)
	{
		ch = -1;
	}

	return ch;
}

int nf_api_get_aibox_info(unsigned int ip, int *nvr_ch, int *port, int *use_ssl, char *id, char *pw)
{
	//output port, use_ssl, id, pw
	mtable *runtime = get_runtime();

	int ch;
	GValue ret_value = {0,};

	if((ch = _get_aibox_db_ch(ip)) >= 0){
		//printf("[%s:%d] ip[%s] ch[%d] is_aibox\n", __func__, __LINE__, _ip_to_str(ip, NULL), ch);
		if(port){
			/*
				if (nf_sysdb_get_key1("cam.ai_box.A%d.http_port", ch, &ret_value, NULL))
				{
					*port = g_value_get_uint(&ret_value);
					g_value_unset(&ret_value);
				}
				else
				{
			*/
			*port = 8443;
			//}
		}

	if(use_ssl){
		/*
		if (nf_sysdb_get_key1("cam.ai_box.A%d.use_ssl", ch, &ret_value, NULL))
		{
			*use_ssl = g_value_get_uint(&ret_value);
			g_value_unset(&ret_value);
		}
		else
		{
		*/
		*use_ssl = 1;
		//}
	}
	if(id){
		if (nf_sysdb_get_key1("cam.ai_box.A%d.id", ch, &ret_value, NULL))
		{
			g_stpcpy(id, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			sprintf(id, "ADMIN");
		}
	}

	if(pw){
		if (nf_sysdb_get_key1("cam.ai_box.A%d.pass", ch, &ret_value, NULL))
		{
			g_stpcpy(pw, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			sprintf(pw, "1234");
		}
	}
}else if((ch = _get_aicam_db_ch(ip)) >= 0){
	//printf("[%s:%d] ip[%s] ch[%d] is_aicam\n", __func__, __LINE__, _ip_to_str(ip, NULL), ch);
	if(port){
		*port = runtime[ch].sys.http_port;
	}

	if(use_ssl){
		*use_ssl = runtime[ch].sys.use_ssl;
	}

	if(id){
		if(nf_sysdb_get_key1("cam.logininfo.L%d.id", ch, &ret_value, NULL))
		{
			g_stpcpy(id, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			sprintf(id, "ADMIN");
		}
	}

	if(pw){
		if(nf_sysdb_get_key1("cam.logininfo.L%d.pwd", ch, &ret_value, NULL))
		{
			g_stpcpy(pw, g_value_get_string(&ret_value));
			g_value_unset(&ret_value);
		}
		else
		{
			sprintf(pw, "1234");
		}
	}
	}else{
		printf("[%s:%d] ip[%s] info not found\n", __func__, __LINE__, _ip_to_str(ip, NULL));
		return -1;
	}

	if(nvr_ch){
		*nvr_ch = ch;
	}

	return 0;
}
										                                                                                                

const char *_api_get_aibox_id(int ch, char *buf)
{
    /*
    '<item key="cam.ai_box.A0.id"           type="STRING" min="0" max="128" val="" />'
    */
	GValue ret_value = {0,};
	static char buffer[129];

	if(buf == NULL){
		buf = buffer;
	}

    buf[0] = 0;

    if (nf_sysdb_get_key1("cam.ai_box.A%d.id", ch, &ret_value, NULL))
    {
		g_stpcpy(buf, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
    }
    else
    {
        printf("[%s:%d] get aibox id db failed ch[%d]\n", __func__, __LINE__, ch);
    }
    
    if(buf[0] == NULL){
        strcpy(buf, "ADMIN");
    }

    return buf;
}

const char *_api_get_aibox_pw(int ch, char *buf)
{
    /*
    '<item key="cam.ai_box.A0.pass"         type="STRING" min="0" max="128" val="" />'
    */
	GValue ret_value = {0,};
	static char buffer[129];

	if(buf == NULL){
		buf = buffer;
	}

    buf[0] = 0;

    if (nf_sysdb_get_key1("cam.ai_box.A%d.pass", ch, &ret_value, NULL))
    {
		g_stpcpy(buf, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
    }
    else
    {
        printf("[%s:%d] get aibox pw db failed ch[%d]\n", __func__, __LINE__, ch);
    }

    if(buf[0] == NULL){
        strcpy(buf, "1234");
    }

    //printf("[%s:%d] buf[%s]\n", __func__, __LINE__, buf);

    return buf;
}

void _aibox_queue_destroy_notify(gpointer data)
{
	IPCAM_DBG(MINOR, "called p[%p] value[%d]\n", data, *(int *)data);
	if(data != NULL){
		free(data);
	}
}
static void _aibox_info_queue_push(aibox_connection_info *data)
{
	IPCAM_DBG(MINOR, "data[%p] ip[%s]\n", data, _ip_to_str(data->ip, NULL));
	if(_aibox_search_queue != NULL && data != 0){
		g_async_queue_push(_aibox_search_queue, (void *)data);
	}
}

static unsigned int _aibox_ip_polling(int timeout)
{
	//unsigned int *data = NULL;
	aibox_connection_info *data = NULL;
	unsigned int ip = 0;
	struct timespec now_time, start_time;
	clock_gettime(CLOCK_REALTIME, &start_time);
	while((data  = (aibox_connection_info *)g_async_queue_try_pop(_aibox_search_queue)) == NULL){
		clock_gettime(CLOCK_REALTIME, &now_time);
		if(now_time.tv_sec - start_time.tv_sec >= timeout){
			IPCAM_DBG(MINOR, "timeout\n");
			break;
		}
	}

	if(data != NULL){
		ip = data->ip;
		free(data);
	}

	IPCAM_DBG(MINOR, "ip[%s]\n", _ip_to_str(ip, NULL));
	return ip;
}

static unsigned int _aibox_list_polling_with_ipset(aibox_search_list *aibox_list, int timeout)
{
	//unsigned int *data = NULL;
	aibox_connection_info *data = NULL;
	unsigned int ret = 0;
	struct timespec now_time, start_time;

	clock_gettime(CLOCK_REALTIME, &start_time);
	while(1){
		data  = (aibox_connection_info *)g_async_queue_try_pop(_aibox_search_queue);
		if(data != NULL){
			aibox_connection_info *info = NULL;
			if((info = nf_api_aibox_list_get_from_mac(aibox_list, data->mac)) == NULL){
				IPCAM_DBG(MINOR, "data->ip[%s] mac[%s]\n", _ip_to_str(data->ip, NULL), _mac_to_str(data->mac, NULL));

                //hub check
                data->ip = aibox_hub_ipset(*data);

				pthread_mutex_lock(&aibox_list->_list_mutex);
				aibox_list->list = g_list_append(aibox_list->list, data);
				pthread_mutex_unlock(&aibox_list->_list_mutex);
				ret++;
			}else{
				//IPCAM_DBG(MINOR, "dup ip[%s] mac[%s]\n", _ip_to_str(info->ip, NULL), _mac_to_str(info->mac, NULL));
			}
		}

		clock_gettime(CLOCK_REALTIME, &now_time);
		if(now_time.tv_sec - start_time.tv_sec >= timeout){
            printf("[%s:%d] time[%u]\n", __func__, __LINE__, (now_time.tv_sec - start_time.tv_sec));
			IPCAM_DBG(MINOR, "timeout\n");
			break;
		}
	}

	return ret;
}

static unsigned int _aibox_list_polling(aibox_search_list *aibox_list, int timeout)
{
	//unsigned int *data = NULL;
	aibox_connection_info *data = NULL;
	unsigned int ret = 0;
	struct timespec now_time, start_time;

	clock_gettime(CLOCK_REALTIME, &start_time);
	while(1){
		data  = (aibox_connection_info *)g_async_queue_try_pop(_aibox_search_queue);
		if(data != NULL){
			aibox_connection_info *info = NULL;
			if((info = nf_api_aibox_list_get_from_mac(aibox_list, data->mac)) == NULL){
				IPCAM_DBG(MINOR, "data->ip[%s] mac[%s]\n", _ip_to_str(data->ip, NULL), _mac_to_str(data->mac, NULL));
				pthread_mutex_lock(&aibox_list->_list_mutex);
				aibox_list->list = g_list_append(aibox_list->list, data);
				pthread_mutex_unlock(&aibox_list->_list_mutex);
				ret++;
			}else{
				//IPCAM_DBG(MINOR, "dup ip[%s] mac[%s]\n", _ip_to_str(info->ip, NULL), _mac_to_str(info->mac, NULL));
			}
		}

		clock_gettime(CLOCK_REALTIME, &now_time);
		if(now_time.tv_sec - start_time.tv_sec > timeout){
			IPCAM_DBG(MINOR, "timeout\n");
			break;
		}
	}

	return ret;
}

/*
 *  discovery search
 */


//static int aibox_rcv_sock = (-1);
static int aibox_snd_sock = (-1);
static int aibox_find_mode = 0;
static void _close_socks()
{
	if(aibox_snd_sock > 0){
	   	close(aibox_snd_sock);
		aibox_snd_sock = -1;
	}
}

static int _is_number(const char *str)
{
	for(;*str;str++){
		if(!isdigit(*str)) return 0;
	}
	return 1;
}

static void _init_aibox_socks(void)
{
	int i = 0;
	int on = 1;
	struct sockaddr_in sin;

	IPCAM_DBG(MAJOR, "start\n");
	if (aibox_snd_sock > 0)
	{
		IPCAM_DBG(MINOR, "ITX admin protocol snd sock init skipped\n");
		goto _label_end;
	}
	aibox_snd_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (aibox_snd_sock < 0)
	{
		IPCAM_DBG(ERROR, "send socket init failed\n");
		perror("socket");
		return ;
	}
	struct timeval send_timeout;
	send_timeout.tv_sec = 5;
	send_timeout.tv_usec = 0;
	setsockopt(aibox_snd_sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	setsockopt(aibox_snd_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&send_timeout, sizeof(send_timeout));

	IPCAM_DBG(MINOR, "ITX admin protocol snd setopt done\n");

_label_end:
	IPCAM_DBG(MAJOR, "end\n");
}

extern int get_hub_sock();
static void _send_aibox_search(void)
{
	int len = 0;
	struct sockaddr_in sin;
	netconf_msg netconf;

#ifdef DUAL_LAN_NETWORK
	struct ifreq ifr;
#endif

	IPCAM_DBG(MAJOR, "start\n");
	if (aibox_snd_sock < 0)
	{
		IPCAM_DBG(ERROR, "aibox_snd_sock disabled\n");
		return;
	}

	memset((void*)&netconf, 0x00, sizeof(netconf_msg));
	netconf.version = 1;
	netconf.opcode = MSG_IP_SEARCH;
	netconf.magic = htonl(0x69547843);

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_BROADCAST;
	sin.sin_port = htons(_ADMIN_CLI_PORT);

#ifdef DUAL_LAN_NETWORK
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth0");
	if (setsockopt(aibox_snd_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}
#endif

	len = sendto(aibox_snd_sock, (void*) &netconf, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));

	if (len < 0)
	{
		IPCAM_DBG(ERROR, "broadcast send failed\n");
		perror("sendto");
	}

#ifdef DUAL_LAN_NETWORK
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth1");
	if (setsockopt(aibox_snd_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}

	len = sendto(aibox_snd_sock, (void*) &netconf, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));

	if (len < 0)
	{
		char message[128] = {0};
		IPCAM_DBG(ERROR, "broadcast send eth1 failed\n");
		snprintf(message, 127, "[%s:%d] sendto", __FUNCTION__, __LINE__);
		perror(message);
	}
#endif

	IPCAM_DBG(MAJOR, "end\n");
}


int _discovery_aibox(netconf_msg* received, struct sockaddr_in* cin)
{
	//return OPENMODE_RTN_FAIL;
	int ret;
	unsigned int ip = 0;
	unsigned int linklocal_ip = 0;
	unsigned int cin_ip;
	aibox_connection_info *data = NULL;


	IPCAM_DBG(MINOR, "start\n");

	ip = received->ciaddr;
	linklocal_ip = *((unsigned int *)(received->vend + 33));
	if(cin != NULL){
		    cin_ip = cin->sin_addr.s_addr;
	}	

	if(aibox_find_mode){
		data = (aibox_connection_info *)malloc(sizeof(aibox_connection_info));
		if(data == NULL){
			IPCAM_DBG(MINOR, "malloc[%d] error ip[%08x]\n", sizeof(aibox_connection_info), ip);
			return -1;
		}
		memset(data, 0x00, sizeof(aibox_connection_info));
		data->ip = ntohl(ip);
		data->linklocal_ip = ntohl(linklocal_ip);
		data->cin_ip = ntohl(cin_ip);
		memcpy(data->mac, received->chaddr, 6);
        data->is_dhcp = (received->version == 0x10) ? 1:0;
        data->transaction = received->xid;
		//IPCAM_DBG(MINOR, "push ip[%s] mac[%s]\n", _ip_to_str(data->ip, NULL), _mac_to_str(data->mac, NULL));
		_aibox_info_queue_push(data);

		data->use_ssl = ntohs(received->reserve_port);
		if(data->use_ssl){
			data->web_port = ntohs(received->https_port);
		}else{
			data->web_port = ntohs(received->http_port);
		}

		snprintf(data->model, sizeof(data->model), "%.25s", received->vend+5);
		
	}
	return 0;
}

int _is_aibox(netconf_msg* received)
{
	unsigned int ip;

	IPCAM_DBG(MINOR, "type[%02x] model[%.25s] reserve_port[%d] http_port[%d] https_port[%d]\n", received->vend[2], received->vend+5,
		received->reserve_port,
		received->http_port,
		received->https_port
	);

	if(received->vend[2] == 0x02){
		IPCAM_DBG(MINOR, "AIBOX type checked\n");
		return 1;
	}else if(memcmp(received->vend+5, "AI-BOX", 6) == 0){
		IPCAM_DBG(MINOR, "AIBOX name checked\n");
		return 1;
	}

	return 0;
}

static int _get_id_password_from_url(const char *url, char *id, char *passwd)
{
	if(url == NULL || id == NULL || passwd == NULL) return -1;

	const char *entry = NULL;
	const char *end = NULL;
	int len = 0;
	char *buf = NULL;

	end = strstr(url, "@");

	if(end == NULL) return -1;

	entry = strstr(url, "//");
	if(entry == NULL){
		entry = url;
	}else{
		entry += 2;
	}

	len = end - entry;
	if(len <= 0) return -1;

	buf = malloc(len+1);
	strncpy(buf, entry, len);
	buf[len] = 0;


	sscanf(buf, "%[^:]%*[:]%s", id, passwd);
	free(buf);

	return 0;
}

static int _get_chinfo_from_aibox_rtsp_url(char *url)
{
	int ret = -1;

	char *p;
	if((p = strstr(url, "/live/")) == 0){
		goto _end;
	}
	sscanf(p+5, "%*[^0-9]%d", &ret);
_end:
	return ret;
}


static GList *_aibox_list_append_ip(aibox_search_list *aibox_list, unsigned int ip)
{
	unsigned int *data = NULL;
	GList *list = NULL;
	if(ip == 0){
		goto _err;
	}

	data = (unsigned int *)malloc(sizeof(unsigned int));
	if(data == NULL){
		IPCAM_DBG(MINOR, "malloc[%d] error ip[%08x]\n", sizeof(unsigned int), ip);
		goto _err;
	}

	*data = ip;

	pthread_mutex_lock(&aibox_list->_list_mutex);
	list = g_list_append(aibox_list->list, (gpointer)data);
	if(aibox_list->list == NULL){
		aibox_list->list = list;
	}
	pthread_mutex_unlock(&aibox_list->_list_mutex);

	return aibox_list->list;
_err:
	if(data != NULL) free(data);
	return NULL;
}

static void _aibox_data_list_free(GList *list)
{
	if(list == NULL){
		return;
	}

	GList *prev = NULL;
	prev = list->prev;

	while(list != NULL)
	{
		GList *list_remove = list;
		if(list->data){
			free(list->data);
		}
		list = list->next;
		g_list_free_1(list_remove);
	}

	return;
}

unsigned int nf_api_aibox_list_get_size(aibox_search_list *aibox_list)
{
	return (unsigned int)g_list_length(aibox_list->list);
}

aibox_connection_info *nf_api_aibox_list_get_data(aibox_search_list *aibox_list, int index)
{
	aibox_connection_info  *ret = NULL;
	ret = (aibox_connection_info *)g_list_nth_data(aibox_list->list, index);
	return ret;
}

aibox_connection_info *nf_api_aibox_list_get_from_mac(aibox_search_list *aibox_list, unsigned char *mac)
{
	aibox_connection_info *ret = NULL;
	aibox_connection_info *data = NULL;
	GList *list = NULL;

	int i = 0;

	for(list = aibox_list->list; list != NULL; list = list->next){
		data = (aibox_connection_info *)list->data;
		if(memcmp(data->mac, mac, sizeof(data->mac)) == 0){
			ret = data;
			break;
		}
	}
		
	return ret;
}

unsigned char *nf_api_aibox_list_get_mac(aibox_search_list *aibox_list, int index)
{
	aibox_connection_info  *data = NULL;
	data = nf_api_aibox_list_get_data(aibox_list, index);
	if(data == NULL){
		return NULL;
	}
	return data->mac;
}

static unsigned int aibox_connection_info_get_ip(aibox_connection_info *data)
{
	unsigned int ret = 0;
    int is_wan = 1;

    if(data == NULL) return ret;

    is_wan = _is_aibox_connected_wan(data);

	pthread_mutex_lock(&_host_inf_mutex);
    if(get_subnet_host_ip(get_host_info_list(), data->ip, is_wan)){
        ret = data->ip;
    }else
    if(get_subnet_host_ip(get_host_info_list(), data->linklocal_ip, is_wan)){
        ret = data->linklocal_ip;
    }else
    if(get_subnet_host_ip(get_host_info_list(), data->cin_ip, is_wan)){
        ret = data->cin_ip;
    }
	pthread_mutex_unlock(&_host_inf_mutex);


	return ret;
}

unsigned int nf_api_aibox_connection_info_get_ip(aibox_connection_info *data)
{
    unsigned int ip;
    ip = aibox_connection_info_get_ip(data);
    return ip;
}

unsigned int nf_api_aibox_list_get_ip(aibox_search_list *aibox_list, int index)
{
	unsigned int ret = 0;
	aibox_connection_info  *data = NULL;
	data = nf_api_aibox_list_get_data(aibox_list, index);
	return aibox_connection_info_get_ip(data);
}

void nf_api_aibox_list_free(aibox_search_list *aibox_list)
{
	if(aibox_list == NULL) return;
	if(aibox_list->list){
		_aibox_data_list_free(aibox_list->list);
	}

	free(aibox_list);
	return;
}

aibox_search_list *_init_aibox_search_list()
{
	aibox_search_list *list= NULL;
	int ret;
	list = (aibox_search_list *)malloc(sizeof(aibox_search_list));
	if(list == NULL){
		goto _err;
	}

	list->list = NULL;
	ret = pthread_mutex_init(&list->_list_mutex, NULL);
	IPCAM_DBG(MINOR, "ret[%d]\n", ret);

	return list;
_err:
	if(list != NULL){
		if(list->list != NULL){
			_aibox_data_list_free(list->list);
		}
		free(list);
	}
	return NULL;
}

static host_inf *get_host_info_list()
{
    /*
	if(host_inf_list != NULL){
		_host_inf_free(host_inf_list);
	}
	host_inf_list = _get_host_inf_list();
    */

	if(host_inf_list != NULL){
		//_host_inf_free(host_inf_list);
    }else{
        host_inf_list = _get_host_inf_list();
    }
    return host_inf_list;
}

unsigned int nf_api_aibox_search(int timeout)
{
	unsigned int ip = 0;
	int i;
	int openmode_state = 0;
	int discovery_running = 0;

	pthread_mutex_lock(&_host_inf_mutex);
	if(host_inf_list != NULL){
		_host_inf_free(host_inf_list);
	}
	host_inf_list = _get_host_inf_list();
	pthread_mutex_unlock(&_host_inf_mutex);

	openmode_state = get_openmode_state();
	discovery_running = get_discovery_state();
	IPCAM_DBG(MAJOR, "start openmode_state[%d] discovery_running[%d]\n", get_openmode_state(), get_discovery_state());

	//init
	if(_aibox_search_queue == NULL){
		_aibox_search_queue = g_async_queue_new_full(_aibox_queue_destroy_notify);
	}else{
		IPCAM_DBG(MINOR, "already searching\n");
		//goto end;
		return 0;
	}

	_init_aibox_socks();
	aibox_find_mode = 1;
	if(timeout <= 0){
		timeout = 3;
	}

	/*
	//search
	set_openmode_state(OPENMODE_STATE_SCANNING);
	_send_aibox_search();
	*/
	
	//search
	if(nf_get_running_mode()){
		nf_openmode_scan_aibox(timeout);
	}else{
		_send_aibox_search();
	}

	//polling
	ip = _aibox_ip_polling(timeout);
	if(nf_get_running_mode()){	//openmode
		set_openmode_state(openmode_state);
		set_discovery_state(discovery_running);
	}

_end:
	_close_socks();
	aibox_find_mode = 0;
	if(_aibox_search_queue != NULL){
		g_async_queue_unref(_aibox_search_queue);
		_aibox_search_queue = NULL;
	}
	return ip;
}

static int _is_aibox_connected_wan(aibox_connection_info *conn)
{
    char eth_buf[50];
    _get_eth_interafce(conn->mac, NULL, eth_buf);
    if(strncasecmp(eth_buf, "eth1", 4) == 0){
        return 0;
    }

    _get_eth_interafce(NULL, conn->ip, eth_buf);
    if(strncasecmp(eth_buf, "eth1", 4) == 0){
        return 0;
    }

    _get_eth_interafce(NULL, conn->linklocal_ip, eth_buf);
    if(strncasecmp(eth_buf, "eth1", 4) == 0){
        return 0;
    }

    _get_eth_interafce(NULL, conn->cin_ip, eth_buf);
    if(strncasecmp(eth_buf, "eth1", 4) == 0){
        return 0;
    }

    return 1;
}

static void _show_aibox_search_list(aibox_search_list *aibox_list)
{
	aibox_connection_info *ret = NULL;
	aibox_connection_info *data = NULL;
	GList *list = NULL;

	for(list = aibox_list->list; list != NULL; list = list->next){
		data = (aibox_connection_info *)list->data;

        char buf[20];
        printf("[%s:%d] debug ip[%s] linklocal[%s] mac[%s] eth[%s] is_wan[%d] is_dhcp[%d]\n", __func__, __LINE__, 
                _ip_to_str(data->ip, buf), 
                _ip_to_str(data->linklocal_ip, NULL), 
                _mac_to_str(data->mac, NULL), 
                _get_eth_interafce(data->mac, NULL, NULL),
                _is_aibox_connected_wan(data),
                data->is_dhcp);
	}
}

aibox_search_list *nf_api_aibox_search_list(int timeout)
{
	aibox_search_list *aibox_list = _init_aibox_search_list();
    _api_aibox_search_list(aibox_list, timeout);
	//_show_aibox_search_list(aibox_list);
    return aibox_list;
}

static aibox_search_list *_api_aibox_search_list(aibox_search_list *aibox_list, int timeout)
{
	unsigned int ip = 0;
	int i;
	int openmode_state = 0;
	int discovery_running = 0;
	openmode_state = get_openmode_state();
	discovery_running = get_discovery_state();

	aibox_connection_info *data = NULL;
    GList *list = NULL;

	//host inf
	pthread_mutex_lock(&_host_inf_mutex);
	if(host_inf_list != NULL){
		_host_inf_free(host_inf_list);
	}
	host_inf_list = _get_host_inf_list();
	pthread_mutex_unlock(&_host_inf_mutex);

	IPCAM_DBG(MAJOR, "start openmode_state[%d] discovery_running[%d]\n", get_openmode_state(), get_discovery_state());

	//init
	if(_aibox_search_queue == NULL){
		_aibox_search_queue = g_async_queue_new_full(_aibox_queue_destroy_notify);
	}else{
		IPCAM_DBG(MINOR, "already searching\n");
		goto _end;
	}

	if(aibox_list == NULL){
		goto _end;
	}

	_init_aibox_socks();
	aibox_find_mode = 1;
	if(timeout <= 0){
		timeout = 3;
	}

	//search
	IPCAM_DBG(MINOR, "mode[%d]\n", nf_get_running_mode());
	if(nf_get_running_mode()){
		nf_openmode_scan_aibox(timeout);
	}else{
		_send_aibox_search();
	}

	//polling
	//_aibox_list_polling(aibox_list, timeout);
	_aibox_list_polling_with_ipset(aibox_list, timeout);

    //hub ip check
    /*
	for(list = aibox_list->list; list != NULL; list = list->next){
		data = (aibox_connection_info *)list->data;
        data->ip = aibox_hub_ipset(*data);
    }
    */

	/*
	if(nf_get_running_mode()){	//openmode
		set_openmode_state(openmode_state);
		set_discovery_state(discovery_running);
	}
	*/
_end:
	_close_socks();
	aibox_find_mode = 0;
	if(_aibox_search_queue != NULL){
		g_async_queue_unref(_aibox_search_queue);
		_aibox_search_queue = NULL;
	}
	return aibox_list;
}

static int _aicamera_license_support(int ch)
{
    int rc;
    int i;

    NFIPCamLicenseKeyList cam_lic;

    memset(&cam_lic, 0, sizeof(NFIPCamLicenseKeyList));

    rc = nf_ipcam_get_camera_license_key(ch, &cam_lic);

    if(rc != IPCAM_SETUP_RTN_DONE){
        printf("[%s:%d] nf_ipcam_get_camera_license_key rtn[%d]\n", __func__, __LINE__, rc);
        return 0;
    }


    for(i = 0; i < cam_lic.count; i++){
        /*
        printf("[%s:%d] num[%d] key[%s] acquired[%u] expired[%u] name[%s] param1[%u] param2[%u]\n", __func__, __LINE__,
                i,
                cam_lic.key_data[i].key, 
                cam_lic.key_data[i].acquired_date, 
                cam_lic.key_data[i].expired_date,
                cam_lic.key_data[i].name,
                cam_lic.key_data[i].param1,
                cam_lic.key_data[i].param2);
                */

        if(strstr(cam_lic.key_data[i].name, "AI")){
            return 1;
        }
    }

    return 0;
}

static int _aicamera_license_check(int ch)
{
    int rc;
    int i;
	mtable *runtime = get_runtime();

    //printf("[%s:%d] ch[%d] length[%d]\n", __func__, __LINE__, ch, runtime[ch].ai.license_length);

    for(i = 0; i < runtime[ch].ai.license_length; i++){
        /*
        printf("[%s:%d] debug i[%02d] | license[%s] req[%d]ins[%d] value[%s]name[%s]\n", 
                __func__, __LINE__, i,
                runtime[ch].ai.license[i].license_name,
                runtime[ch].ai.license[i].license_required,
                runtime[ch].ai.license[i].license_installed,
                runtime[ch].ai.license[i].value,
                runtime[ch].ai.license[i].name
              );
              */

        if(runtime[ch].ai.license[i].license_required == 0) return 1;
        if(runtime[ch].ai.license[i].license_required * runtime[ch].ai.license[i].license_installed) return 1;
    }



    return 0;
}

NF_AI_MODEL_TYPE_E nf_api_ai_type(int ch)
{
	mtable *runtime = get_runtime();

	if(ch < 0 || ch >= NUM_ACTIVE_CH){
		printf("[%s:%d] jhyoo argument error channel[%d]\n", __func__, __LINE__, ch);
		return NF_AI_MODEL_NONE;
	}

	return runtime[ch].ai.model_type_value;
}

int nf_api_is_ai_camera(int ch){
	//runtime va 환경 체크
	mtable *runtime = get_runtime();

	//printf("[%s:%d] vca_version[%d]\n", __func__, __LINE__, runtime[ch].vca.version.major);
	//IPCAM_DBG(MINOR, "vca_version[%d]\n", runtime[ch].vca.version.major);

	if(runtime[ch].ai.version.major >= 1 && _aicamera_license_check(ch)){
		return 1;
	}

	if(runtime[ch].ai.model_type_support & NF_AI_MODEL_AICAM_PRO){
		return 1;
	}
	
	return 0;
}

//int nf_api_aibox_set(unsigned int aibox_ipaddr, aibox_ch_info *info)
int nf_api_aibox_set(unsigned int ch, unsigned int aibox_ipaddr, const char *algorithm)
{
	int ret = -1;

	IPCAM_DBG(MINOR, "aibox_ipaddr[%08x] ch[%d]\n", aibox_ipaddr, ch);
	if(aibox_ipaddr == NULL){
		if(nf_api_is_ai_camera(ch)){
			ret = _send_aicamera_data(ch, algorithm);
		}else{			//error
			goto _end;
		}
	}else{
		ret = nf_api_aibox_set_video_stream(ch, aibox_ipaddr, algorithm);
	}
_end:	
	return ret;
}

static int _string_possible(const char *str)
{
    if(str == NULL) return 0;
    if(strlen(str) > 0) return 1;
    return 0;
}

int vcam_check(int high_quality, int ch)
{
    gchar key[128];
    const char *main_stream;
    const char *second_stream;

    snprintf(key, 128, "cam.logininfo.L%d.rtsp_addr_main", ch);
    main_stream = nf_sysdb_get_str_nocopy(key);

    snprintf(key, 128, "cam.logininfo.L%d.rtsp_addr_second", ch);
    second_stream = nf_sysdb_get_str_nocopy(key);

    if(_string_possible(main_stream) && _string_possible(second_stream)){
        return high_quality;
    }else if(_string_possible(main_stream)){
        return 1;
    }else if(_string_possible(second_stream)){
        return 0;
    }
    return high_quality;
}

char *_get_ch_stream_uri(int ch, unsigned int host_ip, char *buf, int high_quality)
{
	guint ipaddr = 0;

	GValue ret_value = {0,};
	int rtsp_port = 554;
	char name[50];
	char pass[50];
	char rtsp_buf[50];
	char *ptr = NULL;

	if(buf == NULL){
		return NULL;
	}

	if(host_ip == 0){
		return NULL;
	}

	//user
	if(nf_sysdb_get_key1("usr.U%d.name", 0, &ret_value, NULL))
	{
		g_stpcpy(name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}
	//pass
	if(nf_sysdb_get_key1("usr.U%d.pass", 0, &ret_value, NULL))
	{
		g_stpcpy(pass, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}

	//nvr_rtsp_port
	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		rtsp_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	memset(rtsp_buf, 0, sizeof(rtsp_buf));
	if(rtsp_port == 554){
		sprintf(rtsp_buf, "/");
	}else{
		sprintf(rtsp_buf, ":%d/", rtsp_port);
	}



	sprintf(buf, "rtsp://%s:%s@%d.%d.%d.%d%slive/%s%d", 
			name, pass,
			(host_ip&0xff000000)>>24,
			(host_ip&0xff0000)>>16,
			(host_ip&0xff00)>>8,
			host_ip&0xff,
			rtsp_buf,
			(vcam_check(high_quality, ch)) ? "main" : "second",
			ch);

	return buf;
}

static int _get_mac_from_ip(unsigned char *mac, char *ip, char *eth);
int get_mac_from_ip(unsigned char *mac, unsigned int ip)
{
	int ret = 0;
	host_inf *current;
	char ip_buf[20];
	
	memset(ip_buf, 0, sizeof(ip_buf));

	if(mac == NULL) return ret;
	if(ip == 0 || ip == 0xffffffff) return ret;

	_ip_to_str(ip, ip_buf);

	if(host_inf_list == NULL){
		ret = _get_mac_from_ip(mac, ip_buf, NULL);
	}else{
        pthread_mutex_lock(&_host_inf_mutex);
		for(current = host_inf_list; current != NULL; current = current->next)
		{
			ret = _get_mac_from_ip(mac, ip_buf, current->dev);
			if(ret > 0) break;
		}
        pthread_mutex_unlock(&_host_inf_mutex);
	}

	return ret;
}

static int _get_mac_from_ip_inner(unsigned char *mac, char *line)
{
	int i;
	int mac_check = 0;
	sscanf(line, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

	for(i = 0; i < 6; i++){
		if(mac[i] == 0x00){
			mac_check++;	
		}
	}
	if(mac_check == 6){
		IPCAM_DBG(MINOR, "mac address line[%s]\n", line);
		return 0;
	}
	return 1;
}

static int _get_mac_from_ip(unsigned char *mac, char *ip, char *eth)
{
	char cmd[256] = {0};
	char line[1024] = {0};
	FILE *fp = NULL;
	int fd = -1;
	int ret = 0;
	char *ptr;
	
	snprintf(cmd, sizeof(cmd), "arping -c 1 -w 1 -D %s", ip);
	if(eth != NULL){
		snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd) - 1, " -I %s", eth);
	}

	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		printf("%s proxy_popen fail(cmd:%s)\n", __func__, cmd);
		goto ends_label;
	}

	while(fgets(line, sizeof(line), fp) != NULL)
	{
		if(strstr(line, ip) != NULL){
			if((ptr = strstr(line, "[")) == NULL){
				IPCAM_DBG(MINOR, "strstr ret[%p] line[%s]\n", strstr(line, "["), line);
				continue;
			}
			ptr += 1;

			if(_get_mac_from_ip_inner(mac, ptr)){
				ret++;
				break;
			}
		}
	}
	
ends_label:

	if(fp != NULL || fd > 0)
	{
		proxy_pclose(fp, fd, 3);
	}
	return ret;
}

char *nf_api_get_host_from_rtsp_url(char *url, char *buf)
{
	static char buffer[200] = {0, };
	if(buf == NULL) buf = buffer;
    return get_host_from_url(url, buf);
}

int nf_api_is_nvr_stream(const char *rtsp_url, host_inf *list)
{
    int ret = 0;
	char host_buf[100];

    if(rtsp_url == NULL) return 0;
    if(rtsp_url[0] == '\0') return 0;

    if(list){
        ret = is_host_ip(list, get_host_from_url(rtsp_url, host_buf));
    }else{
        pthread_mutex_lock(&_host_inf_mutex);
	    ret = is_host_ip(host_inf_list, get_host_from_url(rtsp_url, host_buf));
        pthread_mutex_unlock(&_host_inf_mutex);
    }

    return ret;
}

int is_nvr_stream(const char *rtsp_url)
{
	char host_buf[100];

    if(rtsp_url == NULL) return 0;
    if(rtsp_url[0] == '\0') return 0;

    return is_nvr_ip(get_host_from_url(rtsp_url, host_buf));
}

void nf_api_load_host_info_list()
{
	pthread_mutex_lock(&_host_inf_mutex);
	if(host_inf_list != NULL){
		_host_inf_free(host_inf_list);
	}
	host_inf_list = _get_host_inf_list();
	pthread_mutex_unlock(&_host_inf_mutex);
}

int is_nvr_ip(const char *ip_str)
{
    unsigned int ip;

	pthread_mutex_lock(&_host_inf_mutex);
    ip = is_host_ip(get_host_info_list(), ip_str);
	pthread_mutex_unlock(&_host_inf_mutex);

    return ip;
}

static char _get_hex_from_char(char a)
{
	if (a >= '0' && a <= '9')
		return (a-'0');

	if (a >= 'a' && a <= 'f')
		return ((a-'a')+0xa);

	if (a >= 'A' && a <= 'F')
		return ((a-'A')+0xa);

	return (-1);
}

static void _get_mac_from_str(char* src, char* dst)
{
	int i=0;
	int j=0;
	char val[12];

	if (strlen(src) > 17)
	{
		memset(dst, 0x00, 6);
		return;
	}

	for (i=0; i<strlen(src); i++)
	{
		val[j] = _get_hex_from_char(src[i]);

		if (val[j] >= 0)
			j++;
	}

	dst[0] = (val[0]<<4) + val[1];
	dst[1] = (val[2]<<4) + val[3];
	dst[2] = (val[4]<<4) + val[5];
	dst[3] = (val[6]<<4) + val[7];
	dst[4] = (val[8]<<4) + val[9];
	dst[5] = (val[10]<<4) + val[11];
}

extern int _get_macaddress_using_arping_by_ip(unsigned char *mac, char *ip, char *eth);
static char *_get_eth_interafce(unsigned char *p_macaddr, unsigned int ip_addr, char *eth_str)
{
    static ret_buf[100];
	char buf[1024] = { 0, };
	char macaddr_str[14] = { 0, };
	unsigned char macaddr_ch[6] = { 0, };
    char ip_buf[20];
	FILE *fp = NULL;

    if(eth_str == NULL){
        eth_str = ret_buf;
    }

    //if(p_macaddr == NULL || eth_str == NULL) return NULL;
    if(p_macaddr == NULL && (ip_addr == 0 || ip_addr == 0xffffffff)) return NULL;

    eth_str[0] = '\0';

	// get mac address
	fp = fopen("/proc/net/arp", "r");
	if (fp == NULL)
		return NULL;

	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		char buf_ipaddr[32] = { 0, };
		char buf_hw_type[32] = { 0, };
		char buf_flags[32] = { 0, };
		char buf_hw_address[32] = { 0, };
		char buf_mask[32] = { 0, };
		char buf_device[32] = { 0, };

        char buf_calc[32];

        unsigned int ip = 0;

		sscanf(buf, "%s\t%s\t%s\t%s\t%s\t%s",
				buf_ipaddr, buf_hw_type, buf_flags, buf_hw_address, buf_mask, buf_device);

        ip = inet_addr(buf_ipaddr);
        if(ip == 0x00 || ip == 0xffffffff){
            //printf("[%s:%d] ip[%s] is not possible\n", __func__, __LINE__, buf_ipaddr);
            continue;
        }

		{
			int i, j = 0;

			for (i = 0; i < strlen(buf_hw_address); i++)
				if (buf_hw_address[i] != ':')
					macaddr_str[j++] = buf_hw_address[i];

			_get_mac_from_str(macaddr_str, macaddr_ch);

            if(p_macaddr){
                if (!memcmp(macaddr_ch, p_macaddr, sizeof(macaddr_ch)))
                {
                    if(!_get_macaddress_using_arping_by_ip(buf_calc, buf_ipaddr, buf_device)){
                        printf("[%s:%d] mac[%s] | ip[%s] dev[%s] is not possible\n", __func__, __LINE__, _mac_to_str(p_macaddr, NULL), buf_ipaddr, buf_device);
                        continue;
                    }

                    fclose(fp);
                    snprintf(eth_str, 32, "%s", buf_device);
                    return eth_str;
                }
            }

            if(ip_addr != 0){
                if(ntohl(inet_addr(buf_ipaddr)) == ip_addr)
                {
                    if(!_get_macaddress_using_arping_by_ip(buf_calc, buf_ipaddr, buf_device)){
                        printf("[%s:%d] ip[%s] | ip[%s] dev[%s] is not possible\n", __func__, __LINE__, _ip_to_str(ip_addr, NULL), buf_ipaddr, buf_device);
                        continue;
                    }

                    fclose(fp);
                    snprintf(eth_str, 32, "%s", buf_device);
                    return eth_str;
                }
            }
		}
	}

	fclose(fp);
	return NULL;
}

unsigned int _get_nvr_host_ip(unsigned int ip)
{
	unsigned int host_ip = 0;
	host_inf *found = NULL;

	pthread_mutex_lock(&_host_inf_mutex);
    host_ip = get_subnet_host_ip(get_host_info_list(), ip, is_ip_wan(ip));
	pthread_mutex_unlock(&_host_inf_mutex);

    /*
	if(found != NULL){
		host_ip = found->ipaddr;
	}else{
		host_ip = get_netif_ip("eth0:0");

		if(host_ip == 0){
			host_ip = get_netif_ip("eth1:0");
		}

		if(host_ip == 0){
			host_ip = get_netif_ip("br0:0");
		}
		host_ip = ntohl(host_ip);
	}
    */
	return host_ip;
}

static host_inf *_get_host_inf_from_ip(host_inf *list, unsigned int ip)
{
	host_inf *current;
	for(current = list; current != NULL; current = current->next)
	{
		if((ip & current->netmask) == (current->ipaddr & current->netmask)){
			return current;
		}
	}
	return NULL;
}

char *_get_zmq_uri(char *buf, unsigned int host_ip)
{
	if (host_ip == 0){
		return NULL;
	}

	if(buf == NULL){
		return NULL;
	}

	sprintf(buf, "tcp://%d.%d.%d.%d:1015", 
			(host_ip&0xff000000)>>24,
			(host_ip&0xff0000)>>16,
			(host_ip&0xff00)>>8,
			host_ip&0xff
		   );

	return buf;
}

int _http_get(char *ipstr, char *path, char *query, char *buffer, int buffer_size)
{
    icm_http        ctx;
    icm_response    res;
	char request_buffer[500];

	int ret = ICM_RTN_UNKNOWN;

	//data set
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));
	memset(request_buffer, 0x00, sizeof(request_buffer));

	strncpy(ctx.ipstr, ipstr, 16);
	strncpy(ctx.username, "ADMIN", 64);
	strncpy(ctx.password, "1234", 64);
	ctx.ssl = 1;
	ctx.auth_type = 0;
	ctx.port = 8443;
	//ctx.port = 8000;
	ctx.timeout = 3;

	if(query != NULL){
		strncpy(request_buffer, query, sizeof(request_buffer) - 1);
	}

	icm_http_new_get_request_json(&ctx, path, NULL, &res);
	IPCAM_DBG(MINOR, "request[%d][%d][%s]\n", ret, ctx.status, res.msg);

	if(res.msg != NULL){
		strncpy(buffer, res.msg, buffer_size - 1);
	}
end:
	icm_http_response_free(&res);
	return ret; 
}

int _http_json(char *ipstr, char *path, json_t *json)
{
    icm_http        ctx;
    icm_response    res;

	int ret = ICM_RTN_UNKNOWN;

	//data set
    memset(&ctx, 0x00, sizeof(ctx));
    memset(&res, 0x00, sizeof(res));


	strncpy(ctx.ipstr, ipstr, 16);
	strncpy(ctx.username, "ADMIN", 64);
	strncpy(ctx.password, "1234", 64);
	ctx.ssl = 1;
	ctx.auth_type = 0;
	ctx.port = 8443;
	//ctx.port = 8000;
	ctx.timeout = 3;

	ret = icm_http_new_post_request_json(&ctx, path, json, &res);

	IPCAM_DBG(MINOR, "request[%d][%d][%s]\n", ret, ctx.status, res.msg);
end:
	icm_http_response_free(&res);
	return ret; 
}

int _send_aicamera_data(unsigned int ch, const char *algorithm)
{
	int ret = DLVA_API_RET_FAILED_INIT;
	HTTP_CTX ctx;
	int rc = 0;
	int len = 0;

    char *format = "algorithm_value0=%s\nalgorithm_cnt=1";
    char post[300];

	Token_iterator iter;
	char *str;
	int i;

	http_init(&ctx);
	
	//arg error check
	if(ch < 0 || ch >= AVAILABLE_MAX_CH){
		printf("[%s:%d] ch[%d] channel range error\n", __func__, __LINE__, ch);
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto err;
	}

	if(algorithm == NULL || strlen(algorithm) <= 0){
		printf("[%s:%d] ch[%d] algorithm[%p][%d]\n", __func__, __LINE__, ch, algorithm, (algorithm) ? strlen(algorithm) : 0);
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto err;
	}

    snprintf(post, sizeof(post), format, algorithm);

	//http setting
	aicam_http_default_setting(&ctx, ch);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	//http_data_set(&ctx, HTTP_ADD_QUERY, "action", "support.ai_algorithm");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.ai_algorithm");
	http_data_set(&ctx, HTTP_SET_POST, post);

	//http request
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto err;
	}
	if(ctx.status != 200) ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
	ret = DLVA_API_RET_OK;

	printf("[%s:%d] res[%s]\n", __func__, __LINE__, http_get_res(&ctx));
err:
	http_release(&ctx);
	return ret;

}

static void _get_net_inf_parse_dev(char *line, char *dev_buf, size_t buf_size)
{
	icm_str_array array = NULL;
	char *ptr = NULL;

	if(line == NULL || dev_buf == NULL)
	{
		goto ends_label;
	}

	array = icm_str_split((const char *)line, " ", 3);
	if(array == NULL) 
	{
		goto ends_label;
	}

	icm_str_trim(array[1], strlen(array[1]));

	ptr = strstr(array[1], ":");
	if(ptr != NULL)
	{
		*ptr = 0;
		snprintf(dev_buf, buf_size, "%s", array[1]);
	}

ends_label:

	if(array != NULL)
	{
		icm_str_array_free(array, 3);
	}
}

host_inf *nf_api_get_host_inf_list(void)
{
	return _get_host_inf_list();
}

static host_inf *_get_host_inf_list(void)
{
	icm_str_array inet_str = NULL;
	icm_str_array addr_str = NULL;
	FILE *fp = NULL;
	int fd = -1;
	unsigned int inf_addr, inf_mask;
	char dev[32] = {0};
	char cmd[256] = {0};
	char line_1[1024] = {0};
	char line_2[1024] = {0};
	host_inf *ret = NULL;
	host_inf *current = NULL;

	snprintf(cmd, sizeof(cmd), "ip -f inet address");
	fp = proxy_popen((const char *)cmd, "r", &fd);
	if(fp == NULL || fd < 0)
	{
		IPCAM_DBG(ERROR, "proxy_popen fail(%s)\n", cmd);
		goto ends_label;
	}

	while(fgets(line_1, sizeof(line_1), fp) != NULL)
	{
		if(isspace(line_1[0]) != 0) { continue; }

		// continue loopback
		if(strstr(line_1, "lo:") != NULL) { continue; }


		_get_net_inf_parse_dev(line_1, dev, sizeof(dev));

		while(fgets(line_2, sizeof(line_2), fp) != NULL)
		{
            int no = 0;
            sscanf(line_2, "%d:*", &no);
			icm_str_trim(line_2, strlen(line_2));

			if(strncmp(line_2, "inet ", 5) == 0)
			{
				inet_str = icm_str_split((const char *)line_2, " ", 3);
				if(inet_str != NULL)
				{
					addr_str = icm_str_split((const char *)inet_str[1], "/", 2);

					icm_str_array_free(inet_str, 3);

					if(addr_str != NULL)
					{
						if(current == NULL){
							ret = (host_inf *)malloc(sizeof(host_inf));
							current = ret;
						}else{
							current->next = (host_inf *)malloc(sizeof(host_inf));
							current = current->next;
						}

						if(current == NULL){
							icm_str_array_free(addr_str, 2);
							goto ends_label;
						}

						memset(current, 0x00, sizeof(host_inf));

						current->ipaddr = ntohl(inet_addr(addr_str[0]));
						current->netmask = _netmask_num_to_addr(atoi(addr_str[1]));
						strncpy(current->dev, dev, sizeof(current->dev));
						
						icm_str_array_free(addr_str, 2);
					}
				}
			}
            else if(no > 0)
            {
                _get_net_inf_parse_dev(line_2, dev, sizeof(dev));
            }
		}
	}

ends_label:

	if(fp != NULL || fd > 0)
	{
		proxy_pclose(fp, fd, 3);
	}

	IPCAM_DBG(MINOR, "end\n");

	return ret;
}

static void _host_inf_free(host_inf *list)
{
	int count = 0;
	while(list){
		host_inf *curr = list;
		list = list->next;
		free(curr);
		count++;
	}
}

char *nf_api_get_nvr_owner(char *owner)
{
	static char buffer[40] = {0, };
	unsigned char mac[6];// = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	char *p;
	int i;

	if(owner == NULL){
		owner = buffer;
	}

	memset(owner, 0x00, 13);

	if(nf_netif_get_mac_str(mac) == NULL) return NULL;

	for(i = 0; i < sizeof(mac); i++){
		sprintf(owner + (i*2), "%02x", mac[i]);
	}
	//printf("[%s:%d] owner[%s]\n", __FUNCTION__, __LINE__, owner);
	return owner;
}

void nf_api_aibox_set_http_timeout(int second)
{
	http_timeout = second;
}

void nf_api_aibox_set_http_connection_timeout(int second)
{
	http_connection_timeout = second;
}


int nf_api_delete_owner(unsigned int aibox_ip)
{
	char date[100];
	char *hash = NULL;
	char *post = "";
	HTTP_CTX ctx;

	if(_get_aibox_owner(aibox_ip, NULL, date) == DLVA_API_RET_AIBOX_CONNECTION_FAILED){
		return DLVA_API_RET_AIBOX_CONNECTION_FAILED;
	}
	if(_delete_owner(aibox_ip, date) == DLVA_API_RET_AIBOX_CONNECTION_FAILED){
		return DLVA_API_RET_AIBOX_CONNECTION_FAILED;
	}
}

static int _delete_owner(unsigned int aibox_ip, char *date)
{
	char *hash = NULL;
	char *post = "";
	int rc = 0;
	HTTP_CTX ctx;

	hash = _get_aibox_hash(post, date);

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/owner/");
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);
	http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
	//http_data_set(&ctx, HTTP_SET_SSL			, 0);

	rc = http_request(&ctx);
	http_release(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT) return DLVA_API_RET_AIBOX_CONNECTION_FAILED;
	if(ctx.status == 204) return 0;
	return -1;
}

static unsigned int _get_aibox_zmq_uri(unsigned aibox_ip, char *buf_uri)
{
    HTTP_CTX ctx;
    json_t *json = NULL;
    json_error_t error;
    const char *owner_ptr = NULL;
    int rc = 0;
    unsigned int ret = 0;
    int i;

    int ch;
    char id[129];
    char pw[129];

    json_t *zmq_info = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] \n", __func__, __LINE__, aibox_ip);
        return 0;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0)
    {
        printf("[%s:%d] argument check error ch[%d] \n", __func__, __LINE__, ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/event/callback/zmq/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    json_array_foreach(json, i, zmq_info){
        const char *zmq_uri;

        zmq_uri = json_string_value(json_object_get(zmq_info, "addr"));
        if(zmq_uri && strlen(zmq_uri) > 0){
            snprintf(buf_uri, "%s", zmq_uri);
            break;
        }
    }
    
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static unsigned int _get_aibox_network_ip(unsigned aibox_ip)
{
    HTTP_CTX ctx;
    json_t *json = NULL;
    json_error_t error;
    const char *owner_ptr = NULL;
    int rc = 0;
    unsigned int ret = 0;

    int ch;
    char id[129];
    char pw[129];

    const char *eth = NULL;
    json_t *eth_detail = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] \n", __func__, __LINE__, aibox_ip);
        return 0;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0)
    {
        printf("[%s:%d] argument check error ch[%d] \n", __func__, __LINE__, ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/network/ip/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    json_object_foreach(json, eth, eth_detail){
        const char *ip_str;
        unsigned int ipaddr = 0;
        unsigned int linklocal_ipaddr = 0;

        ip_str = json_string_value(json_object_get(eth_detail, "ipaddr"));
        if(ip_str){
            ipaddr = htonl(inet_addr(ip_str));
        }

        ip_str = json_string_value(json_object_get(eth_detail, "linklocal_ipaddr"));
        if(ip_str){
            linklocal_ipaddr = htonl(inet_addr(ip_str));
        }

        if(ipaddr == 0) continue;
        ret = ipaddr;
        if(linklocal_ipaddr == aibox_ip){
            break;
        }
    }

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static unsigned int _get_aibox_ip_info(int ch)
{
	GValue ret_value = {0,};
    unsigned int ip = 0;
    unsigned int linklocal_ip = 0;

    if(nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
    {
        linklocal_ip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }

    ip = _get_aibox_network_ip(linklocal_ip);
    if(ip == 0) ip = linklocal_ip;
    return ip;
}

int nf_api_get_aibox_url(int ch, char *url_buffer)
{
    /*
    char ip_buffer[50];
    int port = 8443;
    int ssl = 1;
    int ret = 0;

    if(ch < 0) return 0;
    if(url_buffer == NULL) return 0;

    ret = sprintf(url_buffer, "%s://%s:%d",
            (ssl) ? "https" : "http",
            _ip_to_str(_get_aibox_ip_info(ch), ip_buffer),
            port);

    return ret;
    */
	GValue ret_value = {0,};
    char ip_buffer[50];
    int port = 8443;
    int ssl = 1;
    int ret = 0;
    unsigned int ip = 0;
    unsigned int linklocal_ip = 0;

    if(ch < 0) return 0;
    if(url_buffer == NULL) return 0;

    if(nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
    {
        linklocal_ip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    ip = _get_aibox_network_ip(linklocal_ip);
    if(ip == 0) return 0;

    ret = sprintf(url_buffer, "%s://%s:%d",
            (ssl) ? "https" : "http",
            _ip_to_str(ip, ip_buffer),
            port);

    return ret;
}

static int set_zmq_uri(unsigned int aibox_ip, const char *owner, const char *zmq_uri, char *date)
{
	int ret = 0;
	char *hash = NULL;
	char *post = NULL;
	int rc = 0;
	HTTP_CTX ctx;
	json_t *json = NULL;
	http_init(&ctx);

	if(owner == NULL ||
		zmq_uri == NULL ||
		date == NULL ||
		aibox_ip == 0 || 
		aibox_ip == 0xffffffff)
	{
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	//_get_zmq_uri(zmq_uri, _get_nvr_host_ip(aibox_ip));
	json = json_pack("{s:s,s:s}",
			"owner", owner,
			"zmq_addr", zmq_uri);
	if(json == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	post = json_dumps(json, JSON_ENCODE_ANY);
	if(post == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	hash = _get_aibox_hash(post, date);

	//http api
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/owner/");
	http_data_set(&ctx, HTTP_SET_POST, post);
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	if(ctx.status != 200) ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
endl:
	if(hash) free(hash);
	if(post) free(post);
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

static int set_aibox_zmq_uri(const char *nvr_owner, unsigned int aibox_ip, unsigned int host_ip)
{
    char date[100];
	char zmq_uri[200];
	int rc = 0;

	memset(date, 0, sizeof(date));
	memset(zmq_uri, 0, sizeof(zmq_uri));

	if((rc = _get_aibox_owner(aibox_ip, NULL, date)) < 0){
        return 0;
	}

	_get_zmq_uri(zmq_uri, host_ip);
	rc = set_zmq_uri(aibox_ip, nvr_owner, zmq_uri, date);

    printf("[%s:%d] set_zmq_uri[%d]\n", __func__, __LINE__, rc);

    return 1;
}

static int is_mac_wan(const unsigned char *mac)
{
    int is_wan;
    char eth_buf[50];
    _get_eth_interafce(mac, 0, eth_buf);

    if(strncasecmp(eth_buf, "eth1", 4) == 0){
        return 0;
    }

    return 1;
}

static int is_ip_wan(unsigned int ip)
{
    int is_wan;
    char eth_buf[50];
    _get_eth_interafce(NULL, ip, eth_buf);
    if(strncasecmp(eth_buf, "eth1", 4) == 0){
        return 0;
    }

    return 1;
}

int nf_api_set_aibox_owner(unsigned int aibox_ip)
{
	char owner[20];
	return set_aibox_owner(aibox_ip, nf_api_get_nvr_owner(owner));
}

static int set_aibox_owner(unsigned int aibox_ip, const char *owner)
{
	int ret = 0;
	int rc = 0;
	char aibox_owner[30];
	char date[100];
	char zmq_uri[200];
	json_t *json = NULL;

	if(aibox_ip == 0x00 || aibox_ip == 0xffffffff){
		ret = DLVA_API_RET_ARGUMENT_ERROR;
		goto endl;
	}
	if(owner == NULL){
		ret = DLVA_API_RET_ARGUMENT_ERROR;
		goto endl;
	}
	if(owner[0] == '\0'){
		ret = DLVA_API_RET_ARGUMENT_ERROR;
		goto endl;
	}

	memset(date, 0, sizeof(date));
	memset(zmq_uri, 0, sizeof(zmq_uri));
	
	if((rc = _get_aibox_owner(aibox_ip, aibox_owner, date)) < 0){
		ret = DLVA_API_RET_FAILED_INIT;
		if(rc == DLVA_API_RET_AIBOX_CONNECTION_FAILED)
		   	ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}
	
    printf("[%s:%d] current aibox_owner[%s] nvr_owner[%s]\n", __func__, __LINE__, aibox_owner, owner);
	if(strncasecmp(owner, aibox_owner, 12) != 0){
        _delete_owner(aibox_ip, date);
    }

	_get_zmq_uri(zmq_uri, _get_nvr_host_ip(aibox_ip));
	ret = set_zmq_uri(aibox_ip, owner, zmq_uri, date);

endl:
	return ret;
}

static int _get_aibox_owner(unsigned int aibox_ip, char *owner, char *date)
{
	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;
	const char *owner_ptr = NULL;
	int ret = DLVA_API_RET_OK;
	int rc = 0;

	http_init(&ctx);
	if(aibox_ip == 0 || aibox_ip == 0xffffffff){
	   ret = DLVA_API_RET_ARGUMENT_ERROR;
	   goto endl;
	}

	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/owner/");

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	if(ctx.status != 200){
		ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
	   	goto endl;
	}

	if(date != NULL)
	{
		//strcpy(date, str_null_to_blank(ctx.http_header_date));
		sprintf(date, "%s", str_null_to_blank(ctx.http_header_date));

		//printf("%s date[%s] ctx.http-header_date[%s]\n", __func__, date, ctx.http_header_date);

		/*
		if(ctx.http_header_date == NULL){
			date[0] = '\0';
		}else{
			int date_len = strlen(ctx.http_header_date);
			strncpy(date, ctx.http_header_date, date_len);
			date[date_len] = 0;
		}
		*/
	}

	if(owner != NULL){
		json = json_loads(http_get_res(&ctx), 0, &error);
		if(json == NULL){
			fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
			fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
			ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
			goto endl;
		}
		owner_ptr = json_string_value(json_object_get(json, "owner"));
		sprintf(owner, "%s", str_null_to_blank(owner_ptr));
	}

endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

static char *_get_aibox_hash(char *body, char *date)
{
	char *hash_src = NULL;
	char *ret = NULL;
	int hash_src_len = 0;
	if(body == NULL) body = "";
	if(date == NULL) goto err;

	hash_src_len = strlen(body) + 1 + strlen(date);	
	hash_src = malloc(hash_src_len + 1);
	if(hash_src == NULL) goto err;

	sprintf(hash_src, "%s:%s", body, date);
	hash_src[hash_src_len] = 0;

	//printf("[%s:%d] hash_src[%d][%s]\n", __FUNCTION__, __LINE__,
	//		hash_src_len, hash_src);

	ret = http_get_encrypt_hmac("#a1b0x$haShKey)", hash_src, EVP_sha256());
err:
	if(hash_src) free(hash_src);
	return ret;
}

char *nf_api_get_aibox_owner(unsigned int aibox_ip, char *owner)
{
	static char buffer[40];
	if(owner == NULL){
		memset(buffer, 0, sizeof(buffer));
		owner = buffer;
	}
	
	if(_get_aibox_owner(aibox_ip, owner, NULL) != 0){
		return NULL;
	}

	if(strlen(owner) == 0){
		return NULL;
	}
	
	return owner;
}

static const char *str_null_to_blank(const char *str)
{
	static const char blank[2] = {0, };
	if(str) return str;
	return blank;
}

int nf_api_aibox_delete_video_stream(unsigned int ch, unsigned int aibox_ip)
{
	int ret = DLVA_API_RET_OK;
	int rc = 0;
	char date[100];
	char aibox_owner[30];
	char nvr_owner[30];
	char query[100];
	char *hash = NULL;
	char *post = NULL;
	unsigned int host_ip = 0;

	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;

	http_init(&ctx);
	if(aibox_ip == 0 || aibox_ip == 0xffffffff){
	   ret = DLVA_API_RET_ARGUMENT_ERROR;
	   goto endl;
	}

	memset(date, 0, sizeof(date));
	memset(aibox_owner, 0, sizeof(aibox_owner));
	memset(nvr_owner, 0, sizeof(nvr_owner));
	memset(query, 0, sizeof(query));


    //API Owner 
	if((rc = _get_aibox_owner(aibox_ip, aibox_owner, date)) != DLVA_API_RET_OK){
		ret = DLVA_API_RET_FAILED_INIT;
		if(rc == DLVA_API_RET_AIBOX_CONNECTION_FAILED)
			ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}
	nf_api_get_nvr_owner(nvr_owner);

    if(strncmp(nvr_owner, aibox_owner, strlen(nvr_owner)) != 0){
		//printf("[%s:%d] aibox_owner[%s] nvr_owner[%s]\n", __FUNCTION__, __LINE__, aibox_owner, nvr_owner);
		ret = DLVA_API_RET_AIBOX_PERMISSION_DENIED;
		goto endl;
    }

    //API video source set https://<aibox-ip>:8443/itx/ai/analytics/?owner={owner}
	json = json_array();
	if(json == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	host_ip = _get_nvr_host_ip(aibox_ip);
	ret = json_array_append_new(json,
			json_pack_ex(&error, 0, "{s:i,s:s,s:s,s:s,s:s}",
				"ch", ch,
				"url", "",
				"algorithm", "",
				"owner", nvr_owner,
				"name", ""
				)
			);
	if(ret < 0){
		fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
		fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	post = json_dumps(json, JSON_ENCODE_ANY);
	if(post == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	hash = _get_aibox_hash(post, date);
	sprintf(query, "owner=%s", nvr_owner);


	//http set api
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/analytics/");
	http_data_set(&ctx, HTTP_SET_QUERY, query);
	http_data_set(&ctx, HTTP_SET_POST, post);
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}


endl:
	if(hash) free(hash);
	if(post) free(post);
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

static int _get_algorithm_resolution_quality(int ch, aibox_algorithm_info *info)
{
	if(info == NULL) return 0;
	if(info->value[0] == '\0') return 0;

	mtable *runtime = get_runtime();

	if(strncmp(info->value, "facep_rec_attr", strlen(info->value)) == 0) return 1;

	if(info->resolution_min_height > 360) return 1;
	return 0;
}

int nf_api_aibox_set_video_streams(unsigned int aibox_ip, aibox_algorithm_name *ch_info, unsigned int ch_mask)
{
	int ret = DLVA_API_RET_OK;
	int rc = 0;
	int i;
	char date[100];
	char stream_buf[300];
	char aibox_owner[30];
	char nvr_owner[30];
	char query[100];
	char *hash = NULL;
	char *post = NULL;
	char *algorithm_str = NULL;
	unsigned int host_ip = 0;
	unsigned int is_highquality = 0;
	ai_capa_t *capa = NULL;
	unsigned int aibox_ch;

	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;

	http_init(&ctx);
	if(aibox_ip == 0 || aibox_ip == 0xffffffff){
	   ret = DLVA_API_RET_ARGUMENT_ERROR;
	   goto endl;
	}

	memset(date, 0, sizeof(date));
	memset(stream_buf, 0, sizeof(stream_buf));
	memset(aibox_owner, 0, sizeof(aibox_owner));
	memset(nvr_owner, 0, sizeof(nvr_owner));
	memset(query, 0, sizeof(query));

    //API Owner 
	rc = _get_aibox_capability(aibox_ip, 0, date, &capa);
	if(rc == DLVA_API_RET_AIBOX_CONNECTION_FAILED){
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}
	if(capa == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	strncpy(aibox_owner, capa->owner, sizeof(capa->owner));
	nf_api_get_nvr_owner(nvr_owner);

    if(strncmp(nvr_owner, aibox_owner, strlen(nvr_owner)) != 0){
		//printf("[%s:%d] aibox_owner[%s] nvr_owner[%s]\n", __FUNCTION__, __LINE__, aibox_owner, nvr_owner);
		ret = DLVA_API_RET_AIBOX_PERMISSION_DENIED;
		goto endl;
    }

    //API video source set https://<aibox-ip>:8443/itx/ai/analytics/?owner={owner}
	json = json_array();
	if(json == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	host_ip = _get_nvr_host_ip(aibox_ip);

	for(i = 0; i < AVAILABLE_MAX_CH; i++){
		if((ch_mask >> i) & 1){
			algorithm_str = ch_info[i].value;
		}else{
			algorithm_str = "";
		}

		aibox_ch = _aibox_get_empty_ch(aibox_ip, i);

		if(aibox_ch < 0){
			printf("[%s:%d] stream full aibox_empty_ch[%d] aibox_ip[%s] ch[%d]\n", __func__, __LINE__, aibox_ch, _ip_to_str(aibox_ip, NULL), i);
			fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
			fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
			//ret = DLVA_API_RET_FAILED_INIT;
			//goto endl;
			aibox_ch = i;
		}

		ret = json_array_append_new(json,
				json_pack_ex(&error, 0, "{s:i,s:s,s:s,s:s,s:s}",
					"ch", aibox_ch,
					"url", (algorithm_str[0] == 0) ? "" : _get_ch_stream_uri(i, host_ip, stream_buf, _get_algorithm_resolution_quality(i, _get_algorithm_info(algorithm_str, capa))),
					"algorithm", str_null_to_blank(algorithm_str),
					"owner", nvr_owner,
					"name", (algorithm_str[0] == 0) ? "" : nvr_owner
					)
				);
		if(ret < 0){
			fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
			fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
			ret = DLVA_API_RET_FAILED_INIT;
			goto endl;
		}
	}


	post = json_dumps(json, JSON_ENCODE_ANY);
	if(post == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	hash = _get_aibox_hash(post, date);
	if(hash == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	sprintf(query, "owner=%s", nvr_owner);


	//http set api
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/analytics/");
	http_data_set(&ctx, HTTP_SET_QUERY, query);
	http_data_set(&ctx, HTTP_SET_POST, post);
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	if(ctx.status != 200) ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
endl:
	if(hash) free(hash);
	if(post) free(post);
	if(json) json_decref(json);
	if(capa) nf_api_capability_free(capa);
	http_release(&ctx);
	return ret;
}

//int nf_api_dlva_set_video_stream(unsigned int aibox_ip, aibox_ch_info *info)
int _nf_api_aibox_set_video_stream(unsigned int ch, unsigned int aibox_ch, unsigned int aibox_ip, const char *algorithm)
{
	int ret = DLVA_API_RET_OK;
	int rc = 0;
	int i;
	char date[100];
	char stream_buf[300];
	char aibox_owner[30];
	char nvr_owner[30];
	char query[100];
	char *hash = NULL;
	char *post = NULL;
	unsigned int host_ip = 0;
	unsigned int is_highquality = 0;
	ai_capa_t *capa = NULL;
	aibox_algorithm_info *info = NULL;

	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;

	http_init(&ctx);
	if(aibox_ip == 0 || aibox_ip == 0xffffffff){
	   ret = DLVA_API_RET_ARGUMENT_ERROR;
	   goto endl;
	}

	memset(date, 0, sizeof(date));
	memset(stream_buf, 0, sizeof(stream_buf));
	memset(aibox_owner, 0, sizeof(aibox_owner));
	memset(nvr_owner, 0, sizeof(nvr_owner));
	memset(query, 0, sizeof(query));

    //API Owner 
	rc = _get_aibox_capability(aibox_ip, aibox_ch, date, &capa);
	if(rc == DLVA_API_RET_AIBOX_CONNECTION_FAILED){
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}
	if(capa == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	strncpy(aibox_owner, capa->owner, sizeof(capa->owner));
	nf_api_get_nvr_owner(nvr_owner);

    if(strncmp(nvr_owner, aibox_owner, strlen(nvr_owner)) != 0){
		ret = DLVA_API_RET_AIBOX_PERMISSION_DENIED;
		goto endl;
    }

    //API video source set https://<aibox-ip>:8443/itx/ai/analytics/?owner={owner}
	json = json_array();
	if(json == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	if((algorithm != NULL) && (algorithm[0] != '\0')){
		for(i = 0; i < capa->algorithm_count; i++){
			if(capa->algorithm_list[i].value[0] == '\0') continue;
			if(strncmp(capa->algorithm_list[i].value, algorithm, strlen(algorithm)) == 0){
				info = &(capa->algorithm_list[i]);
				break;
			}
		}
	}

	host_ip = _get_nvr_host_ip(aibox_ip);
	ret = json_array_append_new(json,
			json_pack_ex(&error, 0, "{s:i,s:s,s:s,s:s,s:s}",
				"ch", aibox_ch,
				"url", _get_ch_stream_uri(ch, host_ip, stream_buf, _get_algorithm_resolution_quality(ch, info)),
				"algorithm", str_null_to_blank(algorithm),
				"owner", nvr_owner,
				"name", nvr_owner
				)
			);
	if(ret < 0){
		fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
		fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	post = json_dumps(json, JSON_ENCODE_ANY);
	if(post == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	hash = _get_aibox_hash(post, date);
	if(hash == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	sprintf(query, "owner=%s", nvr_owner);


	//http set api
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/analytics/");
	http_data_set(&ctx, HTTP_SET_QUERY, query);
	http_data_set(&ctx, HTTP_SET_POST, post);
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	if(ctx.status != 200) ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
endl:
	if(hash) free(hash);
	if(post) free(post);
	if(json) json_decref(json);
	if(capa) nf_api_capability_free(capa);
	http_release(&ctx);
	return ret;
}

static int _aibox_get_empty_ch(unsigned int aibox_ip, int ch)
{
	int ret = -1;
	int rc = 0;
	char date[100];
	char stream_buf[300];
	char aibox_owner[30];
	char nvr_owner[30];
	char query[100];
	char *hash = NULL;
	int i;
	int len;

	HTTP_CTX ctx;
	json_t *json = NULL;
	json_error_t error;

	http_init(&ctx);

	//API Owner
	if(_get_aibox_owner(aibox_ip, aibox_owner, date) < 0){
		printf("[%s:%d] _get_aibox_owner failed\n", __func__, __LINE__);
		goto endl;
	}
	nf_api_get_nvr_owner(nvr_owner);

	/*
	 if(_get_aibox_owner(aibox_ip, aibox_owner, date) < 0){
		 printf("[%s:%d] _get_aibox_owner failed\n", __func__, __LINE__);
	 	goto endl;
	 }
	 */

	//API get stream list
	hash = _get_aibox_hash(NULL, date);
	sprintf(query, "owner=%s", nvr_owner);
			

	//http set api
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/analytics/");
	http_data_set(&ctx, HTTP_SET_QUERY, query);
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] timeout failed\n", __func__, __LINE__);
		goto endl;
	}

	//printf("[%s:%d] status[%d][%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
	
	if(ctx.status != 200){
		goto endl;
	}

	json = json_loads(ctx.res_data.memory, 0, &error);
	if(json == NULL){
		fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
		fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		goto endl;
	}

	len = json_array_size(json);

	for(i = 0; i < len; i++){
		aibox_ch_info *info = NULL;
		info = stream_list_parser(json_array_get(json, i));
		if(info){
			//printf("[%s:%d] ret[%d] no[%d] ch[%d] name[%s] url[%s] ai_algorithm_str[%s]\n", __func__, __LINE__, ret, info->no, info->ch, info->name, info->url, info->ai_algorithm_str);

			if(info->ch == ch){
				ret = info->no;
				break;
			}

			if(info->url[0] == '\0' || info->ai_algorithm_str[0] == '\0'){
				if(info->no == ch){
					ret = info->no;
				}

				if(ret < 0){
					ret = info->no;
				}
			}
		}
	}
endl:
	if(hash) free(hash);
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

int nf_api_aibox_set_video_stream(unsigned int ch, unsigned int aibox_ip, const char *algorithm)
{
	int aibox_ch;
	int ret = -1;
	int rc;

	aibox_ch = _aibox_get_empty_ch(aibox_ip, ch);
	if(aibox_ch < 0){
		printf("[%s:%d] aibox_ip[%s] ch[%d] algorithm[%s] aibox_ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch, algorithm, aibox_ch);
		//ret = -1;
		//goto endl;
		aibox_ch = ch;
	}

	ret = _nf_api_aibox_set_video_stream(ch, aibox_ch, aibox_ip, algorithm);
endl:
	return ret;
}

static char *get_host_from_url(char *url, char *buf)
{
	static char buffer[200] = {0, };
	char *ptr;
	int len;
	int i;
	if(buf == NULL) buf = buffer;

	ptr = strstr(url, "://");
	if(ptr != NULL) url = ptr+3;

	ptr = strstr(url, "@");
	if(ptr != NULL) url = ptr+1;

	ptr = strstr(url, "/");
	if(ptr != NULL){
		len = ptr-url;
	}else{
		len = strlen(url);
	}
	strncpy(buf, url, len);
	buf[len] = '\0';

	ptr = strstr(buf, ":");
	if(ptr != NULL){
		*ptr = '\0';
	}

	return buf;
}

unsigned int get_ipaddr_from_url(const char *url)
{
    char *entry;
    char *ptr;
    unsigned int ip_addr = 0;
    int buffer[200];

    if(url == NULL) return 0;

    entry = strstr(url, "//");
    if(entry == NULL){
        entry = url;
    }else{
        entry += 2;
    }
    snprintf(buffer, sizeof(buffer), "%s", str_null_to_blank(entry));

    entry = strstr(buffer, "/");
    if(entry) *entry = '\0';

    entry = strstr(buffer, "@");
    if(entry == NULL){
        entry = buffer;
    }else{
        entry += 1;
    }

    ptr = strstr(entry, ":");
    if(ptr) *ptr = '\0';


    printf("[%s:%d] jhyoo ipaddr[%s]\n", __func__, __LINE__, entry);

    return (inet_addr(entry));
}

int get_port_from_url(char *url)
{
	char buf[200] = {0, };
	int port = 0;
	char *ptr;
	int len;
	int i;

	ptr = strstr(url, "://");
	if(ptr != NULL) url = ptr+3;

	ptr = strstr(url, "@");
	if(ptr != NULL) url = ptr+1;

	ptr = strstr(url, "/");
	if(ptr != NULL){
		len = ptr-url;
	}else{
		len = strlen(url);
	}
	strncpy(buf, url, len);
	buf[len] = '\0';

	ptr = strstr(buf, ":");
	if(ptr != NULL){
		ptr++;
		port = atoi(ptr);
	}else{
		port = 554;
	}

	return port;
}

static aibox_ch_info *stream_list_parser(json_t *json)
{
	int i;
	char host_buf[100];
	const char *ai = NULL;
	const char *name = NULL;
	const char *url = NULL;
	const char *text = NULL;
	const char *algo_type = NULL;
	aibox_ch_info *ret = NULL;

	ai = json_string_value(json_object_get(json, "ai"));
	url = json_string_value(json_object_get(json, "url"));
	//if(url == NULL) goto err;
	//if(url[0] == '\0') goto err;

	ret = (aibox_ch_info *)malloc(sizeof(aibox_ch_info));
	if(ret == NULL) goto err;

	memset(ret, 0, sizeof(aibox_ch_info));
	memset(host_buf, 0, sizeof(host_buf));
	
	name = json_string_value(json_object_get(json, "name"));
	strncpy(ret->name, str_null_to_blank(name), sizeof(ret->name) - 1);


	ret->no = (int)json_integer_value(json_object_get(json, "ch"));
	ret->ch = _get_chinfo_from_aibox_rtsp_url(url);
	_get_id_password_from_url(url, ret->id, ret->passwd);

	strncpy(ret->ai_algorithm_str, str_null_to_blank(ai), sizeof(ret->ai_algorithm_str) - 1);
	strncpy(ret->url, str_null_to_blank(url), sizeof(ret->url) - 1);
	strncpy(ret->ip, str_null_to_blank(get_host_from_url(url, host_buf)), sizeof(ret->ip) - 1);
	ret->rtsp_port = get_port_from_url(url);

	text = json_string_value(json_object_get(json, "text"));
	algo_type = json_string_value(json_object_get(json, "algo_type"));
	strncpy(ret->text, str_null_to_blank(text), sizeof(ret->text) - 1);
	strncpy(ret->algo_type, str_null_to_blank(algo_type), sizeof(ret->algo_type) - 1);

	return ret;
err:
	if(ret) aibox_ch_info_free(ret);
	return NULL;
}

void nf_api_stream_list_free(GList *list)
{
	if(list == NULL) return;
	_g_list_free_full(list, (GDestroyNotify)aibox_ch_info_free);
}

void aibox_ch_info_free(aibox_ch_info *info)
{
	if(info){
		free(info);
	}
}

int ch_conv_nvr_to_aibox(unsigned int aibox_ip, int nvr_ch)
{
	int rc;
	int i;
	int ret = -1;
	int aibox_ch_map[AVAILABLE_MAX_CH];

	rc = get_aibox_ch_map(aibox_ip, aibox_ch_map);
	if(rc < 0){
		ret = -3;
		goto endl;
	}

	for(i = 0; i < AVAILABLE_MAX_CH; i++){
		if(aibox_ch_map[i] == nvr_ch){
			ret = i;
			break;
		}
	}
endl:
	return ret;
}

GList *nf_api_get_stream_list(unsigned int aibox_ip)
{
	GList *ret = NULL;
	int rc = 0;
	char date[100];
	char stream_buf[300];
	char aibox_owner[30];
	char nvr_owner[30];
	char query[100];
	char *hash = NULL;
	int i;
	int len;

	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;

	http_init(&ctx);

    //API Owner 
	if(_get_aibox_owner(aibox_ip, aibox_owner, date) < 0){
		//ret = DLVA_API_RET_FAILED_INIT;
		printf("[%s:%d] _get_aibox_owner failed\n", __func__, __LINE__);
		goto endl;
	}
	nf_api_get_nvr_owner(nvr_owner);

    //API get stream list
	hash = _get_aibox_hash(NULL, date);
	sprintf(query, "owner=%s", nvr_owner);
	

	//http set api
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/analytics/");
	http_data_set(&ctx, HTTP_SET_QUERY, query);
	http_data_set(&ctx, HTTP_ADD_HEADER , "Date", date);
	http_data_set(&ctx, HTTP_ADD_HEADER , "X-Auth-Signature", hash);
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] timeout failed\n", __func__, __LINE__);
		goto endl;
	}

	//printf("[%s:%d] status[%d][%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);

	if(ctx.status != 200){
		//ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
		printf("[%s:%d] ctx.status[%d]\n", __func__, __LINE__, ctx.status);
		goto endl;
	}

	json = json_loads(ctx.res_data.memory, 0, &error);
	if(json == NULL){
		fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
		fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		goto endl;
	}

	len = json_array_size(json);

	for(i = 0; i < len; i++){
		aibox_ch_info *info = NULL;
		info = stream_list_parser(json_array_get(json, i));
		if(info){
			if(strlen(info->url) <= 0){
				free(info);
				continue;
			}
			info->aibox_ip = aibox_ip;
			strncpy(info->owner, aibox_owner, sizeof(info->owner) - 1);
			ret = g_list_append(ret, (gpointer)info);
		}
	}
endl:
	if(hash) free(hash);
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

void nf_api_capabilities_free(ai_capa_t *capa)
{
	int i;
	if(capa == NULL) return;
	for(i = 0; i < AIBOX_MAX_CHANNEL; i++){
		if(capa[i].algorithm_list) free(capa[i].algorithm_list);
	}
	free(capa);
}

void nf_api_capability_free(ai_capa_t *capa)
{
	if(capa == NULL) return;
	if(capa->algorithm_list) free(capa->algorithm_list);
	free(capa);
}
void nf_api_algorithm_list_free(GList *list)
{
	if(list)
		_g_list_free_full(list, (GDestroyNotify)aibox_algorithm_info_free);
}
void aibox_algorithm_info_free(aibox_algorithm_info *info)
{
	if(info){
		free(info);
	}
}


int algorithm_list_parser(json_t *json, aibox_algorithm_info *ret)
{
	const char *value = NULL;
	const char *text = NULL;
	const char *algo_type = NULL;


	if(ret == NULL) goto err;
	memset(ret, 0, sizeof(aibox_algorithm_info));

	value = json_string_value(json_object_get(json, "value"));
	text = json_string_value(json_object_get(json, "text"));
	algo_type = json_string_value(json_object_get(json, "algo_type"));

    //setting
	ret->disabled = json_is_true(json_object_get(json, "disabled"));/*json_boolean_value*/
	strncpy(ret->value, str_null_to_blank(value), sizeof(ret->value) - 1);
	strncpy(ret->text, str_null_to_blank(text), sizeof(ret->text) - 1);
	strncpy(ret->algo_type, str_null_to_blank(algo_type), sizeof(ret->algo_type) - 1);

	json = json_object_get(json, "resolution");
    if(json){
        json = json_object_get(json, "min");
        if(json_array_size(json) >= 2){
            ret->resolution_min_width  = json_integer_value(json_array_get(json, 0));
            ret->resolution_min_height = json_integer_value(json_array_get(json, 1));
        }
    }

	return 0;
err:
	return -1;
}

static int _get_aibox_capability(unsigned int aibox_ip, int ch, char *date, ai_capa_t **capa)
{
	int ret = DLVA_API_RET_FAILED_INIT;
	int rc = 0;
	char query_string[2048];
	int len = 0;
	int i;
    int list_index;
	HTTP_CTX ctx;
	json_t *json = NULL;
	json_t *json_array = NULL;
    json_error_t error;
	const char *owner = NULL;
	
	sprintf(query_string, "category=ai.algorithm&ch=%d&lang=en", ch);

	if(capa == NULL){
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto err;
	}

	if(aibox_ip == 0 || aibox_ip == 0xffffffff){
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto err;
	}

	(*capa) = NULL;

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/capability/");
	http_data_set(&ctx, HTTP_SET_QUERY, query_string);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto err;
	}

	if(ctx.status != 200){
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
	   	goto err;
	}

	if(date != NULL){
		//printf("[%s:%d] ctx.nf_api_get_capability[%s]\n", __func__, __LINE__, ctx.http_header_date);
		strncpy(date, ctx.http_header_date, strlen(ctx.http_header_date));
	}

	//parse request data
	
	ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
	json = json_loads(ctx.res_data.memory, 0, &error);
	if(json == NULL){
		fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
		fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		goto err;
	}

	(*capa) = (ai_capa_t *)malloc(sizeof(ai_capa_t));
	memset((*capa), 0, sizeof(ai_capa_t));

	(*capa)->ch = json_integer_value(json_object_get(json, "ch"));

	owner = json_string_value(json_object_get(json, "owner"));
	if(owner)
	strncpy((*capa)->owner, owner, sizeof((*capa)->owner));

	json_array = json_object_get(json, "api");
	if(!json_array) goto err;
	
	json_array = json_object_get(json_array, "params");
	if(!json_array) goto err;

	json_array = json_object_get(json_array, "ai");
	if(!json_array) goto err;

	json_array = json_object_get(json_array, "options");
	if(!json_array) goto err;

	len = json_array_size(json_array);
	if(len <= 0) goto err; 

	//(*capa)->algorithm_count = len;

	(*capa)->algorithm_list = malloc(sizeof(aibox_algorithm_info) * len);
	if((*capa)->algorithm_list == NULL) goto err;

	for(i = 0, list_index = 0; i < len; i++){
		if(algorithm_list_parser(json_array_get(json_array, i), &((*capa)->algorithm_list[list_index++])) < 0){
            list_index--;
        }
	}
	ret = DLVA_API_RET_OK;
	(*capa)->algorithm_count = list_index;

    if(list_index == 0 && (*capa)->algorithm_list != NULL){
        free((*capa)->algorithm_list);
        (*capa)->algorithm_list = NULL;
    }

	if(json != NULL) json_decref(json);
	http_release(&ctx);
	return ret;
err:
	if((capa != NULL) && (*capa != NULL)){
	   	nf_api_capability_free(*capa);
		*capa = NULL;
	}
	if(json != NULL) json_decref(json);
	http_release(&ctx);
	return ret;
}

static int capability_parser(ai_capa_t *capa, json_t *json)
{
	int i;
	int len = 0;
    int list_index;
	int ret = -1;
	const char *owner = NULL;
	json_t *json_array = NULL;
    json_error_t error;

	capa->ch = json_integer_value(json_object_get(json, "ch"));

	owner = json_string_value(json_object_get(json, "owner"));
	if(owner)
	strncpy(capa->owner, owner, sizeof(capa->owner));

	json_array = json_object_get(json, "api");
	if(!json_array) goto err;
	
	json_array = json_object_get(json_array, "params");
	if(!json_array) goto err;

	json_array = json_object_get(json_array, "ai");
	if(!json_array) goto err;

	json_array = json_object_get(json_array, "options");
	if(!json_array) goto err;

	len = json_array_size(json_array);
	if(len <= 0) goto err; 

	//capa->algorithm_count = len;

	capa->algorithm_list = malloc(sizeof(aibox_algorithm_info) * len);
	if(capa->algorithm_list == NULL){
		capa->algorithm_count = 0;
	   	goto err;
	}

	for(i = 0, list_index = 0; i < len; i++){
		if(algorithm_list_parser(json_array_get(json_array, i), &(capa->algorithm_list[list_index++])) < 0){
            list_index--;
        }
	}
	capa->algorithm_count = list_index;

    if(list_index == 0 && capa->algorithm_list != NULL){
        free(capa->algorithm_list);
        capa->algorithm_list = NULL;
    }

	ret = 0;
err:

	return ret;
}

static int _get_aibox_capabilities(unsigned int aibox_ip, char *date, ai_capa_t **capa)
{
	//ai_capa_t *ret = NULL;
	int ret = DLVA_API_RET_FAILED_INIT;
	int rc = 0;
	char query_string[2048];
	int i;
	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;
	const char *owner = NULL;
	size_t len = 0;
	
	sprintf(query_string, "category=ai.algorithm&lang=en");

	if(capa == NULL){
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto err;
	}

	(*capa) = NULL;

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/capability/");
	http_data_set(&ctx, HTTP_SET_QUERY, query_string);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto err;
	}

	if(ctx.status != 200){
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
	   	goto err;
	}

	if(date != NULL){
		//printf("[%s:%d] ctx.nf_api_get_capability[%s]\n", __func__, __LINE__, ctx.http_header_date);
		strncpy(date, ctx.http_header_date, strlen(ctx.http_header_date));
	}

	//parse request data
	
	ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
	json = json_loads(ctx.res_data.memory, 0, &error);
	if(json == NULL){
		fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
		fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		goto err;
	}

	(*capa) = (ai_capa_t *)malloc(sizeof(ai_capa_t) * AIBOX_MAX_CHANNEL);
	memset((*capa), 0, sizeof(ai_capa_t) * AIBOX_MAX_CHANNEL);

	len = json_array_size(json);
	if(len > AIBOX_MAX_CHANNEL) len = AIBOX_MAX_CHANNEL;

	for(i = 0; i < len; i++){
		capability_parser(*capa+i, json_array_get(json, i));
	}

	ret = DLVA_API_RET_OK;
	if(json != NULL) json_decref(json);
	http_release(&ctx);
	return ret;
err:
	if((capa != NULL) && (*capa != NULL)) nf_api_capabilities_free(*capa);
	if(json != NULL) json_decref(json);
	http_release(&ctx);
	return ret;
}

ai_capa_t *nf_api_get_aicam_capability(int ch)
{
	int rc;
	ai_capa_t *ret = NULL;

	rc = _get_aicam_capability(ch, &ret);

	if(rc != DLVA_API_RET_OK){
		printf("[%s:%d] _get_aicam_capability[%d] ai_capa_t[%p]\n", __func__, __LINE__, rc, ret);
	}

	return ret;
}

ai_capa_t *nf_api_get_aibox_capability(unsigned int aibox_ip, int ch)
{
	int rc;
	ai_capa_t *ret = NULL;

	rc = _get_aibox_capability(aibox_ip, ch, NULL, &ret);

	if(rc != DLVA_API_RET_OK){
		printf("[%s:%d] _get_aibox_capability[%d] ai_capa_t[%p]\n", __func__, __LINE__, rc, ret);
	}

	return ret;
}

const char *get_json_form_string(json_t *json, const char *fmt, ...)
{
	const char *ret = NULL;
	char *key = NULL;
	int alloc_size = 0;
	int len = 0;
	va_list arg;

	if(json == NULL) return NULL;
	if(fmt == NULL) return NULL;
	len = strlen(fmt);
	if(len == 0) return NULL;

	alloc_size = len + 20;
	key = (char *)malloc(alloc_size);
	if(key == NULL) return NULL;

	va_start(arg, fmt);
	vsnprintf(key, alloc_size, fmt, arg);
	va_end(arg);
	

	ret = json_string_value(json_object_get(json, key));
	return ret;
}


static int _get_aicam_capability(int ch, ai_capa_t **capa)
{
	mtable *runtime = get_runtime();
    int count = 0;
	int i;
	int ret = DLVA_API_RET_FAILED_INIT;
	
	//arg error check
	if(ch < 0 || ch >= AVAILABLE_MAX_CH){
		printf("[%s:%d] ch[%d] channel range error\n", __func__, __LINE__, ch);
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto endl;
	}
	
	if(capa == NULL){
		printf("[%s:%d] ch[%d] capability buffer is not set capa[%p]\n", __func__, __LINE__, ch, capa);
		ret = DLVA_API_RET_ARGUMENT_ERROR;	
		goto endl;
	}

    for(i = 0; i < runtime[ch].ai.license_length; i++){
        /*
        printf("[%s:%d] debug i[%02d] | license[%s] req[%d]ins[%d] value[%s]name[%s]\n", 
                __func__, __LINE__, i,
                runtime[ch].ai.license[i].license_name,
                runtime[ch].ai.license[i].license_required,
                runtime[ch].ai.license[i].license_installed,
                runtime[ch].ai.license[i].value,
                runtime[ch].ai.license[i].name
              );
              */

        if(runtime[ch].ai.license[i].license_required == 0){
            count++;
        }else
        if(runtime[ch].ai.license[i].license_required * runtime[ch].ai.license[i].license_installed) {
            count++;
		}
	}

    printf("[%s:%d] debug count[%d]\n", __func__, __LINE__, count);

    if(count <= 0){
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
	}

	//init
	(*capa) = NULL;
	(*capa) = (ai_capa_t *)malloc(sizeof(ai_capa_t));
	memset((*capa), 0, sizeof(ai_capa_t));

    (*capa)->ch= ch;
    (*capa)->algorithm_count = count;
    (*capa)->algorithm_list = malloc(sizeof(aibox_algorithm_info) * count);
    memset((*capa)->algorithm_list, 0, (sizeof(aibox_algorithm_info) * count));


	if((*capa)->algorithm_list == NULL){
		printf("[%s:%d] memory allocation failed\n", __func__, __LINE__);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
	   	goto err;
	}
	memset((*capa)->algorithm_list, 0, sizeof(aibox_algorithm_info) * count);

    for(i = 0, count = 0; i < runtime[ch].ai.license_length; i++){
        /*
        printf("[%s:%d] debug i[%02d] | license[%s] req[%d]ins[%d] value[%s]name[%s]\n", 
                __func__, __LINE__, i,
                runtime[ch].ai.license[i].license_name,
                runtime[ch].ai.license[i].license_required,
                runtime[ch].ai.license[i].license_installed,
                runtime[ch].ai.license[i].value,
                runtime[ch].ai.license[i].name
              );
              */

        if((runtime[ch].ai.license[i].license_required == 0) ||
          (runtime[ch].ai.license[i].license_required * runtime[ch].ai.license[i].license_installed)){
            snprintf((*capa)->algorithm_list[count].value, 50, runtime[ch].ai.license[i].value);
            snprintf((*capa)->algorithm_list[count].text, 100, runtime[ch].ai.license[i].name);
            snprintf((*capa)->algorithm_list[count].algo_type, 50, runtime[ch].ai.license[i].algo_type);

            count++;

            if(count >= runtime[ch].ai.license_length){
                //printf("[%s:%d] debug count[%d]\n", __func__, __LINE__, count);
                break;
            }
        }
    }

    if(count == runtime[ch].ai.license_length){
        ret = DLVA_API_RET_OK;
    }else{
        printf("[%s:%d] err count[%d] length[%d]\n", __func__, __LINE__, count, runtime[ch].ai.license_length);
    }

endl:
	return ret;
err:
	if((capa != NULL) && (*capa != NULL)){
	   	nf_api_capability_free(*capa);
		*capa = NULL;
	}
    goto endl;
}

ai_capa_t *nf_api_get_capability(unsigned int aibox_ip, int ch)
{
	int rc = 0;
	ai_capa_t *ret = NULL;

	if(aibox_ip == 0){
		//ai camera
		rc = _get_aicam_capability(ch, &ret);
        //rc = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
	}else{
		rc = _get_aibox_capability(aibox_ip, ch, NULL, &ret);
	}

	if(rc != DLVA_API_RET_OK){
		printf("[%s:%d] _get_aibox_capability[%d] ai_capa_t[%p]\n", __func__, __LINE__, rc, ret);
	}

	return ret;
}

ai_capa_t *nf_api_get_aibox_capabilities(unsigned int aibox_ip)
{
	ai_capa_t *ret = NULL;
	//AVAILABLE_MAX_CH
	_get_aibox_capabilities(aibox_ip, NULL, &ret);
	return ret;
}

ai_capa_t *nf_api_get_capabilities(unsigned int aibox_ip)
{
	ai_capa_t *ret = NULL;
	//AVAILABLE_MAX_CH
	_get_aibox_capabilities(aibox_ip, NULL, &ret);
	return ret;
}

ai_capa_t *get_ai_capa_mac(GList *list, const char *mac)
{
	GList *iter;

	if(mac == NULL) return NULL;

	for(iter = list; iter != NULL; iter = iter->next){
		ai_capa_t *capa = (ai_capa_t *)iter->data;
		if(memcmp(capa->mac, mac, 6) == 0){
			return capa;
		}
	}

	return NULL;
}

static int is_host_ip_uint(host_inf *list, unsigned int ip)
{
	host_inf *current;

    if(ip == 0x0 || ip == 0xffffffff){
        return 0;
    }

	for(current = list; current != NULL; current = current->next)
	{
		if(current->ipaddr == ip){
			return 1;
		}
	}
	return 0;
}

static int is_host_ip(host_inf *list, char *ipstr)
{
	if(ipstr == NULL) return 0;

    return is_host_ip_uint(list, htonl(inet_addr(ipstr)));
}


void set_ai_connection_info(ai_connection_info *info, unsigned int aibox_ip, const char *mac, const char *algorithm)
{
	if(info == NULL) return;
	if(mac == NULL) return;

	info->aibox_ip = aibox_ip;
	memcpy(info->mac, mac, 6);
	strncpy(info->algorithm, str_null_to_blank(algorithm), sizeof(info->algorithm));
}

int nf_api_aibox_server_state_is_alive(unsigned int aibox_ip, int connection_timeout)
{
	HTTP_CTX ctx;
	json_t *json = NULL;
    json_error_t error;
	const char *owner_ptr = NULL;
	int ret = DLVA_API_RET_OK;
	int rc = 0;


	http_init(&ctx);
	if(aibox_ip == 0 || aibox_ip == 0xffffffff){
	   ret = DLVA_API_RET_ARGUMENT_ERROR;
	   goto endl;
	}

	aibox_http_default_setting(&ctx, aibox_ip);
	if(connection_timeout > 0)
		http_data_set(&ctx, HTTP_SET_CONN_TIME_OUT	, connection_timeout);
	http_data_set(&ctx, HTTP_SET_PATH, "/itx/ai/owner/");

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	if(ctx.status != 200){
		ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
	   	goto endl;
	}
endl:
	http_release(&ctx);
    g_message("[%s, %d] ret : %d", __FUNCTION__, __LINE__, ret);
    
	if(ret == DLVA_API_RET_OK) return 1;
	return 0;
}

int nf_api_aibox_connection_is_alive(unsigned int ch, unsigned int aibox_ip)
{
	int ret = 0;
    GList *iter;
	char owner[20] = {0, };

	nf_api_get_nvr_owner(owner);

	if(owner == NULL) goto endl;
	if(owner[0] == '\0') goto endl;

	GList *aibox_stream_list = nf_api_get_stream_list(aibox_ip);

	for(iter = aibox_stream_list; iter != NULL; iter = iter->next){
		aibox_ch_info *ch_info = (aibox_ch_info *)iter->data;
		if(ch_info == NULL) continue;
		if(ch_info->ch != ch) continue;

		if(ch_info->name != NULL){
			if(strncmp(owner, ch_info->name, strlen(owner)) == 0){
				ret = 1;
			}
		}

		if(ch_info->ip != NULL){
			if(is_nvr_ip(ch_info->ip)){
				ret = 1;
				break;
			}
		}
	}

endl:
	nf_api_stream_list_free(aibox_stream_list);
	return ret;
}


const ai_license_data *nf_api_selected_aicamera_algorithm_data(int ch)
{
    static ai_license_data blank_ret_data = {0, };
	mtable *runtime = get_runtime();
    int i;

    for(i = 0; i < runtime[ch].ai.license_length; i++){
        if(runtime[ch].ai.license[i].license_required == 1 && runtime[ch].ai.license[i].license_installed == 0) continue;
        return &(runtime[ch].ai.license[i]);
    }

    return &blank_ret_data;
}

int nf_api_selected_aicamera_algorithm(int ch, char *value)
{
	int ret = -1;
	if(value == NULL) return -1;

    strcpy(value, nf_api_selected_aicamera_algorithm_data(ch)->algo_type);

	return 0;
}

int nf_api_selected_aibox_algorithm(int ch, unsigned int aibox_ip, char *value)
{
	//printf("[%s:%d] ch[%d] aibox_ip[%s] value[%p]\n", __func__, __LINE__, ch, _ip_to_str(aibox_ip, NULL), value);
	int ret = -1;
    GList *iter;
	char owner[20] = {0, };

	nf_api_get_nvr_owner(owner);

	if(owner == NULL){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}
	if(owner[0] == '\0'){
		ret = DLVA_API_RET_FAILED_INIT;
		goto endl;
	}

	GList *aibox_stream_list = nf_api_get_stream_list(aibox_ip);
	if(aibox_stream_list == NULL){
		ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
		goto endl;
	}

	ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
	for(iter = aibox_stream_list; iter != NULL; iter = iter->next){
		aibox_ch_info *ch_info = (aibox_ch_info *)iter->data;
		if(ch_info == NULL) continue;
		//printf("[%s:%d] name[%s] url[%s] ip[%s]\n", __func__, __LINE__, ch_info->name, ch_info->url, ch_info->ip);
		if(ch_info->ch != ch) continue;

		if(ch_info->name != NULL){
			if(strncmp(owner, ch_info->name, strlen(owner)) == 0){
				strcpy(value, ch_info->ai_algorithm_str); 
				ret = DLVA_API_RET_OK;
				break;
			}
		}
		if(ch_info->ip != NULL){
			if(is_nvr_ip(ch_info->ip)){
				strcpy(value, str_null_to_blank(ch_info->ai_algorithm_str)); 
				ret = DLVA_API_RET_OK;
				break;
			}
		}
	}

endl:
	nf_api_stream_list_free(aibox_stream_list);
	return ret;
}

ai_connection_info *get_ai_connection_info(aibox_search_list *list)
{
	return nf_api_get_ai_connection_info(list);
}

ai_connection_info *nf_api_get_ai_connection_info(aibox_search_list *list)
{
    GList *iter;
	char owner[20] = {0, };
	ai_connection_info *ret = NULL;
	int length = 0;

	GValue ret_value = {0,};
	char nvr_id[65];
	char nvr_pw[65];
	int nvr_rtsp_port = 0;

	//if(list_length == NULL) goto endl;
	if(list == NULL) goto endl;
	if(list->list == NULL) goto endl;
	if(g_list_length(list->list) == 0) goto endl;

	nf_api_get_nvr_owner(owner);

	if(owner == NULL) goto endl;
	if(owner[0] == '\0') goto endl;

	length = AVAILABLE_MAX_CH;
	if(length <= 0) goto endl;

	//return list
	ret = malloc(sizeof(ai_connection_info)*length);
	if(ret == NULL) goto endl;
	memset(ret, 0, sizeof(ai_connection_info)*length);

	//get NVR DB
	memset(nvr_id, 0, sizeof(nvr_id));
	memset(nvr_pw, 0, sizeof(nvr_pw));

	//user
	if(nf_sysdb_get_key1("usr.U%d.name", 0, &ret_value, NULL))
	{
		strncpy(nvr_id, g_value_get_string(&ret_value), sizeof(nvr_id) - 1);
		g_value_unset(&ret_value);
	}

	//pass
	if(nf_sysdb_get_key1("usr.U%d.pass", 0, &ret_value, NULL))
	{
		strncpy(nvr_pw, g_value_get_string(&ret_value), sizeof(nvr_pw) - 1);
		g_value_unset(&ret_value);
	}

	//nvr_rtsp_port
	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		nvr_rtsp_port = (int)g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	for(iter = list->list; iter != NULL; iter = iter->next){
		aibox_connection_info *aibox_info = (aibox_connection_info *)(iter->data);
		int ip = aibox_connection_info_get_ip(aibox_info);

		//get streamlist
		GList *aibox_stream_list = nf_api_get_stream_list(ip);
		GList *stream_iter;
		for(stream_iter = aibox_stream_list; stream_iter != NULL; stream_iter = stream_iter->next){
			aibox_ch_info *ch_info = (aibox_ch_info *)stream_iter->data;
			if(ch_info == NULL) continue;
			if(ch_info->ch >= AVAILABLE_MAX_CH || ch_info->ch < 0) continue;
			/*
			if(ch_info->name != NULL){
				if(strncmp(owner, ch_info->name, strlen(owner)) == 0){
					set_ai_connection_info(&ret[ch_info->ch], ip, aibox_info->mac, ch_info->ai_algorithm_str);
					continue;
				}
			}
			if(ch_info->ip != NULL){
				if(is_host_ip(host_inf_list, ch_info->ip)){
					set_ai_connection_info(&ret[ch_info->ch], ip, aibox_info->mac, ch_info->ai_algorithm_str);
					continue;
				}
			}
			*/

			if(strncmp(owner, ch_info->name, strlen(owner)) != 0 && !is_nvr_ip(ch_info->ip)){
				if(strncmp(owner, ch_info->owner, strlen(owner)) == 0){
					//url reset
					nf_api_aibox_set_video_stream(ch_info->ch, ip, ch_info->ai_algorithm_str);
				}else{
					continue;
				}
			}

			//ID/PW/PORT CHECK
			if(strncmp(nvr_id, ch_info->id, strlen(nvr_id)) != 0 ||
				strncmp(nvr_pw, ch_info->passwd, strlen(nvr_pw)) != 0 ||
				nvr_rtsp_port != ch_info->rtsp_port)
			{
				if(strncmp(owner, ch_info->owner, strlen(owner)) == 0){
					//url reset
					nf_api_aibox_set_video_stream(ch_info->ch, ip, ch_info->ai_algorithm_str);
				}else{
					continue;
				}
			}
			set_ai_connection_info(&ret[ch_info->ch], ip, aibox_info->mac, ch_info->ai_algorithm_str);
		}
		nf_api_stream_list_free(aibox_stream_list);
	}

endl:
	return ret;
}

static aibox_algorithm_info *_get_algorithm_info(char *key, ai_capa_t *capa)
{
	int i;
	if(key == NULL || capa == NULL) return NULL;

	for(i = 0; i < capa->algorithm_count; i++){
		if(strcmp(capa->algorithm_list[i].value, key) == 0){
			return &capa->algorithm_list[i];
		}
	}

	return NULL;
}

static const char *_get_algorithm_type(char *key, ai_capa_t *capa)
{
	int i;
	if(key == NULL || capa == NULL) return "";

	for(i = 0; i < capa->algorithm_count; i++){
		if(strcmp(capa->algorithm_list[i].value, key) == 0){
			return capa->algorithm_list[i].algo_type;
		}
	}

	return "";
}

static const char *_get_algorithm_text(char *key, ai_capa_t *capa)
{
	int i;
	if(key == NULL || capa == NULL) return "";

	for(i = 0; i < capa->algorithm_count; i++){
		if(strcmp(capa->algorithm_list[i].value, key) == 0){
			return capa->algorithm_list[i].text;
		}
	}

	return "";
}

aibox_algorithm_name *nf_api_get_algorithm_data()
{
		int ch;
		const ai_license_data *aicam_algorithm_data = NULL;
		aibox_algorithm_name *ret = NULL;
		aibox_algorithm_name *aibox_algorithm = NULL;
		char key_value[64];
		int devcam, devbox;

		aibox_algorithm = nf_api_get_algorithms_all_ch(0);

		if(aibox_algorithm){
			ret = aibox_algorithm;
		}else{
			ret = malloc(sizeof(aibox_algorithm_name)*AVAILABLE_MAX_CH);
			if(ret == NULL){
				printf("[%s:%d] malloc fail size[%d*%d]\n", __func__, __LINE__, sizeof(aibox_algorithm_name), AVAILABLE_MAX_CH);
				goto endl;
			}
			memset(ret, 0, sizeof(aibox_algorithm_name) * AVAILABLE_MAX_CH);
		}


		for(ch = 0; ch < AVAILABLE_MAX_CH; ch++){
			snprintf(key_value, 64, "cam.dvabx.cfg.R%d.devcam", ch);
			devcam = nf_sysdb_get_bool(key_value);

			snprintf(key_value, 64, "cam.dvabx.cfg.R%d.devbox", ch);
			devbox = nf_sysdb_get_bool(key_value);

			if(devbox){
				/*aibox 값 그대로 사용함*/
			}else if(devcam){
				aicam_algorithm_data = nf_api_selected_aicamera_algorithm_data(ch);
				if(aicam_algorithm_data){
					snprintf(aibox_algorithm[ch].value     , sizeof(aibox_algorithm[ch].value    ), "%s", str_null_to_blank(aicam_algorithm_data->value));
					snprintf(aibox_algorithm[ch].text      , sizeof(aibox_algorithm[ch].text     ), "%s", str_null_to_blank(aicam_algorithm_data->name));
					snprintf(aibox_algorithm[ch].algo_type , sizeof(aibox_algorithm[ch].algo_type), "%s", str_null_to_blank(aicam_algorithm_data->algo_type));
				}else{
					aibox_algorithm[ch].value[0] = '\0';
					aibox_algorithm[ch].text[0] = '\0';
					aibox_algorithm[ch].algo_type[0] = '\0';
				}
			}else{
				aibox_algorithm[ch].value[0] = '\0';
				aibox_algorithm[ch].text[0] = '\0';
				aibox_algorithm[ch].algo_type[0] = '\0';
			}
		}

endl:
	return ret;

err:
	free(ret);
	goto endl;
}



aibox_algorithm_name *nf_api_get_algorithms_all_ch(unsigned int aibox_ip)
{
	int size = 0;
	int i;

	char nvr_owner[20];
	char none_string[100] = {0, };
	unsigned int aibox_list[20] = {0,};

	aibox_algorithm_name *ret = NULL;
	ai_capa_t *capa = NULL;
    const char *algo_type;

	GList *aibox_stream_list = NULL;
    GList *iter;


	printf("[%s:%d] ch size %d aibox_ip[%s]\n", __func__, __LINE__, AVAILABLE_MAX_CH, (aibox_ip) ? _ip_to_str(aibox_ip, NULL) : "ALL");
	ret = malloc(sizeof(aibox_algorithm_name)*AVAILABLE_MAX_CH);
	if(ret == NULL){
		printf("[%s:%d] malloc fail size[%d*%d]\n", __func__, __LINE__, sizeof(aibox_algorithm_name), AVAILABLE_MAX_CH);
		return NULL;
	}

	memset(ret, 0, sizeof(aibox_algorithm_name) * AVAILABLE_MAX_CH);
	memset(nvr_owner, 0, sizeof(nvr_owner));

	if(nf_api_get_nvr_owner(nvr_owner) == NULL){
		free(ret);
		return NULL;
	}

	//get aibox list
	if(aibox_ip != 0){
		size = 1;
		aibox_list[0] = aibox_ip;
	}else{
		size = nf_api_aibox_get_aibox_iplist(aibox_list);
	}

	for(i = 0; i < size; i++){
		printf("[%s:%d] index[%d] aibox_ip[%s]\n", __func__, __LINE__, i, _ip_to_str(aibox_list[i], NULL));
		capa = nf_api_get_capability(aibox_list[i], 0);
		if(capa == NULL) continue;

		aibox_stream_list = nf_api_get_stream_list(aibox_list[i]);
		for(iter = aibox_stream_list; iter != NULL; iter = iter->next){
			aibox_ch_info *ch_info = (aibox_ch_info *)iter->data;
			if(ch_info == NULL) continue;

			if(strncmp(nvr_owner, ch_info->name, strlen(nvr_owner)) != 0 &&
					!is_nvr_ip(ch_info->ip)){
				continue;
			}

			if(ch_info->ch >= AVAILABLE_MAX_CH || ch_info->ch < 0){
				continue;
			}

            sprintf(ch_info->algo_type, "");

            if(strlen(ch_info->algo_type) > 0){
                algo_type = ch_info->algo_type;
            }else{
                algo_type = _get_algorithm_type(ch_info->ai_algorithm_str, capa);
            }

			strncpy(ret[ch_info->ch].value, ch_info->ai_algorithm_str, 49);
            snprintf(ret[ch_info->ch].algo_type, 49, "%s", algo_type);

            printf("[%s:%d] algo_type[%s][%p]  ret[ch_info->ch].algo_type[%s]\n", __func__, __LINE__, algo_type, algo_type, ret[ch_info->ch].algo_type);

            if(strlen(ch_info->text) > 0){
                strncpy(ret[ch_info->ch].text, ch_info->text, strlen(ch_info->text));
            }else{
                strncpy(ret[ch_info->ch].text, _get_algorithm_text(ch_info->ai_algorithm_str, capa), 99);
            }
            printf("[%s:%d] ret[%d] ch[%d] algo_type[%s][%d] key[%s] text[%s][%d]\n", __func__, __LINE__, i, ch_info->ch, ret[ch_info->ch].algo_type, strlen(ch_info->algo_type), ret[ch_info->ch].value, ret[ch_info->ch].text, strlen(ch_info->text));
		}
		
		strncpy(none_string, _get_algorithm_text("", capa), 49);
		if(aibox_stream_list){
			nf_api_stream_list_free(aibox_stream_list);
			aibox_stream_list = NULL;
		}
		if(capa){
			nf_api_capability_free(capa);
			capa = NULL;
		}
	}

	for(i = 0; i < AVAILABLE_MAX_CH; i++){
		if(ret[i].value[0] == 0){
			strcpy(ret[i].text, none_string);
		}
	}

endl:
	if(aibox_stream_list) nf_api_stream_list_free(aibox_stream_list);
	if(capa) nf_api_capability_free(capa);

	return ret;
}

aibox_algorithm_name *nf_api_get_aibox_algorithm_names(unsigned int aibox_ip, int *length)
{
	aibox_algorithm_name *ret = NULL;
	ai_capa_t *capa = NULL;
	int i;
	if(length == NULL){
		printf("%s length arg null\n", __func__);
		goto endl;
	}
	*length = 0;

	capa = nf_api_get_capability(aibox_ip, 0);
	if(capa == NULL){
		printf("%s _get_aibox_capability failed aibox_ip[%s]\n", __func__, _ip_to_str(aibox_ip, NULL));
		goto endl;
	}
	if(capa->algorithm_count == 0){
		printf("%s capa->algorithm_count[%d]\n", __func__, capa->algorithm_count);
		goto endl;
	}

	ret = malloc(sizeof(aibox_algorithm_name)*capa->algorithm_count);
	if(ret == NULL) goto endl;

	*length = capa->algorithm_count;
	memset(ret, 0, sizeof(aibox_algorithm_name)*capa->algorithm_count);

	for(i = 0; i < capa->algorithm_count; i++){
		strncpy(ret[i].value, str_null_to_blank(capa->algorithm_list[i].value), sizeof(ret[i].value) - 1);
		strncpy(ret[i].text, str_null_to_blank(capa->algorithm_list[i].text), sizeof(ret[i].text) - 1);
		strncpy(ret[i].algo_type, str_null_to_blank(capa->algorithm_list[i].algo_type), sizeof(ret[i].text) - 1);
		//printf("[%s] [%s]\n", ret[i].value, ret[i].text);
	}

endl:
	if(capa) nf_api_capability_free(capa);
	return ret;
}

int nf_api_aibox_update_video_stream(unsigned int aibox_ip, unsigned int change_info)
{
	char owner_nvr[20];
	char owner_aibox[20];
	int ret = 0;


	nf_api_get_nvr_owner(owner_nvr);
	if(nf_api_get_aibox_owner(aibox_ip, owner_aibox) == NULL){
		goto endl;
	}

	if(strcmp(owner_nvr, owner_aibox) != 0){
		goto endl;
	}

	ret = _aibox_update_video_stream(aibox_ip, change_info);
endl:
	return ret;
}

typedef enum _AIBOX_CHANGE_INFO_E
{
	AIBOX_CHANGE_INFO_IP = 1,
	AIBOX_CHANGE_INFO_PORT = 2,
	AIBOX_CHANGE_INFO_PASSWORD = 4
}AIBOX_CHANGE_INFO_E;



int _aibox_update_video_stream(unsigned int aibox_ip, unsigned int change_info)
{
	int nvr_rtsp_port;
	int rc;
	char nvr_passwd[50];
	GValue ret_value = {0,};
	GList *aibox_stream_list = NULL;
    GList *iter;

	//get_stream_list
	//aibox all channel check
		//if(diffrent) change request;
	
	//init check
	if(change_info == 0x00) goto endl;


	
	if(change_info & AIBOX_CHANGE_INFO_IP){
		if(host_inf_list == NULL){
			;
		}
	}

	//pass
	if(nf_sysdb_get_key1("usr.U%d.pass", 0, &ret_value, NULL))
	{
		g_stpcpy(nvr_passwd, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
	}

	//nvr_rtsp_port
	if(nf_sysdb_get_key0("net.rtp.rtspport", &ret_value, NULL))
	{
		nvr_rtsp_port = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
	}

	aibox_stream_list = nf_api_get_stream_list(aibox_ip);
	for(iter = aibox_stream_list; iter != NULL; iter = iter->next){
		aibox_ch_info *ch_info = (aibox_ch_info *)iter->data;

		if(change_info & AIBOX_CHANGE_INFO_IP){
            int host_ip = 0;
			/*
		   	시나리오상 aibox의 ip는 모두 내(NVR) stream을 대상으로 해야되기 때문에
		   	ip가 다르면 강제로 변경하도록 하였음
			*/
            pthread_mutex_lock(&_host_inf_mutex);
            host_ip = get_subnet_host_ip(get_host_info_list(), aibox_ip, is_ip_wan(aibox_ip));
            pthread_mutex_unlock(&_host_inf_mutex);

			if(ch_info->ip != NULL && (host_ip != htonl(inet_addr(ch_info->ip)))){
				//stream 재설정
				rc = nf_api_aibox_set_video_stream(ch_info->ch, aibox_ip, ch_info->ai_algorithm_str);
				//printf("[%s:%d] port changed result[%d]\n", __func__, __LINE__, rc);
				continue;
			}
		}
		if((change_info & AIBOX_CHANGE_INFO_PORT) && (nvr_rtsp_port != ch_info->rtsp_port)){
			//stream 재설정
			rc = nf_api_aibox_set_video_stream(ch_info->ch, aibox_ip, ch_info->ai_algorithm_str);
			//printf("[%s:%d] port changed result[%d]\n", __func__, __LINE__, rc);
			continue;
		}
		if((change_info & AIBOX_CHANGE_INFO_PASSWORD) && (strcmp(nvr_passwd, ch_info->passwd) != 0)){
			//stream 재설정
			rc = nf_api_aibox_set_video_stream(ch_info->ch, aibox_ip, ch_info->ai_algorithm_str);
			//printf("[%s:%d] port changed result[%d]\n", __func__, __LINE__, rc);
			continue;
		}
	}
endl:
	if(aibox_stream_list) nf_api_stream_list_free(aibox_stream_list);
	return 0;
}

int _aibox_ip_list_isset(unsigned int *list, int size, unsigned int ip){
	int i;
	for(i = 0; i < size; i++){
		printf("[%s:%d] check index[%d] ip[%08x] list_ip[%08x]\n", __func__, __LINE__, i, ip, list[i]);
		if(list[i] == ip) return 1;

	}
	return 0;
}

int nf_api_aibox_get_aibox_iplist(unsigned int *list)
{
	GValue ret_value = {0,};
	char db_mac[100];
	unsigned int db_addr;
	int size = 0;
	int i;

	if(list == NULL){
		goto endl;
	}

	for(i = 0; i < NUM_ACTIVE_CH; i++){
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
		//printf("[%s:%d]index[%d] addr[%s] mac[%s]\n", __func__, __LINE__, i, _ip_to_str(db_addr, NULL), db_mac);

		//if(db_addr err || db_mac err) continue;
		if(db_addr == 0x00 || db_addr == 0xffffffff) continue;
		if(db_mac[0] == '\0') continue;

		if(!_aibox_ip_list_isset(list, size, db_addr)){
			list[size++] = db_addr;
		}
	}
endl:
	return size;
}

int nf_api_aibox_update_video_stream_all(unsigned int change_info)
{
	int size = 0;
	unsigned int aibox_list[20] = {0,};
	int i;
	char owner_nvr[20];
	char owner_aibox[20];
	int ret = 0;

	printf("[%s:%d] change_info[%x]\n", __func__, __LINE__, change_info);
	if(change_info == 0x00) change_info = 0xff;

	nf_api_get_nvr_owner(owner_nvr);
	if(/*get_nvr_onwer error*/0){
		goto endl;
	}

	//get aibox list
	size = nf_api_aibox_get_aibox_iplist(aibox_list);
	if(/*get_aibox_list error*/0){
		goto endl;
	}

	//check change
	for(i = 0; i < size; i++){
		printf("[%s:%d] index[%d] ip[%s]\n", __func__, __LINE__, i, _ip_to_str(aibox_list[i], NULL));
		if(nf_api_get_aibox_owner(aibox_list[i], owner_aibox) == NULL) continue;
		if(strcmp(owner_nvr, owner_aibox) != 0) continue;
		ret += _aibox_update_video_stream(aibox_list[i], change_info);
	}

endl:
	return ret;
}


GThreadPool *aibox_work_pool = NULL;
typedef struct
{
    void (*func)(void *);
    void *data;
}cb_data;

gpointer aibox_process_cb_func(gpointer data, gpointer user_data)
{
    cb_data *cb = (cb_data *)data;
    if(cb == NULL) return NULL;
    if(cb->func == NULL) return NULL;

    cb->func(cb->data);
    free(cb);

    return NULL;
}

static void *aibox_url_changed(void *data)
{
    int type = (int)data;

    printf("[%s:%d] type[%d]\n", __func__, __LINE__, type);
    switch(type){
        case NF_SYSDB_CATE_NET:
            nf_api_aibox_update_video_stream_all(AIBOX_CHANGE_INFO_IP|AIBOX_CHANGE_INFO_PORT);
            break;
        case NF_SYSDB_CATE_USR:
            nf_api_aibox_update_video_stream_all(AIBOX_CHANGE_INFO_PASSWORD);
            break;
        case NF_SYSDB_CATE_CAM:
            //nf_api_aibox_rule_update_all();  // cmem fix (SWIPXXP-572)
            aibox_configure_status_update_all();
            nf_notify_fire_params("aibox_db_change", NF_AIBOX_DB_RULE, 0, 0xFFFFFFFF, 0);
        default:
            break;
    }
}

GThreadPool *get_work_pool()
{
    if(aibox_work_pool == NULL){
        aibox_work_pool = g_thread_pool_new (
                (GFunc) aibox_process_cb_func,
                NULL,
                1,      //max threads
                TRUE,
                NULL);
    }
    return aibox_work_pool;
}

void aibox_cb_work_push(void *func, void *data)
{
    cb_data *cb;
    GError *err = NULL;

    cb = (cb_data *)malloc(sizeof(cb_data));
    if(cb == NULL){
        printf("[%s:%dm] error! memory alloc failed func[%p] data[%p]\n", __func__, __LINE__, func, data);
        return;
    }
    cb->func = func;
    cb->data = data;

    g_thread_pool_push (get_work_pool(), cb, &err);

    if(err){
        printf("[%s:%d] critical error code[%d] message[%s]\n", __func__, __LINE__, err->code, err->message);
        if(aibox_work_pool){
            g_thread_pool_free(aibox_work_pool, TRUE, FALSE);
            aibox_work_pool = NULL;
        }
        g_clear_error(&err);
        if(cb) free(cb);
    }
}

void nf_api_aibox_url_change_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    int rc;
	g_return_if_fail(pinfo != NULL);
	
    printf("[%s:%d] param0[%d]\n", __func__, __LINE__, (pinfo->d.params[0]));
	switch(pinfo->d.params[0]){
		case NF_SYSDB_CATE_NET:
		case NF_SYSDB_CATE_USR:
        case NF_SYSDB_CATE_CAM:
            aibox_cb_work_push(aibox_url_changed, (pinfo->d.params[0]));
            break;
		default:
			break;
	}
	return;
}

static jpeg_image_data aibox_fr_get_image(unsigned int aibox_ip, int image_index, const char *image_name)
{
    int rc;
    jpeg_image_data ret;
    char path[200];
    HTTP_CTX ctx;
    int ch;
    char id[129];
    char pw[129];

    memset(&ret, 0, sizeof(jpeg_image_data));

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || image_index <= 0) // || image_name == NULL
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] image_index[%d]\n", __func__, __LINE__, aibox_ip, image_index);
        return ret;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return ret;
    }

    http_init(&ctx);
    snprintf(path, sizeof(path), "/api/rule/fr/faces/%d/image/", image_index);
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED\n", __func__, __LINE__);
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR\n", __func__, __LINE__);
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        goto endl;
    }

    ret.memory = ctx.res_data.memory;
    ret.size = ctx.res_data.size;
    ctx.res_data.memory = NULL;

endl:
    http_release(&ctx);
    return ret;
}

int aibox_fr_get(unsigned int aibox_ip, int fr_id, face_info *info){
	//GET /api/rule/lpr/groups/
	int rc;
	HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

	int ch;
	char id[129];
	char pw[129];
	char query_buffer[100];

	json_t *json = NULL;
	json_error_t error;

	char path[300];
	int i;

	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || fr_id <= 0 || info == NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] fr_id[%d] info[%p]\n", __func__, __LINE__, aibox_ip, fr_id, info);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	ch = get_aibox_ch(aibox_ip);
	if(ch < 0 || ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	snprintf(path, sizeof(path), "/api/rule/fr/faces/%d/", fr_id);

	http_data_set(&ctx, HTTP_SET_PATH, path);
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
	goto endl;
	}

	json = json_loads(http_get_res(&ctx), 0, &error);
	if(json == NULL)
	{
		fprintf(stderr, "[%s:%d] json error message[%s] on line %d: %s\n", __FUNCTION__, __LINE__, http_get_res(&ctx), error.line, error.text);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	ret = face_info_parser(json, info);
	//ret = DLVA_API_RET_OK;
endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;

err:
	goto endl;
}

int nf_api_aibox_fr_get(unsigned int aibox_ip, int id, face_info *info, jpeg_image_data *image)
{
    int rc, ret;
    int ch;
            
    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || id <= 0 || info == NULL || image == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] id[%d] info[%p] image[%p]\n", __func__, __LINE__, aibox_ip, id, info, image);
        return DLVA_API_RET_ARGUMENT_ERROR;//argument error
    }
                                            	
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
    	return DLVA_API_RET_ARGUMENT_ERROR;
    }

	rc = aibox_fr_get(aibox_ip, id, info);
	if(rc < 0)
    {
		printf("[%s:%d] aibox_fr_get[%d]\n", __func__, __LINE__, rc);
		return rc;
	}
    
	image->size = 0;
	*image = aibox_fr_get_image(aibox_ip, id, NULL);
	if(image->size <= 0)
	{
		printf("[%s:%d] aibox_fr_get_image size[%d]\n", __func__, __LINE__, image->size);
		return DLVA_API_RET_AIBOX_RETCODE_ERROR;
	}
    
	return DLVA_API_RET_OK;
}



jpeg_image_data nf_api_fr_get_image(unsigned int aibox_ip, int aibox_face_id, const char *image_name)
{
    return aibox_fr_get_image(aibox_ip, aibox_face_id, image_name);
}

static int aibox_fr_face_add_bulk_end(unsigned int aibox_ip)
{
    int rc;
    HTTP_CTX ctx;
    int ret = -1;

    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x]\n", __func__, __LINE__, aibox_ip);
        return DLVA_API_RET_ARGUMENT_ERROR;//argument error
    }
	
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);

    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/face/bulk/");
    http_data_set(&ctx, HTTP_SET_METHOD, "PUT");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    //
    //printf("[%s:%d] debug http_request[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    ret = DLVA_API_RET_OK;//OK;

endl:
    http_release(&ctx);
    return ret;
}


static int aibox_fr_face_add_bulk(unsigned int aibox_ip, jpeg_image_data image, const char *name, const char *group_name)
{
    int rc;
    HTTP_CTX ctx;
    int ret_id = 0;
    json_t *json = NULL;
    json_error_t error;
	
    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || image.memory == NULL || image.size == 0) // || name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] image.memory[%p] image.size[%ld] name[%s]\n", __func__, __LINE__, aibox_ip, image.memory, image.size, name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

	ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    //printf("[%s:%d] image.memory[%p] image.size[%ld] name[%s] group_id[%d]\n", __func__, __LINE__, image.memory, image.size, name, group_id);

    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    //http_data_set(&ctx, HTTP_SET_PATH, "/api-noauth/rule/fr/face/");
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/face/bulk/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    //Name
    curl_formadd(&ctx.formpost,
             &ctx.lastptr,
             CURLFORM_COPYNAME,     "name",
             CURLFORM_COPYCONTENTS, name,
             CURLFORM_END);

    //Group (optional)
    if(group_name){
        //printf("[%s:%d] group_name[%s]\n", __func__, __LINE__, group_name);
        curl_formadd(&ctx.formpost,
                &ctx.lastptr,
                CURLFORM_COPYNAME,     "groups",
                CURLFORM_COPYCONTENTS, group_name,
                CURLFORM_END);
    }

    //Image
    curl_formadd(&ctx.formpost,
             &ctx.lastptr,
             CURLFORM_COPYNAME,     "image",
             CURLFORM_BUFFER,     "add_image.jpg",
             CURLFORM_BUFFERPTR,    image.memory,
             CURLFORM_BUFFERLENGTH, image.size,
             CURLFORM_CONTENTTYPE, "image/jpeg",
             CURLFORM_END);


    rc = http_request(&ctx);
    //printf("[%s:%d] rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret_id = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 201/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        ret_id = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret_id = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }


    json = json_loads(http_get_res(&ctx), 0, &error);
    ret_id = (int)json_integer_value(json_object_get(json, "id"));

endl:
    //printf("[%s:%d] debug ret_id[%d]\n", __func__, __LINE__, ret_id);
    if(json) json_decref(json);
    http_release(&ctx);
    return ret_id;
}

static int aibox_fr_face_add(unsigned int aibox_ip, jpeg_image_data image, const char *name, int group_id)
{
    int rc;
    HTTP_CTX ctx;
    int ret_id = 0;
    json_t *json = NULL;
    json_error_t error;
    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || image.memory == NULL || image.size == 0) // || name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] image.memory[%p] image.size[%ld] name[%s]\n", __func__, __LINE__, aibox_ip, image.memory, image.size, name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    printf("[%s:%d] image.memory[%p] image.size[%ld] name[%s] group_id[%d]\n", __func__, __LINE__, image.memory, image.size, name, group_id);

    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    //http_data_set(&ctx, HTTP_SET_PATH, "/api-noauth/rule/fr/face/");
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/face/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    //Name
    curl_formadd(&ctx.formpost,
             &ctx.lastptr,
             CURLFORM_COPYNAME,     "name",
             CURLFORM_COPYCONTENTS, name,
             CURLFORM_END);

    //Group (optional)
    if(group_id){
        char group_id_buff[200];
        snprintf(group_id_buff, sizeof(group_id_buff), "%d", group_id);
        curl_formadd(&ctx.formpost,
                &ctx.lastptr,
                CURLFORM_COPYNAME,     "groups",
                CURLFORM_COPYCONTENTS, group_id_buff,
                CURLFORM_END);
    }

    //Image
    curl_formadd(&ctx.formpost,
             &ctx.lastptr,
             CURLFORM_COPYNAME,     "image",
             CURLFORM_BUFFER,     "add_image.jpg",
             CURLFORM_BUFFERPTR,    image.memory,
             CURLFORM_BUFFERLENGTH, image.size,
             CURLFORM_CONTENTTYPE, "image/jpeg",
             CURLFORM_END);

    rc = http_request(&ctx);
    //printf("[%s:%d] rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret_id = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 201/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        ret_id = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret_id = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }


    json = json_loads(http_get_res(&ctx), 0, &error);
    ret_id = (int)json_integer_value(json_object_get(json, "id"));

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret_id;
}

int nf_api_fr_group_add(unsigned int aibox_ip, const char *group_name)
{
    //POST /api/rule/fr/group/
    //{"name":"My Group Name"}
    int rc;
    HTTP_CTX ctx;
    int ret_id = 0;
    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;
    char *post = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || group_name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] group_name ptr[%p]\n", __func__, __LINE__, aibox_ip, group_name);
        return 0;
    }

    if(strlen(group_name) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] group_name length[%d]\n", __func__, __LINE__, aibox_ip, strlen(group_name));
        return 0;
    }
    
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    http_init(&ctx);


    //post data
    json = json_pack_ex(&error, 0, "{s:s}",
            "name", group_name);
    post = json_dumps(json, JSON_ENCODE_ANY);
    if(post == NULL){
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }
    json_decref(json);
    json = NULL;

    //set http
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/group/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    //printf("[%s:%d] http_request[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }
    ret_id = (int)json_integer_value(json_object_get(json, "id"));
endl:
    if(json) json_decref(json);
    if(post) free(post);
    http_release(&ctx);
    return ret_id;
err:
    goto endl;
}

int nf_api_fr_face_group_bind(unsigned int aibox_ip, int face_id, int group_id)
{
    //POST /api/rule/fr/bind/
    //{"face_id": 1, "group_id": 2}
    int rc;
    HTTP_CTX ctx;
    int ret = 0;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;
    char *post = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || face_id <= 0 || group_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] face_id[%d] group_id[%d]\n", __func__, __LINE__, aibox_ip, face_id, group_id);
        return 0;
    }
    
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    http_init(&ctx);

    json = json_pack_ex(&error, 0, "{s:i,s:i}",
            "face_id", face_id,
            "group_id", group_id);
    post = json_dumps(json, JSON_ENCODE_ANY);
    if(post == NULL){
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }

    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/bind/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);

    //printf("[%s:%d] http_request[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 201/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        goto endl;
    }

    ret = 1;
endl:
    if(json) json_decref(json);
    if(post) free(post);
    http_release(&ctx);
    return ret;
err:
    goto endl;
}

int nf_api_fr_face_group_unbind(unsigned int aibox_ip, int face_id, int group_id)
{
    //POST /api/rule/fr/unbind/
    //{"face_id": 1, "group_id": 2}
    int rc;
    HTTP_CTX ctx;
    int ret = 0;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;
    char *post = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || face_id <= 0 || group_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] face_id[%d] group_id[%d]\n", __func__, __LINE__, aibox_ip, face_id, group_id);
        return 0;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    http_init(&ctx);

    json = json_pack_ex(&error, 0, "{s:i,s:i}",
            "face_id", face_id,
            "group_id", group_id);
    post = json_dumps(json, JSON_ENCODE_ANY);
    if(post == NULL){
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }

    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/unbind/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);

    //printf("[%s:%d] http_request[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 204/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        goto endl;
    }

    ret = 1;
endl:
    if(json) json_decref(json);
    if(post) free(post);
    http_release(&ctx);
    return ret;
err:
    goto endl;
}




int nf_api_fr_face_add(unsigned int aibox_ip, jpeg_image_data image, const char *nvr_id, int *group_id_list, int group_length)
{
    int ret = 0;
    int rc;
    int i;

    ret = aibox_fr_face_add(aibox_ip, image, nvr_id, (group_length > 0 && group_id_list) ? group_id_list[0] : NULL);

    //group bind
    if(ret > 0 && group_id_list && group_length > 1){
        for(i = 1; i < group_length; i++){
            rc = nf_api_fr_face_group_bind(aibox_ip, ret, group_id_list[i]);
            printf("[%s:%d] index[%d] nf_api_fr_face_group_bind[%d]\n", __func__, __LINE__, i, rc);
        }
    }

    return ret;
}

void free_face_info_list(face_info *list, int length)
{
	/*
    int i;
    if(list == NULL || length <= 0){
        printf("[%s:%d] arg check error list[%p] length[%d]\n", __func__, __LINE__, list, length);
        return;
    }

    for(i = 0; i < length; i++){
        if(list[i].group_list){
            free(list[i].group_list);
        }
    }
	*/

    free(list);
}

int group_info_parser(json_t *json, fr_group_info *info)
{
    const char *name;

    //arg check
    if(info == NULL || json == NULL) return -1;

    //faces(len), id, name
    info->face_count = json_array_size(json_object_get(json, "faces"));
    info->id = (int)json_integer_value(json_object_get(json, "id"));
    name = json_string_value(json_object_get(json, "name"));
    
    snprintf(info->name, sizeof(info->name), name);

    printf("[%s:%d] id[%d] name[%s] faces[%d]\n", __func__, __LINE__, info->id, info->name, info->face_count);

    if(info->id == 0) return -1;

    return 0;
}

fr_group_info *nf_api_fr_group_list_get(unsigned int aibox_ip, int *length)
{
    //GET /api/rule/fr/groups/
    fr_group_info *ret = NULL;
    int rc;
    HTTP_CTX ctx;
    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;

    int len = 0;
    int i;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || length == NULL) // || name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] length[%p]\n", __func__, __LINE__, aibox_ip, length);
        return NULL;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return NULL;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/groups/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    //printf("[%s:%d] debug rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
    
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }

    len = json_array_size(json);
    if(len > 0){
        ret = malloc(sizeof(fr_group_info) * len);
        if(ret == NULL){
            printf("[%s:%d] malloc fail rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
            goto endl;
        }
    }else{
        printf("[%s:%d] nodata rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    memset(ret, 0, sizeof(fr_group_info) * len);

    for(i = 0; i < len; i++){
        if(group_info_parser(json_array_get(json, i), &ret[i]) != 0){
            goto err;
        }
    }

    printf("[%s:%d] len[%d]\n", __func__, __LINE__, len);
endl:
    *length = len;
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
err:
    if(ret) free(ret);
    ret = NULL;
    len = 0;
    goto endl;
}

int face_info_parser(json_t *json, face_info *info)
{
    //const char *name;
    //const char *image_url;
    fr_group_info *group_list = NULL;

    int i;
    int len = 0;
    json_t *json_array = NULL;
    json_t *json_group = NULL;

	json_t *json_image = NULL;

    //arg check
    if(info == NULL || json == NULL) return -1;

    info->id = (int)json_integer_value(json_object_get(json, "id"));
	snprintf(info->memo, sizeof(info->memo),   "%s", str_null_to_blank(json_string_value(json_object_get(json, "memo"))));
	snprintf(info->name, sizeof(info->name),   "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));

	if(json_object_get(json, "image")){
	    json_image = json_object_get(json, "image");
	}else
	if(json_object_get(json, "image_url")){
	    json_image = json_object_get(json, "image_url");
	}
    //name = json_string_value(json_object_get(json, "name"));
    //image_url = json_string_value(json_object_get(json, "image"));

    //snprintf(info->name, sizeof(info->name), name);
    //snprintf(info->image_url, sizeof(info->image_url), image_url);
	snprintf(info->image_url, sizeof(info->image_url),   "%s", str_null_to_blank(json_string_value(json_image)));
    
    if(info->id == 0) return -1;

    json_array = json_object_get(json, "groups"); 
    len = json_array_size(json_array);
    if(len){
        //group_list = malloc(sizeof(fr_group_info) * len);
		group_list = info->group_list;

		if(len > MAX_AIBOX_DB_GROUP_SIZE) len = MAX_AIBOX_DB_GROUP_SIZE;
		
        //if(group_list == NULL) return -1;
        //memset(group_list, 0, sizeof(fr_group_info) * len);

        for(i = 0; i < len; i++){
            json_group = json_array_get(json_array, i);
            
            group_list[i].id = (int)json_integer_value(json_object_get(json_group, "id"));
            // name = json_string_value(json_object_get(json_group, "name"));
            //snprintf(group_list[i].name, sizeof(group_list[i].name), name);
			snprintf(group_list[i].name, sizeof(group_list[i].name),   "%s", str_null_to_blank(json_string_value(json_object_get(json_group , "name"))));

            if(group_list[i].id <= 0){
                printf("[%s:%d] group id parse error id[%d] index[%d]\n", __func__, __LINE__, group_list[i].id, i);
                if(json_group) free(json_group);
                return -1;
            }


            /*
            if(len > 1)
            printf("[%s:%d] name[%s] groups[%d][%d][%s]\n", __func__, __LINE__, info->name, i, group_list[i].id, group_list[i].name);
            */
        }

        info->group_length = len;
        //info->group_list = group_list;
    }
    //printf("[%s:%d] json_array[%p] len[%d]\n", __func__, __LINE__, json_array, len);

    return 0;
}

int nf_api_aibox_fr_face_list_get(unsigned int aibox_ip, face_info **list_ret, int *length)
{
    //GET /api/rule/fr/groups/
    face_info *list = NULL;
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;

    int len = 0;
    int i;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || list_ret == NULL || length == NULL) // || name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] list_ret[%p] length[%p]\n", __func__, __LINE__, aibox_ip, list_ret, length);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/faces/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }
    //printf("[%s:%d] debug rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    len = json_array_size(json);
    if(len > 0){
        list = malloc(sizeof(face_info) * len);
        if(list == NULL){
            printf("[%s:%d] malloc fail rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
            ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
            goto endl;
        }
    }else{
        printf("[%s:%d] nodata rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        if(json_typeof(json) == JSON_ARRAY){
            ret = DLVA_API_RET_OK;
        }else{
            ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        }
        goto endl;
    }

    memset(list, 0, sizeof(face_info) * len);

    for(i = 0; i < len; i++){
        if(face_info_parser(json_array_get(json, i), &list[i]) != 0){
            ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
            goto err;
        }
    }

    ret = DLVA_API_RET_OK;

endl:
    *length = len;
    *list_ret = list;
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;

err:
    if(list) free(list);
    list = NULL;
    len = 0;
    goto endl;
}

static int get_group_id_aibox_fr(unsigned int aibox_ip, const char *group_name)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];
    
	json_t *json = NULL;
	json_t *group_info = NULL;
	json_error_t error;
    
	int len = 0;
	int i;
    
	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || group_name== NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] group_name[%p]\n", __func__, __LINE__, aibox_ip, group_name);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}
    
	len = strlen(group_name);
	if(len <= 0)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] len[%d]\n", __func__, __LINE__, aibox_ip, len);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	ch = get_aibox_ch(aibox_ip);
	if(ch < 0 || ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}
    
	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/v2/groups/");
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	json = json_loads(http_get_res(&ctx), 0, &error);
	if(json == NULL)
	{
		fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	ret = 0;
	json_array_foreach(json, i, group_info){
		const char *name = NULL;
		name = json_string_value(json_object_get(group_info, "name"));
		if(strncmp(group_name, name, len) == 0){
			ret = json_integer_value(json_object_get(group_info, "id"));
			break;
		}
	}
endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

int nf_api_aibox_fr_face_list_get_paging(

		//input
		unsigned int aibox_ip,  
		int page_index,         //íì´ì§ ì¸ë±ì¤
		int page_size,          //íë²ì ë³´ì¬ì¤ íì´ì§ í¬ê¸°

		//search option
		int group_id,
		const fr_db_search_option *search_option,

		//output
		int *max_page_num,
		face_info **list_ret,
		int *length)
        {
			face_info *list = NULL;
			int rc;
			HTTP_CTX ctx;
			int ret = DLVA_API_RET_FAILED_INIT;

			int ch;
			char id[129];
			char pw[129];
			char query_buffer[100];

			json_t *json = NULL;
			json_t *json_array = NULL;
			json_error_t error;

			int len = 0;
			int i;

			//argument check
			if(aibox_ip == 0 || aibox_ip == 0xffffffff || list_ret == NULL || length == NULL || page_index < 0) // ||  == NULL)
			{
				printf("[%s:%d] argument check error aibox_ip[%08x] list_ret[%p] length[%p] page_index[%d]\n", __func__, __LINE__, aibox_ip, list_ret, length, page_index);
				return DLVA_API_RET_ARGUMENT_ERROR;
			}

			*length = 0;
			ch = get_aibox_ch(aibox_ip);
			if(ch < 0 || ch >= AVAILABLE_MAX_CH)
			{
				printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
				return DLVA_API_RET_ARGUMENT_ERROR;
			}
			page_index++;

			http_init(&ctx);
			aibox_http_default_setting(&ctx, aibox_ip);
			http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/v2/faces/");
			http_data_set(&ctx, HTTP_SET_METHOD, "GET");
			http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
			http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

			snprintf(query_buffer, sizeof(query_buffer), "%d", page_index);
			http_data_set(&ctx, HTTP_ADD_QUERY, "page", query_buffer);
			snprintf(query_buffer, sizeof(query_buffer), "%d", page_size);
			http_data_set(&ctx, HTTP_ADD_QUERY, "ipp", query_buffer);

			/*
			 * search options
			 */
			if(group_id > 0){
				snprintf(query_buffer, sizeof(query_buffer), "%d", group_id);
				http_data_set(&ctx, HTTP_ADD_QUERY, "group", query_buffer);
			}else if(group_id == 0){
				//group_none
				http_data_set(&ctx, HTTP_ADD_QUERY, "no_group", "true");
			}

			if(search_option)
			{
				if(search_option->memo && strlen(search_option->memo) > 0)
					http_data_set(&ctx, HTTP_ADD_QUERY, "memo", search_option->memo);
				if(search_option->name && strlen(search_option->name) > 0)
					http_data_set(&ctx, HTTP_ADD_QUERY, "name", search_option->name);
			}

			rc = http_request(&ctx);
			if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
				printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
				ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
				goto endl;
			}

			json = json_loads(http_get_res(&ctx), 0, &error);
			if(json == NULL)
			{
				fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
				ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
				goto endl;
			}

			if(max_page_num){
				if(page_size <= 0){
					*max_page_num = 1;
				}else{
					int total = json_integer_value(json_object_get(json, "total"));
					*max_page_num = total / page_size + (total % page_size > 0);
				}
			}
			json_array = json_object_get(json, "faces");
			len = json_array_size(json_array);
			if(len > 0){
				list = malloc(sizeof(face_info) * len);
				if(list == NULL){
					printf("[%s:%d] malloc fail rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
					ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
					goto endl;
				}
			}else{
				printf("[%s:%d] nodata rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
				if(json_typeof(json_array) == JSON_ARRAY){
					ret = DLVA_API_RET_OK;
				}else{
					ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
				}
				goto endl;
			}

			memset(list, 0, sizeof(face_info) * len);

			for(i = 0; i < len; i++){
				if(face_info_parser(json_array_get(json_array, i), &list[i]) != 0){
					ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
					goto err;
				}
			}

			ret = DLVA_API_RET_OK;
endl:
			*length = len;
			*list_ret = list;
			if(json) json_decref(json);
			http_release(&ctx);
			return ret;

err:
			if(list) free(list);
			list = NULL;
			len = 0;
			goto endl;
		}

face_info *aibox_fr_face_list_get(unsigned int aibox_ip, int *length)
{

    int ret = 0;
    face_info *list = NULL;

    ret = nf_api_aibox_fr_face_list_get(aibox_ip, &list, length);
    /*
    if(length == NULL) return list;
    printf("[%s:%d] ret[%d] length[%d]\n", __func__, __LINE__, ret, *length);
    {
        int i;
        for(i = 0; i < *length; i++){
            printf("[%s:%d] [%03d] : id[%d] name[%s] image_url[%s] group_length[%d]\n", __func__, __LINE__, i, list[i].id, list[i].name, list[i].image_url, list[i].group_length);
        }
    }
    */
    return list;
}


int aibox_fr_group_delete(unsigned int aibox_ip, unsigned int group_id)
{
    //DELETE /api/rule/fr/groups/<group_id>/
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = 0;

    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || group_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] id[%p]\n", __func__, __LINE__, aibox_ip, group_id);
        return 0;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    snprintf(path, sizeof(path), "/api/rule/fr/groups/%d/", group_id);


    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 204){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }


    ret = 1;

endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int aibox_fr_face_delete(unsigned int aibox_ip, unsigned int face_id)
{
    //DELETE /api/rule/fr/faces/<face_id>/
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = 0;

    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || face_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] id[%p]\n", __func__, __LINE__, aibox_ip, face_id);
        return 0;
    }
    
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    snprintf(path, sizeof(path), "/api/rule/fr/faces/%d/", face_id);


    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 204){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    printf("[%s:%d] debug rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    ret = 1;

endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int nf_api_aibox_fr_face_delete_all(unsigned int aibox_ip)
{
    //DELETE /api/rule/fr/faces/<face_id>/
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = DLVA_API_RET_OK;

    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] \n", __func__, __LINE__, aibox_ip);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/face/bulk/");
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 204){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    //printf("[%s:%d] debug rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static int aibox_fr_face_modify(unsigned int aibox_ip, jpeg_image_data image, const char *name, int face_id)
{
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;
    //json_t *json = NULL;
    json_error_t error;
    char buff[200];
    char path[200];

    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || face_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] face_id[%d]image.memory[%p] image.size[%ld] name[%s]\n", __func__, __LINE__, aibox_ip, face_id, image.memory, image.size, name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    printf("[%s:%d] image.memory[%p] image.size[%ld] name[%s] face_id[%d]\n", __func__, __LINE__, image.memory, image.size, name, face_id);

    //name image
    if(name == NULL && (image.memory == NULL || image.size == 0)){
        printf("[%s:%d] argument fail please check name[%s] or image[%p][%d]\n", __func__, __LINE__, name, image.memory, image.size);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }


    //snprintf(path, sizeof(path), "/api/rule/fr/face/%d/", face_id);
    snprintf(path, sizeof(path), "/api/fr/v1/faces/%d/", face_id);

    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "PUT");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    //face_id
    /*
    snprintf(buff, sizeof(buff), "%d", face_id);
    curl_formadd(&ctx.formpost,
            &ctx.lastptr,
            CURLFORM_COPYNAME,     "id",
            CURLFORM_COPYCONTENTS, face_id,
            CURLFORM_END);
            */

    //name
    if(name != NULL && strlen(name) >= 0){
        curl_formadd(&ctx.formpost,
                &ctx.lastptr,
                CURLFORM_COPYNAME,     "name",
                CURLFORM_COPYCONTENTS, name,
                CURLFORM_END);
    }

    //Image
    if(image.memory != NULL && image.size > 0){
        curl_formadd(&ctx.formpost,
                &ctx.lastptr,
                CURLFORM_COPYNAME,     "image",
                CURLFORM_BUFFER,     "change_image.jpg",
                CURLFORM_BUFFERPTR,    image.memory,
                CURLFORM_BUFFERLENGTH, image.size,
                CURLFORM_CONTENTTYPE, "image/jpeg",
                CURLFORM_END);
    }

    rc = http_request(&ctx);
    //printf("[%s:%d] rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    
    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
		ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }


    /*
    json = json_loads(http_get_res(&ctx), 0, &error);
    ret_id = (int)json_integer_value(json_object_get(json, "id"));
    */

    ret = DLVA_API_RET_OK;
endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int nf_api_fr_face_modify(unsigned int aibox_ip, jpeg_image_data image, const char *name, int face_id)
{
    return aibox_fr_face_modify(aibox_ip, image, name, face_id);
}

int nf_api_aibox_fr_modify(unsigned int aibox_ip, face_info *info, jpeg_image_data *image)
{
    int rc;
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	rc = api_aibox_fr_modify(aibox_ip, info);
	if(rc < 0){
		printf("[%s:%d] api_aibox_fr_modify[%d]", __func__, __LINE__, rc);
		return rc;
	}

	if(image){
		return aibox_fr_face_modify(aibox_ip, *image, info->name, info->id);
	}

}

static int api_aibox_fr_modify(unsigned int aibox_ip, face_info *info)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
	char pw[129];
	char *post = NULL;
	char path[300];

	json_t *json = NULL;
	json_t *groups = NULL;
	json_error_t error;

	int len = 0;
	int i;


	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	if(info->id <= 0)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] info->id[%d]\n", __func__, __LINE__, aibox_ip, info->id);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	ch = get_aibox_ch(aibox_ip);
	if(ch < 0 || ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	json = json_pack("{s:s,s:s,s:[]}",
			"name",  info->name,
			"memo",  info->memo,
			"groups");

	for(i = 0; i < info->group_length; i++){
		int group_id = -1;

		if(info->group_list[i].id > 0){
			group_id = info->group_list[i].id;
		}else if(strlen(info->group_list[i].name) > 0){
			group_id = get_group_id_aibox_fr(aibox_ip, info->group_list[i].name);
		}

		if(group_id > 0){
			json_array_append_new(json_object_get(json, "groups"), json_integer(group_id));
		}
	}

	if(json == NULL){
		return DLVA_API_RET_FAILED_INIT;
	}
	post = json_dumps(json, JSON_ENCODE_ANY);
	if(json){
		json_decref(json);
		json = NULL;
	}

	if(post == NULL){
		return DLVA_API_RET_FAILED_INIT;    }

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);

	snprintf(path, sizeof(path), "/api/rule/fr/faces/%d/", info->id);
	http_data_set(&ctx, HTTP_SET_PATH, path);
	http_data_set(&ctx, HTTP_SET_METHOD, "PATCH");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	json = json_loads(http_get_res(&ctx), 0, &error);
	if(json == NULL)
	{
		fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	ret = json_integer_value(json_object_get(json_array_get(json, 0), "id"));
endl:
	if(post) free(post);
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

int nf_api_aibox_login_check(unsigned int aibox_ip, const char *id, const char *pw)
{
    int ret = DLVA_API_RET_ARGUMENT_ERROR;
    int rc;
    HTTP_CTX ctx;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || id == NULL || pw == NULL)// || image_index <= 0) // || image_name == NULL
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] id[%p] pw[%p]\n", __func__, __LINE__, aibox_ip, id, pw);
        return ret;
    }

    if(strlen(id) <= 0 || strlen(pw) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] id & pw length | id[%s] pw[%s]\n", __func__, __LINE__, aibox_ip, (strlen(id) ? "set" : "not set"), (strlen(pw) ? "set" : "not set"));
        return ret;
    }

    http_init(&ctx);
    http_data_set(&ctx, HTTP_SET_USER, id);
    http_data_set(&ctx, HTTP_SET_PASSWD, pw);

    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/faces/-1/image/");

    rc = http_request(&ctx);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED\n", __func__, __LINE__);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status == 401){
        printf("[%s:%d] DLVA_API_RET_AIBOX_AUTH_FAILED\n", __func__, __LINE__);
        ret = DLVA_API_RET_AIBOX_AUTH_FAILED;
        goto endl;
    }

    //printf("[%s:%d] rc[%d] status[%d]\n", __func__, __LINE__, rc, ctx.status);

    ret = DLVA_API_RET_OK;

endl:
    http_release(&ctx);
    return ret;
}

struct UPLOAD_FILE_INFO
{
    char file_path[200];
	char name[200];
	char group[200];    //group name
	struct UPLOAD_FILE_INFO* next;
};
typedef struct UPLOAD_FILE_INFO upload_file_info;

void free_fr_list(upload_file_info *list)
{
    upload_file_info *iter = NULL;
    while(list){
        iter = list;
        list = list->next;
        free(iter);
    }
}

static bool fr_api_is_jpeg(const char *path)
{
    unsigned char buffer[2];
    unsigned char magic[2] = {0xFF, 0xD8};
    FILE *file = NULL;
    int len;
    bool ret = false;

    if(path == NULL) return false;
    if(strlen(path) <= 0) return false;

    file = fopen(path, "rb");
    if(file == NULL) goto endl;

    //FF D8
    len = fread(buffer, 1, 2, file);
    if(len != 2) goto endl;

    //printf("[%s:%d] len[%d] buffer[%02x %02x]\n", __func__, __LINE__, len, buffer[0], buffer[1]);

    if(memcmp(buffer, magic, 2) == 0) ret = true;

endl:
    if(file) fclose(file);

    return ret;
}

static upload_file_info *fr_bulk_list_add(upload_file_info *list, const char *directory_path, const char *file_name, const char *group_name)
{
    upload_file_info *ret = NULL;
    int ext_index = 0;
    char path[200];
    const char *ext_name = NULL;

    //check argument
    if(directory_path == NULL || file_name == NULL || group_name == NULL)
    {
        printf("[%s:%d] argument error directory_path[%p] file_name[%p] group_name[%p]\n", __func__, __LINE__, directory_path, file_name, group_name);
        return list;
    }

    if(strlen(directory_path) <= 0|| strlen(file_name) <= 0|| strlen(group_name) <= 0)
    {
        printf("[%s:%d] argument error directory_path[%s] file_name[%s] group_name[%s]\n", __func__, __LINE__, directory_path, file_name, group_name);
        return list;
    }


    for(ext_index = strlen(file_name) - 1; ext_index >= 0; ext_index--){
        if(file_name[ext_index] == NULL){
            printf("[%s:%d] ??? error filename[%s] index[%d] check char[%c]\n", __func__, __LINE__, file_name, ext_index, file_name[ext_index]);
            ext_index = -1;
            break;
        }
        if(file_name[ext_index] == '.') break;
    }

    //printf("[%s:%d] file_name[%s] ext[%s]\n", __func__, __LINE__, file_name, file_name + ext_index+1);
    if(ext_index <= 0){
        return list;
    }
    ext_name = file_name + ext_index + 1;

    if(strncasecmp(ext_name, "jpg", 3) != 0 &&
       strncasecmp(ext_name, "jpeg", 4) != 0){
        return list;
    }

    snprintf(path, sizeof(path), "%s/%s", directory_path, file_name);
    if(fr_api_is_jpeg(path) == false){
        printf("[%s:%d] file[%s] is not jpeg\n", __func__, __LINE__, path);
        return list;
    }
    
    if(list == NULL){
        ret = malloc(sizeof(upload_file_info));
    }else{
        list->next = malloc(sizeof(upload_file_info));
        ret = list->next;
    }

    if(ret == NULL){
        printf("[%s:%d] malloc failed ret[%p] list[%p]\n", __func__, __LINE__, ret, list);
        return list;
    }
    memset(ret, 0, sizeof(upload_file_info));

    //data set
    snprintf(ret->file_path, sizeof(ret->file_path), path);
    snprintf(ret->group, sizeof(ret->group), group_name);
    strncpy(ret->name, file_name, ext_index);
    
    return ret;
}

static upload_file_info *fr_make_image_list(const char *path, int *len)
{
    char path_rename[200];
    char group_path[200];

    DIR *dir = NULL;
    DIR *dir_group = NULL;
    struct dirent *ent = NULL;
    struct dirent *ent_group = NULL;
    upload_file_info *list = NULL;
    upload_file_info *list_next = NULL;

    upload_file_info *ret = NULL;

    if(path == NULL) return NULL;
    if(strlen(path) == 0) return NULL;
    if(path[strlen(path) - 1] == '/'){
        memset(path_rename, 0, sizeof(path_rename));
        strncpy(path_rename, path, strlen(path) -1);
        path = path_rename;
    }

    dir = opendir(path);
    if(dir == NULL) goto endl;

    if(len) *len = 0;

    while(ent = readdir(dir)){
        if(strcmp(ent->d_name, ".") == 0) continue;
        if(strcmp(ent->d_name, "..") == 0) continue;

        snprintf(group_path, sizeof(group_path), "%s/%s", path, ent->d_name);
        dir_group = opendir(group_path);
        if(dir_group == NULL) continue;

        while(ent_group = readdir(dir_group)){
            if(strcmp(ent_group->d_name, ".") == 0) continue;
            if(strcmp(ent_group->d_name, "..") == 0) continue;

            list_next = fr_bulk_list_add(list, group_path, ent_group->d_name, ent->d_name);
            if(list_next != list){
                //success
                list = list_next;
                if(len) (*len)++;
            }

            if(ret == NULL) ret = list;

            /*
            if(list){
                printf("[%s:%d] debug list[%p] file_path[%s] name[%s] group[%s]\n", __func__, __LINE__, list, list->file_path, list->name, list->group);
            }else{
                printf("[%s:%d] debug list[%p]\n", __func__, __LINE__, list);
            }
            */
        }

        closedir(dir_group);
        dir_group = NULL;
    }

endl:
    if(dir) closedir(dir);
    if(dir_group) closedir(dir_group);

    return ret;
}

static jpeg_image_data get_image_data(const char *path)
{
    FILE *file = NULL;
    jpeg_image_data image = {NULL, 0};
    int len;
    

    file = fopen(path, "rb");
    if(file == NULL) goto endl;

    fseek(file, 0, SEEK_END);
    image.size = ftell(file);
    if(image.size <= 0) goto endl;

    fseek(file, 0, SEEK_SET);

    image.memory = malloc(image.size);
    if(image.memory == NULL) goto endl;

    len = fread(image.memory, 1, image.size, file);
    if(image.size != len){
        free(image.memory);
        memset(&image, 0, sizeof(jpeg_image_data));
        printf("[%s:%d] error! len[%d] image.size[%d]\n", __func__, __LINE__, len, image.size);
    }

endl:
    if(file) fclose(file);
    return image;
}

typedef struct 
{
    upload_file_info *list;
    upload_result_callback functor;
    unsigned int aibox_ip;
    unsigned int channel;
}upload_thread_args;


int aibox_face_bulk_upload_thread(upload_thread_args *arg)
{
	upload_file_info *iter = NULL;
	upload_file_info *list = NULL;
    FR_BULK_UPLOAD_CALLBACK_MESSAGE message;
    
    int rc;
    int count = 0;
    int upload_count = 0;
    int id = 0;

    pthread_detach(pthread_self());

    list = arg->list;
    //printf("[%s:%d] arg[%p]\n", __func__, __LINE__);

    while(list){
        iter = list;
        list = list->next;
        count++;

        //printf("[%s:%d] debug count[%03d] path[%s] name[%s] group[%s]\n", __func__, __LINE__, count, iter->file_path, iter->name, iter->group);
        jpeg_image_data image;
        image = get_image_data(iter->file_path);
        if(image.size == 0){
            if(image.memory) free(image.memory);
            continue;
        }
        
        if(image.memory == NULL){
            continue;
        }

        if((id = aibox_fr_face_add_bulk(arg->aibox_ip, image, iter->name, iter->group)) > 0){
            upload_count++;
            //printf("[%s:%d] upload success id[%d]\n", __func__, __LINE__, id);
        }

        if(arg->functor){
            memset(&message, 0, sizeof(FR_BULK_UPLOAD_CALLBACK_MESSAGE));
            message.type = FR_BULK_TYPE_UPLOAD;

            if(id > 0)
                message.result = DLVA_API_RET_OK;
            else
                message.result = id;

            message.id = id;
            message.index = count - 1;
            snprintf(message.path, sizeof(message.path), iter->file_path);
            arg->functor(message);
        }

        free(image.memory);
        free(iter);
    }
    
    //upload count
    if(upload_count > 0){
        rc = aibox_fr_face_add_bulk_end(arg->aibox_ip);
        printf("[%s:%d] aibox_fr_face_add_bulk_end[%d] count[%d] upload_count[%d]\n", __func__, __LINE__, rc, count, upload_count);
    }else{
        rc = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
    }

    if(arg->functor){
        memset(&message, 0, sizeof(FR_BULK_UPLOAD_CALLBACK_MESSAGE));
        message.type = FR_BULK_TYPE_CONFIRM;
        message.result = rc;
        arg->functor(message);
    }
    

    free(arg);

    //return code
    return upload_count;

}

FR_FACE_BULK_UPLOAD_STATE nf_api_fr_face_bulk_upload(unsigned int aibox_ip, const char *directory_path, int max_face_count, FR_UPLOAD_FILE_INFO **list_result, int *list_len, upload_result_callback functor)
{
	upload_file_info *list = NULL;
	upload_file_info *iter = NULL;
    pthread_t upload_th;

    int rc = 0;
    int i;
    int ret = FR_BULK_ARGUMENT_MISSING;

    int list_length = 0;
    FR_UPLOAD_FILE_INFO *upload_info_list = NULL;
    upload_thread_args *arg = NULL;

    /////////////////////////////////////////////////////////////////
    //argument error check
    /////////////////////////////////////////////////////////////////
    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || directory_path == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] directory_path[%p]\n", __func__, __LINE__, aibox_ip, directory_path);
        return ret;
    }

    if(strlen(directory_path) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] directory_path[%s]\n", __func__, __LINE__, aibox_ip, directory_path);
        return ret;
    }

    //result pointer
    if(list_result == NULL || list_len == NULL)
    {
        printf("[%s:%d] argument check error list[%p] list_len[%p]\n", __func__, __LINE__, list, list_len);
        return ret;
    }

    *list_result = NULL;
    *list_len = 0;
    /////////////////////////////////////////////////////////////////
    // directory parse & list make
    /////////////////////////////////////////////////////////////////
    //list make
    list = fr_make_image_list(directory_path, &list_length);
    printf("[%s:%d] list[%p] len[%d]\n", __func__, __LINE__, list, list_length);

    if(list == NULL){
        ret = FR_BULK_FILE_NOT_FOUND;
        printf("[%s:%d] error list[%p]\n", __func__, __LINE__, list);
        goto endl;
    }

    if(list_length <= 0){
        ret = FR_BULK_FILE_NOT_FOUND;
        *list_result = NULL;
        *list_len = 0;

        free_fr_list(list);
        printf("[%s:%d] error list_length[%d]\n", __func__, __LINE__, list_length);
        goto endl;
    }

    if(max_face_count != 0 && list_length  > max_face_count){
        ret = FR_BULK_FILE_AMOUNT_OVER;
        *list_result = NULL;
        *list_len = list_length;

        free_fr_list(list);
        goto endl;
    }

    /////////////////////////////////////////////////////////////////
    // result list setting(list array & list size)
    /////////////////////////////////////////////////////////////////
    upload_info_list = malloc(sizeof(FR_UPLOAD_FILE_INFO)*list_length);
    if(upload_info_list == NULL){
        free_fr_list(list);
        ret =  FR_BULK_ERROR;
        goto endl;
    }
    
    memset(upload_info_list, 0, sizeof(FR_UPLOAD_FILE_INFO)*list_length);
    iter = list;

    for(i = 0, iter = list; i < list_length; i++, iter = iter->next){
        if(iter == NULL){
            printf("[%s:%d] ?? error iter null\n", __func__, __LINE__);
            break;
        }

        snprintf(upload_info_list[i].file_path, sizeof(upload_info_list[i].file_path), iter->file_path);
        snprintf(upload_info_list[i].name, sizeof(upload_info_list[i].name), iter->name);
        snprintf(upload_info_list[i].group, sizeof(upload_info_list[i].group), iter->group);
    }

    //printf("[%s:%d] iter[%p]\n", __func__, __LINE__, iter);
    if(iter != NULL){
        printf("[%s:%d] ERROR! iter not null\n", __func__, __LINE__);
        //error
    }

    *list_len = list_length;
    *list_result = upload_info_list;

    /////////////////////////////////////////////////////////////////
    // thread make
    /////////////////////////////////////////////////////////////////
    arg = malloc(sizeof(upload_thread_args));
    if(arg == NULL){
        free_fr_list(list);
        free(*list_result);
        *list_result = NULL;

        ret =  FR_BULK_ERROR;
        goto endl;
    }

    memset(arg, 0, sizeof(upload_thread_args));
    arg->list = list;
    arg->functor = functor;
    arg->aibox_ip = aibox_ip;

    pthread_create(&upload_th, NULL, &aibox_face_bulk_upload_thread, (void *)arg);
    ret = FR_BULK_SUCC;

 endl:
    return ret;
}

static int get_key_value(char *string, char **key, char **value)
{
    int len = 0;
    int i;

    if(string == NULL || key == NULL || value == NULL) return -1;

    *key = string;
    for(i = 0, len = strlen(string); i < len; i++){
        if(string[i] == '='){
            string[i] = '\0';
            break;
        }
    }

    if(i + 1 < len){
        *value = string+i+1;
    }else{
        *value = NULL;
        return -1;
    }

    for(i--; i >= 0 && string[i] == ' '; i--){
        string[i] = '\0';
    }

    if(**value == '\0' || **value == ' ') (*value)++;

    return *value - string;
}

int nf_api_aicam_fr_get(unsigned int channel, int id, face_info *info, jpeg_image_data *image)
{
    //argument check
    if(channel < 0 || id <= 0 || info == NULL || image == NULL)
    {
        printf("[%s:%d] argument check error channel[%d] id[%d] info[%p] image[%p]\n", __func__, __LINE__, channel, id, info, image);
        return DLVA_API_RET_ARGUMENT_ERROR;//argument error
    }
    
	memset(info, 0, sizeof(face_info));

	image->size = 0;
	*image = nf_api_aicam_fr_get_image(channel, id, info->name);
	if(image->size <= 0)
	{
		printf("[%s:%d] aibox_fr_get_image size[%d]\n", __func__, __LINE__, image->size);
		return DLVA_API_RET_AIBOX_RETCODE_ERROR;
	}

	return DLVA_API_RET_OK;
}

jpeg_image_data nf_api_aicam_fr_get_image(unsigned int channel, int face_id, char *image_name)
{
	HTTP_CTX ctx;
	int rc = 0;
    char buff[200] = {0,};

	Token_iterator iter;
    char *key;
    char *value;

    jpeg_image_data image;
    memset(&image, 0, sizeof(jpeg_image_data));
    
    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return image;
    }
    if(face_id <= 0){
        printf("[%s:%d] argument error face_id[%d]\n", __func__, __LINE__, face_id);
        return image;
    }

	http_init(&ctx);

    
	//http setting
	aicam_http_default_setting(&ctx, channel);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.face.info");
    snprintf(buff, sizeof(buff), "%d", face_id);
	http_data_set(&ctx, HTTP_ADD_QUERY, "id", buff);
    
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		goto endl;
	}

    camres_foreach(iter, ctx.res_data.memory, key, value){
        //printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        if(strcmp(key, "image") == 0){
            if(value == NULL || strlen(value) <= 0){
                continue;
            }

            image.size = Base64decode_len(value);
            if(image.size <= 0){
                continue;
            }

            image.memory = malloc(image.size);
            if(image.memory == NULL){
                continue;
            }

            rc = Base64decode(image.memory, value);
            //printf("[%s:%d] rc[%d] image.size[%d] image.memory[%s]\n", __func__, __LINE__, rc, image.size, image.memory);

            if(rc <= 0){
                free(image.memory);
                memset(&image, 0, sizeof(jpeg_image_data));
            }else{
                image.size = rc;
            }
            //printf("[%s:%d] image.size[%d]\n", __func__, __LINE__, image.size);
        }else if(strcmp(key, "name") == 0){
            //printf("[%s:%d] get_image id[%d] name[%s]\n", __func__, __LINE__, face_id, value);
            if(image_name){
                strcpy(image_name, value);
            }
        }else{
            //printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        }

    }

endl:
	http_release(&ctx);
    return image;
}

static face_info *_face_list_malloc(face_info *list, int current_size, int new_size)
{
    //printf("[%s:%d] list[%p] current_size[%d] new_size[%d]\n", __func__, __LINE__, list, current_size, new_size);
    //alloc
    if(list == NULL){
        list = malloc(sizeof(face_info)*new_size);
    }else{
        list = realloc(list, sizeof(face_info)*new_size);
    }

    //printf("[%s:%d] list[%p] memset_ptr[%p] face_info_size[%d] size[%d] size_count[%d]\n", __func__, __LINE__, list, list+current_size, sizeof(face_info), sizeof(face_info) * (new_size - current_size), (new_size - current_size));

    //init buffer 
    if(list){
        memset(list+current_size, 0, sizeof(face_info) * (new_size - current_size));
    }

    //printf("[%s:%d] list[%p]\n", __func__, __LINE__, list);
    
    return list;
}


//list get
int nf_api_aicam_fr_face_list_get(unsigned int channel, face_info **list, int *length) //return code
{
	HTTP_CTX ctx;
	int rc = 0;
    int ret = DLVA_API_RET_ARGUMENT_ERROR;

    int count;

    //return value
    int len = 0;
    face_info *list_ret = NULL;

	Token_iterator iter;
    char *key;
    char *value;

    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(list == NULL){
        printf("[%s:%d] argument error list[%p]\n", __func__, __LINE__, list);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(length == NULL){
        printf("[%s:%d] argument error length[%p]\n", __func__, __LINE__, length);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    *length = 0;
	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, channel);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "get_setup.face.infos");
    
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		goto endl;
	}

    camres_foreach(iter, ctx.res_data.memory, key, value){
        //printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        if(strcmp(key, "count") == 0){
            count = atoi(value);
            if(count > len){
                list_ret = _face_list_malloc(list_ret, len, count);
                if(list_ret == NULL){
                    ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
                    goto endl;
                }
                len = count;
            }
        }else if(strstr(key, "id") == key){
            sscanf(key, "id%d", &count);
            if(count < 0) continue;
            if(count+1 > len){
                list_ret = _face_list_malloc(list_ret, len, count+1);
                if(list_ret == NULL){
                    ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
                    goto endl;
                }
                len = count+1;
            }
            list_ret[count].id = atoi(value);
        }else if(strstr(key, "name") == key){
            sscanf(key, "name%d", &count);
            if(count < 0) continue;
            if(count+1 > len){
                list_ret = _face_list_malloc(list_ret, len, count+1);
                if(list_ret == NULL){
                    ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
                    goto endl;
                }
                len = count+1;
            }
            snprintf(list_ret[count].name, sizeof(list_ret[count].name), value);
        }
    }

    *list = list_ret;
    *length = len; 
    ret = DLVA_API_RET_OK;

endl:
	http_release(&ctx);
    return ret;
}

//add
int nf_api_aicam_fr_face_add(unsigned int channel, jpeg_image_data image, const char *name, int *group_id_list, int group_length)
{
	HTTP_CTX ctx;
	int rc = 0;
    int ret = DLVA_API_RET_ARGUMENT_ERROR;
    int id;

    char *post = NULL;
    int len = 0;

	Token_iterator iter;
    char *key;
    char *value;

    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(image.memory == NULL || image.size == 0){
        printf("[%s:%d] argument error image.memory[%p] image.size[%d]\n", __func__, __LINE__, image.memory, image.size);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(name == NULL){
        printf("[%s:%d] argument error name[%p]\n", __func__, __LINE__, name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(strlen(name) <= 0){
        printf("[%s:%d] argument error name[%s]\n", __func__, __LINE__, name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    //http setting
    aicam_http_default_setting(&ctx, channel);

    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/face/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");

    //Name
    curl_formadd(&ctx.formpost,
            &ctx.lastptr,
            CURLFORM_COPYNAME,     "name",
            CURLFORM_COPYCONTENTS, name,
            CURLFORM_END);

    //Image
    curl_formadd(&ctx.formpost,
            &ctx.lastptr,
            CURLFORM_COPYNAME,     "image",
            CURLFORM_BUFFER,     "add_image.jpg",
            CURLFORM_BUFFERPTR,    image.memory,
            CURLFORM_BUFFERLENGTH, image.size,
            CURLFORM_CONTENTTYPE, "image/jpeg",
            CURLFORM_END);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		goto endl;
	}

    camres_foreach(iter, ctx.res_data.memory, key, value){
        printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        if(strstr(key, "id")){
            id = atoi(value);
            if(id > 0)
                ret = id;
        }
    }
    
    if(ret <= 0)
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;

endl:
	http_release(&ctx);
    return ret;
}

//modify
int nf_api_aicam_fr_face_modify(unsigned int channel, jpeg_image_data image, const char *name, int face_id)
{
	HTTP_CTX ctx;
	int rc = 0;
    int ret = DLVA_API_RET_ARGUMENT_ERROR;
    int id;

    char url[256] = {0, };

	Token_iterator iter;
    char *key;
    char *value;

    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if((image.memory == NULL ||image.size == 0) && (name == NULL || strlen(name) <= 0)){
        printf("[%s:%d] argument error image.memory[%p] image.size[%d] name[%s]\n", __func__, __LINE__, image.memory, image.size, name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(face_id == NULL){
        printf("[%s:%d] argument error face_id[%p]\n", __func__, __LINE__, face_id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    
    //http setting
    aicam_http_default_setting(&ctx, channel);

    snprintf(url, sizeof(url), "/api/rule/fr/face/%d", face_id);
    http_data_set(&ctx, HTTP_SET_PATH, url);
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");

    //Name
    if(name != NULL && strlen(name) > 0)
    {
        curl_formadd(&ctx.formpost,
                &ctx.lastptr,
                CURLFORM_COPYNAME,     "name",
                CURLFORM_COPYCONTENTS, name,
                CURLFORM_END);
    }

    //Image
    if(image.memory != NULL && image.size > 0)
    {
        curl_formadd(&ctx.formpost,
                &ctx.lastptr,
                CURLFORM_COPYNAME,     "image",
                CURLFORM_BUFFER,       "modify_image.jpg",
                CURLFORM_BUFFERPTR,    image.memory,
                CURLFORM_BUFFERLENGTH, image.size,
                CURLFORM_CONTENTTYPE, "image/jpeg",
                CURLFORM_END);
    }

    rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		goto endl;
	}

    camres_foreach(iter, ctx.res_data.memory, key, value){
        printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        if(strstr(key, "id")){
            id = atoi(value);
            if(id > 0)
                ret = id;
        }
    }
    
    /*
    if(ret <= 0)
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
    */

    ret = DLVA_API_RET_OK;
endl:
	http_release(&ctx);
    return ret;
}

//delete
int aicam_fr_face_delete(unsigned int channel, unsigned int face_id)
{
	HTTP_CTX ctx;
	int rc = 0;
    int ret = DLVA_API_RET_ARGUMENT_ERROR;
    int id;

    char buff[1024];
    int len = 0;

	Token_iterator iter;
    char *key;
    char *value;

    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    if(face_id <= 0 ){
        printf("[%s:%d] argument error face_id[%p]\n", __func__, __LINE__, face_id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

	http_init(&ctx);
    snprintf(buff, sizeof(buff), "%d", face_id);

	//http setting
	aicam_http_default_setting(&ctx, channel);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.face.delete");
	http_data_set(&ctx, HTTP_ADD_QUERY, "id", buff);
    
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		goto endl;
	}

    camres_foreach(iter, ctx.res_data.memory, key, value){
        printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
        if(strstr(key, "return_code")){
            if(strtoul(value, NULL, 16) == 0){
                ret = 1;
            }
        }
    }
    
endl:
	http_release(&ctx);
    return ret;
}

int nf_api_aicam_fr_face_delete_all(unsigned int channel) //delete all group & face
{
	HTTP_CTX ctx;
	int rc = 0;
    int ret = DLVA_API_RET_ARGUMENT_ERROR;

	Token_iterator iter;
    char *key;
    char *value;

    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return ret;
    }

	http_init(&ctx);

	//http setting
	aicam_http_default_setting(&ctx, channel);
	http_data_set(&ctx, HTTP_SET_PATH, "/cgi-bin/action.fcgi");
	http_data_set(&ctx, HTTP_ADD_QUERY, "api", "set_setup.face.delete_all");
    
	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		goto endl;
	}

    camres_foreach(iter, ctx.res_data.memory, key, value){
        printf("[%s:%d] key[%s] value[%s]\n", __func__, __LINE__, key, value);
    }

    ret = DLVA_API_RET_OK;
endl:
	http_release(&ctx);
    return ret;
}

fr_group_info *nf_api_aicam_fr_group_list_get(unsigned int channel, int *length)
{
	//HTTP_CTX ctx;
	int rc = 0;
    fr_group_info *ret = NULL;


    if(length == NULL){
        printf("[%s:%d] argument error length[%p]\n", __func__, __LINE__, length);
        return ret;
    }
    *length = 0;

    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return ret;
    }

    return ret;
}

int aicam_face_bulk_upload_thread(upload_thread_args *arg)
{
	upload_file_info *iter = NULL;
	upload_file_info *list = NULL;
    FR_BULK_UPLOAD_CALLBACK_MESSAGE message;
    
    int rc;
    int count = 0;
    int upload_count = 0;
    int id = 0;

    pthread_detach(pthread_self());

    list = arg->list;
    //printf("[%s:%d] channel[%d] list[%p] functor[%p]\n", __func__, __LINE__, arg->channel, arg->list, arg->functor);

    while(list){
        iter = list;
        list = list->next;
        count++;

        //printf("[%s:%d] debug count[%03d] path[%s] name[%s] group[%s]\n", __func__, __LINE__, count, iter->file_path, iter->name, iter->group);
        jpeg_image_data image;
        image = get_image_data(iter->file_path);
        if(image.size == 0){
            if(image.memory) free(image.memory);
            continue;
        }
        
        if(image.memory == NULL){
            continue;
        }

        if(id = nf_api_aicam_fr_face_add(arg->channel, image, iter->name, NULL, 0)){
            upload_count++;
            //printf("[%s:%d] upload success id[%d]\n", __func__, __LINE__, id);
        }

        if(arg->functor){
            memset(&message, 0, sizeof(FR_BULK_UPLOAD_CALLBACK_MESSAGE));
            message.type = FR_BULK_TYPE_UPLOAD;

            if(id > 0)
                message.result = DLVA_API_RET_OK;
            else
                message.result = id;

            message.id = id;
            message.index = count - 1;
            snprintf(message.path, sizeof(message.path), iter->file_path);
            arg->functor(message);
        }

        free(image.memory);
        free(iter);
    }
    
    //upload count
    if(upload_count > 0){
        //rc = aibox_fr_face_add_bulk_end(arg->aibox_ip);
        rc = DLVA_API_RET_OK;
        printf("[%s:%d] aibox_fr_face_add_bulk_end[%d] count[%d] upload_count[%d]\n", __func__, __LINE__, rc, count, upload_count);
    }else{
        rc = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
    }

    if(arg->functor){
        memset(&message, 0, sizeof(FR_BULK_UPLOAD_CALLBACK_MESSAGE));
        message.type = FR_BULK_TYPE_CONFIRM;
        message.result = rc;
        arg->functor(message);
    }
    

    free(arg);

    //return code
    return upload_count;

}

FR_FACE_BULK_UPLOAD_STATE nf_api_aicam_fr_face_bulk_upload(
        unsigned int channel,               // aicam channel
        const char *directory_path,         //upload root
        int max_face_count,                 //available count
        FR_UPLOAD_FILE_INFO **list_result,  // result list
        int *list_len,                      // result list len
        upload_result_callback functor)            //functor (optional)
{
	upload_file_info *list = NULL;
	upload_file_info *iter = NULL;
    pthread_t upload_th;

    int rc = 0;
    int i;
    int ret = FR_BULK_ARGUMENT_MISSING;

    int list_length = 0;
    FR_UPLOAD_FILE_INFO *upload_info_list = NULL;
    upload_thread_args *arg = NULL;

    /////////////////////////////////////////////////////////////////
    //argument error check
    /////////////////////////////////////////////////////////////////
    //argument check
    if(channel < 0 || channel >= AVAILABLE_MAX_CH){
        printf("[%s:%d] argument error channel[%d]\n", __func__, __LINE__, channel);
        return ret;
    }

    if(strlen(directory_path) <= 0)
    {
        printf("[%s:%d] argument check error ch[%d] directory_path[%s]\n", __func__, __LINE__, channel, directory_path);
        return ret;
    }

    //result pointer
    if(list_result == NULL || list_len == NULL)
    {
        printf("[%s:%d] argument check error list[%p] list_len[%p]\n", __func__, __LINE__, list, list_len);
        return ret;
    }

    *list_result = NULL;
    *list_len = 0;
    /////////////////////////////////////////////////////////////////
    // directory parse & list make
    /////////////////////////////////////////////////////////////////
    //list make
    list = fr_make_image_list(directory_path, &list_length);
    printf("[%s:%d] list[%p] len[%d]\n", __func__, __LINE__, list, list_length);

    if(list == NULL){
        ret = FR_BULK_FILE_NOT_FOUND;
        printf("[%s:%d] error list[%p]\n", __func__, __LINE__, list);
        goto endl;
    }

    if(list_length <= 0){
        ret = FR_BULK_FILE_NOT_FOUND;
        *list_result = NULL;
        *list_len = 0;

        free_fr_list(list);
        printf("[%s:%d] error list_length[%d]\n", __func__, __LINE__, list_length);
        goto endl;
    }

    if(list_length > max_face_count){
        ret = FR_BULK_FILE_AMOUNT_OVER;
        *list_result = NULL;
        *list_len = list_length;

        free_fr_list(list);
        goto endl;
    }

    /////////////////////////////////////////////////////////////////
    // result list setting(list array & list size)
    /////////////////////////////////////////////////////////////////
    upload_info_list = malloc(sizeof(FR_UPLOAD_FILE_INFO)*list_length);
    if(upload_info_list == NULL){
        free_fr_list(list);
        ret =  FR_BULK_ERROR;
        goto endl;
    }
    
    memset(upload_info_list, 0, sizeof(FR_UPLOAD_FILE_INFO)*list_length);
    iter = list;

    for(i = 0, iter = list; i < list_length; i++, iter = iter->next){
        if(iter == NULL){
            printf("[%s:%d] ?? error iter null\n", __func__, __LINE__);
            break;
        }

        snprintf(upload_info_list[i].file_path, sizeof(upload_info_list[i].file_path), iter->file_path);
        snprintf(upload_info_list[i].name, sizeof(upload_info_list[i].name), iter->name);
        snprintf(upload_info_list[i].group, sizeof(upload_info_list[i].group), iter->group);
    }

    //printf("[%s:%d] iter[%p]\n", __func__, __LINE__, iter);
    if(iter != NULL){
        printf("[%s:%d] ERROR! iter not null\n", __func__, __LINE__);
        //error
    }

    *list_len = list_length;
    *list_result = upload_info_list;

    /////////////////////////////////////////////////////////////////
    // thread make
    /////////////////////////////////////////////////////////////////
    arg = malloc(sizeof(upload_thread_args));
    if(arg == NULL){
        free_fr_list(list);
        free(*list_result);
        *list_result = NULL;

        ret =  FR_BULK_ERROR;
        goto endl;
    }

    memset(arg, 0, sizeof(upload_thread_args));
    arg->list = list;
    arg->functor = functor;
    arg->channel = channel;

    pthread_create(&upload_th, NULL, &aicam_face_bulk_upload_thread, (void *)arg);
    ret = FR_BULK_SUCC;

 endl:
    return ret;
}

static int _parse_aibox_rule_data(aibox_rule_data *rules, const char *trigger, const char *name, json_t *zone, int is_line)
{
    int i;
    int size = 0;
    json_t *point;
    if(rules == NULL || zone == NULL) return -1;

    size = json_array_size(zone);
    if(size <= 0) return -1;

    memset(rules, 0, sizeof(aibox_rule_data));

    snprintf(rules->event_type, sizeof(rules->event_type), "%s", str_null_to_blank(trigger));
    snprintf(rules->name, sizeof(rules->name), "%s", str_null_to_blank(name));

    /*
    if(size > (RULEDATA_MAX_ZONE_SIZE-1)){
        printf("[%s:%d] rule size overflow size[%d]\n", __func__, __LINE__, size);
        size = (RULEDATA_MAX_ZONE_SIZE-1);
    }
    rules->zone_size = size;
    */

	rules->type = _get_aibox_type(trigger);

    json_array_foreach(zone, i, point){
        if(i >= (RULEDATA_MAX_ZONE_SIZE - 1)){
            printf("[%s:%d] error zone size over current index[%d] aibox_zone_size[%d]\n", __func__, __LINE__, i, size);
            break;
        }
        rules->zone_list[i].x = json_real_value(json_object_get(point, "x"));
        rules->zone_list[i].y = json_real_value(json_object_get(point, "y"));
        printf("[%s:%d] aibox_rule_get rules[%d] [%f, %f]\n", __func__, __LINE__, i, rules->zone_list[i].x, rules->zone_list[i].y);
    }
    rules->zone_size = i;

    if(!is_line){
        rules->zone_list[i].x = rules->zone_list[0].x;
        rules->zone_list[i].y = rules->zone_list[0].y;
        (rules->zone_size)++;
    }
    return 0;
}

static int _get_json_rule_data_size(json_t *json_root, int aibox_ch)
{
    int size = 0;
    int index = 0;

    const char *trigger;

    json_t *json_value;
    json_t *json_list = NULL;
    json_t *json_zone = NULL;
    json_t *json_vsource = NULL;

    char *debug = NULL;

    json_object_foreach(json_object_get(json_root, "triggers"), trigger, json_list)
    {
        json_array_foreach(json_list, index, json_value)
        {
            json_zone = json_object_get(json_value, "zone");
            json_vsource = json_object_get(json_value, "vsource");

            if(json_zone && json_vsource){
                if((int)json_integer_value(json_vsource) != aibox_ch)
                    continue;
                size++;
            }
        }
    }

    return size;
}

//foward, enter - right
int nf_api_get_aibox_rules(int nvr_ch, aibox_rule_data **rules, int *rule_size)
{
	mtable *runtime = NULL;
    if(nvr_ch < 0 || rules == NULL || rule_size == NULL){
        printf("[%s:%d] argument check error nvr_ch[%d] rules[%p] rul_size[%p]\n", __func__, __LINE__, nvr_ch, rules, rule_size);
        return -1;
    }
    runtime = get_runtime();

    *rules = NULL;
    *rule_size = 0;

    if(runtime == NULL || runtime[nvr_ch].ai.rule_size < 0){
        printf("[%s:%d] ch[%d] rule not found - rule_size[%d]\n", __func__, __LINE__, nvr_ch, runtime[nvr_ch].ai.rule_size);
        return -1;
    }

    //*rules = runtime[nvr_ch].ai.rules;
    *rule_size = runtime[nvr_ch].ai.rule_size;

	if(*rule_size < 0){
		printf("[%s:%d] ch[%d] rule not found - rule_size[%d]\n", __func__, __LINE__, nvr_ch, runtime[nvr_ch].ai.rule_size);
	    return -1;
	}if(*rule_size > 32){
	    *rule_size = 32;
	}
	
    *rules = malloc(sizeof(aibox_rule_data) * (*rule_size));
    if(*rules == NULL){
	    printf("[%s:%d] ch[%d] rule not found - rule_size[%d]\n", __func__, __LINE__, nvr_ch, runtime[nvr_ch].ai.rule_size);
	    return -1;
	}
	
    memcpy(*rules, runtime[nvr_ch].ai.rules, sizeof(aibox_rule_data) * (*rule_size));

    return 0;
}

int nf_api_aibox_rule_update_all()
{
    int ret = 0;
	int size = 0;
    int i;
	unsigned int aibox_list[20] = {0,};
    int rc;

	size = nf_api_aibox_get_aibox_iplist(aibox_list);
    printf("[%s:%d] aibox iplist size[%d]\n", __func__, __LINE__, size);
	for(i = 0; i < size; i++){
        if(aibox_list[i]){
            rc = nf_api_aibox_update_rules(aibox_list[i], get_aibox_ch_mask(aibox_list[i]));
            if(rc == 0) ret++;
        }
    }

    if(ret > 0){
        nf_notify_fire_params("aibox_db_change", NF_AIBOX_DB_RULE, 0, 0, 0);
    }
    return ret;
}

unsigned int get_aibox_ch_mask(unsigned int ip)
{
    int i;
    unsigned int ch_mask = 0;

    if(ip == 0){
        char ip_buf[20];
        printf("[%s:%d] argument error ip[%s]\n", __func__, __LINE__, _ip_to_str(ip, ip_buf));
        return 0;
    }

    for(i = 0; i < AVAILABLE_MAX_CH; i++){
        if(ip == get_db_aibox_ip(i)){
            ch_mask |= (1<<i);
        }
    }

    return ch_mask;
}

static unsigned int get_db_aibox_ip(int ch)
{
    unsigned int ip = 0;
	GValue ret_value = {0,};
    if(ch < 0) return 0;

    if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL)){
        ip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    return ip;
}

static int _get_cam_ch(const char *rtsp_addr)
{
	    int i;
		char ip_buffer[200] = {0, };
		gchar key[128];

		if(nf_api_get_host_from_rtsp_url(rtsp_addr, ip_buffer) == NULL) return -1;
		if(strlen(ip_buffer) <= 0) return -1;

		for(i = 0; i < AVAILABLE_MAX_CH; i++){
			//camera ip, vcam ip
			snprintf(key, 128, "cam.logininfo.L%d.hostname", i);
			if(strncmp(ip_buffer, nf_sysdb_get_str_nocopy(key), strlen(ip_buffer)) == 0){
				return i;
			}
		}
										
		return -1;
}

int get_aibox_ch_map(unsigned int aibox_ip, int *ch_map)
{
	int ret = -1;
    int i;
	int cam_id;
    GList *iter;
	char owner[20] = {0, };

	nf_api_get_nvr_owner(owner);

	if(owner == NULL) goto endl;
	if(owner[0] == '\0') goto endl;

	GList *aibox_stream_list = nf_api_get_stream_list(aibox_ip);

    for(i = 0; i < AVAILABLE_MAX_CH; i++){
        ch_map[i] = -1;
    }

    /*owner 일치 or ip 일치시 ch_map[no] = ch;*/
	for(iter = aibox_stream_list; iter != NULL; iter = iter->next){
		aibox_ch_info *ch_info = (aibox_ch_info *)iter->data;
		if(ch_info == NULL) continue;

		if(strlen(ch_info->name) > 0){
			if(strncmp(owner, ch_info->name, strlen(owner)) == 0){
                ch_map[ch_info->no] = ch_info->ch;
                continue;
			}
		}

		if(strlen(ch_info->ip) > 0){
			if(is_nvr_ip(ch_info->ip)){
                ch_map[ch_info->no] = ch_info->ch;
                continue;
			}

			cam_id = _get_cam_ch(ch_info->ip);
			if(cam_id >= 0 && cam_id < AVAILABLE_MAX_CH){
				ch_map[ch_info->no] = cam_id;
				continue;
			}
		}
	}

    ret = 0;
endl:
	nf_api_stream_list_free(aibox_stream_list);
	return ret;
}

//aibox ip로부터 rules 정보를 가지고오고 ch_mask값기준으로 parsing 해서 runtime data update
//int rc = nf_api_aibox_update_rules(aibox_ip, get_aibox_ch_mask(aibox_ip));
int nf_api_aibox_update_rules(unsigned int aibox_ip, unsigned int ch_mask)
{
    int i;
    int ch;
    int rc;
    int aibox_ch_map[AVAILABLE_MAX_CH];
    int ret = -1;

    json_t *aibox_rules = NULL;
    mtable *runtime = get_runtime();

    int index;
    int nvr_ch;
    const char *trigger;
    json_t *json_list = NULL;
    json_t *json_zone = NULL;
    json_t *json_vsource = NULL;
    json_t *json_name= NULL;
    json_t *json_value;
    json_error_t error;

    if(ch_mask == 0){
        printf("[%s:%d] ch_mask[%08x]\n", __func__, __LINE__, ch_mask);
        goto endl;
    }

    //aibox_rules = _get_aibox_rule_data(aibox_ip);
	ret = _get_aibox_rule_data(aibox_ip, &aibox_rules);
    if(ret != DLVA_API_RET_OK || aibox_rules == NULL){
        //ret = -2;   //aibox rules get failed
		printf("[%s:%d] _get_aibox_rule_data[%d] aibox_ip[%s]\n", __func__, __LINE__, ret, _ip_to_str(aibox_ip, NULL));
        goto endl;
    }

    //channel mapping
    rc = get_aibox_ch_map(aibox_ip, aibox_ch_map);
    if(rc < 0){
        //ret = -3;
		printf("[%s:%d] get_aibox_ch_map[%d] aibox_ip[%s]\n", __func__, __LINE__, rc, _ip_to_str(aibox_ip, NULL));
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }
    /*
    for(i = 0; i < AVAILABLE_MAX_CH; i++){
        printf("[%s:%d] debug aibox_ch_map aibox[%d] -> nvr[%d]\n", __func__, __LINE__, i, aibox_ch_map[i]);
    }
    */

    //ch_mask 해당채널 초기화
    for(ch = 0; ch < AVAILABLE_MAX_CH; ch++){
        if(ch_mask & (1<<ch)){
            //reset
            runtime[ch].ai.rule_size = 0;
            memset(runtime[ch].ai.rules, 0, sizeof(runtime[ch].ai.rules));
        }
    }
    
    //parsing aibox rule data
    json_object_foreach(json_object_get(aibox_rules, "triggers"), trigger, json_list)
    {
        json_array_foreach(json_list, index, json_value)
        {
            json_zone = json_object_get(json_value, "zone");
            json_vsource = json_object_get(json_value, "vsource");
            json_name = json_object_get(json_value, "name");
            //json_object_get(json_value, "is_line");

            if(json_zone && json_vsource){
                //nvr_ch = aibox_ch_map[json_integer_value(json_vsource)];
				int vsource = (int)json_integer_value(json_vsource);
				if(vsource >= AVAILABLE_MAX_CH || vsource < 0){
				    printf("[%s:%d] vousrce[%d]\n", __func__, __LINE__, vsource);
				     continue;
				}
				
				nvr_ch = aibox_ch_map[vsource];
                if(nvr_ch == -1){
                    continue;
                }

                //ch_mack 확인
                if( ~ch_mask & (1<<nvr_ch) ){
                    continue;
                }

                if(runtime[nvr_ch].ai.rule_size >= 32){
                    continue;
                }

                //printf("[%s:%d] aibox_rule_get vsource[%d] -> nvr_ch[%d]\n", __func__, __LINE__, json_integer_value(json_vsource), nvr_ch);
				printf("[%s:%d] aibox_rule_get name[%s] vsource[%d] -> nvr_ch[%d]\n", __func__, __LINE__, json_string_value(json_object_get(json_value, "name")), json_integer_value(json_vsource), nvr_ch);

                rc = _parse_aibox_rule_data(
                        runtime[nvr_ch].ai.rules + (int)(runtime[nvr_ch].ai.rule_size),
                        trigger,
                        json_string_value(json_object_get(json_value, "name")),
                        json_zone,
                        json_is_true(json_object_get(json_value, "is_line")));

                if(rc < 0){
                    //fail
                }else{
                    runtime[nvr_ch].ai.rule_size++;
                }
            }
        }
    }

    //ret = 0;
	ret = DLVA_API_RET_OK;
endl:
    if(aibox_rules) json_decref(aibox_rules);
    return ret;
}


//static json_t *_get_aibox_rule_data(unsigned int aibox_ip)
int _get_aibox_rule_data(unsigned int aibox_ip, json_t **json_ret)
{
	int ret = DLVA_API_RET_FAILED_INIT;
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int alloc_size = 0;

    int nvr_ch;
    char id[129];
    char pw[129];

    int index;
    const char *trigger;
    json_t *json_root = NULL;
    json_error_t error;

    //argument check
	if(json_ret == NULL)
	{
		printf("[%s:%d] argument check error json_ret[%p] \n", __func__, __LINE__, json_ret);
	    return DLVA_API_RET_ARGUMENT_ERROR;
	}
	*json_ret = NULL;

    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] \n", __func__, __LINE__, aibox_ip);
        //goto endl;
		return DLVA_API_RET_ARGUMENT_ERROR;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        //printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
		printf("[%s:%d] argument check error nvr_ch[%d] aibox_ip[%s] \n", __func__, __LINE__, nvr_ch, _ip_to_str(aibox_ip, NULL));
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/es/v2/presets/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
		ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    /******************************
     * data set
     ******************************/

    json_root = json_loads(http_get_res(&ctx), 0, &error);
    if(json_root == NULL){
        fprintf(stderr, "[%s] json_root error on line %d: %s\n", __func__, error.line, error.text);
        fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

	ret = DLVA_API_RET_OK;
	*json_ret = json_root;

endl:
    http_release(&ctx);
    //return json_root;
	return ret;
}

int _vsource_nvr_ch_conn_status(json_t *vsource, int nvr_ch)
{
    int ret = 0;
    int no;
    int ch;
    json_t *json;

    json_array_foreach(vsource, no, json){
        ch = _get_chinfo_from_aibox_rtsp_url(json_string_value(json_object_get(json, "url")));
        /*
        printf("[%s:%d] nvr_ch[%d] url[%s] ch[%d] conntected[%d]\n", __func__, __LINE__, 
                nvr_ch, 
                json_string_value(json_object_get(json, "url")),
                ch, json_is_true(json_object_get(json, "connected")));
                */
        if(ch < 0){
            continue;
        }
        if(ch == nvr_ch){
            if(json_is_true(json_object_get(json, "connected"))){
                ret = 1;
                break;
            }
        }
    }

    return ret;
}

//nvr state = runtime[ch].ai.status

typedef enum _NF_AIBOX_CONN_STAT_E{
	NF_AIBOX_CONN_STATE_NONE                   = 0,
	NF_AIBOX_CONN_STATE_CONFIGURED             = 1<<0,
	NF_AIBOX_CONN_STATE_CONNECTED              = 1<<1,
	NF_AIBOX_CONN_STATE_NVRSTREAM_CONNECTED    = 1<<2,
	NF_AIBOX_CONN_STATE_STATE_MAX              = 1<<3,
}NF_AIBOX_CONN_STAT_E;


typedef int (*conn_state_functor)(json_t*);

struct _AIBOX_CONN_STATE_INFO
{
    struct _AIBOX_CONN_STATE_TYPE *type;
    time_t last_type_changed;

    int ch;
    time_t last_received;
    int nvr_state;
    unsigned int aibox_ip;
};
struct _AIBOX_CONN_STATE_TYPE
{
    unsigned int state;
    int (*change)(struct _AIBOX_CONN_STATE_INFO *state, int new_state);
};

typedef struct _AIBOX_CONN_STATE_INFO AIBOX_CONN_STATE_INFO;
typedef struct _AIBOX_CONN_STATE_TYPE AIBOX_CONN_STATE_TYPE;

static AIBOX_CONN_STATE_INFO aibox_conn_state[AVAILABLE_MAX_CH] = {0, };
static AIBOX_CONN_STATE_TYPE aibox_conn_type[NF_AIBOX_CONN_STATE_STATE_MAX] = {0, };

int nvr_state_change_map[8][8]={
    //prev 000
    {
        NF_AIBOX_CONFIG_OFF,        //000
        NF_AIBOX_CONFIG_ON,         //001
        -1,
        NF_AIBOX_CONNECTING,        //011
        -1, -1, -1,
        NF_AIBOX_CONN_SUCCESS       //111
    },
    //prev 001
    {
        NF_AIBOX_CONFIG_OFF,        //000
        NF_AIBOX_CONN_FAILED,       //001
        -1,
        NF_AIBOX_CONNECTING,        //011
        -1, -1, -1,
        NF_AIBOX_CONN_SUCCESS       //111
    },
    {-1, -1, -1, -1, -1, -1, -1, -1},
    //prev 011
    {
        NF_AIBOX_CONFIG_OFF,        //000
        NF_AIBOX_CONN_FAILED,       //001
        -1,
        NF_AIBOX_STREAM_CONN_FAILED,//011
        -1, -1, -1,
        NF_AIBOX_CONN_SUCCESS       //111
    },
    {-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1, -1, -1, -1},
    //prev 111
    {
        NF_AIBOX_CONFIG_OFF,        //000
        NF_AIBOX_CONN_FAILED,       //001
        -1,
        NF_AIBOX_STREAM_CONN_FAILED,//011
        -1, -1, -1,
        NF_AIBOX_CONN_SUCCESS       //111
    }
};


const char *keep_alive_str[7] = {
    "NF_AIBOX_CONFIG_OFF",
    "NF_AIBOX_CONFIG_ON",
    "NF_AIBOX_CONNECTING",
    "NF_AIBOX_CONN_SUCCESS",
    "NF_AIBOX_CONN_FAILED",
    "NF_AIBOX_STREAM_CONN_FAILED",
    ""
};

int change_func(AIBOX_CONN_STATE_INFO *state_info, int new_state_code)
{
    mtable *runtime = get_runtime();
    int prev_aibox_state = -1;
    int prev_nvr_state = -1;
    int next_nvr_state = -1;
    /*
    printf("[%s:%d] debug  start check state_info[%p] new_state_code[%d]\n", __func__, __LINE__, state_info, new_state_code);
    if(state_info){
        printf("[%s:%d] debug  start check last_type[%u] ch[%d] last_recv[%u] nvr_state[%d]\n",  __func__, __LINE__,
                state_info->last_type_changed,
                state_info->ch,
                state_info->last_received,
                state_info->nvr_state);
    }
    */

    if(state_info == NULL){
        printf("[%s:%d] error state_info[%p]\n", __func__, __LINE__, state_info);
        return -1;
    }

    if(state_info->type == NULL){
        printf("[%s:%d] error state_info->type[%p]\n", __func__, __LINE__, state_info->type);
        return -1;
    }

    if(new_state_code >= NF_AIBOX_CONN_STATE_STATE_MAX){
        printf("[%s:%d] error new_state_code[%d]\n", __func__, __LINE__, new_state_code);
        return -1;
    }

    //nvr type save
    prev_aibox_state = state_info->type->state;
    if(prev_aibox_state < 0 || prev_aibox_state >= NF_AIBOX_CONN_STATE_STATE_MAX){
        printf("[%s:%d] error prev_aibox_state[%d]\n", __func__, __LINE__, prev_aibox_state);
        return -1;
    }

    prev_nvr_state = state_info->nvr_state;
    if(prev_nvr_state < 0){
        printf("[%s:%d] error prev_nvr_state[%d]\n", __func__, __LINE__, prev_nvr_state);
        return -1;
    }

    //nvr type change
    next_nvr_state = nvr_state_change_map[prev_aibox_state][new_state_code];
    if(next_nvr_state < 0){
        printf("[%s:%d] error next_nvr_state[%d]\n", __func__, __LINE__, next_nvr_state);
        return -1;
    }

    state_info->nvr_state = next_nvr_state;

    //state change
    state_info->type = &aibox_conn_type[new_state_code];
    state_info->last_type_changed = time(NULL);

    runtime[state_info->ch].ai.status = next_nvr_state;

    //recovery mode
    if(next_nvr_state >= NF_AIBOX_CONN_FAILED){
        printf("[%s:%d] recovery ch[%d] state[%d]\n", __func__, __LINE__, state_info->ch, next_nvr_state);
        //aibox_cb_work_push(aibox_connection_recovery, state_info->ch);
    }

    //notify fire
    if(prev_nvr_state != next_nvr_state){
        printf("[%s:%d] notify! ch[%d] prev[aibox_state:%d, nvr_state:%d] next[aibox_state:%d, nvr_state:%d]\n", __func__, __LINE__, 
                state_info->ch,
                prev_aibox_state, prev_nvr_state, new_state_code, next_nvr_state);
        nf_notify_fire_params("ai_keep_alive", state_info->ch, prev_nvr_state, next_nvr_state, state_info->type->state);
    }else{
    #if 0 
		printf("[%s:%d] ch[%d] prev[aibox_state:%d, nvr_state:%d] next[aibox_state:%d, nvr_state:%d]\n", __func__, __LINE__, 
		                state_info->ch,
		                                prev_aibox_state, prev_nvr_state, new_state_code, next_nvr_state);
    #endif                                         
    }
    return 0;
}

static int _aibox_info_search(const unsigned char *mac, int search_time, aibox_connection_info *result)
{
    aibox_search_list *aibox_list = NULL;
    aibox_connection_info *p_info;

    if(mac == NULL || search_time < 0 || result == NULL)
    {
        printf("[%s:%d] argument error mac[%p] search_time[%d] result[%p]\n", __func__, __LINE__, mac, search_time, result);
        return 0;
    }

    //search
    aibox_list = nf_api_aibox_search_list(search_time);
    if(aibox_list == NULL){
        printf("[%s:%d] search_list is null search_time[%d]\n", __func__, __LINE__, search_time);
        return 0;
    }

    p_info = nf_api_aibox_list_get_from_mac(aibox_list, mac);
    if(p_info){
        printf("[%s:%d] search_result ip[%s] mac[%s]\n", __func__, __LINE__,
                _ip_to_str(p_info->ip, NULL), 
                _mac_to_str(p_info->mac, NULL));

        memcpy(result, p_info, sizeof(aibox_connection_info));
    }else{
        //aibox not found
        nf_api_aibox_list_free(aibox_list);
        printf("[%s:%d] aibox not found! mac[%s]\n", __func__, __LINE__, _mac_to_str(mac, NULL));

        //save log
        
        return 0;
    }
    nf_api_aibox_list_free(aibox_list);

    return 1;
}

static int _db_aibox_info(int ch, unsigned int *aibox_ip, unsigned char *mac)
{
    GValue ret_value = {0,};
    char mac_str[30] = {0, };

    if(aibox_ip == NULL || mac == NULL){
        return 0;
    }

    if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
    {
        *aibox_ip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }
    else
    {
        return 0;
    }

    if (nf_sysdb_get_key1("cam.ai_box.A%d.mac", ch, &ret_value, NULL))
    {
		g_stpcpy(mac_str, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
        sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        return 1;
    }

    return 0;
}

int get_host_info_s(host_inf **list)
{
    host_inf *iter;
    host_inf *current = NULL;
    int size = 0;

    if(list == NULL){
        return size;
    }

	pthread_mutex_lock(&_host_inf_mutex);
    _host_inf_free(host_inf_list);
    host_inf_list = NULL;
	get_host_info_list();

    for(iter = host_inf_list, size = 0; iter != NULL; iter = iter->next){
        if(current == NULL){
            *list = (host_inf *)malloc(sizeof(host_inf));
            current = *list;
        }else{
            current->next = (host_inf *)malloc(sizeof(host_inf));
            current = current->next;
        }

        if(current==NULL){
            break;
        }

        memcpy(current, iter, sizeof(host_inf));

        current->next = NULL;

        size++;
    }
	pthread_mutex_unlock(&_host_inf_mutex);

    return size;
}

static is_dev_wan(char *dev)
{
    if(strncmp(dev, "eth1", 4) == 0){
        return 0;
    }else{
        return 1;
    }
}

static int _is_zmq_path_available(host_inf *list, unsigned int ip, int is_wan)
{
	host_inf *current;

	if(ip == 0x0 || ip == 0xffffffff){
		return 0;
	}

	for(current = list; current != NULL; current = current->next)
	{
		if(current->ipaddr == ip && is_wan == is_dev_wan(current->dev)){
			return 1;
		}
	}
	return 0;
}

static int get_subnet_host_ip(host_inf *list, unsigned int ip, int is_wan)
{
	host_inf *current;
    int is_host_wan = 0;

    if(ip == 0x0 || ip == 0xffffffff){
        return 0;
    }

	for(current = list; current != NULL; current = current->next)
	{
		if((current->ipaddr&current->netmask) == (ip&current->netmask)
                // && ((dev == NULL) ? 1 : strcmp(dev, current->dev) == 0)
                 && is_wan == is_dev_wan(current->dev)
                ){
			return current->ipaddr;
		}
	}
	return 0;
}

int check_conn_info_matching(
        unsigned int db_aibox_ip,
        aibox_connection_info *aibox_search_info,
        host_inf *list)
{
    int ret = 0;

    if(aibox_search_info == NULL) return ret;

    printf("[%s:%d] db_aibox_ip[%s] is_wan[%d]\n", __func__, __LINE__, _ip_to_str(db_aibox_ip, NULL), _is_aibox_connected_wan(aibox_search_info));

    if(get_subnet_host_ip(list, aibox_search_info->ip, _is_aibox_connected_wan(aibox_search_info))){
        ret = 1;
        printf("[%s:%d] check ip[%s]\n", __func__, __LINE__, _ip_to_str(aibox_search_info->ip, NULL));
        if(aibox_search_info->ip == db_aibox_ip){
            return 2;
        }
    }

    if(get_subnet_host_ip(list, aibox_search_info->linklocal_ip, _is_aibox_connected_wan(aibox_search_info))){
        ret = 1;
        printf("[%s:%d] check linklocal_ip[%s]\n", __func__, __LINE__, _ip_to_str(aibox_search_info->linklocal_ip, NULL));
        if(aibox_search_info->linklocal_ip == db_aibox_ip){
            return 2;
        }
    }

    if(get_subnet_host_ip(list, aibox_search_info->cin_ip, _is_aibox_connected_wan(aibox_search_info))){
        ret = 1;
        printf("[%s:%d] check cin_ip[%s]\n", __func__, __LINE__, _ip_to_str(aibox_search_info->cin_ip, NULL));
        if(aibox_search_info->cin_ip == db_aibox_ip){
            return 2;
        }
    }
    
    return ret;
}

static void _hub_aibox_ipset_request(aibox_connection_info info, unsigned int ipaddr)
{
    int sock = -1;
	int on = 1;
    int len;
	struct ifreq ifr;
	struct sockaddr_in sin;

    netconf_msg ipsetmsg;
    
    /*
     * socket create
     */
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	struct timeval send_timeout;
	send_timeout.tv_sec = 5;
	send_timeout.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&send_timeout, sizeof(send_timeout));

    /*
     * message setting
     */
    memset((void*)&ipsetmsg, 0x00, sizeof(netconf_msg));
    ipsetmsg.version = 1;
    ipsetmsg.opcode = MSG_IP_SET;
    ipsetmsg.secs = 0;
    ipsetmsg.xid = info.transaction;
    ipsetmsg.magic = htonl(0x69547843);
    memcpy(&ipsetmsg.chaddr[0], info.mac,6);
    ipsetmsg.yiaddr = ipaddr;
    ipsetmsg.miaddr = inet_addr("255.255.255.0");
    ipsetmsg.giaddr = (ipaddr&inet_addr("255.255.255.0"))|inet_addr("0.0.0.1");

    /*
     * eth1 bind
     */

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth1");
	if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) 
	{
		char message[128] = {0};
		snprintf(message, 127, "[%s:%d] setsockopt(SO_BINDTODEVICE)", __FUNCTION__, __LINE__);
		perror(message);
	}

    /*
     * send message
     */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_BROADCAST;
	sin.sin_port = htons(_ADMIN_CLI_PORT);

    len = sendto(sock, (void*) &ipsetmsg, sizeof(netconf_msg), 0, (struct sockaddr*)&sin, sizeof(sin));

endl:
    printf("[%s:%d] hub aibox ipset requested[%d]\n", __func__, __LINE__, len);
    /*
     * close socket
     */
    if(sock >= 0){
        close(sock);
    }
}

static unsigned int make_aibox_ip_connected_hub()
{

#ifdef DUAL_LAN_NETWORK
	const unsigned int class_c_ipbase = get_hub_info() & get_host_netmask();
#else
	const unsigned int class_c_ipbase = get_host_info() & get_host_netmask();
#endif

    return class_c_ipbase | (155 << 24);
}

static unsigned int get_ip_from_zmq_uri(const char *buf_uri)
{
    const char *entry;
    unsigned int ip = 0;
    char ip_str[50];
    char *ptr;

    if(buf_uri == NULL){
        return ip;
    }

    entry = strstr(buf_uri, "://");
    if(entry == NULL){
        return ip;
    }
    entry += 3;

    snprintf(ip_str, sizeof(ip_str), "%s", entry);

    ptr = strstr(ip_str, "/");
    if(ptr)
        *ptr = '\0';

    ptr = strstr(ip_str, ":");
    if(ptr)
        *ptr = '\0';

    printf("[%s:%d] ip_str[%s]\n", __func__, __LINE__, ip_str);

    if(strlen(ip_str) > 0)
        ip = htonl(inet_addr(ip_str));

    return ip;
}

void aibox_connection_recovery(void *data)
{
	GValue set_value = {0,};
    unsigned int new_ipaddr = 0;
    int ch = (int)data;
    int i;

    unsigned char mac[8] = {0, };
    unsigned int aibox_ip;

    unsigned int nvr_host_ip;
    const nvr_host_interface[50];
    int rc;

    host_inf *host_info_list = NULL;
    size_t list_size;

    aibox_connection_info aibox_search_info;
    

    printf("[%s:%d] ch[%d]\n", __func__, __LINE__, ch);

    if(aibox_conn_state[ch].nvr_state < NF_AIBOX_CONN_FAILED && aibox_conn_state[ch].nvr_state != NF_AIBOX_CONFIG_ON){
        printf("[%s:%d] state is normal aibox_conn_state[ch].nvr_state[%d]\n", __func__, __LINE__, aibox_conn_state[ch].nvr_state);
        return;
    }

    list_size = get_host_info_s(&host_info_list);
    if(list_size <= 0){
        goto endl;
    }


    rc = _db_aibox_info(ch, &aibox_ip, mac);
    printf("[%s:%d] rc_db_aibox_info[%d]\n", __func__, __LINE__, rc);
    if(rc == 0) goto endl;

    rc = _aibox_info_search(mac, 5, &aibox_search_info);
    printf("[%s:%d] _aibox_info_search[%d]\n", __func__, __LINE__, rc);
    if(rc == 0) goto endl;

    switch(check_conn_info_matching(aibox_ip, &aibox_search_info, host_info_list))
    {
        case 0: // 가용한 IP 없음. eth1인경우 admin protocol을 이용해서 ip set request
            if(!nf_get_running_mode() && !_is_aibox_connected_wan(&aibox_search_info)){
                //ipset request
                new_ipaddr = make_aibox_ip_connected_hub();
                _hub_aibox_ipset_request(aibox_search_info, new_ipaddr);
                new_ipaddr = htonl(new_ipaddr);
                sleep(7);
            }
            break;

        case 1: // 가용한 IP 있지만 그 값이 DB값이 아님 ( wan ip 라면 최소한 linklocal ip는 있을듯 )
            if(get_subnet_host_ip(host_info_list, aibox_search_info.ip, _is_aibox_connected_wan(&aibox_search_info))){
                printf("[%s:%d] set ip[%s] is_wan[%d]\n", __func__, __LINE__, _ip_to_str(aibox_search_info.ip, NULL), _is_aibox_connected_wan(&aibox_search_info));
                new_ipaddr = aibox_search_info.ip;
            }else
            if(get_subnet_host_ip(host_info_list, aibox_search_info.linklocal_ip, _is_aibox_connected_wan(&aibox_search_info))){
                printf("[%s:%d] set linklocal_ip[%s] is_wan[%d]\n", __func__, __LINE__, _ip_to_str(aibox_search_info.ip, NULL), _is_aibox_connected_wan(&aibox_search_info));
                new_ipaddr = aibox_search_info.linklocal_ip;
            }else{
                printf("[%s:%d] error not set ip[%s] is_wan[%d]\n", __func__, __LINE__, _ip_to_str(aibox_search_info.ip, NULL), _is_aibox_connected_wan(&aibox_search_info));
            }
            break;

        case 2: // 가용한 IP가 있고 db에 설정되어있음. zmq stream check 필요함
            //if(0)   //zmq url 읽기 실패 상황시 recovery 재시도
            {
                char buf_uri[200];
                char aibox_owner[30];
                char nvr_owner[30];

                rc = _get_aibox_owner(aibox_ip, aibox_owner, NULL);
                if(rc == 0)
                {
                    nf_api_get_nvr_owner(nvr_owner);
                    //printf("[%s:%d] nvr_owner[%s] aibox_owner[%s] cmp[%d]\n", __func__, __LINE__, nvr_owner, aibox_owner, strncmp(nvr_owner, aibox_owner, 30));
                    if(strncmp(nvr_owner, aibox_owner, 30) == 0)
                    {
                        rc = _get_aibox_zmq_uri(aibox_ip, buf_uri);
                        printf("[%s:%d] check aibox_ip[%s] zmq_uri[%d][%s] \n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), rc, buf_uri);

                        if(rc == 0 && strlen(buf_uri) > 0)
                        {
                            //if(get_subnet_host_ip(host_info_list, get_ip_from_zmq_uri(buf_uri), _is_aibox_connected_wan(&aibox_search_info))){
							if(_is_zmq_path_available(host_info_list, get_ip_from_zmq_uri(buf_uri), _is_aibox_connected_wan(&aibox_search_info))){
                            //if(is_host_ip_uint(host_info_list, get_ip_from_zmq_uri(buf_uri))){
                                //normal state
                                printf("[%s:%d] ch[%d] aibox[%s] zmq_uri[%s] is normal\n", __func__, __LINE__, ch, _ip_to_str(aibox_ip, NULL), buf_uri);
                            }else{
                                //update zmq uri
                                set_aibox_zmq_uri( nvr_owner, aibox_ip, get_subnet_host_ip(host_info_list, aibox_ip, is_ip_wan(aibox_ip)) );

                                //update aibox's stream url list
                                rc = _aibox_update_video_stream(aibox_ip, AIBOX_CHANGE_INFO_IP|AIBOX_CHANGE_INFO_PORT);

                                printf("[%s:%d] ch[%d] aibox[%s] aibox_video_stream update[%d]\n", __func__, __LINE__, ch, _ip_to_str(aibox_ip, NULL), rc);
                            }
                        }
                    }
                    else
                    {
                        //aibox's owner is not mine(probably aibox is in universal mode)
                    }
                }
                else
                {
                    //connection failed
                }
            }
            break;

        default:
            printf("[%s:%d] not defined\n", __func__, __LINE__);
            break;
    }

    if(new_ipaddr != 0x00 && new_ipaddr != 0xffffffff){
        printf("[%s:%d] ch[%d] aibox db_set to [%s]\n", __func__, __LINE__, ch, _ip_to_str(new_ipaddr, NULL));

        g_value_init(&set_value, G_TYPE_UINT);
        g_value_set_uint(&set_value, new_ipaddr);
        nf_sysdb_set_key1("cam.ai_box.A%d.addr", ch, &set_value, NULL);
        g_value_unset(&set_value);

        nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);

        sleep(1);
        aibox_cb_work_push(aibox_connection_recovery, ch);
    }

endl:
    if(host_info_list) _host_inf_free(host_info_list);
}

static void aibox_zone_update(void *data)
{
    int ch = (int)data;
    int notify = 0;
    
    printf("[%s:%d] ch[%d]\n", __func__, __LINE__, ch);
    notify = nf_api_aibox_update_rules(get_db_aibox_ip(ch), 1<<ch);

    if(notify == 0)
    {
        printf("[%s:%d] notify aibox[%s] ch[%d]\n", __func__, __LINE__, 
                _ip_to_str(get_db_aibox_ip(ch), NULL),
                ch);
        nf_notify_fire_params("aibox_db_change", NF_AIBOX_DB_RULE, get_db_aibox_ip(ch), 1<<ch, 0);
    }
}

static void ai_conn_state_check_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	mtable *runtime = NULL;
    int ch = 0;
    int prev_state = 0;
    int curr_state = 0;
    int state = 0;
    int rc = 0;
    int notify_flag = 0;

	runtime = get_runtime();

	g_return_if_fail(runtime != NULL);
	g_return_if_fail(pinfo != NULL);

	ch = pinfo->d.params[0];
	prev_state = pinfo->d.params[1];
	curr_state = pinfo->d.params[2];
	state = pinfo->d.params[3];

    printf("[%s:%d] state_changed ch[%d] prev[%s] next[%s] state[%d]\n", __func__, __LINE__, ch, keep_alive_str[prev_state], keep_alive_str[curr_state], state);

    //error log
    if(curr_state >= NF_AIBOX_CONN_FAILED && prev_state < NF_AIBOX_CONN_FAILED){
        //정상 -> 에러시 로그
        //error log
    }

    //check zone info
    if(prev_state == NF_AIBOX_CONN_SUCCESS && curr_state != NF_AIBOX_CONN_SUCCESS){
        //connection failed. rule info reset
        
        runtime[ch].ai.rule_size = 0;
        memset(runtime[ch].ai.rules, 0, sizeof(runtime[ch].ai.rules));
        notify_flag = 1;
        printf("[%s:%d] ch[%d] data reset\n", __func__, __LINE__, ch);
    }else if(prev_state != NF_AIBOX_CONN_SUCCESS && curr_state == NF_AIBOX_CONN_SUCCESS){
        //connection reset. rule renew
        aibox_cb_work_push(aibox_zone_update, ch);

        printf("[%s:%d] ch[%d] data renew\n", __func__, __LINE__, ch);
    }else{
    }

    if(notify_flag){
        printf("[%s:%d] notify aibox[%s] ch[%d]\n", __func__, __LINE__, 
                _ip_to_str(get_db_aibox_ip(ch), NULL),
                ch);
        nf_notify_fire_params("aibox_db_change", NF_AIBOX_DB_RULE, get_db_aibox_ip(ch), 1<<ch, 0);
    }
}

void init_aibox_conn_state()
{
    int i;
    int state;

    printf("[%s:%d] keep alive process init\n", __func__, __LINE__);

    for(i = 0; i < NF_AIBOX_CONN_STATE_STATE_MAX; i = (i<<1) + 1){
        aibox_conn_type[i].state = i;
        aibox_conn_type[i].change = change_func;
    }

    memset(aibox_conn_state, 0, sizeof(aibox_conn_state));

    for(i = 0; i < AVAILABLE_MAX_CH; i++){
        aibox_conn_state[i].ch = i;
        aibox_conn_state[i].type = &aibox_conn_type[NF_AIBOX_CONN_STATE_NONE];
    }

	nf_notify_connect_cb("ai_keep_alive", ai_conn_state_check_func, NULL);
}

int state_change_self(AIBOX_CONN_STATE_INFO *state)
{
    if(state == NULL){
        printf("[%s:%d]  state is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type == NULL){
        printf("[%s:%d]  state->type is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type->change == NULL){
        printf("[%s:%d]  change func not set\n", __func__, __LINE__);
        return -1;
    }

    state->type->change(state, state->type->state);
    return 0;
}

int state_change_ai_config_change(AIBOX_CONN_STATE_INFO *state, unsigned int aibox_ip)
{
    if(state == NULL){
        printf("[%s:%d] error state is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type == NULL){
        printf("[%s:%d] error state->type is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type->change == NULL){
        printf("[%s:%d] error change func not set\n", __func__, __LINE__);
        return -1;
    }

    if(aibox_ip != 0){
        if(state->type->state && (state->aibox_ip != aibox_ip)){
            /*
             * ip가 다르면 초기화후 config on
             */
            {
                char buf1[20];
                char buf2[20];
                printf("[%s:%d] ip reset prev[%s] new[%s]\n", __func__, __LINE__,
                        _ip_to_str(state->aibox_ip, buf1),
                        _ip_to_str(aibox_ip, buf2));
            }
            state->type->change(state, NF_AIBOX_CONN_STATE_NONE);
        }

        //ipset & config on
        state->aibox_ip = aibox_ip;
        state->type->change(state, state->type->state | NF_AIBOX_CONN_STATE_CONFIGURED);
    }else{
        state->type->change(state, NF_AIBOX_CONN_STATE_NONE);
    }

    return 0;
}

int nf_api_ai_config_update(int ch)
{
    int rc = -1;
    mtable *runtime = get_runtime();
    GValue ret_value = {0,};
    unsigned int aibox_ip;

    printf("[%s:%d] aibox keepalive config update ch[%d]\n", __func__, __LINE__, ch);

    if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
    {
        aibox_ip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
        if(aibox_ip != 0){
            if((aibox_conn_state[ch].type->state & NF_AIBOX_CONN_STATE_CONFIGURED) && aibox_conn_state[ch].aibox_ip == aibox_ip){
                //do noting
            }else{
                rc = state_change_ai_config_change(&aibox_conn_state[ch], aibox_ip);
            }
        }else{
                rc = state_change_ai_config_change(&aibox_conn_state[ch], aibox_ip);
        }
    }

    return 0;
}

int state_change_ai_data_recv(AIBOX_CONN_STATE_INFO *state, time_t current)
{
    if(state == NULL){
        printf("[%s:%d]  state is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type == NULL){
        printf("[%s:%d]  state->type is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type->change == NULL){
        printf("[%s:%d]  change func not set\n", __func__, __LINE__);
        return -1;
    }

    if(!(state->type->state & NF_AIBOX_CONN_STATE_CONFIGURED)){
        //printf("[%s:%d] ch[%d] not configured. state[%d]\n", __func__, __LINE__, state->ch, state->type->state);
        return -1;
    }

    state->last_received = current;

    if(state->type->state & NF_AIBOX_CONN_STATE_CONNECTED){
        //already connected
    }else{
        state->type->change(state, state->type->state | NF_AIBOX_CONN_STATE_CONNECTED);
    }

    return 0;
}

int state_change_ai_keep_alive(AIBOX_CONN_STATE_INFO *state, int stream_status, time_t current)
{
    if(state == NULL){
        printf("[%s:%d]  state is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type == NULL){
        printf("[%s:%d]  state->type is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type->change == NULL){
        printf("[%s:%d]  change func not set\n", __func__, __LINE__);
        return -1;
    }

    if(!(state->type->state & NF_AIBOX_CONN_STATE_CONFIGURED)){
        //printf("[%s:%d] debug ch[%d] not configured. state[%d]\n", __func__, __LINE__, state->ch, state->type->state);
        return -1;
    }

    if(state->last_received != 0 && (current - state->last_received > 30)){
        printf("[%s:%d] ch[%d] aibox_conn_state[%d] nvr_state[%d] time[%u] | received old keepalive data stream_status[%d] -> [0]\n", __func__, __LINE__, state->ch, state->type->state, state->nvr_state, (unsigned int)(current - state->last_received), stream_status);
        stream_status = 0;
    }

    state->last_received = current;
    if(stream_status){
        if(state->type->state != 7){
            state->type->change(state, 7);
        }
    }else{
        if(state->type->state != 3){
            state->type->change(state, 3);
        }
    }

    return 0;
}

int state_change_ai_conn_close(AIBOX_CONN_STATE_INFO *state)
{
    if(state == NULL){
        printf("[%s:%d]  state is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type == NULL){
        printf("[%s:%d]  state->type is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(state->type->change == NULL){
        printf("[%s:%d]  change func not set\n", __func__, __LINE__);
        return -1;
    }

    state->type->change(state, state->type->state & NF_AIBOX_CONN_STATE_CONFIGURED);

    return 0;
}

int nf_api_aibox_connection_status_update(unsigned int aibox_ip, json_t *vsource)
{
    /*
     * 해당 IP가 연결된 채널 상태 update
     * aibox의 연결끊김이 발생한 채널이 있는지 확인 필요
     */
    int ch;
    time_t current;
	unsigned int vloss = get_vloss_status();
	int is_connecdted = 0;

    current = time(NULL);

    for(ch = 0; ch < AVAILABLE_MAX_CH; ch++){
		is_connecdted = !(vloss & (1 << ch));
		//printf();
        if( is_connecdted && aibox_ip == get_db_aibox_ip(ch)){
            state_change_ai_keep_alive(&aibox_conn_state[ch], _vsource_nvr_ch_conn_status(vsource, ch), current);
        }
    }
}

time_t nf_api_aibox_conn_time_update(int ch, time_t current)
{
    if(ch < 0) return 0;

    if(current == 0){
        current = time(NULL);
    }

    state_change_ai_data_recv(&aibox_conn_state[ch], current);
    
    return current;
}

int nf_api_aibox_conn_check()
{
    /*
       마지막 recv 시간 체크
       */
    mtable *runtime = get_runtime();
    int ch;
    time_t current;
    current = time(NULL);

    for(ch = 0; ch < AVAILABLE_MAX_CH; ch++){
        if(aibox_conn_state[ch].type->state == NF_AIBOX_CONN_STATE_NONE){
            continue;
        }

        /*
        printf("[%s:%d] debug conn check : ch[%d] prev[%u] curr[%u] last_type_changed[%u] recv_time[%d] state_change[%d]\n", __func__, __LINE__, ch,
                aibox_conn_state[ch].last_received, current, aibox_conn_state[ch].last_type_changed,
                (int)((long long)current - (long long)aibox_conn_state[ch].last_received),
                (int)((long long)current - (long long)aibox_conn_state[ch].last_type_changed));
                */
        //connection check
        if(aibox_conn_state[ch].last_received != 0 && (int)((long long)current - (long long)aibox_conn_state[ch].last_received) > 30){
            if(aibox_conn_state[ch].type->state & NF_AIBOX_CONN_STATE_CONNECTED){
                state_change_ai_conn_close(&aibox_conn_state[ch]);
            }
        }

        //last_type_changed check add
        if(aibox_conn_state[ch].last_type_changed != 0 && (int)((long long)current - (long long)aibox_conn_state[ch].last_type_changed) > 120){
            //if(aibox_conn_state[ch].nvr_state == NF_AIBOX_CONFIG_ON || aibox_conn_state[ch].nvr_state == NF_AIBOX_CONNECTING){
                state_change_self(&aibox_conn_state[ch]);
            //}
        }


        //runtime nvr type 과 aibox object의 nvr type 비교
        if(aibox_conn_state[ch].nvr_state != runtime[ch].ai.status){
            printf("[%s:%d] ch[%d] aibox status not matched!! nvr[%d] = aibox[%d]\n", __func__, __LINE__, ch, 
                    runtime[ch].ai.status, aibox_conn_state[ch].nvr_state);
            runtime[ch].ai.status = aibox_conn_state[ch].nvr_state;
        }

    }
}

unsigned int nf_api_get_aibox_connection_status(int ch)
{
	mtable *runtime = get_runtime();
    return runtime[ch].ai.status;
}

unsigned int aibox_hub_ipset(aibox_connection_info info)
{
    unsigned int ip = htonl(info.ip);
    if(!nf_get_running_mode() && !_is_aibox_connected_wan(&info)){
        printf("[%s:%d] ip[%s] is lan ip\n", __func__, __LINE__, _ip_to_str(info.ip, NULL));
        ip = make_aibox_ip_connected_hub();
        _hub_aibox_ipset_request(info, make_aibox_ip_connected_hub());
        printf("[%s:%d] set to ip[%s]\n", __func__, __LINE__, _ip_to_str(htonl(ip), NULL));
    }
    return htonl(ip);
}

void aibox_configure_status_update_all()
{
	mtable *runtime = get_runtime();
	GValue set_value = {0,};
	GValue ret_value = {0,};

    int ch;
    unsigned int aibox_ip;
    unsigned char mac[8] = {0, };
    char mac_str[30] = {0, };

    for(ch = 0; ch < AVAILABLE_MAX_CH; ch++){
        /*
         * init
         */
        aibox_ip = 0;
        memset(mac, 0, sizeof(mac));

        /*
         * get mac & ip
         */
        if (nf_sysdb_get_key1("cam.ai_box.A%d.mac", ch, &ret_value, NULL))
        {
            g_stpcpy(mac_str, g_value_get_string(&ret_value));
            g_value_unset(&ret_value);
            sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        }

        if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
        {
            aibox_ip = g_value_get_uint(&ret_value);
            g_value_unset(&ret_value);
        }

        /*
         * state update
         */
        state_change_ai_config_change(&aibox_conn_state[ch], aibox_ip);

        /*
         * aibox ip check
         * arp 테이블에서 mac으로 찾은후 정상값인지 arping 확인
         * 위에서 확인된 목록가지고 eth1에 연결됬는지확인후 eth0인경우 recovery 모드로 체크
         */
        if(aibox_ip == 0x00 || aibox_ip == 0xffffffff) continue;

        if(is_mac_wan(mac)){
            printf("[%s:%d] aibox mac[%s] ip[%s] is wan\n", __func__, __LINE__, _mac_to_str(mac, NULL), _ip_to_str(aibox_ip, NULL));
        }else{
            printf("[%s:%d] aibox mac[%s] ip[%s] is lan. check recovery\n", __func__, __LINE__, _mac_to_str(mac, NULL), _ip_to_str(aibox_ip, NULL));
            aibox_cb_work_push(aibox_connection_recovery, ch);
        }
    }
}


/***************************************************
 * AIBOX LPR Management functions
***************************************************/

int lpr_info_parser(json_t *json, lp_info *info)
{
    //const char *name;
    //const char *image_url;
    lpr_group_info *group_list = NULL;

    int i;
    int len = 0;
    json_t *json_array = NULL;
    json_t *json_group = NULL;

    //arg check
    if(info == NULL || json == NULL) return -1;

	memset(info, 0, sizeof(lp_info));

    info->id = (int)json_integer_value(json_object_get(json, "id"));
    //snprintf(info->memo, sizeof(info->memo), "%s", json_string_value(json_object_get(json, "memo")));
    //snprintf(info->name, sizeof(info->name), "%s", json_string_value(json_object_get(json, "name")));
    //snprintf(info->phone, sizeof(info->phone), "%s", json_string_value(json_object_get(json, "phone")));
    //snprintf(info->plate, sizeof(info->plate), "%s", json_string_value(json_object_get(json, "plate")));
	snprintf(info->memo, sizeof(info->memo),   "%s", str_null_to_blank(json_string_value(json_object_get(json, "memo"))));
	snprintf(info->name, sizeof(info->name),   "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
	snprintf(info->phone, sizeof(info->phone), "%s", str_null_to_blank(json_string_value(json_object_get(json, "phone"))));
	snprintf(info->plate, sizeof(info->plate), "%s", str_null_to_blank(json_string_value(json_object_get(json, "plate"))));
    
    if(info->id == 0) return -1;

    json_array = json_object_get(json, "groups"); 
    len = json_array_size(json_array);
    if(len){
        //group_list = malloc(sizeof(lpr_group_info) * len);
        //if(group_list == NULL) return -1;
        //memset(group_list, 0, sizeof(lpr_group_info) * len);
		group_list = info->group_list;
		if(len > MAX_AIBOX_DB_GROUP_SIZE) len = MAX_AIBOX_DB_GROUP_SIZE;

        for(i = 0; i < len; i++){
            json_group = json_array_get(json_array, i);
            
            group_list[i].id = (int)json_integer_value(json_object_get(json_group, "id"));
            //snprintf(group_list[i].name, sizeof(group_list[i].name), "%s", json_string_value(json_object_get(json_group, "name")));
            //snprintf(group_list[i].memo, sizeof(group_list[i].memo), "%s", json_string_value(json_object_get(json_group, "memo")));
			snprintf(group_list[i].name, sizeof(group_list[i].name), "%s", str_null_to_blank(json_string_value(json_object_get(json_group, "name"))));
			snprintf(group_list[i].memo, sizeof(group_list[i].memo), "%s", str_null_to_blank(json_string_value(json_object_get(json_group, "memo"))));

            if(group_list[i].id <= 0){
                printf("[%s:%d] group id parse error id[%d] index[%d]\n", __func__, __LINE__, group_list[i].id, i);
                if(json_group) free(json_group);
                return -1;
            }
        }

        info->group_length = len;
        //info->group_list = group_list;
    }
    //printf("[%s:%d] json_array[%p] len[%d]\n", __func__, __LINE__, json_array, len);

    return 0;
}

/*
 * lpr 
 */
static int get_plate_id_aibox_lpr(unsigned int aibox_ip, const char *str_plate)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;
    int ch;
	char id[129];
	char pw[129];

	json_t *json = NULL;
	json_t *plate_info = NULL;
	json_error_t error;

	int len = 0;
	int i;


	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || str_plate == NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] str_plate[%p]\n", __func__, __LINE__, aibox_ip, str_plate);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	len = strlen(str_plate);
	if(len <= 0)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] len[%d]\n", __func__, __LINE__, aibox_ip, len);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	ch = get_aibox_ch(aibox_ip);
	if(ch < 0 || ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/lps/");
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

	http_data_set(&ctx, HTTP_ADD_QUERY, "plate", str_plate);

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	json = json_loads(http_get_res(&ctx), 0, &error);
	if(json == NULL)
	{
		fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	ret = 0;
	json_array_foreach(json_object_get(json, "lps"), i, plate_info){
		const char *plate = json_string_value(json_object_get(plate_info, "plate"));
		if(strncmp(str_plate, plate, len) == 0){
			ret = json_integer_value(json_object_get(plate_info, "id"));
			break;
		}
	}

endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

//list get
int nf_api_aibox_lpr_list_get(
        //input
        unsigned int aibox_ip, 
        //const char *group_name, //null 입력시 전체
        int page_index,         //페이지 인덱스
        int page_size,          //한번에 보여줄 페이지 크기

		//search option
		int group_id,
		const lpr_db_search_option *search_option,

        //output
        int *max_page_num,
        lp_info **list_ret,
        int *length)
{
    //GET /api/rule/lpr/groups/
    lp_info *list = NULL;
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];
    char query_buffer[100];

    json_t *json = NULL;
    json_t *json_array = NULL;
    json_error_t error;

    int len = 0;
    int i;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || list_ret == NULL || length == NULL || page_index < 0) // ||  == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] list_ret[%p] length[%p]\n", __func__, __LINE__, aibox_ip, list_ret, length);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    *length = 0;
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
	page_index++;

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/lps/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    snprintf(query_buffer, sizeof(query_buffer), "%d", page_index);
    http_data_set(&ctx, HTTP_ADD_QUERY, "page", query_buffer);
    snprintf(query_buffer, sizeof(query_buffer), "%d", page_size);
    http_data_set(&ctx, HTTP_ADD_QUERY, "ipp", query_buffer);

	/*
	 * search options
	 */
	if(group_id > 0){
	    snprintf(query_buffer, sizeof(query_buffer), "%d", group_id);
		http_data_set(&ctx, HTTP_ADD_QUERY, "group", query_buffer);
	}else if(group_id == 0){
		//group_none
		http_data_set(&ctx, HTTP_ADD_QUERY, "no_group", "true");
	}

	if(search_option)
	{
		if(search_option->memo && strlen(search_option->memo) > 0)
			http_data_set(&ctx, HTTP_ADD_QUERY, "memo", search_option->memo);
		if(search_option->name && strlen(search_option->name) > 0)
			http_data_set(&ctx, HTTP_ADD_QUERY, "name", search_option->name);
		if(search_option->phone && strlen(search_option->phone) > 0)
			http_data_set(&ctx, HTTP_ADD_QUERY, "phone", search_option->phone);
		if(search_option->plate && strlen(search_option->plate) > 0)
			http_data_set(&ctx, HTTP_ADD_QUERY, "plate", search_option->plate);
	}
    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    if(max_page_num){
		if(page_size <= 0){
			*max_page_num = 1;
		}else{
			int total = json_integer_value(json_object_get(json, "total"));
			*max_page_num = total / page_size + (total % page_size > 0);
		}
        //*max_page_num = json_integer_value(json_object_get(json, "total"));
    }

    json_array = json_object_get(json, "lps");
    len = json_array_size(json_array);
    if(len > 0){
        list = malloc(sizeof(lp_info) * len);
        if(list == NULL){
            printf("[%s:%d] malloc fail rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
            ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
            goto endl;
        }
    }else{
        printf("[%s:%d] nodata rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        if(json_typeof(json_array) == JSON_ARRAY){
            ret = DLVA_API_RET_OK;
        }else{
            ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        }
        goto endl;
    }

    memset(list, 0, sizeof(lp_info) * len);

    for(i = 0; i < len; i++){
        if(lpr_info_parser(json_array_get(json_array, i), &list[i]) != 0){
            ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
            goto err;
        }
    }

    ret = DLVA_API_RET_OK;

endl:
    *length = len;
    *list_ret = list;
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;

err:
    if(list) free(list);
    list = NULL;
    len = 0;
    goto endl;
}

int nf_api_aibox_lpr_get(unsigned int aibox_ip, int lpr_id, lp_info *info)
{
    //GET /api/rule/lpr/groups/
	int rc;
	HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

	int ch;
	char id[129];
	char pw[129];
	char query_buffer[100];

	json_t *json = NULL;
	json_error_t error;

	char path[300];
	int i;

	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || lpr_id <= 0 || info == NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] lpr_id[%d] info[%p]\n", __func__, __LINE__, aibox_ip, lpr_id, info);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	ch = get_aibox_ch(aibox_ip);
	if(ch < 0 || ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}
	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	snprintf(path, sizeof(path), "/api/lpr/v1/lps/%d/", lpr_id);

	http_data_set(&ctx, HTTP_SET_PATH, path);
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	json = json_loads(http_get_res(&ctx), 0, &error);
	if(json == NULL)
	{
		fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	ret = lpr_info_parser(json, info);
	//ret = DLVA_API_RET_OK;
	//printf("[%s:%d] ret[%d]\n", __func__, __LINE__, ret);

endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
err:
	goto endl;
}

//list free
void free_lp_info_list(lp_info *list, size_t length)
{
    int i;
    if(list == NULL){
        printf("[%s:%d] arg check error list[%p] length[%d]\n", __func__, __LINE__, list, length);
        return;
    }

    for(i = 0; i < length; i++){
        if(list[i].group_list){
            free(list[i].group_list);
        }
    }

    free(list);
}

void free_lp_info(lp_info *object)
{
    /*
	if(object->group_list != NULL)
    	free(object->group_list);
    */
	free(object);
}

static int get_lpr_group(unsigned int aibox_ip, int group_id, lpr_group_info *info)
{
	int rc;
	HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

	int ch;
	char id[129];
	char pw[129];

	json_t *json = NULL;
	json_t *group_info = NULL;
	json_error_t error;

	int len = 0;
	int i;

	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL || group_id < 0)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] group_id[%d] info[%p]\n", __func__, __LINE__, aibox_ip, group_id, info);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	ch = get_aibox_ch(aibox_ip);
	if(ch < 0 || ch >= AVAILABLE_MAX_CH)
	{
		printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	http_init(&ctx);
	aibox_http_default_setting(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/groups/");
	http_data_set(&ctx, HTTP_SET_METHOD, "GET");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

	rc = http_request(&ctx);
	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}

	json = json_loads(http_get_res(&ctx), 0, &error);
	if(json == NULL)
	{
		fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	ret = DLVA_API_RET_AIBOX_DATA_NOT_FOUND;
	json_array_foreach(json, i, group_info){
		if(json_integer_value(json_object_get(group_info, "id")) == group_id){
			info->id = group_id;
			snprintf(info->name, sizeof(info->name),   "%s", str_null_to_blank(json_string_value(json_object_get(group_info, "name"))));
			snprintf(info->memo, sizeof(info->memo),   "%s", str_null_to_blank(json_string_value(json_object_get(group_info, "memo"))));
			ret = DLVA_API_RET_OK;
			goto endl;
		}
	}
endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

static int get_group_id_aibox_lpr(unsigned int aibox_ip, const char *group_name)
{
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_t *group_info = NULL;
    json_error_t error;

    int len = 0;
    int i;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || group_name== NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] group_name[%p]\n", __func__, __LINE__, aibox_ip, group_name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    len = strlen(group_name);
    if(len <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] len[%d]\n", __func__, __LINE__, aibox_ip, len);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/groups/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    ret = 0;
    json_array_foreach(json, i, group_info){
        const char *name = NULL;
        name = json_string_value(json_object_get(group_info, "name"));
        if(strncmp(group_name, name, len) == 0){
            ret = json_integer_value(json_object_get(group_info, "id"));
            break;
        }
    }
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

//add
int nf_api_lpr_add(unsigned int aibox_ip, lp_info *info)
{
    lp_info *list = NULL;
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];
	char *post = NULL;

    json_t *json = NULL;
    json_t *groups = NULL;
    json_error_t error;

    int len = 0;
    int i;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

	json = json_pack("{s:s,s:s,s:s,s:s,s:[]}",
			"plate", info->plate,
			"name",  info->name,
			"phone", info->phone,
			"memo",  info->memo,
			"groups");

    //if(group_name != NULL && strlen(group_name) > 0){
    //    int group_id = get_group_id_aibox_lpr(aibox_ip, group_name);
    //    //printf("[%s:%d] group_id[%d]\n", __func__, __LINE__, group_id);
    //     if(group_id > 0){
    //        json_array_append_new(json_object_get(json, "groups"), json_integer(group_id));
    //    }
    //}
	for(i = 0; i < info->group_length; i++){
		const char *group_name = info->group_list[i].name;
		int group_id = info->group_list[i].id;

		if(group_id > 0){
			json_array_append_new(json_object_get(json, "groups"), json_integer(group_id));
		}else if(strlen(group_name) > 0){
			group_id = get_group_id_aibox_lpr(aibox_ip, group_name);
			if(group_id > 0){
				json_array_append_new(json_object_get(json, "groups"), json_integer(group_id));
			}else{
				group_id = nf_api_lpr_group_add(aibox_ip, group_name);
				if(group_id > 0){
					json_array_append_new(json_object_get(json, "groups"), json_integer(group_id));
				}
			}
		}
	}

	if(json == NULL){
		return DLVA_API_RET_FAILED_INIT;
	}
	post = json_dumps(json, JSON_ENCODE_ANY);
    if(json){
        json_decref(json);
        json = NULL;
    }

	if(post == NULL){
		return DLVA_API_RET_FAILED_INIT;
	}

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/lps/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    ret = json_integer_value(json_object_get(json_array_get(json, 0), "id"));
endl:
	if(post) free(post);
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

//modify
int nf_api_lpr_modify(unsigned int aibox_ip, lp_info *info)
{
    lp_info *list = NULL;
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];
	char *post = NULL;
    char path[300];

    json_t *json = NULL;
    json_t *groups = NULL;
    json_error_t error;

    int len = 0;
    int i;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    if(info->id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info->id[%d]\n", __func__, __LINE__, aibox_ip, info->id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

	json = json_pack("{s:s,s:s,s:s,s:s,s:[]}",
			"plate", info->plate,
			"name",  info->name,
			"phone", info->phone,
			"memo",  info->memo,
			"groups");

    for(i = 0; i < info->group_length; i++){
        int group_id = -1;

        if(info->group_list[i].id > 0){
            group_id = info->group_list[i].id;
        }else if(strlen(info->group_list[i].name) > 0){
            group_id = get_group_id_aibox_lpr(aibox_ip, info->group_list[i].name);
        }

        if(group_id > 0){
            json_array_append_new(json_object_get(json, "groups"), json_integer(group_id));
        }
    }

	if(json == NULL){
		return DLVA_API_RET_FAILED_INIT;
	}
	post = json_dumps(json, JSON_ENCODE_ANY);
    if(json){
        json_decref(json);
        json = NULL;
    }

	if(post == NULL){
		return DLVA_API_RET_FAILED_INIT;
	}

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);

    snprintf(path, sizeof(path), "/api/lpr/v1/lps/%d/", info->id);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "PUT");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    ret = json_integer_value(json_object_get(json_array_get(json, 0), "id"));
endl:
	if(post) free(post);
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}


//delete
int aibox_lpr_delete(unsigned int aibox_ip, int plate_id)
{
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];

    char path[300];

    json_t *json = NULL;
    json_t *group_info = NULL;
    json_error_t error;

    int len = 0;
    int i;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        //printf("[%s:%d] argument check error aibox_ip[%08x] lpr_id[%d]\n", __func__, __LINE__, aibox_ip, lpr_id);
		printf("[%s:%d] argument check error aibox_ip[%08x]\n", __func__, __LINE__, aibox_ip);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

	if(plate_id <= 0){
		printf("[%s:%d] get channel failed aibox_ip[%s] plate_id[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), plate_id);
	    return 0;
	}

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);

    snprintf(path, sizeof(path), "/api/lpr/v1/lps/%d/", plate_id);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 204/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    ret = DLVA_API_RET_OK;
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int nf_api_aibox_lpr_delete_all(unsigned int aibox_ip)//, const char *group_name/* NULL : deelte All */)
{
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_t *group_info = NULL;
    json_error_t error;

    int len = 0;
    int i;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x]\n", __func__, __LINE__, aibox_ip);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);

    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/bulk/");
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    if(ctx.status != 204/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    ret = DLVA_API_RET_OK;
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

/*
 * lpr - group
 */
int nf_api_lpr_group_list_get(unsigned int aibox_ip, lpr_group_info **list, int *length)
{
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_t *group_info = NULL;
    json_error_t error;

    int len = 0;
    int i;

    if(length == NULL){
        printf("[%s:%d] argument check error aibox_ip[%08x] list[%p] length[%p]\n", __func__, __LINE__, aibox_ip, list, length);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    *length = 0;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || list == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] list[%p] length[%p]\n", __func__, __LINE__, aibox_ip, list, length);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/groups/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    *length = json_array_size(json);

    if(*length <= 0){
        ret = 0;
        goto endl;
    }

    *list = malloc( sizeof(lpr_group_info) * (*length) );

    json_array_foreach(json, i, group_info){
        (*list)[i].id  = (int)json_integer_value(json_object_get(group_info, "id"));
        snprintf((*list)[i].name, sizeof((*list)[i].name), "%s", json_string_value(json_object_get(group_info, "name")));
        snprintf((*list)[i].memo, sizeof((*list)[i].memo), "%s", json_string_value(json_object_get(group_info, "memo")));
    }
    ret = 0;
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int nf_api_lpr_group_add(unsigned int aibox_ip, const char *group_name)
{
    // /api/lpr/v1/groups/
    // 201
    // POST
    // {"name":"aa"}
    int rc;
    HTTP_CTX ctx;
    int ret_id = 0;
    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;
    char *post = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || group_name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] group_name ptr[%p]\n", __func__, __LINE__, aibox_ip, group_name);
        return 0;
    }

    if(strlen(group_name) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] group_name length[%d]\n", __func__, __LINE__, aibox_ip, strlen(group_name));
        return 0;
    }
    
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    http_init(&ctx);


    //post data
    json = json_pack_ex(&error, 0, "{s:s}",
            "name", group_name);
    post = json_dumps(json, JSON_ENCODE_ANY);
    if(post == NULL){
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }
    json_decref(json);
    json = NULL;

    //set http
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/groups/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    //printf("[%s:%d] http_request[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        goto endl;
    }
    ret_id = (int)json_integer_value(json_object_get(json, "id"));
endl:
    if(json) json_decref(json);
    if(post) free(post);
    http_release(&ctx);
    return ret_id;
err:
    goto endl;
}

int aibox_lpr_group_delete(unsigned int aibox_ip, int group_id)// const char *group_name)
{
    // /api/lpr/v1/groups/group_id/
    // 204
    // DELETE
    
    //DELETE /api/rule/fr/groups/<group_id>/
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = 0;

    //int group_id;

    int ch;
    char id[129];
    char pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        //printf("[%s:%d] argument check error aibox_ip[%08x] group_name[%p]\n", __func__, __LINE__, aibox_ip, group_name);
        return 0;
    }

    if(group_id <= 0){
        //printf("[%s:%d] argument check error aibox_ip[%08x] group_name[%s]\n", __func__, __LINE__, aibox_ip, group_name);
		printf("[%s:%d] get channel failed aibox_ip[%s] group_id[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), group_id);
        return 0;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return 0;
    }

    //group_id = get_group_id_aibox_lpr(aibox_ip, group_name);
    //if(group_id <= 0){
    //    printf("[%s:%d] get channel failed aibox_ip[%s] group_name[%s] group_id[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), group_name, group_id);
    //    return 0;
    //}
    snprintf(path, sizeof(path), "/api/lpr/v1/groups/%d/", group_id);

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 204){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    ret = 1;

endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}
//csv

static void free_csv_line( char **parsed )
{
    char **ptr;

    for ( ptr = parsed; *ptr; ptr++ ) {
        free( *ptr );
    }

    free( parsed );
}

static int count_fields( const char *line )
{
    const char *ptr;
    int cnt, fQuote;

    for ( cnt = 1, fQuote = 0, ptr = line; *ptr; ptr++ )
    {
        if ( fQuote )
        {
            if ( *ptr == '\"' )
            {
                if ( ptr[1] == '\"' )
                {
                    ptr++;
                    continue;
                }
                fQuote = 0;
            }
            continue;
        }

        switch( *ptr )
        {
            case '\"':
                fQuote = 1;
                continue;
            case ',':
                cnt++;
                continue;
            default:
                continue;
        }
    }

    if ( fQuote ) {
        return -1;
    }

    return cnt;
}

static char **parse_csv( const char *line )
{
    char **buf, **bptr, *tmp, *tptr;
    const char *ptr;
    int fieldcnt, fQuote, fEnd;

    fieldcnt = count_fields( line );

    if ( fieldcnt == -1 ) {
        return NULL;
    }

    buf = malloc( sizeof(char*) * (fieldcnt+1) );

    if ( !buf ) {
        return NULL;
    }

    tmp = malloc( strlen(line) + 1 );

    if ( !tmp )
    {
        free( buf );
        return NULL;
    }

    bptr = buf;

    for ( ptr = line, fQuote = 0, *tmp = '\0', tptr = tmp, fEnd = 0; ; ptr++ )
    {
        if ( fQuote )
        {
            if ( !*ptr ) {
                break;
            }

            if ( *ptr == '\"' )
            {
                if ( ptr[1] == '\"' )
                {
                    *tptr++ = '\"';
                    ptr++;
                    continue;
                }
                fQuote = 0;
            }
            else {
                *tptr++ = *ptr;
            }

            continue;
        }

        switch( *ptr )
        {
            case '\"':
                fQuote = 1;
                continue;
            case '\0':
                fEnd = 1;
            case ',':
                *tptr = '\0';
                *bptr = strdup( tmp );

                if ( !*bptr )
                {
                    for ( bptr--; bptr >= buf; bptr-- ) {
                        free( *bptr );
                    }
                    free( buf );
                    free( tmp );

                    return NULL;
                }

                bptr++;
                tptr = tmp;

                if ( fEnd ) {
                    break;
                } else {
                    continue;
                }

            default:
                *tptr++ = *ptr;
                continue;
        }

        if ( fEnd ) {
            break;
        }
    }

    *bptr = NULL;
    free( tmp );
    return buf;
}

static char *rtrim(char *str)
{
    int len;
    int i;
    if(str == NULL) return 0;

    len = (int)strlen(str);
    for(i = len-1; i >= 0; i--){
        if(str[i] == ' ' ||
                str[i] == '\n' ||
                str[i] == '\r' ||
                str[i] == '\t'){
            str[i] = '\0';
        }else{
            break;
        }
    }
    return str;
}

json_t *csv_to_json(const char *file_path)
{
    FILE *file = NULL;

    char buffer[500];
    const char **parsed;
    const char **iter;

    json_t *json_ret = NULL;
    json_t *json_keys = NULL;
    json_t *json_plates = NULL;
    const char *str_plates = NULL;
    
	int i;
	int column_length = 0;

    if(file_path == NULL){
        printf("[%s:%d] file_path error file_path[%s]\n", __func__, __LINE__, file_path);
        return NULL;
    }

    file = fopen(file_path, "r");
    if(file == NULL){
        printf("[%s:%d] fopen error file_path[%s]\n", __func__, __LINE__, file_path);
        return NULL;
    }

    /*
     * json add keys
     */
    if(fgets(buffer, sizeof(buffer), file) == NULL){
        goto endl;
    }
    rtrim(buffer);

    json_keys = json_array();
    if(json_keys == NULL){
        goto endl;
    }

    //make key array
    parsed = parse_csv(buffer);
    for(iter = parsed; *iter; iter++)
    {
        //printf("[%s:%d] keys iter[%s]\n", __func__, __LINE__, *iter);
		if(*iter == NULL) break;
		if(strlen(*iter) <= 0) break;
        json_array_append_new(json_keys, json_string(*iter));
		column_length++;
    }
    free_csv_line(parsed);

    if(json_array_size(json_keys) <= 0){
        goto endl;
    }

	printf("[%s:%d] column length[%d]\n", __func__, __LINE__, column_length);

    json_ret = json_object();
    json_object_set(json_ret, "keys", json_keys);


    /*
     * json add plates
     */
    json_plates = json_array();
    if(json_plates == NULL){
        goto endl;
    }

    while(fgets(buffer, sizeof(buffer), file))
    {
        json_t *json_array_line = json_array();
        if(json_array_line == NULL){
            printf("[%s:%d] error!! json_array memory allocation failed\n", __func__, __LINE__);
            break;
        }

        rtrim(buffer);
        parsed = parse_csv(buffer);
		for(i = 0, iter = parsed; i < column_length; i++, iter++){
	        if(*iter){
		        json_array_append_new(json_array_line, json_string(*iter));
		    }else{
		    	json_array_append_new(json_array_line, json_string(""));
			}
		}

		/*
        for(iter = parsed; *iter; iter++)
        {
            //printf("[%s:%d] plates iter[%s]\n", __func__, __LINE__, *iter);
            json_array_append_new(json_array_line, json_string(*iter));
        }
		*/
        free_csv_line(parsed);

        json_array_append(json_plates, json_array_line);
        json_decref(json_array_line);
    }

    if(json_plates){
        str_plates = json_dumps(json_plates, JSON_ENCODE_ANY);
    }

    if(str_plates){
        json_object_set_new(json_ret, "plates", json_string(str_plates));
    }

endl:
    if(json_keys) json_decref(json_keys);
    if(str_plates) free(str_plates);
    if(json_plates) json_decref(json_plates);
    fclose(file);
    return json_ret;
}


//bulk upload
int nf_api_lpr_bulk_upload(unsigned int aibox_ip, const char *file_path)
{
    int rc;
    HTTP_CTX ctx;
    int ret = 0;
    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_error_t error;
    char *post = NULL;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || file_path == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] file_path[%p]\n", __func__, __LINE__, aibox_ip, file_path);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    if(strlen(file_path) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] file_path length[%d]\n", __func__, __LINE__, aibox_ip, strlen(file_path));
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    
    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);

    //post data
    json = csv_to_json(file_path);
    if(json == NULL){
        printf("[%s:%d] csv to json error file_path[%s]\n", __func__, __LINE__, file_path);
        ret = DLVA_API_RET_FAILED_INIT;
        goto endl;
    }
    post = json_dumps(json, JSON_ENCODE_ANY);
    if(post == NULL){
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_FAILED_INIT;
        goto endl;
    }
    json_decref(json);
    json = NULL;

    //printf("[%s:%d] post[%s]\n", __func__, __LINE__, post);

    //set http
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/lpr/v1/bulk/");
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));
	http_data_set(&ctx, HTTP_SET_POST, post);

    rc = http_request(&ctx);
    printf("[%s:%d] http_request[%d] ctx.status[%d]\n", __func__, __LINE__, rc, ctx.status);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 201/* Created */){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d]\n", __func__, __LINE__, ctx.status);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }
    ret = DLVA_API_RET_OK;
endl:
    if(json) json_decref(json);
    if(post) free(post);
    http_release(&ctx);
    return ret;
err:
    goto endl;
}


/******************************************************************************

                              Triggers

******************************************************************************/


///////////////////////////////////////////////////////////////
// add action
// 1. Trigger Add. 
// 2. get LPR event action
// 3. action rules bind ( trigger ~ event action)

///////////////////////////////////////////////////////////////
// delete action
// 1. delete action rules
// 2. delete Trigger add

///////////////////////////////////////////////////////////////
// modify  action
// 1. modify trigger

///////////////////////////////////////////////////////////////
// lpr trigger      | get modify add delete
// lpr action rules | add delete
// lpr event action | get add

int aibox_add_lpr_trigger(unsigned int aibox_ip, lpr_trigger_info *info)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    
	json_t *json = NULL;
	json_error_t error;
	char *str_json = NULL;

	int i;
	char buff[200];
	char path[200];

	int nvr_ch;
	char nvr_id[129];
	char nvr_pw[129];

	//argument check
	if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
	{
		printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	nvr_ch = get_aibox_ch(aibox_ip);
	if(nvr_ch < 0)
	{
		printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
		return DLVA_API_RET_ARGUMENT_ERROR;
	}

	snprintf(path, sizeof(path), "/api/rule/es/triggers/lpr/");
	http_init(&ctx);
	aibox_http_default_setting_multipart(&ctx, aibox_ip);
	http_data_set(&ctx, HTTP_SET_PATH, path);
	http_data_set(&ctx, HTTP_SET_METHOD, "POST");
	http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
	http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

	//name
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "name", info->name);

	//vsource
	snprintf(buff, sizeof(buff), "%d", info->aibox_ch);
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "vsource", buff);

	// ìì

	//zone
	//snprintf(buff, sizeof(buff), "[{\"x\":0.01,\"y\":0.2},{\"x\":0.75,\"y\":0.2},{\"x\":0.75,\"y\":0.8},{\"x\":0.25,\"y\":0.8}]");
	json = json_array();
	for(i = 0; i < info->zone_size; i++){
		json_t *json_point = json_object();
		json_object_set_new(json_point, "x", json_real(info->zone[i].x));
		json_object_set_new(json_point, "y", json_real(info->zone[i].y));
		json_array_append_new(json, json_point);
	}
	str_json = json_dumps(json, JSON_ENCODE_ANY);
	printf("[zone] %s\n", str_json);
	//http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", "[{\"x\":0.06375,\"y\":0.073333},{\"x\":0.9475,\"y\":0.1},{\"x\":0.9475,\"y\":0.7},{\"x\":0.4475,\"y\":0.7}]");//str_json);
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", str_json);

	free(str_json);
	json_decref(json);
	json = NULL;

	//groups
	json = json_array();
	for(i = 0; i < 20 && i < info->group_size; i++){
		if(strlen(info->group_list[i]) <= 0) break;
		json_array_append_new(json, json_string(info->group_list[i]));
	}
	str_json = json_dumps(json, JSON_ENCODE_ANY);
	printf("[groups] %s\n", str_json);
	//http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", "[\"a\"]");//str_json);
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", str_json);

	free(str_json);
	json_decref(json);
	json = NULL;

	/*
	 * ?? value
	 */
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "rmode", info->rmode);
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "policy", info->policy);

	/*
	 * fixed value
	 */
	//label_conf
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "label_conf", "{\"x\":0.8,\"y\":0.05,\"w\":0.15,\"h\":0.1}");

	//reset
	http_data_set(&ctx, HTTP_ADD_MULTIPART, "reset", "-1");


	/*
	 * http request
	 */
	rc = http_request(&ctx);

	if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
		printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
		ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
		goto endl;
	}


	if(ctx.status != 201){
		printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
		ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
		goto endl;
	}

	if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
		printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
		ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
		goto endl;
	}

	//printf("[%s:%d] memory[%s]\n", __func__, __LINE__, ctx.res_data.memory);
	json = json_loads(http_get_res(&ctx), 0, &error);
	ret = (int)json_integer_value(json_object_get(json, "id"));

endl:
	if(json) json_decref(json);
	http_release(&ctx);
	return ret;
}

int aibox_delete_trigger(unsigned int aibox_ip, int trigger_id, AIBOX_RULE_TYPE_E type)
{
    // /api/rule/es/triggers/lprs/<id>/
    //DELETE /api/rule/fr/faces/<face_id>/
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = 0;
	
    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    const char *type_str;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] \n", __func__, __LINE__, aibox_ip);
        return 0;
    }

    if(trigger_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] trigger_id[%d]\n", __func__, __LINE__, aibox_ip, trigger_id);
        return 0;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    switch(type)
    {
        case RULE_TYPE_LPR:
            type_str = "lprs";
            break;
        case RULE_TYPE_FR:
            type_str = "frs";
            break;
        default:
            type_str = "";
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    snprintf(path, sizeof(path), "/api/rule/es/triggers/%s/%d/", type_str, trigger_id);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 204){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    printf("[%s:%d] debug rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);

    ret = 1;

endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static int set_json_to_lpr_trigger(lpr_trigger_info *info, json_t *json)
{
    json_t *zone = NULL;
    json_t *group = NULL;
    int i;

    if(info == NULL || json == NULL){
        printf("[%s:%d] error lpr_trigger_info[%p] json[%p]\n", __func__, __LINE__, info, json);
        return -2;
    }

    info->id = (int)json_integer_value(json_object_get(json, "id"));
    info->aibox_ch = (int)json_integer_value(json_object_get(json, "vsource"));

    snprintf(info->name, sizeof(info->name), "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
    snprintf(info->policy, sizeof(info->policy), "%s", str_null_to_blank(json_string_value(json_object_get(json, "policy"))));
    snprintf(info->rmode, sizeof(info->rmode), "%s", str_null_to_blank(json_string_value(json_object_get(json, "rmode"))));


    info->group_size = json_array_size(json_object_get(json, "groups"));
    json_array_foreach(json_object_get(json, "groups"), i, group)
    {
        snprintf(info->group_list[i], sizeof(info->group_list[i]), "%s", str_null_to_blank(json_string_value(group)));
        //printf("[%s:%d] %s\n", __func__, __LINE__, info->group_list[i]);
    }

    info->zone_size = json_array_size(json_object_get(json, "zone"));
    json_array_foreach(json_object_get(json, "zone"), i, zone)
    {
        info->zone[i].x = json_real_value(json_object_get(zone, "x"));
        info->zone[i].y = json_real_value(json_object_get(zone, "y"));
        //printf("[%s:%d] %03d | %f %f\n", __func__, __LINE__, i, info->zone[i].x, info->zone[i].y);
    }


    return 0;
}

int aibox_get_lpr_trigger(unsigned int aibox_ip, unsigned int trigger_id, const char *trigger_name, lpr_trigger_info *info)
{
    int ret = -1;
    json_t *rules = NULL;
    json_t *lprs = NULL;
    json_t *trigger = NULL;
    
    const char *name;
    int id;
    int i;
    int len;

    if(info == NULL){
        printf("[%s:%d] trigger_name[%p] info[%p]\n", __func__, __LINE__, trigger_name, info);
        return ret;
    }

    if(trigger_id == 0){
        if(trigger_name == NULL){
            printf("[%s:%d] trigger_name[%p] trigger_id[%d]\n", __func__, __LINE__, trigger_name, trigger_id);
            return ret;
        }

        len = strlen(trigger_name);
        if(len <= 0){
            printf("[%s:%d] trigger_name[%s]\n", __func__, __LINE__, trigger_name);
            return ret;
        }
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        goto endl;
    }

    ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
    memset(info, 0, sizeof(lpr_trigger_info));
    json_array_foreach(json_object_get(json_object_get(rules, "triggers"), "LPR"), i, trigger)
    {
        set_json_to_lpr_trigger(info, trigger);
        if(trigger_id == info->id){
            ret = DLVA_API_RET_OK;
            break;
        }

        if(trigger_name){
            if(strcmp(info->name, trigger_name) == 0){
                ret = DLVA_API_RET_OK;
                break;
            }
        }

        memset(info, 0, sizeof(lpr_trigger_info));
    }

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}

int aibox_modify_lpr_trigger(unsigned int aibox_ip, lpr_trigger_info *info)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    json_t *json = NULL;
    json_error_t error;
    char *str_json = NULL;

    int i;
    char buff[200];
    char path[200];

    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    snprintf(path, sizeof(path), "/api/rule/es/triggers/lprs/%d/", info->id);
    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "PUT");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    //name
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "name", info->name);

    //vsource
    snprintf(buff, sizeof(buff), "%d", info->aibox_ch);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "vsource", buff);

    // ìì
    
    //zone
    //snprintf(buff, sizeof(buff), "[{\"x\":0.01,\"y\":0.2},{\"x\":0.75,\"y\":0.2},{\"x\":0.75,\"y\":0.8},{\"x\":0.25,\"y\":0.8}]");
    json = json_array();
    for(i = 0; i < info->zone_size; i++){
        json_t *json_point = json_object();
        json_object_set_new(json_point, "x", json_real(info->zone[i].x));
        json_object_set_new(json_point, "y", json_real(info->zone[i].y));
        json_array_append_new(json, json_point);
    }
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[zone] %s\n", str_json);
    //http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", "[{\"x\":0.06375,\"y\":0.073333},{\"x\":0.9475,\"y\":0.1},{\"x\":0.9475,\"y\":0.7},{\"x\":0.4475,\"y\":0.7}]");//str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", str_json);

    free(str_json);
    json_decref(json);
    json = NULL;

    //groups
    json = json_array();
    for(i = 0; i < 20 && i < info->group_size; i++){
    if(strlen(info->group_list[i]) <= 0) break;
        json_array_append_new(json, json_string(info->group_list[i]));
    }
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[groups] %s\n", str_json);
    //http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", "[\"a\"]");//str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", str_json);

    free(str_json);
    json_decref(json);
    json = NULL;

    /*
     * ?? value
     */
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "rmode", "ingroup");
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "policy", "veryhigh");

    /*
     * fixed value
     */
    //reset
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "n", "1");

    //label_conf
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "label_conf", "{\"x\":0.8,\"y\":0.05,\"w\":0.15,\"h\":0.1}");

    //reset
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "reset", "-1");


    /*
     * http request
     */
    rc = http_request(&ctx);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }


    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    //printf("[%s:%d] memory[%s]\n", __func__, __LINE__, ctx.res_data.memory);
    json = json_loads(http_get_res(&ctx), 0, &error);
    ret = (int)json_integer_value(json_object_get(json, "id"));

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int aibox_add_action_rules(unsigned int aibox_ip, const char *rule_name, const char *trigger_name, int aibox_ch, sequrinet_event_action *event_action)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    json_t *json = NULL;
    json_error_t error;
    char *str_json = NULL;

    int i;
    char buff[200];
    char path[200];

    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || rule_name == NULL || trigger_name == NULL || event_action == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] rule_name[%p] trigger_name[%p] event_action[%p]\n", __func__, __LINE__, aibox_ip, rule_name, trigger_name, event_action);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    if(strlen(rule_name) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] rule_name[%s]\n", __func__, __LINE__, aibox_ip, rule_name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
	    
    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    snprintf(path, sizeof(path), "/api/rule/es/actionrule/");
    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    //name
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "name", rule_name);

    http_data_set(&ctx, HTTP_ADD_MULTIPART, "cond_triggers", "[]");
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "cond_schedules", "[]");
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "cond_disarm", "1");
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "cond_ndzs", "[]");

    json = json_pack_ex(&error, 0, "[{s:s, s:s, s:s, s:i, s:b}]",
            "type", "LPR",
            "type_label", "LPR",
            "name", trigger_name, 
            "vsource", aibox_ch,
            "is_dup", 0);
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[%s:%d] triggers[%s]\n", __func__, __LINE__, str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "triggers", str_json);
    free(str_json);
    json_decref(json);
    json = NULL;

    json = json_pack_ex(&error, 0, "[{s:i, s:i, s:s, s:s, s:s}]",
            "id", event_action->id,
            "type", 130,//event_action->type,
            "type_name", "Sequrinet",
            "type_label", "Sequrinet",
            "name", event_action->name);
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[%s:%d] actions[%s]\n", __func__, __LINE__, str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "actions", str_json);
    free(str_json);
    json_decref(json);
    json = NULL;

    /*
     * http request
     */
    rc = http_request(&ctx);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }


    if(ctx.status != 201){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    //printf("[%s:%d] memory[%s]\n", __func__, __LINE__, ctx.res_data.memory);
    json = json_loads(http_get_res(&ctx), 0, &error);
    ret = (int)json_integer_value(json_object_get(json, "id"));

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int aibox_delete_action_rules(unsigned int aibox_ip, int id)
{
    // /api/rule/es/actionrules/1/
    // delete
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = 0;

    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] id[%p]\n", __func__, __LINE__, aibox_ip, id);
        return 0;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
    snprintf(path, sizeof(path), "/api/rule/es/actionrules/%d/", id);


    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "DELETE");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 204){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }


    ret = 1;

endl:
    //if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static int set_json_to_action_rule(sequrinet_action_rule *info, json_t *json)
{
    json_t *zone = NULL;
    json_t *group = NULL;
    int i;

    if(info == NULL || json == NULL){
        return -2;
    }

    //integer
    info->id            = (int)json_integer_value(json_object_get(json, "id"));
    info->vsource       = (int)json_integer_value(json_object_get(json, "vsource"));

    //string
    snprintf(info->name,            sizeof(info->name),       "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
    snprintf(info->trigger_name,    (info->trigger_name),     "%s", str_null_to_blank(json_string_value(json_object_get(json, "trigger_name"))));
    snprintf(info->trigger_type,    (info->trigger_type),     "%s", str_null_to_blank(json_string_value(json_object_get(json, "trigger_type"))));

    return 0;
}

int aibox_get_action_rules(unsigned int aibox_ip, int rule_id, const char *name, const char *trigger_name, sequrinet_action_rule *info)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    json_t *json = NULL;
    json_t *action_rule = NULL;
    json_error_t error;
    char *str_json = NULL;
    
    unsigned int host_ip = 0;

    int i;
    char buff[200];
    char path[200];
    sequrinet_action_rule current;


    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    if((trigger_name != NULL && strlen(trigger_name) == 0) && (name != NULL && strlen(name) == 0) == 0 && rule_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p] rule_id[%d]\n", __func__, __LINE__, aibox_ip, info, rule_id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    if(trigger_name != NULL && strlen(trigger_name) == 0){
        trigger_name = NULL;
    }
    if(name != NULL && strlen(name) == 0){
        name = NULL;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/es/actionrules/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }


    json = json_loads(http_get_res(&ctx), 0, &error);

    json_array_foreach(json, i, action_rule)
    {
        set_json_to_action_rule(&current, action_rule);

        const char *str = json_string_value(json_object_get(action_rule, "name"));

        //check id
        if(rule_id > 0){
            if(rule_id == current.id){
                ret = 0;
                memcpy(info, &current, sizeof(sequrinet_action_rule));
                break;
            }
        }

        if(name){
            if(strcmp(name, current.name) == 0){
                ret = 0;
                memcpy(info, &current, sizeof(sequrinet_action_rule));
                break;
            }
        }

        if(trigger_name){
            if(strcmp(trigger_name, current.trigger_name) == 0){
                ret = 0;
                memcpy(info, &current, sizeof(sequrinet_action_rule));
                break;
            }
        }
    }

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static int set_json_to_event_action(sequrinet_event_action *info, json_t *json)
{
    json_t *zone = NULL;
    json_t *group = NULL;
    int i;

    if(info == NULL || json == NULL){
        return -2;
    }

    //integer
    info->id            = (int)json_integer_value(json_object_get(json, "id"));
    info->channel       = (int)json_integer_value(json_object_get(json, "channel"));
    info->duration      = (int)json_integer_value(json_object_get(json, "duration"));
    info->port          = (int)json_integer_value(json_object_get(json, "port"));
    info->proto_checker = (int)json_integer_value(json_object_get(json, "proto_checker"));
    info->recorder_type = (int)json_integer_value(json_object_get(json, "recorder_type"));

    //string
    snprintf(info->name,        sizeof(info->name),         "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
    snprintf(info->authuser,    sizeof(info->authuser),     "%s", str_null_to_blank(json_string_value(json_object_get(json, "authuser"))));
    snprintf(info->authpass,    sizeof(info->authpass),     "%s", str_null_to_blank(json_string_value(json_object_get(json, "authpass"))));
    snprintf(info->host,        sizeof(info->host),         "%s", str_null_to_blank(json_string_value(json_object_get(json, "host"))));
    snprintf(info->caption_msg, sizeof(info->caption_msg),  "%s", str_null_to_blank(json_string_value(json_object_get(json, "caption_msg"))));
    snprintf(info->desc_msg,    sizeof(info->desc_msg),     "%s", str_null_to_blank(json_string_value(json_object_get(json, "desc_msg"))));
    snprintf(info->title_msg,   sizeof(info->title_msg),    "%s", str_null_to_blank(json_string_value(json_object_get(json, "title_msg"))));
    snprintf(info->msg,         sizeof(info->msg),          "%s", str_null_to_blank(json_string_value(json_object_get(json, "msg"))));

    //boolean
    info->is_msg    = (int)json_is_true(json_object_get(json, "is_msg"));
    info->is_panic  = (int)json_is_true(json_object_get(json, "is_panic"));
    info->is_ssl    = (int)json_is_true(json_object_get(json, "ssl"));

    return 0;
}

int aibox_get_event_action(unsigned int aibox_ip, const char *search_name, sequrinet_event_action *info)
{
    int rc;
    HTTP_CTX ctx;
    char path[200];
    int ret = -1;

    int nvr_ch;
    char id[129];
    char pw[129];

    int index;
    json_t *json  = NULL;
    json_error_t error;

    int len;
    json_t *event_action;
    int i;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL || search_name == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] search_name[%p] info[%p]\n", __func__, __LINE__, aibox_ip, search_name, info);
        goto endl;
    }

    len = strlen(search_name);
    if(len <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] search_name[%s]\n", __func__, __LINE__, aibox_ip, search_name);
        goto endl;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/es/actions/sequrinets/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        goto endl;
    }

    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR rc[%d] status[%d] body[%s]\n", __func__, __LINE__, rc, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    /******************************
     * data set
     ******************************/

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL){
        fprintf(stderr, "[%s] json error on line %d: %s\n", __func__, error.line, error.text);
        fprintf(stderr, "[%s] status[%d] res[%s]\n", __func__, ctx.status, ctx.res_data.memory);
        goto endl;
    }

    ret = -2;
    json_array_foreach(json, i, event_action)
    {
        const char *name = json_string_value(json_object_get(event_action, "name"));
        //printf("[%s:%d] %03d | name[%s]\n", __func__, __LINE__, i, name);
        if(strncmp(search_name, name, len) == NULL){
            set_json_to_event_action(info, event_action);
            ret = 0;
            break;
        }
    }

endl:
    http_release(&ctx);
    if(json) json_decref(json);

    return ret;
}

int aibox_add_event_action(unsigned int aibox_ip, sequrinet_event_action *event_action)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    json_t *json = NULL;
    json_error_t error;
    char *str_json = NULL;

    int i;
    char buff[200];
    char path[200];
    
    int nvr_ch;
    char id[129];
    char pw[129];

	char name[50];
	char pass[50];
    unsigned int host_ip;
    int webport;
	GValue ret_value = {0,};

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || event_action == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] event_action[%p]\n", __func__, __LINE__, aibox_ip, event_action);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    if(strlen(event_action->name) <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] event_action_name[%s]\n", __func__, __LINE__, aibox_ip, event_action->name);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    snprintf(path, sizeof(path), "/api/rule/es/actions/sequrinet/");
    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, pw));

    //name
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "name", event_action->name);

    pthread_mutex_lock(&_host_inf_mutex);
    host_ip = get_subnet_host_ip(get_host_info_list(), aibox_ip, is_ip_wan(aibox_ip));
    pthread_mutex_unlock(&_host_inf_mutex);
    _ip_to_str(host_ip, buff);
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"host", buff);


	if(nf_sysdb_get_key0("net.proto.webport", &ret_value, NULL))
	{
		webport = g_value_get_uint(&ret_value);
		g_value_unset(&ret_value);
        snprintf(buff, sizeof(buff), "%d", webport);
        http_data_set(&ctx, HTTP_ADD_MULTIPART,"port", buff);
	}
    else
    {
        http_data_set(&ctx, HTTP_ADD_MULTIPART,"port", "80");
    }
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"ssl", "false");


	//user
	if(nf_sysdb_get_key1("usr.U%d.name", 0, &ret_value, NULL))
	{
		g_stpcpy(name, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
        http_data_set(&ctx, HTTP_ADD_MULTIPART,"authuser", name);
	}
    else
    {
        http_data_set(&ctx, HTTP_ADD_MULTIPART,"authuser", "ADMIN");
    }

	//pass
	if(nf_sysdb_get_key1("usr.U%d.pass", 0, &ret_value, NULL))
	{
		g_stpcpy(pass, g_value_get_string(&ret_value));
		g_value_unset(&ret_value);
        http_data_set(&ctx, HTTP_ADD_MULTIPART,"authpass", pass);
	}
    else
    {
        http_data_set(&ctx, HTTP_ADD_MULTIPART,"authpass", "1234");
    }

    http_data_set(&ctx, HTTP_ADD_MULTIPART,"is_msg", "true");
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"msg", "");
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"is_panic", "false");
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"duration", "1");
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"channel", "1");

    http_data_set(&ctx, HTTP_ADD_MULTIPART,"caption_msg", event_action->caption_msg);
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"title_msg", event_action->title_msg);
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"desc_msg", event_action->desc_msg);
    http_data_set(&ctx, HTTP_ADD_MULTIPART,"proto_checker", "1");

    /*
     * http request
     */
    rc = http_request(&ctx);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }


    if(ctx.status != 201){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    //printf("[%s:%d] memory[%s]\n", __func__, __LINE__, ctx.res_data.memory);
    json = json_loads(http_get_res(&ctx), 0, &error);
    //ret = (int)json_integer_value(json_object_get(json, "id"));
    set_json_to_event_action(event_action, json);
    ret = event_action->id;
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

///////////////////////////////////////////////////////////////
// add action
// 1. Trigger Add. 
// 2. get LPR event action
// 3. action rules bind ( trigger ~ event action)

///////////////////////////////////////////////////////////////
// delete action
// 1. delete action rules
// 2. delete Trigger add

///////////////////////////////////////////////////////////////
// modify  action
// 1. modify trigger

///////////////////////////////////////////////////////////////
// lpr trigger      | get modify add delete
// lpr action rules | add delete
// lpr event action | get add

static char *fr_trigger_gtype[3] =
{
    "attr",
    "comparison",
    "unregi"
};

static char *aibox_gender[3] =
{
    "all",
    "male",
    "female"
};

char *lpr_trigger_mode[3] =
{
    "all",
    "ingroup",
    "unregi"
};

char *lpr_trigger_policy[3] =
{
    "veryhigh",
    "high",
    "normal"
};

static int set_json_to_lpr_rule(unsigned int aibox_ip, lpr_rule *info, json_t *json)
{
    json_t *zone = NULL;
    json_t *group = NULL;
    int group_id;
    int i;

    const char *rmode;
    const char *policy;

    if(info == NULL || json == NULL){
        return -2;
    }

    info->type = RULE_TYPE_LPR;
    info->trigger_id = (int)json_integer_value(json_object_get(json, "id"));
    info->aibox_ch = (int)json_integer_value(json_object_get(json, "vsource"));

    snprintf(info->name, sizeof(info->name), "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
    rmode = str_null_to_blank(json_string_value(json_object_get(json, "rmode")));
    for(i = 0; i < sizeof(lpr_trigger_mode)/sizeof(char *); i++){
        //printf("[%s:%d] %d : rmode - %s\n", __func__, __LINE__, i, lpr_trigger_mode[i]);
        if(strcmp(lpr_trigger_mode[i], rmode) == 0){
            info->rmode = i;
            break;
        }
    }

    policy = str_null_to_blank(json_string_value(json_object_get(json, "policy")));
    for(i = 0; i < sizeof(lpr_trigger_mode)/sizeof(char *); i++){
        //printf("[%s:%d] %d : policy - %s\n", __func__, __LINE__, i, lpr_trigger_policy[i]);
        if(strcmp(lpr_trigger_policy[i], policy) == 0){
            info->policy = i;
            break;
        }
    }


    /*********
     * group
     *********/
    info->group_size = json_array_size(json_object_get(json, "groups"));
    json_array_foreach(json_object_get(json, "groups"), i, group)
    {
        group_id = get_group_id_aibox_lpr(aibox_ip, str_null_to_blank(json_string_value(group)));
        if(group_id < 0) group_id = 0;
        info->group_id_list[i] = group_id;
    }

    /*********
     * zone
     *********/
    info->zone_size = json_array_size(json_object_get(json, "zone"));
    json_array_foreach(json_object_get(json, "zone"), i, zone)
    {
        info->zone[i].x = json_real_value(json_object_get(zone, "x"));
        info->zone[i].y = json_real_value(json_object_get(zone, "y"));
    }


    return 0;
}

int nf_api_get_lpr_rule_by_id(unsigned int aibox_ip, unsigned int trigger_id, lpr_rule *rule)
{
    int ret = DLVA_API_RET_FAILED_INIT;
    json_t *rules = NULL;
    json_t *lprs = NULL;
    json_t *trigger = NULL;
    json_t *lpr_list = NULL;
    
    const char *name;
    int i;

    if(rule == NULL || trigger_id == 0){
        printf("[%s:%d] rule[%p] trigger_id[%d]\n", __func__, __LINE__, rule, trigger_id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        goto endl;
    }
    lpr_list = json_object_get(json_object_get(rules, "triggers"), "LPR");
    
    ret = DLVA_API_RET_AIBOX_DATA_NOT_FOUND;
    memset(rule, 0, sizeof(lpr_rule));
    json_array_foreach(lpr_list, i, trigger)
    {
        set_json_to_lpr_rule(aibox_ip, rule, trigger);
        if(rule->trigger_id == trigger_id){
            ret = DLVA_API_RET_OK;
            break;
        }

        memset(rule, 0, sizeof(lpr_rule));
    }

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}

int nf_api_get_lpr_rules(unsigned int aibox_ip, lpr_rule **rule, int *rule_size)
{
    int ret = DLVA_API_RET_FAILED_INIT;
    json_t *rules = NULL;
    json_t *trigger = NULL;
    json_t *lpr_list = NULL;
    
    const char *name;
    int i;
    int len;

    if(rule == NULL || rule_size == NULL){
        printf("[%s:%d] rule[%p] rule_size[%p]\n", __func__, __LINE__, rule, rule_size);
        return ret;
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        goto endl;
    }
    lpr_list = json_object_get(json_object_get(rules, "triggers"), "LPR");
    

    //memory alloc
    len = json_array_size(lpr_list);
    if(len <= 0){
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }
    *rule = malloc(len * sizeof(lpr_rule));
    if(*rule == NULL){
        ret = DLVA_API_RET_FAILED_INIT;
        goto endl;
    }
    
    json_array_foreach(lpr_list, i, trigger)
    {
        set_json_to_lpr_rule(aibox_ip, *rule + i, trigger);
    }

    *rule_size = i;
    ret = DLVA_API_RET_OK;

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}

static AIBOX_RULE_TYPE_E _get_aibox_type(const char *string)
{
    AIBOX_RULE_TYPE_E type;
    if(strcmp(string, "FR") == 0){
        type = RULE_TYPE_FR;
    }else
    if(strcmp(string, "LPR") == 0){
        type = RULE_TYPE_LPR;
    }else{
        type = RULE_TYPE_UNKNOWN;
    }

    return type;
} 

static const char *_get_aibox_type_string(AIBOX_RULE_TYPE_E type)
{
    const char *ret = NULL;
    switch(type)
    {
        case RULE_TYPE_LPR:
            ret = "LPR";
            break;
        case RULE_TYPE_FR:
            ret = "FR";
            break;
        default:
            ret = "";
    }

    return ret;
} 

int nf_api_add_rule(unsigned int aibox_ip, aibox_rule *rule)
{
    int rc;
    int ret = 0;
    int i;

    char nvr_owner[20];
    char event_action_name[200];

    sequrinet_event_action event_action;


    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || rule == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] rule[%p]\n", __func__, __LINE__, aibox_ip, rule);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    rule->aibox_ch = ch_conv_nvr_to_aibox(aibox_ip, rule->ch);
    printf("[%s:%d] aibox_ch[%d]\n", __func__, __LINE__, rule->aibox_ch);

    /********************
     * get event action
     ********************/
    snprintf(event_action_name, sizeof(event_action_name), "%s_%s", nf_api_get_nvr_owner(nvr_owner), _get_aibox_type_string(rule->type));
    rc = aibox_get_event_action(aibox_ip, event_action_name, &event_action);
    if(rc == 0){
        //get event action
        printf("[%s:%d] get_event_action [%s]\n", __func__, __LINE__, event_action.host);
    }else{
        //create event action
        switch(rule->type)
        {
            case RULE_TYPE_LPR:
                {
                    snprintf(event_action.name, sizeof(event_action.name), event_action_name);
                    snprintf(event_action.caption_msg, sizeof(event_action.caption_msg), "{{trigger_type}}");
                    snprintf(event_action.title_msg,   sizeof(event_action.title_msg),   "{{trigger_name}}, {{object_list}}{{object.lp_text}}{{object_list}}");
                    snprintf(event_action.desc_msg,    sizeof(event_action.desc_msg ),   "{{rule_name}}");
                }
                break;
            case RULE_TYPE_FR:
                {
                    snprintf(event_action.name, sizeof(event_action.name), event_action_name);
                    snprintf(event_action.caption_msg, sizeof(event_action.caption_msg), "{{trigger_type}}");
                    snprintf(event_action.title_msg,   sizeof(event_action.title_msg),   "{{TRIGGER NAME}}, {{LIST OBJECTS}}{{::OBJ[FACE_NAME]}}, {{::OBJ[FACE_GROUP_NAME]}}{{LIST OBJECTS}}");
                    snprintf(event_action.desc_msg,    sizeof(event_action.desc_msg ),   "{{rule_name}}");
                }
                break;
            default:
                {
                    snprintf(event_action.name, sizeof(event_action.name), event_action_name);
                    snprintf(event_action.caption_msg, sizeof(event_action.caption_msg), "{{trigger_type}}");
                    snprintf(event_action.title_msg,   sizeof(event_action.title_msg),   "{{TRIGGER NAME}}, {{LIST OBJECTS}}{{::OBJ[CLASS]}}{{LIST OBJECTS}}");
                    snprintf(event_action.desc_msg,    sizeof(event_action.desc_msg ),   "{{rule_name}}");
                }
        }
        rc = aibox_add_event_action(aibox_ip, &event_action);
        if(rc < 0){
            printf("[%s:%d] aibox_add_event_action[%d]\n", __func__, __LINE__, rc);
            ret = rc;
            goto endl;
        }
    }

    /********************
     * add trigger
     ********************/
    switch(rule->type)
    {
        case RULE_TYPE_LPR:
            {
                lpr_trigger_info trigger;
                lpr_rule *lpr = (lpr_rule *)rule;
                snprintf(trigger.name, sizeof(trigger.name), "%s", str_null_to_blank(lpr->name));
                snprintf(trigger.rmode, sizeof(trigger.rmode), lpr_trigger_mode[lpr->rmode]);
                snprintf(trigger.policy, sizeof(trigger.policy), lpr_trigger_policy[lpr->policy]);

                trigger.aibox_ch =   lpr->aibox_ch;
                trigger.zone_size =  lpr->zone_size;
                memcpy(trigger.zone, lpr->zone, sizeof(trigger.zone));
                trigger.group_size = lpr->group_size;
                for(i = 0; i < lpr->group_size; i++){
                    lpr_group_info group_info;
                    rc = get_lpr_group(aibox_ip, lpr->group_id_list[i], &group_info);

                    snprintf(trigger.group_list[i], sizeof(trigger.group_list[i]), "%s", str_null_to_blank( group_info.name ));
                }
                rc = aibox_add_lpr_trigger(aibox_ip, &trigger);
                rule->trigger_id = rc;
            }
            break;
        case RULE_TYPE_FR:
            {
                fr_trigger_info trigger;
                fr_rule *fr = (fr_rule *)rule;

                snprintf(trigger.name, sizeof(trigger.name), "%s", str_null_to_blank(fr->name));

                trigger.aibox_ch =   fr->aibox_ch;
                trigger.zone_size =  fr->zone_size;

                //zone
                memcpy(trigger.zone, fr->zone, sizeof(trigger.zone));

                snprintf(trigger.gtype, sizeof(trigger.gtype), fr_trigger_gtype[fr->gtype]);

                if(fr->gtype == FR_EVT_MODE_ATTR){
                    trigger.age_max = fr->age_max; 
                    trigger.age_min = fr->age_min;
                    snprintf(trigger.gender, sizeof(trigger.gender), aibox_gender[fr->gender]);
                }else{
                    trigger.liveness = fr->liveness; 
                    trigger.liveness_thr = fr->liveness_thr;

                    //group
                    trigger.group_size = fr->group_size;
                    for(i = 0; i < fr->group_size; i++){
                        fr_group_info group_info;
                        rc = get_fr_group(aibox_ip, fr->group_id_list[i], &group_info);
                        snprintf(trigger.group_list[i], sizeof(trigger.group_list[i]), "%s", str_null_to_blank( group_info.name ));
                    }
                }

                if(fr->gtype == FR_EVT_MODE_COMPARISON){
                    trigger.search_thr = fr->search_thr;
                }

                rc = aibox_add_fr_trigger(aibox_ip, &trigger);
                rule->trigger_id = rc;
            }
            break;
        default:
            rc = -1;
            break;
    }

    if(rc < 0){
        printf("[%s:%d] trigger add ret[%d]\n", __func__, __LINE__, rc);
        ret = rc;
        goto endl;
    }


    /********************
     * add action rule
     *******************/
    rc = aibox_add_action_rules(aibox_ip, rule->name, rule->name, rule->aibox_ch, &event_action);
    if(rc < 0){
        printf("[%s:%d] aibox_add_action_rules[%d]\n", __func__, __LINE__, rc);
        ret = rc;
        goto endl;
    }

    ret = rule->trigger_id;
endl:
    return ret;
}

int nf_api_modify_rule(unsigned int aibox_ip, aibox_rule *rule)
{
    int ret = -1;
    int rc;
    int i;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || rule == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] rule[%p]\n", __func__, __LINE__, aibox_ip, rule);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    switch(rule->type)
    {
        case RULE_TYPE_LPR:
            {
                lpr_trigger_info trigger;
                lpr_rule *lpr = (lpr_rule *)rule;

                rc = aibox_get_lpr_trigger(aibox_ip, lpr->trigger_id, lpr->name, &trigger);
                if(rc < 0){
                    printf("[%s:%d] aibox_get_lpr_trigger[%d] trigger_id[%s] lpr_name[%s]\n", __func__, __LINE__, rc, lpr->trigger_id, lpr->name);
                    ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
                    break;
                }

                snprintf(trigger.rmode, sizeof(trigger.rmode), lpr_trigger_mode[lpr->rmode]);
                snprintf(trigger.policy, sizeof(trigger.policy), lpr_trigger_policy[lpr->policy]);
                trigger.aibox_ch = lpr->aibox_ch;
                trigger.zone_size = lpr->zone_size;
                memcpy(trigger.zone, lpr->zone, sizeof(trigger.zone));
                //trigger.group_size = lpr->group_size;
                for(i = 0, trigger.group_size = 0; i < lpr->group_size; i++){
                    lpr_group_info group_info;
                    rc = get_lpr_group(aibox_ip, lpr->group_id_list[i], &group_info);
                    if(rc == DLVA_API_RET_OK){
                        snprintf(trigger.group_list[trigger.group_size], sizeof(trigger.group_list[trigger.group_size]), "%s", str_null_to_blank( group_info.name ));
                        trigger.group_size++;
                    }
                }

                ret = aibox_modify_lpr_trigger(aibox_ip, &trigger);
            }
            break;
        case RULE_TYPE_FR:
            {
                fr_trigger_info trigger;
                fr_rule *fr = (fr_rule *)rule;

                rc = aibox_get_fr_trigger(aibox_ip, fr->trigger_id, fr->name, &trigger);
                if(rc < 0){
                    printf("[%s:%d] aibox_get_fr_trigger[%d]\n", __func__, __LINE__, rc, fr->trigger_id, fr->name);
                    ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
                    break;
                }

                trigger.aibox_ch = fr->aibox_ch;
                trigger.zone_size = fr->zone_size;
                memcpy(trigger.zone, fr->zone, sizeof(trigger.zone));

                snprintf(trigger.gtype, sizeof(trigger.gtype), fr_trigger_gtype[fr->gtype]);

                if(fr->gtype == FR_EVT_MODE_ATTR){
                    trigger.age_max = fr->age_max; 
                    trigger.age_min = fr->age_min;
                    snprintf(trigger.gender, sizeof(trigger.gender), aibox_gender[fr->gender]);
                }else{
                    trigger.liveness = fr->liveness; 
                    trigger.liveness_thr = fr->liveness_thr;

                    //group
                    //trigger.group_size = fr->group_size;

                    for(i = 0, trigger.group_size = 0; i < fr->group_size; i++){
                        fr_group_info group_info;
                        rc = get_fr_group(aibox_ip, fr->group_id_list[i], &group_info);
                        if(rc == DLVA_API_RET_OK){
                            snprintf(trigger.group_list[trigger.group_size], sizeof(trigger.group_list[trigger.group_size]), "%s", str_null_to_blank( group_info.name ));
                            trigger.group_size++;
                        }
                    }
                }

                if(fr->gtype == FR_EVT_MODE_COMPARISON){
                    trigger.search_thr = fr->search_thr;
                }

                ret = aibox_modify_fr_trigger(aibox_ip, &trigger);
            }
            break;
        default:
            ret = -1;
            break;
    }


    return ret;
}

int nf_api_get_trigger_id(unsigned int aibox_ip, const char *trigger_name, AIBOX_RULE_TYPE_E type)
{
    int ret = -1;
    json_t *rules = NULL;
    json_t *lprs = NULL;
    json_t *trigger = NULL;
    
    const char *name;
    int i;
    int len;
    const char *type_str;

    if(trigger_name == NULL){
        printf("[%s:%d] trigger_name[%p]\n", __func__, __LINE__, trigger_name);
        return ret;
    }

    len = strlen(trigger_name);
    if(len <= 0){
        printf("[%s:%d] trigger_name[%s]\n", __func__, __LINE__, trigger_name);
        return ret;
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        goto endl;
    }

    ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;

    switch(type)
    {
        case RULE_TYPE_LPR:
            type_str = "LPR";
            break;
        case RULE_TYPE_FR:
            type_str = "FR";
            break;
        default:
            type_str = "";
    }

    json_array_foreach(json_object_get(json_object_get(rules, "triggers"), type_str), i, trigger)
    {
        //str = json_dumps(trigger, JSON_ENCODE_ANY);
        name = json_string_value(json_object_get(trigger, "name"));
        if(strncmp(trigger_name, name, len) == 0){
            ret = (int)json_integer_value(json_object_get(trigger, "id"));
            break;
        }
    }

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}


int nf_api_delete_rule(unsigned int aibox_ip, int trigger_id, AIBOX_RULE_TYPE_E type)
{
    int ret;
    int rc;
    int i;
    lpr_trigger_info trigger;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || trigger_id <= 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] trigger_id[%d]\n", __func__, __LINE__, aibox_ip, trigger_id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ret = aibox_delete_trigger(aibox_ip, trigger_id, type);
    return ret;
}

//ì­ì ìì 
int nf_api_add_lpr_rule(unsigned int aibox_ip, lpr_rule *rule)
{
    return nf_api_add_rule(aibox_ip, rule);
}
int nf_api_modify_lpr_rule(unsigned int aibox_ip, lpr_rule *rule)
{
    return nf_api_modify_rule(aibox_ip, rule);
}
int nf_api_delete_lpr_rule(unsigned int aibox_ip, int trigger_id, const char *trigger_name)
{
    if(trigger_id == 0){
        trigger_id = nf_api_get_trigger_id(aibox_ip, trigger_name, RULE_TYPE_LPR);
    }
    return nf_api_delete_rule(aibox_ip, trigger_id, RULE_TYPE_LPR);
}

/**************************************
 *
 * fr trigger api
 *
 **************************************/
static int set_json_to_fr_trigger(fr_trigger_info *trigger, json_t *json)
{
    json_t *zone = NULL;
    json_t *group = NULL;
    int group_id;
    int i;

    const char *gtype;
    const char *gender;

    if(trigger == NULL || json == NULL){
        return -2;
    }

    trigger->id = (int)json_integer_value(json_object_get(json, "id"));
    trigger->aibox_ch = (int)json_integer_value(json_object_get(json, "vsource"));
    snprintf(trigger->name, sizeof(trigger->name), "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
    snprintf(trigger->gtype, sizeof(trigger->gtype), "%s", str_null_to_blank(json_string_value(json_object_get(json, "gtype"))));
    snprintf(trigger->gender, sizeof(trigger->gender), "%s", str_null_to_blank(json_string_value(json_object_get(json, "gender"))));

    trigger->search_thr = (int)json_integer_value(json_object_get(json, "search_thr"));

    trigger->age_max = (int)json_integer_value(json_object_get(json, "age_max"));
    trigger->age_min = (int)json_integer_value(json_object_get(json, "age_min"));

    trigger->liveness = (int)json_boolean_value(json_object_get(json, "liveness"));
    trigger->liveness_thr = (int)json_integer_value(json_object_get(json, "liveness_thr"));

    /*********
     * group
     *********/
    trigger->group_size = json_array_size(json_object_get(json, "groups"));
    json_array_foreach(json_object_get(json, "groups"), i, group)
    {
        snprintf(trigger->group_list[i], sizeof(trigger->group_list[i]), "%s", str_null_to_blank(json_string_value(group)));
    }

    /*********
     * zone
     *********/
    trigger->zone_size = json_array_size(json_object_get(json, "zone"));
    json_array_foreach(json_object_get(json, "zone"), i, zone)
    {
        trigger->zone[i].x = json_real_value(json_object_get(zone, "x"));
        trigger->zone[i].y = json_real_value(json_object_get(zone, "y"));
    }


    return 0;
}

static int set_json_to_fr_rule(unsigned int aibox_ip, fr_rule *rule, json_t *json)
{
    json_t *zone = NULL;
    json_t *group = NULL;
    int group_id;
    int i;

    const char *gtype;
    const char *gender;

    if(rule == NULL || json == NULL){
        return -2;
    }

    rule->type = RULE_TYPE_FR;
    rule->trigger_id = (int)json_integer_value(json_object_get(json, "id"));
    rule->aibox_ch = (int)json_integer_value(json_object_get(json, "vsource"));
    snprintf(rule->name, sizeof(rule->name), "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));
    snprintf(rule->trigger_name, sizeof(rule->trigger_name), "%s", str_null_to_blank(json_string_value(json_object_get(json, "name"))));


    gtype = str_null_to_blank(json_string_value(json_object_get(json, "gtype")));
    for(i = 0; i < sizeof(fr_trigger_gtype)/sizeof(char *); i++){
        //printf("[%s:%d] %d : gtype - %s\n", __func__, __LINE__, i, fr_trigger_gtype[i]);
        if(strcmp(fr_trigger_gtype[i], gtype) == 0){
            rule->gtype = i;
            break;
        }
    }

    gender = str_null_to_blank(json_string_value(json_object_get(json, "gender")));
    for(i = 0; i < sizeof(aibox_gender)/sizeof(char *); i++){
        //printf("[%s:%d] %d : gender - %s\n", __func__, __LINE__, i, aibox_gender[i]);
        if(strcmp(aibox_gender[i], gender) == 0){
            rule->gender = i;
            break;
        }
    }

    rule->search_thr = (int)json_integer_value(json_object_get(json, "search_thr"));

    rule->age_max = (int)json_integer_value(json_object_get(json, "age_max"));
    rule->age_min = (int)json_integer_value(json_object_get(json, "age_min"));

    rule->liveness = (int)json_boolean_value(json_object_get(json, "liveness"));
    rule->liveness_thr = (int)json_integer_value(json_object_get(json, "liveness_thr"));

    /*********
     * group
     *********/
    rule->group_size = json_array_size(json_object_get(json, "groups"));
    json_array_foreach(json_object_get(json, "groups"), i, group)
    {
        group_id = get_group_id_aibox_fr(aibox_ip, str_null_to_blank(json_string_value(group)));
        if(group_id < 0) group_id = 0;
        rule->group_id_list[i] = group_id;
    }

    /*********
     * zone
     *********/
    rule->zone_size = json_array_size(json_object_get(json, "zone"));
    json_array_foreach(json_object_get(json, "zone"), i, zone)
    {
        rule->zone[i].x = json_real_value(json_object_get(zone, "x"));
        rule->zone[i].y = json_real_value(json_object_get(zone, "y"));
    }


    return 0;
}

static int get_fr_group(unsigned int aibox_ip, int group_id, fr_group_info *info)
{
    int rc;
    HTTP_CTX ctx;
	int ret = DLVA_API_RET_FAILED_INIT;

    int ch;
    char id[129];
    char pw[129];

    json_t *json = NULL;
    json_t *group_info = NULL;
    json_error_t error;

    int len = 0;
    int i;

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL || group_id < 0)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] group_id[%d] info[%p]\n", __func__, __LINE__, aibox_ip, group_id, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ch = get_aibox_ch(aibox_ip);
    if(ch < 0 || ch >= AVAILABLE_MAX_CH)
    {
        printf("[%s:%d] get channel failed aibox_ip[%s] ch[%d]\n", __func__, __LINE__, _ip_to_str(aibox_ip, NULL), ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    http_init(&ctx);
    aibox_http_default_setting(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, "/api/rule/fr/v2/groups/");
    http_data_set(&ctx, HTTP_SET_METHOD, "GET");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(ch, id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(ch, pw));

    rc = http_request(&ctx);
    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }

    json = json_loads(http_get_res(&ctx), 0, &error);
    if(json == NULL)
    {
        fprintf(stderr, "[%s:%d] json error on line %d: %s\n", __FUNCTION__, __LINE__, error.line, error.text);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    ret = DLVA_API_RET_AIBOX_DATA_NOT_FOUND;
    json_array_foreach(json, i, group_info){
        if(json_integer_value(json_object_get(group_info, "id")) == group_id){
            info->id = group_id;
            snprintf(info->name, sizeof(info->name),   "%s", str_null_to_blank(json_string_value(json_object_get(group_info, "name"))));
            ret = DLVA_API_RET_OK;
            goto endl;
        }
    }
endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

static int aibox_add_fr_trigger(unsigned int aibox_ip, fr_trigger_info *info)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    json_t *json = NULL;
    json_error_t error;
    char *str_json = NULL;

    int i;
    char buff[200];
    char path[200];

    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }
	
    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    snprintf(path, sizeof(path), "/api/rule/es/triggers/fr/");
    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "POST");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    //name
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "name", info->name);

    //vsource
    snprintf(buff, sizeof(buff), "%d", info->aibox_ch);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "vsource", buff);

    //zone
    json = json_array();
    for(i = 0; i < info->zone_size; i++){
        json_t *json_point = json_object();
        json_object_set_new(json_point, "x", json_real(info->zone[i].x));
        json_object_set_new(json_point, "y", json_real(info->zone[i].y));
        json_array_append_new(json, json_point);
    }
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[zone] %s\n", str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", str_json);

    free(str_json);
    json_decref(json);
    json = NULL;

    //groups
    json = json_array();
    for(i = 0; i < 20 && i < info->group_size; i++){
    if(strlen(info->group_list[i]) <= 0) break;
        json_array_append_new(json, json_string(info->group_list[i]));
    }
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[groups] %s\n", str_json);
    //http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", "[\"a\"]");//str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", str_json);

    free(str_json);
    json_decref(json);
    json = NULL;

    http_data_set(&ctx, HTTP_ADD_MULTIPART, "gtype",        info->gtype);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "liveness",     info->liveness ? "true" : "false");
    snprintf(buff, sizeof(buff), "%d", info->liveness_thr);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "liveness_thr", buff);
    snprintf(buff, sizeof(buff), "%d", info->search_thr);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "search_thr",   buff);
    snprintf(buff, sizeof(buff), "%d", info->age_max);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "age_max",      buff);
    snprintf(buff, sizeof(buff), "%d", info->age_min);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "age_min",      buff);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "gender",       info->gender);

    /*
     * fixed value
     */
    //label_conf
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "label_conf", "{\"x\":0.8,\"y\":0.05,\"w\":0.15,\"h\":0.1}");

    //reset
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "reset", "-1");


    /*
     * http request
     */
    rc = http_request(&ctx);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }


    if(ctx.status != 201){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    //printf("[%s:%d] memory[%s]\n", __func__, __LINE__, ctx.res_data.memory);
    json = json_loads(http_get_res(&ctx), 0, &error);
    ret = (int)json_integer_value(json_object_get(json, "id"));

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int aibox_get_fr_trigger(unsigned int aibox_ip, unsigned int trigger_id, const char *trigger_name, fr_trigger_info *info)
{
    int ret = -1;
    json_t *rules = NULL;
    json_t *frs = NULL;
    json_t *trigger = NULL;
    
    const char *name;
    int id;
    int i;
    int len;

    if(info == NULL){
        printf("[%s:%d] trigger_name[%p] info[%p]\n", __func__, __LINE__, trigger_name, info);
        return ret;
    }

    if(trigger_id == 0){
        if(trigger_name == NULL){
            printf("[%s:%d] trigger_name[%p] trigger_id[%d]\n", __func__, __LINE__, trigger_name, trigger_id);
            return ret;
        }

        len = strlen(trigger_name);
        if(len <= 0){
            printf("[%s:%d] trigger_name[%s]\n", __func__, __LINE__, trigger_name);
            return ret;
        }
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        goto endl;
    }

    ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
    memset(info, 0, sizeof(fr_trigger_info));
    json_array_foreach(json_object_get(json_object_get(rules, "triggers"), "FR"), i, trigger)
    {
        set_json_to_fr_trigger(info, trigger);
        if(trigger_id == info->id){
            ret = DLVA_API_RET_OK;
            break;
        }

        if(trigger_name){
            if(strcmp(info->name, trigger_name) == 0){
                ret = DLVA_API_RET_OK;
                break;
            }
        }

        memset(info, 0, sizeof(fr_trigger_info));
    }

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}

static int aibox_modify_fr_trigger(unsigned int aibox_ip, fr_trigger_info *info)
{
    int rc;
    HTTP_CTX ctx;
    int ret = DLVA_API_RET_FAILED_INIT;

    json_t *json = NULL;
    json_error_t error;
    char *str_json = NULL;

    int i;
    char buff[200];
    char path[200];

    int nvr_ch;
    char nvr_id[129];
    char nvr_pw[129];

    //argument check
    if(aibox_ip == 0 || aibox_ip == 0xffffffff || info == NULL)
    {
        printf("[%s:%d] argument check error aibox_ip[%08x] info[%p]\n", __func__, __LINE__, aibox_ip, info);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    nvr_ch = get_aibox_ch(aibox_ip);
    if(nvr_ch < 0)
    {
        printf("[%s:%d] argument check error nvr_ch[%d] \n", __func__, __LINE__, nvr_ch);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    snprintf(path, sizeof(path), "/api/rule/es/triggers/frs/%d/", info->id);
    http_init(&ctx);
    aibox_http_default_setting_multipart(&ctx, aibox_ip);
    http_data_set(&ctx, HTTP_SET_PATH, path);
    http_data_set(&ctx, HTTP_SET_METHOD, "PUT");
    http_data_set(&ctx, HTTP_SET_USER, _api_get_aibox_id(nvr_ch, nvr_id));
    http_data_set(&ctx, HTTP_SET_PASSWD, _api_get_aibox_pw(nvr_ch, nvr_pw));

    //name
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "name", info->name);

    //vsource
    snprintf(buff, sizeof(buff), "%d", info->aibox_ch);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "vsource", buff);

    // ìì
    
    //zone
    //snprintf(buff, sizeof(buff), "[{\"x\":0.01,\"y\":0.2},{\"x\":0.75,\"y\":0.2},{\"x\":0.75,\"y\":0.8},{\"x\":0.25,\"y\":0.8}]");
    json = json_array();
    for(i = 0; i < info->zone_size; i++){
        json_t *json_point = json_object();
        json_object_set_new(json_point, "x", json_real(info->zone[i].x));
        json_object_set_new(json_point, "y", json_real(info->zone[i].y));
        json_array_append_new(json, json_point);
    }
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[zone] %s\n", str_json);
    //http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", "[{\"x\":0.06375,\"y\":0.073333},{\"x\":0.9475,\"y\":0.1},{\"x\":0.9475,\"y\":0.7},{\"x\":0.4475,\"y\":0.7}]");//str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "zone", str_json);

    free(str_json);
    json_decref(json);
    json = NULL;

    //groups
    json = json_array();
    for(i = 0; i < 20 && i < info->group_size; i++){
    if(strlen(info->group_list[i]) <= 0) break;
        json_array_append_new(json, json_string(info->group_list[i]));
    }
    str_json = json_dumps(json, JSON_ENCODE_ANY);
    printf("[groups] %s\n", str_json);
    //http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", "[\"a\"]");//str_json);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "groups", str_json);

    free(str_json);
    json_decref(json);
    json = NULL;

    /*
     * ?? value
     */
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "gtype",        info->gtype);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "liveness",     info->liveness ? "true" : "false");
    snprintf(buff, sizeof(buff), "%d", info->liveness_thr);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "liveness_thr", buff);
    snprintf(buff, sizeof(buff), "%d", info->search_thr);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "search_thr",   buff);
    snprintf(buff, sizeof(buff), "%d", info->age_max);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "age_max",      buff);
    snprintf(buff, sizeof(buff), "%d", info->age_min);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "age_min",      buff);
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "gender",       info->gender);

    /*
     * fixed value
     */
    //reset
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "n", "1");

    //label_conf
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "label_conf", "{\"x\":0.8,\"y\":0.05,\"w\":0.15,\"h\":0.1}");

    //reset
    http_data_set(&ctx, HTTP_ADD_MULTIPART, "reset", "-1");


    /*
     * http request
     */
    rc = http_request(&ctx);

    if(rc == CURLE_OPERATION_TIMEDOUT){/* CURLE_OPERATION_TIMEDOUT : 28 */
        printf("[%s:%d] DLVA_API_RET_AIBOX_CONNECTION_FAILED rc[%d]\n", __func__, __LINE__, rc);
        ret = DLVA_API_RET_AIBOX_CONNECTION_FAILED;
        goto endl;
    }


    if(ctx.status != 200){
        printf("[%s:%d] DLVA_API_RET_AIBOX_RETCODE_ERROR status[%d] body[%s]\n", __func__, __LINE__, ctx.status, ctx.res_data.memory);
        ret = DLVA_API_RET_AIBOX_RETCODE_ERROR;
        goto endl;
    }

    if(ctx.res_data.memory == NULL || ctx.res_data.size == 0){
        printf("[%s:%d] receive data error memory[%p] size[%ld]\n", __func__, __LINE__, ctx.res_data.memory, ctx.res_data.size);
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }

    //printf("[%s:%d] memory[%s]\n", __func__, __LINE__, ctx.res_data.memory);
    json = json_loads(http_get_res(&ctx), 0, &error);
    ret = (int)json_integer_value(json_object_get(json, "id"));

endl:
    if(json) json_decref(json);
    http_release(&ctx);
    return ret;
}

int nf_api_get_fr_rule_by_id(unsigned int aibox_ip, unsigned int trigger_id, fr_rule *rule)
{
    int ret = DLVA_API_RET_FAILED_INIT;
    json_t *rules = NULL;
    json_t *frs = NULL;
    json_t *trigger = NULL;
    json_t *fr_list = NULL;
    
    const char *name;
    int i;

    if(rule == NULL || trigger_id == 0){
        printf("[%s:%d] rule[%p] trigger_id[%d]\n", __func__, __LINE__, rule, trigger_id);
        return DLVA_API_RET_ARGUMENT_ERROR;
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        goto endl;
    }
    fr_list = json_object_get(json_object_get(rules, "triggers"), "FR");
    
    ret = DLVA_API_RET_AIBOX_DATA_NOT_FOUND;
    memset(rule, 0, sizeof(fr_rule));
    json_array_foreach(fr_list, i, trigger)
    {
        set_json_to_fr_rule(aibox_ip, rule, trigger);
        if(rule->trigger_id == trigger_id){
            ret = DLVA_API_RET_OK;
            break;
        }

        memset(rule, 0, sizeof(fr_rule));
    }

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}

int nf_api_get_fr_rules(unsigned int aibox_ip, fr_rule **rule, int *rule_size)
{
    int ret = DLVA_API_RET_FAILED_INIT;
    json_t *rules = NULL;
    json_t *trigger = NULL;
    json_t *fr_list = NULL;
    
    const char *name;
    int i;
    int len;

    if(rule == NULL || rule_size == NULL){
        printf("[%s:%d] rule[%p] rule_size[%p]\n", __func__, __LINE__, rule, rule_size);
        return ret;
    }

    ret = _get_aibox_rule_data(aibox_ip, &rules);
    if(ret != DLVA_API_RET_OK || rules == NULL){
        printf("[%s:%d] error _get_aibox_rule_data[%d]\n", __func__, __LINE__, ret);
        goto endl;
    }
    fr_list = json_object_get(json_object_get(rules, "triggers"), "FR");
    

    //memory alloc
    len = json_array_size(fr_list);
    if(len <= 0){
        ret = DLVA_API_RET_AIBOX_DATA_PARSE_ERROR;
        goto endl;
    }
    *rule = malloc(len * sizeof(fr_rule));
    if(*rule == NULL){
        ret = DLVA_API_RET_FAILED_INIT;
        goto endl;
    }
    
    json_array_foreach(fr_list, i, trigger)
    {
        set_json_to_fr_rule(aibox_ip, *rule + i, trigger);
    }

    *rule_size = i;
    ret = DLVA_API_RET_OK;

endl:
    if(rules){
        json_decref(rules);
    }
    return ret;
}

unsigned int nf_api_get_aibox_ip(int ch)
{
	GValue ret_value = {0,};
    char key_value[64];
	mtable *runtime = get_runtime();
    unsigned int aibox_ip = 0;
    int devcam, devbox;

    snprintf(key_value, 64, "cam.dvabx.cfg.R%d.devcam", ch);
    devcam = nf_sysdb_get_bool(key_value);

    snprintf(key_value, 64, "cam.dvabx.cfg.R%d.devbox", ch);
    devbox = nf_sysdb_get_bool(key_value);

    if (nf_sysdb_get_key1("cam.ai_box.A%d.addr", ch, &ret_value, NULL))
    {
        aibox_ip = g_value_get_uint(&ret_value);
        g_value_unset(&ret_value);
    }

    if(devbox && aibox_ip){
        return aibox_ip;
    }

    if(devcam){
        return ntohl(runtime[ch].sys.ipaddr);
    }

    return 0;
}
