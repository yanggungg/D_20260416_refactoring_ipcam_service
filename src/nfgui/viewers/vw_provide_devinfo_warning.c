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

#include "vw.h"
#include "vw_internal.h"
#include "vw_provide_devinfo_warning.h"


static NFWINDOW *g_curwnd = 0;
static gint g_retval = 0;


static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_retval = 0;

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}
	
	return FALSE;	
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_retval = 1;

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


gint vw_provide_devinfo_warning_open(NFOBJECT *parent, gchar *prev_str, gchar *next_str)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	
    gint pos_x, pos_y;
    guint size_w, size_h;	
    
  	gchar inBuf[2048];
  	gchar outBuf[2048];
    	
	g_retval = 0;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-800)/2, (1080-350)/2, 800, 350);
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
    pos_y = 80;

    memset(outBuf, 0x00, sizeof(outBuf));
    
    nfutil_get_line_feed_string(lookup_string("If you do not agree to provide device information in the network setup\nwizard, DDNS, remote access, and other network-related services may be\nlimited in functionality."), 
        win->width-40, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, NORMAL_SPACING);        

    obj = nfui_nflabel_new_with_pango_font(outBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, win->width-40, size_h+4); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);    

    pos_y += size_h+20;

    nfutil_get_line_feed_string(lookup_string("You may directly access this option under the System Management menu by running the Network Setup Wizard."), 
        win->width-40, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    size_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, NORMAL_SPACING);        

    obj = nfui_nflabel_new_with_pango_font(outBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(295));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nfobject_set_size(obj, win->width-40, size_h+4); 
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);    

	obj = nftool_normal_button_create_type1(prev_str, 174);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win->width/2-174-5, win->height-56);

	obj = nftool_normal_button_create_type1(next_str, 174);
	nfui_regi_post_event_callback(obj, post_next_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, win->width/2+5, win->height-56);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_PROVIDE_DEVINFO_WARNING, win, nfui_get_last_user());

	gtk_main();	

	nfui_page_close(PGID_PROVIDE_DEVINFO_WARNING, win);

	return g_retval;
}

