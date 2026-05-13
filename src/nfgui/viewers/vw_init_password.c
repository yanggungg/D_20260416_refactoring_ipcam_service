#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"

#include "vw_init_password.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "ix_mem.h"


#define MAX_STRING_SIZE		12

#define PI_WND_SIZE_WID			(guint)(545)
#define PI_WND_SIZE_HEI			(guint)(255)



#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_FIXED_SIZE_WID	(guint)(545)
#define PI_FIXED_SIZE_HEI	(guint)(255)
#define PI_FIXED_POS_X		(guint)(12)
#define PI_FIXED_POS_Y		(guint)(48)

enum {
	PIB_OK,
	PIB_CANCEL,

	PIB_BUTTONS
};

enum {
	INIT_ROW_NEW_PW,
	INIT_ROW_CONFIRM_PW,

	INIT_ROW_NUM
};

static g_ret = 0;


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *value[INIT_ROW_NUM];
static UserManageData g_admin_data;

static gboolean 
post_main_bg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) {
		g_curwnd = 0;
		uxm_unreg_imsg_event(obj, INFY_PASSWD_INIT_BY_WEB);
		gtk_main_quit();
	}
	else if (evt->type == INFY_PASSWD_INIT_BY_WEB) {
		g_ret = 1;
		evt_send_to_window("PASSWORD INIT", WND_CLOSE, 0, 0, 0);
//		nfui_nfobject_hide(obj);
//		nfui_nfobject_destroy(obj);
	}
	return FALSE;
}


static gboolean post_pw_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		gchar *title;
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

		if(DAL_get_enahnced_password())
			title = VirtualKey_Open2(g_curwnd, "ADMIN", nfui_nflabel_get_text(obj), x, y, NFUI_MAX_ENHANCED_PW_SIZE, VKEY2_PASSWORD);
		else
			title = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, NFUI_MAX_PW_SIZE, VKEY_PASSWORD);

		if(title)
		{
			nfui_nflabel_set_text(obj, title);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		GTimeVal last_temp;
		gint i;
		gchar *pw;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		top = nfui_nfobject_get_top(obj);

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[INIT_ROW_NEW_PW]), "")) 
		{
			nftool_mbox(g_curwnd,"NOTICE", "Please input new PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[INIT_ROW_CONFIRM_PW]), "")) 
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input confirm PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[INIT_ROW_NEW_PW]), 
			       nfui_nflabel_get_text((NFLABEL*)value[INIT_ROW_CONFIRM_PW]))) {
		}
		else
		{
			nftool_mbox(g_curwnd, "NOTICE", "Password doesn't match confirmation.", NFTOOL_MB_OK);
			return FALSE;
		}
		pw = nfui_nflabel_get_text((NFLABEL*)value[INIT_ROW_NEW_PW]);

		g_stpcpy(g_admin_data.pw, pw);

		g_get_current_time(&last_temp);
		/*
		for(i=0; i<MAX_USER_COUNT; i++)
			DAL_set_pw_last_changed_time(last_temp.tv_sec, i);

		DAL_save_db("usr");
		*/

		g_admin_data.pw_last_changed = (guint)last_temp.tv_sec;


		// ADMIN value '0'
		DAL_set_userManage_data(g_admin_data, 0);
		DAL_save_setup_db(NFSETUP_WINDOW_USER);

		g_ret = 1;

		nfui_nfobject_hide(top);

		nfui_nfobject_destroy(top);
	}

	return FALSE;
}
	
static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
	    }
		
		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}


gboolean PasswordInit_Open(NFWINDOW *parent)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *lbTemp;
	NFOBJECT *btns[PIB_BUTTONS];

	const gchar *strButton[] = {"OK", "CANCEL"};
	const gchar *strTitle[] = {"NEW PASSWORD", "CONFIRM NEW PASSWORD"};

	guint pos_x, pos_y;
	guint i;

	g_ret = 0;

	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, "PASSWORD", FALSE);
	nfui_nfwindow_set_title(main_wnd, "PASSWORD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)main_wnd, returnkey_proc);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PI_FIXED_SIZE_WID, PI_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, PI_FIXED_POS_X, PI_FIXED_POS_Y);
	nfui_nfobject_show(fixed1);

	pos_x = (guint)(DISPLAY_IS_D1 ? 4:8);
	pos_y = (guint)(DISPLAY_IS_D1 ? 10:8);

	DAL_get_userManage_data(&g_admin_data, 0);

	for(i=0; i<2; i++)
	{
		lbTemp = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		
		nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
		nfui_nflabel_set_spacing((NFLABEL *)lbTemp, SEMI_CONDENSED_SPACING);		
		nfui_nfobject_set_size(lbTemp, 300, 40);
		nfui_nffixed_put((NFFIXED*)fixed1, lbTemp, pos_x, pos_y);
		nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(lbTemp);
		pos_y += 42;
	}
	
	pos_x = 310;
	pos_y = 8;

	for( i=0; i<2; i++ )
	{
			value[i] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	

			nfui_nflabel_set_skin_type((NFLABEL *)value[i], NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nflabel_set_spacing((NFLABEL *)value[i], CONDENSED_SPACING);
			nfui_nflabel_set_align(value[i], NFALIGN_CENTER, 6);
			nfui_nfobject_support_multi_lang((NFOBJECT*)value[i], FALSE);
			nfui_nfobject_set_size(value[i], 190, 40);
			nfui_nflabel_set_invisible((NFLABEL*)value[i], TRUE);
			nfui_nffixed_put((NFFIXED*)fixed1, value[i], pos_x - 10, pos_y);
			nfui_nfobject_show(value[i]);
			pos_y += 42;

			nfui_regi_post_event_callback(value[i], post_pw_label_event_handler);
	}

	for( i=0; i<PIB_BUTTONS; i++ )
	{
		btns[i] = nftool_normal_button_create_type1(strButton[i], 160);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(btns[i]), NFALIGN_CENTER, 0);	
		nfui_nfobject_show(btns[i]);
	}
	
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_OK], 120, 126);
	nfui_nffixed_put((NFFIXED*)fixed1, btns[PIB_CANCEL], 286, 126);
	nfui_nfobject_disable(btns[PIB_CANCEL]);

	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_regi_post_event_callback(btns[PIB_OK], post_okbutton_event_handler);
	nfui_regi_post_event_callback(btns[PIB_CANCEL], post_cancelbutton_event_handler);

	nfui_nfobject_show(main_wnd);

	uxm_reg_imsg_event(main_wnd, INFY_PASSWD_INIT_BY_WEB);

	/* set for key navi */
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(value[INIT_ROW_NEW_PW], TRUE);

	gtk_main();

	return g_ret;
}




