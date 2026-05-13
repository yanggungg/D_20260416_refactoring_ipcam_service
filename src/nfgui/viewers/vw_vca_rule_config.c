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
2012/07/18 Jongbin Yim    Created.

................................................................................
DESCRIPTION:

................................................................................
*/

/**
 * @file  vw_vca_rule_config.c
 * @brief  This file contains VCA rule setup routines.
 */

/*******************************************************************************
 * includes                                                                    *
 ******************************************************************************/

#include "nf_afx.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "tools/ix_mem.h"
#include "tools/nf_ui_tool.h"
#include "viewers/objects/cw_slider.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/vw_colorsel.h"
#include "viewers/vw_vkeyboard.h"
#include "vw_tools.h"
#include "vw_vca_rule_config.h"
#include "nf_meta_data.h"


#include "ivca_def.h"

/*******************************************************************************
 * defines                                                                     *
 ******************************************************************************/

#define	RULE_WIN_X		(guint)((DISPLAY_ACTIVE_WIDTH - RULE_WIN_W) / 2)
#define	RULE_WIN_Y		(guint)((DISPLAY_ACTIVE_HEIGHT - RULE_WIN_H - 72) / 2)
#define	RULE_WIN_W		560
#define	RULE_WIN_H		700//580

#define	RULE_FXD_X		12
#define	RULE_FXD_Y		52
#define	RULE_FXD_W		(RULE_WIN_W - 12 * 2)
#define	RULE_FXD_H		(RULE_WIN_H - RULE_FXD_Y - 12)

#define	RULE_BTN1_X		((RULE_FXD_W - 10) / 2 - RULE_BTN_W)
#define	RULE_BTN2_X		((RULE_FXD_W + 10) / 2)
#define	RULE_BTN_Y		(RULE_FXD_H - RULE_FXD_Y)
#define	RULE_BTN_W		160

/*******************************************************************************
 * typedefs                                                                    *
 ******************************************************************************/

/*******************************************************************************
 * globals                                                                     *
 ******************************************************************************/

/*******************************************************************************
 * locals                                                                      *
 ******************************************************************************/

static NFOBJECT *main_wnd;
static NFOBJECT *btn_ok;
static NFOBJECT *btn_cancel;

static NFOBJECT *ck_active;
static NFOBJECT *lb_dcolor;
static NFOBJECT *ck_posdir;
static NFOBJECT *ck_negdir;
static NFOBJECT *ck_enter;
static NFOBJECT *ck_exit;
static NFOBJECT *ck_stop;
static NFOBJECT *ck_remove;
static NFOBJECT *ck_loiter;
static NFOBJECT *lb_stoptime;
static NFOBJECT *lb_remotime;
static NFOBJECT *lb_loitertime;
static NFOBJECT *ck_color;
static NFOBJECT *lb_ecolor;
static NFOBJECT *sld_sens;

static NFOBJECT *lb_size_min_w;
static NFOBJECT *lb_size_min_h;
static NFOBJECT *lb_size_max_w;
static NFOBJECT *lb_size_max_h;
static NFOBJECT *lb_speed_min;
static NFOBJECT *lb_speed_max;
static NFOBJECT *ck_size;
static NFOBJECT *ck_speed;

static NFOBJECT *ck_valuealert;
static NFOBJECT *ck_resetalert;
static NFOBJECT *lb_valuealert;
static NFOBJECT *lb_value;
static NFOBJECT *cmb_up;
static NFOBJECT *cmb_dn;
static NFOBJECT *btn_reset;

static gboolean retval = FALSE;

static gint g_rt;
static gint g_idx;
static ivca_rule_t g_rule;
static gint g_cntval;

/*******************************************************************************
 * function declarations                                                       *
 ******************************************************************************/

static void
_init_data(gint rt, gint idx, ivca_rule_t*rule, gint *cntval)
{
	g_rt = rt;
	g_idx = idx;
	memcpy(&g_rule, rule, sizeof(g_rule));
	g_cntval = cntval[idx];

	ck_posdir = ck_negdir = NULL;
	ck_enter = ck_exit = NULL;
	ck_stop = ck_remove = NULL;
	lb_stoptime = lb_remotime = NULL;

	ck_valuealert = ck_resetalert = NULL;
	lb_valuealert = NULL;
	lb_value = NULL;
	cmb_up = cmb_dn = NULL;
	btn_reset = NULL;
}	/* _init_data(... */

static gboolean
post_lb_name_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *strTmp;
	gint x, y;
	KEYPAD_KID kpid = -1;
	NFOBJECT *top;

	if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS )
		kpid = evt->key.keyval;

	if ( evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER ) {
		if ( kpid == KEYPAD_ENTER ) {
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);
			x += obj->width / 2 + top->x;
			y += obj->height + top->y;
		}
		else {
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);
			x += (gint)evt->button.x;
			y += (gint)evt->button.y;
		}

		strTmp = VirtualKey_Open((NFWINDOW *)obj,
				nfui_nflabel_get_text((NFLABEL *)obj),
				(guint)x, (guint)y, IVCA_MAX_NAME_LEN-1, VKEY_NORMAL);

		if ( strTmp && strlen(strTmp) > 0 ) {
			/* Update rule data. */
			if ( g_rt == 0 )
				strncpy(g_rule.zonelist[g_idx].name, strTmp,
						sizeof(g_rule.zonelist[g_idx].name));
			else
				strncpy(g_rule.cntrlist[g_idx].name, strTmp,
						sizeof(g_rule.cntrlist[g_idx].name));

			/* Update property data. */
			nfui_nflabel_set_text((NFLABEL *)obj, strTmp);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			ifree(strTmp);
		}
	}
	return FALSE;
}	/* post_lb_name_cb(... */

static gboolean
post_lb_color_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkColor color;
	gint x, y;
	guint8 *pc;
	KEYPAD_KID kpid = -1;
	NFOBJECT *top;

	if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS )
		kpid = evt->key.keyval;

	if ( evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER ) {
		if ( kpid == KEYPAD_ENTER ) {
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);
			x += obj->width / 2 + top->x;
			y += obj->height + top->y;
		}
		else {
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);
			x += (gint)evt->button.x;
			y += (gint)evt->button.y;
		}

		if ( g_rt )
			pc = g_rule.cntrlist[g_idx].color;
		else
			pc = obj == lb_dcolor ? g_rule.zonelist[g_idx].color :
					g_rule.zonelist[g_idx].ecolor;
		color.pixel = 0;
		color.red = (guint16)(pc[0] << 8);
		color.green = (guint16)(pc[1] << 8);
		color.blue = (guint16)(pc[2] << 8);
		if ( color.red == 0 && color.green == 0 && color.blue == 0 ){
			color.red = 0xFF00;
			color.green = color.blue = 0x1000;
		}

		if ( ColorSel_Open((NFWINDOW *)obj, &color, (guint)x, (guint)y) ) {
			/* Update rule data. */
			pc[0] = (guint8)(color.red >> 8);
			pc[1] = (guint8)(color.green >> 8);
			pc[2] = (guint8)(color.blue >> 8);

			/* Update property data. */
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}
	else if ( (evt->type == GDK_EXPOSE || evt->type == GDK_LEAVE_NOTIFY) &&
			!nfui_nfobject_is_disabled(obj) ) {
		NFLABEL *lb = (NFLABEL *)obj;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);

		if ( g_rt )
			pc = g_rule.cntrlist[g_idx].color;
		else
			pc = obj == lb_dcolor ? g_rule.zonelist[g_idx].color :
					g_rule.zonelist[g_idx].ecolor;
		color.pixel = 0;
		color.red = (guint16)(pc[0] << 8);
		color.green = (guint16)(pc[1] << 8);
		color.blue = (guint16)(pc[2] << 8);
		if ( color.red == 0 && color.green == 0 && color.blue == 0 ){
			color.red = 0xFF00;
			color.green = color.blue = 0x1000;
		}
		gdk_gc_set_rgb_fg_color(gc, &color);
		gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID,
				GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

		if ( lb->draw_outline && lb->object.kfocus == NFOBJECT_FOCUS )
			gdk_draw_rectangle(drawable, gc, TRUE, x + 2, y + 2,
					obj->width - 4, obj->height - 4);
		else
			gdk_draw_rectangle(drawable, gc, TRUE, x, y,
					obj->width, obj->height);
		nfui_nfobject_gc_unref(gc);
	}
	return FALSE;
}	/* post_lb_color_cb(... */

static guint
_get_et_mask(NFOBJECT *obj)
{
	if ( obj == ck_posdir )
		return IVCA_ET_DIR_POS;
	else if ( obj == ck_negdir )
		return IVCA_ET_DIR_NEG;
	else if ( obj == ck_enter )
		return IVCA_ET_ENTER;
	else if ( obj == ck_exit )
		return IVCA_ET_EXIT;
	else if ( obj == ck_stop )
		return IVCA_ET_STOPPED;
	else if ( obj == ck_remove )
		return IVCA_ET_REMOVED;
	else if ( obj == ck_loiter)
		return IVCA_ET_LOITERED;
	else if ( obj == ck_color )
		return IVCA_ET_COLOR;
	else if ( obj == ck_valuealert )
		return IVCA_ET_COUNTER;
	else if ( obj == ck_size)
		return IVCA_ET_SIZE;
	else if ( obj == ck_speed)
		return IVCA_ET_SPEED;
	return 0;
}	/* _get_et_mask(... */

static gboolean
post_ckbtn_zone_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean value;
	guint mask;

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		value = nfui_check_button_get_active(NF_CHECKBUTTON(obj));
		if ( obj == ck_active )
			g_rule.zonelist[g_idx].active = (guint8)value;
		else {
			mask = _get_et_mask(obj);

			/* Update rule data. */
			if ( value )
				g_rule.zonelist[g_idx].enabled |= mask;
			else
				g_rule.zonelist[g_idx].enabled &= ~mask;

			/* Enable/disable related controls. */
			if ( obj == ck_stop ) {
				if ( value && g_rule.zonelist[g_idx].stop_time == 0 ) {
					g_rule.zonelist[g_idx].stop_time = 5;
					nfui_nflabel_set_text((NFLABEL *)lb_stoptime, "5");
				}
				vw_obj_endis(lb_stoptime, value, TRUE);
			}
			else if ( obj == ck_remove ) {
				if ( value && g_rule.zonelist[g_idx].remove_time == 0 ) {
					g_rule.zonelist[g_idx].remove_time = 5;
					nfui_nflabel_set_text((NFLABEL *)lb_remotime, "5");
				}
				vw_obj_endis(lb_remotime, value, TRUE);
			}
			else if ( obj == ck_loiter ) {
				if ( value && g_rule.zonelist[g_idx].loiter_time== 0 ) {
					g_rule.zonelist[g_idx].loiter_time = 5;
					nfui_nflabel_set_text((NFLABEL *)lb_loitertime, "5");
				}
				vw_obj_endis(lb_loitertime, value, TRUE);
			}
			else if ( obj == ck_color ) {
				vw_obj_endis(lb_ecolor, value, TRUE);
				vw_obj_endis(sld_sens, value, TRUE);
			}
			else if ( obj == ck_size ) {
				if ( value && g_rule.zonelist[g_idx].size_min[0] == 0 ) {
					g_rule.zonelist[g_idx].size_min[0] = 0;
					nfui_nflabel_set_text((NFLABEL *)lb_size_min_w, "0");
				}
				if ( value && g_rule.zonelist[g_idx].size_min[1] == 0 ) {
					g_rule.zonelist[g_idx].size_min[1] = 0;
					nfui_nflabel_set_text((NFLABEL *)lb_size_min_h, "0");
				}
				if ( value && g_rule.zonelist[g_idx].size_max[0] == 0 ) {
					g_rule.zonelist[g_idx].size_max[0] = 0;
					nfui_nflabel_set_text((NFLABEL *)lb_size_max_w, "0");
				}
				if ( value && g_rule.zonelist[g_idx].size_max[1] == 0 ) {
					g_rule.zonelist[g_idx].size_max[1] = 0;
					nfui_nflabel_set_text((NFLABEL *)lb_size_max_h, "0");
				}
				vw_obj_endis(lb_size_min_w, value, TRUE);
				vw_obj_endis(lb_size_min_h, value, TRUE);
				vw_obj_endis(lb_size_max_w, value, TRUE);
				vw_obj_endis(lb_size_max_h, value, TRUE);
			}
			else if ( obj == ck_speed ) {
				if ( value && g_rule.zonelist[g_idx].speed_min == 0 ) {
					g_rule.zonelist[g_idx].size_max[1] = 0;
					nfui_nflabel_set_text((NFLABEL *)lb_speed_min, "0");
				}
				if ( value && g_rule.zonelist[g_idx].speed_max== 0 ) {
					g_rule.zonelist[g_idx].size_max[1] = 0;
					nfui_nflabel_set_text((NFLABEL *)lb_speed_max, "0");
				}
				vw_obj_endis(lb_speed_min, value, TRUE);
				vw_obj_endis(lb_speed_max, value, TRUE);
			}
		}
	}
	return FALSE;
}	/* post_ckbtn_zone_cb(... */

static gboolean
post_sld_sens_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch ( evt->type ) {
		case GDK_BUTTON_PRESS:
			break;

		case GDK_LEAVE_NOTIFY:
		case GDK_BUTTON_RELEASE:
			/* Update rule data. */
			g_rule.zonelist[g_idx].ecolor_sens =
					(guint8)cw_slider_get_value((CWSLIDER *)sld_sens);
			break;

		case NFEVENT_CWSLIDER_CHANGED_RELEASE:
			break;

		default:
			break;
	}
	return FALSE;
}	/* post_sld_sens_cb(... */

static gboolean
post_ckbtn_cntr_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gboolean value;

	if ( evt->type == NFEVENT_CHECKBUTTON_CHANGED ) {
		value = nfui_check_button_get_active(NF_CHECKBUTTON(obj));
		if ( obj == ck_active )
			g_rule.cntrlist[g_idx].active = (guint8)value;
		else if ( obj == ck_resetalert )
			g_rule.cntrlist[g_idx].resetalert = (guint8)value;
		else if ( obj == ck_valuealert ) {
			/* Update rule data. */
			if ( value )
				g_rule.cntrlist[g_idx].enabled |= IVCA_ET_COUNTER;
			else
				g_rule.cntrlist[g_idx].enabled &= ~IVCA_ET_COUNTER;

			/* Enable/disable related controls. */
			vw_obj_endis(lb_valuealert, value, TRUE);
			vw_obj_endis(ck_resetalert, value, TRUE);
		}
	}
	return FALSE;
}	/* post_ckbtn_cntr_cb(... */

static gboolean
post_lb_num_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gchar *strTmp, strbuf[16];
	gint x, y, val;
	KEYPAD_KID kpid = -1;
	NFOBJECT *top;
	gint max_ch=0;

	if ( evt->type == NFEVENT_KEYPAD_PRESS ||
			evt->type == NFEVENT_REMOCON_PRESS )
		kpid = evt->key.keyval;

	if ( evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER ) {
		if ( kpid == KEYPAD_ENTER ) {
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);
			x += obj->width / 2 + top->x;
			y += obj->height + top->y;
		}
		else {
			if ( evt->button.button == MOUSE_RIGTH_BUTTON )
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);
			x += (gint)evt->button.x;
			y += (gint)evt->button.y;
		}

		max_ch = g_rt ? 5 : 3;
		if(obj == lb_size_min_w || obj == lb_size_min_h ||obj == lb_size_max_w ||obj == lb_size_max_h )
			max_ch = 5;
		else if(obj == lb_speed_min|| obj == lb_speed_max)
			max_ch = 3;
		
		strTmp = VirtualKey_Open((NFWINDOW *)obj->parent,
				nfui_nflabel_get_text((NFLABEL *)obj), (guint)x, (guint)y,
				max_ch, VKEY_NUMERIC);
		if ( !strTmp )
			return FALSE;

		val = atoi(strTmp);
		sprintf(strbuf, "%d", val);
		if ( obj == lb_valuealert ) {
			nfui_nflabel_set_text((NFLABEL *)obj, strbuf);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

			/* Update rule data. */
			g_rule.cntrlist[g_idx].evalue = val;
		}
		else {
			if(obj == lb_size_min_w || obj == lb_size_min_h ||obj == lb_size_max_w ||obj == lb_size_max_h ){
				if ( val >= 0 && val <= IVCA_WIDTH3D_MAX ) {
					nfui_nflabel_set_text((NFLABEL *)obj, strbuf);
					nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

					/* Update rule data. */
					if ( obj == lb_size_min_w )
						g_rule.zonelist[g_idx].size_min[0]= (guint16)val;
					else if ( obj == lb_size_min_h )
						g_rule.zonelist[g_idx].size_min[1]= (guint16)val;
					else if ( obj == lb_size_max_w)
						g_rule.zonelist[g_idx].size_max[0]= (guint16)val;
					else if ( obj == lb_size_max_h)
						g_rule.zonelist[g_idx].size_max[1]= (guint16)val;
				}
				else {
					gchar strbuf[64];

					sprintf(strbuf,
							"Size should be ranged from %d to %d meters.",
							0, IVCA_WIDTH3D_MAX);
					nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
							strbuf, NFTOOL_MB_OK);
				}


			}
			else if(obj == lb_speed_min|| obj == lb_speed_max){
				if ( val >= 0 && val <= IVCA_SPEED3D_MAX ) {
					nfui_nflabel_set_text((NFLABEL *)obj, strbuf);
					nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

					/* Update rule data. */
					if ( obj == lb_speed_min )
						g_rule.zonelist[g_idx].speed_min= (guint16)val;
					else if ( obj == lb_speed_max )
						g_rule.zonelist[g_idx].speed_max = (guint16)val;
				}
				else {
					gchar strbuf[64];

					sprintf(strbuf,
							"Speed should be ranged from %d to %d km/h.",
							0, IVCA_SPEED3D_MAX);
					nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
							strbuf, NFTOOL_MB_OK);
				}
			}
			else{
				if ( val >= IVCA_ETIMEOUT_MIN && val <= IVCA_ETIMEOUT_MAX ) {
					nfui_nflabel_set_text((NFLABEL *)obj, strbuf);
					nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

					/* Update rule data. */
					if ( obj == lb_stoptime )
						g_rule.zonelist[g_idx].stop_time = (guint16)val;
					else if ( obj == lb_remotime )
						g_rule.zonelist[g_idx].remove_time = (guint16)val;
					else if ( obj == lb_loitertime)
						g_rule.zonelist[g_idx].loiter_time= (guint16)val;
				}
				else {
					gchar strbuf[64];

					sprintf(strbuf,
							"Timeout should be ranged from %d to %d seconds.",
							IVCA_ETIMEOUT_MIN, IVCA_ETIMEOUT_MAX);
					nftool_mbox((NFWINDOW *)obj->parent, "ERROR",
							strbuf, NFTOOL_MB_OK);
				}
			}
		}
		ifree(strTmp);
	}
	return FALSE;
}	/* post_lb_num_cb(... */

static gboolean
post_cmb_source_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index, zid;

	if ( evt->type == NFEVENT_COMBOBOX_CHANGED ) {
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		zid = index < g_rule.nzones ? g_rule.zonelist[index].id : -1;
		if ( obj == cmb_up )
			g_rule.cntrlist[g_idx].zid_up = (short)zid;
		else
			g_rule.cntrlist[g_idx].zid_dn = (short)zid;
	}
	return FALSE;
}	/* post_cmb_source_cb(... */

static gboolean
post_btn_reset_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		g_cntval = 0;
		nfui_nflabel_set_text((NFLABEL *)lb_value, "0");
		nfui_signal_emit(lb_value, GDK_EXPOSE, FALSE);
	}
	return FALSE;
}	/* post_btn_reset_cb(... */

static gboolean
post_mainwin_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_DELETE )
		gtk_main_quit();
	return FALSE;
}	/* post_mainwin_cb(... */

static gboolean
post_btn_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == GDK_BUTTON_RELEASE ) {
		if ( evt->button.button == MOUSE_RIGTH_BUTTON )
			return FALSE;

		retval = obj == btn_ok;
		nfui_nfobject_destroy(main_wnd);
	}
	return FALSE;
}	/* post_btn_cb(... */

static void
_init_zone(NFOBJECT *fixed)
{
	ivca_zone_t*z = &g_rule.zonelist[g_idx];
	gboolean isl = z->type == IVCA_RT_LINE;
	gint en = (gint)z->enabled;
	gchar sbuf[16];

	/* Name. */
	vw_label_create(fixed, "Name:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 10, 80, 30, NULL);
	vw_label_create(fixed, z->name, NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			1, 1, 129, 128, 10 + 20 + 80, 10, 300, 30, post_lb_name_cb);

	/* Display color. */
	vw_label_create(fixed, "Display Color:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT,
			0, 0, 1, 292, 1, 10, 50, 150, 30, NULL);
	lb_dcolor = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			1, 1, 292, 1, 10 + 150, 50, 30, 30, post_lb_color_cb);

	/* Type. */
	vw_label_create(fixed, isl ? "Type: Line" : "Type: Area",
			NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 250, 50, 150, 30, NULL);

	/* Active. */
	ck_active = vw_ckbutton_lb_create(fixed, "Active", z->active,
			292, 10, 90, post_ckbtn_zone_cb);

	/* Events. */
	vw_label_create(fixed, "Events:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 130, 100, 30, NULL);

	if ( isl ) {
		ck_posdir = vw_ckbutton_lb_create(fixed, "Forward Direction",
				en & IVCA_ET_DIR_POS, 292, 20, 170, post_ckbtn_zone_cb);
		ck_negdir = vw_ckbutton_lb_create(fixed, "Reverse Direction",
				en & IVCA_ET_DIR_NEG, 292, 20, 210, post_ckbtn_zone_cb);
	}
	else {
		ck_enter = vw_ckbutton_lb_create(fixed, "Enter",
				en & IVCA_ET_ENTER, 292, 20, 170, post_ckbtn_zone_cb);
		ck_exit = vw_ckbutton_lb_create(fixed, "Exit",
				en & IVCA_ET_EXIT, 292, 20, 210, post_ckbtn_zone_cb);
		ck_stop = vw_ckbutton_lb_create(fixed, "Stopped",
				en & IVCA_ET_STOPPED, 292, 20, 250, post_ckbtn_zone_cb);
		ck_remove = vw_ckbutton_lb_create(fixed, "Removed",
				en & IVCA_ET_REMOVED, 292, 20, 290, post_ckbtn_zone_cb);
		ck_loiter = vw_ckbutton_lb_create(fixed, "Loitering",
				en & IVCA_ET_LOITERED, 292, 20, 330, post_ckbtn_zone_cb);

		sprintf(sbuf, "%d", z->stop_time);
		lb_stoptime = vw_label_create(fixed, z->stop_time ? sbuf : "",
				NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4, 1, 1, 129, 128,
				20 + 140, 250, 80, 30, post_lb_num_cb);
		sprintf(sbuf, "%d", z->remove_time);
		lb_remotime = vw_label_create(fixed, z->remove_time ? sbuf : "",
				NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4, 1, 1, 129, 128,
				20 + 140, 290, 80, 30, post_lb_num_cb);
		sprintf(sbuf, "%d", z->loiter_time);
		lb_loitertime= vw_label_create(fixed, z->loiter_time? sbuf : "",
				NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4, 1, 1, 129, 128,
				20 + 140, 330, 80, 30, post_lb_num_cb);
		
		vw_label_create(fixed, "seconds", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
				0, 1, 292, 1, 20 + 140 + 80 + 10, 250, 100, 30, NULL);
		vw_label_create(fixed, "seconds", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
				0, 1, 292, 1, 20 + 140 + 80 + 10, 290, 100, 30, NULL);
		vw_label_create(fixed, "seconds", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
				0, 1, 292, 1, 20 + 140 + 80 + 10, 330, 100, 30, NULL);

		vw_obj_endis(lb_stoptime, !isl && (en & IVCA_ET_STOPPED), TRUE);
		vw_obj_endis(lb_remotime, !isl && (en & IVCA_ET_REMOVED), TRUE);
		vw_obj_endis(lb_loitertime, !isl && (en & IVCA_ET_LOITERED), TRUE);
	}
//captainnn
#if 1
	/* Filters. */
	vw_label_create(fixed, "Filters:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 370, 100, 30, NULL);
	ck_color = vw_ckbutton_lb_create(fixed, "Color", en & IVCA_ET_COLOR, 292, 20, 410,
			post_ckbtn_zone_cb);
	lb_ecolor = vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			1, 1, 292, 1, 20 + 100, 410, 30, 30, post_lb_color_cb);

	vw_label_create(fixed, "Percentage", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 175, 410, 110, 30, NULL);
	sld_sens = (NFOBJECT *)cw_slider_new(0, 220, 40);
	printf("ecolor_sens %d \n",z->ecolor_sens);
	nfui_nfobject_modify_bg(sld_sens, NFOBJECT_STATE_NORMAL, COLOR_IDX(1));
	cw_slider_set_range((CWSLIDER *)sld_sens, 0, 10, 11);
	cw_slider_set_value((CWSLIDER *)sld_sens, z->ecolor_sens);
	nfui_nfobject_show(sld_sens);
	nfui_regi_post_event_callback(sld_sens, post_sld_sens_cb);
	nfui_nffixed_put((NFFIXED *)fixed, sld_sens, 175 + 120, 410 - 5);

	vw_obj_endis(lb_ecolor, en & IVCA_ET_COLOR, TRUE);
	vw_obj_endis(sld_sens, en & IVCA_ET_COLOR, TRUE);

	ck_size = vw_ckbutton_lb_create(fixed, "Size (3D)", en & IVCA_ET_SIZE, 292, 20, 450,
			post_ckbtn_zone_cb);
	
	vw_label_create(fixed, "width", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150, 450, 80, 30, NULL);
	sprintf(sbuf, "%d", z->size_min[0]);
	lb_size_min_w = vw_label_create(fixed, en & IVCA_ET_SIZE ? sbuf : "",
			NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 4, 1, 1, 129, 128,
			20 + 150 + 80, 450, 80, 30, post_lb_num_cb);
	
	vw_label_create(fixed, " ~ ", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150 + 80 + 80, 450, 30, 30, NULL);
	sprintf(sbuf, "%d", z->size_max[0]);
	lb_size_max_w = vw_label_create(fixed, en & IVCA_ET_SIZE? sbuf : "",
			NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 4, 1, 1, 129, 128,
			20 + 150 + 80 + 80 + 30, 450, 80, 30, post_lb_num_cb);
	vw_label_create(fixed, "meters", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150 + 80 + 80 + 30 + 80 + 10, 450, 70, 30, NULL);
	
	vw_label_create(fixed, "height", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150, 490, 80, 30, NULL);
	sprintf(sbuf, "%d", z->size_min[1]);
	lb_size_min_h = vw_label_create(fixed, en & IVCA_ET_SIZE ? sbuf : "",
			NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 4, 1, 1, 129, 128,
			20 + 150 + 80, 490, 80, 30, post_lb_num_cb);
	vw_label_create(fixed, " ~ ", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150 + 80 + 80, 490, 30, 30, NULL);
	sprintf(sbuf, "%d", z->size_max[1]);
	lb_size_max_h = vw_label_create(fixed, en & IVCA_ET_SIZE ? sbuf : "",
			NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 4, 1, 1, 129, 128,
			20 + 150 + 80 + 80 + 30, 490, 80, 30, post_lb_num_cb);
	vw_label_create(fixed, "meters", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150 + 80 + 80 + 30 + 80 + 10, 490, 70, 30, NULL);

	ck_speed = vw_ckbutton_lb_create(fixed, "Speed (3D)", en & IVCA_ET_SPEED, 292, 20, 530,
			post_ckbtn_zone_cb);
	
	sprintf(sbuf, "%d", z->speed_min);
	lb_speed_min = vw_label_create(fixed, en & IVCA_ET_SPEED ? sbuf : "",
			NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 4, 1, 1, 129, 128,
			20 + 150, 530, 80, 30, post_lb_num_cb);
	vw_label_create(fixed, " ~ ", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150 + 80, 530, 30, 30, NULL);
	sprintf(sbuf, "%d", z->speed_max);
	lb_speed_max = vw_label_create(fixed, en & IVCA_ET_SPEED ? sbuf : "",
			NFFONT_MEDIUM_SEMI, NFALIGN_CENTER, 4, 1, 1, 129, 128,
			20 + 150 + 80 + 30, 530, 80, 30, post_lb_num_cb);
	vw_label_create(fixed, "km/h", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 20 + 150 + 80 + 80 + 30 + 10, 530, 80, 30, NULL);

	vw_obj_endis(lb_size_min_w, (en & IVCA_ET_SIZE), TRUE);
	vw_obj_endis(lb_size_max_w, (en & IVCA_ET_SIZE), TRUE);
	vw_obj_endis(lb_size_min_h, (en & IVCA_ET_SIZE), TRUE);
	vw_obj_endis(lb_size_max_h, (en & IVCA_ET_SIZE), TRUE);
	vw_obj_endis(lb_speed_min, (en & IVCA_ET_SPEED), TRUE);
	vw_obj_endis(lb_speed_max, (en & IVCA_ET_SPEED), TRUE);

	
#endif
}	/* _init_zone(... */

static void
_init_cntr_combo(void)
{
	ivca_zone_t *z;
	NFOBJECT *item;
	guint i, j, fill;
	GdkPixbuf *psrc;
	GdkPixbuf *pbuf[NFOBJECT_STATE_COUNT];

	/* Signal to generate submenu. */
	nfui_signal_emit(NF_COMBOBOX(cmb_up)->button, GDK_EXPOSE, FALSE);
	nfui_signal_emit(NF_COMBOBOX(cmb_dn)->button, GDK_EXPOSE, FALSE);

	psrc = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 30, 30);
	for (i = 0, z = g_rule.zonelist; i < (guint)g_rule.nzones + 1; i++, z++) {
		fill = i == g_rule.nzones ? 0 : ((guint)(z->color[0] << 24) |
				(z->color[1] << 16) | (z->color[2] << 8) | 0xFF);
		gdk_pixbuf_fill(psrc, fill);
		for (j = 0; j < NFOBJECT_STATE_COUNT; j++) {
			pbuf[j] = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 40, 40);
			gdk_pixbuf_fill(pbuf[j], 0);
			gdk_pixbuf_composite(psrc, pbuf[j], 5, 5, 30, 30,
					0, 0, 1, 1, GDK_INTERP_BILINEAR, 255);
		}
		item = g_slist_nth_data(NF_COMBOBOX(cmb_up)->item, i);
		nfui_nfbutton_set_icon_image((NFBUTTON *)item, pbuf);
		item = g_slist_nth_data(NF_COMBOBOX(cmb_dn)->item, i);
		nfui_nfbutton_set_icon_image((NFBUTTON *)item, pbuf);
	}
	g_object_unref(psrc);
}	/* _init_cntr_combo(... */

static void
_init_cntr(NFOBJECT *fixed)
{
	ivca_cntr_t *c = &g_rule.cntrlist[g_idx];
	ivca_zone_t *z;
	gint en = (gint)c->enabled;
	gchar sbuf[64];
	guint i, u, d;

	/* Name. */
	vw_label_create(fixed, "Name: ", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 10, 80, 30, NULL);
	vw_label_create(fixed, c->name, NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			1, 1, 129, 128, 10 + 20 + 80, 10, 300, 30, post_lb_name_cb);

	/* Display color. */
	vw_label_create(fixed, "Display Color:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT,
			0, 0, 1, 292, 1, 10, 50, 150, 30, post_lb_name_cb);
	vw_label_create(fixed, "", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			1, 1, 292, 1, 10 + 150, 50, 30, 30, post_lb_color_cb);

	/* Active. */
	ck_active = vw_ckbutton_lb_create(fixed, "Active", c->active,
			292, 10, 90, post_ckbtn_cntr_cb);
//captainnn
#if 1
	/* Events. */
	vw_label_create(fixed, "Events:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0,
			0, 1, 292, 1, 10, 130, 100, 30, NULL);

	ck_valuealert = vw_ckbutton_lb_create(fixed, "Value Alert",
			en & IVCA_ET_COUNTER, 292, 20, 170, post_ckbtn_cntr_cb);

	sprintf(sbuf, "%d", c->evalue);
	lb_valuealert = vw_label_create(fixed, sbuf, NFFONT_MEDIUM_SEMI,
			NFALIGN_LEFT, 4, 1, 1, 129, 128, 210, 170, 100, 30, post_lb_num_cb);

	ck_resetalert = vw_ckbutton_lb_create(fixed, "Reset After Alert",
			c->resetalert, 292, 20, 210, post_ckbtn_cntr_cb);
#endif
	/* Count sources. */
	vw_label_create(fixed, "Count Sources:", NFFONT_MEDIUM_SEMI,
			NFALIGN_LEFT, 0, 0, 1, 292, 1, 10, 250, 200, 30, NULL);
	vw_label_create(fixed, "Up:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0, 0, 1,
			292, 1, 20, 290, 80, 30, NULL);
	vw_label_create(fixed, "Down:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 0, 0, 1,
			292, 1, 20, 340, 80, 30, NULL);

	cmb_up = vw_combo_create(fixed, NULL, 0, 0, NULL, 1, 1,
			20 + 80, 290 - 5, 320, 40, post_cmb_source_cb);
	cmb_dn = vw_combo_create(fixed, NULL, 0, 0, NULL, 1, 1,
			20 + 80, 340 - 5, 320, 40, post_cmb_source_cb);

	u = d = g_rule.nzones;
	for (i = 0, z = g_rule.zonelist; i < g_rule.nzones; i++, z++) {
		sprintf(sbuf, "Zone[%d] %s", z->id + 1, z->name);
		nfui_combobox_append_data(NF_COMBOBOX(cmb_up), sbuf);
		nfui_combobox_append_data(NF_COMBOBOX(cmb_dn), sbuf);
		if ( c->zid_up == z->id )
			u = i;
		if ( c->zid_dn == z->id )
			d = i;
	}
	nfui_combobox_append_data(NF_COMBOBOX(cmb_up), "Disabled");
	nfui_combobox_append_data(NF_COMBOBOX(cmb_dn), "Disabled");
	nfui_combobox_set_index(NF_COMBOBOX(cmb_up), u);
	nfui_combobox_set_index(NF_COMBOBOX(cmb_dn), d);

	_init_cntr_combo();
//captainnn
#if 1
	/* Value. */
	vw_label_create(fixed, "Value:", NFFONT_MEDIUM_SEMI, NFALIGN_LEFT, 4,
			0, 1, 292, 1, 10, 390, 80, 30, NULL);
	sprintf(sbuf, "%d", g_cntval);
	lb_value = vw_label_create(fixed, sbuf, NFFONT_MEDIUM_SEMI,
			NFALIGN_LEFT, 0, 0, 1, 292, 1, 20 + 80, 390, 100, 30, NULL);
	//btn_reset = vw_nmbutton_create(fixed, "RESET", 3, TRUE, 220, 390 - 5,
	//		RULE_BTN_W, post_btn_reset_cb);

	vw_obj_endis(ck_resetalert, en & IVCA_ET_COUNTER, TRUE);
	vw_obj_endis(lb_valuealert, en & IVCA_ET_COUNTER, TRUE);
#endif
}	/* _init_cntr(... */

gboolean
VW_VCA_Rule_Config_Open(NFWINDOW *parent, ivca_rule_t*rule,
		gint *cntval, gint rt, gint idx)
{
	NFOBJECT *main_fixed, *fixed;
	gint i;

	main_wnd = (NFOBJECT *)nftool_create_popup_window(parent,
			RULE_WIN_X, RULE_WIN_Y, RULE_WIN_W, RULE_WIN_H, "RULE", FALSE);
	nfui_regi_post_event_callback(main_wnd, post_mainwin_cb);

	main_fixed = ((NFWINDOW *)main_wnd)->child;

	fixed = vw_fixed_create(main_fixed, -1, 1,
			RULE_FXD_X, RULE_FXD_Y, RULE_FXD_W, RULE_FXD_H, NULL);

	nfui_nfobject_show(main_fixed);
	nfui_nfobject_show(main_wnd);

	_init_data(rt, idx, rule, cntval);

	if ( rt == 0 )
		_init_zone(fixed);
	else
		_init_cntr(fixed);

	/* OK, CANCEL buttons. */
	btn_ok = vw_nmbutton_create(fixed, "OK", 1, TRUE,
			RULE_BTN1_X, RULE_BTN_Y, RULE_BTN_W, post_btn_cb);
	btn_cancel = vw_nmbutton_create(fixed, "CANCEL", 1, TRUE,
			RULE_BTN2_X, RULE_BTN_Y, RULE_BTN_W, post_btn_cb);

	nfui_make_key_hierarchy((NFWINDOW *)main_wnd);
	nfui_set_key_focus(btn_ok, TRUE);

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	if ( retval == TRUE ) {
		if ( g_rt == 0 )
			memcpy(&rule->zonelist[idx], &g_rule.zonelist[idx],
					sizeof(ivca_zone_t));
		else {
			memcpy(&rule->cntrlist[idx], &g_rule.cntrlist[idx],
					sizeof(ivca_cntr_t));
			cntval[idx] = g_cntval;
		}
	}
	return retval;
}	/* VW_VCA_Rule_Config_Open(... */

