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
2012/02/27 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_sys_camera_vca_prop.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nfimglabel.h"
#include "objects/nflabel.h"
#include "objects/nfobject.h"
#include "objects/nfspinbutton.h"
#include "objects/nftable.h"
#include "support/event_loop.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "scm.h"
#include "vsm.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_vca.h"
#include "vw_sys_camera_vca_prop.h"
//#include "vw_sys_camera_vca_zoom.h"
#include "vw_tools.h"

#include <gtk/gtk.h>
#include <string.h>
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "tools/ix_func.h"
#include "tools/ix_mem.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nftab.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/vw_vkeyboard.h"
#include "cmm.h"
#include "uxm.h"
#include "nf_api_live.h"
#include "nf_meta_data.h"
#include "nf_notify.h"

#include "vw_sys_camera_vca_setup_internal.h"
#include "vw_vca_internal.h"

#include <math.h>
#include <pthread.h>

//captainnn
#include "ivca_def.h"
#include "libivcam.h"
#include "vw_vca_rule_config.h"
#include "dtf.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	VCA_NUM_COLUMNS				3
#define	VCA_COL_SPACE				2
#define	VCA_ROW_SPACE				1

#define	VCA_TABLE_LEFT				(4)
#define	VCA_TABLE_TOP				(0)

#define	VCA_CELL_HEIGHT				40

//captainnn0
#define   VCA_WIDTH		640
#define   VCA_HEIGHT		360

#define	L_NCOLS			5//7
#define	L_NROWS			8

#define	NOT_SELECTED	-100000

#define   _GAP_X		10
#define   _GAP_Y		10


#define	_LBL_W			200
#define	_LBL_H			40

#define	_CMB_W			230//250
#define	_CMB_H			40

#define	_SCBTN_W		25
#define	_SCBTN_H		25

/* Display Window */

#define	_DISPLAY_X		4
#define	_DISPLAY_Y		4
#define	_DISPLAY_W		960// + 80 + 48
#define	_DISPLAY_H		540// + 45 + 27



/* Channel. */
#define	_CH_LBL_X			_DISPLAY_X
#define	_CH_LBL_Y			_DISPLAY_H + 2*_GAP_Y
#define	_CH_LBL_W			_LBL_W
#define	_CH_LBL_H			_LBL_H

#define	_CH_CMB_X			_CH_LBL_X + _CH_LBL_W
#define	_CH_CMB_Y			_CH_LBL_Y
#define	_CH_CMB_W			_CMB_W
#define	_CH_CMB_H			_CMB_H

/* Activation */
#define	_ACT_LBL_X			_DISPLAY_X
#define	_ACT_LBL_Y			_CH_LBL_Y + _CH_LBL_H + _GAP_Y
#define	_ACT_LBL_W			_LBL_W
#define	_ACT_LBL_H			_LBL_H

#define	_ACT_CMB_X			_ACT_LBL_X + _CH_LBL_W
#define	_ACT_CMB_Y			_ACT_LBL_Y
#define	_ACT_CMB_W		_CMB_W
#define	_ACT_CMB_H		_CMB_H

/* Detecting mark */
#define	_DET_LBL_X			_DISPLAY_X
#define	_DET_LBL_Y			_ACT_LBL_Y + _ACT_LBL_H + _GAP_Y
#define	_DET_LBL_W			_LBL_W
#define	_DET_LBL_H			_LBL_H

#define	_DET_CMB_X			_DET_LBL_X + _DET_LBL_W
#define	_DET_CMB_Y			_DET_LBL_Y
#define	_DET_CMB_W		_CMB_W
#define	_DET_CMB_H		_CMB_H

/* Display Option */
//#define	_OPT_LBL_X			_DISPLAY_X + _DISPLAY_W + _GAP_X
//#define	_OPT_LBL_Y			_DISPLAY_Y
//#define	_OPT_LBL_W			_LBL_W
//#define	_OPT_LBL_H			_LBL_H

//#define	_OPT_CHK_X			_OPT_LBL_X
//#define	_OPT_CHK_Y			_DISPLAY_Y + _OPT_LBL_H + _GAP_Y

/* Calibration */
#define	_CAL_LBL_X			_DISPLAY_X + _DISPLAY_W + _GAP_X//_OPT_LBL_X
#define	_CAL_LBL_Y			_DISPLAY_Y//_OPT_CHK_Y + 8*_GAP_Y
#define	_CAL_LBL_W			_LBL_W
#define	_CAL_LBL_H			_LBL_H

/* Calibration Set */
#define   _CAL_SET_X			_CAL_LBL_X + _CAL_LBL_W
#define   _CAL_SET_Y			_CAL_LBL_Y
#define   _CAL_SET_W		190

/* Calibration Buttons */
#define   _CAL_BTN_X			_CAL_LBL_X
#define   _CAL_BTN_Y			_CAL_LBL_Y + 20*_GAP_Y

/* Calibration Result */
#define   _CAL_RES_X			_CAL_LBL_X
#define   _CAL_RES_Y			_CAL_BTN_Y + 13*_GAP_Y

/* RUle label. */
#define	_RULE_LBL_X		_CH_CMB_X + _CH_CMB_W + 3*_GAP_X//_OPT_LBL_X
#define	_RULE_LBL_Y		_CH_LBL_Y//_OPT_CHK_Y + 15*_GAP_Y
#define	_RULE_LBL_W		_LBL_W
#define	_RULE_LBL_H		_LBL_H

/* Rule combo. */
#define	_RULE_CMB_X		_RULE_LBL_X + _RULE_LBL_W + 9*_GAP_X
#define	_RULE_CMB_Y		_RULE_LBL_Y
#define	_RULE_CMB_W		180
#define   _RULE_CMB_H		_CMB_H

/* Rule table. */
#define	_RULE_COLS			5
#define	_RULE_ROWS		5
#define	_RULE_TBL_X		_RULE_LBL_X
#define	_RULE_TBL_Y		(_RULE_LBL_Y + _LBL_H + _GAP_Y/2)
#define	_RULE_TBL_W		450
#define	_RULE_TBL_H		((22 + 1) * (_RULE_ROWS + 1) - 1)

/* Rule table scroll bar. */
#define	_RULE_VSC_X		(_RULE_TBL_X + _RULE_TBL_W)
#define	_RULE_VSC_Y		_RULE_TBL_Y

/* Add button. */
#define   _ADD_LINE_X		_RULE_LBL_X//_OPT_LBL_X
#define   _ADD_LINE_Y		_RULE_TBL_Y + _RULE_TBL_H +  _GAP_Y
#define   _ADD_LINE_W		155

#define   _ADD_AREA_X		_ADD_LINE_X + _ADD_LINE_W + 5
#define   _ADD_AREA_Y		_ADD_LINE_Y
#define   _ADD_AREA_W		_ADD_LINE_W

#define   _ADD_CNT_X		_ADD_AREA_X + _ADD_LINE_W + 5
#define   _ADD_CNT_Y		_ADD_LINE_Y
#define   _ADD_CNT_W		_ADD_LINE_W

/* Delete button */
#define   _DEL_X				_ADD_LINE_X
#define   _DEL_Y				_ADD_LINE_Y + 5*_GAP_Y
#define   _DEL_W				_ADD_LINE_W

#define   _DEL_ALL_X			_DEL_X + _ADD_LINE_W + 5 
#define   _DEL_ALL_Y			_DEL_Y
#define   _DEL_ALL_W			_ADD_LINE_W

/* Modify button */
#define   _MOD_X				_DEL_ALL_X + _ADD_LINE_W + 5
#define   _MOD_Y				_DEL_Y
#define   _MOD_W			_ADD_LINE_W

/* VCA Option button */
#define   _VCA_OPT_X			_CH_LBL_X
#define   _VCA_OPT_Y			_ACT_LBL_Y + _ACT_LBL_H + 2*_GAP_Y
#define   _VCA_OPT_W			250

/* Display Option button */
#define   _DIS_OPT_X			_CH_LBL_X
#define   _DIS_OPT_Y			_VCA_OPT_Y + 5*_GAP_Y
#define   _DIS_OPT_W			250

/* Reset button */
#define   _RESET_X			_CH_LBL_X
#define   _RESET_Y			_DIS_OPT_Y  + 5*_GAP_Y
#define   _RESET_W			250

/* Event Log label. */
#define	_LOG_X			_CAL_LBL_X//_OPT_LBL_X //_CH_CMB_X + _CH_CMB_W + 5*_GAP_X
#define	_LOG_Y			_RULE_LBL_Y//_OPT_CHK_Y + 15*_GAP_Y//_ADD_CNT_Y + 5*_GAP_Y //_CH_LBL_Y
#define	_LOG_W			_LBL_W
#define	_LOG_H			_LBL_H

/* Event Log Table. */
#define	_LOG_TBL_X		_LOG_X
#define	_LOG_TBL_Y		_LOG_Y + _LBL_H

/* Log table scroll bar. */
#define	_LOG_VSC_X		(_LOG_TBL_X + _RULE_TBL_W)
#define	_LOG_VSC_Y		_LOG_TBL_Y
#define	_LOG_TBL_H		((25 + 1) * (L_NROWS + 1) - 1)


/* time */
#define	_TIME_LBL_X		_DISPLAY_X
#define	_TIME_LBL_Y		_RESET_Y + 5*_GAP_Y
#define	_TIME_LBL_W		_LBL_W*2
#define	_TIME_LBL_H		_LBL_H

#if 0

#define	NOPTS		(5)

static gchar *strOption[NOPTS] = {
	"Object Bounding Boxes",
	"Object Ids",
	"Object Trajectories",
	"Rules",
	"Enable Calibration"
};

enum{
	DIS_BOUNDING_BOX,
	DIS_ID,
	DIS_TRAJECTORY,
	DIS_RULE,
	ENABLE_CALIBRATION
};


static NFOBJECT *g_chk[NOPTS];
#endif

static NFOBJECT *g_chk_en_cal;

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *lb_cam_ch[GUI_CHANNEL_CNT];
static NFOBJECT *spn_act;
static NFOBJECT *spn_det;
static NFOBJECT *btn_setup[GUI_CHANNEL_CNT];

static NFOBJECT *btn_cancel;
static NFOBJECT *btn_apply;
static NFOBJECT *btn_close;


static NFOBJECT *btn_add_area;
static NFOBJECT *btn_add_line;
static NFOBJECT *btn_add_counter;
static NFOBJECT *btn_del;
static NFOBJECT *btn_del_all;
static NFOBJECT *btn_mod;

static NFOBJECT *btn_cal_set;
static NFOBJECT *btn_cal_live_opt;
static NFOBJECT *btn_cal_tar_add;
static NFOBJECT *btn_cal_tar_del;
static NFOBJECT *btn_cal_tar_reset;
static NFOBJECT *btn_cal_tar_zoom;
static NFOBJECT *btn_cal_estimate;

static NFOBJECT *btn_reset;
static NFOBJECT *btn_display_option;
static NFOBJECT *btn_vca_option;

static NFOBJECT *lb_mnvwtime;

//static VCAPropData g_vpd[GUI_CHANNEL_CNT];
//static VCAPropData g_vpd_org[GUI_CHANNEL_CNT];

static gboolean g_activation[GUI_CHANNEL_CNT];
static gboolean g_activation_org[GUI_CHANNEL_CNT];

static ivca_option_t g_vod[GUI_CHANNEL_CNT];
static ivca_rule_t g_vrd[GUI_CHANNEL_CNT];

static gint g_rt;

static NFOBJECT *g_lb_vid;

static NFOBJECT *g_lb_tbl[L_NROWS][L_NCOLS];
static NFOBJECT *g_btn_up, *g_btn_dn;

static NFOBJECT *g_cmb_ch;

static NFOBJECT *g_btn_reset_va;
static NFOBJECT *g_btn_apply;
static NFOBJECT *g_btn_cancel;
static NFOBJECT *g_btn_exit;

static gchar *g_rule_font;
static gint g_rx, g_ry;		/* Ratio of normalized/display image size. */
static gint g_width, g_height;		/* Processing image size. */
static gint g_R_width, g_R_height;	/* Rounded processing image size. */

static NFOBJECT *g_lb_vid;
static char g_channel = 0;

static ivca_option_t*g_pvod, g_vod_org[GUI_CHANNEL_CNT];
static ivca_rule_t*g_pvrd, g_vrd_org[GUI_CHANNEL_CNT];

static gint g_cntr_values[GUI_CHANNEL_CNT][IVCA_MAX_CNTRS];

static gint g_idx_start;
static gint g_row_sel;

static gint g_sel_ridx;		/* Selected rule index. */
static gint g_sel_pidx;		/* Selected point index. */
static gint g_sel_ridx_p;
static gint g_sel_pidx_p;
static gint instant_erase;

static gint g_sel_x_p;
static gint g_sel_y_p;

static gint g_cur_tab;		/* Current tab. */

static ivca_option_t vod_p;
static ivca_rule_t rule_p;
static ivca_calib_t cal_p;

static volatile gint g_meta_count;
static gint g_meta_count_p;

static ivcam_obj_t*  g_meta_data;
static ivcam_obj_t*  g_meta_data_p;


/* Variables for event log. */
#define	MAX_EVENTS		256
static ivca_rule_event_t evt_buffer[MAX_EVENTS];
static volatile gint evt_head, evt_count, evt_pend;


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

static NFOBJECT *cmb_rt;
static NFOBJECT *tbl_rule;
static NFOBJECT *lb_ruletbl[_RULE_ROWS][_RULE_COLS];
static vsc_t vsc_rule;

static vsc_t vsc_log;


static gint g_beg_ruletbl;
static gint g_sel_ruletbl;
static gint g_foc_ruletbl;

static gint _cntv[IVCA_MAX_CNTRS];


extern char g_draw;

static gint g_draw_reset=0;

static ivca_calib_t g_vcd[GUI_CHANNEL_CNT];
static ivca_calib_t*g_pvcd, g_vcd_org[GUI_CHANNEL_CNT];

static NFOBJECT *cal_cam_height;
static NFOBJECT *cal_tilt_angle;
static NFOBJECT *cal_focal_length;
static NFOBJECT *cal_num_target;

typedef enum{
	RULE_SET_MODE = 0,
	CALIBRATION_SET_MODE,
	CALIBRATION_ACT_MODE
}VCA_SET_MODE;

char g_mode = RULE_SET_MODE;
static gint g_sel_cidx;			/* Selected calib target index. */
static gint g_sel_cpidx;		/* Selected calib target point index. */
static gint g_sel_cidx_p;			
static gint g_sel_cpidx_p;		

static NFOBJECT *cal_tar_height;


static NFOBJECT *cal_min_width;
static NFOBJECT *cal_min_height;

extern char g_erase;

char g_cal_live= 0;


/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static void
_init_data(void)
{
	gchar key[64];
	guint ch;

	memset(g_activation, 0, sizeof(g_activation));

	for (ch = 0; ch < GUI_CHANNEL_CNT; ch++) {
		sprintf(key, "cam.vca.cfg.R%u.act", ch);
		g_activation[ch] = nf_sysdb_get_bool(key);
	}
	
	memcpy(g_activation_org, g_activation, sizeof(g_activation));
	memset(g_vod, 0, sizeof(g_vod));
	memset(g_vrd, 0, sizeof(g_vrd));
	memset(g_vcd, 0, sizeof(g_vcd));
	DAL_get_vca_setup_data_all( g_vod, g_vrd, g_vcd, GUI_CHANNEL_CNT);
	//memcpy(&g_vogd_org, &g_vogd, sizeof(g_vogd));
	//memcpy(g_vod_org, g_vod, sizeof(g_vod));
	//memcpy(g_vrd_org, g_vrd, sizeof(g_vrd));

}

static void
_set_data(gboolean expose)
{
	nfui_combobox_set_index(spn_act , (guint)g_activation[g_channel]);
	vw_obj_endis(spn_act, TRUE, expose);
}

static void
_get_data(void)
{

	
}

static gboolean
post_nmbtn_setup_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint ch;

	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		for (ch = 0; ch < GUI_CHANNEL_CNT; ch++)
			if ( obj == btn_setup[ch] )
				break;
		if ( ch < GUI_CHANNEL_CNT ) {
			if ( vsm_get_covert_state(NULL, ch) ) {
				nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
						"Current channel is coverted.", NFTOOL_MB_OK);
				return FALSE;
			}

			//VW_VCA_Setup_Open(g_curwnd, ch, g_vod, g_vrd);
		}
	}
	return FALSE;
}

static void
vw_vca_rule_changed(void)
{
	int ch;

	printf("vw_vca_rule_changed !!!!!\n");

	for(ch=0;ch <GUI_CHANNEL_CNT;ch++){
		if(memcmp(&g_vod[ch], &g_vod_org[ch], sizeof(ivca_option_t)))
			nf_ipcam_set_va_option(ch,&g_pvod[ch],NULL);
		if(memcmp(&g_vrd[ch], &g_vrd_org[ch], sizeof(ivca_rule_t)))
			nf_ipcam_set_va_config(ch,&g_pvrd[ch],NULL);
	}

	
	printf("vw_vca_rule_changed Completed !!!!!\n");

	
}	/* vw_vca_rule_changed(... */

static gboolean
post_nmbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		if ( obj == btn_cancel ) {
			if ( vw_vca_check_prop_data_changed() ) {
				vw_vca_restore_prop_data(TRUE);
			}
		}
		else if ( obj == btn_apply ) {
			if ( vw_vca_check_prop_data_changed() ) {
				
	printf("\n");
	printf("target ntarget %d height0 %d pt %d %d %d %d ~~~~\n", g_pvcd[g_channel].ntargets, g_pvcd[g_channel].targetlist[0].height
		, g_pvcd[g_channel].targetlist[0].pt[0].x, g_pvcd[g_channel].targetlist[0].pt[0].y, g_pvcd[g_channel].targetlist[0].pt[1].x, g_pvcd[g_channel].targetlist[0].pt[1].y);
	printf("\n");
				vw_vca_save_prop_data();
				
				nftool_mbox_auto(g_curwnd, 1, "NOTICE",
						"Configuration has been saved.");

				syscam_set_changeflag(1);
				
	
			}
			
		}
		else if ( obj == btn_close ) {
			VW_VCACfg_tab_out_handler();
			SystemSetupCam_Destroy(obj);
		}
	}
	return FALSE;
}


//captiannn0
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

static void
_change_obj_focus(NFOBJECT *from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);
	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}	/* _change_obj_focus(... */

static void
_update_evtlog_table(void)
{
	gint i, j, k = g_idx_start, cnt;
	gchar strbuf[64];
	struct tm ltm;
	ivca_rule_event_t*e;

	cnt = (MAX_EVENTS + evt_head - g_idx_start) % MAX_EVENTS;

	for (i = 0; i < MIN(cnt, L_NROWS); i++)  {
		e = &evt_buffer[k];
		localtime_r((time_t *)&e->timestamp, &ltm);
		sprintf(strbuf, "%4u-%02u-%02u %02u:%02u:%02u",
				ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
				ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
		nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][0], strbuf);
		sprintf(strbuf, "%u", e->ch + 1);
		nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][1], strbuf);
		if ( e->rule_id< 0 )
			sprintf(strbuf, "N.A.");
		else
			sprintf(strbuf, "%d", e->rule_id);
		nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][2], strbuf);
		if ( e->object_id < 0 )
			sprintf(strbuf, "N.A.");
		else
			sprintf(strbuf, "%d", e->object_id);
		nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][3], strbuf);
		//captainnn
//		nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][4],
//				nf_vca_class_type_string(e->object_class));
		nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][4],
				nf_vca_event_type_string(e->type));
	//	sprintf(strbuf, "(%3d, %3d)",
	//			(e->rc.x+ e->rc.w / 2) * g_lb_vid->width / g_R_width,
	//			(e->rc.y + e->rc.h / 2) * g_lb_vid->height / g_R_height);
	//	nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][6], strbuf);
		for (j = 0; j < L_NCOLS; j++)
			nfui_signal_emit(g_lb_tbl[i][j], GDK_EXPOSE, FALSE);
		if ( ++k >= MAX_EVENTS )
			k = 0;
	}

	/* Empty remaining rows. */
	for ( ; i < L_NROWS; i++) {
		for (j = 0; j < L_NCOLS; j++) {
			nfui_nflabel_set_text((NFLABEL *)g_lb_tbl[i][j], "");
			nfui_signal_emit(g_lb_tbl[i][j], GDK_EXPOSE, FALSE);
		}
	}

	
	/* Update scroll bar */
	_update_vscrollbar(&vsc_log, evt_count, g_idx_start, TRUE);

	//vw_obj_endis(g_btn_up, evt_count - cnt > 0, TRUE);
	//vw_obj_endis(g_btn_dn, cnt > L_NROWS, TRUE);
}	/* _update_evtlog_table(... */


static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch, //384,90,_MNVD_LBL_W,_MNVD_LBL_H);
         384,// MENU_V_SUBTAB_FIXED_X + MENU_V_SUBTAB_PAGE_X + MENU_V_SUBTAB_INNER_X, 
         144,// + MENU_V_SUBTAB_FIXED_Y + MENU_V_SUBTAB_PAGE_Y + MENU_V_SUBTAB_INNER_Y, 
        _DISPLAY_W, 
        _DISPLAY_H);
    
    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static void
_change_table_focus(gint row, gboolean expose)
{
	gint i;

	if ( g_row_sel == row )
		return;

	/* old. */
	if ( g_row_sel >= 0 && g_row_sel < L_NROWS ) {
		for (i = 0; i < L_NCOLS; i++) {
			nfui_nflabel_modify_fg((NFLABEL *)g_lb_tbl[g_row_sel][i],
					COLOR_IDX(389));
			nfui_nfobject_modify_bg(g_lb_tbl[g_row_sel][i],
					NFOBJECT_STATE_NORMAL, COLOR_IDX(138));
			if ( expose )
				nfui_signal_emit(g_lb_tbl[g_row_sel][i], GDK_EXPOSE, FALSE);
		}
	}

	/* new. */
	if ( row >= 0 && row < L_NROWS ) {
		for (i = 0; i < L_NCOLS; i++) {
			nfui_nflabel_modify_fg((NFLABEL *)g_lb_tbl[row][i],
					COLOR_IDX(391));
			nfui_nfobject_modify_bg(g_lb_tbl[row][i],
					NFOBJECT_STATE_NORMAL, COLOR_IDX(390));
			if ( expose )
				nfui_signal_emit(g_lb_tbl[row][i], GDK_EXPOSE, FALSE);
		}
	}

	g_row_sel = row;
}	/* _change_table_focus(... */
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

	n = g_rt == 0 ? g_pvrd[g_channel].nzones : g_pvrd[g_channel].ncntrs;
	n2 = n - g_beg_ruletbl;
	obj = nfui_nftable_get_child((NFTABLE *)tbl_rule, 3, 0);
	if ( g_rt == 0 ) {
		nfui_nflabel_set_text(NF_LABEL(obj), "TYPE");
		z = &g_pvrd[g_channel].zonelist[g_beg_ruletbl];
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
		c = &g_pvrd[g_channel].cntrlist[g_beg_ruletbl];
		for (i = 0; i < MIN(n2, _RULE_ROWS); i++, c++) {
			sprintf(strbuf, "%d", c->id);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][0]), strbuf);
			nfui_nflabel_set_text(NF_LABEL(lb_ruletbl[i][2]), c->name);
			//captainnn
			//sprintf(strbuf, "%d", _cntv[g_beg_ruletbl + i]);
			sprintf(strbuf, "%d", g_cntr_values[g_channel][g_beg_ruletbl + i]);
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

static gint
_scroll_ruletbl(gint offset, gint n, gint sconly)
{
	gint nitem = g_rt == 0 ? g_pvrd[g_channel].nzones : g_pvrd[g_channel].ncntrs, beg = offset + n;

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
_scroll_logtbl(gint offset, gint n, gint sconly)
{
	gint nitem = evt_count, beg = offset + n;


	//printf("_scroll_logtbl offset %d n %d ", offset, beg);
	if ( beg > nitem - L_NROWS )
		beg = nitem - L_NROWS;
	if ( beg < 0 )
		beg = 0;
	n = beg - g_idx_start;

	if ( n != 0 ) {
		if ( sconly )
			_update_vscrollbar(&vsc_log, nitem, beg, TRUE);
		else {
			g_idx_start = beg;
			_update_evtlog_table();
			if ( g_row_sel != NOT_SELECTED )
				_change_table_focus(g_row_sel + 1, TRUE);
		}
	}
	return n;
}	/* _scroll_ruletbl(... */

static void
_update_cal_data()
{
	char strBuf[64];
	
	if(g_mode == CALIBRATION_SET_MODE){
		//nfui_nfbutton_set_text(btn_cal_set, "RETURN");
			
		vw_obj_endis(btn_cal_live_opt, TRUE, TRUE);
		vw_obj_endis(btn_cal_tar_add, g_pvcd[g_channel].ntargets < IVCA_MAX_CALIB_TARGETS, TRUE);
		vw_obj_endis(btn_cal_tar_del, g_pvcd[g_channel].ntargets > 0, TRUE);
		vw_obj_endis(btn_cal_tar_reset, g_pvcd[g_channel].ntargets > 0, TRUE);
		vw_obj_endis(btn_cal_tar_zoom, TRUE, TRUE);
		vw_obj_endis(btn_cal_estimate, g_pvcd[g_channel].ntargets > 2, TRUE);
		
		vw_obj_endis(btn_add_area, FALSE, TRUE);
		vw_obj_endis(btn_add_line, FALSE, TRUE);
		vw_obj_endis(btn_add_counter, FALSE, TRUE);
		vw_obj_endis(btn_del, FALSE, TRUE);
		vw_obj_endis(btn_del_all, FALSE, TRUE);
		vw_obj_endis(btn_mod, FALSE, TRUE);
		vw_obj_endis(cmb_rt, FALSE, TRUE);

		if(g_sel_cidx >=0 ){
			int height = 0;
			int idx;
			vw_obj_endis(cal_tar_height, TRUE, TRUE);
			height = g_pvcd[g_channel].targetlist[g_sel_cidx].height;
			idx = (height - IVCA_MIN_CALIB_HEIGHT)/5;
			nfui_spin_button_set_index(cal_tar_height, idx);
			nfui_signal_emit(cal_tar_height, GDK_EXPOSE,TRUE);
		}

	}
	else if(g_mode == RULE_SET_MODE){
		//nfui_nfbutton_set_text(btn_cal_set, "SETTING");

		vw_obj_endis(btn_cal_live_opt, FALSE, TRUE);
		vw_obj_endis(btn_cal_tar_add, FALSE, TRUE);
		vw_obj_endis(btn_cal_tar_del, FALSE, TRUE);
		vw_obj_endis(btn_cal_tar_reset, FALSE, TRUE);
		vw_obj_endis(btn_cal_tar_zoom, FALSE, TRUE);
		vw_obj_endis(btn_cal_estimate, FALSE, TRUE);
		
		vw_obj_endis(cal_tar_height, FALSE, FALSE);
		vw_obj_endis(cmb_rt, TRUE, TRUE);

	}

	sprintf(strBuf, "%u",g_pvcd[g_channel].ntargets);
	nfui_nflabel_set_text(cal_num_target,strBuf);
	nfui_signal_emit(cal_num_target, GDK_EXPOSE,TRUE);

	sprintf(strBuf, "%f",g_pvcd[g_channel].height);
	nfui_nflabel_set_text(cal_cam_height,strBuf);
	nfui_signal_emit(cal_cam_height, GDK_EXPOSE,TRUE);
	sprintf(strBuf, "%f",g_pvcd[g_channel].tilt);
	nfui_nflabel_set_text(cal_tilt_angle,strBuf);
	nfui_signal_emit(cal_tilt_angle, GDK_EXPOSE,TRUE);
	sprintf(strBuf, "%f",g_pvcd[g_channel].focal);
	nfui_nflabel_set_text(cal_focal_length,strBuf);
	nfui_signal_emit(cal_focal_length, GDK_EXPOSE,TRUE);
	
	if(g_pvcd[g_channel].paramvalid)
		vw_obj_endis(g_chk_en_cal, TRUE, TRUE);
	else
		vw_obj_endis(g_chk_en_cal, FALSE, TRUE);

	if(!g_pvod[g_channel].en_usecalib){
		vw_obj_endis(cal_min_width, FALSE, TRUE);
		vw_obj_endis(cal_min_height, FALSE, TRUE);
	}
	else{
		vw_obj_endis(cal_min_width, TRUE, TRUE);
		vw_obj_endis(cal_min_height, TRUE, TRUE);
	}
	
	nfui_check_button_set_active((NFCHECKBUTTON *)g_chk_en_cal, g_pvod[g_channel].en_usecalib);

	sprintf(strBuf, "%f",g_pvod[g_channel].min_width3d);
	nfui_nflabel_set_text(cal_min_width, strBuf);
	nfui_signal_emit(cal_min_width, GDK_EXPOSE, TRUE);
	sprintf(strBuf, "%f",g_pvod[g_channel].min_height3d);
	nfui_nflabel_set_text(cal_min_height, strBuf);
	nfui_signal_emit(cal_min_height, GDK_EXPOSE, TRUE);
	
}

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
		
		if(g_pvrd[g_channel].nzones || g_pvrd[g_channel].ncntrs)
			vw_obj_endis(btn_del_all, TRUE, expose);
		else
			vw_obj_endis(btn_del_all, FALSE, expose);
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
				row = MIN(g_pvrd[g_channel].nzones - g_beg_ruletbl, _RULE_ROWS) - 1;
			else
				row = MIN(g_pvrd[g_channel].ncntrs - g_beg_ruletbl, _RULE_ROWS) - 1;
			updated = TRUE;
		}
		_change_ruletbl_selection(row, expose);
		vw_obj_endis(btn_del, TRUE, expose);
		vw_obj_endis(btn_del_all, TRUE, expose);
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
	
	if ( g_rt == 0 ){
		vw_obj_endis(btn_add_line, g_pvrd[g_channel].nzones < IVCA_MAX_ZONES, expose);
		vw_obj_endis(btn_add_area, g_pvrd[g_channel].nzones < IVCA_MAX_ZONES, expose);
		vw_obj_endis(btn_add_counter, FALSE, expose);
	}
	else{
		vw_obj_endis(btn_add_line, FALSE, expose);
		vw_obj_endis(btn_add_area, FALSE, expose);
		vw_obj_endis(btn_add_counter, g_pvrd[g_channel].ncntrs < IVCA_MAX_CNTRS, expose);
	}
	
	if(g_pvrd[g_channel].nzones || g_pvrd[g_channel].ncntrs)
		vw_obj_endis(btn_del_all, TRUE, expose);
	else
		vw_obj_endis(btn_del_all, FALSE, expose);
		
}	/* _rule_update(... */

static gint
_select_rule(gint x, gint y, gboolean flag, ivca_rule_t * rl)
{
	gint n, k, select = -1, ispt = 0, nx = x * (1920*2)/_DISPLAY_W, ny = y * (1080*2)/_DISPLAY_H;
	gint tx = 8 * (1920*2)/_DISPLAY_W, ty = 8 * (1080*2)/_DISPLAY_H;
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
_select_target(gint x, gint y, gboolean flag, ivca_calib_t* cl)
{
	gint n, k, select = -1, ispt = 0, nx = x * (1920*2)/_DISPLAY_W, ny = y * (1080*2)/_DISPLAY_H;
	gint tx = 8 * (1920*2)/_DISPLAY_W, ty = 8 * (1080*2)/_DISPLAY_H;
	ivca_calib_target_t* t;

	/* First, check for already selected zone to cope with overlap &
	 * point selection. */
	if ( g_sel_cidx >= 0 ) {
		t = &cl->targetlist[g_sel_cidx];
		/* Select point. */
		for (k = 0; k < 2 && select < 0; k++) {
			if ( abs(nx - t->pt[k].x) < tx && abs(ny - t->pt[k].y) < ty ) {
				select = k;
				ispt = 1;
			}
		}
		/* Check for previous selection. */
		if ( select < 0 && vw_vca_polygon_test(nx, ny, tx, 2, t->pt) )
			select = g_sel_cidx;
	}

	/* Next, check for unselected rules. */
	for (n = 0; n < cl->ntargets && select < 0; n++) {
		t = &cl->targetlist[n];
		if ( vw_vca_polygon_test(nx, ny, tx, 2, t->pt) )
			select = n;
	}

	if ( !(flag && select == -1) ) {
		if ( ispt )
			g_sel_cpidx = select;
		else {
			if ( g_sel_cidx != select || g_sel_cpidx >= 0 ) {
				g_sel_cpidx = -1;		/* Deselect any point. */
				g_sel_cidx = select;
				// update target height
			}
		}
	}
	if(g_sel_cidx >=0 ){
		char strBuf[64];
		int height = 0;
		int idx;
		//captainnn
		height = g_pvcd[g_channel].targetlist[g_sel_cidx].height;
		idx = (height - IVCA_MIN_CALIB_HEIGHT)/5;
		printf("select target %d height %d ~~~~\n",g_sel_cidx,height);
		nfui_spin_button_set_index(cal_tar_height, idx);
		nfui_signal_emit(cal_tar_height, GDK_EXPOSE,TRUE);
	}
	g_sel_x_p = x;
	g_sel_y_p = y;
	return select;
}	/* _select_rule(... */

static gint
_move_rule(gint x, gint y, ivca_rule_t *rl)
{
	gint i, npts;
	gint dx = (x - g_sel_x_p) * (1920*2)/_DISPLAY_W, dy = (y - g_sel_y_p) * (1080*2)/_DISPLAY_H;
	gint tx = 16 * (1920*2)/_DISPLAY_W, ty = 16 * (1080*2)/_DISPLAY_H, xgap = 16 * (1920*2)/_DISPLAY_W, ygap = 16 * (1080*2)/_DISPLAY_H;
	gint w = g_lb_vid->width * (1920*2)/_DISPLAY_W, h = g_lb_vid->height * (1080*2)/_DISPLAY_H;
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
_move_target(gint x, gint y, ivca_calib_t* cl)
{
	gint i, npts;
	gint dx = (x - g_sel_x_p) * (1920*2)/_DISPLAY_W, dy = (y - g_sel_y_p) * (1080*2)/_DISPLAY_H;
	gint tx = 16 * (1920*2)/_DISPLAY_W, ty = 16 * (1080*2)/_DISPLAY_H, xgap = 16 * (1920*2)/_DISPLAY_W, ygap = 16 * (1080*2)/_DISPLAY_H;
	gint w = g_lb_vid->width * (1920*2)/_DISPLAY_W, h = g_lb_vid->height * (1080*2)/_DISPLAY_H;
	gint xmin = w, xmax = 0, ymin = h, ymax = 0;
	gint mx, my;
	ivca_point_t *pts;

	if ( g_sel_cidx < 0 || (!dx && !dy) )
		return -1;

	npts = 2;
	pts = cl->targetlist[g_sel_cidx].pt;
	

	if ( g_sel_cpidx >= 0 ) {
		xmin = xmax = pts[g_sel_cpidx].x;
		ymin = ymax = pts[g_sel_cpidx].y;
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

	if ( g_sel_cpidx >= 0 ) {
		/* Move points. */
		mx = pts[g_sel_cpidx].x + dx;
		my = pts[g_sel_cpidx].y + dy;

		/* Prohibit to move a point too near to polygon points. */
		for (i = 0; i < npts; i++)
			if ( i != g_sel_cpidx &&
					abs(mx - pts[i].x) < tx && abs(my - pts[i].y) < ty )
				break;
		if ( i < npts )
			return -1;

		pts[g_sel_cpidx].x = (short)mx;
		pts[g_sel_cpidx].y = (short)my;
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
	gint i, j, nx = x * (1920*2)/_DISPLAY_W, ny = y * (1080*2)/_DISPLAY_H, tx = 16 * (1920*2)/_DISPLAY_W, ty = 16 * (1080*2)/_DISPLAY_H;
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
	gint i, j, nx = x * (1920*2)/_DISPLAY_W, ny = y * (1080*2)/_DISPLAY_H, tx = 8 * (1920*2)/_DISPLAY_W, ty = 8 * (1080*2)/_DISPLAY_H;
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

static void
_eventlog_cb(gint ch, gint count, ivca_rule_event_t *events, gchar **snapshots)
{
	gint i;

	if ( ch != (gint)g_channel )
		return;

	for (i = 0; i < count; i++) {
		//captainnn
		/*
		if ( evt_buffer[evt_head].snap_size && evt_buffer[evt_head].snapshot )
			free(evt_buffer[evt_head].snapshot);
		if ( snapshots && events[i].snap_size ) {
			events[i].snapshot = malloc(events[i].snap_size);
			if ( events[i].snapshot )
				memcpy(events[i].snapshot, snapshots[i], events[i].snap_size);
		}
		*/
		evt_buffer[evt_head] = events[i];
		evt_head = (evt_head + 1) % MAX_EVENTS;
		evt_pend = MIN(evt_pend + 1, MAX_EVENTS - 1);
		evt_count = MIN(evt_count + 1, MAX_EVENTS - 1);
	}
}	/* eventlog_cb(... */
static void
_meta_data_cb(gint ch, gint count, ivcam_obj_t* meta)
{
	if ( ch != (gint)g_channel )
		return;
#if 0
	if ( g_meta_data ) {
		free(g_meta_data);
		g_meta_count = -1;
	}
	g_meta_data = malloc((guint)count * sizeof(ivca_meta_obj_t));
	if ( g_meta_data ) {
		memcpy(g_meta_data, meta, (guint)count * sizeof(ivca_meta_obj_t));
		g_meta_count = count;
	}
#else

	if ( g_meta_data ) {
		free(g_meta_data);
		g_meta_count = -1;
	}
	g_meta_data = malloc((guint)count * sizeof(ivcam_obj_t));
	if ( g_meta_data ) {
		memcpy(g_meta_data, meta, (guint)count * sizeof(ivcam_obj_t));
		g_meta_count = count;
	}
	
#endif
}	/* _trackinfo_cb(... */


static void
_counter_cb(gint ch, gint count, ivca_meta_cnt_t *counter)
{
	int i;
	ivca_meta_cnt_t *p;

	p = counter;
	
	if ( ch != (gint)g_channel )
		return;

	for(i=0;i<count;i++,p++){
		g_cntr_values[ch][i] = p->value;
		//printf("_counter_cb %d %d \n",i,p->value);
	}

	if( g_rt == 1 )
		_update_ruletbl(TRUE);
	
	
}	/* _counter_cb(... */


static void
_draw_mnvd_key_edit_mode_outline(gint on)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	drawable = nfui_nfobject_get_window(g_lb_vid);
	gc = nfui_nfobject_get_gc(g_lb_vid);
	gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID,
			GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
	nfui_nfobject_get_offset(g_lb_vid, &x, &y);
	gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(on ? 147 : 0));
	gdk_draw_rectangle(drawable, gc, FALSE, x - 1, y - 1,
			g_lb_vid->width + 2, g_lb_vid->height + 2);
	nfui_nfobject_gc_unref(gc);
}

static gboolean
post_lb_video_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint changed = 0, pressed_l = 0;
	gint ofs_x, ofs_y;
	static gint key_edit_mode = 0;
	
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
		
		case GDK_LEAVE_NOTIFY:
			if ( obj->kfocus != NFOBJECT_FOCUS ) {
				_draw_mnvd_key_edit_mode_outline(0);
				if ( key_edit_mode ) {
					g_sel_pidx = -1;
					key_edit_mode = 0;
				}
			}
			
		case GDK_BUTTON_PRESS:
			if ( evt->button.button != MOUSE_LEFT_BUTTON )
				break;

			pressed_l = 1;
			nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);
			
			if(g_mode == CALIBRATION_SET_MODE){
				_select_target((gint)evt->button.x - ofs_x,
						(gint)evt->button.y - ofs_y, 0, &g_pvcd[g_channel]);
			}
			else if(g_mode == RULE_SET_MODE){
				_select_rule((gint)evt->button.x - ofs_x,
						(gint)evt->button.y - ofs_y, 0, &g_pvrd[g_channel]);
			}
			break;

		case GDK_2BUTTON_PRESS:
			if(g_mode == RULE_SET_MODE){
				/* Add/Delete a point or warp. */
				if ( g_sel_ridx < 0 )
					break;

				nfui_nfobject_get_offset(obj, &ofs_x, &ofs_y);

				
					if ( g_rt == 0 ) {
						if ( evt->button.button == MOUSE_LEFT_BUTTON ) {
							if ( g_pvrd[g_channel].zonelist[g_sel_ridx].npts >= IVCA_MAX_PTSPERZONE ) {
								nftool_mbox((NFWINDOW *)g_lb_vid->parent, "CONFIRM",
										"You can not add more point.", NFTOOL_MB_OK);
								break;
							}
							if ( _add_rule_point(g_sel_ridx,
									(gint)evt->button.x - ofs_x,
									(gint)evt->button.y - ofs_y, &g_pvrd[g_channel]) >= 0 )
								break;
						}
						else {
							_del_rule_point(g_sel_ridx, (gint)evt->button.x - ofs_x,
									(gint)evt->button.y - ofs_y, &g_pvrd[g_channel]);
							break;
						}
					}
					else {
						if ( evt->button.button != MOUSE_LEFT_BUTTON )
							break;
					}

					/* Open modify popup window. */
					if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
							//&g_pvrd[g_channel], _cntv, g_rt, g_sel_ridx) )
							&g_pvrd[g_channel], g_cntr_values[g_channel], g_rt, g_sel_ridx) )
						_update_ruletbl(TRUE);
			}
			break;

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
				if(g_mode == CALIBRATION_SET_MODE){
					if ( _move_target((gint)evt->motion.x - ofs_x,
								(gint)evt->motion.y - ofs_y, &g_pvcd[g_channel]) >= 0 )
						changed = 1;
				}
				else if(g_mode== RULE_SET_MODE){
					if ( _move_rule((gint)evt->motion.x - ofs_x,
								(gint)evt->motion.y - ofs_y, &g_pvrd[g_channel]) >= 0 )
						changed = 1;
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
	
}

static gboolean
post_lb_table_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint row, cnt, n;
	
	switch ( evt->type ) {
		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
			if ( row >= MIN(evt_count, L_NROWS) )
				break;

			_change_table_focus(row, TRUE);
			//_update_snapshot(row);
			break;

		case GDK_2BUTTON_PRESS:
			break;

		case GDK_SCROLL:
			cnt = (MAX_EVENTS + evt_head - g_idx_start) % MAX_EVENTS;
			if ( evt->scroll.direction == GDK_SCROLL_UP ) {
				if ( evt_count - cnt > 0 ) {
					n = MIN(evt_count - cnt, L_NROWS / 2);
					g_idx_start = (MAX_EVENTS + g_idx_start - n) % MAX_EVENTS;
					_update_evtlog_table();
					if ( g_row_sel != NOT_SELECTED )
						_change_table_focus(g_row_sel + n, TRUE);
				}
			}
			else if ( evt->scroll.direction == GDK_SCROLL_DOWN ) {
				if ( cnt > L_NROWS ) {
					n = MIN(cnt - L_NROWS, L_NROWS / 2);
					g_idx_start = (g_idx_start + n) % MAX_EVENTS;
					_update_evtlog_table();
					if ( g_row_sel != NOT_SELECTED )
						_change_table_focus(g_row_sel - n, TRUE);
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
	
}

static gboolean
post_scbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cnt;
	
	if ( evt->type == GDK_BUTTON_PRESS &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		cnt = (MAX_EVENTS + evt_head - g_idx_start) % MAX_EVENTS;

		if ( obj == g_btn_up ) {
			if ( evt_count - cnt > 0 ) {
				g_idx_start = (MAX_EVENTS + g_idx_start - 1) % MAX_EVENTS;
				_update_evtlog_table();
				if ( g_row_sel != NOT_SELECTED )
					_change_table_focus(g_row_sel + 1, TRUE);
			}
		}
		else if ( obj == g_btn_dn ) {
			if ( cnt > L_NROWS ) {
				g_idx_start = (g_idx_start + 1) % MAX_EVENTS;
				_update_evtlog_table();
				if ( g_row_sel != NOT_SELECTED )
					_change_table_focus(g_row_sel - 1, TRUE);
			}
		}
	}
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( obj == g_btn_up ) {
			if ( evt->key.keyval == KEYPAD_DOWN ) {
				_change_obj_focus(obj, g_btn_dn);
				return TRUE;
			}
		}
		else if ( obj == g_btn_dn ) {
			if ( evt->key.keyval == KEYPAD_UP ) {
				_change_obj_focus(obj, g_btn_up);
				return TRUE;
			}
		}
	}
	return FALSE;
	
}
static gboolean
pre_win_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
#if 0
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
	#endif
}	/* pre_win_cb(... */
static gboolean 
post_cal_option_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int index;
	/*
	if (obj == cal_min_width && evt->type == NFEVENT_SPINBUTTON_CHANGED ) {
		index = nfui_spin_button_get_index(obj);
		g_pvod[g_channel].min_width3d = index;
	}
	if (obj == cal_min_height&& evt->type == NFEVENT_SPINBUTTON_CHANGED ) {
		index = nfui_spin_button_get_index(obj);
		g_pvod[g_channel].min_height3d= index;
	}
	*/
	
	if(evt->type == GDK_2BUTTON_PRESS)
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
    		gchar buf[256];

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
  	   	}

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		strTemp = VirtualKey_Open((NFWINDOW *)obj->parent, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 10, VKEY_NUMERIC);

		if(strTemp) 
		{
			if(strlen(strTemp) == 0) return FALSE;

			index = atoi(strTemp);

			if ( index >= 0 && index <= IVCA_MAX_MINH3D) {
				nfui_nflabel_set_text((NFLABEL *)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

				/* Update rule data. */
				if ( obj == cal_min_width )
					g_pvod[g_channel].min_width3d = index;
				else if ( obj == cal_min_height )
					g_pvod[g_channel].min_height3d= index;
			}
			else {
				gchar strbuf[64];

				sprintf(strbuf,
						"It should be ranged from %d to %d mm.",
						0, IVCA_MAX_MINH3D);
				nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
						strbuf, NFTOOL_MB_OK);
			}

			

			ifree(strTemp);
			strTemp = NULL;
		}

	}
}


static gboolean 
post_tar_height_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int index;
	if ( evt->type == NFEVENT_SPINBUTTON_CHANGED ) {
		if(g_sel_cidx >=0){
			index = nfui_spin_button_get_index(obj);
			g_pvcd[g_channel].targetlist[g_sel_cidx].height = IVCA_MIN_CALIB_HEIGHT + 5*index;
			printf("post_tar_height_cb %d height %d\n",g_sel_cidx,g_pvcd[g_channel].targetlist[g_sel_cidx].height);
		}
	}
}

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
			//captainnn
			/*
			for (i = 0; i < MAX_EVENTS; i++)
				if ( evt_buffer[i].snapshot )
					free(evt_buffer[i].snapshot);
					*/
			evt_head = evt_count = evt_pend = 0;
		//	if ( g_ti_data )
		//		free(g_ti_data);
		//	g_ti_data = NULL;
		//	g_ti_count = -1;
		//	if ( g_ti_data_p )
		//		free(g_ti_data_p);
		//	g_ti_data_p = NULL;

			
			if ( g_meta_data )
				free(g_meta_data);
			g_meta_data = NULL;
			g_meta_count = -1;
			if ( g_meta_data_p )
				free(g_meta_data_p);
			g_meta_data_p = NULL;

			//_stop_vca_display_mode();
			//nf_api_vca_set_fgchannel(-1);
			printf("GDK_DELETE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

			uxm_unreg_imsg_event(obj, INFY_VCA_EVENT_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_VCA_TRACKINFO_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_VCA_META_DATA_NOTIFY);
			uxm_unreg_imsg_event(obj, INFY_VCA_COUNTER_NOTIFY);
			//g_window = NULL;

			//CAPTAINNN	
			nf_meta_data_display_live_off();
			g_draw = 0;
			g_mode = 0;
	
			//gtk_main_quit();
			
			//nfui_regi_pre_event_callback((NFOBJECT *)g_curwnd, NULL);
			//nfui_nfwindow_set_returnkey_proc(g_curwnd, NULL);
			g_curwnd = NULL;

			break;

		case INFY_VCA_EVENT_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			//ch = ((ivca_rule_event_t *)pinfo->p.ptr)->ch;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
			
			if(!get_vca_enable(p[0]))
				break;
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
			
		case INFY_VCA_META_DATA_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
		//	printf("INFY_VCA_META_DATA_NOTIFY callback ch %d count %d\n ",ch,count);
			//_meta_data_cb(ch, count, (ivca_meta_obj_t *)(p + 2));
			break;

		case INFY_VCA_COUNTER_NOTIFY:
			pinfo = (NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data;
			p = pinfo->p.ptr;
			ch = p[0];
			count = p[1];
			//printf("callback INFY_VCA_COUNTER_NOTIFY ch %d count %d\n ",ch,count);
			_counter_cb(ch, count, p + 2);
			break;

		default:
			break;
	}
	return FALSE;
}

static gboolean
post_ckbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;
	gboolean value;
	

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		if(g_chk_en_cal == obj){
			value = nfui_check_button_get_active((NFCHECKBUTTON *)g_chk_en_cal);
			g_vod[g_channel].en_usecalib = value;
			if(g_vod[g_channel].en_usecalib){
				g_vod[g_channel].track_ref = IVCA_TREF_GNDP;
				vw_obj_endis(cal_min_width, TRUE, TRUE);
				vw_obj_endis(cal_min_height, TRUE, TRUE);
			}
			else{
				vw_obj_endis(cal_min_width, FALSE, TRUE);
				vw_obj_endis(cal_min_height, FALSE, TRUE);
			}
		}
		
		#if 0
		for (i = 0; i < NOPTS; i++)
			if ( g_chk[i] == obj )
				break;
		if ( i == NOPTS )
			return FALSE;

		/* Update option data. */
		value = nfui_check_button_get_active((NFCHECKBUTTON *)g_chk[i]);

		switch(i){
			case 0:	//"Display Object Bounding Boxes"
				g_vod[g_channel].sw_obj_bb = value;
				break;
			case 1:	//Display Object Ids
				g_vod[g_channel].sw_obj_id= value;
				break;
			case 2:	//Display Object Trajectories
				g_vod[g_channel].sw_obj_tr= value;
				break; 
			case 3:	//Display Rules
				g_vod[g_channel].sw_rule= value;
			case 4:	//enable calib
				g_vod[g_channel].en_usecalib = value;
				if(g_vod[g_channel].en_usecalib){
					vw_obj_endis(cal_min_width, TRUE, TRUE);
					vw_obj_endis(cal_min_height, TRUE, TRUE);
				}
				else{
					vw_obj_endis(cal_min_width, FALSE, TRUE);
					vw_obj_endis(cal_min_height, FALSE, TRUE);
				}
				break;
			default:
				break;

		}
		#endif
	}
	
	return FALSE;
}

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
	#if 0
	else if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS ) {
		if ( evt->key.keyval == KEYPAD_DOWN ) {
			_change_obj_focus(obj, lb_ruletbl[0][0]);
			return TRUE;
		}
	}
	#endif
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

	_rgb888_to_gdkcolor(g_rt == 0 ? g_pvrd[g_channel].zonelist[idx].color :
			g_pvrd[g_channel].cntrlist[idx].color, &color);

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
	if(g_mode == RULE_SET_MODE){
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
					eofs = (g_rt == 0 ? g_pvrd[g_channel].nzones : g_pvrd[g_channel].ncntrs) - g_beg_ruletbl;
					if ( row < MIN(eofs, _RULE_ROWS) )
						_draw_ruletbl_rcolor(obj, g_beg_ruletbl + row);
				}

				_change_ruletbl_focus(obj->kfocus == NFOBJECT_FOCUS ? row : -1);
				break;

			case GDK_BUTTON_PRESS:
				if ( evt->button.button == MOUSE_RIGTH_BUTTON )
					return FALSE;
				row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
				eofs = (g_rt == 0 ? g_pvrd[g_channel].nzones : g_pvrd[g_channel].ncntrs) - g_beg_ruletbl;
				if ( row < MIN(eofs, _RULE_ROWS) ) {
					g_sel_ridx = g_beg_ruletbl + row;
					g_sel_pidx = -1;
					_change_ruletbl_selection(row, TRUE);
					vw_obj_endis(btn_del, TRUE, TRUE);
					vw_obj_endis(btn_del_all, TRUE, TRUE);
					vw_obj_endis(btn_mod, TRUE, TRUE);
				}
				break;

			case GDK_2BUTTON_PRESS:
				if ( evt->button.button == MOUSE_RIGTH_BUTTON )
					return FALSE;
				row = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "rowi"));
				eofs = (g_rt == 0 ? g_pvrd[g_channel].nzones : g_pvrd[g_channel].ncntrs) - g_beg_ruletbl;
				if ( row < MIN(eofs, _RULE_ROWS) ) {
					/* The row is selected already by GDK_BUTTON_PRESS. */
					if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
							&g_pvrd[g_channel], g_cntr_values[g_channel], g_rt, g_beg_ruletbl + row) )
						_update_ruletbl(TRUE);
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
#if 0
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
#endif
			default:
				break;
		}
	}
	return FALSE;
	
}	/* post_lb_ruletbl_cb(... */

static gboolean
post_vsc_rule_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint pressed_l = 0, pressed_y = 0, pressed_i = 0, prev_sc = 0;
	gint dy, n;

	if(g_mode == RULE_SET_MODE){
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
					n = (g_rt == 0 ? g_pvrd[g_channel].nzones : g_pvrd[g_channel].ncntrs) - _RULE_ROWS;
					prev_sc = n * dy / vsc_rule.hv;
					_scroll_ruletbl(pressed_i, prev_sc, TRUE);
				}
				break;
#if 0
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
#endif
			default:
				break;
		}
	}
	return FALSE;
	
}	/* post_vsc_rule_cb(... */
				
static gboolean
post_vsc_log_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static gint pressed_l = 0, pressed_y = 0, pressed_i = 0, prev_sc = 0;
	gint dy, n;

	switch ( evt->type ) {
		case GDK_BUTTON_PRESS:
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;
			if ( obj == vsc_log.btn_up )
				_scroll_logtbl(g_idx_start, -1, FALSE);
			else if ( obj == vsc_log.btn_dn )
				_scroll_logtbl(g_idx_start, 1, FALSE);
			else if ( obj == vsc_log.lb_top )
				_scroll_logtbl(g_idx_start, -L_NROWS, FALSE);
			else if ( obj == vsc_log.lb_bot )
				_scroll_logtbl(g_idx_start, L_NROWS, FALSE);
			else if ( obj == vsc_log.lb_mid ) {
				if ( pressed_l == 0 ) {
					pressed_l = 1;
					pressed_y = (gint)evt->button.y;
					pressed_i = g_idx_start;
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
				n = evt_count - L_NROWS;
				prev_sc = n * dy / vsc_log.hv;
				_scroll_logtbl(pressed_i, prev_sc, TRUE);
			}
			break;
		default:
			break;
	}
	return FALSE;
	
}	/* post_vsc_rule_cb(... */

static gboolean
post_btn_edit_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint idx;
	
	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		if ( obj == btn_add_line) {
			idx = vw_vca_add_zone(obj->parent, &g_pvrd[g_channel],IVCA_RT_LINE);
			if ( idx >= 0 ) {
				_rule_update(idx, TRUE);
				if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
						&g_pvrd[g_channel], g_cntr_values[g_channel], g_rt, idx) )
					_update_ruletbl(TRUE);
			}
		}
		else if ( obj == btn_add_area ) {
			idx = vw_vca_add_zone(obj->parent, &g_pvrd[g_channel],IVCA_RT_AREA);
			if ( idx >= 0 ) {
				_rule_update(idx, TRUE);
				if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
						&g_pvrd[g_channel], g_cntr_values[g_channel], g_rt, idx) )
					_update_ruletbl(TRUE);
			}
		}
		else if ( obj == btn_add_counter ) {
			idx = vw_vca_add_cntr(obj->parent, &g_pvrd[g_channel]);
			if ( idx >= 0 ) {
				g_cntr_values[g_channel][idx] = 0;
				_rule_update(idx, TRUE);
				if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
						&g_pvrd[g_channel], g_cntr_values[g_channel], g_rt, idx) )
					_update_ruletbl(TRUE);
			}
		}
		else if ( obj == btn_del ) {
			if ( g_rt == 0 )	/* Zone. */
				idx = vw_vca_delete_zone(g_beg_ruletbl + g_sel_ruletbl, &g_pvrd[g_channel]);
			else				/* Counter. */
				idx = vw_vca_delete_cntr(g_beg_ruletbl + g_sel_ruletbl, &g_pvrd[g_channel]);
			g_beg_ruletbl -= g_beg_ruletbl > 0;
			_rule_update(idx, TRUE);
		}
		else if ( obj == btn_del_all ) {
			
			g_pvrd[g_channel].nzones = 0;
			memset(g_pvrd[g_channel].zonelist, 0, sizeof(g_pvrd[g_channel].zonelist));
			
			g_pvrd[g_channel].ncntrs= 0;
			memset(g_pvrd[g_channel].cntrlist, 0, sizeof(g_pvrd[g_channel].cntrlist));
				
			
			_rule_update(NOT_SELECTED, TRUE);
		}
		else if ( obj == btn_mod ) {
			idx = g_beg_ruletbl + g_sel_ruletbl;
			if ( idx < 0 || (g_rt == 0 && idx >= g_pvrd[g_channel].nzones) ||
					(g_rt == 1 && idx >= g_pvrd[g_channel].ncntrs) )
				return FALSE;

			if ( VW_VCA_Rule_Config_Open((NFWINDOW *)obj->parent,
					&g_pvrd[g_channel], g_cntr_values[g_channel], g_rt, idx) )
				_update_ruletbl(TRUE);
		}
		else if ( obj == btn_reset ) {
			nf_ipcam_set_va_reset(g_channel,NULL,NULL,NULL);
		}
		else if ( obj == btn_cal_set){
			if(g_mode == CALIBRATION_SET_MODE){
				g_draw = 1;
				g_mode = RULE_SET_MODE;
				
				_rule_update(NOT_SELECTED, TRUE);
				_update_cal_data();

				if(g_cal_live ==0)
					_start_preview(g_channel);
					
				nf_meta_data_display_live_on(g_channel,1);
			}
			else{
				g_draw = 2;
				g_mode = CALIBRATION_SET_MODE;
				if( g_pvcd[g_channel].ntargets > 0)
					g_sel_cidx = 0;
				_update_cal_data();
				//_stop_preview(g_channel);
				g_cal_live = 1;
			
			}

		}
		else if ( obj == btn_cal_tar_add){
			if(g_mode == CALIBRATION_SET_MODE){
				idx = vw_vca_add_cal_target(obj->parent, &g_pvcd[g_channel]);
				printf("vw_vca_add_cal_target idx %d \n",idx);
				g_sel_cidx = idx;
				_update_cal_data();	
			}
		}
		else if ( obj == btn_cal_estimate){
			if(g_mode == CALIBRATION_SET_MODE){
				idx = vw_vca_cal_estimate(obj->parent, &g_pvcd[g_channel]);
				
				if(idx >=0){
					printf("vw_vca_cal_estimate p_w %d p_h %d  height %f tilt %f focal %f \n",g_pvcd[g_channel].p_width,g_pvcd[g_channel].p_height,g_pvcd[g_channel].height,g_pvcd[g_channel].tilt,g_pvcd[g_channel].focal);

					_update_cal_data();
					g_pvcd[g_channel].paramvalid = TRUE;
					
				}
			}
		}
		else if ( obj == btn_cal_tar_del){
			if(g_mode == CALIBRATION_SET_MODE){
				idx = vw_vca_del_cal_target(&g_pvcd[g_channel], g_sel_cidx);
				g_sel_cidx = idx;
				_update_cal_data();
			}
		}
		else if ( obj == btn_cal_tar_reset){
			if(g_mode == CALIBRATION_SET_MODE){
				g_pvcd[g_channel].ntargets = 0;
				memset(g_pvcd[g_channel].targetlist,0,sizeof(g_pvcd[g_channel].targetlist));
	
				g_sel_cidx = -1;
				g_pvcd[g_channel].height = 0;
				g_pvcd[g_channel].tilt = 0;
				g_pvcd[g_channel].focal = 0;
				g_pvcd[g_channel].paramvalid = FALSE;
				g_pvod[g_channel].en_usecalib = 0;
				_update_cal_data();
			}
		}
		else if ( obj == btn_cal_live_opt){
			if(g_mode == CALIBRATION_SET_MODE){
				if(g_cal_live == 0){
					_start_preview(g_channel);
					g_cal_live = 1;
				}
				else{
					_stop_preview(g_channel);
					g_cal_live = 0;
				}
			}
		}
        else if ( obj == btn_cal_tar_zoom){
            if(g_mode == CALIBRATION_SET_MODE){
                _stop_preview(g_channel);
                nfui_nfobject_hide((NFOBJECT*)g_curwnd);
                g_sel_cidx = VW_VCA_Zoom_Open(obj->parent, g_channel, &g_pvcd[g_channel], g_sel_cidx);
                nfui_nfobject_show((NFOBJECT*)g_curwnd);
					g_cal_live = 1;
                		_start_preview(g_channel);
				_update_cal_data();
            }
        }
	}

	return FALSE;
}	/* post_btn_edit_cb(... */

extern char g_vca_channel;

	
static gboolean
post_cmb_channel_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint channel;
	gint i;
	gint changed= 0;
	mb_type ret;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		channel = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);
		if ( g_channel == channel )
			return FALSE;


#if defined(_HDY_0818)|| defined(_HDY_1618)
		if ( channel < NUM_HD_CH || channel >= NUM_HD_CH + 4 ) {
			nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
					"The channel cannot be selected.", NFTOOL_MB_OK);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, g_channel);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			return FALSE;
		}
#endif

		if ( vsm_get_covert_state(NULL, channel) ) {
			nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
					"Current channel is coverted.", NFTOOL_MB_OK);
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, g_channel);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			return FALSE;
		}

		// save db
		if ( vw_vca_check_prop_data_changed() )
			changed |= 1;
		if (changed ){
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\n"
					"Do you want to save?", NFTOOL_MB_OKCANCEL);

			if ( ret == NFTOOL_MB_OK ) {
				scm_put_log(CHANGE_CAM_VCA, 0, 0);
				vw_vca_save_prop_data();

				//syscam_set_changeflag(1);
			}
			else if ( ret == NFTOOL_MB_CANCEL ) {
				/* Restore original data. */
				vw_vca_restore_prop_data(FALSE);
				
			}
		}

		g_channel = channel;
		g_vca_channel = channel;

		g_draw_reset = 1;
		

		//captainnn
		//_start_vca_display_mode(channel);
		_start_preview(channel);

//		if ( debug_en && fg_en )
//			nf_api_vca_set_fgchannel((gint)channel);

		//TODO Redraw options and rules...
		//memset(&rule_p, 0, sizeof(rule_p));
		//memset(&roi_p, 0, sizeof(roi_p));
		instant_erase = 0;
//captainnn
		g_rt = 0;
		nfui_combobox_set_index(cmb_rt, (guint)g_rt);
		nfui_combobox_set_data(cmb_rt, "ZONE");		

		//nfui_check_button_set_active((NFCHECKBUTTON *)g_chk[0], g_pvod[g_channel].sw_obj_bb);
		//nfui_check_button_set_active((NFCHECKBUTTON *)g_chk[1], g_pvod[g_channel].sw_obj_id);
		//nfui_check_button_set_active((NFCHECKBUTTON *)g_chk[2], g_pvod[g_channel].sw_obj_tr);
		//nfui_check_button_set_active((NFCHECKBUTTON *)g_chk[3], g_pvod[g_channel].sw_rule);
		nfui_check_button_set_active((NFCHECKBUTTON *)g_chk_en_cal, g_pvod[g_channel].en_usecalib);
		
		g_mode = RULE_SET_MODE;
		g_draw = 1;
		_rule_update(NOT_SELECTED, TRUE);
		_update_ruletbl(FALSE);
		_update_cal_data();

		_set_data(TRUE);

		
		memset(evt_buffer, 0, sizeof(evt_buffer));
		evt_head = evt_count = evt_pend = 0;
		_update_evtlog_table();
		
		//captainnn
		//vw_vca_option_update(&g_pvod[g_channel], g_cur_tab == 0);
		//vw_vca_rule_zone_update(&g_pvrd[g_channel], -1, g_cur_tab == 1);
		//vw_vca_rule_cntr_update(&g_pvrd[g_channel],
				//g_cntr_values[g_channel], -1, g_cur_tab == 2);
				
		nf_meta_data_display_live_on(g_channel,1);

		
	}
	return FALSE;
}

static gboolean
post_cmb_act_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint act;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		g_activation[g_channel] = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);
			
	}
	return FALSE;
}
static gboolean
post_cmb_det_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
#if 0
	guint det;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		g_vpd[g_channel].detect= (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);	
	}
	return FALSE;
#endif
}

static gboolean
post_btn_option_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;
		
		if(obj == btn_display_option){
			if ( VW_VCA_Display_Option_Config_Open((NFWINDOW *)obj->parent,
						&g_pvod[g_channel]) ) {
				//TODO Send to dsp if required...
			}
		}
		if(obj == btn_vca_option){
			if ( VW_VCA_Option_Config_Open((NFWINDOW *)obj->parent,
						&g_pvod[g_channel]) ) {
				//TODO Send to dsp if required...
			}
		}
	}
	#if 0
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
	#endif
	return FALSE;
}	/* post_btn_option_cb(... */

static void
draw_str(GdkDrawable *drawable, GdkGC *gc)
{
	gint i, emask[IVCA_MAX_CALIB_TARGETS];
	static int cnt=0;
	static char p_draw =0;

	if(cnt++%100 && g_draw){
		
		if(g_draw == 2){
			vw_vca_draw_str(g_lb_vid,drawable, gc, 1, "Calibration Setting Mode");
			vw_vca_draw_str(g_lb_vid,drawable, gc, 0, "Calibration Setting Mode");
		}
		else{
			if(p_draw != g_draw)
				vw_vca_draw_str(g_lb_vid,drawable, gc, 1, "Calibration Setting Mode");
			vw_vca_draw_str(g_lb_vid,drawable, gc, 1, "Rule Setting Mode");
			vw_vca_draw_str(g_lb_vid,drawable, gc, 0, "Rule Setting Mode");
		}
		p_draw = g_draw;
	}

}	/* draw_rules(... */

static void
draw_targets(GdkDrawable *drawable, GdkGC *gc, ivca_calib_t* cl)
{
	gint i, emask[IVCA_MAX_CALIB_TARGETS];
	static int cal_mod_p=0;

	if(g_draw == 2) {
		/* Erase previous state. */
		/* Draw zones. */
		vw_vca_draw_targets(g_lb_vid,drawable, gc, 1, &cal_p, g_rx, g_ry,g_sel_cidx,g_sel_cidx_p);
		/* Draw current state. */
		/* Draw zones. */
		
		vw_vca_draw_targets(g_lb_vid,drawable, gc, 0, cl, g_rx, g_ry,g_sel_cidx,g_sel_cidx_p);
	}
	else if (cal_mod_p == 2 && !g_erase) {
		printf("cal_mod_p vw_vca_draw_targets~~~~\n\n");
		/* Erase previous state. */
		vw_vca_draw_targets(g_lb_vid,drawable, gc, 1, &cal_p, g_rx, g_ry,g_sel_cidx,g_sel_cidx_p);
	}

	memcpy(&cal_p, cl, sizeof(cal_p));
	cal_mod_p = g_draw;
}	/* draw_rules(... */


static void
draw_rules(GdkDrawable *drawable, GdkGC *gc, ivca_rule_t *rl)
{
	gint i, emask[MAX(IVCA_MAX_CNTRS, IVCA_MAX_ZONES)];
	gint ensel_z = g_rt == 0 || instant_erase;
	gint ensel_c = g_rt == 1 || instant_erase;

	if ( g_pvod[g_channel].sw_rule && g_draw == 1) {
		/* Erase previous state. */
		/* Draw counters. */
		//for (i = 0; i < IVCA_MAX_CNTRS; i++)
		//	emask[i] = (g_sel_ridx_p == i && g_sel_ridx != i) ||
		//			memcmp(&rule_p.cntrlist[i], &rl->cntrlist[i],
		//				sizeof(ivca_cntr_t));
		vw_vca_draw_cntrs(g_lb_vid, drawable, gc, ensel_c, 1, 0xffff, &rule_p,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_rule_font,
				g_cntr_values[g_channel]);
		/* Draw zones. */
		for (i = 0; i < IVCA_MAX_ZONES; i++)
			emask[i] = (g_sel_ridx_p == i && g_sel_ridx != i) ||
					(g_sel_ridx == i && (g_sel_ridx_p != i ||
						g_sel_pidx_p != g_sel_pidx)) ||
					memcmp(&rule_p.zonelist[i],
						&rl->zonelist[i], sizeof(ivca_zone_t));
		vw_vca_draw_zones(g_lb_vid, drawable, gc, ensel_z, 1, emask, &rule_p,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_sel_pidx, g_sel_pidx_p);

		/* Draw current state. */
		/* Draw counters. */
		vw_vca_draw_cntrs(g_lb_vid, drawable, gc, ensel_c, 0, NULL, rl,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_rule_font,
				g_cntr_values[g_channel]);
		/* Draw zones. */
		vw_vca_draw_zones(g_lb_vid, drawable, gc, ensel_z, 0, NULL, rl,
				g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_sel_pidx, g_sel_pidx_p);
	}
	else if ( vod_p.sw_rule && !g_erase) {
			/* Erase previous state. */
			for (i = 0; i < (gint)(sizeof(emask) / sizeof(gint)); i++)
				emask[i] = 1;
			/* Draw counters. */
			vw_vca_draw_cntrs(g_lb_vid, drawable, gc, ensel_c, 1, emask, &rule_p,
					g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_rule_font,
					g_cntr_values[g_channel]);
			/* Draw zones. */
			vw_vca_draw_zones(g_lb_vid, drawable, gc, ensel_z, 1, emask, &rule_p,
					g_rx, g_ry, g_sel_ridx, g_sel_ridx_p, g_sel_pidx, g_sel_pidx_p);
	}

	instant_erase = 0;
	g_sel_ridx_p = g_sel_ridx;
	g_sel_pidx_p = g_sel_pidx;
	memcpy(&rule_p, rl, sizeof(rule_p));
	vod_p.sw_rule=( g_pvod[g_channel].sw_rule && g_draw == 1);
}	/* draw_rules(... */
	
static void
draw_meta_data(GdkDrawable *drawable, GdkGC *gc)
{
	gint count;
	ivcam_obj_t *meta;
	static int draw_meta = 0;
	static int draw_cnt = 0;

	if(g_draw == 1){

		draw_cnt++;
		if(draw_cnt == 30){
			g_draw_reset = 1;
		}
		
		if(g_draw_reset && g_meta_count < 0){
			if ( g_meta_data_p ) {
				vw_vca_draw_meta2(g_lb_vid, drawable, gc, 1, &g_pvod[g_channel], &vod_p,
						g_meta_count_p, g_meta_data_p, 1920*2, 1080*2, g_draw);
				free(g_meta_data_p);
				g_meta_data_p = NULL;
			}
			g_draw_reset=0;
			printf("~~~~~~~~~~~~~~~~  g_draw_reset ~~~~~~~~~~~~~~~~~~\n");
		}
		
		if ( g_meta_count < 0 ){
			return;
		}

		count = g_meta_count;
		meta = g_meta_data;
		g_meta_count = -1;
		g_meta_data = NULL;

		/* Erase previous state. */
		if ( g_meta_data_p ) {
			vw_vca_draw_meta2(g_lb_vid, drawable, gc, 1, &g_pvod[g_channel], &vod_p,
					g_meta_count_p, g_meta_data_p, 1920*2, 1080*2, g_draw);
			free(g_meta_data_p);
			g_meta_data_p = NULL;
		}
		
		/* Draw current state. */
		if ( meta )
			vw_vca_draw_meta2(g_lb_vid, drawable, gc, 0, &g_pvod[g_channel], &vod_p,
					count, meta, 1920*2, 1080*2, g_draw);

		g_meta_count_p = count;
		g_meta_data_p = meta;
		vod_p.sw_obj_bb= g_pvod[g_channel].sw_obj_bb;
		vod_p.sw_obj_id= g_pvod[g_channel].sw_obj_id;
		vod_p.sw_obj_tr= g_pvod[g_channel].sw_obj_tr;

		draw_cnt = 0;
	}
	else if(!g_erase){
		
		/* Erase previous state. */
		if ( g_meta_data_p ) {
			vw_vca_draw_meta2(g_lb_vid, drawable, gc, 1, &g_pvod[g_channel], &vod_p,
					g_meta_count_p, g_meta_data_p, 1920*2, 1080*2, g_draw);
			free(g_meta_data_p);
			g_meta_data_p = NULL;
		}
		
	}

}	/* draw_trackinfo(... */

static void
update_eventlog(void)
{
	gint pend, evt_head_p, evt_count_p, cnt_p, idx_start_p;

	if ( evt_pend <= 0 )
		return;		/* There is no new event log. */

	pend = evt_pend;
	evt_pend = 0;
	evt_head_p = (MAX_EVENTS + evt_head - pend) % MAX_EVENTS;
	evt_count_p = evt_count;
	cnt_p = (MAX_EVENTS + evt_head_p - g_idx_start) % MAX_EVENTS;
	idx_start_p = g_idx_start;
	if ( cnt_p == L_NROWS ||
			(evt_count_p - pend <= L_NROWS && evt_count_p > L_NROWS) )
		/* Auto-scroll. */
		g_idx_start = (MAX_EVENTS + evt_head - L_NROWS) % MAX_EVENTS;
	if ( cnt_p + pend >= MAX_EVENTS )
		g_idx_start = (evt_head + 1) % MAX_EVENTS;

	_update_evtlog_table();
	/* Fix focus to the last. */
	if ( !(evt_count_p - pend) ||	/* The first events. */
			(g_row_sel < L_NROWS &&
			((idx_start_p + g_row_sel + 1) % MAX_EVENTS) == evt_head_p) ) {
		/* The last. */
		_change_table_focus(MIN(evt_count, L_NROWS) - 1, TRUE);
		//_update_snapshot(g_row_sel);
	}
	/* Move focus. */
	else if ( (cnt_p == L_NROWS || cnt_p + pend >= MAX_EVENTS) &&
			g_row_sel != NOT_SELECTED )
		_change_table_focus(g_row_sel - pend, TRUE);
}	/* update_eventlog(... */


static gboolean
_proc_timer_draw(void *data)
{
	GdkDrawable *drawable;
	GdkGC *gc;

	NFUTIL_THREADS_ENTER();
	if ( !g_curwnd ) {
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}
	drawable = nfui_nfobject_get_window(g_lb_vid);
	gc = gdk_gc_new(drawable);

	draw_str(drawable, gc);
		
	draw_targets(drawable, gc, &g_pvcd[g_channel]);
	
	draw_meta_data(drawable, gc);
		
	draw_rules(drawable, gc, &g_pvrd[g_channel]);
	
	g_object_unref(gc);
	
		
	NFUTIL_THREADS_LEAVE();
	return TRUE;
}

static gboolean
_proc_timer_eventlog(void *data)
{
	NFUTIL_THREADS_ENTER();
	if ( !g_curwnd ) {
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	update_eventlog();
	
	NFUTIL_THREADS_LEAVE();
	
	return TRUE;
}

static gboolean
_proc_timer_playtime(void *data)
{
	GTimeVal tv;
	gchar strbuf[64];
	struct tm ltm;

	NFUTIL_THREADS_ENTER();
	if ( !g_curwnd ) {
		NFUTIL_THREADS_LEAVE();
		return FALSE;
	}

	gettimeofday(&tv, NULL);

/*
	localtime_r((time_t *)&tv.tv_sec, &ltm);
	sprintf(strbuf, "%4u-%02u-%02u   %02u:%02u:%02u",
			ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

	nfui_nflabel_set_text(NF_LABEL(lb_mnvwtime), strbuf);
	nfui_signal_emit(lb_mnvwtime, GDK_EXPOSE, FALSE);
*/
	
	if ( tv.tv_sec )
		dtf_get_local_datetime(tv.tv_sec, strbuf);
	else
		strbuf[0] = '\0';
	nfui_nflabel_set_text(NF_LABEL(lb_mnvwtime), strbuf);
	nfui_signal_emit(lb_mnvwtime, GDK_EXPOSE, FALSE);

		

	NFUTIL_THREADS_LEAVE();
	return TRUE;
}	/* _proc_timer_playtime(... */

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

static void cb_meta_data(int ch , int cnt, ivcam_obj_t* data)
{
	_meta_data_cb(ch, cnt, (ivcam_obj_t*)data);
}

static void cb_counter(int ch , int cnt, ivca_meta_cnt_t* data)
{
	_counter_cb(ch, cnt, data);
}

void
VW_VCACfg_Prop_init_page(NFOBJECT *parent)
{
	NFOBJECT *fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj;
	gint i, btn_x, btn_y, j;
	gchar strBuf[16];
	static gchar *strTitle[VCA_NUM_COLUMNS] = {
		"CHANNEL", "ACTIVATION", "MARK VCA EVENT"
	};
	static gchar *strOffOn[] = {"OFF", "ON"};
	static guint width[VCA_NUM_COLUMNS] = {195, 200, 200};
	
	gchar *strCh[GUI_CHANNEL_CNT];
	
	gchar *strRule[2] = {"ZONE", "COUNTER"};
	guint tbl_rule_w[_RULE_COLS] = {60, 24, 186, 85, 90};
	gchar *tbl_rule_strcol[_RULE_COLS] = {"ID", "", "NAME", "TYPE", "ACTIVE"};
	NFOBJECT *tbl;
	static guint tbl_w[L_NCOLS] = {200, 30, 70, 90, 120};
	static gchar *strCol[L_NCOLS] = {
		"Time", "Ch", "Rule ID", "Object ID","Event Type"
	};
	//static guint tbl_w[L_NCOLS] = {200, 80, 80, 130, 130, 230, 130};
	//static gchar *strCol[L_NCOLS] = {
	//	"Time", "Channel", "Rule ID", "Object ID",
	//	"Object Class", "Event Type", "Location"
	//};

	
	gchar *strSpin[2] = {"ZONE", "COUNTER"};
	
	gint n_width, n_height, Rx, Ry;

	//gint chk_value[NOPTS];
	gint chk_value;
	
	static gchar *target_height[200];

	g_channel = 0;
	g_vca_channel = 0;
	
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

	/* Load VCA configuration. */
	_init_data();

	g_rt = 0;

	g_pvod = g_vod;
	g_pvrd = g_vrd;
	g_pvcd = g_vcd;

	printf("\n");
	printf("n_width %d nzones %d ncntrs %d calib on %d \n",g_pvrd[g_channel].n_width,g_pvrd[g_channel].nzones,g_pvrd[g_channel].ncntrs, g_pvod[g_channel].en_usecalib);
	printf("\n");
	
	memcpy(g_vod_org, g_vod, sizeof(g_vod_org));
	memcpy(g_vrd_org, g_vrd, sizeof(g_vrd_org));
	memcpy(g_vcd_org, g_vcd, sizeof(g_vcd_org));

	g_idx_start = 0;
	g_row_sel = NOT_SELECTED;
	g_sel_ridx = -1;
	g_sel_pidx = -1;
	g_sel_ridx_p = -1;
	g_sel_pidx_p = -1;
	g_sel_x_p = g_sel_y_p = -1;
	g_cur_tab = 0;
	g_meta_count = -1;
	g_meta_data = NULL;
	g_meta_count_p = 0;
	g_meta_data_p = NULL;
	instant_erase = 0;
	g_sel_cidx = -1;
	g_sel_cpidx = -1;
	g_sel_cidx_p = -1;
	g_sel_cpidx_p = -1;

	g_mode = RULE_SET_MODE;
	
	memset(&rule_p, 0, sizeof(rule_p));

	memset(evt_buffer, 0, sizeof(evt_buffer));
	evt_head = evt_count = evt_pend = 0;

	//captainnn
	g_width = VCA_WIDTH;
	g_height = VCA_HEIGHT;

	//nf_api_vca_get_coordinate(&n_width, &n_height);
	n_width = 1920*2;
	n_height = 1080*2;
	
	g_rx = n_width / _DISPLAY_W;
	g_ry = n_height / _DISPLAY_H;

	Rx = n_width / g_width;
	Ry = n_height / g_height;
	g_R_width = g_width * Rx;
	g_R_height = g_height * Ry;


	g_rule_font = nffont_get_pango_font(NFFONT_MEDIUM_SEMI);

	/* Contents fixed. */
	fixed = vw_fixed_create(parent, 186, 1,
			MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y,
			MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H, NULL);	
	
	uxm_reg_imsg_event(parent, INFY_VCA_EVENT_NOTIFY);
	uxm_reg_imsg_event(parent, INFY_VCA_TRACKINFO_NOTIFY);
	uxm_reg_imsg_event(parent, INFY_VCA_META_DATA_NOTIFY);
	uxm_reg_imsg_event(parent, INFY_VCA_COUNTER_NOTIFY);
	

	/* Label for video window. */
	g_lb_vid = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4,
			1, 1, 116, 115, _DISPLAY_X, _DISPLAY_Y, _DISPLAY_W, _DISPLAY_H,
			post_lb_video_cb);
	nfui_nflabel_set_drawing_outline(NF_LABEL(g_lb_vid), FALSE);
	nfui_nfobject_modify_bg(g_lb_vid, NFOBJECT_STATE_NORMAL,
			COLOR_PRG_IDX(UX_COLOR_000000));

	
	/* Mainview time & progress. */
	lb_mnvwtime = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI,
			NFALIGN_CENTER, 0, FALSE, 1, 664, 0, _TIME_LBL_X,
			_TIME_LBL_Y, _TIME_LBL_W, _TIME_LBL_H, NULL);

	/* combobox for channel selection. */	
	vw_label_create(fixed, "CHANNEL", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CH_LBL_X, _CH_LBL_Y, _CH_LBL_W, _CH_LBL_H, NULL);
	
	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		strCh[i] = malloc(64);
		if ( strCh[i] ) {
			j = sprintf(strCh[i], "CH%u - ", i + 1);
			DAL_get_camera_title(&strCh[i][j], (guint)i);
		}
	}
	g_cmb_ch = vw_combo_create(fixed, strCh, GUI_CHANNEL_CNT, (gint)g_channel,
			NULL, 1, 1, _CH_CMB_X, _CH_CMB_Y, _CH_CMB_W, _CH_CMB_H, post_cmb_channel_cb);
	for (i = 0; i < GUI_CHANNEL_CNT; i++)
		if ( strCh[i] )
			free(strCh[i]);

	/* Activation */
	vw_label_create(fixed, "ACTIVATION", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _ACT_LBL_X, _ACT_LBL_Y, _ACT_LBL_W, _ACT_LBL_H, NULL);
	
	spn_act = vw_combo_create(fixed, strOffOn, 2, (gint)g_activation[g_channel],
			NULL, 1, 1, _ACT_CMB_X, _ACT_CMB_Y, _ACT_CMB_W, _ACT_CMB_H, post_cmb_act_cb);
#if 0		
	/* Detecting mark. */
	vw_label_create(fixed, "MARK VCA EVENT", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _DET_LBL_X, _DET_LBL_Y, _DET_LBL_W, _DET_LBL_H, NULL);

	spn_det = vw_combo_create(fixed, strOffOn, 2, (gint)g_vpd[g_channel].detect,
			NULL, 1, 1, _DET_CMB_X, _DET_CMB_Y, _DET_CMB_W, _DET_CMB_H, post_cmb_det_cb);
#endif
#if 0
	/* Display Option */
	vw_label_create(fixed, "DISPLAY OPTION", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _OPT_LBL_X, _OPT_LBL_Y, _OPT_LBL_W, _OPT_LBL_H, NULL);

	chk_value[DIS_BOUNDING_BOX] = g_pvod[g_channel].sw_obj_bb;
	chk_value[DIS_ID] = g_pvod[g_channel].sw_obj_id;
	chk_value[DIS_TRAJECTORY] = g_pvod[g_channel].sw_obj_tr; 
	chk_value[DIS_RULE] = g_pvod[g_channel].sw_rule; 
	chk_value[ENABLE_CALIBRATION] = g_pvod[g_channel].en_usecalib; 

	g_chk[DIS_BOUNDING_BOX] = vw_ckbutton_lb_create(fixed, strOption[DIS_BOUNDING_BOX], chk_value[DIS_BOUNDING_BOX],
				1, _OPT_CHK_X, _OPT_CHK_Y, post_ckbtn_cb);
	
	g_chk[DIS_ID] = vw_ckbutton_lb_create(fixed, strOption[DIS_ID], chk_value[DIS_ID],
				1, _OPT_CHK_X + 300, _OPT_CHK_Y, post_ckbtn_cb);
	
	g_chk[DIS_TRAJECTORY] = vw_ckbutton_lb_create(fixed, strOption[DIS_TRAJECTORY], chk_value[DIS_TRAJECTORY],
				1, _OPT_CHK_X, _OPT_CHK_Y + 35, post_ckbtn_cb);
	
	g_chk[DIS_RULE] = vw_ckbutton_lb_create(fixed, strOption[DIS_RULE], chk_value[DIS_RULE],
				1, _OPT_CHK_X + 300, _OPT_CHK_Y + 35, post_ckbtn_cb);

#endif

	/* Calibration */
	vw_label_create(fixed, "CALIBRATION", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_LBL_X, _CAL_LBL_Y, _CAL_LBL_W, _CAL_LBL_H, NULL);
	btn_cal_set = vw_nmbutton_create(fixed, "SETTING / RETURN", 3, TRUE,
			_CAL_LBL_X + 230, _CAL_SET_Y, 250, post_btn_edit_cb);

	g_chk_en_cal = vw_ckbutton_lb_create(fixed, "Enable Calibration", g_pvod[g_channel].en_usecalib,
				1, _CAL_LBL_X, _CAL_LBL_Y + _CAL_LBL_H + 20, post_ckbtn_cb);
	if(!g_pvcd[g_channel].paramvalid)
		vw_obj_endis(g_chk_en_cal, FALSE, TRUE);

	btn_cal_live_opt = vw_nmbutton_create(fixed, "LIVE / FREZZE", 3, TRUE,
			_CAL_LBL_X + 230, _CAL_LBL_Y + _CAL_LBL_H + 10, 250, post_btn_edit_cb);
		

	vw_label_create(fixed, "MINIMUM WIDTH", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_LBL_X, _CAL_LBL_Y + _CAL_LBL_H + 30 + 30, _CAL_LBL_W + 30, _CAL_LBL_H, NULL);
	
	sprintf(strBuf, "%f",g_pvod[g_channel].min_width3d);
	cal_min_width = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf,nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nfobject_use_focus(cal_min_width, NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type((NFLABEL*)cal_min_width, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)cal_min_width, NFALIGN_CENTER, 0);
	nfui_nfobject_show(cal_min_width);
	nfui_nfobject_set_size(cal_min_width, 150, 40);
	nfui_nffixed_put((NFFIXED*)fixed, cal_min_width, _CAL_LBL_X + 230 , _CAL_LBL_Y + _CAL_LBL_H + 30 +30);
	nfui_regi_post_event_callback(cal_min_width, post_cal_option_cb);

	
	vw_label_create(fixed, "mm", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0,  _CAL_LBL_X + 230 + 150 + 10, _CAL_LBL_Y + _CAL_LBL_H + 30 +30, _CAL_LBL_W/2 ,_CAL_LBL_H, NULL);
		
	
	vw_label_create(fixed, "MINIMUM HEIGHT", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_LBL_X, _CAL_LBL_Y + _CAL_LBL_H + 30 + 30 + 45, _CAL_LBL_W + 30, _CAL_LBL_H, NULL);
		
	sprintf(strBuf, "%f",g_pvod[g_channel].min_height3d);
	cal_min_height= (NFOBJECT*)nfui_nflabel_new_text_box(strBuf,nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nfobject_use_focus(cal_min_height, NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type((NFLABEL*)cal_min_height, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align((NFLABEL*)cal_min_width, NFALIGN_CENTER, 0);
	nfui_nfobject_show(cal_min_height);
	nfui_nfobject_set_size(cal_min_height, 150, 40);
	nfui_nffixed_put((NFFIXED*)fixed, cal_min_height, _CAL_LBL_X + 230 , _CAL_LBL_Y + _CAL_LBL_H + 30 +30 + 45);
	nfui_regi_post_event_callback(cal_min_height, post_cal_option_cb);

	vw_label_create(fixed, "mm", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0,  _CAL_LBL_X + 230 + 150 + 10, _CAL_LBL_Y + _CAL_LBL_H + 30 +30 + 45, _CAL_LBL_W/2 ,_CAL_LBL_H, NULL);

	if(!g_pvod[g_channel].en_usecalib){
		vw_obj_endis(cal_min_width, FALSE, TRUE);
		vw_obj_endis(cal_min_height, FALSE, TRUE);
	}

	
	vw_label_create(fixed, "ESTIMATION RESULT", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_RES_X, _CAL_RES_Y - 40, _CAL_LBL_W ,_CAL_LBL_H, NULL);
	
	vw_label_create(fixed, "CAMERA HEIGHT", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_RES_X, _CAL_RES_Y, _CAL_LBL_W, _CAL_LBL_H, NULL);
	
	sprintf(strBuf, "%f", g_pvcd[g_channel].height);
	
	cal_cam_height = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf,nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nfobject_use_focus(cal_cam_height, NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type(cal_cam_height, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nfimage_set_font_alignment(cal_cam_height, NFALIGN_RIGHT, 20);
	nfui_nfobject_show(cal_cam_height);
	nfui_nfobject_set_size(cal_cam_height, 150, 40);
	nfui_nffixed_put((NFFIXED*)fixed, cal_cam_height, _CAL_RES_X + 230, _CAL_RES_Y);

	
	vw_label_create(fixed, "m", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0,  _CAL_RES_X + 230 + 150 + 10, _CAL_RES_Y, _CAL_LBL_W/2 ,_CAL_LBL_H, NULL);

	
	//vw_label_create(fixed, "meters", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
	//		0, 1, 120, 0, _CAL_LBL_X + _CAL_LBL_W + 150,  _CAL_LBL_Y + _CAL_LBL_H +30*3,_CAL_LBL_W, _CAL_LBL_H -10, NULL);
	
	
	vw_label_create(fixed, "TILT ANGLE", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_RES_X, _CAL_RES_Y + _CAL_LBL_H + 5, _CAL_LBL_W, _CAL_LBL_H, NULL);
	
	sprintf(strBuf, "%f",g_pvcd[g_channel].tilt);
	
	cal_tilt_angle = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf,nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nfobject_use_focus(cal_tilt_angle, NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type(cal_tilt_angle, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nfimage_set_font_alignment(cal_tilt_angle, NFALIGN_RIGHT, 20);
	nfui_nfobject_show(cal_tilt_angle);
	nfui_nfobject_set_size(cal_tilt_angle, 150, 40);
	nfui_nffixed_put((NFFIXED*)fixed, cal_tilt_angle, _CAL_RES_X + 230, _CAL_RES_Y + _CAL_LBL_H + 5);

	vw_label_create(fixed, "degrees", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0,  _CAL_RES_X + 230 + 150 + 10,  _CAL_RES_Y + _CAL_LBL_H + 5, _CAL_LBL_W/2 ,_CAL_LBL_H, NULL);
	
	vw_label_create(fixed, "FOCAL LENGTH", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_RES_X, _CAL_RES_Y  + 2*(_CAL_LBL_H + 5), _CAL_LBL_W, _CAL_LBL_H, NULL);

	sprintf(strBuf, "%f",g_pvcd[g_channel].focal);
	cal_focal_length = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf,nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nfobject_use_focus(cal_focal_length, NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type(cal_focal_length, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nfimage_set_font_alignment(cal_focal_length, NFALIGN_RIGHT, 20);
	nfui_nfobject_show(cal_focal_length);
	nfui_nfobject_set_size(cal_focal_length, 150, 40);
	nfui_nffixed_put((NFFIXED*)fixed, cal_focal_length, _CAL_RES_X + 230,  _CAL_RES_Y   + 2*(_CAL_LBL_H + 5));

	vw_label_create(fixed, "pixels", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0,  _CAL_RES_X + 230 + 150 + 10,   _CAL_RES_Y   + 2*(_CAL_LBL_H + 5), _CAL_LBL_W/2 ,_CAL_LBL_H, NULL);
	
	vw_label_create(fixed, "NUMBER OF TARGET", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_RES_X, _CAL_RES_Y   + 3*(_CAL_LBL_H + 5), _CAL_LBL_W, _CAL_LBL_H, NULL);
	
	sprintf(strBuf, "%u",g_pvcd[g_channel].ntargets);
	cal_num_target = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf,nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nfobject_use_focus(cal_num_target, NFOBJECT_FOCUS_ON);
	nfui_nflabel_set_skin_type(cal_num_target, NFTEXTBOX_TYPE_OUTPUT);
	nfui_nfimage_set_font_alignment(cal_num_target, NFALIGN_RIGHT, 20);
	nfui_nfobject_show(cal_num_target);
	nfui_nfobject_set_size(cal_num_target, 150, 40);
	nfui_nffixed_put((NFFIXED*)fixed, cal_num_target, _CAL_RES_X + 230,_CAL_RES_Y  + 3*(_CAL_LBL_H + 5));
	
	vw_label_create(fixed, "TARGET HEIGHT", NFFONT_SMALL_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _CAL_RES_X, _CAL_RES_Y  + 4*(_CAL_LBL_H + 5), _CAL_LBL_W+ 10, _CAL_LBL_H, NULL);

	for (i = 0 , j = IVCA_MIN_CALIB_HEIGHT; j < IVCA_MAX_CALIB_HEIGHT; i++, j +=5 ) {
		target_height[i] = malloc(32);
		if ( target_height[i] ) {
			sprintf(target_height[i], "%u", j);
		}
	}
		
	cal_tar_height = (NFOBJECT*)nfui_spinbutton_new((gchar**)target_height, i, 25); 
	nfui_spinbutton_set_skin_type(cal_tar_height, NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(cal_tar_height, 150, 40);
	nfui_nfobject_show(cal_tar_height);
    	nfui_regi_post_event_callback(cal_tar_height, post_tar_height_cb); 
	nfui_nffixed_put((NFFIXED*)fixed, cal_tar_height, _CAL_RES_X + 230, _CAL_RES_Y  + 4*(_CAL_LBL_H + 5));
	vw_obj_endis(cal_tar_height, FALSE, TRUE);

	
	vw_label_create(fixed, "cm", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0,  _CAL_RES_X + 230 + 150 + 10,   _CAL_RES_Y  + 4*(_CAL_LBL_H + 5), _CAL_LBL_W/2 ,_CAL_LBL_H, NULL);

	/* cal buttons */
	btn_cal_tar_add= vw_nmbutton_create(fixed, "TARGET ADD", 3, FALSE,
			_CAL_BTN_X, _CAL_BTN_Y, 170, post_btn_edit_cb);
	btn_cal_tar_del= vw_nmbutton_create(fixed, "TARGET DEL", 3, FALSE,
			_CAL_BTN_X + 175, _CAL_BTN_Y,170, post_btn_edit_cb);
	btn_cal_tar_zoom= vw_nmbutton_create(fixed, "ZOOM", 3, FALSE,
			_CAL_BTN_X + 175 + 175, _CAL_BTN_Y, 130, post_btn_edit_cb);
	btn_cal_estimate=vw_nmbutton_create(fixed, "ESTIMATION", 3, FALSE,
			_CAL_BTN_X, _CAL_BTN_Y + _CAL_LBL_H + 5, 210, post_btn_edit_cb);
	btn_cal_tar_reset= vw_nmbutton_create(fixed, "ESTIMATION RESET", 3, FALSE,
			_CAL_BTN_X + 215, _CAL_BTN_Y+ _CAL_LBL_H + 5,265, post_btn_edit_cb);
	
//	vw_label_create(fixed, "Minimum Object Width in 3D", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
//			0, 1, 120, 0, _OPT_LBL_X, _OPT_LBL_Y, _OPT_LBL_W, _OPT_LBL_H, NULL);


	/* Rule. */
	vw_label_create(fixed, "RULES", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _RULE_LBL_X, _RULE_LBL_Y, _RULE_LBL_W, _RULE_LBL_H, NULL);

	cmb_rt = vw_combo_create(fixed, strRule, 2, g_rt, NULL, 1, 1,
			_RULE_CMB_X, _RULE_CMB_Y, _RULE_CMB_W, _RULE_CMB_H, post_cmb_rt_cb);


	/* Rule table. */
	tbl_rule = vw_table_create(fixed, (NFOBJECT **)lb_ruletbl,
			_RULE_COLS, _RULE_ROWS, 1, 1, tbl_rule_w, 22, tbl_rule_strcol, 0,
			116, 115, 389, 138, _RULE_TBL_X, _RULE_TBL_Y, post_lb_ruletbl_cb);

	_create_vscrollbar(fixed, &vsc_rule, _RULE_ROWS, _RULE_VSC_X, _RULE_VSC_Y,
			_RULE_TBL_H, _SCBTN_W, _SCBTN_H, post_vsc_rule_cb);

	/* Edit buttons. */
	btn_add_line = vw_nmbutton_create(fixed, "ADD LINE", 3, TRUE,
			_ADD_LINE_X, _ADD_LINE_Y, _ADD_LINE_W, post_btn_edit_cb);
	btn_add_area = vw_nmbutton_create(fixed, "ADD AREA", 3, TRUE,
			_ADD_AREA_X, _ADD_AREA_Y, _ADD_AREA_W, post_btn_edit_cb);
	btn_add_counter = vw_nmbutton_create(fixed, "ADD COUNTER", 3, FALSE,
			_ADD_CNT_X, _ADD_CNT_Y, _ADD_CNT_W, post_btn_edit_cb);
	
	btn_del = vw_nmbutton_create(fixed, "DELETE", 3, FALSE,
			_DEL_X, _DEL_Y, _DEL_W, post_btn_edit_cb);
	btn_del_all = vw_nmbutton_create(fixed, "DELETE ALL", 3, FALSE,
			_DEL_ALL_X, _DEL_ALL_Y, _DEL_ALL_W, post_btn_edit_cb);
	btn_mod = vw_nmbutton_create(fixed, "MODIFY", 3, FALSE,
			_MOD_X, _MOD_Y, _MOD_W, post_btn_edit_cb);

	
	btn_vca_option = vw_nmbutton_create(fixed, "VCA OPTION", 3, TRUE,
			_VCA_OPT_X, _VCA_OPT_Y, _VCA_OPT_W, post_btn_option_cb);

	btn_display_option = vw_nmbutton_create(fixed, "DISPLAY OPTION", 3, TRUE,
			_DIS_OPT_X, _DIS_OPT_Y, _DIS_OPT_W, post_btn_option_cb);

	btn_reset = vw_nmbutton_create(fixed, "RESET VCA", 3, TRUE,
			_RESET_X, _RESET_Y, _RESET_W, post_btn_edit_cb);
#if 1

	/* Event log title. */
	vw_label_create(fixed, "LIVE LOG", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, _LOG_X, _LOG_Y, _LOG_W, _LOG_H, NULL);

	/* Event log table. */
	tbl = vw_table_create(fixed, (NFOBJECT **)g_lb_tbl, L_NCOLS, L_NROWS,
			1, 1, tbl_w, 25, strCol, 1, 116, 115, 389, 138,
			_LOG_TBL_X, _LOG_TBL_Y, post_lb_table_cb);

	_create_vscrollbar(fixed, &vsc_log, L_NROWS, _LOG_TBL_X + 520 - 5, _LOG_TBL_Y,
			_LOG_TBL_H, _SCBTN_W, _SCBTN_H, post_vsc_log_cb);
#endif
	//nfui_nftable_set_draw_outline((NFTABLE *)tbl, TRUE);

	/* UP/Down buttons. */
	//g_btn_up = vw_v_scbutton_create(fixed, TRUE, _LOG_TBL_X + 990, _LOG_TBL_Y, post_scbtn_cb);
	//g_btn_dn = vw_v_scbutton_create(fixed, FALSE, _LOG_TBL_X + 990, _LOG_TBL_Y + 25*L_NROWS, post_scbtn_cb);
	//g_btn_up = vw_v_scbutton_create(fixed, TRUE, _LOG_TBL_X + 520 - 5, _LOG_TBL_Y, post_scbtn_cb);
	//g_btn_dn = vw_v_scbutton_create(fixed, FALSE, _LOG_TBL_X + 520 - 5, _LOG_TBL_Y + 25*L_NROWS + 8, post_scbtn_cb);

	
	_update_ruletbl(TRUE);

	/* Timers. */
	g_timeout_add(40, _proc_timer_draw, 0);
	g_timeout_add(100, _proc_timer_eventlog, 0);
	g_timeout_add(250, _proc_timer_playtime, 0);


	/* Set initial state. */
	_rule_update(NOT_SELECTED, TRUE);
	_update_cal_data();
	
	//_start_preview(g_channel);

	//g_draw = 1;

    //jyoonl
	set_meta_data_callback(cb_meta_data, cb_counter);
	nf_meta_data_display_live_on(g_channel,1);
	
	/* Buttons. */
	btn_cancel = vw_nmbutton_create(parent, "CANCEL", 1, TRUE,
			MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y, MENU_BTN_WIDTH,
			post_nmbtn_cb);
	btn_apply = vw_nmbutton_create(parent, "APPLY", 1, TRUE,
			MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y, MENU_BTN_WIDTH,
			post_nmbtn_cb);
	btn_close = vw_nmbutton_create(parent, "CLOSE", 2, TRUE,
			MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y, MENU_BTN_WIDTH,
			post_nmbtn_cb);
	
	nfui_regi_pre_event_callback((NFOBJECT *)g_curwnd, pre_win_cb);
	nfui_regi_post_event_callback(parent, post_page_cb);

	nfui_make_key_hierarchy(g_curwnd);
	//nfui_nfwindow_set_returnkey_proc(g_curwnd, returnkey_proc);
}	/* VW_VCACfg_Prop_init_page(... */

gboolean
vw_vca_check_prop_data_changed(void)
{
	//captainnn
	//_get_data();
	int result=0;
	if(memcmp(g_activation, g_activation_org, sizeof(g_activation)))
		result += 1;
	if(memcmp(g_vod, g_vod_org, sizeof(g_vod)))
		result += 2;
	if(memcmp(g_vrd, g_vrd_org, sizeof(g_vrd)))
		result += 4;
	if(memcmp(g_vcd, g_vcd_org, sizeof(g_vcd)))
		result += 8;
	printf("vw_vca_check_prop_data_changed result %d \n",result);

	return result;
	//	memcmp(g_vpd, g_vpd_org, sizeof(g_vpd)) ||
	//		memcmp(g_vod, g_vod_org, sizeof(g_vod)) ||
	//		memcmp(g_vrd, g_vrd_org, sizeof(g_vrd)) ||
	//		memcmp(g_vcd, g_vcd_org, sizeof(g_vcd));
}	/* vw_vca_check_prop_data_changed(... */

void
vw_vca_save_prop_data(void)
{
	/*
	 * Rule data are already applied. so there is nothing to do with DAL,
	 * but prop data have to checked.
	 */
	 int i;
	GValue set_value = {0,};
	 //captainnn
	//vw_vca_rule_changed();
	
	memcpy(g_activation_org, g_activation, sizeof(g_activation));
	printf("active %d \n", g_activation[g_channel]);

	nf_sysdb_lock(NF_SYSDB_CATE_CAM);

	for(i = 0 ; i < GUI_CHANNEL_CNT ; i++){
		g_value_init(&set_value, G_TYPE_BOOLEAN);
		g_value_set_boolean(&set_value, g_activation[i]);
		if(!nf_sysdb_set_key1("cam.vca.cfg.R%u.act", i, &set_value, NULL))
		{
			g_value_unset(&set_value);
			printf("active set error !!!!!!!!!!!! %d \n", g_activation[i]);
			return 1;
		}
		g_value_unset(&set_value);
	}
	
	nf_sysdb_unlock(NF_SYSDB_CATE_CAM);



	//printf("DAL_set_vca_prop_data_all complete !!!!");s

	memcpy(g_vod_org, g_vod, sizeof(g_vod));
	memcpy(g_vrd_org, g_vrd, sizeof(g_vrd));
	memcpy(g_vcd_org, g_vcd, sizeof(g_vcd));

	scm_put_log(CHANGE_CAM_VCA, 0, 0);
	printf("DAL_set_vca_setup_data_all START !!!!");

	DAL_set_vca_setup_data_all( g_pvod, g_pvrd, g_pvcd,
				GUI_CHANNEL_CNT);	
	
	//captainnn
	DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
	DAL_notify_fire_ipcam_change(NF_IPCAM_CATE_VCA, 0x1 << g_channel);
	evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);
	printf("DAL_set_vca_setup_data_all complete !!!!");

    	vaa_reload();

	for(i=0;i< IVCA_MAX_CNTRS;i++)
		g_cntr_values[g_channel][i] = 0;

	if( g_rt == 1 )
		_update_ruletbl(TRUE);
}	/* vw_vca_save_prop_data(... */

void
vw_vca_restore_prop_data(gboolean expose)
{
	memcpy(g_activation, g_activation_org, sizeof(g_activation));

	memcpy(g_vod, g_vod_org, sizeof(g_vod));
	memcpy(g_vrd, g_vrd_org, sizeof(g_vrd));
	memcpy(g_vcd, g_vcd_org, sizeof(g_vcd));

	//DAL_set_vca_setup_data_all(&g_vogd, g_vod, g_vrd, GUI_CHANNEL_CNT);
	//captainnn
	_rule_update(NOT_SELECTED, TRUE);
	_update_ruletbl(FALSE);
	_update_cal_data();

	_set_data(TRUE);
}	/* vw_vca_restore_prop_data(... */

