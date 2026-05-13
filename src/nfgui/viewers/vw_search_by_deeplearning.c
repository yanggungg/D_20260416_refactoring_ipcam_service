#include "nf_afx.h"
#include "itx_ai_def.h"
#include "nf_va_object_detector.h"
#include "nf_api_dva_eventlog.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nftimelabel.h"
#include "objects/nfimage.h"

#include "vw_search_main.h"
#include "vw_search_by_deeplearning.h"
#include "vw_deeplearning_image_popup.h"
#include "vw_search_by_deeplearning_filter_popup.h"
#include "vw_vkeyboard.h"

#include "vw_live_statusbar.h"
#include "vw_set_date_time.h"
#include "vw_internal.h"
#include "ix_mem.h"
#include "dtf.h"
#include "stm.h"
#include "var.h"
#include "wrk.h"
#include "scm.h"

#include "nf_sysman.h"

////////////////////////////////////////////////////////////
//
// private data types
//

#define SKP_TITLE_SIZE_W            (200)
#define SKP_KEYWORD_SIZE_W          (300)
#define SKP_OPER_SIZE_W             (100)
#define SKP_OBJ_GAP                 (2)

#define THUMBNAIL_MARGIN_SIZE_W 	(50)
#define THUMBNAIL_MARGIN_SIZE_H 	(50)

#define MKB_TAB_SUBGROUP_TITLE_NAME     "mkb_tab_subgroup_title_%d"

#define STR_ENGINE_AIBOX            (_get_aibox_str())
#define STR_ENGINE_BUILTIN          "BUILT-IN AI"

#define STR_TYPE_DETECTOR           "DETECTOR"
#define STR_TYPE_GENERIC            "GENERIC"
#define STR_TYPE_INTRUSION          "INTRUSION DETECTION"
#define STR_TYPE_ILLEGAL_PARKING    "ILLEGAL PARKING"

enum {   
	IDX_AIBOX_DETECTOR = 0,
    IDX_AIBOX_GENERIC,
	AIBOX_TYPE_MAX,
};

enum {   
	IDX_BUILTIN_INTRUSION = 0,
	IDX_BUILTIN_ILLEGAL_PARKING,
	BUILTIN_TYPE_MAX,
};

enum {
    DF_YMD = 0,
    DF_MDY,
    DF_DMY,
    NUM_DATE_FORMATS,
};

#define THUMBNAIL_TITLE             (22)
#define THUMBNAIL_INFO              (60)

#define THUMBNAIL_COL               (5)
#define THUMBNAIL_LOW               (4)
#define THUMBNAIL_COUNT             (20)

#define _FIXED1_X					(0)
#define _FIXED1_Y					(0)
#define _FIXED1_W					(455)
#define _FIXED1_H					(903)

#define _FIXED2_X					(_FIXED1_X+_FIXED1_W)
#define _FIXED2_Y					(_FIXED1_Y)
#define _FIXED2_W					(1401)
#define _FIXED2_H					(_FIXED1_H)

#define THUMBNAIL_SRC_W             (640)
#define THUMBNAIL_SRC_H             (360)

#define DVA_FRAME_WIDTH			    (272)
#define DVA_FRAME_HEIGHT			(117)



////////////////////////////////////////////////////////////
//
// private variable
//

typedef struct _THUMBNAIL_PARAM_T {
    gint idx;
	gint ch;
    GTimeVal ttime;
    gint image_w;
    gint image_h;
} THUMBNAIL_PARAM_T;

typedef struct _DVAOBJ_INFO_T {
	gint ch;
    gchar rule_name[64];
	gchar class_name[64];
    gchar caption[128];
    gchar title[128];
    gchar description[512];
	GTimeVal ttime;
	gint confidence;
    gfloat top_x;
    gfloat top_y;
    gfloat bottom_x;
    gfloat bottom_y;
    GdkPixbuf *fpixbuf;
} DVAOBJ_INFO_T;

typedef struct _DVA_RESULT_T {
    NFOBJECT *obj;
    gint is_selected;
    gint res_log;
    gint res_thumb;
    DVAOBJ_INFO_T dvaobj_info;
} DVA_RESULT_T;



static WRK_ID iwrk = 0; 

static DLOGCTX g_log_ctx = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_aibox_type_fixed[AIBOX_TYPE_MAX];
static NFOBJECT *g_builtin_type_fixed[BUILTIN_TYPE_MAX];

static NFOBJECT *g_from_obj;
static NFOBJECT *g_to_obj;

static NFOBJECT *g_engine_obj;
static NFOBJECT *g_type_obj;
static NFOBJECT *g_generic_obj;
static NFOBJECT *g_type_label_obj;

static NFOBJECT *g_ch_all_obj;
static NFOBJECT *g_search_obj;
static NFOBJECT *g_ch_obj[GUI_CHANNEL_CNT];

static NFOBJECT *g_preview_obj;

static NFOBJECT *g_aibox_dtr_rule_obj;
static NFOBJECT *g_aibox_dtr_forward_obj;
static NFOBJECT *g_aibox_dtr_reverse_obj;
static NFOBJECT *g_aibox_dtr_intrusion_obj;
//static NFOBJECT *g_aibox_dtr_enter_obj;
//static NFOBJECT *g_aibox_dtr_exit_obj;
static NFOBJECT *g_aibox_dtr_remove_obj;
static NFOBJECT *g_aibox_dtr_loiter_obj;
static NFOBJECT *g_aibox_dtr_stop_obj;
static NFOBJECT *g_aibox_dtr_object_obj;
static NFOBJECT *g_aibox_dtr_human_obj;
static NFOBJECT *g_aibox_dtr_car_obj;
static NFOBJECT *g_aibox_dtr_bike_obj;
static NFOBJECT *g_aibox_manual_check_obj;
static NFOBJECT *g_aibox_manual_label_obj;

static NFOBJECT *g_builtin_idz_human_obj;
static NFOBJECT *g_builtin_idz_vehicle_obj;
static NFOBJECT *g_builtin_idz_car_obj;
//static NFOBJECT *g_builtin_idz_bus_obj;
static NFOBJECT *g_builtin_idz_bike_obj;
//static NFOBJECT *g_builtin_idz_animal_obj;

static NFOBJECT *g_builtin_ipz_all_obj;
static NFOBJECT *g_builtin_ipz_car_obj;
//static NFOBJECT *g_builtin_ipz_bus_obj;
static NFOBJECT *g_builtin_ipz_bike_obj;

static NFOBJECT *g_aibox_thumb_obj[AIBOX_TYPE_MAX][THUMBNAIL_COUNT];
static NFOBJECT *g_aibox_generic_thumb_obj[AIBOX_TYPE_MAX][10];
static NFOBJECT *g_aibox_search_obj[AIBOX_TYPE_MAX];
static NFOBJECT *g_aibox_order_obj[AIBOX_TYPE_MAX];
static NFOBJECT *g_aibox_prev_obj[AIBOX_TYPE_MAX];
static NFOBJECT *g_aibox_next_obj[AIBOX_TYPE_MAX];
static NFOBJECT *g_aibox_rate_obj[AIBOX_TYPE_MAX];

static NFOBJECT *g_builtin_thumb_obj[BUILTIN_TYPE_MAX][THUMBNAIL_COUNT];
static NFOBJECT *g_builtin_search_obj[BUILTIN_TYPE_MAX];
static NFOBJECT *g_builtin_order_obj[BUILTIN_TYPE_MAX];
static NFOBJECT *g_builtin_prev_obj[BUILTIN_TYPE_MAX];
static NFOBJECT *g_builtin_next_obj[BUILTIN_TYPE_MAX];
static NFOBJECT *g_builtin_rate_obj[BUILTIN_TYPE_MAX];

static NFOBJECT *g_playback_obj;
static NFOBJECT *g_play_time_obj;

static NFOBJECT *g_aibox_generic_keyword[3];
static NFOBJECT *g_oper[2];

static guint g_preview_timer_id;

static DVA_RESULT_T g_dva_res[THUMBNAIL_COUNT] = {0, };
static DVA_RESULT_T g_generic_dva_res[10] = {0, };
//static DVA_GENERIC_RESULT_T g_dva_res[10] = {0, };

static gchar g_aibbx_str[128];

static OPT_AI_DTR_T g_opt_ai_dtr;
static OPT_AI_GENERIC_T g_opt_ai_generic;
//static OPT_AI_FR_T g_opt_ai_generic;
static OPT_BUILTIN_IDZ_T g_opt_builtin_idz;
static OPT_BUILTIN_IPZ_T g_opt_builtin_ipz;

static gint g_prev_engine = 0;


////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _init_opt_filter(gchar *data)
{
    int i;

    memset(&g_opt_ai_dtr, 1, sizeof(g_opt_ai_dtr));
    memset(g_opt_ai_dtr.obj.strcustom, 0x00, sizeof(g_opt_ai_dtr.obj.strcustom));

    memset(&g_opt_builtin_idz, 1, sizeof(g_opt_builtin_idz));
    memset(&g_opt_builtin_ipz, 1, sizeof(g_opt_builtin_ipz));

    memset(&g_opt_ai_generic, 0x00, sizeof(g_opt_ai_generic));
    memset(g_opt_ai_generic.evt_type_chk, 1, sizeof(g_opt_ai_generic.evt_type_chk));
    g_opt_ai_generic.evt_type = data;

    return 0;
}

static gchar *_get_aibox_str()
{
    memset(g_aibbx_str, 0x00, sizeof(g_aibbx_str));
    snprintf(g_aibbx_str, sizeof(g_aibbx_str), "%s/%s", lookup_string("AI CAM"), lookup_string("AI BOX"));
    return g_aibbx_str;
}

static gint _check_frame_margin_size(gint thumbnail_w, gint thumbnail_h, float *start_coord_x, float *start_coord_y, float *end_coord_x, float *end_coord_y)
{
	float margin_rate_w, margin_rate_h;
	float bbx_half, margin_half;

	margin_rate_w = (float)(THUMBNAIL_MARGIN_SIZE_W)/(float)thumbnail_w;
	margin_rate_h = (float)(THUMBNAIL_MARGIN_SIZE_H)/(float)thumbnail_h;

	if (*end_coord_x-*start_coord_x < margin_rate_w) 
	{
		bbx_half = (*end_coord_x-*start_coord_x)/2.0;
		margin_half = margin_rate_w/2.0;

		if (*start_coord_x + bbx_half - margin_half < 0.0) {
			*start_coord_x = 0;
			*end_coord_x = margin_rate_w;
		}
		else if (*start_coord_x + bbx_half + margin_half > 1.0) {
			*start_coord_x = 1.0 - margin_half;
			*end_coord_x = 1.0;
		}
		else {
			*start_coord_x += bbx_half - margin_half;
			*end_coord_x = *start_coord_x + margin_rate_w;
		} 
	}

	if (*end_coord_y-*start_coord_y < margin_rate_h) 
	{
		bbx_half = (*end_coord_y-*start_coord_y)/2.0;
		margin_half = margin_rate_h/2.0;

		if (*start_coord_y + bbx_half - margin_half < 0.0) {
			*start_coord_y = 0;
			*end_coord_y = margin_rate_h;
		}
		else if (*start_coord_y + bbx_half + margin_half > 1.0) {
			*start_coord_y = 1.0 - margin_half;
			*end_coord_y = 1.0;
		}
		else {
			*start_coord_y += bbx_half - margin_half;
			*end_coord_y = *start_coord_y + margin_rate_h;
		} 
	}

	return 0;
}

static int _proc_get_thumbnail(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
    THUMBNAIL_PARAM_T *param = (THUMBNAIL_PARAM_T*)pmsg->data;
    guchar *image_buf = 0;
    gboolean ret_val;

    GTimeVal from_tv = {0, 0};
    GTimeVal to_tv = {0, 0};
    GTimeVal data_tv = {0, 0};

    ret_val = nf_play_set_thumbnail_geometry(0, 0, param->image_w, param->image_h);
    //usleep(30*1000);
    
    if (ret_val) 
    {
        from_tv.tv_sec = param->ttime.tv_sec;
        from_tv.tv_usec = param->ttime.tv_usec;
        to_tv.tv_sec = param->ttime.tv_sec+1;
        to_tv.tv_usec = param->ttime.tv_usec;

        image_buf = imalloc(sizeof(guchar)*param->image_w*param->image_h*3);
        ret_val = nf_play_get_thumbnail(param->ch, from_tv, to_tv, param->image_w, param->image_h, 24, (gpointer)image_buf, &data_tv);
    }

    if (ret_val) {
        evt_send_to_local(INFY_THUMBNAIL_CMPL_OBJ2, param->idx, 1, image_buf);
    }
    else {
        evt_send_to_local(INFY_THUMBNAIL_CMPL_OBJ2, param->idx, 0, 0);
        if (image_buf) ifree(image_buf);
    }

	return 0;
}

static GdkPixbuf *_get_target_pixbuf(DVAOBJ_INFO_T *dvaobj)
{
	GdkDrawable *drawable = NULL;

    gfloat top_x = 0 , top_y = 0;
    gfloat bottom_x = 0, bottom_y = 0;

	gfloat obj_w, obj_h;
	gfloat objrate;

    GdkPixbuf *src_pixbuf = dvaobj->fpixbuf;
	GdkPixbuf *copy_pixbuf = NULL;
	GdkPixbuf *dst_pixbuf = NULL;

    top_x = dvaobj->top_x;
    top_y = dvaobj->top_y;
    bottom_x = dvaobj->bottom_x;
    bottom_y = dvaobj->bottom_y; 

    _check_frame_margin_size(THUMBNAIL_SRC_W, THUMBNAIL_SRC_H, &top_x, &top_y, &bottom_x, &bottom_y);
    //problem
	//obj_w = THUMBNAIL_SRC_W*(bottom_x-top_x);
	//obj_h = THUMBNAIL_SRC_H*(bottom_y-top_y);
    obj_w = DVA_FRAME_WIDTH;
    obj_h = DVA_FRAME_HEIGHT;

	//if (DVA_FRAME_WIDTH/obj_w >= DVA_FRAME_HEIGHT/obj_h) objrate = DVA_FRAME_HEIGHT/obj_h;
	//else objrate = DVA_FRAME_WIDTH/obj_w;
    objrate = 1;

	copy_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, (gint)((bottom_x-top_x)*THUMBNAIL_SRC_W), (gint)((bottom_y-top_y)*THUMBNAIL_SRC_H));
	gdk_pixbuf_fill (copy_pixbuf, 0x000000ff);

	gdk_pixbuf_copy_area(src_pixbuf, (gint)(top_x*THUMBNAIL_SRC_W), (gint)(top_y*THUMBNAIL_SRC_H), 
			(gint)((bottom_x-top_x)*THUMBNAIL_SRC_W), (gint)((bottom_y-top_y)*THUMBNAIL_SRC_H), copy_pixbuf, 0, 0); 	

	dst_pixbuf = gdk_pixbuf_scale_simple(copy_pixbuf, (gint)((objrate-0.1)*obj_w), (gint)((objrate-0.1)*obj_h), 0); 

	g_object_unref(copy_pixbuf);	
	return dst_pixbuf;
}

static int _init_worker()
{
	iwrk = wrk_create_worker(_proc_get_thumbnail, 0);
	wrk_change_sleep_time(iwrk, 30000);
	return 0;
}

static gint _set_all_chk_btn(NFOBJECT *all, NFOBJECT *unit[], gboolean expose)
{
	gboolean state = TRUE;
	gint i;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
        if (!nfui_nfobject_is_disabled(unit[i])) {
	        if (!nfui_check_button_get_active((NFCHECKBUTTON*)unit[i])) state = FALSE;
        }
	}

	nfui_check_button_set_active((NFCHECKBUTTON*)all, state);
    return 0;
}

static gint _update_covert_info()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        if (ssm_get_covert_mask() & (1 << i))
        {
            nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_obj[i], FALSE);
            nfui_nfobject_disable(g_ch_obj[i]);
        }
    }

    if ((GUI_CHANNEL_CNT == 4) && (ssm_get_covert_mask() == 0xf))
    {
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_all_obj, FALSE);
        nfui_nfobject_disable(g_ch_all_obj);
    }
    else if ((GUI_CHANNEL_CNT == 8) && (ssm_get_covert_mask() == 0xff))
    {
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_all_obj, FALSE);
        nfui_nfobject_disable(g_ch_all_obj);
    }
    else if ((GUI_CHANNEL_CNT == 16) && (ssm_get_covert_mask() == 0xffff))
    {
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_all_obj, FALSE);
        nfui_nfobject_disable(g_ch_all_obj);
    }
    else if ((GUI_CHANNEL_CNT == 32) && (ssm_get_covert_mask() == 0xffffffff))
    {
        nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_all_obj, FALSE);
        nfui_nfobject_disable(g_ch_all_obj);
    }

	_set_all_chk_btn(g_ch_all_obj, g_ch_obj, FALSE);

    return 0;
}

static nftl_df_type prvTransDateFormat(gint db_index)
{
    nftl_df_type ret;

    if(db_index == DF_YMD)          ret = NFTL_DF_YMD;
    else if(db_index == DF_MDY)     ret = NFTL_DF_MDY;
    else if(db_index == DF_DMY)     ret = NFTL_DF_DMY;
    else    ret = NFTL_DF_HIDE;

    return ret;
}

static void *_dup_pointer(void *data, int len)
{
	void *ptr = imalloc(len);
	memcpy(ptr, data, len);
	return ptr;
}

static void _dir_prev_button_enable()
{
    gint i;

    for (i = 0; i < AIBOX_TYPE_MAX; i++) {
	    nfui_nfobject_enable(g_aibox_prev_obj[i]);
	    nfui_signal_emit(g_aibox_prev_obj[i], GDK_EXPOSE, TRUE);
    }

    for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
	    nfui_nfobject_enable(g_builtin_prev_obj[i]);
	    nfui_signal_emit(g_builtin_prev_obj[i], GDK_EXPOSE, TRUE);
    }
}

static void _dir_prev_button_disable()
{
    gint i;

    for (i = 0; i < AIBOX_TYPE_MAX; i++) {
	    nfui_nfobject_disable(g_aibox_prev_obj[i]);
	    nfui_signal_emit(g_aibox_prev_obj[i], GDK_EXPOSE, TRUE);
    }

    for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
	    nfui_nfobject_disable(g_builtin_prev_obj[i]);
	    nfui_signal_emit(g_builtin_prev_obj[i], GDK_EXPOSE, TRUE);
    }
}

static void _dir_next_button_enable()
{
    gint i;

    for (i = 0; i < AIBOX_TYPE_MAX; i++) {
	    nfui_nfobject_enable(g_aibox_next_obj[i]);
	    nfui_signal_emit(g_aibox_next_obj[i], GDK_EXPOSE, TRUE);
    }

    for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
	    nfui_nfobject_enable(g_builtin_next_obj[i]);
	    nfui_signal_emit(g_builtin_next_obj[i], GDK_EXPOSE, TRUE);
    }
}

static void _dir_next_button_disable()
{
    gint i;

    for (i = 0; i < AIBOX_TYPE_MAX; i++) {
	    nfui_nfobject_disable(g_aibox_next_obj[i]);
	    nfui_signal_emit(g_aibox_next_obj[i], GDK_EXPOSE, TRUE);
    }

    for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
	    nfui_nfobject_disable(g_builtin_next_obj[i]);
	    nfui_signal_emit(g_builtin_next_obj[i], GDK_EXPOSE, TRUE);
    }
}

static void _order_button_enable()
{
    gint i;

    for (i = 0; i < AIBOX_TYPE_MAX; i++) {
	    nfui_nfobject_enable(g_aibox_order_obj[i]);
	    nfui_signal_emit(g_aibox_order_obj[i], GDK_EXPOSE, TRUE);
    }

    for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
	    nfui_nfobject_enable(g_builtin_order_obj[i]);
	    nfui_signal_emit(g_builtin_order_obj[i], GDK_EXPOSE, TRUE);
    }
}

static void _order_button_disable()
{
    gint i;

    for (i = 0; i < AIBOX_TYPE_MAX; i++) {
	    nfui_nfobject_disable(g_aibox_order_obj[i]);
	    nfui_signal_emit(g_aibox_order_obj[i], GDK_EXPOSE, TRUE);
    }

    for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
	    nfui_nfobject_disable(g_builtin_order_obj[i]);
	    nfui_signal_emit(g_builtin_order_obj[i], GDK_EXPOSE, TRUE);
    }
}

static void _playback_button_enable()
{
	nfui_nfobject_enable(g_playback_obj);
	nfui_signal_emit(g_playback_obj, GDK_EXPOSE, TRUE);
}

static void _playback_button_disable()
{
	nfui_nfobject_disable(g_playback_obj);
	nfui_signal_emit(g_playback_obj, GDK_EXPOSE, TRUE);
}

static void _preview_obj_on(gchar *buf)
{
	nfui_nflabel_set_text((NFLABEL*)g_preview_obj, buf);
	nfui_nfobject_modify_bg(g_preview_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_signal_emit(g_preview_obj, GDK_EXPOSE, TRUE);
}

static void _preview_obj_off(gchar *buf)
{
	nfui_nflabel_set_text((NFLABEL*)g_preview_obj, buf);
	nfui_nfobject_modify_bg(g_preview_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
	nfui_signal_emit(g_preview_obj, GDK_EXPOSE, TRUE);
}

static gint _set_search_log_cnt(gint cnt)
{
    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            nfui_nfobject_set_data(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], "log cnt", GUINT_TO_POINTER(cnt));
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            nfui_nfobject_set_data(g_aibox_rate_obj[IDX_AIBOX_GENERIC], "log cnt", GUINT_TO_POINTER(cnt));
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            nfui_nfobject_set_data(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], "log cnt", GUINT_TO_POINTER(cnt));
        }
        else {
            nfui_nfobject_set_data(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], "log cnt", GUINT_TO_POINTER(cnt)); 
        }
    }
}

static gint _set_search_log_rate(gint rate)
{
    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            nfui_nfobject_set_data(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], "log rate", GUINT_TO_POINTER(rate));
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            nfui_nfobject_set_data(g_aibox_rate_obj[IDX_AIBOX_GENERIC], "log rate", GUINT_TO_POINTER(rate));
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            nfui_nfobject_set_data(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], "log rate", GUINT_TO_POINTER(rate));
        }
        else {
            nfui_nfobject_set_data(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], "log rate", GUINT_TO_POINTER(rate)); 
        }
    }
}

static gint _get_search_log_cnt()
{
    gint log_cnt = 0;

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], "log cnt"));
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_GENERIC], "log cnt"));
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], "log cnt"));
        }
        else {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], "log cnt"));
        }
    }
    return log_cnt;
}

static gint _get_search_log_rate()
{
    gint log_rate = 0;

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], "log rate"));
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_GENERIC], "log rate"));
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], "log rate"));
        }
        else {
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], "log rate"));
        }
    }
    return log_rate;
}

static void _update_search_log_percent(gint log_rate, gint log_cnt)
{
    gchar strBuf[64];

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0)
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            g_sprintf(strBuf, "(%d / %d)", log_rate, log_cnt);
            nfui_nflabel_set_text((NFLABEL*)g_aibox_rate_obj[IDX_AIBOX_DETECTOR], strBuf);
            nfui_signal_emit(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], GDK_EXPOSE, TRUE);  
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0)
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            g_sprintf(strBuf, "(%d / %d)", log_rate, log_cnt);
            nfui_nflabel_set_text((NFLABEL*)g_aibox_rate_obj[IDX_AIBOX_GENERIC], strBuf);
            nfui_signal_emit(g_aibox_rate_obj[IDX_AIBOX_GENERIC], GDK_EXPOSE, TRUE);  
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0)
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            g_sprintf(strBuf, "(%d / %d)", log_rate, log_cnt);
            nfui_nflabel_set_text((NFLABEL*)g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], strBuf);
            nfui_signal_emit(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], GDK_EXPOSE, TRUE);  
        }
        else
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            g_sprintf(strBuf, "(%d / %d)", log_rate, log_cnt);
            nfui_nflabel_set_text((NFLABEL*)g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], strBuf);
            nfui_signal_emit(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], GDK_EXPOSE, TRUE);  
        }
    }
}

static void _update_aibox_dtr_data(gint log_cnt, LOG_DATA_T data[THUMBNAIL_COUNT])
{
    float top_x = 0 , top_y = 0; 
    float bottom_x = 0, bottom_y = 0;
	gint i;

    for (i = 0; i < THUMBNAIL_COUNT; i++)
    {
        if (i < log_cnt)
        { 
            ai_rule_event_t ai_data;
            THUMBNAIL_PARAM_T thumbnail_param;

            g_dva_res[i].res_log = 1;

            memset(&ai_data, 0x00, sizeof(ai_rule_event_t));
            memcpy(&ai_data, data[i].p.cat_dva.binary, sizeof(ai_rule_event_t));

            g_dva_res[i].dvaobj_info.ch = data[i].p.cat_dva.channel;
            g_dva_res[i].dvaobj_info.ttime.tv_sec = data[i].tvTime.tv_sec;
            g_dva_res[i].dvaobj_info.ttime.tv_usec = data[i].tvTime.tv_usec;

            if (ai_data.type & IVCA_ET_INTRUSION) strcpy(g_dva_res[i].dvaobj_info.rule_name, "intrusion");
            else if (ai_data.type & IVCA_ET_ENTER) strcpy(g_dva_res[i].dvaobj_info.rule_name, "enter");
            else if (ai_data.type & IVCA_ET_EXIT) strcpy(g_dva_res[i].dvaobj_info.rule_name, "exit");
            else if (ai_data.type & IVCA_ET_STOPPED) strcpy(g_dva_res[i].dvaobj_info.rule_name, "stopped");
            else if (ai_data.type & IVCA_ET_LOITERED) strcpy(g_dva_res[i].dvaobj_info.rule_name, "loitering");
            else if (ai_data.type & IVCA_ET_FALL) strcpy(g_dva_res[i].dvaobj_info.rule_name, "fall");
            else if (ai_data.type & IVCA_ET_REMOVED) strcpy(g_dva_res[i].dvaobj_info.rule_name, "removed");
            else if (ai_data.type & IVCA_ET_DIR_POS) strcpy(g_dva_res[i].dvaobj_info.rule_name, "forward direction");
            else if (ai_data.type & IVCA_ET_DIR_NEG) strcpy(g_dva_res[i].dvaobj_info.rule_name, "reverse direction");

            dvatext_translate_to_uxitem(ai_data.object_class, g_dva_res[i].dvaobj_info.class_name, sizeof(g_dva_res[i].dvaobj_info.class_name));
            g_dva_res[i].dvaobj_info.confidence = (gint)(ai_data.confidence*100);
            g_dva_res[i].dvaobj_info.top_x = MAX(ai_data.bbx_position[0], 0);
            g_dva_res[i].dvaobj_info.top_y = MAX(ai_data.bbx_position[1], 0);
            g_dva_res[i].dvaobj_info.bottom_x = MIN(ai_data.bbx_position[2], 1);
            g_dva_res[i].dvaobj_info.bottom_y = MIN(ai_data.bbx_position[3], 1); 

            memset(&thumbnail_param, 0x00, sizeof(THUMBNAIL_PARAM_T));
            thumbnail_param.idx = i;
            thumbnail_param.ch = g_dva_res[i].dvaobj_info.ch;
            thumbnail_param.ttime.tv_sec = g_dva_res[i].dvaobj_info.ttime.tv_sec;
            thumbnail_param.ttime.tv_usec = g_dva_res[i].dvaobj_info.ttime.tv_usec;
            thumbnail_param.image_w = THUMBNAIL_SRC_W;
            thumbnail_param.image_h = THUMBNAIL_SRC_H;

            wrk_run_msg(iwrk, IMSG_NONE, 0, 1, _dup_pointer(&thumbnail_param, sizeof(THUMBNAIL_PARAM_T)));
        }
    }
}

static void _update_aibox_generic_data(gint log_cnt, LOG_DATA_T data[THUMBNAIL_COUNT])
{
    float top_x = 0 , top_y = 0; 
    float bottom_x = 0, bottom_y = 0;
	gint i;

    for (i = 0; i < THUMBNAIL_COUNT; i++)
    {
        if (i < log_cnt)
        { 
            ai_generic_event_t ai_generic_data;
            THUMBNAIL_PARAM_T thumbnail_param;

            g_dva_res[i].res_log = 1;

            memset(&ai_generic_data, 0x00, sizeof(ai_generic_event_t));
            memcpy(&ai_generic_data, data[i].p.cat_dva.binary, sizeof(ai_generic_event_t));

            g_dva_res[i].dvaobj_info.ch = data[i].p.cat_dva.channel;
            g_dva_res[i].dvaobj_info.ttime.tv_sec = data[i].tvTime.tv_sec;
            g_dva_res[i].dvaobj_info.ttime.tv_usec = data[i].tvTime.tv_usec;

            if (ai_generic_data.type & IVCA_ET_GENERIC) strncpy(g_dva_res[i].dvaobj_info.title, ai_generic_data.title, sizeof(g_dva_res[i].dvaobj_info.title));

            memcpy(g_dva_res[i].dvaobj_info.caption, ai_generic_data.caption, sizeof(ai_generic_data.caption));
            memcpy(g_dva_res[i].dvaobj_info.title, ai_generic_data.title, sizeof(ai_generic_data.title));
            //g_dva_res[i].dvaobj_info.title[55] = "\0";
            memcpy(g_dva_res[i].dvaobj_info.description, ai_generic_data.description, sizeof(ai_generic_data.description));
            g_dva_res[i].dvaobj_info.top_x = MAX(ai_generic_data.event_area[0].x, 0);
            g_dva_res[i].dvaobj_info.top_y = MAX(ai_generic_data.event_area[0].y, 0);
            g_dva_res[i].dvaobj_info.bottom_x = MIN(ai_generic_data.event_area[1].x, 1);
            g_dva_res[i].dvaobj_info.bottom_y = MIN(ai_generic_data.event_area[1].y, 1);

            memset(&thumbnail_param, 0x00, sizeof(THUMBNAIL_PARAM_T));
            thumbnail_param.idx = i;
            thumbnail_param.ch = g_dva_res[i].dvaobj_info.ch;
            thumbnail_param.ttime.tv_sec = g_dva_res[i].dvaobj_info.ttime.tv_sec;
            thumbnail_param.ttime.tv_usec = g_dva_res[i].dvaobj_info.ttime.tv_usec;
            thumbnail_param.image_w = THUMBNAIL_SRC_W;
            thumbnail_param.image_h = THUMBNAIL_SRC_H;
            g_message("FUNC:%s TIT:%s TIT:%s DES:%s CAP:%s", __FUNCTION__, ai_generic_data.title, g_dva_res[i].dvaobj_info.title,  ai_generic_data.description, ai_generic_data.caption);
            wrk_run_msg(iwrk, IMSG_NONE, 0, 1, _dup_pointer(&thumbnail_param, sizeof(THUMBNAIL_PARAM_T)));
        }
    }
}

static void _update_builtin_idz_data(gint log_cnt, LOG_DATA_T data[THUMBNAIL_COUNT])
{
    float top_x = 0 , top_y = 0; 
    float bottom_x = 0, bottom_y = 0;
	gint i;

    for (i = 0; i < THUMBNAIL_COUNT; i++)
    {
        if (i < log_cnt)
        { 
            NF_DVA_LOG_DATA dva_data;
            THUMBNAIL_PARAM_T thumbnail_param;

            g_dva_res[i].res_log = 1;

            memset(&dva_data, 0x00, sizeof(NF_DVA_LOG_DATA));
            memcpy(&dva_data, data[i].p.cat_dva.binary, sizeof(NF_DVA_LOG_DATA));

            g_dva_res[i].dvaobj_info.ch = data[i].p.cat_dva.channel;
            g_dva_res[i].dvaobj_info.ttime.tv_sec = data[i].tvTime.tv_sec;
            g_dva_res[i].dvaobj_info.ttime.tv_usec = data[i].tvTime.tv_usec;

            strcpy(g_dva_res[i].dvaobj_info.rule_name, "intrusion");
            dvatext_translate_to_uxitem(dva_data.intrusion_detection.name, g_dva_res[i].dvaobj_info.class_name, sizeof(g_dva_res[i].dvaobj_info.class_name));
            g_dva_res[i].dvaobj_info.confidence = (gint)(dva_data.intrusion_detection.confidence*100);
            g_dva_res[i].dvaobj_info.top_x = MAX(dva_data.intrusion_detection.bbox.coords[0], 0);
            g_dva_res[i].dvaobj_info.top_y = MAX(dva_data.intrusion_detection.bbox.coords[1], 0);
            g_dva_res[i].dvaobj_info.bottom_x = MIN(dva_data.intrusion_detection.bbox.coords[2], 1);
            g_dva_res[i].dvaobj_info.bottom_y = MIN(dva_data.intrusion_detection.bbox.coords[3], 1); 

            memset(&thumbnail_param, 0x00, sizeof(THUMBNAIL_PARAM_T));
            thumbnail_param.idx = i;
            thumbnail_param.ch = g_dva_res[i].dvaobj_info.ch;
            thumbnail_param.ttime.tv_sec = g_dva_res[i].dvaobj_info.ttime.tv_sec;
            thumbnail_param.ttime.tv_usec = g_dva_res[i].dvaobj_info.ttime.tv_usec;
            thumbnail_param.image_w = THUMBNAIL_SRC_W;
            thumbnail_param.image_h = THUMBNAIL_SRC_H;

            wrk_run_msg(iwrk, IMSG_NONE, 0, 1, _dup_pointer(&thumbnail_param, sizeof(THUMBNAIL_PARAM_T)));
        }
    }
}

static void _update_builtin_ipz_data(gint log_cnt, LOG_DATA_T data[THUMBNAIL_COUNT])
{
    float top_x = 0 , top_y = 0; 
    float bottom_x = 0, bottom_y = 0;
	gint i;

    for (i = 0; i < THUMBNAIL_COUNT; i++)
    {
        if (i < log_cnt)
        {
            NF_DVA_LOG_DATA dva_data;
            THUMBNAIL_PARAM_T thumbnail_param;

            g_dva_res[i].res_log = 1;

            memset(&dva_data, 0x00, sizeof(NF_DVA_LOG_DATA));
            memcpy(&dva_data, data[i].p.cat_dva.binary, sizeof(NF_DVA_LOG_DATA));

            g_dva_res[i].dvaobj_info.ch = data[i].p.cat_dva.channel;
            g_dva_res[i].dvaobj_info.ttime.tv_sec = data[i].tvTime.tv_sec;
            g_dva_res[i].dvaobj_info.ttime.tv_usec = data[i].tvTime.tv_usec;

            strcpy(g_dva_res[i].dvaobj_info.rule_name, "illegal parking");
            dvatext_translate_to_uxitem(dva_data.illegal_parking.name, g_dva_res[i].dvaobj_info.class_name, sizeof(g_dva_res[i].dvaobj_info.class_name));
            g_dva_res[i].dvaobj_info.confidence = (gint)(dva_data.illegal_parking.confidence*100);
            g_dva_res[i].dvaobj_info.top_x = MAX(dva_data.illegal_parking.bbox.coords[0], 0);
            g_dva_res[i].dvaobj_info.top_y = MAX(dva_data.illegal_parking.bbox.coords[1], 0);
            g_dva_res[i].dvaobj_info.bottom_x = MIN(dva_data.illegal_parking.bbox.coords[2], 1);
            g_dva_res[i].dvaobj_info.bottom_y = MIN(dva_data.illegal_parking.bbox.coords[3], 1); 

            memset(&thumbnail_param, 0x00, sizeof(THUMBNAIL_PARAM_T));
            thumbnail_param.idx = i;
            thumbnail_param.ch = g_dva_res[i].dvaobj_info.ch;
            thumbnail_param.ttime.tv_sec = g_dva_res[i].dvaobj_info.ttime.tv_sec;
            thumbnail_param.ttime.tv_usec = g_dva_res[i].dvaobj_info.ttime.tv_usec;
            thumbnail_param.image_w = THUMBNAIL_SRC_W;
            thumbnail_param.image_h = THUMBNAIL_SRC_H;

            wrk_run_msg(iwrk, IMSG_NONE, 0, 1, _dup_pointer(&thumbnail_param, sizeof(THUMBNAIL_PARAM_T)));
        }
    }
}

static void _update_search_log_data(gint cnt, LOG_DATA_T data[THUMBNAIL_COUNT])
{
    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            _update_aibox_dtr_data(cnt, data);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            //problem
            _update_aibox_generic_data(cnt, data);
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            _update_builtin_idz_data(cnt, data);
        }
        else {
            _update_builtin_ipz_data(cnt, data);
        }
    }
}

static gboolean _set_preview_time(gpointer data)
{
	GTimeVal tmp;
	gchar strBuf[64];
	gchar *getBuf;

	memset(&tmp, 0x00, sizeof(GTimeVal));
	tmp = vsm_playback_get_previewtime();

	if (tmp.tv_sec != 0) dtf_get_local_datetime(tmp.tv_sec, strBuf);
	else g_sprintf(strBuf, "");

	getBuf = nfui_nflabel_get_text((NFLABEL*)g_play_time_obj);

	if (strcmp(getBuf, strBuf) != 0)
	{
		nfui_nflabel_set_text((NFLABEL*)g_play_time_obj, strBuf);
		nfui_signal_emit(g_play_time_obj, GDK_EXPOSE, TRUE);
	}

	return TRUE;
}

static gint _draw_thumbnail_info(DVA_RESULT_T *result)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    guint bg_color;

	GTimeVal detect_time;
	gchar strBuf[128];
    gint gap_x, gap_y;
	gint text_w, text_h;

    gint focus_state;
    gint select_state;

	GdkPixbuf *zoom_pixbuf;

    gint valid_cnt = 0;
    gint year, mon, day, hour, min, sec;

    focus_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(result->obj, "FOCUS_STATE"));
    select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(result->obj, "SELECT_STATE"));

    if (focus_state == 1) bg_color = 652;
    else if (select_state ==1) bg_color = 655;
    else bg_color = 649;

    nfui_nfobject_get_offset((NFOBJECT*)result->obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(result->obj);
    gc = nfui_nfobject_get_gc(result->obj);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_snprintf(strBuf, sizeof(strBuf), "CH%d", result->dvaobj_info.ch+1);

    text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_SMALL_NORMAL), strBuf, NORMAL_SPACING);
    text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_SMALL_NORMAL), strBuf, NORMAL_SPACING);
    
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
    nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, gap_x+result->obj->width-10-text_w, gap_y+2, text_w, text_h, 
        nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(651), NFALIGN_LEFT, 0);

    memset(strBuf, 0x00, sizeof(strBuf));
    dtf_get_local_datetime((time_t)result->dvaobj_info.ttime.tv_sec, strBuf);

	text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_SMALL_NORMAL), strBuf, NORMAL_SPACING);
	text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_SMALL_NORMAL), strBuf, NORMAL_SPACING);
    
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
	nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, gap_x+4, gap_y+2, text_w, text_h, 
		nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(651), NFALIGN_LEFT, 0);

    if (focus_state != 1 && select_state != 1) bg_color = 658;

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) != 0) {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_snprintf(strBuf, sizeof(strBuf), "%s", lookup_string(result->dvaobj_info.rule_name));

        text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);
        text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
        nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, gap_x+result->obj->width-10-text_w, gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT, text_w, text_h, 
            nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(651), NFALIGN_LEFT, 0);
    }
    else
    {
        int i, cnt = 0;
        memset(strBuf, 0x00, sizeof(strBuf));
        g_snprintf(strBuf, sizeof(strBuf), "%s", lookup_string(result->dvaobj_info.caption));

        text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);
        text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));

        valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, DVA_FRAME_WIDTH);
        nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+result->obj->width-5-text_w, gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT, text_w, text_h, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(651), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);
        
        memset(strBuf, 0x00, sizeof(strBuf));
        nfutil_get_line_feed_string(lookup_string(result->dvaobj_info.title), result->obj->width, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, sizeof(strBuf));

        text_w = nfutil_string_width(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);
        text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, NORMAL_SPACING);
        
        for (i=0; i<strlen(strBuf); i++)
        {
            if (strBuf[i] == '\n')  cnt++;
            if (cnt == 2)
            {
                strBuf[i] = '\0';
                break;
            }
        }
        if (text_h > 40) text_h = 40;

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(bg_color));
        
        nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, gap_x+result->obj->width-5-text_w, gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT+18, text_w, text_h, 
            nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(651), NFALIGN_LEFT, 0);
        
        #if 0
        valid_cnt = nfutil_string_get_valid_count(0, drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), strBuf, DVA_FRAME_WIDTH);
        nfutil_draw_short_text_eng(NULL, NULL, &UX_COLOR(bg_color), drawable, gc, strBuf, valid_cnt, gap_x+result->obj->width-5-text_w, gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT+20, text_w, text_h, 
				nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(651), NULL, NFALIGN_LEFT, 0, 0, NORMAL_SPACING);
        #endif
    }

    nfui_nfobject_gc_unref(gc);
    return 0;
}

static gint _draw_thumbnail_frame(DVA_RESULT_T *result)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

    GdkPixbuf *dst_pixbuf = NULL;
    GdkPixbuf *zoom_pixbuf = NULL;
    gint gap_x, gap_y;

    gint text_h;

    nfui_nfobject_get_offset((NFOBJECT*)result->obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(result->obj);
    gc = nfui_nfobject_get_gc(result->obj);

    if (result->res_thumb)
    {
        if (!result->dvaobj_info.fpixbuf) return 0;

        dst_pixbuf = _get_target_pixbuf(&result->dvaobj_info);

        gdk_draw_pixbuf(drawable, gc, dst_pixbuf, 0, 0, 
            gap_x+1+(DVA_FRAME_WIDTH-gdk_pixbuf_get_width(dst_pixbuf))/2, gap_y+THUMBNAIL_TITLE+(DVA_FRAME_HEIGHT-gdk_pixbuf_get_height(dst_pixbuf))/2, 
            -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

        // IMAGE LOAD
        zoom_pixbuf = nfui_get_image_from_file("search_zoom.png", NULL);
        gdk_draw_pixbuf(drawable, gc, zoom_pixbuf, 0, 0, gap_x+4, gap_y+THUMBNAIL_TITLE+5, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

        g_object_unref(dst_pixbuf);
    }
    else
    {
        text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), "NO IMAGE", NORMAL_SPACING);

        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(659));
        nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(658), drawable, gc, "NO IMAGE", gap_x, gap_y+THUMBNAIL_TITLE+(DVA_FRAME_HEIGHT-text_h)/2, DVA_FRAME_WIDTH, text_h, 
            nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(659), NFALIGN_CENTER, 0);        
    }

    nfui_nfobject_gc_unref(gc);
    return 0;
}
 
static gint _draw_thumbnail_object(DVA_RESULT_T *result)
{
    if (!result->res_log) return -1;

    _draw_thumbnail_info(result);
    _draw_thumbnail_frame(result);
    return 0;
}

static gint _draw_na_result(NFOBJECT *obj)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

    gint gap_x, gap_y;
    gint text_h;

    nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

    drawable = nfui_nfobject_get_window(obj);
    gc = nfui_nfobject_get_gc(obj);

    text_h = nfutil_string_height(drawable, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), "N/A", NORMAL_SPACING);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(659));
    nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(658), drawable, gc, "N/A", gap_x, gap_y+THUMBNAIL_TITLE+(DVA_FRAME_HEIGHT+THUMBNAIL_INFO-text_h)/2, DVA_FRAME_WIDTH, text_h, 
        nffont_get_pango_font(NFFONT_MINI_NORMAL_3), &UX_COLOR(659), NFALIGN_CENTER, 0);        

    nfui_nfobject_gc_unref(gc);    

    return 0;
}

static void _set_textbox_obj(gint expose)
{
    gchar *strKeyword = NULL;
    
    strKeyword = nfui_nflabel_get_text((NFLABEL*)g_aibox_generic_keyword[0]);

    if (strlen(strKeyword)) 
    {
        nfui_nfobject_enable(g_aibox_generic_keyword[1]);
        nfui_nfobject_enable(g_oper[0]);
    }
    else 
    {
        nfui_nfobject_disable(g_aibox_generic_keyword[1]);
        nfui_nfobject_disable(g_oper[0]);
    }

    if (!nfui_nfobject_is_disabled(g_aibox_generic_keyword[1])) 
    {
        strKeyword = nfui_nflabel_get_text((NFLABEL*)g_aibox_generic_keyword[1]);

        if (strlen(strKeyword)) 
        {
            nfui_nfobject_enable(g_aibox_generic_keyword[2]);
            nfui_nfobject_enable(g_oper[1]);
        }
        else 
        {
            nfui_nfobject_disable(g_aibox_generic_keyword[2]);
            nfui_nfobject_disable(g_oper[1]);
        }
    }
    else
    {
        nfui_nfobject_disable(g_aibox_generic_keyword[2]);
        nfui_nfobject_disable(g_oper[1]);
    }
    
    if (expose) {
        nfui_signal_emit(g_aibox_generic_keyword[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_generic_keyword[2], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_oper[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_oper[1], GDK_EXPOSE, TRUE);
    }
    
}

static int _change_dva_channel_filter(DLOGCTX log_ctx)
{
	gint i;
	gboolean state;
	unsigned int chmask = 0;
	int index;
	int lcat;

    for (i = 0; i < var_get_ch_count(); ++i)
    {
        if (nfui_check_button_get_active(NF_CHECKBUTTON(g_ch_obj[i]))) {
            dlogx_set_dlog_filter_ch(log_ctx, i, 1);
        }
        else {
            dlogx_set_dlog_filter_ch(log_ctx, i, 0);
        }
    }

	return 0;
}

static int _change_dva_engine_filter(DLOGCTX log_ctx)
{
    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            scm_set_dlog_filter_algorithm(log_ctx, 1 << DVA_AI_DETECTION);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            guint dva_type_mask = 0;
            //problem
            //scm_set_dlog_filter_algorithm(log_ctx, 1 << DVA_AI_GENERIC);
            dva_type_mask |= (1 << DVA_AI_GENERIC);
            dva_type_mask |= (1 << DVA_AI_DETECTION);
            scm_set_dlog_filter_algorithm(log_ctx, dva_type_mask);
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            scm_set_dlog_filter_algorithm(log_ctx, 1 << DVA_INTRUSION_DETECTION);
        }
        else {
            scm_set_dlog_filter_algorithm(log_ctx, 1 << DVA_ILLEGAL_PARKING);
        }
    }

	return 0;
}

static int _change_dva_rule_filter(DLOGCTX log_ctx)
{
    guint event_mask = 0;

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0)
        {
            if (g_opt_ai_dtr.evt.forward) event_mask |= IVCA_ET_DIR_POS;
            if (g_opt_ai_dtr.evt.reverse) event_mask |= IVCA_ET_DIR_NEG;
            if (g_opt_ai_dtr.evt.intrusion) event_mask |= IVCA_ET_INTRUSION;
            if (g_opt_ai_dtr.evt.removed) event_mask |= IVCA_ET_REMOVED;
            if (g_opt_ai_dtr.evt.loitering) event_mask |= IVCA_ET_LOITERED;
            if (g_opt_ai_dtr.evt.stopped) event_mask |= IVCA_ET_STOPPED;

            scm_set_dlog_filter_event(log_ctx, event_mask);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0)
        {
            //problem
            event_mask |= IVCA_ET_GENERIC;
            scm_set_dlog_filter_event(log_ctx, event_mask);
        }
    }

	return 0;
}

static int _change_dva_object_filter(DLOGCTX log_ctx)
{
    int i;
    gchar ux_searchstr[512];
    gchar eng_searchstr[512];
    gchar eng_searchstr1[512];
    gchar eng_searchstr2[512];
    gchar evt_searchstr[512];

    memset(ux_searchstr, 0x00, sizeof(ux_searchstr));
    memset(eng_searchstr, 0x00, sizeof(eng_searchstr));
    memset(eng_searchstr1, 0x00, sizeof(eng_searchstr1));
    memset(eng_searchstr2, 0x00, sizeof(eng_searchstr2));
    memset(evt_searchstr, 0x00, sizeof(evt_searchstr));

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0)
        {
            if (g_opt_ai_dtr.obj.human) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-strlen(eng_searchstr)-1, "%s", "person,");
            if (g_opt_ai_dtr.obj.car) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-strlen(eng_searchstr)-1, "%s", "car,");
            if (g_opt_ai_dtr.obj.bike) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-strlen(eng_searchstr)-1, "%s", "bike,");
            if (g_opt_ai_dtr.obj.custom) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-strlen(eng_searchstr)-1, "%s", g_opt_ai_dtr.obj.strcustom);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0)
        {
            //problem
            if (g_opt_ai_generic.text[0]) sprintf(eng_searchstr, "%s", g_opt_ai_generic.text[0]);
            if (g_opt_ai_generic.text[1]) sprintf(eng_searchstr1, "%s", g_opt_ai_generic.text[1]);
            if (g_opt_ai_generic.text[2]) sprintf(eng_searchstr2, "%s", g_opt_ai_generic.text[2]);
            if (g_opt_ai_generic.evt_type) sprintf(evt_searchstr, "%s", g_opt_ai_generic.evt_type);

            scm_set_dlog_filter_sub(log_ctx, g_opt_ai_generic.oper, g_opt_ai_generic.match_case, g_opt_ai_generic.match_whole);
            scm_set_dlog_filter_evt_text(log_ctx, g_opt_ai_generic.evt_type);
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0)
        {
            if (g_opt_builtin_idz.human) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-1, "%s", "person,");
            if (g_opt_builtin_idz.car) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-1, "%s", "car,");
            if (g_opt_builtin_idz.bike) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-1, "%s", "bike,");

        }
        else
        {
            if (g_opt_builtin_ipz.car) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-1, "%s", "car,");
            if (g_opt_builtin_ipz.bike) snprintf(eng_searchstr+strlen(eng_searchstr), sizeof(eng_searchstr)-1, "%s", "bike,");

        }

        //dvatext_translate_to_engitem(ux_searchstr, eng_searchstr, sizeof(eng_searchstr));
    }
    
    scm_set_dlog_filter_text(log_ctx, eng_searchstr, eng_searchstr1, eng_searchstr2);
	return 0;
}

static int _change_dva_search_filter(DLOGCTX log_ctx)
{
    scm_reset_dlog_filter(log_ctx);
	_change_dva_channel_filter(log_ctx);
    _change_dva_engine_filter(log_ctx);
    _change_dva_rule_filter(log_ctx);
    _change_dva_object_filter(log_ctx);
	return 0;
}

static gint _search_dva_log(DLOGCTX log_ctx)
{
	GTimeVal from_time;
	GTimeVal to_time;
    gboolean next = FALSE;

	LOG_DATA_T log_data[THUMBNAIL_COUNT];
	gint log_cnt;

    gchar strBuf[16];

	memset(&from_time, 0x00, sizeof(GTimeVal));
	memset(&to_time, 0x00, sizeof(GTimeVal));

	_dir_prev_button_disable();
	_playback_button_disable();
	_preview_obj_off("");
	vsm_playback_preview_stop();

	nfui_nftimelabel_get_datetime((NFTIMELABEL*)g_from_obj, &from_time);
	nfui_nftimelabel_get_datetime((NFTIMELABEL*)g_to_obj, &to_time);

	log_cnt = scm_get_dlog(log_ctx, &from_time, &to_time, THUMBNAIL_COUNT, log_data, &next);
    g_message("%s, %d, log_cnt:%d", __FUNCTION__, __LINE__, log_cnt);

	if (next) _dir_next_button_enable();
	else _dir_next_button_disable();

    _update_search_log_data(log_cnt, log_data);
    _set_search_log_rate(0);
    _set_search_log_cnt(log_cnt);
    _update_search_log_percent(0, log_cnt);

    return 0;
}

static gint _playback_start()
{
    gint type_idx, i;
    guint select_state = 0;

    guint ch_mask = 0;
    GTimeVal play_time = {0,};

    gint log_cnt = 0, log_rate = 0;

    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0)
        {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], "log cnt"));
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_DETECTOR], "log rate"));
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0)
        {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_GENERIC], "log cnt"));
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_aibox_rate_obj[IDX_AIBOX_GENERIC], "log rate"));
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0)
        {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], "log cnt"));
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_INTRUSION], "log rate"));
        }
        else
        {
            log_cnt = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], "log cnt"));
            log_rate = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING], "log rate"));
        }
    }

    if (log_cnt != log_rate) return -1;

    for (i = 0; i < THUMBNAIL_COUNT; i++)
    {
        select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
        if (select_state) {
            ch_mask = 1 << g_dva_res[i].dvaobj_info.ch;
            play_time.tv_sec = g_dva_res[i].dvaobj_info.ttime.tv_sec-1;
        }                
    }

    if (!ch_mask) return -1;

    vsm_playback_preview_stop();
    _preview_obj_off("");

    VW_Search_start_playback();
    vsm_playback_start(ch_mask, play_time, PLAYBACK_NORMAL);

    return 0;
}



////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_from_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	int x, y;
	NFOBJECT *top;

	GTimeVal from_tv;
	GTimeVal to_tv;
	GTimeVal temp_tv;

	memset(&temp_tv, 0x00, sizeof(GTimeVal));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

	if (evt->type == GDK_BUTTON_RELEASE)
	{
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x + obj->width + 4;
		y += top->y;

		nfui_nftimelabel_get_datetime(g_from_obj, &from_tv);
		nfui_nftimelabel_get_datetime(g_to_obj, &to_tv);
		to_tv.tv_sec -= 1;

        temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "FROM", x, y, from_tv.tv_sec, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, to_tv.tv_sec);

		if (temp_tv.tv_sec != 0)
		{
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)g_from_obj, &temp_tv);
			nfui_signal_emit((NFOBJECT*)g_from_obj, GDK_EXPOSE, TRUE);
		}

        if ((temp_tv.tv_sec != 0) && (from_tv.tv_sec != temp_tv.tv_sec))
		{
			_order_button_disable();
		}
	}

	return FALSE;
}

static gboolean post_to_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	int x, y;
	NFOBJECT *top;

	GTimeVal from_tv;
	GTimeVal to_tv;
	GTimeVal temp_tv;

	memset(&temp_tv, 0x00, sizeof(GTimeVal));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

	if (evt->type == GDK_BUTTON_RELEASE)
	{
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x + obj->width + 4;
		y += top->y;

		nfui_nftimelabel_get_datetime(g_from_obj, &from_tv);
		nfui_nftimelabel_get_datetime(g_to_obj, &to_tv);
		from_tv.tv_sec += 1;

        temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "TO", x, y, to_tv.tv_sec, SDT_TYPE_SEC, from_tv.tv_sec, NF_UPPER_TIMELIMIT);

		if (temp_tv.tv_sec != 0)
		{
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)g_to_obj, &temp_tv);
			nfui_signal_emit((NFOBJECT*)g_to_obj, GDK_EXPOSE, TRUE);
		}

        if ((temp_tv.tv_sec != 0) && (to_tv.tv_sec != temp_tv.tv_sec))
		{
			_order_button_disable();
		}
	}

	return FALSE;
}

static gboolean post_engine_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {   
        gint select_state;
        gint i;

        if (g_prev_engine == nfui_combobox_get_cur_index((NFCOMBOBOX*)obj)) return FALSE;

        wrk_clear_job(iwrk);

        nfui_combobox_remove_all((NFCOMBOBOX*)g_type_obj);

        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(obj)), STR_ENGINE_AIBOX) == 0) {
            nfui_nfobject_hide(g_type_obj);
            nfui_nflabel_set_text((NFLABEL*)g_type_label_obj, "");
            //nfui_combobox_append_data((NFCOMBOBOX*)g_type_obj, STR_TYPE_DETECTOR);
            nfui_combobox_append_data((NFCOMBOBOX*)g_type_obj, STR_TYPE_GENERIC);
        }
        else {
            nfui_nfobject_show(g_type_obj);
            nfui_nflabel_set_text((NFLABEL*)g_type_label_obj, "TYPE");
            nfui_combobox_append_data((NFCOMBOBOX*)g_type_obj, STR_TYPE_INTRUSION);
            nfui_combobox_append_data((NFCOMBOBOX*)g_type_obj, STR_TYPE_ILLEGAL_PARKING);
        }
        nfui_signal_emit((NFOBJECT*)g_type_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit((NFOBJECT*)g_type_label_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _dir_prev_button_disable();
        _dir_next_button_disable(); 
        _playback_button_disable();
        _change_dva_search_filter(g_log_ctx);

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
            g_dva_res[i].dvaobj_info.fpixbuf = 0;

            if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0)
                g_dva_res[i].obj = g_aibox_thumb_obj[IDX_AIBOX_DETECTOR][i];
            if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0)
            {
                g_dva_res[i].obj = g_aibox_thumb_obj[IDX_AIBOX_GENERIC][i];
            }
            else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0)
                g_dva_res[i].obj = g_builtin_thumb_obj[IDX_BUILTIN_INTRUSION][i];
            else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_ILLEGAL_PARKING) == 0)
                g_dva_res[i].obj = g_builtin_thumb_obj[IDX_BUILTIN_ILLEGAL_PARKING][i];

            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
            nfui_nfobject_disable(g_dva_res[i].obj);
            g_dva_res[i].res_log = 0;
            g_dva_res[i].res_thumb = 0;
            nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, TRUE);  
        }

        for (i = 0; i < AIBOX_TYPE_MAX; i++) {
            nfui_nfobject_hide(g_aibox_type_fixed[i]); 
        }

        for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
            nfui_nfobject_hide(g_builtin_type_fixed[i]); 
        }

        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            nfui_nfobject_show(g_aibox_type_fixed[IDX_AIBOX_DETECTOR]);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            nfui_nfobject_show(g_aibox_type_fixed[IDX_AIBOX_GENERIC]);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            nfui_nfobject_show(g_builtin_type_fixed[IDX_BUILTIN_INTRUSION]);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_ILLEGAL_PARKING) == 0) {
            nfui_nfobject_show(g_builtin_type_fixed[IDX_BUILTIN_ILLEGAL_PARKING]);
        }

        for (i = 0; i < AIBOX_TYPE_MAX; i++) {
            nfui_signal_emit(g_aibox_type_fixed[i], GDK_EXPOSE, TRUE);
        }

        for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
            nfui_signal_emit(g_builtin_type_fixed[i], GDK_EXPOSE, TRUE);
        }

        nfui_make_key_hierarchy(g_curwnd);
        g_prev_engine = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
    }

    return FALSE;
}

static gboolean post_detection_type_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {   
        gint select_state;
        gint i;

        wrk_clear_job(iwrk);

        _order_button_disable();
        _dir_prev_button_disable();
        _dir_next_button_disable(); 
        _playback_button_disable();
        _change_dva_search_filter(g_log_ctx);

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
            g_dva_res[i].dvaobj_info.fpixbuf = 0;

            if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0)
                g_dva_res[i].obj = g_aibox_thumb_obj[IDX_AIBOX_DETECTOR][i];
            else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0)
            {
                g_dva_res[i].obj = g_aibox_thumb_obj[IDX_AIBOX_GENERIC][i];
            }
            else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0)
                g_dva_res[i].obj = g_builtin_thumb_obj[IDX_BUILTIN_INTRUSION][i];
            else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_ILLEGAL_PARKING) == 0)
                g_dva_res[i].obj = g_builtin_thumb_obj[IDX_BUILTIN_ILLEGAL_PARKING][i];

            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
            nfui_nfobject_disable(g_dva_res[i].obj);
            g_dva_res[i].res_log = 0;
            g_dva_res[i].res_thumb = 0;
            nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, TRUE);  
        }

        for (i = 0; i < AIBOX_TYPE_MAX; i++) {
            nfui_nfobject_hide(g_aibox_type_fixed[i]); 
        }

        for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
            nfui_nfobject_hide(g_builtin_type_fixed[i]); 
        }

        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            nfui_nfobject_show(g_aibox_type_fixed[IDX_AIBOX_DETECTOR]);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            nfui_nfobject_show(g_aibox_type_fixed[IDX_AIBOX_GENERIC]);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            nfui_nfobject_show(g_builtin_type_fixed[IDX_BUILTIN_INTRUSION]);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_ILLEGAL_PARKING) == 0) {
            nfui_nfobject_show(g_builtin_type_fixed[IDX_BUILTIN_ILLEGAL_PARKING]);
        }

        for (i = 0; i < AIBOX_TYPE_MAX; i++) {
            nfui_signal_emit(g_aibox_type_fixed[i], GDK_EXPOSE, TRUE);
        }

        for (i = 0; i < BUILTIN_TYPE_MAX; i++) {
            nfui_signal_emit(g_builtin_type_fixed[i], GDK_EXPOSE, TRUE);
        }

        nfui_make_key_hierarchy(g_curwnd);
    }

    return FALSE;
}

static gboolean post_channel_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        gboolean state;
        gint i;
        guint log_ctx;
        
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (nfui_nfobject_is_disabled(g_ch_obj[i])) continue;

            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_ch_obj[i]), state);
            nfui_signal_emit(g_ch_obj[i], GDK_EXPOSE, TRUE);
        }
        _order_button_disable();
    	_change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_channel_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        _set_all_chk_btn(g_ch_all_obj, g_ch_obj, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_aibox_rule_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_forward_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj), TRUE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_enter_obj), TRUE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_exit_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_remove_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_stop_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_forward_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj), FALSE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_enter_obj), FALSE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_exit_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_remove_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_stop_obj), FALSE);
        }
        nfui_signal_emit(g_aibox_dtr_forward_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_reverse_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_intrusion_obj, GDK_EXPOSE, TRUE);
        //nfui_signal_emit(g_aibox_dtr_enter_obj, GDK_EXPOSE, TRUE);
        //nfui_signal_emit(g_aibox_dtr_exit_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_remove_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_loiter_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_stop_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_aibox_rule_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;

        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_forward_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj))) state = FALSE;
        //if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_enter_obj))) state = FALSE;
        //if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_exit_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_remove_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_stop_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_rule_obj), state);
        nfui_signal_emit(g_aibox_dtr_rule_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_aibox_object_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_human_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_car_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_bike_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_manual_check_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_human_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_car_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_bike_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_manual_check_obj), FALSE);
        }
        nfui_signal_emit(g_aibox_dtr_human_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_car_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_bike_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_manual_check_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_aibox_object_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;

        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_human_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_car_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_bike_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_manual_check_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_object_obj), state);
        nfui_signal_emit(g_aibox_dtr_object_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_aibox_object_filter_edit_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        gchar filter_string[256]; 
        gchar exception_string[64]; 

        memset(filter_string, 0x00, sizeof(filter_string));
        snprintf(filter_string, sizeof(filter_string), "%s", nfui_nflabel_get_text((NFLABEL*)g_aibox_manual_label_obj));

        memset(exception_string, 0x00, sizeof(exception_string));
        snprintf(exception_string, sizeof(exception_string), "%s", "person,car,bike,");

        vw_dvabx_object_filter_edit_popup_open(g_curwnd, 1300, 310, exception_string, filter_string, sizeof(filter_string)-sizeof(exception_string));

        nfui_nflabel_set_text((NFLABEL*)g_aibox_manual_label_obj, filter_string);
        nfui_signal_emit(g_aibox_manual_label_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_builtin_idz_human_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_builtin_idz_vehicle_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_car_obj), TRUE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_bus_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_bike_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_car_obj), FALSE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_bus_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_bike_obj), FALSE);
        }
        nfui_signal_emit(g_builtin_idz_car_obj, GDK_EXPOSE, TRUE);
        //nfui_signal_emit(g_builtin_idz_bus_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_builtin_idz_bike_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_builtin_idz_vehicle_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;
        
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_car_obj))) state = FALSE;
        //if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_bus_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_bike_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_vehicle_obj), state);
        nfui_signal_emit(g_builtin_idz_vehicle_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_builtin_idz_animal_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_builtin_ipz_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_car_obj), TRUE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_bus_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_bike_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_car_obj), FALSE);
            //nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_bus_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_bike_obj), FALSE);
        }
        nfui_signal_emit(g_builtin_ipz_car_obj, GDK_EXPOSE, TRUE);
        //nfui_signal_emit(g_builtin_ipz_bus_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_builtin_ipz_bike_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_builtin_ipz_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;
       
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_ipz_car_obj))) state = FALSE;
        //if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_ipz_bus_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_ipz_bike_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_ipz_all_obj), state);
        nfui_signal_emit(g_builtin_ipz_all_obj, GDK_EXPOSE, TRUE);

        _order_button_disable();
        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_dva_search_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
        gint select_state;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        wrk_clear_job(iwrk);

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
            g_dva_res[i].dvaobj_info.fpixbuf = 0;

            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
            nfui_nfobject_disable(g_dva_res[i].obj);
            g_dva_res[i].res_log = 0;
            g_dva_res[i].res_thumb = 0;
            nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, TRUE);  
        }

        _search_dva_log(g_log_ctx);
        _order_button_enable();
        _playback_button_disable();
	}

	return FALSE;
}

static gboolean post_dva_filter_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;
    gint ret = 0;
    gchar outbuf[512], buf[512];

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if (evt->type == GDK_BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
        {
            if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
                ret = VW_Search_By_Ai_Detector_Filter_Popup(g_curwnd, &g_opt_ai_dtr);
            }
            else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
                ret = VW_Search_By_Ai_Generic_Filter_Popup(g_curwnd, &g_opt_ai_generic);
            }
        }
        else
        {
            if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
                ret = VW_Search_By_Builtin_Idz_Filter_Popup(g_curwnd, &g_opt_builtin_idz);
            }
            else {
                ret = VW_Search_By_Builtin_Ipz_Filter_Popup(g_curwnd, &g_opt_builtin_ipz);
            }
        }

        if (ret)
        {
            memset(buf, 0x00, sizeof(buf));
            memset(outbuf, 0x00, sizeof(outbuf));

            //_make_od_filter_str(buf);

            //nfutil_get_line_feed_string(buf, g_aibox_dtr_filter_label_obj->width - 20, nffont_get_pango_font(NFFONT_SMALL_SEMI), outbuf, sizeof(outbuf));
            //nfui_nflabel_set_text((NFLABEL*)g_aibox_dtr_filter_label_obj, outbuf);
            //nfui_signal_emit(g_aibox_dtr_filter_label_obj, GDK_EXPOSE, TRUE);
        }

        _change_dva_search_filter(g_log_ctx);
    }

    return FALSE;
}

static gboolean post_label_keyword_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    guint x, y;
    gchar *strTemp = NULL;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}
	
    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 64, VKEY_NORMAL);

        if (strTemp)
        {
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            _set_textbox_obj(1);

            ifree(strTemp);
            strTemp = NULL;
        }
    }

    return FALSE;
}

static gboolean post_dva_order_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED)
	{
		gint i, index;
        gint select_state;

        wrk_clear_job(iwrk);

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
            g_dva_res[i].dvaobj_info.fpixbuf = 0;

            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
            nfui_nfobject_disable(g_dva_res[i].obj);
            g_dva_res[i].res_log = 0;
            g_dva_res[i].res_thumb = 0;
            nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, TRUE);              
        }

		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if (index == 0)
		{
			scm_set_dlog_filter_order(g_log_ctx, LF_LATEST);
			_search_dva_log(g_log_ctx);
		}
		else
		{
			scm_set_dlog_filter_order(g_log_ctx, LF_OLDEST);
			_search_dva_log(g_log_ctx);
		}
        _playback_button_disable();
	}

	return FALSE;
}

static gboolean post_dva_prev_log_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean prev = FALSE;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		LOG_DATA_T log_data[THUMBNAIL_COUNT];
		gint i, log_cnt;
        gchar strBuf[16];

        gint select_state;

        wrk_clear_job(iwrk);

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
            g_dva_res[i].dvaobj_info.fpixbuf = 0;

            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
            nfui_nfobject_disable(g_dva_res[i].obj);
            g_dva_res[i].res_log = 0;
            g_dva_res[i].res_thumb = 0;
            nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, TRUE);  
        }

		_dir_next_button_enable();
        _playback_button_disable();

		log_cnt = scm_get_dlog_prev(g_log_ctx, THUMBNAIL_COUNT, log_data, &prev);
		if (!prev) _dir_prev_button_disable();

        _update_search_log_data(log_cnt, log_data);
        _set_search_log_rate(0);
        _set_search_log_cnt(log_cnt);
        _update_search_log_percent(0, log_cnt);
	}

	return FALSE;
}

static gboolean post_dva_next_log_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean next = FALSE;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		LOG_DATA_T log_data[THUMBNAIL_COUNT];
		gint i, log_cnt;
        gchar strBuf[16];

        gint select_state;

        wrk_clear_job(iwrk);

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
            g_dva_res[i].dvaobj_info.fpixbuf = 0;

            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
            nfui_nfobject_disable(g_dva_res[i].obj);
            g_dva_res[i].res_log = 0;
            g_dva_res[i].res_thumb = 0;
            nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, TRUE);  
        }

		_dir_prev_button_enable();
        _playback_button_disable();

		log_cnt = scm_get_dlog_next(g_log_ctx, THUMBNAIL_COUNT, log_data, &next);
		if (!next) _dir_next_button_disable();

        _update_search_log_data(log_cnt, log_data);
        _set_search_log_rate(0);
        _set_search_log_cnt(log_cnt);
        _update_search_log_percent(0, log_cnt);         
	}

	return FALSE;
}

static gboolean post_dva_result_obj_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	switch (event->type) 
	{
		case GDK_EXPOSE:
		{
		    gint gap_x, gap_y;
            gint buff_idx;

            gint focus_state;
            gint select_state;

    		nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

            focus_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "FOCUS_STATE"));
            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "SELECT_STATE"));

            if (focus_state == 1) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(652));
            else if (select_state == 1) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(655));
            else gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(649));
			gdk_draw_rectangle(drawable, gc, TRUE, gap_x+1, gap_y+1, DVA_FRAME_WIDTH,THUMBNAIL_TITLE);

            if (focus_state == 1) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(652));
            else if (select_state == 1) gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(655));
            else gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(658));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x+1, gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT, DVA_FRAME_WIDTH,THUMBNAIL_INFO);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(658));
            gdk_draw_rectangle(drawable, gc, TRUE, gap_x+1, gap_y+THUMBNAIL_TITLE, DVA_FRAME_WIDTH, DVA_FRAME_HEIGHT);

            //gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(649));
            //gdk_draw_rectangle(drawable, gc, TRUE, gap_x+1, gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT, DVA_FRAME_WIDTH,THUMBNAIL_INFO);

            if (nfui_nfobject_is_disabled(obj)) {
                _draw_na_result(obj);
            }
            else {
                buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "RESULT_BUFF_IDX"));
                _draw_thumbnail_object(&g_dva_res[buff_idx]);
            }

			nfui_nfobject_gc_unref(gc);
		}
		break;

        case GDK_BUTTON_PRESS:
        {
            gint buff_idx = 0;
            gint shown_as;
            gint gap_x, gap_y;
            gint i;
            gint select_state;

            gint log_cnt, log_rate;
            GTimeVal timeval = {0,};  

            gint ret_play_start;     

            GdkEventButton *bevent;

            bevent = (GdkEventButton *)event;

            _preview_obj_off("N/A");
            vsm_playback_preview_stop();

            buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "RESULT_BUFF_IDX"));
            if (!g_dva_res[buff_idx].res_log) return FALSE;
            if (!g_dva_res[buff_idx].res_thumb) return FALSE;

            for (i = 0; i < THUMBNAIL_COUNT; i++)
            {
                select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
                if (select_state) {
                    nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
                    nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, FALSE);
                }
            }
            nfui_nfobject_set_data(obj, "SELECT_STATE", GUINT_TO_POINTER(1));
            nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

            log_cnt = _get_search_log_cnt();
            log_rate = _get_search_log_rate();

            if (log_cnt == log_rate) {
                _playback_button_enable();
            }

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            if ((bevent->y < gap_y+THUMBNAIL_TITLE+DVA_FRAME_HEIGHT) && (g_dva_res[buff_idx].dvaobj_info.fpixbuf)) 
            {
                DOBJECT_INFO_T dobj_info;
                gint pos_x, pos_y;

                if (bevent->x+670 > 1920) pos_x = bevent->x-670;
                else pos_x = bevent->x+10;

                if (bevent->y+635 > 1080) pos_y = 1080-635;
                else pos_y = bevent->y;

                memset(&dobj_info, 0x00, sizeof(dobj_info));
                dobj_info.ch = g_dva_res[buff_idx].dvaobj_info.ch;
                memcpy(dobj_info.class_name, g_dva_res[buff_idx].dvaobj_info.class_name, sizeof(dobj_info.class_name));
                memcpy(dobj_info.rule_name, g_dva_res[buff_idx].dvaobj_info.rule_name, sizeof(dobj_info.rule_name));
                dobj_info.ftime = g_dva_res[buff_idx].dvaobj_info.ttime.tv_sec;
                dobj_info.confidence = g_dva_res[buff_idx].dvaobj_info.confidence;
                dobj_info.coords[0] = g_dva_res[buff_idx].dvaobj_info.top_x;
                dobj_info.coords[1] = g_dva_res[buff_idx].dvaobj_info.top_y;
                dobj_info.coords[2] = g_dva_res[buff_idx].dvaobj_info.bottom_x;
                dobj_info.coords[3] = g_dva_res[buff_idx].dvaobj_info.bottom_y;
                if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
                    memcpy(dobj_info.caption, g_dva_res[buff_idx].dvaobj_info.caption, sizeof(g_dva_res[buff_idx].dvaobj_info.caption));
                    memcpy(dobj_info.title, g_dva_res[buff_idx].dvaobj_info.title, sizeof(g_dva_res[buff_idx].dvaobj_info.title));
                    memcpy(dobj_info.description, g_dva_res[buff_idx].dvaobj_info.description, sizeof(g_dva_res[buff_idx].dvaobj_info.description));
                }

                if (nfui_nfobject_is_disabled(g_playback_obj)) dobj_info.act_playbtn = 0;
                else dobj_info.act_playbtn = 1;

                ret_play_start = vw_deeplearning_image_popup_open(g_curwnd, pos_x, pos_y, &dobj_info, g_dva_res[buff_idx].dvaobj_info.fpixbuf);
                if (ret_play_start) _playback_start();
                return FALSE;
            }

            if (log_cnt != log_rate)
            {
                _preview_obj_off("Please wait...");
                return FALSE;
            }

            if (ssm_get_covert_mask() & (1 << g_dva_res[buff_idx].dvaobj_info.ch))
            {
                shown_as = ssm_get_covert_shown_as();

                if (!shown_as) _preview_obj_off("NO VIDEO");
                else _preview_obj_off("COVERT");
                return FALSE;
            }

            timeval.tv_sec = g_dva_res[buff_idx].dvaobj_info.ttime.tv_sec-1;

            nfui_nfobject_get_offset((NFOBJECT*)g_preview_obj, &gap_x, &gap_y);
            _preview_obj_on("");
            vsm_playback_preview_start((1 << g_dva_res[buff_idx].dvaobj_info.ch), 
                        timeval, gap_x, gap_y, g_preview_obj->width, g_preview_obj->height);
        }
        break;

        case GDK_2BUTTON_PRESS:
        {
            gint buff_idx = 0;
            gint select_state;
            gint i;

            _playback_button_disable();
            _preview_obj_off("N/A");
            vsm_playback_preview_stop();

            buff_idx = GPOINTER_TO_UINT(nfui_nfobject_get_data(obj, "RESULT_BUFF_IDX"));
            if (!g_dva_res[buff_idx].res_log) return FALSE;
            //if (!g_dva_res[buff_idx].res_thumb) return FALSE;

            for (i = 0; i < THUMBNAIL_COUNT; i++)
            {
                select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
                if (select_state) {
                    nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
                    nfui_signal_emit(g_dva_res[i].obj, GDK_EXPOSE, FALSE);
                }
            }
            nfui_nfobject_set_data(obj, "SELECT_STATE", GUINT_TO_POINTER(1));
            nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

            _playback_start();
        }
        break;

        case GDK_ENTER_NOTIFY:
        {
            nfui_nfobject_set_data(obj, "FOCUS_STATE", GUINT_TO_POINTER(1));  
            nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
        }
        break;

        case GDK_LEAVE_NOTIFY:
        {
            nfui_nfobject_set_data(obj, "FOCUS_STATE", GUINT_TO_POINTER(0));  
            nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
        }
        break;

		default:
		break;
	}

	return FALSE;
}

static gboolean post_playbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{        
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        _playback_start();
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vw_SearchByDeepLearning_tab_out_handler();

		vsm_playback_preview_stop();
		VW_Search_Destroy();
	}

	return FALSE;
}

static gboolean pre_channel_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y, -1, -1, NFALIGN_LEFT, 0);           
	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_thumbnail_result_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == INFY_THUMBNAIL_CMPL_OBJ2)
    {
        gint obj_idx = ((CMM_MESSAGE_T *)data)->param;
        guchar *image_buf = ((CMM_MESSAGE_T *)data)->data;
        gint i, log_cnt;

        gint shown_as;
        gint gap_x, gap_y;

        gint select_state;
        GTimeVal timeval = {0,};   

        if (nfui_nfobject_is_disabled(g_dva_res[0].obj) && (obj_idx != 0)) {
            return FALSE;
        }

        if (image_buf) 
        {
            GdkPixbuf *tmp_pixbuf = NULL;

            if (g_dva_res[obj_idx].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[obj_idx].dvaobj_info.fpixbuf);

            g_dva_res[obj_idx].dvaobj_info.fpixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, THUMBNAIL_SRC_W, THUMBNAIL_SRC_H);
            gdk_pixbuf_fill (g_dva_res[obj_idx].dvaobj_info.fpixbuf, 0x000000ff);

            tmp_pixbuf = gdk_pixbuf_new_from_data(image_buf, GDK_COLORSPACE_RGB, FALSE, 8, THUMBNAIL_SRC_W, THUMBNAIL_SRC_H, THUMBNAIL_SRC_W*3, NULL, NULL);
            gdk_pixbuf_copy_area(tmp_pixbuf, 0, 0, THUMBNAIL_SRC_W, THUMBNAIL_SRC_H, g_dva_res[obj_idx].dvaobj_info.fpixbuf, 0, 0); 

            if (tmp_pixbuf) g_object_unref(tmp_pixbuf);

            g_dva_res[obj_idx].res_thumb = 1;
        }
        else
        {
            g_dva_res[obj_idx].res_thumb = 0;
        }
        //g_dva_res[obj_idx].res_thumb = 1;
        nfui_nfobject_enable(g_dva_res[obj_idx].obj);
        nfui_signal_emit(g_dva_res[obj_idx].obj, GDK_EXPOSE, FALSE);

        _set_search_log_rate(obj_idx+1);
        log_cnt = _get_search_log_cnt();
        _update_search_log_percent(obj_idx+1, log_cnt);     

        // preview
        if (obj_idx + 1 != log_cnt) return FALSE;

        for (i = 0; i < THUMBNAIL_COUNT; i++)
        {
            select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
            if (select_state) break;
        }
        if (i >= THUMBNAIL_COUNT) return FALSE;

        if (g_dva_res[i].res_thumb == 1)
        {
            timeval.tv_sec = g_dva_res[i].dvaobj_info.ttime.tv_sec-1;

            nfui_nfobject_get_offset((NFOBJECT*)g_preview_obj, &gap_x, &gap_y);
            _preview_obj_on("");
            vsm_playback_preview_start((1 << g_dva_res[i].dvaobj_info.ch), 
                        timeval, gap_x, gap_y, g_preview_obj->width, g_preview_obj->height);  
            _playback_button_enable();
        }
        else
        {
            _preview_obj_off("N/A");
        }
    }
    else if(evt->type == GDK_DELETE)
	{
        uxm_unreg_imsg_event(obj, INFY_THUMBNAIL_CMPL_OBJ2);
	}

    return FALSE;
}

static gboolean post_fixed1_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
        gint i;

		case GDK_EXPOSE:
		{
			if (g_preview_timer_id == 0)
				g_preview_timer_id = g_timeout_add(500, _set_preview_time, NULL);
		}
		break;

		case GDK_DELETE:
		{
            for(i = 0; i < THUMBNAIL_COUNT; i++)
            {
                if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
                g_dva_res[i].dvaobj_info.fpixbuf = 0;
            }

			if(g_preview_timer_id != 0) {
				g_source_remove(g_preview_timer_id);
				g_preview_timer_id = 0;
			}

            scm_close_dlog_ctx(g_log_ctx);
            g_log_ctx = 0;
		}
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean pre_subgroup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h-20);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+20, -1, -1, NFALIGN_LEFT, 0);           
	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h-20);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean pre_subgroup2_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE:
		{
			gint gap_x, gap_y;

    		drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h-20);
            nfutil_draw_pixbuf(drawable, gc, pbuf, gap_x, gap_y+20, -1, -1, NFALIGN_LEFT, 0);

            gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(661)));
            gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
            gdk_draw_line(drawable, gc, gap_x+800, gap_y+50, gap_x+800, gap_y+size_h-20);

	 		nfui_nfobject_gc_unref(gc);
	 	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_TAB_SUB_GROUP02_BG, size_w, size_h-20);
        }
		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
        wrk_destroy_worker(iwrk);
		g_curwnd = 0;
	}

	return FALSE;
}

static void _set_search_btn_handler()
{
    if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_engine_obj)), STR_ENGINE_AIBOX) == 0)
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_DETECTOR) == 0) {
            nfui_regi_post_event_callback((NFOBJECT*)g_search_obj, post_dva_search_event_handler);
        }
        else if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_GENERIC) == 0) {
            nfui_regi_post_event_callback((NFOBJECT*)g_search_obj, post_dva_search_event_handler);
        }
    }
    else
    {
        if (strcmp(nfui_combobox_get_value(NF_COMBOBOX(g_type_obj)), STR_TYPE_INTRUSION) == 0) {
            nfui_regi_post_event_callback((NFOBJECT*)g_search_obj, post_dva_search_event_handler);
        }
        else {
            nfui_regi_post_event_callback((NFOBJECT*)g_search_obj, post_dva_search_event_handler);
        }
    }
}

/*
 :  search by deeplearning main_fixed.

	  --------------------------------------------
	  |  ---------------------------------------  |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|  FIXED1	|		  FIXED2			| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	|			|							| |
	  |	 ---------------------------------------  |
	  |											  |
	  |				PARENT						  |
	  |-------------------------------------------|
*/

////////////////////////////////////////////////////////////
//
// protected interfaces 
//

static gint _init_aibox_detector_fixed(NFOBJECT *dtype_fixed)
{
	NFOBJECT *option_fixed, *result_fixed;
	NFOBJECT *obj;

	const gchar *strCombo[] = {"LATEST", "OLDEST"};
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    gchar strBuf[64];

    gint pos_x, pos_y;
	gint size_w, size_h;
    gint btn_w, btn_h;
	gint i, j;

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	result_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(result_fixed, _FIXED2_W, _FIXED2_H);
	nfui_nfobject_modify_bg(result_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(result_fixed);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, result_fixed, 0, 0);
	nfui_regi_pre_event_callback(result_fixed, pre_subgroup_fixed_event_handler);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RESULT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)result_fixed, obj, 0, 0);

    pos_x = 12;
    pos_y = 48;

    for (i = 0; i < THUMBNAIL_LOW; i ++)
    {
        for (j = 0; j < THUMBNAIL_COL; j ++)
        {  
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(651));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, DVA_FRAME_WIDTH+2, DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(649)); 
            nfui_nfobject_disable(obj);
            nfui_nffixed_put((NFFIXED*)result_fixed, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);
            nfui_regi_post_event_callback(obj, post_dva_result_obj_event_handler);
            g_aibox_thumb_obj[IDX_AIBOX_DETECTOR][i*THUMBNAIL_COL+j] = obj;

            nfui_nfobject_set_data(obj, "RESULT_BUFF_IDX", GUINT_TO_POINTER(i*THUMBNAIL_COL+j));

            pos_x += (DVA_FRAME_WIDTH+2+3);
        }

        pos_x = 12; 
        pos_y += (DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO+3); 
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 20, _FIXED2_H-44);

    obj = nfui_combobox_new(strCombo, 2, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 200, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_order_event_handler);
    g_aibox_order_obj[IDX_AIBOX_DETECTOR] = obj;

//  LOG DIR
    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2-60-size_w, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_prev_log_event_handler);
    g_aibox_prev_obj[IDX_AIBOX_DETECTOR] = obj;

    nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2+60, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_next_log_event_handler);
    g_aibox_next_obj[IDX_AIBOX_DETECTOR] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("(0 / 0)", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(661));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 140, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W-144, _FIXED2_H-44);
    g_aibox_rate_obj[IDX_AIBOX_DETECTOR] = obj;

	return 0;
}

static gint _init_aibox_generic_fixed(NFOBJECT *dtype_fixed)
{
	NFOBJECT *option_fixed, *result_fixed;
	NFOBJECT *obj;

	const gchar *strCombo[] = {"LATEST", "OLDEST"};
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    gchar strBuf[64];
    gchar strTitle[64];

    gint pos_x, pos_y;
	gint size_w, size_h;
    gint btn_w, btn_h;
	gint i, j;

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	result_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(result_fixed, _FIXED2_W, _FIXED2_H);
	nfui_nfobject_modify_bg(result_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(result_fixed);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, result_fixed, 0, 0);
	nfui_regi_pre_event_callback(result_fixed, pre_subgroup_fixed_event_handler);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RESULT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)result_fixed, obj, 0, 0);

    pos_x = 12;
    pos_y = 48;

    for (i = 0; i < THUMBNAIL_LOW; i ++)
    {
        for (j = 0; j < THUMBNAIL_COL; j ++)
        {  
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(651));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, DVA_FRAME_WIDTH+2, DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(649)); 
            nfui_nfobject_disable(obj);
            nfui_nffixed_put((NFFIXED*)result_fixed, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);
            nfui_regi_post_event_callback(obj, post_dva_result_obj_event_handler);
            g_aibox_thumb_obj[IDX_AIBOX_GENERIC][i*THUMBNAIL_COL+j] = obj;

            nfui_nfobject_set_data(obj, "RESULT_BUFF_IDX", GUINT_TO_POINTER(i*THUMBNAIL_COL+j));

            pos_x += (DVA_FRAME_WIDTH+2+3);
        }

        pos_x = 12; 
        pos_y += (DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO+3); 
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 20, _FIXED2_H-44);

    obj = nfui_combobox_new(strCombo, 2, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 200, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_order_event_handler);
    g_aibox_order_obj[IDX_AIBOX_GENERIC] = obj;

//  LOG DIR
    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2-60-size_w, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_prev_log_event_handler);
    g_aibox_prev_obj[IDX_AIBOX_GENERIC] = obj;

    nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2+60, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_next_log_event_handler);
    g_aibox_next_obj[IDX_AIBOX_GENERIC] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("(0 / 0)", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(661));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 140, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W-144, _FIXED2_H-44);
    g_aibox_rate_obj[IDX_AIBOX_GENERIC] = obj;

	return 0;
}

static gint _init_builtin_intrusion_fixed(NFOBJECT *dtype_fixed)
{
	NFOBJECT *option_fixed, *result_fixed;
	NFOBJECT *obj;

	const gchar *strCombo[] = {"LATEST", "OLDEST"};
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    gchar strBuf[64];

    gint pos_x, pos_y;
	gint size_w, size_h;
    gint btn_w, btn_h;
	gint i, j;

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	result_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(result_fixed, _FIXED2_W, _FIXED2_H);
	nfui_nfobject_modify_bg(result_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(result_fixed);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, result_fixed, 0, 0);
	nfui_regi_pre_event_callback(result_fixed, pre_subgroup_fixed_event_handler);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RESULT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)result_fixed, obj, 0, 0);

    pos_x = 12;
    pos_y = 48;

    for (i = 0; i < THUMBNAIL_LOW; i ++)
    {
        for (j = 0; j < THUMBNAIL_COL; j ++)
        {  
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(651));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, DVA_FRAME_WIDTH+2, DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(649)); 
            nfui_nfobject_disable(obj);
            nfui_nffixed_put((NFFIXED*)result_fixed, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);
            nfui_regi_post_event_callback(obj, post_dva_result_obj_event_handler);
            g_builtin_thumb_obj[IDX_BUILTIN_INTRUSION][i*THUMBNAIL_COL+j] = obj;

            nfui_nfobject_set_data(obj, "RESULT_BUFF_IDX", GUINT_TO_POINTER(i*THUMBNAIL_COL+j));

            pos_x += (DVA_FRAME_WIDTH+2+3);
        }

        pos_x = 12; 
        pos_y += (DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO+3); 
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 20, _FIXED2_H-44);

    obj = nfui_combobox_new(strCombo, 2, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 200, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_order_event_handler);
    g_builtin_order_obj[IDX_BUILTIN_INTRUSION] = obj;

//  LOG DIR
    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2-60-size_w, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_prev_log_event_handler);
    g_builtin_prev_obj[IDX_BUILTIN_INTRUSION] = obj;

    nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2+60, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_next_log_event_handler);
    g_builtin_next_obj[IDX_BUILTIN_INTRUSION] = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("(0 / 0)", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(661));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 140, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W-144, _FIXED2_H-44);
    g_builtin_rate_obj[IDX_BUILTIN_INTRUSION] = obj;

	return 0;
}

static gint _init_builtin_illegal_parking_fixed(NFOBJECT *dtype_fixed)
{
	NFOBJECT *option_fixed, *result_fixed;
	NFOBJECT *obj;

	const gchar *strCombo[] = {"LATEST", "OLDEST"};
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];

    gchar strBuf[64];

    gint pos_x, pos_y;
	gint size_w, size_h;
    gint btn_w, btn_h;
	gint i, j;

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);


    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, MKB_TAB_SUBGROUP_TITLE_NAME, dtype_fixed->width);
    nf_ui_create_image_button_method(strBuf, dtype_fixed->width, IMG_TAB_SUBGROUP_TITLE_L, IMG_TAB_SUBGROUP_TITLE_M, IMG_TAB_SUBGROUP_TITLE_R);

	result_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(result_fixed, _FIXED2_W, _FIXED2_H);
	nfui_nfobject_modify_bg(result_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(result_fixed);
	nfui_nffixed_put((NFFIXED*)dtype_fixed, result_fixed, 0, 0);
	nfui_regi_pre_event_callback(result_fixed, pre_subgroup_fixed_event_handler);

	obj = nfui_nfimage_new(strBuf);
	nfui_nfimage_set_text((NFIMAGE*)obj, "RESULT");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)result_fixed, obj, 0, 0);

    pos_x = 12;
    pos_y = 48;

    for (i = 0; i < THUMBNAIL_LOW; i ++)
    {
        for (j = 0; j < THUMBNAIL_COL; j ++)
        {  
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_4), COLOR_IDX(651));
            nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
            nfui_nfobject_set_size(obj, DVA_FRAME_WIDTH+2, DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(649)); 
            nfui_nfobject_disable(obj);
            nfui_nffixed_put((NFFIXED*)result_fixed, obj, pos_x, pos_y);
            nfui_nfobject_show(obj);
            nfui_regi_post_event_callback(obj, post_dva_result_obj_event_handler);
            g_builtin_thumb_obj[IDX_BUILTIN_ILLEGAL_PARKING][i*THUMBNAIL_COL+j] = obj;

            nfui_nfobject_set_data(obj, "RESULT_BUFF_IDX", GUINT_TO_POINTER(i*THUMBNAIL_COL+j));

            pos_x += (DVA_FRAME_WIDTH+2+3);
        }

        pos_x = 12;
        pos_y += (DVA_FRAME_HEIGHT+THUMBNAIL_TITLE+THUMBNAIL_INFO+3); 
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ORDER BY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 20, _FIXED2_H-44);

    obj = nfui_combobox_new(strCombo, 2, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, 200, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_order_event_handler);
    g_builtin_order_obj[IDX_BUILTIN_ILLEGAL_PARKING] = obj;

//  LOG DIR
    nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2-60-size_w, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_prev_log_event_handler);
    g_builtin_prev_obj[IDX_BUILTIN_ILLEGAL_PARKING] = obj;

    nfui_get_pixbuf_size(next_img[0], &size_w, &size_h);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
    nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W/2+60, _FIXED2_H-44);
    nfui_regi_post_event_callback(obj, post_dva_next_log_event_handler);
    g_builtin_next_obj[IDX_BUILTIN_ILLEGAL_PARKING]  = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("(0 / 0)", nffont_get_pango_font(NFFONT_SMALL_NORMAL), COLOR_IDX(661));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 140, 40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)result_fixed, obj, _FIXED2_W-144, _FIXED2_H-44);
    g_builtin_rate_obj[IDX_BUILTIN_ILLEGAL_PARKING] = obj;

	return 0;
}




////////////////////////////////////////////////////////////
//
// public interfaces
//

void vw_init_SearchByDeepLearning_page(NFOBJECT *parent, time_t from_time, time_t to_time, gchar *data)
{
	NFOBJECT *content_fixed;
    NFOBJECT *scrolled_fixed;
	NFOBJECT *fixed1, *fixed2;
	NFOBJECT *obj;

   	NFOBJECT *channel_fixed;
    NFOBJECT *keyword_fixed;
    NFOBJECT *tmp_fixed;

	NFOBJECT *ntb;

	const gchar *strTitle1[] = {"FROM", "TO"};

	gchar strBuf[32];
	guint tbl_width[4];
	guint size_w, size_h;
	guint pos_x, pos_y;
	gint i;
    gint cnt;

    DateTimeData dtdata;
    guint tformat;

	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];


    g_curwnd = nfui_nfobject_get_top(parent);

	memset(&dtdata, 0x00, sizeof(DateTimeData));
	DAL_get_dateTime_data(&dtdata);
    DAL_get_dateTime_format(NULL, &tformat);

    _init_opt_filter(data);

// IMAGE LOAD
	datetime_img[0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	datetime_img[1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);
	datetime_img[2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);
	datetime_img[3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);


    _init_worker();


	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, _FIXED1_W, _FIXED1_H);
	nfui_nfobject_show(fixed1);
	nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED*)content_fixed, fixed1, _FIXED1_X, _FIXED1_Y);
    nfui_regi_pre_event_callback(fixed1, post_fixed1_event_handler);

//  FROM / TO.
    nfui_get_pixbuf_size(datetime_img[0], &size_w, &size_h);

    tbl_width[0] = 80;
    tbl_width[2] = size_w;
    tbl_width[1] = 410-tbl_width[0]-tbl_width[2];

    ntb = (NFOBJECT*)nfui_nftable_new(3, 2, 0, 2, tbl_width, 40);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)fixed1, ntb, 15, 34);

    for (i = 0; i < 2; i++)
    {
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font((gchar*)(strTitle1[i]), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj,  0, i);

        if (i == 0)
        {
            GTimeVal time = {0, 0};
            time.tv_sec = from_time;
            obj = (NFOBJECT*)nfui_nftimelabel_new();
            nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(129));
            nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(128));
            nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
            nfui_nftimelabel_set_size((NFTIMELABEL*)obj, tbl_width[1], 40);
            nfui_nftimelabel_set_datetime((NFTIMELABEL*)obj, &time);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)ntb, obj,  1, i);
            g_from_obj = obj;
        }
        else
        {
            GTimeVal time = {0, 0};
            time.tv_sec = to_time;
            obj = (NFOBJECT*)nfui_nftimelabel_new();
            nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(129));
            nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(128));
            nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
            nfui_nftimelabel_set_size((NFTIMELABEL*)obj, tbl_width[1], 40);
            nfui_nftimelabel_set_datetime((NFTIMELABEL*)obj, &time);
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_show(obj);
            nfui_nftable_attach((NFTABLE*)ntb, obj,  1, i);
            g_to_obj = obj;
        }

        obj = (NFOBJECT*)nfui_nfbutton_new();
        nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
        nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)ntb, obj,  2, i);

        if (i == 0) {
            nfui_regi_post_event_callback(obj, post_from_event_handler);
        }
        else {
            nfui_regi_post_event_callback(obj, post_to_event_handler);
        }
    }

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ENGINE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 80, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, 15, 156);

	obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 330, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, 97, 156);
	nfui_regi_post_event_callback(obj, post_engine_combo_event_handler);
    g_engine_obj = obj;

    if (ivsc.dfunc.support_aibox_itx || ivsc.dfunc.support_aicam_itx) nfui_combobox_append_data((NFCOMBOBOX*)obj, STR_ENGINE_AIBOX);
    if (ivsc.dfunc.support_dlva_itx) nfui_combobox_append_data((NFCOMBOBOX*)obj, STR_ENGINE_BUILTIN);

    g_prev_engine = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_engine_obj);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 80, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, 15, 198);
    g_type_label_obj = obj;

	obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 330, 40);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, 97, 198);
	nfui_regi_post_event_callback(obj, post_detection_type_combo_event_handler);
    g_type_obj = obj;

    if (strcmp(nfui_combobox_get_value((NFCOMBOBOX*)g_engine_obj), STR_ENGINE_AIBOX) == 0){
        //nfui_combobox_append_data((NFCOMBOBOX*)obj, STR_TYPE_DETECTOR);
        nfui_combobox_append_data((NFCOMBOBOX*)g_type_obj, STR_TYPE_GENERIC);
    }
    else
    {
        nfui_nfobject_show(g_type_obj);
        nfui_nflabel_set_text((NFLABEL*)g_type_label_obj, "TYPE");
        nfui_combobox_append_data((NFCOMBOBOX*)obj, STR_TYPE_INTRUSION);
        nfui_combobox_append_data((NFCOMBOBOX*)obj, STR_TYPE_ILLEGAL_PARKING);
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 180, 26);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, 15, 262);

    channel_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(channel_fixed, 417, (60 + (4*38)));
    nfui_nfobject_modify_bg(channel_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_show(channel_fixed);
    nfui_nffixed_put((NFFIXED*)fixed1, channel_fixed, 11, 292);
	nfui_regi_pre_event_callback(channel_fixed, pre_channel_fixed_event_handler);

    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &size_w, &size_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)channel_fixed, obj, 10+2, 10+(38-size_h)/2);
    nfui_regi_post_event_callback(obj, post_channel_all_event_handler);
    g_ch_all_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(obj, 120, 38);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)channel_fixed, obj, 10+2+size_w+2, 10);

    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
    nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, 40, 0, 0);
    nfui_nfscrolledfixed_set_vscroll_offset((NFSCROLLEDFIXED*)scrolled_fixed, 0);
    nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
    nfui_nfobject_set_size(scrolled_fixed, 417 - 20, 4 * 40);
    nfui_nfobject_show(scrolled_fixed);
    nfui_nffixed_put((NFFIXED*)channel_fixed, scrolled_fixed, 10+2, 5 + 38 + 2);

#if 1
    pos_y = 0;
    cnt = 0;
    
    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_show(tmp_fixed);
        nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
        nfui_nfobject_set_size(tmp_fixed, 417/4-4, 40);
        nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, tmp_fixed, (((417/4)+15) * cnt), pos_y);

        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 0, (tmp_fixed->height-size_h)/2);
        nfui_regi_post_event_callback(obj, post_channel_group_event_handler);
        g_ch_obj[i] = obj;

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "CH%02d", i+1);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
        nfui_nfobject_set_size(obj, tmp_fixed->width-size_w-6, 38);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, size_w+2, 0);

        cnt++;
        if (cnt == 3) {
            pos_y += 40;
            cnt = 0;
        }
    }
#else
    tbl_width[0] = channel_fixed->width/4-4;
    tbl_width[1] = channel_fixed->width/4-4;
    tbl_width[2] = channel_fixed->width/4-4;
    tbl_width[3] = channel_fixed->width/4-4;

    ntb = (NFOBJECT*)nfui_nftable_new(4, GUI_CHANNEL_CNT/4, 0, 1, tbl_width, 38);
    nfui_nfobject_show(ntb);
    nfui_nffixed_put((NFFIXED*)scrolled_fixed, ntb, 10, 48);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        tmp_fixed = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_show(tmp_fixed);
        nfui_nftable_attach((NFTABLE*)ntb, tmp_fixed, i%4, i/4);

        obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
        nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
        nfui_check_get_size(obj, &size_w, &size_h);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 2, (tmp_fixed->height-size_h)/2);
        nfui_regi_post_event_callback(obj, post_channel_group_event_handler);
        g_ch_obj[i] = obj;

        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "CH%02d", i+1);

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(122));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(194));
        nfui_nfobject_set_size(obj, tmp_fixed->width-size_w-6, 38);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, 2+size_w+2, 0);
    }
#endif
    obj = nftool_normal_button_create_type3("FILTER", 103);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, 15, channel_fixed->y + channel_fixed->height + 5);
    nfui_regi_post_event_callback(obj, post_dva_filter_event_handler);

    obj = nftool_normal_button_create_type3("SEARCH", 103);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, channel_fixed->x + channel_fixed->width - 103, channel_fixed->y + channel_fixed->height + 5);
    nfui_regi_post_event_callback(obj, post_dva_search_event_handler);
    g_search_obj = obj;

    _set_search_btn_handler();

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PREVIEW", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(662));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(obj, 220, 26);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, 16, content_fixed->height-316);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(665));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));
    nfui_nfobject_set_size(obj, 408, 228);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, 16, content_fixed->height-290);
    g_preview_obj = obj;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(664));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 408, 30);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, 16, content_fixed->height-58);
    g_play_time_obj = obj;

    _update_covert_info();

    for (i = 0; i < AIBOX_TYPE_MAX; i++)
    {
        fixed2 = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed2, _FIXED2_W, _FIXED2_H);
        nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, _FIXED2_X, _FIXED2_Y);
        nfui_regi_post_event_callback(fixed2, post_thumbnail_result_fixed_event_handler);
        g_aibox_type_fixed[i] = fixed2;

        uxm_reg_imsg_event(fixed2, INFY_THUMBNAIL_CMPL_OBJ2);

        if (i == IDX_AIBOX_DETECTOR) _init_aibox_detector_fixed(fixed2);
        else if (i == IDX_AIBOX_GENERIC) _init_aibox_generic_fixed(fixed2);
    }     

    for (i = 0; i < BUILTIN_TYPE_MAX; i++)
    {
        fixed2 = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_set_size(fixed2, _FIXED2_W, _FIXED2_H);
        nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nffixed_put((NFFIXED*)content_fixed, fixed2, _FIXED2_X, _FIXED2_Y);
        nfui_regi_post_event_callback(fixed2, post_thumbnail_result_fixed_event_handler);
        g_builtin_type_fixed[i] = fixed2;

        uxm_reg_imsg_event(fixed2, INFY_THUMBNAIL_CMPL_OBJ2);

        if (i == IDX_BUILTIN_INTRUSION) _init_builtin_intrusion_fixed(fixed2);
        else if (i == IDX_BUILTIN_ILLEGAL_PARKING) _init_builtin_illegal_parking_fixed(fixed2);
    } 

    if(ivsc.vendor_code != 28 && ivsc.vendor_code != 128)
    {
        for (i = 0; i < THUMBNAIL_COUNT; i ++) {  
            g_dva_res[i].obj = g_aibox_thumb_obj[IDX_AIBOX_GENERIC][i];
        }
    }
    else
    {
        for (i = 0; i < THUMBNAIL_COUNT; i ++) {  
            g_dva_res[i].obj = g_builtin_thumb_obj[IDX_BUILTIN_INTRUSION][i];
        }
    }

    if(ivsc.vendor_code != 28 && ivsc.vendor_code != 128)
        nfui_nfobject_show(g_aibox_type_fixed[IDX_AIBOX_GENERIC]);
    else
        nfui_nfobject_show(g_builtin_type_fixed[IDX_BUILTIN_INTRUSION]);

    obj = nftool_normal_button_create_type1("PLAYBACK", MENU_BTN_WIDTH);
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, post_playbutton_event_handler);
    g_playback_obj = obj;

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_log_ctx = scm_open_dlog_ctx();
    _change_dva_search_filter(g_log_ctx);
}

gboolean vw_SearchByDeepLearning_tab_in_handler()
{

	return FALSE;
}

gboolean vw_SearchByDeepLearning_tab_out_handler()
{
    gint i;
    gint select_state;

    wrk_clear_job(iwrk);

    for (i = 0; i < THUMBNAIL_COUNT; i++)
    {
        if (g_dva_res[i].dvaobj_info.fpixbuf) g_object_unref(g_dva_res[i].dvaobj_info.fpixbuf);
        g_dva_res[i].dvaobj_info.fpixbuf = 0;

        select_state = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_dva_res[i].obj, "SELECT_STATE"));
        nfui_nfobject_set_data(g_dva_res[i].obj, "SELECT_STATE", GUINT_TO_POINTER(0));
        nfui_nfobject_disable(g_dva_res[i].obj);
        g_dva_res[i].res_log = 0;
        g_dva_res[i].res_thumb = 0;
    }

	_preview_obj_off("");
	vsm_playback_preview_stop();    
	return FALSE;
}

gboolean vw_SearchByDeepLearning_tab_show()
{
	vw_SearchByDeepLearning_tab_in_handler();
	return FALSE;
}

gboolean vw_SearchByDeepLearning_set_interval(time_t from_time, time_t to_time)
{
	GTimeVal tmp;
	memset(&tmp, 0x00, sizeof(GTimeVal));

	tmp.tv_sec = from_time;
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)g_from_obj, &tmp);

	tmp.tv_sec = to_time;
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)g_to_obj, &tmp);

	return FALSE;
}
