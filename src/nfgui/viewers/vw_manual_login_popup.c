#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "tools/nf_ui_tool.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nfbutton.h"

#include "vw_manual_login_popup.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "ix_mem.h"
#include "uxm.h"

#define PP_SIZE_WID					(550)
#define PP_SIZE_HEI					(240)

#define PP_LABEL_SIZE_WID           (210)
#define PP_LABEL_SIZE_HEI           (40)
#define PP_VALUE_SIZE_WID           (280)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_userid_obj = 0;
static NFOBJECT *g_pass_obj = 0;

static gchar *g_userid_buff;
static gchar *g_pass_buff;
static gint g_userid_len;
static gint g_pass_len;


static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

static gboolean post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable;
	GdkGC *gc;
	gint img_w, img_h;

	if (evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean post_userid_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		gchar *userid;
		NFOBJECT *top;
		guint x, y;

		if (kpid == KEYPAD_ENTER)
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

		userid = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, g_userid_len-1, VKEY_NORMAL);

		if (userid)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, userid);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			ifree(userid);
			userid = NULL;
		}
	}

	return FALSE;
}

static gboolean post_pass_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		gchar *pass;
		NFOBJECT *top;
		guint x, y;

		if (kpid == KEYPAD_ENTER)
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

		pass = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, g_pass_len-1, VKEY_NORMAL);

		if (pass)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, pass);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			ifree(pass);
			pass = NULL;
		}
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		strcpy(g_userid_buff, nfui_nflabel_get_text((NFLABEL*)g_userid_obj));
		strcpy(g_pass_buff, nfui_nflabel_get_text((NFLABEL*)g_pass_obj));
		
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

gint vw_manual_login_popup(NFWINDOW *parent, gchar *title, gchar *userid, gint userid_len, gchar *password, gint password_len)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	guint pos_x, pos_y;


	g_userid_buff = userid;
	g_pass_buff = password;

	g_userid_len = userid_len;
	g_pass_len = password_len;


	main_wnd = nftool_create_popup_window(parent, (1920-PP_SIZE_WID)/2, (1080-PP_SIZE_HEI)/2, PP_SIZE_WID, PP_SIZE_HEI, title, TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	pos_x = 20;
	pos_y = 56;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_x += PP_LABEL_SIZE_WID;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(userid, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_userid_label_event_handler);
	g_userid_obj = obj;

	pos_x = 20;
	pos_y += 44;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_x += PP_LABEL_SIZE_WID;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
	nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_pass_label_event_handler);
	g_pass_obj = obj;

	pos_y = PP_SIZE_HEI - 56;

	obj = nftool_normal_button_create_type1("APPLY", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PP_SIZE_WID/2-125, pos_y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PP_SIZE_WID/2+5, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	gtk_main();

	return 0;
}
