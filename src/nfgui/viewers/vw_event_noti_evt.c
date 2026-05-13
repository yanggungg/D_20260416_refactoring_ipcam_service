
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

#include "vw_menu.h"
#include "vw_event_noti_evt.h"
#include "vw_event_buzzer_noti.h"
#include "vw_event_disp_noti.h"
#include "vw_event_email_noti.h"
#include "vw_event_mobilepush_noti.h"

#include "vw_evt_act_main.h"


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


static void _out_handler_cam_main(gint page)
{
	mb_type ret;

    if (page == mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_BUZZER])
    {
		if (check_evt_noti_buzzer_changed()) {
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK) save_evt_noti_buzzer_data();
			else 					restore_evt_noti_buzzer_data();

			if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
		}
    }
    else if (page == mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_DISP])
    {
		if(check_evt_noti_disp_changed())  {
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK) save_evt_noti_disp_data();
			else 					restore_evt_noti_disp_data();

			if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
		}
    }
    else if (page == mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MAIL])
    {
		if(check_evt_noti_email_changed())  {
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK) save_evt_noti_email_data();
			else 					restore_evt_noti_email_data();

			if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
		}
    }   
    else if (page == mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_FTP])
    {
		if(check_evt_noti_ftp_changed())  {
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK) save_evt_noti_ftp_data();
			else 					restore_evt_noti_ftp_data();

			if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
		}
    }
    else if (page == mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_SMS])
    {
		if(check_evt_noti_sms_changed())  {
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK) save_evt_noti_sms_data();
			else 					restore_evt_noti_sms_data();

			if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
		}
    }
	
    else if (page == mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MOBILEPUSH])
    {
		if(check_evt_noti_mobilepush_changed())  {
			ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
			if(ret == NFTOOL_MB_OK) save_evt_noti_mobilepush_data();
			else 					restore_evt_noti_mobilepush_data();

			if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
		}
    }
}


static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;

	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

		_out_handler_cam_main(cur_page);
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

void VW_Init_EvtNotiEvt_Page(NFOBJECT *parent)
{
	const gchar *strImage_h[2] =  {
				(MKB_IMG_SUBTAB_DIR_H_N_250), 
				(MKB_IMG_SUBTAB_DIR_H_S_250)
	};

	NFOBJECT *nftab;
	NFOBJECT *tab_page[MAX_SUB_CNT];

	static const gchar *strTabTitle[MAX_TAB_CNT];
	const guint colidx[3] = {COLOR_IDX(189), COLOR_IDX(188), COLOR_IDX(188)};

	guint btn_x, btn_y;
	guint i;
    gint pos;

	vw_menu_get_str_sys_menu_event_noti(strTabTitle);

	g_curwnd = nfui_nfobject_get_top(parent);

	nfui_nfobject_set_size(parent, (guint)MENU_V_SUBTAB_FIXED_W, (guint)MENU_V_SUBTAB_FIXED_H);
	nfui_nffixed_put((NFFIXED*)(parent->parent), parent, MENU_V_SUBTAB_FIXED_X, MENU_V_SUBTAB_FIXED_Y);
	nfui_regi_pre_event_callback(parent, pre_page_event_handler);

	nftab = nfui_nftab_new(mcf.sys_event_noti.cnt, (gchar**)strImage_h, 250, 40, NFTAB_DIR_H, (gchar**)strTabTitle, colidx);
	nfui_nftab_set_pango_font((NFTAB*)nftab, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin(nftab, 10);
	nfui_regi_pre_event_callback(nftab, pre_subtab_event_handler);
	nfui_nfobject_show(nftab);
	nfui_nffixed_put((NFFIXED*)parent, nftab, MENU_V_SUBTAB_PAGE_X, 5);

	for (i = 0; i < mcf.sys_event_noti.cnt; i++)
	{
		tab_page[i] = nfui_nffixed_new();
		nfui_nfobject_set_size(tab_page[i], MENU_V_SUBTAB_PAGE_W, MENU_V_SUBTAB_PAGE_H);
		nfui_nftab_regi_page((NFTAB*)nftab, tab_page[i], i);
		nfui_nffixed_put((NFFIXED*)parent, tab_page[i], MENU_V_SUBTAB_PAGE_X, MENU_V_SUBTAB_PAGE_Y);
		nfui_regi_pre_event_callback(tab_page[i], pre_subtab_page_event_handler);
	}

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_BUZZER];	
	if (pos != -1) VW_Init_EvtNoti_Buzzer_Page(tab_page[pos]);

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_DISP];	
	if (pos != -1) VW_Init_EvtNoti_Display_Page(tab_page[pos]);

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MAIL];	
	if (pos != -1) VW_Init_EvtNoti_Email_Page(tab_page[pos]);

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_FTP];	
	if (pos != -1) VW_Init_EvtNoti_FTP_Page(tab_page[pos]);

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_SMS];	    
	if (pos != -1) VW_Init_EvtNoti_SMS_Page(tab_page[pos]);
	
    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MOBILEPUSH];	    
	if (pos != -1) VW_Init_EvtNoti_MobilePush_Page(tab_page[pos]);

	nfui_nfobject_show(tab_page[0]);
}

gboolean VW_EvtNotiEvt_tab_out_handler()
{
    gint pos;
	gchar page_mask = 0;
	mb_type ret;

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_BUZZER];	
	if (pos != -1) {
		if (check_evt_noti_buzzer_changed()) page_mask |= (1 << pos);
	}

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_DISP];	
	if (pos != -1) {
		if (check_evt_noti_disp_changed()) page_mask |= (1 << pos);
	}	

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MAIL];	
	if (pos != -1) {
		if (check_evt_noti_email_changed()) page_mask |= (1 << pos);
	}

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_FTP];	
	if (pos != -1) {
		if (check_evt_noti_ftp_changed()) page_mask |= (1 << pos);
	}

    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_SMS];	
	if (pos != -1) {
		if (check_evt_noti_sms_changed()) page_mask |= (1 << pos);
	}
	
    pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MOBILEPUSH];	
	if (pos != -1) {
		if (check_evt_noti_mobilepush_changed()) page_mask |= (1 << pos);
	}

	if (page_mask != 0) 
	{
		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
		
		if (ret == NFTOOL_MB_OK) {
			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_BUZZER];
			if (pos != -1) {
				if (page_mask & (1 << pos)) save_evt_noti_buzzer_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_DISP];
			if (pos != -1) {
				if (page_mask & (1 << pos)) save_evt_noti_disp_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MAIL];
			if (pos != -1) {
				if (page_mask & (1 << pos)) save_evt_noti_email_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_FTP];
			if (pos != -1) {
				if (page_mask & (1 << pos)) save_evt_noti_ftp_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_SMS];
			if (pos != -1) {
				if (page_mask & (1 << pos)) save_evt_noti_sms_data();
			}
			
			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MOBILEPUSH];
			if (pos != -1) {
				if (page_mask & (1 << pos)) save_evt_noti_mobilepush_data();
			}
		}
		else if (ret == NFTOOL_MB_CANCEL) {
			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_BUZZER];
			if (pos != -1) {
				if (page_mask & (1 << pos)) restore_evt_noti_buzzer_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_DISP];
			if (pos != -1) {
				if (page_mask & (1 << pos)) restore_evt_noti_disp_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MAIL];
			if (pos != -1) {
				if (page_mask & (1 << pos)) restore_evt_noti_email_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_FTP];
			if (pos != -1) {
				if (page_mask & (1 << pos)) restore_evt_noti_ftp_data();
			}

			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_SMS];
			if (pos != -1) {
				if (page_mask & (1 << pos)) restore_evt_noti_sms_data();
			}
			
			pos = mcf.sys_event_noti.menu_pos[SYS_EVENTNOTI_MOBILEPUSH];
			if (pos != -1) {
				if (page_mask & (1 << pos)) restore_evt_noti_mobilepush_data();
			}
		}

		if(ret == NFTOOL_MB_OK) event_act_data_changed(TRUE);
	}

	return FALSE;
}


