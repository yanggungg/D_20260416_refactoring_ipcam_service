#include "nf_onvif_server.h"
#include <sys/msg.h>
#include <sys/errno.h>
#include <time.h>

#include "nf_common.h"
#include "nf_util_netif.h"
#include "nf_util_time.h"
#include "nf_timer.h"

#include "nf_api_ipcam.h"
//#include "nf_webra_def.h"
#include "nf_ptz.h"
#include "scm.h"
#include "smt.h"

#if defined(TCP_BUFFER_CTRL)
#include "log.h"
#endif
//#include "itx_utils.h"
#include "nf_onvif_server_app.h"
#include "onvif_service_util.h"

#include "unp.h"



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

static void _getAudioEncoderConfiguration(arg_AudioEncoder *tmp);
static int _getMetadataConfiguration(arg_Metadata* tmp);
static void _getVideoSourceConfiguration(arg_VideoSource *tmp);
//static int onvif_GetRecordings(NfOnvif *self, int fd, char *buff_rcv); 
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
	"CMD_GetConfigurationOptions",
	"CMD_GetConfigurations",
	"CMD_GetNode",
	"CMD_GetNodes",
	"CMD_GetPresets",
	"CMD_GetPresetTour",
	"CMD_GetPresetTourOptions",
	"CMD_GetPresetTours",
	"CMD_GetServiceCapabilities",
	"CMD_PTZGetStatus",
	"CMD_ModifyPresetTour",
	"CMD_OperatePresetTour",
	"CMD_RelativeMove",
	"CMD_RemovePreset",
	"CMD_RemovePresetTour",
	"CMD_SendAuxiliaryCommand",
	"CMD_SetConfiguration",
	"CMD_SetHomePosition",
	"CMD_SetPreset",
	"CMD_GotoPreset",
	"CMD_Stop",
	"CMD_GetEventCapa",
	"CMD_GetPTZConfigurations",
	"CMD_SetPTZConfiguration",
	"CMD_AddPTZConfiguration",
	"CMD_RemovePTZConfiguration",
	"CMD_CreateRecording",
	"CMD_CreateRecordingJob",
	"CMD_CreateTrack",
	"CMD_DeleteRecording",
	"CMD_DeleteRecordingJob",
	"CMD_DeleteTrack",
	"CMD_GetRecordingConfiguration",
	"CMD_GetRecordingJobConfiguration",
	"CMD_GetRecordingJobs",
	"CMD_GetRecordingJobState",
	"CMD_GetRecordingOptions",
	"CMD_GetRecordings",
	"CMD_GetTrackConfiguration",
	"CMD_SetRecordingConfiguration",
	"CMD_SetRecordingJobConfiguration",
	"CMD_SetRecordingJobMode",
	"CMD_SetTrackConfiguration",
	"CMD_EndSearch",
	"CMD_FindEvent",
	"CMD_FindMetadata",
	"CMD_FindPTZPosition",
	"CMD_FindRecordings",
	"CMD_GetEventSearchResults",
	"CMD_GetMediaAttributes",
	"CMD_GetMetadataSearchResults",
	"CMD_GetRecordingInformation",
	"CMD_GetRecordingSearchResults",
	"CMD_GetRecordingSummary",
	"CMD_GetSearchState",
	"CMD_GetRecordingServiceCapabilities",
	"CMD_GetReplayConfiguration",
	"CMD_GetReplayUri",
	"CMD_SetReplayConfiguration",
	"CMD_GetRecordingJobStateChangeEvent",
	"CMD_GetEventProperties",	
	"CMD_CreatePullPointSubscription",	
	"CMD_PullMessages",
	"CMD_Subscribe",
	"CMD_Notify",
	"CMD_SetSynchronizationPoint",	
	"CMD_Unsubscribe",	
	"CMD_Renew",
	"CMD_GetRelayOptions",
	"CMD_GetDigitalInputs",
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

int nf_onvif_packet_code(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_HEADER header;
	int ret = 0;
	int delay = 3;
	int fail_delay = 15;
	static int before_cmd = -1;
	g_return_val_if_fail(self != NULL, ONVIF_ERR_RET_PARAMETER);
	g_return_val_if_fail(fd != -1, ONVIF_ERR_RET_PARAMETER);
	g_return_val_if_fail(buff_rcv != NULL, ONVIF_ERR_RET_PARAMETER);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));
	
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
		case CMD_GetProfileDB:
			ret = onvif_GetProfileDB(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoder:
			ret = onvif_GetVideoEncoder(self, fd, buff_rcv);
			break;
		case CMD_GetVideoEncoders:
			ret = onvif_GetVideoEncoders(self, fd, buff_rcv);
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
		case CMD_GetVideoEncoderOption:
			ret = onvif_GetVideoEncoderOption(self, fd, buff_rcv);
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
		case CMD_GetAudioSourceOption:
			ret = onvif_GetAudioSourceOption(self, fd, buff_rcv);
			break;
		case CMD_GetAudioEncoderConfiguration:
			ret = onvif_GetAudioEncoderConfiguration(self, fd, buff_rcv);
			break;
		case CMD_GetAudioEncoderConfigurations:
			ret = onvif_GetAudioEncoderConfigurations(self, fd, buff_rcv);
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
		case CMD_GetConfigurationOptions:
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
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
			ret = onvif_ContinuousMove(self, fd, buff_rcv);
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
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
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
			ret = onvif_FindEvent(self, fd, buff_rcv);
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
			ret = onvif_GetEventSearchResults(self, fd, buff_rcv);
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
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			return ONVIF_ERR_RET_INTERNAL;
		}

		g_warning("%s error ret[%d] header.code[%d]", __FUNCTION__, ret,
				header.code);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (ret : %d) (Line : %d)",
				__FUNCTION__, ret, __LINE__);
		g_return_val_if_fail(ret == ONVIF_ERR_RET_SUCCESS,
				ONVIF_ERR_RET_INTERNAL);
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

	//ver.2.6
	//nf_notify_fire_params("sysdb_change", cate, 0, 0, 0);
	//nf_sysdb_save(_sysdb_cate_list[cate]);

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(gint)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
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

int onvif_CheckTokens(NfOnvif *self, int fd, char *buff_rcv) {
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
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Tokens)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetHostName(NfOnvif *self, int fd, char *buff_rcv) {
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
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_HostName)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetHostName(NfOnvif *self, int fd, char *buff_rcv) {
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

	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	DAL_set_ipSetup_data(&ipdata);
	nf_sysdb_save("net");
	//    sysdb_save_cate(NF_SYSDB_CATE_NET);
	//    DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);

	memcpy(packet.data, &tmp, sizeof(arg_HostName));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_HostName)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetDNS(NfOnvif *self, int fd, char *buff_rcv) {
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
	//	tmp.FromDHCP = nf_sysdb_get_bool("onvif.common.dns_dhcp");
	//	sprintf(tmp.SearchDomain, " ");
	/*
	 dns_ip = nf_sysdb_get_uint("net.proto.dns1");
	 if(dns_ip == 0)
	 memset(tmp.IPv4Address[0], 0x00, sizeof(tmp.IPv4Address[0]));
	 else
	 sprintf(tmp.IPv4Address[0], "%d.%d.%d.%d", PRINT_IP(dns_ip));

	 dns_ip = nf_sysdb_get_uint("net.proto.dns2");
	 if(dns_ip == 0)
	 memset(tmp.IPv4Address[1], 0x00, sizeof(tmp.IPv4Address[1]));
	 else
	 sprintf(tmp.IPv4Address[1], "%d.%d.%d.%d", PRINT_IP(dns_ip));
	 */
#if 0	   
	nf_netif_get_info(&ret_net_info);
	ddns1.sin_addr.s_addr = htonl(ret_net_info.dnsserver1);
	ddns2.sin_addr.s_addr = htonl(ret_net_info.dnsserver2);
	snprintf(tmp.IPv4Address[0], COMMON_SIZE - 1, "%s", inet_ntoa(ddns1.sin_addr));
	snprintf(tmp.IPv4Address[1], COMMON_SIZE - 1, "%s", inet_ntoa(ddns2.sin_addr));
#else
	if (tmp.FromDHCP) {
		//FIXME.baek
		sprintf(tmp.IPv4Address[0], "222.112.8.34");
		sprintf(tmp.IPv4Address[1], "168.126.63.1");
	} else {
		if (nf_sysdb_get_bool("net.proto.dhcpon")) {
			dns_ip1 = nf_sysdb_get_uint("net.dhcp.dns1");
			dns_ip2 = nf_sysdb_get_uint("net.dhcp.dns2");
		} else {
			dns_ip1 = nf_sysdb_get_uint("net.proto.dns1");
			dns_ip2 = nf_sysdb_get_uint("net.proto.dns2");
		}
		if (dns_ip1 == 0)
			memset(tmp.IPv4Address[0], 0x00, sizeof(tmp.IPv4Address[0]));
		else
			sprintf(tmp.IPv4Address[0], "%d.%d.%d.%d", PRINT_IP(dns_ip1));

		if (dns_ip2 == 0)
			memset(tmp.IPv4Address[1], 0x00, sizeof(tmp.IPv4Address[1]));
		else
			sprintf(tmp.IPv4Address[1], "%d.%d.%d.%d", PRINT_IP(dns_ip2));
	}
#endif
	snprintf(tmp.SearchDomain, COMMON_SIZE - 1, "%s",
			nf_sysdb_get_str_nocopy("onvif.common.search_domain"));
	_TTY_LOG_ONVIF("SDNS[%s]", tmp.SearchDomain);

	memcpy(packet.data, &tmp, sizeof(arg_DNS));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_DNS)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetDNS(NfOnvif *self, int fd, char *buff_rcv) {
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
	IPSetupData ipdata;
	memset(&ipdata, 0x00, sizeof(IPSetupData));

	/*Load current config */
	DAL_get_ipSetup_data(&ipdata);

	/* Set new config */
	if (tmp.FromDHCP) {
		//Set DHCP -> get new DNS -> set STATIC and new DNS
		//        set_dhcp_and_get_dns(&ipdata.dns1, &ipdata.dns2);
		//FIXME.baek
		prvIntToIP(ipdata.dns1, ip_str_to_uint("222.112.8.34"));
		prvIntToIP(ipdata.dns2, ip_str_to_uint("168.126.63.1"));
	} else {
		if (tmp.IPv4Address[0][0] != '\0') {
			prvIntToIP(ipdata.dns1, ip_str_to_uint(tmp.IPv4Address[0]));
		}
		if (tmp.IPv4Address[1][0] != '\0') {
			prvIntToIP(ipdata.dns2, ip_str_to_uint(tmp.IPv4Address[1]));
		}
	}
	nf_sysdb_set_str("onvif.common.search_domain", tmp.SearchDomain);
	nf_sysdb_set_bool("onvif.common.dns_dhcp", tmp.FromDHCP);

	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	DAL_set_ipSetup_data(&ipdata);
	nf_sysdb_save("net");
	//    sysdb_save_cate(NF_SYSDB_CATE_NET);
	//    DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);

	sleep(10);

	memcpy(packet.data, &tmp, sizeof(arg_DNS));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_DNS)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetNTP(NfOnvif *self, int fd, char *buff_rcv) {
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

	strncpy(tmp.NTPname, nf_sysdb_get_str_nocopy("sys.date.timesvr"),
			COMMON_SIZE - 1);

	len = strlen(tmp.NTPname);
	if (len >= COMMON_SIZE) {
		len = COMMON_SIZE - 1;
	}
	tmp.is_domain = 0;
	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>> get NTP [%s]", tmp.NTPname);
	for (i = 0; i < (int) len; i++) {
		if ((tmp.NTPname[i] >= 'a' && tmp.NTPname[i] <= 'z')
				|| (tmp.NTPname[i] >= 'A' && tmp.NTPname[i] <= 'Z')) {
			_TTY_LOG_ONVIF("ffffffffffffffffffffffffffff[%s][%d]",
					tmp.NTPname, i);
			tmp.is_domain = 1;
			break;
		}
	}
	tmp.FromDHCP = nf_sysdb_get_bool("onvif.common.ntp_dhcp");

	memcpy(packet.data, &tmp, sizeof(arg_NTP));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NTP)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetNTP(NfOnvif *self, int fd, char *buff_rcv) {
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
	int ret = 0;
	DateTimeData dtdata;
	GTimeVal post_time;

	if (tmp.NTPname[0] == '\0') {
		return ONVIF_R_ERR;
	}

	DAL_get_dateTime_data(&dtdata);

	dtdata.auto_timesync = 1;
	memset(dtdata.timeServer, 0x00, sizeof(char) * 32);

	nf_sysdb_set_bool("onvif.common.ntp_dhcp", tmp.FromDHCP);
	if (tmp.FromDHCP) {
		strncpy(dtdata.timeServer, "pool.ntp.org", strlen("pool.ntp.org"));
	} else {
		if (tmp.NTPname[0] == '\0') {
			return ONVIF_R_ERR;
		}
		strncpy(dtdata.timeServer, tmp.NTPname, sizeof(char) * 32 - 1);
	}
	ret = scm_get_ntp_time(dtdata.timeServer, &post_time);
	if (ret < 0 || post_time.tv_sec == 0) {
		_TTY_LOG_ONVIF("NTP sync fail!!");
		//        tmp.result = -2;
		//        return;
		post_time.tv_sec = time(NULL);
	}
	scm_set_system_time(&post_time);

	scm_put_log(CHANGE_SYS_TIME, 0, 0);
	DAL_set_dateTime_data(&dtdata);
	scm_init_timesync();
//	VW_SetupSystem_set_changeflag(1);
	smt_set_service(SMT_TIME_CHANGE);
	scm_apply_time(IRET_SCM_CHANGE_SYSTEM_TIME);

	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_NTP));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NTP)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetDateTime(NfOnvif *self, int fd, char *buff_rcv) {
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
	tmp.Manual_NTP = (int) nf_sysdb_get_uint("sys.date.auto_sync");
	tmp.DST = nf_sysdb_get_bool("sys.date.daylight");
	_TTY_LOG_ONVIF("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< DST[%d] dt[%d]",
			tmp.DST, dtdata.dst);
	_TTY_LOG_ONVIF("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< SYNC[%d] dt[%d]",
			tmp.Manual_NTP, dtdata.auto_timesync);
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
	_TTY_LOG_ONVIF("y[%d] m[%d] d[%d] h[%d] m[%d] s[%d]\n",
			tmp.Year, tmp.Month, tmp.Day, tmp.Hour, tmp.Minute, tmp.Second);

	tz_idx = nf_sysdb_get_uint("sys.date.tz_index");
	if (tz_idx >= (unsigned int) nf_zoneinfo_get_count()) {
		return ONVIF_R_ERR;
	}

	strncpy(tz_name, nf_zoneinfo_get_string((gint) tz_idx),
			sizeof(tz_name) - 1);
	nf_zoneinfo_get_activex_posix_onvif(tz_name, &tzout);

	if (tzout.tz_cnt > 1) {
		snprintf(tmp.TimeZone, COMMON_SIZE, "%s",
				tzout.tz_arr[tmp.loYear - 2008].string);
	} else {
		snprintf(tmp.TimeZone, COMMON_SIZE, "%s", tzout.tz_arr[0].string);
	}

	memcpy(packet.data, &tmp, sizeof(arg_DateTime));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_DateTime)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetDateTime(NfOnvif *self, int fd, char *buff_rcv) {
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
	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DST[%d]dt[%d]",
			tmp.DST, dtdata.dst);
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
			tm_ptr.tm_mon = tmp.Month;
			tm_ptr.tm_mday = tmp.Day;
			tm_ptr.tm_hour = tmp.Hour;
			tm_ptr.tm_min = tmp.Minute;
			tm_ptr.tm_sec = tmp.Second;
			tm_ptr.tm_isdst = tmp.DST;
			post_time.tv_sec = mktime(&tm_ptr);
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
		scm_set_system_time(&post_time);
	}
	//sprintf(dtdata.timeServer, "www.test.ggg");
	scm_put_log(CHANGE_SYS_TIME, 0, 0);
	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> DST[%d]dt[%d]",
			tmp.DST, dtdata.dst);
	DAL_set_dateTime_data(&dtdata);
	scm_init_timesync();
//	VW_SetupSystem_set_changeflag(1);
	smt_set_service(SMT_TIME_CHANGE);
	scm_apply_time(IRET_SCM_CHANGE_SYSTEM_TIME);

	memcpy(packet.data, &tmp, sizeof(arg_DateTime));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_DateTime)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetDeviceInformation(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_GetDeviceInformation tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetDeviceInformation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetDeviceInformation));
 
	// To DO
	snprintf(tmp.FirmwareVersion, sizeof(char)*COMMON_SIZE, "%s", g_strstrip(nf_sysdb_get_str_nocopy("sys.info.swver")));
	snprintf(tmp.HardwareId, sizeof(char)*COMMON_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.model"));
	snprintf(tmp.Manufacturer, sizeof(char)*COMMON_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.vendor"));
	snprintf(tmp.Model, sizeof(char)*COMMON_SIZE, "%s", get_onvif_model_name());
	snprintf(tmp.SerialNumber, sizeof(char)*COMMON_SIZE, "%s", 	nf_sysdb_get_str_nocopy("sys.info.hwver"));

	memcpy(packet.data, &tmp, sizeof(arg_GetDeviceInformation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_GetDeviceInformation)) 	!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_FactoryDefault(NfOnvif *self, int fd, char *buff_rcv) {
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

	// To DO
	smt_set_service(SMT_FAC_DEFAULT);
	scm_run_factory_default(IRET_FACTORY_DEFAULT_NOTIFY);

	memcpy(packet.data, &tmp, sizeof(arg_FactoryDefault));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_FactoryDefault)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_reboot(void)
{
	nf_sysman_poweroff(1);
	
	return 1;
}

int onvif_SystemReboot(NfOnvif *self, int fd, char *buff_rcv) {
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

	memcpy(packet.data, &tmp, sizeof(arg_SystemReboot));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_SystemReboot)) != ONVIF_ERR_RET_SUCCESS) {
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
	memset(&mac_addr, 0x00, sizeof(NF_NETIF_MAC));
	nf_netif_get_mac(&mac_addr);
	sprintf(tmp.macaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
			mac_addr.mac_addr[0] & 0xFF, mac_addr.mac_addr[1] & 0xFF,
			mac_addr.mac_addr[2] & 0xFF, mac_addr.mac_addr[3] & 0xFF,
			mac_addr.mac_addr[4] & 0xFF, mac_addr.mac_addr[5] & 0xFF);

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
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NetworkInfo)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetNetworkInfo(NfOnvif *self, int fd, char *buff_rcv) {
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
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->ip [%d][%d][%d][%d]",
			ipdata.ip[0], ipdata.ip[1], ipdata.ip[2], ipdata.ip[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->gateway [%d][%d][%d][%d]",
			ipdata.gateway[0], ipdata.gateway[1], ipdata.gateway[2], ipdata.gateway[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->subnet [%d][%d][%d][%d]",
			ipdata.subnet[0], ipdata.subnet[1], ipdata.subnet[2], ipdata.subnet[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->dns1[%d][%d][%d][%d]",
			ipdata.dns1[0], ipdata.dns1[1], ipdata.dns1[2], ipdata.dns1[3]);
	_TTY_LOG_ONVIF("prvIntToIP(ipdata->dns2[%d][%d][%d][%d]",
			ipdata.dns2[0], ipdata.dns2[1], ipdata.dns2[2], ipdata.dns2[3]);
	_TTY_LOG_ONVIF("ipdata->netPort[%d]", ipdata.netPort);
	_TTY_LOG_ONVIF("ipdata->webPort[%d]", ipdata.webPort);
	_TTY_LOG_ONVIF("ipdata->txSpeed[%d]", ipdata.txSpeed);

	memcpy(packet.data, &tmp, sizeof(arg_NetworkInfo));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NetworkInfo)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	sleep(3);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	DAL_set_ipSetup_data(&ipdata);
	nf_sysdb_save("net");
	//DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
	sleep(5);
	excute_later_secs(multi_hello, 10);
	
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetNetworkPort(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NetworkPort tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_NetworkPort);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_NetworkPort));

	// To DO
	tmp.http_port = (int)nf_sysdb_get_uint("net.proto.webport");
	tmp.rtsp_port = (int)nf_sysdb_get_uint("net.rtp.rtspport");
	tmp.http_enable = (int)nf_sysdb_get_bool("onvif.common.http_enable");
	tmp.rtsp_enable = (int)nf_sysdb_get_bool("onvif.common.rtsp_enable");

	memcpy(packet.data, &tmp, sizeof(arg_NetworkPort));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NetworkPort)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetNetworkPort(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_NetworkPort tmp;

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

	/* Set New value */
	_TTY_LOG_ONVIF("tmp.http_port[%d]", tmp.http_port);
	_TTY_LOG_ONVIF("tmp.http_enable[%d]", tmp.http_enable);
	_TTY_LOG_ONVIF("tmp.rtsp_port[%d]", tmp.rtsp_port);
	_TTY_LOG_ONVIF("tmp.rtsp_enable[%d]", tmp.rtsp_enable);

	if (tmp.http_port != -1)
		ipdata.webPort = (guint)tmp.http_port;

	if (tmp.http_enable != -1) {
		ipdata.webServ = (guint)tmp.http_enable;
		nf_sysdb_set_bool("onvif.common.http_enable", tmp.http_enable);
	}
	if (tmp.rtsp_port != -1)
		ipdata.rtspport = (guint)tmp.rtsp_port;

	if (tmp.rtsp_enable != -1) {
		nf_sysdb_set_bool("onvif.common.rtsp_enable", tmp.rtsp_enable);
	}
	/* Apply & Save db */
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	DAL_set_ipSetup_data(&ipdata);
	nf_sysdb_save("net");
	//sysdb_save_cate(NF_SYSDB_CATE_NET);
	//DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);
	scm_apply_netinfo_by_db(IRET_SCM_APPLY_NETINFO);
	sleep(5);
	excute_later_secs(multi_hello, 10);

	memcpy(packet.data, &tmp, sizeof(arg_NetworkPort));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_NetworkPort)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int get_scope(arg_Scope *tmp) 
{
	int scope_cnt;
	char buff[64] = { 0, };
	int i;

	scope_cnt = (int) nf_sysdb_get_uint("onvif.common.scope_cnt");

	for (i = 0; i < scope_cnt; i++) {
		sprintf(buff, "onvif.scope%d.name", i);
		strncpy(tmp->scope[i], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		sprintf(buff, "onvif.scope%d.fixed", i);
		tmp->scope_fixed[i] = (int) nf_sysdb_get_uint(buff);
	}
	tmp->scope_cnt = (char) scope_cnt;
	
	return 1;
}

int onvif_GetScope(NfOnvif *self, int fd, char *buff_rcv) {
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
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetScope(NfOnvif *self, int fd, char *buff_rcv) {
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
	char buff[COMMON_SIZE] = { 0, };
	char tmp_scope[COMMON_SIZE] = { 0, };
	int fixed;
	int i, cnt, fixed_cnt = 0;

	tmp.result = 0;
	cnt = 0;
	//printf("start [%d]\n", tmp.scope_cnt);
	_TTY_LOG_ONVIF("start [%d]", tmp.scope_cnt);

	for (i = 0; i < MAX_SCOPE; i++) {
		sprintf(buff, "onvif.scope%d.fixed", i);
		fixed = (int) nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.scope%d.name", i);
		strncpy(tmp_scope, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (fixed) {
			for (cnt = 0; cnt < tmp.scope_cnt; cnt++) {
				if (!strcmp(tmp_scope, tmp.scope[cnt])) {
					return ONVIF_R_ERR;
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
				_TTY_LOG_ONVIF("i[%s]", tmp.scope[cnt]);
				nf_sysdb_set_str(buff, tmp.scope[cnt]);
				cnt++;
			} else {
				nf_sysdb_set_str(buff, "");
			}
		} else
			fixed_cnt++;
	}

	if (cnt == tmp.scope_cnt) {
		;
	} else {
		return ONVIF_R_ERR;
	}
	_TTY_LOG_ONVIF("end [%d][%d]", fixed_cnt, tmp.scope_cnt);
	nf_sysdb_set_uint("onvif.common.scope_cnt", (unsigned int)(fixed_cnt + tmp.scope_cnt));

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
	char buff[COMMON_SIZE] = { 0, };
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

	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_timer_add(3, (GSourceFunc) multi_hello, NULL);

	memcpy(packet.data, &tmp, sizeof(arg_Scope));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveScope(NfOnvif *self, int fd, char *buff_rcv) {
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
	char buff[COMMON_SIZE] = { 0, };
	char tmp_scope[COMMON_SIZE] = { 0, };
	char org_scope[COMMON_SIZE] = { 0, };
	char next_scope[COMMON_SIZE] = { 0, };
	int i, j, k;
	int fixed, change;

	fixed = change = 0;
	scope_cnt = (int) nf_sysdb_get_uint("onvif.common.scope_cnt");

	for (k = 0; k < tmp.scope_cnt; k++) {
		for (i = 0; i < scope_cnt; i++) {
			sprintf(buff, "onvif.scope%d.name", i);
			strncpy(tmp_scope, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
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
					strncpy(next_scope, nf_sysdb_get_str_nocopy(buff),
							COMMON_SIZE - 1);
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

	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	nf_timer_add(3, (GSourceFunc) multi_hello, NULL);

	memcpy(packet.data, &tmp, sizeof(arg_Scope));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Scope)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetDiscovery(NfOnvif *self, int fd, char *buff_rcv) {
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

int onvif_SetDiscovery(NfOnvif *self, int fd, char *buff_rcv) {
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
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_Discovery));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Discovery)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_Passfd(const char *data, int pass_fd) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	return 1;
}

int onvif_GetUser(NfOnvif *self, int fd, char *buff_rcv) {
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

int onvif_SetUser(NfOnvif *self, int fd, char *buff_rcv) {
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
	nf_sysdbchange_make_authfile(man);

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
int onvif_CreateUser(NfOnvif *self, int fd, char *buff_rcv) {
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
	nf_sysdbchange_make_authfile(man);
	_TTY_LOG_ONVIF(">>>>>>>>>>  create user done!!");

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_DeleteUser(NfOnvif *self, int fd, char *buff_rcv) {
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
				if (strncmp("ADMIN", tmp.name[i], sizeof(tmp.name[i])) == 0) {
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
	nf_sysdbchange_make_authfile(new_man);

	memcpy(packet.data, &tmp, sizeof(arg_UserType));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_UserType)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetCapability(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_GetCapa tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetCapa);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetCapa));

	// To DO
	tmp.is_audio = 1;
	tmp.is_focus = 1;
	tmp.is_zoom = 0;
	tmp.is_motion = 1;
	tmp.alarmin = ONVIF_ALARMIN_CNT;
	tmp.relay = ONVIF_RELAY_CNT;

	memcpy(packet.data, &tmp, sizeof(arg_GetCapa));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetCapa)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
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

	//g_message("%s IN", __FUNCTION__);
	tmp.relay_cnt = ONVIF_RELAY_CNT;

	for (i = 0; i < tmp.relay_cnt; i++) {
		sprintf(buff, "onvif.relay%d.token", i);
		strncpy(tmp.relay[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		sprintf(buff, "onvif.relay%d.mode", i);
		tmp.relay[i].mode = (int)nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.relay%d.duration", i);
		tmp.relay[i].duration = (int)nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.relay%d.idle_state", i);
		tmp.relay[i].idle_state = (int)nf_sysdb_get_uint(buff);
	}

	memcpy(packet.data, &tmp, sizeof(arg_Relays));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Relays)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetRelaySettings(NfOnvif *self, int fd, char *buff_rcv) {
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
				sprintf(buff, "act.relay.R%d.act", i);
				nf_sysdb_set_bool(buff, TRUE);  // auto
			} else {
				sprintf(buff, "act.relay.R%d.act", i);
				nf_sysdb_set_bool(buff, FALSE); // manual (bistable)
			}

			if (tmp.duration == 0) {
				//nf_sysdb_set_uint("act.relay.R0.dwell_type", 1); //sync
				;
			} else {
				sprintf(buff, "act.relay.R%d.dwell_time", i);
				nf_sysdb_set_uint(buff, (unsigned int)tmp.duration);
			}

			if (tmp.idle_state == ONVIF_closed) {
				//nf_notify_fire_params("usr_alarm", 1, 0, 0, 0);
				sprintf(buff, "act.relay.R%d.op_type", i);
				nf_sysdb_set_uint(buff, 1); //NC
			} else {
				//nf_notify_fire_params("usr_alarm", 0, 0, 0, 0);
				sprintf(buff, "act.relay.R%d.op_type", i);
				nf_sysdb_set_uint(buff, 0); //NO
			}
			sprintf(buff, "act.relay.R%d.dwell_type", i);
			nf_sysdb_set_uint(buff, 0); //latched
			sprintf(buff, "act.relay.R%d.motion", i);
			nf_sysdb_set_str(buff, map_buff);

			sprintf(buff, "onvif.relay%d.mode", i);
			nf_sysdb_set_uint(buff, (unsigned int)tmp.mode);
			sprintf(buff, "onvif.relay%d.duration", i);
			nf_sysdb_set_uint(buff, (unsigned int)tmp.duration);
			sprintf(buff, "onvif.relay%d.idle_state", i);
			nf_sysdb_set_uint(buff, (unsigned int)tmp.idle_state);

			/* Set Relay */
			sprintf(buff, "act.relay.R%d.op_type", i);
			nf_action_relay_test(1, i, (int)nf_sysdb_get_uint(buff));

			tmp.result = 1;
		}
	}

	if (tmp.result == 1) {
		nf_sysdb_set_bool("onvif.lock.alarm_out", 1);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
		sysdb_save_cate(NF_SYSDB_CATE_ACT);
	} else {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Relay));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Relay)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

pthread_t set_relay_thread_id = 0;

static void usr_alarm_inactive() {
	unsigned duration = nf_sysdb_get_uint("onvif.relay0.duration");
	sleep(duration);
	nf_notify_fire_params("usr_alarm", 0, 0, 0, 0);
}

int onvif_SetRelayState(NfOnvif *self, int fd, char *buff_rcv) {
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
	//    nf_sysdb_set_str("act.relay.R0.usr_alarm", mask_buff);
	tmp.result = 0;
	for (i = 0; i < relay_cnt; i++) {
		sprintf(buff, "onvif.relay%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			sprintf(buff, "onvif.relay%d.mode", i);
			if (nf_sysdb_get_uint(buff) == ONVIF_Monostable) {
				if (tmp.curr_state == ONVIF_active) {
					nf_notify_fire_params("usr_alarm", 1, 0, 0, 0);
					if (set_relay_thread_id != 0) {
						if (pthread_join(set_relay_thread_id, NULL) != 0)
							printf("set_relay_thread join fail\n");
						else
							printf("set_relay_thread join ok\n");
						set_relay_thread_id = 0;
					}
					if (pthread_create(&set_relay_thread_id, NULL,
							(void*) usr_alarm_inactive, NULL) != 0) {
						printf("set_relay_thread create error\n");
					}
				} else {
					//nf_notify_fire_params("usr_alarm", 0, 0, 0, 0);
				}
			} else {
				if (tmp.curr_state == ONVIF_active) {
					sprintf(buff, "onvif.relay%d.idle_state", i);
					if (nf_sysdb_get_uint(buff) == ONVIF_closed) {
						//nf_sysdb_set_uint("act.relay%d.manual", 0); //open
					} else {
						//nf_sysdb_set_uint("act.relay.manual", 1); // close
					}
				} else {
					sprintf(buff, "onvif.relay%d.idle_state", i);
					if (nf_sysdb_get_uint(buff) == ONVIF_closed) {
						//nf_sysdb_set_uint("act.relay.manual", 1); // close
					} else {
						//nf_sysdb_set_uint("act.relay.manual", 0); // open
					}
				}
				//nf_notify_fire_params("sysdb_change", 4, 0, 0, 0);
			}
			tmp.result = 1;
		}
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_Relay));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_Relay)) != ONVIF_ERR_RET_SUCCESS) {
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
static int onvif_Profile_GetVideoSource(arg_ProfileDB *tmp) {
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
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			nf_sysdb_set_str(buff, tmp.token);

			sprintf(buff, "onvif.profile%d.name", i);
			nf_sysdb_set_str(buff, tmp.name);

			// add default encoders & sources
			profile_cnt++;
			nf_sysdb_set_uint("onvif.common.profile_cnt", (unsigned int)profile_cnt);
			tmp.result = 1;
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

int onvif_AddAudioSource(NfOnvif *self, int fd, char *buff_rcv) {
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
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
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
	int profile_cnt, usecount = 0;
	int i;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (profile_cnt : %d)", __FUNCTION__, profile_cnt);
	int token_num = findfieldStringCount("onvif.profile%d.token",
			tmp.node_token, ONVIF_MAX_PROFILE_CNT);
	if (token_num != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		sprintf(buff, "onvif.profile%d.ptz", token_num);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		profile_cnt = ONVIF_PTZ_CNT;
		token_num = findfieldStringCount("onvif.ptz%d.token", tmp_token,
				profile_cnt);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		sprintf(buff, "onvif.ptz%d.node_token", token_num);
		nf_sysdb_set_str(buff, tmp.node_token);
		sprintf(buff, "onvif.ptz%d.token", token_num);
		nf_sysdb_set_str(buff, tmp.token);
		sprintf(buff, "onvif.ptz%d.name", token_num);
		nf_sysdb_set_str(buff, tmp.token);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	} else {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_PTZNode));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_CreateRecording(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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
int onvif_CreateRecordingJob(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RecordingJobConfiguration tmp;



	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RecordingJobConfiguration);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RecordingJobConfiguration));

	// 1. recording_token을 보고 빈자리 검사
	// 레코딩 토큰이 없으면 자리에 채워줌
	// 있으면 최대 개수를 도달했다고 하고 리턴

	/*
	'<item key="onvif.recordingjob0.token"       type="STRING"      min="0" max="64" val="jt0" />'
	'<item key="onvif.recordingjob0.recording_token"       type="STRING"      min="0" max="64" val="rs0" />'
	'<item key="onvif.recordingjob0.mode"       type="UINT"      min="0" max="4096" val="" />'
	'<item key="onvif.recordingjob0.priority"       type="UINT"      min="0" max="4096" val="" />'

	'<item key="onvif.source0.job_token"       type="STRING"      min="0" max="64" val="jt0" />'
	'<item key="onvif.source0.token0"       type="STRING"      min="0" max="64" val="" />'
	'<item key="onvif.source0.dest0"       type="STRING"      min="0" max="64" val="" />'
	'<item key="onvif.source0.token1"       type="STRING"      min="0" max="64" val="" />'
	'<item key="onvif.source0.dest1"       type="STRING"      min="0" max="64" val="" />'
	'<item key="onvif.source0.token2"       type="STRING"      min="0" max="64" val="" />'
	'<item key="onvif.source0.dest2"       type="STRING"      min="0" max="64" val="" />'
	'<item key="onvif.source0.autoCreateReceiver"       type="UINT"      min="0" max="64" val="" />'
	 */

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_RecordingJobConfiguration));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RecordingJobConfiguration))
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

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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
int onvif_DeleteRecordingJob(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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
int onvif_GetRecordingConfiguration(NfOnvif *self, int fd, char *buff_rcv) {

	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RecordingConfiguration tmp;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RecordingConfiguration);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RecordingConfiguration));


	// in here you should put the custom code.
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		if(!strcmp(tmp.RecordingToken, nf_sysdb_get_str_nocopy(buff)))
		{
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			sprintf(buff, "onvif.recording%d.id", i);			
			strncpy(tmp.recordingConf.Source.SourceId, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
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
			strncpy(tmp.recordingConf.Source.Address, nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
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
	memcpy(packet.data, &tmp, sizeof(arg_RecordingConfiguration));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RecordingConfiguration))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
void getRecordingJobConfiguration(arg_JobItem* tmp)
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
				mode = (int)nf_sysdb_get_uint(buff);
				getJobModeString(mode,&tmp->conf.RecordingJobMode);
				_TTY_LOG_ONVIF_DEBUG("tmp->conf.RecordingJobMode : %s",tmp->conf.RecordingJobMode);

				
				sprintf(buff, "onvif.recordingjob%d.priority", i);

				tmp->conf.Priority = (int)nf_sysdb_get_uint(buff);
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)tmp->conf.Priority %d",__FUNCTION__, __LINE__,tmp->conf.Priority);	
	
				for(j=0;j<ONVIF_RECORDING_CNT;j++) // FIX ME:
				{
					sprintf(buff, "onvif.source%d.token%d", i,j);
					strncpy(tmp->conf.source[j].SourceToken,nf_sysdb_get_str_nocopy(buff),COMMON_SIZE-1);				
					strncpy(tmp->conf.source[j].SourceTokenType,"http://www.onvif.org/ver10/schema/Receiver", COMMON_SIZE - 1);
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)tmp->conf.source[i].SourceToken %s",__FUNCTION__, __LINE__,tmp->conf.source[i].SourceToken);
					/*
					if(tmp->conf.source[j].SourceToken[0] == '\0')
					{
						tmp->conf.sourceSize = j;
						break;
						
					}
					*/
					sprintf(buff, "onvif.source%d.dest%d", i,j);
					strncpy(tmp->conf.source[j].track.Destination,nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);										
				}
				if(j == ONVIF_RECORDING_CNT)// FIX ME:
				{
					tmp->conf.sourceSize = j;			
				}
				
	
				return 0;
							
				
			}
		}
	return ONVIF_R_ERR_NO_RECORDINGJOBS;
}

int onvif_GetRecordingJobConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_JobItem tmp;
	int mode=0;
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	g_message( "%s IN", __FUNCTION__);

		memset(&header, 0, sizeof(header));
		memcpy(&header, buff_rcv, sizeof(header));
	
		packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_JobItem);
	
		memcpy(&packet, buff_rcv, packet_len);
		memcpy(&tmp, packet.data, sizeof(arg_JobItem));
	
		/*
		'<item key="onvif.recordingjob0.token"		 type="STRING"		min="0" max="64" val="jt0" />'
		'<item key="onvif.recordingjob0.recording_token"	   type="STRING"	  min="0" max="64" val="rs0" />' 
		'<item key="onvif.recordingjob0.mode"		type="UINT" 	 min="0" max="4096" val="" />' 
		'<item key="onvif.recordingjob0.priority"		 type="UINT"	  min="0" max="4096" val="" />' 
		
		'<item key="onvif.source0.job_token"	   type="STRING"	  min="0" max="64" val="jt0" />' 
		'<item key="onvif.source0.token0"		type="STRING"	   min="0" max="64" val="" />' 
		'<item key="onvif.source0.dest0"	   type="STRING"	  min="0" max="64" val="" />' 
		'<item key="onvif.source0.token1"		type="STRING"	   min="0" max="64" val="" />' 
		'<item key="onvif.source0.dest1"	   type="STRING"	  min="0" max="64" val="" />' 
		'<item key="onvif.source0.token2"		type="STRING"	   min="0" max="64" val="" />' 
		'<item key="onvif.source0.dest2"	   type="STRING"	  min="0" max="64" val="" />' 
		'<item key="onvif.source0.autoCreateReceiver"		type="UINT" 	 min="0" max="64" val="" />' 
		
		
		*/
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
void recordingJobSource(char* jobToken,arg_RecordingJobSource* source)
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
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.source%d.job_token", i);
		if(!strcmp(jobToken,nf_sysdb_get_str_nocopy(buff)))
		{
			for (j = 0; j < ONVIF_JOBSOURCE_CNT; j++) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
				sprintf(buff, "onvif.source%d.token%d", i, j);
				strncpy(source[j].SourceToken,nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				strncpy(source[j].SourceTokenType,"http://www.onvif.org/ver10/schema/Receiver", COMMON_SIZE - 1);
				strncpy(source[j].track.SourceTag,"", COMMON_SIZE - 1);
				sprintf(buff, "onvif.source%d.dest%d", i, j);
				strncpy(source[j].track.Destination,nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			}

		}
	}
}
int onvif_GetRecordingJobs(NfOnvif *self, int fd, char *buff_rcv) {
	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_getRecordingJobsResponseItem tmp;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_getRecordingJobsResponseItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_getRecordingJobsResponseItem));

	memcpy(packet.data, &tmp, sizeof(arg_getRecordingJobsResponseItem));

	// in here you should put the custom code.
/*
'<item key="onvif.recordingjob0.token"       type="STRING"      min="0" max="64" val="jt0" />'
'<item key="onvif.recordingjob0.recording_token"       type="STRING"      min="0" max="64" val="rs0" />'
'<item key="onvif.recordingjob0.mode"       type="UINT"      min="0" max="4096" val="" />'
'<item key="onvif.recordingjob0.priority"       type="UINT"      min="0" max="4096" val="" />'
 */
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (i : %d)",__FUNCTION__, i);
		sprintf(buff, "onvif.recordingjob%d.token", i);
		strncpy(tmp.job[i].JobToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
						_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		sprintf(buff, "onvif.recordingjob%d.recording_token", i);
		strncpy(tmp.job[i].conf.RecordingToken, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		sprintf(buff, "onvif.recordingjob%d.mode", i);


		getJobModeString(nf_sysdb_get_uint(buff),&tmp.job[i].conf.RecordingJobMode);
		_TTY_LOG_ONVIF_DEBUG("@ tmp.job[i].conf.RecordingJobMode: %s (nf_sysdb_get_uint : %d)",tmp.job[i].conf.RecordingJobMode, nf_sysdb_get_uint(buff));
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		sprintf(buff, "onvif.recordingjob%d.priority", i);
		tmp.job[i].conf.Priority = (int)nf_sysdb_get_uint(buff);
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		recordingJobSource(tmp.job[i].JobToken,tmp.job[i].conf.source);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	}



	memcpy(packet.data, &tmp, sizeof(arg_getRecordingJobsResponseItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_getRecordingJobsResponseItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
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
	return 1;
}


int onvif_GetRecordingJobState(NfOnvif *self, int fd, char *buff_rcv) {
	int i=0,j=0;
	char buff[COMMON_SIZE] = { 0, };
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_JobItem tmp;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_JobItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_JobItem));

	/*
	'<item key="onvif.recordingjob0.token"       type="STRING"      min="0" max="64" val="jt0" />'
	'<item key="onvif.recordingjob0.recording_token"       type="STRING"      min="0" max="64" val="rs0" />'
	'<item key="onvif.recordingjob0.mode"       type="UINT"      min="0" max="4096" val="" />'
	'<item key="onvif.recordingjob0.priority"       type="UINT"      min="0" max="4096" val="" />'
	 */
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recordingjob%d.token", i);
		if(!strcmp(tmp.JobToken,nf_sysdb_get_str_nocopy(buff)))
		{
			sprintf(buff, "onvif.recordingjob%d.recording_token", i);
			strncpy(tmp.conf.RecordingToken,nf_sysdb_get_str_nocopy(buff),COMMON_SIZE -1);
			sprintf(buff, "onvif.recordingjob%d.mode", i);			
			getJobModeString((int)nf_sysdb_get_uint(buff), tmp.conf.state);
			_TTY_LOG_ONVIF_DEBUG("tmp.conf.state: %s (Line : %d)",tmp.conf.state, __LINE__);
			recordingJobSourceState(tmp.JobToken,&tmp.conf);
			goto complete;
		}
	}
	if(i == ONVIF_MAX_RECORDING_CNT)
	{
		return ONVIF_R_ERR_NO_RECORDINGJOBS;
	}

	complete:
	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_JobItem));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_JobItem))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetRecordingOptions(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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
int onvif_GetTrackConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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
int onvif_SetRecordingConfiguration(NfOnvif *self, int fd, char *buff_rcv) {

	int i=0,j=0;
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };


	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_RecordingConfiguration tmp;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RecordingConfiguration);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RecordingConfiguration));

/*
'<item key="onvif.recording0.token"               type="STRING"      min="0" max="64" val="rs0" />' 
'<item key="onvif.recording0.id"               type="STRING"      min="0" max="128" val="" />'
'<item key="onvif.recording0.name"               type="STRING"      min="0" max="64" val="Recording0" />'
'<item key="onvif.recording0.location"               type="STRING"      min="0" max="64" val="" />'
'<item key="onvif.recording0.desc"               type="STRING"      min="0" max="64" val="" />'
'<item key="onvif.recording0.address"               type="STRING"      min="0" max="128" val="" />'
'<item key="onvif.recording0.content"               type="STRING"      min="0" max="64" val="" />'
'<item key="onvif.recording0.max_retention_time"               type="UINT"      min="0" max="4096" val="" />'
*/


	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);

		if(!strcmp(tmp.RecordingToken, nf_sysdb_get_str_nocopy(buff)))
		{
			sprintf(buff, "onvif.recording%d.id", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.SourceId);
			_TTY_LOG_ONVIF_DEBUG("1 : %s",tmp.recordingConf.Source.SourceId);
			sprintf(buff, "onvif.recording%d.name", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Name);
						_TTY_LOG_ONVIF_DEBUG("2 : %s",tmp.recordingConf.Source.Name);
			sprintf(buff, "onvif.recording%d.location", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Location);
			_TTY_LOG_ONVIF_DEBUG("3 : %s",tmp.recordingConf.Source.Location);
			sprintf(buff, "onvif.recording%d.desc", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Description);
					_TTY_LOG_ONVIF_DEBUG("4 : %s",tmp.recordingConf.Source.Description);
			sprintf(buff, "onvif.recording%d.address", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Source.Address);
			sprintf(buff, "onvif.recording%d.content", i);			
			nf_sysdb_set_str(buff, tmp.recordingConf.Content);
			sprintf(buff, "onvif.recording%d.max_retention_time", i);			
			nf_sysdb_set_uint(buff, (unsigned int)tmp.recordingConf.MaximumRetentionTime);
						_TTY_LOG_ONVIF_DEBUG("3 : %d",tmp.recordingConf.MaximumRetentionTime);


			event_status_shm_write(OV_PROP_CONFIGURATION_CHANGE, 1);		
			event_msgbuf_shm_append(OV_PROP_CONFIGURATION_CHANGE, (unsigned int)i, 1);	

			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			goto completed;

		}
			

		
	}



	completed:

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_RecordingConfiguration));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RecordingConfiguration))
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

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_JobItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_JobItem));

	/*
	'<item key="onvif.recordingjob0.token"		 type="STRING"		min="0" max="64" val="jt0" />'
	'<item key="onvif.recordingjob0.recording_token"	   type="STRING"	  min="0" max="64" val="rs0" />' 
	'<item key="onvif.recordingjob0.mode"		type="UINT" 	 min="0" max="4096" val="" />' 
	'<item key="onvif.recordingjob0.priority"		 type="UINT"	  min="0" max="4096" val="" />' 
	
	'<item key="onvif.source0.job_token"	   type="STRING"	  min="0" max="64" val="jt0" />' 
	'<item key="onvif.source0.token0"		type="STRING"	   min="0" max="64" val="" />' 
	'<item key="onvif.source0.dest0"	   type="STRING"	  min="0" max="64" val="" />' 
	'<item key="onvif.source0.token1"		type="STRING"	   min="0" max="64" val="" />' 
	'<item key="onvif.source0.dest1"	   type="STRING"	  min="0" max="64" val="" />' 
	'<item key="onvif.source0.token2"		type="STRING"	   min="0" max="64" val="" />' 
	'<item key="onvif.source0.dest2"	   type="STRING"	  min="0" max="64" val="" />' 
	'<item key="onvif.source0.autoCreateReceiver"		type="UINT" 	 min="0" max="64" val="" />' 
	
		
	*/
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
				nf_sysdb_set_str(buff,tmp.conf.source[i].track.Destination);										
			
			}

			break;
						
			
		}
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
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



int onvif_SetRecordingJobMode(NfOnvif *self, int fd, char *buff_rcv) {
	int i=0,log_type=0,mode=0,dbmode=0;
	char buff[COMMON_SIZE] = { 0, };
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_JobItem tmp;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_JobItem);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));

/*
'<item key="onvif.recordingjob0.token"       type="STRING"      min="0" max="64" val="jt0" />'
'<item key="onvif.recordingjob0.recording_token"       type="STRING"      min="0" max="64" val="rs0" />' 
'<item key="onvif.recordingjob0.mode"       type="UINT"      min="0" max="4096" val="" />' 
'<item key="onvif.recordingjob0.priority"       type="UINT"      min="0" max="4096" val="" />' 
'<item key="onvif.recordingjob0.state"       type="STRING"      min="0" max="64" val="" />' 

*/
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
			sprintf(buff, "onvif.recordingjob%d.token", i);

			//1. jobToken을 서치 한다.
			if(!strcmp(tmp.JobToken,nf_sysdb_get_str_nocopy(buff)))
			{
				// 1.5 RecordingToken을 가지고 index를 가져옴.
				// FIX ME : 1.5
				
			
				sprintf(buff, "onvif.recordingjob%d.mode", i);
				//2. Recording Job의 모드를 를 업데이트 한다.
				
				mode = getJobModeInt(tmp.conf.RecordingJobMode);
				dbmode = (int)nf_sysdb_get_uint(buff);
				// mode와 dbmode가 같지 않을 시 event log에 업데이트 한다.
				if(mode != dbmode) {
						nf_sysdb_set_uint(buff,(unsigned int)mode);
						// FIX ME:
						_TTY_LOG_ONVIF_DEBUG("tmp.conf.RecordingJobMode: %s (Line : %d)",tmp.conf.RecordingJobMode, __LINE__);
						// 3. Search를 위해 nf_eventlog_put_param 실행
						// 4. onvif 이벤트 실행.

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

	completed:
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

	g_message( "%s IN", __FUNCTION__);

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
int onvif_EndSearch(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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
int onvif_FindEvent(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(WEBBASE_SEARCH_LOG_DST)];
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
	status = nf_webbase_live_status_cstack_get_interval_read(3, 0);
	
	if( rec_info->ch == token && status.act_recording[token] != ' ' ){
		rec_info->end_time = GTIMEVAL_TO_GUINT64(filter.param.time_end);
	}

}

static void _start_recording_search(void *arg) 
{
	int ret = 0, ch=0, cnt=0, i=0;
	char start_point_ch[ONVIF_CH] = {0,};

	search_filter filter;
	char buff[ SIZE_LOG_BUFF ];		
	NF_LOG_RESULT_HEADER	*log_header = (NF_LOG_RESULT_HEADER	*)&buff[sizeof(int)+sizeof(WEBBASE_SEARCH_LOG_DST)];
	NF_LOG_DATA			elem[ONVIF_MAX_LOG_CNT];	
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

int onvif_FindRecordings(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_FindRecordings tmp;
	int i=0;
	pthread_t id = 0;
	NF_DISK_REC_TIME        nrec_time;	

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_FindRecordings);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_FindRecordings));

	_TTY_LOG_ONVIF("tmp.KeepAliveTime: %d", tmp.KeepAliveTime);
	_TTY_LOG_ONVIF("tmp.Scope.Filter: %s", tmp.Scope.Filter);
	_TTY_LOG_ONVIF("tmp.Scope.sizeSource: %d", tmp.Scope.sizeSource);
	_TTY_LOG_ONVIF("tmp.Scope.sizeRecording: %d", tmp.Scope.sizeRecording);

	// FIXME.Get Search Token
	sprintf(tmp.SearchToken, "srt0");
	init_search_rec_shm((unsigned int)STR2INT_SEARCH_TOKEN(tmp.SearchToken));
	
	// Start Recording Search Session
	memset(&onvif_search_filter, 0x00, sizeof(search_filter));
	_recording_search_filter_init(&tmp, &onvif_search_filter);

	set_search_rec_state_shm((unsigned int)STR2INT_SEARCH_TOKEN(onvif_search_filter.search_token), SEARCH_STATE_SEARCHING);
	if (pthread_create(&id, NULL, (void*) _start_recording_search, (void*) &onvif_search_filter)
			!= 0) {
		printf("thread create error\n");
		return -1;
	}
	pthread_detach(id);
	
	memcpy(packet.data, &tmp, sizeof(arg_FindRecordings));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_FindRecordings)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetEventSearchResults(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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
int onvif_GetMediaAttributes(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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
int onvif_GetRecordingInformation(NfOnvif *self, int fd, char *buff_rcv) 
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	char buff[COMMON_SIZE] = { 0, };
	int i=0;
	int recording_token_num = 0;
	int ret =  0;
	arg_RecordingInformation tmp;
	NF_DISK_REC_TIME        nrec_time;	

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_RecordingInformation);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_RecordingInformation));

	memset(buff, 0x00, sizeof(char)*COMMON_SIZE);
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) {
		sprintf(buff, "onvif.recording%d.token", i);
#if 0 //[[ Jeonghun_0130916_BEGIN -- del
	int StartTime;
	int EndTime;	
	int Status;
	char token[COMMON_SIZE];
	char Content[COMMON_SIZE];		
	arg_RecordingSourceInformation	Source;
	arg_TrackInformation TrackInfo;
#endif //]] Jeonghun_0130916_END -- del

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
		}
	}

	ret = nf_disk_rec_time(1, &nrec_time, NULL);
	if( !ret)	
		return ONVIF_ERR_RET_INTERNAL;

	tmp.StartTime = (int)nrec_time.min_time;
	tmp.EndTime = (int)nrec_time.max_time;

	if( tmp.token[0] != 0 ){
		recording_token_num = atoi(tmp.token+2);
	}

	tmp.TrackInfo[0].EndTime = (int)nrec_time.max_time;
	tmp.TrackInfo[0].StartTime = (int)nrec_time.min_time;
	strncpy(tmp.TrackInfo[0].Description, "test desc", strlen("test desc"));
	strncpy(tmp.TrackInfo[0].Content, "test content", strlen("test content"));
	sprintf(tmp.TrackInfo[0].token, "%s%d",VIDEO_TRACK, recording_token_num);
	tmp.TrackInfo[0].type = 0;
	tmp.TrackInfo[1].EndTime = (int)nrec_time.max_time;
	tmp.TrackInfo[1].StartTime = (int)nrec_time.min_time;
	strncpy(tmp.TrackInfo[1].Description, "test desc", strlen("test desc"));
	strncpy(tmp.TrackInfo[1].Content, "test content", strlen("test content"));
	sprintf(tmp.TrackInfo[1].token, "%s%d",AUDIO_TRACK, recording_token_num);
	tmp.TrackInfo[1].type = 1;
	tmp.TrackInfo[2].EndTime = (int)nrec_time.max_time;
	tmp.TrackInfo[2].StartTime = (int)nrec_time.min_time;
	strncpy(tmp.TrackInfo[2].Description, "test desc", strlen("test desc"));
	strncpy(tmp.TrackInfo[2].Content, "test content", strlen("test content"));
	sprintf(tmp.TrackInfo[2].token, "%s%d",METADATA_TRACK, recording_token_num);
	tmp.TrackInfo[2].type = 2;	

	// in here you should put the custom code.
	memcpy(packet.data, &tmp, sizeof(arg_RecordingInformation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_RecordingInformation))	!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}
int onvif_GetRecordingSearchResults(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PTZNode tmp;

	g_message( "%s IN", __FUNCTION__);

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
	g_message( "%s IN", __FUNCTION__);
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

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	get_replayuri(&tmp, NULL);

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

	g_message( "%s IN", __FUNCTION__);

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

	g_message( "%s IN", __FUNCTION__);

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
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetRecordingsResponseItem);
	_TTY_LOG_ONVIF_DEBUG("packet_len: %d (Line : %d)",packet_len, __LINE__);
	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetRecordingsResponseItem));


	// 1. 레코딩 토근검사

	// 2. track은 트랙 토큰은 3가지로 한다.
	//	tv0, av0, mv0
	for (i = 0; i < ONVIF_MAX_RECORDING_CNT; i++) { 
		// 1. onvif.recording0.token
		// 3개씩 하나의 레코딩임.

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
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (i : %d)",__FUNCTION__, i);
		sprintf(buff, "onvif.recording%d.token", i);
		strncpy(tmp.RecordingToken[i], nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		_TTY_LOG_ONVIF_DEBUG("token: %s (i : %d)",nf_sysdb_get_str_nocopy(buff), i);
		_TTY_LOG_ONVIF_DEBUG("rtoken: %s (i : %d)",tmp.RecordingToken[i], i);

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
int onvif_AddAudioEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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
		_TTY_LOG_ONVIF_DEBUG("tmp.config_token %s, tmp_token %s",
				tmp.config_token, tmp_token);
		if (!strcmp(tmp.config_token, tmp_token)) {
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

		_TTY_LOG_ONVIF_DEBUG("tmp.profile_token: %s tmp_token %s",
				tmp.profile_token, tmp_token);
		if (!strcmp(tmp.profile_token, tmp_token)) {
			profile_index = i;
			break;
		}
	}
	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

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

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.aencoder", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveAudioEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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
	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");

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
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveAudioSource(NfOnvif *self, int fd, char *buff_rcv) {
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
	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");

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
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddVideoSource(NfOnvif *self, int fd, char *buff_rcv) {
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
		sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AddConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AddConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_AddVideoEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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

	if (profile_index >= ONVIF_FIXED_PROFILE_CNT) {
		//if( vencoder_index != 0 )		tmp.result = -2;
		;
	} else if (profile_index < ONVIF_FIXED_PROFILE_CNT) {
		if (vencoder_index != profile_index)
			tmp.result = -2;
	}

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.vencoder", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
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

int onvif_RemoveVideoEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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

	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");

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
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_RemoveVideoSource(NfOnvif *self, int fd, char *buff_rcv) {
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

	profile_cnt = (int)nf_sysdb_get_uint("onvif.common.profile_cnt");
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

	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_DeleteProfile(NfOnvif *self, int fd, char *buff_rcv) {
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
			if (!nf_sysdb_get_bool(buff)) {
				if (i == ONVIF_MAX_PROFILE_CNT - 1) {
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
				} else {
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

				profile_cnt--;
				nf_sysdb_set_uint("onvif.common.profile_cnt", (unsigned int)profile_cnt);

				tmp.result = 1;
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
				break;
			} else {
				tmp.result = -1;
				return ONVIF_ERR_RET_SUCCESS;
			}
		}
	}

	if (i == ONVIF_MAX_PROFILE_CNT) {
		tmp.result = 0;
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_ProfileDB));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,	header.code, sizeof(arg_ProfileDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

static int onvif_Profile_GetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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

int onvif_GetProfilesDB(NfOnvif *self, int fd, char *buff_rcv) {
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
	int audio_enable = 1;
	//struct audio cam_audio;

	//g_message("%s IN", __FUNCTION__);
	/* Force Configuration */
	/* 1. Installation Mode OFF */
	/* 2. Audio Enable */
	/*
	 audio_enable = nf_sysdb_get_bool("camera.audio.enable");
	 audio_in_volume = nf_sysdb_get_uint("camera.audio.in_volume");
	 audio_out_volume = nf_sysdb_get_uint("camera.audio.out_volume");

	 if( strncmp(g_ip_cam.options.sn, "VCN", 3) == 0 ){
	 ;
	 }
	 else{
	 if(audio_enable != 1 || audio_in_volume != 100 || audio_out_volume != 100)
	 {

	 extern int g_audio_mute;
	 WEBBASE_AUDIO	audio;
	 //guint write_len;
	 memset( &audio, 0, sizeof(WEBBASE_AUDIO) );

	 if (restarting_av == 0){
	 restarting_av = 1;

	 memcpy( &audio, packet.data, sizeof(WEBBASE_AUDIO) );

	 memset( &cam_audio, 0, sizeof(struct audio));

	 audio.enable = 1;
	 audio.in_codec = 2;
	 audio.in_volume = 100;
	 audio.out_volume = 100;

	 g_audio_mute = 0;
	 ControlSystemData(_CTLCODE(_G_AUDIO, _C_ACTION, _D_UPDATE, AUDIO_CFG), &(audio), sizeof(WEBBASE_AUDIO));
	 restarting_av = 0;
	 }

	 }
	 }*/
	/* End of Force Configuration */

	//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE) {
	//	;
	//}
	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	tmp.profile_cnt = profile_cnt; //tmp.profile_cnt = profile_cnt;
	_TTY_LOG_ONVIF_DEBUG("tmp.profile_cnt : %d", tmp.profile_cnt);
	int prof_cnt = 0;
	for (i = 0; i < profile_cnt; i++) {
		//_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		if (1) {
			/*
			 if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
			 profile_cnt--;
			 tmp.profile_cnt = profile_cnt;
			 continue;
			 }
			 */
			sprintf(buff, "onvif.profile%d.name", i);
			strncpy(tmp.profile[prof_cnt].name, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.profile[prof_cnt].name %s",
					tmp.profile[prof_cnt].name);
			sprintf(buff, "onvif.profile%d.token", i);
			strncpy(tmp.profile[prof_cnt].token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.profile[prof_cnt].token: %s",
					tmp.profile[prof_cnt].token);
			sprintf(buff, "onvif.profile%d.fixed", i);
			tmp.profile[prof_cnt].fixed = nf_sysdb_get_bool(buff);

			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.vencoder", i);
			strncpy(tmp.profile[prof_cnt].vencoder.token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.profile[prof_cnt].vencoder.token: %s",
					tmp.profile[prof_cnt].vencoder.token);
			sprintf(buff, "onvif.profile%d.vsource", i);
			strncpy(tmp.profile[prof_cnt].vsource.token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.metadata", i);
			strncpy(tmp.profile[prof_cnt].metadata.token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.profile[prof_cnt].metadata.token %s",
					tmp.profile[prof_cnt].metadata.token);

			sprintf(buff, "onvif.profile%d.asource", i);
			strncpy(tmp.profile[prof_cnt].asource.token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.profile[prof_cnt].asource.token %s ",
					tmp.profile[prof_cnt].asource.token);

			if (tmp.profile[prof_cnt].asource.token[0] != '\0')
				GetAudioSourceConfiguration(&tmp.profile[prof_cnt].asource);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.aencoder", i);
			strncpy(tmp.profile[prof_cnt].aencoder.token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			if (tmp.profile[prof_cnt].aencoder.token[0] != '\0')
				_getAudioEncoderConfiguration(&tmp.profile[prof_cnt].aencoder);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			if (tmp.profile[prof_cnt].metadata.token[0] != '\0') {
				if (_getMetadataConfiguration(&tmp.profile[prof_cnt].metadata)
						== ONVIF_R_ERR)
					return ONVIF_R_ERR;
			}
			// FIX ME::
			tmp.profile[prof_cnt].audio_enable = 1;
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			//			tmp.profile[prof_cnt].audio_enable = (int)audio_enable & isUseAudio() & is_EnableAudio();
			if (tmp.profile[prof_cnt].vencoder.token[0] != '\0')
				get_vencoder_table(&tmp.profile[prof_cnt].vencoder);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			if (tmp.profile[prof_cnt].vsource.token[0] != '\0')
				onvif_Profile_GetVideoSource(&tmp.profile[prof_cnt]);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);

			prof_cnt++;
			_TTY_LOG_ONVIF_DEBUG("prof_cnt %d %d", prof_cnt, i);
		}
		/*
		 else {
		 sprintf(buff, "onvif.profile%d.token", i);
		 strncpy(tmp.profile[i].token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		 sprintf(buff, "onvif.profile%d.fixed", i);
		 tmp.profile[i].fixed = nf_sysdb_get_bool(buff);
		 sprintf(buff, "onvif.profile%d.vencoder", i);
		 strncpy(tmp.profile[i].vencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		 sprintf(buff, "onvif.profile%d.vsource", i);
		 strncpy(tmp.profile[i].vsource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		 sprintf(buff, "onvif.profile%d.metadata", i);
		 strncpy(tmp.profile[i].metadata.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		 sprintf(buff, "onvif.profile%d.asource", i);
		 strncpy(tmp.profile[i].asource.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		 if(tmp.profile[i].asource.token[0] != '\0')
		 onvif_GetAudioSourceConfiguration(&tmp.profile[i].asource);
		 sprintf(buff, "onvif.profile%d.aencoder", i);
		 strncpy(tmp.profile[i].aencoder.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		 if(tmp.profile[i].aencoder.token[0] != '\0')
		 onvif_GetAudioEncoderConfiguration(&tmp.profile[i].aencoder);
		 if(tmp.profile[i].metadata.token[0] != '\0')
		 onvif_GetMetadataConfiguration(&tmp.profile[i].metadata);

		 tmp.profile[i].audio_enable = 1; //& isUseAudio() & is_EnableAudio();
		 if(tmp.profile[i].vencoder.token[0] != '\0')
		 onvif_Profile_GetVideoEncoder(&tmp.profile[i]);
		 if(tmp.profile[i].vsource.token[0] != '\0')
		 onvif_Profile_GetVideoSource(&tmp.profile[i]);
		 }
		 */
	}

	memcpy(packet.data, &tmp, sizeof(arg_ProfilesDB));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ProfilesDB)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetProfileDB(NfOnvif *self, int fd, char *buff_rcv) {
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
	int audio_enable = 1;

	// to verify db value is well reserved
	//nf_sysdb_set_uint("onvif.common.profile_cnt", 16);
	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	_TTY_LOG_ONVIF_DEBUG("profile_cnt : %d", profile_cnt);
	for (i = 0; i < profile_cnt; i++) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		// Summer - Question 1
		//if (nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}

		// Get the token for comparision
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token: %s %s", tmp_token, tmp.token);
		if (!strcmp(tmp_token, tmp.token)) {
			strncpy(tmp.token, tmp_token, COMMON_SIZE - 1);
			sprintf(buff, "onvif.profile%d.fixed", i);
			tmp.fixed = nf_sysdb_get_bool(buff);

			//tmp.audio_enable = audio_enable & isUseAudio() & is_EnableAudio(i);
			//tmp.audio_enable = audio_enable & is_EnableAudio(i);

			// Database is not update sync , this is just test
			tmp.audio_enable = 1;
			sprintf(buff, "onvif.profile%d.vencoder", i);
			strncpy(tmp.vencoder.token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);

			sprintf(buff, "onvif.profile%d.vsource", i);
			strncpy(tmp.vsource.token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.metadata", i);
			strncpy(tmp.metadata.token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.asource", i);
			strncpy(tmp.asource.token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			if (tmp.asource.token[0] != '\0')
				GetAudioSourceConfiguration(&tmp.asource);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			sprintf(buff, "onvif.profile%d.aencoder", i);
			strncpy(tmp.aencoder.token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			if (tmp.aencoder.token[0] != '\0')
				_getAudioEncoderConfiguration(&tmp.aencoder);

			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			if (tmp.vencoder.token[0] != '\0')
				get_vencoder_table(&tmp.vencoder);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);

			if (tmp.vsource.token[0] != '\0')
				onvif_Profile_GetVideoSource(&tmp);
			_TTY_LOG_ONVIF_DEBUG("tmp.metadata.token %s", tmp.metadata.token);
			if (tmp.metadata.token[0] != '\0') {
				if (_getMetadataConfiguration(&tmp.metadata) == ONVIF_R_ERR) {
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
							__FUNCTION__, __LINE__);
					return ONVIF_R_ERR;
				}
			}
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			tmp.result = 1;
			break;
		}
	}

	if (i == profile_cnt) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	memcpy(packet.data, &tmp, sizeof(arg_ProfileDB));
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (packet_len : %d)",
			__FUNCTION__, packet_len);
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ProfileDB)) != ONVIF_ERR_RET_SUCCESS) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoder));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoEncoders(NfOnvif *self, int fd, char *buff_rcv) {
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
	//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE) { // second is NONE
	//	vencoder_cnt = 1;
	//}

	tmp.vencoder_cnt = vencoder_cnt;

	for (i = 0; i < vencoder_cnt; i++) {
		sprintf(buff, "onvif.vencoder%d.token", i);
		strncpy(tmp.vencoder[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		get_vencoder_table(&tmp.vencoder[i]);
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv) {
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
	set_vencoder_table(&tmp);
	if (tmp.result == ONVIF_R_ERR_INVALID_PARAM) {
		return ONVIF_R_ERR_INVALID_PARAM;
	}
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
	sysdb_save_cate(NF_SYSDB_CATE_REC);

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoder));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_VideoEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_GetStreamUri(NfOnvif *self, int fd, char *buff_rcv) {
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

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");

	for (i = 0; i < profile_cnt; i++) {
		//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE && i == 1) { // second is NONE
		//	continue;
		//}
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token: %s (tmp.token: %s)",
				tmp_token, tmp.token);
		if (!strcmp(tmp.token, tmp_token)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line: %d)",
					__FUNCTION__, __LINE__);
			break;
		}
	}
	if (i == profile_cnt) {
		_TTY_LOG_ONVIF_DEBUG("i :  %d (profile_cnt: %d)", i, profile_cnt);
		//tmp.result = 0;
		return ONVIF_R_ERR_INVALID_ARG_VAL;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	get_streamuri(&tmp, NULL);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	memcpy(packet.data, &tmp, sizeof(arg_StreamUri));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_StreamUri)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoEncoderOption(NfOnvif *self, int fd, char *buff_rcv) {
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

	// To DO
	char *hw_ver = NULL;
	int i, cnt, start, end, r_index = 0;
	int capa = 0;
	char vencoder_token[COMMON_SIZE] = { 0, };
	char profile_token[COMMON_SIZE] = { 0, };
	char capa_string[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	//int codec;
	int vencoder_cnt = ONVIF_VENCODER_CNT;
	int vstype = 0;

	nf_sig_type_get(&vstype);
	tmp.vs_type = vstype;
	// if no options specified, Generic option shall be specified
	if (tmp.profile_token[0] == '\0' && tmp.encoder_token[0] == '\0') {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.option_cnt = 0;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		get_h264_options(&tmp.voption[tmp.option_cnt]);
		tmp.option_cnt++;
//		get_jpeg_options(&tmp.voption[tmp.option_cnt]);
//		tmp.option_cnt++;
		tmp.result = 1;
	} else {
		// find encoder and validate it
		if (tmp.encoder_token[0] == '\0') {
			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
				sprintf(buff, "onvif.profile%d.token", i);
				strncpy(profile_token, nf_sysdb_get_str_nocopy(buff),
						COMMON_SIZE - 1);
				if (!strcmp(tmp.profile_token, profile_token)) {
					sprintf(buff, "onvif.profile%d.vencoder", i);
					strncpy(tmp.encoder_token, nf_sysdb_get_str_nocopy(buff),
							COMMON_SIZE - 1);
					break;
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT) {
				tmp.result = 0;
				return ONVIF_R_ERR_NO_PROFILE;
			}
		}
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < vencoder_cnt; i++) {
			sprintf(buff, "onvif.vencoder%d.token", i);
			strncpy(vencoder_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			if (!strcmp(tmp.encoder_token, vencoder_token)) {
				//if ( i == 0 ) { /*main_vencoder*/
				tmp.option_cnt = 0;
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);

				get_h264_options(&tmp.voption[tmp.option_cnt]);
				tmp.option_cnt++;
				//get_jpeg_options(&tmp.voption[tmp.option_cnt]);
				//tmp.option_cnt++;

				/*	If the second codec is not MJPEG, the first codec can be MJPEG */
				tmp.result = 1;
				break;
			}
		}

		// end of video encoder
		if (i == vencoder_cnt) {
			tmp.result = -1;
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoEncoderOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoEncoderOption))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetVideoSource(NfOnvif *self, int fd, char *buff_rcv) {
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

	// To DO
	set_vsource_table(&tmp);

	if (tmp.result == -1) {
		return ONVIF_R_ERR_CONFIG_MODIFY;
	}
	memcpy(packet.data, &tmp, sizeof(arg_VideoSource));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}
static void _getVideoSourceConfiguration(arg_VideoSource *tmp) {

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
			strncpy(tmp->s_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);

			//video source fps is first stream's
			//tmp_fps = nf_sysdb_get_uint("camera.S0.fps");
			//tmp->fps = get_fps_from_index(tmp_fps);
			tmp->fps = 30.0;

			sprintf(buff, "onvif.vsource%d.bound_x", i);
			tmp->x = (int) nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource%d.bound_y", i);
			tmp->y = (int) nf_sysdb_get_uint(buff);

			sprintf(buff, "onvif.vsource%d.bound_width", i);
			tmp->width = (int) nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.vsource%d.bound_height", i);
			tmp->height = (int) nf_sysdb_get_uint(buff);

			tmp->use_count = get_vsource_usecount(tmp->token);
			tmp->result = 1;
			break;
		}
	}

	if (i == ONVIF_VSOURCE_CNT) {
		tmp->result = 0;
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n",
				__FUNCTION__, __LINE__);
	}

}
int onvif_GetVideoSourceConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
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
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetVideoSourceConfigurations(NfOnvif *self, int fd, char *buff_rcv) {
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

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetAudioSource(NfOnvif *self, int fd, char *buff_rcv) {
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

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i;
	int asource_cnt = ONVIF_ASOURCE_CNT;

	_TTY_LOG_ONVIF("Set Audio [%s]", tmp.token);

	for (i = 0; i < asource_cnt; i++) {
		sprintf(buff, "onvif.asource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {

			sprintf(buff, "onvif.asource%d.source_token", i);
			strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			if (tmp.s_token[0] != '\0') {
				if (!strcmp(tmp.s_token, tmp_token)) {
					nf_sysdb_set_str(buff, tmp.s_token);
				} else {
					tmp.result = -1;
					return;
				}
			}
			if (tmp.name[0] != '\0') {
				sprintf(buff, "onvif.asource%d.name", i);
				nf_sysdb_set_str(buff, tmp.name);
			}

			if (tmp.save_flag == 1) {
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			}

			tmp.result = 1;
			break;
		}
	}
	if (i == asource_cnt) {
		tmp.result = 0;
		return;
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}
	memcpy(packet.data, &tmp, sizeof(arg_AudioSource));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioSource)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;

}

int onvif_SetAudioEncoder(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoder tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoder));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int i;
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
		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp_token %s\n", tmp_token);
		if (!strcmp(tmp.token, tmp_token)) {
			_TTY_LOG_ONVIF_DEBUG("tmp.encoding  %d\n", tmp.encoding);
			if (tmp.encoding != 0) {
				tmp.result = -1;
				break;
			}
			_TTY_LOG_ONVIF_DEBUG("tmp.bitrate  %d\n", tmp.bitrate);
			if (tmp.bitrate != 64) {
				tmp.result = -1;
				break;
			}
			_TTY_LOG_ONVIF_DEBUG("tmp.s_rate  %d\n", tmp.s_rate);
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

			if (tmp.save_flag == 1) {
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			}

			tmp.result = 1;
			break;
		}
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoder));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

static void _getAudioEncoderConfiguration(arg_AudioEncoder *tmp) {
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int aencoder_cnt = ONVIF_AENCODER_CNT;
	int i;

	//if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE ){   // second is NONE
	//	aencoder_cnt = 1;
	//}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	for (i = 0; i < aencoder_cnt; i++) {
		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		if (!strcmp(tmp->token, tmp_token)) {
			/* Name */
			sprintf(buff, "onvif.aencoder%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			get_mcast_addr(&tmp->mcast, i, "onvif.aencoder");
			sprintf(buff, "onvif.aencoder%d.mcast_port", i);
			tmp->mcast.port = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.aencoder%d.timeout", i);
			tmp->timeout = nf_sysdb_get_uint(buff);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			tmp->use_count = get_aencoder_usecount(tmp->token);
			sprintf(buff, "onvif.aencoder%d.encoding", i);
			tmp->encoding = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.aencoder%d.bitrate", i);
			tmp->bitrate = nf_sysdb_get_uint(buff);
			sprintf(buff, "onvif.aencoder%d.s_rate", i);
			tmp->s_rate = nf_sysdb_get_uint(buff);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
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
		fprintf(stderr, "%s Function %d(line) Token Not Found ERROR\n",
				__FUNCTION__, __LINE__);
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
}
int onvif_GetAudioEncoderConfiguration(NfOnvif *self, int fd, char *buff_rcv) {

	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_AudioEncoder tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	_TTY_LOG_ONVIF_DEBUG("buff_rcv: %s (sizeof : %d)",
			buff_rcv, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_AudioEncoder);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_AudioEncoder));
	// To DO
	_getAudioEncoderConfiguration(&tmp);
	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}
	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoder));
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioEncoder)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioEncoderConfigurations(NfOnvif *self, int fd, char *buff_rcv) {
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
	int i;
	char buff[COMMON_SIZE];

	tmp.aencoder_cnt = ONVIF_AENCODER_CNT;

	for (i = 0; i < tmp.aencoder_cnt; i++) {
		sprintf(buff, "onvif.aencoder%d.token", i);
		strncpy(tmp.aencoder[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		_getAudioEncoderConfiguration(&tmp.aencoder[i]);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioEncoders)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int GetAudioSourceConfiguration(arg_AudioSource *tmp) {
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int i;
	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {

		sprintf(buff, "onvif.asource%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp->token, tmp_token)) {
			// FIX ME::
			//if (is_EnableAudio(i)) {
			//if( !(isUseAudio() && is_EnableAudio(i)) ){
			//tmp.result = -10;
			//return;
			//}
			sprintf(buff, "onvif.asource%d.source_token", i);
			strncpy(tmp->s_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);

			sprintf(buff, "onvif.asource%d.name", i);
			strncpy(tmp->name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			tmp->use_count = get_asource_usecount(tmp->token);

			tmp->result = 1;
			break;
		}
	}
	if (i == ONVIF_ASOURCE_CNT) {
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

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSource));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioSource)) != ONVIF_ERR_RET_SUCCESS) {
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
	int i;
	char buff[COMMON_SIZE];

	// FIX ME::
	/*if( !(isUseAudio() && is_EnableAudio())) {
	 tmp.result = -10;
	 return ;
	 }
	 */

	tmp.asource_cnt = ONVIF_ASOURCE_CNT;

	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {
		sprintf(buff, "onvif.asource%d.token", i);
		strncpy(tmp.asource[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		GetAudioSourceConfiguration(&tmp.asource[i]);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioSourceOption(NfOnvif *self, int fd, char *buff_rcv) {
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

	_TTY_LOG_ONVIF(">>>>>>>>>>>>>>>>>>>");
	// if no options specified, Generic option shall be specified
	if (tmp.profile_token[0] == '\0' && tmp.config_token[0] == '\0') {
		tmp.astoken_cnt = 0;
		snprintf(tmp.astoken[tmp.astoken_cnt], COMMON_SIZE - 1, "as0");
		tmp.astoken_cnt++;
		tmp.result = 1;
	} else {
		_TTY_LOG_ONVIF(">>>profile[%s] conf[%s]",
				tmp.profile_token, tmp.config_token);
		// find encoder and validate it
		if (tmp.config_token[0] == '\0') {
			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
				sprintf(buff, "onvif.profile%d.token", i);
				strncpy(profile_token, nf_sysdb_get_str_nocopy(buff),
						COMMON_SIZE - 1);
				if (!strcmp(tmp.profile_token, profile_token)) {
					sprintf(buff, "onvif.profile%d.asource", i);
					strncpy(tmp.config_token, nf_sysdb_get_str_nocopy(buff),
							COMMON_SIZE - 1);
					break;
				}
			}

			if (i == ONVIF_MAX_PROFILE_CNT) {
				tmp.result = ONVIF_R_ERR_NO_CONFIG;
				return ONVIF_R_ERR;
			}
		}
		_TTY_LOG_ONVIF(">>>profile[%s] conf[%s]",
				tmp.profile_token, tmp.config_token);
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < asource_cnt; i++) {
			sprintf(buff, "onvif.asource%d.token", i);
			strncpy(asource_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			if (!strcmp(tmp.config_token, asource_token)) {
				//if ( i == 0 ) { /*main_vencoder*/
				tmp.astoken_cnt = 0;
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);

				sprintf(buff, "onvif.asource%d.source_token", i);
				strncpy(tmp.astoken[tmp.astoken_cnt],
						nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
				tmp.astoken_cnt++;

				/*	If the second codec is not MJPEG, the first codec can be MJPEG */
				tmp.result = 1;
				break;
			}
		}
		// end of video encoder
		if (i == asource_cnt) {
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}

	if (tmp.result != 1) {
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSourceOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioSourceOption))
			!= ONVIF_ERR_RET_SUCCESS) {
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
			/* We have only a video source */
			tmp.vsource_cnt = ONVIF_VSOURCE_CNT;

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
		snprintf(tmp.vstoken[tmp.vstoken_cnt], COMMON_SIZE - 1, "vs0");
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
	} else {
		// find encoder and validate it
		if (tmp.config_token[0] == '\0') {
			for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
				sprintf(buff, "onvif.profile%d.token", i);
				strncpy(profile_token, nf_sysdb_get_str_nocopy(buff),
						COMMON_SIZE - 1);
				if (!strcmp(tmp.profile_token, profile_token)) {
					sprintf(buff, "onvif.profile%d.vsource", i);
					strncpy(tmp.config_token, nf_sysdb_get_str_nocopy(buff),
							COMMON_SIZE - 1);
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
				//if ( i == 0 ) { /*main_vencoder*/
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

				/*	If the second codec is not MJPEG, the first codec can be MJPEG */
				tmp.result = 1;
				break;
			}
		}
		// end of video encoder
		if (i == vsource_cnt) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			return ONVIF_R_ERR_NO_CONFIG;
		}
	}

	if (tmp.result != 1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR;
	}

	memcpy(packet.data, &tmp, sizeof(arg_VideoSourceOption));
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoSourceOption))
			!= ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	return ONVIF_ERR_RET_SUCCESS;

}

void GetImagingSetting(int ch, arg_ImagingOption *tmp) {
	char butff[COMMON_SIZE] = { 0, };

#if 0 //[[ Jeonghun_0130412_BEGIN --  Del
	// db accsess
	mtable* runtime = NULL;
	runtime = get_runtime();

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	// blc mode is not exist
	//tmp->blc_mode = eProfile.mode.value;
	// show current camera target setting.
	if (runtime[ch].image.supported) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		if (runtime[ch].image.blc.support)
		tmp->blc_mode = runtime[ch].image.blc.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->brightness = runtime[ch].image.brightness.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->color = runtime[ch].image.color.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->contrast = runtime[ch].image.contrast.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->sharpness = runtime[ch].image.sharpness.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	}
	if (runtime[ch].image_onvif.supported_exposure) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->e_gain = runtime[ch].image_onvif.exposure.gain.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->e_time = runtime[ch].image_onvif.exposure.etime.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		if (runtime[ch].image_onvif.ircut.support) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			tmp->ircutfilter = runtime[ch].image_onvif.ircut.value;
		}
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp->e_iris = runtime[ch].image_onvif.exposure.iris.value;
	}
#endif //]] Jeonghun_0130412_END --  Del
	tmp->result = 1;
}

int onvif_GetVideoSources(NfOnvif *self, int fd, char *buff_rcv) {
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
	//g_message("%s IN", __FUNCTION__);
	vsource_cnt = ONVIF_VSOURCE_CNT;
	tmp.vsource_cnt = vsource_cnt;

	if (tmp.vsource_cnt == 0) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		tmp.result = 0;
		return ONVIF_R_ERR;
	}
	_TTY_LOG_ONVIF_DEBUG("tmp.vsource_cnt: %d ", tmp.vsource_cnt);
	strncpy(tmp_fps, nf_sysdb_get_str_nocopy("rec.netstream.fps"),
			COMMON_SIZE - 1);
	for (i = 0; i < tmp.vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp.vsource[i].s_token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		sprintf(buff, "onvif.vsource%d.token", i);
		strncpy(tmp.vsource[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		//video source fps is first stream's

		tmp.vsource[i].fps = get_fps_from_alphabet(tmp_fps[i]);

		sprintf(buff, "onvif.vsource%d.bound_width", i);
		tmp.vsource[i].width = nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.vsource%d.bound_height", i);
		tmp.vsource[i].height = nf_sysdb_get_uint(buff);

		strncpy(tmp.vsource[i].imaging.token, tmp.vsource[i].token,
				COMMON_SIZE - 1);
		//	onvif_GetImagingOption(i,&tmp.vsource[i].imaging);
		GetImagingSetting(i, &tmp.vsource[i].imaging);
	}

	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_VideoSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_VideoSources)) != ONVIF_ERR_RET_SUCCESS) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetAudioSources(NfOnvif *self, int fd, char *buff_rcv) {
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
	int i;
	char buff[COMMON_SIZE];

	tmp.asource_cnt = ONVIF_ASOURCE_CNT;

	for (i = 0; i < ONVIF_ASOURCE_CNT; i++) {
		sprintf(buff, "onvif.asource%d.token", i);
		strncpy(tmp.asource[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		GetAudioSourceConfiguration(&tmp.asource[i]);
	}

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
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

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");

	/* We have two fixed profiles that can have fixed configurations */
	for (i = 0; i < profile_cnt; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {

			tmp.aencoder_cnt = 0;
			if (i >= ONVIF_FIXED_PROFILE_CNT)
				sprintf(buff, "onvif.aencoder0.token");
			else
				sprintf(buff, "onvif.aencoder%d.token", i);
			strncpy(tmp.aencoder[tmp.aencoder_cnt].token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			_getAudioEncoderConfiguration(&tmp.aencoder[tmp.aencoder_cnt]);
			tmp.aencoder_cnt++;
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

	memcpy(packet.data, &tmp, sizeof(arg_AudioEncoders));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioEncoders)) != ONVIF_ERR_RET_SUCCESS) {
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
	//g_message("%s IN", __FUNCTION__);
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
		_TTY_LOG_ONVIF("pppppppppp[%s][%s]", tmp_token, tmp.token);

		if (!strcmp(tmp.token, tmp_token)) {
			tmp.asource_cnt = 0;

			if (i >= ONVIF_FIXED_PROFILE_CNT)
				sprintf(buff, "onvif.asource0.token");
			else
				sprintf(buff, "onvif.asource%d.token", i);

			strncpy(tmp.asource[tmp.asource_cnt].token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			GetAudioSourceConfiguration(&tmp.asource[tmp.asource_cnt]);

			tmp.asource_cnt++;
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

	memcpy(packet.data, &tmp, sizeof(arg_AudioSources));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_AudioSources)) != ONVIF_ERR_RET_SUCCESS) {
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
				sprintf(buff, "onvif.vencoder0.token");
			else
				sprintf(buff, "onvif.vencoder%d.token", i);

			strncpy(tmp.vencoder[tmp.vencoder_cnt].token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			get_vencoder_table(&tmp.vencoder[tmp.vencoder_cnt]);
			tmp.vencoder_cnt++;
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

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
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

			strncpy(tmp.metadata[tmp.metadata_cnt].token,
					nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
			get_vencoder_table(&tmp.metadata[tmp.metadata_cnt]);
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

	if (profile_index >= ONVIF_FIXED_PROFILE_CNT) {
		if (meta_index != 0)
			tmp.result = -2;
	} else if (profile_index < ONVIF_FIXED_PROFILE_CNT) {
		if (meta_index != profile_index)
			tmp.result = -2;
	}

	if (tmp.result > 0) {
		sprintf(buff, "onvif.profile%d.metadata", profile_index);
		nf_sysdb_set_str(buff, tmp.config_token);
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
	int metadata_cnt;

	metadata_cnt = ONVIF_METADATA_CNT;

	//	if(nf_sysdb_get_uint("camera.S1.codec") == CODEC_NONE) { // second is NONE
	//		metadata_cnt = 1;
	//	}
	for (i = 0; i < metadata_cnt; i++) {
		sprintf(buff, "onvif.metadata%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp.token, tmp_token)) {
			sprintf(buff, "onvif.metadata%d.name", i);
			nf_sysdb_set_str(buff, tmp.name);

			//sprintf(buff, "onvif.metadata%d.ptz_status", i);
			//nf_sysdb_set_uint(buff, tmp.ptz_status);
			//sprintf(buff, "onvif.metadata%d.ptz_position", i);
			//nf_sysdb_set_uint(buff, tmp.ptz_position);
			//sprintf(buff, "onvif.metadata%d.event_filter", i);
			//nf_sysdb_set_uint(buff, tmp.event_filter);
			//sprintf(buff, "onvif.metadata%d.event_policy", i);
			//nf_sysdb_set_uint(buff, tmp.event_policy);
			//sprintf(buff, "onvif.metadata%d.analytics", i);
			//nf_sysdb_set_uint(buff, tmp.analytics);
			//sprintf(buff, "onvif.metadata%d.analytics", i);
			//nf_sysdb_set_uint(buff, tmp.analytics);
			tmp.result = 1;
			break;

			if (tmp.save_flag) {
				sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			}
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
	return ONVIF_ERR_RET_SUCCESS;
}

static int _getMetadataConfiguration(arg_Metadata* tmp) {
	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE];
	int metadata_cnt = ONVIF_METADATA_CNT;
	int i;

	for (i = 0; i < metadata_cnt; i++) {
		sprintf(buff, "onvif.metadata%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

		if (!strcmp(tmp->token, tmp_token)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d) %s",
					__FUNCTION__, __LINE__, tmp_token);

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
			strncpy(tmp->event_filter, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.event_filter : %d", tmp->event_filter);
			sprintf(buff, "onvif.metadata%d.event_policy", i);
			strncpy(tmp->event_policy, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			_TTY_LOG_ONVIF_DEBUG("tmp.event_policy : %d", tmp->event_policy);
			sprintf(buff, "onvif.metadata%d.analytics", i);
			tmp->analytics = nf_sysdb_get_bool(buff);
			_TTY_LOG_ONVIF_DEBUG("tmp.analytics : %d", tmp->analytics);
			get_mcast_addr(&tmp->mcast, i, "onvif.metadata");
			sprintf(buff, "onvif.metadata%d.mcast_port", i);
			tmp->mcast.port = nf_sysdb_get_uint(buff);

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

#define ONVIF_BRIGHT_MAX 100
#define ONVIF_BRIGHT_MIN 0
#define ONVIF_COLOR_MAX 100
#define ONVIF_COLOR_MIN 0
#define ONVIF_CONTRAST_MAX 100
#define ONVIF_CONTRAST_MIN 0
#define ONVIF_SHARPNESS_MAX 100
#define ONVIF_SHARPNESS_MIN 0

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

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_SetImagingSettings(NfOnvif *self, int fd, char *buff_rcv) {
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
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;
	int result = 0;
	int negative = 0;
	int i;

	_TTY_LOG_ONVIF_DEBUG("vsource_cnt: %d", vsource_cnt);
	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp.token: %s,tmp_token %s ",
				tmp.token, tmp_token);
		if (!strcmp(tmp.token, tmp_token)) {
			// i is designated field
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			sprintf(buff, "cam.C%d.bright", i);
			if (!nf_sysdb_set_uint(buff, tmp.brightness)) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
				return ONVIF_R_ERR_SETTING_INVALID;

			}

			sprintf(buff, "cam.C%d.color", i);
			if (!nf_sysdb_set_uint(buff, tmp.color)) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
				return ONVIF_R_ERR_SETTING_INVALID;

			}

			sprintf(buff, "cam.C%d.contrast", i);
			if (!nf_sysdb_set_uint(buff, tmp.contrast)) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
				return ONVIF_R_ERR_SETTING_INVALID;

			}

			if (tmp.save_flag == 1) {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);
				sysdb_save_cate(NF_SYSDB_CATE_CAM);
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);
			}
			result = 1;
			break;
		}
	}

	if(result == 0)
	{
		return ONVIF_R_ERR_NO_SOURCE;
	}
#if ONVIF_MODEL_IPX
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	int i=0;
	int ch=atoi(tmp.token);
	_TTY_LOG_ONVIF_DEBUG("channel value : %d",ch);
	gchar buff[256];
	WEBBASE_CAM_COMPATIBILITY_IMAGE_PROFILE profile;

	setImageSettings(ch, tmp);
	tmp.result = 1;

	sysdb_save_cate(NF_SYSDB_CATE_CAM);
#endif

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_GetOptions(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_GetOption tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_GetOption);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_GetOption));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;
	int result = 0;
	int i;

	_TTY_LOG_ONVIF_DEBUG("vsource_cnt: %d\n", vsource_cnt);
	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp.token: %s,tmp_token %s \n",
				tmp.token, tmp_token);
		if (!strcmp(tmp.token, tmp_token)) {
			tmp.bright_max = ONVIF_BRIGHT_MAX;
			tmp.bright_min = ONVIF_BRIGHT_MIN;
			tmp.color_max = ONVIF_COLOR_MAX;
			tmp.color_min = ONVIF_COLOR_MIN;
			tmp.contrast_max = ONVIF_CONTRAST_MAX;
			tmp.contrast_min = ONVIF_CONTRAST_MIN;
			tmp.sharp_max = tmp.sharp_max = ONVIF_SHARPNESS_MAX;
			tmp.sharp_min = tmp.sharp_max = ONVIF_SHARPNESS_MIN;
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			tmp.result = 1;
			//return ONVIF_R_ERR;
		}
	}
	if(tmp.result == 0)
	{
		return ONVIF_R_ERR_NO_SOURCE;
	}


#ifdef ONVIF_MODEL_IPX
	getImageProfile(ch, &profile);
	getExposureImageProfile(ch,&eProfile);
	int ch;
	WEBBASE_CAM_COMPATIBILITY_IMAGE_PROFILE profile;
	WEBBASE_CAM_COMPATIBILITY_EXPOSURE_PROFILE eProfile;
	ch = atoi(tmp.token);
#endif
#if 0
	tmp.exposure_time_max = eProfile.etime.max;
	tmp.exposure_time_min = eProfile.etime.min;
	tmp.gain_max = eProfile.gain.max;
	tmp.gain_min = eProfile.gain.min;
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	tmp.iris_max = eProfile.iris.max;
	tmp.iris_min = eProfile.iris.min;
	if(eProfile.iris.max != 0 && eProfile.iris.min != 0) {
		tmp.op_iris = IRIS_OP_PWM;
	}

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	tmp.sharp_max = profile.sharpness.max;
	tmp.sharp_min = profile.sharpness.min;
	tmp.focus_far_max = profile.farlimit.max;
	tmp.focus_far_min = profile.farlimit.min;
	tmp.focus_near_max = profile.nearlimit.max;
	tmp.focus_near_min = profile.nearlimit.min;
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	tmp.op_af = getOnvifAutoFocusValue((int)profile.focus[ch].value);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	tmp.focus_absolute_speed_max = profile.abspeed.max;
	tmp.focus_absolute_speed_min = profile.abspeed.min;
	tmp.focus_relative_speed_max = profile.respeed.max;
	tmp.focus_relative_speed_min = profile.respeed.min;
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	tmp.focus_continous_speed_max = profile.cospeed.max;
	tmp.focus_continous_speed_min = profile.cospeed.min;
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",__FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF_DEBUG("profile.respeed.max %d profile.respeed.min %d",profile.respeed.max,profile.respeed.min);
	_TTY_LOG_ONVIF_DEBUG("profile.respeed.max %d profile.abspeed.min %d",profile.abspeed.max,profile.abspeed.min);
	_TTY_LOG_ONVIF_DEBUG("profile.respeed.max %d profile.cospeed.min %d",profile.cospeed.max,profile.cospeed.min);

#endif

	memcpy(packet.data, &tmp, sizeof(arg_GetOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_GetOption)) != ONVIF_ERR_RET_SUCCESS) {
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

	if (tmp.move_type == FOCUS_CONTINUOUS) {
		ptz_cmd.ch = atoi(tmp.token);
		if (tmp.speed == 0)
			tmp.speed = 1; // if the data is not existed,  it set the default value
		if (tmp.speed >= 0) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_FAR;
		} else {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			ptz_cmd.cmd = NF_PTZ_CMD_FOCUS_NEAR;
		}
		_TTY_LOG_ONVIF("Speed : %d", tmp.speed);
		ptz_cmd.params[0] = abs(tmp.speed);
		pushPTZControlOperation(&ptz_cmd, CONTINUOUS);

		tmp.result = SUCCESS;
	} else {
		tmp.result = FAULT;
	}

	memcpy(packet.data, &tmp, sizeof(arg_MoveOption));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_MoveOption)) != ONVIF_ERR_RET_SUCCESS) {
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
	char butff[COMMON_SIZE] = { 0, };

#if 0 //[[ Jeonghun_0130412_BEGIN --  Del
	// db accsess
	mtable* runtime = NULL;
	runtime = get_runtime();

	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	// blc mode is not exist
	//tmp.blc_mode = eProfile.mode.value;
	// show current camera target setting.
	if (runtime[ch].image.supported) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		if (runtime[ch].image.blc.support)
		tmp.blc_mode = runtime[ch].image.blc.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.brightness = runtime[ch].image.brightness.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.color = runtime[ch].image.color.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.contrast = runtime[ch].image.contrast.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.sharpness = runtime[ch].image.sharpness.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	}
	if (runtime[ch].image_onvif.supported_exposure) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.e_gain = runtime[ch].image_onvif.exposure.gain.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.e_time = runtime[ch].image_onvif.exposure.etime.value;
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		if (runtime[ch].image_onvif.ircut.support) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			tmp.ircutfilter = runtime[ch].image_onvif.ircut.value;
		}
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		tmp.e_iris = runtime[ch].image_onvif.exposure.iris.value;
	}
#endif //]] Jeonghun_0130412_END --  Del
	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
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

	// To DO
	char tmp_token[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };
	int vsource_cnt = ONVIF_VSOURCE_CNT;
	int result = 0;
	int i;

	for (i = 0; i < vsource_cnt; i++) {
		sprintf(buff, "onvif.vsource%d.source_token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp.token %s tmp_token %s", tmp.token, tmp_token);
		if (!strcmp(tmp.token, tmp_token)) {
			// i is designated field
			// BacklightCompensation
			sprintf(buff, "cam.C%d.bright", i);
			tmp.brightness = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.color", i);
			tmp.color = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.contrast", i);
			tmp.contrast = nf_sysdb_get_uint(buff);
#if 0
			sprintf(buff, "cam.C%d.exposure_mode", i);
			tmp.e_mode = getOnvifExposureMode(nf_sysdb_get_uint(buff));
			sprintf(buff, "cam.C%d.blc_control", i);
			tmp.blc_mode = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.gain", i);
			tmp.e_gain = nf_sysdb_get_uint(buff);
			sprintf(buff, "cam.C%d.etime", i);
			tmp.e_time = nf_sysdb_get_int(buff);
			sprintf(buff, "cam.C%d.iris", i);
			tmp.e_iris = nf_sysdb_get_int(buff);
			tmp.ircutfilter =1; // is not defined
			sprintf(buff, "cam.C%d.sharpness", i);
			tmp.sharpness = nf_sysdb_get_uint(buff);
#endif
			tmp.save_flag = 1;
			result = 1;
			break;
		}
	}
	if(result == 0){
		return ONVIF_R_ERR_NO_SOURCE;
	}

	memcpy(packet.data, &tmp, sizeof(arg_ImagingOption));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_ImagingOption)) != ONVIF_ERR_RET_SUCCESS) {
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

	memcpy(packet.data, &tmp, sizeof(arg_MoveStatus));

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_MoveStatus)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_FocusStop(NfOnvif *self, int fd, char *buff_rcv) {
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_FocusStop tmp;

	g_message("%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_FocusStop);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_FocusStop));

	// To DO
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	_TTY_LOG_ONVIF("Focus Stop ch : %s", tmp.VideoSourceToken);
	PTZOperationStop(tmp.VideoSourceToken);

	memcpy(packet.data, &tmp, sizeof(arg_FocusStop));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_FocusStop)) != ONVIF_ERR_RET_SUCCESS) {
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
			if (j < ONVIF_VSOURCE_CNT - 1) {
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
	sprintf(buff, token);
	sysdb_save_cate(NF_SYSDB_CATE_ONVIF);

}

void preset_name_array(arg_preset* preset) {
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
	strncpy(onvif_name, nf_sysdb_get_str_nocopy(buff),
			ONVIF_PRESET_NAME_SIZE - 1);
	sprintf(buff, "onvif.ptz_pre.token");
	strncpy(onvif_token, nf_sysdb_get_str_nocopy(buff),
			ONVIF_PRESET_TOKEN_SIZE - 1);
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

	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
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
				sprintf(buff, "onvif.ptz%d.node_token", j);
				strncpy(ptz_tmp_token, nf_sysdb_get_str_nocopy(buff),
						COMMON_SIZE - 1);

				_TTY_LOG_ONVIF_DEBUG("ptz_token: %s (tmp_token : %s)",
						ptz_token, ptz_tmp_token);
				if (!strcmp(ptz_token, ptz_tmp_token)) {
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

	g_message("%s IN", __FUNCTION__);

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
		return;
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

	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
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
	int interval;
	NF_PTZ_CMD ptz_cmd;

	_TTY_LOG_ONVIF("ProfileToken: %s ", tmp.ProfileToken);
	ptz_cmd.ch = getPTZTokenNumber(tmp.ProfileToken);
	if (ptz_cmd.ch == -1) {
		tmp.result = 0;
		return;
	}
	panTiltCmd = getPtzPanTiltCmd(&tmp, &interval);
	_TTY_LOG_ONVIF("panTiltCmd %d: ", panTiltCmd);
	if (panTiltCmd != -1) {
		ptz_cmd.cmd = panTiltCmd;
		_TTY_LOG_ONVIF("cmd : %d ", panTiltCmd);

		ptz_cmd.params[0] = interval;
		_TTY_LOG_ONVIF("interval : %d ", ptz_cmd.params[0]);

		_TTY_LOG_ONVIF("ParamValue : %d ", ptz_cmd.params[0]);
		pushPTZControlOperation(&ptz_cmd, CONTINUOUS);
	}
	zoomCmd = getPtzZoomCmd(&tmp, &interval);
	if (zoomCmd != -1) {
		ptz_cmd.cmd = zoomCmd;
		_TTY_LOG_ONVIF("cmd : %d ", zoomCmd);
		ptz_cmd.params[0] = interval;
		_TTY_LOG_ONVIF("ParamValue : %d ", ptz_cmd.params[0]);
		pushPTZControlOperation(&ptz_cmd, CONTINUOUS);
	}
	tmp.result = 1;

	memcpy(packet.data, &tmp, sizeof(arg_PTZOperation));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
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
int  onvif_RemovePTZConfiguration(NfOnvif *self, int fd, char *buff_rcv) {
	int i=0;
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
	// 1. Profile Token 을 찾는다.
	profile_cnt = nf_sysdb_get_uint("onvif.common.profile_cnt");
	sprintf(buff, "onvif.profile%d.token", i);
	_TTY_LOG_ONVIF_DEBUG("profile_cnt  %d", profile_cnt, __LINE__);
	for (i = 0; i < ONVIF_MAX_PROFILE_CNT; i++) {
		sprintf(buff, "onvif.profile%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.profile_token, tmp_token)) {
			sprintf(buff, "onvif.profile%d.ptz", i);
			nf_sysdb_set_str("",nf_sysdb_get_str_nocopy(buff));
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
			goto success;
		}
	}
	if(i==profile_cnt)
	{
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		return ONVIF_R_ERR_NO_PROFILE;
	}

success:

memcpy(packet.data, &tmp, sizeof(arg_RemoveConfig));
if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
		header.code, sizeof(arg_RemoveConfig)) != ONVIF_ERR_RET_SUCCESS) {
	return ONVIF_ERR_RET_INTERNAL;
}
return ONVIF_ERR_RET_SUCCESS;
	// 해당 카운트에 ptz 값을 0으로 세팅한다.
	// Profile token이 없으면 No Profile을 리턴함.
}
int onvif_GetPTZConfigurations(NfOnvif *self, int fd, char *buff_rcv) {
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
	int ptz_cnt;
	char buff[COMMON_SIZE] = { 0, };
	char tmp_token[COMMON_SIZE] = { 0, };
	ptz_cnt = ONVIF_PTZ_CNT;
	for (i = 0; i < ptz_cnt; i++) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		sprintf(buff, "onvif.ptz%d.name", i);
		strncpy(tmp.ptz[i].name, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		sprintf(buff, "onvif.ptz%d.token", i);
		strncpy(tmp.ptz[i].token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		sprintf(buff, "onvif.ptz%d.node_token", i);
		strncpy(tmp.ptz[i].node_token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);

		tmp.ptz[i].use_count = 1;
		tmp.ptz[i].result = 1;
	}
	tmp.ptz_cnt = ONVIF_PTZ_CNT;

	memcpy(packet.data, &tmp, sizeof(arg_PTZNodes));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZNodes)) != ONVIF_ERR_RET_SUCCESS) {
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
		sprintf(buff, "onvif.ptz%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		_TTY_LOG_ONVIF_DEBUG("tmp.token %s (tmp_token : %s)",
				tmp.token, tmp_token);
		if (!strcmp(tmp.token, tmp_token)) {
			sprintf(buff, "onvif.ptz%d.name", i);
			nf_sysdb_set_str(buff, tmp.name);

			_TTY_LOG_ONVIF_DEBUG("tmp.node_token %s (Line : %d)",
					tmp.node_token, __LINE__);

			sprintf(buff, "onvif.ptz%d.node_token", i);
			nf_sysdb_set_str(buff, tmp.node_token);

			sprintf(buff, "onvif.ptz%d.timeout", i);
			nf_sysdb_set_uint(buff, tmp.timeout);
			_TTY_LOG_ONVIF_DEBUG("tmp.timeout %d", tmp.timeout);
			sysdb_save_cate(NF_SYSDB_CATE_ONVIF);
			break;
		}
	}

	if (i == ptz_cnt) {
		tmp.result = 0;
		return;
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
	int result = 0;
	for (i = 0; i < ptz_cnt; i++) {

		sprintf(buff, "onvif.ptz%d.token", i);
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.token, tmp_token)) {
			sprintf(buff, "onvif.ptz%d.name", i);
			strncpy(tmp.name, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.ptz%d.token", i);
			_TTY_LOG_ONVIF_DEBUG("tmp.token %s (Line : %d)",
					tmp.token, __LINE__);
			strncpy(tmp.token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);

			sprintf(buff, "onvif.ptz%d.node_token", i);
			strncpy(tmp.node_token, nf_sysdb_get_str_nocopy(buff),
					COMMON_SIZE - 1);
			sprintf(buff, "onvif.ptz%d.timeout", i);
			_TTY_LOG_ONVIF_DEBUG("tmp.timeout %d", tmp.timeout);
			tmp.timeout = nf_sysdb_get_uint(buff);
			tmp.use_count = 1;
			result = 1;
			break;
		}
	}
	if(i == ptz_cnt)
	{
		return ONVIF_R_ERR_NO_PROFILE;
	}
	tmp.result = result;

	memcpy(packet.data, &tmp, sizeof(arg_PTZNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZNode)) != ONVIF_ERR_RET_SUCCESS) {
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
		strncpy(tmp_token, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
		if (!strcmp(tmp.token, tmp_token)) {
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
	int ptz_cnt;
	char buff[COMMON_SIZE] = { 0, };

	ptz_cnt = ONVIF_PTZ_CNT;
	for (i = 0; i < ptz_cnt; i++) {
		sprintf(buff, "onvif.ptz%d.name", i);
		strncpy(tmp.ptz[i].name, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		sprintf(buff, "onvif.ptz%d.node_token", i);
		strncpy(tmp.ptz[i].node_token, nf_sysdb_get_str_nocopy(buff),
				COMMON_SIZE - 1);
		sprintf(buff, "onvif.ptz%d.MaximumNumberOfPresets", i);
		tmp.ptz[i].max_num_preset = nf_sysdb_get_uint(buff);
		sprintf(buff, "onvif.ptz%d.HomeSupported", i);
		tmp.ptz[i].home_supported = nf_sysdb_get_bool(buff);
		sprintf(buff, "onvif.ptz%d.FixedHomePosition", i);
		tmp.ptz[i].fixed_home_position = nf_sysdb_get_bool(buff);
		tmp.ptz[i].result = 1;

	}
	tmp.ptz_cnt = ptz_cnt;
	_TTY_LOG_ONVIF_DEBUG("tmp.ptz_cnt %d", tmp.ptz_cnt);

	memcpy(packet.data, &tmp, sizeof(arg_PTZNodes));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZNodes)) != ONVIF_ERR_RET_SUCCESS) {
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
	int profile_cnt = ONVIF_PTZ_CNT;
	_TTY_LOG_ONVIF_DEBUG("tmp.profileToken: %s (Line : %d)", tmp.profileToken, __LINE__);
	preset_name_array(preset);
	_TTY_LOG_ONVIF_DEBUG("tmp.profileToken: %s (Line : %d)", tmp.profileToken, __LINE__);
	int profileNum = findfieldStringCount("onvif.profile%d.token",
			tmp.profileToken, profile_cnt);
	
	if(profileNum == -1) return ONVIF_R_ERR_NO_ENTITY;
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	int ch = parseStringToChannel(ch_buff);
	_TTY_LOG_ONVIF_DEBUG("token_num: %d (Line : %d)", ch, __LINE__);
	if (ch != -1) {

		tmp.preset_cnt = 0;
		_TTY_LOG_ONVIF_DEBUG("tmp.preset_cnt: %d (Line : %d)",
				tmp.preset_cnt, __LINE__);
		for (i = 0; i < profile_cnt; i++) {
			if (checkPreset(ch, i + 1) != -1) {
				_TTY_LOG_ONVIF_DEBUG("ch: %d (i : %d)", ch, i+1);
				strncpy(tmp.preset[j].presetName, preset[ch].presetName[i],
						COMMON_SIZE - 1);
				strncpy(tmp.preset[j].presetToken, preset[ch].presetToken[i],
						ONVIF_TOKEN_SIZE - 1);
				_TTY_LOG_ONVIF_DEBUG("tmp.preset[i].presetName: %s (Line : %d)",
						tmp.preset[i].presetName, __LINE__);
				tmp.preset_cnt += 1;
				j++;
				_TTY_LOG_ONVIF_DEBUG("tmp.preset_cnt: %d (Line : %d)",
						tmp.preset_cnt, __LINE__);
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
	for (i = 0; i < ONVIF_VSOURCE_CNT; i++) {
		sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, i);
		if (presetNum == nf_sysdb_get_uint(buff)) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
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
	_TTY_LOG_ONVIF("profileName : %s presetName : %s",
			tmp.profileToken, tmp.presetToken);
	NF_PTZ_CMD ptz_cmd;
	int i = 0;
	arg_preset preset[ONVIF_VSOURCE_CNT];
	char ch_buff[COMMON_SIZE] = { 0, };
	char buff[COMMON_SIZE] = { 0, };

	memset(preset, 0x00, sizeof(arg_preset) * ONVIF_VSOURCE_CNT);
	int profile_cnt = ONVIF_PTZ_CNT;
	preset_name_array(preset);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

	int profileNum = findfieldStringCount("onvif.profile%d.token",
			tmp.profileToken, profile_cnt);
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	int ch = parseStringToChannel(ch_buff);

	int preset_num = -1;
	if (ch != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < ONVIF_PRESET_CNT; i++) {
			_TTY_LOG_ONVIF_DEBUG(
					"tmp.presetToken: %s (preset[token_num].presetToken[%d] : %s)",
					tmp.presetToken, i, preset[ch].presetToken[i]);
			if (!strcmp(tmp.presetToken, preset[ch].presetToken[i])) {
				if (checkPreset(ch, i + 1) == -1) {
					tmp.result = -1;
					return ONVIF_R_ERR;
				}

				preset_num = i;
				_TTY_LOG_ONVIF_DEBUG("ch %d, preset_num %d", ch, preset_num);
				setITXCameraPreset(ch, preset_num, NF_PTZ_CMD_GOTO_PRESET);
				tmp.result = 1;
				return ONVIF_R_ERR;

			}
		}
		if (i == ONVIF_PRESET_CNT) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
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

	memcpy(packet.data, &tmp, sizeof(arg_PTZPresetNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZPresetNode)) != ONVIF_ERR_RET_SUCCESS) {
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
		nf_sysdb_set_uint(buff, nf_sysdb_get_uint(buff_after));
		sprintf(buff, "cam.ptz.P%d.preset.P%d.name", ch, i);
		sprintf(buff_after, "cam.ptz.P%d.preset.P%d.name", ch, i + 1);
		nf_sysdb_set_str(buff, nf_sysdb_get_str_nocopy(buff_after));
	}
	sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, pcnt);
	nf_sysdb_set_uint(buff, 0);
	sprintf(buff, "cam.ptz.P%d.preset.P%d.name", ch, pcnt);
	sprintf(presetName, "OnvifPre%02d%02d", ch, presetNum + 1);
	nf_sysdb_set_str(buff, "");

	_TTY_LOG_ONVIF_DEBUG("pcnt: %d (Line : %d)", pcnt, __LINE__);
	if (pcnt > 0) {
		pcnt = pcnt - 1;
	}
	nf_sysdb_set_uint(pcnt_buff, pcnt);
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

	int profileNum = findfieldStringCount("onvif.profile%d.token",
			tmp.profileToken, profile_cnt);
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
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZOperation)) != ONVIF_ERR_RET_SUCCESS) {
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
	if (!checkPreset(ch, value)) {
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
	_TTY_LOG_ONVIF_DEBUG("buff: %s (Line : %d)", buff, __LINE__);
	int presetNum = getAvailablePresetNum(buff, ONVIF_VSOURCE_CNT);
	_TTY_LOG_ONVIF_DEBUG("presetNum: %d (Line : %d)", presetNum, __LINE__);
	if (presetNum != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		setITXCameraPreset(ch, presetNum, NF_PTZ_CMD_SET_PRESET);
		snprintf(buff, sizeof(buff), "cam.ptz.P%d.preset.PCNT", ch);
		pcnt = nf_sysdb_get_uint(buff) + 1;
		if (pcnt > ONVIF_VSOURCE_CNT) {
			return -1;
		} else {
			nf_sysdb_set_uint(buff, pcnt);
		}

		_TTY_LOG_ONVIF_DEBUG("pcnt: %d (Line : %d)", pcnt, __LINE__);
		sprintf(buff, "cam.ptz.P%d.preset.P%d.number", ch, pcnt - 1);
		nf_sysdb_set_uint(buff, presetNum);

		sprintf(buff, "cam.ptz.P%d.preset.P%d.name", ch, pcnt - 1);
		sprintf(presetName, "OnvifPre%02d%02d", ch, presetNum);
		_TTY_LOG_ONVIF_DEBUG("presetName: %s (Line : %d)",
				presetName, __LINE__);
		nf_sysdb_set_str(buff, presetName);
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
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	char* token = NULL;
	char* context = NULL;
	char spliter[] = "z";
	token = strtok_r(chString, spliter, &context);
	token = strtok_r(NULL, spliter, &context);

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


	_TTY_LOG_ONVIF_DEBUG("tmp.profileToken: %s (Line : %d)", tmp.profileToken, __LINE__);
	int profile_cnt = ONVIF_PTZ_CNT;
	preset_name_array(preset);

	_TTY_LOG_ONVIF_DEBUG("tmp.profileToken: %s (Line : %d)", tmp.profileToken, __LINE__);
	int profileNum = findfieldStringCount("onvif.profile%d.token",
			tmp.profileToken, profile_cnt);
	if(profileNum == -1) {
		return ONVIF_R_ERR_PROFILE_EXIST;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	sprintf(buff, "onvif.profile%d.ptz", profileNum);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (profileNum : %d)", __FUNCTION__, profileNum);
	strncpy(ch_buff, nf_sysdb_get_str_nocopy(buff), COMMON_SIZE - 1);
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
	int ch = parseStringToChannel(ch_buff);


	if (tmp.presetToken[0] == '\0') {
		///null이면 새 preset를 설정함.
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		idx = setNewITXPresetDB(ch);
		if (idx != -1) {
			strncpy(tmp.presetToken, preset[ch].presetToken[idx],
					ONVIF_TOKEN_SIZE - 1);
			if (tmp.presetName[0] != '\0') {
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);
				// copy the presetName to db
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (idx : %d)", __FUNCTION__, idx);
				_TTY_LOG_ONVIF_DEBUG("preset[ch].presetName[idx]: %s (tmp.presetName : %s)", preset[ch].presetName[idx], tmp.presetName);
				strncpy(preset[ch].presetName[idx], tmp.presetName,
						COMMON_SIZE - 1);
				save_array_to_preset_db(preset);
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);
			}
			//tmp.result = 1;
			//return ONVIF_ERR_RET_SUCCESS;
			goto success;

		} else {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			return ONVIF_R_ERR_TOO_MANY_PRESET;
		}
	}

	if (ch != -1) {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
		for (i = 0; i < ONVIF_PRESET_CNT; i++) {
			_TTY_LOG_ONVIF_DEBUG(
					"tmp.presetToken: %s (preset[token_num].presetToken[%d] : %s)",
					tmp.presetToken, i, preset[ch].presetToken[i]);
			if (!strcmp(tmp.presetToken, preset[ch].presetToken[i])) {

				// preset is not in the ITX DB
				if (setExistITXPresetDB(ch, i) == -1) {

					return ONVIF_R_ERR_PROFILE_EXIST;
				}

				if (tmp.presetName[0] != '\0') {
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
							__FUNCTION__, __LINE__);
					// copy the presetName to db
					strncpy(preset[ch].presetName[i], tmp.presetName,
							COMMON_SIZE - 1);
					save_array_to_preset_db(preset);
					_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
							__FUNCTION__, __LINE__);
				}
				_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
						__FUNCTION__, __LINE__);
				// tmp.result = 1;
				// preset count is i
				// return ONVIF_R_ERR;
				//return ONVIF_ERR_RET_SUCCESS;
				goto success;
			}
		}
		if (i == ONVIF_PRESET_CNT) {
			_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)",
					__FUNCTION__, __LINE__);
			// presetToken is not exist

			return ONVIF_R_ERR_PROFILE_EXIST;
		}
	} else {
		_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

		return ONVIF_R_ERR_NO_ENTITY;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);
success:
	memcpy(packet.data, &tmp, sizeof(arg_PTZPresetNode));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY,
			header.code, sizeof(arg_PTZPresetNode)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}
	_TTY_LOG_ONVIF_DEBUG("Entry: %s (Line : %d)", __FUNCTION__, __LINE__);

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
			return;
		}
		PTZOperationStop(ch);
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


int onvif_CreatePullPointSubscription(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_CreatePullPoint tmp;
	
	NF_NETIF_GET_INFO ret_net_info;
	struct sockaddr_in ipaddr;
	char ipaddress[COMMON_SIZE];
	unsigned int ss_id = 0;	
	int ret = 0, http_port=80;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_CreatePullPoint);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_CreatePullPoint));
	memset(ipaddress, 0x00, sizeof(char)*COMMON_SIZE);
	
	// Create New subscription and get ss_id
	ret = OV_event_create_new_subscription(tmp.termination_time, &tmp.filters);
	if( ret <= 0 ){
		return ONVIF_R_ERR;
	}
	ss_id = (unsigned int)ret;

	// Get subscription address	
	http_port = (int)nf_sysdb_get_uint("net.proto.webport");
	nf_netif_get_info(&ret_net_info);
	ipaddr.sin_addr.s_addr = (unsigned long) ret_net_info.ipaddr;
	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);	
	snprintf(ipaddress, sizeof(char)*COMMON_SIZE, "%s", inet_ntoa(ipaddr.sin_addr));		
	snprintf(tmp.ss_address, sizeof(tmp.ss_address), "http://%s:%d/onvif/subscription?subscription=%d", ipaddress, http_port, ss_id);

	memcpy(packet.data, &tmp, sizeof(arg_CreatePullPoint));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_CreatePullPoint)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;

}

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
	int ret = 0, http_port=80;

	g_message( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Subscribe);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Subscribe));

	//Create New subscription and get ss_id
	ret = OV_event_create_new_subscription(tmp.termination_time, &tmp.filters);
	if( ret <= 0 ){
		return ONVIF_R_ERR;
	}
	ss_id = ret;

	//Get subscription address
	http_port = (int)nf_sysdb_get_uint("net.proto.webport");
	nf_netif_get_info(&ret_net_info);
	ipaddr.sin_addr.s_addr = (unsigned long) ret_net_info.ipaddr;
	ipaddr.sin_addr.s_addr = htonl(ipaddr.sin_addr.s_addr);	
	snprintf(ipaddress, sizeof(char)*COMMON_SIZE, "%s", inet_ntoa(ipaddr.sin_addr));		
	snprintf(tmp.ss_address, sizeof(tmp.ss_address), "http://%s:%d/onvif/subscription?subscription=%d", ipaddress, http_port, ss_id);	
	
	ret = OV_event_start_notify(ss_id, tmp.dest_address);	
	if( ret < 0 ){
		return ONVIF_R_ERR;
	}	

	memcpy(packet.data, &tmp, sizeof(arg_Subscribe));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_Subscribe)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}

int onvif_PullMessages(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;
	arg_PullMessages tmp;
	struct event_subscription sub;
	int msgcnt = 0;
	int ret = 0;

	OV_DEBUG( "%s IN", __FUNCTION__);

	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_PullMessages);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_PullMessages));

	OV_DEBUG("ssid[%d] timeout[%d]", tmp.ss_id, tmp.timeout);
	
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

	memcpy(packet.data, &tmp, sizeof(arg_PullMessages));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_PullMessages)) != ONVIF_ERR_RET_SUCCESS) {
		OV_DEBUG("response Fail!!");
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
	
	g_message( "%s IN", __FUNCTION__);
	
	memset(&header, 0, sizeof(header));
	memcpy(&header, buff_rcv, sizeof(header));

	packet_len = sizeof(ONVIF_HEADER) + sizeof(arg_Renew);

	memcpy(&packet, buff_rcv, packet_len);
	memcpy(&tmp, packet.data, sizeof(arg_Renew));

	ret = OV_event_renew_subscription(tmp.ss_id, tmp.termination_time, tmp.is_relative);	
	if ( ret < 0) {
		return ONVIF_R_ERR;
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

int onvif_GetRelayOptions(NfOnvif *self, int fd, char *buff_rcv)
{
	ONVIF_PACKET packet;
	ONVIF_HEADER header;
	guint packet_len;

	arg_RelayOptions tmp;
	int ret = 0, i=0, j=0;	
	char buff[128];
	int delay_list[ONVIF_RELAY_DELAY_CNT] = { 5, 10, 15, 20, 30, 40, 60, 120, 180, 300 };
	
	g_message( "%s IN", __FUNCTION__);
	
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
	
	g_message( "%s IN", __FUNCTION__);
	
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
	}
	//
	
	memcpy(packet.data, &tmp, sizeof(arg_DigitalInputs));
	if (nf_onvif_send(fd, &packet, packet_len, ONVIF_PROTOCOL_REPLY, header.code, sizeof(arg_DigitalInputs)) != ONVIF_ERR_RET_SUCCESS) {
		return ONVIF_ERR_RET_INTERNAL;
	}

	return ONVIF_ERR_RET_SUCCESS;
}




