
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nftab.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"

#include "vw_alarm_out_evt.h"
#include "vw_alarm_out_setup.h"
#include "vw_alarm_out_sched.h"

#include "vw_evt_act_main.h"

#define ALARM_OUT_TAB_W					(300)
#define ALARM_OUT_TAB_H					(40)

static NFWINDOW *g_curwnd = 0;


static gboolean pre_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
{
		GdkGC *gc;
		GdkDrawable *drawable;
		guint x, y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);
			nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_FIXED_BG, x, y, -1, -1, NFALIGN_LEFT, 0);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(10));
			gdk_draw_rectangle(drawable, gc, TRUE, x, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(11));
			gdk_draw_rectangle(drawable, gc, TRUE, x+1, y, 1, MENU_V_SUBTAB_FIXED_H-60);

			nfui_nfobject_gc_unref(gc);
}
		break;

		case GDK_DELETE:
			g_curwnd = 0;
		break;

		default :
			break;
	}

	return FALSE;
}

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;
	mb_type ret;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

		switch(cur_page) {
			case 0:
				if(check_alarm_out_data_changed()) {
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
					if(ret == NFTOOL_MB_OK) save_alarm_out_data();
					else 					restore_alarm_out_data();

					if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
				}
				break;
			case 1:
				if(check_nvr_alarm_out_data_changed()) {
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
					if(ret == NFTOOL_MB_OK) save_nvr_alarm_out_data();
					else 					restore_nvr_alarm_out_data();

					if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
				}
				break;
			case 2:
				if(check_alarm_sched_data_changed())  {
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
					if(ret == NFTOOL_MB_OK) save_alarm_sched_data();
					else 					restroe_alarm_sched_data();

					if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
				}
				break;
			case 3:
				if(check_nvr_alarm_sched_data_changed())  {
					ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
					if(ret == NFTOOL_MB_OK) save_nvr_alarm_sched_data();
					else 					restroe_nvr_alarm_sched_data();

					if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
				}
				break;

			default:
				break;
		}
	}

	return FALSE;
}

static gboolean pre_subtab_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) {
		GdkGC *gc;
		GdkDrawable *drawable;
		guint x, y;

		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_get_offset(obj, &x, &y);
		nfutil_draw_image(drawable, gc, MK_IMG_MENU_V_SUBTAB_PAGE_BG, x, y, -1, -1, NFALIGN_LEFT, 0);
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

void VW_Init_AlarmOutEvt_Page(NFOBJECT *parent)
{
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_300), 
				(MKB_IMG_SUBTAB_DIR_H_S_300)
	};

	NFOBJECT *nftab;
	NFOBJECT *tab_page[4];

	const gchar *strTabTitle[] = {"CAMERA", "NVR", "CAMERA ON/OFF SCHEDULE", "NVR ON/OFF SCHEDULE"};
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	guint btn_x, btn_y;
	guint i;

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(4, (gchar**)strImage_h, ALARM_OUT_TAB_W, ALARM_OUT_TAB_H, NFTAB_DIR_H, strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

	for(i=0; i<4; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

	VW_Init_AlarmOut_SetupPage(tab_page[0]);
	VW_Nvr_Init_AlarmOut_SetupPage(tab_page[1]);
	VW_Init_AlarmOut_SchedPage(tab_page[2]);
	VW_Nvr_Init_AlarmOut_SchedPage(tab_page[3]);

	nfui_nfobject_show(tab_page[0]);
}

gboolean VW_AlarmOut_tab_out_handler()
{
	mb_type ret;
	gchar page_mask = 0;

	if(check_alarm_out_data_changed())		page_mask |= (1 << 0);
	if(check_nvr_alarm_out_data_changed())	page_mask |= (1 << 1);
	if(check_alarm_sched_data_changed())	page_mask |= (1 << 2);
	if(check_nvr_alarm_sched_data_changed())	page_mask |= (1 << 3);

	if(page_mask != 0) {
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
		if(ret == NFTOOL_MB_OK) {
			if(page_mask & (1 << 0))	save_alarm_out_data();
			if(page_mask & (1 << 1))	save_nvr_alarm_out_data();
			if(page_mask & (1 << 2))	save_alarm_sched_data();
			if(page_mask & (1 << 3))	save_nvr_alarm_sched_data();
		}else if(ret == NFTOOL_MB_CANCEL) {
			if(page_mask & (1 << 0))	restore_alarm_out_data();
			if(page_mask & (1 << 1))	restore_nvr_alarm_out_data();
			if(page_mask & (1 << 2))	restroe_alarm_sched_data();
			if(page_mask & (1 << 3))	restroe_nvr_alarm_sched_data();
		}

		if(ret == NFTOOL_MB_OK) {
			event_act_data_changed(TRUE);
			return TRUE;
		}
	}

	return FALSE;
}

