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
2012/03/19 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_sys_camera_vca_schd.c
 * @brief
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nflabel.h"
#include "objects/nfobject.h"
#include "objects/nfspinbutton.h"
#include "objects/nftable.h"
#include "objects/nftile.h"
#include "support/event_loop.h"
#include "support/nf_ui_color.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "tools/nf_ui_tool.h"
#include "scm.h"
#include "vsm.h"
#include "vw_record_sched_control.h"
#include "vw_sys_camera_main.h"
#include "vw_sys_camera_vca.h"
#include "vw_sys_camera_vca_schd.h"
#include "vw_tools.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

/* Day label: XXX TODO */
#define	VCA_DAY_LABEL_X			(4)
#define	VCA_DAY_LABEL_Y			(0)
#define	VCA_DAY_LABEL_W			(200)
#define	VCA_DAY_LABEL_H			(40)

/* Time label. */
#define	VCA_TIME_LABEL_W		(48)
#define	VCA_TIME_LABEL_H		(39)
#define	VCA_TIME_LABEL_X		(VCA_DAY_LABEL_X + VCA_CH_LABEL_W + 2)
#define	VCA_TIME_LABEL_Y		(VCA_DAY_LABEL_Y + VCA_DAY_LABEL_H + 27)

/* Channel table. */
#define	VCA_CH_LABEL_W			(292)
#define	VCA_CH_TABLE_CELL_H		(39)
#define	VCA_CH_TABLE_X			(VCA_DAY_LABEL_X)
#define	VCA_CH_TABLE_Y			(VCA_TIME_LABEL_Y + VCA_TIME_LABEL_H + 2)

/* Tile. */
#define	VCA_TILE_X				(VCA_DAY_LABEL_X + VCA_CH_LABEL_W + 2 - 1)
#define	VCA_TILE_Y				(VCA_CH_TABLE_Y - 1)
#define	VCA_TILE_W				((VCA_TIME_LABEL_W + 2) * 24)
#define	VCA_TILE_H				((VCA_CH_TABLE_CELL_H + 2) * GUI_CHANNEL_CNT)

/* Copy button. */
#define	VCA_COPY_BTN_X			(MENU_V_SUBTAB_INNER_W - 226 - 7)
#define	VCA_COPY_BTN_Y			(MENU_V_SUBTAB_INNER_H - 40 - 4)

/* Legend. */
#define VCA_LEGEND_LABEL_W		(80)
#define	VCA_LEGEND_LABEL_H		(26)
#define	VCA_LEGEND_BOX_W		(22)
#define	VCA_LEGEND_BOX_H		(22)
#define	VCA_LEGEND_ON_LABEL_X	(MENU_V_SUBTAB_INNER_W - VCA_LEGEND_LABEL_W - 7)
#define	VCA_LEGEND_ON_LABEL_Y	(VCA_DAY_LABEL_Y + VCA_DAY_LABEL_H)
#define	VCA_LEGEND_ON_BOX_X		(VCA_LEGEND_ON_LABEL_X - 6 - VCA_LEGEND_BOX_W)
#define	VCA_LEGEND_ON_BOX_Y		(VCA_LEGEND_ON_LABEL_Y)
#define	VCA_LEGEND_OFF_LABEL_X	(VCA_LEGEND_ON_BOX_X - VCA_LEGEND_LABEL_W)
#define	VCA_LEGEND_OFF_LABEL_Y	(VCA_LEGEND_ON_LABEL_Y)
#define	VCA_LEGEND_OFF_BOX_X	(VCA_LEGEND_OFF_LABEL_X - 6 - VCA_LEGEND_BOX_W)
#define	VCA_LEGEND_OFF_BOX_Y	(VCA_LEGEND_ON_LABEL_Y)

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

static NFOBJECT *g_tile;
static NFOBJECT *btn_cancel;
static NFOBJECT *btn_apply;
static NFOBJECT *btn_close;


static NFOBJECT *g_cmb_day;

static VCASchedData g_vsd[GUI_CHANNEL_CNT];
static VCASchedData g_vsd_org[GUI_CHANNEL_CNT];

static int g_day=0;

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static void
_init_data(void)
{
	DAL_get_vca_schd_data_all(g_vsd, GUI_CHANNEL_CNT);
	memcpy(g_vsd_org, g_vsd, sizeof(g_vsd));
}

static void
_set_data(gboolean expose)
{
	guint i, j;
	guint color;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		for (j = 0; j < 24; j++) {
			switch ( g_vsd[i].sched[j+(g_day*24)] ) {
				case '1':
					color = SELECT_STATE_COLOR_2;
					break;
				case '0':
				default:
					color = SELECT_STATE_COLOR_1;
					break;
			}
			if ( expose )
				nfui_tile_draw_color((NFTILE *)g_tile, color, i, j, i, j);
			else
				nfui_tile_no_draw_color((NFTILE *)g_tile, color, i, j, i, j);
		}
	}
}

static gboolean
post_tile_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFEVENT_TILE_INIT:

			printf("data %s \n",g_vsd[0].sched);
			printf("data %s \n",g_vsd[1].sched);
			printf("data %s \n",g_vsd[2].sched);
			printf("data %s \n",g_vsd[3].sched);
			printf("data %s \n",g_vsd[4].sched);
			printf("data %s \n",g_vsd[5].sched);
			printf("data %s \n",g_vsd[6].sched);
			printf("data %s \n",g_vsd[7].sched);
			_set_data(TRUE);
			break;

		case NFEVENT_TILE_START_SELECT:
			NF_TILE(obj)->select_n = SELECT_STATE_COLOR_1;
			break;

		case NFEVENT_TILE_MOVE_SELECT:
			break;

		case NFEVENT_TILE_END_SELECT:
			{
				guint s_row = 0, s_col = 0;
				guint e_row = 0, e_col = 0;
				guint i, j;
				guint sched, color;
				gint x, y;

				gdk_display_get_pointer(gdk_display_get_default(),
						NULL, &x, &y, NULL);
				x = MIN(x, 1920 - 220);
				y = MIN(y, 1080 - 150);

				sched = VW_RecSched_Control_Page(g_curwnd, (guint)x, (guint)y);

				nfui_tile_get_selectArea(NF_TILE(obj),
						&s_row, &s_col, &e_row, &e_col);

				for(i = s_row; i <= e_row; i++) {
					for(j = s_col; j <= e_col; j++) {
						if ( sched == 2 ) {
							switch ( g_vsd[i].sched[j+(g_day*24)] ) {
								case '1':
									color = SELECT_STATE_COLOR_2;
									break;
								case '0':
								default:
									color = SELECT_STATE_COLOR_1;
									break;
							}
						}
						else {
							g_vsd[i].sched[j+(g_day*24)] = (gchar)('0' + sched);
							color = SELECT_STATE_COLOR_1 + sched;
						}
						nfui_tile_draw_color(NF_TILE(g_tile),
								color, i, j, i, j);
					}
				}
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean
post_nmbtn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE &&
			evt->button.button == MOUSE_LEFT_BUTTON ) {
		if ( obj == btn_cancel ) {
			if ( vw_vca_check_schd_data_changed() ) {
				vw_vca_restore_schd_data(TRUE);
			}
		}
		else if ( obj == btn_apply ) {
			if ( vw_vca_check_schd_data_changed() ) {
				vw_vca_save_schd_data();

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

static gboolean
post_page_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint x, y;

	if ( evt->type == GDK_EXPOSE ) {
		/* Draw legend marks. */
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(779));	// OFF
		gdk_draw_rectangle(drawable, gc, TRUE,
				x + VCA_LEGEND_OFF_BOX_X, y + VCA_LEGEND_OFF_BOX_Y,
				VCA_LEGEND_BOX_W, VCA_LEGEND_BOX_H);
		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(780));	// ON
		gdk_draw_rectangle(drawable, gc, TRUE,
				x + VCA_LEGEND_ON_BOX_X, y + VCA_LEGEND_ON_BOX_Y,
				VCA_LEGEND_BOX_W, VCA_LEGEND_BOX_H);

		nfui_nfobject_gc_unref(gc);
	}
	else if ( evt->type == GDK_DELETE ) {
		g_curwnd = NULL;
	}
	return FALSE;
}

static gboolean
post_cmb_day_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint day;
	gint i;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		g_day = (guint)nfui_combobox_get_cur_index((NFCOMBOBOX *)obj);	
		_set_data(TRUE);
	}
	return FALSE;
}

/**
 * @brief  Initializes VCA schedule page.
 */
void
VW_VCACfg_Schd_init_page(NFOBJECT *parent)
{
	NFOBJECT *fixed;
	NFOBJECT *ntb;
	NFOBJECT *obj;
	nffont_type font;
	gint i;
	gchar strBuf[32];
	static guint table_width[] = {VCA_CH_LABEL_W};
	static gchar *legend[] = {"OFF", "ON"};
	static gint legend_x[] = {VCA_LEGEND_OFF_LABEL_X, VCA_LEGEND_ON_LABEL_X};
	const gchar *icon[] = {IMG_CAMERA_OUTPUT_ICON_01, 
						IMG_CAMERA_OUTPUT_ICON_02,
						IMG_CAMERA_OUTPUT_ICON_03,
						IMG_CAMERA_OUTPUT_ICON_04,
						IMG_CAMERA_OUTPUT_ICON_05,
						IMG_CAMERA_OUTPUT_ICON_06,
						IMG_CAMERA_OUTPUT_ICON_07,
						IMG_CAMERA_OUTPUT_ICON_08,
						IMG_CAMERA_OUTPUT_ICON_09,
						IMG_CAMERA_OUTPUT_ICON_10,
						IMG_CAMERA_OUTPUT_ICON_11,
						IMG_CAMERA_OUTPUT_ICON_12,
						IMG_CAMERA_OUTPUT_ICON_13,
						IMG_CAMERA_OUTPUT_ICON_14,
						IMG_CAMERA_OUTPUT_ICON_15,
						IMG_CAMERA_OUTPUT_ICON_16,
						IMG_CAMERA_OUTPUT_ICON_17,
						IMG_CAMERA_OUTPUT_ICON_18,
						IMG_CAMERA_OUTPUT_ICON_19,
						IMG_CAMERA_OUTPUT_ICON_20,												
						IMG_CAMERA_OUTPUT_ICON_21,
						IMG_CAMERA_OUTPUT_ICON_22,
						IMG_CAMERA_OUTPUT_ICON_23,
						IMG_CAMERA_OUTPUT_ICON_24,
						IMG_CAMERA_OUTPUT_ICON_25,
						IMG_CAMERA_OUTPUT_ICON_26,
						IMG_CAMERA_OUTPUT_ICON_27,
						IMG_CAMERA_OUTPUT_ICON_28,
						IMG_CAMERA_OUTPUT_ICON_29,
						IMG_CAMERA_OUTPUT_ICON_30,
						IMG_CAMERA_OUTPUT_ICON_31,
						IMG_CAMERA_OUTPUT_ICON_32 };
	GdkColor tile_color[4] = {
			UX_COLOR(779), UX_COLOR(780), UX_COLOR(781), UX_COLOR(779)
	};
	
	gchar *days[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(parent);

	g_day = 0;

	/* Load VCA schedule. */
	_init_data();

	/* Contents fixed. */
	fixed = vw_fixed_create(parent, 186, 1,
			MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y,
			MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H, NULL);

	
	/* combobox for day selection. */	
	vw_label_create(fixed, "DAY", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 120, 0, VCA_DAY_LABEL_X, VCA_DAY_LABEL_Y, VCA_DAY_LABEL_W, VCA_DAY_LABEL_H, NULL);
	
	g_cmb_day = vw_combo_create(fixed, days, 7, g_day,
			NULL, 1, 1, VCA_DAY_LABEL_X + VCA_DAY_LABEL_W, VCA_DAY_LABEL_Y, VCA_DAY_LABEL_W, VCA_DAY_LABEL_H, post_cmb_day_cb);
	

	/* Legend. */
	font = nftool_cur_language_is_japanese() ?
			NFFONT_MINI_SEMI_5 : NFFONT_SMALL_SEMI;
	for (i = 0; i < 2; i++)
		vw_label_create(fixed, legend[i], font, NFALIGN_LEFT, 0, 0, 1, 1, 0,
				legend_x[i], VCA_LEGEND_OFF_LABEL_Y,
				VCA_LEGEND_LABEL_W, VCA_LEGEND_LABEL_H, NULL);

	/* Time label. */
	for (i = 0; i < 24; i++) {
		sprintf(strBuf, "%02u", i);
		vw_label_create(fixed, strBuf, NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 0,
				0, 1, 116, 115, VCA_TIME_LABEL_X + i * (2 + VCA_TIME_LABEL_W),
				VCA_TIME_LABEL_Y, VCA_TIME_LABEL_W, VCA_TIME_LABEL_H, NULL);
	}

	/* Channel table. */
	ntb = (NFOBJECT *)nfui_nftable_new(1, GUI_CHANNEL_CNT,
			0, 2, table_width, VCA_CH_TABLE_CELL_H);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED *)fixed, ntb, VCA_CH_TABLE_X, VCA_CH_TABLE_Y);

	/* Channel label. */
	for (i = 0; i < GUI_CHANNEL_CNT; i++) {
		DAL_get_camera_title(strBuf, (guint)i);
		obj = (NFOBJECT *)nfui_nfimglabel_new(
				nfui_get_image_from_file(icon[i], NULL), strBuf);
		nfui_nfimglabel_set_align((NFIMGLABEL *)obj, NFALIGN_LEFT);
		nfui_nfimglabel_set_pango_font((NFIMGLABEL *)obj,
				nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE *)ntb, obj, 0, (guint)i);

#if defined(_HDY_0818)|| defined(_HDY_1618)
		if ( i < NUM_HD_CH || i >= NUM_HD_CH + 4 )
			nfui_nfobject_disable(obj);
#endif
	}

	/* Tile. */
	g_tile = nfui_tile_new(GUI_CHANNEL_CNT, 24);
	nfui_tile_set_color((NFTILE *)g_tile, NFTILE_STATE_SELECT, tile_color); 
	nfui_nfobject_set_size(g_tile, VCA_TILE_W, VCA_TILE_H);
	nfui_nfobject_show(g_tile);
	nfui_regi_post_event_callback(g_tile, post_tile_cb);
	nfui_nffixed_put((NFFIXED *)fixed, g_tile, VCA_TILE_X, VCA_TILE_Y);

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

	nfui_regi_post_event_callback(fixed, post_page_cb);
}	/* VW_VCACfg_Sched_init_page(... */

gboolean
vw_vca_check_schd_data_changed(void)
{
	return memcmp(&g_vsd, &g_vsd_org, sizeof(g_vsd)) != 0;
}	/* vw_vca_check_schd_data_changed(... */

void
vw_vca_save_schd_data(void)
{
	DAL_set_vca_schd_data_all(g_vsd, GUI_CHANNEL_CNT);
	memcpy(g_vsd_org, g_vsd, sizeof(g_vsd));

	//captainnn
	//DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
	//DAL_notify_fire_ipcam_change(NF_IPCAM_CATE_VCA, 0xffff);
	//evt_send_to_local(INFY_STREAM_DATA_RELOAD, 0, 0, 0);

	scm_put_log(CHANGE_CAM_VCA, 0, 0);
}	/* vw_vca_save_ched_data(... */

void
vw_vca_restore_schd_data(gboolean expose)
{
	memcpy(g_vsd, g_vsd_org, sizeof(g_vsd));
	_set_data(expose);
}	/* vw_vca_restore_schd_data(... */

