/*
 * vw_provide_devinfo_notice.c
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
#include "vw_provide_devinfo_notice.h"


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_list_obj;

static gint _insert_line_feed_string_listbox(gchar *str, gint expose)
{
	gchar *p;
	gchar *pStr;
	gchar *insert_str;

	p = str;

	while(1)
	{
		pStr = strchr(p,'\n');

		if(pStr == NULL)
		{
			insert_str = g_strdup(p);
			if (!insert_str) break;
			nfui_listbox_set_text((NFLISTBOX*)g_list_obj, &insert_str);
			g_free(insert_str);
			break;
		}
		else
		{
			insert_str = g_strndup(p, (pStr-p));
			if (!insert_str) break;			
			nfui_listbox_set_text((NFLISTBOX*)g_list_obj, &insert_str);
			g_free(insert_str);
			p += (pStr-p+1);
		}
	}

    if (expose) nfui_signal_emit(g_list_obj, GDK_EXPOSE, TRUE);

	return 0;
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


gint vw_provide_devinfo_notice_open(NFOBJECT *parent)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *list_obj;
	
	guint li_size_w, li_size_h;
	gint i, lc_size;
	
  	gchar inBuf[2048];
  	gchar outBuf[2048];
    	
	win = (NFOBJECT*)nfui_nfwindow_new(parent, (1920-800)/2, (1080-800)/2, 800, 800);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	g_curwnd = win;

	// <----- fixed
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, win->width, win->height);
	nfui_nfobject_show(fixed);
	nfui_regi_post_event_callback(fixed, post_fixed_event_handler);


	// <----- title
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Agreement to Provide Device Information", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, win->width-40, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

    lc_size = win->width - 40;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size -= li_size_w;

	list_obj = nfui_listbox_new(1, &lc_size, 40);
	nfui_listbox_set_skin_type(NF_LISTBOX(list_obj),NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(list_obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_listbox_set_use_infocus_box(NF_LISTBOX(list_obj), FALSE);
	nfui_nfobject_set_size(list_obj, win->width-40, win->height-56-70);
	nfui_nfobject_modify_bg(list_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(list_obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(list_obj);
	nfui_nffixed_put((NFFIXED*)fixed, list_obj, 20, 56);
	g_list_obj = list_obj;

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("This unit transmits some information of the device to the server to\nprovide services such as DDNS."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Type of information that device is sending"), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("IP address, MAC address, FW version, DDNS address, HTTP/RTSP Port."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("How this information is used"), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Collected information of device is used for DDNS, mobile remote access,\ncounterfeit identification and used to maintain and improve current\nservices."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Retention period of collected information"), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Unless we no longer need the device information to provide the service,\nthe collected information will be kept until the customer's request."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("The information collected can be permanently destroyed at the request\nof the customer."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("How this information is protected"), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Information collected from the device may be forwarded to an outside\nof country for the purposes described in this agreement."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("All information is securely transmitted using SSL during transmission."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Personal information, such as passwords or identifying information, \nis not collected."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("We are committed to protecting this information from unauthorized \naccess, modification, disclosure, or deletion. This strict confidentiality also\nextends to the authorized employees who have the authority to review \ninformation."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("However, as it does not fully meet requirements of the appropriate\nsafeguards, such as standard privacy provisions, device information\ntransmitted outside of the country may be at risk."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("Withdraw consent"), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("You can revoke consent by\n[Menu > System Settings > System Management > Factory Reset]."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("In order to utilize this information for any other purpose outside of the \nstated purpose in this policy user consent must first be obtained."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    _insert_line_feed_string_listbox(" ", 0);

	memset(outBuf, 0x00, sizeof(outBuf));
    nfutil_get_line_feed_string(lookup_string("If you refuse to accept this request for information, remote functionality may be limited."), 
        lc_size-4, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), outBuf, sizeof(outBuf));
    _insert_line_feed_string_listbox(outBuf, 0);
    
	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_close_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (win->width-174)/2, win->height-56);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_open(PGID_PROVIDE_DEVINFO_NOTICE, win, nfui_get_last_user());

	gtk_main();	

	nfui_page_close(PGID_PROVIDE_DEVINFO_NOTICE, win);

	return 0;
}

