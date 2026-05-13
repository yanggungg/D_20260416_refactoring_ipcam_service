#include <sys/msg.h>
#include <sys/errno.h>
#include <time.h>
#include <regex.h>
#include <sys/types.h>
#include <linux/socket.h>
#include <pthread.h>

#include "nf_common.h"
#include "nf_util_netif.h"
#include "nf_util_time.h"
#include "nf_timer.h"
#include "nf_issm_ctl.h"

#include "nf_api_ipcam.h"
#include "nf_webra_def.h"
#include "nf_ptz.h"
#include "scm.h"
#include "smt.h"

#if defined(TCP_BUFFER_CTRL)
#include "log.h"
#endif
//#include "itx_utils.h"
#include "onvif_common.h"
#include "issm.h"
#include "nvs_onvif_app.h"
#include "nvs_onvif_app_util.h"
#include "../nf_onvif_server.h"

#include "nf_sysdb.h"

#include "unp.h"

#ifdef _ONVIF_STAND_ALONE
void onvif_restart(void);
#endif

#define DEBUG_WEB
#ifdef DEBUG_WEB
#	warning WEB_DEBUG_MODE
#else
#	ifdef g_message
#		undef g_message
#	endif
#	define g_message(x, ...)	do{}while(0)
#endif//DEBUG_WEB

#ifdef USE_PROXY_SYSTEM
#include "proxy_cli.h"
#endif
extern int g_onvif_service_stop;

static unsigned char set_bit[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
float defog_level=0;

static char * ONVIF_DVRauthFilePath = "/NFDVR/webra/passwd/.passwd";
static void _getAudioEncoderConfiguration(arg_AudioEncoder *tmp);
static int _getMetadataConfiguration(arg_Metadata* tmp);
static int _getMetadataConfiguration2(arg_Metadata* tmp);
static void _getVideoSourceConfiguration(arg_VideoSource *tmp);

static int recording_search_token_alive(int token_idx);
int get_end_search_token(int token_idx);
#if 1 
const char onvif_cmd_table[][64] = {
	"CMD_GetHostName", 				// 0
	"CMD_SetHostName",			    // 1
	"CMD_GetDeviceInformation",		// 2
	"CMD_GetDNS", 					// 3
	"CMD_SetDNS",						// 4	
	"CMD_GetNTP",						// 5	
	"CMD_SetNTP",						// 6	
	"CMD_SetDateTime",				// 7
	"CMD_GetDateTime",				// 8
	"CMD_GetScopes",					// 9
	"CMD_SetScopes",					// 10
	"CMD_AddScopes",					// 11
	"CMD_RemoveScopes",				// 12
	"CMD_GetProfilesDB",              // 13
	"CMD_GetProfileDB",               // 14
	"CMD_GetVideoEncoder",			// 15
	"CMD_GetVideoEncoders",			// 16
	"CMD_SetVideoEncoder",            // 17
	"CMD_FactoryDefault", 	 	    // 18
	"CMD_SystemReboot",               // 19
	"CMD_GetStreamUri", 				// 20
	"CMD_GetVideoEncoderOption",		// 21
	"CMD_GetImagingOption",   		// 22
	"CMD_SetImagingOption",           // 23
	"CMD_GetNetworkInfo",             // 24
	"CMD_SetNetworkInfo",             // 25
	"CMD_GetDefaultGateway",             // 25
	"CMD_SetDefaultGateway",             // 25
	"CMD_GetNetworkPort", 			// 26
	"CMD_SetNetworkPort",             // 27
	"CMD_SetVideoSource",				// 28
	"CMD_GetDiscovery",
	"CMD_SetDiscovery",               // 30
	"CMD_CreateProfile",
	"CMD_AddVideoSource",
	"CMD_AddVideoEncoder",
	"CMD_RemoveVideoEncoder",
	"CMD_RemoveVideoSource",
	"CMD_DeleteProfile",
	"CMD_GetVideoSourceConfiguration", 
	"CMD_GetVideoSourceConfigurations", 
	"CMD_CheckTokens",
	"CMD_AddAudioSource",                   // 40
	"CMD_AddAudioEncoder",
	"CMD_RemoveAudioEncoder",
	"CMD_RemoveAudioSource",
	"CMD_GetAudioSourceConfiguration", 
	"CMD_GetAudioSourceConfigurations", 
	"CMD_GetAudioSourceOption",
	"CMD_GetAudioEncoderConfiguration", 
	"CMD_GetAudioEncoderConfigurations", 
	"CMD_GetCompatibleAudioEncoder",
	"CMD_GetCompatibleAudioSource",         // 50
	"CMD_SetAudioEncoder", 
	"CMD_SetAudioSource", 
	"CMD_GetCompatibleVideoSource",
	"CMD_GetVideoSourceOption",
	"CMD_GetVideoSources",
	"CMD_GetAudioSources",	
	"CMD_GetCompatibleVideoEncoder",
	"CMD_GetMetadataConfigurations",
	"CMD_GetCompatibleMetadataConfigurations",
	"CMD_AddMetadataConfiguration",         // 60
	"CMD_RemoveMetadataConfiguration",
	"CMD_SetMetadataConfiguration",
	"CMD_GetMetadataConfiguration",
	"CMD_GetUser",
	"CMD_SetUser",
	"CMD_CreateUsers",
	"CMD_DeleteUsers",
	"CMD_GetImagingSettings",
	"CMD_SetImagingSettings",
	"CMD_GetOptions",  		// 70
	"CMD_Move",
	"CMD_FocusStop",
	"CMD_GetStatus",
	"CMD_GetCapa",
	"CMD_GetRelays",
	"CMD_SetRelaySettings",
	"CMD_SetRelayState",
	"CMD_GotoHomePosition",
	"CMD_AbsoluteMove",
	"CMD_ContinuousMove",		// 80
	"CMD_CreatePresetTour",
	"CMD_GetPTZNodes",
	"CMD_GetPTZNode",
	"CMD_GetPTZConfiguration",
	"CMD_GetPTZConfigurationOptions",
	"CMD_GetConfigurations",
	"CMD_GetNode",
	"CMD_GetNodes",
	"CMD_GetPresets",
	"CMD_GetPresetTour", 						// 90
	"CMD_GetPresetTourOptions",
	"CMD_GetPresetTours",
	"CMD_GetServiceCapabilities",
	"CMD_PTZGetStatus",
	"CMD_ModifyPresetTour",
	"CMD_OperatePresetTour",
	"CMD_RelativeMove",
	"CMD_RemovePreset",
	"CMD_RemovePresetTour",
	"CMD_SendAuxiliaryCommand",					// 100
	"CMD_SetConfiguration",
	"CMD_SetHomePosition",
	"CMD_SetPreset",
	"CMD_GotoPreset",
	"CMD_Stop",
	"CMD_GetEventCapa",
	"CMD_GetPTZConfigurations",
	"CMD_SetPTZConfiguration",
	"CMD_AddPTZConfiguration",
	"CMD_RemovePTZConfiguration",				// 110
	"CMD_CreateRecording",
	"CMD_CreateRecordingJob",
	"CMD_CreateTrack",
	"CMD_DeleteRecording",
	"CMD_DeleteRecordingJob",
	"CMD_DeleteTrack",
	"CMD_GetRecordingConfiguration",
	"CMD_GetRecordingJobConfiguration",
	"CMD_GetRecordingJobs",
	"CMD_GetRecordingJobState",		// 120
	"CMD_GetRecordingOptions",
	"CMD_GetRecordings",
	"CMD_GetTrackConfiguration",
	"CMD_SetRecordingConfiguration",
	"CMD_SetRecordingJobConfiguration",
	"CMD_SetRecordingJobMode",
	"CMD_SetTrackConfiguration",
	"CMD_EndSearch",
	"CMD_FindEvent",
	"CMD_FindMetadata",						// 130
	"CMD_FindPTZPosition",
	"CMD_FindRecordings",
	"CMD_GetEventSearchResults",
	"CMD_GetMediaAttributes",
	"CMD_GetMetadataSearchResults",
	"CMD_GetRecordingInformation",
	"CMD_GetRecordingSearchResults",
	"CMD_GetRecordingSummary",
	"CMD_GetSearchState",
	"CMD_GetRecordingServiceCapabilities",				// 140
	"CMD_GetReplayConfiguration",
	"CMD_GetReplayUri",
	"CMD_SetReplayConfiguration",
	"CMD_GetRecordingJobStateChangeEvent",
	"CMD_GetEventProperties",	
	"CMD_CreatePullPointSubscription",	
	"CMD_PullMessages",
	"CMD_Subscribe",
	"CMD_Notify",
	"CMD_SetSynchronizationPoint",						// 150
	"CMD_Unsubscribe",	
	"CMD_Renew",
	"CMD_GetRelayOptions",
	"CMD_GetDigitalInputs",
	"CMD_CheckDeviceReady",	
	"CMD_GetServices",
	"CMD_GetGuaranteedNumberOfVideoEncoderInstances",
	"CMD_GetSnapshotUri",
	"CMD_GetProfilesDB2",
	"CMD_GetStreamUri2",
	"CMD_GetVideoEncoderOption2",
	"CMD_GetVideoEncoders2",
	"CMD_GetAudioEncoders2",
	"CMD_GetAudioOutput",
	"CMD_GetAudioOutputConfiguration",
	"CMD_GetVideoSourceConfigurations2",
	"CMD_GetAudioSourceConfigurations2",
	"CMD_GetAudioEncoderConfigurations2",
	"CMD_GetAudioOutputConfigurations",
	"CMD_GetCompatibleAudioOutput",
	"CMD_GetAudioEncoderConfigurationOptions",
	"CMD_GetAudioOutput2",
	"CMD_GetAudioDecoder2",
	"CMD_SetAudioOutput",
	"CMD_SetAudioDecoder",
	"CMD_GetRecordings",
	"CMD_CreateRecordingJob",
	"CMD_DeleteRecording",
	"CMD_GetRecordingConfiguration",
	"CMD_GetRecordingJobConfiguration",
	"CMD_GetRecordingJobs",
	"CMD_GetRecordingJobState",
	"CMD_GetRecordingOptions",
	"CMD_SetRecordingConfiguration",
	"CMD_SetRecordingJobMode",
	"CMD_EndSearch",
	"CMD_FindEvent",
	"CMD_FindRecordings",
	"CMD_GetRecordingSearchResults",
	"CMD_GetRecordingInformation",
	"CMD_GetRecordingSearchResults",
	"CMD_StartMulticastStreaming",
	"CMD_StopMulticastStreaming",
	"CMD_CreateProfile2",
	"CMD_GetConfigurationOptions",
	"CMD_GetAnalyticsModules",
	"CMD_ModifyAnalyticsModules",
	"CMD_GetRules",
	"CMD_GetVideoAnalyticsConfiguration",
	"CMD_ModifyRules",
	"CMD_GetVideoAnalyticsConfigurations",
	"CMD_SetVideoAnalyticsConfiguration",
	"CMD_AddVideoAnalyticsConfiguration",
	"CMD_RemoveVideoAnalyticsConfiguration",
	"CMD_VAnalytics_Check_Token",
	"CMD_GetMetadataConfigurationOptions",
	"CMD_SetSynchronizationPoint_trt",
	"CMD_RemoveConfiguration",
	"CMD_AddConfiguration",
	"CMD_GetAnalyticsConfigurations2",
	"CMD_GetMetadataConfigurations2",
	"CMD_AddAudioOutputConfiguration",
	"CMD_RemoveAudioOutputConfiguration",
	"CMD_AddAudioDecoderConfiguration",
	"CMD_RemoveAudioDecoderConfiguration",
	"CMD_GetCompatibleConfigurations",
	"CMD_StartFirmwareUpgrade",
	"CMD_CreateOSD",
	"CMD_GetOSD",
	"CMD_SetOSD",
	"CMD_DeleteOSD"
};

#define	DUMP_LEN	(16)
static int	_onvif_packet_dump(ONVIF_PACKET *packet)
{
	int i=0, j=0;

	unsigned int length = ntohl(packet->header.dlen);
	
	OV_DEBUG("cmd[%s] dlen[%d]", onvif_cmd_table[packet->header.code], length);

	for(i=0; i*DUMP_LEN<length; i++){
		for(j=0; j<DUMP_LEN; j++){
			OV_DEBUG_RAW("%X ", packet->data[i*DUMP_LEN+j]);
		}
		OV_DEBUG_RAW("[");
		for(j=0; j<DUMP_LEN; j++){
			OV_DEBUG_RAW("%c", packet->data[i*DUMP_LEN+j]);
		}			
		OV_DEBUG_RAW("]\n");
	}
	i--;
	if( i*DUMP_LEN >= length){
		return 1;
	}
	for( j=i*DUMP_LEN; j<length; j++ ){
		OV_DEBUG_RAW("%X ", packet->data[j]);
	}
	OV_DEBUG_RAW("[");
	for( j=i*DUMP_LEN; j<length; j++ ){
		OV_DEBUG_RAW("%c", packet->data[j]);
	}		
	OV_DEBUG_RAW("]\n");		
	return 1;
}
#endif

int nf_onvif_packet_code(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_HEADER header;
	int ret = 0;
	int delay = 3;
	//int fail_delay = 15;
	static int before_cmd = -1;
	g_return_val_if_fail(self != NULL, ONVIF_ERR_RET_PARAMETER);
	g_return_val_if_fail(fd != -1, ONVIF_ERR_RET_PARAMETER);
	g_return_val_if_fail(buff_rcv != NULL, ONVIF_ERR_RET_PARAMETER);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	if (g_onvif_service_stop ) {
		//saving_status_file(0);
		if (nf_onvif_send_fault(fd, ret, buff_rcv) != ONVIF_ERR_RET_SUCCESS) {
			return ONVIF_ERR_RET_INTERNAL;
		}
		g_warning("%s error ret[%d] header.code[%d]", __FUNCTION__, ret, header.code);
		g_return_val_if_fail(ret == ONVIF_ERR_RET_SUCCESS, ONVIF_ERR_RET_INTERNAL);
	}
	
	if (header.type == ONVIF_PROTOCOL_REQUEST) {
		OV_DEBUG("cmd[%s] dlen[%d]", onvif_cmd_table[header.code], ntohl(header.dlen));
		switch (header.code) {
		// Block functions.
		case CMD_PullMessages:
			ret = onvif_PullMessages(self, fd, buff_rcv);
			break;
		// Non-block functions.
		case CMD_GetHostName:
			ret = onvif_GetHostName(self, fd, buff_rcv);
			break;
		case CMD_SetHostName:
			ret = onvif_SetHostName(self, fd, buff_rcv);
			break;
		case CMD_GetDNS:
			ret = onvif_GetDNS(self, fd, buff_rcv);
			break;
		case CMD_SetDNS:
			ret = onvif_SetDNS(self, fd, buff_rcv);
			break;
		case CMD_GetNTP:
			ret = onvif_GetNTP(self, fd, buff_rcv);
			break;
		case CMD_SetNTP:
			ret = onvif_SetNTP(self, fd, buff_rcv);
			break;
		case CMD_GetDateTime:
			ret = onvif_GetDateTime(self, fd, buff_rcv);
			break;
		case CMD_SetDateTime:
			ret = onvif_SetDateTime(self, fd, buff_rcv);
			break;
		case CMD_GetDeviceInformation:
			ret = onvif_GetDeviceInformation(self, fd, buff_rcv);
			break;
		case CMD_GetProfilesDB:
			ret = onvif_GetProfilesDB(self, fd, buff_rcv);
			break;
		case CMD_RemoveAudioOutputConfiguration:
			ret = onvif_RemoveAudioOutputConfiguration(self, fd, buff_rcv);
			break;
		case CMD_RemoveAudioDecoderConfiguration:
			ret = onvif_RemoveAudioDecoderConfiguration(self, fd, buff_rcv);
			break;
		case CMD_AddAudioOutputConfiguration:
			ret = onvif_AddAudioOutputConfiguration(self, fd, buff_rcv);
			break;
		case CMD_AddAudioDecoderConfiguration:
			ret = onvif_AddAudioDecoderConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetProfileDB:
			ret = onvif_GetProfileDB(self, fd, buff_rcv);
			break;
		case CMD_GetProfilesDB2:
			ret = onvif_GetProfilesDB2(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoder:
			ret = onvif_GetVideoEncoder(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoders:
			ret = onvif_GetVideoEncoders(self, fd, buff_rcv);
			break;
		case CMD_SetAudioOutput:
			ret = onvif_SetAudioOutput(self, fd, buff_rcv);
			break;
		case CMD_SetAudioDecoder:
			ret = onvif_SetAudioDecoder(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoders2:
			ret = onvif_GetVideoEncoders2(self, fd, buff_rcv);
			break;
        case CMD_GetAudioEncoderConfiguration2:
			ret = onvif_GetAudioEncoderConfigurations(self, fd, buff_rcv);
			break;
		case CMD_SetVideoEncoder:
			ret = onvif_SetVideoEncoder(self, fd, buff_rcv);
			break;
		case CMD_FactoryDefault:
			ret = onvif_FactoryDefault(self, fd, buff_rcv);
			break;
		case CMD_SystemReboot:
			ret = onvif_SystemReboot(self, fd, buff_rcv);
			break;
		case CMD_GetStreamUri:
			ret = onvif_GetStreamUri(self, fd, buff_rcv);
			break;
		case CMD_GetStreamUri2:
			ret = onvif_GetStreamUri2(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoderOption:
			ret = onvif_GetVideoEncoderOption(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoderOption2:
			ret = onvif_GetVideoEncoderOption2(self, fd, buff_rcv);
			break;
		case CMD_GetImagingOption:
			ret = onvif_GetImagingOption(self, fd, buff_rcv);
			break;
		case CMD_SetImagingOption:
			ret = onvif_SetImagingOption(self, fd, buff_rcv);
			break;
		case CMD_GetNetworkInfo:
			ret = onvif_GetNetworkInfo(self, fd, buff_rcv);
			break;
		case CMD_GetNetworkPort:
			ret = onvif_GetNetworkPort(self, fd, buff_rcv);
			break;
		case CMD_SetNetworkPort:
			ret = onvif_SetNetworkPort(self, fd, buff_rcv);
			break;
		case CMD_SetVideoSource:
			ret = onvif_SetVideoSource(self, fd, buff_rcv);
			break;
		case CMD_SetNetworkInfo:
			ret = onvif_SetNetworkInfo(self, fd, buff_rcv);
			break;
		case CMD_CreateProfile2:
			ret = onvif_CreateProfile2(self, fd, buff_rcv);
			break;
		case CMD_GetDefaultGateway:
			ret = onvif_GetDefaultGateway(self, fd, buff_rcv);
			break;
		case CMD_SetDefaultGateway:
			ret = onvif_SetDefaultGateway(self, fd, buff_rcv);
			break;
		case CMD_SetScopes:
			ret = onvif_SetScope(self, fd, buff_rcv);
			break;
		case CMD_GetScopes:
			ret = onvif_GetScope(self, fd, buff_rcv);
			break;
		case CMD_AddScopes:
			ret = onvif_AddScope(self, fd, buff_rcv);
			break;
		case CMD_RemoveScopes:
			ret = onvif_RemoveScope(self, fd, buff_rcv);
			break;
		case CMD_GetDiscovery:
			ret = onvif_GetDiscovery(self, fd, buff_rcv);
			break;
		case CMD_SetDiscovery:
			ret = onvif_SetDiscovery(self, fd, buff_rcv);
			break;
		case CMD_CreateProfile:
			ret = onvif_CreateProfile(self, fd, buff_rcv);
			break;
		case CMD_AddVideoSource:
			ret = onvif_AddVideoSource(self, fd, buff_rcv);
			break;
		case CMD_AddVideoEncoder:
			ret = onvif_AddVideoEncoder(self, fd, buff_rcv);
			break;
		case CMD_RemoveVideoEncoder:
			ret = onvif_RemoveVideoEncoder(self, fd, buff_rcv);
			break;
		case CMD_RemoveVideoSource:
			ret = onvif_RemoveVideoSource(self, fd, buff_rcv);
			break;
		case CMD_DeleteProfile:
			ret = onvif_DeleteProfile(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleVideoSource:
			ret = onvif_GetCompatibleVideoSource(self, fd, buff_rcv);
			break;
		case CMD_GetVideoSourceConfiguration:
			ret = onvif_GetVideoSourceConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetVideoSourceConfigurations:
			ret = onvif_GetVideoSourceConfigurations(self, fd, buff_rcv);
			break;
		case CMD_GetVideoSourceConfigurations2:
			ret = onvif_GetVideoSourceConfigurations2(self, fd, buff_rcv);
			break;
		case CMD_GetVideoSourceOption:
			ret = onvif_GetVideoSourceOption(self, fd, buff_rcv);
			break;
		case CMD_CheckTokens:
			ret = onvif_CheckTokens(self, fd, buff_rcv);
			break;
		case CMD_SetAudioEncoder:
			ret = onvif_SetAudioEncoder(self, fd, buff_rcv);
			break;
		case CMD_SetAudioSource:
			ret = onvif_SetAudioSource(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleAudioEncoder:
			ret = onvif_GetCompatibleAudioEncoder(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleAudioSource:
			ret = onvif_GetCompatibleAudioSource(self, fd, buff_rcv);
			break;
		case CMD_AddAudioSource:
			ret = onvif_AddAudioSource(self, fd, buff_rcv);
			break;
		case CMD_AddAudioEncoder:
			ret = onvif_AddAudioEncoder(self, fd, buff_rcv);
			break;
		case CMD_RemoveAudioEncoder:
			ret = onvif_RemoveAudioEncoder(self, fd, buff_rcv);
			break;
		case CMD_RemoveAudioSource:
			ret = onvif_RemoveAudioSource(self, fd, buff_rcv);
			break;
		case CMD_GetAudioSourceConfiguration:
			ret = onvif_GetAudioSourceConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetAudioSourceConfigurations:
			ret = onvif_GetAudioSourceConfigurations(self, fd, buff_rcv);
			break;
		case CMD_GetAudioSourceConfigurations2:
			ret = onvif_GetAudioSourceConfigurations2(self, fd, buff_rcv);
			break;
		case CMD_GetAudioSourceOption:
			ret = onvif_GetAudioSourceOption(self, fd, buff_rcv);
			break;
		case CMD_GetAudioEncoderConfiguration:
			ret = onvif_GetAudioEncoderConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetAudioEncoderConfigurations:
			ret = onvif_GetAudioEncoderConfigurations(self, fd, buff_rcv);
			break;
		case CMD_GetAudioEncoderConfigurations2:
			ret = onvif_GetAudioEncoderConfigurations2(self, fd, buff_rcv);
			break;
		case CMD_GetAudioEncoderConfigurationOptions:
			ret = onvif_GetAudioEncoderConfigurationOptions(self, fd, buff_rcv);
			break;
		case CMD_GetVideoSources:
			ret = onvif_GetVideoSources(self, fd, buff_rcv);
			break;
		case CMD_GetAudioSources:
			ret = onvif_GetAudioSources(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleVideoEncoder:
			ret = onvif_GetCompatibleVideoEncoder(self, fd, buff_rcv);
			break;
		case CMD_GetMetadataConfigurations:
			ret = onvif_GetMetadataConfigurations(self, fd, buff_rcv);
			break;
        case CMD_GetMetadataConfigurations2:
			ret = onvif_GetMetadataConfigurations2(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleMetadataConfigurations:
			ret = onvif_GetCompatibleMetadataConfigurations(self, fd, buff_rcv);
			break;
		case CMD_RemoveMetadataConfiguration:
			ret = onvif_RemoveMetadataConfiguration(self, fd, buff_rcv);
			break;
		case CMD_AddMetadataConfiguration:
			ret = onvif_AddMetadataConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetMetadataConfiguration:
			ret = onvif_GetMetadataConfiguration(self, fd, buff_rcv);
			break;
		case CMD_RTP_HTTP:
			ret = onvif_Passfd(self, fd, buff_rcv);
			break;
		case CMD_GetUser:
			ret = onvif_GetUser(self, fd, buff_rcv);
			break;
		case CMD_SetUser:
			ret = onvif_SetUser(self, fd, buff_rcv);
			break;
		case CMD_CreateUsers:
			ret = onvif_CreateUser(self, fd, buff_rcv);
			break;
		case CMD_DeleteUsers:
			ret = onvif_DeleteUser(self, fd, buff_rcv);
			break;
		case CMD_GetImagingSettings:
			ret = onvif_GetImagingSettings(self, fd, buff_rcv);
			break;
		case CMD_SetImagingSettings:
			ret = onvif_SetImagingSettings(self, fd, buff_rcv);
			break;
		case CMD_GetRelays:
			ret = onvif_GetRelays(self, fd, buff_rcv);
			break;
		case CMD_SetRelaySettings:
			ret = onvif_SetRelaySettings(self, fd, buff_rcv);
			break;
		case CMD_SetRelayState:
			ret = onvif_SetRelayState(self, fd, buff_rcv);
			break;
		case CMD_GetOptions:
			ret = onvif_GetOptions(self, fd, buff_rcv);
			break;
		case CMD_Move:
			ret = onvif_Move(self, fd, buff_rcv);
			break;
		case CMD_GetStatus:
			ret = onvif_GetStatus(self, fd, buff_rcv);
			break;
		case CMD_GetCapa:
			ret = onvif_GetCapability(self, fd, buff_rcv);
			break;
		case CMD_GotoHomePosition:
			ret = onvif_GotoHomePosition(self, fd, buff_rcv);
			break;
		case CMD_AbsoluteMove:
			ret = onvif_AbsoluteMove(self, fd, buff_rcv);
			break;
		case CMD_ContinuousMove:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_CreatePresetTour:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_GetPTZConfigurationOptions:
			ret = onvif_GetPTZConfigurationOptions(self, fd, buff_rcv);
			break;
		case CMD_SetPTZConfiguration:
			ret = onvif_SetPTZConfiguration(self, fd, buff_rcv);
			break;
			/*
			 case CMD_GetNode:
			 ret = onvif_GetNode(self, fd ,buff_rcv);
			 break;
			 case CMD_GetNodes:
			 ret = onvif_GetNodes(self, fd ,buff_rcv);
			 _TTY_LOG_ONVIF_DEBUG("Entry: %s (ret : %d) (Line : %d)", __FUNCTION__, ret, __LINE__);
			 */
			break;
		case CMD_GetPresets:
			ret = onvif_GetPresets(self, fd, buff_rcv);
			break;
		case CMD_GetPresetTour:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_GetPresetTourOptions:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_GetPresetTours:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_GetServiceCapabilities:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_PTZGetStatus:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_GotoPreset:
			ret = onvif_GotoPreset(self, fd, buff_rcv);
			break;
		case CMD_ModifyPresetTour:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_OperatePresetTour:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_RelativeMove:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_RemovePreset:
			ret = onvif_RemovePreset(self, fd, buff_rcv);
			break;
		case CMD_RemovePresetTour:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_SendAuxiliaryCommand:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_SetConfiguration:
			ret = onvif_SetConfiguration(self, fd, buff_rcv);
			break;
		case CMD_SetHomePosition:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
			break;
		case CMD_SetPreset:
			ret = onvif_SetPreset(self, fd, buff_rcv);
			break;
		case CMD_Stop:
			ret = onvif_Stop(self, fd, buff_rcv);
			break;
		case CMD_FocusStop:
			ret = onvif_FocusStop(self, fd, buff_rcv);
			break;
		case CMD_SetMetadataConfiguration:
			ret = onvif_SetMetadataConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetAnalyticsModules :		// yys onvif
			ret = onvif_GetAnalyticsModules(self, fd, buff_rcv);
			break;
		case CMD_ModifyAnalyticsModules :
			ret = onvif_ModifyAnalyticsModules(self, fd, buff_rcv);
			break;
		case CMD_GetRules :
			ret = onvif_GetRules(self, fd, buff_rcv);
			break;
		case CMD_ModifyRules :
			ret = onvif_ModifyRules(self, fd, buff_rcv);
			break;
		case CMD_GetVideoAnalyticsConfiguration :
			ret = onvif_GetVideoAnalyticsConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetVideoAnalyticsConfigurations :
			ret = onvif_GetVideoAnalyticsConfigurations(self, fd, buff_rcv);
			break;
        case CMD_GetAnalyticsConfigurations2 :
			ret = onvif_GetAnalyticsConfigurations2(self, fd, buff_rcv);
			break;
		case CMD_SetVideoAnalyticsConfiguration :
			ret = onvif_SetVideoAnalyticsConfiguration(self, fd, buff_rcv);
			break;
		case CMD_AddVideoAnalyticsConfiguration :
			ret = onvif_AddVideoAnalyticsConfiguration(self, fd, buff_rcv);
			break;
		case CMD_RemoveVideoAnalyticsConfiguration :
			ret = onvif_RemoveVideoAnalyticsConfiguration(self, fd, buff_rcv);
			break;
		case CMD_VAnalytics_Check_Token :	// yys onvif
			ret = onvif_VAnalytics_Check_Token(self, fd, buff_rcv);
			break;
		case CMD_GetMetadataConfigurationOptions :
			ret = onvif_GetMetadataConfigurationOptions(self, fd, buff_rcv);
			break;
		case CMD_SetSynchronizationPoint_trt :
			ret = onvif_SetSynchronizationPoint_trt(self, fd, buff_rcv);
			break;
		case CMD_RemoveConfiguration :
			ret = onvif_RemoveConfiguration(self, fd, buff_rcv);
			break;
        case CMD_AddConfiguration :
			ret = onvif_AddConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetPTZNodes:
			ret = onvif_GetNodes(self, fd, buff_rcv);
			break;
		case CMD_GetPTZNode:
			ret = onvif_GetNode(self, fd, buff_rcv);
			break;
		case CMD_GetPTZConfigurations:
			ret = onvif_GetPTZConfigurations(self, fd, buff_rcv);
			break;
		case CMD_GetPTZConfiguration:
			ret = onvif_GetPTZConfiguration(self, fd, buff_rcv);
			break;
		case CMD_RemovePTZConfiguration:
			ret = onvif_RemovePTZConfiguration(self, fd, buff_rcv);
			break;
		case CMD_AddPTZConfiguration:
			ret = onvif_AddPTZConfiguration(self, fd, buff_rcv);
			break;
			// now profile G
		case CMD_GetRecordings:
			ret = onvif_GetRecordings(self, fd, buff_rcv);
			break;
		case CMD_CreateRecording:
			ret = onvif_CreateRecording(self, fd, buff_rcv);
			break;
		case CMD_CreateRecordingJob:
			ret = onvif_CreateRecordingJob(self, fd, buff_rcv);
			break;
		case CMD_CreateTrack:
			ret = onvif_CreateTrack(self, fd, buff_rcv);
			break;
		case CMD_DeleteRecording:
			ret = onvif_DeleteRecording(self, fd, buff_rcv);
			break;
		case CMD_DeleteRecordingJob:
			_TTY_LOG_ONVIF("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			ret = onvif_DeleteRecordingJob(self, fd, buff_rcv);
			break;
		case CMD_DeleteTrack:
			ret = onvif_DeleteTrack(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingConfiguration:
			ret = onvif_GetRecordingConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingJobConfiguration:
			ret = onvif_GetRecordingJobConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingJobs:
			ret = onvif_GetRecordingJobs(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingJobState:
			ret = onvif_GetRecordingJobState(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingOptions:
			ret = onvif_GetRecordingOptions(self, fd, buff_rcv);
			break;
		case CMD_GetTrackConfiguration:
			ret = onvif_GetTrackConfiguration(self, fd, buff_rcv);
			break;
		case CMD_SetRecordingConfiguration:
			ret = onvif_SetRecordingConfiguration(self, fd, buff_rcv);
			break;
		case CMD_SetRecordingJobConfiguration:
			ret = onvif_SetRecordingJobConfiguration(self, fd, buff_rcv);
			break;
		case CMD_SetRecordingJobMode:
			ret = onvif_SetRecordingJobMode(self, fd, buff_rcv);
			break;
		case CMD_SetTrackConfiguration:
			ret = onvif_SetTrackConfiguration(self, fd, buff_rcv);
			break;
		case CMD_EndSearch:
			ret = onvif_EndSearch(self, fd, buff_rcv);
			break;
		case CMD_FindEvent:
			ret = onvif_FindRecordings(self, fd, buff_rcv);
			//ret = onvif_FindEvent(self, fd, buff_rcv);
			break;
		case CMD_FindMetadata:
			ret = onvif_FindMetadata(self, fd, buff_rcv);
			break;
		case CMD_FindPTZPosition:
			ret = onvif_FindPTZPosition(self, fd, buff_rcv);
			break;
		case CMD_FindRecordings:
			ret = onvif_FindRecordings(self, fd, buff_rcv);
			break;
		case CMD_GetEventSearchResults:
			//ret = onvif_GetEventSearchResults(self, fd, buff_rcv);
			ret = onvif_GetRecordingSearchResults(self, fd, buff_rcv);
			break;
		case CMD_GetMediaAttributes:
			ret = onvif_GetMediaAttributes(self, fd, buff_rcv);
			break;
		case CMD_GetMetadataSearchResults:
			ret = onvif_GetMetadataSearchResults(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingInformation:
			ret = onvif_GetRecordingInformation(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingSearchResults:
			ret = onvif_GetRecordingSearchResults(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingSummary:
			ret = onvif_GetRecordingSummary(self, fd, buff_rcv);
			break;
		case CMD_GetSearchState:
			ret = onvif_GetSearchState(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingServiceCapabilities:
			ret = onvif_GetRecordingServiceCapabilities(self, fd, buff_rcv);
			break;
		case CMD_GetReplayConfiguration:
			ret = onvif_GetReplayConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetReplayUri:
			ret = onvif_GetReplayUri(self, fd, buff_rcv);
			break;
		case CMD_SetReplayConfiguration:
			ret = onvif_SetReplayConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetRecordingJobStateChangeEvent:
			ret = onvif_GetRecordingJobStateChangeEvent(self, fd, buff_rcv);
			break;
		case CMD_CreatePullPointSubscription:
			ret = onvif_CreatePullPointSubscription(self, fd, buff_rcv);
			break;			
		case CMD_Subscribe:
			ret = onvif_Subscribe(self, fd, buff_rcv);
			break;			
		case CMD_SetSynchronizationPoint:
			ret = onvif_SetSynchronizationPoint(self, fd, buff_rcv);
			break;			
		case CMD_Unsubscribe:
			ret = onvif_Unsubscribe(self, fd, buff_rcv);
			break;
		case CMD_Renew:
			ret = onvif_Renew(self, fd, buff_rcv);
			break;
		case CMD_GetRelayOptions:
			ret = onvif_GetRelayOptions(self, fd, buff_rcv);
			break;				
		case CMD_GetDigitalInputs:
			ret = onvif_GetDigitalInputs(self, fd, buff_rcv);
			break;			
		case CMD_CheckDeviceReady:
			ret = onvif_CheckDeviceReady(self, fd, buff_rcv);
			break;			
		case CMD_GetServices:
			ret = onvif_GetServices(self, fd, buff_rcv);
			break;			
		case CMD_GetGuaranteedNumberOfVideoEncoderInstances:
			ret = onvif_GetGuaranteedNumberOfVideoEncoderInstances(self, fd, buff_rcv);
			break;
		case CMD_GetSnapshotUri:
			ret = onvif_GetSnapshotUri(self, fd, buff_rcv);
			break;
		case CMD_GetAudioOutput:
			ret = onvif_GetAudioOutput(self, fd, buff_rcv);
			break;
		case CMD_GetAudioOutput2:
			ret = onvif_GetAudioOutput2(self, fd, buff_rcv);
			break;
		case CMD_GetAudioDecoder2:
			ret = onvif_GetAudioDecoder2(self, fd, buff_rcv);
			break;
		case CMD_GetAudioOutputConfiguration:
			ret = onvif_GetAudioOutputConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetAudioOutputConfigurations:
			ret = onvif_GetAudioOutputConfigurations(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleAudioOutput:
			ret = onvif_GetCompatibleAudioOutput(self, fd, buff_rcv);
			break;
		case CMD_StopMulticastStreaming:
			ret = onvif_StopMulticastStreaming(self, fd, buff_rcv);
			break;
		case CMD_StartMulticastStreaming:
			ret = onvif_StartMulticastStreaming(self, fd, buff_rcv);
			break;
		case CMD_GetCompatibleConfigurations:
			ret = onvif_GetCompatibleConfigurations(self, fd, buff_rcv);
			break;
		case CMD_StartFirmwareUpgrade:
			ret = onvif_StartFirmwareUpgrade(self, fd, buff_rcv);
			break;
		case CMD_CreateOSD:
			ret = onvif_CreateOSD(self, fd, buff_rcv);
			break;
		case CMD_GetOSD:
			ret = onvif_GetOSD(self, fd, buff_rcv);
			break;
		case CMD_SetOSD:
			ret = onvif_CreateOSD(self, fd, buff_rcv);
			break;
		case CMD_DeleteOSD:
			ret = onvif_DeleteOSD(self, fd, buff_rcv);
			break;
		case CMD_GetHttpAuthMethod:
			ret = onvif_GetHttpAuthMethod(self, fd, buff_rcv);
			break;

		default:
			//saving_status_file(0);
			g_warning("%s command code error[%d]!", __FUNCTION__, header.code);
			return ONVIF_ERR_RET_INTERNAL;
		}
		before_cmd = header.code;
	}
	
	if (ret != ONVIF_ERR_RET_SUCCESS) {
		//saving_status_file(0);
		if (nf_onvif_send_fault(fd, ret, buff_rcv) != ONVIF_ERR_RET_SUCCESS) {
			printf("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			return ONVIF_ERR_RET_INTERNAL;
		}

		g_warning("%s error ret[%d] header.code[%d]", __FUNCTION__, ret, header.code);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (ret : %d) (Line : %d)", __FUNCTION__, ret, __LINE__);
		g_return_val_if_fail(ret == ONVIF_ERR_RET_SUCCESS, ONVIF_ERR_RET_INTERNAL);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	}

	return ONVIF_ERR_RET_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/* Response Function */
//////////////////////////////////////////////////////////////////////////////////////////////
static int _onvif_packet_save(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	gint cate;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(gint);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&cate, packet.data, sizeof(gint));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(gint)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

/****************************************************
 func
 *****************************************************/
int isValidVideoSourceToken(char *token)
{
	int i = 0;
	int result = 0;
	
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;

	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(token, tmp_token)) {
			return i;
		}
	}
	return -1;
}
int isValidProfileToken(char *token)
{
	int i = 0;
	int result = 0;
	
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	for (i = 0; i < MAX_PROFILE; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(token, tmp_token)) {
			return i % ONVIF_CH;
		}
	}
	return -1;
}
int isValidPtzToken(char *token)
{
	int i = 0;
	int result = 0;
	
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	for (i = 0; i < ONVIF_CH; i++) {
		sprintf(buff, "onvif.ptz%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(token, tmp_token)) {
			return i % ONVIF_CH;
		}
	}
	return -1;
}
int isValidPtzNodeToken(char *token)
{
	int i = 0;
	int result = 0;
	
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	for (i = 0; i < ONVIF_CH; i++) {
		sprintf(buff, "onvif.ptz%d.node_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(token, tmp_token)) {
			return i;
		}
	}
	return -1;
}

/****************************************************
 Device Service
 *****************************************************/
/* WARNING : "excute_later_secs" is not thread safe */
static func_interval func_data;

static void later_thread_func(void *arg) {
	func_interval f;
	void (*pfunc)(void);

	memset(&f, 0x00, sizeof(func_interval));
	memcpy(&f, (func_interval *) arg, sizeof(func_interval));

	pfunc = f.func;
	sleep(f.data);

	pfunc();
}

static int excute_later_secs(void *func, unsigned int interval) {
	pthread_t id = 0;

	memset(&func_data, 0x00, sizeof(func_interval));

	func_data.func = func;
	func_data.data = interval;

	if (pthread_create(&id, NULL, (void*) later_thread_func, (void*) &func_data)
			!= 0) {
		printf("thread create error\n");
		return -1;
	}

	pthread_detach(id);
	return 1;
}

int onvif_CheckTokens(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Tokens tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));

	// To DO

	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetHostName(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_HostName tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_HostName);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_HostName));

	// To DO
	size_t len;

	tmp.FromDHCP = nf_sysdb_get_bool("onvif.common.host_dhcp");
	strncpy(tmp.Name, nf_sysdb_get_str_nocopy("onvif.common.hostname"),
			COMMON_SIZE - 1);
	len = strlen(tmp.Name);
	if (len >= COMMON_SIZE) {
		len = COMMON_SIZE - 1;
	}
	tmp.Name[len] = '\0';

	memcpy(packet.data, &tmp, sizeof(arg_HostName));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_HostName)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetHostName(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_HostName tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_HostName);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_HostName));

	// To DO
	IPSetupData ipdata;

	memset(&ipdata, 0x00, sizeof(IPSetupData));
	DAL_get_ipSetup_data(&ipdata);

	if (tmp.FromDHCP) {
		//Set DHCP -> get new HOST -> set STATIC
		//        set_dhcp_and_get_hostname(tmp.Name);
		strncpy(tmp.Name, "dhcp_host", strlen("dhcp_host"));
		nf_sysdb_set_str("onvif.common.hostname", tmp.Name);
	} else {
		nf_sysdb_set_str("onvif.common.hostname", tmp.Name);
	}
	nf_sysdb_set_bool("onvif.common.host_dhcp", tmp.FromDHCP);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	DAL_set_ipSetup_data(&ipdata);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_NET);
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
	memcpy(packet.data, &tmp, sizeof(arg_HostName));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_HostName)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetDNS(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DNS tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DNS);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DNS));

	// To DO

	NF_NETIF_GET_INFO ret_net_info;

	unsigned int dns_ip1, dns_ip2;
	struct sockaddr_in ddns1, ddns2;

	tmp.FromDHCP = nf_sysdb_get_bool("onvif.common.dns_dhcp");
	dns_ip1 = ext_if_get_dns(0);
	dns_ip2 = ext_if_get_dns(1);
	dns_ip1 = ntohl(dns_ip1); 
	dns_ip2 = ntohl(dns_ip2); 

	if (dns_ip1 == 0)
		memset(tmp.IPv4Address[0], 0x00, sizeof(tmp.IPv4Address[0]));
	else
		sprintf(tmp.IPv4Address[0], "%d.%d.%d.%d", PRINT_IP(dns_ip1));

	if (dns_ip2 == 0)
		memset(tmp.IPv4Address[1], 0x00, sizeof(tmp.IPv4Address[1]));
	else
		sprintf(tmp.IPv4Address[1], "%d.%d.%d.%d", PRINT_IP(dns_ip2));

	snprintf(tmp.SearchDomain, COMMON_SIZE - 1, "%s", nf_sysdb_get_str_nocopy("onvif.common.search_domain"));
	_TTY_LOG_ONVIF("SDNS[%s]", tmp.SearchDomain);

	memcpy(packet.data, &tmp, sizeof(arg_DNS));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_DNS)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetDNS(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DNS tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DNS);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DNS));

	// To DO
	/* Set new config */
	if (!tmp.FromDHCP) {
		if (tmp.IPv4Address[0][0] != '\0') {
			nf_sysdb_set_uint("net.proto.dns1",	ip_str_to_uint(tmp.IPv4Address[0]));
		}
		if (tmp.IPv4Address[1][0] != '\0') {
			nf_sysdb_set_uint("net.proto.dns2",	ip_str_to_uint(tmp.IPv4Address[1]));
		}
	}
	nf_sysdb_set_str("onvif.common.search_domain", tmp.SearchDomain);
	nf_sysdb_set_bool("onvif.common.dns_dhcp", tmp.FromDHCP);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_NET);

	nf_netif_apply_dns();

	memcpy(packet.data, &tmp, sizeof(arg_DNS));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_DNS)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetNTP(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NTP tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NTP);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NTP));

	// To DO
	int i;
	size_t len;
	char timesvr[COMMON_SIZE] = {0,};

	memset(timesvr, 0x00, sizeof(char)*COMMON_SIZE);
	strncpy(timesvr, nf_sysdb_get_str_nocopy("sys.date.timesvr"), COMMON_SIZE - 1);

	if (!strncmp(timesvr, "none", strlen("none"))) {
		return ONVIF_R_ERR;
	}
	else if (!strncmp(timesvr, "auto", strlen("auto"))) {
		//FIXME.baek.
		strncpy(tmp.NTPname, "pool.ntp.org", strlen("pool.ntp.org"));
		tmp.FromDHCP = 1;
	}
	else{
		strncpy(tmp.NTPname, timesvr, COMMON_SIZE - 1);
		tmp.FromDHCP = 0;
	}

	len = strlen(tmp.NTPname);
	if (len >= COMMON_SIZE) {
		len = COMMON_SIZE - 1;
	}
	tmp.is_domain = 0;
	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>> get NTP [%s]", tmp.NTPname);
	for (i = 0; i < (int) len; i++) {
		if ((tmp.NTPname[i] >= 'a' && tmp.NTPname[i] <= 'z')
				|| (tmp.NTPname[i] >= 'A' && tmp.NTPname[i] <= 'Z')) {
			tmp.is_domain = 1;
			break;
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_NTP));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_NTP)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetNTP(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NTP tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NTP);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NTP));

	// To DO
	guint idx = 0;


	if( tmp.FromDHCP ){
		nf_sysdb_set_str("sys.date.timesvr", "auto");
	}
	else {
		if (tmp.NTPname[0] == '\0') {
			return ONVIF_R_ERR;
		}
		nf_sysdb_set_str("sys.date.timesvr", tmp.NTPname);
	}
	
	idx = nf_sysdb_get_uint("sys.date.tz_index");	
	scm_apply_timezone(idx, NULL);
	scm_init_timesync();

nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_SYS);
	sleep(5);

	memcpy(packet.data, &tmp, sizeof(arg_NTP));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_NTP)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetDateTime(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DateTime tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DateTime);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DateTime));

	// To DO
	time_t the_time, the_time_utc;
	struct tm *tm_ptr, *tm_ptr_utc;

	DateTimeData dtdata;
	NF_TZINFO_ACTIVEX tzout;
	char tz_name[128] = { 0, };
	unsigned int tz_idx = 0;

	memset(&tzout, 0, sizeof(NF_TZINFO_ACTIVEX));

	DAL_get_dateTime_data(&dtdata);
	tmp.Manual_NTP = (int) nf_sysdb_get_bool("sys.date.auto_sync");
	tmp.DST = nf_sysdb_get_bool("sys.date.daylight");
	_TTY_LOG_ONVIF("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< DST[%d] dt[%d]", tmp.DST, dtdata.dst);
	_TTY_LOG_ONVIF("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< SYNC[%d] dt[%d]",	tmp.Manual_NTP, dtdata.auto_timesync);
	time(&the_time);
	tm_ptr = localtime(&the_time);

	tmp.loYear = tm_ptr->tm_year + 1900;
	tmp.loMonth = tm_ptr->tm_mon + 1;
	tmp.loDay = tm_ptr->tm_mday;
	tmp.loHour = tm_ptr->tm_hour;
	tmp.loMinute = tm_ptr->tm_min;
	tmp.loSecond = tm_ptr->tm_sec;

	time(&the_time_utc);
	tm_ptr_utc = gmtime(&the_time_utc);

	tmp.Year = tm_ptr_utc->tm_year + 1900;
	tmp.Month = tm_ptr_utc->tm_mon + 1;
	tmp.Day = tm_ptr_utc->tm_mday;
	tmp.Hour = tm_ptr_utc->tm_hour;
	tmp.Minute = tm_ptr_utc->tm_min;
	tmp.Second = tm_ptr_utc->tm_sec;
	_TTY_LOG_ONVIF("y[%d] m[%d] d[%d] h[%d] m[%d] s[%d]\n",	tmp.Year, tmp.Month, tmp.Day, tmp.Hour, tmp.Minute, tmp.Second);

	tz_idx = nf_sysdb_get_uint("sys.date.tz_index");
	if (tz_idx >= (unsigned int) nf_zoneinfo_get_count()) {
		return ONVIF_R_ERR;
	}

	strncpy(tz_name, nf_zoneinfo_get_string((gint) tz_idx), sizeof(tz_name) - 1);
	nf_zoneinfo_get_activex_posix_onvif(tz_name, &tzout);

	if (tzout.tz_cnt > 1) {
		snprintf(tmp.TimeZone, COMMON_SIZE, "%s", tzout.tz_arr[tmp.loYear - 2008].string);
	} else {
		snprintf(tmp.TimeZone, COMMON_SIZE, "%s", tzout.tz_arr[0].string);
	}

	memcpy(packet.data, &tmp, sizeof(arg_DateTime));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_DateTime)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetDateTime(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DateTime tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DateTime);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DateTime));

	// To DO
	struct tm tm_ptr;
	DateTimeData dtdata;
	GTimeVal post_time;
	int ret = 0, tz_count = 0, i = 0;
	char server_name[64] = { 0, }, tz_tmp[64] = { 0, };

	memset(server_name, 0x00, sizeof(char) * 64);

	DAL_get_dateTime_data(&dtdata);

	dtdata.dst = (guint)tmp.DST;
	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DST[%d]dt[%d]", tmp.DST, dtdata.dst);
	tz_count = nf_zoneinfo_get_count();
	for (i = 0; i < tz_count; i++) {
		strcpy(tz_tmp, nf_zoneinfo_get_string(i));
		if (!strcmp(tmp.TimeZone, tz_tmp)) {
			dtdata.timeZone = (guint)i;
			//nf_sysdb_set_uint("sys.date.tz_index", i);
			break;
		}
		//baek.FIXME
		if (!strncmp(tmp.TimeZone, "PST8PDT", 7)) {
			dtdata.timeZone = 4;
			//nf_sysdb_set_uint("sys.date.tz_index", 4);
			break;
		}
	}

	// convert time format
	if (tmp.Manual_NTP == DateTimeType__Manual) {
		if ((tmp.Year > 3000 || tmp.Year < 2007)
				|| (tmp.Month < 1 || tmp.Month > 12)
				|| (tmp.Day < 1 || tmp.Day > 31)
				|| (tmp.Hour < 0 || tmp.Hour > 23)
				|| (tmp.Minute < 0 || tmp.Minute > 59)
				|| (tmp.Second < 0 || tmp.Second > 59)) {
			_TTY_LOG_ONVIF("invalid time");
			return ONVIF_R_ERR;
		} else {
			tm_ptr.tm_year = tmp.Year - 1900;
			tm_ptr.tm_mon = tmp.Month -1 ;
			tm_ptr.tm_mday = tmp.Day;
			tm_ptr.tm_hour = tmp.Hour;
			tm_ptr.tm_min = tmp.Minute;
			tm_ptr.tm_sec = tmp.Second;
			tm_ptr.tm_isdst = tmp.DST;
			//post_time.tv_sec = mktime(&tm_ptr);
			post_time.tv_sec = mktime(&tm_ptr) + tm_ptr.tm_gmtoff;
			_TTY_LOG_ONVIF("..................................[%u]", (unsigned int)post_time.tv_sec);
			scm_set_system_time(&post_time);
			dtdata.auto_timesync = 0;
		}
	} else if (tmp.Manual_NTP == DateTimeType__NTP) {
		dtdata.auto_timesync = 1;
		ret = scm_get_ntp_time(dtdata.timeServer, &post_time);
		if (ret < 0 || post_time.tv_sec == 0) {
			_TTY_LOG_ONVIF("NTP sync fail!!");
			return ONVIF_R_ERR;
		}
		// scm_set_system_time(&post_time);
	}
	//sprintf(dtdata.timeServer, "www.test.ggg");
	scm_put_log(CHANGE_SYS_TIME, 0, 0);
	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DST[%d]dt[%d]", tmp.DST, dtdata.dst);
	DAL_set_dateTime_data(&dtdata);
	scm_init_timesync();
//	VW_SetupSystem_set_changeflag(1);
	smt_set_service(SMT_TIME_CHANGE);
	// scm_apply_time(IRET_SCM_CHANGE_SYSTEM_TIME);
	nf_datetime_set(&post_time);

	sleep(3);

	memcpy(packet.data, &tmp, sizeof(arg_DateTime));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_DateTime)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_reboot(void)
{
//	nf_sysman_poweroff(1);
	proxy_system("killall nginx",1,3);
	proxy_system("killall lighttpd",1,3);
	sleep(3);
	printf("onvif_reboot\n");
	nf_dev_board_reset();
	return 1;
}


int onvif_FactoryDefault(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_FactoryDefault tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_FactoryDefault);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_FactoryDefault));

	onvif_service_stop();


	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_FactoryDefault)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	if(tmp.soft == 0){
		printf("[%s] HARD\n",  __FUNCTION__ );
		nf_sysdb_default("");
	}
	else{
		printf("[%s] SOFT\n",  __FUNCTION__ );		
		nf_sysdb_default_onvif("");
	}
	
	// sysdb_save_helper(-1);
	
	//scm_run_factory_default(IRET_FACTORY_DEFAULT_NOTIFY);	
	excute_later_secs(onvif_reboot, 10);

	return ONVIF_ERR_RET_SUCCESS;
}


int onvif_SystemReboot(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_SystemReboot tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_SystemReboot);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_SystemReboot));

	onvif_service_stop();

	memcpy(packet.data, &tmp, sizeof(arg_SystemReboot));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_SystemReboot)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	// To DO
	excute_later_secs(onvif_reboot, 5);
	
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetNetworkInfo(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NetworkInfo tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NetworkInfo);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NetworkInfo));

	// To DO
	NF_NETIF_GET_INFO ret_net_info;
	NF_NETIF_MAC mac_addr;
	struct sockaddr_in ipaddr, subnet, ddns1, ddns2, gateway;

	/* MAC */
	sprintf(tmp.macaddr, "%s", nf_sysdb_get_str_nocopy("sys.info.mac_colon"));

	/* dhcp */
	tmp.dhcp_flag = get_dhcpon_ipaddr();

	/* ipaddr, gateway, subnet */
	nf_netif_get_info(&ret_net_info);

	ipaddr.sin_addr.s_addr = (unsigned long) ret_net_info.ipaddr;
	subnet.sin_addr.s_addr = (unsigned long) ret_net_info.netmask;
	gateway.sin_addr.s_addr = (unsigned long) ret_net_info.gateway;
	ddns1.sin_addr.s_addr = (unsigned long) ret_net_info.dnsserver1;
	ddns2.sin_addr.s_addr = (unsigned long) ret_net_info.dnsserver2;

	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);
	subnet.sin_addr.s_addr = htonl(subnet.sin_addr.s_addr);
	gateway.sin_addr.s_addr = htonl(gateway.sin_addr.s_addr);
	ddns1.sin_addr.s_addr = htonl(ddns1.sin_addr.s_addr);
	ddns2.sin_addr.s_addr = htonl(ddns2.sin_addr.s_addr);

	tmp.prefix_length = (int)get_subnet_prefix_from_subnet(htonl((unsigned long) ret_net_info.netmask));
	snprintf(tmp.ipaddr, sizeof(tmp.ipaddr), "%s", inet_ntoa(ipaddr.sin_addr));
	snprintf(tmp.gateway, sizeof(tmp.gateway), "%s", inet_ntoa(gateway.sin_addr));

	//htonl((unsigned long)ret_net_info.dnsserver1);
	//htonl((unsigned long)ret_net_info.dnsserver2);

	_TTY_LOG_ONVIF("i[%s]", tmp.ipaddr);
	_TTY_LOG_ONVIF("g[%s]", tmp.gateway);
	_TTY_LOG_ONVIF("mac[%s]", tmp.macaddr);

	memcpy(packet.data, &tmp, sizeof(arg_NetworkInfo));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_NetworkInfo)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetNetworkInfo(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NetworkInfo tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NetworkInfo);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NetworkInfo));

	// To DO
	IPSetupData ipdata;

	memset(&ipdata, 0x00, sizeof(IPSetupData));

	/*Load current config */
	DAL_get_ipSetup_data(&ipdata);
	_TTY_LOG_ONVIF("ip[%s]", tmp.ipaddr);

	/* Set new config */
	if (tmp.dhcp_flag != -1) {
		ipdata.dhcp = (guint)tmp.dhcp_flag;
		nf_sysdb_set_bool("onvif.common.ipaddr_dhcp", tmp.dhcp_flag);
	}

	if (tmp.prefix_length > 0 && tmp.prefix_length < 32) {
		convert_prefix_to_netmask((unsigned int)tmp.prefix_length, ipdata.subnet);
	}
	prvIntToIP(ipdata.ip, ip_str_to_uint(tmp.ipaddr));
	prvIntToIP(ipdata.gateway, ip_str_to_uint(tmp.gateway));

	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>> ip change start");
	_TTY_LOG_ONVIF("ipdata->dhcp [%d]", ipdata.dhcp);
	_TTY_LOG_ONVIF("ipdata->webServ [%d]", ipdata.webServ);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->ip [%d][%d][%d][%d]", ipdata.ip[0], ipdata.ip[1], ipdata.ip[2], ipdata.ip[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->gateway [%d][%d][%d][%d]", ipdata.gateway[0], ipdata.gateway[1], ipdata.gateway[2], ipdata.gateway[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->subnet [%d][%d][%d][%d]", ipdata.subnet[0], ipdata.subnet[1], ipdata.subnet[2], ipdata.subnet[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->dns1[%d][%d][%d][%d]",
			ipdata.dns1[0], ipdata.dns1[1], ipdata.dns1[2], ipdata.dns1[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->dns2[%d][%d][%d][%d]",
			ipdata.dns2[0], ipdata.dns2[1], ipdata.dns2[2], ipdata.dns2[3]);
	_TTY_LOG_ONVIF("ipdata->netPort[%d]", ipdata.netPort);
	_TTY_LOG_ONVIF("ipdata->webPort[%d]", ipdata.webPort);
	_TTY_LOG_ONVIF("ipdata->txSpeed[%d]", ipdata.txSpeed);

	multi_bye();
	
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	DAL_set_ipSetup_data(&ipdata);
	sysdb_save_cate(NF_SYSDB_CATE_NET);
	//DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);

	memcpy(packet.data, &tmp, sizeof(arg_NetworkInfo));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_NetworkInfo)) != ONVIF_ERR_RET_SUCCESS) {
//		return ONVIF_ERR_RET_INTERNAL;
	}
	
	sleep(3);	
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
#if 0 //[[ hun_0140624_BEGIN -- del
#ifdef _ONVIF_STAND_ALONE	
	onvif_restart();
#endif
#endif //]] hun_0140624_END -- del
	sleep(5);
	excute_later_secs(multi_hello, 20);
	
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetNetworkPort(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	int https = 0;
	arg_NetworkPort tmp;
	
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NetworkPort);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NetworkPort));

	// To DO
	tmp.is_http = 0;
	tmp.is_rtsp = 1;	
#if 0 //_ONVIF_STAND_ALONE
	tmp.http_port = (int)nf_sysdb_get_uint("net.http.onvifport");
#else
	tmp.http_port = (int)nf_sysdb_get_uint("net.proto.webport");
#endif
	tmp.http_port = (int)nf_sysdb_get_uint("net.proto.webport");
	tmp.rtsp_port = (int)nf_sysdb_get_uint("net.proto.rtspport");
	tmp.https_port = (int)nf_sysdb_get_uint("net.proto.sslport");
	
	tmp.rtsp_enable = (int)nf_sysdb_get_bool("onvif.common.rtsp_enable");

	//https  = nf_sysdb_get_uint("net.http.sslon");
	https = (int)nf_sysdb_get_bool("net.proto.httpson");
	if( https == 0 ){ // https off
		tmp.is_http = 1;
		tmp.http_enable = 1;
		tmp.is_https = 0;
		tmp.https_enable = 0;
	}
	else if( https == 1 ){ // ssl optional
		tmp.is_http = 0;
		tmp.http_enable = 0;
		tmp.is_https = 1;
		tmp.https_enable = 1;
	}
	else { // ssl required
		tmp.is_http = 1;
		tmp.http_enable = 1;
		tmp.is_https = 0;
		tmp.https_enable = 0;
	}

	memcpy(packet.data, &tmp, sizeof(arg_NetworkPort));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NetworkPort)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetNetworkPort(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NetworkPort tmp;
	int restart_onvif_service = 0, change_rtsp=0,  change_http=0;
	unsigned int ssl_mode = 0, https_enable=0;


	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NetworkPort);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NetworkPort));

	// To DO
	IPSetupData ipdata;

	memset(&ipdata, 0x00, sizeof(IPSetupData));

	/* Load current config */
	load_current_net_conf(&ipdata);
	ssl_mode = nf_sysdb_get_bool("net.proto.httpson");
	if( ssl_mode == 1 || ssl_mode == 2 ){
		https_enable = 1;
	}

	/* Set New value */
	_TTY_LOG_ONVIF("tmp.http_port[%d]", tmp.http_port);
	_TTY_LOG_ONVIF("tmp.http_enable[%d]", tmp.http_enable);
	_TTY_LOG_ONVIF("tmp.rtsp_port[%d]", tmp.rtsp_port);
	_TTY_LOG_ONVIF("tmp.rtsp_enable[%d]", tmp.rtsp_enable);

	if (tmp.http_enable != -1) {
		ipdata.webServ = (guint)tmp.http_enable;
	}

	if (tmp.https_enable != -1) {
		https_enable = tmp.https_enable;
	}
	
	if( ipdata.webServ == 1 && https_enable == 0 ){
		ssl_mode = 0; // http only
	}
	else if ( ipdata.webServ == 1 && https_enable == 1 ){
		ssl_mode = 1; // ssl optional
	}
	else if ( ipdata.webServ == 0 && https_enable == 1 ){
		ssl_mode = 2; // ssl required
	}
	else {
		ssl_mode = 0; // http only
	}

	if( ssl_mode != nf_sysdb_get_bool("net.proto.httpson") ){
		nf_sysdb_set_bool("net.proto.httpson", ssl_mode);
		change_http = 1;
	}
#if 0
//#ifdef _ONVIF_STAND_ALONE
	if (tmp.http_port != -1){
		if( tmp.http_port != nf_sysdb_get_uint("net.http.onvifport") ){
			restart_onvif_service = 1;
		}
		else{
			restart_onvif_service = 0;
		}
		nf_sysdb_set_uint("net.http.onvifport", tmp.http_port);
	}
#else
	if (tmp.http_port != -1 && tmp.http_port != ipdata.webPort ){
		ipdata.webPort = (guint)tmp.http_port;
		change_http = 1;
	}
	if (tmp.https_port != -1 && tmp.https_port != nf_sysdb_get_uint("net.proto.sslport") ){
		nf_sysdb_set_uint("net.proto.sslport", tmp.https_port);
		change_http = 1;
	}	
#endif

	if (tmp.rtsp_enable != -1 && tmp.rtsp_enable != nf_sysdb_get_bool("onvif.common.rtsp_enable") ) {
		nf_sysdb_set_bool("onvif.common.rtsp_enable", tmp.rtsp_enable);
		change_rtsp = 1;
	}

	if (tmp.rtsp_port != -1 && tmp.rtsp_port != ipdata.rtspport ){
		ipdata.rtspport = (guint)tmp.rtsp_port;
		change_rtsp = 1;
	}


	if( change_rtsp || change_http ){
		/* Apply & Save db */
		multi_bye();		
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
		DAL_set_ipSetup_data(&ipdata);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_NET);
		scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);

#if 0
		if( restart_onvif_service ){
			onvif_restart();
		}
#endif	
		sleep(5);
		excute_later_secs(multi_hello, 10);
	}
	memcpy(packet.data, &tmp, sizeof(arg_NetworkPort));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_NetworkPort)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int get_scope(arg_Scope *tmp) 
{
	int scope_cnt;
	char buff[COMMON_SIZE*2] = { 0, };
	int i;

	scope_cnt = (int) nf_sysdb_get_uint("onvif.common.scope_cnt");

	for (i = 0; i < scope_cnt; i++) {
		sprintf(buff, "onvif.scope%d.name", i);
		strncpy(tmp->scope[i], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE*2 - 1);
		sprintf(buff, "onvif.scope%d.fixed", i);
		tmp->scope_fixed[i] = (int) nf_sysdb_get_uint(buff);
	}
	tmp->scope_cnt = (char) scope_cnt;

	return 1;
}

int onvif_GetScope(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Scope tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Scope);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Scope));

	// To DO
	get_scope(&tmp);

	memcpy(packet.data, &tmp, sizeof(arg_Scope));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetScope(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Scope tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Scope);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Scope));

	// To DO
	char buff[COMMON_SIZE*2] = { 0, };
	char tmp_scope[COMMON_SIZE*2] = { 0, };
	int fixed;
	int i, cnt, fixed_cnt = 0;
	int equal_flag[MAX_SCOPE] = {0,};
	int equal_cnt=0;

	tmp.result = 0;
	cnt = 0;
	//printf("start [%d]\n", tmp.scope_cnt);
	_TTY_LOG_ONVIF("start [%d]", tmp.scope_cnt);

	for (i = 0; i < MAX_SCOPE; i++) {
		sprintf(buff, "onvif.scope%d.fixed", i);
		fixed = (int) nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.scope%d.name", i);
		strncpy(tmp_scope, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE*2 -1);
		if (fixed) {
			for (cnt = 0; cnt < tmp.scope_cnt; cnt++) {
				if (!strcmp(tmp_scope, tmp.scope[cnt])) {
					printf("\e[31m ## Already Exist[%s] !! ##\e[0m\n", tmp_scope);
					equal_flag[cnt]=1;
					equal_cnt++;
					//return ONVIF_R_ERR;
				}
			}
		}
	}

	cnt = 0;
	for (i = 0; i < MAX_SCOPE; i++) {
		sprintf(buff, "onvif.scope%d.fixed", i);
		fixed = (int) nf_sysdb_get_uint(buff);
		if (!fixed) {
			sprintf(buff, "onvif.scope%d.name", i);
			if (cnt < tmp.scope_cnt) {
				if(equal_flag[cnt] == 1)
				{
					cnt++;
					i--;
				}
				else
				{
					printf("\e[31m ## i[%d] : %s ## \e[0m\n", i, tmp.scope[cnt]);
					nf_sysdb_set_str(buff, tmp.scope[cnt]);
					cnt++;
				}
			}
		} 
		else
			fixed_cnt++;
	}

	if (cnt == tmp.scope_cnt) {
		;
	} else {
		return ONVIF_R_ERR;
	}
	printf("\e[31m ## fixed_cnt[%d], scope_cnt[%d], double_cnt[%d] ##\e[0m\n", fixed_cnt, tmp.scope_cnt, equal_cnt);
	nf_sysdb_set_uint("onvif.common.scope_cnt", (unsigned int)(fixed_cnt + tmp.scope_cnt - equal_cnt));

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_Scope));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddScope(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Scope tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Scope);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Scope));

	// To DO
	int scope_cnt, added_cnt;
	char buff[COMMON_SIZE*2] = { 0, };
	int i, j;

	scope_cnt = (int) nf_sysdb_get_uint("onvif.common.scope_cnt");

	added_cnt = scope_cnt + tmp.scope_cnt;
	if (added_cnt > MAX_SCOPE) {
		return ONVIF_R_ERR;
	}

	nf_sysdb_set_uint("onvif.common.scope_cnt", (unsigned int) added_cnt);
	j = 0;
	for (i = scope_cnt; i < added_cnt; i++) {
		sprintf(buff, "onvif.scope%d.name", i);
		nf_sysdb_set_str(buff, tmp.scope[j++]);
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_timer_add(3, (GSourceFunc) multi_hello, NULL);

	memcpy(packet.data, &tmp, sizeof(arg_Scope));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveScope(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Scope tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Scope);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Scope));

	// To DO
	int scope_cnt;
	char buff[COMMON_SIZE*2] = { 0, };
	char tmp_scope[COMMON_SIZE*2] = { 0, };
	char org_scope[COMMON_SIZE*2] = { 0, };
	char next_scope[COMMON_SIZE*2] = { 0, };
	int i, j, k;
	int fixed, change;

	fixed = change = 0;
	scope_cnt = (int) nf_sysdb_get_uint("onvif.common.scope_cnt");

	for (k = 0; k < tmp.scope_cnt; k++) {
		for (i = 0; i < scope_cnt; i++) {
			sprintf(buff, "onvif.scope%d.name", i);
			strncpy(tmp_scope, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE*2 - 1);
			if (!strcmp(tmp_scope, tmp.scope[k])) {
				sprintf(buff, "onvif.scope%d.fixed", i);
				fixed = (int) nf_sysdb_get_uint(buff);
				if (fixed == 1) {
					return ONVIF_R_ERR;
				}

				sprintf(buff, "onvif.scope%d.name", i);
				nf_sysdb_set_str(buff, "");
				for (j = i; j < scope_cnt - 1; j++) {
					sprintf(org_scope, "onvif.scope%d.name", j);
					sprintf(buff, "onvif.scope%d.name", j + 1);
					strncpy(next_scope, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE*2 - 1);
					nf_sysdb_set_str(org_scope, next_scope);
					nf_sysdb_set_str(buff, "");
				}
				change++;
				break;
			}
		}
	}

	nf_sysdb_set_uint("onvif.common.scope_cnt",
			(unsigned int) (scope_cnt - change));
	get_scope(&tmp);

	scope_cnt = (int) nf_sysdb_get_uint("onvif.common.scope_cnt");

	if (change == 0)
		return ONVIF_R_ERR;

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_timer_add(3, (GSourceFunc) multi_hello, NULL);

	memcpy(packet.data, &tmp, sizeof(arg_Scope));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetDiscovery(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Discovery tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Discovery);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Discovery));

	// To DO
	int discovery_mode;

	discovery_mode = (int) nf_sysdb_get_uint("onvif.common.discovery");
	if (discovery_mode == 1)
		tmp.discovery = 0;
	else
		tmp.discovery = 1;

	memcpy(packet.data, &tmp, sizeof(arg_Discovery));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Discovery)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetDiscovery(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Discovery tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Discovery);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Discovery));

	// To DO
	int discovery_mode;

	if (tmp.discovery == 0) {
		nf_timer_add(3, (GSourceFunc) onvif_discovery_start, NULL);
		discovery_mode = 1;
	} else {
		nf_timer_add(3, (GSourceFunc) onvif_discovery_stop, NULL);
		discovery_mode = 0;
	}
	nf_sysdb_set_uint("onvif.common.discovery", (unsigned int) discovery_mode);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_Discovery));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Discovery)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

static void recv_fd(int local_fd, int *peer_fd)
{
    struct msghdr msg;
    struct cmsghdr *cmsgptr;
    struct iovec iov[1];
    int rv;
    char c;

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;

    iov[0].iov_base = (void *)&c;
    iov[0].iov_len = sizeof(char);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    rv = recvmsg(local_fd, &msg, 0);
    if(rv < 0) {
        printf("recvmsg errno : %d\n", errno);
        *peer_fd = -1;
        return;
    }

    cmsgptr = CMSG_FIRSTHDR(&msg);
    if(cmsgptr == NULL) {
        printf("cmstgptr is NULL\n");
        goto find_next;
    }
    if(cmsgptr->cmsg_len != CMSG_LEN(sizeof(int))) {
        printf("cmsg_len error\n");
        goto find_next;
    }
    if(cmsgptr->cmsg_level != SOL_SOCKET) {
        printf("cmsg_level error\n");
        goto find_next;
    }
    if(cmsgptr->cmsg_type != SCM_RIGHTS) {
        printf("cmsg_type error\n");
        goto find_next;
    }

    *peer_fd = *((int *)CMSG_DATA(cmsgptr));
    return;

find_next:
    {
        *peer_fd = -1;
        return;
    }
}

int onvif_Passfd(NfOnvif *self, int fd, char *buff_rcv)
{
    ONVIF_PACKET packet;
    ONVIF_HEADER header;
    guint packet_len;

    g_message("%s IN", __FUNCTION__);

    memset(&header, 0, sizeof(header));
    memcpy(&header, buff_rcv, sizeof(header));

    packet_len = sizeof(ONVIF_HEADER);

    memcpy(&packet, buff_rcv, packet_len);

    int i;
    int n_flags = 0;
    int pass_fd, pass_fd_dup;

    i = 1;
    recv_fd(fd, &pass_fd);

    pass_fd_dup = dup(pass_fd);
    close(pass_fd);

    n_flags = fcntl(pass_fd_dup, F_GETFL, 0);
    if(fcntl(pass_fd_dup, F_SETFL, n_flags | O_NONBLOCK) < 0)
    {
        fprintf(stderr, "%s fcntl F_SETFL error[%d]\n", __FUNCTION__, errno);
        return ONVIF_ERR_RET_SUCCESS;
    }
#if 1
    if(setsockopt(pass_fd_dup, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(i)) < 0)
    {
        fprintf(stderr, "%s fcntl TCP_NODELAY error[%s]\n", __FUNCTION__,strerror(errno));
        return ONVIF_ERR_RET_SUCCESS;
    }
#endif
	int ret =0;
	//baek.debug
	printf("\e[31m >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> HTTP FD[%d] <<<<<<<<<<<<<<<<<<<< \e[0m\n", pass_fd_dup);
	ret = issm_register_http_tunneling_fd(pass_fd_dup);
	printf("\e[31m >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Result[%d] <<<<<<<<<<<<<<<<<<<<<< \e[0m\n", ret);
    //tcp_passfd(packet.data, pass_fd_dup);

    return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetUser(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_UserType tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_UserType);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_UserType));

	// To DO
	int i, cnt = 0;
	char db_name[COMMON_SIZE] = { 0, };
	char tmp_grp[COMMON_SIZE] = { 0, };
	ONVIF_USR_MAN man[8];

	memset(db_name, 0x00, sizeof(char) * COMMON_SIZE);
	memset(tmp_grp, 0x00, sizeof(char) * COMMON_SIZE);

	load_current_usrman(man);

	tmp.user_cnt = man[0].usrcnt;

	for (i = 0; i < tmp.user_cnt; i++) {
		strncpy(tmp.name[i], man[i].usrid, sizeof(man[i].usrid));
		if (convert_group_to_onvif(tmp.grpname[i], man[i].grpname) < 0) {
			return ONVIF_R_ERR;
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetUser(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_UserType tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_UserType);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_UserType));

	// To DO
	int i, cnt = 0;
	char db_name[COMMON_SIZE] = { 0, };
	char tmp_grp[COMMON_SIZE] = { 0, };
	ONVIF_USR_MAN man[8];

	load_current_usrman(man);

	for (cnt = 0; cnt < tmp.user_cnt; cnt++) {
		for (i = 0; i < man[0].usrcnt; i++) {
			if (!strcmp(man[i].usrid, tmp.name[cnt])) {
				if (man[i].passwd == '\0') {
					return ONVIF_R_ERR_PASS_TOO_WEAK;
				}
				strncpy(man[i].passwd, tmp.pass[cnt], sizeof(char) * 32);
				if (convert_onvif_to_group(man[i].grpname, tmp.grpname[cnt])
						< 0) {
					return ONVIF_R_ERR;
				}
				break;
			}
		}
		if (i == man[0].usrcnt) {
			return ONVIF_R_ERR_USER_NOT_FOUND;
		}
	}

	/* save */
	save_current_usrman(man);
	nf_onvif_sysdbchange_make_authfile(man);

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_CreateUser(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_UserType tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_UserType);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_UserType));

	// To DO
	int i, j, success_cnt = 0;
	char tmp_name[COMMON_SIZE] = { 0, };
	char db_name[COMMON_SIZE] = { 0, };
	char buff[128];
	unsigned int cnt = 0, current_user_cnt = 0;
	ONVIF_USR_MAN man[8];
	GTimeVal tvTemp;

	memset(man, 0, sizeof(man));
	memset(tmp_name, 0x00, sizeof(char) * COMMON_SIZE);
	memset(db_name, 0x00, sizeof(char) * COMMON_SIZE);

	/* Load current usr information */
	current_user_cnt = nf_sysdb_get_uint("usr.UCNT");
	_TTY_LOG_ONVIF("> ucnt[%d]", current_user_cnt);

	if (((int) current_user_cnt + tmp.user_cnt) > USER_SIZE) {
		/* Fault : TooManyUsers */
		return ONVIF_R_ERR_TOO_MANY_USER;
	}

	load_current_usrman(man);

	/* Check Username Duplication */
	for (i = 0; i < tmp.user_cnt; i++) {
		for (j = 0; j < (int) current_user_cnt; j++) {
			if (!strcmp(man[j].usrid, tmp.name[i])) {
				/* Fault : Username already exists. */
				return ONVIF_R_ERR_USERNAME_CONFLICT;
			}
		}
	}

	/* Add New users */
	cnt = current_user_cnt;
	for (i = 0; i < tmp.user_cnt; i++) {
		strncpy(man[cnt].usrid, tmp.name[i], sizeof(tmp.name[i]));

		/* check weakness of the password */
		if (tmp.pass[i][0] != '\0') {
			strncpy(man[cnt].passwd, tmp.pass[i], sizeof(tmp.pass[i]));
			memset(&tvTemp, 0, sizeof(GTimeVal));
			g_get_current_time(&tvTemp);
			memset(buff, 0, sizeof(buff));
			man[cnt].pw_last_changed = (unsigned int) tvTemp.tv_sec;
		} else {
			/* Fault : Too weak password. */
			return ONVIF_R_ERR_PASS_TOO_WEAK;
		}

		if (convert_onvif_to_group(man[cnt].grpname, tmp.grpname[i]) < 0) {
			return ONVIF_R_ERR;
		}
		cnt++;
	}
	man[0].usrcnt = (unsigned char) (tmp.user_cnt + (int) current_user_cnt);

	/* save */
	save_current_usrman(man);
	nf_onvif_sysdbchange_make_authfile(man);
	_TTY_LOG_ONVIF(">>>>>>>>>>  create user done!!");

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_DeleteUser(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_UserType tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_UserType);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_UserType));

	// To DO
	int i, j, current_user_cnt = 0, new_cnt = 0, del_cnt = 0;
	ONVIF_USR_MAN man[8], new_man[8];

	/* Load current usr information */
	current_user_cnt = (int)nf_sysdb_get_uint("usr.UCNT");
	memset(man, 0, sizeof(man));
	memset(new_man, 0, sizeof(new_man));

	load_current_usrman(man);

	/* Delete users */
	for (i = 0; i < tmp.user_cnt; i++) {
		for (j = 0; j < current_user_cnt; j++) {
			if (man[j].usrid[0] != '\0') {
				if (strncmp("admin", tmp.name[i], sizeof(tmp.name[i])) == 0) {
					if (j == current_user_cnt) { // Fixed User error !!
						return ONVIF_R_ERR_FIXED_USER;
					}
				} else if (strncmp(man[j].usrid, tmp.name[i],
						sizeof(tmp.name[i])) == 0) {
					memset(man[j].usrid, 0x00, sizeof(char) * 33); //Delete User
					del_cnt++;
					break;
				}
			}
			if (j == current_user_cnt) { // username not found error
				return ONVIF_R_ERR_USER_NOT_FOUND;
			}
		}
	}

	if (del_cnt != tmp.user_cnt) {
		return ONVIF_R_ERR_USER_NOT_FOUND;
	}

	/* make new_man[8] */
	for (i = 0; i < 8; i++) {
		if (man[i].usrid[0] != '\0') {
			memcpy(&new_man[new_cnt], &man[i], sizeof(ONVIF_USR_MAN));
			new_cnt++;
		}
	}
	new_man[0].usrcnt = (unsigned char)new_cnt;

	/* save new_man[8]*/
	save_current_usrman(new_man);
	nf_onvif_sysdbchange_make_authfile(new_man);

	/* wait until nf_ftpsvr_user_reload() is called */
	for(i = 0; i < 5; i++) {
		if(getpwnam(tmp.name[0]) == NULL)
			break;
		sleep(1);
	}

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int get_enable_audio_cnt()
{
	int i=0, cnt=0;

	for(i=0; i<ONVIF_ASOURCE_CNT; i++){
		if(is_EnableAudio(i%ONVIF_ASOURCE_CNT)){
			cnt++;
		}
	}

	return cnt;
}
void GetCapability(arg_GetCapa *capa, int ch)
{
	char buff[COMMON_SIZE] = {0,};
	int ptz_status = 0;
	int ret = 0;
	int i=0;

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), "cam.ptz.P%d.rs485", ch);
	ptz_status = 1;

	capa->is_audio[ch] = nf_sysdb_get_bool("audio.enable");
	capa->audioSupport[ch]  = capa->is_audio;

	capa->alarmin[ch] = 1;
	capa->alarmCnt[ch] = 1;
		
	capa->relay[ch] = 1;
	capa->relayCnt[ch] = 1;

	capa->is_motion[ch] = 1;
	capa->motionSupport[ch] = 1;

	capa->brightnessSupport[ch] = 1;
	capa->brightnessMin[ch]     = 0;
	capa->brightnessMax[ch]     = 100;
	
	capa->colorSupport[ch]      = 1;
	capa->colorMin[ch]          = 0;
	capa->colorMax[ch]          = 100;

	capa->contrastSupport[ch]   = 1;
	capa->contrastMin[ch]       = 0;
	capa->contrastMax[ch]       = 100;
	
	capa->sharpnessSupport[ch]  = 1;
	capa->sharpnessMin[ch]      = 1; //0
	capa->sharpnessMax[ch]      = 15; //8

	capa->blcSupport[ch] = 0;
	capa->wdrSupport[ch] = 0;
	capa->dnnSupport[ch] = 0;
	capa->wbSupport[ch] = 0;

	capa->exposureSupport[ch] = 0;
	capa->gainSupport[ch] = 0;
	capa->gainMin[ch]     = 0;
	capa->gainMax[ch]     = 0;
	
	capa->irisSupport[ch] = 0;
	capa->irisMin[ch] = 0;
	capa->irisMax[ch] = 0;

	capa->focusSupport[ch] = 1;

	capa->focusRelativeMoveSupport[ch]   = 0;
	capa->focusRelativeRangeMin[ch]      = 0;
	capa->focusRelativeRangeMax[ch]      = 0;
	
	capa->focusContinuousMoveSupport[ch] = 1;
	capa->focusContinuousRangeMin[ch]    = -1;
	capa->focusContinuousRangeMax[ch]    = 1;	
	
	capa->focusAbsoluteMoveSupport[ch]   = 0;
	capa->focusAbsoluteRangeMin[ch]      = 0;
	capa->focusAbsoluteRangeMax[ch]      = 0;
	
	capa->focusAutoModeSupport[ch]  = 0;
	capa->focusNearLimitSupport[ch] = 0;
	capa->focusFarLimitSupport[ch]  = 0;
	
	capa->ptSupport[ch] = (1 & ptz_status);

	capa->ptContinuousMoveSupport[ch] = (1 & capa->ptSupport[ch]);
	capa->panContinuousRangeMin[ch]   = -1;
	capa->panContinuousRangeMax[ch]   = 1;
	capa->tiltContinuousRangeMin[ch]  = -1;
	capa->tiltContinuousRangeMax[ch]  = 1;

	capa->ptAbsoluteMoveSupport[ch] = 0;
	capa->panAbsoluteRangeMin[ch]   = 0;
	capa->panAbsoluteRangeMax[ch]   = 0;
	capa->tiltAbsoluteRangeMin[ch]  = 0;
	capa->tiltAbsoluteRangeMax[ch]  = 0;

	capa->ptAbsoluteSpeedDefault[ch] = 1;
	capa->ptAbsoluteSpeedSupport[ch] = 0;
	capa->ptAbsoluteSpeedMin[ch]     = 0;
	capa->ptAbsoluteSpeedMax[ch]     = 0;

	capa->ptRelativeMoveSupport[ch] = 0;
	capa->panRelativeRangeMin[ch]   = 0;
	capa->panRelativeRangeMax[ch]   = 0;
	capa->tiltRelativeRangeMin[ch]  = 0;
	capa->tiltRelativeRangeMax[ch]  = 0;

	capa->ptzContinuousTimeOutDefault[ch] = 0; //msec
	capa->ptzContinuousTimeOutSupport[ch] = 1;
	capa->ptzContinuousTimeOutMin[ch] = 0;
	capa->ptzContinuousTimeOutMax[ch] = 0;		// modified yys

	capa->homePositionSupport[ch] = 0;
	capa->presetSupport[ch]       = 1;
	capa->presetMaxCnt[ch]        = 255;
	
	capa->presetTourSupport[ch]   = 0;
	capa->presetTourMaxCnt[ch]    = 0;
	capa->presetTourSeqMaxCnt[ch] = 0;
	
	capa->zoomSupport[ch] = 1;
	
	capa->zoomAbsoluteMoveSupport[ch] = 0;
	capa->zoomAbsoluteRangeMin[ch]    = 0;
	capa->zoomAbsoluteRangeMax[ch]    = 1;
	capa->zoomRelativeMoveSupport[ch] = 0;
	capa->zoomRelativeRangeMin[ch]    = -1;
	capa->zoomRelativeRangeMax[ch]    = 1;

	capa->zoomContinuousMoveSupport[ch] = 1;
	capa->zoomContinuousRangeMin[ch]    = -1;
	capa->zoomContinuousRangeMax[ch]    = 1;

	capa->zoomAbsoluteSpeedSupport[ch] = 0;
	capa->zoomAbsoluteSpeedMin[ch]     = 0;
	capa->zoomAbsoluteSpeedMax[ch]     = 1;
}
int onvif_GetRelays(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Relays tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Relays);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Relays));

	// To DO
	char buff[COMMON_SIZE] = { 0, };
	int i;

	tmp.relay_cnt = ONVIF_RELAY_CNT;

	for (i = 0; i < tmp.relay_cnt; i++) {
		sprintf(buff, "onvif.relay%d.token", i);
		strncpy(tmp.relay[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.relay%d.mode", i);
		tmp.relay[i].mode = (int)nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.relay%d.duration", i);
		tmp.relay[i].duration = (int)nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.relay%d.idle_state", i);
		tmp.relay[i].idle_state = (int)nf_sysdb_get_uint(buff);
	}

	memcpy(packet.data, &tmp, sizeof(arg_Relays));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Relays)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetRelaySettings(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Relay tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Relay);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Relay));

	// To DO
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	int i;
	gchar map_buff[17] = "0000000000000000";

	int relay_cnt = ONVIF_RELAY_CNT;

	_TTY_LOG_ONVIF("int onvif_SetRelaySettings");
	_TTY_LOG_ONVIF("token[%s]", tmp.token);
	_TTY_LOG_ONVIF("mode[%d]", tmp.mode);
	_TTY_LOG_ONVIF("duration[%d]", tmp.duration);
	_TTY_LOG_ONVIF("idle_state[%d]", tmp.idle_state);

	for (i = 0; i < relay_cnt; i++) {
		sprintf(buff, "onvif.relay%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			if (tmp.mode == ONVIF_Monostable) {
				sprintf(buff, "act.arout.R%d.active", i);
				// nf_sysdb_set_bool(buff, RELAY_MODE_MONOSTABLE);  // auto
			} else {
				sprintf(buff, "act.arout.R%d.active", i);
				nf_sysdb_set_bool(buff, RELAY_MODE_BISTABLE); // manual (bistable)
			}

			if (tmp.duration == 0) {
				//nf_sysdb_set_uint("act.arout.R0.dwell_type", 1); //sync
				;
			} else {
				sprintf(buff, "act.arout.R%d.duration", i);
				nf_sysdb_set_uint(buff, (unsigned int)tmp.duration);
			}

			if (tmp.idle_state == ONVIF_closed) {
				sprintf(buff, "act.arout.R%d.op_type", i);
				nf_sysdb_set_bool(buff, ONVIF_closed); //NC
			} else {
				sprintf(buff, "act.arout.R%d.op_type", i);
				nf_sysdb_set_bool(buff, ONVIF_open); //NO
			}
			sprintf(buff, "onvif.relay%d.mode", i);
			nf_sysdb_set_uint(buff, (unsigned int)tmp.mode);
			sprintf(buff, "onvif.relay%d.duration", i);
			nf_sysdb_set_uint(buff, (unsigned int)tmp.duration);
			sprintf(buff, "onvif.relay%d.idle_state", i);
			nf_sysdb_set_uint(buff, (unsigned int)tmp.idle_state);

			tmp.result = 1;
		}
	}

	if (tmp.result == 1) {
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ACT, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ACT);
	} else {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Relay));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Relay)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

pthread_t set_relay_thread_id = 0;

static void usr_alarm_inactive() {

	unsigned duration = nf_sysdb_get_uint("onvif.relay0.duration");

	sleep(duration);
	nf_dev_relay_off(0);
// Relay OFF does not trigger a separate event,
// meaning no callback message is delivered.
// Implemented additional logic to handle this condition explicitly.
	struct e_relay_type e_relay;
	e_relay.ch = 0;
	e_relay.active = RELAY_STATE_INACTIVE;
 	append_onvif_event_msg(E_KEY_RELAY, &e_relay, E_Changed, 0);
}


static void usr_alarm_active() {
	unsigned duration = nf_sysdb_get_uint("onvif.relay0.duration");
	sleep(duration);
 	nf_dev_relay_ch_on(0);
	nf_sysdb_set_bool("act.arout.R0.active", RELAY_STATE_INACTIVE);
}

int onvif_SetRelayState(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Relay tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Relay);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Relay));

	// To DO
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	int i;

	int relay_cnt = ONVIF_RELAY_CNT;

	_TTY_LOG_ONVIF("int onvif_SetRelayState");
	_TTY_LOG_ONVIF("token[%s]", tmp.token);
	_TTY_LOG_ONVIF("mode[%d]", tmp.mode);
	_TTY_LOG_ONVIF("duration[%d]", tmp.duration);
	_TTY_LOG_ONVIF("idle_state[%d]", tmp.idle_state);

	//    gchar mask_buff[17] = "1111111111111111";
	//    nf_sysdb_set_str("act.arout.R0.usr_alarm", mask_buff);
	tmp.result = 0;
	struct e_relay_type e_relay;

	for (i = 0; i < relay_cnt; i++)
	{
		sprintf(buff, "onvif.relay%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token))
		{
			sprintf(buff, "onvif.relay%d.mode", i);
			if (nf_sysdb_get_uint(buff) == ONVIF_Monostable)
			{
				if (tmp.curr_state == RELAY_STATE_ACTIVE)
				{
					nf_dev_relay_ch_on(i);
#if 1 //[[ hun_0140723_BEGIN -- del 
					if (set_relay_thread_id != 0)
					{
						if (pthread_join(set_relay_thread_id, NULL) != 0)
							printf("set_relay_thread join fail\n");
						else
							printf("set_relay_thread join ok\n");
						set_relay_thread_id = 0;
					}
					if (pthread_create(&set_relay_thread_id, NULL, (void*) usr_alarm_inactive, NULL) != 0)
					{
						printf("set_relay_thread create error\n");
					}
#endif //]] hun_0140723_END -- del

				} else {
					// when relay maintain inactive state ODTT demand changed event,,
					e_relay.active = RELAY_STATE_INACTIVE;
					e_relay.ch = i;
					append_onvif_event_msg(E_KEY_RELAY, &e_relay, E_Changed, 0);
				}
			}
			else
			{
				if (tmp.curr_state == ONVIF_active)
				{
					nf_dev_relay_ch_on(i);
					e_relay.active = RELAY_STATE_ACTIVE;
					e_relay.ch = i;
					append_onvif_event_msg(E_KEY_RELAY, &e_relay, E_Changed, 0);
				}
				else
				{
					nf_dev_relay_off(i);
					e_relay.active = RELAY_STATE_INACTIVE;
					e_relay.ch = i;
					append_onvif_event_msg(E_KEY_RELAY, &e_relay, E_Changed, 0);
				}
			}
			tmp.result = 1;
		}
	}

	if (tmp.result != 1)
	{
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Relay));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Relay)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}



/****************************************************
 End of Device Service
 *****************************************************/

/****************************************************
 Media Service
 *****************************************************/
static int onvif_Profile_GetVideoSource(arg_ProfileDB *tmp)
{
	_getVideoSourceConfiguration(&tmp->vsource);
	return 1;
}

int onvif_CreateProfile(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfileDB tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfileDB);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfileDB));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char profile[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int profile_cnt;
	int i;
	//arg_AddConfig vsource, vencoder, asource, aencoder;
	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF_DEBUG("profile_cnt : %d , ONVIF_MAX_PROFILE_CNT %d\n",
			profile_cnt, ONVIF_MAX_PROFILE_CNT);
	if (profile_cnt + 1 > ONVIF_MAX_PROFILE_CNT) {
		return ONVIF_R_ERR_MAX_NVT_PROFILES;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(profile, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(profile, tmp.token)) {
			return ONVIF_R_ERR_PROFILE_EXIST;
		}
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	if (tmp.token[0] == '\0')
		strncpy(tmp.token, tmp.name, COMMON_SIZE - 1);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		_TTY_LOG_ONVIF_DEBUG("onvif.profile%d.token", i);
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(profile, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (profile[0] == '\0') {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			nf_sysdb_set_str(buff, tmp.token);

			sprintf(buff, "onvif.profile%d.name", i);
			nf_sysdb_set_str(buff, tmp.name);

			sprintf(buff, "onvif.profile%d.fixed", i);		// yys
			nf_sysdb_set_bool(buff, 0);
			// add default encoders & sources
			profile_cnt++;
			nf_sysdb_set_uint("onvif.common.profile_cnt", (unsigned int)profile_cnt);

			tmp.result = 1;

			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			goto success;
		}
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	}
	_TTY_LOG_ONVIF_DEBUG("i: %d (profile_cnt : %d)", i, profile_cnt);
	// if the all profiled is reserved.
	if (i == ONVIF_MAX_PROFILE_CNT) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR_MAX_NVT_PROFILES;
	}

	if (tmp.result != 1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR_PROFILE_EXIST;
	}

	success:

	memcpy(packet.data, &tmp, sizeof(arg_ProfileDB));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ProfileDB)) != ONVIF_ERR_RET_SUCCESS) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_CreateProfile2(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfileDB2 tmp;
	int config_index = 0;

    struct e_profile_change_type prof_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfileDB2);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfileDB2));

	// To DO
	char profile[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int profile_cnt;
	int i;
	//arg_AddConfig vsource, vencoder, asource, aencoder;
	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");
				
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF_DEBUG("profile_cnt : %d , ONVIF_MAX_PROFILE_CNT %d\n",
			profile_cnt, ONVIF_MAX_PROFILE_CNT);
    
	if (profile_cnt + 1 > ONVIF_MAX_PROFILE_CNT) {
		return ONVIF_R_ERR_MAX_NVT_PROFILES;
	}

    for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.name", i);
		strncpy(profile, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(profile, tmp.profile_name)) {
			goto exit;
		}
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
	{
		int fixed = 0;

		sprintf(buff, "onvif.profile%d.fixed", i);
		fixed = nf_sysdb_get_bool(buff);

		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(profile, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!fixed && profile[0] == '\0')
		{
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
            
			// memset(buff, 0x00, sizeof(buff));
			sprintf(buff, "onvif.profile%d.name", i);
			nf_sysdb_set_str(buff, tmp.profile_name);
			
			sprintf(buff, "onvif.profile%d.token", i);
			nf_sysdb_set_str(buff, tmp.profile_name);

			sprintf(buff, "onvif.profile%d.fixed", i);
			nf_sysdb_set_bool(buff, 0);

			if (tmp.configSize)
			{
				for (config_index = 0; config_index < tmp.configSize; config_index++)
				{
					printf("\e[31m type[%s] token[%s] \e[0m\n", tmp.config_type[config_index], tmp.config_token[config_index]);
					if (!strcmp(tmp.config_type[config_index], "VideoSource"))
					{
						sprintf(buff, "onvif.profile%d.vsource", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "vsconfig0");
						}
						else
						{
							if (!strcmp(tmp.config_token[config_index], "vsconfig0"))
							{
								nf_sysdb_set_str(buff, tmp.config_token[config_index]);
							}
							else
							{
								return ONVIF_R_ERR_NO_CONFIG;
							}
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "VideoEncoder"))
					{
						sprintf(buff, "onvif.profile%d.vencoder", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "ve0");
						}
						else
						{
							nf_sysdb_set_str(buff, tmp.config_token[config_index]);
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "AudioSource"))
					{
						sprintf(buff, "onvif.profile%d.asource", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "asconfig0");
						}
						else
						{
							if (!strcmp(tmp.config_token[config_index], "asconfig0"))
							{
								nf_sysdb_set_str(buff, tmp.config_token[config_index]);
							}
							else
							{
								return ONVIF_R_ERR_NO_CONFIG;
							}
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "AudioEncoder"))
					{
						sprintf(buff, "onvif.profile%d.aencoder", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "ae0");
						}
						else
						{
							nf_sysdb_set_str(buff, tmp.config_token[config_index]);
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "Metadata"))
					{
						sprintf(buff, "onvif.profile%d.metadata", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "m0");
						}
						else
						{
							nf_sysdb_set_str(buff, tmp.config_token[config_index]);
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "PTZ"))
					{
						sprintf(buff, "onvif.profile%d.ptz", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "ptz0");
						}
						else
						{
							nf_sysdb_set_str(buff, tmp.config_token[config_index]);
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "AudioOutput"))
					{
						sprintf(buff, "onvif.profile%d.aoutput", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "aoconfig0");
						}
						else
						{
							nf_sysdb_set_str(buff, tmp.config_token[config_index]);
						}
					}
					else if (!strcmp(tmp.config_type[config_index], "AudioDecoder"))
					{
						sprintf(buff, "onvif.profile%d.adecoder", i);
						if (tmp.config_token[0] == '\0')
						{
							nf_sysdb_set_str(buff, "ad0");
						}
						else
						{
							nf_sysdb_set_str(buff, tmp.config_token[config_index]);
						}
					}
					else
					{}
				}
			}

			profile_cnt++;
			nf_sysdb_set_uint("onvif.common.profile_cnt", (unsigned int)profile_cnt);
			
			tmp.result = 1;


			strcpy(prof_ch.token, tmp.profile_name);
			append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &prof_ch, E_Changed, 0);

			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

			break;
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_ProfileDB2));

exit:
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ProfileDB2)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

    strcpy(prof_ch.token, tmp.profile_name);
    append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &prof_ch, E_Changed, 0);

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetAnalyticsModules(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char buff[COMMON_SIZE] ={0,};
	char tmp_token[COMMON_SIZE] ={0,};
	arg_Motion tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Motion);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Motion));

	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.vanalytics0.token");
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	if(strcmp(tmp.VAnalytics_Token, tmp_token))
		return ONVIF_R_ERR_NO_CONFIG;

	// To DO
	onvif_makeVideoAnalyticsModule(&tmp, ONVIF_GET);

	memcpy(packet.data, &tmp, sizeof(arg_Motion));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Motion)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_makeVideoAnalyticsModule(arg_Motion *tmp, int get_modify)
{
	char buff[COMMON_SIZE] = { 0, };
	int act, save_flag = 0, i = 0;

	g_message("%s IN", __FUNCTION__);

	if(get_modify == ONVIF_GET)
	{
		for(i=0;i<4;i++)
		{
			snprintf(buff, sizeof(char)*COMMON_SIZE -1, "alarm.motion.M0.area%d.sense_d", i);
			tmp->sensitivity[i] = nf_sysdb_get_uint(buff);
		}
	}
	else
	{
		// Received Sensitivty Adjust
		for(i=0;i<4;i++)
		{
			if(tmp->is_change_sensitivity[i] == 1)
			{
				tmp->sensitivity[i] = (int)((tmp->sensitivity[i] / 100.0) * 30.0);
				if(tmp->sensitivity[i] == 0)
					tmp->sensitivity[i] = 1;
				snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "alarm.motion.M0.area%d.sense_d", i);
				nf_sysdb_set_uint(buff, tmp->sensitivity[i]);
				snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "alarm.motion.M0.area%d.sense_n", i);
				nf_sysdb_set_uint(buff, tmp->sensitivity[i]);
				save_flag = 1;
			}
		}

		if(save_flag)
		{
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ALARM);

		}
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_ModifyAnalyticsModules(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char buff[COMMON_SIZE] ={0,};
	char tmp_token[COMMON_SIZE] ={0,};
	arg_Motion tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Motion);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Motion));

	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.vanalytics0.token");
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	if(strcmp(tmp.VAnalytics_Token, tmp_token))
		return ONVIF_R_ERR_NO_CONFIG;

	onvif_makeVideoAnalyticsModule(&tmp, ONVIF_MODIFY);

	memcpy(packet.data, &tmp, sizeof(arg_Motion));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Motion)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetRules(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Active_Cell tmp;
	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Active_Cell);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Active_Cell));

	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.vanalytics0.token");
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	if(strcmp(tmp.VAnalytics_Token, tmp_token))
		return ONVIF_R_ERR_NO_CONFIG;

	onvif_makeVideoAnalyticsRule(&tmp, ONVIF_GET);
	
	memcpy(packet.data, &tmp, sizeof(arg_Active_Cell));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Active_Cell)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_ModifyRules(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char buff[COMMON_SIZE] = {0,};
	char tmp_token[COMMON_SIZE] = {0,};
	arg_Active_Cell tmp;
	int result=0;
	
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Active_Cell);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Active_Cell));

	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.vanalytics0.token");
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	if(strcmp(tmp.VAnalytics_Token, tmp_token))
		return ONVIF_R_ERR_NO_CONFIG;
	
	result = onvif_makeVideoAnalyticsRule(&tmp, ONVIF_MODIFY);
	if(result != ONVIF_ERR_RET_SUCCESS)
	{
		return result;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Active_Cell));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Active_Cell)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_makeVideoAnalyticsRule(arg_Active_Cell *tmp, int get_modify)
{
	char buff[COMMON_SIZE] = { 0, };
	char *cells = NULL;
	char db_str[ONVIF_AREA] = {0, };
	char db_set_str[ONVIF_AREA] = {0,};
	char output[ONVIF_AREA / 8]={0,};
	char packbit_encoding[ONVIF_AREA] = {0, };
	char decoded_packbit[ONVIF_AREA] = { 0, };
	int len=0, outlen=0, save_flag=0, motion_flag=0, i=0, j=0;
	int packbit_cnt = ONVIF_AREA / 8;

	g_message("%s IN", __FUNCTION__);

	if(get_modify == ONVIF_GET)
	{
		for(i=0;i<4;i++)
		{
			snprintf(buff, (COMMON_SIZE * 7 - 1) * sizeof(char), "alarm.motion.M0.area%d", i);
			strncpy(db_str, nf_sysdb_get_str_nocopy(buff), ONVIF_AREA);
			convert_hex(db_str);
			memset(output, 0x00, ONVIF_AREA / 8);
			before_packbits_bitmask(db_str, output);
			len = packbits(output, packbit_encoding, packbit_cnt);
			cells = g_base64_encode(packbit_encoding, len);
			strncpy(tmp->Acitve_Cells[i], cells, 100 - 1);
			g_free(cells);
		}  
	}
	else
	{
		// To Modify DB & Set Motion Area
		for(i=0;i<4;i++)
		{
			if(tmp->is_active_cells[i])
			{
				save_flag=1;
				snprintf(buff, (COMMON_SIZE * 7 - 1) * sizeof(char), "alarm.motion.M0.area%d", i);
				cells = g_base64_decode(tmp->Acitve_Cells[i], &outlen);
				unpackbits(decoded_packbit, cells, ONVIF_AREA, outlen);
				convert_string(decoded_packbit, db_set_str);
				nf_sysdb_set_str(buff, db_set_str);	
				g_free(cells);
			}
		}
		if(save_flag)
		{
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ALARM, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ALARM);

		}
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_makeVideoAnalyticsConfiguration(arg_VanalyticsConfig *conf, int get_modify)
{
	int result=0;

	g_message("%s IN", __FUNCTION__);
	
	strncpy(conf->name, "CellMotion", COMMON_SIZE - 1);
	conf->use_count = 1;
	if(get_modify == ONVIF_GET)
	{
		onvif_makeVideoAnalyticsModule(&conf->module, ONVIF_GET);
		onvif_makeVideoAnalyticsRule(&conf->rule, ONVIF_GET);
	}
	else
	{
		result = onvif_makeVideoAnalyticsRule(&conf->rule, ONVIF_MODIFY);
		if(result != ONVIF_ERR_RET_SUCCESS)
		{
			return result;	
		}
		onvif_makeVideoAnalyticsModule(&conf->module, ONVIF_MODIFY);
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	
	char buff[COMMON_SIZE] = {0,};
	char tmp_token[COMMON_SIZE] = {0,};
	int i=0;
	arg_VanalyticsConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VanalyticsConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VanalyticsConfig));

	for(i=0;i<ONVIF_CH;i++)
	{
		snprintf(buff, sizeof(char) * (COMMON_SIZE -1), "onvif.vanalytics%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp.token, tmp_token))
		{
			break;
		}
	}
	if(i==ONVIF_CH+1)
	{
		return ONVIF_R_ERR_NO_CONFIG;
	}
	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.vanalytics%d.token", i);
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	
	onvif_makeVideoAnalyticsConfiguration(&tmp, ONVIF_GET);
		
	memcpy(packet.data, &tmp, sizeof(arg_VanalyticsConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_VanalyticsConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoAnalyticsConfigurations(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	
	int i=0;
	arg_VanalyticsConfigs tmp;
	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VanalyticsConfigs);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VanalyticsConfigs));

	for(i=0;i<ONVIF_CH;i++)
	{
		snprintf(buff, sizeof(char) * (COMMON_SIZE -1), "onvif.vanalytics%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		strncpy(tmp.config[i].token, tmp_token, COMMON_SIZE -1 );
		onvif_makeVideoAnalyticsConfiguration(&tmp.config[i], ONVIF_GET);
	}

	memcpy(packet.data, &tmp, sizeof(arg_VanalyticsConfigs));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_VanalyticsConfigs)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAnalyticsConfigurations2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AnalyticsConfigs2 tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AnalyticsConfigs2);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AnalyticsConfigs2));

	// To DO
	
	int i = 0, j = 0;
	char tmp_token[COMMON_SIZE] = {0,};
	char confToken[COMMON_SIZE] = {0,}, profileToken[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};

	memset(buff, 0x00, sizeof(buff));

	if (tmp.conf_token != NULL && tmp.conf_token[0] != '\0')
	{
		if (!strcmp(tmp.conf_token, "va0"))
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.vanalytics%d.token", 0);
			strncpy(tmp.config[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			onvif_makeVideoAnalyticsConfiguration(&tmp.config[0], ONVIF_GET);
            }
		else
		{
			return ONVIF_R_ERR_NO_CONFIG;
        }
    }
	else if (tmp.profile_token != NULL && tmp.profile_token[0] != '\0')
	{
        memset(tmp_token, 0x00, sizeof(tmp_token));

		for (i = 0; i < MAX_PROFILE; i++)
		{
			sprintf(buff, "onvif.profile%d.token", i);
    		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp.profile_token, tmp_token) || !strcmp(tmp.profile_token, "testMedia2"))
			{
				sprintf(buff, "onvif.profile%d.vanalytics", i);
				strncpy(tmp.config[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				if (!strcmp(tmp.config[0].token, "va0"))
				{
					onvif_makeVideoAnalyticsConfiguration(&tmp.config[0], ONVIF_GET);
    			break;
    		}
				else
				{
					return ONVIF_R_ERR_SETTING_INVALID;
				}
    	}
    }
		if (i == MAX_PROFILE)
    	{
			return ONVIF_R_ERR_NO_PROFILE;
		}
    }
	else
	{
		for (i = 0; i < ONVIF_CH; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE -1), "onvif.vanalytics%d.token", i);
			strncpy(tmp.config[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
            onvif_makeVideoAnalyticsConfiguration(&tmp.config[i], ONVIF_GET);
        }
	}
/*
	for(i=0;i<ONVIF_CH;i++)
	{
		snprintf(buff, sizeof(char) * (COMMON_SIZE -1), "onvif.vanalytics%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		strncpy(tmp.config[i].token, tmp_token, COMMON_SIZE -1 );
		onvif_makeVideoAnalyticsConfiguration(&tmp.config[i], ONVIF_GET);
	}
*/
	memcpy(packet.data, &tmp, sizeof(arg_AnalyticsConfigs2));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AnalyticsConfigs2)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	
	char buff[COMMON_SIZE] = {0,};
	char tmp_token[COMMON_SIZE] = {0,};
	int result=0;
	arg_VanalyticsConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VanalyticsConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VanalyticsConfig));
	
	result = onvif_makeVideoAnalyticsConfiguration(&tmp, ONVIF_MODIFY);
	if(result != ONVIF_ERR_RET_SUCCESS)
	{
		return result;	
	}

	memcpy(packet.data, &tmp, sizeof(arg_VanalyticsConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_VanalyticsConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
	
}

int onvif_AddVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	arg_Tokens tmp;
	int i=0, flag=0;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));


	for(i=0;i<ONVIF_CH;i++)
	{
		snprintf(buff, sizeof(char)*(COMMON_SIZE-1), "onvif.profile%d.token", i);
		strncpy(tmp_token ,nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp.profile_token, tmp_token))
		{
			snprintf(buff, sizeof(char)*(COMMON_SIZE-1), "onvif.profile%d.vanalytics", i);
			nf_sysdb_set_str(buff, tmp.config_token);
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			flag = 1;
		}
	}
	if(flag == 0)
	{
		return ONVIF_R_ERR_PROFILE_EXIST;	
	}
	
	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_VAnalytics_Check_Token(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Tokens tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.vanalytics0.token");
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	if(strcmp(tmp.config_token, tmp_token))
		return ONVIF_R_ERR_NO_CONFIG;
	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetMetadataConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	char config_token[COMMON_SIZE] = {0,};
	arg_Tokens tmp;
	int i=0;
	int profile_cnt=0, result = -1;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));

	profile_cnt = ONVIF_MAX_PROFILE_CNT;

	for(i = 0; i < profile_cnt; i++)
	{
		result = 0;
		snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.metadata", i);
		strncpy(config_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp.config_token, config_token))
		{
			result = 1;
			break;
		}
	}
	if(result == 0)
	{
		return ONVIF_R_ERR_NO_CONFIG;	
	}
	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS){
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	arg_RemoveConfig tmp;
	int i=0;
	int profile_cnt=0;
	char conf_token[COMMON_SIZE] = {0,};
	char conf_type[COMMON_SIZE] = {0,};
	int is_valid = 0;
	int ret;
	int profile_index=0;
    int fixed;

    struct e_profile_change_type prof_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	
	for(profile_index = 0; profile_index < MAX_PROFILE; profile_index++)
	{
		snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", profile_index);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp.profile_token, tmp_token))
		{
			for ( i = 0; i < tmp.config_size; i++)
			{
				if (tmp.config_token[i] != NULL && tmp.config_token[i][0] != '\0')
				{
					strncpy(conf_token, tmp.config_token[i], COMMON_SIZE - 1);
					strncpy(conf_type, tmp.config_type[i], COMMON_SIZE - 1);
					is_valid = 1;
				}

				if ( is_valid )
				{
					Removeconfiguration(conf_token, conf_type, profile_index);

					memset(&prof_ch, 0x00, sizeof(prof_ch));
					if(conf_token[0] != '\0') {
				        strcpy(prof_ch.token, tmp.profile_token);
				    }
				    else {
				        strcpy(prof_ch.token, conf_token);
				    }
				    strcpy(prof_ch.type, conf_type);
				    append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &prof_ch, E_Changed, 0);
					
					is_valid = 0;
				}
			}

			break;
		}
	}

    if(profile_index == MAX_PROFILE)
	    return ONVIF_R_ERR_MAX_NVT_PROFILES;

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
    
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

    char tmp_token[COMMON_SIZE] = {0,};
    char buff[COMMON_SIZE] = {0,};
    arg_AddConfig2 tmp;
    int i=0;
    int profile_cnt=0;
    char profile_name[COMMON_SIZE] = {0,};
    char conf_token[COMMON_SIZE] = {0,};
    char conf_type[COMMON_SIZE] = {0,};
    int is_valid = 0;
    int ret;
    int profile_index=0;
    int fixed;

    struct e_profile_change_type prof_ch;

    g_message("%s IN", __FUNCTION__);

    memset(&header, 0, sizeof(header));
    memcpy(&header, buff_rcv, sizeof(header));

    packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig2);

    memcpy(&packet, buff_rcv, packet_len);
    memcpy(&tmp, packet.data, sizeof(arg_AddConfig2));

    profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");

    // if(profile_cnt >= ONVIF_MAX_PROFILE_CNT)
    //     return ONVIF_R_ERR_MAX_NVT_PROFILES;

    for(profile_index = 0; profile_index < ONVIF_MAX_PROFILE_CNT; profile_index++)
    { 
    	memset(buff, 0x00, sizeof(COMMON_SIZE));
		/*
        snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.fixed", profile_index);
        fixed = nf_sysdb_get_bool(buff);

        if(fixed) {
            continue;
        }
        */
        snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", profile_index);
        strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
        
        if(!strcmp(tmp.profile_token, tmp_token)) {
            tmp.result = 0;
            break;
        }
    }

	if(profile_index == ONVIF_MAX_PROFILE_CNT)
		return ONVIF_R_ERR_MAX_NVT_PROFILES;

	memset(buff, 0x00, sizeof(COMMON_SIZE));

    // if(tmp.profile_name != NULL && tmp.profile_name[0] != '\0') {
    //     strncpy(profile_name, tmp.profile_name, COMMON_SIZE - 1);
	// 	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.name", profile_index);
	// 	nf_sysdb_set_str(buff, profile_name);
    // }
	// else
	// {
	// 	snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.name", profile_index);
	// 	nf_sysdb_set_str(buff, profile_name);
	// }

    // AddConfiguration
    for (i = 0; i < tmp.config_size; i++)
    {
        if(tmp.config_token[i] != NULL && tmp.config_token[i][0] != '\0') {
            strncpy(conf_token, tmp.config_token[i], COMMON_SIZE - 1);
			strncpy(conf_type, tmp.config_type[i], COMMON_SIZE - 1);
			is_valid = 1;
        }

        if ( is_valid )
        {
            AddConfiguration(conf_token, conf_type, profile_index);
			
			memset(&prof_ch, 0x00, sizeof(prof_ch));
			if(conf_token[0] != '\0') {
		        strcpy(prof_ch.token, tmp.profile_token);
		    }
		    else {
		        strcpy(prof_ch.token, conf_token);
		    }
			strcpy(prof_ch.token, tmp.profile_token);
			append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &prof_ch, E_Changed, 0);
			is_valid = 0;
        }   
    }

	
	strcpy(prof_ch.token, tmp.profile_token);
	append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &prof_ch, E_Changed, 0);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

    memcpy(packet.data, &tmp, sizeof(arg_AddConfig2));
    if(nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AddConfig2)) != ONVIF_ERR_RET_SUCCESS)
    {
        return ONVIF_ERR_RET_INTERNAL;
    }
    
    return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetCompatibleConfigurations(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0, ch = 0;
	arg_PTZConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZConfig));
	
	get_ptz_config(&tmp);

	memcpy(packet.data, &tmp, sizeof(arg_PTZConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

void before_packbits_bitmask(unsigned char *arr, char *output)
{
	int i=0;

	int index=0, bit =0;

	for(i=0;i<ONVIF_AREA;i++)
	{		
		if(arr[i] == 1)
		{
			index = i / 8;
        	bit   = i % 8;
			output[index] |= set_bit[bit];
		}
	}
}

void convert_hex(unsigned char *arr)
{
	int i=0;

	int index=0, bit =0;

	for(i=0;i<ONVIF_AREA;i++)
	{
		if(arr[i] != '0')
			arr[i] = 0x01;
		else
			arr[i] = 0x00;
	}
}

void convert_string(unsigned char *recv_data, char *db_set_data)
{
	int i=0;
	int data;
	int left=0;
	int index=0;
	
	for(i=0;i<ONVIF_AREA;i++)
	{
		left = i % 8;
		index = i / 8;
		data = (recv_data[index] << left) & 0x80;
		if(data == 0)
		{
			db_set_data[i] = '0';
		}
		else
		{
			db_set_data[i] = '1';
		}
	}
}

unsigned int packbits(unsigned char *src, unsigned char *dst, unsigned int n)
{  
    unsigned char *p, *q, *run, *dataend;  
    int count, maxrun;  
  
    dataend = src + n;  
    for( run = src, q = dst; n > 0; run = p, n -= count ){  
        // A run cannot be longer than 128 bytes.  
        maxrun = n < 128 ? n : 128;  
        if(run <= (dataend-3) && run[1] == run[0] && run[2] == run[0]){  
            // 'run' points to at least three duplicated values.  
            // Step forward until run length limit, end of input,  
            // or a non matching byte:  
            for( p = run+3; p < (run+maxrun) && *p == run[0]; )  
                ++p;  
            count = p - run;  
  
            // replace this run in output with two bytes:  
            *q++ = 1+256-count; /* flag byte, which encodes count (129..254) */  
  
            *q++ = run[0];      /* byte value that is duplicated */  
  
        }else{  
            // If the input doesn't begin with at least 3 duplicated values,  
            // then copy the input block, up to the run length limit,  
            // end of input, or until we see three duplicated values:  
            for( p = run; p < (run+maxrun); )  
                if(p <= (dataend-3) && p[1] == p[0] && p[2] == p[0])  
                    break; // 3 bytes repeated end verbatim run  
                else
                    ++p;  
            count = p - run;  
            *q++ = count-1;        /* flag byte, which encodes count (0..127) */  
            memcpy(q, run, count); /* followed by the bytes in the run */  
            q += count;  
        }  
    }  
    return q - dst;  
}

unsigned int unpackbits(unsigned char *outp, unsigned char *inp, unsigned int outlen, unsigned int inlen)
{  
    unsigned int i, len;  
    int val;  
  
    /* i counts output bytes; outlen = expected output size */  
    for(i = 0; inlen > 1 && i < outlen;){
        /* get flag byte */  
        len = *inp++;  
        --inlen;  
  
        if(len == 128) /* ignore this flag value */  
            ; // warn_msg("RLE flag byte=128 ignored");  
        else{  
            if(len > 128){  
                len = 1+256-len;  
  
                /* get value to repeat */  
                val = *inp++;  
                --inlen;  
  
                if((i+len) <= outlen)  
                    memset(outp, val, len);  
                else{  
                    memset(outp, val, outlen-i); // fill enough to complete row  
                    printf("unpacked RLE data would overflow row (run)\n");  
                    len = 0; // effectively ignore this run, probably corrupt flag byte  
                }  
            }else{  
                ++len;  
                if((i+len) <= outlen){  
                    if(len > inlen)  
                        break; // abort - ran out of input data  
                    /* copy verbatim run */  
                    memcpy(outp, inp, len);  
                    inp += len;  
                    inlen -= len;  
                }else{
                    memcpy(outp, inp, outlen-i); // copy enough to complete row  
                    printf("unpacked RLE data would overflow row (copy)\n");  
                    len = 0; // effectively ignore  
                }  
            }  
            outp += len;  
            i += len;  
        }  
    } 
    if(i < outlen)  
        printf("not enough RLE data for row\n");  
    return i;  
}

int onvif_RemoveVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	arg_Tokens tmp;
	int i=0;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));
	
	for(i=0;i<ONVIF_CH;i++)
	{
		snprintf(buff, sizeof(char) * (COMMON_SIZE -1), "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp.profile_token, tmp_token))
		{
			break;
		}
	}
	if(i==ONVIF_CH+1)
	{
		return ONVIF_R_ERR_NO_CONFIG;
	}
	snprintf(buff, sizeof(char) * (COMMON_SIZE -1), "onvif.profile%d.vanalytics", i);
	nf_sysdb_set_str(buff, "\0");
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddAudioSource(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, config_index, profile_index;

	tmp.result = 1;

	//if( !(isUseAudio() && is_EnableAudio()) ) {
	//	tmp.result = -10;
	//	return;
	//}
	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {
		sprintf(buff, "onvif.asource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.config_token, tmp_token)) {
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
				continue;
			}		
			config_index = i;
			break;
		}
	}
	if (i == ONVIF_VSOURCE_CNT) {
		tmp.result = -1;
		return ONVIF_R_ERR;
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.asource", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZNode);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZNode));

	// To DO
#if 1
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, vsource_index, profile_index;

	tmp.result = 1;
	
	if (tmp.token != NULL && tmp.token[0] != '\0') {
		for (i = 0; i < ONVIF_PTZ_CNT; i++) {
			sprintf(buff, "onvif.ptz%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		
			if (!strcmp(tmp.token, tmp_token)) {
				break;
			}
		}
		if (i == ONVIF_PTZ_CNT) {
			tmp.result = -1;
			return ONVIF_R_ERR;
		}
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.node_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.ptz", profile_index);
		nf_sysdb_set_str(buff, tmp.token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}
#else
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");

	int token_num = findfieldStringCount("onvif.profile%d.token", tmp.node_token, ONVIF_MAX_PROFILE_CNT);
	if (token_num != -1) {
		sprintf(buff, "onvif.profile%d.ptz", token_num);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		profile_cnt = ONVIF_PTZ_CNT;
		token_num = findfieldStringCount("onvif.ptz%d.token", tmp_token, profile_cnt);
		sprintf(buff, "onvif.ptz%d.node_token", token_num);
		nf_sysdb_set_str(buff, tmp.node_token);
		sprintf(buff, "onvif.ptz%d.token", token_num);
		nf_sysdb_set_str(buff, tmp.token);
		sprintf(buff, "onvif.ptz%d.name", token_num);
		nf_sysdb_set_str(buff, tmp.token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_helper(NF_SYSDB_CATE_ONVIF);
	} else {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}	
#endif
	memcpy(packet.data, &tmp, sizeof(arg_PTZNode));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_PTZNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}


int onvif_AddAudioOutputConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, audio_output_index, profile_index;

	tmp.result = 1;

	for (i = 0; i < ONVIF_AUDIOOUTPUT_CNT; i++)
	{
		sprintf(buff, "onvif.aoutput%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.config_token, tmp_token))
		{
			audio_output_index = i;
			break;
		}
	}
	
	if (i == ONVIF_AUDIOOUTPUT_CNT)
	{
		return ONVIF_R_ERR;
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
	{
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token))
		{
			profile_index = i;
			break;
		}
	}
	
	if (i == ONVIF_MAX_PROFILE_CNT)
	{
		return ONVIF_R_ERR;
	}

	if (profile_index >= ONVIF_FIXED_PROFILE_CNT+2)
	{
		if (audio_output_index >= 2)
			tmp.result = -2;
	}
	else if (profile_index < ONVIF_FIXED_PROFILE_CNT)
	{
		if (audio_output_index != profile_index)
			tmp.result = -2;
	}

	if (tmp.result > 0)
	{
		sprintf(buff, "onvif.profile%d.aoutput", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	if (tmp.result != 1)
	{
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_AddAudioDecoderConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, profile_index = 0;

	tmp.result = 1;

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
	{
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token))
		{
			profile_index = i;
			break;
		}
	}

	sprintf(buff, "onvif.profile%d.adecoder", profile_index);
	nf_sysdb_set_str(buff, tmp.config_token);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_CreateRecording(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_CreateTrack(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_DeleteRecording(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_DeleteTrack(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}


void recordingJobSourceState(char* jobToken,arg_RecordingJobConfiguration* conf)
{
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	/*
	 *
	    '<item key="onvif.source0.job_token"       type="STRING"      min="0" max="64" val="jt0" />'
		'<item key="onvif.source0.token0"       type="STRING"      min="0" max="64" val="" />'
		'<item key="onvif.source0.dest0"       type="STRING"      min="0" max="64" val="" />'
		'<item key="onvif.source0.token1"       type="STRING"      min="0" max="64" val="" />'
		'<item key="onvif.source0.dest1"       type="STRING"      min="0" max="64" val="" />'
		'<item key="onvif.source0.token2"       type="STRING"      min="0" max="64" val="" />'
		'<item key="onvif.source0.dest2"       type="STRING"      min="0" max="64" val="" />'
		'<item key="onvif.source0.autoCreateReceiver"       type="UINT"      min="0" max="64" val="" />'

	 */
     conf->sourceSize = ONVIF_MAX_RECORDING_CNT;
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.source%d.job_token", i);
		if(!strcmp(jobToken,nf_sysdb_get_str_nocopy(buff)))
		{
			for (j = 0; j < ONVIF_JOBSOURCE_CNT; j++) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				sprintf(buff, "onvif.source%d.token%d", i, j);
				strncpy(conf->stateSource[j].SourceToken,nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				strncpy(conf->stateSource[j].SourceType,"http://www.onvif.org/ver10/schema/Receiver", COMMON_SIZE - 1);
				strncpy(conf->stateSource[j].state,conf->state, COMMON_SIZE - 1);


				strncpy(conf->stateSource[j].track.SourceTag,"", COMMON_SIZE - 1);
				sprintf(buff, "onvif.source%d.dest%d", i, j);
				strncpy(conf->stateSource[j].track.Destination,nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				strncpy(conf->stateSource[j].track.state,conf->state, COMMON_SIZE - 1);
				strncpy(conf->stateSource[j].track.error,"", COMMON_SIZE - 1);
			}
		}
	}

}





int onvif_GetTrackConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetRecordingJobConfiguration(NfOnvif *self, int fd, char *buff_rcv) {

	int i=0,j=0;
        char tmp_token[COMMON_SIZE] = { 0, };
        char buff[COMMON_SIZE] = { 0, };
int mode=0;
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_JobItem tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_JobItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_JobItem));

	
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recordingjob%d.token", i);
		if(!strcmp(tmp.JobToken,nf_sysdb_get_str_nocopy(buff)))
		{	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);	
			sprintf(buff, "onvif.recordingjob%d.recording_token", i);
			nf_sysdb_set_str(buff,tmp.conf.RecordingToken);			

			sprintf(buff, "onvif.recordingjob%d.mode", i);
			if(!strcmp(tmp.conf.RecordingJobMode,"Idle"))
			{
						nf_sysdb_set_uint(buff,0);
			}else 
			{
						nf_sysdb_set_uint(buff,1);
			}



			sprintf(buff, "onvif.recordingjob%d.priority", i);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (tmp.conf.Priority : %d)",__FUNCTION__, tmp.conf.Priority);	
			nf_sysdb_set_uint(buff,(unsigned int)tmp.conf.Priority);
			sprintf(buff, "onvif.recordingjob%d.token", i);

			for(j=0;j<tmp.conf.sourceSize;j++)
			{
				_TTY_LOG_ONVIF_DEBUG("tmp.conf.source[i].SourceToken: %s (tmp.conf.Priority : %d)",tmp.conf.source[i].SourceToken, tmp.conf.Priority);	
				sprintf(buff, "onvif.source%d.token%d", i,j);
				nf_sysdb_set_str(buff,tmp.conf.source[i].SourceToken);				
     			sprintf(buff, "onvif.source%d.dest%d", i,j);
//				nf_sysdb_set_str(buff,tmp.conf.source[i].track.Destination);										
			
			}

			break;
						
			
		}
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	getRecordingJobConfiguration(&tmp);

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_JobItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_JobItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}



int onvif_SetTrackConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_GetTracksConfiguration tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetTracksConfiguration);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetTracksConfiguration));



	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetTracksConfiguration));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetTracksConfiguration))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}


int onvif_FindEvent(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_FindMetadata(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_FindPTZPosition(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}


/* WARNING : "excute_later_secs" is not thread safe */
static search_filter onvif_search_filter;

static int _check_is_start_time(char ch, recording_info *rec_info, int cnt)
{
	int i=0;
	
	for(i=0; i<cnt; i++){
		if( ch == rec_info[i].ch && rec_info[i].start_time != 0){
			return 1;
		}
	}

	return 0;
}

static int _check_is_end_time(char ch, recording_info *rec_info, int cnt)
{
	int i=0;
	
	for(i=0; i<cnt; i++){
		if( ch == rec_info[i].ch && rec_info[i].end_time != 0){
			return 1;
		}
	}
	return 0;
}

int put_rec_info_endtime(char ch,recording_info *rec_info,guint64 timestamp, int cnt)
{
	int i=0;

	for(i=0; i<cnt; i++){
		// FIXME.It may occur an error when the start & end time is same.
		if(rec_info[i].ch == ch ){
			if(rec_info[i].start_time <= timestamp){
				rec_info[i].end_time= timestamp;
			}
			return 1;
		}
	}
	return 0;
}

static void get_recording_info(unsigned char token, recording_info *rec_info) 
{
	int ret = 0, ch=0, cnt=0, i=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(ONVIF_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[ONVIF_MAX_LOG_CNT];	
	ONVIF_LIVE_STATUS	status;
	NF_DISK_REC_TIME        nrec_time;	

	memset( filter.param.param1_mask, 0xff, sizeof(filter.param.param1_mask) );	
	memset(rec_info, 0x00, sizeof(recording_info));

	/* filter init */
	filter.type = ONVIF_SEARCH_RECORDING;
	filter.timeout = 0;
	ret = nf_disk_rec_time(1, &nrec_time, NULL);
	if( !ret) return;

	filter.param.mode = NF_LOG_PARAM_MODE_TIME;
	filter.param.time_begin.tv_sec = (glong)nrec_time.min_time;
	filter.param.time_begin.tv_usec = 0;
	filter.param.time_end.tv_sec = (glong)nrec_time.max_time;
	filter.param.time_end.tv_usec = 0;
	filter.param.type_mask = 0;
	filter.param.request_count = ONVIF_MAX_LOG_CNT;
	filter.param.direction = 0;
	//	param.channelMask = get_log.channel_mask;

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

	for(i=0; i<(int)log_header->result_count; i++){
		if( token == elem[i].param1 ){
			rec_info->ch = (char)token;
			rec_info->start_time = elem[i].timestamp;
			cnt++;
			break;
		}
	}
	
	while( log_header->result_count == ONVIF_MAX_LOG_CNT){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 0; // Foword
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STARTED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(i=0; i<ONVIF_MAX_LOG_CNT; i++){
			if( token == elem[i].param1 && _check_is_start_time((char)token, rec_info, 1 ) == 0){
				rec_info->ch = (char)token;
				rec_info->start_time = elem[i].timestamp;
				cnt++;
				break;
			}
		}
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
	for(i=0; i<(int)log_header->result_count; i++){
		if( token == elem[i].param1 ){
			/* Find Strat time and put End time */
			ret = put_rec_info_endtime((char)token, rec_info, elem[i].timestamp, 1);
			if( !ret ){
				/* There is no Strat time. Add rec_info */
				rec_info->ch = (char)token;
				rec_info->start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
				rec_info->end_time = elem[i].timestamp;
				cnt++;	
			}
			break;
		}
	}

	while( log_header->result_count == ONVIF_MAX_LOG_CNT ){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 1; // Backward
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STOPPED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(i=0; i<ONVIF_MAX_LOG_CNT; i++){
			if( token == elem[i].param1 && _check_is_end_time((char)token, rec_info, 1 ) == 0){
				/* Find Strat time and put End time */
				ret = put_rec_info_endtime((char)token, rec_info, elem[i].timestamp, 1);
				if( !ret ){
					/* There is no Strat time. Add rec_info */
					rec_info->ch = (char)token;
					rec_info->start_time = GTIMEVAL_TO_GUINT64(filter.param.time_begin);
					rec_info->end_time = elem[i].timestamp;
					cnt++;
				}
				break;
			}
		}
	}

	/* Only Start Time, Set End Time to search end time */
	//status = (ONVIF_LIVE_STATUS)nf_webbase_live_status_cstack_get_interval_read(3, 0);
	
	if( rec_info->ch == token && status.act_recording[token] != ' ' ){
		rec_info->end_time = GTIMEVAL_TO_GUINT64(filter.param.time_end);
	}

}

static void _start_event_search(void *arg) 
{
	int ret = 0, ch=0, cnt=0, i=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(ONVIF_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[ONVIF_MAX_LOG_CNT];	
	ONVIF_LIVE_STATUS	status;
	
	recording_info rec_info[MAX_MSG_BUF];

	memset(&filter, 0x00, sizeof(search_filter));
	memcpy(&filter, (search_filter *) arg, sizeof(search_filter));
	memset(rec_info, 0x00, sizeof(recording_info)*ONVIF_MAX_RECORDING_CNT);

	GTimeVal tval;
	int token_idx = STR2INT_SEARCH_TOKEN(filter.search_token);

	/* find event informations of each channels */
	filter.param.mode = NF_LOG_PARAM_MODE_TIME; 
	filter.param.direction = 0; //forward
	if(filter.start_point > 0 && filter.end_point > 0)
	{	
		/* EndPoint is The point of time where the search will stop. 
		 * This can be a time before the StartPoint, 
		 * in which case the search is performed backwards in time.
		 */
		
		if(filter.start_point > filter.end_point)
			filter.param.direction = 1; //backward
	}
	
	filter.param.type_mask = ONVIF_LC_MOTION;
	if(filter.start_point > 0)	filter.param.time_begin.tv_sec = filter.start_point;
	if(filter.end_point > 0)	filter.param.time_end.tv_sec = filter.end_point;
	if(filter.max_matches > 0)	filter.param.request_count = filter.max_matches;
	
	ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);
	if(ret == 0)
	{
		g_message("%s Result Count[%d] ", __FUNCTION__,ret);
	}
	
	for(ch=0; ch<ONVIF_CH; ch++)
	{
		for(i=0; i<(int)log_header->result_count; i++)
		{
			if( ch == (int)elem[i].param1 )
			{				
				rec_info[cnt].ch = (char)ch;
				rec_info[cnt].start_time = elem[i].timestamp;
				cnt++;

				if(cnt == MAX_MSG_BUF)
					break;
			}
		}
		
		if(cnt == MAX_MSG_BUF)
			break;
	}

	for(i = 0; i < cnt; i++)
	{
		GUINT64_TO_GTIMEVAL( rec_info[i].start_time, tval);
		g_printf("[%d] ch[%d] timestamp[%d]\n", i, rec_info[i].ch, tval.tv_sec);
	}

	/* todo - SearchFilter
	 *   Refer to "RecordingSearch Service Spec - 5.19 Recording Event Descriptions"
	 *   SearchFilter contains the topic and message filter needed to define what events to search for.
	 */

	/* todo - IncludeStartState
	 *   By setting the IncludeStartState to true, the client indicates 
	 *   that virtual events at the time of StartPoint should be returned to 
	 *   represent the state in the recording. 
	 * 
	 *   In case of a backward search, virtual events at the time of EndPoint and StartPoint should be 
	 *   returned. Support for virtual events is mandatory for recording events. 
	 *   Support for additional virtual events is signaled via the GeneralStartEvents capability.
	 */

	/* Put searched recording info */
	unsigned	int srt = 0;
	srt = (unsigned int)atoi(filter.search_token+3);
	g_printf("PUSH SHM token[%s] [%d] cnt[%d] max[%d]\n", filter.search_token, srt, cnt, filter.max_matches);

	/* limit MaxMatches */
	g_printf("filter.max_matches[%d] cnt[%d]\n", filter.max_matches, cnt);
	if( ((int)filter.max_matches < cnt) && ((int)filter.max_matches > 0)){
		cnt = (int)filter.max_matches;
	}

	g_printf("filter.max_matches[%d] cnt[%d]\n", filter.max_matches, cnt);
	
	for(i=0; i<cnt; i++){
		/* The last msg must have Search Complete State!! */
		push_rec_info_shm(srt, &rec_info[i]);

#if 0	/* for EndSearch test */	
		if(get_end_search_token(token_idx))
		{
			set_end_search_token(token_idx, 0);
			break;
		}
		usleep(200*1000);
#endif
	}
	
	set_search_rec_state_shm(srt, SEARCH_STATE_COMPLETED);

#if 0
	while( log_header->result_count == ONVIF_MAX_LOG_CNT){
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 0; // foward
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = ONVIF_LC_MOTION | LC_ALARM;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<ONVIF_MAX_LOG_CNT; i++){
				if( ch == elem[i].param1 && _check_is_start_time((char)ch, rec_info, cnt ) == 0){
					rec_info[cnt].ch = (char)ch;
					rec_info[cnt].start_time = elem[i].timestamp;
					cnt++;
					break;
				}
			}
			if( cnt == ONVIF_CH )	break;			
		}
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
		g_printf("complete... ch[%u] start[%lld] end[%lld]\n", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}

	/* Put searched recording info */
	unsigned	int srt = 0;
	srt = (unsigned int)atoi(filter.search_token+3);
	g_printf("PUSH SHM token[%s] [%d] cnt[%d] max[%d]\n", filter.search_token, srt, cnt, filter.max_matches);

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
}

static void _start_recording_search(void *arg) 
{
	int ret = 0, ch=0, cnt=0, i=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(ONVIF_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[ONVIF_MAX_LOG_CNT];	
	ONVIF_LIVE_STATUS	status;
	
	recording_info rec_info[ONVIF_MAX_RECORDING_CNT];

	memset(&filter, 0x00, sizeof(search_filter));
	memcpy(&filter, (search_filter *) arg, sizeof(search_filter));
	memset(rec_info, 0x00, sizeof(recording_info)*ONVIF_MAX_RECORDING_CNT);

	int token_idx = STR2INT_SEARCH_TOKEN(filter.search_token);

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
	
	while( log_header->result_count == ONVIF_MAX_LOG_CNT){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 0; // Foword
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STARTED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<ONVIF_MAX_LOG_CNT; i++){
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
	
	while( log_header->result_count == ONVIF_MAX_LOG_CNT ){
		// Continue Searching start point
		filter.param.mode = NF_LOG_PARAM_MODE_LOGID;
		filter.param.direction = 1; // Backward
		filter.param.log_id = elem[log_header->result_count-1].log_id;
		filter.param.type_mask = LT_MASK_RECORD_STOPPED;

		ret = nf_eventlog_get( &filter.param, log_header, elem, NULL);

		for(ch=0; ch<ONVIF_CH; ch++){
			for(i=0; i<ONVIF_MAX_LOG_CNT; i++){
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
	//status = (ONVIF_LIVE_STATUS)nf_webbase_live_status_cstack_get_interval_read(3, 0);
	
	for(i=0; i<cnt; i++){
		for(ch=0; ch<ONVIF_CH; ch++){
			if( rec_info[i].ch == ch && status.act_recording[ch] != ' ' ){
				rec_info[i].end_time = GTIMEVAL_TO_GUINT64(filter.param.time_end);
			}
		}
	}
	
	for(i=0; i<ONVIF_CH; i++){
		g_printf("complete... ch[%u] start[%lld] end[%lld]\n", rec_info[i].ch, rec_info[i].start_time, rec_info[i].end_time);
	}

	/* Put searched recording info */
	unsigned	int srt = 0;
	srt = (unsigned int)atoi(filter.search_token+3);
	g_printf("PUSH SHM token[%s] [%d] cnt[%d] max[%d]\n", filter.search_token, srt, cnt, filter.max_matches);

	/* limit MaxMatches */
	g_printf("filter.max_matches[%d] cnt[%d]\n", filter.max_matches, cnt);
	if( ((int)filter.max_matches < cnt) && ((int)filter.max_matches > 0)){
		cnt = (int)filter.max_matches;
	}
	g_printf("filter.max_matches[%d] cnt[%d]\n", filter.max_matches, cnt);
	
	for(i=0; i<cnt; i++){
		/* The last msg must have Search Complete State!! */
		push_rec_info_shm(srt, &rec_info[i]);
	}
	
	set_search_rec_state_shm(srt, SEARCH_STATE_COMPLETED);				
}

static void _recording_search_filter_init(arg_FindRecordings *tmp, search_filter *filter) 
{
	NF_DISK_REC_TIME        nrec_time;	
	int ret = 0;

	filter->type = ONVIF_SEARCH_RECORDING;
	filter->timeout = 0;
	strcpy(filter->search_token, tmp->SearchToken);
	
	ret = nf_disk_rec_time(1, &nrec_time, NULL);
	if( !ret) return;

	filter->param.mode = NF_LOG_PARAM_MODE_TIME;
	if(filter->param.mode == NF_LOG_PARAM_MODE_TIME) 
	{
		//param.time_search.tv_sec = get_log.start_time;
		//param.time_search.tv_usec = get_log.start_time_sub*5000;
		filter->param.time_begin.tv_sec = (glong)nrec_time.min_time;
		filter->param.time_begin.tv_usec = 0;
		filter->param.time_end.tv_sec = (glong)nrec_time.max_time;
		filter->param.time_end.tv_usec = 0;
	}

	filter->param.type_mask = 0;
	filter->param.request_count = ONVIF_MAX_LOG_CNT;
	filter->param.direction = 0;
	//	param.channelMask = get_log.channel_mask;
	memset( filter->param.param1_mask, 0xff, sizeof(filter->param.param1_mask) );	

	filter->max_matches = (unsigned int)tmp->MaxMatches;
}

static int 		end_search_token[MAX_REC_SEARCH_SESSION] = {0};
static int 		search_token[MAX_REC_SEARCH_SESSION] = {0};
//static GMutex *	srt_mutex = NULL;
//static GMutex *	end_srt_mutex = NULL;

extern pthread_mutex_t		srt_mutex;
extern pthread_mutex_t		ri_mutex;
extern pthread_mutex_t		end_srt_mutex;
#if 0
int get_end_search_token(int token_idx)
{
	/* '1' means that EndSearch occurs for the SearchToken */
	int value = 0;
	
	if(end_srt_mutex == NULL)
		end_srt_mutex = g_mutex_new();

	g_mutex_lock(end_srt_mutex);
	value = end_search_token[token_idx];
	g_mutex_unlock(end_srt_mutex);

	return value;
}
#endif

int onvif_GetMediaAttributes(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetMetadataSearchResults(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}


int onvif_GetRecordingSummary(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RecordingSummary tmp;
	g_message("%s IN", __FUNCTION__);
	int ret = 0;

	NF_DISK_REC_TIME        nrec_time;	

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RecordingSummary);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RecordingSummary));

	ret = nf_disk_rec_time(1, &nrec_time, NULL);
	if( !ret)	
		return ONVIF_ERR_RET_INTERNAL;

	tmp.DataFrom = (int)nrec_time.min_time;
	tmp.DataUntil = (int)nrec_time.max_time;
	//FIXME. 
	tmp.NumberRecordings	= ONVIF_CH;

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_RecordingSummary));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RecordingSummary)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetSearchState(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetRecordingServiceCapabilities(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetReplayConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetReplayUri(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_StreamSetup tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_StreamSetup);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_StreamSetup));

	int i, profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.RecordingToken, tmp_token)) {
			break;
		}
	}
	if (i == ONVIF_MAX_RECORDING_CNT) {
		_TTY_LOG_ONVIF_DEBUG("i :  %d (profile_cnt: %d)", i, profile_cnt);
		//tmp.result = 0;
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}
	get_replayuri(&tmp, NULL);

	if(tmp.TransportProtocol == 3 /*tt__TransportProtocol__HTTP*/)
		g_printf(">>>>>>>>>>>>>>> ReplayUri[rtsp://%s]\n", tmp.uri_http);
	else
		g_printf(">>>>>>>>>>>>>>> ReplayUri[rtsp://%s]\n", tmp.uri);

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_StreamSetup));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_StreamSetup)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_SetReplayConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetRecordingJobStateChangeEvent(NfOnvif *self, int fd, char *buff_rcv) {

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	char buff[COMMON_SIZE] = { 0, };

	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_GetRecordingJobStateChangeEvent tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingJobStateChangeEvent);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingJobStateChangeEvent));

	//	tmp.JobTokenRef
		/*
		'<item key="onvif.recordingjob0.token"		 type="STRING"		min="0" max="64" val="jt0" />'
		'<item key="onvif.recordingjob0.recording_token"	   type="STRING"	  min="0" max="64" val="rs0" />' 
		'<item key="onvif.recordingjob0.mode"		type="UINT" 	 min="0" max="4096" val="" />' 
		'<item key="onvif.recordingjob0.priority"		type="UINT" 	 min="0" max="4096" val="" />' 
		'<item key="onvif.recordingjob0.state"		 type="STRING"		min="0" max="64" val="" />' 
		
		*/
		
	strncpy(tmp.Topic, "tns1:RecordingConfig/JobState\0",COMMON_SIZE -1);
	sprintf(buff, "onvif.recordingjob%d.token", tmp.JobTokenRef);
	strncpy(tmp.JobToken, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
	sprintf(buff, "onvif.recordingjob%d.state", tmp.JobTokenRef);
	strncpy(tmp.state, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
	
	recordingJobSourceState(tmp.JobToken,&tmp.conf);

	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingJobStateChangeEvent));

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingJobStateChangeEvent));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingJobStateChangeEvent))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
#if 0
int onvif_GetRecordings(NfOnvif *self, int fd, char *buff_rcv) {
int i=0,row=0,col=0;
	ONVIF_PACKET packet;
    ONVIF_HEADER header;
	guint packet_len;
	arg_GetRecordingsResponseItem tmp;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	_TTY_LOG_ONVIF("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);
	_TTY_LOG_ONVIF("packet_len: %d (Line : %d)",packet_len, __LINE__);
	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));


	// 1. ???ڵ? ???ٰ˻?

	// 2. track�� Ʈ?? ??ū�� 3?????? ?Ѵ?.
	//	tv0, av0, mv0
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) { 
		// 1. onvif.recording0.token
		// 3???? ?ϳ??? ???ڵ???.

		/*
		
		'<item key="onvif.recording0.token" 			  type="STRING" 	 min="0" max="64" val="rs0" />' 
		'<item key="onvif.recording0.id"			   type="STRING"	  min="0" max="128" val="" />'
		'<item key="onvif.recording0.name"				 type="STRING"		min="0" max="64" val="Recording0" />'
		'<item key="onvif.recording0.location"				 type="STRING"		min="0" max="64" val="" />'
		'<item key="onvif.recording0.desc"				 type="STRING"		min="0" max="64" val="" />'
		'<item key="onvif.recording0.address"				type="STRING"	   min="0" max="128" val="" />'
		'<item key="onvif.recording0.content"				type="STRING"	   min="0" max="64" val="" />'
		'<item key="onvif.recording0.max_retention_time"			   type="UINT"		min="0" max="4294967295" val="" />'

		*/
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
		 int cnt  = ONVIF_TRACK_CNT * i;
		 sprintf(buff, "%s%d",VIDEO_TRACK, i);
		 strncpy(tmp.track[cnt].TrackToken,buff,COMMON_SIZE - 1);
		 sprintf(buff, "%s%d",AUDIO_TRACK, i);
		 strncpy(tmp.track[cnt+1].TrackToken,buff,COMMON_SIZE - 1);
		 sprintf(buff, "%s%d",METADATA_TRACK, i);
		 strncpy(tmp.track[cnt+2].TrackToken,buff,COMMON_SIZE - 1);

		 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	}
	//tmp.RecordingToken
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (packet_len : %d)",__FUNCTION__, packet_len);
	memcpy(packet.data, &tmp, sizeof(arg_GetRecordingsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetRecordingsResponseItem)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	return ONVIF_ERR_RET_SUCCESS;
}
#endif 
int onvif_AddAudioEncoder(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, config_index = 0, profile_index;

	tmp.result = 1;
	//if( !(isUseAudio() && is_EnableAudio())) {
	//	tmp.result = -10;
	//	return;
	//}

	// find AE configuration index
	for (i = 0; i < ONVIF_AENCODER_CNT; i++) {
		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp.config_token %s, tmp_token %s", tmp.config_token, tmp_token);
		if (!strcmp(tmp.config_token, tmp_token)) {
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
				continue;
			}			
			config_index = i;
			break;
		}
	}
	if (i == ONVIF_AENCODER_CNT) {
		tmp.result = -1;
		return ONVIF_R_ERR;
	}
	// find profile index matching with configuration index
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		_TTY_LOG_ONVIF_DEBUG("tmp.profile_token: %s tmp_token %s", tmp.profile_token, tmp_token);
		if (!strcmp(tmp.profile_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
/*
	if (profile_index >= ONVIF_FIXED_PROFILE_CNT) {
		//baek.FIXME.test tool.
		//if( config_index != 0 )		tmp.result = -2;
		;
	} else if (profile_index < ONVIF_FIXED_PROFILE_CNT) {
		if (config_index != profile_index) {
			tmp.result = -2;
			return ONVIF_R_ERR;
		}
	}
*/
	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.aencoder", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveAudioEncoder(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	// FIX ME::
	//if( !(isUseAudio() && is_EnableAudio())) {
	//	tmp.result = -10;
	//	return;
	//}
	profile_cnt = ONVIF_MAX_PROFILE_CNT;//(int)nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			sprintf(buff, "onvif.profile%d.aencoder", i);
			nf_sysdb_set_str(buff, "\0");
			break;
		}
	}
	if (i == profile_cnt) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	tmp.result = 1;
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveAudioOutputConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	profile_cnt = ONVIF_MAX_PROFILE_CNT;

	for (i = 0; i < profile_cnt; i++)
	{
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token))
		{
			sprintf(buff, "onvif.profile%d.aoutput", i);
			nf_sysdb_set_str(buff, "\0");
			break;
		}
	}

	if (i == profile_cnt)
	{
		return ONVIF_R_ERR;
	}
	
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_RemoveAudioDecoderConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	profile_cnt = ONVIF_MAX_PROFILE_CNT;

	for (i = 0; i < profile_cnt; i++)
	{
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token))
		{
			sprintf(buff, "onvif.profile%d.adecoder", i);
			nf_sysdb_set_str(buff, "\0");
			break;
		}
	}

	if (i == profile_cnt)
	{
		return ONVIF_R_ERR;
	}
	
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_RemoveAudioSource(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	//if( !(isUseAudio() && is_EnableAudio())) {
	//			tmp.result = -10;
	//		return;
	//}
	profile_cnt = ONVIF_MAX_PROFILE_CNT;//(int)nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//	if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			sprintf(buff, "onvif.profile%d.asource", i);
			nf_sysdb_set_str(buff, "\0");
			break;
		}
	}
	if (i == profile_cnt) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	tmp.result = 1;
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddVideoSource(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, vsource_index, profile_index;

	tmp.result = 1;

	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.config_token, tmp_token)) {
			vsource_index = i;
			break;
		}
	}
	if (i == ONVIF_VSOURCE_CNT) {
		tmp.result = -1;
		return ONVIF_R_ERR;
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.vsource", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddVideoEncoder(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, vencoder_index, profile_index;

	tmp.result = 1;
	for (i = 0; i < ONVIF_VENCODER_CNT; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		
		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.config_token, tmp_token)) {
			vencoder_index = i;
			break;
		}
	}
	if (i == ONVIF_VENCODER_CNT) {
		tmp.result = -1;
		return ONVIF_R_ERR;
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

#if 0
	if (profile_index >= ONVIF_FIXED_PROFILE_CNT) {
		//if( vencoder_index != 0 )		tmp.result = -2;
		;
	} else if (profile_index < ONVIF_FIXED_PROFILE_CNT) {
		if (vencoder_index != profile_index)
		{
			tmp.result = -2;
		}
	}
#endif

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.vencoder", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveVideoEncoder(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	profile_cnt = ONVIF_MAX_PROFILE_CNT;//(int)nf_sysdb_get_uint("onvif.common.profile_cnt");

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.vencoder", i);
			nf_sysdb_set_str(buff, "\0");

			tmp.result = 1;
			break;
		}
	}
	if (i == profile_cnt) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveVideoSource(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	profile_cnt = ONVIF_MAX_PROFILE_CNT;//(int)nf_sysdb_get_uint("onvif.common.profile_cnt");
	for (i = 0; i < profile_cnt; i++) {
		//	if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//		continue;
		//	}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			sprintf(buff, "onvif.profile%d.vsource", i);
			nf_sysdb_set_str(buff, "\0");
			tmp.result = 1;
			break;
		}
	}
	if (i == profile_cnt) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_DeleteProfile(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfileDB tmp;

	struct e_profile_change_type prof_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfileDB);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfileDB));

	// To DO
	int profile_cnt, i, j;
	char buff[COMMON_SIZE] = { 0, };
	char buff_next[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");
	if (profile_cnt <= 0) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp_token, tmp.token)) {
			sprintf(buff, "onvif.profile%d.fixed", i);
			if (!nf_sysdb_get_bool(buff)) 
			{
//				if (i == ONVIF_MAX_PROFILE_CNT - 1)
				{
					sprintf(buff, "onvif.profile%d.token", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.name", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.vencoder", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.vsource", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.aencoder", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.asource", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.metadata", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.vanalytics", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.ptz", i);
					nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.aoutput", i);
                    nf_sysdb_set_str(buff, "\0");
					sprintf(buff, "onvif.profile%d.adecoder", i);
                    nf_sysdb_set_str(buff, "\0");
				}
				/*
				else
				{
					//insert current field to next field value and set null the next field
					for (j = i + 1; j < ONVIF_MAX_PROFILE_CNT; j++) {
						sprintf(buff, "onvif.profile%d.token", j - 1);
						sprintf(buff_next, "onvif.profile%d.token", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.name", j - 1);
						sprintf(buff_next, "onvif.profile%d.name", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.vencoder", j - 1);
						sprintf(buff_next, "onvif.profile%d.vencoder", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.vsource", j - 1);
						sprintf(buff_next, "onvif.profile%d.vsource", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.aencoder", j - 1);
						sprintf(buff_next, "onvif.profile%d.aencoder", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.asource", j - 1);
						sprintf(buff_next, "onvif.profile%d.asource", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.metadata", j - 1);
						sprintf(buff_next, "onvif.profile%d.metadata", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.vanalytics", j - 1);
						sprintf(buff_next, "onvif.profile%d.vanalytics", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");

						sprintf(buff, "onvif.profile%d.ptz", j - 1);
						sprintf(buff_next, "onvif.profile%d.ptz", j);
						nf_sysdb_set_str(buff,
								nf_sysdb_get_str_nocopy(buff_next));
						nf_sysdb_set_str(buff_next, "\0");
					}
				}
				*/
				profile_cnt--;
				nf_sysdb_set_uint("onvif.common.profile_cnt", (unsigned int)profile_cnt);


		    	strcpy(prof_ch.token, tmp.token);
    			append_onvif_event_msg(E_KEY_CHANGE_PROFILE, &prof_ch, E_Changed, 0);
				tmp.result = 1;
				// nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
				break;
			} else {
				tmp.result = -1;
				return ONVIF_R_ERR_FIXED_USER;
			}
		}
	}

	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR_NO_PROFILE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_ProfileDB));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_ProfileDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_Profile_GetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfileDB tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfileDB);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfileDB));

	// To DO	
	get_vencoder_table(&tmp.vencoder);

	memcpy(packet.data, &tmp, sizeof(arg_ProfilesDB));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ProfilesDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetProfilesDB(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfilesDB tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfilesDB);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfilesDB));

	// To DO

	int i;
	int profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	
	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	tmp.profile_cnt = profile_cnt;
	_TTY_LOG_ONVIF_DEBUG("tmp.profile_cnt : %d", tmp.profile_cnt);
	int prof_cnt = 0;

	for (i = 0; i < profile_cnt; i++) {
		//_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

		sprintf(buff, "onvif.profile%d.name", i);
		strncpy(tmp.profile[prof_cnt].name, nf_sysdb_get_str_nocopy(buff),	COMMON_SIZE - 1);

		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp.profile[prof_cnt].token, nf_sysdb_get_str_nocopy(buff),	COMMON_SIZE - 1);

		sprintf(buff, "onvif.profile%d.fixed", i);
		tmp.profile[prof_cnt].fixed = nf_sysdb_get_bool(buff);

		sprintf(buff, "onvif.profile%d.vencoder", i);
		strncpy(tmp.profile[prof_cnt].vencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		sprintf(buff, "onvif.profile%d.vsource", i);
		strncpy(tmp.profile[prof_cnt].vsource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		sprintf(buff, "onvif.profile%d.metadata", i);
		strncpy(tmp.profile[prof_cnt].metadata.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		sprintf(buff, "onvif.profile%d.ptz", i);
		strncpy(tmp.profile[prof_cnt].ptz.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

//		int is_audio = 0;
//		is_audio = OV_is_EnableAudio(i);		
//		if(is_audio ){
		sprintf(buff, "onvif.profile%d.asource", i);
		strncpy(tmp.profile[prof_cnt].asource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (tmp.profile[prof_cnt].asource.token[0] != '\0')
			GetAudioSourceConfiguration(&tmp.profile[prof_cnt].asource);

		sprintf(buff, "onvif.profile%d.aencoder", i);
		strncpy(tmp.profile[prof_cnt].aencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (tmp.profile[prof_cnt].aencoder.token[0] != '\0')
			_getAudioEncoderConfiguration(&tmp.profile[prof_cnt].aencoder);
		sprintf(buff, "onvif.profile%d.aoutput", i);
		strncpy(tmp.profile[prof_cnt].aoutput.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (tmp.profile[prof_cnt].aoutput.token[0] != '\0'){
			snprintf(tmp.profile[prof_cnt].aoutput.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
			snprintf(tmp.profile[prof_cnt].aoutput.conf_token, COMMON_SIZE - 1, "aoconfig0");
			tmp.profile[prof_cnt].aoutput.output_level = nf_sysdb_get_uint("audio.out_volume");
			strncpy(tmp.profile[prof_cnt].aoutput.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
			tmp.profile[prof_cnt].aoutput.use_count = 2;
			tmp.profile[prof_cnt].aoutput.enable = 1;
		}

		sprintf(buff, "onvif.profile%d.adecoder", i);
		strncpy(tmp.profile[prof_cnt].adecoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (tmp.profile[prof_cnt].adecoder.token[0] != '\0'){
			snprintf(tmp.profile[prof_cnt].adecoder.name, COMMON_SIZE - 1, "ad0");
			tmp.profile[prof_cnt].adecoder.use_count = 2;
		}
//		}
		
		if (tmp.profile[prof_cnt].metadata.token[0] != '\0') {
			if (_getMetadataConfiguration(&tmp.profile[prof_cnt].metadata) == ONVIF_R_ERR)
				return ONVIF_R_ERR;
		}
		if (tmp.profile[prof_cnt].vencoder.token[0] != '\0')
		{
			get_vencoder_table(&tmp.profile[prof_cnt].vencoder);
			if ( tmp.profile[prof_cnt].vencoder.codec == ONVIF_VIDEO_CODEC_H265 )
				tmp.profile[prof_cnt].vencoder.codec = ONVIF_VIDEO_CODEC_H264;				
		}
		if (tmp.profile[prof_cnt].vsource.token[0] != '\0')
			onvif_Profile_GetVideoSource(&tmp.profile[prof_cnt]);
		if (tmp.profile[prof_cnt].ptz.token[0] != '\0'){
			get_ptz_config(&tmp.profile[prof_cnt].ptz.token);
		}
		prof_cnt++;
	}

	memcpy(packet.data, &tmp, sizeof(arg_ProfilesDB));
	
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_ProfilesDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetProfileDB(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfileDB tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfileDB);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfileDB));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i;
	int profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	// to verify db value is well reserved
	//nf_sysdb_set_uint("onvif.common.profile_cnt", 16);
	profile_cnt = MAX_PROFILE;
	_TTY_LOG_ONVIF_DEBUG("profile_cnt : %d", profile_cnt);
	
	for (i = 0; i < profile_cnt; i++) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		// Get the token for comparision
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token: %s %s", tmp_token, tmp.token);
		if (!strcmp(tmp_token, tmp.token)) {
			strncpy(tmp.token, tmp_token, COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.fixed", i);
			tmp.fixed = nf_sysdb_get_bool(buff);

			sprintf(buff, "onvif.profile%d.name", i);
			strncpy(tmp.name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			// Database is not update sync , this is just test
			sprintf(buff, "onvif.profile%d.vencoder", i);
			strncpy(tmp.vencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.profile%d.vsource", i);
			strncpy(tmp.vsource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
			sprintf(buff, "onvif.profile%d.metadata", i);
			strncpy(tmp.metadata.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.vanalytics", i);
			strncpy(tmp.vanalytics.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);


			sprintf(buff, "onvif.profile%d.ptz", i);
			strncpy(tmp.ptz.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			int is_audio = 0;
//			is_audio = OV_is_EnableAudio(i);
			is_audio = (int)nf_sysdb_get_bool("audio.enable");	
			if(is_audio ){
				tmp.audio_enable = 1;

				sprintf(buff, "onvif.profile%d.asource", i);
				strncpy(tmp.asource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (tmp.asource.token[0] != '\0'){
					GetAudioSourceConfiguration(&tmp.asource);
				}

				sprintf(buff, "onvif.profile%d.aencoder", i);
				strncpy(tmp.aencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (tmp.aencoder.token[0] != '\0'){
					_getAudioEncoderConfiguration(&tmp.aencoder);
				}

				sprintf(buff, "onvif.profile%d.aoutput", i);
                strncpy(tmp.aoutput.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
                if (tmp.aoutput.token[0] != '\0'){
                    snprintf(tmp.aoutput.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
                    snprintf(tmp.aoutput.conf_token, COMMON_SIZE - 1, "aoconfig0");
                    tmp.aoutput.output_level = nf_sysdb_get_uint("audio.out_volume");
                    strncpy(tmp.aoutput.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
					tmp.aoutput.use_count = 2;
                }

                sprintf(buff, "onvif.profile%d.adecoder", i);
                strncpy(tmp.adecoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
                if (tmp.adecoder.token[0] != '\0'){
                    snprintf(tmp.adecoder.name, COMMON_SIZE - 1, "ad0");
					tmp.adecoder.use_count = 2;
				}
			}

			if (tmp.vencoder.token[0] != '\0'){
				get_vencoder_table(&tmp.vencoder);
				if ( tmp.vencoder.codec == ONVIF_VIDEO_CODEC_H265 )
					tmp.vencoder.codec = ONVIF_VIDEO_CODEC_H264;
			}

			if (tmp.vsource.token[0] != '\0'){
				onvif_Profile_GetVideoSource(&tmp);
			}
			if (tmp.metadata.token[0] != '\0') {
				if (_getMetadataConfiguration(&tmp.metadata) == ONVIF_R_ERR) {
					return ONVIF_R_ERR;
				}
				onvif_makeVideoAnalyticsConfiguration(&tmp.vanalytics, ONVIF_GET);
			}

			if (tmp.ptz.token[0] != '\0'){
				get_ptz_config(&tmp.ptz);
			}
			
			if (tmp.vanalytics.token[0] != '\0'){
				onvif_makeVideoAnalyticsConfiguration(&tmp.vanalytics, ONVIF_GET);
			}
			
			tmp.result = 1;
			break;
		}
	}

	if (i == profile_cnt) {
		tmp.result = 0;
		return ONVIF_R_ERR_NO_PROFILE;
	}
	memcpy(packet.data, &tmp, sizeof(arg_ProfileDB));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_ProfileDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetProfilesDB2(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ProfilesDB tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ProfilesDB);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ProfilesDB));

	// To DO
	int i;
	int profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	int is_find_prof = 0;

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	tmp.profile_cnt = profile_cnt;
	_TTY_LOG_ONVIF_DEBUG("(line=%d) tmp.profile_cnt : %d\n", __LINE__, tmp.profile_cnt);
	int prof_cnt = 0;
    int is_fixed = 1;

	for (i = 0; i < profile_cnt; i++) {

		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("(line=%d) tmp_token: %s %s", __LINE__, tmp_token, tmp.prof_token);

		if (!strcmp(tmp_token, tmp.prof_token) && tmp_token[0] != '\0') {		// profile token find
			is_find_prof = 1;
			tmp.profile_cnt = 1;
			strncpy(tmp.profile[0].token, tmp_token, COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.fixed", i);
			tmp.profile[0].fixed = nf_sysdb_get_bool(buff);

			sprintf(buff, "onvif.profile%d.name", i);
			strncpy(tmp.profile[0].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			// Database is not update sync , this is just test
			sprintf(buff, "onvif.profile%d.vencoder", i);
			strncpy(tmp.profile[0].vencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.profile%d.vsource", i);
			strncpy(tmp.profile[0].vsource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.profile%d.metadata", i);
			strncpy(tmp.profile[0].metadata.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.vanalytics", i);
			strncpy(tmp.profile[0].vanalytics.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

//			if(i < ONVIF_PTZ_CNT)
			{
				sprintf(buff, "onvif.profile%d.ptz", i);
				strncpy(tmp.profile[0].ptz.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			}

			int is_audio = 0;
			is_audio = (int)nf_sysdb_get_bool("audio.enable");
			if( is_audio ) {
				tmp.profile[0].audio_enable = 1;
				sprintf(buff, "onvif.profile%d.asource", i);
				strncpy(tmp.profile[0].asource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				if (tmp.profile[0].asource.token[0] != '\0'){
					GetAudioSourceConfiguration(&tmp.profile[0].asource);
				}

				sprintf(buff, "onvif.profile%d.aencoder", i);
				strncpy(tmp.profile[0].aencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (tmp.profile[0].aencoder.token[0] != '\0'){
				_getAudioEncoderConfiguration(&tmp.profile[0].aencoder);
				}

				sprintf(buff, "onvif.profile%d.aoutput", i);
				strncpy(tmp.profile[0].aoutput.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (tmp.profile[0].aoutput.token[0] != '\0'){
					snprintf(tmp.profile[0].aoutput.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
					snprintf(tmp.profile[0].aoutput.conf_token, COMMON_SIZE - 1, "aoconfig0");
					tmp.profile[0].aoutput.output_level = nf_sysdb_get_uint("audio.out_volume");
					strncpy(tmp.profile[0].aoutput.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
					tmp.profile[0].aoutput.use_count = 2;
					tmp.profile[0].aoutput.enable = 1;
				}
				sprintf(buff, "onvif.profile%d.adecoder", i);
				strncpy(tmp.profile[0].adecoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (tmp.profile[0].adecoder.token[0] != '\0'){
					snprintf(tmp.profile[0].adecoder.name, COMMON_SIZE - 1, "ad0");
					tmp.profile[0].adecoder.use_count = 2;
				}
			}

			if (tmp.profile[0].vencoder.token[0] != '\0'){
				get_vencoder_table(&tmp.profile[0].vencoder);
			}

			if (tmp.profile[0].vsource.token[0] != '\0'){
				onvif_Profile_GetVideoSource(&tmp.profile[0]);
			}
			if (tmp.profile[0].metadata.token[0] != '\0') {
				if (_getMetadataConfiguration(&tmp.profile[0].metadata) == ONVIF_R_ERR) {
					return ONVIF_R_ERR;
				}
				onvif_makeVideoAnalyticsConfiguration(&tmp.profile[0].vanalytics, ONVIF_GET);
			}

			if (tmp.profile[0].ptz.token[0] != '\0'){
				get_ptz_config(&tmp.profile[0].ptz);
			}
			
			if (tmp.profile[0].vanalytics.token[0] != '\0'){
				onvif_makeVideoAnalyticsConfiguration(&tmp.profile[0].vanalytics, ONVIF_GET);
			}
			
			tmp.profile[0].result = 1;
			goto exit;
		}
	}
	if ( !is_find_prof && tmp.prof_token[0] != '\0' )
		return ONVIF_R_ERR_NO_PROFILE;

	if (!is_find_prof)
	{
		for (i = 0; i < profile_cnt; i++) {			
			sprintf(buff, "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
            sprintf(buff, "onvif.profile%d.fixed", i);
            is_fixed = nf_sysdb_get_bool(buff);
            
			/*if(tmp_token[0] == NULL && !is_fixed) {
                break;
			}*/

			if (tmp_token[0] == NULL)
			{
				continue;
			}
			
			//_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.name", i);
			strncpy(tmp.profile[prof_cnt].name, nf_sysdb_get_str_nocopy(buff),	COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.token", i);
			strncpy(tmp.profile[prof_cnt].token, nf_sysdb_get_str_nocopy(buff),	COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.fixed", i);
			tmp.profile[prof_cnt].fixed = nf_sysdb_get_bool(buff);

			sprintf(buff, "onvif.profile%d.vencoder", i);
			strncpy(tmp.profile[prof_cnt].vencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.vsource", i);
			strncpy(tmp.profile[prof_cnt].vsource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.metadata", i);
			strncpy(tmp.profile[prof_cnt].metadata.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			//if(i < ONVIF_PTZ_CNT)
			//{
				sprintf(buff, "onvif.profile%d.ptz", i);
				strncpy(tmp.profile[prof_cnt].ptz.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			//}

			int is_audio = 0;
	//		is_audio = OV_is_EnableAudio(i);		
			is_audio = (int)nf_sysdb_get_bool("audio.enable");
			//if(is_audio ){
			tmp.profile[prof_cnt].audio_enable = 1;
			sprintf(buff, "onvif.profile%d.asource", i);
			strncpy(tmp.profile[prof_cnt].asource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (tmp.profile[prof_cnt].asource.token[0] != '\0')
				GetAudioSourceConfiguration(&tmp.profile[prof_cnt].asource);

			sprintf(buff, "onvif.profile%d.aencoder", i);
			strncpy(tmp.profile[prof_cnt].aencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (tmp.profile[prof_cnt].aencoder.token[0] != '\0')
				_getAudioEncoderConfiguration(&tmp.profile[prof_cnt].aencoder);

			sprintf(buff, "onvif.profile%d.aoutput", i);
			strncpy(tmp.profile[prof_cnt].aoutput.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (tmp.profile[prof_cnt].aoutput.token[0] != '\0'){
				snprintf(tmp.profile[prof_cnt].aoutput.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
				snprintf(tmp.profile[prof_cnt].aoutput.conf_token, COMMON_SIZE - 1, "aoconfig0");
				tmp.profile[prof_cnt].aoutput.output_level = nf_sysdb_get_uint("audio.out_volume");
				strncpy(tmp.profile[prof_cnt].aoutput.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
				tmp.profile[prof_cnt].aoutput.use_count = 2;
				tmp.profile[prof_cnt].aoutput.enable = 1;
			}

			sprintf(buff, "onvif.profile%d.adecoder", i);
			strncpy(tmp.profile[prof_cnt].adecoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (tmp.profile[prof_cnt].adecoder.token[0] != '\0'){
				snprintf(tmp.profile[prof_cnt].adecoder.name, COMMON_SIZE - 1, "ad0");
				tmp.profile[prof_cnt].adecoder.use_count = 2;
				}
			//}

			if (tmp.profile[prof_cnt].metadata.token[0] != '\0') {
				if (_getMetadataConfiguration(&tmp.profile[prof_cnt].metadata) == ONVIF_R_ERR)
					return ONVIF_R_ERR;
			}

			if (tmp.profile[prof_cnt].vencoder.token[0] != '\0')
				get_vencoder_table(&tmp.profile[prof_cnt].vencoder);
			if (tmp.profile[prof_cnt].vsource.token[0] != '\0')
				onvif_Profile_GetVideoSource(&tmp.profile[prof_cnt]);

			if (tmp.profile[prof_cnt].ptz.token[0] != '\0'){
				get_ptz_config(&tmp.profile[prof_cnt].ptz);
			}
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.vanalytics", i);
			strncpy(tmp.profile[prof_cnt].vanalytics.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (tmp.profile[prof_cnt].vanalytics.token[0] != '\0'){
				onvif_makeVideoAnalyticsConfiguration(&tmp.profile[prof_cnt].vanalytics, ONVIF_GET);
			}
			prof_cnt++;
		}

	}

exit:
	memcpy(packet.data, &tmp, sizeof(arg_ProfilesDB));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_ProfilesDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoder tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoder));

	// To DO
	get_vencoder_table(&tmp);

	if ( tmp.codec == ONVIF_VIDEO_CODEC_H265 )
		tmp.codec = ONVIF_VIDEO_CODEC_H264;

	if(tmp.result == 0)
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoder));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoEncoders(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoders));

	// To DO
	char buff[COMMON_SIZE] = { 0, };
	int vencoder_cnt;
	int i;

	vencoder_cnt = ONVIF_VENCODER_CNT;

	tmp.vencoder_cnt = 0;

	for (i = 0; i < vencoder_cnt; i++) {
		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp.vencoder[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (tmp.vencoder[i].token[0] != '\0')
		{
			get_vencoder_table(&tmp.vencoder[i]);
			if ( tmp.vencoder[i].codec == ONVIF_VIDEO_CODEC_H265 )
				tmp.vencoder[i].codec = ONVIF_VIDEO_CODEC_H264;
			if ( tmp.vencoder[i].result == 1)
				tmp.vencoder_cnt++;
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_VideoEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoEncoders2(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoders));
	// To DO
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	int vencoder_cnt = 0;
	int is_prof_token = 0, is_conf_token = 0;
	int i = 0, j = 0, profile_cnt = 0, v_idx = 0;

	vencoder_cnt = ONVIF_VENCODER_CNT;
	tmp.vencoder_cnt = 0;
	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
    
	if ( tmp.conf_token != NULL && tmp.conf_token[0] != '\0')	// Configuration Token Exist
	{
		is_conf_token = 1;
	}
	if ( tmp.token != NULL && tmp.token[0] != '\0')	// Profile Token Exist
	{
		is_prof_token = 1;
	}

	if ( !is_conf_token && !is_prof_token )		// All Vencoder Token
	{
		for (i = 0; i < ONVIF_VENCODER_CNT; i++)
		{
			sprintf(buff, "onvif.vencoder%d.token", i);
			strncpy(tmp.vencoder[v_idx].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if ( strcmp(tmp.vencoder[v_idx].token, "") )
			{
				get_vencoder_table(&tmp.vencoder[v_idx]);
	            tmp.vencoder_cnt++;
				v_idx++;
			}
		}
		goto exit;
	}

	if ( is_prof_token && is_conf_token )
	{
		for ( i = 0; i < MAX_PROFILE; i++)
		{
			memset(buff, 0x00, sizeof(buff));
            snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.token", i);
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if ( !strcmp(tmp_token, tmp.token) )
			{
				memset(buff, 0x00, sizeof(buff));
	            snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.vencoder", i);
	            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if ( strcmp(tmp_token, tmp.conf_token) )
				{
					return ONVIF_R_ERR_NO_CONFIG;
				}
				else
				{
					strncpy(tmp.vencoder[0].token, tmp.conf_token, COMMON_SIZE -1 );
					get_vencoder_table(&tmp.vencoder[0]);
					tmp.vencoder_cnt++;
					goto exit;				
				}
			}				
		}
	}

	if ( is_conf_token )
	{
		for (i = 0; i < vencoder_cnt; i++)
		{
			memset(buff, 0x00, sizeof(buff));
            snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", i);
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
            if (!strcmp(tmp.conf_token, tmp_token) )
			{
				strncpy(tmp.vencoder[0].token, tmp.conf_token, COMMON_SIZE -1 );
				get_vencoder_table(&tmp.vencoder[0]);
				tmp.vencoder_cnt++;
				goto exit;
            }
        }
		if ( i == ONVIF_VENCODER_CNT )		// Not Find Configuration Token
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "onvif.profile0.vencoder");
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
			strncpy(tmp.vencoder[0].token, tmp_token, COMMON_SIZE -1 );
			get_vencoder_table(&tmp.vencoder[0]);
			tmp.vencoder_cnt++;
			goto exit;
		}
	}

	if ( is_prof_token )
	{
		for ( i = 0; i < MAX_PROFILE; i++)
		{
			memset(buff, 0x00, sizeof(buff));
            snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.token", i);
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if ( !strcmp(tmp_token, tmp.token) )
			{
				
				memset(buff, 0x00, sizeof(buff));
	            snprintf(buff, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.profile%d.vencoder"), i);
	            strncpy(tmp_token, buff, COMMON_SIZE - 1);
				
				// No vencoder Token -> ve1
				if ( !strstr(tmp_token, "ve"))
				{
					// return ALL profiles
					for (j = 0; j < ONVIF_VENCODER_CNT; j++)
					{
						sprintf(buff, "onvif.vencoder%d.token", j);
						strncpy(tmp.vencoder[v_idx].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						if ( strcmp(tmp.vencoder[v_idx].token, "") )
						{
							get_vencoder_table(&tmp.vencoder[v_idx]);
							tmp.vencoder_cnt++;
							v_idx++;
						}
					}
					goto exit;
				}

				strncpy(tmp.vencoder[0].token, tmp_token, COMMON_SIZE -1 );
				get_vencoder_table(&tmp.vencoder[0]);
				tmp.vencoder_cnt++;
				break;
			}
		}
		if ( i == MAX_PROFILE )
		{
			memset(buff, 0x00, sizeof(buff));
			snprintf(buff, COMMON_SIZE - 1, "onvif.profile0.vencoder");
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
			strncpy(tmp.vencoder[0].token, tmp_token, COMMON_SIZE -1 );
			get_vencoder_table(&tmp.vencoder[0]);
			tmp.vencoder_cnt++;
			goto exit;
		}
	}

exit:
	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_VideoEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

void wait_for_start_streaming()
{
	sleep(15);
}

int onvif_SetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoder tmp;

	struct e_configuration_change_type config_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoder));

	// To DO
	int need_to_be_check_streaming = 0;
	set_vencoder_table(&tmp);
	if (tmp.result == ONVIF_R_ERR_INVALID_PARAM) {
		return ONVIF_R_ERR_INVALID_PARAM;
	}
	nf_issm_ctl_set_restart_checker();
	time_t currentTime;
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_CAM);	
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_NET);

	if (need_to_be_check_streaming == 1) {
		int count = 0;
		//Waiting for the streaming module to restart
		while (nf_issm_ctl_get_restart_checker() == 0 && count++ < 50) {
			usleep(100*1000);
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoder));

	memset(&config_ch, 0x00, sizeof(config_ch));
    strcpy(config_ch.token, tmp.token);
    strcpy(config_ch.type, "VideoEncoder");
    append_onvif_event_msg(E_KEY_CHANGE_CONFIGURATION, &config_ch, E_Changed, 0);
	wait_for_start_streaming();

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_VideoEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetStreamUri(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_StreamUri tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_StreamUri);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_StreamUri));

	// To DO
	int i, profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token: %s (tmp.token: %s)",tmp_token, tmp.token);
		if (!strcmp(tmp.token, tmp_token)) {
			break;
		}
	}
	if (i == profile_cnt) {
		_TTY_LOG_ONVIF_DEBUG("i :  %d (profile_cnt: %d)", i, profile_cnt);
		//tmp.result = 0;
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}
	get_streamuri(&tmp, NULL);
	memcpy(packet.data, &tmp, sizeof(arg_StreamUri));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_StreamUri)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetStreamUri2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_StreamUri2 tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_StreamUri2);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_StreamUri2));

	// To DO
	int i, profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = MAX_PROFILE;//nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < MAX_PROFILE; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token: %s (tmp.token: %s)",tmp_token, tmp.token);
		if (!strcmp(tmp.token, tmp_token)) {
			break;
		}
	}
	if (i == MAX_PROFILE) {
		_TTY_LOG_ONVIF_DEBUG("i :  %d (profile_cnt: %d)", i, profile_cnt);
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}

	get_streamuri2(&tmp, NULL);

	memcpy(packet.data, &tmp, sizeof(arg_StreamUri2));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_StreamUri2)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetCurCodec(int stream)
{
	gint codec = 0;
	gchar buff[COMMON_SIZE] = {0};

	sprintf(buff, "rec.video.V0.stream.S%d.codec", stream);
	codec = nf_sysdb_get_uint(buff);

	return codec;
}

int onvif_StrToResolution(char *resol, int *width, int *height)
{
	/*  [in] resol : "1920x1080"
	 *  [out] width : 1920
	 *  [out] height : 1080
	 */
	int i = 0;
	int len = 0;
	gchar w[5] = {0};
	gchar h[5] = {0};

	len = strlen(resol);
	for(i = 0; i < len; i++)
	{
		if(resol[i] == 'x')
		{
			strncpy(w, resol, i);
			strncpy(h, resol + (i + 1), len - i);

			*width = atoi(w);
			*height = atoi(h);
			return 0;
		}
	}
	return -1;
}

int onvif_StrToFps(char *capa_string)
{
	int rLength = strlen(capa_string);
	int cnt = 0;
	char max_fps;

	max_fps = capa_string[rLength-1];

	switch(max_fps) {
		case 'A' : 
			return 0;
			break;
		case 'B' : 
			return 1;
			break;
		case 'C' : 
			return 2;
			break;
		case 'D' : 
			return 3;
			break;
		case 'E' : 
			return 4;
			break;
		case 'F' : 
			return 5;
			break;
		case 'G' : 
			return 6;
			break;
		case 'H' : 
			return 7;
			break;
		case 'I' : 
			return 8;
			break;
		case 'J' : 
			return 9;
			break;
		case 'K' : 
			return 10;
			break;
		case 'L' : 
			return 11;
			break;
		case 'M' : 
			return 12;
			break;
		case 'N' : 
			return 13;
			break;
		case 'O' : 
			return 14;
			break;
		case 'P' : 
			return 15;
			break;
		case 'Q' : 
			return 16;
			break;
		case 'R' : 
			return 17;
			break;
		case 'S' : 
			return 18;
			break;
		case 'T' : 
			return 19;
			break;
		case 'U' : 
			return 20;
			break;
		case 'V' : 
			return 21;
			break;
		case 'W' : 
			return 22;
			break;
		case 'X' : 
			return 23;
			break;
		case 'Y' : 
			return 24;
			break;
		case 'Z' : 
			return 25;
			break;
		case 'a' : 
			return 26;
			break;
		case 'b' : 
			return 27;
			break;
		case 'c' : 
			return 28;
			break;
		case 'd' : 
			return 29;
			break;
		case 'e' : 
			return 30;
			break;
		default :
            return 1;
            break;
	}
}

int onvif_GetVideoEncoderOption(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoderOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoderOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoderOption));

	int ch = 0;
	int stream = 0;

	int i = 0, j = 0;
	char vencoder_token[COMMON_SIZE] = { 0, };
	char profile_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	
	char avail_res_1st[32] = {0,};
	char avail_res_2nd[32] = {0,};
	char cur_fps;
 
	int vencoder_cnt = ONVIF_VENCODER_CNT;

	guint64 tmp_resol_capa, tmp_resol_cur;

	tmp.vs_type = DISPLAY_IS_PAL;
	// if no options specified, Generic option shall be specified

	if (tmp.profile_token[0] == '\0')
	{
		strncpy(tmp.profile_token, "p0", COMMON_SIZE - 1);	
	}
	
	if (tmp.encoder_token[0] == '\0')
	{
		for(j=0;j<vencoder_cnt;j++)
		{
			snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.token", j);
			strncpy(buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if(strcmp(tmp.profile_token, buff) == 0)
				break;
		}
		snprintf(tmp.encoder_token, COMMON_SIZE - 1, "ve%d", j);	
	}
	
	for (i = 0; i < vencoder_cnt; i++) {
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.vencoder%d.token", i);
		
		strncpy(vencoder_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.encoder_token, vencoder_token)) {		
			if(stream == 0)
			{
				/* main */
				nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_H264);

				resol_bitmask_to_char(tmp_resol_capa, DISPLAY_IS_PAL, avail_res_1st);

				tmp.option_cnt = 0;
				get_h264_options(&tmp.voption[tmp.option_cnt], avail_res_1st, ch,  stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 512;
				tmp.voption[tmp.option_cnt].max_bitrate = 15000;
				tmp.option_cnt++;

				nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_JPEG);
				resol_bitmask_to_char(tmp_resol_capa, DISPLAY_IS_PAL, avail_res_1st);
				get_jpeg_options(&tmp.voption[tmp.option_cnt], avail_res_1st, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 512;
				tmp.voption[tmp.option_cnt].max_bitrate = 15000;
				tmp.option_cnt++;
			}
			else
			{			    
				/* second */
				nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_H264);
				resol_bitmask_to_char(tmp_resol_capa, DISPLAY_IS_PAL, avail_res_2nd);
				tmp.option_cnt = 0;
				get_h264_options(&tmp.voption[tmp.option_cnt], avail_res_2nd, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 64;
				tmp.voption[tmp.option_cnt].max_bitrate = 1500;
				tmp.option_cnt++;

				nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_JPEG);
				resol_bitmask_to_char(tmp_resol_capa, DISPLAY_IS_PAL, avail_res_2nd);
				get_jpeg_options(&tmp.voption[tmp.option_cnt], avail_res_2nd, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 64;
				tmp.voption[tmp.option_cnt].max_bitrate = 1500;
				tmp.option_cnt++;
			}
			
			tmp.result = 1;
			break;
		}
	}
	// end of video encoder
	if (i == vencoder_cnt) {
		tmp.result = -1;
		return ONVIF_R_ERR_NO_CONFIG;
	}
	

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoderOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_VideoEncoderOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoEncoderOption2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoderOption2 tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoderOption2);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoderOption2));

	//for utm2
	int ch = 0;
	int stream = 0;

	int i = 0, j = 0;
	char vencoder_token[COMMON_SIZE] = { 0, };
	char profile_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	
	char avail_res_1st[32] = {0,};
	char avail_res_2nd[32] = {0,};
 
	int vencoder_cnt = ONVIF_VENCODER_CNT;

	guint64 tmp_resol_capa, tmp_resol_cur;
	NFIPCamEncoderCap info;
	memset(&info, 0, sizeof(NFIPCamEncoderCap));

	tmp.vs_type = DISPLAY_IS_PAL;
	// if no options specified, Generic option shall be specified

	if (tmp.profile_token[0] == '\0')
	{
		strncpy(tmp.profile_token, "p0", COMMON_SIZE - 1);
	}
	
	if (tmp.encoder_token[0] == '\0')
	{
		for(j=0;j<vencoder_cnt;j++)
		{
			snprintf(buff, COMMON_SIZE - 1, "onvif.profile%d.token", j);
			strncpy(buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if(strcmp(tmp.profile_token, buff) == 0)
				break;
		}
		snprintf(tmp.encoder_token, COMMON_SIZE - 1, "ve%d", j);	
	}
	
	for (i = 0; i < vencoder_cnt; i++) {
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.vencoder%d.token", i);
		
		strncpy(vencoder_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.encoder_token, vencoder_token)) {		
			RESOL_INFO_T resol;
			memset(&resol, 0, sizeof(resol));

			if(stream == 0)
			{
				/* main */
				nf_ipcam_get_encoder_capability(ch, &info);
				resol_bitmask_to_char(info.res_support[0], DISPLAY_IS_PAL, avail_res_1st);
				// empty resol case
				if (avail_res_1st[0] == '\0') {
					snprintf(avail_res_1st, sizeof(avail_res_1st), "RQST");
				}
				tmp.option_cnt = 0;
				get_h265_options(&tmp.voption[tmp.option_cnt], avail_res_1st, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 512;
				tmp.voption[tmp.option_cnt].max_bitrate = 15000;
				tmp.option_cnt++;

				// nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_H264);
				// resol_bitmask_to_char(info.res_support[0], DISPLAY_IS_PAL, avail_res_1st);
				get_h264_options(&tmp.voption[tmp.option_cnt], avail_res_1st, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 512;
				tmp.voption[tmp.option_cnt].max_bitrate = 15000;
				tmp.option_cnt++;

				// nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_JPEG);
				// resol_bitmask_to_char(tmp_resol_capa, DISPLAY_IS_PAL, avail_res_1st);
				// get_jpeg_options(&tmp.voption[tmp.option_cnt], avail_res_1st, ch, stream);
				// tmp.voption[tmp.option_cnt].min_bitrate = 512;
				// tmp.voption[tmp.option_cnt].max_bitrate = 15000;
				// tmp.option_cnt++;
			}
			else
			{			    
				/* second */
				onvif_get_cam_resol_profile(ch, &info);
				resol_bitmask_to_char(info.res_support[1], DISPLAY_IS_PAL, avail_res_2nd);

				if (avail_res_2nd[0] == '\0') {					
					snprintf(avail_res_2nd, sizeof(avail_res_2nd), "RQST");
				}
				tmp.option_cnt = 0;
				get_h265_options(&tmp.voption[tmp.option_cnt], avail_res_2nd, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 64;
				tmp.voption[tmp.option_cnt].max_bitrate = 1500;
				tmp.option_cnt++;

				// nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_H264);
				// resol_bitmask_to_char(info.res_support[0], DISPLAY_IS_PAL, avail_res_2nd);
				get_h264_options(&tmp.voption[tmp.option_cnt], avail_res_2nd, ch, stream);
				tmp.voption[tmp.option_cnt].min_bitrate = 64;
				tmp.voption[tmp.option_cnt].max_bitrate = 1500;
				tmp.option_cnt++;

				// nf_live_get_resol_by_codec(ch, stream, &tmp_resol_capa, &tmp_resol_cur, 0, NF_VIDEO_CODEC_JPEG);
				// resol_bitmask_to_char(tmp_resol_capa, DISPLAY_IS_PAL, avail_res_2nd);
				// get_jpeg_options(&tmp.voption[tmp.option_cnt], avail_res_2nd, ch, stream);
				// tmp.voption[tmp.option_cnt].min_bitrate = 64;
				// tmp.voption[tmp.option_cnt].max_bitrate = 1500;
				// tmp.option_cnt++;
			}
			
			tmp.result = 1;
			break;
		}
	}
	// end of video encoder
	if (i == vencoder_cnt) {
		tmp.result = -1;
		return ONVIF_R_ERR_NO_CONFIG;
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoderOption2));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_VideoEncoderOption2)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetVideoSource(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSource tmp;

	struct e_configuration_change_type config_ch;
	
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSource);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSource));

	// To DO
	set_vsource_table(&tmp);

	if (tmp.result == -1) {
		return ONVIF_R_ERR_CONFIG_MODIFY;
	}

	memset(&config_ch, 0x00, sizeof(config_ch));
	strcpy(config_ch.token, tmp.token);
	strcpy(config_ch.type, "VideoSource");
	append_onvif_event_msg(E_KEY_CHANGE_CONFIGURATION, &config_ch, E_Changed, 0);

	memcpy(packet.data, &tmp, sizeof(arg_VideoSource));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_VideoSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

void GetImagingSetting(arg_ImagingOption *tmp) 
{

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;
	int result = 0;
	int i;

	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp->token %s tmp_token %s", tmp->token, tmp_token);
		if (!strcmp(tmp->token, tmp_token)) {
			// i is designated field
			// BacklightCompensation
			sprintf(buff, "cam.C%d.bright", i);
			tmp->brightness = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.color", i);
			tmp->color = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.contrast", i);
			tmp->contrast = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.sharpness", i);
			tmp->sharpness = nf_sysdb_get_uint(buff);			
			tmp->save_flag = 1;
			result = 1;
			break;
		}
	}
	if(result == 0){
		return ONVIF_R_ERR_NO_SOURCE;
	}

	tmp->result = 1;
}

static void _getVideoSourceConfiguration(arg_VideoSource *tmp)
{

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int i;

	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp->token, tmp_token)) {
			sprintf(buff, "onvif.vsource%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.vsource%d.source_token", i);
			strncpy(tmp->s_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			//video source fps is first stream's
			//tmp_fps = nf_sysdb_get_uint("camera.S0.fps");
			//tmp->fps = get_fps_from_index(tmp_fps);
			tmp->fps = DISPLAY_IS_PAL?25.0:30.0;

			sprintf(buff, "onvif.vsource%d.bound_x", i);
			tmp->x = (int) nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource%d.bound_y", i);
			tmp->y = (int) nf_sysdb_get_uint(buff);

			sprintf(buff, "onvif.vsource%d.bound_width", i);
			tmp->width = (int) nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource%d.bound_height", i);
			tmp->height = (int) nf_sysdb_get_uint(buff);

			tmp->use_count = get_vsource_usecount(tmp->token);
			strncpy(tmp->imaging.token, tmp->token, COMMON_SIZE - 1);
			GetImagingSetting(&tmp->imaging);			
			tmp->result = 1;
			break;
		}
	}

	if (i == ONVIF_VSOURCE_CNT) {
		tmp->result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n", __FUNCTION__, __LINE__);
	}

}
int onvif_GetVideoSourceConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSource tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSource);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSource));

	_getVideoSourceConfiguration(&tmp);

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoSource));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_VideoSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoSourceConfigurations(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSources));

	// To DO
	char buff[COMMON_SIZE] = { 0, };
	int i, tmp_fps;
	tmp.vsource_cnt = ONVIF_VSOURCE_CNT;

	for (i = 0; i < tmp.vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		sprintf(buff, "onvif.vsource%d.name", i);
		strncpy(tmp.vsource[i].name, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp.vsource[i].s_token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		//video source fps is first stream's
		// WARNING ME :: FPS is not stated
		//	tmp_fps = nf_sysdb_get_uint("camera.S0.fps");
		//tmp.vsource[i].fps = get_fps_from_index(tmp_fps);

		sprintf(buff, "onvif.vsource%d.bound_x", i);
		tmp.vsource[i].x = nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.vsource%d.bound_y", i);
		tmp.vsource[i].y = nf_sysdb_get_uint(buff);

		sprintf(buff, "onvif.vsource%d.bound_width", i);
		tmp.vsource[i].width = nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.vsource%d.bound_height", i);
		tmp.vsource[i].height = nf_sysdb_get_uint(buff);

		//get use count
		tmp.vsource[i].use_count = get_vsource_usecount(tmp.vsource[i].token);
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_VideoSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetVideoSourceConfigurations2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSources));

	// To DO
	char buff[COMMON_SIZE] = { 0, }, profileToken[COMMON_SIZE] = { 0, }, configToken[COMMON_SIZE] = { 0, };
	int i = 0, j = 0, tmp_fps;
	int is_config_token = 0, is_prof_token = 0;
	
	tmp.vsource_cnt = 0;

	memset(buff, 0x00, sizeof(buff));

	if (tmp.vsource[0].token != NULL && tmp.vsource[0].token[0] != '\0')	// If ConfigurationToken was provided
	{
		memset(configToken, 0x00, sizeof(configToken));
		
		for (i = 0; i < ONVIF_VSOURCE_CNT; i++)
		{
			sprintf(buff, "onvif.vsource%d.token", i);
			strncpy(configToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (!strcmp(tmp.vsource[0].token, configToken))
			{
				is_config_token = 1;
				break;
			}
		}
		if (i == ONVIF_VSOURCE_CNT)
			return ONVIF_R_ERR_NO_SOURCE;
	}
	else if (tmp.token != NULL && tmp.token[0] != '\0')				// If ProfileToken was provided
	{
		memset(profileToken, 0x00, sizeof(profileToken));

		for (j = 0; j < MAX_PROFILE; j++)
		{
			sprintf(buff, "onvif.profile%d.token", j);
			strncpy(profileToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (!strcmp(tmp.token, profileToken))
			{
				is_prof_token = 1;
				break;
			}
		}
	}

	if (is_config_token == 0 && is_prof_token == 0)		// If ConfigurationToken and ProfileToken wasn't provided
	{
		for (i = 0; i < ONVIF_VSOURCE_CNT; i++)
		{
			sprintf(buff, "onvif.vsource%d.token", i);
			strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (tmp.vsource[i].token[0] != '\0')
			{
				sprintf(buff, "onvif.vsource%d.token", i);
				strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.name", i);
				strncpy(tmp.vsource[i].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.source_token", i);
				strncpy(tmp.vsource[i].s_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.bound_x", i);
				tmp.vsource[i].x = nf_sysdb_get_uint(buff);
				
				sprintf(buff, "onvif.vsource%d.bound_y", i);
				tmp.vsource[i].y = nf_sysdb_get_uint(buff);

				sprintf(buff, "onvif.vsource%d.bound_width", i);
				tmp.vsource[i].width = nf_sysdb_get_uint(buff);
				
				sprintf(buff, "onvif.vsource%d.bound_height", i);
				tmp.vsource[i].height = nf_sysdb_get_uint(buff);

				//get use count
				tmp.vsource[i].use_count = get_vsource_usecount(tmp.vsource[i].token);

				tmp.vsource_cnt++;
			}
		}
	}
	else
	{
		if (is_config_token == 0)		// If only ProfileToken was provided
		{
			sprintf(buff, "onvif.profile%d.vsource", j);
			strncpy(tmp.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (tmp.token[0] != '\0')
			{
				for (i = 0; i < ONVIF_VSOURCE_CNT; i++)
				{
					sprintf(buff, "onvif.vsource%d.token", i);
					strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
					if (tmp.vsource[i].token[0] != '\0')
					{
						sprintf(buff, "onvif.vsource%d.token", i);
						strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						sprintf(buff, "onvif.vsource%d.name", i);
						strncpy(tmp.vsource[i].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						sprintf(buff, "onvif.vsource%d.source_token", i);
						strncpy(tmp.vsource[i].s_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						sprintf(buff, "onvif.vsource%d.bound_x", i);
						tmp.vsource[i].x = nf_sysdb_get_uint(buff);
						
						sprintf(buff, "onvif.vsource%d.bound_y", i);
						tmp.vsource[i].y = nf_sysdb_get_uint(buff);

						sprintf(buff, "onvif.vsource%d.bound_width", i);
						tmp.vsource[i].width = nf_sysdb_get_uint(buff);
						
						sprintf(buff, "onvif.vsource%d.bound_height", i);
						tmp.vsource[i].height = nf_sysdb_get_uint(buff);

						//get use count
						tmp.vsource[i].use_count = get_vsource_usecount(tmp.vsource[i].token);

						tmp.vsource_cnt++;
					}
				}
			}
			else
			{
				sprintf(buff, "onvif.vsource%d.token", j % ONVIF_CH);
				strncpy(tmp.vsource[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.name", j % ONVIF_CH);
				strncpy(tmp.vsource[0].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.source_token", j % ONVIF_CH);
				strncpy(tmp.vsource[0].s_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.bound_x", j % ONVIF_CH);
				tmp.vsource[0].x = nf_sysdb_get_uint(buff);
				
				sprintf(buff, "onvif.vsource%d.bound_y", j % ONVIF_CH);
				tmp.vsource[0].y = nf_sysdb_get_uint(buff);

				sprintf(buff, "onvif.vsource%d.bound_width", j % ONVIF_CH);
				tmp.vsource[0].width = nf_sysdb_get_uint(buff);
				
				sprintf(buff, "onvif.vsource%d.bound_height", j % ONVIF_CH);
				tmp.vsource[0].height = nf_sysdb_get_uint(buff);

				//get use count
				tmp.vsource[0].use_count = get_vsource_usecount(tmp.vsource[0].token);

				tmp.vsource_cnt = 1;
			}
		}
		else			// If only Configuration was provided or Configuration & ProfileToken was provided
		{
			memset(buff, 0x00, sizeof(buff));
			sprintf(buff, "vsconfig%d", i);

			if (!strcmp(tmp.vsource[0].token, buff))
			{
				sprintf(buff, "onvif.vsource%d.token", i);
				strncpy(tmp.vsource[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.name", i);
				strncpy(tmp.vsource[0].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.source_token", i);
				strncpy(tmp.vsource[0].s_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				sprintf(buff, "onvif.vsource%d.bound_x", i);
				tmp.vsource[0].x = nf_sysdb_get_uint(buff);
				
				sprintf(buff, "onvif.vsource%d.bound_y", i);
				tmp.vsource[0].y = nf_sysdb_get_uint(buff);

				sprintf(buff, "onvif.vsource%d.bound_width", i);
				tmp.vsource[0].width = nf_sysdb_get_uint(buff);
				
				sprintf(buff, "onvif.vsource%d.bound_height", i);
				tmp.vsource[0].height = nf_sysdb_get_uint(buff);

				//get use count
				tmp.vsource[0].use_count = get_vsource_usecount(tmp.vsource[0].token);

				tmp.vsource_cnt = 1;
			}
			else
			{
				return ONVIF_R_ERR_NO_SOURCE;
			}
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_VideoSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetAudioSource(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSource tmp;

	struct e_configuration_change_type config_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSource);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSource));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i;
	int asource_cnt = ONVIF_ASOURCE_CNT;

	_TTY_LOG_ONVIF("Set Audio [%s]", tmp.token);

	for (i = 0; i < asource_cnt; i++) {
		sprintf(buff, "onvif.asource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
				continue;
			}
			sprintf(buff, "onvif.asource%d.source_token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
			if (tmp.s_token[0] != '\0') {
				if (!strcmp(tmp.s_token, tmp_token)) {
					nf_sysdb_set_str(buff, tmp.s_token);
				} else {
					tmp.result = -1;
					return ONVIF_R_ERR;
				}
			}
			if (tmp.name[0] != '\0') {
				sprintf(buff, "onvif.asource%d.name", i);
				nf_sysdb_set_str(buff, tmp.name);
			}

			if (tmp.save_flag == 1) {
				nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			}

			memset(&config_ch, 0x00, sizeof(config_ch));
			strcpy(config_ch.token, tmp.token);
			strcpy(config_ch.type, "AudioSource");
			append_onvif_event_msg(E_KEY_CHANGE_CONFIGURATION, &config_ch, E_Changed, 0);

			tmp.result = 1;
			break;
		}
	}
	if (i == asource_cnt) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}
	memcpy(packet.data, &tmp, sizeof(arg_AudioSource));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetAudioOutput(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioOutput tmp;

	struct e_configuration_change_type config_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioOutput);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioOutput));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i;

	_TTY_LOG_ONVIF("Set Audio Output [%s]", tmp.token);

	memset(buff, 0x00, sizeof(buff));

	if (nf_sysdb_get_bool("audio.enable"))
	{
		if (tmp.token[0] != '\0')
		{
			if (tmp.send_primacy[0] != '\0')
			{
				if (!strcmp(tmp.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client"))
				{
					sprintf(buff, "onvif.aoutput0.send_primacy");
					nf_sysdb_set_str(buff, tmp.send_primacy);
				}
				else
				{
					return ONVIF_R_ERR_INVALID_ARG_VAL;
				}
			}

			if (tmp.output_level >= 0 && tmp.output_level <= 100)
			{
				sprintf(buff, "audio.out_volume");
				nf_sysdb_set_uint(buff, tmp.output_level);
			}
			else
			{
				return ONVIF_R_ERR_INVALID_ARG_VAL;
			}

			if (strcmp(tmp.token, "aoconfig0"))
			{
				return ONVIF_R_ERR_NO_CONFIG;
			}

			sprintf(buff, "onvif.aoutput0.name");
			nf_sysdb_set_str(buff, tmp.name);

			sprintf(buff, "onvif.aoutput0.output_token");
			nf_sysdb_set_str(buff, tmp.conf_token);

			sprintf(buff, "onvif.aoutput0.use_count");
			nf_sysdb_set_uint(buff, tmp.use_count);

			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			sysdb_save_cate(NF_SYSDB_CATE_AUDIO);

			memset(&config_ch, 0x00, sizeof(config_ch));
			strcpy(config_ch.token, tmp.token);
			strcpy(config_ch.type, "AudioOutput");
			append_onvif_event_msg(E_KEY_CHANGE_CONFIGURATION, &config_ch, E_Changed, 0);

		}
	}
	else
	{
		return ONVIF_R_ERR_NOT_SUPPORT;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioOutput));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioOutput)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetAudioDecoder(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioDecoder tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioDecoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioDecoder));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i;

	_TTY_LOG_ONVIF("Set Audio Output [%s]", tmp.token);

	memset(buff, 0x00, sizeof(buff));

	if (nf_sysdb_get_bool("audio.enable"))
	{
		if (tmp.token[0] != '\0')
		{
			if (strcmp(tmp.token, "ad0"))
			{
				return ONVIF_R_ERR_NO_CONFIG;
			}
		}
		if ( tmp.name[0] != '\0' )
		{
			sprintf(buff, "onvif.adecoder0.name");
			nf_sysdb_set_str(buff, tmp.name);
		}
	}
	else
	{
		return ONVIF_R_ERR_NOT_SUPPORT;
	}

	

	memcpy(packet.data, &tmp, sizeof(arg_AudioDecoder));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioDecoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetAudioEncoder(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoder tmp;

	struct e_configuration_change_type config_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoder));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i, ch, stream;
	int aencoder_cnt = ONVIF_AENCODER_CNT;
	/*
	 if( !(isUseAudio() && is_EnableAudio())) {
	 tmp.result = -10;
	 return ;
	 }
	 if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE) { // second is NONE
	 aencoder_cnt = 1;
	 }
	 */
	for (i = 0; i < aencoder_cnt; i++) {		
		if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
			continue;
		}
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.token, tmp_token)) {
			if (tmp.encoding != 0) {
				tmp.result = -1;
				break;
			}
			if (tmp.bitrate != 64) {
				tmp.result = -1;
				break;
			}
			if (tmp.s_rate != 8) {
				tmp.result = -1;
				break;
			}
			sprintf(buff, "onvif.aencoder%d.encoding", i);
			nf_sysdb_set_uint(buff, tmp.encoding);
			sprintf(buff, "onvif.aencoder%d.bitrate", i);
			nf_sysdb_set_uint(buff, tmp.bitrate);
			sprintf(buff, "onvif.aencoder%d.s_rate", i);
			nf_sysdb_set_uint(buff, tmp.s_rate);
			sprintf(buff, "onvif.aencoder%d.name", i);
			nf_sysdb_set_str(buff, tmp.name);

			set_mcast_addr_from_rtp(&tmp.mcast, ch, stream, ONVIF_MULTICAST_AUDIO);
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_NET);

			if (tmp.save_flag == 1) {
				nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			}

			memset(&config_ch, 0x00, sizeof(config_ch));
			strcpy(config_ch.token, tmp.token);
			strcpy(config_ch.type, "AudioEncoder");
			append_onvif_event_msg(E_KEY_CHANGE_CONFIGURATION, &config_ch, E_Changed, 0);

			tmp.result = 1;
			break;
		}
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoder));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AudioEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

static void _getAudioEncoderConfiguration(arg_AudioEncoder *tmp)
{
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int aencoder_cnt = ONVIF_AENCODER_CNT;
	int i, ch, stream;

	for (i = 0; i < aencoder_cnt; i++) {
		if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
			continue;
		}

		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp->token, tmp_token)) {
			tmp->enable = is_EnableAudio(i);
			/* Name */
			sprintf(buff, "onvif.aencoder%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			get_mcast_addr_from_rtp(&tmp->mcast, ch, stream, ONVIF_MULTICAST_AUDIO);

			sprintf(buff, "onvif.aencoder%d.timeout", i);
			tmp->timeout = nf_sysdb_get_uint(buff);

			tmp->use_count = get_aencoder_usecount(tmp->token);

			sprintf(buff, "onvif.aencoder%d.encoding", i);
			tmp->encoding = nf_sysdb_get_uint(buff);

			sprintf(buff, "onvif.aencoder%d.bitrate", i);
			tmp->bitrate = nf_sysdb_get_uint(buff);

			sprintf(buff, "onvif.aencoder%d.s_rate", i);
			tmp->s_rate = nf_sysdb_get_uint(buff);
			
			tmp->result = 1;
			break;
		}
	}
	// FIX ME::
	//if (!(is_EnableAudio(i))) {
	//if( !(isUseAudio() && is_EnableAudio()) ){
	//	tmp.result = -10;
	//	return;
	//}
	if (i == aencoder_cnt) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->result = 0;
		//fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n", __FUNCTION__, __LINE__);
	}
}
int onvif_GetAudioEncoderConfiguration(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoder tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	_TTY_LOG_ONVIF_DEBUG("buff_rcv: %s (sizeof : %d)", buff_rcv, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));
	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoder));
	// To DO
	_getAudioEncoderConfiguration(&tmp);
	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}
	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoder));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AudioEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioEncoderConfigurations(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoders));

	// To DO
	int i, cnt =0;
	char buff[COMMON_SIZE];

	for (i = 0; i < ONVIF_AENCODER_CNT; i++) {
		if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
			continue;
		}		
		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp.aencoder[cnt].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_getAudioEncoderConfiguration(&tmp.aencoder[cnt]);
		cnt++;
	}
	tmp.aencoder_cnt = cnt;

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AudioEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioEncoderConfigurations2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoders));

	// To DO
	int i = 0, j = 0, cnt = 0, index = 0;
	int ae0_flag = 0;
	char buff[COMMON_SIZE];
	char tmp_token[COMMON_SIZE] = { 0, };


	memset(buff, 0x00, sizeof(buff));

	if (tmp.token[0] == '\0' && tmp.aencoder[0].token[0] == '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));

		for (i = 0; i < ONVIF_AENCODER_CNT; i++)
		{
			sprintf(buff, "onvif.aencoder%d.token", i);
			strncpy(tmp.aencoder[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			_getAudioEncoderConfiguration(&tmp.aencoder[i]);
		}

		tmp.aencoder_cnt = i;
	}
	else
	{
		if (tmp.token[0] != '\0')
		{
			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
			{
				sprintf(buff, "onvif.profile%d.token", i);
				strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (!strcmp(tmp.token, tmp_token))
				{
					sprintf(buff, "onvif.profile%d.aencoder", i);
					strncpy(tmp.aencoder[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

					if (tmp.aencoder[0].token[0] == '\0')
					{
						int index = 0;

						for (index = 0; index < ONVIF_AENCODER_CNT; index++)
						{
							sprintf(buff, "onvif.aencoder%d.token", index);
							strncpy(tmp.aencoder[index].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

							_getAudioEncoderConfiguration(&tmp.aencoder[index]);
						}

						tmp.aencoder_cnt = index;

						break;
					}
					else
					{
						_getAudioEncoderConfiguration(&tmp.aencoder[0]);

						tmp.aencoder_cnt = 1;
						break;
					}
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT)
			{
				return ONVIF_R_ERR_NO_PROFILE;
			}
		}
		else if (tmp.aencoder[0].token[0] != '\0')
		{
			for (i = 0; i < ONVIF_AENCODER_CNT; i++)
			{
				sprintf(buff, "onvif.aencoder%d.token", i);
				strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (!strcmp(tmp.aencoder[0].token, tmp_token))
				{
					_getAudioEncoderConfiguration(&tmp.aencoder[0]);

					tmp.aencoder_cnt = 1;
					break;
				}
			}

			if (i == ONVIF_AENCODER_CNT)
			{
				return ONVIF_R_ERR_NO_CONFIG;
			}
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AudioEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioEncoderConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoders));

	int i = 0;
	char buff[COMMON_SIZE];
	char tmp_token[COMMON_SIZE] = { 0, };

	// To DO

	if (tmp.aencoder[0].token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));

		for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp_token, tmp.aencoder[0].token))
			{
				tmp.aencoder[0].enable = 1;
				
				break;
			}
		}

		if (i == ONVIF_MAX_PROFILE_CNT)
		{
			return ONVIF_R_ERR_NO_PROFILE;
		}
	}
	else if (tmp.aencoder[0].conf_token[0] != '\0')
	{
		for (i = 0; i < ONVIF_AENCODER_CNT; i++)
		{
			memset(tmp_token, 0x00, sizeof(buff));
			
			sprintf(buff, "onvif.aencoder%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp.aencoder[0].conf_token, tmp_token))
			{
				tmp.aencoder[0].enable = 1;
				
				break;
			}
		}

		if (i == ONVIF_AENCODER_CNT)
		{
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_AudioEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int GetAudioSourceConfiguration(arg_AudioSource *tmp) {
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int i;
	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {

		sprintf(buff, "onvif.profile%d.asource", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp->token, tmp_token)) {
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
				continue;
			}
			sprintf(buff, "onvif.asource%d.source_token", i);
			strncpy(tmp->s_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.asource%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			tmp->use_count = get_asource_usecount(tmp->token);

			tmp->result = 1;
			break;
		}
	}
	if (i == MAX_PROFILE) {
		tmp->result = 0;
	}
}

int onvif_GetAudioSourceConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSource tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSource);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSource));

	// To DO
	GetAudioSourceConfiguration(&tmp);

	if (tmp.result == 0 ) {
		return ONVIF_R_ERR;
	}
	else if (tmp.result == -10 ) {
		return ONVIF_R_ERR_NOT_SUPPORT;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSource));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_AudioSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioSourceConfigurations(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSources));

	// To DO
	int i=0, cnt = 0;
	char buff[COMMON_SIZE];

	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {
		if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) ){
			sprintf(buff, "onvif.asource%d.token", i);
			strncpy(tmp.asource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			GetAudioSourceConfiguration(&tmp.asource[i]);
			cnt++;
		}		
	}
	
	if( cnt == 0 ){
		return ONVIF_R_ERR_NOT_SUPPORT;
	}
	
	tmp.asource_cnt = cnt;	

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioSourceConfigurations2(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSources));

	// To DO
	int i=0, j=0, cnt = 0;
	int is_config_token = 0, is_prof_token = 0;
	char buff[COMMON_SIZE], configToken[COMMON_SIZE] = { 0, }, profileToken[COMMON_SIZE] = { 0, };

	tmp.asource_cnt = ONVIF_ASOURCE_CNT;

	memset(buff, 0x00, sizeof(buff));
	
	if (tmp.asource[0].token != NULL && tmp.asource[0].token[0] != '\0')		// If ConfigurationToken was provided
	{
		memset(configToken, 0x00, sizeof(configToken));

		for (i = 0; i < ONVIF_ASOURCE_CNT; i++)
		{
			sprintf(buff, "onvif.asource%d.token", i);
			strncpy(configToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (!strcmp(tmp.asource[0].token, configToken))
			{
				is_config_token = 1;
				break;
			}
		}

		if (i == ONVIF_ASOURCE_CNT)
			return ONVIF_R_ERR_NO_CONFIG;
	}
	else if (tmp.token != NULL && tmp.token[0] != '\0')			// If ProfileToken was provided
	{
		memset(profileToken, 0x00, sizeof(profileToken));

		for (j = 0; j < MAX_PROFILE; j++)
		{
			sprintf(buff, "onvif.profile%d.token", j);
			strncpy(profileToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (!strcmp(tmp.token, profileToken))
			{
				is_prof_token = 1;
				break;
			}
		}

		if (j == MAX_PROFILE)
			return ONVIF_R_ERR_NO_PROFILE;
	}
	
	if (is_config_token == 0 && is_prof_token == 0)			// If ConfigurationToken and ProfileToken wasn't provided
	{
		for (i = 0; i < ONVIF_ASOURCE_CNT; i++)
		{
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) )
			{
				sprintf(buff, "onvif.asource%d.token", i);
				strncpy(tmp.asource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				GetAudioSourceConfiguration(&tmp.asource[i]);
				cnt++;
			}
		}
	}
	else
	{
		if (is_config_token == 0)		// If only ProfileToken was provided
		{
			sprintf(buff, "onvif.profile%d.asource", j);
			strncpy(tmp.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (tmp.token[0] != '\0')
			{
				// for (i = 0; i < ONVIF_ASOURCE_CNT; i++)
				// {
					if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) )
					{
						sprintf(buff, "onvif.asource%d.token", j % ONVIF_CH);
						strncpy(tmp.asource[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						GetAudioSourceConfiguration(&tmp.asource[0]);
						cnt = 1;
					}
				// }
			}
			else
			{
				for (i = 0; i < ONVIF_ASOURCE_CNT; i++)
				{
					if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) )
					{
						sprintf(buff, "onvif.asource%d.token", i);
						strncpy(tmp.asource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						GetAudioSourceConfiguration(&tmp.asource[i]);
						cnt =1 ;
					}
				}
			}
		}
		else				// If only ConfigurationToken was provided or Configuration & ProfileToken was provided
		{
			memset(buff, 0x00, sizeof(buff));
			sprintf(buff, "asconfig%d", i);

			if (!strcmp(tmp.asource[0].token, buff))
			{
				sprintf(buff, "onvif.asource%d.token", i);
				strncpy(tmp.asource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				for (i = 0; i < ONVIF_ASOURCE_CNT; i++)
				{
					if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) )
					{
						GetAudioSourceConfiguration(&tmp.asource[i]);
						cnt = 1;
					}
				}
			}
			else
			{
				return ONVIF_R_ERR_NO_SOURCE;
			}
		}
	}

	if( cnt == 0 ){
		return ONVIF_R_ERR_NOT_SUPPORT;
	}
	
	tmp.asource_cnt = cnt;	

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioSourceOption(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSourceOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSourceOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSourceOption));

	// To DO
	int i, cnt, start, end, r_index = 0;
	int capa = 0;
	char asource_token[COMMON_SIZE] = { 0, };
	char config_token[COMMON_SIZE] = { 0, };
	char profile_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int asource_cnt = ONVIF_ASOURCE_CNT;

	// if no options specified, Generic option shall be specified
	if (tmp.profile_token[0] == '\0' && tmp.config_token[0] == '\0') {
		snprintf(tmp.astoken[0], COMMON_SIZE - 1, "as0");
		tmp.astoken_cnt = 1;
		tmp.result = 1;
	} 
	else {
		if (tmp.profile_token[0] != '\0')
		{
			tmp.astoken_cnt = 0;

			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
			{
				if( OV_is_EnableAudio(i) ){
					sprintf(buff, "onvif.profile%d.token", i);
					strncpy(profile_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);


					if (!strcmp(tmp.profile_token, profile_token))
					{
						sprintf(buff, "onvif.profile%d.asource", i);
						strncpy(tmp.config_token, nf_sysdb_get_str_nocopy(buff), 	COMMON_SIZE - 1);
						//sprintf(buff, "onvif.asource%d.source_token", i);
						//strncpy(tmp.astoken[0], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
//						tmp.astoken_cnt = 1;
//						tmp.result = 1;
						break;
					}
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT)
			{
				tmp.result = ONVIF_R_ERR_NO_CONFIG;
				//return ONVIF_R_ERR_NO_PROFILE;
				return ONVIF_R_ERR;
			}

			printf("###profile[%s] conf[%s]\n", tmp.profile_token, tmp.config_token);
			for (i = 0; i < asource_cnt; i++) {
				if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) ){
					sprintf(buff, "onvif.asource%d.token", i);
					strncpy(asource_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
					if (!strcmp(tmp.config_token, asource_token)) {
						//if ( i == 0 ) { /*main_vencoder*/
//						tmp.astoken_cnt = 0;
						sprintf(buff, "onvif.asource%d.source_token", i);
						strncpy(tmp.astoken[tmp.astoken_cnt], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
						tmp.astoken_cnt++;
						/*	If the second codec is not MJPEG, the first codec can be MJPEG */
						tmp.result = 1;
						break;
					}
				}
			}

			// end of video encoder
			if (i == asource_cnt) {
				return ONVIF_R_ERR_NO_CONFIG;
			}
		}
		else if (tmp.config_token[0] != '\0') {
			for (i = 0; i < asource_cnt; i++)
			{
				if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) ){
					sprintf(buff, "onvif.asource%d.token", i);
					strncpy(asource_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

					if (!strcmp(tmp.config_token, asource_token))
					{
						sprintf(buff, "onvif.asource%d.source_token", i);
						strncpy(tmp.astoken[0], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

						tmp.astoken_cnt = 1;
						tmp.result = 1;
						break;
					}
				}
			}

			if (i == asource_cnt)
			{
				return ONVIF_R_ERR_NO_CONFIG;
			}
		}
	}

#if 0

			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
				if( OV_is_EnableAudio(i) ){
					//baek.debug
					sprintf(buff, "onvif.profile%d.token", i);
					strncpy(profile_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
					if (!strcmp(tmp.profile_token, profile_token)) {
						sprintf(buff, "onvif.profile%d.asource", i);
						strncpy(tmp.config_token, nf_sysdb_get_str_nocopy(buff), 	COMMON_SIZE - 1);
						break;
					}
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT) {
				tmp.result = ONVIF_R_ERR_NO_CONFIG;
				return ONVIF_R_ERR;
			}
		}
		printf("###profile[%s] conf[%s]\n", tmp.profile_token, tmp.config_token);
		for (i = 0; i < asource_cnt; i++) {
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) ){
				sprintf(buff, "onvif.asource%d.token", i);
				strncpy(asource_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (!strcmp(tmp.config_token, asource_token)) {
					//if ( i == 0 ) { /*main_vencoder*/
					tmp.astoken_cnt = 0;
					sprintf(buff, "onvif.asource%d.source_token", i);
					strncpy(tmp.astoken[tmp.astoken_cnt], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
					tmp.astoken_cnt++;
					/*	If the second codec is not MJPEG, the first codec can be MJPEG */
					tmp.result = 1;
					break;
				}
			}
		}
		// end of video encoder
		if (i == asource_cnt) {
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}
#endif

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSourceOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_AudioSourceOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetCompatibleVideoSource(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSources));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	//g_message("%s IN", __FUNCTION__);
	int i, profile_cnt;
	unsigned int tmp_fps;

	profile_cnt = ONVIF_MAX_PROFILE_CNT;

	if (profile_cnt == 0) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	/* We have two fixed profiles that can have fixed configurations */
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			/* We have only a video source */
			tmp.vsource_cnt =1;

			sprintf(buff, "onvif.vsource0.token");
			strncpy(tmp.vsource[0].token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);

			sprintf(buff, "onvif.vsource0.name");
			strncpy(tmp.vsource[0].name, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);

			sprintf(buff, "onvif.vsource0.source_token");
			strncpy(tmp.vsource[0].s_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);

			//video source fps is first stream's
			tmp_fps = nf_sysdb_get_uint("camera.S0.fps");
			tmp.vsource[0].fps = get_fps_from_index(tmp_fps);

			sprintf(buff, "onvif.vsource0.bound_x");
			tmp.vsource[0].x = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource0.bound_y");
			tmp.vsource[0].y = nf_sysdb_get_uint(buff);

			sprintf(buff, "onvif.vsource0.bound_width");
			tmp.vsource[0].width = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource0.bound_height");
			tmp.vsource[0].height = nf_sysdb_get_uint(buff);

			tmp.vsource[0].use_count = get_vsource_usecount(
					tmp.vsource[0].token);
			tmp.result = 1;
			break;
		}
	}

	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n",
				__FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}
	memcpy(packet.data, &tmp, sizeof(arg_VideoSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoSourceOption(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSourceOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSourceOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSourceOption));

	// To DO
	int i, cnt, start, end, r_index = 0;
	int capa = 0;
	char vsource_token[COMMON_SIZE] = { 0, };
	char config_token[COMMON_SIZE] = { 0, };
	char profile_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;

	// if no options specified, Generic option shall be specified
	if (tmp.profile_token[0] == '\0' && tmp.config_token[0] == '\0') {
		tmp.vstoken_cnt = 0;
		for (i = 0; i < vsource_cnt; i++) {
			sprintf(buff, "onvif.vsource%d.token", i);
			strncpy(vsource_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

			sprintf(buff, "onvif.vsource%d.source_token", i);
			strncpy(tmp.vstoken[tmp.vstoken_cnt], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			tmp.vstoken_cnt++;
		}
			tmp.x_min = ONVIF_VSOURCE_X_MIN;
			tmp.x_max = ONVIF_VSOURCE_X_MAX;
			tmp.y_min = ONVIF_VSOURCE_Y_MIN;
			tmp.y_max = ONVIF_VSOURCE_Y_MAX;
			tmp.width_min = ONVIF_VSOURCE_WIDTH_MIN;
			tmp.width_max = ONVIF_VSOURCE_WIDTH_MAX;
			tmp.height_min = ONVIF_VSOURCE_HEIGHT_MIN;
			tmp.height_max = ONVIF_VSOURCE_HEIGHT_MAX;

			tmp.result = 1;
	}
	else {
		// find encoder and validate it
		if (tmp.config_token[0] == '\0') {
			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
				sprintf(buff, "onvif.profile%d.token", i);
				strncpy(profile_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				if (!strcmp(tmp.profile_token, profile_token)) {
					sprintf(buff, "onvif.profile%d.vsource", i);
					strncpy(tmp.config_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
					break;
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT) {
				return ONVIF_R_ERR;
			}
		}
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < vsource_cnt; i++) {
			sprintf(buff, "onvif.vsource%d.token", i);
			strncpy(vsource_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			if (!strcmp(tmp.config_token, vsource_token)) {
				tmp.vstoken_cnt = 0;
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);

				sprintf(buff, "onvif.vsource%d.source_token", i);
				strncpy(tmp.vstoken[tmp.vstoken_cnt],
						nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				tmp.vstoken_cnt++;

				tmp.x_min = ONVIF_VSOURCE_X_MIN;
				tmp.x_max = ONVIF_VSOURCE_X_MAX;
				tmp.y_min = ONVIF_VSOURCE_Y_MIN;
				tmp.y_max = ONVIF_VSOURCE_Y_MAX;
				tmp.width_min = ONVIF_VSOURCE_WIDTH_MIN;
				tmp.width_max = ONVIF_VSOURCE_WIDTH_MAX;
				tmp.height_min = ONVIF_VSOURCE_HEIGHT_MIN;
				tmp.height_max = ONVIF_VSOURCE_HEIGHT_MAX;

				tmp.result = 1;
				break;
			}
		}
		// end of video encoder
		if (i == vsource_cnt) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", _FUNCTION__, __LINE__);
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}

	if (tmp.result != 1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoSourceOption));
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_VideoSourceOption))!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	return ONVIF_ERR_RET_SUCCESS;

}


int onvif_GetVideoSources(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoSources));
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	// To DO
	char buff[COMMON_SIZE] = { 0, };
	int i;
	char tmp_fps[COMMON_SIZE] = { 0, };
	int vsource_cnt = 0;
	
	vsource_cnt = ONVIF_VSOURCE_CNT;
	tmp.vsource_cnt = vsource_cnt;

	if (tmp.vsource_cnt == 0) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	_TTY_LOG_ONVIF_DEBUG("tmp.vsource_cnt: %d ", tmp.vsource_cnt);
	strncpy(tmp_fps, nf_sysdb_get_str_nocopy("rec.netstream.fps"), COMMON_SIZE - 1);
	for (i = 0; i < tmp.vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_getVideoSourceConfiguration(&tmp.vsource[i]);
	}

	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_VideoSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_VideoSources)) != ONVIF_ERR_RET_SUCCESS) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioSources(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSources));

	// To DO
	int i=0, cnt = 0;
	char buff[COMMON_SIZE];

	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {
		if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) ){
			sprintf(buff, "onvif.asource%d.token", i);
			strncpy(tmp.asource[cnt].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			GetAudioSourceConfiguration(&tmp.asource[cnt]);
			cnt++;
		}
	}
	
	if( cnt == 0 ){
		return ONVIF_R_ERR_NOT_SUPPORT;
	}

	tmp.asource_cnt = cnt;

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetCompatibleAudioEncoder(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoders));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int profile_cnt;
	int i;

	profile_cnt = ONVIF_MAX_PROFILE_CNT;

	/* We have two fixed profiles that can have fixed configurations */
	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {			
			tmp.aencoder_cnt = 0;
			if (i >= ONVIF_FIXED_PROFILE_CNT){
				sprintf(buff, "onvif.aencoder0.token");
			}
			else{
				if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
					continue;
				}				
				sprintf(buff, "onvif.aencoder%d.token", i);
			}
			strncpy(tmp.aencoder[tmp.aencoder_cnt].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_getAudioEncoderConfiguration(&tmp.aencoder[tmp.aencoder_cnt]);
			tmp.aencoder_cnt++;
			break;
		}
	}

	if (i == profile_cnt) {
		tmp.result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}
	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetCompatibleAudioSource(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioSources tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioSources);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioSources));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	
	int i, profile_cnt;
	//unsigned int tmp_fps;

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");

	if (profile_cnt == 0) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	/* We have two fixed profiles that can have fixed configurations */
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			if( is_EnableAudio(i%ONVIF_ASOURCE_CNT) == 0){
				continue;
			}
			
			tmp.asource_cnt = 0;
			if (i >= ONVIF_FIXED_PROFILE_CNT)
				sprintf(buff, "onvif.asource0.token");
			else
				sprintf(buff, "onvif.asource%d.token", i%ONVIF_ASOURCE_CNT);

			strncpy(tmp.asource[tmp.asource_cnt].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			GetAudioSourceConfiguration(&tmp.asource[tmp.asource_cnt]);
			tmp.asource_cnt++;
			tmp.result = 1;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetCompatibleVideoEncoder(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_VideoEncoders tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_VideoEncoders);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_VideoEncoders));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int profile_cnt;
	int i;

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	if (profile_cnt == 0) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	/* We have two fixed profiles that can have fixed configurations */
	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {

			tmp.vencoder_cnt = 0;

			if (i >= ONVIF_FIXED_PROFILE_CNT)
				snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder0.token");
			else
				snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", i);

			if ( !strcmp(nf_sysdb_get_str_nocopy(buff), "\0") )
			{
				tmp.vencoder_cnt = 0;
				for ( i = 0 ; i < ONVIF_VENCODER_CNT; i++)
				{
					snprintf(buff, COMMON_SIZE - 1, "onvif.vencoder%d.token", i);
					strncpy(tmp.vencoder[tmp.vencoder_cnt].token,
						nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
					get_vencoder_table(&tmp.vencoder[tmp.vencoder_cnt]);
					if ( tmp.vencoder[tmp.vencoder_cnt].codec == ONVIF_VIDEO_CODEC_H265 )
						tmp.vencoder[tmp.vencoder_cnt].codec = ONVIF_VIDEO_CODEC_H264;
				
					tmp.vencoder_cnt++;
				}
				break;
			}
			else
			{
				tmp.vencoder_cnt = 0;
			strncpy(tmp.vencoder[tmp.vencoder_cnt].token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				
			get_vencoder_table(&tmp.vencoder[tmp.vencoder_cnt]);
				if ( tmp.vencoder[tmp.vencoder_cnt].codec == ONVIF_VIDEO_CODEC_H265 )
					tmp.vencoder[tmp.vencoder_cnt].codec = ONVIF_VIDEO_CODEC_H264;
				
			tmp.vencoder_cnt++;
			break;
		}
	}
	}

	if (i == profile_cnt) {
		tmp.result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n",
				__FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}
	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetMetadataConfigurations(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Metadatas tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Metadatas);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Metadatas));

	// To DO

	int i = 0;

	char buff[COMMON_SIZE] = { 0, };

	tmp.metadata_cnt = ONVIF_METADATA_CNT;

	for (i = 0; i < tmp.metadata_cnt; i++) {
		sprintf(buff, "onvif.metadata%d.token", i);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		strncpy(tmp.metadata[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		if (_getMetadataConfiguration(&tmp.metadata[i]) == ONVIF_R_ERR)
			return ONVIF_R_ERR;

	}

	tmp.result = 1;
	_TTY_LOG_ONVIF_DEBUG("End of metadata Entry: %s (Line : %d)",
			__FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_Metadatas));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Metadatas)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}


int onvif_GetCompatibleMetadataConfigurations(NfOnvif *self, int fd,
		char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Metadatas tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Metadatas);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Metadatas));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int profile_cnt;
	int i;

	profile_cnt = MAX_PROFILE;
	if (profile_cnt == 0) {
		return ONVIF_R_ERR;
	}

	/* We have two fixed profiles that can have fixed configurations */
	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {

			tmp.metadata_cnt = 0;

			if (i >= ONVIF_FIXED_PROFILE_CNT)
				sprintf(buff, "onvif.metadata0.token");
			else
				sprintf(buff, "onvif.metadata%d.token", i);

			strncpy(tmp.metadata[tmp.metadata_cnt].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_getMetadataConfiguration(&tmp.metadata[tmp.metadata_cnt]);
			tmp.metadata_cnt++;

			break;
		}
	}

	if (i == profile_cnt) {
		tmp.result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n",
				__FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}

	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_Metadatas));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Metadatas)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));

	// To DO

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i, profile_cnt;

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//	if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//		continue;
		//	}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			sprintf(buff, "onvif.profile%d.metadata", i);
			nf_sysdb_set_str(buff, "\0");
			break;
		}
	}

	if (i == profile_cnt) {
		return ONVIF_R_ERR;
	}
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_AddMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AddConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AddConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AddConfig));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int i = 0, meta_index, profile_index;

	tmp.result = 1;

	for (i = 0; i < ONVIF_METADATA_CNT; i++) {
		//	if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//		continue;
		//	}
		sprintf(buff, "onvif.metadata%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.config_token, tmp_token)) {
			meta_index = i;
			break;
		}
	}
	if (i == ONVIF_METADATA_CNT) {
		return ONVIF_R_ERR;
	}

	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.profile_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		return ONVIF_R_ERR;
	}
/*
	if (profile_index >= ONVIF_FIXED_PROFILE_CNT) {
		if (meta_index != 0)
			tmp.result = -2;
	} else if (profile_index < ONVIF_FIXED_PROFILE_CNT) {
		if (meta_index != profile_index)
			tmp.result = -2;
	}
*/
	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.metadata", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Metadata tmp;

	struct e_configuration_change_type config_ch;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Metadata);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Metadata));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i = 0;

	char buff[COMMON_SIZE];
	char tmp_token[COMMON_SIZE];
	int metadata_cnt, ch, stream;

	metadata_cnt = ONVIF_METADATA_CNT;

	//	if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE) { // second is NONE
	//		metadata_cnt = 1;
	//	}
	for (i = 0; i < metadata_cnt; i++) {
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.metadata%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			if(tmp.name != NULL && tmp.name != '\0'){
				sprintf(buff, "onvif.metadata%d.name", i);
				nf_sysdb_set_str(buff, tmp.name);
			}
			
			sprintf(buff, "onvif.metadata%d.ptz_status", i);
			nf_sysdb_set_bool(buff, tmp.ptz_status);
			sprintf(buff, "onvif.metadata%d.ptz_position", i);
			nf_sysdb_set_bool(buff, tmp.ptz_position);
			sprintf(buff, "onvif.metadata%d.event_filter", i);
			nf_sysdb_set_str(buff, tmp.event_filter);
			/*
			sprintf(buff, "onvif.metadata%d.event_policy", i);
			nf_sysdb_set_str(buff, tmp.event_policy);
			*/
			sprintf(buff, "onvif.metadata%d.analytics", i);
			nf_sysdb_set_bool(buff, tmp.analytics);

			set_mcast_addr_from_rtp(&tmp.mcast, ch, stream, ONVIF_MULTICAST_META);
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_NET);
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			tmp.result = 1;
			break;
		}
	}
	if (i == metadata_cnt) {
		return ONVIF_R_ERR;
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Metadata));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Metadata)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

    strcpy(config_ch.token, tmp.token);
    strcpy(config_ch.type, "Metadata");
    append_onvif_event_msg(E_KEY_CHANGE_CONFIGURATION, &config_ch, E_Changed, 0);

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetMetadataConfigurations2(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Metadatas2 tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Metadatas2);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Metadatas2));

	// To DO

	int i = 0, j = 0;
    int profile_cnt = 0;
	int is_config_token = 0, is_prof_token = 0;

	char buff[COMMON_SIZE] = { 0, };
    char tmp_token[COMMON_SIZE] = {0,};

    profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
    tmp.metadata_cnt = 0;

	memset(buff, 0x00, sizeof(buff));

	if (tmp.conf_token != NULL && tmp.conf_token[0] != '\0')
	{
        memset(tmp_token, 0x00, sizeof(tmp_token));
		
        for (i = 0; i < ONVIF_METADATA_CNT; i++)
		{
            sprintf(buff, "onvif.metadata%d.token", i);
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
            if (!strcmp(tmp.conf_token, tmp_token))
			{
				is_config_token = 1;
                break;
            }
        }
		if (i == ONVIF_METADATA_CNT)
			return ONVIF_R_ERR_NO_CONFIG;
    }
    else if (tmp.token != NULL && tmp.token[0] != '\0')
	{
        memset(tmp_token, 0x00, sizeof(tmp_token));
		
        for (j = 0; j < MAX_PROFILE; j++)
		{
            sprintf(buff, "onvif.profile%d.token", j);
            strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			
            if (!strcmp(tmp.token, tmp_token))
			{
				is_prof_token = 1;
                break;
            }
        }
    }

	if (is_config_token == 0 && is_prof_token == 0)
	{
		for (i = 0; i < ONVIF_METADATA_CNT; i++)
		{
			sprintf(buff, "onvif.metadata%d.token", i);
			strncpy(tmp.metadata[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (tmp.metadata[i].token[0] != '\0')
			{
				if (_getMetadataConfiguration2(&tmp.metadata[i]) == ONVIF_R_ERR)
				{
					return ONVIF_R_ERR;
				}
			}
			tmp.metadata_cnt++;
    	}   
		//if (!strcmp(tmp.conf_token, "m0"))	tmp.metadata_cnt = 1;
	}
	else
	{
		if (is_config_token == 0)
		{
			sprintf(buff, "onvif.profile%d.metadata", j);
			strncpy(tmp.metadata[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (tmp.metadata[0].token[0] != '\0')
			{
				if (_getMetadataConfiguration2(&tmp.metadata[0]) == ONVIF_R_ERR)
				{
    				return ONVIF_R_ERR;
    			}
    		}
			else
			{
				sprintf(buff, "onvif.metadata%d.token", 0);
				strncpy(tmp.metadata[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				if (_getMetadataConfiguration2(&tmp.metadata[0]) == ONVIF_R_ERR)
				{
					return ONVIF_R_ERR;
				}
			}

        	tmp.metadata_cnt = 1;
		}
		else
		{
			memset(buff, 0x00, sizeof(buff));
			sprintf(buff, "m%d", i);

			if (!strcmp(tmp.conf_token, buff))
			{
				sprintf(buff, "onvif.metadata%d.token", i);
        		strncpy(tmp.metadata[0].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				if (tmp.metadata[0].token[0] != '\0')
				{
        			if (_getMetadataConfiguration2(&tmp.metadata[0]) == ONVIF_R_ERR)
					{
            			return ONVIF_R_ERR;
    				}
				}
			}
			else
			{
				return ONVIF_R_ERR_NO_CONFIG;
			}

			tmp.metadata_cnt = 1;
		}
	}

	tmp.result = 1;
	_TTY_LOG_ONVIF_DEBUG("End of metadata Entry: %s (Line : %d)",
			__FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_Metadatas2));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Metadatas2)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

static int _getMetadataConfiguration(arg_Metadata* tmp)
{
	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int metadata_cnt = ONVIF_METADATA_CNT;
	int i, ch, stream;

	for (i = 0; i < metadata_cnt; i++) {
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.metadata%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp->token, tmp_token)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d) %s", __FUNCTION__, __LINE__, tmp_token);

			sprintf(buff, "onvif.metadata%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.name %s ", tmp->name);

			sprintf(buff, "onvif.metadata%d.ptz_status", i);
			tmp->ptz_status = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.ptz_status %d", tmp->ptz_status);

			sprintf(buff, "onvif.metadata%d.ptz_position", i);
			tmp->ptz_position = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.ptz_position : %d", tmp->ptz_position);

			sprintf(buff, "onvif.metadata%d.event_filter", i);
			strncpy(tmp->event_filter, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.event_filter : %s", tmp->event_filter);

			sprintf(buff, "onvif.metadata%d.event_policy", i);
			strncpy(tmp->event_policy, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.event_policy : %s", tmp->event_policy);

			sprintf(buff, "onvif.metadata%d.analytics", i);
			tmp->analytics = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.analytics : %d", tmp->analytics);

			get_mcast_addr_from_rtp(&tmp->mcast, ch, stream, ONVIF_MULTICAST_META);

			sprintf(buff, "onvif.metadata%d.timeout", i);
			tmp->timeout = nf_sysdb_get_uint(buff);
			tmp->use_count = get_metadata_usecount(tmp->token);
			tmp->result = 1;
			break;
		}
	}

	if (i == metadata_cnt)
		return ONVIF_R_ERR;
	else
		return ONVIF_ERR_RET_SUCCESS;

}

static int _getMetadataConfiguration2(arg_Metadata* tmp)
{
	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int metadata_cnt = ONVIF_METADATA_CNT;
	int i, ch, stream;

	for (i = 0; i < metadata_cnt; i++) {
		ch = i%ONVIF_CH;
		stream = i/ONVIF_CH;

		sprintf(buff, "onvif.metadata%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp->token, tmp_token)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d) %s", __FUNCTION__, __LINE__, tmp_token);

			sprintf(buff, "onvif.metadata%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.name %s ", tmp->name);

			sprintf(buff, "onvif.metadata%d.ptz_status", i);
			tmp->ptz_status = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.ptz_status %d", tmp->ptz_status);

			sprintf(buff, "onvif.metadata%d.ptz_position", i);
			tmp->ptz_position = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.ptz_position : %d", tmp->ptz_position);

			sprintf(buff, "onvif.metadata%d.event_filter", i);
			strncpy(tmp->event_filter, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.event_filter : %s", tmp->event_filter);

			sprintf(buff, "onvif.metadata%d.event_policy", i);
			strncpy(tmp->event_policy, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.event_policy : %s", tmp->event_policy);

			sprintf(buff, "onvif.metadata%d.analytics", i);
			tmp->analytics = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.analytics : %d", tmp->analytics);

			get_mcast_addr_from_rtp(&tmp->mcast, ch, stream, ONVIF_MULTICAST_META);

			sprintf(buff, "onvif.metadata%d.timeout", i);
			tmp->timeout = nf_sysdb_get_uint(buff);
			tmp->use_count = get_metadata_usecount(tmp->token);
			tmp->result = 1;
			break;
		}
	}

	if (i == metadata_cnt)
		return ONVIF_R_ERR;
	else
		return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv) {

	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Metadata tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Metadata);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Metadata));

	if (_getMetadataConfiguration(&tmp) == ONVIF_R_ERR)
		return ONVIF_R_ERR;

	memcpy(packet.data, &tmp, sizeof(arg_Metadata));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Metadata)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}
/****************************************************
 End of Media Service
 *****************************************************/
/****************************************************
 Imaging Service
 *****************************************************/
int onvif_GetImagingOption(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ImagingOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ImagingOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ImagingOption));

	// To DO
	/*
	 WEBBASE_CAM_COMPATIBILITY_IMAGE_PROFILE profile;
	 getImageProfile(ch,&profile);
	 tmp.brightness = profile.brightness.value;
	 tmp.color = profile.color.value;
	 tmp.contrast = profile.contrast.value;
	 tmp.sharpness = profile.sharpness.value;
	 tmp.result = 1;
	 */

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetImagingOption(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ImagingOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ImagingOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ImagingOption));

	// To DO

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetImagingSettings(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ImagingOption tmp;
	unsigned char temp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ImagingOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ImagingOption));

	//To Do
	char buff[COMMON_SIZE] = { 0, };
	int vs = isValidVideoSourceToken(tmp.token);
	if(vs < 0)
		return ONVIF_R_ERR_NO_SOURCE;

	arg_GetCapa capa;
	memset(&capa, 0x00, sizeof(arg_GetCapa));
	
	GetCapability(&capa, vs);
	capa.ch = vs;
	vs %= ONVIF_CH;

	/* -1 means that the client did not select the value. */
	if(
	((tmp.brightness   != -1) && (tmp.brightness < capa.brightnessMin[vs] || tmp.brightness > capa.brightnessMax[vs]))
	|| ((tmp.color        != -1) && (tmp.color < capa.colorMin[vs] || tmp.color > capa.colorMax[vs]))
	|| ((tmp.contrast     != -1) && (tmp.contrast < capa.contrastMin[vs] || tmp.contrast > capa.contrastMax[vs]))
	|| ((tmp.sharpness    != -1) && (tmp.sharpness < capa.sharpnessMin[vs] || tmp.sharpness > capa.sharpnessMax[vs]))
	|| ((tmp.exposureMode != -1) && (tmp.exposureMode < 0 || tmp.exposureMode > 1))
	|| ((tmp.gain         != -1) && (tmp.gain < capa.gainMin[vs] || tmp.gain > capa.gainMax[vs]))
//	|| (tmp.iris         != -1 && (tmp.iris < capa.irisMin || tmp.iris > capa.irisMax))
	|| ((tmp.blcMode      != -1) && (tmp.blcMode < 0 || tmp.blcMode > 1 || !capa.blcSupport[vs]))
	|| ((tmp.dnnMode      != -1) && (tmp.dnnMode < 0 || tmp.dnnMode > 2 || !capa.dnnSupport[vs]))
	|| ((tmp.wdrMode      != -1) && (tmp.wdrMode < 0 || tmp.wdrMode > 1 || !capa.wdrSupport[vs]))
	|| ((tmp.wbMode       != -1) && (tmp.wbMode < 0  || tmp.wbMode > 1 || !capa.wbSupport[vs]))
	|| ((tmp.focusMode 	 != -1) && (tmp.focusMode < 0 || tmp.focusMode > 1 || !capa.focusSupport[vs])))
	{
		printf("\e[31m ######### INVLAID PARAM ############ \e[0m\n");
		return ONVIF_R_ERR_INVALID_PARAM;
	}
	
	if(tmp.brightness != -1)
	{
		sprintf(buff, "cam.C%d.bright", vs);
		nf_sysdb_set_uint(buff, tmp.brightness);
	}
	if(tmp.color != -1)
	{
		sprintf(buff, "cam.C%d.color", vs);
		nf_sysdb_set_uint(buff, tmp.color);
	}
	if(tmp.contrast != -1)
	{
		sprintf(buff, "cam.C%d.contrast", vs);
		nf_sysdb_set_uint(buff, tmp.contrast);
	}
	if(tmp.sharpness != -1)
	{
		sprintf(buff, "cam.C%d.sharpness", vs);
		nf_sysdb_set_uint(buff, tmp.sharpness);
	}
	if(tmp.focusMode != -1)
	{
		sprintf(buff, "cam.C%d.focus_mode", vs);
		nf_sysdb_set_uint(buff, tmp.focusMode);
	}
	if(tmp.defogMode != -1)
	{
		if(tmp.defogMode == 0)
		{
			sprintf(buff, "cam.C%d.defog", vs);
			nf_sysdb_set_uint(buff, tmp.defogMode);
		}
		else if(tmp.defogMode == 1)
		{
			sprintf(buff, "cam.C%d.defog", vs);
			nf_sysdb_set_uint(buff, tmp.defogMode);

			if(tmp.defogLevel >= 0 && tmp.defogLevel < 0.35)
			{
				sprintf(buff, "cam.C%d.defog", vs);
				nf_sysdb_set_uint(buff, 1);
			}
			if(tmp.defogLevel >= 0.35 && tmp.defogLevel < 0.7)
			{
				sprintf(buff, "cam.C%d.defog", vs);
				nf_sysdb_set_uint(buff, 2);
			}
			if(tmp.defogLevel >= 0.7 && tmp.defogLevel <= 1)
			{
				sprintf(buff, "cam.C%d.defog", vs);
				nf_sysdb_set_uint(buff, 3);
			}
			defog_level = tmp.defogLevel;
		}
		else
		{
			printf("\e[31m ### NOT INVALID DEFOG MODE[%d] ###\e[0m\n", tmp.defogMode);		
		}
	}
	
#if 0 // not supported on utm2
	if(tmp.exposureMode != -1)
	{
		/* convert onvif idx -> camera idx
		 * auto(0)->1, manual(1)->0
		 */
		temp = (unsigned char)!tmp.exposureMode;
		nf_sysdb_set_uint("cam.C0.exposure_mode", temp);

		exposure_mode = temp;
	}

	/* gain and iris are not supported on auto exposure */
	if(exposure_mode == 0)
	{
		if(tmp.gain != -1)
		{	
			nf_sysdb_set_uint("cam.C0.agc_gain", tmp.gain);
		}

		#if 0
		if(tmp.iris != -1)
		{
			//why?
			//if( tmp.iris == 0 ) tmp.iris = 1;
				
			temp = (unsigned char)tmp.iris;
			if (temp != cur_iris) {
				set_piris_position(tmp.iris);
			}
		}
		#endif
	}
	#if 0
	if(tmp.focusMode != -1)
	{
		temp = (unsigned char)tmp.focusMode;
		if (cam_ae_awb.v_focus_mode != temp) {
			ControlSystemData(CAMERA_AC_UPDATE_FOCUS_MODE, &temp, sizeof(temp));
			nf_sysdb_set_uint("camera.func.focus_mode", temp);
			cam_ae_awb.v_focus_mode = temp;
		}
	}
	#endif
	if(tmp.blcMode != -1)
	{
		nf_sysdb_set_uint("cam.C0.blc_control", tmp.blcMode);
	}
	if(tmp.dnnMode != -1)
	{
		nf_sysdb_set_uint("cam.C0.day_night_mode", tmp.dnnMode);
	}
	if(tmp.wdrMode != -1)
	{
		/* convert onvif idx -> camera idx
		 * on(1)->mid(2)
		 */
		temp = tmp.wdrMode ? 2 : 0;
		nf_sysdb_set_uint("cam.C0.wdr_mode", temp);
	}
	if(tmp.wbMode != -1)
	{
		/* convert onvif idx -> camera idx
		 * auto(0)->1, manual(1)->0
		 */
		temp = tmp.wbMode == 0 ? 1 : 0;
		nf_sysdb_set_uint("cam.C0.wb_mode", temp);

		if(tmp.wbMode)
		{
			nf_sysdb_set_uint("cam.C0.mwb_mode", 1); //indoor(1), outdoor(2)
		}
	}
#endif

nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_CAM);	
	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;	
}


int onvif_GetOptions(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_GetOption tmp;
	arg_GetCapa capa;

	memset(&capa, 0x00, sizeof(arg_GetCapa));

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetOption));

	// To DO
	int vs = isValidVideoSourceToken(tmp.token);
	if(vs < 0)
		return ONVIF_R_ERR_NO_SOURCE;
	
	if(onvif_is_ptz(vs, NF_IPCAM_IMAGE_FOCUS))
	{
		tmp.focus_continous_speed_max = 1;
		tmp.focus_continous_speed_min = -1;
	}
	else
	{
		tmp.focus_continous_speed_max = 0;
		tmp.focus_continous_speed_min = 0;
	}
	capa.ch = vs;
	GetCapability(&capa, vs);
	
	tmp.bright_max = capa.brightnessMin[vs];
	tmp.bright_min = capa.brightnessMax[vs];
	tmp.color_max = capa.colorMax[vs];
	tmp.color_min = capa.colorMin[vs];
	tmp.contrast_max = capa.contrastMax[vs];
	tmp.contrast_min = capa.contrastMin[vs];
	tmp.sharp_max = capa.sharpnessMax[vs];
	tmp.sharp_min = capa.sharpnessMin[vs];
	tmp.result = 1;

	tmp.op_af = 0;
	tmp.focus_near_min = capa.focusAbsoluteRangeMin[vs];
	tmp.focus_near_max = tmp.focus_near_min;
	tmp.focus_far_min = capa.focusAbsoluteRangeMax[vs];
	tmp.focus_far_max = tmp.focus_far_min;

	memcpy(packet.data, &tmp, sizeof(arg_GetOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_GetOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_Move(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_MoveOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_MoveOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_MoveOption));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	NF_PTZ_CMD ptz_cmd;

	int vs = isValidVideoSourceToken(tmp.token);
	if(vs < 0)
		return ONVIF_R_ERR_NO_SOURCE;

	if (tmp.move_type == FOCUS_CONTINUOUS) {
		if( tmp.token[0] != 'v' || tmp.token[1] != 's'){
			return ONVIF_R_ERR_NO_CONFIG;
		}
		ptz_cmd.ch = atoi(tmp.token+strlen("vs"));

		if( ptz_cmd.ch < 0 || ptz_cmd.ch > NUM_ACTIVE_CH ){
			return ONVIF_R_ERR_NO_CONFIG;
		}

		if(onvif_is_ptz(ptz_cmd.ch, NF_IPCAM_IMAGE_FOCUS))
		{
			if( tmp.speed < -1 || tmp.speed > 1){
				return ONVIF_R_ERR_INVALID_PARAM;
			}
			if (tmp.speed >= 0) {
				ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_FAR_CON;
			} else {
				ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_NEAR_CON;
			}	
		}
		if(tmp.speed != 0)
			return ONVIF_R_ERR_INVALID_ARG_VAL;

		else
		{
			
		}
		ptz_cmd.params[0] = 100 * abs(tmp.speed);
		pushPTZControlOperation(&ptz_cmd, CONTINUOUS);

		tmp.result = SUCCESS;
	} 
	/*
	else if (tmp.move_type == FOCUS_RELATIVE) {
		if( tmp.token[0] != 'v' || tmp.token[1] != 's'){
			return ONVIF_R_ERR_NO_CONFIG;
		}
		ptz_cmd.ch = atoi(tmp.token+strlen("vs"));
		if( ptz_cmd.ch < 0 || ptz_cmd.ch > NUM_ACTIVE_CH ){
			return ONVIF_R_ERR_NO_CONFIG;
		}
		
		if( tmp.position < ONVIF_MOV_DISTANCE_MIN || tmp.position > ONVIF_MOV_DISTANCE_MAX){
			return ONVIF_R_ERR_INVALID_PARAM;
		}
		if (tmp.position >= 0) {
			ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_FAR;
		} else {
			ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_NEAR;
		}
		ptz_cmd.params[0] = abs(tmp.position);
		pushPTZControlOperation(&ptz_cmd, LIMIT);

		tmp.result = SUCCESS;
	} 
	*/
	else {
		return ONVIF_R_ERR_INVALID_PARAM;
	}

	memcpy(packet.data, &tmp, sizeof(arg_MoveOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_MoveOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetImagingSetting(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ImagingOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ImagingOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ImagingOption));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;
	int result = 0;
	int i;

	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.token, tmp_token)) {
			sprintf(buff, "cam.C%d.bright", i);
			tmp.brightness = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.color", i);
			tmp.color = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.contrast", i);
			tmp.contrast = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.sharpness", i);
			tmp.sharpness = nf_sysdb_get_uint(buff);
			tmp.result = 1;
			break;
		}
	}
	if(tmp.result == 0)
	{
		return ONVIF_R_ERR_NO_SOURCE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetImagingSettings(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_ImagingOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_ImagingOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_ImagingOption));

	// To Do
	char buff[COMMON_SIZE] = { 0, };
	int vs = isValidVideoSourceToken(tmp.token);
	if(vs < 0)
		return ONVIF_R_ERR_NO_SOURCE;

#if 0 // not supprted on utm2
	tmp.exposureMode = !nf_sysdb_get_uint("cam.C0.exposure_mode");//0:MANUAL 1:AUTO
	tmp.gain = nf_sysdb_get_uint("cam.C0.agc_gain");//0~36dB
	tmp.iris = 0;
	tmp.blcMode = nf_sysdb_get_uint("cam.C0.blc_control") ? 1 : 0; //0:OFF 1:ADAPTIVE 2:ZONE-LOWER 3:ZONE-MIDDLE...
	tmp.wdrMode = nf_sysdb_get_uint("cam.C0.wdr_mode") ? 1 : 0;  //0:OFF 1:LOW 2:MID 3:HIGH
	tmp.dnnMode = nf_sysdb_get_uint("cam.C0.day_night_mode"); //0:DAY(ICR ON) 1:NIGHT(ICR OFF) 2:AUTO
	tmp.wbMode  = !nf_sysdb_get_uint("cam.C0.wb_mode"); //0:MANUAL 1:AUTO
#endif

	sprintf(buff, "cam.C%d.bright", vs);
	tmp.brightness = nf_sysdb_get_uint(buff);

	sprintf(buff, "cam.C%d.color", vs);
	tmp.color      = nf_sysdb_get_uint(buff);

	sprintf(buff, "cam.C%d.contrast", vs);
	tmp.contrast   = nf_sysdb_get_uint(buff);

	sprintf(buff, "cam.C%d.sharpness", vs);
	tmp.sharpness  = nf_sysdb_get_uint(buff);

	sprintf(buff, "cam.C%d.blc_control", vs);
	tmp.blcMode = nf_sysdb_get_uint(buff);

	sprintf(buff, "cam.C%d.defog", vs);
	tmp.defogMode = nf_sysdb_get_uint(buff);
	tmp.defogLevel = defog_level;

	tmp.focusMode = 0;
	tmp.iris = 0;

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetStatus(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_MoveStatus tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_MoveStatus);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_MoveStatus));

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;
	int result = 0;
	int i;
	
	memcpy(packet.data, &tmp, sizeof(arg_MoveStatus));
	
	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.token, tmp_token)) {
			tmp.position = 0;
			result = 1;
			break;
		}
	}
	if(result == 0){
		return ONVIF_R_ERR_NO_SOURCE;
	}

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_MoveStatus)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_FocusStop(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_FocusStop tmp;
	int ch =0;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_FocusStop);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_FocusStop));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF("Focus Stop ch : %s", tmp.VideoSourceToken);

	if( tmp.VideoSourceToken[0] != 'v' || tmp.VideoSourceToken[1] != 's'){
		return ONVIF_R_ERR_NO_CONFIG;
	}

	ch = atoi(tmp.VideoSourceToken+strlen("vs"));
	if( ch < 0 || ch > NUM_ACTIVE_CH ){
		return ONVIF_R_ERR_NO_CONFIG;
	}
	
	PTZOperationStop(ch, 1, 1);

	memcpy(packet.data, &tmp, sizeof(arg_FocusStop));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_FocusStop)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
/****************************************************
 End of Imaging Service
 *****************************************************/

/****************************************************
 PTZ Service
 *****************************************************/
#define ONVIF_PRESET_NAME_SIZE 16384
#define ONVIF_PRESET_TOKEN_SIZE 4096

void save_array_to_preset_db(arg_preset* preset) {
	char name[16384] = { 0, };
	char token[4096] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i = 0, j = 0;
	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		for (j = 0; j < ONVIF_PRESET_CNT; j++) {
			strcat(name, preset[i].presetName[j]);
			strcat(token, preset[i].presetToken[j]);
			if (j < ONVIF_PRESET_CNT - 1) {
				strcat(name, ",");
				strcat(token, ",");
			}
		}
		strcat(name, "|");
		strcat(token, "|");
	}
	sprintf(buff, "onvif.ptz_pre.name");
	nf_sysdb_set_str(buff, name);
	sprintf(buff, "onvif.ptz_pre.token");
	nf_sysdb_set_str(buff, token);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

}

void preset_name_array(arg_preset* preset) 
{
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char* token = NULL;
	char* sub_token = NULL;
	char* context = NULL;
	char* sub_context = NULL;

	char spliter[] = "|";
	char sub_spliter[] = ",";
	char chSpliter[] = ",";
	int i = 0, j = 0;
	char buff[COMMON_SIZE] = { 0, };
	char onvif_token[ONVIF_PRESET_TOKEN_SIZE] = { 0, };
	char onvif_name[ONVIF_PRESET_NAME_SIZE] = { 0, };

	// read from DB
	sprintf(buff, "onvif.ptz_pre.name");
	strncpy(onvif_name, nf_sysdb_get_str_nocopy(buff), ONVIF_PRESET_NAME_SIZE - 1);
	sprintf(buff, "onvif.ptz_pre.token");
	strncpy(onvif_token, nf_sysdb_get_str_nocopy(buff), ONVIF_PRESET_TOKEN_SIZE - 1);
	// name splitter
	token = strtok_r(onvif_name, spliter, &context);
	while (token != NULL) {

		// split sub token
		sub_token = strtok_r(token, sub_spliter, &sub_context);
		while (sub_token != NULL) {
			strncpy(preset[i].presetName[j], sub_token, COMMON_SIZE);
			sub_token = strtok_r(NULL, sub_spliter, &sub_context);
			j++;

		}
		j = 0;
		token = strtok_r(NULL, spliter, &context);
		i++;
	}
	// token splitter
	i = 0, j = 0;
	token = strtok_r(onvif_token, spliter, &context);
	while (token != NULL) {
		// split sub token
		sub_token = strtok_r(token, sub_spliter, &sub_context);
		while (sub_token != NULL) {
			strncpy(preset[i].presetToken[j], sub_token, ONVIF_TOKEN_SIZE);
			sub_token = strtok_r(NULL, sub_spliter, &sub_context);
			j++;

		}
		j = 0;
		token = strtok_r(NULL, spliter, &context);
		i++;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
}

int onvif_GotoHomePosition(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF("ProfileToken: %s ", tmp.ProfileToken);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int getPTZTokenNumber(char* token) {
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i, j;
	int ptz_cnt = ONVIF_PTZ_CNT;
	int profile_cnt;

	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	char ptz_tmp_token[COMMON_SIZE] = { 0, };
	char ptz_token[COMMON_SIZE] = { 0, };

	profile_cnt = ONVIF_MAX_PROFILE_CNT;
	_TTY_LOG_ONVIF_DEBUG("profile_cnt  %d", profile_cnt, __LINE__);

	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("token: %s (tmp_token : %s)", token, tmp_token);

		sprintf(buff, "onvif.profile%d.ptz", i);
		strncpy(ptz_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("ptz_token: %s ", ptz_token);

		if (!strcmp(token, tmp_token)) {
			for (j = 0; j < ptz_cnt; j++) {
				sprintf(buff, "onvif.ptz%d.token", j);
				strncpy(ptz_tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				_TTY_LOG_ONVIF_DEBUG("ptz_token: %s (tmp_token : %s)", ptz_token, ptz_tmp_token);
				if (!strcmp(ptz_token, ptz_tmp_token) || !strcmp(ptz_token, ptz_tmp_token)) {
					_TTY_LOG_ONVIF_DEBUG("ch %d", j);
					return j;
				}
			}
		}
	}
	return -1;
}
int onvif_AbsoluteMove(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;
	arg_GetCapa capa;
	
	g_message("%s IN", __FUNCTION__);

	memset(&capa, 0x00, sizeof(arg_GetCapa));
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	int ch, panTiltCmd, zoomCmd, param0 = 0, param1 = 0;
	int interval;
	NF_PTZ_CMD ptz_cmd;

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF("ProfileToken: %s ", tmp.ProfileToken);

	ptz_cmd.ch = getPTZTokenNumber(tmp.ProfileToken);
	if (ptz_cmd.ch < 0) {
		tmp.result = 0;
		return ONVIF_ERR_RET_INTERNAL;
	}

	GetCapability(&capa, ptz_cmd.ch);

	if(tmp.Zoom.x < capa.zoomAbsoluteRangeMin[ptz_cmd.ch] || tmp.Zoom.x > capa.zoomAbsoluteRangeMax[ptz_cmd.ch])
	{
		return ONVIF_R_ERR_INVALID_PARAM;
	}
	if(tmp.PanTilt.x < capa.panAbsoluteRangeMin[ptz_cmd.ch] || tmp.PanTilt.x > capa.panAbsoluteRangeMax[ptz_cmd.ch] ||
		tmp.PanTilt.y < capa.tiltAbsoluteRangeMin[ptz_cmd.ch] || tmp.PanTilt.y > capa.tiltAbsoluteRangeMax[ptz_cmd.ch])
	{
		return ONVIF_R_ERR_INVALID_PARAM;
	}
	
	_TTY_LOG_ONVIF("ptz_cmd.ch: %d ", ptz_cmd.ch);
	panTiltCmd = getPtzPanTiltCmd(&tmp, &interval);
	if (panTiltCmd != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		ptz_cmd.cmd = panTiltCmd;
		_TTY_LOG_ONVIF("cmd : %d ", panTiltCmd);

		ptz_cmd.params[0] = interval;
		pushPTZControlOperation(&ptz_cmd, LIMIT);
	}
	zoomCmd = getPtzZoomCmd(&tmp, &interval);
	if (zoomCmd != -1) {
		ptz_cmd.cmd = zoomCmd;
		_TTY_LOG_ONVIF("cmd : %d ", zoomCmd);
		ptz_cmd.params[0] = interval;
		_TTY_LOG_ONVIF("ParamValue : %d ", ptz_cmd.params[0]);
		pushPTZControlOperation(&ptz_cmd, LIMIT);
	}

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_ContinuousMove(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int ch, panTiltCmd, zoomCmd, param0 = 0, param1 = 0;
	int interval, pt_move_flag = 0, z_move_flag = 0;
	NF_PTZ_CMD pt_cmd, z_cmd;

	_TTY_LOG_ONVIF("ProfileToken: %s ", tmp.ProfileToken);

	pt_cmd.ch = getPTZTokenNumber(tmp.ProfileToken);
	z_cmd.ch = pt_cmd.ch;
	if (pt_cmd.ch == -1) {
		tmp.result = 0;
		return ONVIF_ERR_RET_INTERNAL;
	}
	panTiltCmd = getPtzPanTiltCmd(&tmp, &interval);
	_TTY_LOG_ONVIF("panTiltCmd %d: ", panTiltCmd);
	if (panTiltCmd != -1) {
		if( panTiltCmd == NF_PTZ_CMD_STOP){
			PTZOperationStop(pt_cmd.ch, 0, 1);
		}
		else {
			pt_cmd.cmd = panTiltCmd;
			pt_cmd.params[0] = interval;
			pt_cmd.params[1] = interval;		
			pt_move_flag = 1;
		}
	}
	zoomCmd = getPtzZoomCmd(&tmp, &interval);
	if (zoomCmd != -1) {
		if( zoomCmd == NF_PTZ_CMD_STOP){
			PTZOperationStop(z_cmd.ch, 1, 0);
		}
		else {
			z_cmd.cmd = zoomCmd;
			z_cmd.params[0] = interval;
			z_move_flag = 1;
		}
	}

	if(pt_move_flag){
		pushPTZControlOperation(&pt_cmd, CONTINUOUS);
	}
	if(z_move_flag){
		pushPTZControlOperation(&z_cmd, CONTINUOUS);
	}
	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_CreatePresetTour(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_RemovePTZConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	int i = 0;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RemoveConfig tmp;
	int profile_cnt = -1;
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RemoveConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RemoveConfig));
	// 1. Profile Token �� ã?´?.
	profile_cnt = ONVIF_MAX_PROFILE_CNT;
	sprintf(buff, "onvif.profile%d.token", i);
	_TTY_LOG_ONVIF_DEBUG("profile_cnt  %d", profile_cnt, __LINE__);
	for (i = 0; i < profile_cnt; i++)
	{
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.profile_token, tmp_token))
		{
			sprintf(buff, "onvif.profile%d.ptz", i);
			nf_sysdb_set_str(buff, "\0");
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			goto success;
		}
	}
	if (i == profile_cnt)
	{
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR_NO_PROFILE;
	}

success:

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
	// ?ش? ī??Ʈ?? ptz ??�� 0��?? ?????Ѵ?.
	// Profile token?? ??��?? No Profile�� ??????.
}


int onvif_GetPTZConfigurations(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZConfigs tmp;
	arg_GetCapa capa;
	memset(&capa, 0x00, sizeof(arg_GetCapa));

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZConfigs);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZConfigs));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i;
	int ptz_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	int index=0;

	tmp.cnt = ONVIF_PTZ_CNT;
	
	for (i = 0; i < tmp.cnt ; i++) {
		GetCapability(&capa, i);
		capa.ch = i;

		if(capa.ptSupport[i] || capa.zoomSupport[i])
		{
			sprintf(buff, "onvif.ptz%d.token", i);
			strncpy(tmp.conf[index].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.ptz%d.name", i%ONVIF_CH);
			strncpy(tmp.conf[index].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			/* node_token */
			sprintf(buff, "onvif.ptz%d.node_token", i%ONVIF_CH);
			strncpy(tmp.conf[index].node_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			tmp.conf[index].timeout = 0;
			tmp.conf[index].use_count = 2;
			
			if(capa.ptSupport[i]){
				strncpy(tmp.conf[index].def_continous_PTSpace, CONTINUOUS_PTV_SPACE, COMMON_SIZE *2);
			}
			if(capa.zoomSupport[i]){
				strncpy(tmp.conf[index].def_continous_ZSpace, CONTINUOUS_ZV_SPACE, COMMON_SIZE *2);			
			}
			tmp.conf[index].ch = i;
			index++;
		}
		else
		{
			continue;
		}	
	}

	memcpy(packet.data, &tmp, sizeof(arg_PTZConfigs));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZConfigs)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_SetPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZNode);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZNode));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i;
	int ptz_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	ptz_cnt = ONVIF_PTZ_CNT;

	for (i = 0; i < ptz_cnt; i++) {
		sprintf(buff, "onvif.profile%d.ptz", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);


		_TTY_LOG_ONVIF_DEBUG("tmp.token %s (tmp_token : %s)",
				tmp.token, tmp_token);
		if (!strcmp(tmp.token, tmp_token)) {
			sprintf(buff, "onvif.ptz%d.name", i);
			nf_sysdb_set_str(buff, tmp.name);

			_TTY_LOG_ONVIF_DEBUG("tmp.node_token %s (Line : %d)", tmp.node_token, __LINE__);

			sprintf(buff, "onvif.ptz%d.node_token", i);
			nf_sysdb_set_str(buff, tmp.node_token);

			sprintf(buff, "onvif.ptz%d.timeout", i);
			nf_sysdb_set_uint(buff, tmp.timeout);

			if (tmp.default_continuous_zoom_velocity_space[0] != '\0') {
				sprintf(buff, "onvif.ptz%d.DefaultContinuousPanTiltVelocitySpace", i);
				nf_sysdb_set_str(buff, tmp.default_continuous_zoom_velocity_space);
			}

			if (tmp.default_continuous_pant_tilt_velocity_space[0] != '\0') {
				sprintf(buff, "onvif.ptz%d.default_continuous_pant_tilt_velocity_space", i);
				nf_sysdb_set_str(buff, tmp.default_continuous_pant_tilt_velocity_space);
			}

			_TTY_LOG_ONVIF_DEBUG("tmp.timeout %d", tmp.timeout);
			nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			break;
		}
	}

	if (i == ptz_cnt) {
		tmp.result = 0;
		return ONVIF_ERR_RET_INTERNAL;		
	}

	tmp.result = 1;
	memcpy(packet.data, &tmp, sizeof(arg_PTZNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZConfig tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZConfig));

	// To DO
	// if (tmp.token[0] != '\0'){
		get_ptz_config(&tmp);
	// }

	if( tmp.result == -1 ){
		return ONVIF_R_ERR_NO_PROFILE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_PTZConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetNode(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZNode);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZNode));

	// To DO

	int i;
	int ptz_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	ptz_cnt = ONVIF_PTZ_CNT;
	for (i = 0; i < ptz_cnt; i++) {
		sprintf(buff, "onvif.ptz%d.node_token", i);
		strncpy(tmp.node_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.token, tmp.node_token)) {
			sprintf(buff, "onvif.ptz%d.name", i);
			strncpy(tmp.name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.ptz%d.MaximumNumberOfPresets", i);
			tmp.max_num_preset = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.ptz%d.HomeSupported", i);
			tmp.home_supported = nf_sysdb_get_bool(buff);
			sprintf(buff, "onvif.ptz%d.FixedHomePosition", i);
			tmp.fixed_home_position = nf_sysdb_get_bool(buff);
			tmp.result = 1;
			break;
		}

	}
	if (i == ptz_cnt) {
		_TTY_LOG_ONVIF_DEBUG("ONVIF_R_ERR_NO_ENTITY: %d (Line : %d)",
				ONVIF_R_ERR_NO_ENTITY, __LINE__);
		return ONVIF_R_ERR_NO_ENTITY;
		//tmp.result = 0;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	memcpy(packet.data, &tmp, sizeof(arg_PTZNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZNode)) != ONVIF_ERR_RET_SUCCESS) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_ERR_RET_INTERNAL;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetNodes(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNodes tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZNodes);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZNodes));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int i;
	int ptz_cnt=0;
	char buff[COMMON_SIZE] = { 0, };
	arg_GetCapa capa;
	memset(&capa, 0x00, sizeof(arg_GetCapa));

	for (i = 0; i < ONVIF_PTZ_CNT; i++) {
		GetCapability(&capa, i);

		if(capa.ptSupport[i] || capa.zoomSupport[i])
		{
			sprintf(buff, "onvif.ptz%d.name", i);
			strncpy(tmp.ptz[ptz_cnt].name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			sprintf(buff, "onvif.ptz%d.node_token", i);
			strncpy(tmp.ptz[ptz_cnt].node_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			sprintf(buff, "onvif.ptz%d.MaximumNumberOfPresets", i);
			tmp.ptz[ptz_cnt].max_num_preset = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.ptz%d.HomeSupported", i);
			tmp.ptz[ptz_cnt].home_supported = nf_sysdb_get_bool(buff);
			sprintf(buff, "onvif.ptz%d.FixedHomePosition", i);
			tmp.ptz[ptz_cnt].fixed_home_position = nf_sysdb_get_bool(buff);
			tmp.ptz[ptz_cnt].result = 1;
			tmp.ptz[ptz_cnt].ch = i;
			ptz_cnt++;
		}
		else
		{
			// Not Do
		}
		
	}
	tmp.ptz_cnt = ptz_cnt;
	_TTY_LOG_ONVIF_DEBUG("tmp.ptz_cnt %d", tmp.ptz_cnt);

	memcpy(packet.data, &tmp, sizeof(arg_PTZNodes));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, 	header.code, sizeof(arg_PTZNodes)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetPresets(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZPresetNodes tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZPresetNodes);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZPresetNodes));

	// To DO
	int i = 0, j = 0;
	arg_preset preset[ONVIF_VSOURCE_CNT];
	char buff[COMMON_SIZE] = { 0, };
	char ch_buff[COMMON_SIZE] = { 0, };

	memset(preset, 0x00, sizeof(arg_preset) * ONVIF_VSOURCE_CNT);
	int profile_cnt = ONVIF_PRESET_CNT;
	preset_name_array(preset);
	int profileNum = findfieldStringCount("onvif.profile%d.token", tmp.profileToken, profile_cnt);
	
	if(profileNum == -1) return ONVIF_R_ERR_NO_ENTITY;
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	int ch = parseStringToChannel(ch_buff);
	_TTY_LOG_ONVIF_DEBUG("token_num: %d (Line : %d)", ch, __LINE__);
	if (ch != -1) {
		tmp.preset_cnt = 0;
		_TTY_LOG_ONVIF_DEBUG("tmp.preset_cnt: %d (Line : %d)", tmp.preset_cnt, __LINE__);
		for (i = 0; i < profile_cnt; i++) {
			if (checkPreset(ch, i + 1) != -1) {
				_TTY_LOG_ONVIF_DEBUG("ch: %d (i : %d)", ch, i+1);
				strncpy(tmp.preset[j].presetName, preset[ch].presetName[i], COMMON_SIZE - 1);
				strncpy(tmp.preset[j].presetToken, preset[ch].presetToken[i], ONVIF_TOKEN_SIZE - 1);
				_TTY_LOG_ONVIF_DEBUG("tmp.preset[i].presetName: %s (Line : %d)", tmp.preset[i].presetName, __LINE__);
				tmp.preset_cnt += 1;
				j++;
				_TTY_LOG_ONVIF_DEBUG("tmp.preset_cnt: %d (Line : %d)", tmp.preset_cnt, __LINE__);
			}
		}
		goto success;
		//tmp.result = 1;
		//_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	} else {
		//tmp.result = 0;
		return ONVIF_R_ERR_NO_ENTITY;
	}
success:
	memcpy(packet.data, &tmp, sizeof(arg_PTZPresetNodes));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZPresetNodes)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetPresetTour(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetPresetTourOptions(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetPresetTours(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetServiceCapabilities(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_PTZGetStatus(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int checkPreset(int ch, int presetNum) {
	int retNum = -1;
	int i = 0;
	char buff[COMMON_SIZE] = { 0, };
	// if it is exist, set the preset to camera.
	for (i = 0; i < ONVIF_PRESET_CNT; i++) {
		sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, i);
		if (presetNum == nf_sysdb_get_uint(buff)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			retNum = i;
			return retNum;
		}
	}

	return retNum;
}
int onvif_GotoPreset(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZPresetNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZPresetNode);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZPresetNode));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF("profileName : %s presetName : %s", tmp.profileToken, tmp.presetToken);
	NF_PTZ_CMD ptz_cmd;
	int i = 0;
	arg_preset preset[ONVIF_VSOURCE_CNT];
	char ch_buff[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	memset(preset, 0x00, sizeof(arg_preset) * ONVIF_VSOURCE_CNT);
	int profile_cnt = ONVIF_PTZ_CNT;
	preset_name_array(preset);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	int profileNum = findfieldStringCount("onvif.profile%d.token", tmp.profileToken, profile_cnt);
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	int ch = parseStringToChannel(ch_buff);

	int preset_num = -1;
	if (ch != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < ONVIF_PRESET_CNT; i++) {
			_TTY_LOG_ONVIF_DEBUG("tmp.presetToken: %s (preset[token_num].presetToken[%d] : %s)", tmp.presetToken, i, preset[ch].presetToken[i]);
			if (!strcmp(tmp.presetToken, preset[ch].presetToken[i])) {
				if (checkPreset(ch, i + 1) == -1) {
					tmp.result = -1;
					return ONVIF_R_ERR;
				}

				preset_num = i+1;
				_TTY_LOG_ONVIF_DEBUG("ch %d, preset_num %d", ch, preset_num);
				setITXCameraPreset(ch, preset_num, NF_PTZ_CMD_GOTO_PRESET);
				tmp.result = 1;
				goto goto_preset_done;
			}
		}
		if (i == ONVIF_PRESET_CNT) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			// presetToken is not exist
			tmp.result = -1;
			return ONVIF_R_ERR;
		}
	} else {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	tmp.result = 1;

goto_preset_done:

	memcpy(packet.data, &tmp, sizeof(arg_PTZPresetNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZPresetNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_ModifyPresetTour(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_OperatePresetTour(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_RelativeMove(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int removePresetDB(int ch, int presetNum, arg_preset* preset) {
	int i = 0, j = 0;
	char buff[COMMON_SIZE] = { 0, };
	char pcnt_buff[COMMON_SIZE] = { 0, };
	char buff_after[COMMON_SIZE] = { 0, };
	char presetName[COMMON_SIZE] = { 0, };
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int pcnt = 0;

	// init the presetName
	sprintf(presetName, "OnvifPre%02d%02d", ch, presetNum + 1);
	strncpy(preset[ch].presetName[presetNum], presetName, COMMON_SIZE - 1);
	int cnt = checkPreset(ch, presetNum + 1);
	if (cnt == -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return -1;
	}
	snprintf(pcnt_buff, sizeof(pcnt_buff), "cam.ptz.P%d.preset.PCNT", ch);
	pcnt = nf_sysdb_get_uint(pcnt_buff);

	for (i = cnt; i < pcnt; i++) {
		sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, i);
		sprintf(buff_after, "cam.ptz.P%d.preset.P%d.number", ch, i + 1);
		if( (i+1) < pcnt ){
			nf_sysdb_set_uint(buff, nf_sysdb_get_uint(buff_after));
		}
		else{
			nf_sysdb_set_uint(buff, 0);
		}
		
		sprintf(buff, "cam.ptz.P%d.preset.P%d.name", ch, i);
		sprintf(buff_after, "cam.ptz.P%d.preset.P%d.name", ch, i + 1);
		if( (i+1) < pcnt ){
			nf_sysdb_set_str(buff, nf_sysdb_get_str_nocopy(buff_after));
		}
		else{
			nf_sysdb_set_str(buff, "");
		}
		
	}
	if( pcnt < ONVIF_PRESET_CNT ){
		sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, pcnt);
		nf_sysdb_set_uint(buff, 0);
		sprintf(buff, "cam.ptz.P%d.preset.P%d.name", ch, pcnt);
		nf_sysdb_set_str(buff, "");
	}

//	sprintf(presetName, "OnvifPre%02d%02d", ch, presetNum + 1);
	_TTY_LOG_ONVIF_DEBUG("pcnt: %d (Line : %d)", pcnt, __LINE__);
	if (pcnt > 0) {
		pcnt = pcnt - 1;
	}
	nf_sysdb_set_uint(pcnt_buff, pcnt);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_CAM);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	return 1;
}
int onvif_RemovePreset(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZPresetNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZPresetNode);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZPresetNode));

	// To DO

	int i = 0;
	int profile_cnt = ONVIF_PTZ_CNT;
	char ch_buff[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	int profileNum = findfieldStringCount("onvif.profile%d.token", tmp.profileToken, profile_cnt);
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	int ch = parseStringToChannel(ch_buff);

	arg_preset preset[ONVIF_VSOURCE_CNT];
	memset(preset, 0x00, sizeof(arg_preset) * ONVIF_VSOURCE_CNT);
	preset_name_array(preset);
	for (i = 0; i < ONVIF_PRESET_CNT; i++) {
		if (!strcmp(tmp.presetToken, preset[ch].presetToken[i])) {
			// preset is not in the ITX DB
			/*
			 _TTY_LOG_ONVIF_DEBUG("ch: %d (i : %d)",ch, i+1);
			 if (checkPreset(ch, i+1) == -1) {
			 _TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
			 tmp.result = -1;
			 return;
			 }
			 */
			if (removePresetDB(ch, i, preset) == -1) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			//	tmp.result = -1;
				return ONVIF_R_ERR_PROFILE_EXIST;
			}
			save_array_to_preset_db(preset);
			setITXCameraPreset(ch, i, NF_PTZ_CMD_CLEAR_PRESET);


			goto success;
			//tmp.result = 1;
			// preset count is i
			//return ONVIF_R_ERR;
		}
	}
	if (i == ONVIF_PRESET_CNT) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		// presetToken is not exist
		//tmp.result = -1;
		return ONVIF_R_ERR_PROFILE_EXIST;
	}
	//tmp.result = 1;
success:
	memcpy(packet.data, &tmp, sizeof(arg_PTZPresetNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZPresetNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_RemovePresetTour(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_SendAuxiliaryCommand(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_SetConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_SetHomePosition(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZOperation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZOperation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZOperation));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
void setITXCameraPreset(int ch, int presetNum, int command) {
	NF_PTZ_CMD ptz_cmd;
	ptz_cmd.ch = ch;
	ptz_cmd.cmd = command;
	ptz_cmd.params[0] = presetNum;
	pushPTZControlOperation(&ptz_cmd, CONTINUOUS);
}

int setExistITXPresetDB(int ch, int presetNum) {

	int value = presetNum + 1;
	if (checkPreset(ch, value) == -1) {
		// preset is not exist in the ITX DB
		return -1;
	}
	setITXCameraPreset(ch, presetNum, NF_PTZ_CMD_SET_PRESET);
	return 1;
}
int getAvailablePresetNum(char* field, int length) {
	int i = 0, j = 0, exist = 0;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	for (j = 1; j < MAX_PRESET_NUM; j++) {
		for (i = 0; i < length; i++) {
			sprintf(buff, field, i);
			if (j == nf_sysdb_get_uint(buff)) {
				exist = 1;
			}
		}
		if (!exist) {
			// no preset matched
			return j;

		} else {
			exist = 0;
		}

	}
	return -1;
}
int setNewITXPresetDB(int ch) {
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	// find empty slot, if it does not search the db file, db sync could not correct.

	int i = 0, pcnt = 0;
	char buff[COMMON_SIZE] = { 0, };

	char presetName[COMMON_SIZE] = { 0, };
	sprintf(buff, "cam.ptz.P%d.preset.P%s.number", ch, "%d");
	int presetNum = getAvailablePresetNum(buff, ONVIF_PRESET_CNT);
	if (presetNum != -1) {
		setITXCameraPreset(ch, presetNum, NF_PTZ_CMD_SET_PRESET);
		snprintf(buff, sizeof(buff), "cam.ptz.P%d.preset.PCNT", ch);
		pcnt = nf_sysdb_get_uint(buff) + 1;
		if (pcnt > ONVIF_PRESET_CNT) {
			return -1;
		} else {
			nf_sysdb_set_uint(buff, pcnt);
		}

		_TTY_LOG_ONVIF_DEBUG("pcnt: %d (Line : %d)", pcnt, __LINE__);
		sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, pcnt - 1);
		nf_sysdb_set_uint(buff, presetNum);

		sprintf(buff, "cam.ptz.P%d.preset.P%d.name", ch, pcnt - 1);
		sprintf(presetName, "OnvifPre%02d%02d", ch, presetNum);
		_TTY_LOG_ONVIF_DEBUG("presetName: %s (Line : %d)", presetName, __LINE__);
		nf_sysdb_set_str(buff, presetName);
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_CAM, 0, 0, 0);
		sysdb_save_cate(NF_SYSDB_CATE_CAM);
		return presetNum - 1;
	} else {
		// nothing is available
		return -1;
	}

	// set the name number
	// return index

}
int parseStringToChannel(char* chString) {
	char* token = NULL;
	char* context = NULL;
	char* ret = NULL;
#if 0 //[[ hun_0140530_BEGIN -- del
	char spliter[] = "z";
	token = strtok_r(chString, spliter, &context);
	token = strtok_r(NULL, spliter, &context);
#endif //]] hun_0140530_END -- del
	ret = strstr(chString, ONVIF_PTZ_TOKEN_PREFIX);
	if(ret == NULL){
		printf("ONVIF_ERR: %s chString[%s] is invalid!\n", __FUNCTION__, chString);
		return -1;
	}
	token = ret + strlen(ONVIF_PTZ_TOKEN_PREFIX);
	int ch = atoi(token);
	_TTY_LOG_ONVIF_DEBUG("ch: %d (Line : %d)", ch, __LINE__);
	if (ch < 0) {
		_TTY_LOG_ONVIF_DEBUG("ASSERT");
	}
	return ch;
}
int onvif_SetPreset(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZPresetNode tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZPresetNode);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZPresetNode));

	// To DO

	int i = 0;
	int idx = 0;
	char ch_buff[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };


	arg_preset preset[ONVIF_VSOURCE_CNT];
	memset(preset, 0x00, sizeof(arg_preset) * ONVIF_VSOURCE_CNT);

	int profile_cnt = ONVIF_PTZ_CNT;
	preset_name_array(preset);

	int profileNum = findfieldStringCount("onvif.profile%d.token", tmp.profileToken, profile_cnt);
	if(profileNum == -1) {
		return ONVIF_R_ERR_PROFILE_EXIST;
	}
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	int ch = parseStringToChannel(ch_buff);

	if (tmp.presetToken[0] == '\0') {
		///null?̸? ?? preset?? ??��??.
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		idx = setNewITXPresetDB(ch);
		if (idx != -1) {
			strncpy(tmp.presetToken, preset[ch].presetToken[idx], ONVIF_TOKEN_SIZE - 1);
			if (tmp.presetName[0] != '\0') {
				// copy the presetName to db
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (idx : %d)", __FUNCTION__, idx);
				_TTY_LOG_ONVIF_DEBUG("preset[ch].presetName[idx]: %s (tmp.presetName : %s)", preset[ch].presetName[idx], tmp.presetName);
				strncpy(preset[ch].presetName[idx], tmp.presetName, COMMON_SIZE - 1);
				save_array_to_preset_db(preset);
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			}
			//tmp.result = 1;
			//return ONVIF_ERR_RET_SUCCESS;
			goto success;

		} else {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			return ONVIF_R_ERR_TOO_MANY_PRESET;
		}
	}

	if (ch != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < ONVIF_PRESET_CNT; i++) {
			_TTY_LOG_ONVIF_DEBUG("tmp.presetToken: %s (preset[token_num].presetToken[%d] : %s)", tmp.presetToken, i, preset[ch].presetToken[i]);
			if (!strcmp(tmp.presetToken, preset[ch].presetToken[i])) {

				// preset is not in the ITX DB
				if (setExistITXPresetDB(ch, i) == -1) {

					return ONVIF_R_ERR_PROFILE_EXIST;
				}

				if (tmp.presetName[0] != '\0') {
					// copy the presetName to db
					strncpy(preset[ch].presetName[i], tmp.presetName,	COMMON_SIZE - 1);
					save_array_to_preset_db(preset);
				}
				goto success;
			}
		}
		if (i == ONVIF_PRESET_CNT) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			// presetToken is not exist

			return ONVIF_R_ERR_PROFILE_EXIST;
		}
	} else {
		return ONVIF_R_ERR_NO_ENTITY;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
success:
	memcpy(packet.data, &tmp, sizeof(arg_PTZPresetNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PTZPresetNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_Stop(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Stop tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Stop);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Stop));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("tmp.ProfileToken %s (Line : %d)",
			tmp.ProfileToken, __LINE__);
	_TTY_LOG_ONVIF_DEBUG("tmp.PanTilt  %d (tmp.Zoom  : %d)",
			tmp.PanTilt, tmp.Zoom);
	int ch = -1;
	if (tmp.PanTilt == 1 || tmp.Zoom == 1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		ch = getPTZTokenNumber(tmp.ProfileToken);
		if (ch == -1) {
			tmp.result = 0;
			return ONVIF_ERR_RET_INTERNAL;
		}
		PTZOperationStop(ch, tmp.Zoom, tmp.PanTilt);
	}
	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_Stop));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Stop)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
/****************************************************
 End of PTZ Service
 *****************************************************/

int onvif_Subscribe(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_Subscribe tmp;
	
	NF_NETIF_GET_INFO ret_net_info;
	struct sockaddr_in ipaddr;
	char ipaddress[COMMON_SIZE];	
	int ss_id = 0;
	int ret = 0, http_port = 80, https_port = 443;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Subscribe);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Subscribe));

	//Create New subscription and get ss_id
	tmp.current_time = (unsigned int)time(NULL);
	ret = OV_event_create_new_subscription(tmp.termination_time, &tmp.filters);
	if( ret <= 0 ){
		g_message("%s ONVIF_R_ERR\n", __FUNCTION__);

		return ONVIF_R_ERR;
	}
	ss_id = ret;

	//Get subscription address
	http_port = (int)nf_sysdb_get_uint("net.http.onvifport");
	https_port = (int)nf_sysdb_get_uint("net.proto.sslport");

	nf_netif_get_info(&ret_net_info);
	ipaddr.sin_addr.s_addr = (unsigned long) ret_net_info.ipaddr;
	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);	
	snprintf(ipaddress, sizeof(char)*COMMON_SIZE, "%s", inet_ntoa(ipaddr.sin_addr));

	if(is_https_required())
		snprintf(tmp.ss_address, sizeof(tmp.ss_address), "https://%s:%d/onvif/subscription?subscription=%d", ipaddress, https_port, ss_id);
	else
		snprintf(tmp.ss_address, sizeof(tmp.ss_address), "http://%s:%d/onvif/subscription?subscription=%d", ipaddress, http_port, ss_id);

	OV_event_set_subscription_is_first_req(ss_id, 1);
	ret = OV_event_start_notify(ss_id, tmp.dest_address);	
	if( ret < 0 ){
		g_message("%s ONVIF_R_ERR 2\n", __FUNCTION__);

		return ONVIF_R_ERR;
	}		

	memcpy(packet.data, &tmp, sizeof(arg_Subscribe));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Subscribe)) != ONVIF_ERR_RET_SUCCESS) {
		g_message("%s ONVIF_RET_INTERNAL_ERR", __FUNCTION__);
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_Renew(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_Renew tmp;
	int ret = 0;
	
	g_message("%s IN", __FUNCTION__);
	
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Renew);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Renew));

	ret = OV_event_renew_subscription(tmp.ss_id, tmp.termination_time, tmp.is_relative);	
	if ( ret < 0) {
		tmp.err_code = ONVIF_PROCESS_RET_ERROR;
	}	
	tmp.termination_time = (unsigned int)ret;
	tmp.current_time = (unsigned int)time(NULL);
	
	memcpy(packet.data, &tmp, sizeof(arg_Renew));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Renew)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetSynchronizationPoint(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_SetSync tmp;
	int ret = 0;
	
	OV_DEBUG( "%s IN", __FUNCTION__);
	
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_SetSync);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_SetSync));

	ret = OV_event_set_sync(tmp.ss_id);
	if ( ret < 0) {
		return ONVIF_R_ERR;
	}
	
	memcpy(packet.data, &tmp, sizeof(arg_SetSync));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_SetSync)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetSynchronizationPoint_trt(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	arg_Tokens tmp;
	int i = 0, ret = 0;
	int profile_cnt=0;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));

	profile_cnt = ONVIF_MAX_PROFILE_CNT;//nf_sysdb_get_uint("onvif.common.profile_cnt");
	
	for(i=0;i<profile_cnt;i++)
	{
		snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp.profile_token, tmp_token))
		{
			break;
		}
	}
	
	if(i == profile_cnt)
	{
		return ONVIF_R_ERR_NO_PROFILE;	
	}

	ret = OV_event_set_sync_trt(1);

	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetRelayOptions(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_RelayOptions tmp;
	int ret = 0, i=0, j=0;	
	char buff[128];
	int delay_list[ONVIF_RELAY_DELAY_CNT] = { 5, 10, 15, 20, 30, 40, 60, 120, 180, 300 };
	int token_index = -1;
	g_message("%s IN", __FUNCTION__);
	
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RelayOptions);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RelayOptions));

	//
	memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
	tmp.relay_op_cnt = ONVIF_RELAY_CNT;
	for( i=0; i<tmp.relay_op_cnt ; i++){
		sprintf(buff, "onvif.relay%d.token", i);

		if (!strcmp(tmp.relay_token, nf_sysdb_get_str_nocopy(buff))) {
			token_index = i;
			tmp.token_index = token_index;
		}

		strncpy(tmp.relay_op[i].token, nf_sysdb_get_str_nocopy(buff), sizeof(char)*COMMON_SIZE);

		for(j=0; j<ONVIF_RELAY_DELAY_CNT; j++) {
			tmp.relay_op[i].delay_msec[j] = delay_list[j] * 1000;
		}
		tmp.relay_op[i].is_delay_discrete = 1;
		tmp.relay_op[i].mode = 0;
	}
	//
	
	memcpy(packet.data, &tmp, sizeof(arg_RelayOptions));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RelayOptions)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}


int onvif_GetDigitalInputs(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DigitalInputs tmp;
	int ret = 0, i=0;	
	char buff[128];
	
	g_message("%s IN", __FUNCTION__);
	
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DigitalInputs);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DigitalInputs));

	//
	memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
	tmp.cnt = ONVIF_ALARMIN_CNT;
	for( i=0; i<tmp.cnt ; i++){
		sprintf(buff, "%s%d",DIGITAL_INPUT_TOKEN_PREFIX, i);
		strncpy(tmp.digital_input[i].token, buff, strlen(buff));
		tmp.digital_input[i].idle_state = 0;
	}
	//
	
	memcpy(packet.data, &tmp, sizeof(arg_DigitalInputs));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_DigitalInputs)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_CheckDeviceReady(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DeviceReady tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DeviceReady);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DeviceReady));

	// To DO	
	if( dvrReady_info() != 0 ){
		tmp.result = STATUS_DEVICE_READY;
	}
	else{
		tmp.result = STATUS_DEVICE_NOT_READY;
	}


	memcpy(packet.data, &tmp, sizeof(arg_DeviceReady));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_DeviceReady)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetServices(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	guint service_cnt = 0;
	int i=0;

	arg_GetServices tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetServices);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetServices));

	int is_pt=0, is_zoom=0;
	
	// To DO
	memset(tmp.media2_configuration, 0x00, sizeof(tmp.media2_configuration));
	strncpy(tmp.media2_configuration, "VideoSource VideoEncoder", COMMON_SIZE*2-1);
	strcat(tmp.media2_configuration, " AudioSource AudioEncoder AudioOutput AudioDecoder");
	strcat(tmp.media2_configuration, " Metadata");
	//if(nf_caps_get_uint(nf_caps_get_root_obj(),"lens.mfz",NULL)){
		strcat(tmp.media2_configuration, " PTZ");	
	//}

	tmp.vs_cnt = ONVIF_VSOURCE_CNT;
	tmp.as_cnt = get_enable_audio_cnt();
	tmp.relay_cnt = ONVIF_RELAY_CNT;
	tmp.alarmin_cnt = ONVIF_ALARMIN_CNT;
    tmp.RuleSupport = true;             // yys onvif
    tmp.AnalyticsModuleSupport = true;
    tmp.CellBasedSceneDescriptionSupported = true;

	//tmp.service_flag |= ONVIF_SUPPORT_ANALYTICS; service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_DEVICE;	service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_EVENT;	service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_IMAGING;	service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_DEVICEIO;	service_cnt++;

	/*for(i=0;i<ONVIF_CH;i++)
	{
		if(onvif_is_ptz(i, NF_IPCAM_IMAGE_PAN))
		{
			is_pt = 1;
			break;
		}
		else
			is_pt = 0;

		if(onvif_is_ptz(i, NF_IPCAM_IMAGE_ZOOM))
		{
			is_zoom = 1;
			break;
		}
		else
			is_zoom = 0;
	}

	if(is_pt || is_zoom)
	{
		tmp.service_flag |= ONVIF_SUPPORT_PTZ;	service_cnt++;
	}*/
	tmp.service_flag |= ONVIF_SUPPORT_PTZ;	service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_MEDIA;	service_cnt++;
	/* Media1 Not supported in NVR */
	// service_cnt++;
/* ProfileG Recording Part
	tmp.service_flag |= ONVIF_SUPPORT_RECORDING;service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_SEARCH;	service_cnt++;
	tmp.service_flag |= ONVIF_SUPPORT_REPLAY;	service_cnt++;
*/
//	tmp.service_flag |= ONVIF_SUPPORT_RECEIVER;
//	tmp.service_flag |= ONVIF_SUPPORT_DISPLAY;	
//	tmp.service_flag |= ONVIF_SUPPORT_ACTION;
	tmp.service_cnt = service_cnt;
	tmp.ntp = 0;
	tmp.snapshot = 1;
	tmp.max_profiles = MAX_PROFILE;
	tmp.rtp_multicast = 1;
	tmp.rtp_tcp = 1;
	tmp.rtp_rtsp_tcp = 1;
	tmp.websocket = 1;

	char tmp_ip[COMMON_SIZE] = {0, };

	memset(tmp.service_url, 0x00, sizeof(char)*SIZE_OF_SERVICES*COMMON_SIZE);
	memset(tmp_ip, 0x00, sizeof(char)*COMMON_SIZE);

	get_ipaddress_p(tmp_ip, sizeof(char)*COMMON_SIZE);
	
	uint http_port = is_https_required() ? nf_sysdb_get_uint("net.proto.sslport") : nf_sysdb_get_uint("net.proto.webport");
	char* protocol = is_https_required() ? "https" : "http";

	sprintf(tmp.service_url[ONVIF_ALL_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_DEVICE_SERVICE_URL); //  DEFAULT URL 
	sprintf(tmp.service_url[ONVIF_ANALYTICS_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_ANALYTICS_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_DEVICE_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_DEVICE_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_EVENT_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_EVENT_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_IMAGING_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_IMAGING_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_DEVICEIO_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_DEVICEIO_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_PTZ_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_PTZ_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_MEDIA_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_MEDIA_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_RECORDING_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_RECORDING_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_SEARCH_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_SEARCH_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_REPLAY_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_REPLAY_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_RECEIVER_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_RECEIVER_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_DISPLAY_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_DISPLAY_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_ACTION_SERVICE], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_ACTION_SERVICE_URL);
	sprintf(tmp.service_url[ONVIF_SUBSCRIPTION], "%s://%s:%d/%s", protocol, tmp_ip, http_port, ONVIF_SUBSCRIPTION_URL);	

	memcpy(packet.data, &tmp, sizeof(arg_GetServices));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_GetServices)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetGuaranteedNumberOfVideoEncoderInstances(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_GuaranteedNumVideoEncoder tmp;
	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i=0;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GuaranteedNumVideoEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GuaranteedNumVideoEncoder));

	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			break;
		}
	}

	if (i == ONVIF_VSOURCE_CNT) {
		return ONVIF_R_ERR_NO_CONFIG;
	}
	
	// To DO
	tmp.total = 2;
	tmp.MJPEG = 0;
	tmp.H264 = 2;
	
	memcpy(packet.data, &tmp, sizeof(arg_GuaranteedNumVideoEncoder));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_GuaranteedNumVideoEncoder)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}


int onvif_GetDefaultGateway(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DefaultGateway tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DefaultGateway);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DefaultGateway));

	// To DO
	NF_NETIF_GET_INFO ret_net_info;
	NF_NETIF_MAC mac_addr;
	struct sockaddr_in ipaddr, subnet, ddns1, ddns2, gateway;

	/* dhcp */
	tmp.dhcp_flag = nf_sysdb_get_bool("onvif.common.gw_dhcp");

	/* ipaddr, gateway, subnet */
	nf_netif_get_info(&ret_net_info);

	gateway.sin_addr.s_addr = (unsigned long) ret_net_info.gateway;
	gateway.sin_addr.s_addr = htonl(gateway.sin_addr.s_addr);

	snprintf(tmp.gateway, sizeof(tmp.gateway), "%s", inet_ntoa(gateway.sin_addr));

	_TTY_LOG_ONVIF("g[%s]", tmp.gateway);

	memcpy(packet.data, &tmp, sizeof(arg_DefaultGateway));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_DefaultGateway)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetDefaultGateway(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_DefaultGateway tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_DefaultGateway);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_DefaultGateway));

	// To DO
	guint prev_dhcp = 0;

	multi_bye();
	
	/* Set new config */
	if (tmp.dhcp_flag != -1) {
		nf_sysdb_set_bool("onvif.common.gw_dhcp", tmp.dhcp_flag);
	}
	nf_sysdb_set_bool("onvif.common.gw_dhcp", tmp.dhcp_flag);
	if( tmp.dhcp_flag == 0){
		nf_sysdb_set_uint("net.proto.gateway", ip_str_to_uint(tmp.gateway));
	}
	nf_netif_apply_gateway();

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_NET);

	memcpy(packet.data, &tmp, sizeof(arg_DefaultGateway));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_DefaultGateway)) != ONVIF_ERR_RET_SUCCESS) {
//		return ONVIF_ERR_RET_INTERNAL;
	}
	
	excute_later_secs(multi_hello, 10);
	
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetAudioOutputConfiguration(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0;
	arg_AudioOutput tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioOutput);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioOutput));

	if (!nf_sysdb_get_bool("audio.enable"))
		return ONVIF_R_ERR_NOT_SUPPORT;

	if (tmp.token != NULL && tmp.token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));
		
		snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.aoutput0.token");
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp_token, tmp.token))
		{
			tmp.enable = (int)nf_sysdb_get_bool("audio.enable");
	
			snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
			snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
			snprintf(tmp.conf_token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.output_token"));
			tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
			snprintf(tmp.send_primacy, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.send_primacy"));
			tmp.use_count = nf_sysdb_get_uint("onvif.aoutput0.use_count");
		}
		else
		{
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}
	else
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioOutput));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioOutput)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioOutputConfigurations(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0;
	arg_AudioOutput tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioOutput);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioOutput));

	memset(tmp_token, 0x00, sizeof(tmp_token));
	
	snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.aoutput0.token");
	strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

	if (tmp_token[0] != '\0')
	{
		tmp.enable = (int)nf_sysdb_get_bool("audio.enable");

		snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
		snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
		snprintf(tmp.conf_token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.output_token"));
		tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
		snprintf(tmp.send_primacy, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.send_primacy"));
		tmp.use_count = 2;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioOutput));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioOutput)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}


int onvif_GetAudioOutput(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0;
	arg_AudioOutput tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioOutput);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioOutput));

	if (!nf_sysdb_get_bool("audio.enable"))
		return ONVIF_R_ERR_NOT_SUPPORT;

	if (tmp.token != NULL && tmp.token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));
		
		for (i = 0; i < MAX_PROFILE; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp_token, tmp.token))
			{
				break;
			}
		}
	}

	if (i == MAX_PROFILE)
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}

	tmp.enable = (int)nf_sysdb_get_bool("audio.enable");
	
	snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
	snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
	snprintf(tmp.conf_token, COMMON_SIZE - 1, "aoconfig0");
	tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
	strncpy(tmp.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
	tmp.use_count = 2;

	memcpy(packet.data, &tmp, sizeof(arg_AudioOutput));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioOutput)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioOutput2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0;
	arg_AudioOutput tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioOutput);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioOutput));

	if (!nf_sysdb_get_bool("audio.enable"))
	{
		return ONVIF_R_ERR_NOT_SUPPORT;
	}

	if (tmp.token[0] == '\0' && tmp.conf_token[0] == '\0')
	{
		tmp.enable = (int)nf_sysdb_get_bool("audio.enable");
	
		snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
		snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
		snprintf(tmp.conf_token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.output_token"));
		tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
		strncpy(tmp.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
	}
	else
	{
		if (tmp.token[0] != '\0')
		{
			memset(tmp_token, 0x00, sizeof(tmp_token));

			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
			{
				snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
				strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				if (!strcmp(tmp_token, tmp.token))
				{
					tmp.enable = (int)nf_sysdb_get_bool("audio.enable");

					snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
					snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
					snprintf(tmp.conf_token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.output_token"));
					tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
					strncpy(tmp.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);
					
					break;
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT)
			{
				return ONVIF_R_ERR_NO_PROFILE;
			}
		}
		else if (tmp.conf_token[0] != '\0')
		{
			for (i = 0; i < ONVIF_AUDIOOUTPUT_CNT; i++)
			{
				memset(tmp_token, 0x00, sizeof(buff));
				
				sprintf(buff, "onvif.aoutput%d.token", i);
				strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

				if (!strcmp(tmp.conf_token, tmp_token))
				{
					tmp.enable = (int)nf_sysdb_get_bool("audio.enable");
					
					snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
					snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
					snprintf(tmp.conf_token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.output_token"));
					tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
					strncpy(tmp.send_primacy, "www.onvif.org/ver20/HalfDuplex/Client", COMMON_SIZE - 1);

					break;
				}
			}

			if (i == ONVIF_AUDIOOUTPUT_CNT)
			{
				return ONVIF_R_ERR_NO_CONFIG;
			}
		}
	}
	tmp.use_count = 2;

	memcpy(packet.data, &tmp, sizeof(arg_AudioOutput));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioOutput)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioDecoder2(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0;
	arg_AudioDecoder tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioDecoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioDecoder));

	if (!nf_sysdb_get_bool("audio.enable"))
	{
		return ONVIF_R_ERR_NOT_SUPPORT;
	}
	else
	{
		tmp.enable = (int)nf_sysdb_get_bool("audio.enable");
	}

	if (tmp.token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));

		for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp_token, tmp.token))
			{				
				break;
			}
		}

		if (i == ONVIF_MAX_PROFILE_CNT)
		{
			return ONVIF_R_ERR_NO_PROFILE;
		}
	}
	else if (tmp.conf_token[0] != '\0')
	{
		for (i = 0; i < ONVIF_AUDIOOUTPUT_CNT; i++)
		{
			memset(tmp_token, 0x00, sizeof(buff));
			
			strncpy(tmp_token, nf_sysdb_get_str_nocopy("onvif.adecoder0.token"), COMMON_SIZE - 1);

			if (!strcmp(tmp.conf_token, tmp_token))
			{
				break;
			}
		}
		
		if (i == ONVIF_AUDIOOUTPUT_CNT)
		{
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}

	strncpy(tmp.conf_token, nf_sysdb_get_str_nocopy("onvif.adecoder0.token"), COMMON_SIZE - 1);
	strncpy(tmp.name, nf_sysdb_get_str_nocopy("onvif.adecoder0.name"), COMMON_SIZE - 1);

	memcpy(packet.data, &tmp, sizeof(arg_AudioDecoder));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioDecoder)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}


int onvif_GetCompatibleAudioOutput(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0;
	arg_AudioOutput tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioOutput);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioOutput));

	if (tmp.token != NULL && tmp.token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));
		
		for (i = 0; i < MAX_PROFILE; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp_token, tmp.token))
			{
				break;
			}
		}
	}

	if (i == MAX_PROFILE)
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}

	tmp.enable = (int)nf_sysdb_get_bool("audio.enable");
	
	snprintf(tmp.token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.token"));
	snprintf(tmp.name, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.name"));
	snprintf(tmp.conf_token, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.output_token"));
	tmp.output_level = nf_sysdb_get_uint("audio.out_volume");
	snprintf(tmp.send_primacy, COMMON_SIZE - 1, nf_sysdb_get_str_nocopy("onvif.aoutput0.send_primacy"));
	tmp.use_count = nf_sysdb_get_uint("onvif.aoutput0.use_count");

	memcpy(packet.data, &tmp, sizeof(arg_AudioOutput));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_AudioOutput)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetSnapshotUri(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_SnapshotUri tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_SnapshotUri);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_SnapshotUri));

	// To DO
	int i, profile_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token: %s (tmp.token: %s)",tmp_token, tmp.token);
		if (!strcmp(tmp.token, tmp_token)) {
			break;
		}
	}
	if (i == profile_cnt) {
		_TTY_LOG_ONVIF_DEBUG("i :  %d (profile_cnt: %d)", i, profile_cnt);
		//tmp.result = 0;
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}
	
	get_snapshot_uri(i, &tmp);
	
	memcpy(packet.data, &tmp, sizeof(arg_SnapshotUri));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_SnapshotUri)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_GetPTZConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZConfig tmp;
	int i=0;
	char buff[COMMON_SIZE]={0,};
	char tmp_token[COMMON_SIZE]={0,};
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PTZConfig);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PTZConfig));

	// To DO
	for(i=0;i<ONVIF_PTZ_CNT;i++)
	{
		sprintf(buff, "onvif.ptz%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if(!strcmp(tmp_token, tmp.token))
		{
			makeConfiguration(&tmp, i);
			sprintf(buff, "onvif.ptz%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			strncpy(tmp.token, tmp_token, sizeof(tmp.token));

			sprintf(buff, "onvif.ptz%d.name", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			strncpy(tmp.name,  tmp_token, sizeof(tmp.name));

			sprintf(buff, "onvif.ptz%d.node_token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			strncpy(tmp.node_token, tmp_token,sizeof(tmp.node_token));
			break;
		}
	}
	if(i==ONVIF_PTZ_CNT)
		return ONVIF_R_ERR_NO_CONFIG;
	

	memcpy(packet.data, &tmp, sizeof(arg_PTZConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_PTZConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_StartMulticastStreaming(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0, ch = 0;
	arg_Tokens tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));

	if (tmp.profile_token != NULL && tmp.profile_token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));

		for (i = 0; i < MAX_PROFILE; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp_token, tmp.profile_token))
			{
				ch = get_vencoder_index_from_profile(tmp_token);
				int convert_ch = (ch % ONVIF_CH) + ((ch / ONVIF_CH) * 16);
				
				nf_issm_ctl_mcast_start(convert_ch, 0);
				g_message("\e[31m nf_issm_ctl_mcast_start[%d] \e[0m\n", convert_ch);
				break;
			}
		}
	}

	if (i == MAX_PROFILE)
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_StopMulticastStreaming(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	char tmp_token[COMMON_SIZE] = {0,};
	char buff[COMMON_SIZE] = {0,};
	int i = 0, ch = 0;
	arg_Tokens tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Tokens);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Tokens));

	if (tmp.profile_token != NULL && tmp.profile_token[0] != '\0')
	{
		memset(tmp_token, 0x00, sizeof(tmp_token));
		
		for (i = 0; i < MAX_PROFILE; i++)
		{
			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.profile%d.token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			if (!strcmp(tmp_token, tmp.profile_token))
			{
				ch = get_vencoder_index_from_profile(tmp_token);
				int convert_ch = (ch % ONVIF_CH) + ((ch / ONVIF_CH) * 16);
				
				nf_issm_ctl_mcast_stop(convert_ch);
				g_message("\e[31m nf_issm_ctl_mcast_stop[%d] \e[0m\n", convert_ch);
				break;
			}
		}
	}

	if (i == MAX_PROFILE)
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Tokens));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

void nf_onvif_sysdbchange_make_authfile(ONVIF_USR_MAN *man) 
{
  FILE *fp;
  int i, n;
  ONVIF_USR_MAN     getman[8];
  ONVIF_USR_MAN     authman;
  ONVIF_USR_AUTH    grp_auth;
  int usrcnt;
  unsigned char tmp[64];
  unsigned char cal_mac[16];
  unsigned char macaddr[64];

  if ( man == NULL) {
    char buff[128];

    memset(&grp_auth, 0x00, sizeof(ONVIF_USR_AUTH));

    grp_auth.setupman   = nf_sysdb_get_bool("usr.grp.G1.sys_setup");
    grp_auth.searchman  = nf_sysdb_get_bool("usr.grp.G1.search");
    grp_auth.archman    = nf_sysdb_get_bool("usr.grp.G1.archive");
    grp_auth.recsetman  = nf_sysdb_get_bool("usr.grp.G1.rec_setup");
    grp_auth.eventman   = nf_sysdb_get_bool("usr.grp.G1.event");
    grp_auth.audioman   = nf_sysdb_get_bool("usr.grp.G1.audio");
    grp_auth.micman     = nf_sysdb_get_bool("usr.grp.G1.microphone");
    grp_auth.remoteman  = nf_sysdb_get_bool("usr.grp.G1.remote");
    grp_auth.shutman    = nf_sysdb_get_bool("usr.grp.G1.shutdown");

    grp_auth.setupusr   = nf_sysdb_get_bool("usr.grp.G2.sys_setup");
    grp_auth.searchusr  = nf_sysdb_get_bool("usr.grp.G2.search");
    grp_auth.archusr    = nf_sysdb_get_bool("usr.grp.G2.archive");
    grp_auth.recsetusr  = nf_sysdb_get_bool("usr.grp.G2.rec_setup");
    grp_auth.eventusr   = nf_sysdb_get_bool("usr.grp.G2.event");
    grp_auth.audiousr   = nf_sysdb_get_bool("usr.grp.G2.audio");
    grp_auth.micusr     = nf_sysdb_get_bool("usr.grp.G2.microphone");
    grp_auth.remoteusr  = nf_sysdb_get_bool("usr.grp.G2.remote");
    grp_auth.shutusr    = nf_sysdb_get_bool("usr.grp.G2.shutdown");

    memset(&getman, 0x00, sizeof(getman));
    memset(&authman, 0x00, sizeof(authman));

    usrcnt = nf_sysdb_get_uint("usr.UCNT");

    for (i=0, n=0; i < usrcnt; i++) {
      gboolean remote_login;

      snprintf(buff, sizeof(buff), "usr.U%d.grpname", i);
      snprintf(authman.grpname, sizeof(man[i].grpname),"%s", nf_sysdb_get_str_nocopy(buff));

      if( strcmp(authman.grpname, "MANAGER") == 0 )  {
        remote_login = grp_auth.remoteman;
      } else if( strcmp(authman.grpname, "USER" ) == 0 ) {
        remote_login = grp_auth.remoteusr;
      } else if( strcmp(authman.grpname, "ADMIN" ) == 0 ) {
        remote_login = 1;
      } else {
        remote_login = 0;
      }

      if( remote_login ) {
        memset(buff, 0, sizeof(buff));
        snprintf(buff, sizeof(buff), "usr.U%d.name", i);
        snprintf(getman[n].usrid, sizeof(man[i].usrid),"%s", nf_sysdb_get_str_nocopy(buff));

        memset(buff, 0, sizeof(buff));
        snprintf(buff, sizeof(buff), "usr.U%d.pass", i);
        snprintf(getman[n].passwd , sizeof(man[i].passwd),"%s", nf_sysdb_get_str_nocopy(buff));
        authman.usrcnt = ++n;
      }
    }

    getman[0].usrcnt = authman.usrcnt;
  } else {
    memcpy(getman, man, sizeof(getman));
  }

  if ( getman[0].usrcnt == 0 ) {
    g_warning("%s user nothing error", __FUNCTION__);
    return;
  }

  if ( (fp = fopen(ONVIF_DVRauthFilePath, "w")) == NULL ) {
    g_warning("%s file oepn error", __FUNCTION__);
    return;
  }

  for (i = 0; i < getman[0].usrcnt; i++) {
    fprintf(fp, "%s:", getman[i].usrid);
    fprintf(fp, "%s\n", getman[i].passwd);
  }

    memset(macaddr, 0x00, sizeof(macaddr));
    strncpy( macaddr, nf_sysdb_get_str_nocopy("sys.info.mac"), sizeof(macaddr)-1 );

    cal_mac[0] = (unsigned char)(((macaddr[0] + macaddr[5]) & 0xff) / 2);
    cal_mac[1] = (unsigned char)((macaddr[1] + macaddr[3]) & 0xff);
    cal_mac[2] = (unsigned char)((macaddr[2] + macaddr[4]) & 0xff) / 2;

    snprintf(tmp, 64, "%02X%02X%02X@3246", cal_mac[0], cal_mac[1], cal_mac[2]);
    fprintf(fp, "%s%s%s:%s\n", "Fw","Up","3398", tmp);

  fchmod(fp, S_IEXEC);

  fclose(fp);
}

int AddConfiguration(char *p_config_token, char *p_config_type, int p_profile_index)
{
	printf("\e[31m [%s][%d] token[%s] type[%s] index[%d] \e[0m\n", __FUNCTION__, __LINE__, p_config_token, p_config_type, p_profile_index);
	char buff[COMMON_SIZE] = {0,};
    int ret = 0;

	if ( strcmp(p_config_type, "All") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vsource", p_profile_index);
		nf_sysdb_set_str(buff, "vsconfig0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vencoder", p_profile_index);
		nf_sysdb_set_str(buff, "ve0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.asource", p_profile_index);
		nf_sysdb_set_str(buff, "asconfig0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aencoder", p_profile_index);
		nf_sysdb_set_str(buff, "ae0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.metadata", p_profile_index);
		nf_sysdb_set_str(buff, "m0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vanlaytics", p_profile_index);
		nf_sysdb_set_str(buff, "va0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.adecoder", p_profile_index);
		nf_sysdb_set_str(buff, "ad0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.ptz", p_profile_index);
		nf_sysdb_set_str(buff, "ptz0");
	}
	else if ( strcmp(p_config_type, "VideoSource") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vsource", p_profile_index);
		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "VideoEncoder") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vencoder", p_profile_index);
   		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "AudioSource") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.asource", p_profile_index);
   		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "AudioEncoder") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aencoder", p_profile_index);
   		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "AudioOutput") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aoutput", p_profile_index);
		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "AudioDecoder") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.adecoder", p_profile_index);
		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "Metadata") == 0 )	
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.metadata", p_profile_index);
   		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "Analytics") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vanalytics", p_profile_index);
   		nf_sysdb_set_str(buff, p_config_token);
	}
	else if ( strcmp(p_config_type, "PTZ") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.ptz", p_profile_index);
   		nf_sysdb_set_str(buff, p_config_token);
	}
	else
	{
		return ONVIF_R_ERR_INVALID_PARAM;
	}
}

int Removeconfiguration(char *p_config_token, char *p_config_type, int p_profile_index)
{
	printf("\e[31m [%s][%d] token[%s] type[%s] index[%d] \e[0m\n", __FUNCTION__, __LINE__, p_config_token, p_config_type, p_profile_index);
	char buff[COMMON_SIZE] = {0,};
	
	if ( strcmp(p_config_type, "All") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vsource", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vencoder", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.asource", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aencoder", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.metadata", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vanalytics", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.ptz", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aoutput", p_profile_index);
		nf_sysdb_set_str(buff, "\0");

		memset(buff, 0x00, COMMON_SIZE);
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.adecoder", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}// we only one token in profile this cam (If we create fisheye cam, modify will be)
	else if ( strcmp(p_config_type, "VideoSource") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vsource", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "VideoEncoder") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vencoder", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "AudioSource") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.asource", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "AudioEncoder") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aencoder", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "AudioOutput") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.aoutput", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "AudioDecoder") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.adecoder", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "Metadata") == 0 )	
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.metadata", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "Analytics") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.vanalytics", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else if ( strcmp(p_config_type, "PTZ") == 0 )
	{
		snprintf(buff, (COMMON_SIZE - 1) * sizeof(char), "onvif.profile%d.ptz", p_profile_index);
		nf_sysdb_set_str(buff, "\0");
	}
	else
	{
	}
}

int onvif_StartFirmwareUpgrade(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_FirmwareUpgrade tmp;

	int i = 0;
	char tmp_UploadUri[COMMON_SIZE] = {0, };
	char ip_addr[COMMON_SIZE] = {0, };

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_FirmwareUpgrade);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_FirmwareUpgrade));

	//TO DO
	get_ipaddress(ip_addr);
	
	if(is_https_required())
		snprintf(tmp_UploadUri, (COMMON_SIZE - 1) * sizeof(char), "https://%s:%d/upgrade-legacy", ip_addr, nf_sysdb_get_uint("net.proto.sslport"));
	else
		snprintf(tmp_UploadUri, (COMMON_SIZE - 1) * sizeof(char), "http://%s:%d/upgrade-legacy", ip_addr, nf_sysdb_get_uint("net.proto.webport"));

	strncpy(tmp.UploadUri, tmp_UploadUri, COMMON_SIZE - 1);
	tmp.UploadDelay = 2000;
	tmp.ExpectedDownTime = 120000;

	memcpy(packet.data, &tmp, sizeof(arg_FirmwareUpgrade));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_FirmwareUpgrade)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_CreateOSD(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_OSDConfiguration tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_OSDConfiguration);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_OSDConfiguration));

	char profile[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	char video_source_token[COMMON_SIZE] = { 0, };
	int video_source_index;

	// Get VideoSource token's index
	if (tmp.video_source_token) {
		for(video_source_index = 0; video_source_index < ONVIF_VSOURCE_CNT; video_source_index++)
		{ 
			memset(buff, 0x00, sizeof(COMMON_SIZE));

			snprintf(buff, sizeof(char) * (COMMON_SIZE - 1), "onvif.vsource%d.token", video_source_index);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			g_message("tmp_token: %s (tmp.video_token: %s)",tmp_token, tmp.video_source_token);
			if(!strcmp(tmp.video_source_token, tmp_token)) {
				break;
			}
		}		
	}

	if (video_source_index == ONVIF_VSOURCE_CNT)
		return ONVIF_R_ERR_NO_CONFIG;

	sprintf(buff, "onvif.vsource%d.osd.type", video_source_index);
	nf_sysdb_set_uint(buff, tmp.type);
	sprintf(buff, "onvif.vsource%d.osd.position_type", video_source_index);
	nf_sysdb_set_uint(buff, tmp.position_type);
	sprintf(buff, "onvif.vsource%d.osd.plain_text", video_source_index);
	nf_sysdb_set_str(buff, tmp.plain_text);
	
	sprintf(buff, "onvif.vsource%d.osd.token", video_source_index);
	nf_sysdb_set_str(buff, tmp.osd_token);
	
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_ONVIF, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_NET, 0, 0, 0);
	sysdb_save_cate(NF_SYSDB_CATE_NET);
	
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_OSDConfiguration)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetOSD(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_OSDConfigurations tmp;

	int i;
	int ret;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_OSDConfigurations);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_OSDConfigurations));


	// Specific OSD Token Return
	if (tmp.osd_token[0] != '\0' || tmp.video_source_token[0] != '\0') {
		if (tmp.osd_token[0] != '\0') {
			for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
				sprintf(buff, "onvif.vsource%d.osd.token", i);
				g_message("nf_sysdb_get_str_nocopy(buff): %s (tmp.osd_token: %s)", nf_sysdb_get_str_nocopy(buff), tmp.osd_token);

				if (!strcmp(nf_sysdb_get_str_nocopy(buff), tmp.osd_token)) {
					break;
				}
			}
		}
		
		if (tmp.video_source_token[0] != '\0') {
			for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
				sprintf(buff, "onvif.vsource%d.token", i);
				strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				g_message("tmp_token: %s (tmp.video_token: %s)",tmp_token, tmp.video_source_token);
				if(!strcmp(tmp.video_source_token, tmp_token)) {
					break;
				}
			}
		}

		if (i == ONVIF_VSOURCE_CNT)
			return ONVIF_R_ERR_NO_CONFIG;

		tmp.osd_cnt = 1;
		sprintf(buff, "onvif.vsource%d.osd.type", i);
		tmp.osd_configs[0].type = nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.vsource%d.osd.position_type", i);
		tmp.osd_configs[0].position_type = nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.vsource%d.osd.plain_text", i);
		strncpy(tmp.osd_configs[0].plain_text, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.vsource%d.osd.token", i);
		strncpy(tmp.osd_configs[0].osd_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp.osd_configs[0].video_source_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (tmp.osd_configs[0].osd_token[0] == '\0') {
			return ONVIF_R_ERR_NO_CONFIG;
		}

		memcpy(packet.data, &tmp, sizeof(arg_OSDConfigurations));
		
		if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_OSDConfigurations)) != ONVIF_ERR_RET_SUCCESS) {
			return ONVIF_ERR_RET_INTERNAL;
		}

	}

	// All OSD Token Return

	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		sprintf(buff, "onvif.vsource%d.osd.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (tmp_token[0] != '\0') {
			sprintf(buff, "onvif.vsource%d.token", i);
			strncpy(tmp.osd_configs[tmp.osd_cnt].video_source_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			sprintf(buff, "onvif.vsource%d.osd.type", i);
			tmp.osd_configs[tmp.osd_cnt].type = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource%d.osd.position_type", i);
			tmp.osd_configs[tmp.osd_cnt].position_type = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource%d.osd.plain_text", i);
			strncpy(tmp.osd_configs[tmp.osd_cnt].plain_text, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			sprintf(buff, "onvif.vsource%d.osd.token", i);
			strncpy(tmp.osd_configs[tmp.osd_cnt].osd_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			tmp.osd_cnt++;
		}
	}

	memcpy(packet.data, &tmp, sizeof(arg_OSDConfigurations));
		
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_OSDConfigurations)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}


// int onvif_SetOSD(NfOnvif *self, int fd, char *buff_rcv)
// {
// 	ONVIF_PACKET packet;
// 	ONVIF_HEADER header;
// 	guint packet_len;
// 	arg_OSDConfiguration tmp;

// 	g_message("%s IN", __FUNCTION__);

// 	memset(&header, 0, sizeof(header));
// 	memcpy(&header, buff_rcv, sizeof(header));

// 	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_OSDConfiguration);

// 	memcpy(&packet, buff_rcv, packet_len);
// 	memcpy(&tmp, packet.data, sizeof(arg_OSDConfiguration));

// 	return ONVIF_ERR_RET_SUCCESS;
// }

int onvif_DeleteOSD(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_OSDConfiguration tmp;
	char buff[COMMON_SIZE] = {0,};
	int i;
	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_OSDConfiguration);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_OSDConfiguration));

	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		sprintf(buff, "onvif.vsource%d.osd.token", i);
		if (!strcmp(nf_sysdb_get_str_nocopy(buff), tmp.osd_token)) {
			break;
		}		
	}

	if (i == ONVIF_VSOURCE_CNT) {
		return ONVIF_R_ERR_NO_CONFIG;
	}

	sprintf(buff, "onvif.vsource%d.osd.plain_text", i);
	nf_sysdb_set_str(buff, "\0");
	sprintf(buff, "onvif.vsource%d.osd.token", i);
	nf_sysdb_set_str(buff, "\0");

	memcpy(packet.data, &tmp, sizeof(arg_OSDConfiguration));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_OSDConfiguration)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetHttpAuthMethod(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_HttpAuth tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_HttpAuth);
	
	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_HttpAuth));

	tmp.http_auth = nf_sysdb_get_uint("net.proto.httpauth_method");

	memcpy(packet.data, &tmp, sizeof(arg_HttpAuth));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_FirmwareUpgrade)) != ONVIF_ERR_RET_SUCCESS)
	{
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}