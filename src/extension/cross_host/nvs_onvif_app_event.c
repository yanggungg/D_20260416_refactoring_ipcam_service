#include "onvif_common.h"

#if defined(WIN32)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "soapStub.h"
#include "onvif_util.h"

#else

#include "../nf_onvif_server.h"
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

#include <glib.h>
#include "../nvs_service/nvs_onvif_app_util.h"
#endif
#include <pthread.h>

#include "onvif_cross_util.h"
#include "glib.h"

extern struct event_subscription	g_onvif_event_sub[MAX_SUBSCRIPTION_CNT];
extern ONVIF_EVENT_MSGBUF	g_onvif_event_msgbuf;

int print_time(char *msg, time_t tt) {
    struct tm *local = localtime(&tt); // localtime���� ��ȯ

    printf("Local time: %s", asctime(local));

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
    printf("SKSHIN] (%s), Formatted local time: %s\n", msg, buffer);

    return 0;
}

#if !defined(WIN32)

static pthread_mutex_t		g_event_msgbuf_mutex;
static pthread_mutex_t		g_event_subscription_mutex;
static pthread_mutex_t		g_event_notify_data;
#else
void random_bytes( unsigned char *dest, int len )
{
	int i;

	for( i = 0; i < len; ++i )
		dest[i] = random() & 0xff;
}


#endif

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

int check_match_topic(const char* te, const char * topic)
{
	char *p_filter=NULL, *p_filter_sub=NULL, *p_filter_end= NULL, *p_topic =NULL, *p_tmp =NULL, str_pass=0;

//	if (!topic) {
//		return ONVIF_FILTERED;
//	}
	
	// SKSHIN
	char *pos = strstr(te, "/.");
	if (pos != 0) {
		int ret = ONVIF_FILTERED;
		char tmp[128];
		memset(tmp, 0x00, sizeof(tmp));
		strncpy(tmp, te, pos - te);

		if (strstr(topic, tmp) != 0) {
			ret = ONVIF_NOT_FILTERED;
			return ret;
		}
	}
	else if (strstr(topic, te) != NULL) {
	    gchar **splited_text = NULL;
		gint count_splited_text = 0;		

		int ret = ONVIF_FILTERED;
		splited_text = g_strsplit(te, "|", MAX_MSG_FILTER_CNT);
		count_splited_text = g_strv_length(splited_text);
		int i = 0;
		for (i = 0; i < count_splited_text; ++i) {
			if (strncmp(topic, splited_text[i], strlen(splited_text[i])) == 0) {
			//if (strcmp(splited_text[i], topic) == 0) {
				ret = ONVIF_NOT_FILTERED;
				break;
			}
			else {
				ret =  ONVIF_FILTERED;
			}
		}
		g_strfreev(splited_text); 	
		return ret;
	}
	else if (strstr(te, topic) != NULL) {
	    gchar **splited_text = NULL;
		gint count_splited_text = 0;		

		int ret = ONVIF_FILTERED;
		splited_text = g_strsplit(te, "|", MAX_MSG_FILTER_CNT);
		count_splited_text = g_strv_length(splited_text);
		int i = 0;
		for (i = 0; i < count_splited_text; ++i) {
			if (strncmp(topic, splited_text[i], strlen(splited_text[i])) == 0) {
			//if (strcmp(splited_text[i], topic) == 0) {
				ret = ONVIF_NOT_FILTERED;
				break;
			}
			else {
				ret =  ONVIF_FILTERED;
			}
		}
		g_strfreev(splited_text); 	
		return ret;
	}
	else {
		return ONVIF_FILTERED;
	}

	p_filter = te;

	while ( p_filter <  te + sizeof(char)*MAX_FILTER_DATA_SIZE ){
		OV_DEBUG("p_filter[%s]", p_filter);		
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
			OV_DEBUG("p_filter_sub[%s] p_topic[%s]", p_filter_sub, p_topic);			
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
						
						p_tmp = next_find_char(p_filter_sub, ':', p_filter_sub + strlen(p_filter_sub));
						if( p_tmp ){
							p_filter_sub = p_tmp;
						}
						else{
							p_filter_sub++;						
						}
						
						p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
						if( p_tmp ){
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
		OV_DEBUG("retry p_filter[%s] p_topic[%s]", p_filter, p_topic);
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
							
							p_tmp = next_find_char(p_filter_sub, ':', p_filter_sub + strlen(p_filter_sub));
							if( p_tmp ){
								p_filter_sub = p_tmp;
							}
							else{
								p_filter_sub++;						
							}
							
							p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
							if( p_tmp ){
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

#if 0
int check_match_topic(const char* te, const char * topic)
{
	printf("SKSHIN], [[%s]], [[%s]]\n", te, topic);
	char *p_filter=NULL, *p_filter_sub=NULL, *p_filter_end= NULL, *p_topic =NULL, *p_tmp =NULL, str_pass=0;

	if (strstr(te, topic)) {
		printf("SKSHIN] not filtered. [%d]\n", __LINE__);
		return ONVIF_NOT_FILTERED;
	}
	else {
		p_filter = te;

		while ( p_filter <  te + sizeof(char)*MAX_FILTER_DATA_SIZE ){
			printf("p_filter[%s]\n", p_filter);		
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
				printf("SKSHIN] filtered. [%d]\n", __LINE__);
				return ONVIF_FILTERED;
			}

			p_topic = next_find_char(topic, ':', topic + strlen(topic));
			if (!p_topic) {
				printf("SKSHIN] filtered. [%d]\n", __LINE__);
				return ONVIF_FILTERED;
			}

			str_pass = 0;
			p_filter_sub = p_filter;
			while( p_filter_sub <= p_filter_end ){
				printf("p_filter_sub[%s] p_topic[%s]\n", p_filter_sub, p_topic);			
				if( *p_filter_sub == '/' ){
					/* End of Str */
					if( *(p_filter_sub+1) == '/' && *(p_filter_sub+2) == '.' ){ // hun_0131119 : case "//."
						if( (*p_topic == '/' || *p_topic == '\0') && str_pass ){
							printf("SKSHIN] not filtered. [%d]\n", __LINE__);
							return ONVIF_NOT_FILTERED;
						}
						else {
							break;					
						}
					}
					else{									// case "/"
						if( *p_topic == '/' && str_pass ){
							str_pass = 0;

							p_tmp = next_find_char(p_filter_sub, ':', p_filter_sub + strlen(p_filter_sub));
							if( p_tmp ){
								p_filter_sub = p_tmp;
							}
							else{
								p_filter_sub++;						
							}

							p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
							if( p_tmp ){
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
							printf("SKSHIN] not filtered. [%d]\n", __LINE__);
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
			printf("retry p_filter[%s] p_topic[%s]\n", p_filter, p_topic);
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
							printf("SKSHIN] not filtered. [%d]\n", __LINE__);
								return ONVIF_NOT_FILTERED;
							}
							else {
								break;					
							}
						}
						else{									// case "/"
							if( *p_topic == '/' && str_pass ){
								str_pass = 0;

								p_tmp = next_find_char(p_filter_sub, ':', p_filter_sub + strlen(p_filter_sub));
								if( p_tmp ){
									p_filter_sub = p_tmp;
								}
								else{
									p_filter_sub++;						
								}

								p_tmp = next_find_char(p_topic, ':', topic + strlen(topic));
								if( p_tmp ){
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
							printf("SKSHIN] not filtered. [%d]\n", __LINE__);
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

		printf("SKSHIN] filtered. [%d]\n", __LINE__);
		return ONVIF_FILTERED;
	}
}
#endif

int apply_topic_ex(char *te, struct event_subscription *sub)
{
	char *p, *p_tmp;
	int i=0;

	if (strlen(te) == 0) {
		return 0;
	}

	for(i=0; i<NUMBER_OF_EVENTS; i++){
		sub->tf.topic_filter[i] = ONVIF_FILTERED;
	}

	sub->tf.topic_filter[E_KEY_SENSOR] 							= check_match_topic(te, str_tns1_tnsitx_Alarm);
	sub->tf.topic_filter[E_KEY_ITX_MOTION]				 		= check_match_topic(te, str_tns1_tnsitx_Motion);
	sub->tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION0]			= check_match_topic(te, str_tns1_VideoAnalytics_Motion);
	sub->tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION1]			= check_match_topic(te, str_tns1_VideoAnalytics_Motion);
	sub->tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION2]			= check_match_topic(te, str_tns1_VideoAnalytics_Motion);
	sub->tf.topic_filter[E_KEY_VIDEOANALYTICS_MOTION3]			= check_match_topic(te, str_tns1_VideoAnalytics_Motion);
	sub->tf.topic_filter[E_KEY_MOTION_ALARM] 					= check_match_topic(te, str_tns1_General_Motion);
	sub->tf.topic_filter[E_KEY_RECORDING_JOB_STATE_CHANGE]	= check_match_topic(te, str_tns1_tnsitx_jobState_change);
	sub->tf.topic_filter[E_KEY_CONFIGURATION_CHANGE]			= check_match_topic(te, str_tns1_tnsitx_conf_change);
	sub->tf.topic_filter[E_KEY_RECORDINGHISTORY_RECORDING_STATE]							= check_match_topic(te, str_tns1_relay);
	sub->tf.topic_filter[E_KEY_RECORDINGCONFIG_TRACK_CONFIGURATION] = check_match_topic(te, str_tns1_tnsitx_RecordingConfigTrackConfig);
	sub->tf.topic_filter[E_KEY_DATA_DELETION]					= check_match_topic(te, str_tns1_tnsitx_data_deletion);
	sub->tf.topic_filter[E_KEY_RECORDING_AND_TRACK]			= check_match_topic(te, str_tns1_recording_and_track);
	sub->tf.topic_filter[E_KEY_DIGITAL_INPUT]					= check_match_topic(te, str_tns1_digital_input);
	sub->tf.topic_filter[E_KEY_RELAY]							= check_match_topic(te, str_tns1_relay);
	sub->tf.topic_filter[E_KEY_VIDEO_GLOBAL_SCENE_CHANGE_IMAGE]	= check_match_topic(te, str_tns1_GlobalSceneChange);	
	sub->tf.topic_filter[E_KEY_VIDEO_SIGNAL_LOSS]	= check_match_topic(te, str_tns1_VideoSignalLoss);		
	sub->tf.topic_filter[E_KEY_EVENT_CONFIGURATION_CHANGE]	= check_match_topic(te, str_tns1_tnsitx_EventConfChange);
	sub->tf.topic_filter[E_KEY_ADVANCED_SENSOR]	= check_match_topic(te, str_tns1_tnsitx_AdvancedSensor);	
	sub->tf.topic_filter[E_KEY_CHANGE_CONFIGURATION]	= check_match_topic(te, str_tns1_ConfigurationChange);
	sub->tf.topic_filter[E_KEY_CHANGE_PROFILE]	= check_match_topic(te, str_tns1_ProfileChange);	
	sub->tf.topic_filter[E_KEY_IMAGE_TOO_BLURRY]	= check_match_topic(te, str_tns1_ImageBlur);
	// sub->tf.topic_filter[E_KEY_IMAGE_TOO_DARK]	= check_match_topic(te, str_tns1_ImageDark);
	// sub->tf.topic_filter[E_KEY_IMAGE_TOO_BRIGHT]	= check_match_topic(te, str_tns1_ImageBright);
	// sub->tf.topic_filter[E_KEY_IMAGE_GLOBAL]	= check_match_topic(te, str_tns1_ImageGlobalSceneChange);
		


	// for(i=0; i<NUMBER_OF_EVENTS; i++){
    // 	printf("%d's Event -> %s\n", i,
    //        sub->tf.topic_filter[i] ? "FILTERED" : "NOT FILTERED");	}
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

	if(  filters != NULL ){
		memcpy(&tmp_filters, filters, sizeof(struct onvif_filters ));
	}

	if( termination_time == 0 ){
		now = time(NULL);
		termination_time = now + 600;
		printf("termination time is 0, set to current[%d] termination[%d]\n", (int)now, (int)termination_time);
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
			return SSID_IS_DUPLICATED;
		}
		
		if (subs[i].ss_id == INVALID_SSID) {
			sub_index = i;
			break;
		}
	}
	if (i == MAX_SUBSCRIPTION_CNT )
	{
		UNLOCK_onvif_event_subs();
		return SUB_BUF_IS_FULL;
	}

	subs[sub_index].ss_id = ss_id;
	subs[sub_index].t_time = termination_time + 5;
	subs[sub_index].u_time = eb.msg_buf[eb.tail].uptime_tick;
//	subs[sub_index].old_start_idx = eb.tail;
#if 0
	if ( eb.tail >= (MAX_MSG_BUF - 1) )
	{
		subs[i].mbuf_end = 0;
	}
	else
	{
		subs[i].mbuf_end = eb.tail + 1;
	}
#endif
	subs[sub_index].mbuf_end = eb.tail;
	//FIXME.convert filter
	memset(&subs[sub_index].tf, 0x00, sizeof(struct e_topic_filter));
	memset(&subs[sub_index].mf, 0x00, sizeof(struct e_msg_filter));
	// tmp_filters  ->  sub.[i].tf / sub.[i].mf
	if( filters->cnt ){
		ret = make_filter_flags(filters, &subs[sub_index]);
		if( ret < 0){
			printf("Making Filter is Failed !!\n");
			UNLOCK_onvif_event_subs();
			return -1;
		}
	}
//	sub_index = i;
	subs[sub_index].is_first_req = 1;

#if 0
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)
	{
		if ( subs[i].ss_id == 0 )
		{
			subs[i].ss_id = ss_id;
			subs[i].t_time = termination_time + 5;
			subs[i].u_time = eb.msg_buf[eb.tail].uptime_tick;
			subs[i].old_start_idx = eb.tail;
			if ( eb.tail >= (MAX_MSG_BUF - 1) )
			{
				subs[i].mbuf_end = 0;
			}
			else
			{
				subs[i].mbuf_end = eb.tail + 1;
			}
			//FIXME.convert filter
			memset(&subs[i].tf, 0x00, sizeof(struct e_topic_filter));
			memset(&subs[i].mf, 0x00, sizeof(struct e_msg_filter));
			// tmp_filters  ->  sub.[i].tf / sub.[i].mf
			if( filters->cnt ){
				ret = make_filter_flags(filters, &subs[i]);
				if( ret < 0){
					printf("Making Filter is Failed !!\n");
					UNLOCK_onvif_event_subs();
					return -1;
				}
			}
			sub_index = i;
			subs[sub_index].is_first_req = 1;
			break;
		}
		else
		{
			now = time(NULL);
			if ( subs[i].t_time < now )
			{
				subs[i].ss_id = ss_id;
				subs[i].u_time = eb.msg_buf[eb.tail].uptime_tick;
				subs[i].old_start_idx = eb.tail;
				if ( eb.tail >= (MAX_MSG_BUF - 1) )
				{
					subs[i].mbuf_end = 0;
				}
				else
				{
					subs[i].mbuf_end = eb.tail + 1;
				}
				subs[i].t_time = termination_time + 5;
				//FIXME.convert filter
				memset(&subs[i].tf, 0x00, sizeof(struct e_topic_filter));
				memset(&subs[i].mf, 0x00, sizeof(struct e_msg_filter));
				// tmp_filters  ->  sub.[i].tf / sub.[i].mf
				if( filters->cnt ){
					ret = make_filter_flags(filters, &subs[i]);
					if( ret < 0){
						printf("Making Filter is Failed !!\n");
						UNLOCK_onvif_event_subs();
						return -1;
					}
				}
				sub_index = i;
				break;
			}
		}
	}
#endif

	memcpy(g_onvif_event_sub, subs, sizeof(struct event_subscription) * MAX_SUBSCRIPTION_CNT);
#if 0
	for(i=0; i<MAX_SUBSCRIPTION_CNT; i++){
		OV_DEBUG("sub[%d] id[%d] t[%d] up[%d] tail[%d] end[%d]", i, g_onvif_event_sub[i].ss_id, g_onvif_event_sub[i].t_time, g_onvif_event_sub[i].u_time, g_onvif_event_sub[i].old_start_idx, g_onvif_event_sub[i].mbuf_end);
	}
#endif
	UNLOCK_onvif_event_subs();

	return sub_index;
}

static unsigned int g_ssid = START_SSID;

int OV_event_create_new_subscription(unsigned int termination, struct onvif_filters *filters)
{
	int ret = 0, sub_id=0, rnd = 0, current_time = 0;
	int ss_id;

	current_time = (int)time(NULL);

	ret = -1;

	while (ret < 0) {
//		random_bytes((unsigned char *)&rnd, sizeof(unsigned char) );
		ss_id = g_ssid;
		//sub_id = current_time | (rnd << 23);
		ret = add_event_subscription( ss_id, (time_t)termination, filters);
		if (ret == SUB_ERROR || ret == SUB_BUF_IS_FULL) return -1;
		++g_ssid;
		if (g_ssid == INVALID_SSID) g_ssid = START_SSID;
	}

#if 1
//	unsigned int ss_id = sub_id;

printf("SKSHIN] NEW SUBSCRIPTION ID = [%d]\n", ss_id);

	struct event_subscription sub;
	memset(&sub, 0x00, sizeof(sub));
	ret = OV_event_get_subscription(&sub, ss_id);
	if( ret != ss_id ){
		return -1;
	}

//	if (sub.is_first_req) {
{
//		OV_event_set_subscription_is_first_req(ss_id, 0);
		sub.is_first_req = 0;

		int i;
		for(i=0; i<NUMBER_OF_EVENTS; i++){
			guint m_state;
			struct e_motion_type motion;
			struct e_relay_type e_relay;
			struct e_ad_alarm_type alarm;
			struct e_image_type image;
			if (sub.tf.topic_filter[i] == 0) {
				switch (i) {
					case E_KEY_MOTION_ALARM:
						for (int j = 0; j < NUM_ACTIVE_CH; ++j) {
							motion.ch = j;
							m_state =	nf_notify_get_param0("motion");
							motion.value = m_state;
							append_onvif_event_msg(E_KEY_MOTION_ALARM, &motion, E_Initialized, ss_id);
						}
						break;
					case E_KEY_DIGITAL_INPUT:
						for (int j = 0; j < NUM_ACTIVE_CH; ++j) {
							alarm.ch = j;
							m_state =	nf_notify_get_param0("sensor");
							alarm.state = m_state;
							append_onvif_event_msg(E_KEY_DIGITAL_INPUT, &alarm, E_Initialized, ss_id);
						}
						break;
					case E_KEY_RELAY:
						for (int j = 0; j < ONVIF_RELAY_CNT; ++j) {
							e_relay.ch = j;
							e_relay.active = RELAY_STATE_INACTIVE;
							append_onvif_event_msg(E_KEY_RELAY, &e_relay, E_Initialized, ss_id);
						}
						break;
					case E_KEY_IMAGE_TOO_BLURRY:
						image.ch = 0;
						image.value = 1;
						append_onvif_event_msg(E_KEY_IMAGE_TOO_BLURRY, &image, E_Initialized, ss_id);
						break;
				}
			}
		}

		//	OV_event_set_subscription_is_first_req(ss_id, 0);
	}
#endif
	return ss_id;
}

#if defined(WIN32)
int onvif_CreatePullPointSubscription(void* tmp_, int size)
#else

int onvif_CreatePullPointSubscription(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
	arg_CreatePullPoint tmp;

	unsigned int ss_id = 0;
	int ret = 0, http_port = 80, https_port = 443;
#if defined(WIN32)
	memcpy(&tmp, tmp_, size);
#else
	    NF_NETIF_GET_INFO ret_net_info;
		struct sockaddr_in ipaddr;
		char ipaddress[COMMON_SIZE];
		ONVIF_PACKET packet;
		ONVIF_HEADER header;
		guint packet_len;
        memset(&header, 0, sizeof (header));
        memcpy(&header, buff_rcv, sizeof (header));

        packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_CreatePullPoint);

        memcpy(&packet, buff_rcv, packet_len);
        memcpy(&tmp, packet.data, sizeof (arg_CreatePullPoint));
		memset(ipaddress, 0x00, sizeof(char)*COMMON_SIZE);

#endif
	// Create New subscription and get ss_id
	print_time("SUBSCRIPTION, TERMINATION TIME", tmp.termination_time);
	ret = OV_event_create_new_subscription(tmp.termination_time, &tmp.filters);
	if( ret <= 0 ){
		tmp.err_code = ONVIF_PROCESS_RET_ERROR;
		memcpy(packet.data, &tmp, sizeof(arg_CreatePullPoint));
		if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_CreatePullPoint)) != ONVIF_ERR_RET_SUCCESS) {
			return ONVIF_ERR_RET_INTERNAL;
		}
		return ONVIF_R_ERR;
	}
	ss_id = (unsigned int)ret;


	// Get subscription address

#if defined(WIN32)
	sprintf(tmp.ss_address,  "http://%s:%d/onvif/subscription?subscription=%d", SIMULATOR_IP, 19000, ss_id);
	memcpy(tmp_, &tmp, size);
#else
	http_port = (int)nf_sysdb_get_uint("net.http.onvifport");
	https_port = (int)nf_sysdb_get_uint("net.proto.sslport");

	nf_netif_get_info(&ret_net_info);
	ipaddr.sin_addr.s_addr = (unsigned long) ret_net_info.ipaddr;
	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);
	snprintf(ipaddress, sizeof(char)*COMMON_SIZE, "%s", inet_ntoa(ipaddr.sin_addr));
	if ( is_https_required() )
		snprintf(tmp.ss_address, sizeof(tmp.ss_address), "https://%s:%d/onvif/subscription?subscription=%d", ipaddress, https_port, ss_id);
	else
	snprintf(tmp.ss_address, sizeof(tmp.ss_address), "http://%s:%d/onvif/subscription?subscription=%d", ipaddress, http_port, ss_id);
	memcpy(packet.data, &tmp, sizeof(arg_CreatePullPoint));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_CreatePullPoint)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

#endif


	return ONVIF_ERR_RET_SUCCESS;
}


#if 1
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
			now = (int)time(NULL);
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

	print_time("SKSHIN", time(0));
	OV_DEBUG("ss id [%d]", ss_id);
	if ( !ss_id ) {
		return -1;
	}

	LOCK_onvif_event_subs();
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)	{
		if ( g_onvif_event_sub[i].ss_id == ss_id )	{
			now = time(NULL);
			if ( g_onvif_event_sub[i].t_time > now ) {
				g_onvif_event_sub[i].u_time = now;
//				g_onvif_event_sub[i].old_start_idx = old_start;
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

int _is_event_subscribed(int ss_id, int key)
{
	int shmid_bn, shmid_pull, ret1, ret2, i, j;
	time_t now;
	void *mem_bn, *mem_pull;
	int semid_bn, semid_pull;

	struct event_subscription sub_list[MAX_SUBSCRIPTION_CNT];

	OV_DEBUG("ss id [%d]", ss_id);
	if ( !ss_id ) {
		return 0;
	}

	LOCK_onvif_event_subs();
	for (i = 0; i < MAX_SUBSCRIPTION_CNT; i++)	{
		if ( g_onvif_event_sub[i].ss_id == ss_id )	{
			if ( g_onvif_event_sub[i].tf.topic_filter[key] == 0 ) {
				return 1;
			}
		}
	}
	UNLOCK_onvif_event_subs();

	if ( i == MAX_SUBSCRIPTION_CNT ) {
		return 0;
	}

	return 0;
}


extern int count_event_buf();
int OV_BLOCK_event_pull_messages(int ss_id, struct e_msg  *dest, unsigned int limit, unsigned int timeout )
{
	ONVIF_EVENT_MSGBUF eb;
	struct event_subscription sub;
	int ret = 0;
	unsigned int loop_cnt =0;
	unsigned int termination_time = 0, msg_num=0, current_mbuf_end = 0;
	time_t now;

	if( dest == NULL ){
		return -1;
	}

#if 1
	printf("====================== START EVENT PULL ==============================\n");
	printf("====================== START EVENT PULL ==============================\n");
	printf("====================== START EVENT PULL ==============================\n");
	printf("====================== START EVENT PULL ==============================\n");
	printf("====================== START EVENT PULL ==============================\n");
	printf("====================== START EVENT PULL (ss_id:%d)==============================\n", ss_id);
	printf("====================== START EVENT PULL (limit:%d)==============================\n", limit);
	printf("====================== START EVENT PULL (timeout:%d)==============================\n", timeout);
#endif

	memset(&sub, 0x00, sizeof(sub));
	ret = OV_event_get_subscription(&sub, ss_id);
	if( ret != ss_id ){
		return -1;
	}

	now = time(NULL);
	termination_time = (unsigned int)now + timeout;


	while ( now < (time_t)termination_time ) {
		print_time("EVENT SEND, NOW=", now);
		print_time("EVENT SEND, termination=", termination_time);

		int msgcnt = count_event_buf();
		printf("SKSHIN] CURRENT MESSSAGE COUNT = %d\n", msgcnt);

		copy_from_event_msgbuf(&eb);

		ret = OV_event_get_subscription(&sub, ss_id);

		if( ret != ss_id ){
			return -1;
		}

		if (now >= sub.t_time ){
			OV_event_unsubscribe(ss_id);
			remove_onvif_event_msg(ss_id);
			return -1;
		}
		int org_mbuf_end;
		int end_flag = 0;
		if (sub.mbuf_end == 0) {
			org_mbuf_end = MAX_MSG_BUF - 1;
		} else {
			org_mbuf_end = sub.mbuf_end - 1;
		}
		int mbuf_end = sub.mbuf_end;

		g_assert(sub.mbuf_end >= 0 && sub.mbuf_end < MAX_MSG_BUF);
		int cnt = 0;

		while (1) {
		// The original code increments the index first, causing the first message to be skipped.
		// The revised code without an end flag would miss checking the last message.
		// The logic has been updated as follows to ensure all messages, including the first and last, are properly checked.

			if (mbuf_end == org_mbuf_end) {
				end_flag = 1;
				printf("SKSHIN] BLOCK PULL LOOP EXIT\n");
			}


#if 1
			// printf("SKSHIN] sub.ss_id = %d, sub.u_time=%d\n", sub.ss_id, sub.u_time);
			// printf("SKSHIN] mbuf_end=%d, key=%d, ss_id=%d, sent=%d\n", mbuf_end, eb.msg_buf[mbuf_end].key, eb.msg_buf[mbuf_end].ss_id, eb.msg_buf[mbuf_end].sent);
#endif
			if(	eb.msg_buf[mbuf_end].key == 0 || 
//				eb.msg_buf[mbuf_end].uptime_tick <= sub.u_time ||
				eb.msg_buf[mbuf_end].sent == 1 ||
				eb.msg_buf[mbuf_end].ss_id != sub.ss_id) 
			{
				if (end_flag == 1)
					break;
				mbuf_end++;
				if ( mbuf_end >= MAX_MSG_BUF ) {
					mbuf_end = 0;
				}
				g_usleep(100);
				continue;
			}

#endif
//			printf("SKSHIN] BLOCK pull message send\n");
			/* Filter Check */
			switch ( eb.msg_buf[mbuf_end].key ) {
				case E_KEY_SENSOR:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_SENSOR] == ONVIF_FILTERED )
					{
						break;
					}
//					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_ITX_MOTION:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_ITX_MOTION] == ONVIF_FILTERED )
					{
						break;
					}	
//					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_MOTION_ALARM:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_MOTION_ALARM] == ONVIF_FILTERED )
					{
						break;
					}
					else 
					{
//						int subch = eb.msg_buf[sub.mbuf_end].motion.ch;
						int subch = eb.msg_buf[mbuf_end].motion.ch;
						if (sub.mf.motion_alarm.ch[subch] == ONVIF_FILTERED)
						{
							break;
						}
						else {

						}
					}

//					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_RECORDING_JOB_STATE_CHANGE:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_RECORDING_JOB_STATE_CHANGE] == ONVIF_FILTERED )
					{
						break;
					}	
					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_CONFIGURATION_CHANGE:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_CONFIGURATION_CHANGE] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_RECORDINGHISTORY_RECORDING_STATE:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_RECORDINGHISTORY_RECORDING_STATE] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_RECORDINGCONFIG_TRACK_CONFIGURATION:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_RECORDINGCONFIG_TRACK_CONFIGURATION] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_DATA_DELETION:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_DATA_DELETION] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_RECORDING_AND_TRACK:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_RECORDING_AND_TRACK] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_DIGITAL_INPUT:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_DIGITAL_INPUT] == ONVIF_FILTERED )
					{
						break;
					}
					else 
					{
//						int subch = eb.msg_buf[sub.mbuf_end].alarm.ch;
						int subch = eb.msg_buf[mbuf_end].alarm.ch;
						if (sub.mf.digital_input.ch[subch] == ONVIF_FILTERED)
						{
							break;
						}
						else {

						}
					}

//					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_RELAY:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_RELAY] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_CHANGE_PROFILE:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_CHANGE_PROFILE] == ONVIF_FILTERED )
					{
						break;
					}
//					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_CHANGE_CONFIGURATION:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_CHANGE_CONFIGURATION] == ONVIF_FILTERED )
					{
						break;
					}
//					memcpy(&dest[msg_num], &eb.msg_buf[sub.mbuf_end], sizeof(struct e_msg));
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
					break;
				case E_KEY_IMAGE_TOO_BLURRY:
					/* Filter check */
					if ( sub.tf.topic_filter[E_KEY_IMAGE_TOO_BLURRY] == ONVIF_FILTERED )
					{
						break;
					}
					memcpy(&dest[msg_num], &eb.msg_buf[mbuf_end], sizeof(struct e_msg));
					update_onvif_event_msg(mbuf_end, 1);
					msg_num++;
					sub.mbuf_end = mbuf_end;
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
					printf("SKSHIN] UNKNOWN MESSAGE\n");
					update_onvif_event_msg(mbuf_end, 1);
					sub.mbuf_end = mbuf_end;
					break;
			}

			if (end_flag == 1)
				break;
						
			mbuf_end++;
			if ( mbuf_end >= MAX_MSG_BUF ) {
				mbuf_end = 0;
			}

				
			if (msg_num > 0 || msg_num >= limit ) {
				goto event_done;
			}

			++cnt;
			g_usleep(100);
		}	// while


event_keep_searching:
		g_usleep(500000);
		now = time(NULL);

	}	// while

	// No Message
	return 0;

event_done :

	ret = onvif_update_event_subscription( ss_id, 0, eb.tail, sub.mbuf_end);

	printf("SKSHIN] I got [%d] messages\n", msg_num);
	return (int)msg_num;
}

#if defined(WIN32)
#else
int onvif_PullMessages(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
	arg_PullMessages tmp;
	unsigned int ss_id = 0;
	int ret = 0, http_port = 80, https_port = 443;
	int msgcnt = 0;
	struct event_subscription sub;
#if defined(WIN32)
	memcpy(&tmp, tmp_, size);
#else
		ONVIF_PACKET packet;
		ONVIF_HEADER header;
		guint packet_len;
        memset(&header, 0, sizeof (header));
        memcpy(&header, buff_rcv, sizeof (header));

        packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_PullMessages);

        memcpy(&packet, buff_rcv, packet_len);
        memcpy(&tmp, packet.data, sizeof (arg_PullMessages));
#endif
	//printf("ssid[%d] timeout[%d]\n", tmp.ss_id, tmp.timeout);
	//printf("ssid[%d] limit[%d]\n", tmp.ss_id, tmp.limit);

	ret = OV_event_renew_subscription(tmp.ss_id, tmp.timeout + 60, RENEW_RELATIVE);
	if ( ret < 0) {
		OV_DEBUG("Fail!! ret[%d]", ret);
		return ONVIF_R_ERR;
	}
	tmp.termination_time = (unsigned int)ret;

	/* This function may incur block */
	if( tmp.limit > COMMON_SIZE ){
		tmp.limit = COMMON_SIZE;
	}

	printf("SKSHIN] BLOCK pull : ss_id = (%d)\n", tmp.ss_id);
	printf("SKSHIN] BLOCK pull : timeout = (%d)\n", tmp.timeout);
	printf("SKSHIN] BLOCK pull : limit = (%d)\n", tmp.limit);
	msgcnt = OV_BLOCK_event_pull_messages(tmp.ss_id, tmp.msgs, tmp.limit, tmp.timeout);
	if( msgcnt < 0 ){
		OV_DEBUG("Fail!! msgcnt[%d]", msgcnt);
		return ONVIF_R_ERR;
	}
	tmp.msgcnt = (unsigned int)msgcnt;
	OV_DEBUG("I got messages [%d]!!", tmp.msgcnt);

	// Get current and termination time
	ret = OV_event_get_subscription(&sub, tmp.ss_id);
	if( ret < 0 ){
		OV_DEBUG("Fail!! ret[%d]", ret);
		return ONVIF_R_ERR;
	}
	tmp.current_time = (unsigned int)time(NULL);
#if defined(WIN32)
	memcpy(tmp_, &tmp, size);
#else
	memcpy(packet.data, &tmp, sizeof (arg_PullMessages));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
						header.code, sizeof (arg_PullMessages)) != ONVIF_ERR_RET_SUCCESS) {
				return ONVIF_ERR_RET_INTERNAL;
	}
#endif
	return ONVIF_ERR_RET_SUCCESS;
}

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
//				if ( g_onvif_event_sub[i].t_time > now ) {
					g_onvif_event_sub[i].t_time = 0;
					g_onvif_event_sub[i].u_time = 0;
					memset(&g_onvif_event_sub[i].tf, 0x00, sizeof(struct e_topic_filter));
					memset(&g_onvif_event_sub[i].mf, 0x00, sizeof(struct e_msg_filter));
					g_onvif_event_sub[i].is_first_req = 1;
					g_onvif_event_sub[i].ss_id = INVALID_SSID;
					break;
//				}
			}
		}
	}
	UNLOCK_onvif_event_subs();

	if (i == MAX_SUBSCRIPTION_CNT )	{
		OV_DEBUG( " fail![%d]", ss_id);
		return -1;
	}

	LOCK_onvif_event_msgbuf();
	for (i = 0; i < MAX_MSG_BUF; ++i) {
		if ( g_onvif_event_msgbuf.msg_buf[i].ss_id == ss_id)	{
			g_onvif_event_msgbuf.msg_buf[i].ss_id = INVALID_SSID;
			g_onvif_event_msgbuf.msg_buf[i].sent = 1;
		}
	}
	UNLOCK_onvif_event_msgbuf();

	return 1;
}

#if defined(WIN32)
int onvif_Unsubscribe(void* tmp_, int size)
#else
int onvif_Unsubscribe(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_Unsubscribe tmp;
		int ret=0;

	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_Unsubscribe);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_Unsubscribe));
    #endif
		ret = OV_event_unsubscribe(tmp.ss_id);
		if ( ret < 0) {
			tmp.err_code = ONVIF_PROCESS_RET_ERROR;
	//		return ONVIF_R_ERR;
		}

	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_Unsubscribe));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_Unsubscribe)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}
