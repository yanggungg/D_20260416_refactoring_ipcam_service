/*
 * vw_warning_provide_device_open.c
 *	- dependencies :
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, May 26, 2015
 *
 */

#include "nf_afx.h"

#include "nf_sysman.h"

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
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfwindow.h"
#include "objects/nflistbox.h"

#include "iux_msg.h"
#include "uxm.h"
#include "smt.h"
#include "scm.h"

#include "vw.h"
#include "vw_internal.h"
#include "vw_unblock_authcode_popup.h"
#include "vw_vkeyboard.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_authcode_obj = 0;
static NFOBJECT *g_apply_obj = 0;

static NFOBJECT *g_wait_pop = NULL;


static gboolean post_authcode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    GdkEventKey *kevt;
    KEYPAD_KID kpid;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        NFOBJECT *top;
        gchar *strTemp;
        guint x, y;
        gchar buf[256];

        if(kpid == KEYPAD_ENTER)
        {
            nfui_nfobject_get_offset(obj, &x, &y);
            top = nfui_nfobject_get_top(obj);

            x += (obj->width)/2 + top->x;
            y += obj->height + top->y;
        }
        else
        {
            if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
    
            nfui_nfobject_get_window_pos(obj, &x, &y);

            x += ((GdkEventButton*)evt)->x;
            y += ((GdkEventButton*)evt)->y;
        }

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 16, VKEY_NORMAL);

        if (strTemp) 
        {
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            if (strlen(strTemp)) {
                nfui_nfobject_enable(g_apply_obj);
            }
            else {
                nfui_nfobject_disable(g_apply_obj);
            }

            nfui_signal_emit(g_apply_obj, GDK_EXPOSE, TRUE);

            ifree(strTemp);
            strTemp = NULL;
        }
    }
    
    return FALSE;
}

static gboolean post_apply_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;
		gchar strBuf[32];
    	mb_type ret;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		ret = nftool_mbox(g_curwnd, "CONFIRM", "This device will reboot and verify the auth code.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
        if (ret == NFTOOL_MB_CANCEL) return FALSE;

        strcpy(strBuf, nfui_nflabel_get_text((NFLABEL*)g_authcode_obj));
        DAL_set_dev_authcode(strBuf);
        DAL_save_setup_db(NFSETUP_WINDOW_SYSTEM);

        smt_set_service(SMT_SHUTDOWN);  
        scm_shutdown_system(IRET_SCM_SHUTDOWN_AUTHCODE);		
        scm_notify_to_system("dvr_status", NF_DVR_STATUS_SHUTDOWN);

        g_wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	}
	else if (evt->type == IRET_SCM_SHUTDOWN_AUTHCODE)
	{
        nftool_remove_waitbox((NFOBJECT*)g_wait_pop);              

        nftool_mbox_wait(g_curwnd, "NOTICE", "POWER OFF");        
        scm_reboot_system(RR_NORMAL_BOOT, 2000);
	}
	
	return FALSE;	
}

static gboolean post_close_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}
	
	return FALSE;	
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = nfui_nfobject_get_gc(obj);

   		nfui_nfobject_get_size(obj, &size_w, &size_h);
    	pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
    	nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
    	nfui_nfobject_get_size(obj, &size_w, &size_h);
    	nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

    	g_curwnd = 0;
		gtk_main_quit();    	
	}
	return FALSE;
}


gint vw_unblock_authcode_popup_open(NFOBJECT *parent)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	
    gint pos_x, pos_y;
    guint size_w, size_h;	

    SysInfoData sys_info;
    gchar strBuf[40];

	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);
       	
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-700)/2, (1080-450)/2, 700, 450);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	g_curwnd = win;

	// <----- fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win->width, win->height);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);


	// <----- title
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WARNING", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, win->width-8, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);
	
    pos_x = 20;
    pos_y = 70;

    obj = nfui_nflabel_new_with_pango_font("This device is blocked.\nPlease contact us and inform below information.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, win->width-40, 80); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);    

    pos_y += 100;

    obj = nfui_nflabel_new_with_pango_font("MAC ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box(sys_info.macAddr, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+(win->width-40)/2, pos_y);

    pos_y += 41;

    obj = nfui_nflabel_new_with_pango_font("EXTERNAL IP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(strBuf, 0x00, sizeof(strBuf));
    var_get_external_addr(strBuf, 40);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+(win->width-40)/2, pos_y);

    pos_y += 41;

    obj = nfui_nflabel_new_with_pango_font("WEB SERVICE PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(strBuf, 0x00, sizeof(strBuf));
	sprintf(strBuf, "%d", sys_info.webPort);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+(win->width-40)/2, pos_y);

    pos_y += 52;

    obj = nfui_nflabel_new_with_pango_font("AUTH CODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(strBuf, 0x00, sizeof(strBuf));
	DAL_get_dev_authcode(strBuf);

    obj = (NFOBJECT*)nfui_nflabel_new_text_box(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
    nfui_nflabel_set_spacing((NFLABEL *)obj, SEMI_CONDENSED_SPACING);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, (win->width-40)/2, 40); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+(win->width-40)/2, pos_y);
    nfui_regi_post_event_callback(obj, post_authcode_event_handler);
    g_authcode_obj = obj;
    
	obj = nftool_normal_button_create_type1("APPLY", 174);
	nfui_regi_post_event_callback(obj, post_apply_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win->width/2-174-5, win->height-56);
    g_apply_obj = obj;

    if (strlen(strBuf) == 0) nfui_nfobject_disable(obj);

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_close_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win->width/2+5, win->height-56);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

    uxm_reg_imsg_event(g_apply_obj, IRET_SCM_SHUTDOWN_AUTHCODE);
    uxm_monitor_on_imsg_event(g_apply_obj, IRET_SCM_SHUTDOWN_AUTHCODE);

	nfui_page_open(PGID_UNBLOCK_AUTHCODE, win, nfui_get_last_user());

	gtk_main();	

	nfui_page_close(PGID_UNBLOCK_AUTHCODE, win);

	return 0;
}

