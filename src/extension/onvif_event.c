#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#include "nf_common.h"
#include "nf_util_time.h"
#include "nf_util_netif.h"
#include "nf_action.h"
//#include "nf_webra.h"
#include "nf_api_ipcam.h"
//#include "nf_webra_def.h"
#include "onvif_common.h"
#include "onvif_service_util.h"

#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
	extern guint _nf_action_num_alarm;
#endif
pthread_mutex_t		g_event_msgbuf_mutex;
pthread_mutex_t		g_event_subscription_mutex;
pthread_mutex_t		g_event_notify_data;
struct event_subscription	g_onvif_event_sub[MAX_SUBSCRIPTION_CNT];
ONVIF_EVENT_MSGBUF	g_onvif_event_msgbuf;

void LOCK_onvif_event_msgbuf()
{
	pthread_mutex_lock(&g_event_msgbuf_mutex);
}

void UNLOCK_onvif_event_msgbuf()
{
	pthread_mutex_unlock(&g_event_msgbuf_mutex);
}

void LOCK_onvif_event_subs()
{
	pthread_mutex_lock(&g_event_subscription_mutex);
}

void UNLOCK_onvif_event_subs()
{
	pthread_mutex_unlock(&g_event_subscription_mutex);
}

void LOCK_onvif_event_notify_data()
{
	pthread_mutex_lock(&g_event_notify_data);
}

void UNLOCK_onvif_event_notify_data()
{
	pthread_mutex_unlock(&g_event_notify_data);
}

unsigned int copy_from_event_msgbuf(ONVIF_EVENT_MSGBUF * dest)
{
	memset(dest, 0x00, sizeof(ONVIF_EVENT_MSGBUF));
	LOCK_onvif_event_msgbuf();
	memcpy(dest, &g_onvif_event_msgbuf, sizeof(ONVIF_EVENT_MSGBUF));
	UNLOCK_onvif_event_msgbuf();
	return 1;
}

unsigned int copy_from_event_subscription_list(struct event_subscription	* dest)
{
	memset(dest, 0x00, sizeof(struct event_subscription)*MAX_SUBSCRIPTION_CNT);
	LOCK_onvif_event_subs();
	memcpy(dest, &g_onvif_event_sub, sizeof(struct event_subscription)*MAX_SUBSCRIPTION_CNT);
	UNLOCK_onvif_event_subs();
	return 1;	
}


void onvif_event_mem_init()
{
	pthread_mutex_init(&g_event_msgbuf_mutex, NULL);
	memset(&g_onvif_event_msgbuf, 0x00, sizeof(EVENT_MSGBUF_SHM));	
	
	pthread_mutex_init(&g_event_subscription_mutex, NULL);
	memset(g_onvif_event_sub, 0x00, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);	

	pthread_mutex_init(&g_event_notify_data, NULL);
}

int append_onvif_event_msg(unsigned int key, void *data, unsigned int state, int ss_id)
{
	ONVIF_EVENT_MSGBUF	*ebuf;
	struct sysinfo info;
	time_t now; 
	
	 sysinfo(&info);
	 now = time(NULL);
	 
	/* LOCK */
	LOCK_onvif_event_msgbuf();
	
	ebuf = &g_onvif_event_msgbuf;
	
	/* append msg */
	ebuf->start++;
	if( ebuf->start >= MAX_MSG_BUF )
		ebuf->start = 0;

	ebuf->msg_buf[ebuf->start].key = key;
	ebuf->msg_buf[ebuf->start].state = state;	
	ebuf->msg_buf[ebuf->start].time_tick = (unsigned int)now;
	ebuf->msg_buf[ebuf->start].uptime_tick = info.uptime;
	// If the ss_id is not 0, this message can be used for only the subscription.
	ebuf->msg_buf[ebuf->start].ss_id = ss_id;

	switch(key){
		case E_KEY_SENSOR:
			memcpy(&ebuf->msg_buf[ebuf->start].alarm, data, sizeof(struct e_alarm_type));
			break;
		case E_KEY_ITX_MOTION:
			memcpy(&ebuf->msg_buf[ebuf->start].motion, data, sizeof(struct e_motion_type));
			break;
		case E_KEY_MOTION_ALARM:
			memcpy(&ebuf->msg_buf[ebuf->start].motion, data, sizeof(struct e_motion_type));			
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
			memcpy(&ebuf->msg_buf[ebuf->start].alarm, data, sizeof(struct e_alarm_type));
			break;
		case E_KEY_RELAY:
			memcpy(&ebuf->msg_buf[ebuf->start].alarm, data, sizeof(struct e_alarm_type));
			break;
//ONVIF_NEW_EVENT			
#if 0
		case E_KEY_RELAY:
			memcpy(&ebuf->msg_buf[ebuf->start].relay, data, sizeof(struct e_alarm_type));
			break;
#endif
		default:			
			break;			
	}

	/* UNLOCK */
	UNLOCK_onvif_event_msgbuf();

#if 0
	int i=0;
	printf("============================================\n");
	for(i=0; i< MAX_MSG_BUF; i++){
		printf("[%d] key[%d] state[%d] time_t[%d] uptime[%ld]\n", i, ebuf->msg_buf[i].key, ebuf->msg_buf[i].state, ebuf->msg_buf[i].time_tick, ebuf->msg_buf[i].uptime_tick);
	}
	printf("\n\n\n");
#endif

	return 1;	
}

int add_event_subscription(int ss_id, time_t termination_time, struct onvif_filters *filters)
{
	int ret, i;
	time_t now;
	struct event_subscription subs[MAX_SUBSCRIPTION_CNT];
	ONVIF_EVENT_MSGBUF eb;
	struct onvif_filters tmp_filters;

	int sub_index = 0;

	if ( !ss_id )
	{
		return -1;
	}

	if(  filters != NULL ){
		memcpy(&tmp_filters, filters, sizeof(struct onvif_filters ));
	}	

	if( termination_time == 0 ){
		now = time(NULL);
		termination_time = now + 600;
		OV_DEBUG("termination time is 0, set to current[%d] termination[%d]", (int)now, (int)termination_time);
	}

	copy_from_event_msgbuf(&eb);

	LOCK_onvif_event_subs();
	memcpy(subs, g_onvif_event_sub, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);

	/* check same subscription id */
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
	{
		if ( subs[i].ss_id == ss_id )
		{
			UNLOCK_onvif_event_subs();
			return 5;
		}
	}

	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
	{
		if ( subs[i].ss_id == 0 )
		{
			subs[i].ss_id = ss_id;
			subs[i].t_time = termination_time;
			subs[i].u_time = eb.msg_buf[eb.start].uptime_tick;
			subs[i].old_start_idx = eb.start;
			if ( eb.start >= (MAX_MSG_BUF - 1) )
			{
				subs[i].mbuf_end = 0;
			}
			else
			{
				subs[i].mbuf_end = eb.start + 1;
			}
			//FIXME.convert filter
			memset(&subs[i].tf, 0x00, sizeof(struct e_topic_filter));
			memset(&subs[i].mf, 0x00, sizeof(struct e_msg_filter));
			// tmp_filters  ->  sub.[i].tf / sub.[i].mf
			if( filters->cnt ){
				ret = make_filter_flags(filters, &subs[i]);
				if( ret < 0){
					OV_DEBUG("Making Filter is Failed !!\n");
				}
			}
			sub_index = i;
			break;
		}
		else
		{
			now = time(NULL);
			if ( subs[i].t_time < now )
			{
				subs[i].ss_id = ss_id;
				subs[i].u_time = eb.msg_buf[eb.start].uptime_tick;
				subs[i].old_start_idx = eb.start;
				if ( eb.start >= (MAX_MSG_BUF - 1) )
				{
					subs[i].mbuf_end = 0;
				}
				else
				{
					subs[i].mbuf_end = eb.start + 1;
				}
				subs[i].t_time = termination_time;
				//FIXME.convert filter
				memset(&subs[i].tf, 0x00, sizeof(struct e_topic_filter));
				memset(&subs[i].mf, 0x00, sizeof(struct e_msg_filter));
				// tmp_filters  ->  sub.[i].tf / sub.[i].mf
				if( filters->cnt ){
					ret = make_filter_flags(filters, &subs[i]);
					if( ret < 0){
						OV_DEBUG("Making Filter is Failed !!\n");
					}
				}				
				sub_index = i;
				break;
			}
		}
	}
	if (i == MAX_SUBSCRIPTION_CNT )
	{
		UNLOCK_onvif_event_subs();
		return -1;
	}
   
	memcpy(g_onvif_event_sub, subs, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);
#if 0
	for(i=0; i<MAX_SUBSCRIPTION_CNT; i++){
		OV_DEBUG("sub[%d] id[%d] t[%d] up[%d] start[%d] end[%d]", i, g_onvif_event_sub[i].ss_id, g_onvif_event_sub[i].t_time, g_onvif_event_sub[i].u_time, g_onvif_event_sub[i].old_start_idx, g_onvif_event_sub[i].mbuf_end);
	}
#endif
	UNLOCK_onvif_event_subs();

	return sub_index;
}


/*
1. Description : add new subscription to event subsctiprion list.
2. IN 
 - Termination time
 - Filter option
3. OUT 
 - Subscription ID.
*/
void random_bytes( unsigned char *dest, int len );
int OV_event_create_new_subscription(unsigned int termination, struct onvif_filters *filters)
{
	
	int ret = 0, sub_id=0, rnd = 0, current_time = 0;

	current_time = (int)time(NULL);
	
	ret = 5; // 5 means that the sub_id is same.

	while (ret == 5) {
		random_bytes((unsigned char *)&rnd, sizeof(unsigned char) );
		sub_id = current_time | (rnd << 23);
		ret = add_event_subscription( sub_id, (time_t)termination, filters);
	}
	if ( ret < 0) {
		return -1;
	}
	
	return sub_id;
}

int OV_event_get_subscription(struct event_subscription *sub, int ss_id)
{
	struct event_subscription sub_list[MAX_SUBSCRIPTION_CNT];
	int i=0;
	time_t now;
		
	memset(sub_list, 0x00, sizeof(struct event_subscription)*MAX_SUBSCRIPTION_CNT);
	LOCK_onvif_event_subs();
	memcpy(sub_list, g_onvif_event_sub, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);
	UNLOCK_onvif_event_subs();		

	if ( !ss_id )
	{
		return -1;
	}
	else
	{
		for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
		{
			if ( sub_list[i].ss_id == ss_id )
			{
				now = time(NULL);
				if ( sub_list[i].t_time > now)
				{
					memcpy(sub, &sub_list[i], sizeof(struct event_subscription));
#if 0
OV_DEBUG(">> ssid[%d] t_time[%d] utime[%d]", sub->ss_id, sub->t_time, sub->u_time);
#endif
					return ss_id;
				}
			}
		}
	}
	
	/* fail */
	return -1;

}

int onvif_update_event_subscription(int ss_id, long utime, unsigned int old_start, unsigned int buf_end)
{
	int shmid_bn, shmid_pull, ret1, ret2, i, j;
	time_t now;
	void *mem_bn, *mem_pull;
	int semid_bn, semid_pull;

	struct event_subscription sub_list[MAX_SUBSCRIPTION_CNT];

	OV_DEBUG("ss id [%d]", ss_id);
	if ( !ss_id ) {
		return -1;
	}
	
	LOCK_onvif_event_subs();
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)	{
		if ( g_onvif_event_sub[i].ss_id == ss_id )	{
			now = time(NULL);
			if ( g_onvif_event_sub[i].t_time > now ) {
				g_onvif_event_sub[i].u_time = utime;
				g_onvif_event_sub[i].old_start_idx = old_start;
				g_onvif_event_sub[i].mbuf_end = buf_end;
				break;
			}
		}
	}
	UNLOCK_onvif_event_subs();

	if ( i == MAX_SUBSCRIPTION_CNT ) {
		return -1;
	}

	return 0;
}


/*
1. Description : retrieve event messages
2. IN 
 - Subscription ID
 - Pointer to copy event messages 
 - Message limit : This function will be terminated when the number of messages to be sent is reach the message limit.
 - Timeout : This function will be terminated when the timeout is expired even if there are not any messages to be sent.
3. OUT 
 - The number of event message
*/

int OV_BLOCK_event_pull_messages(int ss_id, struct e_msg  *dest, unsigned int limit, unsigned int timeout )
{
	ONVIF_EVENT_MSGBUF eb;
	struct event_subscription sub;
	int ret = 0;
	unsigned int termination_time = 0, msg_num=0, current_mbuf_end = 0;
	time_t now;

	if( dest == NULL ){
		return -1;
	}

	ret = OV_event_get_subscription(&sub, ss_id);
	if( ret != ss_id ){
		return 0;
	}

	now = time(NULL);	
	termination_time = (unsigned int)now + timeout + 1;

	while ( now < (time_t)termination_time )	{

		copy_from_event_msgbuf(&eb);

		ret = OV_event_get_subscription(&sub, ss_id);		

		if( ret != ss_id ){
			return -1;
		}

		if( timeout == 0 && now >= sub.t_time ){
			return -1;
		}		

//		if ((sub.filter & ONVIF_F_PROPERTY_OP_CHANGE) && (sub.filter & ONVIF_F_PROPERTY_OP_INIT)) {		

			unsigned int loop_cnt = 0;
//OV_DEBUG(" key[%d] u[%d] id[%d]", eb.msg_buf[sub.mbuf_end].key, eb.msg_buf[sub.mbuf_end].uptime_tick, eb.msg_buf[sub.mbuf_end].ss_id );						
			while ( eb.msg_buf[sub.mbuf_end].key != 0 && eb.msg_buf[sub.mbuf_end].uptime_tick >= sub.u_time ) {  
				       
				if(eb.msg_buf[sub.mbuf_end].ss_id != 0 && eb.msg_buf[sub.mbuf_end].ss_id != sub.ss_id) { // If the ss_id of msg is not 0, this msg can be used for only the subscription
					break;
				}

				/* Filter Check */
				switch ( eb.msg_buf[sub.mbuf_end].key ) {
					case E_KEY_SENSOR:

						/* Filter check */						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
						msg_num++;
						break;
					case E_KEY_ITX_MOTION:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;				
					case E_KEY_MOTION_ALARM:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
					case E_KEY_RECORDING_JOB_STATE_CHANGE:

						/* Filter check */
						printf("Summer Noti : E_KEY_RECORDING_JOB_STATE_CHANGE \n");
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
					case E_KEY_CONFIGURATION_CHANGE:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
					case E_KEY_DATA_DELETION:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
					case E_KEY_RECORDING_AND_TRACK:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
					case E_KEY_DIGITAL_INPUT:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
					case E_KEY_RELAY:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;						
//ONVIF_NEW_EVENT
#if 0
					case E_KEY_ONVIF_NEW_EVENT:

						/* Filter check */
						
						memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));						
						msg_num++;						
						break;
#endif
					default:
						break;
				}
				
				sub.mbuf_end++;
				if ( sub.mbuf_end >= MAX_MSG_BUF ) {
					sub.mbuf_end = 0;
				}

				loop_cnt++;
				if ( loop_cnt >= MAX_MSG_BUF ){
					break;
				}

				if (msg_num >= limit ){
					goto event_done;
				}
			}
//		}
		
		if (msg_num > 0 ) {
			goto event_done;
		}

event_keep_searching:
		usleep(200000);
		now = time(NULL);

	}

	// No Message
	return 0;

event_done :

	/* Set sync time to last msg's uptime_tick */
	if ( sub.mbuf_end == 0 ){
		current_mbuf_end = MAX_MSG_BUF - 1;
	}
	else{
		current_mbuf_end = sub.mbuf_end - 1;
	}

	ret = onvif_update_event_subscription( ss_id, eb.msg_buf[current_mbuf_end].uptime_tick, eb.start, sub.mbuf_end);

	if (msg_num > limit ){
		msg_num = limit;
	}

	OV_DEBUG("I got [%d] messages", msg_num);
	return (int)msg_num;
}

/*
1. Description : Connect to client and send event messages.
2. IN 
 - Subscription ID
 - Destination address
3. OUT 
 - result
*/

typedef struct _notify_param {
    int id;
    char addr[128];
} notify_param;
notify_param func_d;

char* make_http_header(char *send_buf, char *uri, char *tmp_str_addr, unsigned int tmp_port, unsigned int buflen)
{
	char tmp[128] = {0, }, *p;

	p = send_buf;
	/* make HTTP header */
	sprintf(tmp, "POST %s HTTP/1.0\n", uri);
	p = stpcpy(p, tmp);
	sprintf(tmp, "Host: %s:%d\n", tmp_str_addr, tmp_port);
	p = stpcpy(p, tmp);
	sprintf(tmp, "Content-Type: application/soap+xml; charset=utf-8; action=\"http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify\"\n");
	p = stpcpy(p, tmp);
	sprintf(tmp, "Connection: close \n");
	p = stpcpy(p, tmp);
	sprintf(tmp, "Content-Length: %d\n", buflen);
	p = stpcpy(p, tmp);
	sprintf(tmp, "SOAPAction: \"http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify\"\n\n");
	p = stpcpy(p, tmp);

    return p;
}


//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh
//ONVIF_NEW_EVENT
//const char *str_new_topic = "tns1:UserAlarm/tnsitx:AlarmIN\0";
const char *str_tns1_tnsitx_Alarm = "tns1:UserAlarm/tnsitx:AlarmIN\0";
const char *str_tns1_General_Motion = "tns1:VideoSource/MotionAlarm\0";
const char *str_tns1_tnsitx_Motion = "tns1:VideoAnalytics/tnsitx:MotionDetection\0";
const char *str_tns1_tnsitx_jobState_change = "tns1:RecordingConfig/JobState\0";
const char *str_tns1_tnsitx_conf_change = "tns1:RecordingConfig/RecordingConfiguration\0";
const char *str_tns1_tnsitx_data_deletion = "tns1:RecordingConfig/DeleteTrackData\0";
const char *str_tns1_recording_and_track = "tns1:RecordingConfig/CreateRecording\0";
const char *str_tns1_digital_input = "tns1:Device/Trigger/DigitalInput\0";
const char *str_tns1_relay = "tns1:Device/Trigger/Relay\0";

const char* get_topic_name(unsigned int key)
{
	switch( key ){
		case E_KEY_SENSOR:
			return str_tns1_tnsitx_Alarm;
		case E_KEY_ITX_MOTION:
			return str_tns1_tnsitx_Motion;
		case E_KEY_MOTION_ALARM:
			return str_tns1_General_Motion;
		case E_KEY_RECORDING_JOB_STATE_CHANGE:
			return str_tns1_tnsitx_jobState_change;
		case E_KEY_DIGITAL_INPUT:
			return str_tns1_digital_input;			
		case E_KEY_RELAY:
			return str_tns1_relay;		
//ONVIF_NEW_EVENT			
//		case E_KEY_ONVIF_NEW_EVENT:
//			return str_new_topic;
		default:
			return str_tns1_tnsitx_Alarm;
	}
}

char* make_buf_notification_msg(char *send_buf, struct e_msg *msg, unsigned int msg_cnt, char* host_ip, unsigned int tmp_port, int ss_id)
{
	char tmp[128] = {0, }, tmp_time[128] = {0, }, *p;
	int i=0;

	p = send_buf;
	/* make event messages */
	for (i = 0; i < (int)msg_cnt; i++)
	{
		sprintf(tmp, "<ns6:NotificationMessage>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<ns6:SubscriptionReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "http://%s:%d/onvif/subscription?subscription=%d", host_ip, tmp_port, ss_id );
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</ns6:SubscriptionReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<ns6:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete\">");
		p = stpcpy(p, tmp);
		sprintf(tmp, "%s", get_topic_name(msg[i].key));
		p = stpcpy(p, tmp);
		sprintf(tmp, "</ns6:Topic>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<ns6:ProducerReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "http://%s:%d/onvif/subscription?subscription=%d", host_ip, tmp_port, ss_id );
		p = stpcpy(p, tmp);
		sprintf(tmp, "</wsa5:Address>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</ns6:ProducerReference>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "<ns6:Message>");
		p = stpcpy(p, tmp);

		//ONVIF Evnet Key �߰��� �ʼ� ���� : http://222.112.8.34:8090/x/SgNh			
		switch( msg[i].key ){
			case E_KEY_SENSOR:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%s\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%d\" Name=\"Port\">", msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].alarm.value?"close":"open");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_ITX_MOTION:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%s\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"vs%d\" Name=\"VideoSourceConfigurationToken\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"MotionActive\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");				
				p = stpcpy(p, tmp);				
				break;
			case E_KEY_MOTION_ALARM:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%s\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].motion.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].motion.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_DIGITAL_INPUT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%s\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s%d\" Name=\"InputToken\">", DIGITAL_INPUT_TOKEN_PREFIX, msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"LogicalState\">", msg[i].alarm.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);
				break;
			case E_KEY_RELAY:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%s\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s%d\" Name=\"InputToken\">", RELAY_TOKEN_PREFIX, msg[i].alarm.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"LogicalState\">", msg[i].alarm.value?"Active":"Inactive");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);
				break;
//ONVIF_NEW_EVENT				
#if 0
			case E_KEY_ONVIF_NEW_EVENT:
				strftime(tmp_time, 128, "%Y-%m-%dT%H:%M:%S", gmtime((time_t *)&msg[i].time_tick));
				sprintf(tmp, "<ns8:Message PropertyOperation=\"%s\" UtcTime=\"%s\">", msg[i].state?"Initialized":"Changed", tmp_time);
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"vs%d\" Name=\"Source\">", msg[i].new.ch);
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Source>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "<ns8:SimpleItem Value=\"%s\" Name=\"State\">", msg[i].new.value?"true":"false");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:SimpleItem>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Data>");
				p = stpcpy(p, tmp);
				sprintf(tmp, "</ns8:Message>");
				p = stpcpy(p, tmp);
				break;
#endif
			default:
				break;
		}			
		sprintf(tmp, "</ns6:Message>");
		p = stpcpy(p, tmp);
		sprintf(tmp, "</ns6:NotificationMessage>");
		p = stpcpy(p, tmp);
	}
	
	return p;
}

char* make_buf_head(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<SOAP-ENV:Envelope" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns5=\"http://docs.oasis-open.org/wsrf/bf-2\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xmime=\"http://tempuri.org/xmime.xsd\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:xop=\"http://www.w3.org/2004/08/xop/include\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns7=\"http://docs.oasis-open.org/wsn/t-1\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns8=\"http://www.onvif.org/ver10/schema\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns4=\"http://docs.oasis-open.org/wsrf/r-2\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns10=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns11=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns12=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns13=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns14=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns15=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns16=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns6=\"http://docs.oasis-open.org/wsn/b-2\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns9=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:ns1=\"http://www.onvif.org/ver10/events/wsdl\"" );
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tnsitx=\"http://www.itxsecurity.com/onvif/event0/topics\"");
	p = stpcpy(p, tmp);
	sprintf(tmp, " xmlns:tns1=\"http://www.onvif.org/ver10/topics\">");
	p = stpcpy(p, tmp);
	sprintf(tmp, " <SOAP-ENV:Header>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<wsa5:Action>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify");
	p = stpcpy(p, tmp);
	sprintf(tmp, "</wsa5:Action>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "</SOAP-ENV:Header>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<SOAP-ENV:Body>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "<ns6:Notify>");
	p = stpcpy(p, tmp);

	return p;
}

char* make_buf_tail(char *send_buf)
{
	char tmp[128] = {0, }, *p;
	p = send_buf;
	sprintf(tmp, "</ns6:Notify>");
	p = stpcpy(p, (const char*)tmp);
	sprintf(tmp, "</SOAP-ENV:Body>");
	p = stpcpy(p, tmp);
	sprintf(tmp, "</SOAP-ENV:Envelope>\n\n");
	p = stpcpy(p, tmp);

	return p;
}

void __w3curlparse(const char *szurl,
                   char *szprotocol,
                   char *szuser,
                   char *szpassword,
                   char *szaddress,
                   unsigned long *nport,
                   char *szuri )
{
    char szport[1024] = "\0";
    unsigned long npos = 0;
    int bflag = false;

    while (strlen(szurl) > 0 && npos < strlen(szurl) && strncmp((szurl + npos), ":", 1))
        ++npos;

    if (!strncmp((szurl + npos + 1), "/", 1))
    {    // is protocol
        if (szprotocol)
        {
            strncpy(szprotocol, szurl, npos);
            szprotocol[npos] = 0;
        }
        bflag = true;
    }
    else
    {    // is host
        if (szprotocol)
        {
            strncpy(szprotocol, "http", 4);
            szprotocol[5] = '\0';
        }
    }

    unsigned long nsp = 0, usp = 0;

    if (bflag)
    {
        usp = nsp = npos += 3;
    }
    else
    {
        usp = nsp = npos = 0;
    }

    while (strlen(szurl) > 0 && usp < strlen(szurl) && strncmp((szurl + usp), "@", 1))
        ++usp;

    if (usp < strlen(szurl))
    { // find username and find password
        unsigned long ssp = nsp;
        while (strlen(szurl) > 0 && npos < strlen(szurl) && strncmp((szurl + ssp), ":", 1))
            ++ssp;

        if (ssp < usp)
        { // find
            strncpy(szuser, szurl + nsp, ssp - nsp);
            szuser[ssp - nsp + 1] = '\0';
            strncpy(szpassword, szurl + ssp + 1, usp - ssp - 1);
            szpassword[usp - ssp] = '\0';
        }

        nsp = npos = usp + 1;
    }

    bflag = false;
    while (strlen(szurl) > 0 && npos < strlen(szurl) && strncmp((szurl + npos), "/", 1))
        ++npos;

    unsigned long nf = nsp;

    for (;nf <= npos;nf++)
    {
        if (!strncmp((szurl + nf), ":", 1))
        { // find PORT
            bflag = true;
            break;
        }
    }

    if (bflag)
    {
        char sztmp[1024] = "\0";
	strncpy(sztmp, (szurl + nf + 1), npos - nf);	
        *nport = (unsigned long)atol(sztmp);
        strncpy(szaddress, (szurl + nsp), nf - nsp);
    }
    else if (!strncmp(szprotocol, "https", strlen("https")))
    {
        *nport = (unsigned long)443;
        strncpy(szaddress, (szurl + nsp), npos - nsp);
    }
    else if (!strncmp(szprotocol, "ftp", strlen("fps")))
    {
        *nport = (unsigned long)23;
        strncpy(szaddress, (szurl + nsp), npos - nsp);
    }
    else
    {
        *nport = (unsigned long)80;
        strncpy(szaddress, (szurl + nsp), npos - nsp);
    }
    if (npos < strlen(szurl))
    { // find URI
        strncpy(szuri, (szurl + npos), strlen(szurl) - npos);
    }
    else
    {
        szuri[0] = '/';
        szuri[1] = '\0';
    }

    return ;
}

int sendn(int socket, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while(nleft > 0) {
		if((nwritten = send(socket, ptr, nleft, MSG_DONTWAIT)) < 0) {
			OV_DEBUG("%s:%d error : %s\n", __FUNCTION__, __LINE__, strerror(errno));
			if (errno == EAGAIN  || errno == EWOULDBLOCK ) {
				usleep(500*1000);
				continue;				
			}
			else{
				if(nleft == n){
					OV_DEBUG("send error!![%d]", nleft);					
					return -1;
				}
				else
					break;				
			}
		}
		else if(nwritten == 0){
			break;
		}

		nleft -= (size_t)nwritten;
		ptr += nwritten;
	}

	return (int)(n-nleft);
}


#define MAX_UNIT_MSG_SIZE	(4096)
#define MAX_NOTI_MSG_CNT	(16) // MAX_NOTIFY_BUFLEN / MAX_UNIT_MSG_SIZE. This is approximative value.
#define MAX_NOTIFY_BUFLEN	(1024*64)
#define MAX_NOTIFY_H_BUFLEN	(1024)

static void notifier_func(void *arg)
{
	char c_addr[128] = {0,}, c_protocol[16] = {0,}, tmp_str_addr[128] = {0, }, uri[128] = {0, }, host_ip[COMMON_SIZE], send_buf[MAX_NOTIFY_BUFLEN] = {0, }, send_buf_h[MAX_NOTIFY_H_BUFLEN] = {0, }, tmp[256], *p;
	notify_param f;
	in_addr_t tmp_addr = 0;
	struct sockaddr_in server_addr;	
	int ss_id=0, client_socket=0, fail_cnt=0, nsent=0, nleft=0, ret=0;
	struct e_msg  emsg[MAX_NOTI_MSG_CNT];
	unsigned int msg_cnt = 0, tmp_port=0, buflen=0, buflen_h=0;

	memset(&f, 0x00, sizeof(notify_param));	
	LOCK_onvif_event_notify_data();
	memcpy(&f, (notify_param *)arg, sizeof(notify_param));
	memset(arg, 0x00, sizeof(notify_param));
	UNLOCK_onvif_event_notify_data();

	memset(c_addr, 0x00, sizeof(char)*128);
	memset(c_protocol, 0x00, sizeof(char)*16);    
	strncpy(c_addr, f.addr, sizeof(char)*128 - 1);	

	/*WARNNING: c_addr must have port number ex) 192.168.10.11:8080 */
	memset(tmp_str_addr, 0x00, sizeof(char)*128);
	memset(uri, 0x00, sizeof(char)*128);
	__w3curlparse(c_addr, c_protocol, NULL, NULL, tmp_str_addr, (unsigned long *)&tmp_port, uri);
	tmp_addr = inet_addr(tmp_str_addr);
	ss_id = f.id;

	if (!ss_id) {
		OV_DEBUG("ss_id failed..");
		return;
	}
	if ( !tmp_port ){
		tmp_port = 8080;
	}
	if ( !tmp_addr ){
		tmp_addr = inet_addr("192.168.10.151");
	}

	memset(host_ip, 0x00, sizeof(char)*COMMON_SIZE);
	if (get_ipaddress(host_ip) < 0) {
		OV_DEBUG("get ip address  failed..");
		return;
	}

	/* Connect consumer server */
	client_socket = socket( PF_INET, SOCK_STREAM, 0);
	if ( -1 == client_socket) {
		OV_DEBUG( "create socket failed\n");
		return;
	}

	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((uint16_t )tmp_port);
	server_addr.sin_addr.s_addr = tmp_addr;

	/* wait for listening */
	sleep(3);

	while ( connect( client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr) ) == -1 ) {
		if (fail_cnt <= 10)	{
			sleep(1);
			fail_cnt++;
		}
		else {
			OV_DEBUG("connection[%s:%d] failed..", tmp_str_addr, tmp_port);
			return;
		}
	}

	memset(emsg, 0x00, sizeof(struct e_msg)*MAX_NOTI_MSG_CNT);

	while(1){
		ret = OV_BLOCK_event_pull_messages(ss_id, emsg, MAX_NOTI_MSG_CNT, 0 );
		if( ret <= 0){
			break; // timeout ...
		}

		msg_cnt = (unsigned int)ret;

		/* Make a Message */
		memset(send_buf, 0x00, sizeof(char)*MAX_NOTIFY_BUFLEN);
		p = make_buf_head(send_buf);
		p = make_buf_notification_msg(p, emsg, msg_cnt, host_ip, tmp_port, ss_id);
		p = make_buf_tail(p);
		buflen = (unsigned int)(p - send_buf);		

		memset(send_buf_h, 0x00, sizeof(char)*MAX_NOTIFY_H_BUFLEN);
		p = make_http_header(send_buf_h, uri, tmp_str_addr, tmp_port, buflen);
		buflen_h = (unsigned int)(p - send_buf_h);

		ret = sendn( client_socket, send_buf_h, buflen_h);
		if( ret != (int)buflen_h )	break;
		ret = sendn( client_socket, send_buf, buflen);		
		if( ret != (int)buflen )	break;		
	}
	
	close(client_socket);
	return;
}



int OV_event_start_notify(int ss_id, char *dest_address)
{
    pthread_t tid = 0;

    memset(&func_d, 0x00, sizeof(notify_param));
    OV_DEBUG("create bn [%d][%s]", ss_id, dest_address);
    func_d.id = ss_id;
    memset(func_d.addr, 0x00, sizeof(char)*128);
    strncpy(func_d.addr, dest_address, sizeof(char)*128 - 1);

    if ( pthread_create(&tid, NULL, (void*)notifier_func, (void*)&func_d) != 0)
    {
        return 0;
    }

    pthread_detach(tid);
    return 1;
}

/*
1. Description : extend the termination time
2. IN 
 - Subscription ID
 - Termination time : Absolute time or Duration.
 - Is_relative : 0 -Alsolute time, 1 - Duration.
3. OUT 
 - result
*/
int OV_event_renew_subscription(int ss_id, unsigned int termination_time, int is_relative )
{
	struct event_subscription subs[MAX_SUBSCRIPTION_CNT];
	int i = 0, ret=0, now=0;
	
	if ( !ss_id ) {
		return -1;
	}
	LOCK_onvif_event_subs();
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++) {
		if ( g_onvif_event_sub[i].ss_id == ss_id )	{
			now = time(NULL);
			if ( g_onvif_event_sub[i].t_time > now ) {
				if ( is_relative ){
					g_onvif_event_sub[i].t_time = (time_t)MAX( now + (int)termination_time, (int)g_onvif_event_sub[i].t_time);
				}
				else{
					g_onvif_event_sub[i].t_time = (time_t)termination_time;
				}
				ret = (int)g_onvif_event_sub[i].t_time;
				break;
			}
		}
	}
	UNLOCK_onvif_event_subs();

	if ( i == MAX_SUBSCRIPTION_CNT ) {
		return -1;
	}

	return ret;
}

/*
1. Description : Delete subscription
2. IN 
 - Subscription ID
3. OUT 
 - result
*/
int OV_event_unsubscribe(int ss_id)
{
	int	i=0, now=0;
	
	OV_DEBUG( "ssid[%d]", ss_id);
	if ( !ss_id ) {
		return -1;
	}

	LOCK_onvif_event_subs();
	if ( ss_id ) {
		for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++) {
			if ( g_onvif_event_sub[i].ss_id == ss_id )	{
				now = time(NULL);
				if ( g_onvif_event_sub[i].t_time > now ) {
					g_onvif_event_sub[i].t_time = 0;
					g_onvif_event_sub[i].u_time = 0;
					g_onvif_event_sub[i].ss_id = 0;					
					memset(&g_onvif_event_sub[i].tf, 0x00, sizeof(struct e_topic_filter));
					memset(&g_onvif_event_sub[i].mf, 0x00, sizeof(struct e_msg_filter));					
					break;
				}
			}
		}
	}
	UNLOCK_onvif_event_subs();	

	if (i == MAX_SUBSCRIPTION_CNT )	{
		OV_DEBUG( " fail![%d]", ss_id);
		return -1;
	}	
	
	return 1;
}


/*
1. Description : 
2. IN 
 - Subscription ID
3. OUT 
 - result
*/
int OV_event_set_sync(int ss_id)
{
	unsigned int	i=0;
	struct e_motion_type motion;
	struct e_alarm_type alarm;
	NF_NOTIFY_INFO *pnotify = NULL;

	if ( !ss_id ) {
		return -1;
	}

	pnotify = nf_notify_get("motion");
	if(pnotify) {
		for( i=0; i< NUM_ACTIVE_CH; i++){
			motion.ch = i;
			motion.value = (pnotify->d.params[0]>>i)&0x1;
			append_onvif_event_msg(E_KEY_ITX_MOTION, &motion, E_Initialized, ss_id);
			append_onvif_event_msg(E_KEY_MOTION_ALARM, &motion, E_Initialized, ss_id);
		}
		nf_notify_free(pnotify);
	}

	pnotify = nf_notify_get("sensor");
	if(pnotify) {
		#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
		for (i = 0; i < _nf_action_num_alarm; i++)
		#else
		for( i=0; i< NUM_ALARM; i++)
		{
			alarm.ch = i;
			alarm.value = (pnotify->d.params[0]>>i)&0x1;
			append_onvif_event_msg(E_KEY_SENSOR, &alarm, E_Initialized, ss_id);
			append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Initialized, ss_id);			
		}
		nf_notify_free(pnotify);
	}

//ONVIF_NEW_EVENT
#if 0
	pnotify = nf_notify_get("sensor");
	if(pnotify) {
		for( i=0; i< NUM_ALARM; i++){
			alarm.ch = i;
			alarm.value = (pnotify->d.params[0]>>i)&0x1;
			append_onvif_event_msg(E_KEY_SENSOR, &alarm, E_Initialized, ss_id);
		}
		nf_notify_free(pnotify);
	}	
#endif

	return 1;
}

//////////////////////////////////////////////////////////////////

#define SEM_KEY         ((int)0x21) //key for semaphore

struct sembuf sb_lock = {0, -1, 0};
struct sembuf sb_unlock = {0, 1, 0};

void wait_and_lock( int semid )
{
    semop( semid, &sb_lock, 1);
}

void signal_and_unlock( int semid )
{
    semop( semid, &sb_unlock, 1);
}

int pop_rec_info_shm(unsigned int token, recording_info *rec_info)
{
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;

	memset(&tmp_sess, 0x00, sizeof(struct search_session));
	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));
	
	tmp_sess = &ebuf.sess[token];

	/* pop msg */
	if( tmp_sess->rp == tmp_sess->wp ){
		// There are not any messages in buffer...
		signal_and_unlock( semid );
		return -1;
	}
	
	memcpy(rec_info, &tmp_sess->rec_info[tmp_sess->rp], sizeof(recording_info));
	
#if 0
	/* print buf */
	int i;
	OV_DEBUG("=====================");
	for(i=0; i< MAX_MSG_BUF; i++){
		if( ebuf.sess[token].rp == i )
			OV_DEBUG("<");

		OV_DEBUG("[%d] state[%d] search token[%d] ch[%d] s[%lld] e[%lld]",
			i, ebuf.sess[token].search_state, ebuf.sess[token].token, ebuf.sess[token].rec_info[i].ch, ebuf.sess[token].rec_info[i].start_time, ebuf.sess[token].rec_info[i].end_time);
	}
#endif
	tmp_sess->rp++; 

	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;
}

int init_search_rec_shm(unsigned int token)
{
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;

	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));

	tmp_sess = &ebuf.sess[token];

#if 1
	OV_DEBUG("===================== Session[%d] is initialized",tmp_sess->token);
#endif
	memset(tmp_sess, 0x00, sizeof(struct search_session));
	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;

}


int set_search_rec_state_shm(unsigned int token, char search_state)
{
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;

	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));

	tmp_sess = &ebuf.sess[token];

	tmp_sess->search_state = search_state;
#if 1
	OV_DEBUG("===================== Session[%d] is [%d]",tmp_sess->token, tmp_sess->search_state);
#endif


	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;

}

int push_rec_info_shm(unsigned int token, recording_info *rec_info)
{
	int shmid, ret;
	void *mem;
	SEARCH_REC_SHM    ebuf;
	int semid;
	struct search_session	*tmp_sess;
	/*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
	shmid = shmget(SEARCH_REC_SHM_ID , 0, 0);
	if( shmid < 0){
		return -1;
	}

	semid= semget( SEARCH_REC_SEM_ID, 0 ,IPC_CREAT );
	wait_and_lock( semid );

	mem = shmat(shmid, (const void *)0, 0);
	if( mem < 0){
		signal_and_unlock( semid );
		return -1;
	}
	memcpy(&ebuf, mem, sizeof(SEARCH_REC_SHM));

	tmp_sess = &ebuf.sess[token];

	/* check if buff is full */
	tmp_sess->wp++;
	if( tmp_sess->wp >= MAX_MSG_BUF )
		tmp_sess->wp = 0;

#if 1 // OVERWRITE
	if( tmp_sess->wp == tmp_sess->rp ){
		// Overwrite ..
		tmp_sess->rp++;
		if( tmp_sess->rp >= MAX_MSG_BUF )
			tmp_sess->rp = 0;
	}		
#else // RETURN
	if( tmp_sess->wp == tmp_sess->ep ){
		signal_and_unlock( semid );
		return -1;
	}		
#endif //]] Jeonghun_0130914_END -- overwirte

	/* append msg */
	memcpy(&tmp_sess->rec_info[tmp_sess->wp], rec_info, sizeof(recording_info));
 
#if 0
	/* print buf */
	int i;
	OV_DEBUG("=====================");
	for(i=0; i< MAX_MSG_BUF; i++){
		if( ebuf.sess[token].wp == i )
			OV_DEBUG(">");

		OV_DEBUG("[%d] state[%d] search token[%d] ch[%d] s[%lld] e[%lld]",
			i, ebuf.sess[token].search_state, ebuf.sess[token].token, ebuf.sess[token].rec_info[i].ch, ebuf.sess[token].rec_info[i].start_time, ebuf.sess[token].rec_info[i].end_time);
	}
#endif


	memcpy(mem, &ebuf, sizeof(SEARCH_REC_SHM));
	ret = shmdt(mem);
	signal_and_unlock( semid );
	if( ret < 0){
		return -1;
	}

	return 0;
}

int event_status_shm_write(int event_id, unsigned int event_status_flag)
{
    int shmid, ret;
    time_t now;
    void *mem;
    EVENT_STATUS_SHM event_status;

    int semid;
    /*Get the shared memory segment using EVENT_SHM_ID  */
    shmid = shmget(EVENT_SHM_ID , 0, 0);
    if( shmid < 0){
        return -1;
    }

    semid= semget( EVENT_SEM_ID, 0 ,IPC_CREAT );
    wait_and_lock( semid );

    mem = shmat(shmid, (const void *)0, 0);
    if( mem < 0){
        signal_and_unlock( semid );
        return -1;
    }

    memcpy(&event_status, mem, sizeof(EVENT_STATUS_SHM));

    now = time(NULL);

	switch(event_id) {
		case OV_PROP_MOTION:
	        event_status.motion_flags = event_status_flag;
		break;
		case OV_PROP_SENSOR:
    	    event_status.alarm_flags = event_status_flag;			
		break;
		case OV_PROP_RECORDING_JOB_STATE_CHANGE:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",__FUNCTION__, __LINE__);	
			event_status.recording_job_state_chage = event_status_flag;
		break;
		case OV_PROP_CONFIGURATION_CHANGE:
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",__FUNCTION__, __LINE__);	
			event_status.recording_configuration_change = event_status_flag;
		break;
		case OV_PROP_DATA_DELETION:
			event_status.recording_data_deletion = event_status_flag;
		break;
		case OV_PROP_RECORDING_AND_TRACK:
			event_status.recording_recording_and_track = event_status_flag;			
		break;
		default:
		break;			
	
	}
	



    memcpy(mem, &event_status, sizeof(EVENT_STATUS_SHM));

    ret = shmdt(mem);
    signal_and_unlock( semid );
    if( ret < 0){
        return -1;
    }

    return 0;
}

int event_msgbuf_shm_append(unsigned int event_id, unsigned int ch, unsigned int flags)
{
    int shmid, ret;
    void *mem;
    EVENT_MSGBUF_SHM    ebuf;
    struct sysinfo info;
    time_t now; 

    int semid;
	
     sysinfo(&info);
     now = time(NULL);

//	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",__FUNCTION__, __LINE__);	
	

    /*Get the shared memory segment using EVENT_MSGBUF_SHM_ID  */
    shmid = shmget(EVENT_MSGBUF_SHM_ID , 0, 0);
    if( shmid < 0){
        return -1;
    }

    semid= semget( EVENT_MSGBUF_SEM_ID, 0 ,IPC_CREAT );
    wait_and_lock( semid );
 
    mem = shmat(shmid, (const void *)0, 0);
    if( mem < 0){
        signal_and_unlock( semid );
        return -1;
    }
    memcpy(&ebuf, mem, sizeof(EVENT_MSGBUF_SHM));

    /* append msg */
    ebuf.start++;
    if( ebuf.start >= MAX_MSG_BUF )
        ebuf.start = 0;

    ebuf.msg_buf[ebuf.start].key = event_id;
    ebuf.msg_buf[ebuf.start].ch = ch;	
    ebuf.msg_buf[ebuf.start].value = flags;
    ebuf.msg_buf[ebuf.start].time_tick = now;
    ebuf.msg_buf[ebuf.start].uptime_tick = info.uptime;

#if 0
    /* print buf */
    int i;
OV_DEBUG("=====================");
    for(i=0; i< MAX_MSG_BUF; i++){
        if( ebuf.start == i )
            OV_DEBUG(">");

        OV_DEBUG("[%d][%d][%d][%d][%ld]",
                i, ebuf.msg_buf[i].key, ebuf.msg_buf[i].value, ebuf.msg_buf[i].time_tick, ebuf.msg_buf[i].uptime_tick);
    }
#endif

    memcpy(mem, &ebuf, sizeof(EVENT_MSGBUF_SHM));
    ret = shmdt(mem);
    signal_and_unlock( semid );
    if( ret < 0){
        return -1;
    }

    return 0;
}

#if 0
static NOTIFY_INFO _g_notify_info[] =
{
	{PROP_0					,"null"				,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_SENSOR			,"sensor"			,0	,NF_NOTIFY_PARAM, NULL , NULL},	// 
	{PROP_MOTION			,"motion"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_USER_ALARM		,"user_alarm"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_VLOSS				,"vloss"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_ALARM				,"alarm"			,0	,NF_NOTIFY_PARAM, NULL , NULL},		
	{PROP_LOG				,"log"				,0	,NF_NOTIFY_POINTER, NULL , NULL},	

	{PROP_ANALOG_REC		,"analog_rec"		,0	,NF_NOTIFY_CHMAP, NULL , NULL},	
	{PROP_IPCAM_REC			,"ipcam_rec"		,0	,NF_NOTIFY_CHMAP, NULL , NULL},	

	{PROP_ENC_STATUS		,"enc_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_STATUS		,"net_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_RXTX			,"net_rxtx"			,0	,NF_NOTIFY_PARAM, NULL , NULL},				
	
	{PROP_FS_STATUS			,"fs_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_FULL			,"disk_full"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_OVERWR		,"disk_overwr"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_DISK_USAGE		,"disk_usage"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_SMART		,"disk_smart"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_SYSDB_CAM_TITLE	,"sysdb_cam_title"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_COVERT		,"sysdb_covert"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_PTZ			,"sysdb_ptz"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_TIME_FORMAT	,"sysdb_tformat"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYSDB_TIME_ZONE	,"sysdb_tzone"		,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_GUI_SETUP			,"gui_setup"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_SYSDB_CHANGE		,"sysdb_change"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_TIME_CHANGE		,"time_change"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	

	{PROP_DVR_STATUS		,"dvr_status"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_DVR_WDTIMER		,"dvr_wdtimer"		,0	,NF_NOTIFY_PARAM, NULL , NULL},

#ifdef ENABLE_PND_PROP
	{PROP_PND_EVENT			,"pnd_event"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_PND_PROGRESS		,"pnd_progress"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
#endif

	{PROP_PND_HUB_STATUS	,"pnd_hub_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
#ifdef ENABLE_IPCAM_PROP
	{PROP_IPCAM_SENSOR		,"ipcam_sensor"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_MOTION		,"ipcam_motion"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_USER_ALARM	,"ipcam_user_alarm"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_VLOSS		,"ipcam_vloss"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_IPCAM_ALARM		,"ipcam_alarm"		,0	,NF_NOTIFY_PARAM, NULL , NULL},	
	{PROP_IPCAM_ENC_STATUS	,"ipcam_enc_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
#endif

	{PROP_DISK_WRITE_FAIL	,"disk_write_fail"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_EXHAUST		,"disk_exhaust"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_NODISK		,"disk_nodisk"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_DISK_SMART_REQCHK	,"disk_smart_reqchk",0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_SYS_FAN			,"sys_fan"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_TEMPERATURE	,"sys_temperature"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_POE_STATUS	,"sys_poe_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_POE_STATUS_HUB,"sys_poe_status_hub",0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_DVR_LOGIN_FAIL	,"dvr_login_fail"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_LOGIN_FAIL	,"net_login_fail"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_NET_WAN_STATUS	,"net_wan_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_NET_DDNS_STATUS	,"net_ddns_status"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_MOTION_RAW_DATA	,"mraw_data"		,0	,NF_NOTIFY_POINTER, NULL , NULL},	

	{PROP_SYS_BOOTING		,"sys_booting"		,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_BUZZER			,"buzzer"			,0	,NF_NOTIFY_PARAM, NULL , NULL},
	
	{PROP_LOGIN_USER        ,"login_user"		,0	,NF_NOTIFY_CHMAP, NULL , NULL},
	{PROP_NET_IP_CHANGED    ,"net_ip_changed"	,0	,NF_NOTIFY_PARAM, NULL , NULL},
	{PROP_SYS_POE_PORT      ,"sys_poe_port"	    ,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{PROP_SYSDB_TMP_CHANGE	,"sysdb_tmp_change"	 ,0	,NF_NOTIFY_PARAM, NULL , NULL},

	{LAST_PROP				,"null"				,0	,NF_NOTIFY_PARAM, NULL , NULL}
};
#endif

static void
_add_onvif_event_msg_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static guint old_val[LAST_PROP] = {0,};
	gint ch=0;

	g_return_if_fail(pinfo != NULL);
	
	guint prop_id = (guint)data;
	guint new_val, m_new, m_old;
	guint *curr_val, *rise_val;
	gint i;

//	gint div_count = _nf_action->email_state.div_count;

	g_assert( prop_id < LAST_PROP );

#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] 
		|| _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB_SIMPLE] )
		g_message("%s prop_id[%d] [%ld.%06ld] type[%d] [0x%02x]", __FUNCTION__,
					prop_id,
					pinfo->timestamp.tv_sec, pinfo->timestamp.tv_usec,
					pinfo->type, pinfo->d.params[0] );
#endif
	
	new_val	= pinfo->d.params[0];
	
	if( old_val[prop_id] == new_val )
	{
#ifdef DEBUG_ACTION_LOG
		if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
			g_message("%s  same value, skip old[0x%08x] new[0x%08x]",
					__FUNCTION__, old_val[prop_id] , new_val);
#endif
		return;
	}

	if(prop_id == PROP_MOTION ){
//		event_status_shm_write(OV_PROP_MOTION, new_val);		 //Search ME:
		ch=NUM_ACTIVE_CH;
	}else if(prop_id == PROP_SENSOR){
//		event_status_shm_write(OV_PROP_SENSOR, new_val);
		#if defined(ENABLE_SENSOR_IPCAM)
			#if defined(ENABLE_CLASSIFY_BOARD_TYPE)
				ch = _nf_action_num_alarm;
			#else
				ch=NUM_ALARM;
			#endif
		#else
			ch=NUM_ACTIVE_CH;
		#endif
	}else if(prop_id == PROP_VLOSS ){
		ch=NUM_ACTIVE_CH;
	#if defined(ENABLE_EVENT_TAMPER)
		}else if(prop_id == PROP_TAMPER ){
			ch=NUM_ACTIVE_CH;
	#endif
	}else{
		g_message("%s prop_id[%d] error", __FUNCTION__, prop_id);
		return;
	}

	for(i=0; i<ch; i++)
	{
		m_new = (new_val >> i) & 1;
		m_old = (old_val[prop_id] >> i) & 1;

		if( m_new != m_old )
		{
#ifdef DEBUG_ACTION_LOG
			if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
				g_message("%s prop_id[%d] ch[%d] [%d]-->[%d]",  __FUNCTION__,
								prop_id, i,  m_old, m_new);
#endif
			struct e_motion_type motion;
			struct e_alarm_type alarm;
			
			if(m_new){								
				if(prop_id == PROP_MOTION ){
					/* add General Motion event */
					motion.ch = i;
					motion.value = 1;
					append_onvif_event_msg(E_KEY_MOTION_ALARM, &motion, E_Changed, 0);
					append_onvif_event_msg(E_KEY_ITX_MOTION, &motion, E_Changed, 0);
					_LOG_PTS("Motion ON ch[%d]", i);
				}
				else if(prop_id == PROP_SENSOR){
					alarm.ch = i;
					alarm.value = 1;					
					append_onvif_event_msg(E_KEY_SENSOR, &alarm, E_Changed, 0);
					append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Changed, 0);
					_LOG_PTS("Alarm In ON ch[%d]", i);
				}
				OV_DEBUG("event msg append [%d][%d] on", prop_id, i);
			}else{
				if(prop_id == PROP_MOTION ){
					/* add General Motion event */
					motion.ch = i;
					motion.value = 0;
					append_onvif_event_msg(E_KEY_MOTION_ALARM, &motion, E_Changed, 0);
					append_onvif_event_msg(E_KEY_ITX_MOTION, &motion, E_Changed, 0);
					_LOG_PTS("Motion OFF ch[%d]", i);
				}
				else if(prop_id == PROP_SENSOR ){
					alarm.ch = i;
					alarm.value = 0;					
					append_onvif_event_msg(E_KEY_SENSOR, &alarm, E_Changed, 0);
					append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Changed, 0);
					_LOG_PTS("Alarm In OFF ch[%d]", i);
				}			
				OV_DEBUG("event msg append [%d][%d] off", prop_id, i);				
			}
		}
	}


		
#ifdef DEBUG_ACTION_LOG
	if( _DEBUG_ACTION_log[DEBUG_ACTION_IDX_CB] )
		g_message("%s curr[0x%08x] rise[0x%08x]",  __FUNCTION__, *curr_val, *rise_val);
#endif
	old_val[prop_id] = new_val;
	
}


void search_rec_shm_init()
{
    int shmid, page_size, shm_size, i=2;
    int semid;

    page_size = getpagesize();
    /*Semaphore init */
    semid= semget( SEARCH_REC_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    while( page_size < sizeof(SEARCH_REC_SHM)){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    //printf(">>>>>>>>>>>>>>> shm_size[%d]\n", shm_size);

    /*Create the shared memory sement using EVENT_SHM_ID */
    shmid = shmget( SEARCH_REC_SHM_ID, shm_size, 0666 | IPC_CREAT );

    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    return;
}



void event_status_shm_init()
{
    int shmid, page_size, shm_size, i=2;
    int semid;

    page_size = getpagesize();
    /*Semaphore init */
    semid= semget( EVENT_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    while( page_size < sizeof(EVENT_STATUS_SHM)){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    //printf(">>>>>>>>>>>>>>> shm_size[%d]\n", shm_size);

    /*Create the shared memory sement using EVENT_SHM_ID */
    shmid = shmget( EVENT_SHM_ID, shm_size, 0666 | IPC_CREAT );

    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    return;
}

void event_status_msgbuf_shm_init()
{
    int shmid, page_size, shm_size, i=2, ret;
    void *mem;
    int semid;

    /*Semaphore init */
    semid= semget( EVENT_MSGBUF_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    page_size = getpagesize();
    while( page_size < sizeof(EVENT_MSGBUF_SHM)){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    wait_and_lock( semid );
    /*Create the shared memory sement using EVENT_MSGBUF_SHM_ID */
    shmid = shmget(EVENT_MSGBUF_SHM_ID, shm_size, 0666 | IPC_CREAT );

    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    mem = shmat(shmid, (const void *)0, 0);
    if( mem < 0){
        signal_and_unlock( semid );
        return;
    }
    memset(mem, 0, sizeof(EVENT_MSGBUF_SHM));
    ret = shmdt(mem);
    signal_and_unlock( semid );

    if( ret < 0){
        return;
    }

    return;
}


void event_proc_shm_init()
{
    int shmid, page_size, shm_size, i=2;
    int semid;

    /*Semaphore init */
    semid= semget( EVENT_BN_PROC_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);
    semid= semget( EVENT_PULL_PROC_SEM_ID, 1 ,IPC_CREAT );
    semctl(semid, 0, SETVAL, 1);

    page_size = getpagesize();
    while( page_size < sizeof(struct event_proc) * MAX_SUBSCRIPTION_CNT ){
        page_size *= i;
        i++;
    }
    shm_size = page_size;

    //printf(">>>>>>>>>>>>>>> shm_size[%d]\n", shm_size);

    /*Create the shared memory sement using EVENT_BN_PROC_SHM_ID */
    shmid = shmget( EVENT_BN_PROC_SHM_ID, shm_size, 0666 | IPC_CREAT );
    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    shmid = shmget( EVENT_PULL_PROC_SHM_ID, shm_size, 0666 | IPC_CREAT );
    if( shmid >= 0 ){
        printf("Created a shared memory segment %d\n", shmid);
    }

    return;
}

void event_shm_init(void)
{
	int semid = 0;
	
    //create a semaphore for process synchronization
    semid= semget( SEM_KEY,1,IPC_CREAT );
    //set_sem_value(semid,0,1); //set initial value of semaphore
    semctl(semid, 0, SETVAL,1);

    sb_lock.sem_num = 0;
    sb_lock.sem_op = -1;
    sb_lock.sem_flg = 0;
    sb_unlock.sem_num = 0;
    sb_unlock.sem_op = 1;
    sb_unlock.sem_flg = 0;
	
//memset(&g_event_status, 0, sizeof(EVENT_STATUS_SHM));
	event_status_shm_init();
	event_status_msgbuf_shm_init();
	event_proc_shm_init();
	search_rec_shm_init();

    return;
}

void	onvif_event_init(void) //Search ME:
{
	gulong cb_handle = 0;

	onvif_event_mem_init();
	
	// To be deprecated....
	//	event_shm_init();
	
	cb_handle= nf_notify_connect_cb( "sensor", _add_onvif_event_msg_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "motion", _add_onvif_event_msg_cb_func , (gpointer)PROP_MOTION );
	g_message("%s motion connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
	
#if 0
	cb_handle= nf_notify_connect_cb( "sensor", _add_onvif_event_msg_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_ARI_PANIC)
	cb_handle= nf_notify_connect_cb( "sensor", _add_onvif_event_msg_cb_func , (gpointer)PROP_SENSOR );
	g_message("%s sensor connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif	
	
	cb_handle= nf_notify_connect_cb( "motion", _add_onvif_event_msg_cb_func , (gpointer)PROP_MOTION );
	g_message("%s motion connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "vloss", _add_onvif_event_msg_cb_func , (gpointer)PROP_VLOSS);
	g_message("%s vloss connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	#if defined(ENABLE_EVENT_TAMPER)
		cb_handle= nf_notify_connect_cb( "tamper", _add_onvif_event_msg_cb_func , (gpointer)PROP_TAMPER);
		g_message("%s tamper connect_cb[%ld]",__FUNCTION__, cb_handle );
		g_assert(cb_handle >0);
	#endif

	cb_handle= nf_notify_connect_cb( "sysdb_change", _add_onvif_event_msg_cb_func , (gpointer)PROP_SETUP_CHG);
	g_message("%s sysdb change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	// disk event
	cb_handle= nf_notify_connect_cb( "disk_overwr", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_OVERWR);
	g_message("%s disk overwr connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_smart", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_SMART);
	g_message("%s disk nodisk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_nodisk", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_NODISK);
	g_message("%s disk nodisk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_write_fail", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_WRFAIL);
	g_message("%s disk exhaust connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_exhaust", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_EXHAUST);
	g_message("%s disk exhaust connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_smart_reqchk", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_SMART_REQCHK);
	g_message("%s disk smart_reqchk connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
	
	cb_handle= nf_notify_connect_cb( "disk_full", _add_onvif_event_msg_cb_func, (gpointer)PROP_DISK_FULL);
	g_message("%s disk disk_full connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "disk_usage", _add_onvif_event_msg_cb_func, NULL);
	g_message("%s disk usage connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	// system event
	cb_handle= nf_notify_connect_cb( "sys_booting", _add_onvif_event_msg_cb_func , (gpointer)PROP_SYS_BOOTING);
	g_message("%s system sys_booting connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "dvr_login_fail", _add_onvif_event_msg_cb_func , (gpointer)PROP_SYS_LOGON_FAIL);
	g_message("%s system dvr_login_fail connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_fan", _add_onvif_event_msg_cb_func , (gpointer)PROP_SYS_FAN);
	g_message("%s system sys_fan connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_temperature", _add_onvif_event_msg_cb_func , (gpointer)PROP_SYS_TEMPERATURE);
	g_message("%s system sys_temperature connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_POE_CHECK)
	cb_handle= nf_notify_connect_cb( "sys_poe_status", _add_onvif_event_msg_cb_func , (gpointer)PROP_SYS_POE);
	g_message("%s system sys_poe_status connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_poe_status_hub", _add_onvif_event_msg_cb_func , (gpointer)PROP_SYS_POE_HUB);
	g_message("%s system sys_poe_status_hub connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

	// network event
	cb_handle= nf_notify_connect_cb( "net_login_fail", _add_onvif_event_msg_cb_func ,(gpointer)PROP_NET_LOGIN_FAIL);
	g_message("%s net net_login_fail connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "net_ddns_status", _add_onvif_event_msg_cb_func ,(gpointer)PROP_NET_DDNS_FAIL);
	g_message("%s net net_ddns_status connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "net_wan_status", _add_onvif_event_msg_cb_func ,(gpointer)PROP_NET_TROUBLE);
	g_message("%s net net_wan_status connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	//register sysdb reload callbock function
	cb_handle= nf_notify_connect_cb( "sysdb_change", _add_onvif_event_msg_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "time_change", _add_onvif_event_msg_cb_func, NULL);
	g_message("%s reload time_change connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);


	cb_handle= nf_notify_connect_cb( "analog_rec", _add_onvif_event_msg_cb_func , (gpointer)0);
	g_message("%s analog_rec connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

#if defined(ENABLE_FAN_FAIL_CHECK)
	cb_handle= nf_notify_connect_cb( "sys_fan", _add_onvif_event_msg_cb_func , NULL);
	g_message("%s sys_fan connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

#if defined(USE_DEV_GENNUM)
	cb_handle= nf_notify_connect_cb( "std_type", _add_onvif_event_msg_cb_func , (gpointer)0);
	g_message("%s std_type connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
#endif

#endif


}


///////////////// MESSAGE FILTER ///////////////////
char* next_find_char(char *p, const char compare, char *p_end)
{
	if (*p == '\0'){
		return 0;
	}
	
	if ( p_end - p <= 0){
		return 0;		
	}
	
	while ( *p != compare ) {
		if (*p == '\0' || p == p_end)
			return 0;
		p++;
	}
	return p + 1;
}

char* next_find_str(char *p, const char *compare, int size, char *p_end)
{
	if (*p == '\0'){
		return 0;
	}
	
	if ( p_end - p <= 0){
		return 0;		
	}	
	while ( strncmp(p, compare, size) ) {
		if (*p == '\0' || p == p_end)
			return 0;
		p++;
	}
	return p + size;
}

char* next_match_char(char *p, const char compare)
{
	if (*p == '\0') {
		return 0;
	}
	
	while ( *p != compare ) {
		if ( *p == ' ' || *p == '\n' || *p == '\r'){
			p++;
		}
		else{
			return NULL;
		}
	}
	return p + 1;
}

char* next_match_str(char *p, const char *compare, int size)
{
	if (*p == '\0'){
		return 0;
	}
	while ( strncmp(p, compare, size) )
	{
		if ( *p == ' ' || *p == '\n' || *p == '\r'){
			p++;
		}
		else{
			return NULL;
		}
	}
	return p + size;
}

char* find_end_tag(char* p, char *p_end)
{
	int depth = 1; // current pointer
	
	if (*p == '\0'){
		return 0;
	}

	while ( *p != '\0' && depth != 0 && p < p_end) {
		if ( *p == '(') {
			depth++;
		}
		else if ( *p == ')') {
			depth--;
		}
		p++;
	}

	if ( depth != 0 ) {
		return 0;
	}

	return p + 1;
}

int check_match_topic(const char* te, const char * topic)
{
#if 0 //[[ hun_0131119_BEGIN -- comment
CASE 0
FILTER :  tns1:RuleEngine
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

CASE 1
FILTER : tns1:VideoAnalytics/tnsitx:MotionDetection
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

CASE 2
FILTER : tns1:VideoAnalytics//.|tns1:RuleEngine/MotionDetection
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

CASE 3
FILTER : tnsitx:MotionDetection
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

CASE 4
FILTER : tns1:VideoAnalytics/oth:others
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

CASE 5
FILTER : tns1:VideoAnalytics/oth:others
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

CASE 6
FILTER : tns1:VideoTest/oth:others
TOPIC : tns1:VideoAnalytics/tnsitx:MotionDetection

...

#endif //]] hun_0131119_END -- comment

	char *p_filter=NULL, *p_filter_sub=NULL, *p_filter_end= NULL, *p_topic =NULL, *p_tmp =NULL, str_pass=0;

	if (!topic) {
		return ONVIF_FILTERED;
	}

	p_filter = te;

	while ( p_filter <  te + sizeof(char)*MAX_FILTER_DATA_SIZE ){
		sleep(1);
		//OV_DEBUG("p_filter[%s]", p_filter);		
		p_tmp = next_find_char(p_filter, '|', te + sizeof(char)*MAX_FILTER_DATA_SIZE);
		if( p_tmp ){
			p_filter_end = p_tmp;
		}
		else{
			p_filter_end = p_filter + sizeof(char)*MAX_FILTER_DATA_SIZE;
		}
		//OV_DEBUG("p_tmp[%x] p_filter[%x] p_filter_end[%x]", p_tmp, p_filter, p_filter_end);
		p_filter = next_find_char(p_filter, ':', p_filter_end);
		if (!p_filter) {
			return ONVIF_FILTERED;
		}

		p_topic = next_find_char(topic, ':', topic + strlen(topic));
		if (!p_topic) {
			return ONVIF_FILTERED;
		}

		str_pass = 0;
		p_filter_sub = p_filter;
		while( p_filter_sub <= p_filter_end ){
			//OV_DEBUG("p_filter_sub[%s] p_topic[%s]", p_filter_sub, p_topic);			
			if( *p_filter_sub == '/' ){
				/* End of Str */
				if( *(p_filter_sub+1) == '/' && *(p_filter_sub+2) == '.' ){ // hun_0131119 : case "//."
					if( (*p_topic == '/' || *p_topic == '\0') && str_pass ){
						return ONVIF_NOT_FILTERED;
					}
					else {
						break;					
					}
				}
				else{									// case "/"
					if( *p_topic == '/' && str_pass ){
						str_pass = 0;
						
						p_tmp = next_find_char(p_filter_sub, ':', topic + strlen(topic));
						if( p_tmp ){
							p_filter_sub = p_tmp;
						}
						else{
							p_filter_sub++;						
						}
						
						p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
						if( !p_tmp ){
							p_topic = p_tmp;
						}
						else{
							p_topic++;
						}
						continue;
					}
					else{
						break;
					}
				}
			}
			else if( *p_filter_sub == '\0' || *p_filter_sub == '\n' || *p_filter_sub == '\r' || *p_filter_sub == '|'){
				/* End of Str */			
				if( *p_topic == '\0' && str_pass ){
					return ONVIF_NOT_FILTERED;
				}
				else{
					break;
				}
			}
			else if( *p_filter_sub == *p_topic ){
				str_pass = 1;
			}
			else {
				break;
			}		
			p_filter_sub++;
			p_topic++;
		}

		//	topic_filter_not_fully_matched but it may be matched partially:  ex) tnsitx:MotionDetection (VS) tns1:VideoAnalytics/tnsitx:MotionDetection
		str_pass = 0;
		//OV_DEBUG("retry p_filter[%s] p_topic[%s]", p_filter, p_topic);
		while( p_topic = next_find_char(p_topic, '/', topic + strlen(topic)) ){
			
			p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
			if (p_tmp) {
				p_topic = p_tmp;
			}
			else {
				p_topic++;
			}

			str_pass = 0;
			p_filter_sub = p_filter;
			while( p_filter_sub <= p_filter_end ){
				//OV_DEBUG("p_filter_sub[%s] p_topic[%s]", p_filter_sub, p_topic);
				if( *p_filter_sub == '/' ){
					/* End of Str */
					if( *(p_filter_sub+1) == '/' && *(p_filter_sub+2) == '.' ){ // hun_0131119 : case "//."
						if( (*p_topic == '/' || *p_topic == '\0') && str_pass ){
							return ONVIF_NOT_FILTERED;
						}
						else {
							break;					
						}
					}
					else{									// case "/"
						if( *p_topic == '/' && str_pass ){
							str_pass = 0;
							
							p_tmp = next_find_char(p_filter_sub, ':', topic + strlen(topic));
							if( p_tmp ){
								p_filter_sub = p_tmp;
							}
							else{
								p_filter_sub++;						
							}
							
							p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
							if( !p_tmp ){
								p_topic = p_tmp;
							}
							else{
								p_topic++;
							}
							continue;
						}
						else{
							break;
						}
					}
				}
				else if( *p_filter_sub == '\0' || *p_filter_sub == '\n' || *p_filter_sub == '\r' || *p_filter_sub == '|'){
					/* End of Str */			
					if( *p_topic == '\0' && str_pass ){
						return ONVIF_NOT_FILTERED;
					}
					else{
						break;
					}
				}
				else if( *p_filter_sub == *p_topic ){
					str_pass = 1;
				}
				else {
					break;
				}		
				p_filter_sub++;
				p_topic++;
			}
		}

		/* Find Next Topic Filter */
		p_tmp = next_find_char(p_filter, '|', te + sizeof(char)*MAX_FILTER_DATA_SIZE); 
		if( p_tmp ){
			p_filter = p_tmp;
		}
		else{
			break;
		}
	}

	return ONVIF_FILTERED;
}

#define MAX_FILTER_CNT  (10)

int apply_topic_ex(char *te, struct event_subscription *sub)
{
	char *p, *p_tmp;
	int i=0;

	for(i=0; i<NUMBER_OF_EVENTS; i++){
		sub->tf.topic_filter[i] = ONVIF_FILTERED;
	}

	sub->tf.topic_filter[E_KEY_SENSOR] 							= check_match_topic(te, str_tns1_tnsitx_Alarm);
	sub->tf.topic_filter[E_KEY_ITX_MOTION]				 		= check_match_topic(te, str_tns1_tnsitx_Motion);
	sub->tf.topic_filter[E_KEY_MOTION_ALARM] 					= check_match_topic(te, str_tns1_General_Motion);
	sub->tf.topic_filter[E_KEY_RECORDING_JOB_STATE_CHANGE]	= check_match_topic(te, str_tns1_tnsitx_jobState_change);
	sub->tf.topic_filter[E_KEY_CONFIGURATION_CHANGE]			= check_match_topic(te, str_tns1_tnsitx_conf_change);
	sub->tf.topic_filter[E_KEY_DATA_DELETION]					= check_match_topic(te, str_tns1_tnsitx_data_deletion);
	sub->tf.topic_filter[E_KEY_RECORDING_AND_TRACK]			= check_match_topic(te, str_tns1_recording_and_track);
	sub->tf.topic_filter[E_KEY_DIGITAL_INPUT]					= check_match_topic(te, str_tns1_digital_input);	
	sub->tf.topic_filter[E_KEY_RELAY]							= check_match_topic(te, str_tns1_relay);		

#if 0
	OV_DEBUG("===========Topic Filter Result==============");
	for(i=0; i<NUMBER_OF_EVENTS; i++){
		OV_DEBUG_RAW("%d ", sub->tf.topic_filter[i]);
	}
	OV_DEBUG_RAW("\n");
#endif

	return 1;
}



void print_topic_ex_list(char **t, int size)
{
	int i;
	char te[MAX_FILTER_CNT][512];

	memset(te, 0, sizeof(te));
	memcpy(te, t, sizeof(te));

	if (size)
	{
		_TTY_LOG_ONVIF("******** Topic Expression List *********");
		for (i = 0; i < size; i++)
		{
			if ( te[i] && te[i][0] != '\0')
				_TTY_LOG_ONVIF("te[%d][%s]", i, te[i]);
		}
		_TTY_LOG_ONVIF("****************************************");
	}
}

char* get_flags_from_attr_comp(char *str, int *flags, char * str_end)
{
	char *p, *p_tmp, *p_end;
	int ret_flags = 0;

	p = str;

	if( str_end - str <= 0) {
		return 0;
	}

	if ( (p_tmp = next_match_char(p, '@')))
	{
		p = p_tmp;
		if ((p_tmp = next_match_str(p, "Name", 4)))
		{
			p = p_tmp;
			p = next_match_char(p, '=');
			if (!p)
				return NULL;
			if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
			{
				p = p_tmp;
			}
			else
				return NULL;
			if ((p_tmp = next_match_str(p, "VideoSourceConfigurationToken", 29)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "MotionActive", 12)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "Port", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
				ret_flags |= ONVIF_F_ALARM_OPEN;
			}
			else if ((p_tmp = next_match_str(p, "State", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
				ret_flags |= ONVIF_F_ALARM_OPEN;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "Source", 6)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}

			else
				return NULL;
		}
		else if ((p_tmp = next_match_str(p, "Value", 5)))
		{
			p = p_tmp;
			p = next_match_char(p, '=');
			if (!p)
				return NULL;
			if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
			{
				p = p_tmp;
			}
			else
				return NULL;
			if ((p_tmp = next_match_str(p, "general_vsource", 15)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "true", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_ON;
			}
			else if ((p_tmp = next_match_str(p, "false", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_OFF;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "close", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
			}
			else if ( (p_tmp = next_match_str(p, "open", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_OPEN;
			}
			else
				return NULL;

		}
		else
			return NULL;

		if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
		{
			p = p_tmp;
		}
		else
			return NULL;

		*flags = ret_flags; // return filter flags
	}
	else if ( (p_tmp = next_match_char(p, '(')))
	{
		p = p_tmp;
		p_end = find_end_tag(p, str_end);
		if (p_end == NULL )
			return 0;		
		if ( (p = get_flags_from_attr_comp( p, &ret_flags, p_end)) )
		{
			p = next_match_char(p, ')');
			if (!p){
				free(p_tmp);
				return NULL;
			}			
		}
		else{
			free(p_tmp);
			return NULL;
		}
		free(p_tmp);		

		*flags = ret_flags; // return filter flags
	}
	else if ( (p_tmp = next_match_str(p, "not", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(p, &ret_flags, str_end)) )
		{
			*flags = ~ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	if ((p_tmp = next_match_str(p, "and", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(p, &ret_flags, str_end)) )
		{
			*flags &= ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	if ((p_tmp = next_match_str(p, "or", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(p, &ret_flags, str_end)) )
		{
			*flags |= ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	return p;
}

int get_flags_from_node(char *str,  char *str_end)
{
	char *p, *p_tmp, *p_end;
	int ret_flags = 0, i;

	if( str_end - str <= 0) {
		return 0;
	}

	p = next_match_str(str, "//", 2);
	if (!p)
		return 0;	

	if ((p = next_find_str(str, "SimpleItem", 10, str_end)))
	{
		p = next_match_char(p, '[');
		if (!p)
			return 0;
		p_end = next_find_char(p, ']', str_end);
		if (!p_end)
			return 0;

		if ( (p = get_flags_from_attr_comp( p, &ret_flags, p_end)))
		{
			;
		}
		else{
			free(p_tmp);
			return 0;
		}
		free(p_tmp);
	}
	else
		return 0;

	return ret_flags;
}

int apply_contents_ex(MSG_FilterExpr* p_expr)
{
	int tmp_flags = 0;
	char *ex_end = 0;

	if (p_expr != NULL)
	{
		if (p_expr->ex_boolean != NULL)
		{
			if (p_expr->ex_boolean->ex_path != NULL)
			{
				if (p_expr->ex_boolean->ex_path->ex_path_str != NULL && p_expr->ex_boolean->ex_path->ex_path_str[0] != '\0')
				{
					if ( p_expr->_not )
					{
						ex_end = p_expr->ex_boolean->ex_path->ex_path_str+ p_expr->ex_boolean->ex_path->_size;
						tmp_flags = ~get_flags_from_node(p_expr->ex_boolean->ex_path->ex_path_str, ex_end);
					}
					else
					{
						ex_end = p_expr->ex_boolean->ex_path->ex_path_str+ p_expr->ex_boolean->ex_path->_size;
						tmp_flags = get_flags_from_node(p_expr->ex_boolean->ex_path->ex_path_str, ex_end);
					}
				}
			}
		}
		if ( p_expr->p_child != NULL )
		{
			tmp_flags = apply_contents_ex(p_expr->p_child);
		}
		if ( p_expr->p_next_and != NULL )
		{
			tmp_flags &= apply_contents_ex(p_expr->p_next_and);
		}
		if ( p_expr->p_next_or != NULL )
		{
			tmp_flags |= apply_contents_ex(p_expr->p_next_and);
		}
	}

	return tmp_flags;
}

void print_contents_list(MSG_FilterExpr* p_expr)
{
	if (p_expr != NULL)
	{
		if ( p_expr->_not )
			_TTY_LOG_ONVIF("(not)");
		if (p_expr->ex_boolean != NULL)
		{
			if (p_expr->ex_boolean->ex_path != NULL)
			{
				if (p_expr->ex_boolean->ex_path->ex_path_str != NULL
				        && p_expr->ex_boolean->ex_path->ex_path_str[0] != '\0')
				{
					_TTY_LOG_ONVIF("%s", p_expr->ex_boolean->ex_path->ex_path_str);
				}
			}
		}
	}
	if ( p_expr->p_child != NULL )
	{
		_TTY_LOG_ONVIF(">>>>");
		print_contents_list(p_expr->p_child);
		_TTY_LOG_ONVIF("<<<<");
	}
	if ( p_expr->p_next_and != NULL )
	{
		_TTY_LOG_ONVIF("and");
		print_contents_list(p_expr->p_next_and);
	}
	if ( p_expr->p_next_or != NULL )
	{
		_TTY_LOG_ONVIF("or");
		print_contents_list(p_expr->p_next_or);
	}
}

MSG_FilterExpr* get_content_expr(const char *str, char * str_end)
{
	char *p, *p_end, *p_tmp;
	MSG_FilterExpr *msg_filter;
	int ret = 0;

	if ( *str == '\0' )
		return 0;

	if( str_end - str <= 0) {
		return 0;
	}
	
	msg_filter = (MSG_FilterExpr *)malloc(sizeof(MSG_FilterExpr));
	if( !msg_filter ){
		return 0;
	}	
	memset(msg_filter, 0, sizeof(MSG_FilterExpr));

	p = (char *)str;

	if ( *p != '\0' )
	{
		p_tmp = next_match_str(p, "boolean", 7);
		if (p_tmp != NULL)
		{
			p = p_tmp;
			msg_filter->ex_boolean = (BoolExpr *)malloc(sizeof(BoolExpr));
			memset(msg_filter->ex_boolean, 0, sizeof(BoolExpr));
			p = next_match_char(p, '(');
			if ( p == NULL )
				return 0;
			else
			{
				msg_filter->ex_boolean->ex_path = (PathExpr *)malloc(sizeof(PathExpr));
				memset(msg_filter->ex_boolean->ex_path, 0, sizeof(PathExpr));
				p_end = next_find_char(p, ')', str_end);
				if (p_end == NULL)
					return 0;
				else
				{
					msg_filter->ex_boolean->ex_path->_size = sizeof(char) * (p_end - p);
					msg_filter->ex_boolean->ex_path->ex_path_str = (char *)malloc( msg_filter->ex_boolean->ex_path->_size);
					memset(msg_filter->ex_boolean->ex_path->ex_path_str, 0, msg_filter->ex_boolean->ex_path->_size);
					memcpy(msg_filter->ex_boolean->ex_path->ex_path_str, p, msg_filter->ex_boolean->ex_path->_size-1);
					p = p_end;
					p_tmp = next_match_str(p, "and", 3);
					if (p_tmp != NULL)
					{
						p = p_tmp;
						msg_filter->p_next_and = get_content_expr(p, str_end);
						if (msg_filter->p_next_and == NULL)
							return 0;
						else
						{
							msg_filter->p_next_and->p_prev = msg_filter;
							return msg_filter;
						}
					}
					else
					{
						p_tmp = next_match_str(p, "or", 2);
						if (p_tmp != NULL)
						{
							p = p_tmp;
							msg_filter->p_next_or = get_content_expr(p, str_end);
							if (msg_filter->p_next_or == NULL)
								return 0;
							else
							{
								msg_filter->p_next_or->p_prev = msg_filter;
								return msg_filter;
							}
						}
						else
							return msg_filter;
					}

				}
			}
		}
		else
		{
			p_tmp = next_match_str(p, "not", 3);
			if ( p_tmp != NULL)
			{
				p = p_tmp;
				msg_filter->_not = 1;
				p = next_match_char(p, '(');
				if ( p == NULL )
					return 0;
				else
				{
					p = next_match_str(p, "boolean", 7);
					if ( p == NULL )
						return 0;
					else
					{
						msg_filter->ex_boolean = (BoolExpr *)malloc(sizeof(BoolExpr));
						memset(msg_filter->ex_boolean, 0, sizeof(BoolExpr));
						p = next_match_char(p, '(');
						if ( p == NULL )
							return 0;
						else
						{
							msg_filter->ex_boolean->ex_path = (PathExpr *)malloc( sizeof(PathExpr));
							memset(msg_filter->ex_boolean->ex_path, 0, sizeof(PathExpr));
							p_end = next_find_char(p, ')', str_end);
							if (p_end == NULL)
								return 0;
							else
							{
								msg_filter->ex_boolean->ex_path->_size = sizeof(char) * (p_end - p);
								msg_filter->ex_boolean->ex_path->ex_path_str = (char *)malloc(msg_filter->ex_boolean->ex_path->_size );
								memset(msg_filter->ex_boolean->ex_path->ex_path_str, 0, msg_filter->ex_boolean->ex_path->_size);
								memcpy(msg_filter->ex_boolean->ex_path->ex_path_str, p, msg_filter->ex_boolean->ex_path->_size-1);
								p = p_end;
								p_tmp = next_match_str(p, "and", 3);
								if (p_tmp != NULL)
								{
									p = p_tmp;
									msg_filter->p_next_and = get_content_expr(p, str_end);
									if (msg_filter->p_next_and == NULL)
										return 0;
									else
									{
										msg_filter->p_next_and->p_prev = msg_filter;
										return msg_filter;
									}
								}
								else
								{
									p_tmp = next_match_str(p, "or", 2);
									if (p_tmp != NULL)
									{
										p = p_tmp;
										msg_filter->p_next_or = get_content_expr( p, str_end);
										if (msg_filter->p_next_or == NULL)
											return 0;
										else
										{
											msg_filter->p_next_or->p_prev = msg_filter;
											return msg_filter;
										}
									}
									else
										return msg_filter;
								}
							}
						}
					}
				}
			}
			else
			{
				p_tmp = next_match_char(p, '(');
				if ( p_tmp != NULL)
				{
					p = p_tmp;
					p_end = find_end_tag(p, str_end);
					if (p_end == NULL )
						return 0;

					msg_filter->p_child = get_content_expr(p, p_end);
					if (msg_filter->p_child == NULL){
						return 0;
					}
					else{
						msg_filter->p_child->p_parent = msg_filter;
					}

					p = p_end;
					p_tmp = next_match_str(p, "and", 3);
					if (p_tmp != NULL)
					{
						p = p_tmp;
						msg_filter->p_next_and = get_content_expr(p, str_end);
						if (msg_filter->p_next_and == NULL)
							return 0;
						else
						{
							msg_filter->p_next_and->p_prev = msg_filter;
							return msg_filter;
						}
					}
					else
					{
						p_tmp = next_match_str(p, "or", 2);
						if (p_tmp != NULL)
						{
							p = p_tmp;
							msg_filter->p_next_or = get_content_expr(p, str_end);
							if (msg_filter->p_next_or == NULL)
								return 0;
							else
							{
								msg_filter->p_next_or->p_prev = msg_filter;
								return msg_filter;
							}
						}
						else
							return msg_filter;
					}
				}
				else
					return 0;
			}
		}
	}
	return msg_filter;
}

MSG_FilterExpr* get_msg_contents_filter(const char *str, char *str_end)
{
	/* WARNING : This function use malloc !!! use this with clear_msg_filter() */
	char *p, *p_end, tmp[512];
	MSG_FilterExpr *msg_filter;

	if( str_end - str <= 0) {
		return 0;
	}
	
	msg_filter = get_content_expr(p, str_end);
	if (msg_filter == NULL)
		return NULL;

	return msg_filter;
}


int get_topic_ex_filter(const char *str, char **tmp_topic_ex)
{
	char *p, *p_end, te_str[512], topic_ex[MAX_FILTER_CNT][512];
	int i, size_of_topic_ex = 0;

	memset(te_str, 0, sizeof(te_str));
	memset(topic_ex, 0, sizeof(topic_ex));
	p = strstr(str, ":TopicExpression");

	if (p)
	{
		p = strchr(p, '>');
		p++;
		if (p != NULL && *p != '\0' && *p != '<' )
		{
			p_end = strchr(p, '<');
			if (!p_end)
				return 0;
			if ( (p_end - p) < 512 )
			{
				strncpy(te_str, p, p_end - p);
			}
			else
				return 0;
		}
	}
	else
		return 0;

	p = te_str;
	for (i = 0; i < MAX_FILTER_CNT; i++)
	{
		p_end = strstr(p, "//.");
		if (p_end)
		{
			strncpy(topic_ex[i], p, p_end - p);
		}
		else
		{
			/* There is no "//." tag */
			p_end = strchr(p, '\0');
			if ( (p_end - p) < 512 )
			{
				strncpy(topic_ex[i], p, p_end - p);
				i++;
				break;
			}
			else
				return 0;
		}
		p = strchr(p, '|');
		if (!p)
		{
			i++;
			break;
		}
		else
			p++;
	}

	size_of_topic_ex = i;
	memcpy(tmp_topic_ex, topic_ex, sizeof(topic_ex));

	return size_of_topic_ex;
}

int clear_msg_filter(MSG_FilterExpr* msg_filter)
{
	if( !msg_filter ){
		return 0;
	}

	if(msg_filter->ex_boolean ){
		if(msg_filter->ex_boolean->ex_path ){
			if( msg_filter->ex_boolean->ex_path->ex_path_str ){
				free(msg_filter->ex_boolean->ex_path->ex_path_str);
				msg_filter->ex_boolean->ex_path->ex_path_str = NULL;
			}	
			free(msg_filter->ex_boolean->ex_path);
			msg_filter->ex_boolean->ex_path  = NULL;			
		}
		free(msg_filter->ex_boolean);
		msg_filter->ex_boolean = NULL;
	}
	
	if(msg_filter->p_next_and){
		clear_msg_filter(msg_filter->p_next_and);
		msg_filter->p_next_and = NULL;
	}

	if(msg_filter->p_next_or){
		clear_msg_filter(msg_filter->p_next_or);
		msg_filter->p_next_or = NULL;
	}

	if(msg_filter->p_child){
		clear_msg_filter(msg_filter->p_child);
		msg_filter->p_child = NULL;
	}
}

int make_filter_flags(struct onvif_filters *filters, struct event_subscription *sub )
{
	char *p, *p_end, topic_ex[MAX_FILTER_CNT][512], *str_end;
	int i, ret_flags = 0, size_of_topic_ex = 0;
	MSG_FilterExpr *msg_filter;

	if(filters == NULL || sub == NULL){
		return 0;
	}
	
	if( filters->cnt == 0 ){
		return 0;
	}

	OV_DEBUG("FILTER cnt[%d]", filters->cnt);
	
	for(i=0; i<filters->cnt; i++){
		if( filters->unit[i].type == TOPIC_EXPRESSION_TYPE){
			OV_DEBUG("FILTER [%d] TOPIC_EXPRESSION_TYPE [%s]", i, filters->unit[i].data);			
			apply_topic_ex(filters->unit[i].data, sub);
			
		}
		else if( filters->unit[i].type == MESSAGE_CONTENTS_TYPE ){
			OV_DEBUG("FILTER [%d] MESSAGE_CONTENTS_TYPE [%s]", i, filters->unit[i].data);			
#if 0			
			str_end = filters->unit[i].data + sizeof(char)*MAX_FILTER_DATA_SIZE;
			msg_filter = get_msg_contents_filter(filters->unit[i].data, str_end);
			if ( msg_filter != NULL ) {
				sub->mf = apply_contents_ex(msg_filter);
			}			
			clear_msg_filter(msg_filter);
			msg_filter = NULL;
#endif			
		}
		else{
			OV_DEBUG("FILTER [%d] type[%d] is wrong !!", i, filters->unit[i].type);						
			return -1;
		}
		// apply Message content filter to Topic filter
	}

	return 1;
}




#if 0
char* next_find_char(char *p, const char compare)
{
	if (*p == '\0')
		return 0;
	while ( *p != compare ) {
		if (*p == '\0')
			return 0;
		p++;
	}
	return p + 1;
}

char* next_find_str(char *p, const char *compare, int size)
{
	if (*p == '\0')
		return 0;
	while ( strncmp(p, compare, size) ) {
		if (*p == '\0')
			return 0;
		p++;
	}
	return p + size;
}

char* next_match_char(char *p, const char compare)
{
	if (*p == '\0') {
		return 0;
	}
	
	while ( *p != compare ) {
		if ( *p == ' ' || *p == '\n' || *p == '\r'){
			p++;
		}
		else{
			return NULL;
		}
	}
	return p + 1;
}

char* next_match_str(char *p, const char *compare, int size)
{
	if (*p == '\0'){
		return 0;
	}
	while ( strncmp(p, compare, size) )
	{
		if ( *p == ' ' || *p == '\n' || *p == '\r'){
			p++;
		}
		else{
			return NULL;
		}
	}
	return p + size;
}

char* find_end_tag(char* p)
{
	int depth = 1; // current pointer
	
	if (*p == '\0'){
		return 0;
	}

	while ( *p != '\0' && depth != 0) {
		if ( *p == '(') {
			depth++;
		}
		else if ( *p == ')') {
			depth--;
		}
		p++;
	}

	if ( depth != 0 ) {
		return 0;
	}

	return p + 1;
}

#define MAX_FILTER_CNT  (10)

int apply_topic_ex(struct soap *soap, char **t, int size)
{
	char *p, *p_tmp;
	int ret_flags = 0, i;
	char te[MAX_FILTER_CNT][512];

	memset(te, 0, sizeof(te));
	memcpy(te, t, sizeof(te));

	ret_flags |= ONVIF_F_PROPERTY_OP_INIT;
	ret_flags |= ONVIF_F_PROPERTY_OP_CHANGE;

	if (size)
	{
		for (i = 0; i < size; i++)
		{
			if ( te[i] && te[i][0] != '\0')
			{
				// apply
				p = next_find_char(te[i], ':');
				if (!p)
					return 0;
				else
				{
					if ( (p_tmp = next_match_str(p, "VideoAnalytics", 14)) )
					{
						p = p_tmp;
						if ( (p_tmp = next_match_str(p, "//.", 3)) || *p == '\0' )
						{
							p = p_tmp;
							ret_flags |= ONVIF_F_MOTION;
							ret_flags |= ONVIF_F_MOTION_ON;
							ret_flags |= ONVIF_F_MOTION_OFF;
						}
						else if ( (p_tmp = next_match_char(p, '/')) )
						{
							p = p_tmp;
							p_tmp = next_find_char(p, ':');
							if (p_tmp)
								p = p_tmp;  // There may not be namespace
							if ( (p_tmp = next_find_str(p, "MotionDetection", 15)))
							{
								//To DO
								ret_flags |= ONVIF_F_MOTION;
								ret_flags |= ONVIF_F_MOTION_ON;
								ret_flags |= ONVIF_F_MOTION_OFF;
							}
							else
								return 0;   // As itx rules, It's fail.
						}
						else
							return 0;
					}
					else if ( (p_tmp = next_match_str(p, "UserAlarm", 9)))
					{
						p = p_tmp;
						if ( (p_tmp = next_match_str(p, "//.", 3)) || *p == '\0' )
						{
							ret_flags |= ONVIF_F_ALARM;
							ret_flags |= ONVIF_F_ALARM_CLOSE;
							ret_flags |= ONVIF_F_ALARM_OPEN;
						}
						else if ((p_tmp = next_match_char(p, '/')) )
						{
							p = p_tmp;
							p_tmp = next_find_char(p, ':');
							if (p_tmp)
								p = p_tmp;  // There may not be namespace
							if ( (p_tmp = next_find_str(p, "AlarmIN", 7)))
							{
								//To DO
								ret_flags |= ONVIF_F_ALARM;
								ret_flags |= ONVIF_F_ALARM_CLOSE;
								ret_flags |= ONVIF_F_ALARM_OPEN;
							}
							else
								return 0;   // As itx rules, It's fail.
						}
						else
							return 0;
					}
					else if ( (p_tmp = next_match_str(p, "VideoSource", 11)) )
					{
						p = p_tmp;
						if ( (p_tmp = next_match_str(p, "//.", 3)) || *p == '\0' )
						{
							p = p_tmp;
							ret_flags |= ONVIF_F_G_MOTION;
							ret_flags |= ONVIF_F_G_MOTION_ON;
							ret_flags |= ONVIF_F_G_MOTION_OFF;
						}
						else if ( (p_tmp = next_match_char(p, '/')) )
						{
							p = p_tmp;
							p_tmp = next_find_char(p, ':');
							if (p_tmp)
								p = p_tmp;  // There may not be namespace
							if ( (p_tmp = next_find_str(p, "MotionAlarm", 11)))
							{
								//To DO
								ret_flags |= ONVIF_F_G_MOTION;
								ret_flags |= ONVIF_F_G_MOTION_ON;
								ret_flags |= ONVIF_F_G_MOTION_OFF;
							}
							else
								return 0;   // As itx rules, It's fail.
						}
						else
							return 0;
					}
					else if ( (p_tmp = next_match_str(p, "MotionDetection", 15)))
					{
						p = p_tmp;
						ret_flags |= ONVIF_F_MOTION;
						ret_flags |= ONVIF_F_MOTION_ON;
						ret_flags |= ONVIF_F_MOTION_OFF;
					}
					else if ( (p_tmp = next_match_str(p, "AlarmIN", 7)))
					{
						p = p_tmp;
						ret_flags |= ONVIF_F_ALARM;
						ret_flags |= ONVIF_F_ALARM_CLOSE;
						ret_flags |= ONVIF_F_ALARM_OPEN;
					}
					else if ( (p_tmp = next_match_str(p, "MotionAlarm", 11)))
					{
						p = p_tmp;
						ret_flags |= ONVIF_F_G_MOTION;
						ret_flags |= ONVIF_F_G_MOTION_ON;
						ret_flags |= ONVIF_F_G_MOTION_OFF;
					}
					else
					{
						return 0;
					}
				}
			}
		}
	}
	else
		return 0;

	return ret_flags;
}



void print_topic_ex_list(struct soap *soap, char **t, int size)
{
	int i;
	char te[MAX_FILTER_CNT][512];

	memset(te, 0, sizeof(te));
	memcpy(te, t, sizeof(te));

	if (size)
	{
		_TTY_LOG_ONVIF("******** Topic Expression List *********");
		for (i = 0; i < size; i++)
		{
			if ( te[i] && te[i][0] != '\0')
				_TTY_LOG_ONVIF("te[%d][%s]", i, te[i]);
		}
		_TTY_LOG_ONVIF("****************************************");
	}
}

char* get_flags_from_attr_comp(struct soap *soap, char *str, int *flags)
{
	char *p, *p_tmp, *p_end;
	int ret_flags = 0;

	p = str;

	if ( (p_tmp = next_match_char(p, '@')))
	{
		p = p_tmp;
		if ((p_tmp = next_match_str(p, "Name", 4)))
		{
			p = p_tmp;
			p = next_match_char(p, '=');
			if (!p)
				return NULL;
			if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
			{
				p = p_tmp;
			}
			else
				return NULL;
			if ((p_tmp = next_match_str(p, "VideoSourceConfigurationToken", 29)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "MotionActive", 12)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "Port", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
				ret_flags |= ONVIF_F_ALARM_OPEN;
			}
			else if ((p_tmp = next_match_str(p, "State", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
				ret_flags |= ONVIF_F_ALARM_OPEN;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "Source", 6)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}

			else
				return NULL;
		}
		else if ((p_tmp = next_match_str(p, "Value", 5)))
		{
			p = p_tmp;
			p = next_match_char(p, '=');
			if (!p)
				return NULL;
			if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
			{
				p = p_tmp;
			}
			else
				return NULL;
			if ((p_tmp = next_match_str(p, "general_vsource", 15)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_MOTION_OFF;
				ret_flags |= ONVIF_F_G_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "true", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_ON;
				ret_flags |= ONVIF_F_G_MOTION_ON;
			}
			else if ((p_tmp = next_match_str(p, "false", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_MOTION_OFF;
				ret_flags |= ONVIF_F_G_MOTION_OFF;
			}
			else if ( (p_tmp = next_match_str(p, "close", 5)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_CLOSE;
			}
			else if ( (p_tmp = next_match_str(p, "open", 4)) )
			{
				p = p_tmp;
				ret_flags |= ONVIF_F_ALARM_OPEN;
			}
			else
				return NULL;

		}
		else
			return NULL;

		if ((p_tmp = next_match_char(p, '"')) || (p_tmp = next_match_char(p, 29)))
		{
			p = p_tmp;
		}
		else
			return NULL;

		*flags = ret_flags; // return filter flags
	}
	else if ( (p_tmp = next_match_char(p, '(')))
	{
		p = p_tmp;
		p_end = find_end_tag(p);
		if (p_end == NULL )
			return 0;
		p_tmp = (char *)soap_malloc(soap, sizeof(char) * (p_end - p));
		memset(p_tmp, 0, sizeof(char) *(p_end - p));
		memcpy(p_tmp, p, sizeof(char) *(p_end - p - 1));
		if ( (p = get_flags_from_attr_comp(soap, p_tmp, &ret_flags)) )
		{
			p = next_match_char(p, ')');
			if (!p)
				return NULL;
		}
		else
			return NULL;

		*flags = ret_flags; // return filter flags
	}
	else if ( (p_tmp = next_match_str(p, "not", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(soap, p, &ret_flags)) )
		{
			*flags = ~ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	if ((p_tmp = next_match_str(p, "and", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(soap, p, &ret_flags)) )
		{
			*flags &= ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	if ((p_tmp = next_match_str(p, "or", 3)))
	{
		p = p_tmp;
		if ( (p = get_flags_from_attr_comp(soap, p, &ret_flags)) )
		{
			*flags |= ret_flags;    // return filter flags
		}
		else
			return NULL;
	}

	return p;
}

int get_flags_from_node(struct soap *soap, char *str)
{
	char *p, *p_tmp, *p_end;
	int ret_flags = 0, i;

	p = next_match_str(str, "//", 2);
	if (!p)
		return 0;

	if ((p = next_find_str(str, "SimpleItem", 10)))
	{
		p = next_match_char(p, '[');
		if (!p)
			return 0;
		p_end = next_find_char(p, ']');
		if (!p_end)
			return 0;
		p_tmp = (char *)soap_malloc(soap, sizeof(char) * (p_end - p));
		memset(p_tmp, 0, sizeof(char) *(p_end - p));
		memcpy(p_tmp, p, sizeof(char) *(p_end - p - 1));
		if ( (p = get_flags_from_attr_comp(soap, p_tmp, &ret_flags)))
		{
			;
		}
		else
			return 0;
	}
	else
		return 0;

	return ret_flags;
}

int apply_contents_ex(struct soap *soap, MSG_FilterExpr* p_expr)
{
	int tmp_flags = 0;

	if (p_expr != NULL)
	{
		if (p_expr->ex_boolean != NULL)
		{
			if (p_expr->ex_boolean->ex_path != NULL)
			{
				if (p_expr->ex_boolean->ex_path->ex_path_str != NULL && p_expr->ex_boolean->ex_path->ex_path_str[0] != '\0')
				{
					if ( p_expr->_not )
					{
						tmp_flags = ~get_flags_from_node(soap, p_expr->ex_boolean->ex_path->ex_path_str);
					}
					else
					{
						tmp_flags = get_flags_from_node(soap, p_expr->ex_boolean->ex_path->ex_path_str);
					}
				}
			}
		}
		if ( p_expr->p_child != NULL )
		{
			tmp_flags = apply_contents_ex(soap, p_expr->p_child);
		}
		if ( p_expr->p_next_and != NULL )
		{
			tmp_flags &= apply_contents_ex(soap, p_expr->p_next_and);
		}
		if ( p_expr->p_next_or != NULL )
		{
			tmp_flags |= apply_contents_ex(soap, p_expr->p_next_and);
		}
	}

	return tmp_flags;
}

void print_contents_list(MSG_FilterExpr* p_expr)
{
	if (p_expr != NULL)
	{
		if ( p_expr->_not )
			_TTY_LOG_ONVIF("(not)");
		if (p_expr->ex_boolean != NULL)
		{
			if (p_expr->ex_boolean->ex_path != NULL)
			{
				if (p_expr->ex_boolean->ex_path->ex_path_str != NULL
				        && p_expr->ex_boolean->ex_path->ex_path_str[0] != '\0')
				{
					_TTY_LOG_ONVIF("%s", p_expr->ex_boolean->ex_path->ex_path_str);
				}
			}
		}
	}
	if ( p_expr->p_child != NULL )
	{
		_TTY_LOG_ONVIF(">>>>");
		print_contents_list(p_expr->p_child);
		_TTY_LOG_ONVIF("<<<<");
	}
	if ( p_expr->p_next_and != NULL )
	{
		_TTY_LOG_ONVIF("and");
		print_contents_list(p_expr->p_next_and);
	}
	if ( p_expr->p_next_or != NULL )
	{
		_TTY_LOG_ONVIF("or");
		print_contents_list(p_expr->p_next_or);
	}
}

MSG_FilterExpr* get_content_expr(struct soap *soap, const char *str)
{
	char *p, *p_end, *p_tmp, *next_str;
	MSG_FilterExpr *msg_filter;
	int ret = 0;

	if ( *str == '\0' )
		return 0;
	msg_filter = (MSG_FilterExpr *)soap_malloc(soap, sizeof(MSG_FilterExpr));
	memset(msg_filter, 0, sizeof(MSG_FilterExpr));

	p = (char *)str;

	if ( *p != '\0' )
	{
		p_tmp = next_match_str(p, "boolean", 7);
		if (p_tmp != NULL)
		{
			p = p_tmp;
			msg_filter->ex_boolean = (BoolExpr *)soap_malloc(soap, sizeof(BoolExpr));
			memset(msg_filter->ex_boolean, 0, sizeof(BoolExpr));
			p = next_match_char(p, '(');
			if ( p == NULL )
				return 0;
			else
			{
				msg_filter->ex_boolean->ex_path = (PathExpr *)soap_malloc(soap, sizeof(PathExpr));
				memset(msg_filter->ex_boolean->ex_path, 0, sizeof(PathExpr));
				p_end = next_find_char(p, ')');
				if (p_end == NULL)
					return 0;
				else
				{
					msg_filter->ex_boolean->ex_path->ex_path_str =
					    (char *)soap_malloc(soap, sizeof(char) * (p_end - p));
					memset(msg_filter->ex_boolean->ex_path->ex_path_str, 0, sizeof(char) *(p_end - p));
					memcpy(msg_filter->ex_boolean->ex_path->ex_path_str, p, sizeof(char) *(p_end - p - 1));
					p = p_end;
					p_tmp = next_match_str(p, "and", 3);
					if (p_tmp != NULL)
					{
						p = p_tmp;
						msg_filter->p_next_and = get_content_expr(soap, p);
						if (msg_filter->p_next_and == NULL)
							return 0;
						else
						{
							msg_filter->p_next_and->p_prev = msg_filter;
							return msg_filter;
						}
					}
					else
					{
						p_tmp = next_match_str(p, "or", 2);
						if (p_tmp != NULL)
						{
							p = p_tmp;
							msg_filter->p_next_or = get_content_expr(soap, p);
							if (msg_filter->p_next_or == NULL)
								return 0;
							else
							{
								msg_filter->p_next_or->p_prev = msg_filter;
								return msg_filter;
							}
						}
						else
							return msg_filter;
					}

				}
			}
		}
		else
		{
			p_tmp = next_match_str(p, "not", 3);
			if ( p_tmp != NULL)
			{
				p = p_tmp;
				msg_filter->_not = 1;
				p = next_match_char(p, '(');
				if ( p == NULL )
					return 0;
				else
				{
					p = next_match_str(p, "boolean", 7);
					if ( p == NULL )
						return 0;
					else
					{
						msg_filter->ex_boolean = (BoolExpr *)soap_malloc(soap, sizeof(BoolExpr));
						memset(msg_filter->ex_boolean, 0, sizeof(BoolExpr));
						p = next_match_char(p, '(');
						if ( p == NULL )
							return 0;
						else
						{
							msg_filter->ex_boolean->ex_path = (PathExpr *)soap_malloc(soap, sizeof(PathExpr));
							memset(msg_filter->ex_boolean->ex_path, 0, sizeof(PathExpr));
							p_end = next_find_char(p, ')');
							if (p_end == NULL)
								return 0;
							else
							{
								msg_filter->ex_boolean->ex_path->ex_path_str =
								    (char *)soap_malloc(soap, sizeof(char) * (p_end - p));
								memset(msg_filter->ex_boolean->ex_path->ex_path_str, 0, sizeof(char) *(p_end - p));
								memcpy(msg_filter->ex_boolean->ex_path->ex_path_str, p, sizeof(char) *(p_end - p - 1));
								p = p_end;
								p_tmp = next_match_str(p, "and", 3);
								if (p_tmp != NULL)
								{
									p = p_tmp;
									msg_filter->p_next_and = get_content_expr(soap, p);
									if (msg_filter->p_next_and == NULL)
										return 0;
									else
									{
										msg_filter->p_next_and->p_prev = msg_filter;
										return msg_filter;
									}
								}
								else
								{
									p_tmp = next_match_str(p, "or", 2);
									if (p_tmp != NULL)
									{
										p = p_tmp;
										msg_filter->p_next_or = get_content_expr(soap, p);
										if (msg_filter->p_next_or == NULL)
											return 0;
										else
										{
											msg_filter->p_next_or->p_prev = msg_filter;
											return msg_filter;
										}
									}
									else
										return msg_filter;
								}
							}
						}
					}
				}
			}
			else
			{
				p_tmp = next_match_char(p, '(');
				if ( p_tmp != NULL)
				{
					p = p_tmp;
					p_end = find_end_tag(p);
					if (p_end == NULL )
						return 0;
					next_str = (char *)soap_malloc(soap, sizeof(char) * (p_end - p));
					memset(next_str, 0, sizeof(char) *(p_end - p));
					memcpy(next_str, p, sizeof(char) *(p_end - p - 1));

					msg_filter->p_child = get_content_expr(soap, next_str);
					if (msg_filter->p_child == NULL)
						return 0;
					else
						msg_filter->p_child->p_parent = msg_filter;

					p = p_end;
					p_tmp = next_match_str(p, "and", 3);
					if (p_tmp != NULL)
					{
						p = p_tmp;
						msg_filter->p_next_and = get_content_expr(soap, p);
						if (msg_filter->p_next_and == NULL)
							return 0;
						else
						{
							msg_filter->p_next_and->p_prev = msg_filter;
							return msg_filter;
						}
					}
					else
					{
						p_tmp = next_match_str(p, "or", 2);
						if (p_tmp != NULL)
						{
							p = p_tmp;
							msg_filter->p_next_or = get_content_expr(soap, p);
							if (msg_filter->p_next_or == NULL)
								return 0;
							else
							{
								msg_filter->p_next_or->p_prev = msg_filter;
								return msg_filter;
							}
						}
						else
							return msg_filter;
					}
				}
				else
					return 0;
			}
		}
	}
	return msg_filter;
}

MSG_FilterExpr* get_msg_contents_filter(struct soap *soap, const char *str)
{
	char *p, *p_end, tmp[512];
	MSG_FilterExpr *msg_filter;

	memset(tmp, 0, sizeof(tmp));
	p = strstr(str, ":MessageContent");

	if (p)
	{
		p = strchr(p, '>');
		p++;
		if (p != NULL && *p != '\0' && *p != '<' )
		{
			p_end = strchr(p, '<');
			if (!p_end)
				return 0;
			if ( (p_end - p) < 512 )
			{
				strncpy(tmp, p, p_end - p);
			}
			else
				return NULL;
		}
	}
	else
		return NULL;

	p = tmp;
	if ( *p != '<' && *p != '\0')
	{
		msg_filter = get_content_expr(soap, p);
		if (msg_filter == NULL)
			return NULL;
	}
	else
		return NULL;

	return msg_filter;
}


int get_topic_ex_filter(const char *str, char **tmp_topic_ex)
{
	char *p, *p_end, te_str[512], topic_ex[MAX_FILTER_CNT][512];
	int i, size_of_topic_ex = 0;

	memset(te_str, 0, sizeof(te_str));
	memset(topic_ex, 0, sizeof(topic_ex));
	p = strstr(str, ":TopicExpression");

	if (p)
	{
		p = strchr(p, '>');
		p++;
		if (p != NULL && *p != '\0' && *p != '<' )
		{
			p_end = strchr(p, '<');
			if (!p_end)
				return 0;
			if ( (p_end - p) < 512 )
			{
				strncpy(te_str, p, p_end - p);
			}
			else
				return 0;
		}
	}
	else
		return 0;

	p = te_str;
	for (i = 0; i < MAX_FILTER_CNT; i++)
	{
		p_end = strstr(p, "//.");
		if (p_end)
		{
			strncpy(topic_ex[i], p, p_end - p);
		}
		else
		{
			/* There is no "//." tag */
			p_end = strchr(p, '\0');
			if ( (p_end - p) < 512 )
			{
				strncpy(topic_ex[i], p, p_end - p);
				i++;
				break;
			}
			else
				return 0;
		}
		p = strchr(p, '|');
		if (!p)
		{
			i++;
			break;
		}
		else
			p++;
	}

	size_of_topic_ex = i;
	memcpy(tmp_topic_ex, topic_ex, sizeof(topic_ex));

	return size_of_topic_ex;
}

int make_filter_flags(struct soap *soap, const char *str )
{
	char *p, *p_end, topic_ex[MAX_FILTER_CNT][512];
	int i, ret_flags = 0, size_of_topic_ex = 0;
	MSG_FilterExpr *msg_filter;

	if (str == NULL && str[0] == '\0')
		return 0;

	size_of_topic_ex = get_topic_ex_filter(str, (char **)topic_ex);
	if ( size_of_topic_ex )
	{
		ret_flags = apply_topic_ex(soap, (char**)topic_ex, size_of_topic_ex);
	}

	msg_filter = get_msg_contents_filter(soap, str);
	if ( msg_filter != NULL )
	{
		ret_flags = apply_contents_ex(soap, msg_filter);
	}

	return ret_flags;
}



#endif


