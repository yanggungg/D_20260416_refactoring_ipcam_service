#ifndef _SETUP_IPCAM_INTERNAL_H_
#define _SETUP_IPCAM_INTERNAL_H_

#include "objects/nfobject.h"

#include "nf_api_ipcam.h"
#include "nf_api_cam.h"
#include "nf_ipcam_defs.h"

#include "iux_afx.h"
#include "ix_mem.h"
#include "vw.h"
#include "ix_conf.h"
#include "ix_func.h"
#include "scm.h"


// parent inner fixed width     - 1505
// parent inner fixed height    - 817


#define VIDEO_X                 (0)
#define VIDEO_Y                 (0)
#define VIDEO_W                 (16*39)
#define VIDEO_H                 ((9*39)-3)

#define LEFT_FIXED_X            (0)
#define LEFT_FIXED_Y            (VIDEO_Y + VIDEO_H + 20)
#define LEFT_FIXED_W            (VIDEO_W)
#define LEFT_FIXED_H            (MENU_V_IPCAMSET_SUBTAB_INNER_H - VIDEO_H - 20)

#define RIGHT_FIXED_X           (LEFT_FIXED_X + LEFT_FIXED_W +50)
#define RIGHT_FIXED_Y           (0)
#define RIGHT_FIXED_W           (MENU_V_IPCAMSET_SUBTAB_INNER_W - LEFT_FIXED_W - 50)
#define RIGHT_FIXED_H           (MENU_V_IPCAMSET_SUBTAB_INNER_H - 40)

#define RIGHT_PAGE_FIXED_X      (0)
#define RIGHT_PAGE_FIXED_Y      (0)
#define RIGHT_PAGE_FIXED_W      (RIGHT_FIXED_W)
#define RIGHT_PAGE_FIXED_H      (RIGHT_FIXED_H)

#define MAX_COL_CNT             (6)
#define MAX_ROW_CNT             (30)

#define KEY_MAX_LEN             (32)
#define TEXT_MAX_LEN            (32)
#define ITEM_CNT                (30)

typedef enum _CATEGORY {
	CATEGORY_FOCUS = 0,
	CATEGORY_WB,
	CATEGORY_WDR,
	CATEGORY_DN,
    CATEGORY_CV,
	CATEGORY_EXPOSURE_TIME,
	CATEGORY_GAIN,
	CATEGORY_IRIS,
	CATEGORY_DNR,
	CATEGORY_NR
} CATEGORY;

typedef enum _OBJ_TYPE_E {
	OBJ_LABEL	            = 0,
	OBJ_SPIN	            = 1,
	OBJ_COMBO		        = 2,
	OBJ_SLIDER	            = 3,
	OBJ_BUTTON	            = 4,
	OBJ_TYPE_MAX
} OBJ_TYPE_E;

typedef enum _LABEL_TYPE_E {
	LABEL_CATEGORY	        = 0,
	LABEL_TITLE	            = 1,
	LABEL_NUMBER	        = 2,
	LABEL_LETTER	        = 3,
	LABEL_LEGEND	        = 4,
	LABEL_TYPE_MAX
} LABEL_TYPE_E;

typedef enum _SPIN_TYPE_E {
	SPIN_NUMBER	            = 0,
	SPIN_LETTER	            = 1,
	SPIN_TYPE_MAX
} SPIN_TYPE_E;

typedef enum _COMBO_TYPE_E {
	COMBO_NUMBER	        = 0,
	COMBO_LETTER	        = 1,
	COMBO_TYPE_MAX
} COMBO_TYPE_E;

typedef enum _SLIDER_TYPE_E {
	SLIDER_NORMAL	        = 0,
	SLIDER_TYPE_MAX
} SLIDER_TYPE_E;

typedef enum _BUTTON_TYPE_E {
	BUTTON_LETTER	        = 0,
	BUTTON_IMAGE	        = 1,
	BUTTON_TYPE_MAX
} BUTTON_TYPE_E;

typedef struct _LABEL_DATA_T
{
    LABEL_TYPE_E    type;

	union {
        struct {
            gchar       *img;
            gchar       *text;
        } category;

        struct {
            gchar       *text;
        } title;

        struct {
            gint        init;
            gint        min;
            gint        max;
        } number;

        struct {
            gchar       *text;
        } letter;

        struct {
            gchar       *text;
        } legend;
    };
} LABEL_DATA_T;

typedef struct _SPIN_DATA_T
{
    SPIN_TYPE_E     type;

	union {
        struct {
            gint        init;
            gint        min;
            gint        max;
            gint        step;
        } number;

        struct {
            gint        init;
            gint        cnt;
            guint       val;
            gchar       *text[ITEM_CNT];
        } letter;
    };
} SPIN_DATA_T;

typedef struct _COMBO_DATA_T
{
    COMBO_TYPE_E    type;

	union {
        struct {
            gint        init;
            gint        min;
            gint        max;
            gint        step;
        } number;

        struct {
            gint        init;
            gint        cnt;
            guint       val;
            gchar       *text[ITEM_CNT];
        } letter;
    };
} COMBO_DATA_T;

typedef struct _SLIDER_DATA_T
{
    SLIDER_TYPE_E   type;

	union {
        struct {
            gint        init;
            gint        min;
            gint        max;
            gint        cnt;
        } normal;
    };

} SLIDER_DATA_T;

typedef struct _BUTTON_DATA_T
{
    BUTTON_TYPE_E   type;

	union {
        struct {
            gchar       *text;
        } letter;

        struct {
            GdkPixbuf   *name[NFOBJECT_STATE_COUNT];
        } image;
    };

} BUTTON_DATA_T;

typedef struct _OBJECT_INFO_T
{
	OBJ_TYPE_E      obj_type;
	gint            enable;
    gint            width;
    gpointer        handler;
	gchar			nickname[32];

    union {
        struct {
            LABEL_DATA_T   data;
        } label;

        struct {
            SPIN_DATA_T    data;
        } spin;

        struct {
            COMBO_DATA_T   data;
        } combo;

        struct {
            SLIDER_DATA_T  data;
        } slider;

        struct {
            BUTTON_DATA_T  data;
        } button;
    };
} OBJECT_INFO_T;

typedef struct _COL_INFO_T
{
    guint64           key;               // col NF_IPCAM_IMAGE_ONVIF, NF_IPCAM_EXPOSURE_ONVIF
    OBJECT_INFO_T   *obj_info;
    NFOBJECT        *obj;
} COL_INFO_T;

typedef struct _ROW_INFO_T
{
    guint64           key;                // row NF_IPCAM_IMAGE_ONVIF, NF_IPCAM_EXPOSURE_ONVIF
    NFOBJECT        *fixed;
	COL_INFO_T      *col_info[MAX_COL_CNT];
    gint            col_cnt;
} ROW_INFO_T;

typedef struct _FIXED_INFO_T
{
//    guint           key;                // mode NFIPCamOption_onvif value
    guint64           visible;
    guint64           enable;
    NFOBJECT        *fixed;
	ROW_INFO_T      *row_info[MAX_ROW_CNT];
    gint            row_cnt;
} FIXED_INFO_T;

typedef struct _IPCAM_SUBFIXED_T
{
    FIXED_INFO_T imageSet;
    FIXED_INFO_T rotate;
    FIXED_INFO_T focusMode;
    FIXED_INFO_T wbMode;
    FIXED_INFO_T dnMode;
    FIXED_INFO_T exposureMode;
    FIXED_INFO_T exposureBlc;
    FIXED_INFO_T exposureAnti;
    FIXED_INFO_T exposureTime;
    FIXED_INFO_T exposureGain;
    FIXED_INFO_T exposureIris;
    FIXED_INFO_T exposureDnr;
    FIXED_INFO_T exposureWdr;
    FIXED_INFO_T exposureHlc;
    FIXED_INFO_T exposureDefog;
} IPCAM_SUBFIXED_T;

extern IPCAM_SUBFIXED_T g_ipcam_subFixed[GUI_CHANNEL_CNT];

typedef struct _IMAGE_PROFILE_T
{
    gint                            done;
	NFIPCamImageProfile_onvif       profile;
} IMAGE_PROFILE_T;

typedef struct _EXPOSURE_PROFILE_T
{
    gint                            done;
	NFIPCamExposureProfile_onvif    profile;
} EXPOSURE_PROFILE_T;

typedef struct _IPCAM_MANAGE_T
{
    gint                  is_onvif;
    gint                  image_default;
	IMAGE_PROFILE_T       image;
    gint                  exposure_default;
	EXPOSURE_PROFILE_T    exposure;
} IPCAM_MANAGE_T;

extern IPCAM_MANAGE_T g_ipcam_manage[GUI_CHANNEL_CNT];
extern IPCamSetupData g_ipcamData[GUI_CHANNEL_CNT];
extern IPCamSetupData g_org_ipcamData[GUI_CHANNEL_CNT];


typedef struct _DEPENDENT_CHECK_PROFILE_T
{
    NFIPCamOption_onvif base_shutter_temp[NF_IPCAM_BASE_SHUTTER_MODE_PAL_NR];
    gint base_shutter_temp_cnt;
    gint base_shutter_temp_sel;

    NFIPCamOption_onvif max_shutter_temp[NF_IPCAM_MAX_SHUTTER_MODE_PAL_NR];
    gint max_shutter_temp_cnt;
    gint max_shutter_temp_sel;

} DEPENDENT_CHECK_PROFILE_T;

extern DEPENDENT_CHECK_PROFILE_T g_ipcam_data_temp[GUI_CHANNEL_CNT];


typedef struct _NM_TYPE_T
{
    gint                    init;
    guint64                 category;		    //!< ī�װ��. @see _NF_IPCAM_IMAGE_E
	guint                   value;				//!< ���簪.
	gint                    min;			    //!< �ּҰ�.
	gint                    max;				//!< �ִ밪.
	guint64                 dependent_category;	//!< difference ���� ������ �ִ� ī�װ��.
	gint                    difference;
} NM_TYPE_T;

typedef struct _LT_TYPE_T
{
    gint                    init;
    gint                    cnt;
    guint64                 category;
    guint                   val[ITEM_CNT];
    guint64                 dependent_category[ITEM_CNT];
    guint64                 visible_category[ITEM_CNT];
    guint64                 enable_category[ITEM_CNT];
    gchar                   text[ITEM_CNT][TEXT_MAX_LEN];
} LT_TYPE_T;

typedef struct _ROW_MANAGE_T
{
    ROW_INFO_T *image[NF_IPCAM_IMAGE_ONVIF_NR];
    ROW_INFO_T *exposure[NF_IPCAM_EXPOSURE_ONVIF_NR];
} ROW_MANAGE_T;

// sub fixed (vw_sys_camera_ipcam_subFixed.c)

NFOBJECT* _make_subFixed_image_set(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_rotate(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_focus(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_white_balance(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_wide_dynamic(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_day_night(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_mode(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_colorvu(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_blc(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_dnr(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_antiflicker(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_hlc(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_defog(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_time(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_gain(gint ch, gint *subFixed_height);
NFOBJECT* _make_subFixed_exposure_iris(gint ch, gint *subFixed_height);

// row info (vw_sys_camera_ipcam_object.c)

gint _make_objs_of_subFixed(FIXED_INFO_T *fixed_info, gint width);
gint _set_visible_subFixed(FIXED_INFO_T *fixed_info);
gint _set_enable_subFixed(FIXED_INFO_T *fixed_info);


// row info (vw_sys_camera_ipcam_manage.c)

gint _update_ipcam_profile(gint ch);
gint _set_drawing_profile();

gint _make_row_info(gint ch);
gint _destory_row_info(gint ch);

gint _load_ipcam_db();
gint _save_ipcam_db();
gint _cancel_ipcam_db();
gint _copy_ipcam_db(gint src_ch, guint copy_mask);
gint _is_changed_ipcam_db();

ROW_INFO_T *find_row_info_exposure(gint ch, guint64 category);
NFOBJECT *find_obj_exposure(gint ch, guint64 category, gpointer data);

gint _get_fixed_info_image_set(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_rotate(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_focus_mode(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_wb_mode(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_wdr_mode(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_dn_mode(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_mode(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_dnr(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_blc_mode(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_antiflicker(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_time(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_gain(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_iris(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_hlc(gint ch, FIXED_INFO_T *fixed_info);
gint _get_fixed_info_exposure_defog(gint ch, FIXED_INFO_T *fixed_info);


// handler (vw_sys_camera_ipcam_callback.c)

gboolean _post_focus_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_white_balance_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_wide_dynamic_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_ircut_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_ircutm_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_blc_control_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_brightness_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_brightness_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_contrast_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_contrast_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_tint_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_tint_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_color_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_color_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_sharpness_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_sharpness_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_near_limit_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_near_limit_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_far_limit_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_far_limit_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_default_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_default_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_absolute_position_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_absolute_position_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_absolute_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_absolute_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_relative_distance_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_relative_distance_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_relative_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_relative_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_continuous_speed_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_continuous_speed_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_focus_limit_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_ir_correction_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_stabilizer_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_cr_gain_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_cr_gain_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_cb_gain_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_cb_gain_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_wide_dynamic_level_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_wide_dynamic_level_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_blc_level_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_blc_level_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_hlc_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_defog_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_time_min_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_time_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_time_max_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_time_max_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_manual_exposure_time_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_manual_exposure_time_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_gain_min_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_gain_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_gain_max_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_gain_max_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_manual_exposure_gain_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_manual_exposure_gain_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_iris_min_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_iris_min_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_iris_max_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_auto_exposure_iris_max_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_manual_exposure_iris_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_manual_exposure_iris_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_rotate_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_mwb_mode_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_dnn_toggle_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_priority_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_60_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_50_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_50_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_60_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_100_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_120_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_100_300_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_100_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_120_360_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_120_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_120_262_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_30_262_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_25_100_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_25_300_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_25_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_30_120_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_30_360_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_base_shutter_30_5000_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_motion_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_slowshutter_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_maxagc_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_dciris_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_dciris_motion_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_calibration_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_antiflicker_m_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_antiflicker_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_antiflicker_a_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_antiflicker_m_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_motion_off_on_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_auto_off_off_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_auto_off_on_tv_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_max_shutter_auto_off_on_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_one_push_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_home_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_continuous_near_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_continuous_far_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_exposure_dnr_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_adaptive_ir_combo_event_handler(NFOBJECT * obj, GdkEvent * evt, gpointer data);
gboolean _post_dnn_sense_dton_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_dnn_sense_dton_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_dnn_sense_ntod_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_dnn_sense_ntod_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_illumination_ctrl_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_illumination_level_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_illumination_level_slider_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
gboolean _post_ddn_schedule_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data);
#endif

