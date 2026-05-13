#ifndef _NF_DAL_H_
#define _NF_DAL_H_

#include "config.h"
#include "ivca_def.h"
#include "itx_ai_def.h"


#if defined(_ATM_1648) || defined(_ATM_1624)    ||  \
    defined(_ATM_0824) || defined(_ANF_0824CS) || defined(_OTM_1648) || defined(_OTM_0824)
#define DB_ALARM_IN_4CH_SUPPORT
#endif
#define _UPNP_SUPPORT_

#define MAX_GUI_CHANNEL 32


#if 1
#if defined(GUI_4CH_SUPPORT)
enum {
	SCR_DIV_TYPE1 = 0,
	SCR_DIV_TYPE4,
	NUM_SCR_DIV_TYPES
};
#elif defined(GUI_8CH_SUPPORT)
enum {
	SCR_DIV_TYPE1 = 0,
	SCR_DIV_TYPE4,
	SCR_DIV_TYPE9,
#ifndef _NOT_SUPPORT_SPC_DIV
	SCR_DIV_TYPE8,
	SCR_DIV_TYPE6,
#endif
	NUM_SCR_DIV_TYPES
};
#elif defined(GUI_16CH_SUPPORT)
enum {
	SCR_DIV_TYPE1 = 0,
	SCR_DIV_TYPE4,
	SCR_DIV_TYPE9,
	SCR_DIV_TYPE16,
#ifndef _NOT_SUPPORT_SPC_DIV
	SCR_DIV_TYPE8,
	SCR_DIV_TYPE6,
#endif
	NUM_SCR_DIV_TYPES
};
#elif defined(GUI_32CH_SUPPORT)
enum {
	SCR_DIV_TYPE1 = 0,
	SCR_DIV_TYPE4,
	SCR_DIV_TYPE9,
	SCR_DIV_TYPE16,
	SCR_DIV_TYPE36,
#ifndef _NOT_SUPPORT_SPC_DIV
	SCR_DIV_TYPE8,
	SCR_DIV_TYPE6,
#endif
	NUM_SCR_DIV_TYPES
};
#else
enum {
	SCR_DIV_TYPE1 = 0,
	SCR_DIV_TYPE4,
	SCR_DIV_TYPE9,
	SCR_DIV_TYPE16,
#ifndef _NOT_SUPPORT_SPC_DIV
	SCR_DIV_TYPE8,
	SCR_DIV_TYPE6,
#endif	
	NUM_SCR_DIV_TYPES
};
#endif
#endif


typedef enum _SEQ_VALID_MODE_E {
	SEQ_MODE_DELETED = 0,
	SEQ_MODE_INVALID,
	SEQ_MODE_VALID,
} SEQ_VALID_MODE_E;


typedef enum _SPOT_VALID_MODE_E {
	SPOT_MODE_DELETED = 0,
	SPOT_MODE_INVALID,
	SPOT_MODE_VALID,
} SPOT_VALID_MODE_E;


#if 0
#define	NFGUI_PROTO_ITXA	1
#define NFGUI_PROTO_ITXB	2

#define NFGUI_BAUD_1200		1
#define NFGUI_BAUD_2400		2
#define NFGUI_BAUD_4800		3
#define NFGUI_BAUD_9600		4
#define NFGUI_BAUD_19200	5
#define NFGUI_BAUD_38400	6
#define NFGUI_BAUD_115200	7

#define	DISPLAY_DIV_TYPE1	0
#define	DISPLAY_DIV_TYPE4	1
#define	DISPLAY_DIV_TYPE9	2
#define	DISPLAY_DIV_TYPE16	3
#define	DISPLAY_DIV_TYPE8	4
#define	DISPLAY_DIV_TYPE6	5

#define	SEQ_VALID_MODE_INVALID	0
#define	SEQ_VALID_MODE_VALID	1
#define	SEQ_VALID_MODE_ACTIVE	2
#endif


typedef struct _StdRect_t StdRect;
struct _StdRect_t {
	gint x;
	gint y;
	gint w;
	gint h;
};


typedef struct _StdPoint_t StdPoint;
struct _StdPoint_t {
	gint x;
	gint y;
};

/******************************************* *
// Setup --> SYSTEM --> DATE/TIME --> HOLIDAY 
* *******************************************/

#define USR_DEF_HOLIDAY_CNT 50
#define HOLIDAY_lEN 9 

typedef struct _UsrDefHoliday_t UsrDefHolidayData;
struct _UsrDefHoliday_t{ 
	int year;
	int month;
	int day;
	int yoil;
	int week;
	char type;
}; 

guint DAL_default_UsrDefHoliday_Data(guint holidayIdx);
guint DAL_get_holiday_count();
guint DAL_set_holiday_count(guint cnt);
guint DAL_get_UsrDefHoliday_Data(UsrDefHolidayData *data, guint holidayIdx);
guint DAL_set_UsrDefHoliday_Data(UsrDefHolidayData *data, guint holidayIdx);
/************************************
 * Setup --> System --> Camera
 * *********************************/
typedef struct _SysCamIPCam_t	IPCamData;
struct _SysCamIPCam_t {
	gchar title[STRING_SIZE_64];	///< ip camera title
	gchar mac[STRING_SIZE_32];		///< camera¿IP
	gchar model[STRING_SIZE_32];	///< camera model name
	gchar method[STRING_SIZE_16];		///< resolution
	gchar rtsp_url[STRING_SIZE_128];		///< resolution
	guint rtsp_port;					///< resolution
	gchar http_url[STRING_SIZE_128];		///< resolution
	guint http_port;					///< resolution
	gchar codec[STRING_SIZE_32];	///<
	gchar size[STRING_SIZE_32];		///< resolution
	gboolean enMotion;					///< enable Ptz
	gboolean enAlarm;				///< enable Audio
	gboolean useDVRPassword;		///< 0: dvr password »ç¿ë		1: camera password
	gchar id[STRING_SIZE_32];		///< ID
	gchar passwd[STRING_SIZE_32];	///< Password
	gboolean covert;
	gboolean audio;
};


guint DAL_get_ipCam_data(IPCamData *data, guint camIdx);
guint DAL_set_ipCam_data(IPCamData *data, guint camIdx);
guint DAL_save_ipcam_data();
guint DAL_get_ipcam_count();
gboolean DAL_get_audio_all_ch(guint data[33]);


/***********************************************************/

typedef struct _SysCamCamera_t	CameraData;
struct _SysCamCamera_t {
	gboolean show;
	gchar title[STRING_SIZE_CAMTITLE];
#define CAMERA_COVERT_OFF	0
#define CAMERA_COVERT_ON	1
	guint covert;
	guint audio_ch;
	gboolean admin;
	gboolean manager;
	gboolean user;
	gboolean logoff;
	guint type;
};

gboolean DAL_get_cam_show();
guint DAL_set_cam_show(gboolean show);

guint DAL_get_camera_data(CameraData *data, guint channel);
guint DAL_set_camera_data(CameraData data, guint channel);
guint DAL_set_camera_data_all(CameraData *data, guint ch_num);

guint DAL_get_camtitle_data(CameraData *data, guint channel);
guint DAL_set_camtitle_data(CameraData data, guint channel);
guint DAL_set_camtitle_data_all(CameraData *data, guint ch_num);

guint DAL_get_covert_data(CameraData *data, guint channel);
guint DAL_set_covert_data(CameraData data, guint channel);
guint DAL_set_covert_data_all(CameraData *data, guint ch_num);

gboolean DAL_get_camera_title(gchar *title, guint channel);
gboolean DAL_get_audio_ch();

gboolean DAL_is_covert(guint channel);
gboolean DAL_get_logoff_covert(gchar covert[33]);

/*************************************************************/

typedef struct _SysCamColor_t	ColorData;
struct _SysCamColor_t {
	guint	bright;
	guint	contrast;
	guint	tint;
	guint	color;
};

guint DAL_get_color_data(ColorData *data, guint channel);
guint DAL_set_color_data(ColorData data, guint channel);
guint DAL_set_color_data_all(ColorData *data, guint ch_num);


/*************************************************************/

typedef struct _SysCamAdvanced_t	AdvancedData;
struct _SysCamAdvanced_t {
	guint		sharpness;
	guint		exposure_mode;
	guint		agc_gain;
	guint		shutter_speed;
	guint		slow_shutter;
	guint		max_agc;
	guint		iris_control;
	guint		blc_control;
	guint		day_night_mode;
	guint		day_night_duration;
	guint		wb_mode;
	guint		mwb_mode;
	guint		rotate;
	guint		antiflicker;
	guint       wdr_mode;
	gint        wdr_level;
	guint       dnr;
	guint       adaptive_ir;
};


guint DAL_get_advanced_data(AdvancedData *data, guint channel);
guint DAL_set_advanced_data(AdvancedData data, guint channel);
guint DAL_set_advanced_data_all(AdvancedData *data, guint ch_num);

gboolean DAL_get_cam_reset_msg_check();
guint DAL_set_cam_reset_msg_check(gboolean check_state);


/*************************************************************/

typedef struct _SysIPCamSetup_t     IPCamSetupData;
struct _SysIPCamSetup_t {
	guint	    bright;
	guint	    contrast;
	guint	    tint;
	guint	    color;
	guint		sharpness;
	guint		rotate;
	guint		focus_mode;
    gint        focus_default_speed;
    gint        focus_near_limit;
    gint        focus_far_limit;
    gint        focus_abposition;
    gint        focus_abspeed;
    gint        focus_redistance;
    gint        focus_respeed;
    gint        focus_cospeed;
    guint       focus_limit;
    guint       ir_correction;
    guint       stabilizer;
	guint		wb_mode;
    gint        wb_crgain;
    gint        wb_cbgain;
	guint		mwb_mode;
	guint		wdr_mode;
    gint        wdr_level;
	guint		day_night_mode;
	guint		day_night_duration;
	guint		exposure_mode;
	guint		exposure_priority;
	guint		blc_control;
    gint        blc_level;
    gint        etime;
    gint        min_etime;
    gint        max_etime;
	guint		slow_shutter;
	guint		shutter_speed;
    gint        gain;
    gint        min_gain;
    gint        max_gain;
	guint		agc_gain;
	guint		max_agc;
    gint        iris;
    gint        min_iris;
    gint        max_iris;
	guint		iris_control;
	guint		antiflicker;
	guint		max_shutter;
	guint       base_shutter;
	guint       dnr;
	guint       adaptive_ir;
	guint	    dnn_sense_ntod;
	guint 	    dnn_sense_dton;
	guint	    defog;
	guint	    hlc;
	guint	    dnn_start_hour;
	guint	    dnn_start_min;
	guint	    dnn_end_hour;
	guint	    dnn_end_min;
	guint	    illumination_control;
	guint	    illumination_level;
};


guint DAL_get_IPCamSetup_data(IPCamSetupData *data, guint channel);
guint DAL_set_IPCamSetup_data(IPCamSetupData data, guint channel);
guint DAL_set_IPCamSetup_data_all(IPCamSetupData *data, guint ch_num);


/***********************************************************/

#define MAX_PTZ_PROTO_STR_LENGTH 	(16)
enum {
	BAUD_NONE = -1,

	BAUD_2400 = 0,
	BAUD_4800,
	BAUD_9600,
	BAUD_19200,
	BAUD_38400,
	BAUD_57600,
	BAUD_115200,

	NUM_BAUD_RATES,
};

typedef enum _PTZ_MODE_E {
	PTZ_MODE_OFF = 0,
	PTZ_MODE_SCAN,
	PTZ_MODE_TOUR,
	NUM_PTZ_MODE,
} PTZ_MODE_E;

typedef struct _SysCamPtz_t	PtzData;
struct _SysCamPtz_t {
	guint	addr;
//	gchar	proto[STRING_SIZE_32];
//	gchar	baud[STRING_SIZE_16];
	guint	proto;
	guint	baud;
	guint	channel;
	gchar	driver[STRING_SIZE_32];
	guint	autoFocus;
	guint	autoIris;
	guint	PTSpeed;
	guint	zoomSpeed;
	guint	focusSpeed;
	guint	irisSpeed;
	guint	mode;           // PTZ_MODE_E
//	gint	tour_mode;
//	gint	tour_preset[32];
//	gint	tour_dwell[32];
	gboolean rs485;
};

guint DAL_get_ptz_data(PtzData *data, guint channel);
guint DAL_set_ptz_data(PtzData data, guint channel);
guint DAL_set_ptz_data_all(PtzData *data, guint ch_num);
guint DAL_get_ptz_protocol_name(gchar *name, guint proto_num);
gboolean DAL_get_ptz_rs485_supported(guint channel);

/**********************************************************/

#define	MAX_PRESET_COUNT	(16)

typedef struct _SysCamPtzPreset_t	PtzPresetData;
struct _SysCamPtzPreset_t {
	guint       cnt;
	guint       number[MAX_PRESET_COUNT];
	gchar       name[MAX_PRESET_COUNT][STRING_SIZE_PRESETNAME];
};

guint DAL_get_ptz_preset_data(PtzPresetData *data, guint channel);
guint DAL_set_ptz_preset_data(PtzPresetData data, guint channel);
guint DAL_set_ptz_preset_data_all(PtzPresetData *data, guint ch_num);

/**********************************************************/

typedef struct _SysCamPtzScan_t	PtzScanData;
struct _SysCamPtzScan_t {
	guint       number[2];
	guint       dwell[2];
};

guint DAL_get_ptz_scan_data(PtzScanData *data, guint channel);
guint DAL_set_ptz_scan_data(PtzScanData data, guint channel);
guint DAL_set_ptz_scan_data_all(PtzScanData *data, guint ch_num);


/**********************************************************/

#define	MAX_TOUR_COUNT	    (16)

typedef struct _SysCamPtzTour_t	PtzTourData;
struct _SysCamPtzTour_t {
	guint       cnt;
	guint       number[MAX_TOUR_COUNT];
	guint       dwell[MAX_TOUR_COUNT];
};

guint DAL_get_ptz_tour_data(PtzTourData *data, guint channel);
guint DAL_set_ptz_tour_data(PtzTourData data, guint channel);
guint DAL_set_ptz_tour_data_all(PtzTourData *data, guint ch_num);

/**********************************************************/

#define MAX_RECT_COUNT		(10)

typedef struct _RectArea	RectArea;
struct _RectArea {
	guint start_r;
	guint start_c;
	guint end_r;
	guint end_c;
};

typedef struct _SysCamMotion_t	MotionData;
struct _SysCamMotion_t {
	gboolean	act;
	guint		detect;
	guint		sense_d;
	guint		sense_n;
	guint		mini_d;
	guint		mini_n;
 	guint		sense;
	guchar		area[1401];
	guint		time_start;
	guint		time_end;

	guint		rect_cnt;
	RectArea 	rect[MAX_RECT_COUNT];
};

typedef struct _SysCamSmartMotion_t SysCamSmartMotion_t;
struct _SysCamSmartMotion_t
{
    char name[50];
    int enable;
    int threshold;
};

guint DAL_get_motionsensor_data(MotionData *data, guint channel);
gboolean DAL_get_motionsensor_detect();
guint DAL_set_motionsensor_data(MotionData data, guint channel);

guint DAL_get_motion_data(MotionData *data, guint channel);
guint DAL_set_motion_data(MotionData data, guint channel);
guint DAL_set_motion_data_all(MotionData *data, guint ch_num);

/**********************************************************/
enum {
	AREA_TYPE_NONE = 0,
	AERA_TYPE_RECT,
	AERA_TYPE_POLY,		// unsupported
	AREA_TYPE_CELL,
	AREA_TYPE_RAW		// not used
};

typedef struct _SysOnvifMotion_t		OnvifMotionData;
typedef struct _SysOnvifMotionCell_t	OnvifMotionCell;
typedef struct _SysOnvifMotionRect_t	OnvifMotionRect;
typedef struct _SysOnvifMotionIPCam_t	OnvifMotionIPCam;

struct _SysOnvifMotionCell_t {
	/*
	guint		sense;
	guint		interval;
	guint		mini_block;
	*/
	guint		sense_d;
	guint		sense_n;
	guint		mini_d;
	guint		mini_n;
	guint		time_start;
	guint		time_end;

	guint		interval;
};

struct _SysOnvifMotionRect_t {
	RectArea 	rect;

	gchar 		name[STRING_SIZE_64];
	guint		sense;
	guint		interval;
	guint		threshold;
	gboolean	exclude;
};

struct _SysOnvifMotionIPCam_t {
	guint		method;

	guint		row;
	guint 		col;
	guint		sense_min;
	guint 		sense_max;

	guint 		smart_motion_option_size;
	gchar 		smart_interest_obj[256];
	SysCamSmartMotion_t smart_motion_options[15];
};

struct _SysOnvifMotion_t {
	gint		rect_cnt;
	guchar		area[1400];
#if 1
	RectArea 	rect[MAX_RECT_COUNT];

	OnvifMotionIPCam	ipcam_d;
	OnvifMotionCell		cell_d;
#else	// using later
	union {
		OnvifMotionCell		cell_d;
		OnvifMotionRect		rect_d[MAX_RECT_COUNT];
	};
#endif
	gint		smart_motion;
	gint		use_ai_alarmevt;
};

guint DAL_get_onvif_motion_data(OnvifMotionData *data, guint channel, gint area_type);
guint DAL_set_onvif_motion_data(OnvifMotionData data, guint channel);
guint DAL_set_onvif_motion_data_all(OnvifMotionData *data, guint ch_mask);
guint DAL_get_onvif_ipcam_data(OnvifMotionIPCam *data, guint channel);
guint DAL_set_onvif_ipcam_data(OnvifMotionIPCam data, guint channel);

/**********************************************************/
typedef struct _IPCam_Login_t		IPCamLoginData;
struct _IPCam_Login_t	{
	gchar id[STRING_SIZE_32];
	gchar password[STRING_SIZE_32];
	guchar mac[STRING_SIZE_32];
	guchar hostname[STRING_SIZE_64];
//	guint ipaddr;
//	guint subnet;
//	guint gateway;
//	guint dns1;
//	guint dns2;
	guint http_port;
	guint rtsp_port;

    guint virtual_camera;
    guint vcam_cnt;
    gchar vcam_rtsp_addr[2][256];
    gchar model[64];
	guint ethernet;
};

gint DAL_get_ipcam_login_data(IPCamLoginData *data, gint ch);
gint DAL_set_ipcam_login_data(IPCamLoginData data, gint ch);


/**********************************************************/
typedef struct _IPCam_Inst_Option_t		IPCamInstOptData;
struct _IPCam_Inst_Option_t	{
    gboolean auto_scan;
};

gint DAL_get_ipcam_install_opt_data(IPCamInstOptData *data);
gint DAL_set_ipcam_install_opt_data(IPCamInstOptData *data);

/**********************************************************/

#define MAX_PRIVACY_CNT (10)

typedef struct _PrivacyArea	PrivacyArea;
struct _PrivacyArea {
    gint level;
	gint sx;
	gint sy;
	gint ex;
	gint ey;
};

typedef struct _SysCamPrivacy_t	PrivacyData;
struct _SysCamPrivacy_t {
	gboolean	    act;
	guint		    color;
	guint		    area_cnt;
	PrivacyArea 	area[MAX_PRIVACY_CNT];
};

gint DAL_get_privacy_max_cnt();
guint DAL_get_privacy_data(PrivacyData *data, guint channel);
guint DAL_set_privacy_data(PrivacyData data, guint channel);
guint DAL_set_privacy_data_all(PrivacyData *data, guint ch_num);

/**********************************************************/

#define MAX_ROI_AREA_CNT (10)

typedef struct _ROIArea	ROIArea;
struct _ROIArea {
    gint level;
	gint tx;
	gint ty;
	gint bx;
	gint by;
};

typedef struct _SysCamROI_t	ROIData;
struct _SysCamROI_t {
	guint	        mode;
	gint            quality;
	ROIArea 	    area[MAX_ROI_AREA_CNT];
};

gint DAL_get_roi_area_max_cnt();
guint DAL_get_roi_data(ROIData *data, guint channel);
guint DAL_set_roi_data(ROIData data, guint channel);
guint DAL_set_roi_data_all(ROIData *data, guint ch_num);

/**********************************************************/

typedef struct _VCAPropData_t	VCAPropData;
struct _VCAPropData_t {
	guint 		unit;
	/* Algorithm options. */
	gboolean    en_shadowrm;
    guint       track_ref;
    gboolean    en_usecalib;
    guint       min_width3d;
	guint       min_height3d;
	gboolean	active;
	gboolean	detect;
	gboolean	en_roi;
	gboolean	en_tamper;
	gboolean	en_snapshot;
	StdRect 	roi_rect;
	gboolean 	sw_obj_bb;
	gboolean 	sw_obj_id;
	gboolean 	sw_obj_ar;
	gboolean 	sw_obj_tm;
	gboolean 	sw_obj_tr;
	gboolean 	sw_rule;
	gboolean 	sw_rule_name;
	gboolean 	sw_roi;
	gboolean 	sw_obj_w3d;
	gboolean 	sw_obj_h3d;
	gboolean 	sw_obj_s3d;
};

typedef struct _VCAZone_t	VCAZone;
struct _VCAZone_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		enabled;
	gint 		stop_time;
	gint 		abandon_time;
	gint 		remove_time;
	gint 		loiter_time;
	gint 		fall_time;
	guint 		ecolor;
	gchar 		ecolor_sens;
	guint 		size_min[2];
	guint 		size_max[2];
	guint 		speed_min;
	guint 		speed_max;
	guint 		eclass;
	guint 		color;
	gint 		npts;
	StdPoint 	pt[16];
	guint 		sensitivity;
};

typedef struct _VCAZoneData_t	VCAZoneData;
struct _VCAZoneData_t {
	guint 		cnt;
	VCAZone 	zone[16];
};

typedef struct _VCACntr_t	VCACntr;
struct _VCACntr_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		enabled;
	gint 		zid_up;
	gint 		zid_dn;
	gint 		evalue;
	guint 		resetalert;
	guint 		color;
	StdPoint 	pt[4];
};

typedef struct _VCACntrData_t	VCACntrData;
struct _VCACntrData_t {
	guint 		cnt;
	VCACntr 	cntr[16];
};

typedef struct _VCACalb_t	VCACalb;
struct _VCACalb_t {
	StdPoint    pt[2];
	gint        height;
};

typedef struct _VCACalbData_t	VCACalbData;
struct _VCACalbData_t {
	guint       cnt;
	VCACalb 	calb[32];
};

typedef struct _VCACalbResData_t	VCACalbResData;
struct _VCACalbResData_t {
	gfloat      focal;
	gfloat      height;
	gfloat      tilt;
	guint       p_width;
	guint       p_height;
	guint       paramvalid;
};

typedef struct _VCARule_t	VCARule;
struct _VCARule_t {
	VCAZoneData     zonelist;
	VCACntrData     cntrlist;
};

typedef struct _VCAData_t	VCAData;
struct _VCAData_t {
	VCAPropData     prop;
	VCAZoneData     zonelist;
	VCACntrData     cntrlist;

	VCACalbData     calblist;
	VCACalbResData  calbres;
};

guint DAL_get_vca_data(VCAData *data, guint channel);
guint DAL_get_vca_prop_data(VCAPropData *data, guint channel);
guint DAL_get_vca_zone_data(VCAZoneData *data, guint channel);
guint DAL_get_vca_cntr_data(VCACntrData *data, guint channel);
guint DAL_get_vca_calb_data(VCACalbData *data, guint channel);
guint DAL_get_vca_calbres_data(VCACalbResData *data, guint channel);

guint DAL_set_vca_data(VCAData data, guint channel);
guint DAL_set_vca_prop_data(VCAPropData data, guint channel);
guint DAL_set_vca_zone_data(VCAZoneData data, guint channel);
guint DAL_set_vca_cntr_data(VCACntrData data, guint channel);
guint DAL_set_vca_calb_data(VCACalbData data, guint channel);
guint DAL_set_vca_calbres_data(VCACalbResData data, guint channel);

guint DAL_set_vca_data_all(VCAData *data, guint ch_num);
guint DAL_set_vca_prop_data_all(VCAPropData *data, guint ch_num);
guint DAL_set_vca_zone_data_all(VCAZoneData *data, guint ch_num);
guint DAL_set_vca_cntr_data_all(VCACntrData *data, guint ch_num);
guint DAL_set_vca_calb_data_all(VCACalbData *data, guint ch_num);
guint DAL_set_vca_calbres_data_all(VCACalbResData *data, guint ch_num);


typedef struct _VCASchedData_t	VCASchedData;
struct _VCASchedData_t {
	gchar sched[24*7 + 1];
};

guint DAL_get_vca_schd_data_all(VCASchedData *data, guint ch_num);

guint DAL_set_vca_schd_data(VCASchedData data, guint channel);
guint DAL_set_vca_schd_data_all(VCASchedData *data, guint ch_num);

//captainnn
typedef struct _EvtActVCAElem	EA_VCAElem;
struct _EvtActVCAElem {
	guint64		almOut;

	gboolean	buzzer;
	gboolean	vpop;
	gboolean	email;
};

typedef struct _EvtActVCAData	EA_VCAData;
struct _EvtActVCAData {
	EA_VCAElem	zone[IVCA_MAX_ZONES];
	EA_VCAElem	cntr[IVCA_MAX_CNTRS];
};

guint DAL_get_vca_act_data_all(EA_VCAData *data, guint ch_num);

gboolean DAL_get_vca_vPop_boolean(gint ch, gint type, gint elem);
guint DAL_set_vca_act_data_all(EA_VCAData *data, guint ch_num);


/**********************************************************/

typedef struct _DvaBxPropData_t	DvaBxPropData;
struct _DvaBxPropData_t {
	gboolean 	en_engine;
	guint 		unit;
	/* Algorithm options. */
	gboolean    en_shadowrm;
    guint       track_ref;
    gboolean    en_usecalib;
    guint       min_width3d;
	guint       min_height3d;
	gboolean    en_static_filter;
	guint       static_filter_sense;
	gboolean	active;
	gboolean	detect;
	gboolean	en_roi;	
	gboolean	en_tamper;
	gboolean	en_snapshot;	
	StdRect 	roi_rect;
	gboolean 	sw_obj_bb;
	gboolean 	sw_obj_id;
	gboolean 	sw_obj_ar;
	gboolean 	sw_obj_tm;
	gboolean 	sw_obj_tr;
	gboolean 	sw_rule;
	gboolean 	sw_rule_name;
	gboolean 	sw_roi;
	gboolean 	sw_obj_w3d;
	gboolean 	sw_obj_h3d;
	gboolean 	sw_obj_s3d;
};

typedef struct _DvaBxZone_t	DvaBxZone;
struct _DvaBxZone_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		enabled;
	gint		all_detect_obj;
	gchar		interest_obj[256];
	gint 		stop_time;
	gint 		abandon_time;
	gint 		remove_time;
	gint 		loiter_time;
	gint 		fall_time;	
	guint 		ecolor;
	gchar 		ecolor_sens;
	guint 		size_min[2];
	guint 		size_max[2];
	guint 		speed_min;
	guint 		speed_max;
	guint 		c_threshold;
	guint 		eclass;
	guint 		color;
	gint 		npts;
	StdPoint 	pt[16];
	guint 		sensitivity;
	gchar       event_audio[8][256];
};

typedef struct _DvaBxZoneData_t	DvaBxZoneData;
struct _DvaBxZoneData_t {
	guint 		cnt;
	DvaBxZone 	zone[16];
};

typedef struct _DvaBxCntr_t	DvaBxCntr;
struct _DvaBxCntr_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		enabled;
	gint 		zid_up;
	gint 		zid_dn;
	gint 		evalue;
	guint 		resetalert;
	guint 		color;
	StdPoint 	pt[4];
};

typedef struct _DvaBxCntrData_t	DvaBxCntrData;
struct _DvaBxCntrData_t {
	guint 		cnt;
	DvaBxCntr 	cntr[16];
};

typedef struct _DvaBxCalb_t	DvaBxCalb;
struct _DvaBxCalb_t {
	StdPoint    pt[2];
	gint        height;
};

typedef struct _DvaBxCalbData_t	DvaBxCalbData;
struct _DvaBxCalbData_t {
	guint       cnt;
	DvaBxCalb 	calb[32];
};

typedef struct _DvaBxCalbResData_t	DvaBxCalbResData;
struct _DvaBxCalbResData_t {
	gfloat      focal;
	gfloat      height;
	gfloat      tilt;
	guint       p_width;
	guint       p_height;
	guint       paramvalid;
};

typedef struct _DvaBxEOsdData_t DvaBxEOsdData;
struct _DvaBxEOsdData_t {
	guint display_mode;
	guint object_color;
	guint rule_color;
	guint event_color;
	guint line_width;
	guint line_transparency;
	gchar object_type[256];
};

typedef struct _DvaBxData_t	DvaBxData;
struct _DvaBxData_t {
	guint				use;
	DvaBxPropData     	prop;
	DvaBxZoneData     	zonelist;
	DvaBxCntrData     	cntrlist;
	DvaBxCalbData     	calblist;
	DvaBxCalbResData  	calbres;
	DvaBxEOsdData		eosd;
};

guint DAL_get_dvabx_data(DvaBxData *data, guint channel);
guint DAL_get_dvabx_prop_data(DvaBxPropData *data, guint channel);
guint DAL_get_dvabx_zone_data(DvaBxZoneData *data, guint channel);
guint DAL_get_dvabx_cntr_data(DvaBxCntrData *data, guint channel);
guint DAL_get_dvabx_calb_data(DvaBxCalbData *data, guint channel);
guint DAL_get_dvabx_calbres_data(DvaBxCalbResData *data, guint channel);
guint DAL_get_dvabx_eosd_data(DvaBxEOsdData *data, guint channel);

guint DAL_set_dvabx_data(DvaBxData data, guint channel);
guint DAL_set_dvabx_prop_data(DvaBxPropData data, guint channel);
guint DAL_set_dvabx_zone_data(DvaBxZoneData data, guint channel);
guint DAL_set_dvabx_cntr_data(DvaBxCntrData data, guint channel);
guint DAL_set_dvabx_calb_data(DvaBxCalbData data, guint channel);
guint DAL_set_dvabx_calbres_data(DvaBxCalbResData data, guint channel);
guint DAL_set_dvabx_eosd_data(DvaBxEOsdData data, guint channel);

guint DAL_set_dvabx_data_all(DvaBxData *data, guint ch_num);
guint DAL_set_dvabx_prop_data_all(DvaBxPropData *data, guint ch_num);
guint DAL_set_dvabx_zone_data_all(DvaBxZoneData *data, guint ch_num);
guint DAL_set_dvabx_cntr_data_all(DvaBxCntrData *data, guint ch_num);
guint DAL_set_dvabx_calb_data_all(DvaBxCalbData *data, guint ch_num);
guint DAL_set_dvabx_calbres_data_all(DvaBxCalbResData *data, guint ch_num);
guint DAL_set_dvabx_eosd_data_all(DvaBxEOsdData *data, guint ch_num);


typedef struct _DvaBxFacePropData_t	DvaBxFacePropData;
struct _DvaBxFacePropData_t {
	gboolean	active;
	gboolean 	sw_obj_bb;
	gboolean 	sw_obj_grpname;
	gboolean 	sw_rule;
	gboolean 	sw_rule_name;
};

typedef struct _DvaBxFaceZone_t	DvaBxFaceZone;
struct _DvaBxFaceZone_t {
	gboolean 	active;
	gint 		id;
	gchar 		name[32];
	guint 		type;
	gint 		npts;
	StdPoint 	pt[16];
	guint 		d_color;
	gchar		group_filter[1024];
	guint 		threshold;
};

typedef struct _DvaBxFaceZoneData_t	DvaBxFaceZoneData;
struct _DvaBxFaceZoneData_t {
	guint 			cnt;
	DvaBxFaceZone 	zone[16];
};

typedef struct _DvaBxFaceCntr_t	DvaBxFaceCntr;
struct _DvaBxFaceCntr_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		d_color;
	StdPoint 	pt[4];
};

typedef struct _DvaBxFaceCntrData_t	DvaBxFaceCntrData;
struct _DvaBxFaceCntrData_t {
	guint 			cnt;
	DvaBxFaceCntr 	cntr[16];
};

typedef struct _DvaBxFaceData_t	DvaBxFaceData;
struct _DvaBxFaceData_t {
	guint				use;
	DvaBxFacePropData   prop;
	DvaBxFaceZoneData	zonelist;
	DvaBxFaceCntrData	cntrlist;
};

guint DAL_get_dvabx_face_data(DvaBxFaceData *data, guint channel);
guint DAL_get_dvabx_face_prop_data(DvaBxFacePropData *data, guint channel);
guint DAL_get_dvabx_face_zone_data(DvaBxFaceZoneData *data, guint channel);
guint DAL_get_dvabx_face_cntr_data(DvaBxFaceCntrData *data, guint channel);

guint DAL_set_dvabx_face_data(DvaBxFaceData data, guint channel);
guint DAL_set_dvabx_face_prop_data(DvaBxFacePropData data, guint channel);
guint DAL_set_dvabx_face_zone_data(DvaBxFaceZoneData data, guint channel);
guint DAL_set_dvabx_face_cntr_data(DvaBxFaceCntrData data, guint channel);

guint DAL_set_dvabx_face_data_all(DvaBxFaceData *data, guint ch_num);
guint DAL_set_dvabx_face_prop_data_all(DvaBxFacePropData *data, guint ch_num);
guint DAL_set_dvabx_face_zone_data_all(DvaBxFaceZoneData *data, guint ch_num);
guint DAL_set_dvabx_face_cntr_data_all(DvaBxFaceCntrData *data, guint ch_num);
guint DAL_get_dvabx_face_grp(DvaBxFaceZoneData *data, gint ch);


typedef struct _DvaBxPlatenoPropData_t	DvaBxPlatenoPropData;
struct _DvaBxPlatenoPropData_t {
	gboolean	active;
	gboolean 	sw_obj_bb;
	gboolean 	sw_obj_grpname;
	gboolean 	sw_obj_number;	
	gboolean 	sw_rule;
	gboolean 	sw_rule_name;	
};

typedef struct _DvaBxPlatenoZone_t	DvaBxPlatenoZone;
struct _DvaBxPlatenoZone_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		d_color;
	gint 		npts;
	StdPoint 	pt[16];
	guint 		threshold;
	guint 		group_mask;
};

typedef struct _DvaBxPlatenoZoneData_t	DvaBxPlatenoZoneData;
struct _DvaBxPlatenoZoneData_t {
	guint 				cnt;
	DvaBxPlatenoZone 	zone[16];
};

typedef struct _DvaBxPlatenoCntr_t	DvaBxPlatenoCntr;
struct _DvaBxPlatenoCntr_t {
	gchar 		name[32];
	gint 		id;
	guint 		type;
	gboolean 	active;
	guint 		d_color;
	StdPoint 	pt[4];
};

typedef struct _DvaBxPlatenoCntrData_t	DvaBxPlatenoCntrData;
struct _DvaBxPlatenoCntrData_t {
	guint 				cnt;
	DvaBxPlatenoCntr 	cntr[16];
};

typedef struct _DvaBxPlatenoData_t	DvaBxPlatenoData;
struct _DvaBxPlatenoData_t {
	guint					use;
	DvaBxPlatenoPropData	prop;
	DvaBxPlatenoZoneData    zonelist;
	DvaBxPlatenoCntrData    cntrlist;	
};

guint DAL_get_dvabx_plateno_data(DvaBxPlatenoData *data, guint channel);
guint DAL_get_dvabx_plateno_prop_data(DvaBxPlatenoPropData *data, guint channel);
guint DAL_get_dvabx_plateno_zone_data(DvaBxPlatenoZoneData *data, guint channel);
guint DAL_get_dvabx_plateno_cntr_data(DvaBxPlatenoCntrData *data, guint channel);

guint DAL_set_dvabx_plateno_data(DvaBxPlatenoData data, guint channel);
guint DAL_set_dvabx_plateno_prop_data(DvaBxPlatenoPropData data, guint channel);
guint DAL_set_dvabx_plateno_zone_data(DvaBxPlatenoZoneData data, guint channel);
guint DAL_set_dvabx_plateno_cntr_data(DvaBxPlatenoCntrData data, guint channel);

guint DAL_set_dvabx_plateno_data_all(DvaBxPlatenoData *data, guint ch_num);
guint DAL_set_dvabx_plateno_prop_data_all(DvaBxPlatenoPropData *data, guint ch_num);
guint DAL_set_dvabx_plateno_zone_data_all(DvaBxPlatenoZoneData *data, guint ch_num);
guint DAL_set_dvabx_plateno_cntr_data_all(DvaBxPlatenoCntrData *data, guint ch_num);


typedef struct _DvaBxSchedData_t	DvaBxSchedData;
struct _DvaBxSchedData_t {
	gchar sched[24*7 + 1];
};

guint DAL_get_dvabx_schd_data_all(DvaBxSchedData *data, guint ch_num);

guint DAL_set_dvabx_schd_data(DvaBxSchedData data, guint channel);
guint DAL_set_dvabx_schd_data_all(DvaBxSchedData *data, guint ch_num);

//////////////

typedef struct _DVACntrData_t	DVACntrData;
struct _DVACntrData_t {
	gboolean	active;
	guint		display_color;
	gboolean	notification;
	guint		e_value;
	gboolean	reset;
};

typedef struct _DVAIdzData_t	DVAIdzData;
struct _DVAIdzData_t {
	gboolean	active;
	guint		human_item_cnt;
	gchar		human_item_list[512];
	guint		vehicle_item_cnt;
	gchar		vehicle_item_list[512];
	guint		animal_item_cnt;
	gchar		animal_item_list[512];
	gboolean    en_static_filter;
	guint       static_filter_sense;
	guint       confidence_threshold;
	DVACntrData cntr;
};

typedef struct _DVAIpzData_t	DVAIpzData;
struct _DVAIpzData_t {
	gboolean	active;
	guint		item_cnt;
	gchar		item_list[512];
	guint		dwell;
};

typedef struct _DVALprData_t	DVALprData;
struct _DVALprData_t {
	gboolean	active;
	guint		white_item_cnt;
	gchar		white_item_list[512];
	guint		black_item_cnt;
	gchar		black_item_list[512];
};

typedef struct _DVAPropData_t	DVAPropData;
struct _DVAPropData_t {
	gboolean	active;
	guint		ignore_interval;
	DVAIdzData	idz;
	DVAIpzData	ipz;
	DVALprData	lpr;
	gfloat 		roi_sx;
	gfloat 		roi_sy;
	gfloat 		roi_ex;
	gfloat 		roi_ey;
};

guint DAL_get_dva_prop_data(DVAPropData *data, guint channel);

guint DAL_set_dva_prop_data(DVAPropData data, guint channel);
guint DAL_set_dva_prop_data_all(DVAPropData *data, guint ch_num);

typedef struct _DVASchedData_t	DVASchedData;
struct _DVASchedData_t {
	gchar 	sched[7][24];
};

guint DAL_get_dva_schd_data(DVASchedData *data, guint ch);

guint DAL_set_dva_schd_data(DVASchedData data, guint ch);
guint DAL_set_dva_schd_data_all(DVASchedData *data, guint ch_num);

//////////////

typedef struct _AiAnalysisActData_t	AiAnalysisActData;
struct _AiAnalysisActData_t {
	gboolean classic_active;
	gboolean builtin_active;
	gboolean dvacam_active;
	gboolean dvabox_active;
	gchar dvabox_mac[32];
	guint dvabox_ipaddr;
	gchar dvabox_id[128];
	gchar dvabox_pass[128];
	gchar algorithm[256];
};

guint DAL_get_aianalysis_act_data(AiAnalysisActData *data, guint channel);
guint DAL_set_aianalysis_act_data(AiAnalysisActData data, guint channel);
guint DAL_set_aianalysis_act_data_all(AiAnalysisActData *data, guint ch_num);


typedef struct _AiAnalysisSchedData_t	AiAnalysisSchedData;
struct _AiAnalysisSchedData_t {
	gchar sched[24*7 + 1];
};

guint DAL_get_aianalysis_schd_data_all(AiAnalysisSchedData *data, guint ch_num);

guint DAL_set_aianalysis_schd_data(AiAnalysisSchedData data, guint channel);
guint DAL_set_aianalysis_schd_data_all(AiAnalysisSchedData *data, guint ch_num);


#ifdef __SUPPORT_BS8418__
typedef struct _LowContrast_t 	LowContrData;
struct _LowContrast_t {
	gboolean	white_detect;
	gboolean 	black_detect;
	guint 		white_level;
	guint		black_level;
};

gboolean DAL_get_low_contr_data(LowContrData *data);
gboolean DAL_set_low_contr_data(LowContrData *data);
#endif

typedef struct _SysCamStream_t	StreamData;
struct _SysCamStream_t {
	gchar           size[STRING_SIZE_8][STRING_SIZE_8];
	guint		    max_fps[STRING_SIZE_8];
	guint		    min_bps[STRING_SIZE_8];
	guint		    max_bps[STRING_SIZE_8];
	gchar           control[STRING_SIZE_8][STRING_SIZE_8];
	gchar           vcodec[STRING_SIZE_8][STRING_SIZE_8];
	guint			corridor_mode;
};

guint DAL_get_stream_data(StreamData *data, guint channel);
guint DAL_set_stream_data(StreamData data, guint channel);

/////////////////////////
typedef struct _SysCamFocus_t FocusData;
struct _SysCamFocus_t {
    guint       cell_cnt;
    gboolean    day_night;
    gboolean    temperature;
};

guint DAL_get_focus_data(FocusData *data, guint channel);
guint DAL_set_focus_data(FocusData data, guint channel);

/////////////////////////
typedef struct _FishEyeData_t FishEyeData;
struct _FishEyeData_t {
    guint dewarp_mode;
    guint mount_type;
};

guint DAL_get_fisheye_data(FishEyeData *data, gint ch);
guint DAL_set_fisheye_data(FishEyeData *data, gint ch_num);

typedef struct _SysCamItxFisheye_t CamItxFisheyeData;
struct _SysCamItxFisheye_t {
    gboolean    act;
    guint    	mount_mode;
    gboolean    lens_mode;
	gchar    	lens_type[256];
	guint    	default_view;	
};

guint DAL_get_camera_itx_fisheye_data(CamItxFisheyeData *data, guint channel);
guint DAL_set_camera_itx_fisheye_data(CamItxFisheyeData data, guint channel);
guint DAL_set_camera_itx_fisheye_data_all(CamItxFisheyeData *data, guint ch_num);

#define MAX_FISHEYE_VTYPE 4

typedef struct _ItxTmpFisheye_t ItxTmpFisheyeData;
struct _ItxTmpFisheye_t {
    guint    	dewarp;
    guint    	viewtype;	
	gfloat		pan[MAX_FISHEYE_VTYPE];
	gfloat		tilt[MAX_FISHEYE_VTYPE];
	gfloat		zoom[MAX_FISHEYE_VTYPE];
	gfloat		roll[MAX_FISHEYE_VTYPE];
};

guint DAL_get_itx_tmp_fisheye_data(ItxTmpFisheyeData *data, guint channel);
guint DAL_set_itx_tmp_fisheye_data(ItxTmpFisheyeData data, guint channel);



/////////////////////////


/**********************************************
 * Setup --> System --> Display
 * ********************************************/
typedef struct _SysDispOsd_t	OsdData;
struct _SysDispOsd_t {
	guint	statusBar;
	guint	camTitle;
	guint	evtIcon;
	guint	border;
	guint	borderColor;
	guint	menuTrans;
	guint	motSenDisp;
	guint	motionColor;
	guint	motionTrans;
	gchar	lang[STRING_SIZE_32];
	gboolean	audio;
	guint		timeline;
	gboolean	user_name;
	gboolean	sideicon;
	gboolean	statusicon;
	guint   zoompip;
    gboolean    time;
};

guint DAL_set_language(gchar *lang);
void DAL_get_language(gchar *lang);
gint DAL_get_support_lang_cnt();
gboolean DAL_get_support_lang(gint index, gchar *buf);
gboolean DAL_get_support_lang_alias(gint index, gchar *buf);
guint DAL_get_osd_data(OsdData *data);
guint DAL_set_osd_data(OsdData *data);
gboolean DAL_get_border_color(guint *color_index);
guint DAL_get_live_status_on_time();
guint DAL_get_live_timeline_on_mode();
guint DAL_set_live_timeline_on_mode(guint mode);
gboolean DAL_get_username_on_off();

/************************************************/

typedef struct _SysDispMonitor_t	MonitorData;
struct _SysDispMonitor_t {
	guint		seqDwell;
	guint		spotDwell;
    guint       hd_spotDwell;
	guint		deinterlace;
	guint		alarmPopup;
	guint		alarmDwell;
	guint		motPopup;
	guint		motDwell;
	gboolean	aspect;
	gboolean	overscan;
	guint		monitorType;
	gboolean	dualmonitor;
    guint       hdsdiOutMode;
	guint		resolution;
};

guint DAL_get_monitor_data(MonitorData *data);
guint DAL_set_monitor_data(MonitorData *data);

guint DAL_get_sequence_dwell();
guint DAL_get_spot_dwell_time();
guint DAL_get_alarm_popup_onoff();

/**********************************************/
gboolean DAL_set_winid_data(gint *data);
gboolean DAL_get_winid_data(gint *data);

/**********************************************/

/**********************************************/

guint DAL_get_sequence_count();
guint DAL_set_sequence_count(guint cnt);

/**********************************************/

typedef struct _SysDispSeqElement_t	SeqElementData;
struct _SysDispSeqElement_t {
	guint	type;
#define MAX_DISP_DIVISION_NUM	32
	guint	conf[MAX_DISP_DIVISION_NUM];
};

/**********************************************/

typedef struct _SysDispSeq_t	SeqData;
struct _SysDispSeq_t {
	gchar				title[STRING_SIZE_16];
	gchar				createdby[STRING_SIZE_32];
	SEQ_VALID_MODE_E	valid_mode;
	guint	numItems;
#define	MAX_SEQ_ITEMS_NUM	16
	SeqElementData		items[MAX_SEQ_ITEMS_NUM];
};

guint DAL_get_seq_data(SeqData *data, guint seq_idx);
guint DAL_set_seq_data(SeqData data, guint seq_idx);
guint DAL_set_seq_data_all(SeqData *data, guint seq_num);

/**********************************************/

// ?¯?ÏÇÏ°Ô ÇÏ³ª?Ç ±¸Á¶Ã¼¿¡¼­ 16Ã¤³Î ¸ðµÎ?Ç Á¤º¸¸¦ °¡Áö°í ?Ö?½.
#if 0
typedef struct _SysDispSpot_t	SpotData;
struct _SysDispSpot_t {
	guint	channel[16];
};
#else

typedef struct _SysDispSpotElement_t	SpotElementData;
struct _SysDispSpotElement_t {
	guint	type;
#define MAX_SPOT_DIVISION_NUM	32
	guint	conf[MAX_SPOT_DIVISION_NUM];
};

typedef struct _SysDispSpot_t	SpotData;
struct _SysDispSpot_t {
	gchar				title[STRING_SIZE_16];
	gchar				createdby[STRING_SIZE_32];
	SPOT_VALID_MODE_E	valid_mode;
	guint				numItems;
#define	MAX_SPOT_ITEMS_NUM	32
	SpotElementData		items[MAX_SPOT_ITEMS_NUM];
};

#endif

guint DAL_get_spot_data(SpotData *data, guint spot_idx);
guint DAL_set_spot_data(SpotData data, guint spot_idx);
guint DAL_set_spot_data_all(SpotData *data, guint spot_num);

guint DAL_get_HD_spot_data(SpotData *data, guint spot_idx);
guint DAL_set_HD_spot_data(SpotData data, guint spot_idx);
guint DAL_set_HD_spot_data_all(SpotData *data, guint spot_num);

/**********************************************/

guint DAL_get_spot_count();
guint DAL_set_spot_count(guint cnt);

guint DAL_get_HD_spot_count();
guint DAL_set_HD_spot_count(guint cnt);

guint DAL_get_spot_valid_mode(guint spot_idx);


/**********************************************/

typedef struct _SysDispAdvancedDual_t   AdvDualData;
struct _SysDispAdvancedDual_t {
    guint   mode;
    guint   monitor_type[2];
    guint   monitor_resol[2];
    guint   spot_cnt;
#define MAX_SPOT_ITEMS_NUM  32
    SpotElementData     spot[MAX_SPOT_ITEMS_NUM];
};

guint DAL_get_advanced_dual_data(AdvDualData *data);
guint DAL_set_advanced_dual_data(AdvDualData *data);

/**********************************************/

guint DAL_get_spot_count();
guint DAL_set_spot_count(guint cnt);

/**********************************************/

typedef struct _SysDispScrSaver_t		ScrSaverData;
struct _SysDispScrSaver_t {
	guint	autoBright;
	guint	dispOff;
	guint	from;	// data type ¹Ù²î¾î¾ß ÇÔ. ?§Á¬ Æ¯¼º °í·Á..
	guint	to;		// data type ¹Ù²î¾î¾ß ÇÔ.  ?§Á¬ Æ¯¼º °í·Á...
};

guint DAL_get_scrSaver_data(ScrSaverData *data);
guint DAL_set_scrSaver_data(ScrSaverData *data);


/**********************************************/

typedef struct _SysDispPosOsd_t		    PosOsdData;
struct _SysDispPosOsd_t {
	guint	    disp_mode;
	guint	    position;
	guint	    font;
	guint	    normal_color;
	guint	    duration;
	guint	    format;
	guint	    scroll;
	gboolean	highlight;
	gchar	    highlight_text[8][STRING_SIZE_32];
	guint	    highlight_color[8];
	gboolean    exclude;
	gchar	    exclude_text[8][STRING_SIZE_32];
};

guint DAL_get_pososd_data(PosOsdData *data);
guint DAL_set_pososd_data(PosOsdData *data);

/**********************************************/
typedef struct _SysDispLiveElement_t	LiveElementData;
struct _SysDispLiveElement_t {
	guint	type;
#define MAX_LIVE_DIVISION_NUM	16
	guint	conf[MAX_LIVE_DIVISION_NUM];
};

typedef struct _SysDispLive_t	LiveData;
struct _SysDispLive_t {
	gchar	title[STRING_SIZE_16];
	gchar	createdby[STRING_SIZE_32];
//	guint	valid_mode;
	LiveElementData	items;
};

guint DAL_get_live_count();
guint DAL_set_live_count(guint cnt);
guint DAL_get_live_data(LiveData *data, guint idx);
guint DAL_set_live_data(LiveData data, guint idx);
guint DAL_set_live_data_all(LiveData *data, guint num);


/*****************************************************
 * Setup --> System --> Sound
 * **************************************************/
typedef struct _SysSoundAudio_t	AudioData;
struct _SysSoundAudio_t {
	guint liveAudio;
	guint channel;
#ifdef __SUPPORT_BS8418__
	guint aud_mode[4];
	guint aud_in[4];
#endif
	guint netAudioTx;
	guint netAudioRx;
	guint inDev[16];
	guint audioType;
};

guint DAL_get_audio_data(AudioData *data);
guint DAL_set_audio_data(AudioData *data);
gboolean DAL_set_audio_channel(guint ch);

/**************************************************/

typedef struct _SysSoundBuzzer_t	BuzzerData;
struct _SysSoundBuzzer_t {
	gboolean keypad;
	gboolean remote;
};

guint DAL_get_buzzer_data(BuzzerData *data);
guint DAL_set_buzzer_data(BuzzerData *data);





/****************************************************
 * Setup --> System --> System
 * *************************************************/
typedef struct _SysSystemDateTime_t	DateTimeData;
struct _SysSystemDateTime_t {
	guint	dateFormat;
	guint	timeFormat;
	gchar	timeServer[STRING_SIZE_32];
	guint	timeZone;
	guint	dst;
    guint	auto_timesync;
    guint	auto_interval_min;
    guint	sync_time;
    guint   sync_freq;
    guint   last_sync_time;
	guint	time_sync_off;
};

guint DAL_get_Dst_data();
guint DAL_get_dateTime_data(DateTimeData *data);
guint DAL_set_dateTime_data(DateTimeData *data);
gboolean DAL_get_dateTime_format(guint *dFormat, guint *tFormat);
gboolean DAL_set_timezone(guint timezone);
gint DAL_get_dst();
gint DAL_get_tz_data(guint *timeZone);
gboolean DAL_set_last_sync_time(guint sync_time);


/*****************************************************/

typedef struct _SysSystemManage_t	SysManageData;
struct _SysSystemManage_t {
	gchar	name[STRING_SIZE_32];
	guint	enPassword;
	guint	pwExpired;
	guint	pwChanged;
	guint	poeLimt;
	guint	poeHubLimt;
	guint	sigType;
	guint	swmode;
	guint	poemode;
	guint	remotefw_check;
};

guint DAL_get_sysManage_data(SysManageData *data);
guint DAL_set_sysManage_data(SysManageData *data);
gboolean DAL_get_passwd_enable();
gboolean DAL_get_passwd_change_enable();
guint DAL_get_pw_expired_conf();
gboolean DAL_get_encorder_mode();

/****************************************************/

typedef	struct _SysSystemInfo_t	SysInfoData;
struct _SysSystemInfo_t	{
	gchar	model[STRING_SIZE_64];
	gchar	sysId[STRING_SIZE_32];
	gchar	swVer[STRING_SIZE_32];
	gchar	hwVer[STRING_SIZE_32];
	gchar	ipAddr[STRING_SIZE_32];
	gchar	macAddr[STRING_SIZE_32];
	gchar	ddnsName[STRING_SIZE_256];
	guint	rtspPort;
	guint	webPort;
	guint	fwupTime;

//	gchar	netPort[STRING_SIZE_32];
//	gchar	webPort[STRING_SIZE_32];
//	gchar	site[STRING_SIZE_32];	//??
};

guint DAL_get_sysInfo_data(SysInfoData *data);
guint DAL_set_sysInfo_data(SysInfoData *data);
gboolean DAL_set_sig_type(gint type);
gint DAL_get_sig_type();
gboolean DAL_set_agr_policy(gint agr);
gint DAL_get_agr_policy();
guint DAL_set_dev_authcode(gchar *code);
guint DAL_get_dev_authcode(gchar *code);
guint DAL_get_fw_version(gchar *version);
guint DAL_get_hub1_version(gchar *version);
guint DAL_get_hub2_version(gchar *version);
guint DAL_get_disp_fw_version(gchar *version);
guint DAL_get_disp_hw_version(gchar *version);
gboolean DAL_set_fwup_date(guint fwup_time);
gboolean DAL_set_fwup_state(gint fwup_state);
gint DAL_get_fwup_state();


/****************************************************/

typedef struct _FacInitData_t	FacInitData;
struct _FacInitData_t {
	gchar	lang[STRING_SIZE_32];
	guint	timeZone;
	guint	sigType;
};

gboolean DAL_get_fac_init_func(void);
gboolean DAL_get_fac_init_run(void);
guint DAL_set_fac_init_run(gboolean data);
guint DAL_get_fac_init_data(FacInitData *data);
guint DAL_set_fac_init_data(FacInitData *data);

/****************************************************/

#define MAX_CTRLDEV_PROTO_STR_LENGTH 	(16)
typedef	struct _SysSystemControlDev_t	ControlDevData;
struct _SysSystemControlDev_t {
	guint	sysID;
	guint	proto;
	guint	baud;
	guint	rmcID;
/*
	gchar	proto[STRING_SIZE_32];
	gchar	baud[STRING_SIZE_16];
*/
#ifdef __SUPPORT_BS8418__
	guint 	port;
	guint 	remote_id;
	guint	ups_proto;
#endif

	guint	secomdual_act;
	guint	secomdual_ipaddr[4];
	guint	secomdual_port;
};

guint DAL_get_SystemID();
guint DAL_get_RemoconID();
guint DAL_get_controlDev_data(ControlDevData *data);
guint DAL_set_controlDev_data(ControlDevData *data);
//guint DAL_get_controlDev_protocol_name(gchar *name, guint proto_num);
// khj776
guint DAL_get_controlDev_protocol(gchar *name, gint idx);
gboolean DAL_get_controlDev_addr(guint *addr);
#if defined(__SAMSUNG_UI__)
gboolean DAL_get_remote_id(guint *id);
gboolean DAL_set_remote_id(guint id);
#endif

/****************************************************/

typedef struct _SysSystemSecurity_t	SecurityData;
struct _SysSystemSecurity_t{
	guint audio;
	guint snapshot;
	guint diagnostic_data;
	guint pwrule;
	guint loginSearchArch;
	guint pwRawbackup;
	guint usable_defpw;
    guint double_login; 
    guint id_input_mode;
    guint pin_id_input_mode;
    guint idrule;
    guint question_pw_reset;
};

guint DAL_get_security_data(SecurityData *data);
guint DAL_set_security_data(SecurityData *data);
gboolean DAL_get_support_audio();
gboolean DAL_get_support_snapshot();
gboolean DAL_get_enahnced_password();
gboolean DAL_set_snapshot_off();
gboolean DAL_get_use_double_login(gboolean *use);
guint DAL_get_id_input_mode();
gboolean DAL_get_enahnced_id();


/*****************************************************
      System-> pos setup
*****************************************************/

typedef	struct _SysPosDev_t	PosDevData;
struct _SysPosDev_t {
    guint   enable;
    guint   type;
    gchar   port[128];
	guint	protocol;
	guint	char_set;
	guint   baud;
	guint   parity;
	guint   databit;
	guint   stopbit;
    gchar   transact_start[256];
    gchar   transact_end[256];
    gchar   endofline[256];
    gchar   ignore[256];
};

guint DAL_get_posdev_data(PosDevData *data, guint channel);
guint DAL_set_posdev_data(PosDevData *data, guint channel);

/*****************************************************
      System-> License
*****************************************************/
#define MAX_LICENSE_CNT     16

typedef struct _SysLicenseKey_t LicenseKeyData;
struct _SysLicenseKey_t {
    gchar key[64];
    guint acquired_date;
    guint expired_date;
};

typedef struct _SysLicense_t LicenseData;
struct _SysLicense_t {
    LicenseKeyData key_data[MAX_LICENSE_CNT];
    guint count;
};

guint DAL_get_license_data(LicenseData *data);
guint DAL_set_license_data(LicenseData *data);

/*******************************************************
 * Setup --> System --> User
 * ****************************************************/
#define	MAX_USER_COUNT	(120)
#define QNA_COUNT  (3)

typedef struct _SysUserManage_t UserManageData;
struct _SysUserManage_t {
	gchar	id[STRING_SIZE_16];
	gchar	pw[STRING_SIZE_16+1];
	gchar	group[STRING_SIZE_16];
	gchar	email[STRING_SIZE_64];
	guint	email_noti;
	guint   email_certification;
	gchar	covert[STRING_SIZE_32+1];
	guint	expired_check;
	guint	pw_last_changed;
	gchar	phone[STRING_SIZE_16];
	guint	phone_noti;
	guint   phone_certification;
	guint	e_serv;
	guint	init_pw_changed;
	guint   certi_popup_hide;
	guint	use_pin;
	gchar	pin_number[32];
    gint    question[QNA_COUNT];
    gchar   answer[QNA_COUNT][STRING_SIZE_64+1];
};

guint DAL_get_userManage_data(UserManageData *data, guint idx);
guint DAL_set_userManage_data(UserManageData data, guint idx);
guint DAL_set_userManage_data_all(UserManageData *data, guint user_num);

guint DAL_get_user_count();
guint DAL_set_user_count(guint cnt);

gboolean DAL_get_user_id(gchar *id, guint idx);
gboolean DAL_get_user_passwd(gchar *passwd, guint idx);
gboolean DAL_set_user_passwd(gchar *passwd, guint idx);
gboolean DAL_get_user_group(gchar *group, guint idx);
gboolean DAL_get_user_email(gchar *email, guint idx);
gboolean DAL_get_user_covert(gchar *covert, guint idx);
gboolean DAL_get_user_phone(gchar *phone, guint idx);

guint DAL_get_expire_check_time(guint idx);
guint DAL_set_expire_check_time(guint time, guint idx);
guint DAL_get_pw_last_changed_time(guint idx);
guint DAL_set_pw_last_changed_time(guint time, guint idx);
guint DAL_get_init_pw_changed(guint idx);
guint DAL_set_init_pw_changed(guint time, guint idx);

/****************************************************/

#define	MAX_GROUP_COUNT		(3)
typedef struct _SysUserAuth_t	UserAuthData;
struct _SysUserAuth_t {
	gchar	grp_name[32];
	guint	sys_setup;
	guint	search;
	guint	archive;
	guint	rec_setup;
	guint	event;
	guint	audio;
	guint	microphone;
	guint 	remote;
	guint 	ptz;
	guint	shutdown;
	guint	cvt_disp;
	guint   sequrinet;
};

guint DAL_get_group_count();
gboolean DAL_get_grp_covert_shown_as(guint grp_idx);
guint DAL_get_userAuth_data(UserAuthData *data, guint idx);
guint DAL_set_userAuth_data(UserAuthData data, guint idx);
guint DAL_set_userAuth_data_all(UserAuthData *data, guint group_num);

/****************************************************/

typedef struct _SysUserLogout_t	LogoutData;
struct _SysUserLogout_t {
	guint	autoLogout;
	guint	duration;
};
guint DAL_get_logout_data(LogoutData *data);
guint DAL_set_logout_data(LogoutData *data);




/********************************************************
 * Setup --> System --> Network
 * *****************************************************/
typedef struct _SysNetIP_t	IPSetupData;
struct _SysNetIP_t {
	guint	dhcp;
	guint	webServ;
	guint	ip[4];
	guint	gateway[4];
	guint	subnet[4];
	guint	dns1[4];
	guint	dns2[4];
	guint	netPort;
	guint	webPort;
	guint	txSpeed;
#ifdef _UPNP_SUPPORT_
  	guint 	rtspport;
#endif
	guint 	autoPort;
};

guint DAL_get_ipSetup_data(IPSetupData *data);
guint DAL_set_ipSetup_data(IPSetupData *data);
gboolean DAL_get_ipAddr(gchar *ipAddr);
gboolean DAL_is_dhcp_on();

/****************************************************/
#define SUPPORT_IPV6_ADDR_CNT       4

typedef struct _SysNetIPv6_t    IPv6Data;
struct _SysNetIPv6_t {
    guint use;
    gchar linklocal[STRING_SIZE_64];
    gchar ipaddr[SUPPORT_IPV6_ADDR_CNT][STRING_SIZE_64];
    guint prefix_len[SUPPORT_IPV6_ADDR_CNT];
    gchar gateway[STRING_SIZE_64];
    gchar dns1[STRING_SIZE_64];
    gchar dns2[STRING_SIZE_64];
};

guint DAL_get_ipv6_data(IPv6Data *data);
guint DAL_set_ipv6_data(IPv6Data *data);

/****************************************************/

typedef struct _SysNetDualLanIP_t	IPDualLanData;
struct _SysNetDualLanIP_t {
	guint	dhcpsvr;
	guint	ip[4];
	guint	gateway[4];
	guint	subnet[4];
	guint	dhcp_pool_start[4];
	guint	dhcp_pool_end[4];
};

guint DAL_get_ipDualLan_data(IPDualLanData *data);
guint DAL_set_ipDualLan_data(IPDualLanData *data);

/****************************************************/

typedef struct _SysNetDdns_t	 DDNSData;
struct _SysNetDdns_t {
	guint	enable;
	gchar	server[STRING_SIZE_32];
	gchar	serverip[STRING_SIZE_32];
	gchar	id[STRING_SIZE_64];
	gchar	passwd[STRING_SIZE_64];
  	gchar 	host_name[STRING_SIZE_64];
  	guint 	p2p_enable;
};

guint DAL_get_ddns_data(DDNSData *data);
guint DAL_set_ddns_data(DDNSData *data);
gboolean DAL_get_ddns_hostname(gchar *hostname);
gboolean DAL_is_ddns_on();

/****************************************************/

typedef struct _SysNetEmail_t	EmailData;
struct _SysNetEmail_t {
	gchar	server[STRING_SIZE_256];
	guint	port;
	guint	security;
	gchar	user[STRING_SIZE_64];
	gchar	passwd[STRING_SIZE_32];
	gchar	testMail[STRING_SIZE_64];
	gchar	from[STRING_SIZE_64];
	guint	individual;
};

guint DAL_get_email_data(EmailData *data);
guint DAL_set_email_data(EmailData *data);

/****************************************************/
#ifdef __SUPPORT_BS8418__
#define FTP_SERVER_COUNT 	2

typedef struct _SysNetFTP_t		FtpData;
struct _SysNetFTP_t {
	gchar server[FTP_SERVER_COUNT][STRING_SIZE_64];
	gchar user[FTP_SERVER_COUNT][STRING_SIZE_32];
	gchar passwd[FTP_SERVER_COUNT][STRING_SIZE_32];
	gchar path[FTP_SERVER_COUNT][STRING_SIZE_256];
};

guint DAL_get_ftp_data(FtpData *data);
guint DAL_set_ftp_data(FtpData *data);


/****************************************************/
typedef struct _SysNetModem_t		ModemData;
struct _SysNetModem_t {
	gchar model[STRING_SIZE_64];
	gchar vendor[STRING_SIZE_64];
	gchar user[STRING_SIZE_32];
	gchar passwd[STRING_SIZE_32];
	guint retry;
};

guint DAL_get_modem_data(ModemData *data);
guint DAL_set_modem_data(ModemData *data);
#endif

/****************************************************/
typedef struct _SysNetSecurityEncrypt_t	EncryptData;
struct _SysNetSecurityEncrypt_t {
	guint	srtspon;
	gchar	srtsp_method[STRING_SIZE_64];
	guint	srtsp_level;
	guint	httpson;
	guint	webport_2nd;
	guint	httpauth_method;
};

guint DAL_get_encrypt_data(EncryptData *data);
guint DAL_set_encrypt_data(EncryptData *data);
gboolean DAL_get_httpson();

/****************************************************/
#define IPFILTER_RULE_MAX (8)

typedef struct _SysNetSecurityIPFilterList_t_t	IPFilterListData;
struct _SysNetSecurityIPFilterList_t_t{
	guint	type;
	guint	listAddr[4];
};

typedef struct _SysNetSecurityIPFilter_t	IPFilterData;
struct _SysNetSecurityIPFilter_t {
	guint	enable;
	guint	opmode;

	IPFilterListData filter_list[IPFILTER_RULE_MAX];
};

guint DAL_get_ipfilter_data(IPFilterData *data);
guint DAL_set_ipfilter_data(IPFilterData *data);
guint DAL_get_ipfilter_count();
guint DAL_set_ipfilter_count(guint cnt);

/****************************************************/

typedef struct _SysNetRtp_t RtpData;
struct _SysNetRtp_t
{
    guint   rtpsport;
    guint   rtpeport;
    gboolean   audio_backch_mode;
    guint   audio_backch_port;
};

guint DAL_get_rtp_data(RtpData *data);
guint DAL_set_rtp_data(RtpData *data);

/****************************************************/

typedef struct _SysNetMulticastList_t MulticastListData;
struct _SysNetMulticastList_t
{
    guint   stream_ip[4];
    guint   video_port;
    guint   audio_port;
};

typedef struct _SysNetMulticast_t MulticastData;
struct _SysNetMulticast_t
{
    guint   ttl;

    MulticastListData list[2][32];
};

guint DAL_get_multicast_data(MulticastData *data, gint stream_cnt, gint ch_cnt);
guint DAL_set_multicast_data(MulticastData *data, gint stream_cnt, gint ch_cnt);

/****************************************************/
// Network -> Snmp -> SnmpData
//

typedef struct _SnmpData_t SnmpData;
struct _SnmpData_t {
    guint snmp_mode;

    gchar v2c_cmt_str[64];
    gchar v2c_address_tr[64];
    gchar v2c_cmt_str_tr[64];

    gchar v3_engine_id[64];
    gchar v3_userid[128];

    guint v3_auth_combo;
    gchar v3_auth_key[128];
    guint v3_priv_combo;
    gchar v3_priv_key[128];
    gchar v3_address_tr[64];
};

gboolean DAL_get_SnmpData(SnmpData *data);
gboolean DAL_set_SnmpData(SnmpData *data);

/****************************************************/
// Network -> Security -> 802.1x
//

typedef struct _IEEE8021xData_t IEEE8021xData;
struct _IEEE8021xData_t {
	guint use;
	guint eap_type;
	guint eapol_ver;
	gchar id[64];
	gchar pw[128];
	gchar anonymous[128];
	gchar inner_auth[128];
	gchar prikey_pw[128];
};

guint DAL_get_8021x_data(IEEE8021xData *data);
guint DAL_set_8021x_data(IEEE8021xData *data);

/****************************************************/


/**************************************************************
 * Setup --> System --> Event / Sensor
 * ************************************************************/
typedef struct _SysEventHdd_t	HDDData;
struct _SysEventHdd_t {
	guint	smart;
	guint	interval;
	guint	diskFull;
};

guint DAL_get_hddEvt_data(HDDData *data);
guint DAL_set_hddEvt_data(HDDData *data);
guint DAL_set_smart_interval(guint interval);
guint DAL_get_smart_interval();

/****************************************************/

typedef struct _SysEventAlarmIn_t	AlarmInputData;
struct _SysEventAlarmIn_t {
	gchar	desc[STRING_SIZE_64];
	guint	oper;
	guint	type;
#ifdef __SUPPORT_BS8418__
	guint 	delay;
	guint 	false_cnt;
#endif
};

guint DAL_get_alarmInput_data(AlarmInputData *data, guint idx);
guint DAL_set_alarmInput_data(AlarmInputData data, guint idx);
guint DAL_set_alarmInput_data_all(AlarmInputData *data, guint ch_num);
gboolean DAL_get_alarmInput_desc(guint ch_num, gchar desc[STRING_SIZE_64]);

/****************************************************/

typedef struct _SysEventAlarmOut_t	AlarmOutData;
struct _SysEventAlarmOut_t {
	guint	mode;
	guint	type;
	guint	oper;
	guint	duration;
	guint	hddEvt;
#ifdef __SUPPORT_BS8418__
	guint  	upsEvt;
#endif

	guint	alarm[16];
	guint	videoLoss[16];
	guint	motion[16];
#ifdef __SUPPORT_BS8418__
	guint 	lowContr[16];
#endif
};

guint DAL_get_alarmOut_data(AlarmOutData *data, guint channel);
guint DAL_set_alarmOut_data(AlarmOutData data, guint channel);
guint DAL_set_alarmOut_data_all(AlarmOutData *data, guint ch_num);

/****************************************************/

typedef struct _SysEventBuzzOut_t	BuzzOutData;
struct _SysEventBuzzOut_t {
	guint	oper;
	guint	mode;
	guint	hddEvt;
	guint	duration;
#ifdef __SUPPORT_BS8418__
	guint	upsEvt;
#endif

	guint	alarm[16];
	guint	videoLoss[16];
	guint	motion[16];
#ifdef __SUPPORT_BS8418__
	guint	lowContr[16];
#endif
};

guint DAL_get_buzzOut_data(BuzzOutData *data);
guint DAL_set_buzzOut_data(BuzzOutData *data);

/****************************************************/

typedef struct _SysEventEmailNoti_t	EmailNotiData;
struct _SysEventEmailNoti_t {
	guint	noti;
	guint	hddEvt;
	guint	setupChange;
	guint	bootEvt;
#ifdef __SUPPORT_BS8418__
	guint	upsEvt;
#endif

	guint	freq;

	guint	alarm[16];
	guint	videoLoss[16];
	guint	motion[16];
#ifdef __SUPPORT_BS8418__
	guint 	lowContr[16];
#endif
	guint 	snapshot_ch;
};

guint DAL_get_emailNoti_data(EmailNotiData *data);
guint DAL_set_emailNoti_data(EmailNotiData *data);

/****************************************************/
#ifdef __SUPPORT_BS8418__
typedef struct _SysEventPtzPreset_t		PtzPresetdata;
struct _SysEventPtzPreset_t {
	gboolean 	alarm[16];
	gboolean	vloss[16];
	gboolean	motion[16];

	guint	alarm_preset[16];
	guint	vloss_preset[16];
	guint	motion_preset[16];
};

guint DAL_get_ptzPreset_data(PtzPresetdata *data);
guint DAL_set_ptzPreset_data(PtzPresetdata *data);
#endif

/****************************************************/
#if defined(__SUPPORT_BS8418__) || defined(__SAMSUNG_UI__)
typedef struct _SysEventSnapCapture_t	SnapCaptureData;
struct _SysEventSnapCapture_t {
	gboolean 	alarm[16];
	gboolean	motion[16];
	gboolean	lcont[16];

	guint	alarm_delay[16];
	guint	motion_delay[16];
	guint	lcont_delay[16];
};

guint DAL_get_snapCapture_data(SnapCaptureData *data);
guint DAL_set_snapCapture_data(SnapCaptureData *data);
#endif


/******************************************************
 * Setup --> Event&Action --> Alarm Out --> Alarm Out
 * ***************************************************/
typedef struct _EvtActAlarmOutData	EA_AlmOutData;
struct _EvtActAlarmOutData {
	gchar 		name[STRING_SIZE_ALARMOUT];
	guchar 		type;
	guchar 		oper;
	gint 		duration;
};

gboolean DAL_set_almOut_data(EA_AlmOutData data, guint ch);
gboolean DAL_get_almOut_data(EA_AlmOutData *data, guint ch);
gboolean DAL_set_almOut_data_all(EA_AlmOutData *data, guint ch);
//gboolean DAL_get_nvr_almOut_data(EA_AlmOutData *data, guint ch);
//gboolean DAL_set_nvr_almOut_data_all(EA_AlmOutData data, guint ch);



/***************************************************************
 * Setup --> Event&Action --> Alarm Out --> On/Off Schedule
 * *************************************************************/
typedef struct _EvtActAlarmSchedData	EA_AlmSchedData;
struct _EvtActAlarmSchedData {
	gint 	day;
	gchar 	sched[7][24];
};

gboolean DAL_set_almSched_data(EA_AlmSchedData data, guint ch);
gboolean DAL_get_almSched_data(EA_AlmSchedData *data, guint ch);
gboolean DAL_set_almSched_data_all(EA_AlmSchedData *data, guint ch);
gboolean DAL_get_nvr_almSched_data(EA_AlmSchedData *data, guint ch);
gboolean DAL_set_nvr_almSched_data_all(EA_AlmSchedData *data, guint ch);


/***************************************************************
 * Setup --> Event&Action --> Event Notification --> Buzzer
 * *************************************************************/
gboolean DAL_set_evtNoti_buzzer_duration(gint duration);
gint DAL_get_evtNoti_buzzer_duration();



/***************************************************************
 * Setup --> Event&Action --> Event Notification --> Display
 * *************************************************************/
typedef struct _EvtActEventNotiDispData		EA_EvtNotiDispData;
struct _EvtActEventNotiDispData {
	guint		vpop_duration;
	guint		vpop_multi;
	gboolean 	vpop_spot;

	guint 		opop_duration;
	gboolean 	opop_spot;
};

gboolean DAL_set_evtNoti_disp_data(EA_EvtNotiDispData data);
gboolean DAL_get_evtNoti_disp_data(EA_EvtNotiDispData *data);
guint DAL_get_evtNoti_vPop_duration();
guint DAL_get_evtNoti_oPop_duration();



/***************************************************************
 * Setup --> Event&Action --> Event Notification --> E-mail
 * *************************************************************/
#define EMAIL_SCHEDULING_MAX_CNT    (10)

typedef struct _EMAIL_SCHED_T           EMAIL_SCHED_T;
struct _EMAIL_SCHED_T {
    guint wday;
    guint from_h;
    guint from_m;
    guint to_h;
    guint to_m;
};

typedef struct _EvtActEventNotiData		EA_EvtNotiEmailData;
struct _EvtActEventNotiData {
	guint count;
	guint frequency;
	gchar address[8][65];
	gboolean include_jpeg;
	guint serv;
	guint from_time;
    guint to_time;
    guint day[7];
    guint al_switch;
    guint al_switch_port;
    EMAIL_SCHED_T sched[EMAIL_SCHEDULING_MAX_CNT];
};

gboolean DAL_set_evtNoti_email_data(EA_EvtNotiEmailData data);
gboolean DAL_get_evtNoti_email_data(EA_EvtNotiEmailData *data);

gboolean DAL_set_evtNoti_email_address(gchar *addr, guint idx);
gboolean DAL_get_evtNoti_email_address(gchar *addr, guint idx);
gboolean DAL_set_evtNoti_email_frequency(guint frequency);
guint DAL_get_evtNoti_email_frequency();



/***************************************************************
 * Setup --> Event&Action --> Event Notification --> FTP
 * *************************************************************/
typedef struct _EvtActFTPNotiData		EA_EvtNotiFTPData;
struct _EvtActFTPNotiData {
	guint	    weblink;
	guint	    freq;
	gboolean    include_jpeg;
	guint	    dir_mode;
	gchar	    dir_path[STRING_SIZE_32];
	guint	    filename_mode;
	gchar	    filename[STRING_SIZE_32];
	gchar	    host[STRING_SIZE_32];
	guint	    port;
	gchar	    username[STRING_SIZE_64];
	gchar	    passwd[STRING_SIZE_16];
	gboolean    include_video;
};

gboolean DAL_set_evtNoti_ftp_data(EA_EvtNotiFTPData data);
gboolean DAL_get_evtNoti_ftp_data(EA_EvtNotiFTPData *data);


/***************************************************************
 * Setup --> Event&Action --> Event Notification --> SMS
 * *************************************************************/
typedef struct _EvtActSMSNotiData		EA_EvtNotiSMSData;
struct _EvtActSMSNotiData {
	gchar		server[STRING_SIZE_64];
	gchar		appid[STRING_SIZE_64];
	gchar		user[STRING_SIZE_64];
	gchar		password[STRING_SIZE_64];
	guint		sched_from;
	guint		sched_to;
	guint		count;
};

gboolean DAL_set_evtNoti_sms_data(EA_EvtNotiSMSData data);
gboolean DAL_get_evtNoti_sms_data(EA_EvtNotiSMSData *data);


/***************************************************************
 * Setup --> Event&Action --> Event Notification --> Mobile Push
 * *************************************************************/
gboolean DAL_set_evtNoti_mobilepush_frequency(gint frequency);
gint DAL_get_evtNoti_mobilepush_frequency();



/***************************************************************
 * Setup --> Event&Action --> Alarm Sensor
 * *************************************************************/
typedef struct _EvtActAlarmSensorData	EA_AlmSenData;
struct _EvtActAlarmSensorData {
	gchar 		name[STRING_SIZE_ALARMIN];
	guchar 		oper;

	guint		linkCam;
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	vpop;
	gboolean 	opop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_almSen_data(EA_AlmSenData data, guint ch);
gboolean DAL_get_almSen_data(EA_AlmSenData *data, guint ch);
gboolean DAL_get_almSen_vPop_boolean(guint ch);
gboolean DAL_get_almSen_oPop_boolean(guint ch);
gboolean DAL_get_almSen_name(guint ch, gchar buf[]);
gboolean DAL_set_almSen_data_all(EA_AlmSenData *data, guint ch);
guint DAL_get_almSen_linkCam(guint ch);


/***************************************************************
 * Setup --> Event&Action --> Motion Sensor
 * *************************************************************/
typedef struct _EvtActMotionSensorData 	EA_MotSenData;
struct _EvtActMotionSensorData {
	gint 		interval;
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	vpop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_MotSen_data(EA_MotSenData data, guint ch);
gboolean DAL_get_MotSen_data(EA_MotSenData *data, guint ch);
gboolean DAL_get_MotSen_vPop_boolean(guint ch);
gboolean DAL_set_MotSen_data_all(EA_MotSenData *data, guint ch);
gint DAL_get_MotSen_ignor_interval(guint ch);

/***************************************************************
 * Setup --> Event&Action --> Video Loss
 * *************************************************************/
typedef struct _EvtActVideoLossData		EA_VLossData;
struct _EvtActVideoLossData {
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_VLoss_Data(EA_VLossData data, guint ch);
gboolean DAL_get_VLoss_Data(EA_VLossData *data, guint ch);
gboolean DAL_set_VLoss_Data_all(EA_VLossData *data, guint ch);


/***************************************************************
 * Setup --> Event&Action --> AI Detection Event
 * *************************************************************/
typedef struct _EvtActDvaData		EA_DvaData;
struct _EvtActDvaData {
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_dva_event_data(EA_DvaData data, guint ch);
gboolean DAL_get_dva_event_data(EA_DvaData *data, guint ch);
gboolean DAL_set_dva_event_data_all(EA_DvaData *data, guint ch);


/***************************************************************
 * Setup --> Event&Action --> POS/ATM
 * *************************************************************/
typedef struct _EvtActPOSData	EA_PosData;
struct _EvtActPOSData {
	gchar 		text[32];

	guint		linkCam;
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	vpop;
	gboolean 	opop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_posevent_data(EA_PosData data, guint ch);
gboolean DAL_set_posevent_data_all(EA_PosData *data, guint ch);
gboolean DAL_get_posevent_data(EA_PosData *data, guint ch);
gboolean DAL_get_posevent_vPop_boolean(guint ch);
gboolean DAL_get_posevent_oPop_boolean(guint ch);
guint DAL_get_posevent_linkCam(guint ch);


/***************************************************************
 * Setup --> Event&Action --> AI Analysis Event
 * *************************************************************/
typedef struct _EvtActAnalysisEvtData 	EA_AnalysisData;
struct _EvtActAnalysisEvtData {
	gint 		interval;
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	vpop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
	gboolean	doubleknock;
};

gboolean DAL_get_analysis_vPop_boolean(guint ch);
gboolean DAL_get_analysis_event_data(EA_AnalysisData *data, guint ch);
gboolean DAL_set_analysis_event_data(EA_AnalysisData data, guint ch);
gboolean DAL_set_analysis_event_data_all(EA_AnalysisData *data, guint ch);
guint DAL_get_double_knock_data();
gboolean DAL_set_double_knock_data(guint double_knock);


/***************************************************************
 * Setup --> Event&Action --> System Event --> Disk
 * *************************************************************/
typedef enum {
	OW_START_EVT_DATA = 0,
	DISK_FULL_EVT_DATA,
	DISK_EXHAUSTED_EVT_DATA,
	SMART_EVT_DATA,
	NO_DIST_EVT_DATA
}SysDiskDataType;

typedef struct _EvtActSystemDiskData	EA_SysDiskData;
struct _EvtActSystemDiskData {
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	opop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_SysDisk_data(EA_SysDiskData data, SysDiskDataType data_type);
gboolean DAL_get_SysDisk_data(EA_SysDiskData *data, SysDiskDataType data_type);
gboolean DAL_get_SysDisk_oPop_boolean(SysDiskDataType data_type);
gboolean DAL_set_SysDisk_data_all(EA_SysDiskData *data);

typedef struct EvtActSystemDiskEvtData	EA_SysDiskEvtData;
struct EvtActSystemDiskEvtData {
	guint 	disk_exhau;
	gint 	nodisk_cnt;
};

gboolean DAL_set_SysDiskEvt_data(EA_SysDiskEvtData data);
gboolean DAL_get_SysDiskEvt_data(EA_SysDiskEvtData *data);


/***************************************************************
 * Setup --> Event&Action --> System Event --> Record
 * *************************************************************/
typedef enum {
	PANIC_START_EVT_DATA = 0,
	RECORD_AUDIT_EVT_DATA
}SysRecDataType;

typedef struct _EvtActSystemRecData	EA_SysRecData;
struct _EvtActSystemRecData {
	guint64 		almOut;

	gboolean 	osd_popup;
	gboolean 	buzzer;
	gboolean 	email;
	gboolean 	ftp;
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_SysRec_Data(EA_SysRecData data, SysRecDataType data_type);
gboolean DAL_get_SysRec_data(EA_SysRecData *data, SysRecDataType data_type);

guint DAL_set_RecAudit_restart(gboolean active);
gboolean DAL_get_RecAudit_restart();

/***************************************************************
 * Setup --> Event&Action --> System Event --> System
 * *************************************************************/
typedef enum {
	BOOTING_EVT_DATA = 0,
	LOGON_FAIL_EVT_DATA,
	FAN_FAIL_EVT_DATA,
	TEMPER_FAIL_EVT_DATA,
	POE_FAIL_EVT_DATA,
	IP_CONFLICT_EVT_DATA

}SysSysDataType;

typedef struct _EvtActSystemSysData	EA_SysSysData;
struct _EvtActSystemSysData {
	guint64	    almOut;
	gboolean    buzzer;
	gboolean    osd_popup;
	gboolean    email;
	gboolean 	ftp;
	gboolean 	mobile;
	gboolean	mobilepush;

	gint 	    failCnt;
	gint 	    failPoe;
};

gboolean DAL_set_SysSys_Data(EA_SysSysData data, SysSysDataType data_type);
gboolean DAL_get_SysSys_Data(EA_SysSysData *data, SysSysDataType data_type);
gboolean DAL_set_SysSys_Data_all(EA_SysSysData *data);
gboolean DAL_get_SysSys_oPop_boolean(SysSysDataType data_type);


/***************************************************************
 * Setup --> Event&Action --> System Event --> Network
 * *************************************************************/
typedef enum {
	INTERNET_TROUBLE_EVT_DATA = 0,
	REMOTE_LOGON_FAIL_EVT_DATA,
	DDNS_UPDATE_FAIL_EVT_DATA,
	IP_CONFLICT_EVENT,
#if defined(_SUPPORT_AIBOX)
	AIBOX_TROUBLE_EVT_DATA,
#endif
	SYS_NET_EVT_CNT
}SysNetDataType;

typedef struct _EvtActSystemNetData	EA_SysNetData;
struct _EvtActSystemNetData {
	guint64 	    almOut;

	gboolean    buzzer;
	gboolean    osd;
	gboolean    email;
	gboolean 	ftp;
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_SysNet_data_all(EA_SysNetData *data);
gboolean DAL_set_SysNet_data(EA_SysNetData data, SysNetDataType data_type);
gboolean DAL_get_SysNet_data(EA_SysNetData *data, SysNetDataType data_type);


typedef struct _EvtActSystemNetFailCnt	EA_SysNetFailCnt;
struct _EvtActSystemNetFailCnt {
	gint rfail_cnt;
	gint dfail_cnt;
};

gboolean DAL_set_SysNet_FCount(EA_SysNetFailCnt data);
gboolean DAL_get_SysNet_FCount(EA_SysNetFailCnt *data);

/***************************************************************
 * Setup --> Event&Action --> Tamper Event
 * *************************************************************/
typedef struct _EvtActTamperData EA_TamperData;
struct _EvtActTamperData {
	gint 		interval;

	guint		linkCam;
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	vpop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean 	mobile;
	gboolean	mobilepush;
};

gint DAL_set_tamper_data(EA_TamperData data, guint ch);
gint DAL_get_tamper_data(EA_TamperData *data, guint ch);
gint DAL_set_tamper_all_data(EA_TamperData *data, guint max_ch);


/***************************************************************
 * Setup --> Event&Action --> VA Event
 * *************************************************************/
typedef struct _EvtActVCAEvtData 	EA_VCAEvtData;
struct _EvtActVCAEvtData {
	gint 		interval;
	guint64 		almOut;

	gboolean 	buzzer;
	gboolean 	vpop;
	gboolean 	email;
	gboolean 	ftp;
	gboolean    preset;
	gint        preset_num[32];
	gboolean 	mobile;
	gboolean	mobilepush;
};

gboolean DAL_set_VCAEvt_data(EA_VCAEvtData data, guint ch);
gboolean DAL_set_VCAEvt_data_all(EA_VCAEvtData *data, guint ch);
gboolean DAL_get_VCAEvt_data(EA_VCAEvtData *data, guint ch);
gboolean DAL_get_VCAEvt_vPop_boolean(guint ch);
gint DAL_get_VCAEvt_ignor_interval(guint ch);



/******************************************************
 * Setup --> System --> Disk Manage
 * ***************************************************/
typedef struct _SysDiskManage_t	 DiskManageData;
struct _SysDiskManage_t {
	guint	timeLimit;
	guint	overWrite;
	guint   timeType;
	gboolean custom_cal;
};

guint DAL_get_diskManage_data(DiskManageData *data);
guint DAL_set_diskManage_data(DiskManageData *data);
guint DAL_get_rtime_limit();
guint DAL_get_disk_write_mode();
guint DAL_set_disk_smart_interval(guint interval);
guint DAL_get_disk_smart_interval();




/*****************************************************************
 * Setup --> Record Setup --> Recording Operations
 * **************************************************************/
typedef struct _SysRecordOper_t	 RecordOperData;
struct _SysRecordOper_t {
	guint	mode;
	guint	autoConfig;
	guint	schedMode;
	guint	preRecTime;
	guint	postRecTime;
	guint	panicTime;
	guint	priMode;
	guint   smart_storage;
};

gboolean DAL_get_recordOper_data(RecordOperData *data);
gboolean DAL_set_recordOper_data(RecordOperData *data);
gboolean DAL_get_recordOper_mode_data(guint *data);
gboolean DAL_set_recordOper_mode_data(RecordOperData *data);

/*****************************************************************/

// Setup --> Record Setup --> continous/motion & alarm recording
#define CAMERA_CHANNEL	MAX_GUI_CHANNEL

enum {
	REC_SUN_PARAM = 0,
	REC_MON_PARAM,
	REC_TUE_PARAM,
	REC_WED_PARAM,
	REC_THU_PARAM,
	REC_FRI_PARAM,
	REC_SAT_PARAM,
	REC_DAILY_PARAM,
	REC_USR_HOL_PARAM,
	REC_WEEKLY_PARAM
};
#define HOURS_A_DAY		24
typedef struct _SysRecordParam_t  RecordParamData;
struct _SysRecordParam_t {
	gchar sizeParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar fpsParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar qualParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar audioParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar modeParam[CAMERA_CHANNEL][HOURS_A_DAY];
}__attribute__((packed));

#if defined(_ATM_1648) || defined(_ATM_1624) || defined(_ATM_0824) ||  \
    defined(_ANF_0824CS) || defined(_ATM_0424) || defined(_ATM_0412) ||  \
    defined(_OTM_1648) || defined(_OTM_0824)
typedef struct _SysAlarmRecordParam_t  AlarmRecordParamData;
struct _SysAlarmRecordParam_t {
	gchar sizeParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar fpsParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar qualParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar audioParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar alarmChParam[CAMERA_CHANNEL][HOURS_A_DAY];
	gchar modeParam[CAMERA_CHANNEL][HOURS_A_DAY];
}__attribute__((packed));
#endif

gboolean DAL_get_recordContParam_data(RecordParamData *data, guint key);
gboolean DAL_set_recordContParam_data(RecordParamData *data, guint key);
gboolean DAL_get_recordMotionParam_data(RecordParamData *data, guint key);
gboolean DAL_set_recordMotionParam_data(RecordParamData *data, guint key);
#if defined(_ATM_1648) || defined(_ATM_1624)    ||  \
    defined(_ATM_0824) || defined(_ANF_0824CS) || defined(_OTM_1648) || defined(_OTM_0824)
gboolean DAL_get_recordAlarmParam_data(AlarmRecordParamData *data, guint key);
gboolean DAL_set_recordAlarmParam_data(AlarmRecordParamData *data, guint key);
#else
gboolean DAL_get_recordAlarmParam_data(RecordParamData *data, guint key);
gboolean DAL_set_recordAlarmParam_data(RecordParamData *data, guint key);
#endif

/*****************************************************************/

// Setup --> Record Setup --> panic recording
typedef struct _SysRecordPanic_t  RecordPanicData;
struct _SysRecordPanic_t {
	gchar sizeParam[CAMERA_CHANNEL];
	gchar fpsParam[CAMERA_CHANNEL];
	gchar qualParam[CAMERA_CHANNEL];
	gchar audioParam[CAMERA_CHANNEL];
};

gboolean DAL_get_recordPanic_data(RecordPanicData *data);
gboolean DAL_set_recordPanic_data(RecordPanicData *data);

/*****************************************************************/

// Setup --> Record Setup --> Network Streaming

#if defined(_OTM_1648) || defined(_OTM_0824) || defined(_SNF_MODEL)
typedef struct _SysSetupNetStream_t  SetupNetStreamData;
struct _SysSetupNetStream_t {
	gchar sizeParam[CAMERA_CHANNEL];
	gchar fpsParam[CAMERA_CHANNEL];
	gchar qualParam[CAMERA_CHANNEL];
	gboolean enableStreamControl;
};

gboolean DAL_get_setupNetStream_data(SetupNetStreamData *data);
gboolean DAL_set_setupNetStream_data(SetupNetStreamData *data);
#endif




/*****************************************************************/

// Setup --> Record Setup --> auto

typedef struct _SysRecordAuto_t  RecordAutoData;
struct _SysRecordAuto_t {
	gchar sizeParam[CAMERA_CHANNEL];
	gchar fpsParam[CAMERA_CHANNEL];
	gchar qualParam[CAMERA_CHANNEL];
	gchar audioParam[CAMERA_CHANNEL];
};

typedef struct _SysRecordAutoIntensive_t  RecordAutoIntensiveData;
struct _SysRecordAutoIntensive_t {
	gchar normal_sizeParam[CAMERA_CHANNEL];
	gchar normal_fpsParam[CAMERA_CHANNEL];
	gchar normal_qualParam[CAMERA_CHANNEL];
	gchar event_sizeParam[CAMERA_CHANNEL];
	gchar event_fpsParam[CAMERA_CHANNEL];
	gchar event_qualParam[CAMERA_CHANNEL];
	gchar audioParam[CAMERA_CHANNEL];
};


// Setup --> Record Setup --> auto --> motion
gboolean DAL_get_recordAuto_Motion_data(RecordAutoData *data);
gboolean DAL_set_recordAuto_Motion_data(RecordAutoData *data);


// Setup --> Record Setup --> auto --> alarm
gboolean DAL_get_recordAuto_Alarm_data(RecordAutoData *data);
gboolean DAL_set_recordAuto_Alarm_data(RecordAutoData *data);


// Setup --> Record Setup --> auto --> motion_alarm
gboolean DAL_get_recordAuto_MotionAlarm_data(RecordAutoData *data);
gboolean DAL_set_recordAuto_MotionAlarm_data(RecordAutoData *data);


// Setup --> Record Setup --> auto --> intensive motion
gboolean DAL_get_recordAuto_intensive_Motion_data(RecordAutoIntensiveData *data);
gboolean DAL_set_recordAuto_intensive_Motion_data(RecordAutoIntensiveData *data);


// Setup --> Record Setup --> auto --> intensive alarm
gboolean DAL_get_recordAuto_intensive_Alarm_data(RecordAutoIntensiveData *data);
gboolean DAL_set_recordAuto_intensive_Alarm_data(RecordAutoIntensiveData *data);


// Setup --> Record Setup --> auto --> intensive motion_alarm
gboolean DAL_get_recordAuto_intensive_MotionAlarm_data(RecordAutoIntensiveData *data);
gboolean DAL_set_recordAuto_intensive_MotionAlarm_data(RecordAutoIntensiveData *data);

/*****************************************************************/


/******************************************************
 *
 *  IPCAM Recording Setup
 *
 *****************************************************/

typedef struct _IPCamRecOper_t	 IPCamRecOperData;
struct _IPCamRecOper_t {
	guint	schedMode;
	guint	preRecTime;
	guint	postRecTime;
};

gboolean DAL_get_IPCamRecOper_data(IPCamRecOperData *data);
gboolean DAL_set_IPCamRecOper_data(IPCamRecOperData *data);

/*****************************************************************/

// Setup --> Record Setup --> continous/motion & alarm recording
#define IPCAM_CHANNEL	4
typedef struct _IPCamRecParam_t  IPCamRecParamData;
struct _IPCamRecParam_t {
	gchar sizeParam[IPCAM_CHANNEL][HOURS_A_DAY];
	gchar fpsParam[IPCAM_CHANNEL][HOURS_A_DAY];
	gchar qualParam[IPCAM_CHANNEL][HOURS_A_DAY];
	gchar audioParam[IPCAM_CHANNEL][HOURS_A_DAY];
	gchar modeParam[IPCAM_CHANNEL][HOURS_A_DAY];
}__attribute__((packed));

gboolean DAL_get_IPCamRecMotParam_data(IPCamRecParamData *data, guint key);
gboolean DAL_set_IPCamRecMotParam_data(IPCamRecParamData *data, guint key);
gboolean DAL_get_IPCamRecAlarmParam_data(IPCamRecParamData *data, guint key);
gboolean DAL_set_IPCamRecAlarmParam_data(IPCamRecParamData *data, guint key);

/*****************************************************************/

// Setup --> Record Setup --> panic recording
typedef struct _IPCamRecPanic_t  IPCamRecPanicData;
struct _IPCamRecPanic_t {
	gchar sizeParam[CAMERA_CHANNEL];
	gchar fpsParam[CAMERA_CHANNEL];
	gchar qualParam[CAMERA_CHANNEL];
	gchar audioParam[CAMERA_CHANNEL];
};

gboolean DAL_get_IPCamRecPanic_data(IPCamRecPanicData *data);
gboolean DAL_set_IPCamRecPanic_data(IPCamRecPanicData *data);





/*****************************************************************/

// Archiving --> Schedule Archiving Setup
//#if defined(__DIGI_SUPPORT__) || defined(LOREX)
typedef struct _ScheduleArch_t	ScheduleArchData;
struct _ScheduleArch_t {
	guint	operation;
	gchar	device[STRING_SIZE_32];
	guint	mode;
	guint	starttime;
	guint	from;
	guint	to;
	gchar	camera[CAMERA_CHANNEL];
	gchar	audio[CAMERA_CHANNEL];
};


void init_testScheduleArchData(void);

guint DAL_get_schedule_arch_data(ScheduleArchData *data);
guint DAL_set_schedule_arch_data(ScheduleArchData *data);

//#endif  /*__DIGI_SUPPORT__*/

/******************************************************************/

//#if defined(LOREX)
typedef struct _ArchFtp_t	ArchFtpData;
struct _ArchFtp_t {
	gchar		host[STRING_SIZE_32];
	guint		port;
	gboolean	is_anon;
	gchar		username[STRING_SIZE_64];
	gchar		passwd[STRING_SIZE_16];
	gchar		dir_path[STRING_SIZE_64];
	guint		period;
};

guint DAL_set_archftp_data(ArchFtpData *data);
guint DAL_get_archftp_data(ArchFtpData *data);
//#endif

/*************************************************
 * ETC FUNCTION PROTOTYPE
 * **********************************************/

/* tools/nf_ui_tool.h
typedef enum {
	NFSETUP_WINDOW_CAMERA = 0,
	NFSETUP_WINDOW_DISPLAY,
	NFSETUP_WINDOW_SOUND,
	NFSETUP_WINDOW_SYSTEM,
	NFSETUP_WINDOW_USER,
	NFSETUP_WINDOW_NETWORK,
	NFSETUP_WINDOW_EVENT,
	NFSETUP_WINDOW_DISK,

	NFSETUP_WINDOW_SEARCH,
	NFSETUP_WINDOW_RECORDING,
	NFSETUP_WINDOW_ARCHIVING,

	NFSETUP_WINDOW_END
} nfsetup_window_type;
*/

/** dal init*/
void DAL_init(int ch_count);

guint DAL_notify_fire_DB_change(guint category);
guint DAL_notify_fire_DB_change2(guint category, guint chmask, gint sub_category);
guint DAL_notify_fire_DB_sync(guint category, guint channel);
guint DAL_notify_fire_ipcam_change(guint category, guint chmask);

gboolean DAL_save_db(const gchar *cat);
void DAL_save_setup_db(guint wtype);	// wtype : nfsetup_window_type
void DAL_save_db_all();

gboolean DAL_factory_default();
gboolean DAL_factory_default_without_network();
gboolean DAL_system_data_save(const gchar *filename);
gboolean DAL_system_data_load(const gchar *filename);
gboolean DAL_system_data_load_without_network(const gchar *filename);

gboolean DAL_get_passwd_enable();


/*******************************************************
 * WIZARD
 * ****************************************************/


typedef struct _WizardCheck_t WizardCheck;
struct _WizardCheck_t {
	guint usable_defpw;
	gchar	pw[STRING_SIZE_16+1];
	gboolean netwiz;
};

typedef struct _DATATIME_DATA_T {
    guint	dateFormat;
	guint	timeFormat;
	guint	timeZone;
	guint	dst;
} DATATIME_DATA_T;

typedef struct _RECORD_DATA_T {
    guint mode;
    guint autoConfig;
} RECORD_DATA_T;

guint DAL_get_Netwizard_func(int *data);
guint DAL_get_Langwizard_func(int *data);
guint DAL_set_Langwizard_func(int data);
guint DAL_get_WizardCheck_Data(WizardCheck *data, guint idx);
guint DAL_set_WizardCheck_Data(gchar *key, gboolean data);
guint DAL_set_wizard_datetime_data(DATATIME_DATA_T *data);
gboolean DAL_get_wizard_record_data(RECORD_DATA_T *data);
gboolean DAL_set_wizard_record_data(RECORD_DATA_T *data);


/*****************************************************************/

/*************************************************
 * SEQURINET
 * **********************************************/

void DAL_get_QR_Code_URL(gchar *url);
gint DAL_get_Sequrinet_Enable(gboolean *enable);
guint DAL_set_Sequrinet_Status(gboolean data);
gint DAL_get_Sequrinet_Status(gboolean *status);

/*************************************************
 * UNIMO CHECKER
 * **********************************************/
typedef struct _SysNetUnimo_t	 UNIMOData;
struct _SysNetUnimo_t {
	guint	enable;
	gchar	server[STRING_SIZE_64];
	guint	port;
};
guint DAL_set_Unimo_Data(UNIMOData *data);
gint DAL_get_Unimo_Data(UNIMOData *data);

/************************************************/



/*
	* ip cam?Ç key¸¦ ?Ô·ÂÇÏ¸é Çö?ç ÇÒ´çµÈ Ã¤³Î?» µ¹·ÁÁÖ´Â ÇÔ¼ö ÇÊ¿ä

	* ¸ðµç µ¥?ÌÅÍ?Ç Á¤?Ç´Â DAL?Ì °¡Áö°í ?Ö¾î¾ßÇÔ.
		¿¹1) Baud rate?Ç °¡Áö¼ö´Â dal¸¸ ¾Ë°í ?Ö?¸¸ç ÇÔ¼ö¸¦ ÅëÇØ °¹¼ö¿Í Á¾·ù¸¦ ¹ÝÈ¯ÇÒ ¼ö ?Ö¾î¾ß ÇÑ´Ù.
				gchar *DAL_get_kinds_of_baud();
				guint DAL_get_num_of_bauds();
		¿¹2) protocol?Ç Á¾·ù´Â dal¸¸?Ì ¾Ë°í ?Ö?¸¸ç ÇÔ¼ö¸¦ ÅëÇØ °¹¼ö¿Í Á¾·ù¸¦ ¹ÝÈ¯ ÇÒ ¼ö ?Ö¾î¾ß ÇÑ´Ù.
				gchar *DAL_get_kinds_of_protocol();
				guint DAL_get_num_of_protocol();
*/

/*************************************************
 * USER DEFINED DST
 * **********************************************/
typedef struct _UDD_DATA_T {
    gint use;
    gchar dst_name[8];
    gchar std_name[8];
    gint offset;
    guint start;
    guint end;
}UDD_DATA_T;


guint DAL_get_user_defined_dst_data(UDD_DATA_T *data);
guint DAL_set_user_defined_dst_data(UDD_DATA_T *data);


/************************************************/

#if defined(__DIGI_SUPPORT__)
guint DAL_get_autoSync_interval_min();
#endif  /*__DIGI_SUPPORT__*/
#endif
