#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

//#include "services/uxm.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfprogressbar.h"

#include "viewers/vw_arch_export.h"

#include "modules/ssm.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"

#define ARCHR_WIN_SIZE_W	(678)
#define ARCHR_WIN_SIZE_H	(220)

#define ARCHR_POS_X		((DISPLAY_ACTIVE_WIDTH - ARCHR_WIN_SIZE_W)/4*2)
#define ARCHR_POS_Y		((DISPLAY_ACTIVE_HEIGHT - ARCHR_WIN_SIZE_H)/2)

#define MAX_TAG_STRING_SIZE			14

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_win;
static NFOBJECT *g_tag;


static gboolean post_tag_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *str = NULL;
		gchar tag[32];
		gchar user[16];
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) 
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_TAG_STRING_SIZE, VKEY_ALPHANUMERIC);

		if(str) {
			g_stpcpy(tag, str);
			ifree(str);

			nfui_nflabel_set_text((NFLABEL*)obj, tag);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

		}
	}
	
	return FALSE;
}


static gboolean 
post_ok_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	RSV_CODE_E ret_val = RSV_SUCCESS;

	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		mb_type ret;
		GError *err = NULL;

		gchar *tag_name;
		NFOBJECT *top = NULL;
		top = nfui_nfobject_get_top(obj);
		tag_name = nfui_nflabel_get_text((NFLABEL*)g_tag);
		gint strLen = nfui_nflabel_get_strlen((NFLABEL*)g_tag);

		if (strLen < 1) {
			nftool_mbox(g_curwnd, "NOTICE", "Please insert a tag name.", NFTOOL_MB_OK);
			return FALSE;
		}

		char user_id[100];
		memset(user_id, 0x00, sizeof(user_id));
		ssm_get_cur_id(&user_id);
		nf_arch_info_modify(tag_name, user_id, "");

		ret_val = scm_reserve_avi();
		switch (ret_val) {
		case RSV_SUCCESS:
			ret = nftool_mbox(g_curwnd, "NOTICE", "Data is reserved successfully.", NFTOOL_MB_OK);
			if (ret == NFTOOL_MB_OK) nfui_nfobject_destroy(top);
			break;
		
		case RSV_CODE_FULL_LIST:													// List is full
			ret = nftool_mbox(g_curwnd, "NOTICE", "It's unable to reserve the data\nbecause the archiving list is full.",
							NFTOOL_MB_OK);
			if (ret == NFTOOL_MB_OK) nfui_nfobject_destroy(top);
			break;

		case RSV_CODE_FAIL_LOCK:
			ret = nftool_mbox(g_curwnd, "NOTICE", "Fail to reserve because of too many locked data.", NFTOOL_MB_OK);
			if (ret == NFTOOL_MB_OK) nfui_nfobject_destroy(top);
			break;

		case RSV_CODE_INV_COMMAND:
		case RSV_CODE_INV_DEV:
		case RSV_CODE_INV_PARAM:
		case RSV_CODE_INV_MEDIA:
		case RSV_CODE_FAIL:
			ret = nftool_mbox(g_curwnd, "WARNING", "Fail to reserve as internal error.", NFTOOL_MB_OK);
			if (ret == NFTOOL_MB_OK) nfui_nfobject_destroy(top);
			break;

		default:
			ret = nftool_mbox(g_curwnd, "WARNING","Unknown error.", 
										NFTOOL_MB_OK);
			if (ret == NFTOOL_MB_OK) nfui_nfobject_destroy(top);
			break;

		}
	}

	return FALSE;
}

static gboolean 
post_cancel_btn_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;


		NFOBJECT *top = NULL;
		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
} 

static gboolean 
post_Win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch (evt->type) 
	{
		case GDK_DELETE:
		{
			g_curwnd = 0;
			gtk_main_quit();
		}
		break;

		default:
		break;
	}

	return FALSE;
}


static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE)
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

///////////////////////////////////////////////////////////////////////
//
//
//

void vw_arch_reserved_open(NFWINDOW *parent)
{

	NFOBJECT *Fixed;
	NFOBJECT *obj;

	/* window */
	g_win = (NFOBJECT*)nfui_nfwindow_new(parent,  
										ARCHR_POS_X, ARCHR_POS_Y, 
										ARCHR_WIN_SIZE_W, ARCHR_WIN_SIZE_H);
	g_curwnd = g_win;

		
	nfui_regi_post_event_callback(g_win, post_Win_event_handler);

	/* fixed */
	Fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(Fixed, ARCHR_WIN_SIZE_W, ARCHR_WIN_SIZE_H);
	nfui_regi_post_event_callback(Fixed, post_fixed_event_cb);
	nfui_nfobject_show(Fixed);
	
	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RESERVE", 
									nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));	
	nfui_nfobject_set_size(obj, 670, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, 4, 4);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TAG NAME", 
									nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));	
	nfui_nfobject_set_size(obj, 641, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, 15, 61);

	// tag
	g_tag = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_tag, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)g_tag, NFALIGN_LEFT, 4);
	nfui_nfobject_support_multi_lang((NFOBJECT*)g_tag, FALSE);
	nfui_nfobject_set_size(g_tag, 641, 40);
	nfui_regi_post_event_callback(g_tag, post_tag_event_handler);
	nfui_nfobject_show(g_tag);
	nfui_nffixed_put((NFFIXED*)Fixed, g_tag, 15, 85);

	obj = nftool_normal_button_create_type1("OK", 150);
	nfui_regi_post_event_callback(obj, post_ok_btn_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, 200, 150);

	obj = nftool_normal_button_create_type1("CANCEL", 150);
	nfui_regi_post_event_callback(obj, post_cancel_btn_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)Fixed, obj, 360, 150);

	nfui_nfwindow_add((NFWINDOW*)g_win, Fixed);
	nfui_run_main_event_handler(g_win);
	nfui_nfobject_show(g_win);

	nfui_make_key_hierarchy((NFWINDOW*)g_win);
	nfui_set_key_focus(g_tag, TRUE);
	nfui_page_open(PGID_ARCH_RESERVE, g_win, nfui_get_last_user());
	
	gtk_main();

	nfui_page_close(PGID_ARCH_RESERVE, g_win);
}


