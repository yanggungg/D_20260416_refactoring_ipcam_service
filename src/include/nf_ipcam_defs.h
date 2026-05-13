/*
 * ITX Security Co.,Ltd.
 *  System software section
 *
 *  2011-01-13 Author jykim
 */

#ifndef __NF_IPCAM_DEFS_H__
#define __NF_IPCAM_DEFS_H__


#include <arpa/inet.h>
/* GLib Object compatible */
#include <glib-object.h>
#include <nf_common.h>
#include <openssl/ssl.h>
#include "nf_api_ipcam.h"
#include "nf_api_dlva.h"


#define IPCAM_UNIT_TEST					(0)

#define MAX_VIDEO_STREAM				(3)

#define LOG								printf
#define CAM_LOG_DOMAIN					"CAMGR"
#define WAN_PORT_NUM					(9)
#define NAS_PORT_NUM					(8)
#define AVAILABLE_SET_PORT				(8)
#define AVAILABLE_MAX_CH				(32)
#define LINK_LOCAL_DEVICE				"eth1:0"
#define LOCAL_ETH_DEVICE				"eth1:1"
#define HUB_ETH_DEVICE					"eth1"
#define SUB_NETWORK_CHANGED				(1001)
#define SUB_NETWORK_UNCHANGED			(1000)


#define MVSW_CMD_ZERO                   (0)
#define MVSW_CMD_SHOW_PORTS             (1)
#define MVSW_CMD_SHOW_PHYS              (2)
#define MVSW_CMD_SHOW_ATU               (3)
#define MVSW_CMD_READ_REG               (4)
#define MVSW_CMD_WRITE_REG              (5)
#define MVSW_CMD_FIND_MAC               (6)
#define MVSW_CMD_FSTAT                  (7)
#define MVSW_CMD_RSTAT                  (8)

#define REG_PORT_STATUS					(0)
#define REG_PORT_CONTROL				(4)
#define LINK_BIT_MASK					(0x0800)

#if defined(_IPX_0824VE)||defined(_IPX_0412VE)||defined(_IPX_0824ECO)||defined(_IPX_0412ECO)
#define SWITCH_SYSCALL					(371)	// EA1.4 version system call
#elif defined(_KERNEL_HI3535)
	#define SWITCH_SYSCALL				(378)	// hi3535 kernel
#elif defined(_KERNEL_HI3536)
	#define SWITCH_SYSCALL				(384)	// hi3536 kernel
//	#define SWITCH_SYSCALL				(278)	// hi3536 kernel
#else
//#define SWITCH_SYSCALL				(371)	// PSP_12
#define SWITCH_SYSCALL					(370)	// EA1.4 version system call
//#define SWITCH_SYSCALL				(366)	// EA1.3 version system call
#endif

#if IPCAM_UNIT_TEST
#define MAKE_NOTIFY_FIRE				(0)
#define MAKE_NOTIFY_PROG				(0)
#define PRINT_PROGRESS_NOTIFY			(1)
#else
#define MAKE_NOTIFY_FIRE				(1)
#if defined(ENABLE_PROJECT_KMW)||defined(ENABLE_PROJECT_KUMMI)
  #define MAKE_NOTIFY_PROG              (0)
#else
  #define MAKE_NOTIFY_PROG              (1)
#endif
#define PRINT_PROGRESS_NOTIFY			(1)
#endif

#if defined(_IPX_0412M4) || defined(_IPX_0824M4) || defined(_IPX_1648M4) ||  defined(_IPX_0824P4E) ||  defined(_IPX_1648P4E) \
 || defined(_IPX_0412M4E) || defined(_IPX_0824M4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
	#define HUB_PORT						(9)
	#define WAN_PORT						(4)
#else
	#define HUB_PORT						(8)
	#define WAN_PORT						(9)
#endif	
//#define DISABLE_ONVIF_2ND_STREAM		(0)

#define ONVIF_MSG						static int
#define ONVIF_API						extern int
#define ONVIF_VNET_MAX					AVAILABLE_MAX_CH
#define ARP_PILE_MAX					(0x40)	// 32
#define NF_ONVIF_REBOOTING				(-101)

#define META_STREAM_LEN_MAX				(16384)

#define XM_DEFAULT_ALARM_SERVER_PORT	(610)
#define NF_IPCAM_EVENT_NOTI_PORT		(611)
#define RATE_SUSTENANCE_TIME			(30)
#define RATE_IGNORE_TIME				(5)

#define AUTO_SSL_OFF					(1)

#define AUTO_ONVIF_CAM_CONNECTION		(1)

#if defined(WIN32)
	#define MUTEX_TYPE            HANDLE
	#define MUTEX_SETUP(x)        (x) = CreateMutex(NULL, FALSE, NULL)
	#define MUTEX_CLEANUP(x)      CloseHandle(x)
	#define MUTEX_LOCK(x)         WaitForSingleObject((x), INFINITE)
	#define MUTEX_UNLOCK(x)       ReleaseMutex(x)
	#define THREAD_ID             GetCurrentThreadId()
#else
	#define MUTEX_TYPE            pthread_mutex_t
	#define MUTEX_SETUP(x)        pthread_mutex_init(&(x), NULL)
	#define MUTEX_CLEANUP(x)      pthread_mutex_destroy(&(x))
	#define MUTEX_LOCK(x)         pthread_mutex_lock(&(x))
	#define MUTEX_UNLOCK(x)       pthread_mutex_unlock(&(x))
	#define THREAD_ID             pthread_self()
#endif
 

#if 1
#define NF_ONVIF_ERROR(fmt, ...) \
do { \
	struct timespec a; \
	clock_gettime(CLOCK_REALTIME, &a); \
	printf("\e[1;32mONVIF\e[0m [%lu.%03lu] [%s] \e[1;31mERROR\e[0m | " fmt, \
			D_SECS(a), __FUNCTION__, ##__VA_ARGS__); \
} while (0)

#define NF_ONVIF_WARN(fmt, ...) \
do { \
	struct timespec a; \
	clock_gettime(CLOCK_REALTIME, &a); \
	printf("\e[1;32mONVIF\e[0m [%lu.%03lu] [%s] \e[1;33mWARN\e[0m | " fmt, \
			D_SECS(a), __FUNCTION__, ##__VA_ARGS__); \
} while (0)

#define NF_ONVIF_MINOR(fmt, ...) if(0)\
do { \
	struct timespec a; \
	clock_gettime(CLOCK_REALTIME, &a); \
	printf("\e[1;32mONVIF\e[0m [%lu.%03lu] [%s] | " fmt, \
			D_SECS(a), __FUNCTION__, ##__VA_ARGS__); \
} while (0)

#define NF_ONVIF_MAJOR(fmt, ...) if(0)\
do { \
	struct timespec a; \
	clock_gettime(CLOCK_REALTIME, &a); \
	printf("\e[1;32mONVIF\e[0m [%lu.%03lu] [%s] | " fmt, \
			D_SECS(a), __FUNCTION__, ##__VA_ARGS__); \
} while (0)

#define NF_ONVIF_DBG(level, fmt, ...) if(0)\
do { \
	NF_ONVIF_##level(fmt, ##__VA_ARGS__); \
} while (0)

#define D_SECS(a) ((a).tv_sec),(((a).tv_nsec)/1000/1000)

#define SOAP_ERROR_PRINT(soap) \
{ \
	char buff[1024]; \
	soap_sprint_fault((soap), buff, 1024); \
	fprintf(stderr, "[%s:%d] ERROR \n========================================\n\
%s\n========================================\n\n" \
			, __func__, __LINE__, buff); \
}



#else
#define NF_ONVIF_DBG if(0) printf
#endif

enum __ITX_CAM_SDK_TYPE_E_
{
	ITX_CAM_SDK_TYPE_DEPRECATED	= 0,
	ITX_CAM_SDK_TYPE_TI368		= 1,  // TI368 Camera
	ITX_CAM_SDK_TYPE_NPT		= 2,  // NPT Camera
	ITX_CAM_SDK_TYPE_NPT2		= 3,  // NPT2 Camera
	ITX_CAM_SDK_TYPE_IRPTZ		= 4,  // IRPTZ Camera
	ITX_CAM_SDK_TYPE_S1		= 5,  // S1 보급형
	ITX_CAM_SDK_TYPE_S1I		= 6,  // S1 지능형(Intelligence)
	ITX_CAM_SDK_TYPE_S1WL		= 7,  // S1 무선(Wireless)
	ITX_CAM_SDK_TYPE_NCX3		= 8,  // NCX3 Camera
	ITX_CAM_SDK_TYPE_DMVAI		= 9,  // DMVA 지능형(Intelligence)
	ITX_CAM_SDK_TYPE_TI385		= 10, // TI385 기능
	ITX_CAM_SDK_TYPE_A2		= 11, // A2카메라
	ITX_CAM_SDK_TYPE_HS		= 12, // Hisilicon 
	ITX_CAM_SDK_TYPE_NR		= 13
};

enum __IPX_MGMT_TABLE_STATE_E_
{
	MGMT_STATE_NULL				= 0,

	/* 0 ~ F */
	MGMT_STATE_LINKED			= 1<<0,
	MGMT_STATE_UNLINKED			= 1<<1,
	//MGMT_STATE_UNDEFINED		= 1<<2,
	//MGMT_STATE_UNDEFINED		= 1<<3,

	/* 00 ~ F0 */
	MGMT_STATE_ONVIF_ARP		= 1<<4,
	MGMT_STATE_ONVIF_STATIC		= 1<<5,
	MGMT_STATE_ONVIF_DYNAMIC	= 1<<6,
	MGMT_STATE_ONVIF_IPDONE		= 1<<7,

	/* 000 ~ F00 */
	MGMT_STATE_READY			= 1<<8,
	MGMT_STATE_WAITING			= 1<<9,
	MGMT_STATE_CONFIGURED		= 1<<10,
	MGMT_STATE_USING			= 1<<11,

	/* 0000 ~ F000 */
	MGMT_STATE_IDPASS_WAITING	= 1<<12,
	//MGMT_STATE_UNDEFINED		= 1<<13,
	//MGMT_STATE_UNDEFINED		= 1<<14,
	//MGMT_STATE_UNDEFINED		= 1<<15,

	/* 0 0000 ~ F 0000 */
	MGMT_STATE_TIMEOUT			= 1<<16,
	MGMT_STATE_UNKNOWN			= 1<<17,
	//MGMT_STATE_UNDEFINED		= 1<<18,
	//MGMT_STATE_UNDEFINED		= 1<<19,

	/* 00 0000 ~ F0 0000 */
	MGMT_STATE_HANDLING_LINK	= 1<<20,
	MGMT_STATE_HANDLING_UNLINK	= 1<<21,
	//MGMT_STATE_UNDEFINED		= 1<<22,
	MGMT_STATE_HANDLING			= 1<<23,

	/* 000 0000 ~ F00 0000 */
	OPENMODE_STATE_CONFIGURING	= 1<<24,
	OPENMODE_STATE_CONFIG_FAIL	= 1<<25,
	OPENMODE_STATE_READY		= 1<<26,
	//MGMT_STATE_UNDEFINED		= 1<<27,

	/* 0000 0000 ~ F000 0000 */
	//MGMT_STATE_UNDEFINED		= 1<<28,
	//MGMT_STATE_UNDEFINED		= 1<<29,
	//MGMT_STATE_UNDEFINED		= 1<<30,
	//MGMT_STATE_UNDEFINED		= 1<<31,

	MGMT_STATE_NR				= 32
};

enum __IPX_IPCAM_DELAYED_SETUP_TYPE_E_
{
	IPCAM_DS_RATE_DOWN,
	IPCAM_DS_RATE_RECOVER,
	IPCAM_DS_SET_ENCODER,
	IPCAM_DS_MOTION,
	IPCAM_DS_XM_EVENT,
	IPCAM_DS_TYPE_MAX
};

enum __IPX_IPCAM_VLOSS_TYPE_E_
{
	IPCAM_VLOSS_ADD_NEW_CH = 0,
	IPCAM_VLOSS_REMOVE_CH,
	IPCAM_VLOSS_REMOVE_ALL,
	IPCAM_VLOSS_TYPE_MAX
};

enum __IPX_IPCAM_EVENT_TYPE_E_
{
	IPCAM_EVENT_NULL,

	IPCAM_EVENT_LINK_CHANGED,

	IPCAM_EVENT_DELAY_TO_CONN,
	IPCAM_EVENT_DELAY_READY,
	IPCAM_EVENT_DEVICE_READY,
	IPCAM_EVENT_DEVICE_OUT,

	IPCAM_EVENT_NVS_SUBCH_READY,
	IPCAM_EVENT_NVS_SUBCH_CLOSE,

	IPCAM_EVENT_STREAM_READY,
	IPCAM_EVENT_ONVIF_PRIME_READY,
	IPCAM_EVENT_ONVIF_GENERAL_READY,

	IPCAM_EVENT_UNKNOWN_DEVICE,
	IPCAM_EVENT_CONNECTION_FAIL,
	IPCAM_EVENT_LOGIN_FAIL,
	IPCAM_EVENT_MODEL_UNSUPPORT,
	IPCAM_EVENT_CONFIG_FAIL,
	IPCAM_EVENT_STREAM_FAIL,

	IPCAM_EVENT_TYPE_MAX
};

enum __IPX_IPCAM_RESULTS_
{
	IPX_SEARCH_CONNECTION_FAILED = 0,
	IPX_SEARCH_LOGIN_FAIL,
	IPX_SEARCH_FOUND_NOT_SUPPORTED,
	IPX_SEARCH_FOUND,
	IPX_SEARCH_FOUND_AND_SET,

	IPX_SETUP_INIT_FAILED,
	IPX_SETUP_GET_FAILED,
	IPX_SETUP_SET_FAILED,
	IPX_SETUP_DONE
};

enum __IPX_IPCAM_SETUP_RTN_E_
{
	IPCAM_SETUP_RTN_FAILED,
	IPCAM_SETUP_RTN_DONE,
	IPCAM_SETUP_RTN_PENDING
};

enum __IPX_HUB_STATUS_NOTIFY_VALUE_E_
{
	IPX_HUB_STATUS_UNLINKED,
	IPX_HUB_STATUS_LINKED,
	IPX_HUB_STATUS_CONNECTING,
	IPX_HUB_STATUS_CONNECTED,
	IPX_HUB_STATUS_FWUP_START,
	IPX_HUB_STATUS_FWUP_END
};

enum __IPCAM_DISCOVERY_STATE_E_
{
	DISCOVERY_STOPPED = 0,
	DISCOVERY_RUNNING = 1,
	DISCOVERY_HANDLER_RUNNING = 2
};

enum __CONN_TYPE_E_
{
	CONN_TYPE_ITX_LOCAL = 0,
	CONN_TYPE_ITX_MANAGED_SWITCH,
	CONN_TYPE_ITX_OPEN,
	CONN_TYPE_ONVIF_LOCAL,
	CONN_TYPE_ONVIF_MANAGED_SWITCH,
	CONN_TYPE_ONVIF_OPEN
};

enum __PROTO_OPCODE_
{
	MSG_IP_SEARCH = 1,
	MSG_CAM_ACK,
	MSG_IP_ACK,
	MSG_IP_SET,
	MSG_CAM_SET,
	MSG_IPUTIL_SEARCH,
	MSG_IPUTIL_RUN
};

enum __CAM_AI_TYPE_
{
	CAM_AI_TYPE_NORMAL = 0,
	CAM_AI_TYPE_PRO = 1,
	CAM_AI_TYPE_CLAIR2 = 2,
	CAM_AI_TYPE_CLAIR1
};

enum __RESOL_RATIO_
{
	RESOL_16_9 = 0,
	RESOL_4_3,
	RESOL_5_4,
	RESOL_1_1
};

struct __NETCONF_MSG_T_
{
	unsigned char	version;
	unsigned char	opcode;
	unsigned short	secs;
	unsigned int	xid;
	unsigned int	magic;
	unsigned int	ciaddr;
	unsigned char	chaddr[16];
	unsigned int	yiaddr;
	unsigned int	miaddr;
	unsigned int	giaddr;
	unsigned int	d1iaddr;
	unsigned int	d2iaddr;
	unsigned short	http_port;
	unsigned short	https_port;
	unsigned short	rtsp_port;
	unsigned short	reserve_port;
	unsigned char	vend[64];
} __attribute__((packed));
typedef struct __NETCONF_MSG_T_ netconf_msg;
struct __MGMT_RCV_T_
{
	int state;		// __MGMT_TABLE_STATE_
	unsigned char macaddr[6];
	unsigned int ipaddr;
	unsigned int transaction;
};
typedef struct __MGMT_RCV_T_ rtbl;

typedef struct _CAM_SETUP_INFO__ cam_info;
struct _CAM_SETUP_INFO__
{
	union
	{
		struct {
			int installation_mode;
			int reserved[10];
		} install;

		struct {
			uint64_t resolution[3];
			unsigned int fps[3];
			unsigned int bitrate[3];
			unsigned int mirror;
			unsigned int af;
			unsigned int capture;
			unsigned int bitctrl[3];
			unsigned int vcodec[3];
		} vcodec;

		struct {
			unsigned int audio_tx;
			unsigned int audio_rx;
			unsigned int audio_codec;
			unsigned int mic_volume;
			unsigned int speaker_volume;
			int reserved[6];
		} acodec;

		struct {
			int brightness;
			int contrast;
			int color;
			int tint;
			int reserved[7];
		} image;

		struct {
			int in_onoff;
			int in_type;
			int out_onoff;
			int out_type;
			int reserved[7];
		} alarm;

		struct {
			int mfz;
			int iris;
			int zoom_min;
			int zoom_max;
			int focus_min;
			int focus_max;
			int iris_min;
			int iris_max;
			int reserved[3];
		} afcapa;
	};
};

typedef struct _CAM_SETUP_IMAGE__ image_info;
struct _CAM_SETUP_IMAGE__
{
	int ae;
	int agc;
	int shutter;
	int ss;
	int max_agc;
	int iris;
	int blc;
	int dnn;
	int dnn_time;
	int awb;
	int mwb;
	int sharpness;
	int brightness;
	int contrast;
	int color;
	int tint;

	int wd;
	int focus_mode;

	int ff_mode;
	int max_shutter;
	int base_shutter; 		// Base Shutter Speed

	int dnr_ctr;			// 3D DNR Control
	int adaptive_ir;

	int dnn_sense_ntod;		// Day & Night Sensitivity Night to Day
	int dnn_sense_dton;		// Day & Night Sensitivity Day to Night

	int defog;
	int hlc;

	int focus_limit;
	int stabilizer;
	int ir_correction;

	unsigned int dnn_start_hour;
	unsigned int dnn_end_hour;
	unsigned int dnn_start_min;
	unsigned int dnn_end_min;

	unsigned int colorvu_ctrl;
	unsigned int colorvu_level;
};

#define FOCUS_AREA_MAX	1
typedef struct _CAM_SETUP_FOCUS_COMP__ focus_comp_info;
struct _CAM_SETUP_FOCUS_COMP__
{
	unsigned int tem_comp_mode;
	unsigned int dnn_comp_mode;
	unsigned int maskarea[FOCUS_AREA_MAX];
	unsigned int areatx[FOCUS_AREA_MAX];
	unsigned int areaty[FOCUS_AREA_MAX];
	unsigned int areabx[FOCUS_AREA_MAX];
	unsigned int areaby[FOCUS_AREA_MAX];
};

/*
 * ONVIF imaging service
 */


typedef struct _CAM_SETUP_VIDEO_ENCODER_ONVIF_ video_encoder_onvif;
struct _CAM_SETUP_VIDEO_ENCODER_ONVIF_
{
	int width;				// Resolution->Width
	int height;				// Resolution->Height
	int quality;			// Quality

	int encoding;			// Encoding

	int frameratelimit;		// RateControl->FrameRateLimit
	int encodinginterval;	// RateControl->EncodingInterval
	int bitratelimit;		// RateControl->BitrateLimit

	int govlength;			// GovLength
	int profile;			// H264Profile
};


typedef struct _CAM_SETUP_EXPOSURE_ONVIF_ image_exposure_onvif;
struct _CAM_SETUP_EXPOSURE_ONVIF_
{
	int mode;		// Exposure->Mode
	int priority;	// Exposure->Priority

	int minetime;	// Exposure->MinExposureTime
	int maxetime;	// Exposure->MaxExposureTime
	int etime;		// Exposure->ExposureTime

	int mingain;	// Exposure->MinGain
	int maxgain;	// Exposure->MaxGain
	int gain;		// Exposure->Gain

	int miniris;	// Exposure->MinIris
	int maxiris;	// Exposure->MaxIris
	int iris;		// Exposure->Iris

	int bottom;		// Exposure->Window->bottom
	int top;		// Exposure->Window->top
	int right;		// Exposure->Window->right
	int left;		// Exposure->Window->left
        int iris_mode;
};

typedef struct _CAM_SETUP_FOCUS_ONVIF_ image_focus_onvif;
struct _CAM_SETUP_FOCUS_ONVIF_
{
	int mode;		// Focus->AutoFocusMode
	int defaultspeed;	// Focus->DefaultSpeed
	int nearlimit;	// Focus->NearLimit
	int farlimit;	// Focus->FarLimit
};

typedef struct _CAM_SETUP_WB_ONVIF_ image_wb_onvif;
struct _CAM_SETUP_WB_ONVIF_
{
	int mode;		// WhiteBalance->Mode
	int crgain;		// WhiteBalance->CrGain
	int cbgain;		// WhiteBalance->CbGain
};

typedef struct _CAM_FOCUS_MOVE_ONVIF_ focus_move_onvif;
struct _CAM_FOCUS_MOVE_ONVIF_
{
	int mode;		// Absolute, Relative, Continuous
	int position;	// Absolute->Position
	int distance;	// Relative->Distance
	int speed;		// ...->Speed
};

typedef struct _CAM_SETUP_IMAGE_ONVIF_ image_info_onvif;
struct _CAM_SETUP_IMAGE_ONVIF_
{
	int brightness;	// Brightness
	int color;		// ColorSaturation
	int contrast;	// Contrast
	int sharpness;	// Sharpness

	image_exposure_onvif exposure;	// Exposure

	image_focus_onvif focus;			// Focus

	image_wb_onvif wb;				// WhiteBalance

	int wdrmode;	// WideDynamicRange->Mode
	int wdrlevel;	// WideDynamicRange->Level

	int ircut;		// IrCutFilter

	// later use
	int blcmode;	// BacklightCompensation->Mode
	int blclevel;	// BacklightCompensation->Level

	int imgsmode;	// Extension->ImageStabilization->Mode
	int imgslevel;	// Extension->ImageStabilization->Level
};

typedef struct _CAM_SETUP_PTZ_ONVIF ptz_info_onvif;
struct _CAM_SETUP_PTZ_ONVIF
{
	int mode;		// Absolute, Relative, Continuous

	int ePTZAreaId; // fisheye split view each area number

	int absolute_pan;
	int absolute_tilt;
	int absolute_zoom;
	int relative_pan;
	int relative_tilt;
	int relative_zoom;
	int speed_pan;
	int speed_tilt;
	int speed_zoom;
};

typedef struct _CAM_SETUP_PTZ_ ptz_info;
struct _CAM_SETUP_PTZ_
{
	int cmd;
	int pt_speed;
	int zoom_speed;
	int focus_speed;
	int iris_speed;
};

typedef struct _NFIPCamCallbacks NFIPCamCallbacks;
struct _NFIPCamCallbacks
{
	int picked;
	int sock;
	int run_cnt;

	int reserved;

	NFIPCamSetupCallback cb_func;
	gpointer user_data;
	NFIPCamProgress progress;

	NFIPCamSetupCallback cb_next;
	gpointer user_data_next;

	NFIPCamSetupMotionArea data_next;
};

#if 0
typedef struct _CAM_MOTION_INFO__ motion_info;
struct _CAM_MOTION_INFO__
{
	int activation;
	int sensitivity;
	int min_block;
	int num_blocks;
	char area[256];
};
#endif

typedef enum __MGMT_SYSTEM_DEVICE_TYPE_E_ {
	SYSTEM_DEVICE_IPCAM		= 1<<0,
	SYSTEM_DEVICE_NVS		= 1<<1,
	SYSTEM_DEVICE_NR		= 1<<2
} SYSTEM_DEVICE_TYPE_E;

typedef struct _CAM_MODEL_INFO__ cam_model_info;
struct _CAM_MODEL_INFO__
{
	unsigned char mac[8];
	char name[128];
	char swver[64];
	char swver2[64];
	char vendor[128];

	char stdver[64];
	char sdkver[64];

	SYSTEM_DEVICE_TYPE_E type;
	int nvs_sub_ch;
	int vstype; // 0 : NTSC, 1 : PAL
	
	char zoom_module_name[64];
	char zoom_module_fwver[64];

	char capa_version[16];
};

typedef struct __MGMT_VALUE_T_ values_t;
struct __MGMT_MVAL_T
{
	int min;
	int max;
	int value;
};

typedef struct __MGMT_VALUE_T_ values;
struct __MGMT_VALUE_T_
{
	int min;
	int max;
	int value;

	// 20120723 to convert float to int(normally 1)
	// ex) min = 0.001 -> min = 1 and multix = 1000
	int multiplier;
	int diff;
	uint64_t dep_cate;
};

typedef struct __MGMT_MODE_T_ modes;
struct __MGMT_MODE_T_
{
	int support;
	int value;
};

typedef struct __MGMT_MOTION_INFO_T_ motion_info;
struct __MGMT_MOTION_INFO_T_
{
	int block_width;
	int block_height;
	MOTION_AREA_METHOD_E area_method;
	int max_area_cnt;
	MAREA marea[MAX_MOTION_AREA];
};


typedef struct __MGMT_TIME_T_ nf_time;
struct __MGMT_TIME_T_
{
	unsigned int hour;
	unsigned int min;
};

typedef struct __MGMT_DURATION_T_ duration;
struct __MGMT_DURATION_T_
{
	nf_time start;
	nf_time end;
};


typedef struct __IPCAM_VCA_CAPA_VERSION__
{
	int major;
	int minor;
	int sub;

}vca_version_t;

typedef struct __IPCAM_VCA_PROFILE__
{
	unsigned int support;
	vca_version_t version;
}vca_t;

typedef struct __NF_IPCAM_AI_CAPABILITIEIS__
{
	vca_version_t version;

    ai_license_data license[IPX_LICENSE_KEY_MAX];
    int license_length;
    aibox_rule_data rules[32];
    size_t rule_size;
    unsigned int status;
    time_t last_recv;
	int is_rule_engine;
	int ai_support; // metadata = 1 , not metadata (ex. sens) = 0

	int model_type_support;
	NF_AI_MODEL_TYPE_E model_type_value;
}ai_t;

typedef struct __NF_IPCAM_AI_RULE_INFO__
{
	// zone
	int type;
	char zone_name[32];
	int zone_id;
	int zone_color;
	int zone_active;
	char zone_object[256];
	int zone_event;
	char zone_timeout[24];
	char zone_area[192];
	int npts;

	// counter
	char count_name[32];
	int count_id;
	int count_color;
	int count_active;
	int count_event;
	int count_threshold;
	char count_area[192];
	char zid[12];
	int e_value;
	int reset_alert;
} rule_info;

typedef struct __NF_IPCAM_AI_RULE_ENGINE__
{
	int have_ai_engine;
	int z_cnt;
	int c_cnt;
	int en_engine;

	rule_info rule[16];
} ai_rule_engine;

typedef struct __NF_IPCAM_AI_DL_OPTION__
{
	unsigned int track_ref; // centroid, ground point
	unsigned int en_static_filter; // check box on/off
	unsigned int static_filter_sense; // low, mid, high
} ai_dl_option;

typedef struct __NF_IPCAM_EMBEDDED_OSD__
{
	unsigned int display_mode;
	unsigned int object_color;
	unsigned int rule_color;
	unsigned int event_color;
	unsigned int line_width;
	unsigned int line_transparency;
	char object_type[255];
} embedded_osd;

enum __IPCAM_SSL_STATE_E_
{
	IPCAM_SSL_NOT_AVAILABLE = 0,
	IPCAM_SSL_INIT,
	IPCAM_SSL_CONNECTING,
	IPCAM_SSL_WAITING,
	IPCAM_SSL_SHUTDOWN
};
typedef struct __MGMT_SYSTEM_T_ system_t;
struct __MGMT_SYSTEM_T_
{
	unsigned char macaddr[8];
	unsigned int ipaddr;
	unsigned int transaction;

	unsigned int rx_method;

	char rtsp_url[MAX_VIDEO_STREAM][256];
	unsigned short rtsp_port[MAX_VIDEO_STREAM];
	unsigned short http_port;
	unsigned short use_ssl;

	SSL *ssl[NF_IPCAM_TYPE_MAX];
	SSL_CTX *ctx[NF_IPCAM_TYPE_MAX];
	SSL *ssl_g;
	SSL_CTX *ctx_g;
	pthread_mutex_t ssl_mtx[NF_IPCAM_TYPE_MAX];
	unsigned int ssl_state[NF_IPCAM_TYPE_MAX];

	int retry_cnt;
	int progress;

	int model_code;

	char model[64];
	char swver[64];
	char swver2[64];
	char vendor[64];

	char stdver[64];
	char sdkver[64];

	SYSTEM_DEVICE_TYPE_E type;
	int nvs_sub_ch;
	int nvs_stream_start;

	char zoom_module_name[64];
	char zoom_module_fwver[64];
};

typedef struct __MGMT_FOCUS_T_ focus_t;
struct __MGMT_FOCUS_T_
{
	unsigned int supported;
	modes tem_comp;
	modes dnn_comp;
	values_t maskarea[FOCUS_AREA_MAX];
	values_t areatx[FOCUS_AREA_MAX];
	values_t areaty[FOCUS_AREA_MAX];
	values_t areabx[FOCUS_AREA_MAX];
	values_t areaby[FOCUS_AREA_MAX];
};



typedef enum __VIDEO_SETUP_TYPE_E_ {
	VIDEO_SETUP_RESOLUTION		= 1<<0,
	VIDEO_SETUP_FPS				= 1<<1,
	VIDEO_SETUP_BITRATE			= 1<<2,
	VIDEO_SETUP_ANTIFLICKER		= 1<<3,
	VIDEO_SETUP_MIRROR			= 1<<4,
	VIDEO_SETUP_CAPTURE_MODE 	= 1<<5,
	VIDEO_SETUP_BITRATE_CONTROL = 1<<6,
	VIDEO_SETUP_VCODEC 			= 1<<7,
	VIDEO_SETUP_NR			= 8 
} VIDEO_SETUP_TYPE_E;
typedef struct __VIDEO_RESOLUTION_T_ resolution_t;
struct __VIDEO_RESOLUTION_T_
{
	uint64_t supported;
	uint64_t resolution[MAX_VIDEO_STREAM];
};
typedef struct __MGMT_VIDEO_T_ video_t;
struct __MGMT_VIDEO_T_
{
	unsigned int supported;		// VIDEO_SETUP_TYPE_E
	unsigned int onthefly;		// VIDEO_SETUP_TYPE_E

	int          stream_cnt;		// value at least 1 (to MAX_VIDEO_STREAM)
	unsigned int vcodec[MAX_VIDEO_STREAM];
	resolution_t resolution;
	modes        fps[2][MAX_VIDEO_STREAM];
	values_t     bitrate[MAX_VIDEO_STREAM];
	unsigned int quality[MAX_VIDEO_STREAM][NF_IPCAM_QUALITY_MAX];
	modes        anti_flicker;
	modes        mirror;
	modes        capture;
	unsigned int bitctrl[MAX_VIDEO_STREAM];

	unsigned int corridor_mode_value; // current value
	unsigned int corridor_mode_min;
	unsigned int corridor_mode_max;
	unsigned int corridor_support;
};


typedef struct __MGMT_ENCODER_T_ encoder_t;
struct __MGMT_ENCODER_T_
{
	uint64_t res_support[3];
	unsigned int max_framerate[3];
	unsigned int min_bitrate[3];
	unsigned int max_bitrate[3];
	unsigned int cur_maxfps[3];
	unsigned int bitctrl[3];
	unsigned int vcodec[3];
};


typedef struct __MGMT_AUDIO_T_ audio_t;
struct __MGMT_AUDIO_T_
{
	modes        acodec;
	int          audio_rx;
	int          audio_tx;
	values_t     mic_volume;
	values_t     speaker_volume;
};

typedef struct __MGMT_ALARM_T_ alarm_t;
struct __MGMT_ALARM_T_
{
	unsigned int alarm_in;
	modes        alarm_in_type;
	unsigned int alarm_out;
	modes        alarm_out_type;
};

typedef struct __MGMT_MOTION_T_ motion_t;
struct __MGMT_MOTION_T_
{
	unsigned int method;

	values_t     sensitivity;
	unsigned int block_width;
	unsigned int block_height;
	unsigned int num_block;
	unsigned int min_block;
	unsigned int max_rect;
	unsigned char* stream_snap;
	unsigned int smart_motion_support;
	unsigned int smart_motion_enable;
	unsigned int ai_alarm_event;
	NFIPCamMotionSmartOption smart_motion_options[MAX_AI_MOTION];
	guint smart_motion_option_size;
};

typedef struct __MGMT_PRIVACY_MASK_T_ pmask_t;
struct __MGMT_PRIVACY_MASK_T_
{
	unsigned int block_width;
	unsigned int block_height;
	unsigned int max_rect;
};

typedef struct __MGMT_ROI_AREA_T_ roi_t;
struct __MGMT_ROI_AREA_T_
{
	unsigned int block_width;
	unsigned int block_height;
	unsigned int max_rect;
	unsigned int chipset;
	unsigned int options;
};

typedef struct __MGMT_IMAGE_T_ image_t;
struct __MGMT_IMAGE_T_
{
	uint64_t supported;
	uint64_t onthefly;

	modes exposure;
	values_t agc;
	values_t eshutter_speed;
	modes slow_shutter;
	modes max_agc;
	modes iris;
	modes blc;

	modes day_night;
	modes tg_time;

	modes wb;
	modes mwb;

	values_t sharpness;
	values_t brightness;
	values_t contrast;
	values_t color;
	values_t tint;

	modes wd;
	modes focus_mode;

	modes anti_flicker;
	modes max_shutter;

	modes dnr_ctr;

	modes adaptive_ir;
	values_t dnn_sense_ntod;
	values_t dnn_sense_dton;
	modes defog;
	modes hlc;

	modes focus_limit;
	modes stabilizer;
	modes ir_correction;
	modes base_shutter;
	uint64_t base_shutter_table[15][6];
	duration dnn_schedule;

	modes colorvu_ctrl;
	values_t colorvu_level;
};

typedef struct __MGMT_EXPOSURE_T_ONVIF_ image_exposure_t_onvif;
struct __MGMT_EXPOSURE_T_ONVIF_
{
	modes mode;			// Exposure->Mode
	modes priority;		// Exposure->Priority

	values minetime;	// Exposure->MinExposureTime
	values maxetime;	// Exposure->MaxExposureTime
	values etime;		// Exposure->ExposureTime

	values mingain;		// Exposure->MinGain
	values maxgain;		// Exposure->MaxGain
	values gain;		// Exposure->Gain

	values miniris;		// Exposure->MinIris
	values maxiris;		// Exposure->MaxIris
	values iris;		// Exposure->Iris

	values bottom;		// Exposure->Window->bottom
	values top;			// Exposure->Window->top
	values right;		// Exposure->Window->right
	values left;		// Exposure->Window->left
        modes iris_mode;	// Exposure->Mode
};

typedef struct __MGMT_FOCUS_T_ONVIF_ image_focus_t_onvif;
struct __MGMT_FOCUS_T_ONVIF_
{
	modes mode;			// Focus->Mode
	values defaultspeed;// Focus->DefaultSpeed
	values nearlimit;	// Focus->NearLimit
	values farlimit;	// Focus->FarLimit
};

typedef struct __MGMT_WB_T_ONVIF_ image_wb_t_onvif;
struct __MGMT_WB_T_ONVIF_
{
	modes mode;			// WhiteBalance->Mode
	values crgain;		// WhiteBalance->CrGain
	values cbgain;		// WhiteBalance->CbGain
};

typedef struct __MGMT_FOCUS_MOVE_T_ONVIF_ focus_move_t_onvif;
struct __MGMT_FOCUS_MOVE_T_ONVIF_
{
	modes mode;			// FocusMove->
	values abposition;	// Absolute->Position
	values abspeed;		// Absolute->Speed
	values redistance;	// Relative->Distance
	values respeed;		// Relative->Speed
	values cospeed;		// Continuous->Speed
};

typedef struct __MGMT_IMAGE_T_ONVIF_ image_t_onvif;
struct __MGMT_IMAGE_T_ONVIF_
{
	//unsigned int supported;
	uint64_t supported_image;
	uint64_t supported_exposure;
	uint64_t onthefly_image;
	uint64_t onthefly_exposure;

	values brightness;	// Brightness
	values color;		// ColorSaturation
	values contrast;	// Contrast
	values sharpness;	// Sharpness

	image_exposure_t_onvif exposure;// Exposure

	image_focus_t_onvif focus;		// Focus

	image_wb_t_onvif wb;			// WhiteBalance

	focus_move_t_onvif move;		// FocusMove

	modes wdrmode;		// WideDynamicRange->Mode
	values wdrlevel;	// WideDynamicRange->Level

	modes ircut;		// IrCutFilter

	// later use
	modes blcmode;		// BacklightCompensation->Mode
	values blclevel;	// BacklightCompensation->Level

	modes imgsmode;		// Extension->ImageStabilization->Mode
	values imgslevel;	// Extension->ImageStabilization->Level

	// videosource.rotate
	modes mirror;
	modes adaptive_ir;
	values dnn_sense_ntod;	// DNN Sensitivity Night to Day
	values dnn_sense_dton;	// DNN Sensitivity Day to Night
	modes defog;
	modes hlc;

	modes focus_limit;
	modes stabilizer;
	modes ir_correction;
};

typedef struct __MGMT_PTZ_T_ONVIF_ ptz_t_onvif;
struct __MGMT_PTZ_T_ONVIF_
{
	unsigned int support_ptz;

	values continuous_pan;
	values continuous_tilt;
	values continuous_zoom;

	values timeout;

	// support later
	values absolute_pan;
	values absolute_tilt;
	values absolute_zoom;
	values relative_pan;
	values relative_tilt;
	values relative_zoom;
	// speed use only absolute&relative
	values pan_speed;
	values tilt_speed;
	values zoom_speed;

	int preset_cnt;

	int supported_preset_cnt;	//add
	int current_preset_cnt;		//add
	int preset_num[256];		//add

	char preset_token[256][64];

	char preset_name[256][64]; 	//add

	NFIPCamAuxiliary auxiliary;
};

typedef struct __MGMT_PTZ_PRESET_T_ ptz_preset;
struct __MGMT_PTZ_PRESET_T_
{
	int preset_cnt;
	int preset_number[16];
	char preset_name[16][64];
	char preset_token[16][64];
};

typedef enum _PTZ_SETUP_FUNCS_E_
{
	PTZ_SETUP_PAN				= 1<<0,
	PTZ_SETUP_TILT				= 1<<1,
	PTZ_SETUP_ZOOM				= 1<<2,
	PTZ_SETUP_FOCUS				= 1<<3,
	PTZ_SETUP_IRIS				= 1<<4,
	PTZ_SETUP_PRESET			= 1<<5,
	PTZ_SETUP_ONEPUSH			= 1<<6,
	PTZ_SETUP_CALIBRATION		= 1<<7,
	PTZ_SETUP_ZOOM_NONPTZ		= 1<<8,
	PTZ_SETUP_FOCUS_NONPTZ		= 1<<9,
	PTZ_SETUP_IRIS_NONPTZ		= 1<<10,
	PTZ_SETUP_NR				= 11
} PTZ_FUNCS_E;
typedef struct __MGMT_PTZ_T_ ptz_t;
struct __MGMT_PTZ_T_
{
	unsigned int moving;
	unsigned int supported;

	values speed;

	values pan;
	values tilt;
	values zoom;
	values focus;
	values iris;
};

enum __NF_ONVIF_SERVICE_E_
{
	NF_ONVIF_SERVICE_DEVICE = 0,
	NF_ONVIF_SERVICE_MEDIA,
	NF_ONVIF_SERVICE_IMAGE,
	NF_ONVIF_SERVICE_EVENT,
	NF_ONVIF_SERVICE_PTZ,
	NF_ONVIF_SERVICE_ANALYTICS,
	NF_ONVIF_SERVICE_MEDIA2,
	NF_ONVIF_SERVICE_NR
};
#define __OFM(a) (1<<a)	// ONVIF Function Mask

enum __NF_ONVIF_AUTH_METHODS_E_
{
	NF_ONVIF_AUTH_NONE = 0,
	NF_ONVIF_AUTH_TEXT,
	NF_ONVIF_AUTH_DIGEST,
	NF_ONVIF_AUTH_MAX,

};

// Onvif Support Service

struct __ONVIF_DEVICE_IO_T_
{
	int supported;
	int InputConnectors;
	int RelayOutputs;
};
struct __ONVIF_DEVICE_SERVICE_T_
{
	int supported;
	struct __ONVIF_DEVICE_IO_T_ IO;
};
struct __ONVIF_MEDIA_SERVICE_T_
{
	int supported;
};
struct __ONVIF_PTZ_SERVICE_T_
{
	int supported;
};
struct __ONVIF_ANALYTICS_SERVICE_T_
{
	int supported;
	int CellBasedSceneDescriptionSupported;
	int RuleSupport;
	int AnalyticsModuleSupport;
};
struct __ONVIF_DEVICEIO_SERVICE_T_
{
	int supported;
};
struct __ONVIF_IMAGING_SERVICE_T_
{
	int supported;
};
struct __ONVIF_EVENT_SERVICE_T_
{
	int supported;
};

typedef struct __MGMT_ONVIF_SERVICE_T_ onvif_service_t;
struct __MGMT_ONVIF_SERVICE_T_
{
	struct __ONVIF_DEVICE_SERVICE_T_ device;
	struct __ONVIF_MEDIA_SERVICE_T_ media;
	struct __ONVIF_PTZ_SERVICE_T_ ptz;
	struct __ONVIF_ANALYTICS_SERVICE_T_ analytics;
	struct __ONVIF_DEVICEIO_SERVICE_T_ deviceio;
	struct __ONVIF_IMAGING_SERVICE_T_ imaging;
	struct __ONVIF_EVENT_SERVICE_T_ event;
};


enum ONVIF_MOTION_TYPE {
	OMT_CELL 	= 1, 		// Cell Motion Detection
	OMT_REGION	= 2,		// Hitorn Type Mototion
	OMT_MOTION	= 3,		// LG Camera Type Motion
	OMT_NONE	= 4,		// NONE Type
};

typedef struct __MGMT_ONVIF_T_ onvif_t;
struct __MGMT_ONVIF_T_
{
	int vnet_id;
	unsigned int onvif_service;

	char xaddr_dev_tail[128];
	char xaddr[NF_ONVIF_SERVICE_NR][128];

	int auth_method;	// __NF_ONVIF_AUTH_METHODS_E_

	/* Common devices */
	char profile_token[MAX_VIDEO_STREAM][64];
	int gov_length;
	int width;
	int height;
	int pixels;

	int waiting;

	int audio_enable;
	int audio_bitrate;
	int audio_samplerate;

	char vs_token[64];
	char vsc_token[MAX_VIDEO_STREAM][64];		// VideoSourceConfiguration Token
	char vec_token[MAX_VIDEO_STREAM][64];		// VideoEncoderConfiguration Token
	float vec_min_quality[MAX_VIDEO_STREAM][2];
	float vec_max_quality[MAX_VIDEO_STREAM][2];
	float vec_cur_quality[MAX_VIDEO_STREAM];

	int vac_type;     // Video Analytics Configuration Type 
	char vac_token[64]; // Video Analytics Configuration Token

	char ptz_token[64];	// PTZ Token

	char xaddr_event[128];	// CreatePullPointSubscription

	int query_onthefly_state;

	NFIPCamAuxiliary auxiliary;
};

struct __FISHEYE_MOUNT_T_
{
	int value;
};

typedef struct __MGMT_FISHEYE_T_ fisheye_t;
struct __MGMT_FISHEYE_T_
{
	uint64_t fisheye_supported;

	modes mount;
	modes dewarp;
};

typedef struct __MGMT_TABLE_T_ mtable;
struct __MGMT_TABLE_T_
{
	int state;		// __MGMT_TABLE_STATE_E_

	uint64_t func;
	int profile_id;

	int conn_type;	// __CONN_TYPE_E_

	char username[64];
	char password[64];

	unsigned short admin_http;
	unsigned short admin_rtsp;

	int onvif_wait;
	int intensive_ch;		// intensively viewing this channel
	int rate_control;
	int rec_changed;
	unsigned int motion_flag;
	unsigned int alarm_flag;

	//unsigned int corridor_mode;

	int delayed;
	enum __IPX_IPCAM_EVENT_TYPE_E_ next_event;

	int comp_init;
	int comp_x;
	int comp_y;

	system_t		sys;
	encoder_t		encoder;
	video_t			video;
	audio_t			audio;
	image_t			image;
	ptz_t			ptz;
	alarm_t			alarm;
	motion_t		motion;
	onvif_t			onvif;
	focus_t			focus;
	vca_t			vca;

	image_t_onvif   image_onvif;
	ptz_t_onvif		ptz_onvif;
	ptz_preset		preset;
	pmask_t			privacymask;
    roi_t			roi_area;

	unsigned int setup_time;

	gpointer funcs[NF_IPCAM_TYPE_MAX];
	gpointer recv_handler;

	NFIPCamPrivacyMask prev_pmask; // 20171024 add to pmask DB changed check

    ai_t ai;
	ai_rule_engine rule_engine;
	ai_dl_option dl_option;
	embedded_osd e_osd;
    
    // add for Fisheye
	NF_IPCAM_CAM_TYPES_E cam_type;
	fisheye_t		     fisheye;
};

typedef struct __MGMT_PROFILE_T_ profile;
struct __MGMT_PROFILE_T_
{
	char name[64];

	int rtsp[MAX_VIDEO_STREAM];
	short unsigned int rtsp_port[MAX_VIDEO_STREAM];
	short unsigned int http_port;

	int vendor;
	int user;
	int pass;

	int model_code;

	unsigned int func;
	int encoder;
	int video;
	int audio;
	int image;
	int ptz;
	int alarm;
	int motion;

	int funcs;
	int recv_handler;
	int get_model_func;
	int focus;
};


typedef struct _IPX_PND_EVENT_T IPX_PND_EVENT;
struct _IPX_PND_EVENT_T
{
	gint type;
	gint port;
	GTimeVal ctime;

	union {
		/* Integer Event Case */
		struct { 
			guint params[4]; 
		} d;
		/* Pointer Event Case */
		struct { 
			guint len;
			gpointer ptr;
			guint reserved[2];
		} p;
	};
};

enum __IPCAM_PND_STATUS_E_
{
	PND_TYPE_UNPLUGGED = 0,
	PND_TYPE_PLUGGED,
	PND_TYPE_MAC_RESOLVED,
	PND_TYPE_IP_REQUESTED,
	PND_TYPE_IP_DONE,
	PND_TYPE_SETUP_REQUESTED,
	PND_TYPE_SETUP_DONE,
	PND_TYPE_VIDEO_START,

	PND_TYPE_TIMEOUT,			// Retry
	PND_TYPE_UNKNOWN,			// Retry
	PND_TYPE_UNSUPPORTED,		// Retry
	PND_TYPE_CONNECTION_FAIL,	// Retry
	PND_TYPE_CONFIG_FAIL,		// Retry
	PND_TYPE_STREAM_FAIL,		// Retry

	PND_TYPE_LOGIN_FAIL,		// No retry
	PND_TYPE_SOMETHING_FAIL,	// No retry

	PND_TYPE_HUB_STATUS,
	PND_TYPE_MAX
};

typedef enum __NF_IPCAM_PND_OSD_STATUS_E_
{
	PND_OSD_NONE = 0,

	PND_OSD_DETECT,
	PND_OSD_DHCP,
	PND_OSD_IPREQUEST,
	PND_OSD_STREAM,

	PND_OSD_MODEL,
	PND_OSD_CONFIG_INIT,
	PND_OSD_CONFIG_ALARM,
	PND_OSD_CONFIG_VIDEO,
	PND_OSD_CONFIG_AUDIO,
	PND_OSD_CONFIG_IMAGE,
	PND_OSD_CONFIG_PMASK,
	PND_OSD_CONFIG_MOTION,
	PND_OSD_CONFIG_VCA,

	PND_OSD_CONFIG_REBOOT,

	PND_OSD_TIMEOUT,
	PND_OSD_CONFIG_FAIL,
	PND_OSD_STREAM_FAIL,
        PND_OSD_CONFIG_ROI,

	PND_OSD_MAX
} NF_IPCAM_PND_OSD_E;

typedef struct _ARP_STACK_UNIT ARP_STACK_UNIT;
struct _ARP_STACK_UNIT
{
	unsigned char  dest_mac[6];
	unsigned char  src_mac[6];
	unsigned short eth_type;

	unsigned short hw_type;
	unsigned short proto_type;
	unsigned char  hw_size;
	unsigned char  proto_size;
	unsigned short op;
	unsigned char  sender_mac[6];
	unsigned int   sender_ip;
	unsigned char  target_mac[6];
	unsigned int   target_ip;
}__attribute__((__packed__));

enum __IPCAM_DISCOVERY_STATE_
{
	IPCAM_DISC_STATE_NONE = 0,	/* Unlinked */
	IPCAM_DISC_STATE_LINK,		/* Link detected */

	IPCAM_DISC_STATE_IPSEARCH,	/* ITX cam search */
	IPCAM_DISC_STATE_IPSET,		/* ITX cam ip requested */
	IPCAM_DISC_STATE_IPSETW,	/* ITX cam ip ack waiting */

	IPCAM_DISC_STATE_VNET,		/* Virtual network created */
	IPCAM_DISC_STATE_IPDONE,	/* ONVIF cam IP assigned */
	IPCAM_DISC_STATE_CAPA,		/* ONVIF get capabilities */

	IPCAM_DISC_STATE_DONE		/* ITX discovery done */
};

enum __IPCAM_DISCOVERY_LAYER_
{
	IPCAM_DISC_LAYER_NONE = 0,
	IPCAM_DISC_LAYER_DVR,
	IPCAM_DISC_LAYER_VHUB,
	IPCAM_DISC_LAYER_OPEN,
};

enum __IPCAM_SN_VENDOR_
{
	IPCAM_SN_VENDOR_NONE = 0,
	IPCAM_SN_VENDOR_ITX,
	IPCAM_SN_VENDOR_CBC,
	IPCAM_SN_VENDOR_KBD,
	IPCAM_SN_VENDOR_ERV,
	IPCAM_SN_VENDOR_Y3K = 5,
	IPCAM_SN_VENDOR_PSP,
	IPCAM_SN_VENDOR_VCN,
	IPCAM_SN_VENDOR_GPS,
	IPCAM_SN_VENDOR_ITY,
	IPCAM_SN_VENDOR_SPS = 10,
	IPCAM_SN_VENDOR_LTV,
	IPCAM_SN_VENDOR_KMW,
	IPCAM_SN_VENDOR_SCS,
	IPCAM_SN_VENDOR_TIS,
	IPCAM_SN_VENDOR_NVS = 15,
	IPCAM_SN_VENDOR_DSS,
	IPCAM_SN_VENDOR_S10,
	IPCAM_SN_VENDOR_GRZ,
	IPCAM_SN_VENDOR_TEL,
	IPCAM_SN_VENDOR_VDC = 20,
	IPCAM_SN_VENDOR_ORI,
	IPCAM_SN_VENDOR_G4S,
	IPCAM_SN_VENDOR_TKK,
	IPCAM_SN_VENDOR_IME,
	IPCAM_SN_VENDOR_AMB = 25,
	IPCAM_SN_VENDOR_ENE,

};

static char* __DISCOVERY_STATE_STR[] = {
	"NONE  ",	//IPCAM_DISC_STATE_NONE
	"LINK  ",	//IPCAM_DISC_STATE_LINK
	"SEARCH",	//IPCAM_DISC_STATE_IPSEARCH
	"IPSET ",	//IPCAM_DISC_STATE_IPSET
	"IPACK ",	//IPCAM_DISC_STATE_IPSETW	
	"VNET  ",	//IPCAM_DISC_STATE_VNET
	"IPDONE",	
	"CAPA  ",
	"DONE  "
};
typedef struct __MGMT_DISCOVERY_TABLE_ dtable;
struct __MGMT_DISCOVERY_TABLE_
{
	int layer;
	unsigned int state;
	int state_cnt;
	int vnet_id;
	int retry_cnt;
	unsigned int transaction;
	unsigned int ipaddr;
	unsigned char macaddr[8];
	int polling_delay;
	int loss_stat_cnt;
	int login_retry;
};


struct sn_info {
	char vendor[4];	// Vendor
	char nation[4];	// Nation
	char date[16];	// Date + Index
	char mega[8];	// Mega Pixel
	char lens[4];	// Lens
	char heater[4];	// Header
	char irled[4];	// IR LED
	char sensor[4];	// Sensor
	char ads[4]; 	// Adjust DNN's Sensitivity
	char na[4]; 	// N/A
};

/* theweak: Axis Camera discovery result */
enum _AXIS_DICOVERY_RESULT {
	AD_CONNECTION_FAIL,
	AD_NOT_MATCH_ID_PASS,
	AD_ENABLED_WEBSERVICE,
	AD_DISABLE_WEBSERVICE
};

/* sjlim87: DC IRIS CALIBRATION RETURN VALUE */
enum _DC_IRIS_CAL_STATUS {
	RTN_ERROR = -1,
	RTN_NORMAL,
	RTN_RUNNING,
	RTN_END,
	RTN_TIMEOUT
};

typedef enum __NF_IPCAM_PND_RTSP_ERR_E_ {
	NF_RTSP_ERR_NO_ERR = 0,

	NF_RTSP_ERR_OPEN_INVALID_CH = 1,
	NF_RTSP_ERR_OPEN_INVALID_STREAM,
	NF_RTSP_ERR_OPEN_NULL_LOCATION,
	NF_RTSP_ERR_OPEN_INVALID_PORT_NUM,
	NF_RTSP_ERR_OPEN_INVALID_RTP_METHOD,
	NF_RTSP_ERR_OPEN_CH_DUP = 6,
	NF_RTSP_ERR_OPEN_INVALID_LOCATION,
	NF_RTSP_ERR_OPEN_CONN_FAIL,
	NF_RTSP_ERR_OPEN_OPTION_FAIL,
	NF_RTSP_ERR_OPEN_NO_AUDIO,
	NF_RTSP_ERR_OPEN_NO_AUDIO_PT = 11,
	NF_RTSP_ERR_OPEN_NO_AUDIO_CTRL,
	NF_RTSP_ERR_OPEN_NO_VIDEO,
	NF_RTSP_ERR_OPEN_NO_VIDEO_PT,
	NF_RTSP_ERR_OPEN_NO_VIDEO_CTRL,
	NF_RTSP_ERR_OPEN_NO_VIDEO_RTPMAP = 16,
	NF_RTSP_ERR_OPEN_NO_VIDEO_FMTP,
	NF_RTSP_ERR_OPEN_ENC_H264,
	NF_RTSP_ERR_OPEN_NO_VIDEO_SPSPPS,
	NF_RTSP_ERR_OPEN_DESCRIBE_FAIL,
	NF_RTSP_ERR_OPEN_SESSION_FAIL = 21,
	NF_RTSP_ERR_OPEN_RES_TEMP_UNAVAIL,

	NF_RTSP_ERR_PLAY_REQ_FAIL,

	NF_RTSP_ERR_RTP_UNDEFINED,
	NF_RTSP_ERR_RTP_UNHANDLED,
	NF_RTSP_ERR_RTP_MEM_FAIL = 26,

	NF_RTSP_ERR_CLOSE_INTERNAL,
	NF_RTSP_ERR_CLOSE_INVALID_CH,
	NF_RTSP_ERR_CLOSE_INVALID_STREAM,
	NF_RTSP_ERR_CLOSE_IDLE_STREAM,
	NF_RTSP_ERR_CLOSE_FAIL = 31,

	NF_RTSP_ERR_INITIAL
} NF_IPCAM_PND_RTSP_ERR_E;

static const char* RTSP_ERR_STR[] = {
	"Success",

	"Open Invalid channel number",
	"Open Invalid stream number",
	"Open NULL RTSP address",
	"Open Invalid RTSP port",
	"Open Invalid RTP Method",
	"Open Channel number duplicated",
	"Open Invalid RTSP address",
	"Open TCP connection fail",
	"Open RTSP OPTION fail",
	"Open No audio stream",
	"Open Invalid audio payload type",
	"Open No audio control",
	"Open No video",
	"Open Invalid video payload type",
	"Open No audio control",
	"Open Invalid video RTP map",
	"Open Invalid video FMTP",
	"Open Video other than H264",
	"Open Invalid SPS/PPS",
	"Open RTSP DESCRIBE fail",
	"Open RTSP SESSION fail",
	"Open Resource temporarily unavailable",

	"Play RTSP PLAY fail",

	"RTP Undefined RTP header type",
	"RTP Unsupported RTP header type",
	"RTP Memory allocation fail",

	"Close from internal request",
	"Close Invalid channel number",
	"Close Invalid stream number",
	"Close requested to an idle stream",
	"Close request fail",

	"On going"
};

typedef struct _ipcam_onvif_auth_info_t {
	int auth_method;
	char* username;
	char* password;
	char* endpoint;
} ipcam_onvif_auth_info_t;


typedef struct axis_Discovery_t {
	int isAxisCamera;
	char modelName[10];
	int discoveryResult;
} axis_Discovery;

struct arp_table {
	uint32_t ip;
	uint8_t mac[6];
	uint8_t dev[8];
};

axis_Discovery IsAxisCamera(char *pIpAddr);



static const char* IPCAM_UPGRADE_STATUS_STR[] = {
	"UNKNOWN",
	"MODE_REQUIRED",
	"READY_TO_UPGRADE",
	"UPLOADING",
	"VERIFYING",
	"WRITING",
	"DONE"
};
static const char* IPCAM_UPGRADE_ERR_STR[] = {
	"NO_ERR_STATUS 0",
	"NETWORK_ERROR",
	"INTERNAL_ERROR",
	"NO_ERR_STATUS 1",
	"NO_ERR_STATUS 2",
	"UPLOADING FAILED",
	"VERIFICATON FAILED",
	"WRITING FAILED",
	"NO_ERR_STATUS 3"
};
enum _IPCAM_UPGRADE_STATUS_E__ {
	IPCAM_FWUP_STATUS_MIN = 0,
	IPCAM_FWUP_STATUS_MODE_REQUIRED,
	IPCAM_FWUP_STATUS_READY,
	IPCAM_FWUP_STATUS_UPLOADING,
	IPCAM_FWUP_STATUS_VERIFYING,
	IPCAM_FWUP_STATUS_WRITING = 5,
	IPCAM_FWUP_STATUS_DONE,
	IPCAM_FWUP_STATUS_MAX,

	IPCAM_FWUP_STATUS_ERR_MIN = 98,
	IPCAM_FWUP_STATUS_ERR_NETWORK = 99,
	IPCAM_FWUP_STATUS_ERR_INTERNAL = 100,
	IPCAM_FWUP_STATUS_ERR_UPLOAD = 103,
	IPCAM_FWUP_STATUS_ERR_VERIFY = 104,
	IPCAM_FWUP_STATUS_ERR_WRITE = 105,
	IPCAM_FWUP_STATUS_ERR_MAX
};
typedef struct _ipcam_upgrade_context_t_ ipcam_upgrade_ctx;
struct _ipcam_upgrade_context_t_
{
	/* upgrade parameter */
	int ch;
	int fw_len;
	char* fw_data_stream;
	char* fw_name;

	/* upgrade progress */
	int status;
	int timeout;
	int time_waiting;

	/* upgrade canceled */
	int cancel_required;
};


extern void set_running_state(unsigned int val);
extern void ipaddr_manager(int channel_num);
extern int discovered_device_handler(int port);

extern void switch_mtx_lock(void);
extern void switch_mtx_unlock(void);
extern unsigned int get_sub_network(void);
extern int set_sub_network(void);
extern int get_interrupt_port(unsigned char* mac);
extern int get_interrupt_port_set_camport(unsigned char* mac);
extern int peak_port_macaddr_table(int port, unsigned char* mac);
extern unsigned int get_available_ip(int port);

extern mtable* get_runtime(void);
extern GAsyncQueue* get_queue(void);
extern int* get_portscan_trigger(void);
extern unsigned int get_vloss_status(void);
extern int get_pnd_status(int ch_num);
extern IPX_PND_EVENT* ipx_ipcam_new_event(int event_type);
extern int plug_and_detect_restart(void);

extern void checking_setting_network_for_discovery(void);
extern unsigned int get_init_info(void);
extern unsigned int get_local_net_ip(void);
extern unsigned int get_host_info(void);
extern unsigned int get_host_gateway(void);
extern unsigned int get_host_netmask(void);
extern unsigned int get_netif_ip(const char*);
extern unsigned int get_netif_mask(const char*);

extern profile* get_model_db(void);
extern int get_model_db_cnt(void);

extern void vhub_manager_init(void);

extern void nf_ipcam_get_ipstr(int id, char* dst_buf);
extern void nf_ipcam_get_username(int id, char* dst_buf);
extern void nf_ipcam_get_password(int id, char* dst_buf);
extern unsigned short nf_ipcam_get_http_port(int ch);
extern unsigned short nf_ipcam_get_rtsp_port(int ch);
extern unsigned int nf_ipcam_get_ipaddr(int ch);
extern unsigned int nf_ipcam_is_ssl(int id);
extern int build_management_table(int);
extern int build_management_table_onvif(int);

extern int nf_ipcam_setup_request(int ch, NF_IPCAM_SETUP_TYPE_E type, NFIPCamSetupCallback cb_fxn, gpointer user_data, void* info);
extern void nf_ipcam_setup_waiting(int ch, NF_IPCAM_SETUP_TYPE_E type, int sock);
extern void cam_setup_restart(void);

extern NFIPCamOption* get_cam_model_option(uint64_t func, unsigned int value);
extern NFIPCamOption_onvif* get_cam_model_option_image(uint64_t func, unsigned int value);
extern NFIPCamOption_onvif* get_cam_model_option_exposure(uint64_t func, unsigned int value);

extern void cam_onvif_init(void);
extern void arp_pile_analysis(int*, int*);
extern void try_onvif_discovery(void);
extern int  arp_pile_try_lock(void);
extern void arp_pile_unlock(void);
extern struct arp_table* get_arp_pile(void);

/* ONVIF services related */
extern int nf_onvif_get_service_capabilities(int);
extern int nf_onvif_get_appropriate_profile(int);
extern void nf_onvif_soap_init_set(struct soap*);
extern time_t _nf_onvif_get_time(const char* );
extern int _nf_onvif_add_auth(struct soap *_soap, int auth, const char* user, const char* pass, const char* endpoint);
//extern void nf_onvif_init(void);

extern void _release_resource(int*, char*, SSL**, SSL_CTX**);
extern void nf_pnd_evt_notify_fire(int ch, int type, int l, char* f);
extern void nf_pnd_prog_notify_fire(int ch, int prog, int l, char* f);
extern void nf_pnd_queue_push(int ch, int type, int l, char* f);
extern void nf_pnd_queue_delay(int ch, int next, int delay, int l, char *f);

extern int cam_sn_get(struct sn_info *info, int cam_id, int v);
extern gboolean nf_eventlog_put_ipcam_msg(const char *msg, int ch);
#endif //__NF_IPCAM_DEFS_H__



