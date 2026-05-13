
#ifndef _VWND_H_
#define _VWND_H_


#include "nf_afx.h"
#include "vsm.h"
#include "dit.h"
#include "pos.h"

#define VWND_TEXT_LEN 63

#define STRING_NONE			        ""
#define STRING_NO_VIDEO			    "NO VIDEO"
#define STRING_VLOSS			    "VIDEO LOSS"
#define STRING_NO_REC		        "NO RECORD"
#define STRING_END_VIDEO		    "END VIDEO"
#define STRING_COVERT		        "COVERT"
#define STRING_OVERLAPPED		    "OVERLAPPED"
#define STRING_UNKNOWN_DEV          "UNKNOWN DEVICE"
#define STRING_UNSUPPORT_CAM        "UNSUPPORTED CAMERA"
#define STRING_FAIL_CONNECT         "CONNECTION FAIL"
#define STRING_FAIL_LOGIN           "LOGIN FAIL"
#define STRING_FAIL_CONFIG          "CONFIGURATION FAIL"

#define VWND_POS_LINE               32
#define VWND_DEBUG_LINE             8

#define VWND_VCA_MAX_CNT			16
#define VWND_DVABX_MAX_CNT			16

typedef enum _RECORD_ICON {
	REC_ICON_NONE	= 0,
	REC_ICON_PRE,
	REC_ICON_CONT,
	REC_ICON_ALARM,
	REC_ICON_MOT,
	REC_ICON_PANIC
} RECORD_ICON;

typedef enum _VIDEO_ST {
	VST_NONE		= 0,
	VST_NO_VIDEO,
	VST_VLOSS,
	VST_NO_REC,
	VST_END_VIDEO,
	VST_COVERT,
	VST_OVERLAPPED,
	VST_UNKNOWN_DEV,
	VST_UNSUPPORT_CAM,
	VST_FAIL_CONNECT,
	VST_FAIL_LOGIN,
	VST_FAIL_CONFIG
} VIDEO_ST;

typedef enum _PND_ERROR {
	PERR_NONE		= 0,
	PERR_UNKNOWN,
	PERR_UNSUPPORT,
	PERR_CONNECT_FAIL,
	PERR_LOGIN_FAIL,
	PERR_CONFIG_FAIL
} PND_ERROR;

typedef enum _ALARM_ST {
	AST_NONE		= 0,
	AST_OFF,
	AST_ON
} ALARM_ST;

typedef enum _NET_ST {
	NST_NONE		= 0,
	NST_ERROR,
	NST_CONFLICT,
	NST_NORMAL
} NET_ST;

typedef enum _DISK_ST {
	DST_NONE		= 0,
	DST_NORMAL,
	DST_NEEDCHECK,
	DST_FULL,
	DST_OW,
	DST_SMT_ERR,
	DST_NO
} DISK_ST;

typedef enum _RAID_ST {
    RST_NORMAL = 0,
    RST_DEGRADE,
    RST_REBUILD,
    RST_BROKEN
}RAID_ST;

typedef enum _ARCH_ST {
	ARST_NONE		= 0,
	ARST_ON
} ARCH_ST;

typedef enum _FILL_ST {
	FILL_ON		= 0,
	FILL_OFF		= 1,
} FILL_ST;

typedef enum _DLVA_CNTR_ICON {
	DLVA_CNTR_ICON_NONE			= 0,
	DLVA_CNTR_ICON_HUMAN,
	DLVA_CNTR_ICON_VEHICLE,
	DLVA_CNTR_ICON_ANIMAL,

	DLVA_CNTR_ICON_NUM
} DLVA_CNTR_ICON;

typedef enum _ITX_VCA_ICON {
	ITX_VCA_ICON_NONE			= 0,
	ITX_VCA_ICON_DIR_NEG_N,
	ITX_VCA_ICON_DIR_NEG_E,
	ITX_VCA_ICON_DIR_POS_N,
	ITX_VCA_ICON_DIR_POS_E,
	ITX_VCA_ICON_ENTER_N,
	ITX_VCA_ICON_ENTER_E,
	ITX_VCA_ICON_EXIT_N,
	ITX_VCA_ICON_EXIT_E,
	ITX_VCA_ICON_STOP_N,
	ITX_VCA_ICON_STOP_E,
	ITX_VCA_ICON_REMOVE_N,
	ITX_VCA_ICON_REMOVE_E,
	ITX_VCA_ICON_LOITER_N,
	ITX_VCA_ICON_LOITER_E,
	ITX_VCA_ICON_FALL_N,
	ITX_VCA_ICON_FALL_E,
	ITX_VCA_ICON_COUNT_N,
	ITX_VCA_ICON_COUNT_E,

	ITX_VCA_ICON_NUM
} ITX_VCA_ICON;

typedef enum _ITX_DVABX_ICON {
	ITX_DVABX_ICON_NONE			= 0,
	ITX_DVABX_ICON_DIR_NEG_N,
	ITX_DVABX_ICON_DIR_NEG_E,
	ITX_DVABX_ICON_DIR_POS_N,
	ITX_DVABX_ICON_DIR_POS_E,
	ITX_DVABX_ICON_ENTER_N,
	ITX_DVABX_ICON_ENTER_E,
	ITX_DVABX_ICON_EXIT_N,
	ITX_DVABX_ICON_EXIT_E,
	ITX_DVABX_ICON_STOP_N,
	ITX_DVABX_ICON_STOP_E,
	ITX_DVABX_ICON_REMOVE_N,
	ITX_DVABX_ICON_REMOVE_E,
	ITX_DVABX_ICON_LOITER_N,
	ITX_DVABX_ICON_LOITER_E,
	ITX_DVABX_ICON_FALL_N,
	ITX_DVABX_ICON_FALL_E,
	ITX_DVABX_ICON_COUNT_N,
	ITX_DVABX_ICON_COUNT_E,
	ITX_DVABX_ICON_INTRUSION_N,
	ITX_DVABX_ICON_INTRUSION_E,	
	ITX_DVABX_ICON_GENERIC_N,
	ITX_DVABX_ICON_GENERIC_E,	

	ITX_DVABX_ICON_NUM
} ITX_DVABX_ICON;

typedef enum _S1_VCA_ICON {
	S1_VCA_ICON_NONE			= 0,
	S1_VCA_ICON_INVASION_N,
	S1_VCA_ICON_INVASION_E,
	S1_VCA_ICON_LOITERING_N,
	S1_VCA_ICON_LOITERING_E,
	S1_VCA_ICON_ABANDON_N,
	S1_VCA_ICON_ABANDON_E,
	S1_VCA_ICON_STEAL_N,
	S1_VCA_ICON_STEAL_E,
	S1_VCA_ICON_TOPPLE_N,
	S1_VCA_ICON_TOPPLE_E,
	S1_VCA_ICON_FENCE_N,
	S1_VCA_ICON_FENCE_E,
	S1_VCA_ICON_COUNT_N,
	S1_VCA_ICON_COUNT_E,
	S1_VCA_ICON_TAMPERING_N,
	S1_VCA_ICON_TAMPERING_E,
	S1_VCA_ICON_PRIVACY_N,
	S1_VCA_ICON_PRIVACY_E,
} S1_VCA_ICON;

typedef struct _VIDEO_IMAGE_T {
    GdkRectangle    box;
    gchar           title[64];
    GdkPixbuf       *pbuf;
} VIDEO_IMAGE_T;

typedef struct _WIN_INFO_T {
	gboolean 		is_focus;

	gchar 			title_text[VWND_TEXT_LEN+1];
	GdkRectangle	title_area;

	VIDEO_ST		video_st;
	gchar           video_text[VWND_TEXT_LEN+1];
	GdkRectangle	video_area;
	GdkRectangle	vlogo_area;

	gchar 			alarm_text[VWND_TEXT_LEN+1];
	GdkRectangle	alarm_area;

	RECORD_ICON 	rec_mode;
	GdkRectangle	recIcon_area;

	gboolean 		audio;
	GdkRectangle	audioIcon_area;

	gboolean 		mic;
	GdkRectangle	micIcon_area;

	gboolean 		motion;
	GdkRectangle	motionIcon_area;

	gboolean 		freeze;
	GdkRectangle	freezeIcon_area;

	gint 			pnd_rate;
	GdkRectangle	pndPrg_area;

	gboolean		login_fail;
	GdkRectangle	loginFail_area;

	gboolean 		is_connecting;
	GdkRectangle	connecting_area;

	DLVA_CNTR_ICON 	dlva_cntr_icon[3];
	guint			dlva_cntr_val[3];
	GdkRectangle	dlva_cntr_area[3];

	ITX_VCA_ICON 	vca_itx_icon[VWND_VCA_MAX_CNT];
	S1_VCA_ICON 	vca_s1_icon[VWND_VCA_MAX_CNT];
	gchar 			vca_text[VWND_VCA_MAX_CNT][VWND_TEXT_LEN+1];
	GdkRectangle	vca_area[VWND_VCA_MAX_CNT];

	gboolean 		vcaline;
	gboolean 		vca_updating;
	gint 			vca_dic_cnt;
	DICPTR 			*vca_pdics;

	ITX_DVABX_ICON 	dvabx_itx_icon[VWND_DVABX_MAX_CNT];
	gchar 			dvabx_text[VWND_DVABX_MAX_CNT][VWND_TEXT_LEN+1];
	GdkRectangle	dvabx_area[VWND_DVABX_MAX_CNT];

	gboolean 		dvabxline;
	gboolean 		dvabx_updating;
	gint 			dvabx_dic_cnt;
	DICPTR 			*dvabx_pdics;

	gint            pos_row;
	gint            pos_color[VWND_POS_LINE];
	gchar 		    pos_text[VWND_POS_LINE][128];
	GdkRectangle	pos_area[VWND_POS_LINE];

	gint			corridor_mode;
	gint			stream_ratio_w;
	gint			stream_ratio_h;

	gchar           debug_text[VWND_DEBUG_LINE][128];
	GdkRectangle	debug_area;

//	gboolean 		conn_btn;

} WIN_INFO_T;

typedef struct _VWND_T {
	WIN_INFO_T		winfo[VSM_WIN_MAX];

	gboolean		border;
	guint			border_color;

	gboolean		seq_icon;
	GdkRectangle	seqIcon_area;

	gchar 			user[VWND_TEXT_LEN+1];
	GdkRectangle	user_area;

	gchar 			time_text[VWND_TEXT_LEN+1];
	GdkRectangle	time_area;
	gboolean        time_off;

	DIR_RATE_E 		play_st;
	GdkRectangle	playST_area;

	ALARM_ST		alarm_st;
	GdkRectangle	alarmST_area;

	NET_ST			net_st;
	guint 			net_cnt;
	GdkRectangle	netST_area;

	DISK_ST			disk_st;
	guint 			disk_usage;
	GdkRectangle	diskST_area;

	RAID_ST			raid_st;
	guint 			raid_usage;

	ARCH_ST			arch_st;
	guint 			arch_cnt;
	GdkRectangle	archST_area;

	gboolean        clondev;
	GdkRectangle	clondev_area;

	POS_CONF_T      pos_cf;
	gboolean 		pos_onoff;

	GdkRectangle	switch_box;

    gint            arrow_ptcnt;
    GdkPoint        arrow_pt[32];

    VIDEO_IMAGE_T   vimage_cf;

	gboolean 		osd_off;
	VSM_DIV_E		dtype;
	FILL_ST			fill;
	guint 			plt_w;
	guint 			plt_h;

	gint			force;
	GdkRegion	    *expose_region;
} VWND_T;

GtkWidget *vwnd_get_main_widget(void);
void vwnd_init(NFWINDOW *parent);
void vwnd_show(void);
void vwnd_set_sfc_mode(VWND_T *p);
void vwnd_repaint();
void vwnd_set_item_position();
void vwnd_set_item_position_connecting(gint ch);
void vwnd_set_blind();
void vwnd_unset_blind();
NFOBJECT* get_live_object();
#endif

