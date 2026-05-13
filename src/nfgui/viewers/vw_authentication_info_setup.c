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
#include "viewers/objects/nfcheckbutton.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard_strnum.h"

#include "vw_authentication_info_setup.h"
#include "vw_desc.h"

#include "scm.h"
#include "ix_mem.h"
#include "nf_util_sms.h"
#include "nf_util_mail.h"


#define UA_WIN_SIZE_WID				(980)
#define UA_WIN_SIZE_HEI				(470)   //620
#define UA_WIN_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH-UA_WIN_SIZE_WID)/2)
#define UA_WIN_POS_Y 				(guint)((DISPLAY_ACTIVE_HEIGHT-UA_WIN_SIZE_HEI)/2)

#define PP_LABEL_SIZE_WID           (300)
#define PP_LABEL_SIZE_HEI           (40)
#define PP_VALUE_SIZE_WID           (320)

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_mail_obj;
static NFOBJECT *g_mail_send_obj;
static NFOBJECT *g_mail_authen_obj;
static NFOBJECT *g_mail_limit_obj;
static NFOBJECT *g_mail_confirm_obj;

static NFOBJECT *g_phone_obj;
static NFOBJECT *g_phone_send_obj;
static NFOBJECT *g_phone_authen_obj;
static NFOBJECT *g_phone_limit_obj;
static NFOBJECT *g_phone_confirm_obj;
static NFOBJECT *g_auth_hide_obj;
static NFOBJECT *g_ok_btn_obj;
static NFOBJECT *g_wait_pop = NULL;

static gint g_user_idx = 0;
static gint g_need_check = 0;
static gchar g_veriCode[64];

static gint g_mail_cmpl = 0;
static gint g_phone_cmpl = 0;

static guint g_exp_timer = 0;
static gint g_cnt_sec = 0;

static gint g_ret_val = -1;


#define MSG_1 	"E-mail has not been certified.\nA registered authentication information may be used for verification\nif you forget your password."



static char *_trans_string_lang(char *str)
{
    gchar lang[64];

    memset(lang, 0x00, sizeof(lang));
    DAL_get_language(lang);

    if (!strcmp(lang, "KOREAN"))
    {     
        if (!strcmp(str, "[vendor] Please enter the certification number[%s].")) {
            return "������ȣ[%s]�� �Է��� �ּ���.";
        }               
        else
            return lookup_string(str);
    }
    else
    {
        return lookup_string(str);
    }
    
    return 0;
}

static gint _set_mail_pre_status()
{
    if (g_mail_cmpl == 1) return -1;

	nfui_clear_key_focus(g_mail_authen_obj);
	nfui_nflabel_set_text((NFLABEL*)g_mail_authen_obj, "");
	nfui_nflabel_set_text((NFLABEL*)g_mail_limit_obj, "");

    nfui_nfobject_enable(g_mail_obj);
    nfui_nfobject_enable(g_mail_send_obj);
    nfui_nfobject_disable(g_mail_authen_obj);
    nfui_nfobject_disable(g_mail_limit_obj);
    nfui_nfobject_disable(g_mail_confirm_obj);

    nfui_signal_emit(g_mail_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_confirm_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_mail_post_status()
{
    if (g_mail_cmpl == 1) return -1;

    nfui_nfobject_disable(g_mail_obj);
    nfui_nfobject_disable(g_mail_send_obj);
    nfui_nfobject_enable(g_mail_authen_obj);
    nfui_nfobject_enable(g_mail_limit_obj);
    nfui_nfobject_enable(g_mail_confirm_obj);

    nfui_signal_emit(g_mail_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_confirm_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_mail_block_status()
{
    if (g_mail_cmpl == 1) return -1;

    nfui_nfobject_disable(g_mail_obj);
    nfui_nfobject_disable(g_mail_send_obj);
    nfui_nfobject_disable(g_mail_authen_obj);
    nfui_nfobject_disable(g_mail_limit_obj);
    nfui_nfobject_disable(g_mail_confirm_obj);

    nfui_signal_emit(g_mail_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_confirm_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_mail_cmpl_status()
{
    nfui_nflabel_set_skin_type(g_mail_obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nfobject_use_focus(g_mail_obj, 0);
    nfui_nfobject_enable(g_mail_obj);

    nfui_nfobject_disable(g_mail_send_obj);
    nfui_nfobject_disable(g_mail_authen_obj);
    nfui_nfobject_disable(g_mail_limit_obj);
    nfui_nfobject_disable(g_mail_confirm_obj);

    nfui_signal_emit(g_mail_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_mail_confirm_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_ok_btn_obj);
    nfui_signal_emit(g_ok_btn_obj, GDK_EXPOSE, TRUE);

    nfui_make_key_hierarchy(g_curwnd);

    g_mail_cmpl = 1;

    return 0;
}

static gint _set_phone_pre_status()
{
    if (g_phone_cmpl == 1) return -1;

	nfui_clear_key_focus(g_phone_authen_obj);
	nfui_nflabel_set_text((NFLABEL*)g_phone_authen_obj, "");
	nfui_nflabel_set_text((NFLABEL*)g_phone_limit_obj, "");

    nfui_nfobject_enable(g_phone_obj);
    nfui_nfobject_enable(g_phone_send_obj);
    nfui_nfobject_disable(g_phone_authen_obj);
    nfui_nfobject_disable(g_phone_limit_obj);
    nfui_nfobject_disable(g_phone_confirm_obj);

    nfui_signal_emit(g_phone_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_confirm_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_phone_post_status()
{
    if (g_phone_cmpl == 1) return -1;

    nfui_nfobject_disable(g_phone_obj);
    nfui_nfobject_disable(g_phone_send_obj);
    nfui_nfobject_enable(g_phone_authen_obj);
    nfui_nfobject_enable(g_phone_limit_obj);
    nfui_nfobject_enable(g_phone_confirm_obj);

    nfui_signal_emit(g_phone_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_confirm_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_phone_block_status()
{
    if (g_phone_cmpl == 1) return -1;

    nfui_nfobject_disable(g_phone_obj);
    nfui_nfobject_disable(g_phone_send_obj);
    nfui_nfobject_disable(g_phone_authen_obj);
    nfui_nfobject_disable(g_phone_limit_obj);
    nfui_nfobject_disable(g_phone_confirm_obj);

    nfui_signal_emit(g_phone_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_confirm_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_phone_cmpl_status()
{
    nfui_nflabel_set_skin_type(g_phone_obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nfobject_use_focus(g_phone_obj, 0);
    nfui_nfobject_enable(g_phone_obj);

    nfui_nfobject_disable(g_phone_send_obj);
    nfui_nfobject_disable(g_phone_authen_obj);
    nfui_nfobject_disable(g_phone_limit_obj);
    nfui_nfobject_disable(g_phone_confirm_obj);

    nfui_signal_emit(g_phone_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_send_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_authen_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_limit_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(g_phone_confirm_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_ok_btn_obj);
    nfui_signal_emit(g_ok_btn_obj, GDK_EXPOSE, TRUE);

    nfui_make_key_hierarchy(g_curwnd);

    g_phone_cmpl = 1;

    return 0;
}

static gboolean _exp_mail_update_proc(gpointer data)
{
	char buf[64];
	NFOBJECT *label = (NFOBJECT*)data;

	--g_cnt_sec;

	if (g_cnt_sec == -1)
	{
        _set_mail_pre_status();
        _set_phone_pre_status();

		g_exp_timer = 0;
		return FALSE;
	}

	sprintf(buf, "%d : %02d", g_cnt_sec/60, g_cnt_sec%60);
	nfui_nflabel_erase(g_mail_limit_obj);
	nfui_nflabel_set_text((NFLABEL*)g_mail_limit_obj, buf);
	nfui_signal_emit(g_mail_limit_obj, GDK_EXPOSE, TRUE);

	return TRUE;
}

static gboolean _exp_phone_update_proc(gpointer data)
{
	char buf[64];
	NFOBJECT *label = (NFOBJECT*)data;

	--g_cnt_sec;

	if (g_cnt_sec == -1)
	{
        _set_mail_pre_status();
        _set_phone_pre_status();

		g_exp_timer = 0;
		return FALSE;
	}

	sprintf(buf, "%d : %02d", g_cnt_sec/60, g_cnt_sec%60);
	nfui_nflabel_erase(g_phone_limit_obj);
	nfui_nflabel_set_text((NFLABEL*)g_phone_limit_obj, buf);
	nfui_signal_emit(g_phone_limit_obj, GDK_EXPOSE, TRUE);

	return TRUE;
}

static gint _generate_verification_code(gchar *code)
{
	time_t ger_num = 0;
	GTimeVal time;

	g_get_current_time(&time);

	ger_num = (time.tv_sec * 2 * 8502) % 100000;

	if (ger_num < 0)
		ger_num *= -1;

//	g_message("%s, %d, %05ld", __FUNCTION__, __LINE__, ger_num);
	g_sprintf(code, "%05ld", ger_num);

	return 0;
}

static gint _send_sms_verification_code(gchar *code, gchar *recv)
{
	gint ret_val;
	gchar text[512];

    nftool_mbox_sleep_auto(g_curwnd, 2, "WAIT", "Please wait...");

    memset(text, 0x00, sizeof(text));
    g_sprintf(text, _trans_string_lang("[vendor] Please enter the certification number[%s]."), code);

//	ret_val = scm_send_s1_sms_verification_code(recv, text);
	return ret_val;
}

static gint _display_sms_error_code(gint err_code)
{
	gint retVal = -1;

	g_message("%s, %d, error_code:%d", __FUNCTION__, __LINE__, err_code);

	switch (err_code)
	{
		case NF_SMS_OK:
			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Sending a verification code succeeded.");
			retVal = 0;
		break;
		case NF_SMS_CONNECTION_ERROR:
		case NF_SMS_NOT_AVAILABLE_TIME:
		case NF_NOT_CONNECTED:
		case NF_FAIL_TO_MAKE_FILE_DIRECTORY:
		case NF_FAIL_TO_MAKE_BLACKLIST_DIRECTORY:
		case NF_WRONG_CMD_ID:
		case NF_FAIL_TO_SET_DEVICE:
		case NF_SMS_IS_BLACK_LIST:
		case NF_SMS_DAILY_RESTRICTION_OVER:
		case NF_ERROR_NOT_DEFINED:
			nftool_mbox(g_curwnd, "WARNING", "Sending a verification code failed.", NFTOOL_MB_OK);
			retVal = -1;
		break;
#if 0
		case NF_SMS_DNS_ERROR:
			nftool_mbox(g_curwnd, "WARNING", "DNS server is not responding.", NFTOOL_MB_OK);
			retVal = -1;
		break;
#endif
		default:
			nftool_mbox(g_curwnd, "WARNING", "Unknown error.", NFTOOL_MB_OK);
		break;
	}

	return retVal;
}

static gint _send_mail_verification_code(gchar *code)
{
	gchar *from;
	gint ret_val;
	gchar text[512];

	g_wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

    memset(text, 0x00, sizeof(text));
    g_sprintf(text, _trans_string_lang("[vendor] Please enter the certification number[%s]."), code);

	from = nfui_nflabel_get_text((NFLABEL*)g_mail_obj);
	ret_val = scm_send_mail_verification_code(from, from, text);
	return ret_val;
}

static gint _display_mail_error_code(gint err_code)
{
	gint retVal = -1;

	if (err_code == NF_MAIL_SEND_STATUS_SUCCESS)
	{
		if(g_wait_pop)
	  	{
	  		nftool_remove_waitbox(g_wait_pop);
	  		g_wait_pop = NULL;
	  	}

		nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Sending a verification code succeeded.");
		retVal = 0;
	}
	else if (err_code == NF_MAIL_SEND_STATUS_FAILED)
	{
		if(g_wait_pop)
	  	{
	  		nftool_remove_waitbox(g_wait_pop);
	  		g_wait_pop = NULL;
	  	}

		nftool_mbox(g_curwnd, "WARNING", "Sending a verification code failed.", NFTOOL_MB_OK);
		retVal = -1;
	}

	return retVal;
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

        email = nfui_nflabel_get_text((NFLABEL*)obj);

   		strTemp = VirtualKey_Open(g_curwnd, email, x, y, 64, VKEY_MAIL);

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

static gboolean post_mail_send_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *autherStr;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		autherStr = nfui_nflabel_get_text((NFLABEL*)g_mail_obj);

		if ((autherStr == NULL) || (strlen(autherStr) == 0) || (nf_mail_send_check_email(autherStr) == FALSE))
		{
			nftool_mbox(g_curwnd, "WARNING", "Please check the E-mail.", NFTOOL_MB_OK);
			return FALSE;
		}

		memset(g_veriCode, 0x00, sizeof(g_veriCode));
		_generate_verification_code(g_veriCode);

		if (_send_mail_verification_code(g_veriCode) == -1)
		{
			g_message("%s, %d", __FUNCTION__, __LINE__);
			_display_mail_error_code(NF_MAIL_SEND_STATUS_FAILED);
		}
	}

	return FALSE;
}

static gboolean post_phone_send_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *autherStr;
		gint err_code;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		autherStr = nfui_nflabel_get_text((NFLABEL*)g_phone_obj);

		if ((autherStr == NULL) || (strlen(autherStr) == 0))
		{
			nftool_mbox(g_curwnd, "WARNING", "Please check the phone.", NFTOOL_MB_OK);
			return FALSE;
		}

		memset(g_veriCode, 0x00, sizeof(g_veriCode));
		_generate_verification_code(g_veriCode);

		err_code = _send_sms_verification_code(g_veriCode, autherStr);

		if (_display_sms_error_code(err_code) == -1) return FALSE;

        _set_phone_post_status();
        _set_mail_block_status();

		g_cnt_sec = 600;
		g_exp_timer = g_timeout_add(1000, _exp_phone_update_proc, 0);
	}

	return FALSE;
}

static gboolean post_veri_value_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
			if ((g_exp_timer != 0) && (ret == NFTOOL_MB_OK))
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

static gboolean post_mail_confirm_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
        gchar *strTemp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		strTemp = nfui_nflabel_get_text((NFLABEL*)g_mail_authen_obj);

		if (strcmp(g_veriCode, strTemp) != 0)
		{
			nftool_mbox(g_curwnd, "WARNING", "Verification code is incorrect.", NFTOOL_MB_OK);
			return FALSE;
		}

        if (g_exp_timer) {
    		g_source_remove(g_exp_timer);
    		g_exp_timer = 0;
        }

		nftool_mbox(g_curwnd, "NOTICE", "Verification code is correct.", NFTOOL_MB_OK);

        _set_mail_cmpl_status();
        _set_phone_pre_status();
	}

	return FALSE;
}

static gboolean post_phone_confirm_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
        gchar *strTemp;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		strTemp = nfui_nflabel_get_text((NFLABEL*)g_phone_authen_obj);

		if (strcmp(g_veriCode, strTemp) != 0)
		{
			nftool_mbox(g_curwnd, "WARNING", "Verification code is incorrect.", NFTOOL_MB_OK);
			return FALSE;
		}

        if (g_exp_timer) {
    		g_source_remove(g_exp_timer);
    		g_exp_timer = 0;
        }

		nftool_mbox(g_curwnd, "NOTICE", "Verification code is correct.", NFTOOL_MB_OK);

        _set_phone_cmpl_status();
        _set_mail_pre_status();
	}

	return FALSE;
}


static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
        UserManageData userdata;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (g_need_check) {
            if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK")) return FALSE;
        }

	    memset(&userdata, 0x00, sizeof(UserManageData));
        DAL_get_userManage_data(&userdata, g_user_idx);

        if (g_exp_timer) {
            g_source_remove(g_exp_timer);
            g_exp_timer = 0;
        }

        if (g_mail_cmpl) {
            strcpy(userdata.email, nfui_nflabel_get_text((NFLABEL*)g_mail_obj));
            userdata.email_certification = 1;
        }

        if (g_phone_cmpl) {
            strcpy(userdata.phone, nfui_nflabel_get_text((NFLABEL*)g_phone_obj));
            userdata.phone_certification = 1;
        }

        DAL_set_userManage_data(userdata, g_user_idx);
        DAL_save_setup_db(NFSETUP_WINDOW_USER);

        nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

        g_ret_val = 1;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (g_exp_timer) {
    		g_source_remove(g_exp_timer);
    		g_exp_timer = 0;
        }

        g_ret_val = 0;

		if(ivsc.dfunc.support_auth_popup_hide) // ZICOM
		{
			gboolean hide = 0;
			UserManageData userdata;
			
			hide = nfui_check_button_get_active((NFCHECKBUTTON*)g_auth_hide_obj);

			if (hide) 
			{
				memset(&userdata, 0x00, sizeof(UserManageData));
				DAL_get_userManage_data(&userdata, g_user_idx);

				userdata.certi_popup_hide = 1;

				DAL_set_userManage_data(userdata, g_user_idx);
				DAL_save_setup_db(NFSETUP_WINDOW_USER);
			}

		}

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
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

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == INFY_EMAIL_VERIFICATION_RESULT)
	{
		gint err_code = ((CMM_MESSAGE_T *)data)->param;

		if (_display_mail_error_code(err_code) == -1) return FALSE;

        _set_mail_post_status();
        _set_phone_block_status();

		g_cnt_sec = 600;
		g_exp_timer = g_timeout_add(1000, _exp_mail_update_proc, 0);
	}
    else if (evt->type == INFY_USRDB_CHANGE_NOTIFY)
    {
		NFOBJECT *topwin;

		g_message("###yanggungg : %s, %d INFY_USRDB_CHANGE_NOTIFY", __FUNCTION__, __LINE__);

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
    }
	else if (evt->type == GDK_DELETE)
	{
        if (g_exp_timer) {
            g_source_remove(g_exp_timer);
            g_exp_timer = 0;
        }

		uxm_unreg_imsg_event(obj, INFY_EMAIL_VERIFICATION_RESULT);
		uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);

		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

gint vw_open_authen_info_setup(NFWINDOW *parent, gint usr_idx, gint need_check)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i;

    UserManageData userdata;

    g_user_idx = usr_idx;
    g_need_check = need_check;

    g_mail_cmpl = 0;
    g_phone_cmpl = 0;

    g_ret_val = -1;
	
    DAL_get_userManage_data(&userdata, usr_idx);

	if (!scm_is_wan_connected()) return 0;
//    if(!strcmp(userdata.id, S1_RASS_STR)) return 0;

	main_wnd = nftool_create_popup_window(parent, UA_WIN_POS_X, UA_WIN_POS_Y, UA_WIN_SIZE_WID, UA_WIN_SIZE_HEI, "", TRUE);
    nfui_nfwindow_set_title((NFWINDOW*)main_wnd, "AUTHEN INFO SETUP");
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUTHENTICATION INFORMATION SETUP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, UA_WIN_SIZE_WID-40, 36);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);

	pos_x = 20;
	pos_y = 66;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(MSG_1, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(226));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, UA_WIN_SIZE_WID-40, 90);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_msg_label_event_cb);

	pos_y += 110;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "E-MAIL ADDRESS SETUP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 60;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("E-MAIL ADDRESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(userdata.email, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20, pos_y);
	nfui_regi_post_event_callback(obj, post_input_email_event_cb);
    g_mail_obj = obj;

	obj = nftool_normal_button_create_popup_type1("SEND VERIFICATION CODE", 290);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_spacing(NF_BUTTON(obj), SEMI_CONDENSED_SPACING);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20+PP_VALUE_SIZE_WID+4, pos_y);
	nfui_regi_post_event_callback(obj, post_mail_send_button_event_handler);
    g_mail_send_obj = obj;

	pos_y += 44;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INPUT VERIFICATION CODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID/2-1, PP_LABEL_SIZE_HEI);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20, pos_y);
	nfui_regi_post_event_callback(obj, post_veri_value_event_cb);
    g_mail_authen_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID/2-1, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20+PP_VALUE_SIZE_WID/2+2, pos_y);
    g_mail_limit_obj = obj;

	obj = nftool_normal_button_create_popup_type1("CONFIRM", 290);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_spacing(NF_BUTTON(obj), SEMI_CONDENSED_SPACING);
    nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20+PP_VALUE_SIZE_WID+4, pos_y);
	nfui_regi_post_event_callback(obj, post_mail_confirm_button_event_handler);
    g_mail_confirm_obj = obj;

	if(ivsc.dfunc.support_auth_popup_hide) // ZICOM
	{
		obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size((NFCHECKBUTTON*)obj, &size_w, &size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+80);
		g_auth_hide_obj = obj;	

		obj = nfui_nflabel_new_with_pango_font("Do not show the window again.", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 320, 27);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x + size_w + 5, pos_y+80);
	}
	
	pos_y += 80;

	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "PHONE NUMBER SETUP");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 60;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PHONE NUMBER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(userdata.phone, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20, pos_y);
	nfui_regi_post_event_callback(obj, post_input_phone_event_cb);
    g_phone_obj = obj;

	obj = nftool_normal_button_create_popup_type1("SEND VERIFICATION CODE", 290);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_spacing(NF_BUTTON(obj), SEMI_CONDENSED_SPACING);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20+PP_VALUE_SIZE_WID+4, pos_y);
	nfui_regi_post_event_callback(obj, post_phone_send_button_event_handler);
    g_phone_send_obj = obj;

	pos_y += 44;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INPUT VERIFICATION CODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID/2-1, PP_LABEL_SIZE_HEI);
	nfui_nfobject_disable(obj);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20, pos_y);
	nfui_regi_post_event_callback(obj, post_veri_value_event_cb);
    g_phone_authen_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID/2-1, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_disable(obj);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20+PP_VALUE_SIZE_WID/2+2, pos_y);
    g_phone_limit_obj = obj;

	obj = nftool_normal_button_create_popup_type1("CONFIRM", 290);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_spacing(NF_BUTTON(obj), SEMI_CONDENSED_SPACING);
	nfui_nfobject_disable(obj);
//	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+PP_LABEL_SIZE_WID+20+PP_VALUE_SIZE_WID+4, pos_y);
	nfui_regi_post_event_callback(obj, post_phone_confirm_button_event_handler);
    g_phone_confirm_obj = obj;

	obj = nftool_normal_button_create_type1("APPLY", 192);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, UA_WIN_SIZE_WID/2-4-192, UA_WIN_SIZE_HEI - 70);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	g_ok_btn_obj = obj;

	obj = nftool_normal_button_create_type1("AUTHENTICATE LATER", 260);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, UA_WIN_SIZE_WID/2+4, UA_WIN_SIZE_HEI - 70);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	uxm_reg_imsg_event(main_wnd, INFY_EMAIL_VERIFICATION_RESULT);
	uxm_reg_imsg_event(main_wnd, INFY_USRDB_CHANGE_NOTIFY);

	nfui_set_key_focus(obj, TRUE);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);

	gtk_main();

	return g_ret_val;
}

gint VW_Authen_Info_Setup_close()
{
	if (!g_curwnd) return -1;

    evt_send_to_window("AUTHEN INFO SETUP", WND_CLOSE, 0, 0, 0);

    return 0;
}
