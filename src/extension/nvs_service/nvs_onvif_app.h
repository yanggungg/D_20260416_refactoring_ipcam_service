#ifndef __NF_ONVIF_SERVER_APP_H__
#include "scm.h"
#include "feature_def.h"
#include "onvif_common.h"
#include "nf_api_eventlog.h"
#include "nf_util_time.h"
#include "../nf_onvif_server.h"

#define __NF_ONVIF_SERVER_APP_H__
#define ONVIF_LC_MOTION	((LT_MASK_MOTION_DETECTION))
#define SIZE_LOG_BUFF	(sizeof(int)+sizeof(NF_LOG_RESULT_HEADER)+ONVIF_MAX_LOG_DATA)
#define TOKEN_PTZ_NODE	"ptz0"
#define TOKEN_PTZ_CONF	"pc0"
////////////////////////////////////////////////////////////////////////////////
///////////////////ENUM/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct _search_filter 
{
	int 			type; /* Recordings, Events, Metadata, PTZs  */
	time_t 			start_point;
	time_t 			end_point;
	int			 	max_matches; 
	unsigned int	timeout;
	char 		search_token[COMMON_SIZE];
	NF_LOG_PARAM param;
} search_filter;



typedef enum _ONVIF_FIELD_NAME_E {
	ONVIF_FIELD_NR,
} ONVIF_FIELD_NAME_E;

//void VW_SetupSystem_set_changeflag(guint flag);
gboolean nf_zoneinfo_get_activex_posix_onvif( const gchar *tzname, NF_TZINFO_ACTIVEX *tzout);
gboolean nf_action_relay_test(gboolean is_test_on, gint relay_num, gboolean type);
int set_search_rec_state_shm(unsigned int token, char search_state);
enum _ONVIF_GET_MODIFY {
	ONVIF_GET=0,
	ONVIF_MODIFY,
};
int init_search_rec_shm(unsigned int token, REC_FIND_TYPE FindType, time_t StartPoint, time_t EndPoint);
void GetCapability(arg_GetCapa *capa, int ch);
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////Application/////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int onvif_CheckTokens(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetHostName(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetHostName(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetDNS(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetDNS(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetNTP(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetNTP(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetDateTime(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetDateTime(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetDeviceInformation(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_FactoryDefault(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SystemReboot(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetNetworkInfo(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetNetworkInfo(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetNetworkPort(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetNetworkPort(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetScope(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetScope(NfOnvif *self, int fd, char *buff_rcv);
int onvif_AddScope(NfOnvif *self, int fd, char *buff_rcv);
int onvif_RemoveScope(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetDiscovery(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetDiscovery(NfOnvif *self, int fd, char *buff_rcv);
int onvif_Passfd(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetUser(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetUser(NfOnvif *self, int fd, char *buff_rcv);
int onvif_CreateUser(NfOnvif *self, int fd, char *buff_rcv);
int onvif_DeleteUser(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetCapability(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRelays(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetRelaySettings(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetRelayState(NfOnvif *self, int fd, char *buff_rcv);
int onvif_CreateProfile(NfOnvif *self, int fd, char *buff_rcv);
int onvif_AddAudioSource(NfOnvif *self, int fd, char *buff_rcv);
int onvif_AddPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetAudioOutput(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetAudioDecoder(NfOnvif *self, int fd, char *buff_rcv);
int onvif_CreateRecording(NfOnvif *self, int fd, char *buff_rcv);
int onvif_CreateRecordingJob(NfOnvif *self, int fd, char *buff_rcv);
int onvif_CreateTrack(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_DeleteRecording(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_DeleteRecordingJob(NfOnvif *self, int fd, char *buff_rcv);
int onvif_DeleteTrack(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordingConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordingJobConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordingJobs(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordingJobState(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordingOptions(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetTrackConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetRecordingConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetRecordingJobConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetRecordingJobMode(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetTrackConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_EndSearch(NfOnvif *self, int fd, char *buff_rcv);
int onvif_FindEvent(NfOnvif *self, int fd, char *buff_rcv);
int onvif_FindMetadata(NfOnvif *self, int fd, char *buff_rcv);
int onvif_FindPTZPosition(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_FindRecordings(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetEventSearchResults(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetMediaAttributes(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetMetadataSearchResults(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordingInformation(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetRecordingSearchResults(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetRecordingSummary(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetSearchState(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetRecordingServiceCapabilities(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetReplayConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetReplayUri(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetReplayConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetRecordingJobStateChangeEvent(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetRecordings(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_AddAudioEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemoveAudioEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemoveAudioSource(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_AddVideoSource(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_AddVideoEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemoveVideoEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemoveVideoSource(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_DeleteProfile(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_Profile_GetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetProfilesDB(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetProfileDB(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetVideoEncoders(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetVideoEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetStreamUri(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetVideoEncoderOption(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetVideoSource(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetDefaultGateway(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetDefaultGateway(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRecordings(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetSnapshotUri(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetGuaranteedNumberOfVideoEncoderInstances(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetVideoSourceConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetVideoSourceConfigurations(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetAudioSource(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetAudioEncoder(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetAudioEncoderConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetAudioEncoderConfigurations(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetAudioSourceConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetAudioSourceConfigurations(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetAudioSourceOption(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetCompatibleVideoSource(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetVideoSourceOption(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetVideoSources(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetAudioSources(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetCompatibleAudioEncoder(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetCompatibleAudioSource(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetCompatibleVideoEncoder(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetMetadataConfigurations(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetMetadataConfigurations2(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetCompatibleMetadataConfigurations(NfOnvif *self, int fd, char *buff_rcv);
int onvif_RemoveMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_AddMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetMetadataConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetImagingOption(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetImagingOption(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetImagingSettings(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetOptions(NfOnvif *self, int fd, char *buff_rcv);
int onvif_Move(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetImagingSetting(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetImagingSettings(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetStatus(NfOnvif *self, int fd, char *buff_rcv);
int onvif_FocusStop(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GotoHomePosition(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_AbsoluteMove(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_ContinuousMove(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_CreatePresetTour(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetPTZConfigurations(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetPTZConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemovePTZConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetNode(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetNodes(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetPresets(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetPresetTour(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetPresetTourOptions(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetPresetTours(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GetServiceCapabilities(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_PTZGetStatus(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_GotoPreset(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_ModifyPresetTour(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_OperatePresetTour(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RelativeMove(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemovePreset(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_RemovePresetTour(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SendAuxiliaryCommand(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetConfiguration(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetHomePosition(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_SetPreset(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_Stop(NfOnvif *self, int fd, char *buff_rcv); 
int onvif_CreatePullPointSubscription(NfOnvif *self, int fd, char *buff_rcv);
int onvif_Subscribe(NfOnvif *self, int fd, char *buff_rcv);
int onvif_PullMessages(NfOnvif *self, int fd, char *buff_rcv);
int onvif_Renew(NfOnvif *self, int fd, char *buff_rcv);
int onvif_Unsubscribe(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetSynchronizationPoint(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRelayOptions(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetDigitalInputs(NfOnvif *self, int fd, char *buff_rcv);
int onvif_CheckDeviceReady(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetServices(NfOnvif *self, int fd, char *buff_rcv);
int onvif_Unsubscribe(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetPTZConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetAnalyticsModules(NfOnvif *self, int fd, char *buff_rcv);
int onvif_VAnalytics_Check_Token(NfOnvif *self, int fd, char *buff_rcv);
int onvif_ModifyAnalyticsModules(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetRules(NfOnvif *self, int fd, char *buff_rcv);
int onvif_ModifyRules(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_SetVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_AddVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_RemoveVideoAnalyticsConfiguration(NfOnvif *self, int fd, char *buff_rcv);
int onvif_makeVideoAnalyticsModule(arg_Motion *tmp, int get_modify);
int onvif_makeVideoAnalyticsRule(arg_Active_Cell *tmp, int get_modify);
int onvif_makeVideoAnalyticsConfiguration(arg_VanalyticsConfig *conf, int get_modify);

int onvif_SetSynchronizationPoint_trt(NfOnvif *self, int fd, char *buff_rcv);
int onvif_GetMetadataConfigurationOptions(NfOnvif *self, int fd, char *buff_rcv);

unsigned int packbits(unsigned char *src, unsigned char *dst, unsigned int n);
unsigned int unpackbits(unsigned char *outp, unsigned char *inp, unsigned int outlen, unsigned int inlen);

#endif//__NF_ONVIF_SERVER_APP_H__
