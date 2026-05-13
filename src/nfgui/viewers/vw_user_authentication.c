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
#include "viewers/objects/nfcombobox.h"

#include "vw_vkeyboard.h"
#include "vw_vkeyboard_strnum.h"

#include "vw_change_account_popup.h"
#include "vw_user_authentication.h"
#include "vw_init_userinfo.h"
#include "vw_sys_user_add.h"

#include "scm.h"
#include "ix_mem.h"
#include "nf_util_sms.h"
#include "nf_util_mail.h"


#define UA_WIN_SIZE_WID				(980)
#define UA_WIN_SIZE_HEI				(450)
#ifdef VIDECON
	#define UA_WIN_SIZE_HEI				(750)
#endif
#define UA_WIN_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH-UA_WIN_SIZE_WID)/2)
#define UA_WIN_POS_Y 				(guint)((DISPLAY_ACTIVE_HEIGHT-UA_WIN_SIZE_HEI)/2)

#define PP_LABEL_SIZE_WID           (300)
#define PP_LABEL_SIZE_HEI           (40)
#define PP_VALUE_SIZE_WID           (300)
#define QUE_SIZE_WID		        (480)
#define ANS_SIZE_WID		        (260)
#define DO_TITLE_MARGIN             (20)

#define QNA_LABEL_SIZE_WID			(150)

#define AUTHEN_TYPE_QNA				(2)

enum {
	AUTHEN_TYPE_PHONE,
	AUTHEN_TYPE_MAIL,
	AUTHEN_TYPE_MAX
};

enum {
	AUTHEN_MODE_REGISTER,
	AUTHEN_MODE_VERIFICATION,	
	AUTHEN_MODE_MAX
};

typedef enum {
    CHOOSE_ID_MODE = 0,
    DIRECT_INPUT_MODE
};

static mb_type g_retVal = 0;
static NFWINDOW *g_curwnd = 0;

static NFOBJECT *g_winLabel_obj;

static NFOBJECT *g_id_obj;
static NFOBJECT *g_id_obj2;
static NFOBJECT *g_radio_btn_obj[AUTHEN_TYPE_MAX];
static NFOBJECT *g_radio_label_obj[AUTHEN_TYPE_MAX];
static NFOBJECT *g_radio_send_obj[AUTHEN_TYPE_MAX];

static NFOBJECT *g_radio_qna_btn_obj;
static NFOBJECT *g_answer_obj[QNA_COUNT];
static NFOBJECT *g_question_obj[QNA_COUNT];

static NFOBJECT *g_veriLabel_obj;
static NFOBJECT *g_veriValue_obj;
static NFOBJECT *g_timeexp_obj;

static NFOBJECT *g_ok_btn_obj;
static NFOBJECT *g_cancel_btn_obj;

static NFOBJECT *wait_pop = NULL;

static UserManageData g_userData[120];

static gchar g_veriCode[64];
static gint g_radio_idx = 0;
static gint g_auth_mode = 0;
static gint g_user_idx = 0;
static gint user_data_max = 0;

static guint g_exp_timer = 0;

#define MSG_1 	"The password may be changed after completing the verification process through\na registered e-mail."

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

static gint _send_sms_verification_code(gchar *code)
{
	gchar *receiver;
	gint ret_val;

	receiver = nfui_nflabel_get_text((NFLABEL*)g_radio_label_obj[g_radio_idx]);
	ret_val = scm_sned_s1_sms_verification_code(receiver, code);	
	return ret_val;
}

static gint _display_sms_error_code(gint err_code)
{
	gint retVal = -1;

	g_message("%s, %d, error_code:%d", __FUNCTION__, __LINE__, err_code);

	switch (err_code)
	{
		case NF_SMS_OK:
			nftool_mbox(g_curwnd, "NOTICE", "Sending a verification code succeeded.", NFTOOL_MB_OK);
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

	wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

	from = nfui_nflabel_get_text((NFLABEL*)g_radio_label_obj[g_radio_idx]);
	ret_val = scm_send_mail_verification_code(from, from, code);
	return ret_val;
}

static gint _display_mail_error_code(gint err_code)
{
	gint retVal = -1;

  	if(wait_pop)
  	{
  		nftool_remove_waitbox(wait_pop);
  		wait_pop = NULL;
  	}

	if (err_code == NF_MAIL_SEND_STATUS_SUCCESS)
	{
		nftool_mbox(g_curwnd, "NOTICE", "Sending a verification code succeeded.", NFTOOL_MB_OK);
		retVal = 0;
	}
	else if (err_code == NF_MAIL_SEND_STATUS_FAILED)
	{
		nftool_mbox(g_curwnd, "WARNING", "Sending a verification code failed.", NFTOOL_MB_OK);
		retVal = -1;
	}

	return retVal;
}

static gboolean post_radio_authen_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
	{
		gint index, i;

		index = nfui_radio_button_get_index((NFBUTTON*)obj);

		if (index == AUTHEN_TYPE_PHONE)
		{ 
			nfui_nfobject_enable(g_radio_label_obj[AUTHEN_TYPE_PHONE]);
			nfui_nfobject_disable(g_radio_label_obj[AUTHEN_TYPE_MAIL]);
			if(ivsc.dfunc.support_qna) {
				for(i = 0; i < QNA_COUNT; i++) {	
					nfui_nfobject_disable(g_question_obj[i]);
					nfui_nfobject_disable(g_answer_obj[i]);
				}
			}
			
			if (g_auth_mode == AUTHEN_MODE_VERIFICATION)
			{
				nfui_nfobject_enable(g_radio_send_obj[AUTHEN_TYPE_PHONE]);
				nfui_nfobject_disable(g_radio_send_obj[AUTHEN_TYPE_MAIL]);
			}
		}
		else if (index == AUTHEN_TYPE_MAIL)
		{
			nfui_nfobject_enable(g_id_obj);
			nfui_nfobject_enable(g_radio_label_obj[AUTHEN_TYPE_MAIL]);
			nfui_nfobject_disable(g_radio_label_obj[AUTHEN_TYPE_PHONE]);
			if(ivsc.dfunc.support_qna) {
				nfui_nfobject_disable(g_id_obj2);
				for(i = 0; i < QNA_COUNT; i++) {
					nfui_nfobject_disable(g_question_obj[i]);
					nfui_nfobject_disable(g_answer_obj[i]);
				}
			}

			if (g_auth_mode == AUTHEN_MODE_VERIFICATION)
			{
				nfui_nfobject_enable(g_radio_send_obj[AUTHEN_TYPE_MAIL]);
				nfui_nfobject_disable(g_radio_send_obj[AUTHEN_TYPE_PHONE]);
			}
		}
		else if (index == AUTHEN_TYPE_QNA)
		{
			nfui_nfobject_disable(g_id_obj);
			nfui_nfobject_disable(g_radio_label_obj[AUTHEN_TYPE_PHONE]);
			nfui_nfobject_disable(g_radio_label_obj[AUTHEN_TYPE_MAIL]);
			nfui_nfobject_enable(g_id_obj2);
			for(i = 0; i < QNA_COUNT; i++) {
				nfui_nfobject_enable(g_question_obj[i]);
				nfui_nfobject_enable(g_answer_obj[i]);
			}
			if (g_auth_mode == AUTHEN_MODE_VERIFICATION)
			{
				nfui_nfobject_disable(g_radio_send_obj[AUTHEN_TYPE_PHONE]);
				nfui_nfobject_disable(g_radio_send_obj[AUTHEN_TYPE_MAIL]);
			}
		}

		nfui_signal_emit(g_radio_label_obj[AUTHEN_TYPE_PHONE], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_radio_label_obj[AUTHEN_TYPE_MAIL], GDK_EXPOSE, TRUE);		
		if(ivsc.dfunc.support_qna) {
			nfui_signal_emit(g_id_obj, GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_id_obj2, GDK_EXPOSE, TRUE);
			for(i = 0; i < QNA_COUNT; i++) {
				nfui_signal_emit(g_question_obj[i], GDK_EXPOSE, TRUE);
				nfui_signal_emit(g_answer_obj[i], GDK_EXPOSE, TRUE);
			}
		}	

		if (g_auth_mode == AUTHEN_MODE_VERIFICATION)
		{
			nfui_signal_emit(g_radio_send_obj[AUTHEN_TYPE_PHONE], GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_radio_send_obj[AUTHEN_TYPE_MAIL], GDK_EXPOSE, TRUE);
		}	

		g_radio_idx = index;
		g_message("%s, %d radio_idx : %d", __FUNCTION__, __LINE__, g_radio_idx);
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

static gboolean post_input_id_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	gchar *id;
	gint i, j;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *strTemp;
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

		id = nfui_nflabel_get_text(obj);

		strTemp = VirtualKey_Open(g_curwnd, id, x, y, 64, VKEY_ID);

		if(strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);

			for(i = 0; i < user_data_max; i++){
				if(!strcmp(g_userData[i].id, strTemp)) break;
			}

			for(j = 0; j < QNA_COUNT; j++){
				nfui_combobox_set_index_no_expose((NFCOMBOBOX*)g_question_obj[j], g_userData[i].question[j]);
			}

			ifree(strTemp);
			strTemp = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		for(i = 0; i < QNA_COUNT; i++){
			nfui_signal_emit(g_question_obj[i], GDK_EXPOSE, TRUE);
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

static int cnt_sec = 0;
static gboolean _exp_update_proc(gpointer data)
{
	char buf[64];
	if (g_exp_timer == 0) return FALSE;

	--cnt_sec;
	if (cnt_sec == -1) {
		nfui_nfobject_disable(g_ok_btn_obj);
		nfui_signal_emit(g_ok_btn_obj, GDK_EXPOSE, TRUE);

		nfui_nflabel_erase(g_timeexp_obj);
		nfui_nfobject_hide(g_timeexp_obj);
		nfui_signal_emit(g_timeexp_obj, GDK_EXPOSE, TRUE);

		g_exp_timer = 0;
		return FALSE;
	}

	sprintf(buf, "%d : %d", cnt_sec/60, cnt_sec%60);
	nfui_nflabel_erase(g_timeexp_obj);
	nfui_nflabel_set_text((NFLABEL*)g_timeexp_obj, buf);
	nfui_signal_emit(g_timeexp_obj, GDK_EXPOSE, TRUE);
	
	return TRUE;
}

static gboolean post_send_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		gchar *autherStr;	

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		autherStr = nfui_nflabel_get_text((NFLABEL*)g_radio_label_obj[g_radio_idx]);

		if (strlen(autherStr) == 0) return FALSE;

		memset(g_veriCode, 0x00, sizeof(g_veriCode));
		_generate_verification_code(g_veriCode);

		if (g_radio_idx == AUTHEN_TYPE_PHONE)
		{
			gint err_code;
			
			err_code = _send_sms_verification_code(g_veriCode);

			if (_display_sms_error_code(err_code) == -1) return FALSE;

    		nfui_nfobject_show(g_veriLabel_obj);
    		nfui_nfobject_show(g_veriValue_obj);
    		nfui_nfobject_show(g_timeexp_obj);

    		nfui_nfobject_disable(g_radio_send_obj[g_radio_idx]);
    		nfui_nfobject_enable(g_ok_btn_obj);

    		nfui_signal_emit(g_veriLabel_obj, GDK_EXPOSE, TRUE);
    		nfui_signal_emit(g_veriValue_obj, GDK_EXPOSE, TRUE);
    		nfui_signal_emit(g_timeexp_obj, GDK_EXPOSE, TRUE);
    		nfui_signal_emit(g_radio_send_obj[g_radio_idx], GDK_EXPOSE, TRUE);
    		nfui_signal_emit(g_ok_btn_obj, GDK_EXPOSE, TRUE);		

    		nfui_make_key_hierarchy(g_curwnd);		

    		cnt_sec = 600;		// 10 min
    		g_exp_timer = g_timeout_add(1000, _exp_update_proc, 0);
    	}
		else if (g_radio_idx == AUTHEN_TYPE_MAIL)
		{
			if (_send_mail_verification_code(g_veriCode) == -1)
			{
				g_message("%s, %d", __FUNCTION__, __LINE__);
				_display_mail_error_code(NF_MAIL_SEND_STATUS_FAILED);
			}
		}
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

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gchar *autherStr;	
		gchar *id;
		gchar *email = 0;
		gchar new_password[64];	
		gchar answer_tmp[QNA_COUNT][65];
		guint opt = 0;
		gint isExist = 0;
		gint i;
    	gint j;

		GTimeVal last_temp;
		mb_type retType;
		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (g_auth_mode == AUTHEN_MODE_REGISTER)
		{
			if(g_radio_idx == AUTHEN_TYPE_QNA)
				id = nfui_nflabel_get_text((NFLABEL*)g_id_obj2);
			else
				id = nfui_nflabel_get_text((NFLABEL*)g_id_obj);
			
			for(i = 0; i < user_data_max; i++){
				if(strcmp(g_userData[i].id, id) == 0) {
					g_user_idx = i;
					break;
				}
			}			

			if (i == user_data_max)
			{
				nftool_mbox(g_curwnd, "WARNING", "User authentication failed.", NFTOOL_MB_OK);				
				return FALSE;
			}
			// g_message("%s, %d, autherStr:%s", __FUNCTION__, __LINE__, autherStr);

			if (g_radio_idx == AUTHEN_TYPE_PHONE)
			{
				autherStr = nfui_nflabel_get_text((NFLABEL*)g_radio_label_obj[g_radio_idx]);

				if (strlen(g_userData[g_user_idx].phone) == 0) 
				{
					nftool_mbox(g_curwnd, "WARNING", "User authentication failed.", NFTOOL_MB_OK);	
					return FALSE;
				}

				if (strcmp(g_userData[g_user_idx].phone, autherStr) != 0) 
				{
					nftool_mbox(g_curwnd, "WARNING", "User authentication failed.", NFTOOL_MB_OK);	
					return FALSE;
				}
			}
			else if (g_radio_idx == AUTHEN_TYPE_MAIL)
			{
				autherStr = nfui_nflabel_get_text((NFLABEL*)g_radio_label_obj[g_radio_idx]);

				if (nf_mail_send_check_email(autherStr) == FALSE)
				{
					nftool_mbox(g_curwnd, "WARNING", "Please check the E-mail.", NFTOOL_MB_OK);
					return FALSE;
				}

				if (strlen(g_userData[g_user_idx].email) == 0 || strlen(id) == 0) 
				{
					nftool_mbox(g_curwnd, "WARNING", "User authentication failed.", NFTOOL_MB_OK);				
					return FALSE;
				}
			
				if (strlen(g_userData[g_user_idx].email) && strcmp(g_userData[g_user_idx].email, autherStr) != 0) 
				{
					nftool_mbox(g_curwnd, "WARNING", "User authentication failed.", NFTOOL_MB_OK);				
					return FALSE;
				}
			}
			else if (g_radio_idx == AUTHEN_TYPE_QNA)
			{
				if(!strlen(nfui_nflabel_get_text((NFLABEL*)g_id_obj2))){
					nftool_mbox(g_curwnd, "WARNING", "Please input user ID.", NFTOOL_MB_OK);
					return FALSE;
				}

				if (nfui_combobox_get_cur_index(g_question_obj[0]) == nfui_combobox_get_cur_index(g_question_obj[1])
					|| nfui_combobox_get_cur_index(g_question_obj[1]) == nfui_combobox_get_cur_index(g_question_obj[2])
					|| nfui_combobox_get_cur_index(g_question_obj[2]) == nfui_combobox_get_cur_index(g_question_obj[0])) 
				{
					nftool_mbox(g_curwnd, "WARNING", "Duplicate question. Please select another question.", NFTOOL_MB_OK);
					return FALSE;
				}

				if (!strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[0]))
				|| !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[1]))
				|| !strlen(nfui_nflabel_get_text((NFLABEL*)g_answer_obj[2])))
				{
					nftool_mbox(g_curwnd, "WARNING", "Please input answers for each question.", NFTOOL_MB_OK);
					return FALSE;
				}

				for (i = 0; i < QNA_COUNT; i++) {
					memset(&answer_tmp[i], 0x00, sizeof(65));
					for (j = 0; j < QNA_COUNT; j++) {
						if (g_userData[g_user_idx].question[i] == nfui_combobox_get_cur_index(g_question_obj[j])) {
							g_stpcpy(answer_tmp[i], nfui_nflabel_get_text((NFLABEL*)g_answer_obj[j]));
							break;
						}
					}
				}

				for (i = 0; i < QNA_COUNT; i++) {
					if (!strlen(answer_tmp[i])) {
						nftool_mbox(g_curwnd, "WARNING", "Selected question does not match the registered question.", NFTOOL_MB_OK);
						return FALSE;
					} 
				}

				for (i = 0; i < QNA_COUNT; i++) {
					if (strcmp(g_userData[g_user_idx].answer[i], strlwr(answer_tmp[i])) != 0) {
						nftool_mbox(g_curwnd, "WARNING", "Answer does not match the registered answer.", NFTOOL_MB_OK);
						return FALSE;
					}
				}

				if (DAL_get_enahnced_password())
				{
					opt |= (1 << OPT_ENAHNCED_PASSWORD);	
					retType = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, g_userData[g_user_idx].id, NULL, new_password, NFUI_MAX_ENHANCED_PW_SIZE, 0);
				}
				else
				{
					retType = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, g_userData[g_user_idx].id, NULL, new_password, NFUI_MAX_ENHANCED_PW_SIZE, 0);
				}

				if (retType == 1) 
				{
					g_stpcpy(g_userData[g_user_idx].pw, new_password);
					g_get_current_time(&last_temp);			
					g_userData[g_user_idx].pw_last_changed = last_temp.tv_sec;

					DAL_set_userManage_data(g_userData[g_user_idx], g_user_idx);
					DAL_save_setup_db(NFSETUP_WINDOW_USER);

					g_retVal = 1;
				}
				
				topwin = nfui_nfobject_get_top(obj);			
					nfui_nfobject_destroy(topwin);
			}

			if (g_radio_idx != AUTHEN_TYPE_QNA) {
				for (i = 0; i < AUTHEN_TYPE_MAX; i++)
				{
					if (i == AUTHEN_TYPE_PHONE) continue;
					
					nfui_nfobject_disable(g_radio_btn_obj[i]);
					nfui_nfobject_disable(g_id_obj);
					nfui_nfobject_disable(g_radio_label_obj[i]);		
					nfui_nfobject_show(g_radio_send_obj[i]);

					nfui_signal_emit(g_id_obj, GDK_EXPOSE, TRUE);
					nfui_signal_emit(g_radio_btn_obj[i], GDK_EXPOSE, TRUE);
					nfui_signal_emit(g_radio_label_obj[i], GDK_EXPOSE, TRUE);			
					nfui_signal_emit(g_radio_send_obj[i], GDK_EXPOSE, TRUE);	
				}	
				if(ivsc.dfunc.support_qna) {
					nfui_nfobject_disable(g_radio_qna_btn_obj);
					nfui_signal_emit(g_radio_qna_btn_obj, GDK_EXPOSE, TRUE);
				}

				nfui_nfobject_enable(g_radio_send_obj[g_radio_idx]);
				nfui_signal_emit(g_radio_send_obj[g_radio_idx], GDK_EXPOSE, TRUE);

				nfui_nfobject_disable(g_ok_btn_obj);
				nfui_signal_emit(g_ok_btn_obj, GDK_EXPOSE, TRUE);	

				nfui_nflabel_set_text((NFLABEL*)g_winLabel_obj, "FIND PASSWORD");
				nfui_signal_emit(g_winLabel_obj, GDK_EXPOSE, TRUE);
				
				g_auth_mode = AUTHEN_MODE_VERIFICATION;
			}
		}
		else if (g_auth_mode == AUTHEN_MODE_VERIFICATION)
		{
			memset(new_password, 0x00, sizeof(new_password));
			autherStr = nfui_nflabel_get_text((NFLABEL*)g_veriValue_obj);

			if (strcmp(g_veriCode, autherStr) != 0) 
			{
				nftool_mbox(g_curwnd, "WARNING", "Verification code is incorrect.", NFTOOL_MB_OK);				
				return FALSE;
			}

			if (g_exp_timer) {
				g_source_remove(g_exp_timer);
				nfui_nfobject_hide(g_timeexp_obj);
				nfui_signal_emit(g_timeexp_obj, GDK_EXPOSE, TRUE);
				g_exp_timer = 0;
			}
	
//			opt |= (1 << OPT_FORCE_CHANGE_PASSWORD); 

			if (DAL_get_enahnced_password())
			{
				opt |= (1 << OPT_ENAHNCED_PASSWORD);				
				retType = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, g_userData[g_user_idx].id, NULL, new_password, NFUI_MAX_ENHANCED_PW_SIZE, 0);
			}
			else
			{
				retType = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, g_userData[g_user_idx].id, NULL, new_password, NFUI_MAX_PW_SIZE, 0);
			}

			if (retType == 1) 
			{
				g_stpcpy(g_userData[g_user_idx].pw, new_password);
				g_get_current_time(&last_temp);			
				g_userData[g_user_idx].pw_last_changed = last_temp.tv_sec;

				DAL_set_userManage_data(g_userData[g_user_idx], g_user_idx);
				DAL_save_setup_db(NFSETUP_WINDOW_USER);

				g_retVal = 1;
			}
			
			topwin = nfui_nfobject_get_top(obj);			
			nfui_nfobject_destroy(topwin);
		}
	}
	
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_source_remove(g_exp_timer);
		g_exp_timer = 0;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}
	
	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == INFY_EMAIL_VERIFICATION_RESULT)
	{
		gint err_code = ((CMM_MESSAGE_T *)data)->param;
	
		if (_display_mail_error_code(err_code) == -1) return FALSE;
		
		nfui_nfobject_show(g_veriLabel_obj);
		nfui_nfobject_show(g_veriValue_obj);
		nfui_nfobject_show(g_timeexp_obj);

		nfui_nfobject_disable(g_radio_send_obj[g_radio_idx]);
		nfui_nfobject_enable(g_ok_btn_obj);

		nfui_signal_emit(g_veriLabel_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_veriValue_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_timeexp_obj, GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_radio_send_obj[g_radio_idx], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_ok_btn_obj, GDK_EXPOSE, TRUE);		

		nfui_make_key_hierarchy(g_curwnd);		

		cnt_sec = 600;		// 10 min
		g_exp_timer = g_timeout_add(1000, _exp_update_proc, 0);	
	}
	else if (evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, INFY_EMAIL_VERIFICATION_RESULT);

		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

mb_type VW_Open_User_Authen_Popup(NFWINDOW *parent, gint usr_idx, gint g_id_input_mode)
{
	g_retVal = 0;

	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *obj;
	guint pos_x, pos_y;
	guint size_w, size_h;	
	gint i;

	const gchar *strTitle[AUTHEN_TYPE_MAX] = {"MOBILE PHONE / TELEPHONE", "E-MAIL"};
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];
	GSList *slist = NULL;

	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	memset(&g_userData, 0x00, sizeof(UserManageData));
	memset(g_veriCode, 0x00, sizeof(g_veriCode));

	g_radio_idx = AUTHEN_TYPE_PHONE;
	g_auth_mode = AUTHEN_MODE_REGISTER;
	g_user_idx = usr_idx;
	user_data_max = DAL_get_user_count();

	for(i = 0; i < user_data_max; i++){
		DAL_get_userManage_data(&g_userData[i], i);
	}

	main_wnd = nftool_create_popup_window(parent, UA_WIN_POS_X, UA_WIN_POS_Y, UA_WIN_SIZE_WID, UA_WIN_SIZE_HEI, "", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
//	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);

	pos_x = 20;
	pos_y = 64;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(MSG_1, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_spacing((NFLABEL*)obj,  SEMI_CONDENSED_SPACING);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, UA_WIN_SIZE_WID-20, PP_LABEL_SIZE_HEI*2);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 100;

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER AUTHENTICATION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, UA_WIN_SIZE_WID-40, 36);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	g_winLabel_obj = obj;

	if(ivsc.dfunc.support_qna) {
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, "E-MAIL ADDRESS SETUP");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, DO_TITLE_MARGIN);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

		pos_y += 60;
	}

// USER ID
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 4);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);	

	if(g_id_input_mode == DIRECT_INPUT_MODE){
		obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
		nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_regi_post_event_callback(obj, post_input_id_event_cb);
	} else {
	obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_userData[g_user_idx].id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
	}
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40+PP_LABEL_SIZE_WID+4, pos_y);
	g_id_obj = obj;

	pos_y += 42;

	for (i = 0; i < AUTHEN_TYPE_MAX; i++)
	{
// RADIO BUTTON	
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, size_h);
		if (ivsc.dfunc.support_qna) {
			nfui_nfobject_show(obj);
		}
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y-42);			
		nfui_regi_post_event_callback(obj, post_radio_authen_event_cb);
		g_radio_btn_obj[i] = obj;
	
		if (i == AUTHEN_TYPE_PHONE)
		{		
			slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			nfui_nfobject_hide(obj);
		}
		else if (i == AUTHEN_TYPE_MAIL)
		{
			nfui_radio_button_add_group(NF_BUTTON(obj), slist);
		}

// TITLE LABEL 
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 4);
		nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
	
		if (i == AUTHEN_TYPE_PHONE)
		{		
			nfui_nfobject_hide(obj);
		}

// VALUE LABEL
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
		nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nfobject_disable(obj);		
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40+PP_LABEL_SIZE_WID+4, pos_y);
		g_radio_label_obj[i] = obj;

		if (i == AUTHEN_TYPE_PHONE)
		{		
			nfui_regi_post_event_callback(obj, post_input_phone_event_cb);
			nfui_nfobject_hide(obj);
		}
		else if (i == AUTHEN_TYPE_MAIL)
		{
			nfui_regi_post_event_callback(obj, post_input_email_event_cb);
		}

// SEND BTN
		obj = nftool_normal_button_create_type3("SEND VERIFICATION CODE", 290);
		nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
		nfui_nfbutton_set_spacing(NF_BUTTON(obj), SEMI_CONDENSED_SPACING);	
		nfui_nfobject_disable((NFLABEL*)obj);		
		nfui_nfobject_hide(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40+PP_LABEL_SIZE_WID+4+PP_VALUE_SIZE_WID+4, pos_y);
		nfui_regi_post_event_callback(obj, post_send_button_event_handler);
		g_radio_send_obj[i] = obj;
		
		if (i == AUTHEN_TYPE_MAIL) pos_y += 42;
	}

	if (strlen(g_userData[g_user_idx].email))
	{
		nfui_nfobject_enable(g_radio_btn_obj[AUTHEN_TYPE_MAIL]);
		nfui_nfobject_enable(g_radio_label_obj[AUTHEN_TYPE_MAIL]);
		nfui_radio_button_set_toggled((NFBUTTON*)g_radio_btn_obj[AUTHEN_TYPE_MAIL], TRUE);

		g_radio_idx = 1;
	}
/*
	if (strlen(g_userData.phone))
	{
		nfui_nfobject_disable(g_radio_label_obj[AUTHEN_TYPE_MAIL]);
	
		nfui_nfobject_enable(g_radio_btn_obj[AUTHEN_TYPE_PHONE]);
		nfui_nfobject_enable(g_radio_label_obj[AUTHEN_TYPE_PHONE]);
		nfui_radio_button_set_toggled((NFBUTTON*)g_radio_btn_obj[AUTHEN_TYPE_PHONE], TRUE);
		
		g_radio_idx = 0;
	}
*/

	pos_y += 10;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INPUT VERIFICATION CODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 4);
	nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);	
	g_veriLabel_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40+PP_LABEL_SIZE_WID+4, pos_y);
	nfui_regi_post_event_callback(obj, post_veri_value_event_cb);	
	g_veriValue_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nfobject_hide(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40+PP_LABEL_SIZE_WID+4+PP_VALUE_SIZE_WID+4, pos_y);
	g_timeexp_obj = obj;

	if (ivsc.dfunc.support_qna) {
		pos_y += 60;

		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, "SET SECURITY QUESTIONS");
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, DO_TITLE_MARGIN);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

		pos_y += 60;

		// RADIO BUTTON	
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(40-size_h)/2);			
		nfui_regi_post_event_callback(obj, post_radio_authen_event_cb);
		g_radio_qna_btn_obj = obj;

		nfui_radio_button_add_group(NF_BUTTON(obj), slist);

		// pos_x = pos_x + (guint)size_w;

		// USER ID2
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 4);
		nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);	

		if(g_id_input_mode == DIRECT_INPUT_MODE){
			obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
			nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_regi_post_event_callback(obj, post_input_id_event_cb);
		} else {
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_userData[g_user_idx].id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
			nfui_nfobject_set_size(obj, PP_VALUE_SIZE_WID, PP_LABEL_SIZE_HEI);
		}
		nfui_nfobject_show(obj);
		nfui_nfobject_disable(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40+PP_LABEL_SIZE_WID+4, pos_y);
		g_id_obj2 = obj;

		pos_y += 60;

		// QNA
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("QUESTIONS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 4);
		nfui_nfobject_set_size(obj, QNA_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

		for(i = 0; i < QNA_COUNT; i++) {
			obj = nfui_combobox_new(QUESTIONS, 11, 0);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			if(g_id_input_mode == CHOOSE_ID_MODE){
				nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), g_userData[g_user_idx].question[i]);
			}
			nfui_nfobject_set_size(obj, QUE_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_show(obj);
			nfui_nfobject_disable(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+QNA_LABEL_SIZE_WID+8, pos_y);
			g_question_obj[i] = obj;

			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
			nfui_nfobject_set_size(obj, ANS_SIZE_WID, PP_LABEL_SIZE_HEI);
			nfui_nfobject_show(obj);
			nfui_nfobject_disable(obj);
			nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+QNA_LABEL_SIZE_WID+8+QUE_SIZE_WID+10, pos_y);
			nfui_regi_post_event_callback(obj, post_input_answer_event);
			g_answer_obj[i] = obj;

			pos_y += 44;
		}
	}

	obj = nftool_normal_button_create_type1("APPLY", 168);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, UA_WIN_SIZE_WID/2-4-168, UA_WIN_SIZE_HEI - 70);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);
	g_ok_btn_obj = obj;

	if ((strlen(g_userData[g_user_idx].email) == 0) && (strlen(g_userData[g_user_idx].phone) == 0) && (strlen(g_userData[g_user_idx].answer[0]) == 0))
		nfui_nfobject_disable(g_ok_btn_obj);
	
	obj = nftool_normal_button_create_type1("CANCEL", 168);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, UA_WIN_SIZE_WID/2+4, UA_WIN_SIZE_HEI - 70);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	g_cancel_btn_obj = obj;

	uxm_reg_imsg_event(main_wnd, INFY_EMAIL_VERIFICATION_RESULT);

	nfui_set_key_focus(obj, TRUE);

	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);

	gtk_main();

	return g_retVal;
}

