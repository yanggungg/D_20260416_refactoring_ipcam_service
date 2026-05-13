#ifndef __NVS_ONVIF_APP_UTIL_H__
#define __NVS_ONVIF_APP_UTIL_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <syslog.h>
#include <math.h>

#include "onvif_common.h"
#include "nf_common.h"
#include "nf_util_time.h"
#include "nf_util_netif.h"
//#include "nf_webra.h"
#include "nf_api_ipcam.h"
//#include "nf_webra_def.h"
#include "nf_timer.h"
#include "nf_ptz.h"
#include "nf_sysman.h"
#include "nfdal.h"
#include <regex.h>
#include <sys/types.h>
#include "nvs_onvif_app.h"
#include "nf_nvs_cgi_ipc.h"
#if 1
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif

#define MAX_RESOLUTION (11)
#define PTZ_NORMALIZE_VALUE (100)
#define START_RESOLUTION_2M (0)
#define NUM_OF_RESOUTION_ALL    (12)
#define NUM_OF_RESOUTION    (11)
#define MAX_PRESET_NUM 255

//for utm2
#define AVAIL_1ST_RESOL "aDCB"
#define AVAIL_2ND_RESOL "B"
#define QUAL_DATA_MAX	5
#define QUAL_DATA_HIGHEST       "HIGHEST"
#define QUAL_DATA_HIGH	        "HIGH"
#define QUAL_DATA_STANDARD	    "STANDARD"
#define QUAL_DATA_LOW   	    "LOW"
#define QUAL_DATA_LOWEST	    "LOWEST"

typedef struct _ONVIF_USR_AUTH_T
{
  gboolean    setupman;
  gboolean    searchman;
  gboolean    archman;
  gboolean    recsetman;
  gboolean    eventman;
  gboolean    audioman;
  gboolean    micman;
  gboolean    remoteman;
  gboolean    shutman;

  gboolean    setupusr;
  gboolean    searchusr;
  gboolean    archusr;
  gboolean    recsetusr;
  gboolean    eventusr;
  gboolean    audiousr;
  gboolean    micusr;
  gboolean    remoteusr;
  gboolean    shutusr;
} ONVIF_USR_AUTH;

typedef struct _ONVIF_USR_MAN_T
{
  int     flag;

  char    usrid[33];
  char    reserved0[3];

  char    passwd[33];
  char    reserved1[3];

  char    email[65];
  char    reserved2[3];

  char    email_notify;

  unsigned int  email_serv;

  char    phone[65];
  char    reserved3[3];

  char    phone_notify;

  //char    desc[64];
  //char    reserved3[4];

  char    grpname[33];
  char    reserved4[3];

  char    covert[17];

  unsigned int    pw_last_changed;
  unsigned int    expired_check;

  unsigned int    init_pw_changed;

  unsigned char   usrcnt;
} ONVIF_USR_MAN;

typedef struct _ONVIF_IPCAM_INFORMATION_OPTION_ONVIF_T {
  guint category;
  guint category_high;
  guint value;
  guint selected; // 0: false, 1: true
  guint dependent_category;
  guint dependent_category_high;
  guint enable_category;
  guint enable_category_high;
  guint disable_category;
  guint disable_category_high;
  guint visible_category;
  guint visible_category_high;
  guint invisible_category;
  guint invisible_category_high;
  gchar caption[32];
} ONVIF_IPCAM_INFORMATION_OPTION_ONVIF;

typedef struct _ONVIF_IPCAM_INFORMATION_VALUE_T {
  guint category;
  guint category_high;
  gint value;
  gint min;
  gint max;
  guint dependent_category;
  guint dependent_category_high;
  gint difference;
} ONVIF_IPCAM_INFORMATION_VALUE;

typedef struct _ONVIF_CAM_COMPATIBILITY_EXPOSURE_PROFILE_T {
  char ch;

  guint supported_exposure;
  guint supported_exposure_high;

  gint mode_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF mode[10];

  gint priority_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF priority[10];

  gint blc_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF blc[10];
  ONVIF_IPCAM_INFORMATION_VALUE blclevel;

  ONVIF_IPCAM_INFORMATION_VALUE minetime;
  ONVIF_IPCAM_INFORMATION_VALUE maxetime;
  ONVIF_IPCAM_INFORMATION_VALUE etime;
  gint slowshutter_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF slowshutter[10];

  ONVIF_IPCAM_INFORMATION_VALUE mingain;
  ONVIF_IPCAM_INFORMATION_VALUE maxgain;
  ONVIF_IPCAM_INFORMATION_VALUE gain;
  gint maxagc_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF maxagc[10];

  ONVIF_IPCAM_INFORMATION_VALUE miniris;
  ONVIF_IPCAM_INFORMATION_VALUE maxiris;
  ONVIF_IPCAM_INFORMATION_VALUE iris;
  gint dc_iris_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF dc_iris[10];

  gint wdr_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF wdr[10];
  ONVIF_IPCAM_INFORMATION_VALUE wdrlevel;

  gint dnr_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF dnr[10];

  ONVIF_IPCAM_INFORMATION_VALUE bottom;
  ONVIF_IPCAM_INFORMATION_VALUE top;
  ONVIF_IPCAM_INFORMATION_VALUE right;
  ONVIF_IPCAM_INFORMATION_VALUE left;

  gint antiflicker_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF antiflicker[10];
  gint antiflicker_motion_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF antiflicker_motion[10];


  gint max_shutter_50_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF max_shutter_50[10];
  gint max_shutter_60_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF max_shutter_60[10];
  gint max_shutter_off_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF max_shutter_off[10];
  gint max_shutter_motion_off_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF max_shutter_motion_off[10];

  gint ircut_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF ircut[10];

  gint ircutm_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF ircutm[10];

  gint dnn_toggle_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF dnn_toggle[10];

  gint adaptive_ir_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF adaptive_ir[10];

  ONVIF_IPCAM_INFORMATION_VALUE dnn_sense_dton;
  ONVIF_IPCAM_INFORMATION_VALUE dnn_sense_ntod;

  gint defog_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF defog[10];

  gint hlc_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF hlc[10];
} ONVIF_CAM_COMPATIBILITY_EXPOSURE_PROFILE;

typedef struct _ONVIF_CAM_COMPATIBILITY_IMAGE_PROFILE_T {
  char ch;
  char model[64];
  guint onvif_support;
  guint supported_image;
  guint supported_image_high;

  ONVIF_IPCAM_INFORMATION_VALUE brightness;
  ONVIF_IPCAM_INFORMATION_VALUE contrast;
  ONVIF_IPCAM_INFORMATION_VALUE sharpness;
  ONVIF_IPCAM_INFORMATION_VALUE color;
  ONVIF_IPCAM_INFORMATION_VALUE tint;

  gint mirror_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF mirror[10];

  gint focus_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF focus[10];
  ONVIF_IPCAM_INFORMATION_VALUE defaultspeed;
  ONVIF_IPCAM_INFORMATION_VALUE nearlimit;
  ONVIF_IPCAM_INFORMATION_VALUE farlimit;

  ONVIF_IPCAM_INFORMATION_VALUE abposition;
  ONVIF_IPCAM_INFORMATION_VALUE abspeed;
  ONVIF_IPCAM_INFORMATION_VALUE redistance;
  ONVIF_IPCAM_INFORMATION_VALUE respeed;
  ONVIF_IPCAM_INFORMATION_VALUE cospeed;

  gint wb_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF wb[10];
  ONVIF_IPCAM_INFORMATION_VALUE crgain;
  ONVIF_IPCAM_INFORMATION_VALUE cbgain;

  gint mwb_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF mwb[5];

  gint focus_limit_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF focus_limit[10];

  gint stabilizer_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF stabilizer[10];

  gint ir_correction_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF ir_correction[10];

  gint dnn_toggle_cnt;
  ONVIF_IPCAM_INFORMATION_OPTION_ONVIF dnn_toggle[10];
} ONVIF_CAM_COMPATIBILITY_IMAGE_PROFILE;

typedef struct _ONVIF_SEARCH_LOG_DST_T
{
  gint dston[ONVIF_MAX_LOG_CNT];

} ONVIF_SEARCH_LOG_DST;

typedef struct _ONVIF_LIVE_STATUS_T {

  guchar act_alarm[17];
  guchar act_alarm_dvr[17];
  guchar act_motion[17];
  guchar act_vloss[17];
  guchar act_novideo[17];
  guchar act_vid_valid[17];
  guchar act_recording[17];
  guchar act_alarm_out[17];
  guchar act_alarm_out_dvr[17];
  guchar rise_alarm[17];
  guchar rise_alarm_dvr[17];
  guchar rise_motion[17];
  guchar rise_vloss[17];
  guchar rise_novideo[17];
  guchar rise_vid_valid[17];
  guchar rise_recording[17];
  guchar rise_alarm_out[17];
  guchar rise_alarm_out_dvr[17];
  guint  netaudiotrans;

  guchar op_type[17]; //This string is display setting of relay type. (N/O or N/C).

  gulong record_fps;
  gulong net_client;
  gulong disk_used;
  gulong disk_total;
  gulong disk_full;
  gulong disk_smart;
  gulong fs_online;
  gulong dvr_status;
  gulong sysdb_cam_title;
  gulong sysdb_covert;
  gulong sysdb_ptz;
  gulong sysdb_tformat;
  gulong sysdb_tzone;

  gulong curr_gmttime;

} ONVIF_LIVE_STATUS;

typedef struct _func_interval {
    void * func;
    unsigned int data;
} func_interval;

typedef struct _PathExpr
{
    int _not;
    int _size;
    char *ex_path_str;
} PathExpr;

typedef struct _BoolExpr
{
    int _not;
    PathExpr *ex_path;
} BoolExpr;

typedef struct _MSG_FilterExpr
{
    int _not;
    BoolExpr *ex_boolean;

    struct _MSG_FilterExpr *p_next_and;
    struct _MSG_FilterExpr *p_next_or;
    struct _MSG_FilterExpr *p_prev;
    struct _MSG_FilterExpr *p_parent, *p_child;
} MSG_FilterExpr;

enum ONVIF_VA_TYPE{
	ONVIF_CLASSIC_VA_ZONE = 0,
	ONVIF_CLASSIC_VA_COUNTER,
	ONVIF_AI_ZONE,
	ONVIF_AI_COUNTER,
	ONVIF_BUILT_IN_VA,
};

enum ONVIF_CAM_CONNECT_STATE{
	ONVIF_CAM_DISCONNECTED=0,
	ONVIF_CAM_CONNECTED,
};

enum ONVIF_ALARM_IN_OUT{
	ONVIF_ALARM_IN_SUPPORTED=0,
	ONVIF_ALARM_OUT_SUPPORTED,
};

enum ONVIF_AUDIO_IN_OUT{
	ONVIF_AUDIO_IN_SUPPORTED=0,
	ONVIF_AUDIO_OUT_SUPPORTED,
};

enum ONVIF_TRACK_TYPE{
	ONVIF_MULTICAST_VIDEO = 0,
	ONVIF_MULTICAST_AUDIO = 1,
	ONVIF_MULTICAST_META  = 2
};

enum ONVIF_H264_PROFILE {
	ONVIF_H264_PROFILE_BASELINE = 0,
	ONVIF_H264_PROFILE_MAIN = 1,
	ONVIF_H264_PROFILE_EXTENDED = 2,
	ONVIF_H264_PROFILE_HIGH = 3,
};


enum PTZ_OPERATION_ENUM {
	CONTINUOUS, LIMIT
};

enum __ONVIF_VLC_MODE_
{
	ONVIF_VLC_MODE_OFF,
	ONVIF_VLC_MODE_ON,
};
enum __ONVIF_IRCUT_FILTER_MODE_
{
	ONVIF_IRCUT_FILTER_MODE_ON,
	ONVIF_IRCUT_FILTER_MODE_OFF,
	ONVIF_IRCUT_FILTER_MODE_AUTO,
};
enum __ONVIF_EXPOSURE_MODE_
{
	ONVIF_EXPOSURE_MODE_AUTO,
	ONVIF_EXPOSURE_MODE_MANUAL,
};
enum __ONVIF_AUTO_FOCUS_MODE_
{
	ONVIF_AUTO_FOCUS_MODE_AUTO,
	ONVIF_AUTO_FOCUS_MODE_MANUAL,
};

enum __ONVIF_CODEC_MODE_ {
	ONVIF_JPEG_CODEC, ONVIF_MPEG_CODEC, ONVIF_H264_CODEC,  NUM_OF_CODECS
};

extern const char	*_sysdb_cate_list[];
extern arg_fps fps_nt[MAX_FPS];
extern arg_fps fps_pal[MAX_FPS];
extern arg_resolution ipx_resolution[];
extern arg_2nd_resolution avail_2nd_resolution[];

int multi_hello(void);
void multi_bye(void);
void onvif_discovery_start(void);
void onvif_discovery_end(void);
int isUseAudio(void);
gboolean onvif_SetDbUint(char *_dbName, int _ch, gint _data);
unsigned int get_subnet_prefix();
unsigned int get_subnet_prefix_from_subnet(unsigned int subnet);
unsigned int set_subnet_prefix(unsigned int prefix);
int load_current_net_conf(IPSetupData *ipdata);
int convert_prefix_to_netmask(unsigned int prefix, guint *netmask);
int get_ipaddress(char *tmp_ip);
int getResolutionIndex(char char_tmp);
void get_video_size(char char_tmp, int *width, int *height);
void set_vencoder_table(arg_VideoEncoder *tmp);
void get_vencoder_table(arg_VideoEncoder *tmp);
int get_vencoder_usecount(char *token);
int get_ptz_usecount(char *token);
void getVEncoderData(int profile_cnt, arg_VideoEncoder* encoder);
int is_EnableAudio(int index);
void get_streamuri(arg_StreamUri *tmp, const char *vencoder);
int getEncoding(int i);
int get_vsource_usecount(char *token);
float get_fps_from_index(unsigned int req_fps);
int get_aencoder_usecount(char *token);
int get_asource_usecount(char *token);
int get_metadata_usecount(char *token);
unsigned int load_current_usrman(ONVIF_USR_MAN* tmp);
unsigned int save_current_usrman(ONVIF_USR_MAN* man);
int convert_onvif_to_group(char *dest, char *onvif_grp);
int convert_group_to_onvif(char *dest, char *src);
#ifdef ONVIF_MODEL_IPX
void getExposureImageProfile(int ch,   ONVIF_CAM_COMPATIBILITY_EXPOSURE_PROFILE* profile);
 void getImageProfile(int ch, ONVIF_CAM_COMPATIBILITY_IMAGE_PROFILE *profile);
 int getOnvifExposureMode(int itxMode);
  int getOnvifIrcutfilterMode(int itxMode);
 int getIrcutfilterMode(int onvifMode);
 int getOnvifBlcMode(int itxMode);
 int getBlcMode(int onvifMode);
 int getExposureMode(int onvifMode);
#endif
 void setImageSettings(int ch, arg_ImagingOption* tmp);
int getOnvifAutoFocusValue(int val);
int adjustValue(float val);
int getPtzZoomCmd(arg_PTZOperation *tmp, int* outInterval);
int getPtzPanTiltCmd(arg_PTZOperation *tmp, int* outInterval);
void PTZOperationStop(int ch, int zoom_stop, int pantilt_stop);
void pushPTZControlOperation(NF_PTZ_CMD *ptz_cmd, int operation);
int get_fps_from_alphabet(char ch);
int get_size_string_from_rect(char *size, int ch, int width, int height);
int get_h265_options(arg_voption *voption, char *capa_string, int ch, int stream);
int get_h264_options(arg_voption *voption, char *capa_string, int ch, int stream);
int get_jpeg_options(arg_voption *voption, char *capa_string, int ch, int stream);
int findfieldStringCount(char* field,char* token,int length);
void prvIntToIP(unsigned int *out, guint in);
unsigned int prvIPToInt(unsigned int ip[4]);
unsigned int ip_str_to_uint(const char *temp);
int multi_hello(void);
gboolean nf_debug_hexdump_tty(gpointer p, int len);
gboolean get_dhcpon_ipaddr(void);
gboolean get_dhcpon_host(void);
gboolean get_dhcpon_dns(void);
gboolean get_dhcpon_ntp(void);
void set_dhcp(void);
int set_dhcp_and_get_dns(unsigned int *d1, unsigned int *d2);
int set_dhcp_and_get_hostname(char *host);
unsigned int dvrReady_info(void);
char *get_onvif_model_name();
int get_ipaddress_p(char *tmp_ip, unsigned int size);
int regExp(char* string, regmatch_t* mt,char* spliter );
void onvif_get_width_height(guint64 resol, int* w, int* h);
void get_cam_capa_resol(int ch, char *capa_string, int stream);
gchar onvif_get_qual_data(gchar *info);
gchar onvif_get_fps_data(gint ch, gchar *info);
gchar onvif_get_size_data(gchar *info);
gchar* onvif_get_size_info(gchar data);
void makeConfiguration(arg_PTZConfig *tmp, int ch);
gboolean onvif_camera_connect_check(int ch);
gboolean onvif_is_ptz(int ch, uint64_t category);
gboolean onvif_is_audio(int ch, int type);
void onvif_cam_connect_db_save(int ch);
void onvif_cam_disconnect_db_remove(int ch);
void onvif_db_sync(int ch, int type);
int nf_onvif_find_rule_id(int type, int ch, int id);
int onvif_audio_capability();
char* next_find_char(char *p, const char compare, char *p_end);

#endif

