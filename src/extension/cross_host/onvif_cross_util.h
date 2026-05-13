/* panic recording�� ���� �ϴ� �Լ� 
priority : �켱 ���� ���� �� active�����϶� priority�� ���� ������ ���� ���� action�� �������� �׷��� �������� action�� ���� �����´�.
mode = Inactive
mode =  Active

ret : ������ ���¸� ������.
*/
#define NF_SYSMAN_HW_PARAM_MAX_LEN	32

#include <pthread.h>
 typedef struct _arg_DeviceRecordingSummary
{
	int dataFrom;
	int dataUntil;
	int recordingNum;
}arg_DeviceRecordingSummary;
typedef enum {
ONVIF_RECORDING_ALL,
ONVIF_RECORDING_STATUS, 
ONVIF_RECORDING_CLIP
}ONVIF_RECORDING_ENUM;

 typedef struct  _arg_RecordingInfo {

	int time;
	union data {
		int IsDataPresent; // Panic Recording clip ���� ������ true, �������� false �� ��.
		int IsRecording;
	};
	int type;
 }arg_recordingInfo;

 
int onvif_panic_recording(char* mode,int priority);
//definition for cross util
void onvif_cross_util_test(int a);
int checkRecordingExist(char* recordingToken);
int getJobModeString(int modint, char* mode);
void recordingJobSource(char* jobToken,arg_RecordingJobSource* source);
int getJobModeInt(char* mode);
int GetPanicRecording(int ch,char* mode);
int SetPanicRecording(int ch, char* mode);
gboolean GetDeviceRecordingSummary(arg_DeviceRecordingSummary *,int);
gboolean nf_sysman_get_hw_param_vendor(gchar vendor[NF_SYSMAN_HW_PARAM_MAX_LEN]);
static int PanicRecordingArray[ONVIF_CH];

int append_onvif_event_msg(unsigned int key, void *data, unsigned int state, int ss_id);
extern GArray* rec_arr,*eve_arr;
extern pthread_mutex_t ri_mutex;
int onvif_recording_event(int ch, int Max,int Filter, time_t start_time, time_t end_time );
int onvif_recording_metadata(int ch, int Max,int Filter, time_t start_time, time_t end_time );