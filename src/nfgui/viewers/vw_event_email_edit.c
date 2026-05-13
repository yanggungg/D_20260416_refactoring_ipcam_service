
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vkeyboard.h"

#include "vw_event_email_edit.h"
#include "ix_mem.h"

enum {
	EMAIL_ADD_COMPLETE = 0,
	EMAIL_ADD_FULL,
	EMAIL_ADD_DUPLICATION
};


static gchar (*g_address)[EMAIL_STRING_LENGTH];
static guint g_addrCnt = 0;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_addrObj;
static NFOBJECT *g_addObj;
static NFOBJECT *g_lboxObj;


static void init_address(gchar (*address)[EMAIL_STRING_LENGTH]);
static gint add_address(gchar *address);
static gboolean delete_address(guint index);
static void redraw_lbox();


static void init_address(gchar (*address)[EMAIL_STRING_LENGTH])
{
	guint i;

	if(g_address) g_address = NULL;
	if(g_addrCnt) g_addrCnt = 0;

	if(address != NULL) {
		g_address = address;

		for(i=0; i<EMAIL_COUNT; i++) {
			//g_message("%s ::::::::::::::::::::::::: %p %d::::::::::::", __FUNCTION__, g_address[i], strlen(g_address[i]));
			if(strlen(g_address[i]))
				g_addrCnt++;
		}
	}
}

static gint add_address(gchar *address) 
{
	gint i;

	if(address) {
		//g_message("%s ::::::::::::::::::addrCnt %d  address : %s ", __FUNCTION__, g_addrCnt, address);
		if(g_addrCnt < EMAIL_COUNT) {
			for(i=0; i<=g_addrCnt; i++) {
				if(!strcmp(g_address[i], address)) 
					return EMAIL_ADD_DUPLICATION;
			}

			g_stpcpy(g_address[g_addrCnt], address);
			g_addrCnt += 1;
			
			return EMAIL_ADD_COMPLETE;
		}
	}
	return EMAIL_ADD_FULL;
}

static gboolean delete_address(guint index)
{
	if(index + 1 == EMAIL_COUNT) {
		memset(g_address[index], 0x00, EMAIL_STRING_LENGTH);
		g_addrCnt -= 1;
	}
	else if (index + 1 < EMAIL_COUNT) {
    	g_memmove(g_address[index], g_address[index + 1], ((EMAIL_COUNT - index - 1) * EMAIL_STRING_LENGTH));
    	memset(g_address[g_addrCnt - 1], 0x00, EMAIL_STRING_LENGTH);

    	g_addrCnt -= 1;
	}
	else {
		memset(g_address[index], 0x00, EMAIL_STRING_LENGTH);
		return FALSE;
	}

	return TRUE;
}

static void redraw_lbox()
{
	guint i;
	gchar *pStr;

	nfui_listbox_delete_all(NF_LISTBOX(g_lboxObj));
	
	for(i=0; i<g_addrCnt; i++)  {
		//g_message("%s ::::::::::::::::::addrCnt %d  address : %s ", __FUNCTION__, g_addrCnt, g_address[i]);
		pStr = g_address[i];
		nfui_listbox_set_text(NF_LISTBOX(g_lboxObj), &pStr);
	}

	nfui_signal_emit(g_lboxObj, GDK_EXPOSE, TRUE);
}

static gboolean post_email_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_2BUTTON_PRESS)
	{
		gchar *addr = NULL;
		guint x, y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		nfui_nfobject_get_window_pos(obj, &x, &y);
		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

		addr = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(g_addrObj), x, y, EMAIL_STRING_LENGTH - 1, VKEY_MAIL);

		if(addr) {
			nfui_nflabel_set_text((NFLABEL*)g_addrObj, addr);
			nfui_signal_emit(g_addrObj, GDK_EXPOSE, FALSE);

			if(nfui_nfobject_is_disabled(g_addObj)) {
				nfui_nfobject_enable(g_addrObj);
				nfui_signal_emit(g_addObj, GDK_EXPOSE, FALSE);
			}

			ifree(addr);
			addr = NULL;
		}
	}

	return FALSE;
}

static gboolean post_add_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *addr = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		addr = nfui_nflabel_get_text(g_addrObj);
		if(strlen(addr)) 
		{
			if(nf_mail_send_check_email(addr) == FALSE) {  
				nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
				return FALSE;
			}

			switch(add_address(addr)) {
				case EMAIL_ADD_COMPLETE:
					redraw_lbox();

					nfui_nflabel_set_text(g_addrObj, "");
					nfui_signal_emit(g_addrObj, GDK_EXPOSE, FALSE);
					break;

				case EMAIL_ADD_FULL:
					nftool_mbox(g_curwnd, "WARNING", "It's unable to add the email because the list is full.", NFTOOL_MB_OK); 
					return FALSE;

				case EMAIL_ADD_DUPLICATION:
					nftool_mbox(g_curwnd, "WARNING", "The same email address already exists.", NFTOOL_MB_OK); 
					return FALSE;
			}
		}
		else 
		{
			nftool_mbox(g_curwnd, "WARNING", "Please input address.", NFTOOL_MB_OK);		// addr NULL
		}
	}
	return FALSE;
}

static gboolean post_lbox_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_EXPOSE) 
	{
		gint lcnt = 0;

		lcnt = nfui_listbox_get_box_count(NF_LISTBOX(obj));

        if (lcnt == -1)
        {
    		if (g_addrCnt) redraw_lbox();
        }
        else
        {
    		if (lcnt != g_addrCnt) redraw_lbox();
        }
	}

	return FALSE;
}

static gboolean post_close_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);
		if(top) nfui_nfobject_destroy(top);
	}
	return FALSE;
}

static gboolean post_delete_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint f_idx;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		f_idx = nfui_listbox_get_focus_idx(NF_LISTBOX(g_lboxObj));
		if(f_idx >= 0) {
			if(!strlen(g_address[f_idx])) 
				return FALSE;

			delete_address((guint)f_idx);
			nfui_listbox_delete(NF_LISTBOX(g_lboxObj), (guint)f_idx);
			nfui_signal_emit(g_lboxObj, GDK_EXPOSE, TRUE);
		}
		//g_message("%s ::::::::::::::::::focus idx : %d ", __FUNCTION__, f_idx);

	}
	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
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
	}

	return FALSE;
}

static gboolean post_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

void VW_EvtNoti_Email_Edit(NFWINDOW *parent, gchar (*address)[EMAIL_STRING_LENGTH])
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *lbox;
	NFOBJECT *obj;
	NFOBJECT *delete_btn;

	gint scl_w, scl_h;
	guint pos_x, pos_y;
	guint col_w = 428;


	// init data
	//g_message("%s ::::::::::::::::::::::::: %p ::::::::::", __FUNCTION__, address);
	init_address(address);


	/* window */
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 660, 196, 592, 598);
	g_curwnd = win;
	nfui_nfwindow_set_title(win, "SYSTEM SETUP - EVENT - EVENT NOTI - EMAIL");
	nfui_regi_post_event_callback(win, post_win_event_cb);

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, 592, 598);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EDIT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	
	pos_x = 26;
	pos_y = 64;

	/* email address */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("E-MAIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);


	pos_x += (96 + 10);

	g_addrObj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_addrObj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(g_addrObj, 338, 40);
	nfui_nflabel_set_align((NFLABEL*)g_addrObj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(g_addrObj);
	nfui_regi_post_event_callback(g_addrObj, post_email_event_cb);

	nfui_nffixed_put((NFFIXED*)fixed, g_addrObj, pos_x, pos_y);


	/* add */
	pos_x += (338 + 10);

	g_addObj = nftool_normal_button_create_type3("ADD", 80);
	nfui_regi_post_event_callback(g_addObj, post_add_button_event_handler);
	nfui_nfobject_show(g_addObj);

	nfui_nffixed_put((NFFIXED*)fixed, g_addObj, pos_x, pos_y);



	/* list box */
	pos_x = 26;
	pos_y += (40 + 13);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 96, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	pos_x += (10 + 96);

	nfui_get_image_size(IMG_N_SCROLL_UP, &scl_w, &scl_h);
    col_w -= scl_w;

	g_lboxObj = nfui_listbox_new(1, &col_w, 40);
	nfui_listbox_set_skin_type(NF_LISTBOX(g_lboxObj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(g_lboxObj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_listbox_support_multi_lang(NF_LISTBOX(g_lboxObj), FALSE);
	nfui_nfobject_set_size(g_lboxObj, 428, 370);
	nfui_nfobject_use_focus(g_lboxObj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(g_lboxObj);
	nfui_regi_post_event_callback(g_lboxObj, post_lbox_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed, g_lboxObj, pos_x, pos_y);

	pos_x = 126;
	pos_y += (370 + 34);

	/* button */
	obj = nftool_normal_button_create_type1("DELETE", 174);
	nfui_regi_post_event_callback(obj, post_delete_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
	delete_btn = obj;

	pos_x += (174 + 6);

	obj = nftool_normal_button_create_type1("CLOSE", 174);
	nfui_regi_post_event_callback(obj, post_close_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(delete_btn, TRUE);

	nfui_page_open(PGID_EVT_EMAIL_EDIT_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_EVT_EMAIL_EDIT_POPUP, win);
}
