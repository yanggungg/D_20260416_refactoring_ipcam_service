// vim:ts=4:sw=4
/*******************************************************************************
*  (c) COPYRIGHT 2012 ITX security                                             *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
*  Jongbin Yim,  jongbina@itxsecurity.com                                      *
********************************************************************************

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
2012/07/12 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_search_by_smart.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "services/vsm.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nfobject.h"
#include "objects/nftable.h"
#include "objects/nftimelabel.h"
#include "objects/nfwindow.h"
#include "uxm.h"
#include "vw_search_by_smart.h"
#include "vw_search_main.h"
#include "vw_set_date_time.h"
#include "vw_tools.h"
#include "vw_vca_option_config.h"
#include "vw_vca_rule_config.h"
#include "vw_vca_internal.h"
#include "vw_vca_detail_search.h"

#include "nf_api_play.h"
#include "nf_meta_data.h"
#include "dtf.h"


#include "ivca_def.h"
#include "libivcam.h"

#define USE_IVCAM

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/* Fixed. */
#define	_FIXED1_X		0
#define	_FIXED1_Y		0
#define	_FIXED1_W		1060
#define	_FIXED1_H		903

#define	_FIXED2_X		(_FIXED1_X + _FIXED1_W)
#define	_FIXED2_Y		_FIXED1_Y
#define	_FIXED2_W		796
#define	_FIXED2_H		_FIXED1_H

#define	_LBL_H1			40
#define	_LBL_H2			26

#define	_SCBTN_W		25
#define	_SCBTN_H		25

/* Mainview. */
#define	_MNVD_LBL_X		((_FIXED1_W - _MNVD_LBL_W - MENU_V_INNER_X) / 2)
#define	_MNVD_LBL_Y		0
#define	_MNVD_LBL_W		960
#define	_MNVD_LBL_H		540

/* From/to. */
#define	_TIME_COLS		3
#define	_TIME_ROWS		2
#define	_TIME_TBL_X		(13 + 10)
#define	_TIME_TBL_Y		(_MNVD_LBL_Y + _MNVD_LBL_H + 10 + 10)
#define	_TIME_TBL_W		410

/* Channel. */
#define	_CHAN_LBL_X		_TIME_TBL_X
#define	_CHAN_LBL_Y		(_TIME_TBL_Y + _LBL_H1 * _TIME_ROWS + 2 + 10)
#define	_CHAN_LBL_W		(_TIME_TBL_W - _CHAN_CMB_W)

#define	_CHAN_CMB_X		(_CHAN_LBL_X + _CHAN_LBL_W)
#define	_CHAN_CMB_W		203

/* Option button. */
#define	_OPTI_BTN_X		(_TIME_TBL_X + _TIME_TBL_W - 203)
#define	_OPTI_BTN_Y		(_CHAN_LBL_Y + _LBL_H1 + 10)

/* RUle label. */
#define	_RULE_LBL_X		(_TIME_TBL_X + _TIME_TBL_W + 30)
#define	_RULE_LBL_Y		_TIME_TBL_Y
#define	_RULE_LBL_W		(_RULE_TBL_W + 25 - _RULE_CMB_W)

/* Rule combo. */
#define	_RULE_CMB_X		(_RULE_LBL_X + _RULE_LBL_W)
#define	_RULE_CMB_Y		_RULE_LBL_Y
#define	_RULE_CMB_W		203

/* Rule table. */
#define	_RULE_COLS		5
#define	_RULE_ROWS		7
#define	_RULE_TBL_X		_RULE_LBL_X
#define	_RULE_TBL_Y		(_RULE_LBL_Y + _LBL_H1 + 5)
#define	_RULE_TBL_W		530
#define	_RULE_TBL_H		((22 + 1) * (_RULE_ROWS + 1) - 1)

/* Rule table scroll bar. */
#define	_RULE_VSC_X		(_RULE_TBL_X + _RULE_TBL_W)
#define	_RULE_VSC_Y		_RULE_TBL_Y

/* Add button. */
#define	_EDIT_BTN_W		160
#define	_RADD_BTN_X		(_RDEL_BTN_X - _EDIT_BTN_W - 5)
#define	_RADD_BTN_Y		(_RULE_TBL_Y + _RULE_TBL_H + 5)

/* Delete button. */
#define	_RDEL_BTN_X		(_RMOD_BTN_X - _EDIT_BTN_W - 5)
#define	_RDEL_BTN_Y		_RADD_BTN_Y

/* Modify button. */
#define	_RMOD_BTN_X		(_RULE_CMB_X + _RULE_CMB_W - _EDIT_BTN_W)
#define	_RMOD_BTN_Y		_RDEL_BTN_Y

/* Search button. */
#define	_SRCH_BTN_X		(_STOP_BTN_X - 203 - 10)
#define	_SRCH_BTN_Y		(_FIXED1_H - _LBL_H1)

/* Stop button. */
#define	_STOP_BTN_X		(_RULE_CMB_X + _RULE_CMB_W - 203)
#define	_STOP_BTN_Y		_SRCH_BTN_Y

/* Mainview time & progress. */
#define	_MVTM_LBL_X		_TIME_TBL_X//0
#define	_MVTM_LBL_Y		_OPTI_BTN_Y + 100//_SRCH_BTN_Y
#define	_MVTM_LBL_W		400

/* Log table. */
#define	_LOGT_COLS		5
#define	_LOGT_ROWS		19
#define	_LOGT_TBL_X		30
#define	_LOGT_TBL_Y		0
#define	_LOGT_TBL_W		725
#define	_LOGT_TBL_H		((25 + 1) * (_LOGT_ROWS + 1) - 1)

/* Log table scroll bar. */
#define	_LOGT_VSC_X		(_LOGT_TBL_X + _LOGT_TBL_W)
#define	_LOGT_VSC_Y		_LOGT_TBL_Y


/* Search Result */
#define	_RES_COLS		4
#define	_RES_ROWS		7
#define	_RES_TBL_X		_LOGT_TBL_X
#define	_RES_TBL_Y		560
#define	_RES_TBL_W		305
#define	_RES_TBL_H		((22 + 1) * (_RES_ROWS + 1) - 1)

/* Search Result scroll bar. */
#define	_RES_VSC_X		(_RES_TBL_X + _RES_TBL_W)
#define	_RES_VSC_Y		_RES_TBL_Y

/* Log count. */
#define	_LOGC_LBL_X		_LOGT_TBL_X
#define	_LOGC_LBL_Y		(_LOGT_TBL_Y + _LOGT_TBL_H)
#define	_LOGC_LBL_W		250

/* Log total count. */
#define	_LOGTC_LBL_X		_LOGT_TBL_X
#define	_LOGTC_LBL_Y		_RES_TBL_Y + _RES_TBL_H + 40//(_LOGT_TBL_Y + _LOGT_TBL_H)
#define	_LOGTC_LBL_W		250

/* Preview. */
#define	_PRVW_LBL_X		_LOGT_TBL_X
#define	_PRVW_LBL_Y		550
#define	_PRVW_LBL_W		170

#define	_PRVD_LBL_X		_PRVW_LBL_X
#define	_PRVD_LBL_Y		(_PRVW_LBL_Y + _LBL_H2 + 2)
#define	_PRVD_LBL_W		410
#define	_PRVD_LBL_H		230

#define	_PRTM_LBL_X		_PRVD_LBL_X
#define	_PRTM_LBL_Y		(_PRVD_LBL_Y + _PRVD_LBL_H + 4)
#define	_PRTM_LBL_W		_PRVD_LBL_W

/* Snapshot. */
#define	_SNAP_LBL_X		(_PRVW_LBL_X + _PRVD_LBL_W + 20)
#define	_SNAP_LBL_Y		_PRVW_LBL_Y
#define	_SNAP_LBL_W		220

#define	_SNIM_LBL_X		_SNAP_LBL_X
#define	_SNIM_LBL_Y		(_SNAP_LBL_Y + _LBL_H2 + 2)
#define	_SNIM_LBL_W		200
#define	_SNIM_LBL_H		160

/* Log order. */
#define	_LOGO_LBL_X		_LOGT_TBL_X
#define	_LOGO_LBL_Y		(_FIXED1_H - _LBL_H1)
#define	_LOGO_LBL_W		180

#define	_LOGO_CMB_X		(_LOGO_LBL_X + _LOGO_LBL_W)
#define	_LOGO_CMB_Y		_LOGO_LBL_Y
#define	_LOGO_CMB_W		260

/* Log dir. */
#define	_LOGD_BTN_W		84
#define	_LDTX_LBL_W		120

#define	_LOGP_BTN_X		(_LOGO_LBL_X + 500)
#define	_LOGP_BTN_Y		(_LOGO_LBL_Y)
#define	_LOGN_BTN_X		(_LOGP_BTN_X + _LOGD_BTN_W + _LDTX_LBL_W)
#define	_LOGN_BTN_Y		(_LOGO_LBL_Y)

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,
	NUM_DATE_FORMATS,
};

typedef enum {
	LOGLABEL_TYPE = 0,
	LOGLABEL_TIME,
	LOGLABEL_CONTENTS,
	LOGLABEL_ALL
} LOGLABEL_E;

typedef struct {
	NFOBJECT *btn_up;
	NFOBJECT *btn_dn;
	NFOBJECT *lb_top;
	NFOBJECT *lb_mid;
	NFOBJECT *lb_bot;
	gint nrows;
	gint x;
	gint y;
	gint w;
	gint h;			/* Height of the scroll bar including buttons. */
	gint bw;
	gint bh;
	gint sy;
	gint sh;		/* Height of the scroll bar excluding buttons. */
	gint hv;
} vsc_t;

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static gboolean drawing_enable = FALSE;

static NFWINDOW *g_curwnd = NULL;

static NFOBJECT *fixed1, *fixed2;

static NFOBJECT *lb_mnvd;

static NFOBJECT *tlb_from;
static NFOBJECT *tlb_to;

static NFOBJECT *btn_from;
static NFOBJECT *btn_to;
static NFOBJECT *cmb_ch;
static NFOBJECT *btn_option;
static NFOBJECT *cmb_rt;
static NFOBJECT *tbl_rule;
static NFOBJECT *lb_ruletbl[_RULE_ROWS][_RULE_COLS];
static vsc_t vsc_rule;

static NFOBJECT *tbl_search_result;
static NFOBJECT *lb_search_result[_RES_ROWS][_RES_COLS];
static vsc_t vsc_result;
static int events[IVCA_MAX_ZONES];
static NFOBJECT *btn_reset_result;
static NFOBJECT *btn_detail_search;

static NFOBJECT *btn_add;
static NFOBJECT *btn_del;
static NFOBJECT *btn_mod;
static NFOBJECT *btn_search;
static NFOBJECT *btn_stop;
static NFOBJECT *lb_mnvwtime;

static NFOBJECT *lb_logtbl[_LOGT_ROWS][_LOGT_COLS];
static vsc_t vsc_log;

static NFOBJECT *lb_logcnt;
static NFOBJECT *lb_total_logcnt;

static NFOBJECT *lb_prvd;
static NFOBJECT *lb_prvwtime;

static NFOBJECT *lb_snapshot;

static NFOBJECT *cmb_order;
static NFOBJECT *btn_prev, *btn_next;

static NFOBJECT *btn_play;
static NFOBJECT *btn_close;

static gint g_channel;
static gint g_rt;
static gint g_order;

//static VCAOptGBLData _vogd;
static ivca_option_t _vod;
static ivca_option_t _vod_p;
static ivca_rule_t _vrd;
static gint _cntv[IVCA_MAX_CNTRS];

//static VCAOptGBLData _vogd_p;
static ivca_rule_t _vrd_p;
static gint _cntv_p[IVCA_MAX_CNTRS];

static gint g_sel_ridx;		/* Selected rule index. */
static gint g_sel_pidx;		/* Selected point index. */
static gint g_sel_ridx_p;
static gint g_sel_pidx_p;
static gint instant_erase;

static gint g_sel_x_p, g_sel_y_p;

static gchar *g_rule_font;
static gint g_rx, g_ry;		/* Ratio of normalized/display image size. */
static gint g_width, g_height;		/* Processing image size. */
static gint g_R_width, g_R_height;	/* Rounded processing image size. */

#define	NOT_SELECTED	-100000

static gint g_beg_ruletbl;
static gint g_sel_ruletbl;
static gint g_foc_ruletbl;

static gint g_beg_restbl;
static gint g_sel_restbl;
static gint g_foc_restbl;

static gint g_beg_logtbl;
static gint g_sel_logtbl;
static gint g_foc_logtbl;
static gint g_idx_logtbl[_LOGT_ROWS];

/* Variables for track information. */
static volatile gint g_ti_count;
static gint g_ti_count_p;
static ivcam_obj_t*g_ti_data;
static ivcam_obj_t *g_ti_data_p;


//captainnn
static volatile gint g_meta_count;
static gint g_meta_count_p;

#ifdef USE_IVCAM
static ivcam_obj_t* g_meta_data;
static ivcam_obj_t* g_meta_data_p;
#else
static ivca_meta_obj_t *g_meta_data;
static ivca_meta_obj_t *g_meta_data_p;
#endif

/* Variables for event log. */
#define	MAX_EVENTS		1024
static ivca_rule_event_t evt_buffer[MAX_EVENTS];
static volatile gint evt_head, evt_count, evt_pend;

static ivca_rule_event_t evt_buffer_saved[MAX_EVENTS];
static volatile gint evt_head_saved, evt_count_saved, evt_pend_saved;

gboolean search_started = FALSE;
static gboolean search_paused = FALSE;
static time_t t_beg, t_end;

/* For debug. */
static NFOBJECT *lb_cpu;
static NFOBJECT *lb_mem;
static NFOBJECT *lb_ftm;
static NFOBJECT *lb_frt;
static NFOBJECT *ind_cpu;
static NFOBJECT *ind_mem;
static NFOBJECT *ck_fg;

static gboolean fg_en = FALSE;
static gboolean debug_en = FALSE;

#define	CHEAT_CODE_SIZE	8
static gint cheatkey_cnt = 0;
static guint cheat_code[CHEAT_CODE_SIZE] = {
	KEYPAD_CH01,
	KEYPAD_CH02,
	KEYPAD_CH03,
	KEYPAD_CH04,
	KEYPAD_CH01,
	KEYPAD_CH02,
	KEYPAD_CH03,
	KEYPAD_CH04
};


GTimeVal g_tv;

static void _debug_init(void);

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static void
_init_data(void)
{
	guint tmp;

	//TODO load from DB...
	//memset(&_vogd, 0, sizeof(_vogd));
	memset(&_vod, 0, sizeof(_vod));
	memset(&_vrd, 0, sizeof(_vrd));
	memset(&_vod_p, 0, sizeof(_vod_p));

	_vod.sw_obj_bb= TRUE;
	_vod.sw_obj_id= TRUE;
	_vod.sw_obj_tr= TRUE;
	_vod.sw_rule= TRUE;
	_vod.en_shadowrm = TRUE;
	_vod.en_snapshot = TRUE;
	_vod.sw_obj_w3d= FALSE;
	_vod.sw_obj_h3d= FALSE;
	_vod.sw_obj_s3d= FALSE;
	g_channel = 0;
	g_rt = 0;
	g_order = 1;
	g_sel_ridx = -1;
	g_sel_pidx = -1;
	g_sel_ridx_p = -1;
	g_sel_pidx_p = -1;
	g_beg_ruletbl = 0;
	g_sel_ruletbl = NOT_SELECTED;
	g_foc_ruletbl = -1;
	g_beg_logtbl = 0;
	g_sel_logtbl = NOT_SELECTED;
	g_foc_logtbl = -1;
	memset(g_idx_logtbl, 0xFF, sizeof(g_idx_logtbl));
	g_meta_count = -1;
	g_meta_data = NULL;
	g_meta_count_p = 0;
	g_meta_data_p = NULL;
	instant_erase = 0;
	g_sel_x_p = g_sel_y_p = -1;

	
	g_beg_restbl= 0;
	g_sel_restbl= NOT_SELECTED;
	g_foc_restbl= -1;

	memset(&_vrd_p, 0, sizeof(_vrd_p));
	memset(_cntv, 0, sizeof(_cntv));
	memset(_cntv_p, 0, sizeof(_cntv_p));

	memset(evt_buffer, 0, sizeof(evt_buffer));
	evt_head = evt_count = evt_pend = 0;
	memset(evt_buffer_saved, 0, sizeof(evt_buffer_saved));
	evt_head_saved= evt_count_saved= evt_pend_saved= 0;

	memset(events, 0, sizeof(events));

	time((time_t *)&tmp);
	srand(tmp);
}	/* _init_data(... */

static nftl_df_type
prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if ( db_index == DF_YMD )
		ret = NFTL_DF_YMD;
	else if ( db_index == DF_MDY )
		ret = NFTL_DF_MDY;
	else if ( db_index == DF_DMY )
		ret = NFTL_DF_DMY;
	else
		ret = NFTL_DF_HIDE;

	return ret;
}	/* prvTransDateFormat(... */

static void
_preview_on(gchar *buf)
{
	nfui_nflabel_set_text(NF_LABEL(lb_prvd), buf);
	nfui_nfobject_modify_bg(lb_prvd, NFOBJECT_STATE_NORMAL,
			COLOR_PRG_IDX(UX_COLOR_000000));	
	nfui_signal_emit(lb_prvd, GDK_EXPOSE, FALSE);
}	/* _preview_on(... */

static void
_preview_off(gchar *buf)
{
	nfui_nflabel_set_text(NF_LABEL(lb_prvd), buf);
	nfui_nfobject_modify_bg(lb_prvd, NFOBJECT_STATE_NORMAL, COLOR_IDX(666));	
	nfui_signal_emit(lb_prvd, GDK_EXPOSE, FALSE);
}	/* _preview_off(... */

static void
_change_obj_focus(NFOBJECT *from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);
	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}	/* _change_obj_focus(... */

static NFOBJECT *_create_button(NFOBJECT *parent, GdkPixbuf *img[],
		gint enable, guint x, guint y, gpointer postcb)
{
	NFOBJECT *obj;
	gint w, h;

	obj = (NFOBJECT *)nfui_nfbutton_new();
	if ( obj ) {
		nfui_get_pixbuf_size(img[0], &w, &h);
		nfui_nfbutton_set_image(NF_BUTTON(obj), img);
		nfui_nfobject_set_size(obj, w, h);
		if ( !enable )
			nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
		if ( parent->type == NFOBJECT_TYPE_NFFIXED )
			nfui_nffixed_put((NFFIXED *)parent, obj, x, y);
		else if ( parent->type == NFOBJECT_TYPE_NFTABLE )
			nfui_nftable_attach((NFTABLE *)parent, obj, x, y);
		nfui_regi_post_event_callback(obj, postcb);
	}
	return obj;
}	/* _create_button(... */

static void
_update_ind(NFOBJECT *ind, guint val)
{
	guint i;
	gint bg;
	NFOBJECT *obj;

	for (i = 0; i < 10; i++) {
		obj = nfui_nftable_get_child((NFTABLE *)ind, i, 0);
		if ( i > val )
			bg = COLOR_IDX(0);
		else if ( i < 3 )
			bg = COLOR_IDX(366);
		else if ( i < 7 )
			bg = COLOR_IDX(348);
		else
			bg = COLOR_IDX(364);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, bg);
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	}
}	/* _update_ind(... */

static NFOBJECT *
_create_ind(NFOBJECT *parent, guint cell_width, gint height, gint x, gint y)
{
	NFOBJECT *tbl;
	gchar *strcol[10] = {"", "", "", "", "", "", "", "", "", ""};
	guint i, ind_w[10];

	for (i = 0; i < 10; i++)
		ind_w[i] = cell_width;

	tbl = vw_table_create(parent, NULL, 10, 0, 1, 1, ind_w, height,
			strcol, 1, 0, 1, 0, 1, x, y, NULL);
	if ( tbl )
		nfui_nftable_set_draw_outline((NFTABLE *)tbl, TRUE);
	return tbl;
}	/* _create_ind(... */

static void
_create_vscrollbar(NFOBJECT *fixed, vsc_t *vsc, gint nrows,
		gint x, gint y, gint h, gint bw, gint bh, gpointer postcb)
{
	vsc->nrows = nrows;
	vsc->x = x;
	vsc->y = y;
	vsc->h = h;
	vsc->bw = bw;
	vsc->bh = bh;
	vsc->sy = y + bh;
	vsc->sh = h - bh * 2;
	vsc->hv = 2;

	/* Scroll buttons. */
	vsc->btn_up = vw_v_scbutton_create(fixed, 1, x, y, postcb);
	vsc->btn_dn = vw_v_scbutton_create(fixed, 0, x, y + h - bh, postcb);
	//nfui_nfobject_disable(vsc->btn_up);
	//nfui_nfobject_disable(vsc->btn_dn);
	/* Scroll bar labels. */
	vsc->lb_top = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_LEFT, 0, 1, 1, 0, 0, x, y + bh, bw, 1, postcb);
	vsc->lb_mid = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_LEFT, 0, 1, 1, 389, 138, x,
			y + bh + 1, bw, vsc->sh - 2, postcb);
	vsc->lb_bot = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_LEFT, 0, 1, 1, 0, 0, x, y + h - bh - 1, bw, 1, postcb);
	nfui_nflabel_set_drawing_outline(NF_LABEL(vsc->lb_top), FALSE);
	nfui_nflabel_set_drawing_outline(NF_LABEL(vsc->lb_mid), FALSE);
	nfui_nflabel_set_drawing_outline(NF_LABEL(vsc->lb_bot), FALSE);
}	/* _create_vscrollbar(... */

/**
 * @brief  Updates scroll information.
 */
static void
_update_vscrollbar(vsc_t *vsc, gint nitems, gint sidx, gboolean expose)
{
	gint h1, h2, h3, y2, y3;

	if ( nitems <= vsc->nrows ) {
		h1 = 1;
		h2 = vsc->sh - 2;
		h3 = 1;
	}
	else {
		h2 = vsc->sh * vsc->nrows / nitems;
		h2 = MIN(h2, vsc->sh - 2);
		h2 = MAX(h2, 50);
		h1 = (vsc->sh - h2) * sidx / (nitems - vsc->nrows);
		h1 = MAX(h1, 1);
		h1 = MIN(h1, vsc->sh - h2 - 1);
		h3 = vsc->sh - h1 - h2;
	}
	vsc->hv = h1 + h3;
	y2 = vsc->sy + h1;
	y3 = y2 + h2;

	nfui_nfobject_set_size(vsc->lb_top, vsc->bw, h1);
	nfui_nfobject_set_size(vsc->lb_mid, vsc->bw, h2);
	nfui_nfobject_set_size(vsc->lb_bot, vsc->bw, h3);
	nfui_nfobject_move(vsc->lb_mid, vsc->x, y2);
	nfui_nfobject_move(vsc->lb_bot, vsc->x, y3);

	nfui_signal_emit(vsc->lb_top, GDK_EXPOSE, FALSE);
	nfui_signal_emit(vsc->lb_mid, GDK_EXPOSE, FALSE);
	nfui_signal_emit(vsc->lb_bot, GDK_EXPOSE, FALSE);

	/* Enable or disable scroll buttons. */
	//vw_obj_endis(vsc->btn_up, sidx > 0, expose);
	//vw_obj_endis(vsc->btn_dn, nitems - sidx > vsc->nrows, expose);
}	/* _update_vscrollbar(... */

/**
 * @brief  Updates contents of the rule table.
 */
static void
_update_ruletbl(gboolean expose)
{
	gint i, j, n, n2;
	gchar strbuf[64];
	NFOBJECT *obj;
	ivca_zone_t*z;
	ivca_cntr_t*c;

	n = g_rt == 0 ? _vrd.nzones : _vrd.ncntrs;
	n2 = n - g_beg_ruletbl;
	obj = nfui_nftable_get_child((NFTABLE *)tbl_rule, 3, 0);
	if ( g_rt == 0 ) {
		nfui_nflabel_set_text(NF_LABEL(obj), "TYPE");
		z = &_vrd.zonelist[g_beg_ruletbl];
		for (i = 0; i < MIN(n2, _RULE_ROWS); i++, z++) {
			sprintf(strbuf, "%d", z->id);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][0]), strbuf);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][2]), z->name);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][3]),
					z->type == IVCA_RT_AREA ? "Area" : "Line");
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][4]),
					z->active ? "Yes" : "No");
		}
	}
	else {
		nfui_nflabel_set_text(NF_LABEL(obj), "VALUE");
		c = &_vrd.cntrlist[g_beg_ruletbl];
		for (i = 0; i < MIN(n2, _RULE_ROWS); i++, c++) {
			sprintf(strbuf, "%d", c->id);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][0]), strbuf);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][2]), c->name);
			sprintf(strbuf, "%d", _cntv[g_beg_ruletbl + i]);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][3]), strbuf);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][4]),
					c->active ? "Yes" : "No");
		}
	}

	/* Empty remaining rows. */
	for ( ; i < _RULE_ROWS; i++)
		for (j = 0; j < _RULE_COLS; j++)
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][j]), "");

	if ( expose ) {
		nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		for (i = 0; i < _RULE_ROWS; i++)
			for (j = 0; j < _RULE_COLS; j++)
				nfui_signal_emit(lb_ruletbl[i][j], GDK_EXPOSE, FALSE);
	}

	/* Update scroll bar */
	_update_vscrollbar(&vsc_rule, n, g_beg_ruletbl, expose);
}	/* _update_ruletbl(... */

/**
 * @brief  Updates contents of the rule table.
 */
static void
_update_result_tbl(gboolean expose)
{
	gint i, j, n, n2;
	gchar strbuf[64];
	ivca_zone_t*z;
	ivca_cntr_t*c;

	n = _vrd.nzones;
	n2 = n - g_beg_restbl;
	
	z = &_vrd.zonelist[g_beg_restbl];
	for (i = 0; i < MIN(n2, _RES_ROWS); i++, z++) {
		sprintf(strbuf, "%d", z->id);
		nfui_nflabel_set_text(NF_LABEL(lb_search_result[i][0]), strbuf);
		nfui_nflabel_set_text(NF_LABEL(lb_search_result[i][2]), z->name);
		sprintf(strbuf, "%d", events[z->id]);
		nfui_nflabel_set_text(NF_LABEL(lb_search_result[i][3]), strbuf);
	}

	/* Empty remaining rows. */
	for ( ; i < _RES_ROWS; i++)
		for (j = 0; j < _RES_COLS; j++)
			nfui_nflabel_set_text(NF_LABEL(lb_search_result[i][j]), "");

	if ( expose ) {
		for (i = 0; i < _RES_ROWS; i++)
			for (j = 0; j < _RES_COLS; j++)
				nfui_signal_emit(lb_search_result[i][j], GDK_EXPOSE, FALSE);
	}

	/* Update scroll bar */
	_update_vscrollbar(&vsc_result, n, g_beg_restbl, expose);
}	/* _update_ruletbl(... */
/**
 * @brief  Changes rule table selection by row number.
 */
static void
_change_ruletbl_selection(gint row, gboolean expose)
{
	gint i;
	NFOBJECT **objs;

	if ( g_sel_ruletbl == row )
		return;

	/* old. */
	if ( g_sel_ruletbl >= 0 && g_sel_ruletbl < _RULE_ROWS ) {
		objs = lb_ruletbl[g_sel_ruletbl];
		for (i = 0; i < _RULE_COLS; i++) {
			nfui_nflabel_modify_fg(NF_LABEL(objs[i]), COLOR_IDX(389));
			nfui_nfobject_modify_bg(objs[i],
					NFOBJECT_STATE_NORMAL, COLOR_IDX(138));
			if ( expose )
				nfui_signal_emit(objs[i], GDK_EXPOSE, FALSE);
		}
	}

	/* new. */
	if ( row >= 0 && row < _RULE_ROWS ) {
		objs = lb_ruletbl[row];
		for (i = 0; i < _RULE_COLS; i++) {
			nfui_nflabel_modify_fg(NF_LABEL(objs[i]), COLOR_IDX(391));
			nfui_nfobject_modify_bg(objs[i],
					NFOBJECT_STATE_NORMAL, COLOR_IDX(390));
			if ( expose )
				nfui_signal_emit(objs[i], GDK_EXPOSE, FALSE);
		}
	}

	g_sel_ruletbl = row;
}	/* _change_ruletbl_selection(... */

static gint
_scroll_ruletbl(gint offset, gint n, gint sconly)
{
	gint nitem = g_rt == 0 ? _vrd.nzones : _vrd.ncntrs, beg = offset + n;

	if ( beg > nitem - _RULE_ROWS )
		beg = nitem - _RULE_ROWS;
	if ( beg < 0 )
		beg = 0;
	n = beg - g_beg_ruletbl;

	if ( n != 0 ) {
		if ( sconly )
			_update_vscrollbar(&vsc_rule, nitem, beg, TRUE);
		else {
			g_beg_ruletbl = beg;
			_update_ruletbl(TRUE);
			if ( g_sel_ruletbl != NOT_SELECTED )
				_change_ruletbl_selection(g_sel_ruletbl - n, TRUE);
		}
	}
	return n;
}	/* _scroll_ruletbl(... */

static gint
_scroll_result_tbl(gint offset, gint n, gint sconly)
{
	gint nitem = _vrd.nzones, beg = offset + n;

	if ( beg > nitem - _RES_ROWS )
		beg = nitem - _RES_ROWS;
	if ( beg < 0 )
		beg = 0;
	n = beg - g_beg_restbl;

			printf("_scroll_result_tbl n %d \n",n);
	if ( n != 0 ) {
		if ( sconly )
			_update_vscrollbar(&vsc_result, nitem, beg, TRUE);
		else {
			g_beg_restbl= beg;
			_update_result_tbl(TRUE);
			//if ( g_sel_restbl!= NOT_SELECTED )
				//_change_ruletbl_selection(g_sel_restbl- n, TRUE);
		}
	}
	return n;
}	/* _scroll_ruletbl(... */

/**
 * @brief  Selects rule table by zone index, and update related controls.
 */
static gboolean
_select_ruletbl(gint idx, gboolean expose)
{
	gboolean updated = FALSE;
	gint row;

	if ( idx < 0 ) {
		_change_ruletbl_selection(NOT_SELECTED, expose);
		vw_obj_endis(btn_del, FALSE, expose);
		vw_obj_endis(btn_mod, FALSE, expose);
	}
	else {
		if ( idx - g_beg_ruletbl < 0 ) {
			g_beg_ruletbl = idx;
			_update_ruletbl(expose);
			row = 0;
			updated = TRUE;
		}
		else if ( idx - g_beg_ruletbl < _RULE_ROWS ) {
			row = idx - g_beg_ruletbl;
		}
		else {
			g_beg_ruletbl = idx < _RULE_ROWS ? 0 : (idx - _RULE_ROWS + 1);
			_update_ruletbl(expose);
			if ( g_rt == 0 )
				row = MIN(_vrd.nzones - g_beg_ruletbl, _RULE_ROWS) - 1;
			else
				row = MIN(_vrd.ncntrs - g_beg_ruletbl, _RULE_ROWS) - 1;
			updated = TRUE;
		}
		_change_ruletbl_selection(row, expose);
		vw_obj_endis(btn_del, TRUE, expose);
		vw_obj_endis(btn_mod, TRUE, expose);
	}
	return updated;
}	/* _select_ruletbl(... */

static void
_rule_update(gint idxsel, gboolean expose)
{
	if ( idxsel < 0 )
		instant_erase = 1;

	if ( idxsel != g_sel_ridx || idxsel < 0 ) {
		g_sel_ridx = idxsel;
		g_sel_pidx = -1;
	}

	if ( !_select_ruletbl(idxsel, expose) )
		_update_ruletbl(expose);

	if ( !search_started ) {
		if ( g_rt == 0 )
			vw_obj_endis(btn_add, _vrd.nzones < IVCA_MAX_ZONES, expose);
		else
			vw_obj_endis(btn_add, _vrd.ncntrs < IVCA_MAX_CNTRS, expose);
	}
	else
		vw_obj_endis(btn_add, FALSE, expose);
}	/* _rule_update(... */

/**
 * @brief  Updates contents of the event log table.
 */
static void
_update_logtbl(void)
{
	gint i, j, k = g_beg_logtbl, cnt;
	gchar strbuf[64];
	ivca_rule_event_t*e;

	cnt = (MAX_EVENTS + evt_head - g_beg_logtbl) % MAX_EVENTS;

	for (i = 0; i < MIN(cnt, _LOGT_ROWS); i++) {
		if ( g_idx_logtbl[i] != k ) {
			e = &evt_buffer[k];
			sprintf(strbuf, "%d", k + 1);
			nfui_nflabel_set_text(NF_LABEL(lb_logtbl[i][0]), strbuf);
			dtf_get_local_datetime((time_t)e->timestamp, strbuf);
			nfui_nflabel_set_text(NF_LABEL(lb_logtbl[i][1]), strbuf);
			if ( e->rule_id< 0 )
				sprintf(strbuf, "N.A.");
			else
				sprintf(strbuf, "%d", e->rule_id);
			nfui_nflabel_set_text(NF_LABEL(lb_logtbl[i][2]), strbuf);
			if ( e->object_id < 0 )
				sprintf(strbuf, "N.A.");
			else
				sprintf(strbuf, "%d", e->object_id);
			nfui_nflabel_set_text(NF_LABEL(lb_logtbl[i][3]), strbuf);
			nfui_nflabel_set_text(NF_LABEL(lb_logtbl[i][4]),
					nf_vca_event_type_string(e->type));

			for (j = 0; j < _LOGT_COLS; j++)
				nfui_signal_emit(lb_logtbl[i][j], GDK_EXPOSE, FALSE);
			g_idx_logtbl[i] = k;
		}
		if ( ++k >= MAX_EVENTS )
			k = 0;
	}

	/* Empty remaining rows. */
	for ( ; i < _LOGT_ROWS; i++) {
		for (j = 0; j < _LOGT_COLS; j++) {
			nfui_nflabel_set_text(NF_LABEL(lb_logtbl[i][j]), "");
			nfui_signal_emit(lb_logtbl[i][j], GDK_EXPOSE, FALSE);
		}
	}

	sprintf(strbuf, "Searched %d Events", evt_count);
	nfui_nflabel_set_text(NF_LABEL(lb_logcnt), strbuf);
	nfui_signal_emit(lb_logcnt, GDK_EXPOSE, FALSE);

	sprintf(strbuf, "Total %d Events", evt_count_saved > evt_count ? evt_count_saved : evt_count);
	nfui_nflabel_set_text(NF_LABEL(lb_total_logcnt), strbuf);
	nfui_signal_emit(lb_total_logcnt, GDK_EXPOSE, FALSE);

	_update_vscrollbar(&vsc_log, evt_count, evt_count - cnt, TRUE);

	/* Enable or disable log dir buttons. */
	vw_obj_endis(btn_prev, evt_count > cnt, TRUE);
	vw_obj_endis(btn_next, cnt > _LOGT_ROWS, TRUE);
}	/* _update_logtbl(... */

/**
 * @brief  Changes log table selection by row number.
 */
static void
_change_logtbl_selection(gint row, gboolean expose)
{
	gint i;
	NFOBJECT **objs;

	if ( g_sel_logtbl == row )
		return;

	/* old. */
	if ( g_sel_logtbl >= 0 && g_sel_logtbl < _LOGT_ROWS ) {
		objs = lb_logtbl[g_sel_logtbl];
		for (i = 0; i < _LOGT_COLS; i++) {
			nfui_nflabel_modify_fg(NF_LABEL(objs[i]), COLOR_IDX(139));
			nfui_nfobject_modify_bg(objs[i],
					NFOBJECT_STATE_NORMAL, COLOR_IDX(138));
			if ( expose )
				nfui_signal_emit(objs[i], GDK_EXPOSE, FALSE);
		}
	}

	/* new. */
	if ( row >= 0 && row < _LOGT_ROWS ) {
		objs = lb_logtbl[row];
		for (i = 0; i < _RULE_COLS; i++) {
			nfui_nflabel_modify_fg(NF_LABEL(objs[i]), COLOR_IDX(391));
			nfui_nfobject_modify_bg(objs[i],
					NFOBJECT_STATE_NORMAL, COLOR_IDX(390));
			if ( expose )
				nfui_signal_emit(objs[i], GDK_EXPOSE, FALSE);
		}
	}

	g_sel_logtbl = row;
}	/* _change_logtbl_selection(... */

static gint
_scroll_logtbl(gint offset, gint n, gboolean sconly)
{
	gint nitem = evt_count, beg = offset + n, cnt;

	if ( beg > nitem - _LOGT_ROWS )
		beg = nitem - _LOGT_ROWS;
	if ( beg < 0 )
		beg = 0;
	n = beg - g_beg_logtbl;

	if ( n != 0 ) {
		if ( sconly ) {
			cnt = (MAX_EVENTS + evt_head - beg) % MAX_EVENTS;
			_update_vscrollbar(&vsc_log, evt_count, evt_count - cnt, TRUE);
		}
		else {
			g_beg_logtbl = beg;
			_update_logtbl();
			if ( g_sel_logtbl != NOT_SELECTED )
				_change_logtbl_selection(g_sel_logtbl - n, TRUE);
		}
	}
	return n;
}	/* _scroll_logtbl(... */

static void
_update_snapshot(gint row)
{
	nfui_signal_emit(lb_snapshot, GDK_EXPOSE, FALSE);
}	/* _update_snapshow(... */


/**
 * @brief  Selects a rule by given point.
 */
static gint
_select_rule(gint x, gint y, gboolean flag, ivca_rule_t * rl)
{
	gint n, k, select = -1, ispt = 0, nx = x * g_rx, ny = y * g_ry;
	gint tx = 8 * g_rx, ty = 8 * g_ry;
	ivca_zone_t *z;
	ivca_cntr_t *c;

	/* First, check for already selected zone to cope with overlap &
	 * point selection. */
	if ( g_sel_ridx >= 0 ) {
		if ( g_rt == 0 ) {
			z = &rl->zonelist[g_sel_ridx];
			/* Select point. */
			for (k = 0; k < z->npts && select < 0; k++) {
				if ( abs(nx - z->pt[k].x) < tx && abs(ny - z->pt[k].y) < ty ) {
					select = k;
					ispt = 1;
				}
			}
			/* Check for previous selection. */
			if ( select < 0 && vw_vca_polygon_test(nx, ny, tx, z->npts, z->pt) )
				select = g_sel_ridx;
		}
		else {
			c = &rl->cntrlist[g_sel_ridx];
			if ( vw_vca_polygon_test(nx, ny, tx, 4, c->pt) )
				select = g_sel_ridx;
		}
	}

	/* Next, check for unselected rules. */
	if ( g_rt == 0 ) {
		for (n = 0; n < rl->nzones && select < 0; n++) {
			z = &rl->zonelist[n];
			if ( vw_vca_polygon_test(nx, ny, tx, z->npts, z->pt) )
				select = n;
		}
	}
	else {
		for (n = 0; n < rl->ncntrs && select < 0; n++) {
			c = &rl->cntrlist[n];
			if ( vw_vca_polygon_test(nx, ny, tx, 4, c->pt) )
				select = n;
		}
	}

	if ( !(flag && select == -1) ) {
		if ( ispt )
			g_sel_pidx = select;
		else {
			if ( g_sel_ridx != select || g_sel_pidx >= 0 ) {
				g_sel_pidx = -1;		/* Deselect any point. */
				g_sel_ridx = select;
				//nfui_signal_emit(lb_mnvd, GDK_EXPOSE, FALSE);
				_select_ruletbl(g_sel_ridx, TRUE);
			}
		}
	}
	g_sel_x_p = x;
	g_sel_y_p = y;
	return select;
}	/* _select_rule(... */

static gint
_move_rule(gint x, gint y, ivca_rule_t *rl)
{
	gint i, npts;
	gint dx = (x - g_sel_x_p) * g_rx, dy = (y - g_sel_y_p) * g_ry;
	gint tx = 16 * g_rx, ty = 16 * g_ry, xgap = 16 * g_rx, ygap = 16 * g_ry;
	gint w = lb_mnvd->width * g_rx, h = lb_mnvd->height * g_ry;
	gint xmin = w, xmax = 0, ymin = h, ymax = 0;
	gint mx, my;
	ivca_point_t *pts;

	if ( g_sel_ridx < 0 || (!dx && !dy) )
		return -1;

	if ( g_rt == 0 ) {
		npts = rl->zonelist[g_sel_ridx].npts;
		pts = rl->zonelist[g_sel_ridx].pt;
	}
	else {
		npts = 4;
		pts = rl->cntrlist[g_sel_ridx].pt;
	}

	if ( g_sel_pidx >= 0 ) {
		xmin = xmax = pts[g_sel_pidx].x;
		ymin = ymax = pts[g_sel_pidx].y;
	}
	else {
		for (i = 0; i < npts; i++) {
			xmin = MIN(xmin, pts[i].x);
			xmax = MAX(xmax, pts[i].x);
			ymin = MIN(ymin, pts[i].y);
			ymax = MAX(ymax, pts[i].y);
		}
	}

	dx = MAX(dx, -xmin + xgap);
	dx = MIN(dx, w - xmax - xgap);
	dy = MAX(dy, -ymin + ygap);
	dy = MIN(dy, h - ymax - ygap);

	if ( g_sel_pidx >= 0 ) {
		/* Move points. */
		mx = pts[g_sel_pidx].x + dx;
		my = pts[g_sel_pidx].y + dy;

		/* Prohibit to move a point too near to polygon points. */
		for (i = 0; i < npts; i++)
			if ( i != g_sel_pidx &&
					abs(mx - pts[i].x) < tx && abs(my - pts[i].y) < ty )
				break;
		if ( i < npts )
			return -1;

		pts[g_sel_pidx].x = (short)mx;
		pts[g_sel_pidx].y = (short)my;
	}
	else {
		/* Move entire region. */
		for (i = 0; i < npts; i++) {
			pts[i].x = (short)(pts[i].x + dx);
			pts[i].y = (short)(pts[i].y + dy);
		}
	}
	g_sel_x_p = x;
	g_sel_y_p = y;
	return 0;
}	/* _move_rule(... */

static gint
_add_rule_point(gint ridx, gint x, gint y, ivca_rule_t *rl)
{
	gint i, j, nx = x * g_rx, ny = y * g_ry, tx = 16 * g_rx, ty = 16 * g_ry;
	ivca_zone_t *z;

	if ( ridx < 0 )
		return -1;

	z = &rl->zonelist[ridx];
	if ( z->type != IVCA_RT_AREA || z->npts >= IVCA_MAX_PTSPERZONE )
		return -1;

	/* Check if the point is near polygon points. */
	for (i = 0; i < z->npts; i++)
		if ( abs(z->pt[i].x - nx) < tx && abs(z->pt[i].y - ny) < ty )
			break;
	if ( i < z->npts )
		return -1;	/* Do not add a new point near existing points. */

	/* Find the line segment to add a point. */
	for (i = 0, j = z->npts - 1; i < z->npts; j = i, i++)
		if ( vw_vca_line_test(nx, ny, tx, &z->pt[j], &z->pt[i]) )
			break;
	if ( i >= z->npts )
		return -1;

	/* Now insert a new point at next to z->pt[i]. */
	for (j = z->npts; j > i; j--)
		z->pt[j] = z->pt[j - 1];
	z->pt[i].x = (short)nx;
	z->pt[i].y = (short)ny;
	z->npts++;
	g_sel_pidx = i;
	return 0;
}	/* _add_rule_point(... */

static gint
_del_rule_point(gint ridx, gint x, gint y, ivca_rule_t *rl)
{
	gint i, j, nx = x * g_rx, ny = y * g_ry, tx = 8 * g_rx, ty = 8 * g_ry;
	ivca_zone_t *z;

	if ( ridx < 0 )
		return -1;

	z = &rl->zonelist[ridx];
	if ( z->type != IVCA_RT_AREA || z->npts < 4 )
		return -1;

	/* Find the point to delete. */
	for (i = 0; i < z->npts; i++)
		if ( abs(z->pt[i].x - nx) < tx && abs(z->pt[i].y - ny) < ty )
			break;
	if ( i >= z->npts )
		return -1;

	/* Now delete a point z->pt[i]. */
	for (j = i; j < z->npts - 1; j++)
		z->pt[j] = z->pt[j + 1];
	z->pt[j].x = z->pt[j].y = 0;
	z->npts--;
	g_sel_pidx = -1;	/* Deselect any point. */
	return 0;
}	/* _del_rule_point(... */

static gboolean
_set_option(void)
{
#if 0
	static NF_VCA_OPTION opt;

	memset(&opt, 0, sizeof(opt));
	opt.en_shadowremoval = (guint8)_vod.en_shadowrm;
	opt.en_prediction = 1;
	opt.en_roi = 0;
	opt.en_tamper = 0;
	opt.en_eventsnapshot = (guint8)_vod.en_snapshot;
	//TODO roi...
	//captainnn
	//return nf_api_vca_set_option(NF_VCA_CHID_PB, 1, &opt);
#endif
}	/* _set_option(... */

static gboolean
_set_rule(void)
{
	gint n;
	static ivca_rule_t rule;
	int res=0;

	memset(&rule, 0, sizeof(rule));
	rule.nzones = _vrd.nzones;
	rule.ncntrs = _vrd.ncntrs;
	for (n = 0; n < _vrd.nzones; n++) {
		memcpy(&rule.zonelist[n], &_vrd.zonelist[n], offsetof(ivca_zone_t, eclass));
		rule.zonelist[n].eclass = _vrd.zonelist[n].eclass;
		rule.zonelist[n].color[0] = _vrd.zonelist[n].color[0];
		rule.zonelist[n].color[1] = _vrd.zonelist[n].color[1];
		rule.zonelist[n].color[2] = _vrd.zonelist[n].color[2];
		rule.zonelist[n].npts = _vrd.zonelist[n].npts;
		memcpy(&rule.zonelist[n].pt, &_vrd.zonelist[n].pt,
				sizeof(_vrd.zonelist[n].pt) + sizeof(_vrd.zonelist[n].name));
	}

	for (n = 0; n < _vrd.ncntrs; n++) {
		rule.cntrlist[n].id = _vrd.cntrlist[n].id;
		rule.cntrlist[n].type = IVCA_RT_CNTR;
		rule.cntrlist[n].active = _vrd.cntrlist[n].active;
		rule.cntrlist[n].enabled = _vrd.cntrlist[n].enabled;
		rule.cntrlist[n].zid_up = _vrd.cntrlist[n].zid_up;
		rule.cntrlist[n].zid_dn = _vrd.cntrlist[n].zid_dn;
		rule.cntrlist[n].evalue = _vrd.cntrlist[n].evalue;
		rule.cntrlist[n].resetalert = _vrd.cntrlist[n].resetalert;
		rule.cntrlist[n].color[0] = _vrd.cntrlist[n].color[0];
		rule.cntrlist[n].color[1] = _vrd.cntrlist[n].color[1];
		rule.cntrlist[n].color[2] = _vrd.cntrlist[n].color[2];
		memcpy(&rule.cntrlist[n].pt, &_vrd.cntrlist[n].pt,
				sizeof(_vrd.cntrlist[n].pt) + sizeof(_vrd.cntrlist[n].name));
	}
	//captainnn
	//return nf_api_vca_set_rule(NF_VCA_CHID_PB, 1, &rule);
	// smart search vca id is NUM_ACTIVE_CH
	res = nf_smart_search_set_rule(NUM_ACTIVE_CH, &rule,0,0);
	if(res < 0)
		printf("nf_smart_search_set_rule Fail !!!!!!\n");
	else
		printf("nf_smart_search_set_rule Success !!!!!!\n");
		
}	/* _set_rule(... */

static void
_update_debuginfo(void)
{
//captainnn
#if 0
	gchar strbuf[64];
	static NF_VCA_PERFORMANCE perf;

	if ( nf_api_vca_get_performance(&perf) < 0 )
		return;

	sprintf(strbuf, "CPU %.2f %%", (float)perf.cpu_load / 100);
	nfui_nflabel_set_text(NF_LABEL(lb_cpu), strbuf);
	sprintf(strbuf, "MEM %.2f %%", (float)perf.mem_load / 100);
	nfui_nflabel_set_text(NF_LABEL(lb_mem), strbuf);
	sprintf(strbuf, "%.2f ms/frame",
			(float)perf.avg_time[NF_VCA_CHID_PB] / 1000);
	nfui_nflabel_set_text(NF_LABEL(lb_ftm), strbuf);
	sprintf(strbuf, "%.2f fps", (float)perf.rate[NF_VCA_CHID_PB] / 100);
	nfui_nflabel_set_text(NF_LABEL(lb_frt), strbuf);

	nfui_signal_emit(lb_cpu, GDK_EXPOSE, FALSE);
	nfui_signal_emit(lb_mem, GDK_EXPOSE, FALSE);
	nfui_signal_emit(lb_ftm, GDK_EXPOSE, FALSE);
	nfui_signal_emit(lb_frt, GDK_EXPOSE, FALSE);

	_update_ind(ind_cpu, perf.cpu_load / 1000);
	_update_ind(ind_mem, perf.mem_load / 1000);
	#endif
}	/* _update_debuginfo(... */

static void
_show_search_frame(void)
{
	//captainnn
	GTimeVal tv_start;
	GTimeVal tv_end;

	nfui_nftimelabel_get_datetime((NFTIMELABEL *)tlb_from, &tv_start);
	
	tv_end.tv_sec = tv_start.tv_sec + 1000000;

	
	//vsm_playback_smart_mainview_stop();
	
	vsm_playback_smart_mainview_start(g_channel, tv_start, tv_end,NF_PLAY_SMART_SEARCH_META_VIEW);
	//vsm_playback_smart_preview_start((guint)(1 << g_channel),tv_start);
	//captainnn test
	usleep(300000);
	vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_META_VIEW);
	//vsm_playback_smart_preview_stop();
	
	//vsm_playback_smart_mainview_start((guint)(1 << g_channel), tv_start,tv_end , FALSE);
}	/* _show_search_frame(... */

static void
_start_search(gboolean stop_live_vca)
{
	gint i;
	//captainnn test
	GTimeVal tv_start;
	GTimeVal tv_end;

	/* Clear previous search result. */
//	for (i = 0; i < MAX_EVENTS; i++)
//		if ( evt_buffer[i].snapshot )
//			free(evt_buffer[i].snapshot);
	memset(evt_buffer, 0, sizeof(evt_buffer));
	evt_head = evt_count = evt_pend = 0;
	memset(evt_buffer_saved, 0, sizeof(evt_buffer_saved));
	evt_head_saved= evt_count_saved= evt_pend_saved= 0;

	g_beg_logtbl = 0;
	memset(g_idx_logtbl, 0xFF, sizeof(g_idx_logtbl));
	_change_logtbl_selection(NOT_SELECTED, TRUE);
	//_update_snapshot(NOT_SELECTED);
	_update_logtbl();
	memset(events,0,sizeof(events));
	_update_result_tbl(TRUE);
	
	g_order = 1;
	nfui_combobox_set_index(NF_COMBOBOX(cmb_order), (guint)g_order);

	/* Set option & rule. */
	_set_option();
	_set_rule();

	/* Start. */
	//captainnn
	//nf_api_vca_start_smart_search(stop_live_vca);

	
	/* meta data send */
	nf_meta_data_display_playback_on();

	nf_meta_data_reset_cnt();
	
	//captainnn test
	nfui_nftimelabel_get_datetime((NFTIMELABEL *)tlb_from, &tv_start);
	t_beg = tv_start.tv_sec;
	nfui_nftimelabel_get_datetime((NFTIMELABEL *)tlb_to, &tv_end);
	t_end = tv_end.tv_sec;
	vsm_playback_smart_mainview_start(g_channel, tv_start,tv_end,NF_PLAY_SMART_SEARCH_META);

	search_started = TRUE;
	search_paused = FALSE;

	nfui_nfbutton_set_text(NF_BUTTON(btn_search), "PAUSE");
	nfui_signal_emit(btn_search, GDK_EXPOSE, FALSE);

	/* Disable related controls. */
	_rule_update(NOT_SELECTED, TRUE);
	vw_obj_endis(btn_stop, TRUE, TRUE);
	vw_obj_endis(btn_from, FALSE, TRUE);
	vw_obj_endis(btn_to, FALSE, TRUE);
	vw_obj_endis(cmb_ch, FALSE, TRUE);
	vw_obj_endis(btn_option, FALSE, TRUE);
	vw_obj_endis(btn_play, FALSE, TRUE);
	vw_obj_endis(cmb_order, FALSE, TRUE);

	nfui_make_key_hierarchy(g_curwnd);

	/* Disable tab switching. */
	VW_Search_config_smart(TRUE);
	
	gettimeofday(&g_tv, NULL);
}	/* _start_search(... */

static void
_stop_search(void)
{
	/* Stop. */
	vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_META_VIEW);
	vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_META);

	//captainnn
	//nf_api_vca_stop_smart_search();

	search_started = FALSE;
	search_paused = FALSE;

	nfui_nfbutton_set_text(NF_BUTTON(btn_search), "SEARCH");
	if ( btn_stop->kfocus == NFOBJECT_FOCUS )
		nfui_set_key_focus(btn_stop, FALSE);
	nfui_set_key_focus(btn_search, TRUE);
	nfui_signal_emit(btn_search, GDK_EXPOSE, FALSE);

	/* Enable related controls. */
	_rule_update(NOT_SELECTED, TRUE);
	vw_obj_endis(btn_stop, FALSE, TRUE);
	vw_obj_endis(btn_from, TRUE, TRUE);
	vw_obj_endis(btn_to, TRUE, TRUE);
	vw_obj_endis(cmb_ch, TRUE, TRUE);
	vw_obj_endis(btn_option, TRUE, TRUE);
	vw_obj_endis(cmb_order, TRUE, TRUE);
	vw_obj_endis(btn_play, g_sel_logtbl != NOT_SELECTED, TRUE);

	nfui_make_key_hierarchy(g_curwnd);

	/* Enable tab switching. */
	VW_Search_config_smart(FALSE);

	/* Refresh main view. */
	nfui_signal_emit(lb_mnvd, GDK_EXPOSE, FALSE);

	memcpy(evt_buffer_saved,evt_buffer,sizeof(evt_buffer_saved));
	evt_head_saved = evt_head;
	evt_pend_saved = evt_pend;
	evt_count_saved = evt_count;

}	/* _stop_search(... */

static void
_start_preview(gint eidx)
{
	GTimeVal tv;

	GTimeVal time_end;
	
	vsm_playback_smart_preview_stop();
	//vsm_playback_smart_mainview_stop();
	
	_preview_on("");
	if ( evt_buffer[eidx].ch != g_channel )
		return;

	tv.tv_usec = 0;
	tv.tv_sec = (glong)evt_buffer[eidx].timestamp - 3;
	
	//captainnn
	time_end.tv_sec = tv.tv_sec + 100000;
	
	vsm_playback_smart_preview_start(evt_buffer[eidx].ch, tv);

	//nf_smart_search_reset(1,NULL);

	//vsm_playback_smart_mainview_start((guint)(1 << g_channel), tv,time_end , FALSE);
	
}	/* _start_preview(... */

static void
_start_playback(gint eidx)
{
	GTimeVal tv;

	tv.tv_usec = 0;
	tv.tv_sec = (glong)evt_buffer[eidx].timestamp - 3;
	vsm_playback_smart_preview_stop();
	_preview_off("");
	VW_Search_start_playback();
	vsm_playback_start((guint)(1 << evt_buffer[eidx].ch), tv, PLAYBACK_NORMAL);
}	/* _start_playback(... */

/* GUI callbacks. */

static void
_draw_mnvd_key_edit_mode_outline(gint on)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	drawable = nfui_nfobject_get_window(lb_mnvd);
	gc = nfui_nfobject_get_gc(lb_mnvd);
	gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	nfui_nfobject_get_offset(lb_mnvd, &x, &y);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(on ? 147 : 0));
	gdk_draw_rectangle(drawable, gc, FALSE, x - 1, y - 1,
			lb_mnvd->width + 2, lb_mnvd->height + 2);
	nfui_nfobject_gc_unref(gc);
}

static void
_show_key_edit_help(gint mode)
{
	switch ( mode ) {
		case 0:
			_preview_off("");
			break;

		case 1:
			_preview_off("[Rule selection mode]\n"
					"LEFT: Add a rule\n"
					"RIGHT: Delete selected rule\n"
					"UP: Select the previous rule\n"
					"DOWN: Select the next rule\n"
					"ENTER: Enter to Rule move mode\n"
					"EXIT: Stop edit");
			break;

		case 2:
			_preview_off("[Rule move move]\n"
					"LEFT: Move left\n"
					"RIGHT: Move right\n"
					"UP: Move up\n"
					"DOWN: Move down\n"
					"ENTER: Enter to point selection mode\n"
					"EXIT: Exit from this mode");
			break;

		case 3:
			_preview_off("[Point selection mode]\n"
					"LEFT: Add a point\n"
					"RIGHT: Delete selected point\n"
					"UP: Select the previous point\n"
					"DOWN: Select the next point\n"
					"ENTER: Enter to Point move mode\n"
					"EXIT: Exit from this mode");
			break;

		case 4:
			_preview_off("[Point move mode]\n"
					"LEFT: Move left\n"
					"RIGHT: Move right\n"
					"UP: Move up\n"
					"DOWN: Move down\n"
					"ENTER: Select the next point\n"
					"EXIT: Exit from this mode");
			break;

		default:
			break;
	}
}	/* _show_key_edit_help(... */

static gboolean
_key_edit_rule_select(KEYPAD_KID kpid)
{
	mb_type ret;
	gint idx, nrule = g_rt == 0 ? _vrd.nzones : _vrd.ncntrs;

	switch ( kpid ) {
		case KEYPAD_LEFT:
			idx = g_rt == 0 ? vw_vca_add_zone(lb_mnvd->parent, &_vrd , -1) :
					vw_vca_add_cntr(lb_mnvd->parent, &_vrd);
			if ( idx >= 0 ) {
				_rule_update(idx, TRUE);
				if ( VW_VCA_Rule_Config_Open((NFWINDOW *)lb_mnvd->parent,
							&_vrd, _cntv, g_rt, idx) )
					_update_ruletbl(TRUE);
			}
			break;

		case KEYPAD_RIGHT:
			if ( g_sel_ridx >= 0 ) {
				ret = nftool_mbox((NFWINDOW *)lb_mnvd->parent, "CONFIRM",
						"Do you want to delete the selected zone?",
						NFTOOL_MB_OKCANCEL);
				if ( ret == NFTOOL_MB_CANCEL )
					break;
				idx = g_rt == 0 ? vw_vca_delete_zone(g_sel_ridx, &_vrd) :
						vw_vca_delete_cntr(g_sel_ridx, &_vrd);
				g_beg_ruletbl -= g_beg_ruletbl > 0;
				_rule_update(idx, TRUE);
			}
			break;

		case KEYPAD_UP:
			if ( nrule > 0 ) {
				if ( --g_sel_ridx < -1 )
					g_sel_ridx = nrule - 1;
				_select_ruletbl(g_sel_ridx, TRUE);
			}
			break;

		case KEYPAD_DOWN:
			if ( nrule > 0 ) {
				if ( ++g_sel_ridx >= nrule )
					g_sel_ridx = -1;
				_select_ruletbl(g_sel_ridx, TRUE);
			}
			break;

		default:
			return FALSE;
	}

	g_sel_pidx = -1;
	g_sel_x_p = 0;
	g_sel_y_p = 0;
	return TRUE;
}	/* _key_edit_rule_select(... */

static gboolean
_key_edit_rule_move(KEYPAD_KID kpid)
{
	if ( g_sel_ridx < 0 )
		return TRUE;

	switch ( kpid ) {
		case KEYPAD_LEFT:
			_move_rule(g_sel_x_p - 20, g_sel_y_p, &_vrd);
			break;

		case KEYPAD_RIGHT:
			_move_rule(g_sel_x_p + 20, g_sel_y_p, &_vrd);
			break;

		case KEYPAD_UP:
			_move_rule(g_sel_x_p, g_sel_y_p - 20, &_vrd);
			break;

		case KEYPAD_DOWN:
			_move_rule(g_sel_x_p, g_sel_y_p + 20, &_vrd);
			break;

		default:
			return FALSE;
	}
	return TRUE;
}	/* _key_edit_rule_move(... */

static gboolean
_key_edit_rule_select_point(KEYPAD_KID kpid)
{
	gint i, j;
	ivca_zone_t *z;

	if ( g_sel_ridx < 0 || g_rt == 1 )
		return TRUE;
	z = &_vrd.zonelist[g_sel_ridx];

	switch ( kpid ) {
		case KEYPAD_LEFT:
			if ( z->type == IVCA_RT_LINE )
				break;
			if ( z->npts >= IVCA_MAX_PTSPERZONE ) {
				nftool_mbox((NFWINDOW *)lb_mnvd->parent, "CONFIRM",
						"You can not add more point.", NFTOOL_MB_OK);
				break;
			}
			i = j = g_sel_pidx >= 0 ? g_sel_pidx : 0;
			if ( --j < 0 )
				j = z->npts - 1;
			_add_rule_point(g_sel_ridx, (z->pt[i].x + z->pt[j].x) / g_rx / 2,
						(z->pt[i].y + z->pt[j].y) / g_ry / 2, &_vrd);
			break;

		case KEYPAD_RIGHT:
			if ( z->type == IVCA_RT_LINE )
				break;
			if ( g_sel_pidx >= 0 ) {
				if ( _del_rule_point(g_sel_ridx, z->pt[g_sel_ridx].x / g_rx,
						z->pt[g_sel_ridx].y / g_ry, &_vrd) >= 0 ) {
					if ( --g_sel_pidx < 0 )
						g_sel_pidx = z->npts - 1;
				}
			}
			break;

		case KEYPAD_UP:
			if ( --g_sel_pidx < -1 )
				g_sel_pidx = z->npts - 1;
			break;

		case KEYPAD_DOWN:
			if ( ++g_sel_pidx >= z->npts )
				g_sel_pidx = -1;
			break;

		default:
			return FALSE;
	}
	g_sel_x_p = 0;
	g_sel_y_p = 0;
	return TRUE;
}	/* _key_edit_rule_select_point(... */

static gboolean
_key_edit_rule_move_point(KEYPAD_KID kpid)
{
	if ( g_sel_ridx < 0 || g_sel_pidx < 0 || g_rt == 1 )
		return TRUE;

	switch ( kpid ) {
		case KEYPAD_LEFT:
			_move_rule(g_sel_x_p - 10, g_sel_y_p, &_vrd);
			break;

		case KEYPAD_RIGHT:
			_move_rule(g_sel_x_p + 10, g_sel_y_p, &_vrd);
			break;

		case KEYPAD_UP:
			_move_rule(g_sel_x_p, g_sel_y_p - 10, &_vrd);
			break;

		case KEYPAD_DOWN:
			_move_rule(g_sel_x_p, g_sel_y_p + 10, &_vrd);
			break;

		case KEYPAD_ENTER:
			if ( ++g_sel_pidx >= _vrd.zonelist[g_sel_ridx].npts )
				g_sel_pidx = 0;
			break;

		default:
			return FALSE;
	}
	return TRUE;
}	/* _key_edit_rule_move_point(... */

static gboolean
post_lb_mnvd_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint changed = 0, pressed_l = 0;
	static gint key_edit_mode = 0;
	gint ofs_x, ofs_y;

	switch ( evt->type ) {
		case GDK_EXPOSE:
			if ( obj->kfocus != NFOBJECT_FOCUS ) {
				if ( key_edit_mode ) {
					g_sel_pidx = -1;
					key_edit_mode = 0;
				}
			}
			_draw_mnvd_key_edit_mode_outline(key_edit_mode);
			break;

		case GDK_BUTTON_PRESS:
			if ( search_started )
				break;
			if ( evt->button.button != MOUSE_LEFT_BUTTON )
				break;

			pressed_l = 1;
			nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
			_select_rule((gint)evt->button.x - ofs_x,
					(gint)evt->button.y - ofs_y, 0, &_vrd);
			break;

		case GDK_2BUTTON_PRESS:
			/* Add/Delete a point or modify. */
			if ( g_sel_ridx < 0 )
				break;

			nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
			if ( g_rt == 0 ) {
				if ( evt->button.button == MOUSE_LEFT_BUTTON ) {
					if ( _vrd.zonelist[g_sel_ridx].npts >= IVCA_MAX_PTSPERZONE ) {
						nftool_mbox((NFWINDOW *)lb_mnvd->parent, "CONFIRM",
								"You can not add more point.", NFTOOL_MB_OK);
						break;
					}
					if ( _add_rule_point(g_sel_ridx,
							(gint)evt->button.x - ofs_x,
							(gint)evt->button.y - ofs_y, &_vrd) >= 0 )
						break;
				}
				else {
					_del_rule_point(g_sel_ridx, (gint)evt->button.x - ofs_x,
							(gint)evt->button.y - ofs_y, &_vrd);
					break;
				}
			}
			else {
				if ( evt->button.button != MOUSE_LEFT_BUTTON )
					break;
			}

			/* Open modify popup window. */
			if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
					&_vrd, _cntv, g_rt, g_sel_ridx) ){
				_update_ruletbl(TRUE);
				_update_result_tbl(TRUE);
			}
			break;

		case GDK_LEAVE_NOTIFY:
			if ( obj->kfocus != NFOBJECT_FOCUS ) {
				_draw_mnvd_key_edit_mode_outline(0);
				if ( key_edit_mode ) {
					g_sel_pidx = -1;
					key_edit_mode = 0;
				}
			}
		case GDK_BUTTON_RELEASE:
			pressed_l = 0;
			if ( changed )
				changed = 0;
			break;

		case GDK_MOTION_NOTIFY:
			nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
			if ( evt->motion.state & GDK_BUTTON1_MASK ) {
				if ( !pressed_l )
					break;
				if ( _move_rule((gint)evt->motion.x - ofs_x,
							(gint)evt->motion.y - ofs_y, &_vrd) >= 0 )
					changed = 1;
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			if ( search_started )
				break;
			if ( evt->key.keyval == KEYPAD_ENTER ) {
				if ( key_edit_mode == 0 ||
						(key_edit_mode == 1 && g_sel_ridx >= 0) ||
						(key_edit_mode == 2 && g_rt == 0) ||
						(key_edit_mode == 3 && g_sel_pidx >= 0) ) {
					if ( ++key_edit_mode == 1 ) {
						vsm_playback_smart_preview_stop();
						_draw_mnvd_key_edit_mode_outline(1);
					}
					_show_key_edit_help(key_edit_mode);
					return TRUE;
				}
			}
			else if ( evt->key.keyval == KEYPAD_EXIT ) {
				if ( key_edit_mode > 0 ) {
					if ( --key_edit_mode == 0 ) {
						g_sel_pidx = -1;
						_draw_mnvd_key_edit_mode_outline(0);
					}
					_show_key_edit_help(key_edit_mode);
					if ( key_edit_mode == 2 ) {
						g_sel_x_p = 0;
						g_sel_y_p = 0;
						g_sel_pidx = -1;
					}
					return TRUE;
				}
			}

			switch ( key_edit_mode ) {
				case 1:
					return _key_edit_rule_select(evt->key.keyval);
				case 2:
					return _key_edit_rule_move(evt->key.keyval);
				case 3:
					return _key_edit_rule_select_point(evt->key.keyval);
				case 4:
					return _key_edit_rule_move_point(evt->key.keyval);
				default:
					break;
			}
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_lb_mnvd_cb(... */

static gboolean
pre_fixed1_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkGC *gc;
	GdkDrawable *drawable;
	gint x, y;

	if ( evt->type == GDK_EXPOSE ) {
		nfui_nfobject_get_offset(obj, &x, &y);

		drawable = nfui_nfobject_get_window(obj);
		gc = gdk_gc_new(drawable);
		gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID,
				GDK_CAP_ROUND, GDK_JOIN_ROUND);
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(604));

		/* Draw a vertical line. */
		gdk_draw_line(drawable, gc, x + obj->width - 1, y,
				x + obj->width - 1, y + obj->height);

		/* Draw group boxes. */
		gdk_draw_rectangle(drawable, gc, FALSE,
				x + _TIME_TBL_X - 10, y + _TIME_TBL_Y - 10,
				_TIME_TBL_W + 20, _RADD_BTN_Y + 40 - _RULE_LBL_Y + 20);
		gdk_draw_rectangle(drawable, gc, FALSE,
				x + _RULE_LBL_X - 10, y + _RULE_LBL_Y - 10,
				_RULE_TBL_W + 25 + 20, _RADD_BTN_Y + 40 - _RULE_LBL_Y + 20);

		g_object_unref(gc);
	}
	else if ( evt->type == GDK_DELETE ) {
	}
	return FALSE;
}	/* pre_fixed1_cb(... */

static gboolean
post_btn_fromto_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint x, y;
	NFOBJECT *top, *tlobj = NULL;
	GTimeVal from_tv, to_tv, temp_tv, *ptv = NULL;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;
		memset(&from_tv, 0, sizeof(GTimeVal));
		memset(&to_tv, 0, sizeof(GTimeVal));
		memset(&temp_tv, 0, sizeof(GTimeVal));

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);
		x += top->x + obj->width + 4;
		y += top->y;

		nfui_nftimelabel_get_datetime((NFTIMELABEL *)tlb_from, &from_tv);
		nfui_nftimelabel_get_datetime((NFTIMELABEL *)tlb_to, &to_tv);

		if ( obj == btn_from ) {
			to_tv.tv_sec -= 1;
			temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "FROM",
					(guint)x, (guint)y, from_tv.tv_sec, SDT_TYPE_SEC,
					LIMIT_UPPER, to_tv.tv_sec);
			tlobj = tlb_from;
			ptv = &from_tv;
		}
		else if ( obj == btn_to ) {
			from_tv.tv_sec += 1;
			temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "TO",
					(guint)x, (guint)y, to_tv.tv_sec, SDT_TYPE_SEC,
					LIMIT_LOWER, from_tv.tv_sec);
			tlobj = tlb_to;
			ptv = &to_tv;
		}
		if ( tlobj && temp_tv.tv_sec ) {
			nfui_nftimelabel_set_datetime((NFTIMELABEL *)tlobj, &temp_tv);
			nfui_signal_emit(tlobj, GDK_EXPOSE, TRUE);
			if ( obj == btn_from )
				_show_search_frame();
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( obj == btn_from ) {
		}
		else if ( obj == btn_to ) {
			if ( evt->key.keyval == KEYPAD_DOWN ) {
				_change_obj_focus(obj, cmb_ch);
				return TRUE;
			}
		}
	}
	return FALSE;
}	/* post_btn_fromto_cb(... */

static gboolean
post_cmb_ch_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if ( g_channel != index ) {
			vsm_playback_smart_preview_stop();
			_preview_off("");
			nf_play_set_smart_geometry(index, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
			if ( debug_en )
				nf_play_smart_set_fgdebug(fg_en);
			_show_search_frame();
		}
		g_channel = index;
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_UP ) {
			_change_obj_focus(obj, btn_to);
			return TRUE;
		}
		else if ( evt->key.keyval == KEYPAD_DOWN ) {
			_change_obj_focus(obj, btn_option);
			return TRUE;
		}
	}
	return FALSE;
}	/* post_cmb_ch_cb(... */

static gboolean
post_btn_option_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		if ( VW_VCA_Display_Option_Config_Open((NFWINDOW *)obj->parent,
					&_vod) ) {
			//TODO Send to dsp if required...
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_UP ) {
			_change_obj_focus(obj, cmb_ch);
			return TRUE;
		}
		else if ( evt->key.keyval == KEYPAD_DOWN ) {
			_change_obj_focus(obj, btn_search);
			return TRUE;
		}
	}
	return FALSE;
}	/* post_btn_option_cb(... */

static gboolean
post_cmb_rt_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint rt;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		rt = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		if ( g_rt == rt )
			return FALSE;

		g_rt = rt;
		g_beg_ruletbl = 0;
		_rule_update(NOT_SELECTED, TRUE);
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_DOWN ) {
			_change_obj_focus(obj, lb_ruletbl[0][0]);
			return TRUE;
		}
	}
	return FALSE;
}	/* post_cmb_rt_cb(... */

static void
_rgb888_to_gdkcolor(guint8 *rgb, GdkColor *gdkcolor)
{
	gdkcolor->pixel = 0;
	gdkcolor->red = (guint16)(rgb[0] << 8);
	gdkcolor->green = (guint16)(rgb[1] << 8);
	gdkcolor->blue = (guint16)(rgb[2] << 8);
}	/* _rgb888_to_gdkcolor(... */

static void
_draw_ruletbl_rcolor(NFOBJECT *obj, gint idx)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkColor color;
	gint x, y;

	_rgb888_to_gdkcolor(g_rt == 0 ? _vrd.zonelist[idx].color :
			_vrd.cntrlist[idx].color, &color);

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	nfui_nfobject_get_offset(obj, &x, &y);

	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_draw_rectangle(drawable, gc, TRUE, x + 2, y + 1, 20, 20);
	nfui_nfobject_gc_unref(gc);
}	/* _draw_ruletbl_rcolor(... */

static void
_draw_ruletbl_row_outline(gint row, gint color)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	drawable = nfui_nfobject_get_window(lb_ruletbl[row][0]);
	gc = nfui_nfobject_get_gc(lb_ruletbl[row][0]);
	gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	nfui_nfobject_get_offset(lb_ruletbl[row][0], &x, &y);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));
	gdk_draw_rectangle(drawable, gc, FALSE, x - 1, y - 1, _RULE_TBL_W, 22 + 1);
	nfui_nfobject_gc_unref(gc);
}	/* draw_ruletbl_row_outline(... */

static void
_update_evt_buffer(gint rule_id_mask , gint event_mask)
{
	int i;

	printf("rule_id_mask rule_id_mask %x event_mask %x", rule_id_mask,event_mask);
	

	//if(_vrd.nzones <= id)
		//return;
	
	evt_head = 0;
	evt_pend = 0;
	evt_count = 0;
	memset(evt_buffer, 0,sizeof(evt_buffer));
	
	for(i=0;i<evt_count_saved;i++){
		if(((0x1 << evt_buffer_saved[i].rule_id) & rule_id_mask)
			&& ( evt_buffer_saved[i].type & event_mask)){
			memcpy(&evt_buffer[evt_head], &evt_buffer_saved[i], sizeof(ivca_rule_event_t));
			evt_head = (evt_head + 1) % MAX_EVENTS;
			//evt_pend = MIN(evt_pend + 1, MAX_EVENTS - 1);
			evt_count = MIN(evt_count + 1, MAX_EVENTS - 1);		
		}
	}
	memset(g_idx_logtbl, 0xFF, sizeof(g_idx_logtbl));
	g_beg_logtbl = 0;
	_update_logtbl();

}

static void
_change_ruletbl_focus(gint row)
{
	if ( g_foc_ruletbl == row )
		return;

	if ( g_foc_ruletbl >= 0 )
		_draw_ruletbl_row_outline(g_foc_ruletbl, 0);
	if ( row >= 0 )
		_draw_ruletbl_row_outline(row, 390);

	g_foc_ruletbl = row;
}	/* _change_ruletbl_focus(... */

static gboolean
post_lb_ruletbl_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint row, eofs;
	NFOBJECT *cur_focus;

	if ( (evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS) &&
			evt->key.keyval == KEYPAD_ENTER ) {
		row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
		evt->type = g_sel_ruletbl == row ? GDK_2BUTTON_PRESS : GDK_BUTTON_PRESS;
		evt->button.button = MOUSE_LEFT_BUTTON;
	}

	switch ( evt->type ) {
		case GDK_EXPOSE:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			/* Draw rule color. */
			if ( lb_ruletbl[row][1] == obj ) {
				eofs = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - g_beg_ruletbl;
				if ( row < MIN(eofs, _RULE_ROWS) )
					_draw_ruletbl_rcolor(obj, g_beg_ruletbl + row);
			}

			_change_ruletbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
			break;

		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( search_started )
				return FALSE;
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			eofs = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - g_beg_ruletbl;
			if ( row < MIN(eofs, _RULE_ROWS) ) {
				g_sel_ridx = g_beg_ruletbl + row;
				g_sel_pidx = -1;
				_change_ruletbl_selection(row, TRUE);
				vw_obj_endis(btn_del, TRUE, TRUE);
				vw_obj_endis(btn_mod, TRUE, TRUE);
			}
			break;

		case GDK_2BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( search_started )
				return FALSE;
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			eofs = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - g_beg_ruletbl;
			if ( row < MIN(eofs, _RULE_ROWS) ) {
				/* The row is selected already by GDK_BUTTON_PRESS. */
				if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
						&_vrd, _cntv, g_rt, g_beg_ruletbl + row) ){
					_update_ruletbl(TRUE);
					_update_result_tbl(TRUE);
				}
			}
			break;

		case GDK_SCROLL:
			if ( evt->scroll.direction == GDK_SCROLL_UP )
				_scroll_ruletbl(g_beg_ruletbl, -1, FALSE);
			else if ( evt->scroll.direction == GDK_SCROLL_DOWN )
				_scroll_ruletbl(g_beg_ruletbl, 1, FALSE);
			break;

		case GDK_ENTER_NOTIFY:
			cur_focus = nfui_get_cur_focus(obj);
			if ( cur_focus )
				_change_obj_focus(cur_focus, obj);
			break;

		case GDK_LEAVE_NOTIFY:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			_change_ruletbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			if ( evt->key.keyval == KEYPAD_RIGHT ) {
				_change_obj_focus(obj, row < _RULE_ROWS / 2 ? vsc_rule.btn_up :
						vsc_rule.btn_dn);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_LEFT ) {
				if ( row < 1 )
					_change_obj_focus(obj, btn_to);
				else if ( row < 3 )
					_change_obj_focus(obj, cmb_ch);
				else
					_change_obj_focus(obj, btn_option);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_UP ) {
				_change_obj_focus(obj, !row ? cmb_rt : lb_ruletbl[row - 1][0]);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_DOWN ) {
				_change_obj_focus(obj, row == _RULE_ROWS - 1 ? btn_add :
						lb_ruletbl[row + 1][0]);
				return TRUE;
			}
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_lb_ruletbl_cb(... */

static gboolean
post_vsc_rule_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint pressed_l = 0, pressed_y = 0, pressed_i = 0, prev_sc = 0;
	gint dy, n;

	switch ( evt->type ) {
		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( obj == vsc_rule.btn_up )
				_scroll_ruletbl(g_beg_ruletbl, -1, FALSE);
			else if ( obj == vsc_rule.btn_dn )
				_scroll_ruletbl(g_beg_ruletbl, 1, FALSE);
			else if ( obj == vsc_rule.lb_top )
				_scroll_ruletbl(g_beg_ruletbl, -_RULE_ROWS, FALSE);
			else if ( obj == vsc_rule.lb_bot )
				_scroll_ruletbl(g_beg_ruletbl, _RULE_ROWS, FALSE);
			else if ( obj == vsc_rule.lb_mid ) {
				if ( pressed_l == 0 ) {
					pressed_l = 1;
					pressed_y = (gint)evt->button.y;
					pressed_i = g_beg_ruletbl;
					prev_sc = 0;
				}
			}
			break;

		case GDK_BUTTON_RELEASE:
			if ( pressed_l )
				_scroll_ruletbl(pressed_i, prev_sc, FALSE);
			pressed_l = 0;
			break;

		case GDK_MOTION_NOTIFY:
			if ( pressed_l ) {
				dy = (gint)evt->motion.y - pressed_y;
				n = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - _RULE_ROWS;
				prev_sc = n * dy / vsc_rule.hv;
				_scroll_ruletbl(pressed_i, prev_sc, TRUE);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			if ( obj == vsc_rule.btn_up ) {
				if ( evt->key.keyval == KEYPAD_DOWN ) {
					_change_obj_focus(obj, vsc_rule.btn_dn);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_LEFT ) {
					_change_obj_focus(obj, lb_ruletbl[0][0]);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_RIGHT ) {
					_change_obj_focus(obj, lb_logtbl[_LOGT_ROWS - 1][0]);
					return TRUE;
				}
			}
			else if ( obj == vsc_rule.btn_dn ) {
				if ( evt->key.keyval == KEYPAD_UP ) {
					_change_obj_focus(obj, vsc_rule.btn_up);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_LEFT ) {
					_change_obj_focus(obj, lb_ruletbl[_RULE_ROWS - 1][0]);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_RIGHT ) {
					_change_obj_focus(obj, cmb_order);
					return TRUE;
				}
			}
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_vsc_rule_cb(... */

static gboolean
post_lb_result_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint row, eofs;
	NFOBJECT *cur_focus;
#if 1
	if ( (evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS) &&
			evt->key.keyval == KEYPAD_ENTER ) {
		row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
		evt->type = g_sel_restbl== row ? GDK_2BUTTON_PRESS : GDK_BUTTON_PRESS;
		evt->button.button = MOUSE_LEFT_BUTTON;
	}

	switch ( evt->type ) {
		case GDK_EXPOSE:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			/* Draw rule color. */
			if ( lb_search_result[row][1] == obj ) {
				eofs = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - g_beg_restbl;
				if ( row < MIN(eofs, _RES_ROWS) )
					_draw_ruletbl_rcolor(obj, g_beg_restbl+ row);
			}

			//_change_ruletbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
			break;

		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( search_started )
				return FALSE;
			
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			eofs = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - g_beg_restbl;
			printf("row %d eofs %d \n", row, eofs);
			_update_evt_buffer(0x1 << (g_beg_restbl + row), 0xffff);
			nfui_combobox_set_index(cmb_order, 1);
			vw_obj_endis(cmb_order, TRUE, TRUE);
			

		case GDK_2BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( search_started )
				return FALSE;
			
			break;

		case GDK_SCROLL:
			if ( evt->scroll.direction == GDK_SCROLL_UP )
				_scroll_result_tbl(g_beg_restbl, -1, FALSE);
			else if ( evt->scroll.direction == GDK_SCROLL_DOWN )
				_scroll_result_tbl(g_beg_restbl, 1, FALSE);
			break;

		case GDK_ENTER_NOTIFY:
			//cur_focus = nfui_get_cur_focus(obj);
			//if ( cur_focus )
			//	_change_obj_focus(cur_focus, obj);
			break;

		case GDK_LEAVE_NOTIFY:
			//row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			//_change_ruletbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			if ( evt->key.keyval == KEYPAD_RIGHT ) {
				_change_obj_focus(obj, row < _RULE_ROWS / 2 ? vsc_result.btn_up :
						vsc_result.btn_dn);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_LEFT ) {
				if ( row < 1 )
					_change_obj_focus(obj, btn_to);
				else if ( row < 3 )
					_change_obj_focus(obj, cmb_ch);
				else
					_change_obj_focus(obj, btn_option);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_UP ) {
				_change_obj_focus(obj, !row ? cmb_rt : lb_search_result[row - 1][0]);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_DOWN ) {
				_change_obj_focus(obj, row == _RULE_ROWS - 1 ? btn_add :
						lb_search_result[row + 1][0]);
				return TRUE;
			}
			break;

		default:
			break;
	}
#endif
	return FALSE;
}	/* post_lb_ruletbl_cb(... */

static gboolean
post_vsc_result_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint pressed_l = 0, pressed_y = 0, pressed_i = 0, prev_sc = 0;
	gint dy, n;

#if 1
	switch ( evt->type ) {
		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( obj == vsc_result.btn_up )
				_scroll_result_tbl(g_beg_restbl, -1, FALSE);
			else if ( obj == vsc_result.btn_dn )
				_scroll_result_tbl(g_beg_restbl, 1, FALSE);
			else if ( obj == vsc_result.lb_top )
				_scroll_result_tbl(g_beg_restbl, -_RES_ROWS, FALSE);
			else if ( obj == vsc_result.lb_bot )
				_scroll_result_tbl(g_beg_restbl, _RES_ROWS, FALSE);
			else if ( obj == vsc_result.lb_mid ) {
				if ( pressed_l == 0 ) {
					pressed_l = 1;
					pressed_y = (gint)evt->button.y;
					pressed_i = g_beg_restbl;
					prev_sc = 0;
				}
			}
			break;

		case GDK_BUTTON_RELEASE:
			if ( pressed_l )
				_scroll_result_tbl(pressed_i, prev_sc, FALSE);
			pressed_l = 0;
			break;

		case GDK_MOTION_NOTIFY:
			if ( pressed_l ) {
				dy = (gint)evt->motion.y - pressed_y;
				n = (g_rt == 0 ? _vrd.nzones : _vrd.ncntrs) - _RES_ROWS;
				prev_sc = n * dy / vsc_result.hv;
				_scroll_result_tbl(pressed_i, prev_sc, TRUE);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			if ( obj == vsc_result.btn_up ) {
				if ( evt->key.keyval == KEYPAD_DOWN ) {
					_change_obj_focus(obj, vsc_result.btn_dn);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_LEFT ) {
					_change_obj_focus(obj, lb_ruletbl[0][0]);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_RIGHT ) {
					_change_obj_focus(obj, lb_logtbl[_LOGT_ROWS - 1][0]);
					return TRUE;
				}
			}
			else if ( obj == vsc_result.btn_dn ) {
				if ( evt->key.keyval == KEYPAD_UP ) {
					_change_obj_focus(obj, vsc_result.btn_up);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_LEFT ) {
					_change_obj_focus(obj, lb_search_result[_RES_ROWS - 1][0]);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_RIGHT ) {
					_change_obj_focus(obj, cmb_order);
					return TRUE;
				}
			}
			break;

		default:
			break;
	}
#endif
	return FALSE;
}	/* post_vsc_rule_cb(... */

static gboolean
post_btn_edit_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint idx;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		if ( obj == btn_add ) {
			if ( g_rt == 0 )	/* Zone. */
				idx = vw_vca_add_zone(obj->parent, &_vrd,-1);
			else				/* Counter. */
				idx = vw_vca_add_cntr(obj->parent, &_vrd);
			if ( idx >= 0 ) {
				_rule_update(idx, TRUE);
				if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
						&_vrd, _cntv, g_rt, idx) ){
					_update_ruletbl(TRUE);
				}
				_update_result_tbl(TRUE);
			}
		}
		else if ( obj == btn_del ) {
			if ( g_rt == 0 )	/* Zone. */
				idx = vw_vca_delete_zone(g_beg_ruletbl + g_sel_ruletbl, &_vrd);
			else				/* Counter. */
				idx = vw_vca_delete_cntr(g_beg_ruletbl + g_sel_ruletbl, &_vrd);
			g_beg_ruletbl -= g_beg_ruletbl > 0;
			_rule_update(idx, TRUE);
			_update_result_tbl(TRUE);
		}
		else if ( obj == btn_mod ) {
			idx = g_beg_ruletbl + g_sel_ruletbl;
			if ( idx < 0 || (g_rt == 0 && idx >= _vrd.nzones) ||
					(g_rt == 1 && idx >= _vrd.ncntrs) )
				return FALSE;

			if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
					&_vrd, _cntv, g_rt, idx) ){
				_update_ruletbl(TRUE);
			}
			_update_result_tbl(TRUE);
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( obj == btn_add ) {
			if ( evt->key.keyval == KEYPAD_LEFT ) {
				_change_obj_focus(obj, btn_option);
				return TRUE;
			}
		}
		else if ( obj == btn_mod ) {
			if ( evt->key.keyval == KEYPAD_RIGHT ) {
				_change_obj_focus(obj, cmb_order);
				return TRUE;
			}
		}
	}
	return FALSE;
}	/* post_btn_edit_cb(... */

static gboolean
returnkey_proc(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	mb_type ret;

	if ( search_started ) {
		ret = nftool_mbox((NFWINDOW *)obj->parent, "WARNING",
				"You may lose the search result.\n"
				"Do you want to close?", NFTOOL_MB_OKCANCEL);
		if ( ret == NFTOOL_MB_CANCEL )
			return FALSE;
		_stop_search();
	}
	vsm_playback_smart_preview_stop();
	return TRUE;
}	/* returnkey_proc(... */

static gboolean
post_btn_search_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret = NFTOOL_MB_CANCEL;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		if ( !search_started ) {
			if ( _vrd.nzones == 0 ) {
				nftool_mbox((NFWINDOW *)obj->parent, "CONFIRM",
						"No rule is defined.\nAdd rules first.", NFTOOL_MB_OK);
				return FALSE;
			}
			if ( evt_count > 0 ) {
				ret = nftool_mbox((NFWINDOW *)obj->parent, "WARNING",
						"Previous search result will be lost.\n"
						"Do you want to continue anyway?", NFTOOL_MB_OKCANCEL);
				if ( ret == NFTOOL_MB_CANCEL )
					return FALSE;
			}
//captainnn
#if 1
				/*
			ret = nftool_mbox((NFWINDOW *)obj->parent, "CONFIRM",
					"Do you want to stop the live VCA to speed up searching?",
					NFTOOL_MB_YESNO);
					
					
			ret = nftool_mbox((NFWINDOW *)obj->parent, "CONFIRM",
					"Do you want to search with a maximum speed?",
					NFTOOL_MB_YESNO);
					*/

#endif
			vsm_playback_smart_preview_stop();
			_preview_off("");
			_start_search(ret == NFTOOL_MB_YES);
		}
		else {
			if ( search_paused == FALSE ){
				vsm_playback_smart_mainview_pause(NF_PLAY_SMART_SEARCH_META_VIEW);
				vsm_playback_smart_mainview_pause(NF_PLAY_SMART_SEARCH_META);
			}
			else{
				vsm_playback_smart_mainview_resume(NF_PLAY_SMART_SEARCH_META_VIEW);
				vsm_playback_smart_mainview_resume(NF_PLAY_SMART_SEARCH_META);
			}
			search_paused = !search_paused;
			nfui_nfbutton_set_text(NF_BUTTON(btn_search),
					search_paused ? "RESUME" : "PAUSE");
			nfui_signal_emit(btn_search, GDK_EXPOSE, FALSE);
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_UP ) {
			if ( !nfui_nfobject_is_disabled(btn_add) )
				_change_obj_focus(obj, btn_add);
			else if ( !nfui_nfobject_is_disabled(btn_del) )
				_change_obj_focus(obj, btn_del);
			else if ( !nfui_nfobject_is_disabled(btn_mod) )
				_change_obj_focus(obj, btn_mod);
			else
				_change_obj_focus(obj, lb_ruletbl[_RULE_ROWS - 1][0]);
			return TRUE;
		}
	}
	return FALSE;
}	/* post_btn_search_cb(... */

static gboolean
post_btn_result_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;
		if ( !search_started ){
			memset(evt_buffer, 0,sizeof(evt_buffer));
			memcpy(evt_buffer, evt_buffer_saved, sizeof(evt_buffer));
			evt_head = evt_head_saved;
			evt_pend= evt_pend_saved;
			evt_count= evt_count_saved;
			g_beg_logtbl =0;
			memset(g_idx_logtbl, 0xFF, sizeof(g_idx_logtbl));
			_update_logtbl();
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
	}
	return FALSE;
}	/* post_btn_stop_cb(... */

static gboolean
post_btn_detail_search_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret;
	gint res;
	int rule_id_mask;
	int event_mask;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;
		
		if ( !search_started  &&  _vrd.nzones){
			res = VW_VCA_Detail_Search_Open((NFWINDOW *)obj->parent, &rule_id_mask,&event_mask, _vrd.nzones);
			if(res == TRUE){
				_update_evt_buffer(rule_id_mask,event_mask);
				nfui_combobox_set_index(cmb_order, 1);
				vw_obj_endis(cmb_order, TRUE, TRUE);
			}
		}

	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
	}
	return FALSE;
}	/* post_btn_stop_cb(... */
static gboolean
post_btn_stop_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		if ( search_started ) {
			ret = nftool_mbox((NFWINDOW *)obj->parent, "WARNING",
					"You cannot resume the search after stop.\n"
					"Do you want to stop?", NFTOOL_MB_OKCANCEL);
			if ( ret == NFTOOL_MB_CANCEL )
				return FALSE;
			
			_stop_search();
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
	}
	return FALSE;
}	/* post_btn_stop_cb(... */

static void
_draw_logtbl_rcolor(NFOBJECT *obj, gint idx)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkColor color;
	gint n, x, y;
	ivca_zone_t *z;
	ivca_cntr_t *c;

	if ( evt_buffer[idx].rule_id< 0 )
		return;

	if ( !(evt_buffer[idx].type & IVCA_ET_COUNTER) ) {
		for (n = 0, z = _vrd.zonelist; n < _vrd.nzones; n++, z++)
			if ( z->id == evt_buffer[idx].rule_id )
				break;
		if ( n >= _vrd.nzones )
			return;
		_rgb888_to_gdkcolor(z->color, &color);
	}
	else {
		for (n = 0, c = _vrd.cntrlist; n < _vrd.ncntrs; n++, c++)
			if ( c->id == evt_buffer[idx].rule_id )
				break;
		if ( n >= _vrd.ncntrs )
			return;
		_rgb888_to_gdkcolor(c->color, &color);
	}

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);
	gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	nfui_nfobject_get_offset(obj, &x, &y);

	gdk_gc_set_rgb_fg_color(gc, &color);
	gdk_draw_rectangle(drawable, gc, TRUE,
			x + obj->width - 21 - 2, y + 2, 21, 21);
	nfui_nfobject_gc_unref(gc);
}	/* _draw_logtbl_rcolor(... */

static void
_draw_logtbl_row_outline(gint row, gint color)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	drawable = nfui_nfobject_get_window(lb_logtbl[row][0]);
	gc = nfui_nfobject_get_gc(lb_logtbl[row][0]);
	gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	nfui_nfobject_get_offset(lb_logtbl[row][0], &x, &y);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(color));
	gdk_draw_rectangle(drawable, gc, FALSE, x - 1, y - 1, _LOGT_TBL_W, 25 + 1);
	nfui_nfobject_gc_unref(gc);
}	/* _draw_logtbl_row_outline(... */

static void
_change_logtbl_focus(gint row)
{
	if ( g_foc_logtbl == row )
		return;

	if ( g_foc_logtbl >= 0 )
		_draw_logtbl_row_outline(g_foc_logtbl, 0);
	if ( row >= 0 )
		_draw_logtbl_row_outline(row, 390);

	g_foc_logtbl = row;
}	/* _change_logtbl_focus(... */

static gboolean
post_lb_logtbl_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint row;
	NFOBJECT *cur_focus;

	if ( (evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS) &&
			evt->key.keyval == KEYPAD_ENTER ) {
		row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
		evt->type = g_sel_logtbl == row ? GDK_2BUTTON_PRESS : GDK_BUTTON_PRESS;
		evt->button.button = MOUSE_LEFT_BUTTON;
	}

	switch ( evt->type ) {
		case GDK_EXPOSE:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			/* Draw rule color. */
			if( lb_logtbl[row][2] == obj ) {
				if ( row < MIN(evt_count, _LOGT_ROWS) )
					_draw_logtbl_rcolor(obj, (g_beg_logtbl + row) % MAX_EVENTS);
			}

			_change_logtbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
			break;

		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			if ( row >= MIN(evt_count, _LOGT_ROWS) )
				break;

			_change_logtbl_selection(row, TRUE);
			//captainnn
			//_update_snapshot(row);

			if ( !search_started )
				vw_obj_endis(btn_play, TRUE, TRUE);

			if ( !search_started || search_paused)
				_start_preview((g_beg_logtbl + row) % MAX_EVENTS);
			
			break;

		case GDK_2BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			if ( row >= MIN(evt_count, _LOGT_ROWS) )
				break;

			if ( search_started )
				break;

			/* The row is selected already by GDK_BUTTON_PRESS. */
			_start_playback((g_beg_logtbl + row) % MAX_EVENTS);
			break;

		case GDK_SCROLL:
			if ( evt->scroll.direction == GDK_SCROLL_UP )
				_scroll_logtbl(g_beg_logtbl, -_LOGT_ROWS / 4, FALSE);
			else if ( evt->scroll.direction == GDK_SCROLL_DOWN )
				_scroll_logtbl(g_beg_logtbl, _LOGT_ROWS / 4, FALSE);
			break;

		case GDK_ENTER_NOTIFY:
			cur_focus = nfui_get_cur_focus(obj);
			if ( cur_focus )
				_change_obj_focus(cur_focus, obj);
			break;

		case GDK_LEAVE_NOTIFY:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			_change_logtbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			if ( evt->key.keyval == KEYPAD_RIGHT ) {
				_change_obj_focus(obj, row < _LOGT_ROWS / 2 ? vsc_log.btn_up :
						vsc_log.btn_dn);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_LEFT ) {
				_change_obj_focus(obj, lb_mnvd);
				return TRUE;
			}
			else if ( evt->key.keyval == KEYPAD_UP ) {
				if ( row == 0 ) {
					_change_obj_focus(obj, btn_play);
					return TRUE;
				}
			}
			else if ( evt->key.keyval == KEYPAD_DOWN ) {
				if ( row == _LOGT_ROWS - 1 ) {
					_change_obj_focus(obj, cmb_order);
					return TRUE;
				}
			}
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_lb_logtbl_cb(... */

static gboolean
post_vsc_log_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint pressed_l = 0, pressed_y = 0, pressed_i = 0, prev_sc = 0;
	gint dy;

	switch ( evt->type ) {
		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;

			if ( obj == vsc_log.btn_up )
				_scroll_logtbl(g_beg_logtbl, -1, FALSE);
			else if ( obj == vsc_log.btn_dn )
				_scroll_logtbl(g_beg_logtbl, 1, FALSE);
			else if ( obj == vsc_log.lb_top )
				_scroll_logtbl(g_beg_logtbl, -_LOGT_ROWS, FALSE);
			else if ( obj == vsc_log.lb_bot )
				_scroll_logtbl(g_beg_logtbl, _LOGT_ROWS, FALSE);
			else if ( obj == vsc_log.lb_mid ) {
				if ( pressed_l == 0 ) {
					pressed_l = 1;
					pressed_y = (gint)evt->button.y;
					pressed_i = g_beg_logtbl;
					prev_sc = 0;
				}
			}
			break;

		case GDK_BUTTON_RELEASE:
			if ( pressed_l )
				_scroll_logtbl(pressed_i, prev_sc, FALSE);
			pressed_l = 0;
			break;

		case GDK_MOTION_NOTIFY:
			if ( pressed_l ) {
				dy = (gint)evt->motion.y - pressed_y;
				prev_sc = (evt_count - _LOGT_ROWS) * dy / vsc_log.hv;
				_scroll_logtbl(pressed_i, prev_sc, TRUE);
			}
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			if ( obj == vsc_log.btn_up ) {
				if ( evt->key.keyval == KEYPAD_DOWN ) {
					_change_obj_focus(obj, vsc_log.btn_dn);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_LEFT ) {
					_change_obj_focus(obj, lb_logtbl[0][0]);
					return TRUE;
				}
			}
			else if ( obj == vsc_log.btn_dn ) {
				if ( evt->key.keyval == KEYPAD_UP ) {
					_change_obj_focus(obj, vsc_log.btn_up);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_DOWN ) {
					_change_obj_focus(obj, btn_next);
					return TRUE;
				}
				else if ( evt->key.keyval == KEYPAD_LEFT ) {
					_change_obj_focus(obj, lb_logtbl[_LOGT_ROWS - 1][0]);
					return TRUE;
				}
			}
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_vsc_log_cb(... */

static gboolean
post_lb_snapshot_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	ivca_rule_event_t *eb;
	GInputStream *stream;
	GdkPixbuf *pixbuf;
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y, w, h;

	if ( evt->type == GDK_EXPOSE && !nfui_nfobject_is_disabled(obj) ) {
		if ( abs(g_sel_logtbl) >= MAX_EVENTS )
			return FALSE;

		eb = &evt_buffer[(MAX_EVENTS + g_beg_logtbl + g_sel_logtbl) %
				MAX_EVENTS];
		if ( !eb->snap_size || !eb->snapshot )
			return FALSE;

		//FIXME: Do this job in _update_snapshot() for the performance.
		stream = g_memory_input_stream_new_from_data(eb->snapshot,
				(gssize)eb->snap_size, NULL);

		pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
		nfui_get_pixbuf_size(pixbuf, &w, &h);
		if ( w > obj->width || h > obj->height ) {
			/* Image size is larger than label size: Scale snapshot. */
			g_input_stream_close(stream, NULL, NULL);
			stream = g_memory_input_stream_new_from_data(eb->snapshot,
					(gssize)eb->snap_size, NULL);
			g_object_unref(pixbuf);
			w = MIN(w, obj->width);
			h = MIN(h, obj->height);
			pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream,
					w, h, TRUE, NULL, NULL);
		}
		drawable = nfui_nfobject_get_window(obj);
		gc = gdk_gc_new(drawable);
		nfui_nfobject_get_offset(obj, &x, &y);
		nfutil_draw_pixbuf(drawable, gc, pixbuf, x, y,
				obj->width, obj->height, NFALIGN_CENTER, 0);
		g_object_unref(gc);
		g_object_unref(pixbuf);

		g_input_stream_close(stream, NULL, NULL);
	}
	return FALSE;
}	/* post_lb_snapshot_cb(... */

static gboolean
post_cmb_order_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index, i;
	size_t sz;
	ivca_rule_event_t *evt_temp;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if ( g_order != index ) {
			vsm_playback_smart_preview_stop();
			_preview_off("");

			sz = (size_t)evt_count * sizeof(ivca_rule_event_t);
			evt_temp = malloc(sz);
			if ( evt_temp ) {
				for (i = 0; i < evt_count; i++)
					evt_temp[i] = evt_buffer[evt_count - 1 - i];
				memcpy(evt_buffer, evt_temp, sz);
				free(evt_temp);

				g_beg_logtbl = 0;
				memset(g_idx_logtbl, 0xFF, sizeof(g_idx_logtbl));
				_change_logtbl_selection(NOT_SELECTED, TRUE);
				//_update_snapshot(NOT_SELECTED);
				_update_logtbl();
			}
		}
		g_order = index;
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_UP ) {
			_change_obj_focus(obj, lb_logtbl[_LOGT_ROWS - 1][0]);
			return TRUE;
		}
	}
	return FALSE;
}	/* post_cmb_order_cb(... */

static gboolean
post_btn_logdir_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_PRESS ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		if ( obj == btn_prev )
			_scroll_logtbl(g_beg_logtbl, -_LOGT_ROWS, FALSE);
		else if ( obj == btn_next )
			_scroll_logtbl(g_beg_logtbl, _LOGT_ROWS, FALSE);
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_UP ) {
			_change_obj_focus(obj, lb_logtbl[_LOGT_ROWS - 1][0]);
			return TRUE;
		}
	}
	return FALSE;
}	/* post_btn_logdir_cb(... */

static gboolean
post_nmbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;
		if ( obj == btn_play ) {
			if ( search_started )
				return FALSE;

			_start_playback((g_beg_logtbl + g_sel_logtbl) % MAX_EVENTS);
		}
		else if ( obj == btn_close ) {
			if ( search_started ) {
				ret = nftool_mbox((NFWINDOW *)obj->parent, "WARNING",
						"You may lose the search result.\n"
						"Do you want to close?", NFTOOL_MB_OKCANCEL);
				if ( ret == NFTOOL_MB_CANCEL )
					return FALSE;
			}

			_stop_search();
			vsm_playback_smart_preview_stop();
			VW_Search_Destroy();
			drawing_enable = FALSE;
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		//TODO
	}
	return FALSE;
}	/* post_nmbtn_cb(... */

static gboolean
post_ck_fg_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean val;

	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON || !debug_en )
			return FALSE;

		val = nfui_check_button_get_active(NF_CHECKBUTTON(ck_fg));
		if ( val != fg_en ) {
			nf_play_smart_set_fgdebug(val);
			//captainnn
			//nf_api_vca_set_fgchannel(val ? NF_VCA_CHID_PB : -1);
		}
		fg_en = val;
	}
	return FALSE;
}

/* IMSG event handlers. */

static void
_eventlog_cb(gint ch, gint count, ivca_rule_event_t *event, gchar *snapshots)
{
	gint i = 0;
	GTimeVal tv;

	ivca_rule_event_t *temp;


	if ( ch < 16)
		return;

	if ( evt_count >= MAX_EVENTS - 1 )
		return;

	//XXX Below code is required when detected # > MAX !
	//XXX but in smart search, it may not be happen.....
	//if ( evt_buffer[evt_head].snap_size && evt_buffer[evt_head].snapshot )
	//	free(evt_buffer[evt_head].snapshot);
	//if ( snapshots && event->snap_size ) {
	//	event->snapshot = malloc(event->snap_size);
	//	if ( event->snapshot )
	//		memcpy(event->snapshot, snapshots, event->snap_size);
	//}

	//FIXME is this ok? (timestamp)
	//tv = vsm_playback_get_smarttime();
	//event->timestamp = (guint)tv.tv_sec;

	//event->ch = (guint8)g_channel;

	for(i=0;i<count;i++){
		temp = &event[i];
		temp->ch = (guint8)g_channel;
		memcpy(&evt_buffer[evt_head], temp, sizeof(ivca_rule_event_t));
		evt_head = (evt_head + 1) % MAX_EVENTS;
		evt_pend = MIN(evt_pend + 1, MAX_EVENTS - 1);
		evt_count = MIN(evt_count + 1, MAX_EVENTS - 1);
		events[temp->rule_id]++;
	}
}	/* _eventlog_cb(... */

#if 0
static void
_trackinfo_cb(gint ch, gint count, ivcam_obj_t *ti)
{
	if ( ch != NF_VCA_CHID_PB )
		return;

	if ( g_ti_data ) {
		free(g_ti_data);
		g_ti_count = -1;
	}
	g_ti_data = malloc((guint)count * sizeof(ivcam_obj_t));
	if ( g_ti_data ) {
		memcpy(g_ti_data, ti, (guint)count * sizeof(ivcam_obj_t));
		g_ti_count = count;
	}
}	/* _trackinfo_cb(... */
#endif

static void
_meta_data_cb(gint ch, gint count, ivcam_obj_t* meta)
{

#ifdef USE_IVCAM
	if ( ch != (gint)g_channel )
		return;

	if ( g_meta_data ) {
		free(g_meta_data);
		g_meta_count = -1;
	}
	g_meta_data = malloc((guint)count * sizeof(ivcam_obj_t));
	if ( g_meta_data ) {
		memcpy(g_meta_data, meta, (guint)count * sizeof(ivcam_obj_t));
		g_meta_count = count;
	}
	
#else
	if ( g_meta_data ) {
		free(g_meta_data);
		g_meta_count = -1;
	}
	g_meta_data = malloc((guint)count * sizeof(ivca_meta_obj_t));
	if ( g_meta_data ) {
		memcpy(g_meta_data, meta, (guint)count * sizeof(ivca_meta_obj_t));
		g_meta_count = count;
	}
#endif
	
}	/* _trackinfo_cb(... */

static void
_counter_cb(gint ch, gint count, gint *values)
{
	//if ( ch != NF_VCA_CHID_PB )
	//	return;

	memcpy(_cntv, values, (size_t)count * sizeof(gint));
}	/* _counter_cb(... */

static gboolean
pre_win_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch ( evt->type ) {
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			if ( evt->key.keyval == cheat_code[cheatkey_cnt] )
				cheatkey_cnt++;
			else
				cheatkey_cnt = 0;

			if ( cheatkey_cnt == CHEAT_CODE_SIZE ) {
				cheatkey_cnt = 0;
				if ( !debug_en ) {
					debug_en = TRUE;
					_debug_init();
				}
			}
			break;

		default:
			break;
	}
	return FALSE;
}	/* pre_win_cb(... */

static gboolean
post_page_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NF_NOTIFY_INFO *pinfo;
	gint i, ch, count;
	gint *p;
	gchar *snap;

	switch ( evt->type ) {
		case GDK_DELETE:
			/* Deallocate resources. */
			//for (i = 0; i < MAX_EVENTS; i++)
			//	if ( evt_buffer[i].snapshot )
			//		free(evt_buffer[i].snapshot);
			memset(evt_buffer, 0, sizeof(evt_buffer));
			evt_head = evt_count = evt_pend = 0;
			if ( g_meta_data )
				free(g_meta_data);
			g_meta_data = NULL;
			g_meta_count = -1;
			if ( g_meta_data_p )
				free(g_meta_data_p);
			g_meta_data_p = NULL;

			/* meta data off */
			nf_meta_data_display_playback_off();
			//captainnn
			//nf_api_vca_set_fgchannel(-1);

			uxm_unreg_imsg_event(obj, INFY_VCA_EVENT_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_VCA_TRACKINFO_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_VCA_COUNTER_NOTIFY);

			nfui_regi_pre_event_callback((NFOBJECT *)g_curwnd, NULL);
			nfui_nfwindow_set_returnkey_proc(g_curwnd, NULL);
			g_curwnd = NULL;
			break;
/*
		case INFY_VCA_EVENT_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			ch = ((ivca_rule_event_t*)pinfo->p.ptr)->ch;
			snap = (gchar *)((ivca_rule_event_t *)pinfo->p.ptr + 1);
			_eventlog_cb(ch, pinfo->p.ptr, snap);
			break;

		case INFY_VCA_TRACKINFO_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			p = pinfo->p.ptr;
			ch = ((gint *)p)[0];
			count = ((gint *)p)[1];
			_trackinfo_cb(ch, count, (ivcam_obj_t*)(p + 2));
			break;

		case INFY_VCA_COUNTER_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
			_counter_cb(ch, count, p + 2);
			break;
*/
		case INFY_VCA_EVENT_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			//ch = ((ivca_rule_event_t *)pinfo->p.ptr)->ch;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
		//printf("callback INFY_VCA_EVENT_NOTIFY ch %d count %d \n ",ch,count);
			//snap = (gchar *)((ivca_rule_event_t *)pinfo->p.ptr + 1);
			_eventlog_cb(ch, count, (ivca_rule_event_t *)(p + 2), &snap);
			break;

		case INFY_VCA_TRACKINFO_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
			//printf("callback INFY_VCA_TRACKINFO_NOTIFY ch %d count %d\n ",ch,count);
			//_meta_data_cb(ch, count, (ivca_meta_obj_t *)(p + 2));
			_meta_data_cb(ch, count, (ivcam_obj_t*)(p + 2));
			//_trackinfo_cb(ch, count, (ivca_meta_obj_t *)(p + 2));
			break;

		case INFY_VCA_COUNTER_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
			//printf("callback INFY_VCA_COUNTER_NOTIFY ch %d count %d\n ",ch,count);
			//_counter_cb(ch, count, p + 2);
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_page_cb(... */


static void
_rule_cntr_update_value(void)
{
	gint ofs = _vrd.ncntrs - g_beg_ruletbl, i;
	gint *c = &_cntv[g_beg_ruletbl];
	gint *cp = &_cntv_p[g_beg_ruletbl];
	gchar strbuf[16];

	for (i = 0; i < MIN(ofs, _RULE_ROWS); i++) {
		if ( c[i] != cp[i] ) {
			sprintf(strbuf, "%d", c[i]);
			//captainnn
			if(g_rt)
				nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][3]), strbuf);
			nfui_signal_emit(lb_ruletbl[i][3], GDK_EXPOSE, FALSE);
			cp[i] = c[i];
		}
	}
}	/* _rule_cntr_update_value(... */
#if 0
static void
draw_trackinfo(GdkDrawable *drawable, GdkGC *gc)
{
	gint count;
	ivcam_obj_t *ti;

	if ( g_ti_count < 0 || !search_started )
		return;

	count = g_ti_count;
	ti = g_ti_data;
	g_ti_count = -1;
	g_ti_data = NULL;

	/* Erase previous state. */
	if ( g_ti_data_p ) {
		vw_vca_draw_ti(lb_mnvd, drawable, gc, 1, &_vod, &_vod_p,
				g_ti_count_p, g_ti_data_p, g_R_width, g_R_height);
		free(g_ti_data_p);
	}

	/* Draw current state. */
	if ( ti )
		vw_vca_draw_ti(lb_mnvd, drawable, gc, 0, &_vod, &_vod_p,
				count, ti, g_R_width, g_R_height);

	g_ti_count_p = count;
	g_ti_data_p = ti;
	_vod_p.sw_obj_bb = _vod.sw_obj_bb;
	_vod_p.sw_obj_id = _vod.sw_obj_id;
	_vod_p.sw_obj_tr = _vod.sw_obj_tr;
}	/* draw_trackinfo(... */
#endif


static void
draw_str(GdkDrawable *drawable, GdkGC *gc)
{
	gint i, emask[IVCA_MAX_CALIB_TARGETS];
	static int cnt=0;

	if(cnt++%100 && drawing_enable){
		vw_vca_draw_str(lb_mnvd,drawable, gc, 1, "Rule Setting Mode");
		if(search_paused){
			vw_vca_draw_str(lb_mnvd,drawable, gc, 0, "Pause");
		}
		else if(search_started){
			vw_vca_draw_str(lb_mnvd,drawable, gc, 0, "Searching ...");
		}
		else{
			vw_vca_draw_str(lb_mnvd,drawable, gc, 0, "Rule Setting Mode");
		}
	}

}	/* draw_rules(... */

static void
draw_meta_data(GdkDrawable *drawable, GdkGC *gc)
{

#ifdef USE_IVCAM
	gint count;
	ivcam_obj_t *meta;

	if ( g_meta_count < 0 )
		return;

	count = g_meta_count;
	meta = g_meta_data;
	g_meta_count = -1;
	g_meta_data = NULL;

	/* Erase previous state. */
	if ( g_meta_data_p ) {
		vw_vca_draw_meta2(lb_mnvd, drawable, gc, 1, &_vod, &_vod_p,
				g_meta_count_p, g_meta_data_p, 1920*2, 1080*2,1);
		free(g_meta_data_p);
	}

	/* Draw current state. */
	if ( meta )
		vw_vca_draw_meta2(lb_mnvd, drawable, gc, 0, &_vod, &_vod_p,
				count, meta, 1920*2, 1080*2,1);

	g_meta_count_p = count;
	g_meta_data_p = meta;
	_vod_p.sw_obj_bb= _vod.sw_obj_bb;
	_vod_p.sw_obj_id= _vod.sw_obj_id;
	_vod_p.sw_obj_tr= _vod.sw_obj_tr;
	
#else
	gint count;
	ivca_meta_obj_t *meta;

	if ( g_meta_count < 0 )
		return;

	count = g_meta_count;
	meta = g_meta_data;
	g_meta_count = -1;
	g_meta_data = NULL;

	/* Erase previous state. */
	if ( g_meta_data_p ) {
		vw_vca_draw_meta(lb_mnvd, drawable, gc, 1, &_vod, &_vod_p,
				g_meta_count_p, g_meta_data_p, 1920*2, 1080*2);
		free(g_meta_data_p);
	}

	/* Draw current state. */
	if ( meta )
		vw_vca_draw_meta(lb_mnvd, drawable, gc, 0, &_vod, &_vod_p,
				count, meta, 1920*2, 1080*2);

	g_meta_count_p = count;
	g_meta_data_p = meta;
	_vod_p.sw_obj_bb= _vod.sw_obj_bb;
	_vod_p.sw_obj_id= _vod.sw_obj_id;
	_vod_p.sw_obj_tr= _vod.sw_obj_tr;
	
#endif
}	/* draw_trackinfo(... */

static void
draw_rules(GdkDrawable *drawable, GdkGC *gc)
{
	gint i, emask[MAX(IVCA_MAX_CNTRS, IVCA_MAX_ZONES)];
	gint ensel_z = g_rt == 0 || instant_erase;
	gint ensel_c = g_rt == 1 || instant_erase;

	if ( _vod.sw_rule ) {
		/* Erase previous state. */
		/* Draw counters. */
		for (i = 0; i < IVCA_MAX_CNTRS; i++)
			emask[i] = (g_sel_ridx_p == i && g_sel_ridx != i) ||
					memcmp(&_vrd_p.cntrlist[i], &_vrd.cntrlist[i],
					sizeof(ivca_cntr_t));
		vw_vca_draw_cntrs(lb_mnvd, drawable, gc, ensel_c, 1, emask, &_vrd_p,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_rule_font, _cntv);

		/* Draw zones. */
		for (i = 0; i < IVCA_MAX_ZONES; i++)
			emask[i] = (g_sel_ridx_p == i && g_sel_ridx != i) ||
					(g_sel_ridx == i && (g_sel_ridx_p != i ||
					g_sel_pidx_p != g_sel_pidx)) ||
					memcmp(&_vrd_p.zonelist[i], &_vrd.zonelist[i],
					sizeof(ivca_zone_t));
		vw_vca_draw_zones(lb_mnvd, drawable, gc, ensel_z, 1, emask, &_vrd_p,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_sel_pidx, g_sel_pidx_p);

		/* Draw current state. */
		/* Draw counters. */
		vw_vca_draw_cntrs(lb_mnvd, drawable, gc, ensel_c, 0, NULL, &_vrd,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_rule_font, _cntv);

		/* Draw zones. */
		vw_vca_draw_zones(lb_mnvd, drawable, gc, ensel_z, 0, NULL, &_vrd,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_sel_pidx, g_sel_pidx_p);
	}
	else if ( _vod_p.sw_rule) {
		/* Erase previous state. */
		for (i = 0; i < (gint)(sizeof(emask) / sizeof(gint)); i++)
			emask[i] = 1;
		/* Draw counters. */
		vw_vca_draw_cntrs(lb_mnvd, drawable, gc, ensel_c, 1, emask, &_vrd_p,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_rule_font, _cntv);
		/* Draw zones. */
		vw_vca_draw_zones(lb_mnvd, drawable, gc, ensel_z, 1, emask, &_vrd_p,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_sel_pidx, g_sel_pidx_p);
	}

	_rule_cntr_update_value();

	instant_erase = 0;
	g_sel_ridx_p = g_sel_ridx;
	g_sel_pidx_p = g_sel_pidx;
	memcpy(&_vrd_p, &_vrd, sizeof(_vrd_p));
	_vod_p.sw_rule = _vod.sw_rule;
}	/* draw_rules(... */

static void
update_eventlog(void)
{
	gint pend, head_p, count_p, cnt_p, beg_p;

	if ( evt_pend <= 0 )
		return;		/* There is no new event log. */

	pend = evt_pend;
	evt_pend = 0;
	head_p = (MAX_EVENTS + evt_head - pend) % MAX_EVENTS;
	count_p = evt_count;
	cnt_p = (MAX_EVENTS + head_p - g_beg_logtbl) % MAX_EVENTS;
	beg_p = g_beg_logtbl;
	if ( cnt_p == _LOGT_ROWS ||
			(count_p - pend <= _LOGT_ROWS && count_p > _LOGT_ROWS) )
		/* Auto-scroll. */
		g_beg_logtbl = (MAX_EVENTS + evt_head - _LOGT_ROWS) % MAX_EVENTS;
	if ( cnt_p + pend >= MAX_EVENTS )
		g_beg_logtbl = (evt_head + 1) % MAX_EVENTS;

	_update_logtbl();

	/* Fix focus to the last. */
	if ( !(count_p - pend) ||	/* The first events. */
			(g_sel_logtbl < _LOGT_ROWS &&
			((beg_p + g_sel_logtbl + 1) % MAX_EVENTS) == head_p) ) {
		/* The last. */
		vsm_playback_smart_preview_stop();
		_preview_off("");
		_change_logtbl_selection(MIN(evt_count, _LOGT_ROWS) - 1, TRUE);
		//_update_snapshot(g_sel_logtbl);
	}
	/* Move focus. */
	else if ( (cnt_p == _LOGT_ROWS || cnt_p + pend >= MAX_EVENTS) &&
			g_sel_logtbl != NOT_SELECTED )
		_change_logtbl_selection(g_sel_logtbl - pend, TRUE);

	_update_result_tbl(TRUE);
}	/* update_eventlog(... */

static gboolean
_proc_timer_draw(void *data)
{
	GdkDrawable *drawable;
	GdkGC *gc;

	static int cnt=0;

	NFUTIL_THREADS_ENTER();
	if ( !g_curwnd ) {
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}


	if ( drawing_enable ) {
		
		if(!search_started || (search_started && (cnt++%20 == 0))){ 
			drawable = nfui_nfobject_get_window(lb_mnvd);
			gc = gdk_gc_new(drawable);

			draw_str(drawable, gc);

			draw_rules(drawable, gc);
			
			draw_meta_data(drawable, gc);
			
			//draw_trackinfo(drawable, gc);

			g_object_unref(gc);
		}
	}
	NFUTIL_THREADS_LEAVE();
	return TRUE;
}	/* _proc_timer_draw(... */

static gboolean
_proc_timer_eventlog(void *data)
{
	NFUTIL_THREADS_ENTER();
	if ( !g_curwnd ) {
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}
	//test
//if ( !search_started ) 
	update_eventlog();
	//if ( debug_en )
	//	_update_debuginfo();

	NFUTIL_THREADS_LEAVE();
	return TRUE;
}	/* _proc_timer_eventlog(... */

static gboolean
_proc_timer_playtime(void *data)
{
	GTimeVal tv;
	gchar strbuf[64];
	static time_t preview_time_p = 0;
	gint l;
	DIR_RATE_E dir_rate;
	//captainnn
	static int fps;
	int cur_fps;

	NFUTIL_THREADS_ENTER();
	if ( !g_curwnd ) {
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	/* Check preview time. */
	tv = vsm_playback_get_previewtime();
	if ( tv.tv_sec != preview_time_p ) {
		if ( tv.tv_sec )
			dtf_get_local_datetime(tv.tv_sec, strbuf);
		else
			strbuf[0] = '\0';
		nfui_nflabel_set_text(NF_LABEL(lb_prvwtime), strbuf);
		nfui_signal_emit(lb_prvwtime, GDK_EXPOSE, FALSE);
		preview_time_p = tv.tv_sec;
	}

	/* Check mainview time & status. */
	if ( search_started ) {
		dir_rate = vsm_playback_get_smart_dir_rate();
		if ( dir_rate == DR_STOP )
			tv.tv_sec = t_end;	/* End video. */
		else
			tv = vsm_playback_get_smarttime();
		if ( tv.tv_sec ) {
			l = dtf_get_local_datetime(tv.tv_sec, strbuf);
			sprintf(&strbuf[l], "    (%.2f%%)",
					(float)(tv.tv_sec - t_beg) * 100 / (float)(t_end - t_beg));
			if ( tv.tv_sec >= t_end || evt_count >= MAX_EVENTS - 1 ) {	
				
					
				_stop_search();
				/* Check whether # of events exceeds MAX_EVENTS or not. */
				if ( evt_count >= MAX_EVENTS - 1 )
					nftool_mbox((NFWINDOW *)btn_stop->parent, "CONFIRM",
							"Maximum number of events are detected.\n"
							"You have to restart from the stopped position.",
							NFTOOL_MB_OK);
				else{
					char strbuf[128];
					
					GTimeVal tv;

					gettimeofday(&tv, NULL);

					sprintf(strbuf, "Smart Searching is done in %d seconds (%d frames)\n", tv.tv_sec - g_tv.tv_sec,nf_meta_data_get_frame_cnt());
					nftool_mbox((NFWINDOW *)btn_stop->parent, "CONFIRM",
							strbuf, NFTOOL_MB_OK);
					
					memcpy(evt_buffer_saved,evt_buffer,sizeof(evt_buffer_saved));
					evt_head_saved = evt_head;
					evt_pend_saved = evt_pend;
					evt_count_saved = evt_count;

				}
				
			}
#if 1
			else {
				static int reflesh=0;
				reflesh++;
				if(reflesh%20 == 0 && search_paused == FALSE){
					GTimeVal tv_end;
					
					tv_end.tv_sec = tv.tv_sec + 100;
					vsm_playback_smart_mainview_start(g_channel,tv,tv_end,NF_PLAY_SMART_SEARCH_META_VIEW);
					
				}
				else if(reflesh%20 == 1 && search_paused == FALSE){

					vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_META_VIEW);
				
				}
			}
#endif

			nfui_nflabel_set_text(NF_LABEL(lb_mnvwtime), strbuf);
			nfui_signal_emit(lb_mnvwtime, GDK_EXPOSE, FALSE);

			
		}

		if(!search_paused){
			vsm_playback_smart_preview_stop();
		}
		//captainnn
		/* check stream fps */
		#if 0
		cur_fps = nf_play_smart_get_fps();
		if(cur_fps != fps && cur_fps != 0){
			if(cur_fps < 15){
				nftool_mbox((NFWINDOW *)btn_stop->parent, "CONFIRM",
							"Current fps is smaller than 15 fps.\n"
							"Result of smart search is not guaranteed.",
							NFTOOL_MB_OK);
			}
			fps = cur_fps;
		}
		#endif
		
	}

	NFUTIL_THREADS_LEAVE();
	return TRUE;
}	/* _proc_timer_playtime(... */

static void
_debug_init(void)
{
	NFOBJECT *obj;

	lb_cpu = vw_label_create(fixed1, "", NFFONT_MINI_SEMI_5,
			NFALIGN_LEFT, 0, 0, 1, 120, 0, 23, 750, 150, 20, NULL);
	lb_mem = vw_label_create(fixed1, "", NFFONT_MINI_SEMI_5,
			NFALIGN_LEFT, 0, 0, 1, 120, 0, 23, 770, 150, 20, NULL);
	lb_ftm = vw_label_create(fixed1, "", NFFONT_MINI_SEMI_5,
			NFALIGN_LEFT, 0, 0, 1, 120, 0, 23, 790, 150, 20, NULL);
	lb_frt = vw_label_create(fixed1, "", NFFONT_MINI_SEMI_5,
			NFALIGN_LEFT, 0, 0, 1, 120, 0, 23, 810, 150, 20, NULL);
	ind_cpu = _create_ind(fixed1, 3, 16, 174, 750+1);
	ind_mem = _create_ind(fixed1, 3, 16, 174, 770+1);

	ck_fg = nfui_checkbutton_new(fg_en);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(ck_fg),
			NFCHECK_TYPE_SMALL);
	nfui_nfobject_show(ck_fg);
	nfui_regi_post_event_callback(ck_fg, post_ck_fg_cb);
	nfui_nffixed_put((NFFIXED *)fixed1, ck_fg, 225, 750);
	obj = vw_label_create(fixed1, "Show FG", NFFONT_MINI_SEMI_5,
			NFALIGN_LEFT, 0, 0, 1, 120, 0, 245, 750, 100, 20, NULL);

	nfui_signal_emit(lb_cpu, GDK_EXPOSE, FALSE);
	nfui_signal_emit(lb_mem, GDK_EXPOSE, FALSE);
	nfui_signal_emit(lb_ftm, GDK_EXPOSE, FALSE);
	nfui_signal_emit(lb_frt, GDK_EXPOSE, FALSE);
	nfui_signal_emit(ck_fg, GDK_EXPOSE, FALSE);
	nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
	nfui_signal_emit(ind_cpu, GDK_EXPOSE, FALSE);
	nfui_signal_emit(ind_mem, GDK_EXPOSE, FALSE);

	if ( fg_en ) {
		nf_play_smart_set_fgdebug(TRUE);
		//captainnn
		//nf_api_vca_set_fgchannel(NF_VCA_CHID_PB);
	}
}	/* _debug_init(... */

static void cb_meta_data(int ch , int cnt, ivcam_obj_t* data)
{
	_meta_data_cb(ch, cnt, (ivcam_obj_t*)data);
}

void
vw_init_SearchBySmart_page(NFOBJECT *parent, time_t from, time_t to)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;
	guint width1[_TIME_COLS];
	gint size_w, size_h, n_width, n_height, Rx, Ry;
	guint i, tformat;
	gchar *strCh[GUI_CHANNEL_CNT];
	DateTimeData dtdata;
	GTimeVal time = {0, 0};
	GdkPixbuf *time_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];
	gchar *strTitle1[] = {"FROM", "TO"};
	gchar *strLogDir[] = {"LATEST", "OLDEST"};
	gchar *strRule[2] = {"ZONE", "COUNTER"};
	guint tbl_rule_w[_RULE_COLS] = {80, 24, 261, 80, 80};
	gchar *tbl_rule_strcol[_RULE_COLS] = {"ID", "", "NAME", "TYPE", "ACTIVE"};	
	guint tbl_result_w[_RES_COLS] = {50, 24, 146, 80};
	gchar *tbl_result_strcol[_RES_COLS] = {"ID", "", "NAME", "EVENTS"};
	guint tbl_log_w[_LOGT_COLS] = {80, 260, 80, 100, 200};
	gchar *tbl_log_strcol[_LOGT_COLS] = {
			"No.", "Time", "Rule ID", "Object ID", "Event Type"
	};

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

	uxm_reg_imsg_event(parent, INFY_VCA_EVENT_NOTIFY);
	uxm_reg_imsg_event(parent, INFY_VCA_TRACKINFO_NOTIFY);
	uxm_reg_imsg_event(parent, INFY_VCA_COUNTER_NOTIFY);

	/* Initialize variables. */
	memset(&dtdata, 0x00, sizeof(DateTimeData));
	DAL_get_dateTime_data(&dtdata);
	DAL_get_dateTime_format(NULL, &tformat);

	_init_data();
//captainnn
//	debug_en = nf_api_vca_check_debug_enabled();
	fg_en = FALSE;

	/* Get vca coordinate information. */
//	if ( !nf_api_vca_get_vsrc(NF_VCA_CHID_PB, NULL, &g_width, &g_height) ) {
//		/* Set to default value. */
//		g_width = 320;
//		g_height = 180;
//	}

		g_width = 640;
		g_height = 360;

	//nf_api_vca_get_coordinate(&n_width, &n_height);
	n_width = 1920*2;
	n_height = 1080*2;
	
	g_rx = n_width / _MNVD_LBL_W;
	g_ry = n_height / _MNVD_LBL_H;

	Rx = n_width / g_width;
	Ry = n_height / g_height;
	g_R_width = g_width * Rx;
	g_R_height = g_height * Ry;

	/* Load images. */
	time_img[0] = nfui_get_image_from_file(IMG_BT_DATETIME_N, NULL);
	time_img[1] = nfui_get_image_from_file(IMG_BT_DATETIME_O, NULL);
	time_img[2] = nfui_get_image_from_file(IMG_BT_DATETIME_P, NULL);
	time_img[3] = nfui_get_image_from_file(IMG_BT_DATETIME_D, NULL);

	prev_img[0] = nfui_get_image_from_file(IMG_SUBMENU_TL_PREV_N_BTN, NULL);
	prev_img[1] = nfui_get_image_from_file(IMG_SUBMENU_TL_PREV_O_BTN, NULL);
	prev_img[2] = nfui_get_image_from_file(IMG_SUBMENU_TL_PREV_P_BTN, NULL);
	prev_img[3] = nfui_get_image_from_file(IMG_SUBMENU_TL_PREV_D_BTN, NULL);

	next_img[0] = nfui_get_image_from_file(IMG_SUBMENU_TL_NEXT_N_BTN, NULL);
	next_img[1] = nfui_get_image_from_file(IMG_SUBMENU_TL_NEXT_O_BTN, NULL);
	next_img[2] = nfui_get_image_from_file(IMG_SUBMENU_TL_NEXT_P_BTN, NULL);
	next_img[3] = nfui_get_image_from_file(IMG_SUBMENU_TL_NEXT_D_BTN, NULL);

	nfui_get_pixbuf_size(time_img[0], &size_w, &size_h);

	g_rule_font = nffont_get_pango_font(NFFONT_MEDIUM_SEMI);

	/* Fixed. */
	content_fixed = vw_fixed_create(parent, 0, 1, MENU_V_INNER_X,
			MENU_V_INNER_Y, MENU_H_INNER_W, MENU_H_INNER_H, NULL);
	fixed1 = vw_fixed_create(content_fixed, 0, 1, _FIXED1_X, _FIXED1_Y,
			_FIXED1_W, _FIXED1_H, pre_fixed1_cb);
	fixed2 = vw_fixed_create(content_fixed, 0, 1, _FIXED2_X, _FIXED2_Y,
			_FIXED2_W, _FIXED2_H, NULL);

	/* Mainview. */
	lb_mnvd = vw_label_create(fixed1, "", NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 0,
			1, 1, 665, 666, _MNVD_LBL_X, _MNVD_LBL_Y, _MNVD_LBL_W, _MNVD_LBL_H,
			post_lb_mnvd_cb);
	nfui_nfobject_modify_bg(lb_mnvd, NFOBJECT_STATE_NORMAL,
			COLOR_PRG_IDX(UX_COLOR_000000));

	/* From/To. */
	width1[0] = 80;
	width1[2] = (guint)size_w;
	width1[1] = _TIME_TBL_W - width1[0] - width1[2];
	tbl = (NFOBJECT *)nfui_nftable_new(_TIME_COLS, _TIME_ROWS, 0, 2,
			width1, _LBL_H1);
	nfui_nfobject_show(tbl);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nffixed_put((NFFIXED *)fixed1, tbl, _TIME_TBL_X, _TIME_TBL_Y);

	for (i = 0; i < _TIME_ROWS; i++) {
		vw_label_create(tbl, strTitle1[i], NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
				0, 1, 120, 0, 0, (gint)i, -1, -1, NULL);

		time.tv_sec = i == 0 ? from : to;

		obj = (NFOBJECT *)nfui_nftimelabel_new();
		nfui_nftimelabel_set_fg_color((NFTIMELABEL *)obj, COLOR_IDX(129));
		nfui_nftimelabel_set_bg_color((NFTIMELABEL *)obj, COLOR_IDX(128));
		nfui_nftimelabel_set_mode((NFTIMELABEL *)obj,
				prvTransDateFormat((gint)dtdata.dateFormat), tformat + 1);
		nfui_nftimelabel_set_size((NFTIMELABEL *)obj, width1[1], _LBL_H1);
		nfui_nftimelabel_set_datetime((NFTIMELABEL *)obj, &time);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE *)tbl, obj, 1, i);
		if ( i == 0 )
			tlb_from = obj;
		else
			tlb_to = obj;

		obj = _create_button(tbl, time_img, 1, 2, i, post_btn_fromto_cb);

		if ( i == 0 )
			btn_from = obj;
		else
			btn_to = obj;
	}

	/* Channel. */
	vw_label_create(fixed1, "CHANNEL", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CHAN_LBL_X, _CHAN_LBL_Y, _CHAN_LBL_W, _LBL_H1, NULL);

	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		strCh[i] = malloc(64);
		if ( strCh[i] )
			sprintf(strCh[i], "CH %02d", i + 1);
	}
	cmb_ch = vw_combo_create(fixed1, strCh, GUI_CHANNEL_CNT, 0, NULL, 1, 1,
			_CHAN_CMB_X, _CHAN_LBL_Y, _CHAN_CMB_W, _LBL_H1, post_cmb_ch_cb);
	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		if ( strCh[i] )
			free(strCh[i]);

	/* Option button. */
	btn_option = vw_nmbutton_create(fixed1, "DISPLAY OPTION", 3, TRUE,
			_OPTI_BTN_X, _OPTI_BTN_Y, 203, post_btn_option_cb);

	/* Rule. */
	vw_label_create(fixed1, "RULES", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 662, 0, _RULE_LBL_X, _RULE_LBL_Y, _RULE_LBL_W, _LBL_H1, NULL);
	#if 0
	cmb_rt = vw_combo_create(fixed1, strRule, 2, 0, NULL, 1, 1,
			_RULE_CMB_X, _RULE_CMB_Y, _RULE_CMB_W, _LBL_H1, post_cmb_rt_cb);
	#endif

	/* Rule table. */
	tbl_rule = vw_table_create(fixed1, (NFOBJECT **)lb_ruletbl,
			_RULE_COLS, _RULE_ROWS, 1, 1, tbl_rule_w, 22, tbl_rule_strcol, 0,
			116, 115, 389, 138, _RULE_TBL_X, _RULE_TBL_Y, post_lb_ruletbl_cb);

	_create_vscrollbar(fixed1, &vsc_rule, _RULE_ROWS, _RULE_VSC_X, _RULE_VSC_Y,
			_RULE_TBL_H, _SCBTN_W, _SCBTN_H, post_vsc_rule_cb);

	/* Edit buttons. */
	btn_add = vw_nmbutton_create(fixed1, "ADD", 3, TRUE,
			_RADD_BTN_X, _RADD_BTN_Y, _EDIT_BTN_W, post_btn_edit_cb);
	btn_del = vw_nmbutton_create(fixed1, "DELETE", 3, FALSE,
			_RDEL_BTN_X, _RDEL_BTN_Y, _EDIT_BTN_W, post_btn_edit_cb);
	btn_mod = vw_nmbutton_create(fixed1, "MODIFY", 3, FALSE,
			_RMOD_BTN_X, _RMOD_BTN_Y, _EDIT_BTN_W, post_btn_edit_cb);

	/* Search button. */
	btn_search = vw_nmbutton_create(fixed1, "SEARCH", 3, TRUE,
			_SRCH_BTN_X, _SRCH_BTN_Y, 203, post_btn_search_cb);

	/* Stop button. */
	btn_stop = vw_nmbutton_create(fixed1, "STOP", 3, FALSE,
			_STOP_BTN_X, _STOP_BTN_Y, 203, post_btn_stop_cb);

	/* Mainview time & progress. */
	lb_mnvwtime = vw_label_create(fixed1, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_CENTER, 0, FALSE, 1, 664, 0, _MVTM_LBL_X,
			_MVTM_LBL_Y, _MVTM_LBL_W, _LBL_H1, NULL);

	/* Log table. */
	vw_table_create(fixed2, (NFOBJECT **)lb_logtbl, _LOGT_COLS, _LOGT_ROWS,
			1, 1, tbl_log_w, 25, tbl_log_strcol, 0, 116, 115, 139, 138,
			_LOGT_TBL_X, _LOGT_TBL_Y, post_lb_logtbl_cb);

	_create_vscrollbar(fixed2, &vsc_log, _LOGT_ROWS, _LOGT_VSC_X, _LOGT_VSC_Y,
			_LOGT_TBL_H, _SCBTN_W, _SCBTN_H, post_vsc_log_cb);

	lb_logcnt = vw_label_create(fixed2, "", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 664, 0, _LOGC_LBL_X, _LOGC_LBL_Y, _LOGC_LBL_W, _LBL_H2, NULL);

	
	/* Search Result */
	vw_label_create(fixed2, "SEARCH RESULT", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 662, 0, _RES_TBL_X, _RES_TBL_Y, _PRVW_LBL_W, _LBL_H2, NULL);
	
	tbl_search_result= vw_table_create(fixed2, (NFOBJECT **)lb_search_result,
			_RES_COLS, _RES_ROWS, 1, 1, tbl_result_w, 22, tbl_result_strcol, 0,
			116, 115, 389, 138, _RES_TBL_X, _RES_TBL_Y + _LBL_H2 + 5, post_lb_result_cb);

	_create_vscrollbar(fixed2, &vsc_result, _RES_ROWS, _RES_VSC_X, _RES_VSC_Y + _LBL_H2 + 5,
			_RES_TBL_H, _SCBTN_W, _SCBTN_H, post_vsc_result_cb);

	lb_total_logcnt = vw_label_create(fixed2, "", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 664, 0, _LOGTC_LBL_X, _LOGTC_LBL_Y, _LOGTC_LBL_W, _LBL_H2, NULL);

	///* Reset Result. */
	//btn_reset_result = vw_nmbutton_create(fixed2, "TOTAL RESULT", 3, TRUE,
	//		_LOGC_LBL_X, _LOGC_LBL_Y + 30, 203, post_btn_result_cb);
	
	/* Detail Search */
	btn_detail_search = vw_nmbutton_create(fixed2, "DETAIL SEARCH", 3, TRUE,
			_PRVW_LBL_X + 350 + 200 , _PRVW_LBL_Y - 18, 203, post_btn_detail_search_cb);

	/* Preview. */
	vw_label_create(fixed2, "PREVIEW", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 662, 0, _PRVW_LBL_X + 350, _PRVW_LBL_Y, _PRVW_LBL_W, _LBL_H2, NULL);

	lb_prvd = vw_label_create(fixed2, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_CENTER, 0, 0, 1, 665, 666, _PRVD_LBL_X + 350, _PRVD_LBL_Y,
			_PRVD_LBL_W, _PRVD_LBL_H, NULL);
	lb_prvwtime = vw_label_create(fixed2, "", NFFONT_MINI_SEMI_5,
			NFALIGN_CENTER, 0, 0, 1, 664, 0, _PRTM_LBL_X + 350,
			_PRTM_LBL_Y, _PRTM_LBL_W, _LBL_H2, NULL);
#if 0
	/* Snapshot. */
	vw_label_create(fixed2, "SNAPSHOT", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 662, 0, _SNAP_LBL_X, _SNAP_LBL_Y, _SNAP_LBL_W, _LBL_H2, NULL);

	lb_snapshot = vw_label_create(fixed2, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_CENTER, 0, 0, 1, 665, 666, _SNIM_LBL_X, _SNIM_LBL_Y,
			_SNIM_LBL_W, _SNIM_LBL_H, post_lb_snapshot_cb);
#endif

	/* Log order. */
	vw_label_create(fixed2, "ORDER BY", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _LOGO_LBL_X, _LOGO_LBL_Y, _LOGO_LBL_W, _LBL_H1, NULL);

	cmb_order = vw_combo_create(fixed2, (gchar **)strLogDir, 2, 1, NULL, 1, 1,
			_LOGO_CMB_X, _LOGO_CMB_Y, _LOGO_CMB_W, _LBL_H1, post_cmb_order_cb);
	nfui_nfobject_disable(cmb_order);

	/* Log dir. */
	btn_prev = _create_button(fixed2, prev_img, 0,
			_LOGP_BTN_X, _LOGP_BTN_Y, post_btn_logdir_cb);
	btn_next = _create_button(fixed2, next_img, 0,
			_LOGN_BTN_X, _LOGN_BTN_Y, post_btn_logdir_cb);

	/* Playback/Close Buttons. */
	btn_play = vw_nmbutton_create(parent, "PLAYBACK", 1, FALSE,
			MENU_H_BTN_R2_X, MENU_H_BTN_Y, MENU_BTN_WIDTH, post_nmbtn_cb);
	btn_close = vw_nmbutton_create(parent, "CLOSE", 2, TRUE,
			MENU_H_BTN_R1_X, MENU_H_BTN_Y, MENU_BTN_WIDTH, post_nmbtn_cb);

	/* Debug information. */
	if ( debug_en )
		_debug_init();

	/* Set initial state. */
	_rule_update(NOT_SELECTED, TRUE);

	set_meta_data_callback(cb_meta_data, NULL);

	/* Timers. */
	g_timeout_add(40, _proc_timer_draw, 0);
	g_timeout_add(250*4, _proc_timer_eventlog, 0);
	g_timeout_add(250, _proc_timer_playtime, 0);

	nfui_regi_pre_event_callback((NFOBJECT *)g_curwnd, pre_win_cb);
	nfui_regi_post_event_callback(parent, post_page_cb);

	nfui_make_key_hierarchy(g_curwnd);
	nfui_nfwindow_set_returnkey_proc(g_curwnd, returnkey_proc);
}	/* vw_init_SearchBySmart_page(... */

gboolean vw_SearchBySmart_tab_out_handler(void)
{

	_stop_search();

	drawing_enable = FALSE;

	vsm_playback_smart_preview_stop();
	_preview_off("");
	nf_play_unset_smart_geometry();
	return TRUE;
}	/* vw_SearchBySmart_tab_out_handler(... */

gboolean vw_SearchBySmart_tab_in_handler(void)
{
	//captainnn
	while (1) {
		usleep(100000);
		if (!nfui_nfthumbnail_check_running()) break;
	}
	
	drawing_enable = TRUE;
	nf_play_set_smart_geometry(g_channel, 48, 64, 960, 540);
	if ( debug_en )
		nf_play_smart_set_fgdebug(fg_en);
	//captiannn
	_show_search_frame();
	nfui_nflabel_set_text(NF_LABEL(lb_mnvwtime), "");
	nfui_signal_emit(lb_mnvwtime, GDK_EXPOSE, FALSE);
	return TRUE;
}	/* vw_SearchBySmart_tab_in_handler(... */

gboolean vw_SearchBySmart_tab_show(void)
{
	vw_SearchBySmart_tab_in_handler();
	return TRUE;
}	/* vw_SearchBySmart_tab_show(... */

void set_draw(gboolean draw)
{
	drawing_enable = draw;
}	


