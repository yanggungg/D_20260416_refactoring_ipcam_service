#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <glib.h>
#include <math.h>
#include "nf_api_live.h"
#include "nf_va_object_detector.h"
#include "nf_api_eventlog.h"
#include "nf_dva_event.h"
#include "nf_action.h"

static GSList *alg_ipz_list[16] = {0};

static int _is_dva_active_on(int ch, char *alg_type)
{
	gchar sysdb_key[32] = {0};
	
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.%s.act", ch, alg_type);
	
	return nf_sysdb_get_bool(sysdb_key);
}

static int _is_dva_schedule_on(gint ch)
{
	gchar sysdb_key[32] = {0};
	gchar *sched;
	GDate *date;
	GDateWeekday weekday;
	struct tm *t;
	time_t cur_ts;
	guint idx;

	cur_ts = time(NULL);
	date = g_date_new();
	g_date_set_time_t(date, cur_ts);
	
	weekday = g_date_get_weekday(date) % 7; //0:sunday
	g_date_free(date);

	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.sched", ch);
	sched = nf_sysdb_get_str_nocopy(sysdb_key);
	
	t = localtime(&cur_ts);
	idx = (24 * weekday) + (guint)t->tm_hour;
	
	if(sched[idx] == '0')
		return 0;	
	return 1;
}

static char *_get_db_map_obj_name(char *obj_name)
{
	//if (!strcmp(obj_name, "person"))
	//	return "human";
	return obj_name;
}

gboolean nf_dva_eventlog_put(DVA_MSG *msg)
{
	NF_DVA_LOG_DATA dva_log;
	NF_LOG_DATA log_data;
	
	GTimeVal	tval;
	tval.tv_sec = (glong)msg->timestamp;
	tval.tv_usec = (glong)msg->timestampl * 5 * 1000;
	
	memset(&dva_log, 0x00, sizeof(NF_DVA_LOG_DATA));
	memset(&log_data, 0x00, sizeof(NF_LOG_DATA));

	log_data.type = LT_DVA;
	log_data.timestamp = GTIMEVAL_TO_GUINT64(tval);
	log_data.param1 = msg->ch;
	log_data.param2 = msg->type;
	
	if(msg->type == DVA_INTRUSION_DETECTION)
	{
		memcpy(&dva_log.intrusion_detection, &msg->intrusion_detection, sizeof(DVA_MSG_IDZ));
	}
	else if (msg->type == DVA_ILLEGAL_PARKING)
	{
		memcpy(&dva_log.illegal_parking, &msg->illegal_parking, sizeof(DVA_MSG_IPZ));
	}
	else if (msg->type == DVA_COUNTER)
	{
		memcpy(&dva_log.counter, &msg->counter, sizeof(DVA_MSG_COUNTER));
	}
	memcpy(log_data.text, &dva_log, sizeof(dva_log));
	
	if (nf_eventlog_put(&log_data, NULL))
	{
		log_data.param2 += DVA_GRP_INTRUSION_DETECTION;
		return nf_eventlog_put(&log_data, NULL);
	}
	return 0;
}

typedef struct {
	gint cls_id;
	guint64 timestamp;
} DVA_NOTI_TS;

static gint _is_ignr_itvl_over(gint alg_type, gint ch, guint64 timestamp, struct object *obj)
{
	static GSList *noti_ts_list[NUM_ACTIVE_CH] = {0};
	GSList *list = noti_ts_list[ch];
	DVA_NOTI_TS *noti_ts;
	
	guint len = g_slist_length(list);
	gchar dbkey[64] = {0};
	guint ignr_itvl = 0;
	guint i = 0;

	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ignore_interval", ch);
	ignr_itvl = nf_sysdb_get_uint(dbkey);

	for (i = 0; i < len; i++)
	{
		noti_ts = (DVA_NOTI_TS *)g_slist_nth_data(list, i);
		if (!noti_ts)
			return 0;
		
		if(obj->id == noti_ts->cls_id)
		{
			g_print("[%s][%d][%d] cls_id[%d] noti_ts[%lld] ignr_itvl[%d] cur_ts[%lld]\n", __FUNCTION__, __LINE__, alg_type, noti_ts->cls_id, noti_ts->timestamp, ignr_itvl, timestamp);
			if(timestamp < noti_ts->timestamp) //datetime changed to the past
				noti_ts->timestamp = 0;
			
			if(noti_ts->timestamp + ignr_itvl <= timestamp)
			{	
				noti_ts->timestamp = timestamp;
				g_print("[%s][%d][%d] noti_ts->timestamp update to [%lld]\n", __FUNCTION__, __LINE__, alg_type, timestamp);
				return 1;
			}
			return 0;
		}
	}

	if (len < OBJ_CLS_MAX_CNT)
	{
		noti_ts = (DVA_NOTI_TS *)malloc(sizeof(DVA_NOTI_TS));
		noti_ts->cls_id = obj->id;
		noti_ts->timestamp = timestamp;
		noti_ts_list[ch] = g_slist_append(list, noti_ts);
	}
	return 1;
}

static DVA_OBJ_COUNTER _obj_counts[NUM_ACTIVE_CH];
DVA_OBJ_COUNTER *nf_dva_get_obj_count(gint ch)
{
	return &_obj_counts[ch];
}

static gboolean nf_dva_alg_object_count(struct objects *objs, struct object *obj)
{
	#define DB_OBJ_CNT_LIMIT 5
	#define DB_OBJ_CNT_RESET 1
	
	guint *count = NULL;
	gint ch = objs->ch;
	size_t slen = strlen(obj->name);

	gboolean should_noti = 0;
	gboolean should_reset = 0;
	guint threshold = 0;

	{
		gchar sysdb_key[64] = {0};
		
		snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.counter.active", ch);
		if (!nf_sysdb_get_bool(sysdb_key))
			return FALSE;

		snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.counter.noti", ch);
		should_noti = nf_sysdb_get_bool(sysdb_key);

		snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.counter.e_value", ch);
		threshold = nf_sysdb_get_uint(sysdb_key);

		snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.counter.reset", ch);
		should_reset = nf_sysdb_get_bool(sysdb_key);
	}

	_obj_counts[ch].ch = ch;
	if (!strncmp(obj->name, "person", slen))
	{
		count = &_obj_counts[ch].person;
	}
	else if(!strncmp(obj->name, "bicycle", slen) || !strncmp(obj->name, "motorbike", slen) ||
			!strncmp(obj->name, "bike", slen) || !strncmp(obj->name, "bus", slen) ||
			!strncmp(obj->name, "car", slen))
	{
		count = &_obj_counts[ch].vehicle;
	}
	else if(!strncmp(obj->name, "bird", slen) || !strncmp(obj->name, "cat", slen) ||
			!strncmp(obj->name, "dog", slen) || !strncmp(obj->name, "cow", slen)||
			!strncmp(obj->name, "horse", slen))
	{
		count = &_obj_counts[ch].animal;
	}
	else
	{
		return FALSE;
	}

	*count += 1;
	
	if (*count == threshold && should_noti)
	{
		DVA_MSG *msg = malloc(sizeof(DVA_MSG));
		if (msg == NULL) {
			printf("[%s][%d] malloc failed.\n", __FUNCTION__, __LINE__);
			return;
		}
		memset(msg, 0x00, sizeof(DVA_MSG));
		
		msg->type = DVA_COUNTER;
		msg->ch = (guchar)objs->ch;
		msg->timestamp = objs->time;
		msg->timestampl = objs->timel;

		strncpy(msg->counter.name, obj->name, strlen(obj->name));
		msg->counter.confidence = obj->confidence;
		msg->counter.bbox.width = 300;
		msg->counter.bbox.height = 300;
		msg->counter.bbox.coords[0] = obj->bbx.top.x;
		msg->counter.bbox.coords[1] = obj->bbx.top.y;
		msg->counter.bbox.coords[2] = obj->bbx.bottom.x;
		msg->counter.bbox.coords[3] = obj->bbx.bottom.y;
		msg->counter.count = *count;

		msg->snapshot.format = DVA_IMG_JPG;
		msg->snapshot.width = objs->img.width;
		msg->snapshot.height = objs->img.height;
		msg->snapshot.size = objs->img.size;
		memcpy(msg->snapshot.data, objs->img.data, sizeof(unsigned char) * objs->img.size);

		printf("\e[31m[%s][%d]\e[0m dva event fire [ch%d:%s]\n", __FUNCTION__, __LINE__, ch, obj->name);
		nf_notify_fire_pointer("dva_event", msg, sizeof(DVA_MSG));
		nf_dva_eventlog_put(msg);

		if (msg)
			free(msg);
			
		if (should_reset)
			*count = 0;
	}
	
	return TRUE;
}

static void nf_dva_event_object_count_noti(gint ch)
{
	printf("\e[31m[%s][%d]\e[0m dva_counter ch[%d] p[%d] v[%d] a[%d] fire\n", __FUNCTION__, __LINE__,  
		_obj_counts[ch].ch, _obj_counts[ch].person, _obj_counts[ch].vehicle, _obj_counts[ch].animal);

	nf_notify_fire_pointer("dva_counter", &_obj_counts[ch], sizeof(DVA_OBJ_COUNTER));
}

#define MAX_STATIC_REGION 50
typedef struct {
	unsigned int timestamp;
	float x1;
	float y1;
	float x2;
	float y2;
} DVA_STATIC_REGION;

static int nf_dva_alg_intrusion_detection(struct objects *objs)
{
	gchar sysdb_key[64] = {0};
	gchar obj_name[32] = {0};
	gchar *obj_list[3];
	gboolean is_counted = 0;
	guint c_threshold = 0;
	
	gboolean static_filter = 0;
	guint static_filter_sense;
	
	
	struct object *obj;
	int i = 0, k = 0;
	
	if(!_is_dva_active_on(objs->ch, "idz"))
		return 0;
		
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.en_static_filter", objs->ch);
	static_filter = nf_sysdb_get_bool(sysdb_key);
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.static_filter_sense", objs->ch);
	static_filter_sense = nf_sysdb_get_uint (sysdb_key);
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.human.item_list", objs->ch);
	obj_list[0] = nf_sysdb_get_str_nocopy(sysdb_key);
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.vehicle.item_list", objs->ch);
	obj_list[1] = nf_sysdb_get_str_nocopy(sysdb_key);
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.animal.item_list", objs->ch);
	obj_list[2] = nf_sysdb_get_str_nocopy(sysdb_key);
	
	snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.idz.c_threshold", objs->ch);
	c_threshold = nf_sysdb_get_uint(sysdb_key);
	
	for(i = 0; i < objs->obj_num; i++)
	{
		obj = &objs->obj[i];
		
		if (obj->confidence * 100 < c_threshold * 1.0) {
			continue;
		}
		
		for(k = 0; k < 3; k++)
		{
			snprintf(obj_name, sizeof(obj_name), "[%s:1]", _get_db_map_obj_name(obj->name));
			//printf("%s:%s\n", obj_name, obj_list[k]);
			if(strstr(obj_list[k], obj_name))
			{
			
				static GSList *static_region[NUM_ACTIVE_CH] = {0};
				GSList *list = static_region[objs->ch];
				DVA_STATIC_REGION *unit;
				int j,cnt,call_cnt,flag=0;
				
				if(!_is_ignr_itvl_over(DVA_INTRUSION_DETECTION, objs->ch, objs->time, obj))
					break;
				 if(!nf_aciton_get_double_knock_ch(objs->ch))
				 	break;
				 
				if(static_filter){
					guint sense;

					switch(static_filter_sense){
						case 0:
							sense = 5;
							break;
						case 1:
							sense = 10;
							break;
						case 2:
							sense = 20;
							break;
						default:
							sense = 10;
							break;
					}
			
					// 70 sec del
					cnt = g_slist_length(list);
					call_cnt = 0;
					
					for(j=0; j < cnt; j++){
						GSList *llist = g_slist_nth(list, call_cnt);
						if (llist) {
							DVA_STATIC_REGION *tmp = (struct DVA_STATIC_REGION *)llist->data;

							if(objs->time > tmp->timestamp + 65){
								//printf("[%s][%d] free(llist->data) time %d %d \n", __FUNCTION__, __LINE__,objs->time, tmp->timestamp);
								list = g_slist_remove_link(list, llist);
								
								free(llist->data);
								
								g_slist_free(llist);
								cnt--;
							}
							else{
								//g_slist_free(llist);
								call_cnt++;
								break;
							}
						}
						else{
							break;
						}
					}

					if(cnt >= MAX_STATIC_REGION){
						GSList *llist = g_slist_nth(list, 0);
						if (llist) {
							list = g_slist_remove_link(list, llist);
							//printf("[%s][%d] free(llist->data)\n", __FUNCTION__, __LINE__);
							free(llist->data);
							
							g_slist_free(llist);
							cnt--;
						}
					}
					
					

					// check
					for(j=0; j < cnt; j++){
					DVA_STATIC_REGION *tmp;
						GSList *llist = g_slist_nth(list, j);
						tmp = (struct DVA_STATIC_REGION *)(llist->data);
					//printf("[%s][%d] 4444444444 %f %f %f %f", __FUNCTION__, __LINE__,tmp->x1,obj->bbx.top.x,tmp->y1,obj->bbx.top.y);
						if(abs(tmp->x1*1000 - obj->bbx.top.x*1000) < (tmp->x2 - tmp->x1)*1000/sense
						   && abs(tmp->y1*1000 - obj->bbx.top.y*1000) < (tmp->y2 - tmp->y1)*1000/sense
						   && abs(tmp->x2*1000 - obj->bbx.bottom.x*1000) < (tmp->x2 - tmp->x1)*1000/sense
						   && abs(tmp->y2*1000 - obj->bbx.bottom.y*1000) < (tmp->y2 - tmp->y1)*1000/sense){
							flag = 1;
							//printf("[%s][%d] SKIP Static region \n", __FUNCTION__, __LINE__);
							break;
						}
					}

					// update
					unit = (DVA_STATIC_REGION *)malloc(sizeof(DVA_STATIC_REGION));
					unit->timestamp = objs->time;
					unit->x1 = obj->bbx.top.x;
					unit->y1= obj->bbx.top.y;
					unit->x2 = obj->bbx.bottom.x;
					unit->y2 = obj->bbx.bottom.y;
					static_region[objs->ch] = g_slist_append(list, unit);		

					printf("[%s][%d] DLVA Event Update flag %d Time %d list cnt %d %f %f %f %f", __FUNCTION__, __LINE__,flag, objs->time,cnt,obj->bbx.top.x,obj->bbx.top.y,obj->bbx.bottom.x,obj->bbx.bottom.y);

					if(!flag){
						DVA_MSG *msg = malloc(sizeof(DVA_MSG));
						if (msg == NULL) {
							printf("[%s][%d] malloc failed.\n", __FUNCTION__, __LINE__);
							return;
						}
						memset(msg, 0x00, sizeof(DVA_MSG));
						
						msg->type = DVA_INTRUSION_DETECTION;
						msg->ch = (guchar)objs->ch;
						msg->timestamp = objs->time;
						msg->timestampl = objs->timel;

						strncpy(msg->intrusion_detection.name, obj->name, sizeof(obj->name));
						msg->intrusion_detection.confidence = obj->confidence;
						msg->intrusion_detection.bbox.width = 300;
						msg->intrusion_detection.bbox.height = 300;
						msg->intrusion_detection.bbox.coords[0] = obj->bbx.top.x;
						msg->intrusion_detection.bbox.coords[1] = obj->bbx.top.y;
						msg->intrusion_detection.bbox.coords[2] = obj->bbx.bottom.x;
						msg->intrusion_detection.bbox.coords[3] = obj->bbx.bottom.y;

						msg->snapshot.format = DVA_IMG_JPG;
						msg->snapshot.width = objs->img.width;
						msg->snapshot.height = objs->img.height;
						msg->snapshot.size = objs->img.size;
						memcpy(msg->snapshot.data, objs->img.data, sizeof(unsigned char) * objs->img.size);

						printf("\e[31m[%s][%d]\e[0m dva event fire [ch%d:%s]\n", __FUNCTION__, __LINE__, objs->ch, obj->name);
						nf_notify_fire_pointer("dva_event", msg, sizeof(DVA_MSG));
						nf_dva_eventlog_put(msg);
						
						is_counted = nf_dva_alg_object_count(objs, obj);
						//nf_cloud_event_put_object_detected(msg);
						//break;

						if (msg)
							free(msg);
					}
				}
				else{
					DVA_MSG *msg = malloc(sizeof(DVA_MSG));
					if (msg == NULL) {
						printf("[%s][%d] malloc failed.\n", __FUNCTION__, __LINE__);
						return;
					}
					memset(msg, 0x00, sizeof(DVA_MSG));
					
					msg->type = DVA_INTRUSION_DETECTION;
					msg->ch = (guchar)objs->ch;
					msg->timestamp = objs->time;
					msg->timestampl = objs->timel;

					strncpy(msg->intrusion_detection.name, obj->name, sizeof(obj->name));
					msg->intrusion_detection.confidence = obj->confidence;
					msg->intrusion_detection.bbox.width = 300;
					msg->intrusion_detection.bbox.height = 300;
					msg->intrusion_detection.bbox.coords[0] = obj->bbx.top.x;
					msg->intrusion_detection.bbox.coords[1] = obj->bbx.top.y;
					msg->intrusion_detection.bbox.coords[2] = obj->bbx.bottom.x;
					msg->intrusion_detection.bbox.coords[3] = obj->bbx.bottom.y;

					msg->snapshot.format = DVA_IMG_JPG;
					msg->snapshot.width = objs->img.width;
					msg->snapshot.height = objs->img.height;
					msg->snapshot.size = objs->img.size;
					memcpy(msg->snapshot.data, objs->img.data, sizeof(unsigned char) * objs->img.size);

					printf("\e[31m[%s][%d]\e[0m dva event fire [ch%d:%s]\n", __FUNCTION__, __LINE__, objs->ch, obj->name);
					nf_notify_fire_pointer("dva_event", msg, sizeof(DVA_MSG));
					nf_dva_eventlog_put(msg);
					
					is_counted = nf_dva_alg_object_count(objs, obj);
					//nf_cloud_event_put_object_detected(msg);
					//break;

					if (msg)
						free(msg);
				}
			}
		}
	}

	if (is_counted)
		nf_dva_event_object_count_noti(objs->ch);
	
	return 0;
}

static gfloat _get_distance(float x1, float y1, float x2, float y2)
{
    return (gfloat)sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}

static gboolean _is_same_obj(struct object *obja, struct object *objb, gfloat *dist)
{
	gfloat dist_a;
	gfloat dist_b;

	if (obja->id != objb->id || !dist)
		return 0;
	
	dist_a = _get_distance(obja->bbx.top.x, obja->bbx.top.y, objb->bbx.top.x, objb->bbx.top.y);
	dist_b = _get_distance(obja->bbx.bottom.x, obja->bbx.bottom.y, objb->bbx.bottom.x, objb->bbx.bottom.y);

	//printf("\e[31m[%s][%d]\e[0m dist [%.2lf,%.2lf]\n", __FUNCTION__, __LINE__, dist_a, dist_b);
	if (dist_a >= 0.15 || dist_b >= 0.15)
	{
		*dist = 0.0;
		return 0;
	}
		
	*dist = (dist_a / dist_b) / 2.0;
	return 1;
}

static gfloat _search_same_object(gint ch, guint time, struct object *obj)
{
	GSList *list = alg_ipz_list[ch];
	guint len = g_slist_length(list);
	gint i = 0;
	gint j = 0;
	guint dwell_time = 0;
	guint ignr_itvl = 0;
	guint detected_itvl = 0;
	gchar dbkey[64] = {0};
	struct objs_coords *objs;
	struct objs_coords *objs_1st = NULL;

	gfloat compared_objs = 0;
	gfloat matched_obj = 0;
	gfloat dist = 0;	
	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ipz.dwell", ch);
#ifdef DLVA_FOR_QBRIDGE
	dwell_time = nf_sysdb_get_uint(dbkey) * 3; //1min->3sec, 2min->6sec
#else
	dwell_time = nf_sysdb_get_uint(dbkey) * 60;
#endif

	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ignore_interval", ch);
	ignr_itvl = nf_sysdb_get_uint(dbkey);

	if (len == 0)
	{
		g_print("[%s][%d] len == 0\n", __FUNCTION__, __LINE__);
		return 0.0;
	}

	for(i = len-2; i >= 0; i--)
	{
		objs = (struct objs_coords *)g_slist_nth_data(list, i);
		if (!objs)
		{
			g_print("[%s][%d] !objs\n", __FUNCTION__, __LINE__);
			return 0.0;
		}

		//g_print("[%s][%d] objs->time[%d] time[%d]\n", __FUNCTION__, __LINE__, objs->time, time);
		if (objs->time < time - dwell_time - 5)
			break;

		compared_objs++;
		
		for (j = 0; j < objs->obj_num; j++)
		{
			if (_is_same_obj(&objs->obj[j], obj, &dist))
			{
				if (objs_1st)
				{
					detected_itvl = objs_1st->time - objs->time;
					
					if (detected_itvl > 8 || (detected_itvl > 4 && dist > 0.03))
					{
						g_print("[%s][%d] similar pos, but detected interval is too long. obj->time[%d].\n", __FUNCTION__, __LINE__, objs->time);
						i = 0;
						break;
					}
				}

				//g_print("[%s][%d] objs_1st set! objs->ts[%d]\n", __FUNCTION__, __LINE__, objs->time);
				objs_1st = objs;
				matched_obj++;
				break;
			}
		}
	}

	if (!objs_1st)
	{
		g_print("[%s][%d]!objs_1st\n", __FUNCTION__, __LINE__);
		return 0.0;
	}

	if(time - objs_1st->time < dwell_time)
	{
		g_print("[%s][%d] not enough datas. dwell_time[%d] time[%d] objs_1st->time[%d] diff[%d]\n", __FUNCTION__, __LINE__, dwell_time, time, objs_1st->time, time - objs_1st->time);
		return 0.0;
	}

	g_print("[%s][%d] matched_obj/compared_objs [%lf]\n", __FUNCTION__, __LINE__, matched_obj / compared_objs);
	return (matched_obj / compared_objs);
}

static int nf_dva_alg_illegal_parking(struct objects *objs)
{
	gchar *vehicles;
	gchar dbkey[64] = {0};
	gchar obj_name[64] = {0};
	struct object *obj = NULL;
	gint i = 0;
	gint ch = objs->ch;

	static guint noti_ts[NUM_ACTIVE_CH] = {0};
	guint ignr_itvl = 0;

#ifdef DLVA_FOR_QBRIDGE
	vehicles = "person:1";
#else
	if(!_is_dva_active_on(ch, "ipz"))
		return 0;
		
	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ipz.item_list", ch);
	vehicles = nf_sysdb_get_str_nocopy(dbkey);
#endif

	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ignore_interval", ch);
	ignr_itvl = nf_sysdb_get_uint(dbkey);

	if(objs->time < noti_ts[ch]) //datetime changed to the past
		noti_ts[ch] = 0;
		
	if (noti_ts[ch] + ignr_itvl >= objs->time)
		return 0;

	for (i = 0; i < objs->obj_num; i++)
	{
		obj = &objs->obj[i];
		snprintf(obj_name, sizeof(obj_name), "[%s:1]", obj->name);
		if(strstr(vehicles, obj_name))
		{
			if (_search_same_object(ch, objs->time, obj) > 0.7)
			{
				DVA_MSG *msg = malloc(sizeof(DVA_MSG));
				if (msg == NULL) {
					printf("[%s][%d] malloc failed.\n", __FUNCTION__, __LINE__);
					return;
				}
				memset(msg, 0x00, sizeof(DVA_MSG));
				
				msg->type = DVA_ILLEGAL_PARKING;
				msg->ch = (guchar)ch;
				msg->timestamp = objs->time;
				msg->timestampl = objs->timel;
				
				strncpy(msg->illegal_parking.name, obj->name, sizeof(obj->name));
				msg->illegal_parking.confidence = obj->confidence;
				msg->illegal_parking.bbox.width = 300;
				msg->illegal_parking.bbox.height = 300;
				msg->illegal_parking.bbox.coords[0] = obj->bbx.top.x;
				msg->illegal_parking.bbox.coords[1] = obj->bbx.top.y;
				msg->illegal_parking.bbox.coords[2] = obj->bbx.bottom.x;
				msg->illegal_parking.bbox.coords[3] = obj->bbx.bottom.y;

				msg->snapshot.format = DVA_IMG_JPG;
				msg->snapshot.width = objs->img.width;
				msg->snapshot.height = objs->img.height;
				msg->snapshot.size = objs->img.size;
				memcpy(msg->snapshot.data, objs->img.data, sizeof(unsigned char) * objs->img.size);

				printf("\e[31m[%s][%d]\e[0m dva event fire [ch%d:%s]\n", __FUNCTION__, __LINE__, ch, obj->name);
#ifdef DLVA_FOR_QBRIDGE
				nf_cloud_event_put_object_detected(msg);
#else
				nf_notify_fire_pointer("dva_event", msg, sizeof(DVA_MSG));
				nf_dva_eventlog_put(msg);
#endif					
				noti_ts[ch] = objs->time;
				
				if (msg)
					free(msg);
			}
		}
	}

	return 0;
}

static int nf_dva_alg_illegal_parking_append_objs(struct objects *objs)
{
	//struct objects *_objs;
	struct objs_coords *_objs;
	gint ch = objs->ch;
	GSList *list = alg_ipz_list[ch];
	gchar obj_name[64] = {0};
	gchar dbkey[64] = {0};
	gchar *vehicles;
	guint dwell_time = 0;
	guint ignr_itvl = 0;
	gint i = 0;
	guint len = 0;
	struct object *obj;

	if(!_is_dva_active_on(ch, "ipz"))
		return 0;
		
	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ipz.dwell", ch);
	dwell_time = nf_sysdb_get_uint(dbkey) * 60;

	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ignore_interval", ch);
	ignr_itvl = nf_sysdb_get_uint(dbkey);

#ifdef DLVA_FOR_QBRIDGE
	vehicles = "person:1";
#else
	snprintf(dbkey, sizeof(dbkey), "cam.dva.D%d.ipz.item_list", objs->ch);
	vehicles = nf_sysdb_get_str_nocopy(dbkey);
#endif
	
	for (i = 0; i < objs->obj_num; i++)
	{
		obj = &objs->obj[i];
		snprintf(obj_name, sizeof(obj_name), "[%s:1]", obj->name);
		if(strstr(vehicles, obj_name))
		{
			_objs = (struct objs_coords*)malloc(sizeof(struct objs_coords));
			memcpy(_objs, objs, sizeof(struct objs_coords));
			list = g_slist_append(list, _objs);
			len = g_slist_length(list);

			{
				GSList *llist = g_slist_nth(list, 0);
				if (llist) {
					struct objs_coords *objs_1st = (struct objs_coords *)llist->data;

					if(objs->time - objs_1st->time > dwell_time + ignr_itvl
					|| len >= dwell_time + 256)
					{
						list = g_slist_remove_link(list, llist);
						
						printf("[%s][%d] free(llist->data)\n", __FUNCTION__, __LINE__);
						free(llist->data);
						
						printf("[%s][%d] g_slist_free(llist)\n", __FUNCTION__, __LINE__);
						g_slist_free(llist);
					}
				}
			}

			printf("\e[31m[%s][%d]\e[0m obj append[%s] len[%d]\n", __FUNCTION__, __LINE__, obj->name, len);
			break;
		}
	}
	alg_ipz_list[ch] = list;
	
	return 0;
}


static void nf_dva_alg_run(struct objects *objs)
{
	gint i = 0;
	struct object *obj = NULL;
	
	if (!_is_dva_schedule_on(objs->ch))
		return;

	for (i = 0; i < objs->obj_num; i++)
	{
		obj = &objs->obj[i];
		obj->bbx.top.x = obj->bbx.top.x < 0.0 ? 0.0 : (obj->bbx.top.x > 1.0 ? 1.0 : obj->bbx.top.x);
		obj->bbx.top.y = obj->bbx.top.y < 0.0 ? 0.0 : (obj->bbx.top.y > 1.0 ? 1.0 : obj->bbx.top.y);
		obj->bbx.bottom.x = obj->bbx.bottom.x < 0.0 ? 0.0 : (obj->bbx.bottom.x > 1.0 ? 1.0 : obj->bbx.bottom.x);
		obj->bbx.bottom.y = obj->bbx.bottom.y < 0.0 ? 0.0 : (obj->bbx.bottom.y > 1.0 ? 1.0 : obj->bbx.bottom.y);
	}

	nf_dva_alg_illegal_parking_append_objs(objs);
	
	if (nf_va_object_detector_get_event()) // skip event during settting dva
	{
		nf_dva_alg_intrusion_detection(objs);
		nf_dva_alg_illegal_parking(objs);
	}
}

void nf_dva_detector_cb_func(struct objects *objs)
{
	if (!objs)
		return;
		
	nf_notify_fire_pointer("dva_object", objs, sizeof(struct objects));

	gint is_vloss = nf_notify_get_param0("vloss") >> objs->ch & 0x1;
	if (!is_vloss) 
		nf_dva_alg_run(objs);
}

void nf_dva_event_init(void)
{
	gint ch = 0;
	
	memset(&_obj_counts, 0x0, sizeof(_obj_counts));
	for (ch = 0; ch < NUM_ACTIVE_CH; ch++)
		_obj_counts[ch].ch = ch;
		
	/*
	for (ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		DVA_OBJ_COUNTER *obj_cnt = nf_dva_get_obj_count(ch);
		printf("[%s] ch[%d]\n", __FUNCTION__, obj_cnt->ch);
	}
	*/
}

