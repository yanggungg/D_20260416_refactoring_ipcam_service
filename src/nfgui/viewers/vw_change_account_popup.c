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

#include "vw_change_account_popup.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "ix_mem.h"
#include "uxm.h"

#define CHANGE_ACC_PP_SIZE_WID		(550)
#define CHANGE_ACC_PP_SIZE_HEI		(280)
#define CHANGE_ACC_PP_POS_X			(guint)((DISPLAY_ACTIVE_WIDTH- CHANGE_ACC_PP_SIZE_WID)/2)
#define CHANGE_ACC_PP_POS_Y 		(guint)((DISPLAY_ACTIVE_HEIGHT - CHANGE_ACC_PP_SIZE_HEI)/2 - 10)

#define PP_LABEL_SIZE_WID           (320)
#define PP_LABEL_SIZE_HEI           (40)
#define PP_VALUE_SIZE_WID           (190)

#define MAX_ID_STRING_SIZE		10

static mb_type g_retVal = 0;
static NFWINDOW *g_curwnd = 0;
static gchar *g_campw;
static gchar *g_id;
static guint g_maxch;
static guint g_opt;
static gchar *g_org_pw;
static guint g_pop_type;

enum {
	ROW_USER_ID = 0,
	ROW_USE_NVR_PW,
	ROW_ORG_PW,
	ROW_NEW_PW,
	ROW_CON_FW,

	ROW_MAX
};

static NFOBJECT *value[ROW_MAX];

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
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

	if(evt->type == GDK_EXPOSE) 
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean post_pw_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		gchar *pw;
		NFOBJECT *top;
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
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			  return FALSE;
	  	   	}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		if(g_opt & (1 << OPT_ENAHNCED_PASSWORD))
			pw = VirtualKey_Open2(g_curwnd, g_id, nfui_nflabel_get_text(obj), x, y, g_maxch, VKEY2_PASSWORD);
		else if(g_opt & (1 << OPT_USE_VKEY_ALPHANUMERIC))
			pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, g_maxch, VKEY_ALPHANUMERIC);
		else
			pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, g_maxch, VKEY_PASSWORD);

		if(pw)
		{
			if ( var_get_vendor_code() == 32 || var_get_vendor_code() == 132 || var_get_vendor_code() == 28 || var_get_vendor_code() == 128) 
			{
				nfui_nflabel_set_text((NFLABEL*)obj, pw);
			}
			else 
			{
				if (strcmp(pw, "1234") == 0)
				{
					nftool_mbox(g_curwnd, "ERROR", "It can't be used as user password.", NFTOOL_MB_OK);
				}
				else
				{
					nfui_nflabel_set_text((NFLABEL*)obj, pw);
				}
			}

			ifree(pw);
			pw = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_usenvrpwbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gchar passwd[STRING_SIZE_32];

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		if(DAL_get_user_passwd(passwd, 0))
		{
			//nfui_nflabel_set_text((NFLABEL*)value[ROW_NEW_PW], passwd);
			//nfui_signal_emit((NFOBJECT*)value[ROW_NEW_PW], GDK_EXPOSE, TRUE);
			g_stpcpy(g_campw, passwd);
			g_retVal = 1;

			topwin = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(topwin);
		}
	}
	return FALSE;
}


static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

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
		gchar *pw;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);
		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[ROW_NEW_PW]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input new PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[ROW_CON_FW]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input confirm PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(strcmp(nfui_nflabel_get_text((NFLABEL*)value[ROW_NEW_PW]), nfui_nflabel_get_text((NFLABEL*)value[ROW_CON_FW])))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Password doesn't match confirmation.", NFTOOL_MB_OK);
			return FALSE;
		}

		if (g_pop_type == ORG_PW_CHECK)	//mode = 1
		{
			if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[ROW_ORG_PW]), ""))
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please input PW.", NFTOOL_MB_OK);
				return FALSE;
			}

			if(strcmp(nfui_nflabel_get_text((NFLABEL*)value[ROW_ORG_PW]), g_org_pw))
			{
				nftool_mbox(g_curwnd, "NOTICE", "Invalid password.", NFTOOL_MB_OK);
				return FALSE;
			}
		}


		pw = nfui_nflabel_get_text((NFLABEL*)value[ROW_NEW_PW]);
		g_stpcpy(g_campw, pw);

		g_retVal = 1;
		nfui_nfobject_destroy(topwin);
	}
	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gint VW_Open_account_change_Popup(NFWINDOW *parent, gchar *title, guint opt, gchar* id, gchar* org_password, gchar* password, guint max_ch, mode_type mode)
{
	g_retVal = 0;

	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	guint pos_x, pos_y;
	guint wnd_pos_y, wnd_hei;
	int i;

	const gchar *strTitle[] = {"USER ID", "APPLY NVR ADMIN PASSWORD", "CONFIRM PASSWORD", "NEW PASSWORD",  "CONFIRM NEW PASSWORD"};

	if(opt & (1 << OPT_USE_ADMIN_PASS))
	{
		wnd_hei = CHANGE_ACC_PP_SIZE_HEI + 80;
		if (mode == ORG_PW_CHECK)	wnd_hei += 43;
	}
	else
	{
		wnd_hei = CHANGE_ACC_PP_SIZE_HEI;
		if (mode == ORG_PW_CHECK)	wnd_hei += 43;
	}

	main_wnd = nftool_create_popup_window(parent, CHANGE_ACC_PP_POS_X, CHANGE_ACC_PP_POS_Y, CHANGE_ACC_PP_SIZE_WID, wnd_hei, title, TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	nfui_nfwindow_set_title(main_wnd, "ACCOUNT CHANGE POPUP");
	
	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	g_curwnd = main_wnd;
	g_id = id;
	g_campw = password;
	g_maxch = max_ch;
	g_org_pw = org_password;
	g_pop_type = mode;
	g_opt = opt;

	/*
	if(opt & (1 << OPT_ENAHNCED_PASSWORD))
		g_enh_pw = TRUE;
	else
		g_enh_pw = FALSE;
		*/

	pos_y = 56;

	for(i = ROW_USER_ID; i < ROW_MAX; i++)
	{
		pos_x = 20;

		if(i == ROW_USER_ID)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += PP_LABEL_SIZE_WID;

			value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_use_focus(value[i], NFOBJECT_FOCUS_OFF);
			nfui_nfobject_support_multi_lang((NFOBJECT*)value[i], FALSE);
			nfui_nfobject_set_size(value[i], PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_show(value[i]);
			nfui_nflabel_set_text((NFLABEL*)value[i], id);

			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
		}
		else if(i == ROW_USE_NVR_PW)
		{
			if(!(opt & (1 << OPT_USE_ADMIN_PASS)))
				continue;

			pos_y += 20;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, 310, PP_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += 320;

			value[i] = nftool_normal_button_create_type1("APPLY", 110);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(value[i]), NFALIGN_CENTER, 0);
			nfui_nfobject_show(value[i]);
			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_usenvrpwbutton_event_handler);

			pos_y += 20;
		}
		else if(i == ROW_ORG_PW)	//mode = 1
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);

			value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nfobject_support_multi_lang((NFOBJECT*)value[i], FALSE);
			nfui_nflabel_set_invisible((NFLABEL*)value[i], TRUE);
			nfui_nfobject_set_size(value[i], PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_show(value[i]);

			if (mode == ORG_PW_CHECK)
			{
				nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
				pos_x += PP_LABEL_SIZE_WID;
				nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
				nfui_regi_post_event_callback(value[i], post_pw_label_event_handler);
			}
			else
				pos_y -= 43;
		}
		else if(i == ROW_NEW_PW)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += PP_LABEL_SIZE_WID;

			value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nfobject_support_multi_lang((NFOBJECT*)value[i], FALSE);
			nfui_nflabel_set_invisible((NFLABEL*)value[i], TRUE);
			nfui_nfobject_set_size(value[i], PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_show(value[i]);

			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_pw_label_event_handler);
		}
		else // (i == ROW_CON_FW)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

			pos_x += PP_LABEL_SIZE_WID;

			value[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)value[i], NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nfobject_support_multi_lang((NFOBJECT*)value[i], FALSE);
			nfui_nflabel_set_invisible((NFLABEL*)value[i], TRUE);
			nfui_nfobject_set_size(value[i], PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_show(value[i]);

			nfui_nffixed_put((NFFIXED*)main_fixed, value[i], pos_x, pos_y);
			nfui_regi_post_event_callback(value[i], post_pw_label_event_handler);
		}

		pos_y += 43;
	}

	pos_y = wnd_hei - 70;

	obj = nftool_normal_button_create_type1("APPLY", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, CHANGE_ACC_PP_SIZE_WID/2-125, pos_y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 120);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, CHANGE_ACC_PP_SIZE_WID/2+5, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	
	if(opt & (1 << OPT_FORCE_CHANGE_PASSWORD))
	{
		nfui_nfwindow_set_returnkey_proc((NFWINDOW*)main_wnd, returnkey_proc);
		nfui_nfobject_disable(obj);
	}

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	gtk_main();

	return g_retVal;
}

gint VW_Close_account_change_Popup()
{
	if (!g_curwnd) return -1;

    evt_send_to_window("ACCOUNT CHANGE POPUP", WND_CLOSE, 0, 0, 0);
//	nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
	return 0;
}

