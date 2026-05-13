
#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"
#include "tools/nf_ui_function.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"

#include "modules/ssm.h"

#include "vw_userpwd.h"
#include "vw_live_statusbar.h"
#include "vw_live_start_menu.h"

#include "iux_msg.h"
#include "iux_afx.h"
#include "vsm.h"
#include "nfdal.h"
#include "uxm.h"





#define	PWEM_POS_X			((DISPLAY_ACTIVE_WIDTH - PWEM_WIDTH)/4*2)
#define	PWEM_POS_Y			((DISPLAY_ACTIVE_HEIGHT - PWEM_HEIGHT)/2)
#define	PWEM_WIDTH			(500)
#define	PWEM_HEIGHT			(280)

#define PWEM_BORDER			(guint)(4)
#define	PWEM_GAP			(guint)(8)
#define	PWEM_HEAD_SIZE		(guint)(40)

#define	PWEM_FIXED_X		(guint)(PWEM_BORDER + PWEM_GAP)
#define	PWEM_FIXED_Y		(guint)(PWEM_BORDER + PWEM_HEAD_SIZE + PWEM_GAP)
#define	PWEM_FIXED_WIDTH	(guint)(PWEM_WIDTH - ((PWEM_BORDER+PWEM_GAP)*2))
#define	PWEM_FIXED_HEIGHT	(guint)(PWEM_HEIGHT - ((PWEM_BORDER+PWEM_GAP)*2 + PWEM_HEAD_SIZE))

#define	PWEM_LABEL_X		(PWEM_GAP)
#define	PWEM_LABEL_Y		(PWEM_GAP*2)
#define	PWEM_LABEL_WIDTH	(guint)(PWEM_FIXED_WIDTH - (PWEM_GAP*2))
#define	PWEM_LABEL_HEIGHT	(guint)(22)

#define	PWEM_CHECK_X		(PWEM_GAP)
#define	PWEM_CHECK_Y		(112)

#define	PWEM_OK_BTN_X		((PWEM_FIXED_WIDTH - 192)/2)
#define	PWEM_OK_BTN_Y		(PWEM_FIXED_HEIGHT - 40 - 20)


static NFWINDOW *g_curwnd = 0;
static gint g_ret_val = 0;

static gboolean post_later_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		NFOBJECT *check_btn;
		gint is_checked = 0;
		gint usr_idx;
		GTimeVal tvTemp;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		check_btn = nfui_nfobject_get_data(obj, "check_button");
		usr_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "usr_idx"));
		
		if(check_btn)
		{
			is_checked = nfui_check_button_get_active((NFCHECKBUTTON*)check_btn);

			if(is_checked)
			{
				memset(&tvTemp, 0, sizeof(GTimeVal));
				g_get_current_time(&tvTemp);
				DAL_set_expire_check_time(tvTemp.tv_sec, usr_idx);
				DAL_save_db("usr");
			}
		}

        g_ret_val = 1;

		top  = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_msg_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE) {
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}


static gint prvExpiredDlgOpen(NFWINDOW *parent, gint usr_idx, gint remained_day)
{
	NFOBJECT *msg_win;
	NFOBJECT *msg_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *label;
	NFOBJECT *check_btn;
	NFOBJECT *ok_btn;

	gint chk_w, chk_h;

	gchar strTemp[512];
	const gchar *utf_str = NULL;

	const gchar *strCont[] = 	{
		 "Password expiration day is %d remained.\nChange your password.",
		 "Password expiration day was passed.\nChange your password."
	};

    g_ret_val = 0;

	msg_win = nftool_create_popup_window(parent, PWEM_POS_X, PWEM_POS_Y, PWEM_WIDTH, PWEM_HEIGHT, "PASSWORD EXPIRATION NOTICE", FALSE);
	msg_fixed = ((NFWINDOW*)msg_win)->child;
	nfui_regi_post_event_callback(msg_win, post_msg_win_event_handler);
	nfui_nfobject_show(msg_fixed);
	g_curwnd = msg_win;

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, PWEM_FIXED_WIDTH, PWEM_FIXED_HEIGHT);
	nfui_nfobject_show(fixed1);
	nfui_nffixed_put((NFFIXED*)msg_fixed, fixed1, PWEM_FIXED_X, PWEM_FIXED_Y);

	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
	nfui_nfobject_set_size(label, PWEM_LABEL_WIDTH, PWEM_LABEL_HEIGHT*3);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed1, label, PWEM_LABEL_X, PWEM_LABEL_Y);
	nfui_nfobject_show(label);

	if(remained_day > 0)
	{
		utf_str = lookup_string(strCont[0]);
		if(utf_str == NULL)	utf_str = strCont[0];

		g_sprintf(strTemp, utf_str, remained_day);
	}
	else
	{
		utf_str = lookup_string(strCont[1]);
		if(utf_str == NULL)	utf_str = strCont[1];

		g_sprintf(strTemp, "%s", utf_str);
	}

	nfui_nflabel_set_text((NFLABEL*)label, strTemp);

	check_btn = nfui_checkbutton_new(FALSE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(check_btn), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(check_btn, &chk_w, &chk_h);
	nfui_nfobject_show(check_btn);
	nfui_nffixed_put((NFFIXED*)fixed1, check_btn, PWEM_CHECK_X, PWEM_CHECK_Y);

	label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Don't show this message for a day.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
	nfui_nflabel_set_align((NFLABEL*)label, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(label, PWEM_LABEL_WIDTH - PWEM_CHECK_X - (chk_w*2), PWEM_LABEL_HEIGHT);
	nfui_nfobject_use_focus(label, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)fixed1, label, PWEM_CHECK_X + (chk_w*2), PWEM_CHECK_Y);
	nfui_nfobject_show(label);


	ok_btn = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(ok_btn, post_later_button_event_handler);
	nfui_nfobject_set_data(ok_btn, "check_button", check_btn);
	nfui_nfobject_set_data(ok_btn, "usr_idx", GINT_TO_POINTER(usr_idx));	
	nfui_nfobject_show(ok_btn);
	nfui_nffixed_put((NFFIXED*)fixed1, ok_btn, PWEM_OK_BTN_X, PWEM_OK_BTN_Y);

	nfui_nfobject_show(msg_win);

	nfui_make_key_hierarchy((NFWINDOW*)msg_win);
	nfui_set_key_focus(ok_btn, TRUE);

	gtk_main();

	return g_ret_val;
}

gint VW_CheckPasswordExpired(NFWINDOW *parent)
{
	gint remain = 0;
	gint ret;
	
	remain = ssm_get_remain_day_to_expire();
	if(remain == 0) ret = prvExpiredDlgOpen(parent, ssm_get_cur_idx(), remain);

	return ret;
}

gint VW_CheckPasswordExpired_id(NFWINDOW *parent, gchar *id)
{
	gint remain = 0;
	gint ret;
	
	remain = ssm_get_remain_day_to_expire_user(id);
	if(remain == 0) ret = prvExpiredDlgOpen(parent, ssm_get_usr_idx(id), remain);

	return ret;
}

gint VW_CheckPasswordExpired_Close()
{
	if (!g_curwnd) return;
	
	nfui_nfobject_destroy(g_curwnd);
}
