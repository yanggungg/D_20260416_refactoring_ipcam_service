#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "onvif_common.h"
#include "onvif_cross_util.h"
#include <glib.h>
#include <pthread.h>
#if defined(WIN32)
#include "onvif_util.h"
#include <time.h>
#else 
#include "sys/sysinfo.h"
#endif

extern struct event_subscription	g_onvif_event_sub[MAX_SUBSCRIPTION_CNT];
extern ONVIF_EVENT_MSGBUF	g_onvif_event_msgbuf;
static int find_metadata_num=1;
extern GArray* rec_arr;
static gchar hw_param_vendor[NF_SYSMAN_HW_PARAM_MAX_LEN];

gboolean nf_sysman_get_hw_param_vendor(gchar vendor[NF_SYSMAN_HW_PARAM_MAX_LEN])
{
	memset(vendor, 0x00, sizeof(vendor));

//	g_message("[%s] VENDOR=[%s]", __FUNCTION__, hw_param_vendor);
	
	strncpy(vendor, hw_param_vendor, NF_SYSMAN_HW_PARAM_MAX_LEN);
//	strncpy(vendor, "ITX-KR-201112050001-50-R4-ZAP1B", NF_SYSMAN_HW_PARAM_MAX_LEN);

	return TRUE;
}

int cleanup_event_buf()
{
	int i, j;
	int ss_id;
	unsigned int sub_t_time;

	LOCK_onvif_event_subs();
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++) {
		ss_id = g_onvif_event_sub[i].ss_id;
		if (ss_id == 0 || ss_id == INVALID_SSID) continue;
		sub_t_time = g_onvif_event_sub[i].t_time;

	// printf("===================*** SUB ID ***=============================\n");
	// printf("                         %d\n", ss_id);
	// printf("===================*** SUB ID ***=============================\n");
		LOCK_onvif_event_msgbuf();
		for (j = 0; j < MAX_MSG_BUF; ++j) {
			if ( g_onvif_event_msgbuf.msg_buf[j].ss_id == ss_id )	{
				if ( g_onvif_event_msgbuf.msg_buf[j].time_tick > sub_t_time ) {
					// printf("SKSHIN] Invalid Subscription time\n");
					break;
				}
			}
		}
		UNLOCK_onvif_event_msgbuf();
	}
	UNLOCK_onvif_event_subs();

}

int count_event_buf()
{
	int i, j;
	int ss_id;
	unsigned int sub_t_time;
	int cnt = 0;

		LOCK_onvif_event_msgbuf();
		for (j = 0; j < MAX_MSG_BUF; ++j) {
			if ( g_onvif_event_msgbuf.msg_buf[j].ss_id >= START_SSID)	{
				if ( g_onvif_event_msgbuf.msg_buf[j].sent == 0)	{
					++cnt;
				}
			}
		}
		UNLOCK_onvif_event_msgbuf();
//	printf("SKSHIN] CURRENT MESSSAGE COUNT = %d\n", cnt);
	return cnt;
}


int append_onvif_meta_event_msg(unsigned int key, void *data, unsigned int state, int ss_id)
{
	ONVIF_EVENT_MSGBUF	*ebuf;
	struct sysinfo info;
	time_t now; 
	int i;
	int cnt = 0;
	

	now = time(NULL);

	//put metadata
	nf_rec_metadata_handoff(key, data, state,  ss_id, now, now);//info.uptime);

	return 1;
}

int append_onvif_single_event_msg(unsigned int key, void *data, unsigned int state, int ss_id)
{
	ONVIF_EVENT_MSGBUF	*ebuf;
	struct sysinfo info;
	time_t now; 
	int i;
	int cnt = 0;
	

	now = time(NULL);

	//put metadata
//	nf_rec_metadata_handoff(key, data, state,  ss_id, now, now);//info.uptime);

	/* LOCK */
	LOCK_onvif_event_msgbuf();
	
	ebuf = &g_onvif_event_msgbuf;

	while (1) {
		if (ebuf->msg_buf[ebuf->tail].ss_id == INVALID_SSID) {
			break;
		}
	ebuf->tail++;
	if (ebuf->tail == MAX_MSG_BUF) {
		ebuf->tail = 0;
	}

		// g_message("[%s] event msg buf tail moved to %d", __FUNCTION__, ebuf->tail);

		cnt++;
		if (cnt == MAX_MSG_BUF) {
			UNLOCK_onvif_event_msgbuf();
			g_message("[%s] event msg buf full!!!", __FUNCTION__);
			return -1;
		}
	}

	struct event_subscription sub;
	int ret = 0;

	memset(&sub, 0x00, sizeof(sub));
	ret = OV_event_get_subscription(&sub, ss_id);
	if( ret != ss_id ){
		UNLOCK_onvif_event_msgbuf();
		return -1;
	}

	switch(key){
		case E_KEY_SENSOR:
			if (sub.tf.topic_filter[E_KEY_SENSOR] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].alarm, data, sizeof(struct e_alarm_type));
//			printf("E_KEY_SENSOR ch[%d] val[%d]\n", ebuf->msg_buf[ebuf->tail].alarm.ch, ebuf->msg_buf[ebuf->tail].alarm.value);
			break;
		case E_KEY_ITX_MOTION:
			if (sub.tf.topic_filter[E_KEY_ITX_MOTION] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].motion, data, sizeof(struct e_motion_type));
//			printf("E_KEY_ITX_MOTION ch[%d] val[%d]\n", ebuf->msg_buf[ebuf->tail].motion.ch, ebuf->msg_buf[ebuf->tail].motion.value);	
			break;
		case E_KEY_MOTION_ALARM:
			if (sub.tf.topic_filter[E_KEY_MOTION_ALARM] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].motion, data, sizeof(struct e_motion_type));
//			printf("E_KEY_MOTION_ALARM ch[%d] val[%d]\n", ebuf->msg_buf[ebuf->tail].motion.ch, ebuf->msg_buf[ebuf->tail].motion.value);
			break;
		case E_KEY_VIDEOANALYTICS_MOTION0:
			if (sub.tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION0] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION1:
			if (sub.tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION1] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION2:
			if (sub.tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION2] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_VIDEOANALYTICS_MOTION3:
			if (sub.tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION3] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_RECORDING_JOB_STATE_CHANGE:
			break;
		case E_KEY_CONFIGURATION_CHANGE:
			break;
		case E_KEY_DATA_DELETION:
			break;
		case E_KEY_RECORDING_AND_TRACK:
			break;
		case E_KEY_DIGITAL_INPUT:
			if (sub.tf.topic_filter[E_KEY_DIGITAL_INPUT] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].alarm, data, sizeof(struct e_alarm_type));
			break;
		case E_KEY_RELAY:
			if (sub.tf.topic_filter[E_KEY_RELAY] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].relay, data, sizeof(struct e_relay_type));
//			printf("E_KEY_RELAY ch[%d] val[%d]\n", ebuf->msg_buf[ebuf->tail].relay.ch, ebuf->msg_buf[ebuf->tail].relay.active);
			break;
		case E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE:
			if (sub.tf.topic_filter[E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].image, data, sizeof(struct e_image_type));
//			printf("E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE ch[%d] val[%d]\n", ebuf->msg_buf[ebuf->tail].image.ch, ebuf->msg_buf[ebuf->tail].image.value);
			break;
		case E_KEY_VIDEO_SIGNAL_LOSS:
			if (sub.tf.topic_filter[E_KEY_VIDEO_SIGNAL_LOSS] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].image, data, sizeof(struct e_image_type));
//			printf("E_KEY_VIDEO_SIGNAL_LOSS ch[%d] val[%d]\n", ebuf->msg_buf[ebuf->tail].image.ch, ebuf->msg_buf[ebuf->tail].image.value);
			break;			
		case E_KEY_EVENT_CONFIGURATION_CHANGE:
			if (sub.tf.topic_filter[E_KEY_EVENT_CONFIGURATION_CHANGE] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].event_conf, data, sizeof(struct e_event_config_type));
//			printf("E_KEY_EVENT_CONFIGURATION_CHANGE cate[%d] prev[%04X] new[%04X]\n", ebuf->msg_buf[ebuf->tail].event_conf.category	, ebuf->msg_buf[ebuf->tail].event_conf.prev, ebuf->msg_buf[ebuf->tail].event_conf.next);			
			break;
		case E_KEY_ADVANCED_SENSOR:
			if (sub.tf.topic_filter[E_KEY_ADVANCED_SENSOR] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].ad_alarm, data, sizeof(struct e_ad_alarm_type));
//			printf("E_KEY_ADVANCED_SENSOR ch[%d] state[%d]\n", ebuf->msg_buf[ebuf->tail].ad_alarm.ch, ebuf->msg_buf[ebuf->tail].ad_alarm.state);
			break;
        case E_KEY_CHANGE_PROFILE:
			if (sub.tf.topic_filter[E_KEY_CHANGE_PROFILE] == ONVIF_FILTERED)
				goto filtered;
            memcpy(&ebuf->msg_buf[ebuf->tail].prof_change, data, sizeof(struct e_profile_change_type));
            break;
        case E_KEY_CHANGE_CONFIGURATION:
			if (sub.tf.topic_filter[E_KEY_CHANGE_CONFIGURATION] == ONVIF_FILTERED)
				goto filtered;
            memcpy(&ebuf->msg_buf[ebuf->tail].config_change, data, sizeof(struct e_configuration_change_type));
			break;
		case E_KEY_IMAGE_TOO_BLURRY:
			if (sub.tf.topic_filter[E_KEY_IMAGE_TOO_BLURRY] == ONVIF_FILTERED)
				goto filtered;
			memcpy(&ebuf->msg_buf[ebuf->tail].image, data, sizeof(struct e_image_type));
			break;
//ONVIF_NEW_EVENT			
#if 0
		case E_KEY_RELAY:
			memcpy(&ebuf->msg_buf[ebuf->tail].relay, data, sizeof(struct e_alarm_type));
			break;
#endif
		default:			
			break;			
	}

	ebuf->msg_buf[ebuf->tail].key = key;
	ebuf->msg_buf[ebuf->tail].state = state;	
	ebuf->msg_buf[ebuf->tail].time_tick = (unsigned int)now;
	ebuf->msg_buf[ebuf->tail].uptime_tick = now;//info.uptime;
	// If the ss_id is not 0, this message can be used for only the subscription.
	ebuf->msg_buf[ebuf->tail].ss_id = ss_id;
	ebuf->msg_buf[ebuf->tail].sent = 0;

	/* append msg */
	ebuf->tail++;
	if( ebuf->tail>= MAX_MSG_BUF )
		ebuf->tail= 0;

	/* UNLOCK */
	UNLOCK_onvif_event_msgbuf();

	count_event_buf();
#if 0
	int i=0;
	printf("============================================\n");
	for(i=0; i< MAX_MSG_BUF; i++){
		printf("[%d] key[%d] state[%d] time_t[%d] uptime[%ld]\n", i, ebuf->msg_buf[i].key, ebuf->msg_buf[i].state, ebuf->msg_buf[i].time_tick, ebuf->msg_buf[i].uptime_tick);
	}
	printf("\n\n\n");
#endif

	return 1;

filtered:
	UNLOCK_onvif_event_msgbuf();
	return 0;
}

int append_onvif_event_msg(unsigned int key, void *data, unsigned int state, int ss_id)
{
	struct event_subscription subs[MAX_SUBSCRIPTION_CNT];
	int i;
	int sid;

	cleanup_event_buf();
		
	LOCK_onvif_event_subs();
	memcpy(subs, g_onvif_event_sub, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);
	UNLOCK_onvif_event_subs();

	if (ss_id == BROADCAST_SSID) {
		if (nf_issm_ctl_get_restart_checker() == 1)
			append_onvif_meta_event_msg(key, data, state, 0);

		for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
		{
			if ( subs[i].ss_id >= START_SSID )
			{
				sid = subs[i].ss_id;
				append_onvif_single_event_msg(key, data, state, sid);
			}
		}
	}
	else {
		/* check same subscription id */
		for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
		{
			if ( subs[i].ss_id == ss_id )
			{
				append_onvif_single_event_msg(key, data, state, ss_id);
				break;
			}
		}
	}
	return 1;	
}

int remove_onvif_event_msg(int ss_id)
{
	ONVIF_EVENT_MSGBUF	*ebuf;
	 
	int i;

	/* LOCK */
	LOCK_onvif_event_msgbuf();
	
	ebuf = &g_onvif_event_msgbuf;

	for (i = 0; i < MAX_MSG_BUF; ++i) {
		if (ebuf->msg_buf[i].ss_id == ss_id) {
			ebuf->msg_buf[i].ss_id = INVALID_SSID;
			ebuf->msg_buf[i].sent = 1;
		}
	}

	/* UNLOCK */
	UNLOCK_onvif_event_msgbuf();
}

int update_onvif_event_msg(int pos, int sent)
{
	ONVIF_EVENT_MSGBUF	*ebuf;
	 
	/* LOCK */
	LOCK_onvif_event_msgbuf();
	
	ebuf = &g_onvif_event_msgbuf;
	ebuf->msg_buf[pos].ss_id = INVALID_SSID;
	ebuf->msg_buf[pos].sent = sent;

	/* UNLOCK */
	UNLOCK_onvif_event_msgbuf();
}

void onvif_cross_util_test(int a)
{
	printf("a %d \n",a);
}


int checkRecordingExist(char* recordingToken)
{
	char* value;
	char name[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int recordingTokenExist =0;
	int i=0;
	for(i=0; i<ONVIF_MAX_RECORDING_CNT ; i++) {
		sprintf(name, "onvif.recording%d.token",i);
		value = nf_sysdb_get_str_nocopy(name);
		if(!strcmp(value,recordingToken))
		{

				return ONVIF_ERR_RET_SUCCESS;
			
		}

	}
	if(i == ONVIF_MAX_RECORDING_CNT) { // recording token is not exist
		return ONVIF_R_ERR_NO_RECORDING;
	}
	

	return ONVIF_ERR_RET_SUCCESS;
}
int getJobModeInt(char* mode)
{
	if(!strcmp(mode,"Active"))
	{
		return 1;
	}else if(!strcmp(mode,"Idle")){
		return 0;
	}else {
		_TTY_LOG_ONVIF_DEBUG("Warning:: Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	}
	return 1;
}
int getJobModeString(int modint, char* mode)
{
	if(modint == 1)
	{
		strncpy(mode,"Active",COMMON_SIZE -1);
	}else if(modint == 0) {
		strncpy(mode,"Idle",COMMON_SIZE -1);
	}else {
		_TTY_LOG_ONVIF_DEBUG("Warning:: Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	}
	
	return 1;

}
void recordingJobSource(char* jobToken,arg_RecordingJobSource* source)
{
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };


	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recordingjob%d.token", i);
		if(!strcmp(jobToken,nf_sysdb_get_str_nocopy(buff)))
		{

			sprintf(buff, "onvif.recordingjob%d.source_token", i);
			strncpy(source[0].SourceToken,nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			strncpy(source[0].SourceTokenType,"http://www.onvif.org/ver10/schema/Profile", COMMON_SIZE - 1);
			
			

		}
	}
}



int SetPanicRecording(int ch, char* mode)
{
	int modeint=0;
	if(!strcmp(mode,"Idle")) {
		printf("======================set Idle==================");
		modeint =0;
	}
	else if(!strcmp(mode,"Active"))  {
		printf("======================set Active==================");
		modeint =1;
	}else {
		printf("ASSERT!!!! %s %d",__FUNCTION__, __LINE__);
	}
	PanicRecordingArray[ch] = modeint;
	return modeint;
}
int GetPanicRecording(int ch,char* buff)
{
	switch(PanicRecordingArray[ch])
	{
	case 0:
		printf("======================Idle==================");
		strncpy(buff,"Idle",COMMON_SIZE-1);
		break;
	case 1:
		printf("======================Active==================");
		strncpy(buff,"Active",COMMON_SIZE-1);
		break;
	}
	return PanicRecordingArray[ch];
}

gboolean GetDeviceRecordingSummary(arg_DeviceRecordingSummary * sum, int ch)
{
	int ret=1;

#if defined(WIN32) 
	sum->dataUntil =VIRTUAL_DATA_TIME_END;
	sum->dataFrom =  VIRTUAL_DATA_TIME_START;
	sum->recordingNum = ONVIF_CH;
#else
//	ret = nf_disk_rec_time(1, &nrec_time, NULL);
#endif 
	return ret;
}
int onvif_recording_metadata(int ch,  int max,int filter, time_t start_time, time_t end_time )
{
	int i=0,j=0;
	recordingEvent_info* ri;
	int order=0;
	//for(i=0; i<ONVIF_CH;i++)
	//{
	//	if( (ch & (1 << i)) == 0) {
	//		continue;
	//	}	
	

			if(ch==0) return 0; // �ƹ� ��û�� ���� �ʾҴ�.

					// here below mockup code.
					// event log �� Panic ���� �ð� ���ð��� ������ �ͼ�  �־��ش�., �˻� ������ start_time, �˻� ���� end_time 
			if(start_time<end_time) order = 0; // desending
			else order =1; // asending
				
					if(max == -1)  max = 10; // 
					while(eve_arr->len < max) {
						
						if(j*2 >= max)
						{
							break;
						}
						ri = g_new (recordingEvent_info, 1);
						if(order) ri->time = VIRTUAL_DATA_TIME_START + 1000*(4-j);
						else ri->time = VIRTUAL_DATA_TIME_START + 1000*j;

						if((ri->time < start_time || ri->time > end_time) && !order) {
							j++;
							continue;
						}
						if((ri->time > start_time || ri->time < end_time) && order) {
							j++;
							continue;
						}
						ri->ch = 0;
						ri->eventType = TYPE_METADATA;
						strncpy(ri->trackToken,"tm0",COMMON_SIZE-1);
						pthread_mutex_lock(&ri_mutex);
						g_array_append_val(eve_arr, *ri);		
						pthread_mutex_unlock(&ri_mutex);
						j++;
					
					}
			


//	}

	return 0;
}
int onvif_recording_event(int ch,  int max,int filter, time_t start_time, time_t end_time )
{
	int i=0,j=0;
	recordingEvent_info* ri;

	for(i=0; i<ONVIF_CH;i++)
	{
		if( (ch & (1 << i)) == 0) {
			continue;
		}	
	
		if(filter == ONVIF_RECORDING_ALL || filter == ONVIF_RECORDING_STATUS)
		{
			

					// here below mockup code.
					// event log �� Panic ���� �ð� ���ð��� ������ �ͼ�  �־��ش�., �˻� ������ start_time, �˻� ���� end_time 
			
					if(max == -1)  max = 10; // 
					while(eve_arr->len < max) {
						pthread_mutex_lock(&ri_mutex);

						ri = g_new (recordingEvent_info, 1);
						ri->time = VIRTUAL_DATA_TIME_START + 1000*j++;
						ri->ch = i;
						ri->eventType = TYPE_RECORDING;
						strncpy(ri->trackToken,"tv0",COMMON_SIZE-1);
						ri->is_recording = true;
						g_array_append_val(eve_arr, *ri);		
						pthread_mutex_unlock(&ri_mutex);
						
						if(j*2 >= max)
						{
							break;
						}
					}


		}
		if(filter == ONVIF_RECORDING_ALL || filter == ONVIF_RECORDING_CLIP)
		{
			
				while(eve_arr->len < max) {
					pthread_mutex_lock(&ri_mutex);

					ri = g_new (recordingEvent_info, 1);
					ri->time =  VIRTUAL_DATA_TIME_START + 1000*j++;
					ri->ch = i;
					ri->eventType = TYPE_TRACK;
					strncpy(ri->trackToken,"t0",COMMON_SIZE-1);
					ri->is_data_present = true; // �������� : true ������ : false
					g_array_append_val(eve_arr, *ri);		
					pthread_mutex_unlock(&ri_mutex);
				}
			
		}
	}




	return 0;
}
