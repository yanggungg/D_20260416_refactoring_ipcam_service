#include "nf_afx.h"

#include "iux_afx.h"

#include <ctype.h>
#include <string.h>

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
#include "objects/nfimage.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfcombobox.h"

#include "vw_desc.h"
#include "vw_init_userinfo.h"
#include "vw_sys_user_set_email.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "vw_pin_edit_popup.h"
#include "ix_mem.h"


#define MSG_1 	"The initial Administrator password is not secure.\nChanging the password from the initial setting is highly recommended."
#define MSG_2 	"A registered e-mail address may be used for verification\nif you forget your password."

#define PI_WND_SIZE_WID			(guint)(840)
#ifdef VIDECON
	#define PI_WND_SIZE_HEI			(guint)(980)
#else
	#define PI_WND_SIZE_HEI			(guint)(740)
#endif
#define SE_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH - PI_WND_SIZE_WID)/2)
#define SE_PP_POS_Y				(guint)((DISPLAY_ACTIVE_HEIGHT - PI_WND_SIZE_HEI)/2)

#define PI_LABEL_SIZE_WID       (380)
#define PI_LABEL_SIZE_HEI       (40)
#define PI_VALUE_SIZE_WID       (400)
#define QUE_SIZE_WID		    (500)
#define ANS_SIZE_WID		    (290)

enum {
	INIT_ROW_NEW_PW,
	INIT_ROW_CONFIRM_PW,
	INIT_ROW_NUM
};

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_pass_value[INIT_ROW_NUM];
static NFOBJECT *g_mail_obj;
static NFOBJECT *g_phone_obj;
static NFOBJECT *g_auth_hide_obj[2];
static NFOBJECT *g_pin_check_obj;
static NFOBJECT *g_pin_edit_button;
static NFOBJECT *g_answer_obj[QNA_COUNT];
static NFOBJECT *g_question_obj[QNA_COUNT];

static gchar g_pin_number[16];
static g_ret = 0;


char *strlwr(char *str)
{
  unsigned char *p = (unsigned char *)str;

  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }

  return str;
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
		gchar *pw;
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
    		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;

		}

		if(DAL_get_enahnced_password())
			pw = VirtualKey_Open2(g_curwnd, "ADMIN", nfui_nflabel_get_text(obj), x, y, NFUI_MAX_ENHANCED_PW_SIZE, VKEY2_PASSWORD);
		else
			pw = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, NFUI_MAX_PW_SIZE, VKEY_PASSWORD);

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

static gboolean post_pin_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gchar pinBuf[16];
		gint retVal = 0;
        gint onoff = 0;

        onoff = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        if (onoff) {
			if (strlen(g_pin_number) == 0) {
				retVal = vw_pin_edit_popup_open(g_curwnd, pinBuf, TRUE);
				if (retVal) {
					memset(g_pin_number, 0x00, sizeof(g_pin_number));
					strcpy(g_pin_number, pinBuf);
					nfui_nfobject_enable(g_pin_edit_button);
					nfui_signal_emit(g_pin_edit_button, GDK_EXPOSE, TRUE);
				}
				else {
					nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)obj, FALSE);
					nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
				}
			}
			else {
				nfui_nfobject_enable(g_pin_edit_button);
				nfui_signal_emit(g_pin_edit_button, GDK_EXPOSE, TRUE);
			}
		}
		else {
			nfui_nfobject_disable(g_pin_edit_button);
			nfui_signal_emit(g_pin_edit_button, GDK_EXPOSE, TRUE);
		}
	}
	
	return FALSE;
}

static gboolean post_pin_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		gchar pinBuf[16];
		gint retVal = 0;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(pinBuf, 0x00, sizeof(pinBuf));
		retVal = vw_pin_edit_popup_open(g_curwnd, pinBuf, strlen(g_pin_number) == 0);
		if (retVal) {
			memset(g_pin_number, 0x00, sizeof(g_pin_number));
			strcpy(g_pin_number, pinBuf);
		}
	}

	return FALSE;
}

static gboolean post_input_email_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *email;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        gint ret = 0;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}

		if (ivsc.vendor_code == 28 || ivsc.vendor_code == 228)
		{
			gchar email[64];
			
			ret = VW_Open_Set_Email_Popup(g_curwnd, email, sizeof(email));

			if (ret) 
			{
				nfui_nflabel_set_text((NFLABEL*)obj, email);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		}
		else
		{
			gchar *email = NULL;
			
			email = nfui_nflabel_get_text((NFLABEL*)obj);
			strTemp = VirtualKey_Open(g_curwnd, email, x, y, 64, VKEY_MAIL);        

			if (strTemp) 
			{
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			
				ifree(strTemp);
				strTemp = NULL;
			}
		}
	}

	return FALSE;
}

static gboolean post_input_phone_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *number;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        mb_type ret = NFTOOL_MB_OK;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}

        number = nfui_nflabel_get_text((NFLABEL*)obj);
		
   		strTemp = NumberKey_Str_Open(g_curwnd, number, x, y, 16);        

		if (strTemp) 
		{
			if (ret == NFTOOL_MB_OK)
			{
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			}
		
			ifree(strTemp);
			strTemp = NULL;
		}

	}

	return FALSE;
}

static gboolean post_input_answer_event(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    gchar *email;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER )
	{
		NFOBJECT *top;
		gchar *strTemp;
		guint x, y;
        gint ret = 0;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
	  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += (guint)((GdkEventButton*)evt)->x;
			y += (guint)((GdkEventButton*)evt)->y;
		}
		gchar *answer = NULL;
		
		answer = nfui_nflabel_get_text((NFLABEL*)obj);
		strTemp = VirtualKey_Open(g_curwnd, answer, x, y, 64, VKEY_MAIL); 

		if (strTemp) 
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		
			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_auth_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		GTimeVal last_temp;
		gint i;

        UserManageData userdata;
		memset(&userdata, 0x00, sizeof(userdata));

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);

        if (!strcmp(nfui_nflabel_get_text((NFLABEL*)g_mail_obj), "") && !strcmp(nfui_nflabel_get_text((NFLABEL*)g_phone_obj), ""))
        {
			nftool_mbox(g_curwnd, "NOTICE", "Please input an e-mail address.", NFTOOL_MB_OK);
			return FALSE;
        }

        if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_mail_obj), "") != 0)
        {
			if (nf_mail_send_check_email(nfui_nflabel_get_text((NFLABEL*)g_mail_obj)) == FALSE)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
				return FALSE;
			}
        }
        		
    	DAL_get_userManage_data(&userdata, 0);
		
		g_stpcpy(userdata.email, nfui_nflabel_get_text((NFLABEL*)g_mail_obj));
		g_stpcpy(userdata.phone, nfui_nflabel_get_text((NFLABEL*)g_phone_obj));

		DAL_set_userManage_data_nofire(userdata, 0);

		vw_open_authen_info_setup(g_curwnd, 0, 0);

		DAL_get_userManage_data(&userdata, 0);
		nfui_nflabel_set_text((NFLABEL*)g_mail_obj, userdata.email);
		nfui_nflabel_set_text((NFLABEL*)g_phone_obj, userdata.phone);

		if (userdata.email_certification == 1)
			nfui_nfobject_disable(g_mail_obj);

		if (userdata.phone_certification == 1)
			nfui_nfobject_disable(g_phone_obj);

	    nfui_signal_emit(g_mail_obj, GDK_EXPOSE, TRUE);
    	nfui_signal_emit(g_phone_obj, GDK_EXPOSE, TRUE);

	}

	return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top;
		GTimeVal last_temp;
		gchar vendor[256];
		gint i;

        UserManageData userdata;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)g_pass_value[INIT_ROW_NEW_PW]), "")) 
		{
			nftool_mbox(g_curwnd,"NOTICE", "Please input new PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)g_pass_value[INIT_ROW_CONFIRM_PW]), "")) 
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input confirm PW.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)g_pass_value[INIT_ROW_NEW_PW]), 
			       nfui_nflabel_get_text((NFLABEL*)g_pass_value[INIT_ROW_CONFIRM_PW]))) {
		}
		else
		{
			nftool_mbox(g_curwnd, "NOTICE", "Password doesn't match confirmation.", NFTOOL_MB_OK);
			return FALSE;
		}
/*
        if (!strcmp(nfui_nflabel_get_text((NFLABEL*)g_mail_obj), "") && !strcmp(nfui_nflabel_get_text((NFLABEL*)g_phone_obj), ""))
        {
			nftool_mbox(g_curwnd, "NOTICE", "Please input an e-mail address.", NFTOOL_MB_OK);
			return FALSE;
        }
*/

        if (ivsc.dfunc.pin.support) {
		    if (nfui_check_button_get_active((NFCHECKBUTTON*)g_pin_check_obj) && (strlen(g_pin_number) == 0)) {
				nftool_mbox(g_curwnd, "NOTICE", "Please enter pin number.", NFTOOL_MB_OK);
		    	return FALSE;
			}
        }

		if (ivsc.vendor_code == 28)
		{
			if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_mail_obj), "") != 0)
			{
				if (nf_mail_send_check_email(nfui_nflabel_get_text((NFLABEL*)g_mail_obj)) == FALSE)
				{
					nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
					return FALSE;
				}
			}
			else
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please input an e-mail address.", NFTOOL_MB_OK);
				return FALSE;
			}
		}

		if (ivsc.dfunc.support_qna) {
			if (strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[0])) 
				|| strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[1])) 
				|| strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[2]))) 
			{
				if (nfui_combobox_get_cur_index(g_question_obj[0]) == nfui_combobox_get_cur_index(g_question_obj[1])
					|| nfui_combobox_get_cur_index(g_question_obj[1]) == nfui_combobox_get_cur_index(g_question_obj[2])
					|| nfui_combobox_get_cur_index(g_question_obj[2]) == nfui_combobox_get_cur_index(g_question_obj[0])) 
				{
					nftool_mbox(g_curwnd, "NOTICE", "Duplicate question. Please select another question.", NFTOOL_MB_OK);
					return FALSE;
				}

				if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[0]))
				|| !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[1]))
				|| !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[2])))
				{
					nftool_mbox(g_curwnd, "NOTICE", "Please input answers for each question.", NFTOOL_MB_OK);
					return FALSE;
				}
			}
		}

    	DAL_get_userManage_data(&userdata, 0);
    	
		g_stpcpy(userdata.pw, nfui_nflabel_get_text((NFLABEL*)g_pass_value[INIT_ROW_NEW_PW]));
		g_get_current_time(&last_temp);
		userdata.pw_last_changed = last_temp.tv_sec;
		g_stpcpy(userdata.email, nfui_nflabel_get_text((NFLABEL*)g_mail_obj));
		g_stpcpy(userdata.phone, nfui_nflabel_get_text((NFLABEL*)g_phone_obj));
    	
		for (i = 0; i < 2; i++)
		{
			if (nfui_radio_button_get_toggled((NFBUTTON*)g_auth_hide_obj[i])) 
				userdata.certi_popup_hide = i;
		}

		if (ivsc.dfunc.support_qna) {
			for (i = 0; i < QNA_COUNT; i++){
				userdata.question[i] = nfui_combobox_get_cur_index(g_question_obj[i]);
				g_stpcpy(userdata.answer[i], nfui_nflabel_get_text((NFLABEL*)g_answer_obj[i]));
				strlwr(userdata.answer[i]);
			}
		}

        if (ivsc.dfunc.pin.support) {
		    userdata.use_pin = nfui_check_button_get_active((NFCHECKBUTTON*)g_pin_check_obj);
		    g_stpcpy(userdata.pin_number, g_pin_number);
        }

		DAL_set_userManage_data(userdata, 0);
		DAL_save_setup_db(NFSETUP_WINDOW_USER);
		g_ret = 1;

		nfui_nfobject_hide(top);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_msg_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_EXPOSE)
    {

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

static gboolean post_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE) 
	{
		g_curwnd = 0;
		uxm_unreg_imsg_event(obj, INFY_USERINFO_INIT_BY_WEB);
		gtk_main_quit();
	}
	else if (evt->type == INFY_USERINFO_INIT_BY_WEB) 
	{
		g_ret = 1;
		evt_send_to_window("PASSWORD INIT", WND_CLOSE, 0, 0, 0);
	}
	
	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gint vw_init_userinfo_open(NFWINDOW *parent)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;

	const gchar *strTitle[] = {"NEW PASSWORD", "CONFIRM NEW PASSWORD"};
	const gchar *strShow[] = {"ON", "OFF"};

	guint pos_x, pos_y;
	guint i;
	guint size_w, size_h;
	guint chk_w, chk_h;

	GSList *slist = NULL;
	
    UserManageData userdata;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];


	memset(&userdata, 0x00, sizeof(userdata));
	DAL_get_userManage_data(&userdata, 0);

    memset(g_pin_number, 0x00, sizeof(g_pin_number));
	g_ret = 0;

	main_wnd = nftool_create_popup_window(parent, SE_PP_POS_X, SE_PP_POS_Y, PI_WND_SIZE_WID, PI_WND_SIZE_HEI, "USER INFORMATION SETUP", FALSE);
	nfui_nfwindow_set_title(main_wnd, "PASSWORD INIT");
	nfui_regi_post_event_callback(main_wnd, post_win_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)main_wnd, returnkey_proc);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	
	pos_x = 20;
	pos_y = 66;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(MSG_1, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
	nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(226));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	if(ivsc.vendor_code == 28){
		nfui_nfobject_set_size(obj, PI_WND_SIZE_WID-40, 140);
	}
	else
	{
	nfui_nfobject_set_size(obj, PI_WND_SIZE_WID-40, 90);
	}
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_msg_label_event_cb);

	if(ivsc.vendor_code == 28){
		pos_y += 150;
	}
	else{
	pos_y += 110;
	}

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SET PASSWORD");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 60;

	for (i = 0; i < 2; i++)
	{
		obj = nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_set_size(obj, PI_LABEL_SIZE_WID, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
		
		obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
		nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
		nfui_nflabel_set_skin_type((NFLABEL *)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
		nfui_nfobject_set_size(obj, PI_VALUE_SIZE_WID, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PI_LABEL_SIZE_WID+20, pos_y);
		nfui_regi_post_event_callback(obj, post_pw_label_event_handler);		
		g_pass_value[i] = obj;
		
		pos_y += 42;
	}

	pos_y += 8;
	
	if (ivsc.dfunc.pin.support) {
		obj = nfui_checkbutton_new(userdata.use_pin);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size(obj, &chk_w, &chk_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, 60, pos_y + (40-chk_h)/2);
		nfui_regi_post_event_callback(obj, post_pin_check_event_handler);
		g_pin_check_obj = obj;

		obj = nfui_nflabel_new_with_pango_font("USE PIN LOGIN", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));		
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
		nfui_nfobject_set_size(obj, 280, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, 64+chk_w, pos_y);

		obj = nftool_normal_button_create_popup_type2("EDIT", 210);
		nfui_nfobject_show(obj);
		if (!userdata.use_pin) nfui_nfobject_disable(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, 394+chk_w, pos_y+2);
		nfui_regi_post_event_callback(obj, post_pin_edit_button_event_handler);
		g_pin_edit_button = obj;
	}

	pos_y += 50;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(MSG_2, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
	nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(226));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	if(ivsc.vendor_code == 28){
		nfui_nfobject_set_size(obj, PI_WND_SIZE_WID-40, 140);
	}
	else{
	nfui_nfobject_set_size(obj, PI_WND_SIZE_WID-40, 90);
	}
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_msg_label_event_cb);

	if(ivsc.vendor_code == 28){
		pos_y += 150;
	}
	else{
	pos_y += 110;
	}

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "AUTHENTICATION INFORMATION SETUP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 60;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("E-MAIL ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, PI_LABEL_SIZE_WID, PI_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(obj, PI_VALUE_SIZE_WID, PI_LABEL_SIZE_HEI);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PI_LABEL_SIZE_WID+20, pos_y);
	nfui_regi_post_event_callback(obj, post_input_email_event_cb);
    g_mail_obj = obj;

	pos_y += 44;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PHONE NUMBER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, PI_LABEL_SIZE_WID, PI_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(obj, PI_VALUE_SIZE_WID, PI_LABEL_SIZE_HEI);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PI_LABEL_SIZE_WID+20, pos_y);
	nfui_regi_post_event_callback(obj, post_input_phone_event_cb);
    g_phone_obj = obj;

	pos_y += 22;

	if(ivsc.dfunc.support_qna) {
		obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, "PASSWORD RECOVERY QUESTIONS SETUP");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

		pos_y += 60;	

		for(i = 0; i < QNA_COUNT; i++) {
			obj = nfui_combobox_new(QUESTIONS, 11, 0);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, QUE_SIZE_WID, PI_LABEL_SIZE_HEI);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
			g_question_obj[i] = obj;

			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nfobject_set_size(obj, ANS_SIZE_WID, PI_LABEL_SIZE_HEI);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+QUE_SIZE_WID+10, pos_y);
			nfui_regi_post_event_callback(obj, post_input_answer_event);
			g_answer_obj[i] = obj;

			pos_y += 44;
		}
	}

	if(ivsc.dfunc.support_auth_popup_hide)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUTHENTICATION INFORMATION SETUP", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_set_size(obj, 450, PI_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

		radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
		radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
		radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
		radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);		

		pos_x = (8 + 200);

		nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

		for (i = 0; i < 2; i++)
		{
			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
			nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PI_LABEL_SIZE_WID+20, pos_y + (40-size_h)/2);
			g_auth_hide_obj[i] = obj;

			if (i == 0 && userdata.certi_popup_hide == 0) { 
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);    // ON - hide off
    		}
    		else if (i == 1 && userdata.certi_popup_hide == 1) {    
    		    nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);    // OFF - hide on
    		}

			if (i == 0)		slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			else nfui_radio_button_add_group(NF_BUTTON(obj), slist);

			pos_x += 40;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strShow[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, 60, PI_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PI_LABEL_SIZE_WID+20, pos_y);

			pos_x += 60;
		}

		pos_y += 43;
	}

	pos_y += 22;

	obj = nftool_normal_button_create_type1("AUTHENTICATE", 200);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (PI_WND_SIZE_WID/2-4-200), PI_WND_SIZE_HEI-68);
	nfui_regi_post_event_callback(obj, post_auth_button_event_handler);
	if (!scm_is_wan_connected()) nfui_nfobject_disable(obj);

	obj = nftool_normal_button_create_type1("OK", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (PI_WND_SIZE_WID/2+4), PI_WND_SIZE_HEI-68);
	nfui_regi_post_event_callback(obj, post_okbutton_event_handler);

	nfui_nfobject_show(main_wnd);
	uxm_reg_imsg_event(main_wnd, INFY_USERINFO_INIT_BY_WEB);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);

	gtk_main();

	return g_ret;
}
