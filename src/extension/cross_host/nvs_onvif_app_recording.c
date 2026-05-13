#include "onvif_common.h"

#if defined(WIN32)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "soapStub.h"
#include "onvif_util.h"
#else 
#include "../nf_onvif_server.h"
#include "nf_api_disk.h"
#endif

#include "../cross_host/onvif_cross_util.h"
#include "nf_sysdb.h"
#include <glib.h>
#include <string.h>
#include <stdlib.h>

static int 		end_search_token[MAX_REC_SEARCH_SESSION] = {0};
static int 		search_token[MAX_REC_SEARCH_SESSION] = {0};

extern void GetCapability(arg_GetCapa *capa, int ch);
extern int isValidVideoSourceToken(char *token);
extern int isValidProfileToken(char *token);
extern int isValidPtzToken(char *token);
extern int isValidPtzNodeToken(char *token);

pthread_mutex_t		srt_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		ri_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		end_srt_mutex=PTHREAD_MUTEX_INITIALIZER;

GHashTable *g_rec_hash_table;
GArray* rec_arr,*eve_arr;

int checkRecordingEmpty(int i) {
	char buff[COMMON_SIZE] = { 0, };
	char value[COMMON_SIZE] = { 0, };
	
	sprintf(buff, "onvif.recordingjob%d.source_token",i);
	strncpy(value, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE-1);

	if (!strcmp(value , "")) 
	{
		return 1;
	}

	return 0;


}
static void recording_search_token_expire(int token_idx)
{
	char key[COMMON_SIZE-1] = {0,};
	if(token_idx < 0 || token_idx > MAX_REC_SEARCH_SESSION)
		return;

	
	search_token[token_idx] = 0;
	sprintf(key, "srt%d", token_idx);
	
	//pthread_mutex_unlock(&srt_mutex);
}
#if defined(WIN32)
int onvif_GetRecordings(void* tmp_, int size) {
#else
int onvif_GetRecordings(NfOnvif *self, int fd, char *buff_rcv) {
#endif
	//declaration here.
	arg_GetRecordingsResponseItem tmp;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	int i=0, row=0, col=0;
	int cnt ;
	//declaration end.	
#if defined(WIN32)

	memcpy(&tmp, tmp_, size);
#else
	ONVIF_PACKET packet;
    ONVIF_HEADER header;
	guint packet_len;
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));
	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);
	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));
#endif
	

	

	//_TTY_LOG_ONVIF("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);



	onvif_cross_util_test(3);
	
	// 1. ???ڵ? ???ٰ˻?

	// 2. track�� Ʈ?? ??ū�� 3?????? ?Ѵ?.
	//	tv0, av0, mv0
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) { 
	//	if(checkRecordingEmpty(i)) continue;
		_TTY_LOG_ONVIF("Entry: %s (i : %d)",__FUNCTION__, i);
		sprintf(buff, "onvif.recording%d.token", i);
		strncpy(tmp.RecordingToken[i], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		_TTY_LOG_ONVIF("token: %s (i : %d)",nf_sysdb_get_str_nocopy(buff), i);
		_TTY_LOG_ONVIF("rtoken: %s (i : %d)",tmp.RecordingToken[i], i);

		sprintf(buff, "onvif.recording%d.id", i);
		strncpy(tmp.recordingConf[i].Source.SourceId, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.recording%d.name", i);
		strncpy(tmp.recordingConf[i].Source.Name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp.recordingConf[i].Source.Name: %s (i : %d)",tmp.recordingConf[i].Source.Name, i);
		sprintf(buff, "onvif.recording%d.location", i);
		strncpy(tmp.recordingConf[i].Source.Location, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.recording%d.desc", i);
		strncpy(tmp.recordingConf[i].Source.Description, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.recording%d.address", i);
		strncpy(tmp.recordingConf[i].Source.Address, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.recording%d.content", i);
		strncpy(tmp.recordingConf[i].Content, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.recording%d.max_retention_time", i);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		 tmp.recordingConf[i].MaximumRetentionTime = (int)nf_sysdb_get_uint(buff);
		 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		  cnt  = ONVIF_TRACK_CNT * i;
		 sprintf(buff, "%s%d",VIDEO_TRACK, i);
		 strncpy(tmp.track[cnt].TrackToken,buff,COMMON_SIZE - 1);
		 sprintf(buff, "%s%d",AUDIO_TRACK, i);
		 strncpy(tmp.track[cnt+1].TrackToken,buff,COMMON_SIZE - 1);
//		 sprintf(buff, "%s%d",METADATA_TRACK, i);
	//	 strncpy(tmp.track[cnt+2].TrackToken,buff,COMMON_SIZE - 1);

		 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	}
	//tmp.RecordingToken
	
#if defined(WIN32)
		memcpy(tmp_, &tmp, size);
#else
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}	
#endif
	return ONVIF_ERR_RET_SUCCESS;
}



#if defined(WIN32)
int onvif_GetRecordingOptions(void* tmp_, int size) {
#else
int onvif_GetRecordingOptions(NfOnvif *self, int fd, char *buff_rcv) {

#endif
	//declaration here.
	arg_RecordingOptionsItem tmp;
	int i=0,recordingTokenExist=0,recordingTokenJobEmpty=0,result;
	char name[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	char* value;
	gchar ** splitbuf;

	//declaration end.
	#if defined(WIN32)
	memcpy(&tmp, tmp_, size);
	#else
		ONVIF_PACKET packet;
		ONVIF_HEADER header;
		guint packet_len;
		memset(&header, 0, sizeof(header));
		memcpy(&header, buff_rcv, sizeof(header));

		packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RecordingOptionsItem);

		memcpy(&packet, buff_rcv, packet_len);
		memcpy(&tmp, packet.data, sizeof(arg_RecordingOptionsItem));
	#endif


	

	if(checkRecordingExist(tmp.RecordingToken) == ONVIF_R_ERR_NO_RECORDING) {
		return ONVIF_R_ERR_NO_RECORDING;
	}
	
	
		

	// get compatible source
		
		splitbuf = g_strsplit_set(tmp.RecordingToken,"s",0);	
		sprintf(buff,"p%s",splitbuf[1]);
		strncpy(tmp.CompatibleSources,buff, COMMON_SIZE-1);
		recordingTokenJobEmpty = checkRecordingEmpty(atoi(splitbuf[1]));
		g_strfreev(splitbuf);
		
		
	if(recordingTokenJobEmpty) {
		tmp.JobSpare = 1;
	}else {
		tmp.JobSpare = 0;
	}
	tmp.TrackSpareAudio = 0;
	tmp.TrackSpareMetadata = 0;
	tmp.TrackSpareTotal =0;
	tmp.TrackSpareVideo =0;

	 
	#if defined (WIN32)
	memcpy(tmp_, &tmp, size);
#else
            memcpy(packet.data, &tmp, sizeof (arg_RecordingOptionsItem));
             if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
                                    header.code, sizeof (arg_RecordingOptionsItem)) != ONVIF_ERR_RET_SUCCESS) {
                         return ONVIF_ERR_RET_INTERNAL;
            }           
#endif

	

	return ONVIF_ERR_RET_SUCCESS;
}
#if defined (WIN32)
int onvif_GetRecordingJobs(void * tmp_, int size) {
#else
int onvif_GetRecordingJobs(NfOnvif *self, int fd, char *buff_rcv) {
		
#endif
             //declaration here.
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	arg_getRecordingJobsResponseItem tmp;
	             //declaration end.
	    #if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_getRecordingJobsResponseItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_getRecordingJobsResponseItem));
    #endif


	

	// in here you should put the custom code.
/*
'<item key="onvif.recordingjob0.token"       type="STRING"      min="0" max="64" val="jt0" />'
'<item key="onvif.recordingjob0.recording_token"       type="STRING"      min="0" max="64" val="rs0" />'
'<item key="onvif.recordingjob0.mode"       type="UINT"      min="0" max="4096" val="" />'
'<item key="onvif.recordingjob0.priority"       type="UINT"      min="0" max="4096" val="" />'
 */
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {

		sprintf(buff,"onvif.recordingjob%d.source_token",i);
		if(!strcmp(nf_sysdb_get_str_nocopy(buff), ""))
		{
			continue;
		}
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (i : %d)",__FUNCTION__, i);
		sprintf(buff, "onvif.recordingjob%d.token", i);
		strncpy(tmp.job[i].JobToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
						_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		sprintf(buff, "onvif.recordingjob%d.recording_token", i);
		strncpy(tmp.job[i].conf.RecordingToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		sprintf(buff, "onvif.recordingjob%d.mode", i);


		getJobModeString(nf_sysdb_get_uint(buff),tmp.job[i].conf.RecordingJobMode);
		_TTY_LOG_ONVIF_DEBUG("@ tmp.job[i].conf.RecordingJobMode: %s (nf_sysdb_get_uint : %d)",tmp.job[i].conf.RecordingJobMode, nf_sysdb_get_uint(buff));
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		sprintf(buff, "onvif.recordingjob%d.priority", i);
		tmp.job[i].conf.Priority = (int)nf_sysdb_get_uint(buff);
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		recordingJobSource(tmp.job[i].JobToken,tmp.job[i].conf.source);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	}


#if defined (WIN32)
                        memcpy(tmp_, &tmp, size);
#else
            memcpy(packet.data, &tmp, sizeof (arg_getRecordingJobsResponseItem));
             if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
                                    header.code, sizeof (arg_getRecordingJobsResponseItem)) != ONVIF_ERR_RET_SUCCESS) {
                         return ONVIF_ERR_RET_INTERNAL;
            }           
#endif



	return ONVIF_ERR_RET_SUCCESS;
}


	
#if defined (WIN32)
int onvif_DeleteRecordingJob(void * tmp_, int size) {
#else
int onvif_DeleteRecordingJob(NfOnvif *self, int fd, char *buff_rcv) {

#endif
             //declaration here.
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	char value[COMMON_SIZE] = {0,};
	arg_JobItem tmp;
	             //declaration end.
	    #if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_JobItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_JobItem));
    #endif


			printf("%s" , tmp.JobToken);

			// type the code here
			sprintf(value, "onvif.recordingjob%d.source_token", i);
			for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
				sprintf(buff, "onvif.recordingjob%d.token", i);
				printf("%s" , nf_sysdb_get_str_nocopy(buff));
				if(!strcmp(tmp.JobToken,nf_sysdb_get_str_nocopy(buff)))
				{
					nf_sysdb_set_str(value,"");
					break;
				}
			}

			if( i == ONVIF_MAX_RECORDING_CNT) {
				return ONVIF_R_ERR_INVALID_PARAM;
			}

			

			#if defined (WIN32)
						memcpy(tmp_, &tmp, size);
			#else
						memcpy(packet.data, &tmp, sizeof (arg_JobItem));
						 if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
												header.code, sizeof (arg_JobItem)) != ONVIF_ERR_RET_SUCCESS) {
									 return ONVIF_ERR_RET_INTERNAL;
						}           
			#endif



	return ONVIF_ERR_RET_SUCCESS;

}

#if defined(WIN32)
int onvif_CreateRecordingJob(void* tmp_, int size)
#else 
int onvif_CreateRecordingJob(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		struct e_recording_type recording;
		arg_JobItem tmp;
		char buff[COMMON_SIZE] = { 0, };
		int i,mode;

	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_JobItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_JobItem));
    #endif
			// find empty recording token
			
			for(i=0;i < ONVIF_MAX_RECORDING_CNT;i++) {
				sprintf(buff, "onvif.recordingjob%d.source_token", i);
				if(!strcmp(nf_sysdb_get_str_nocopy(buff),""))
				{
					nf_sysdb_set_str(buff, tmp.conf.source[0].SourceToken);
					sprintf(buff, "onvif.recordingjob%d.token", i);
					strncpy(tmp.JobToken, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE-1);

					
					sprintf(buff, "onvif.recordingjob%d.priority", i);
					nf_sysdb_set_uint(buff,tmp.conf.Priority);

					sprintf(buff, "onvif.recordingjob%d.mode", i);
					
					
					recording.token = i;
					recording.recordingToken = i;
					recording.value = SetPanicRecording(i, tmp.conf.RecordingJobMode);
					append_onvif_event_msg(E_KEY_RECORDING_JOB_STATE_CHANGE, &recording, E_Initialized, 0);
				//	SetPanicRecording(i, tmp.conf.RecordingJobMode);
					//nf_sysdb_set_uint(buff,mode);
					
					
						break;
				}
			}

			if(i == ONVIF_MAX_RECORDING_CNT) {
				return ONVIF_R_ERR_NO_RECORDINGJOBS;
			}

			






	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_JobItem));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_JobItem)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}


#if defined(WIN32)
int onvif_GetCapability(void* tmp_, int size)
#else 
int onvif_GetCapability(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
	printf("%s In\n", __FUNCTION__);
		arg_GetCapa tmp;
		int ch = -1;
		int i=0;

	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_GetCapa);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_GetCapa));
    #endif

	if(!strncmp(tmp.token, "ALL", COMMON_SIZE - 1))
	{
		for(i=0;i<ONVIF_CH;i++)
		{
			GetCapability(&tmp, i);
		}
	}
	else
	{
		ch = isValidVideoSourceToken(tmp.token);
		if(ch == -1)
		{
			ch = isValidProfileToken(tmp.token);
		}
		if(ch == -1)
		{
			ch = isValidPtzToken(tmp.token);
		}
		if(ch == -1)
		{
			ch = isValidPtzNodeToken(tmp.token);
		}
		if(ch != -1)
		{
			GetCapability(&tmp, ch);
			tmp.ch = ch;
		}
		else
		{
			printf("\e[31m #### NOT INVALID TOKEN[%s] #####\e[0m\n", tmp.token);
		}
	}
	
	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_GetCapa));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_GetCapa)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif

	return ONVIF_ERR_RET_SUCCESS;
}

#if defined(WIN32)
int onvif_GetDeviceInformation(void* tmp_, int size)
#else 
int onvif_GetDeviceInformation(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_GetDeviceInformation tmp;


	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
			snprintf(tmp.Model, sizeof(char)*COMMON_SIZE, "%s", "ONVIF SIMULATOR");
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_GetDeviceInformation);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_GetDeviceInformation));

			// gchar vendor[NF_SYSMAN_HW_PARAM_MAX_LEN]={0,};
			// nf_sysman_get_hw_param_vendor(vendor);

			if(strcmp(nf_sysdb_get_str_nocopy("sys.info.model"), "") == 0)
        		snprintf(tmp.Model, sizeof(char)*COMMON_SIZE, "%s", get_onvif_model_name());
        	else
        		snprintf(tmp.Model, sizeof(char)*COMMON_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.model"));
    #endif
		snprintf(tmp.FirmwareVersion, sizeof(char)*COMMON_SIZE, "%s", g_strstrip(nf_sysdb_get_str_nocopy("sys.info.swver")));
		snprintf(tmp.HardwareId, sizeof(char)*COMMON_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.model"));
		snprintf(tmp.Manufacturer, sizeof(char)*COMMON_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.vendor"));
		
		snprintf(tmp.SerialNumber, sizeof(char)*COMMON_SIZE, "%s", 	nf_sysdb_get_str_nocopy("sys.info.mac_colon"));
		//snprintf(tmp.SerialNumber, sizeof(char)*COMMON_SIZE, "%s", vendor);


	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_GetDeviceInformation));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_GetDeviceInformation)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}



	
#if defined(WIN32)
int onvif_GetRecordingJobState(void* tmp_, int size)
#else 
int onvif_GetRecordingJobState(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_JobItem tmp;
		char buff[COMMON_SIZE] = {0,};
		int i=0;
		int mode;
	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_JobItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_JobItem));
    #endif

		for(i=0;i < ONVIF_MAX_RECORDING_CNT;i++) {
			sprintf(buff, "onvif.recordingjob%d.token", i);
			if(!strcmp(nf_sysdb_get_str_nocopy(buff),tmp.JobToken))
			{
				sprintf(buff, "onvif.recordingjob%d.recording_token", i);
				strncpy(tmp.conf.RecordingToken, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE-1);
				GetPanicRecording(i,buff);
				strncpy(tmp.conf.RecordingJobMode,buff, COMMON_SIZE-1);
				
				sprintf(buff, "onvif.recordingjob%d.source_token", i);
				strncpy(tmp.conf.source[0].SourceToken , nf_sysdb_get_str_nocopy(buff),COMMON_SIZE-1);
				strncpy(tmp.conf.source[0].SourceTokenType , "http://www.onvif.org/ver10/schema/Profile",COMMON_SIZE-1);
				strncpy(tmp.conf.source[0].state,tmp.conf.RecordingJobMode,COMMON_SIZE-1);
				strncpy(tmp.conf.source[0].track[0].state,tmp.conf.RecordingJobMode,COMMON_SIZE-1);
				strncpy(tmp.conf.source[0].track[1].state,tmp.conf.RecordingJobMode,COMMON_SIZE-1);
				break;
			}
		}

		
	if(i==ONVIF_MAX_RECORDING_CNT) {
				return ONVIF_R_ERR_NO_RECORDINGJOBS;
	}


	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_JobItem));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_JobItem)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}




#if defined(WIN32)
int GetRecordingSummary(void* tmp_, int size)
#else 
int GetRecordingSummary(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_RecordingSummary tmp;
		NF_DISK_REC_TIME        nrec_time;	
		arg_DeviceRecordingSummary sum;
		int ret=0;
	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_RecordingSummary);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_RecordingSummary));
    #endif
			// -1 �̸� ��� ä���� ���ڵ� �ð��� summary �ؼ� ������
			ret = GetDeviceRecordingSummary(&sum,-1);
			if( !ret)	
				return ONVIF_ERR_RET_INTERNAL;

			tmp.DataFrom = (int)sum.dataFrom;
			tmp.DataUntil = (int)sum.dataUntil;
			//FIXME. 
			tmp.NumberRecordings	= sum.recordingNum;

	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_RecordingSummary));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_RecordingSummary)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}
static void recording_search_token_keep_alive_mng(void *arg) 
{
	char buff[COMMON_SIZE] = {0,};
	int token_idx = 0;
		
	pthread_mutex_init(&srt_mutex, NULL);

	while(1)
	{
		
		for(token_idx = 0; token_idx < MAX_REC_SEARCH_SESSION; token_idx++)
		{				
				if(search_token[token_idx] == 0) continue; // �������� �ʴ� search token��.
				search_token[token_idx] -= 1000;
				printf("search_token[%d] = %d\n", token_idx, search_token[token_idx]);
				if(search_token[token_idx] == 0) {
					sprintf(buff, "srt%d",search_token[token_idx]);
					pthread_mutex_lock(&srt_mutex);
					g_hash_table_remove(g_rec_hash_table,buff);
					pthread_mutex_unlock(&srt_mutex);
					search_token[token_idx] = 0;
				}


		}
		
		g_usleep(1000000);
	}
}
void set_end_search_token(int token_idx, int value)
{
	/* '1' means that EndSearch occurs for the SearchToken */
	pthread_mutex_init(&end_srt_mutex, NULL);

	pthread_mutex_lock(&end_srt_mutex);
	end_search_token[token_idx] = value;
	pthread_mutex_unlock(&end_srt_mutex);
}

static int recording_search_token_new(int keepalive)
{
	static int is_first = 1;
	int token_idx = 0;
	pthread_t tid;

	pthread_mutex_init(&srt_mutex, NULL);

	if(is_first)
	{
		
		if (pthread_create(&tid, NULL, (void*) recording_search_token_keep_alive_mng, NULL) != 0)
		{
			printf("recording_search_token_keep_alive_mng thread create error\n");
			return -1;
		}
		is_first = 0;
	}

	pthread_mutex_lock(&srt_mutex);
	for(token_idx = 0; token_idx < MAX_REC_SEARCH_SESSION; token_idx++)
	{
		
		if(search_token[token_idx] == 0)
		{
			search_token[token_idx] = keepalive;
			set_end_search_token(token_idx, 0);
			pthread_mutex_unlock(&srt_mutex);
			return token_idx;
		}
		
	}
	
	pthread_mutex_unlock(&srt_mutex);
	
	return -1;
}
static void _start_metadata_search(void *arg) 
{

			char* token ,* filter;
			int pass =0;
			arg_FindRecordings * fre, *a;
			arg_recordingInfo info[MAX_MSG_BUF];
			recordingEvent_info* ri,*gs,*loopRi;	
			int aa;
			int current_time,filter_count=0;
			int i=0,max_count=0,filterEnum=0;
			unsigned int j=0;
			token =  (char *) arg;
			fre = (arg_FindRecordings *)g_hash_table_lookup(g_rec_hash_table,token);
			g_free(token);
#if defined(WIN32) 
			current_time = (int)time(NULL);

			pthread_mutex_init(&ri_mutex, NULL);

			if(!eve_arr) eve_arr =  g_array_sized_new( FALSE, FALSE, sizeof(recordingEvent_info), 200 );
			//ri.start_time 
			if(fre->MaxMatches>0) max_count = fre->MaxMatches;
			else max_count = ONVIF_CH;

			onvif_recording_metadata(fre->Scope.Recording, fre->MaxMatches,filterEnum,fre->StartPoint,fre->EndPoint);
			


#else
#if 0 // host refactoring
	int ret = 0, ch=0, cnt=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(ONVIF_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[MAX_LOG_CNT];	
	ONVIF_LIVE_STATUS	status;
	
	recording_info rec_info[ONVIF_MAX_RECORDING_CNT];

	memset(&filter, 0x00, sizeof(search_filter));
	memcpy(&filter, (search_filter *) arg, sizeof(search_filter));
	memset(rec_info, 0x00, sizeof(recording_info)*ONVIF_MAX_RECORDING_CNT);

	/* find recording informations of each channels */
	/* Find Start Time */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME; 
	filter.param.direction = 0; // Foword
	filter.param.type_mask = LT_MASK_RECORD_STARTED;
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
		//return WEBBASE_ERR_RET_INTERNAL;
	}
	for(ch=0; ch<ONVIF_CH; ch++){
		for(i=0; i<(int)log_header->result_count; i++){
			if( ch == (int)elem[i].param1 ){
				rec_info[cnt].ch = (char)ch;
				rec_info[cnt].start_time = elem[i].timestamp;
				cnt++;
				break;
			}
		}
	}
	
	while( log_header->result_count == MAX_LOG_CNT){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 0; // Foword
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STARTED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_start_time((char)ch, rec_info, cnt ) == 0){
					rec_info[cnt].ch = (char)ch;
					rec_info[cnt].start_time = elem[i].timestamp;
					cnt++;
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
		//baek.debug

	}

	/* Searching start point is complete.  Find End Time */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME;
	filter.param.direction = 1; // Backward
	filter.param.type_mask = LT_MASK_RECORD_STOPPED;
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
		//return WEBBASE_ERR_RET_INTERNAL;
	}
	for(ch=0; ch<ONVIF_CH; ch++){
		for(i=0; i<(int)log_header->result_count; i++){
			if( ch == (int)elem[i].param1 ){
				/* Find Strat time and put End time */
				ret = put_rec_info_endtime((char)ch, rec_info, elem[i].timestamp, cnt);
				if( !ret ){
					/* There is no Strat time. Add rec_info */
					rec_info[cnt].ch = (char)ch;					
					rec_info[cnt].start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
					rec_info[cnt].end_time = elem[i].timestamp;
					cnt++;	
				}
				break;
			}
		}
		if( cnt == ONVIF_CH )	break;
	}
	//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
	_TTY_LOG_ONVIF("cnt[%d]", cnt);
	for(i=0; i<cnt; i++){
		_TTY_LOG_ONVIF("ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}
#endif //]] Jeonghun_0130913_END -- del
	
	while( log_header->result_count == MAX_LOG_CNT ){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 1; // Backward
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STOPPED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_end_time((char)ch, rec_info, cnt ) == 0){
					/* Find Strat time and put End time */
					ret = put_rec_info_endtime((char)ch, rec_info, elem[i].timestamp, cnt);
					if( !ret ){
						/* There is no Strat time. Add rec_info */
						rec_info[cnt].ch = (char)ch;
						rec_info[cnt].start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
						rec_info[cnt].end_time = elem[i].timestamp;
						cnt++;
					}
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
		//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
		for(i=0; i<ONVIF_CH; i++){
			_TTY_LOG_ONVIF("cont... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
		}
#endif //]] Jeonghun_0130913_END -- del
	}

	/* Only Start Time, Set End Time to search end time */
	status = nf_webbase_live_status_cstack_get_interval_read(3, 0);
	
	for(i=0; i<cnt; i++){
		for(ch=0; ch<ONVIF_CH; ch++){
			if( rec_info[i].ch == ch && status.act_recording[ch] != ' ' ){
				rec_info[i].end_time = GTIMEVAL_TO_GUINT64(filter.param.time_end);
			}
		}
	}
	
	for(i=0; i<ONVIF_CH; i++){
		_TTY_LOG_ONVIF("complete... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}

	/* Put searched recording info */
	unsigned	int srt = 0;
	srt = (unsigned int)atoi(filter.search_token+3);
	_TTY_LOG_ONVIF("PUSH SHM token[%s] [%d] cnt[%d] max[%d]", filter.search_token, srt, cnt, filter.max_matches);

	/* limit MaxMatches */
	if( (int)filter.max_matches < cnt && filter.max_matches != 0){
		cnt = (int)filter.max_matches;
	}
	
	for(i=0; i<cnt; i++){
		/* The last msg must have Search Complete State!! */
		push_rec_info_shm(srt, &rec_info[i]);
	}
	
	set_search_rec_state_shm(srt, SEARCH_STATE_COMPLETED);	
#endif	
#endif

	
	fre->SearchState = SEARCH_STATE_COMPLETED;
	
	
}
static void _start_event_search(void *arg) 
{

			char* token ,* filter;
			int pass =0;
			arg_FindRecordings * fre, *a;
			arg_recordingInfo info[MAX_MSG_BUF];
			recordingEvent_info* ri,*gs,*loopRi;	
			int aa;
			int current_time,filter_count=0;
			int i=0,max_count=0,filterEnum=0;
			unsigned int j=0;
			token =  (char *) arg;
			fre = (arg_FindRecordings *)g_hash_table_lookup(g_rec_hash_table,token);
			g_free(token);
#if defined(WIN32) 
			current_time = (int)time(NULL);

			pthread_mutex_init(&ri_mutex, NULL);

//			g_mutex_lock(ri_mutex);
			if(!eve_arr) eve_arr =  g_array_sized_new( FALSE, FALSE, sizeof(recordingEvent_info), 200 );
			//ri.start_time 
			if(fre->MaxMatches>0) max_count = fre->MaxMatches;
			else max_count = ONVIF_CH;
			if(strstr("tns1:RecordingHistory/Recording/State",fre->Scope.Filter)) {
				filterEnum = ONVIF_RECORDING_STATUS;
			}else if(strstr("tns1:RecordingHistory/Track/State",fre->Scope.Filter)) {
				filterEnum = ONVIF_RECORDING_CLIP;
			}else if(strstr("",fre->Scope.Filter)) {
				filterEnum = ONVIF_RECORDING_ALL;
			}
			onvif_recording_event(fre->Scope.Recording, fre->MaxMatches,filterEnum,fre->StartPoint,fre->EndPoint);


#else
#if 0 // host refactoring
	int ret = 0, ch=0, cnt=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(ONVIF_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[MAX_LOG_CNT];	
	ONVIF_LIVE_STATUS	status;
	
	recording_info rec_info[ONVIF_MAX_RECORDING_CNT];

	memset(&filter, 0x00, sizeof(search_filter));
	memcpy(&filter, (search_filter *) arg, sizeof(search_filter));
	memset(rec_info, 0x00, sizeof(recording_info)*ONVIF_MAX_RECORDING_CNT);

	/* find recording informations of each channels */
	/* Find Start Time */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME; 
	filter.param.direction = 0; // Foword
	filter.param.type_mask = LT_MASK_RECORD_STARTED;
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
		//return WEBBASE_ERR_RET_INTERNAL;
	}
	for(ch=0; ch<ONVIF_CH; ch++){
		for(i=0; i<(int)log_header->result_count; i++){
			if( ch == (int)elem[i].param1 ){
				rec_info[cnt].ch = (char)ch;
				rec_info[cnt].start_time = elem[i].timestamp;
				cnt++;
				break;
			}
		}
	}
	
	while( log_header->result_count == MAX_LOG_CNT){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 0; // Foword
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STARTED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_start_time((char)ch, rec_info, cnt ) == 0){
					rec_info[cnt].ch = (char)ch;
					rec_info[cnt].start_time = elem[i].timestamp;
					cnt++;
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
		//baek.debug

	}

	/* Searching start point is complete.  Find End Time */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME;
	filter.param.direction = 1; // Backward
	filter.param.type_mask = LT_MASK_RECORD_STOPPED;
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
		//return WEBBASE_ERR_RET_INTERNAL;
	}
	for(ch=0; ch<ONVIF_CH; ch++){
		for(i=0; i<(int)log_header->result_count; i++){
			if( ch == (int)elem[i].param1 ){
				/* Find Strat time and put End time */
				ret = put_rec_info_endtime((char)ch, rec_info, elem[i].timestamp, cnt);
				if( !ret ){
					/* There is no Strat time. Add rec_info */
					rec_info[cnt].ch = (char)ch;					
					rec_info[cnt].start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
					rec_info[cnt].end_time = elem[i].timestamp;
					cnt++;	
				}
				break;
			}
		}
		if( cnt == ONVIF_CH )	break;
	}
	//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
	_TTY_LOG_ONVIF("cnt[%d]", cnt);
	for(i=0; i<cnt; i++){
		_TTY_LOG_ONVIF("ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}
#endif //]] Jeonghun_0130913_END -- del
	
	while( log_header->result_count == MAX_LOG_CNT ){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 1; // Backward
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STOPPED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_end_time((char)ch, rec_info, cnt ) == 0){
					/* Find Strat time and put End time */
					ret = put_rec_info_endtime((char)ch, rec_info, elem[i].timestamp, cnt);
					if( !ret ){
						/* There is no Strat time. Add rec_info */
						rec_info[cnt].ch = (char)ch;
						rec_info[cnt].start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
						rec_info[cnt].end_time = elem[i].timestamp;
						cnt++;
					}
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
		//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
		for(i=0; i<ONVIF_CH; i++){
			_TTY_LOG_ONVIF("cont... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
		}
#endif //]] Jeonghun_0130913_END -- del
	}

	/* Only Start Time, Set End Time to search end time */
	status = nf_webbase_live_status_cstack_get_interval_read(3, 0);
	
	for(i=0; i<cnt; i++){
		for(ch=0; ch<ONVIF_CH; ch++){
			if( rec_info[i].ch == ch && status.act_recording[ch] != ' ' ){
				rec_info[i].end_time = GTIMEVAL_TO_GUINT64(filter.param.time_end);
			}
		}
	}
	
	for(i=0; i<ONVIF_CH; i++){
		_TTY_LOG_ONVIF("complete... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}

	/* Put searched recording info */
	unsigned	int srt = 0;
	srt = (unsigned int)atoi(filter.search_token+3);
	_TTY_LOG_ONVIF("PUSH SHM token[%s] [%d] cnt[%d] max[%d]", filter.search_token, srt, cnt, filter.max_matches);

	/* limit MaxMatches */
	if( (int)filter.max_matches < cnt && filter.max_matches != 0){
		cnt = (int)filter.max_matches;
	}
	
	for(i=0; i<cnt; i++){
		/* The last msg must have Search Complete State!! */
		push_rec_info_shm(srt, &rec_info[i]);
	}
	
	set_search_rec_state_shm(srt, SEARCH_STATE_COMPLETED);	
#endif	
#endif

	
	fre->SearchState = SEARCH_STATE_COMPLETED;
	
	
}

static void _start_recording_search(void *arg) 
{

			char* token ,* filter;
			int pass =0;
			arg_FindRecordings * fre, *a;
			recording_info* ri,*gs,*loopRi;	
			int aa;
			int current_time,filter_count=0;
			int i=0,max_count=0;
			unsigned int j=0;
			token =  (char *) arg;
			fre = (arg_FindRecordings *)g_hash_table_lookup(g_rec_hash_table,token);
			g_free(token);
#if defined(WIN32) 
			current_time = (int)time(NULL);

			pthread_mutex_init(&ri_mutex, NULL);

			pthread_mutex_lock(&ri_mutex);
			if(!rec_arr) rec_arr =  g_array_sized_new( FALSE, FALSE, sizeof(recording_info), 200 );
			//ri.start_time 
			if(fre->MaxMatches>0) max_count = fre->MaxMatches;
			else max_count = ONVIF_CH;

			for(i=0; i<max_count;i++)
			{

				ri = g_new (recording_info, 1);
				ri->end_time = 1427788559;
				ri->start_time = 1427778559;
				ri->ch = i;
				
				
				
				if(!strcmp(fre->Scope.Filter,""))
				{
					pass =1;
				}
				filter = strstr(fre->Scope.Filter,"Video");
				if(pass || filter) {
					ri->track[filter_count].start_time = 1427778559;
					ri->track[filter_count].end_time = 1427788559;
					ri->track[filter_count].tracktype = 0;
					filter_count++;

				}
				
				filter = strstr(fre->Scope.Filter,"Audio");
				if(pass || filter) {
					ri->track[filter_count].start_time = 1427778559;
					ri->track[filter_count].end_time = 1427788559;
					ri->track[filter_count].tracktype = 1;
					filter_count++;
				}

				filter = strstr(fre->Scope.Filter,"Metadata");
				if(pass || filter) {
					ri->track[filter_count].start_time = 1427778559;
					ri->track[filter_count].end_time = 1427788559;
					ri->track[filter_count].tracktype = 2;
					filter_count++;
				}
				ri->track_cnt = filter_count;
				filter_count=0;
				for(j=0; j < rec_arr->len ; j++) {
					loopRi = &g_array_index (rec_arr, recording_info, 0);
					if(ri->ch == loopRi->ch) {
						if(ri->end_time < loopRi->end_time) {
							ri->end_time = loopRi->end_time;
						}
						if(ri->start_time > loopRi->start_time) {
							ri->start_time = loopRi->start_time;
						}
						g_free(ri);
						continue;
					}

				}
				g_array_append_val(rec_arr, *ri);
			}
			
			pthread_mutex_unlock(&ri_mutex);

			aa=0;
#else
#if 0 // host refactoring
	int ret = 0, ch=0, cnt=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(ONVIF_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[MAX_LOG_CNT];	
	ONVIF_LIVE_STATUS	status;
	
	recording_info rec_info[ONVIF_MAX_RECORDING_CNT];

	memset(&filter, 0x00, sizeof(search_filter));
	memcpy(&filter, (search_filter *) arg, sizeof(search_filter));
	memset(rec_info, 0x00, sizeof(recording_info)*ONVIF_MAX_RECORDING_CNT);

	/* find recording informations of each channels */
	/* Find Start Time */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME; 
	filter.param.direction = 0; // Foword
	filter.param.type_mask = LT_MASK_RECORD_STARTED;
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
		//return WEBBASE_ERR_RET_INTERNAL;
	}
	for(ch=0; ch<ONVIF_CH; ch++){
		for(i=0; i<(int)log_header->result_count; i++){
			if( ch == (int)elem[i].param1 ){
				rec_info[cnt].ch = (char)ch;
				rec_info[cnt].start_time = elem[i].timestamp;
				cnt++;
				break;
			}
		}
	}
	
	while( log_header->result_count == MAX_LOG_CNT){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 0; // Foword
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STARTED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_start_time((char)ch, rec_info, cnt ) == 0){
					rec_info[cnt].ch = (char)ch;
					rec_info[cnt].start_time = elem[i].timestamp;
					cnt++;
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
		//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
		for(i=0; i<ONVIF_CH; i++){
			_TTY_LOG_ONVIF("cont... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
		}
#endif //]] Jeonghun_0130913_END -- del
	}

	/* Searching start point is complete.  Find End Time */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME;
	filter.param.direction = 1; // Backward
	filter.param.type_mask = LT_MASK_RECORD_STOPPED;
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
		//return WEBBASE_ERR_RET_INTERNAL;
	}
	for(ch=0; ch<ONVIF_CH; ch++){
		for(i=0; i<(int)log_header->result_count; i++){
			if( ch == (int)elem[i].param1 ){
				/* Find Strat time and put End time */
				ret = put_rec_info_endtime((char)ch, rec_info, elem[i].timestamp, cnt);
				if( !ret ){
					/* There is no Strat time. Add rec_info */
					rec_info[cnt].ch = (char)ch;					
					rec_info[cnt].start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
					rec_info[cnt].end_time = elem[i].timestamp;
					cnt++;	
				}
				break;
			}
		}
		if( cnt == ONVIF_CH )	break;
	}
	//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
	_TTY_LOG_ONVIF("cnt[%d]", cnt);
	for(i=0; i<cnt; i++){
		_TTY_LOG_ONVIF("ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}
#endif //]] Jeonghun_0130913_END -- del
	
	while( log_header->result_count == MAX_LOG_CNT ){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 1; // Backward
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STOPPED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_end_time((char)ch, rec_info, cnt ) == 0){
					/* Find Strat time and put End time */
					ret = put_rec_info_endtime((char)ch, rec_info, elem[i].timestamp, cnt);
					if( !ret ){
						/* There is no Strat time. Add rec_info */
						rec_info[cnt].ch = (char)ch;
						rec_info[cnt].start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
						rec_info[cnt].end_time = elem[i].timestamp;
						cnt++;
					}
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
		//baek.debug
#if 0 //[[ Jeonghun_0130913_BEGIN -- del
		for(i=0; i<ONVIF_CH; i++){
			_TTY_LOG_ONVIF("cont... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
		}
#endif //]] Jeonghun_0130913_END -- del
	}

	/* Only Start Time, Set End Time to search end time */
	status = nf_webbase_live_status_cstack_get_interval_read(3, 0);
	
	for(i=0; i<cnt; i++){
		for(ch=0; ch<ONVIF_CH; ch++){
			if( rec_info[i].ch == ch && status.act_recording[ch] != ' ' ){
				rec_info[i].end_time = GTIMEVAL_TO_GUINT64(filter.param.time_end);
			}
		}
	}
	
	for(i=0; i<ONVIF_CH; i++){
		_TTY_LOG_ONVIF("complete... ch[%u] start[%lld] end[%lld]", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}

	/* Put searched recording info */
	unsigned	int srt = 0;
	srt = (unsigned int)atoi(filter.search_token+3);
	_TTY_LOG_ONVIF("PUSH SHM token[%s] [%d] cnt[%d] max[%d]", filter.search_token, srt, cnt, filter.max_matches);

	/* limit MaxMatches */
	if( (int)filter.max_matches < cnt && filter.max_matches != 0){
		cnt = (int)filter.max_matches;
	}
	
	for(i=0; i<cnt; i++){
		/* The last msg must have Search Complete State!! */
		push_rec_info_shm(srt, &rec_info[i]);
	}
	
	set_search_rec_state_shm(srt, SEARCH_STATE_COMPLETED);	
#endif	
#endif

	
	fre->SearchState = SEARCH_STATE_COMPLETED;
	
	
}



void
free_data (gpointer data)
{
  printf ("freeing: %s %p\n", (char *) data, data);
  free (data);
}

#if defined(WIN32)
int onvif_FindRecordings(void* tmp_, int size)
#else 
int onvif_FindRecordings(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_FindRecordings tmp,* fre;
		int search_token_i=0;
		char* token;
		char* filter;
		pthread_t id;

		
	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_FindRecordings);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_FindRecordings));
    #endif
	_TTY_LOG_ONVIF("tmp.KeepAliveTime: %d", tmp.KeepAliveTime);
	_TTY_LOG_ONVIF("tmp.Scope.Filter: %s", tmp.Scope.Filter);
	_TTY_LOG_ONVIF("tmp.Scope.sizeSource: %d", tmp.Scope.sizeSource);
	_TTY_LOG_ONVIF("tmp.Scope.sizeRecording: %d", tmp.Scope.sizeRecording);

	// FIXME.Get Search Token
	
	search_token_i = recording_search_token_new(tmp.KeepAliveTime);
	if(search_token_i < 0)
	{
		_TTY_LOG_ONVIF("Recording search session is full.\n");
		return ONVIF_R_ERR_NO_SOURCE;
	}
	
	
	sprintf(tmp.SearchToken, "srt%d", search_token_i);
	if(!g_rec_hash_table) g_rec_hash_table  = g_hash_table_new_full(g_str_hash, g_str_equal,g_free,g_free);
	fre = g_new(arg_FindRecordings,1);	
	
	memcpy(fre, &tmp,sizeof(arg_FindRecordings));

	token = g_new(char,COMMON_SIZE);	
	strncpy(token, tmp.SearchToken, COMMON_SIZE-1);

	fre->SearchState = SEARCH_STATE_SEARCHING;
	g_hash_table_insert(g_rec_hash_table,token,fre);	
	

	// dev �� search_filter�� enable�ϴ� �κ�
	// Start Recording Search Session
//	memset(&onvif_search_filter, 0x00, sizeof(search_filter));
//	_recording_search_filter_init(&tmp, &onvif_search_filter);
	token = g_new (char, COMMON_SIZE);
	strncpy(token,tmp.SearchToken,COMMON_SIZE-1);

	
	if(tmp.FindType == FIND_RECORDING) {
		if (pthread_create(&id, NULL,  (void*)_start_recording_search, (void*)token) != 0)
		{
			recording_search_token_expire(search_token_i);
			printf("thread create error\n");
			return -1;
		}
	}else if (tmp.FindType == FIND_EVENT) {
		if (pthread_create(&id, NULL,  (void*)_start_event_search, (void*)token) != 0)
		{
			recording_search_token_expire(search_token_i);
			printf("thread create error\n");
			return -1;
		}
	}else {
		if (pthread_create(&id, NULL,  (void*)_start_metadata_search, (void*)token) != 0)
		{
			recording_search_token_expire(search_token_i);
			printf("thread create error\n");
			return -1;
		}
	}
	pthread_detach(id);


	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_FindRecordings));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_FindRecordings)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	
	return ONVIF_ERR_RET_SUCCESS;
}



static int recording_search_token_alive(int token_idx)
{
	int state = 0;

	pthread_mutex_init(&srt_mutex, NULL);
		
	pthread_mutex_lock(&srt_mutex);
	state = search_token[token_idx] ? 1 : 0;
	pthread_mutex_unlock(&srt_mutex);

	return state;
}

#if defined(WIN32)
int onvif_GetEventSearchResults(void* tmp_, long  size)
#else 
int onvif_GetEventSearchResults(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_EventSearchResult tmp;
		arg_FindRecordings * fre;
		int token_idx=0;
		char result_search_state = 0;
		int i=0, j=0, min_cnt=0, max_cnt=ONVIF_CH, result=0;
		unsigned int rec_info_cnt=0;
		recording_info tmp_msg[MAX_MSG_BUF];
		
		int elapsed_ms = 0;
		int r=0;
		recordingEvent_info *ri;

	#if defined (WIN32)
			
			memcpy(&tmp, tmp_, size);
	
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_EventSearchResult);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_EventSearchResult));
    #endif
			
	 token_idx = (unsigned int)STR2INT_SEARCH_TOKEN(tmp.SearchToken);
	printf("%s:%d SearchToken[%s] token_idx[%d]\n", __FUNCTION__, __LINE__, tmp.SearchToken, token_idx);

	if(!recording_search_token_alive(token_idx))
	{
		printf("%s:%d SearchToken[%s] is not alive.\n", __FUNCTION__, __LINE__, tmp.SearchToken);
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}

	/* POP rec_info from shared mem */
	//init_rp_rec_info_shm(token_idx);
	while(1)
	{
		if(tmp.WaitTime > 0 && elapsed_ms > tmp.WaitTime)
			break;
		
		//result = pop_rec_info_shm(token_idx, &tmp_msg[rec_info_cnt], &result_search_state);

	 	fre = (arg_FindRecordings *)g_hash_table_lookup(g_rec_hash_table,tmp.SearchToken);
	
		result_search_state = fre->SearchState;
		 if( result_search_state  == SEARCH_STATE_COMPLETED) {
			 break;
		 }
		printf("poped ret[%d] token[%d] state[%d] rec_info_cnt[%d]\n", result, token_idx, result_search_state, rec_info_cnt);
		
		/* if no messages in buffer */
		if(result < 0)
		{
			if(result_search_state == SEARCH_STATE_COMPLETED
			|| result_search_state == SEARCH_STATE_QUEUED
			|| result_search_state == SEARCH_STATE_UNKNOWN)
			{
				if(result_search_state == SEARCH_STATE_QUEUED)
					g_usleep(1000000);				
				break;
			}
			else if(result_search_state == SEARCH_STATE_SEARCHING)
			{
				elapsed_ms += 200;
				g_usleep(200000);
				continue;
			}
		}
		else
		{
			rec_info_cnt++;
		}

		// check the limit of messages
		if(rec_info_cnt >= (unsigned int)max_cnt)
			goto search_rec_done;
		
		elapsed_ms += 2;
		g_usleep(20000);
	}	

search_rec_done:

	tmp.SearchState = result_search_state;
	tmp.RecInfoCnt = eve_arr->len;
	
	//if(tmp.MaxResult > 0 && tmp.MaxResult < (int)eve_arr->len ) {
	//	tmp.RecInfoCnt = tmp.MaxResult ;
	//}else {
		tmp.RecInfoCnt = eve_arr->len;
	//}
	
	pthread_mutex_lock(&ri_mutex);

	if(tmp.RecInfoCnt > 0) {
	
		for(r=0;r < tmp.RecInfoCnt;r++) {

			
			ri = &g_array_index (eve_arr, recordingEvent_info, 0);
			memcpy(&tmp.eventInfo[r], ri, sizeof(recordingEvent_info));
			g_array_remove_index(eve_arr,0);
			
		}
		g_array_remove_range (eve_arr,0,eve_arr->len);
		
		//memcpy(&tmp.RecInfo, &tmp_msg, sizeof(tmp_msg));
	}
	pthread_mutex_unlock(&ri_mutex);
	printf("tmp.SearchState[%d] tmp.RecInfoCnt[%d]\n", tmp.SearchState, tmp.RecInfoCnt);

	#if defined (WIN32)

				
 				memcpy_s(tmp_,size, &tmp, size);

	#else
				memcpy(packet.data, &tmp, sizeof (arg_EventSearchResult));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_EventSearchResult)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}

#if defined(WIN32)
int onvif_GetRecordingSearchResults(void* tmp_, long  size)
#else 
int onvif_GetRecordingSearchResults(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_SearchResult tmp;
		arg_FindRecordings * fre;
		int token_idx=0;
		char result_search_state = 0;
		int i=0, j=0, min_cnt=0, max_cnt=ONVIF_CH, result=0;
		unsigned int rec_info_cnt=0;
		recording_info tmp_msg[MAX_MSG_BUF];
		
		int elapsed_ms = 0;
		int r=0;
		recording_info *ri;

	#if defined (WIN32)
			
			memcpy(&tmp, tmp_, size);
	
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_SearchResult);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_SearchResult));
    #endif
			
	 token_idx = (unsigned int)STR2INT_SEARCH_TOKEN(tmp.SearchToken);
	printf("%s:%d SearchToken[%s] token_idx[%d]\n", __FUNCTION__, __LINE__, tmp.SearchToken, token_idx);

	if(!recording_search_token_alive(token_idx))
	{
		printf("%s:%d SearchToken[%s] is not alive.\n", __FUNCTION__, __LINE__, tmp.SearchToken);
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}




	

	 if(tmp.FindType == FIND_RECORDING){
		/* todo
		 *  verify search token is for recording.
		 */
	 }
	 else if(tmp.FindType == FIND_EVENT){
		/* todo
		 *  verify search token is for event.
		 */
		max_cnt = MAX_MSG_BUF;
	 }
	
	/* POP rec_info from shared mem */
	//init_rp_rec_info_shm(token_idx);
	while(1)
	{
		if(tmp.WaitTime > 0 && elapsed_ms > tmp.WaitTime)
			break;
		
		//result = pop_rec_info_shm(token_idx, &tmp_msg[rec_info_cnt], &result_search_state);

	 	fre = (arg_FindRecordings *)g_hash_table_lookup(g_rec_hash_table,tmp.SearchToken);
	
		result_search_state = fre->SearchState;
		 if( result_search_state  == SEARCH_STATE_COMPLETED) {
			 break;
		 }
		printf("poped ret[%d] token[%d] state[%d] rec_info_cnt[%d]\n", result, token_idx, result_search_state, rec_info_cnt);
		
		/* if no messages in buffer */
		if(result < 0)
		{
			if(result_search_state == SEARCH_STATE_COMPLETED
			|| result_search_state == SEARCH_STATE_QUEUED
			|| result_search_state == SEARCH_STATE_UNKNOWN)
			{
				if(result_search_state == SEARCH_STATE_QUEUED)
					g_usleep(1000000);				
				break;
			}
			else if(result_search_state == SEARCH_STATE_SEARCHING)
			{
				elapsed_ms += 200;
				g_usleep(200000);
				continue;
			}
		}
		else
		{
			rec_info_cnt++;
		}

		// check the limit of messages
		if(rec_info_cnt >= (unsigned int)max_cnt)
			goto search_rec_done;
		
		elapsed_ms += 2;
		g_usleep(20000);
	}	

search_rec_done:

	tmp.SearchState = result_search_state;
	tmp.RecInfoCnt = rec_arr->len;
	
	if(tmp.MaxResult > 0 && tmp.MaxResult < (int)rec_arr->len ) {
		tmp.RecInfoCnt = tmp.MaxResult ;
	}else {
		tmp.RecInfoCnt = rec_arr->len;
	}
	

	pthread_mutex_lock(&ri_mutex);
	if(tmp.RecInfoCnt > 0) {
	
		for(r=0;r < tmp.RecInfoCnt;r++) {

			
			ri = &g_array_index (rec_arr, recording_info, 0);
			memcpy(&tmp.RecInfo[r], ri, sizeof(recording_info));
			g_array_remove_index(rec_arr,0);
			
		}
		g_array_remove_range (rec_arr,0,rec_arr->len);
		
		//memcpy(&tmp.RecInfo, &tmp_msg, sizeof(tmp_msg));
	}
	pthread_mutex_unlock(&ri_mutex);
	printf("tmp.SearchState[%d] tmp.RecInfoCnt[%d]\n", tmp.SearchState, tmp.RecInfoCnt);

	#if defined (WIN32)

				
 				memcpy_s(tmp_,size, &tmp, size);

	#else
				memcpy(packet.data, &tmp, sizeof (arg_SearchResult));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_SearchResult)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}

#if defined(WIN32)
int onvif_EndSearch(void* tmp_, int size)
#else 
int onvif_EndSearch(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_FindRecordings tmp;
		int tokenValue =0;
		
		char *str;
	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_FindRecordings);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_FindRecordings));
    #endif
			str= strstr(tmp.SearchToken,"srt");
			if(str == NULL) return ONVIF_R_ERR_INVALID_ARG_VAL;
			tokenValue  = atoi(str);
			if(tokenValue < 0 || tokenValue > ONVIF_CH) {
				return ONVIF_R_ERR_INVALID_ARG_VAL;
			}

			if(!recording_search_token_alive(tokenValue)) {
					return ONVIF_R_ERR_INVALID_ARG_VAL;
			}
			// change state to 0 and end search
			 search_token[tokenValue] = 0;

			tmp.EndPoint = (int)time(NULL);
	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_FindRecordings));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_FindRecordings)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}

#if defined(WIN32)
int onvif_SetRecordingJobMode(void* tmp_, int size)
#else 
int onvif_SetRecordingJobMode(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_JobItem tmp;
		gchar ** splitbuf;
		int ch;
		struct e_recording_type recording;
	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_JobItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_JobItem));
    #endif

			
			
			
			
			splitbuf = g_strsplit_set(tmp.JobToken,"t",0);				
			ch = atoi(splitbuf[1]);
			recording.token = ch;
			recording.recordingToken = ch;
			recording.value = SetPanicRecording(ch, tmp.conf.RecordingJobMode);
			append_onvif_event_msg(E_KEY_RECORDING_JOB_STATE_CHANGE, &recording, E_Changed, 0);
			g_strfreev(splitbuf);
#if 0
			for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
			sprintf(buff, "onvif.recordingjob%d.token", i);

			//1. jobToken�� ��ġ �Ѵ�.
			if(!strcmp(tmp.JobToken,nf_sysdb_get_str_nocopy(buff)))
			{
				// 1.5 RecordingToken�� ������ index�� ������.
				// FIX ME : 1.5
				
			
				sprintf(buff, "onvif.recordingjob%d.mode", i);
				//2. Recording Job�� ��带 �� ������Ʈ �Ѵ�.
				
				mode = getJobModeInt(tmp.conf.RecordingJobMode);
				dbmode = (int)nf_sysdb_get_uint(buff);
				// mode�� dbmode�� ���� ���� �� event log�� ������Ʈ �Ѵ�.
				if(mode != dbmode) {
						nf_sysdb_set_uint(buff,(unsigned int)mode);
						// FIX ME:
						_TTY_LOG_ONVIF_DEBUG("tmp.conf.RecordingJobMode: %s (Line : %d)",tmp.conf.RecordingJobMode, __LINE__);
						// 3. Search�� ���� nf_eventlog_put_param ����
						// 4. onvif �̺�Ʈ ����.

						GTimeVal	curr_time;
						g_get_current_time( &curr_time);
						if(mode) {
							_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d) LT_RECORD_STARTED",__FUNCTION__, __LINE__);	
							log_type = LT_RECORD_STARTED;

						}else {
							_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d) LT_RECORD_STOPPED",__FUNCTION__, __LINE__);	
							log_type = LT_RECORD_STOPPED;
						}
						nf_eventlog_put_param( &curr_time, log_type, 
						i /* ch_num */, 
						LP2_RECORD_STARTED_PANIC /* record_reason */,
						"Onvif");

				}
				event_status_shm_write(OV_PROP_RECORDING_JOB_STATE_CHANGE, 1);		 //Search ME:
				event_msgbuf_shm_append(OV_PROP_RECORDING_JOB_STATE_CHANGE, (unsigned int)i, 1);		//Search ME:	
				goto completed;
			}

	}
#endif


	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_JobItem));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_JobItem)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}




	#if defined(WIN32)
int onvif_GetRecordingConfiguration(void* tmp_, int size)
#else 
int onvif_GetRecordingConfiguration(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		int i=0,j=0;
		char tmp_token[COMMON_SIZE] = { 0, };
		char buff[COMMON_SIZE] = { 0, };
		arg_RecordingConfiguration tmp;


	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_RecordingConfiguration);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_RecordingConfiguration));
    #endif
	// in here you should put the custom code.
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		if(!strcmp(tmp.RecordingToken, nf_sysdb_get_str_nocopy(buff)))
		{
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			sprintf(buff, "onvif.recording%d.id", i);			
			strncpy(tmp.recordingConf.Source.SourceId, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE*2 -1);
			_TTY_LOG_ONVIF_DEBUG("source id = %s",tmp.recordingConf.Source.SourceId);
			sprintf(buff, "onvif.recording%d.name", i);			
			strncpy(tmp.recordingConf.Source.Name, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
						_TTY_LOG_ONVIF_DEBUG("source id = %s",tmp.recordingConf.Source.Name);
			sprintf(buff, "onvif.recording%d.location", i);			
			strncpy(tmp.recordingConf.Source.Location, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
						_TTY_LOG_ONVIF_DEBUG("source id = %s",tmp.recordingConf.Source.Location);
			sprintf(buff, "onvif.recording%d.desc", i);			
			strncpy(tmp.recordingConf.Source.Description, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
				_TTY_LOG_ONVIF_DEBUG("source id = %s",tmp.recordingConf.Source.Description);
			sprintf(buff, "onvif.recording%d.address", i);			
			strncpy(tmp.recordingConf.Source.Address, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE*2 -1);
			sprintf(buff, "onvif.recording%d.content", i);			
			strncpy(tmp.recordingConf.Content, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.max_retention_time", i);			
			tmp.recordingConf.MaximumRetentionTime = (int)nf_sysdb_get_uint(buff);
			_TTY_LOG_ONVIF_DEBUG("source id = %d",tmp.recordingConf.MaximumRetentionTime);

			goto completed;

		}
			
		
	}

	if(i == ONVIF_MAX_RECORDING_CNT) {
		return ONVIF_R_ERR_NO_RECORDING;

	}

completed:	

	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_RecordingConfiguration));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_RecordingConfiguration)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}

int getRecordingJobConfiguration(arg_JobItem* tmp)
{
int mode=0;
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	

	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
			sprintf(buff, "onvif.recordingjob%d.token", i);
			if(!strcmp(tmp->JobToken,nf_sysdb_get_str_nocopy(buff)))
			{		
				sprintf(buff, "onvif.recordingjob%d.recording_token", i);
				strncpy(tmp->conf.RecordingToken,nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
				sprintf(buff, "onvif.recordingjob%d.mode", i);
				

				GetPanicRecording(i,buff);
				strncpy(tmp->conf.RecordingJobMode,buff,COMMON_SIZE-1);
				//_TTY_LOG_ONVIF_DEBUG("tmp->conf.RecordingJobMode : %s",tmp->conf.RecordingJobMode);

				
				sprintf(buff, "onvif.recordingjob%d.priority", i);

				tmp->conf.Priority = (int)nf_sysdb_get_uint(buff);
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)tmp->conf.Priority %d",__FUNCTION__, __LINE__,tmp->conf.Priority);	
	
				tmp->conf.sourceSize = 1;
				sprintf(buff, "onvif.recordingjob%d.source_token", i);
				strncpy(tmp->conf.source[j].SourceToken,nf_sysdb_get_str_nocopy(buff),COMMON_SIZE-1);				
				strncpy(tmp->conf.source[j].SourceTokenType,"http://www.onvif.org/ver10/schema/Profile", COMMON_SIZE - 1);
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)tmp->conf.source[i].SourceToken %s",__FUNCTION__, __LINE__,tmp->conf.source[i].SourceToken);

			//	sprintf(buff, "onvif.source%d.dest%d", i,j);
			//	strncpy(tmp->conf.source[j].track[i].Destination,nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);										
				//return 0;
				break;

			}
		}
	if(i == ONVIF_MAX_RECORDING_CNT)
	{
		return ONVIF_R_ERR_NO_RECORDINGJOBS;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

	#if defined(WIN32)
int onvif_GetRecordingJobConfiguration(void* tmp_, int size)
#else 
int onvif_GetRecordingJobConfiguration(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_JobItem tmp;


	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_JobItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_JobItem));
    #endif

			if(getRecordingJobConfiguration(&tmp) == ONVIF_R_ERR_NO_RECORDINGJOBS) {
				return ONVIF_R_ERR_NO_RECORDINGJOBS;
			}
	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_JobItem));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_JobItem)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}

	
#if defined(WIN32)
int onvif_SetRecordingConfiguration(void* tmp_, int size)
#else 
int onvif_SetRecordingConfiguration(NfOnvif *self, int fd, char *buff_rcv)
#endif
{

		int i=0,j=0;
		char tmp_token[COMMON_SIZE] = { 0, };
		char buff[COMMON_SIZE] = { 0, };
		arg_RecordingConfiguration tmp;
		struct e_recording_type recording;

	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_RecordingConfiguration);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_RecordingConfiguration));
    #endif



for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);

		if(!strcmp(tmp.RecordingToken, nf_sysdb_get_str_nocopy(buff)))
		{

			recording.recordingToken = i;
			recording.value = i;
			recording.token = i;

			sprintf(buff, "onvif.recording%d.id", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.SourceId);
			strncpy(recording.sourceId , tmp.recordingConf.Source.SourceId, COMMON_SIZE*2 -1);
			sprintf(buff, "onvif.recording%d.name", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Name);
			strncpy(recording.name , tmp.recordingConf.Source.Name, COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.location", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Location);
			strncpy(recording.location , tmp.recordingConf.Source.Location, COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.desc", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Description);
			strncpy(recording.description , tmp.recordingConf.Source.Description, COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.address", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Address);
			strncpy(recording.address , tmp.recordingConf.Source.Address, COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.content", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Content);
			strncpy(recording.content , tmp.recordingConf.Content, COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.max_retention_time", i);			
			nf_sysdb_set_uint(buff, (unsigned int)tmp.recordingConf.MaximumRetentionTime);
			recording.value = tmp.recordingConf.MaximumRetentionTime;
						
		//	event_status_shm_write(OV_PROP_CONFIGURATION_CHANGE, 1);		
			//event_msgbuf_shm_append(OV_PROP_CONFIGURATION_CHANGE, (unsigned int)i, 1);	

			//sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

			

			append_onvif_event_msg(E_KEY_CONFIGURATION_CHANGE, &recording, E_Changed, 0);
			goto completed;

		}
			

		
	}



	completed:


	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_RecordingConfiguration));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_RecordingConfiguration)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}


#if defined(WIN32)
int onvif_GetRecordingInformation(void* tmp_, int size)
#else 
int onvif_GetRecordingInformation(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_DeviceRecordingSummary sum;
		arg_RecordingInformation tmp;
		char buff[COMMON_SIZE] = { 0, };
		int i=0;
		int recording_token_num = 0;
		int ret =  0;
		NF_DISK_REC_TIME        nrec_time;	
	

	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_RecordingInformation);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_RecordingInformation));
    #endif
	memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);


		if(!strcmp(tmp.token, nf_sysdb_get_str_nocopy(buff)))
		{
			sprintf(buff, "onvif.recording%d.content", i);			
			strncpy(tmp.Content, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);

			sprintf(buff, "onvif.recording%d.name", i);			
			strncpy(tmp.Source.Name, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);

			sprintf(buff, "onvif.recording%d.location", i);			
			strncpy(tmp.Source.Location, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);

			sprintf(buff, "onvif.recording%d.desc", i);			
			strncpy(tmp.Source.Description, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);

			sprintf(buff, "onvif.recording%d.address", i);			
			strncpy(tmp.Source.Address, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
			sprintf(buff, "onvif.recording%d.content", i);			
			strncpy(tmp.Content, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
			break;
		}
	}
	
	ret = GetDeviceRecordingSummary(&sum,i);
	
	if( !ret)	
		return ONVIF_ERR_RET_INTERNAL;

	tmp.StartTime = (int)sum.dataFrom;
	tmp.EndTime = (int)sum.dataUntil;

	if( tmp.token[0] != 0 ){
		recording_token_num = atoi(tmp.token+2);
	}

	tmp.TrackInfo[0].StartTime = (int)sum.dataFrom;
	tmp.TrackInfo[0].EndTime = (int)sum.dataUntil;
	tmp.TrackInfo[0].type = 0;
	sprintf(tmp.TrackInfo[0].token, "%s%d",VIDEO_TRACK, recording_token_num);
	
	tmp.TrackInfo[1].StartTime = (int)sum.dataFrom;
	tmp.TrackInfo[1].EndTime = (int)sum.dataUntil;
	sprintf(tmp.TrackInfo[1].token, "%s%d",AUDIO_TRACK, recording_token_num);
	tmp.TrackInfo[1].type = 1;

	tmp.TrackInfo[2].StartTime = (int)sum.dataFrom;
	tmp.TrackInfo[2].EndTime = (int)sum.dataUntil;
	sprintf(tmp.TrackInfo[1].token, "%s%d",METADATA_TRACK, recording_token_num);
	tmp.TrackInfo[2].type = 2;

	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_RecordingInformation));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_RecordingInformation)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}




#if defined(WIN32)
int onvif_temp(void* tmp_, int size)
#else 
int onvif_temp(NfOnvif *self, int fd, char *buff_rcv)
#endif
{
		arg_JobItem tmp;


	#if defined (WIN32)
			memcpy(&tmp, tmp_, size);
    #else
			ONVIF_PACKET packet;
			ONVIF_HEADER header;
			guint packet_len;
            memset(&header, 0, sizeof (header));
            memcpy(&header, buff_rcv, sizeof (header));

            packet_len = sizeof (ONVIF_HEADER) + sizeof (arg_JobItem);

            memcpy(&packet, buff_rcv, packet_len);
            memcpy(&tmp, packet.data, sizeof (arg_JobItem));
    #endif


	#if defined (WIN32)
				memcpy(tmp_, &tmp, size);
	#else
				memcpy(packet.data, &tmp, sizeof (arg_JobItem));
					if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
										header.code, sizeof (arg_JobItem)) != ONVIF_ERR_RET_SUCCESS) {
								return ONVIF_ERR_RET_INTERNAL;
				}           
	#endif
	return ONVIF_ERR_RET_SUCCESS;
}

