#include "nf_afx.h"

#include "ssm.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcheckbutton.h"

#include "vw_sys_user_main.h"
#include "vw_sys_user_manage.h"
#include "vw_sys_user_add.h"
#include "vw_pin_edit_popup.h"
#include "vw_question_edit_popup.h"
#include "vw_init_userinfo.h"

#include "vw_change_account_popup.h"
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "vw_vkeyboard_strnum.h"
#include "ix_mem.h"
#include "nf_util_mail.h"
#include "ssm.h"
#include "vw_desc.h"

#define MAX_ID_STRING_SIZE			(10)
#define MAX_EMAIL_STRING_SIZE		(63)

enum {
	UA_ROW_ID = 0,
	UA_ROW_PASSWORD,
	UA_ROW_GROUP,
	UA_ROW_EMAIL,
	UA_ROW_EMAIL_NOTI,
#if defined(_SUPPORT_USER_PHONE)	
	UA_ROW_PHONE,
#if !defined(_SUPPORT_S1_STEP1)
	UA_ROW_PHONE_NOTI,
#endif
#endif
#if defined(_SUPPORT_DUAL_SMTPSERVER)
	UA_ROW_SELECT_SERVER,
#endif
	UA_ROW_AUTH_POPUP_HIDE,
	UA_ROW_MAX
};

enum {
	UA_CHECK_ALL = 0,
	UA_CHECK_ROW1,
#if defined (GUI_8CH_SUPPORT) || defined (GUI_16CH_SUPPORT) || defined (GUI_32CH_SUPPORT)
	UA_CHECK_ROW2,
#if defined (GUI_16CH_SUPPORT) || defined (GUI_32CH_SUPPORT)
	UA_CHECK_ROW3,
	UA_CHECK_ROW4,
#if defined (GUI_32CH_SUPPORT)
	UA_CHECK_ROW5,
	UA_CHECK_ROW6,
	UA_CHECK_ROW7,
	UA_CHECK_ROW8,
#endif
#endif	
#endif	
	UA_CHECK_ROW_MAX
};

enum {
	UAB_OK = 0,
	UAB_CANCEL,
	UAB_BUTTONS
};

#define PP_LABEL_SIZE_WID			(250)
#define PP_LABEL_SIZE_HEI			(40)
#define PP_VLABEL_SIZE_WID			(190)

#define USER_PP_SIZE_WID			(490)
#define USER_PP_SIZE_HEI			(44 + 8 + 43*UA_ROW_MAX + 44 + 42*UA_CHECK_ROW_MAX + 130)

#define USER_PP_POS_X				(guint)((DISPLAY_ACTIVE_WIDTH- USER_PP_SIZE_WID)/2)
#define USER_PP_POS_Y 				(guint)((DISPLAY_ACTIVE_HEIGHT - USER_PP_SIZE_HEI)/2 - 10)

#define USER_FIXED_SIZE_WID			(450)
#define USER_FIXED_SIZE_HEI			(USER_PP_SIZE_HEI-60)

#define PP_GROUP_STRING_CNT	 	3

static NFOBJECT *value[UA_ROW_MAX];

static guint ret_val = USER_EDIT_RET_CANCEL;
static gboolean g_edit_enable;

static UserManageData *g_user_data;
static gint g_user_idx;
static guint g_user_cnt;
static guint g_popup_mode;

static NFOBJECT *g_pin_check_obj;
static NFOBJECT *g_pin_edit_button;
static NFOBJECT *g_question_edit_button;
static gchar g_pin_number[16];
static gint *g_question[QNA_COUNT];
static gchar *g_answer[QNA_COUNT];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_covert_check_all;
static NFOBJECT *g_covert_check[GUI_CHANNEL_CNT];

static NFOBJECT *g_server_obj[2];
static NFOBJECT *g_auth_hide_obj[2];


/*
static guint _get_email_server_value(NFOBJECT *first, NFOBJECT *second)
{
	guint ret = 0;

	if (nfui_check_button_get_active((NFCHECKBUTTON*)first))
		ret = ret | (1 << 0);		
		
	if (nfui_check_button_get_active((NFCHECKBUTTON*)second))
		ret = ret | (1 << 1);

	return ret;
}

static gint _set_email_server_value(guint value, NFOBJECT *first, NFOBJECT *second)
{
	gboolean active;

	active = (value & (1<<0)) ? 1 : 0;	
	nfui_check_button_set_active((NFCHECKBUTTON*)first, active);

	active = (value & (1<<1)) ? 1 : 0;
	nfui_check_button_set_active((NFCHECKBUTTON*)second, active);

	return 0;
}
*/

static gboolean _is_same_char(gchar *id)
{
    gint same_ch_cnt = 0;
    gint len = 0;

    len = strlen(id);

    while((*id) != NULL)
    {
        if( !((*id) - *(id+1)))
            same_ch_cnt++;
        else
            same_ch_cnt = 0;

        if(same_ch_cnt == (len-1))
            return FALSE;

        id++;
    }

    return TRUE;
}

static gint _check_id_rules(gchar *id)
{
	if(!strncmp(id, SYS_USER_STRING_ADMIN, 5)) {
		nftool_mbox(g_curwnd, "NOTICE", "It's unable to use the account.\nPlease another name.", NFTOOL_MB_OK);
		return 0;
	}

	if (DAL_get_enahnced_id())
	{
    	if (strlen(id) < 5) 
    	{
    	    nftool_mbox(g_curwnd, "NOTICE", "Please input 5 to 10 characters for ID", NFTOOL_MB_OK);
    	    return 0;
    	}
    	else if (!_is_same_char(id))
    	{
    	    nftool_mbox(g_curwnd, "NOTICE", "User ID with the same letters can not be used.", NFTOOL_MB_OK);
    	    return 0;
    	}
	}
	else
	{
	    if (strlen(id) < 1) {
	        nftool_mbox(g_curwnd, "NOTICE", "Please input user ID.", NFTOOL_MB_OK);
	        return 0;
        }
	}

	return 1;
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

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}
	return FALSE;
}

static gboolean post_id_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		NFOBJECT *top;
		gchar *title;
		guint x, y;

		if(g_edit_enable == FALSE)
			return FALSE;

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

		title = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_ID_STRING_SIZE, VKEY_ALPHANUMERIC);

		if(title)
		{
            if (_check_id_rules(title))
            {
                nfui_nflabel_set_text((NFLABEL*)obj, title);
            }

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
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
		gchar *title;
		NFOBJECT *top;
		guint x, y;
		guint opt = 0;
		guint max_ch = 0;
		gchar userid[32];
		gchar userpw[32];

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

/*
        if (g_popup_mode == USER_MODE_EDIT)
		{
    		if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
  	   	}
*/	
		if (DAL_get_enahnced_password()) {
			opt |= (1 << OPT_ENAHNCED_PASSWORD);
			max_ch = NFUI_MAX_ENHANCED_PW_SIZE;
		}
		else {
			max_ch = NFUI_MAX_PW_SIZE;
        }

		memset(userid, 0x00, sizeof(userid));
		memset(userpw, 0x00, sizeof(userpw));
		strcpy(userid, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_ID]));		

		if (VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, userid, NULL, userpw, max_ch, 0))
		{
			if(strcmp(userpw, nfui_nflabel_get_text((NFLABEL*)obj)) == 0) return FALSE;

			nfui_nflabel_set_text(((NFLABEL*)obj), userpw);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}
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
		retVal = vw_pin_edit_popup_open(g_curwnd, pinBuf, strlen(g_pin_number)==0);
		if (retVal) {
			memset(g_pin_number, 0x00, sizeof(g_pin_number));
			strcpy(g_pin_number, pinBuf);
		}
	}

	return FALSE;
}

static gboolean post_question_edit_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint retVal = 0;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
		retVal = vw_question_edit_popup_open(g_curwnd, g_question, g_answer, g_user_data, g_user_idx, g_user_cnt);
	}

	return FALSE;
}

static gboolean post_email_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		gchar *title;
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


		title = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, MAX_EMAIL_STRING_SIZE, VKEY_MAIL);

		if(title)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, title);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_phone_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		gchar *title;
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

		title = NumberKey_Str_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, 16);

		if (title)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, title);

			ifree(title);
			title = NULL;
		}

		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_covert_check_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	gboolean state;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
		for(i = 0; i < GUI_CHANNEL_CNT; i++)
			nfui_check_button_set_active((NFCHECKBUTTON*)g_covert_check[i], state);
	}

	return FALSE;
}

static gboolean post_covert_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int i;
	gint chk_cnt = 0;

	if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		for (i = 0 ; i < GUI_CHANNEL_CNT ; i++) 
		{
			if (nfui_check_button_get_active((NFCHECKBUTTON*)g_covert_check[i]))
				chk_cnt += 1;
		}

		if(chk_cnt == GUI_CHANNEL_CNT)
			nfui_check_button_set_active((NFCHECKBUTTON*)g_covert_check_all, TRUE);
		else
			nfui_check_button_set_active((NFCHECKBUTTON*)g_covert_check_all, FALSE);
	}

	return FALSE;
}

static gboolean post_deletebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
		{
			return FALSE;
		}


		if(g_edit_enable == FALSE)
		{
			nftool_mbox(g_curwnd, "NOTICE", "Account ADMIN can't be deleted.", NFTOOL_MB_OK);
			return FALSE;
		}

		ret_val = USER_EDIT_RET_DELETE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		gchar *email_str;
		gchar *phone_str;		
		gint cmpRet = 0;
		gint i, j; 
	    guint init_pw = 0;	
		struct timeval tv;
		
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}

		topwin = nfui_nfobject_get_top(obj);

		if (g_user_idx >= 0)    //EDIT mode..
		{
			for(i=0; i<g_user_cnt; i++)
			{
				if(i != g_user_idx)
				{
					if(!strcmp(&g_user_data[i].id, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_ID])))
					{
						nftool_mbox(g_curwnd, "NOTICE", "The same user ID already exists.\nPlease input another user ID.", NFTOOL_MB_OK);
						return FALSE;
					}
				}
			}
		}
		else   //ADD mode..
		{
			for(i=0; i<g_user_cnt; i++)
			{
				if(!strcmp(&g_user_data[i].id, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_ID])))
				{
					nftool_mbox(g_curwnd, "NOTICE", "The same user ID already exists.\nPlease input another user ID.", NFTOOL_MB_OK);
					return FALSE;
				}
			}
		}
		
		if(g_user_idx >= 0)
			i = g_user_idx;

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_ID]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input user ID.", NFTOOL_MB_OK);
			return FALSE;
		}

		if(!strcmp(nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_PASSWORD]), ""))
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input a password.", NFTOOL_MB_OK);
			return FALSE;
		}

		if (DAL_get_enahnced_password())
		{
	        guint result;	

            result = ssm_check_enhanced_passwd(nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_ID]), nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_PASSWORD]));

            if (result & PASS_ERROR_INCLUDE_ID)
            {
				nftool_mbox(g_curwnd, "NOTICE", "The corresponding ID cannot be used as part of the password.", NFTOOL_MB_OK);
				return FALSE;
            }
		}

        if (ivsc.dfunc.pin.support) {
		    if (nfui_check_button_get_active((NFCHECKBUTTON*)g_pin_check_obj) && (strlen(g_pin_number) == 0)) {
				nftool_mbox(g_curwnd, "NOTICE", "Please enter pin number.", NFTOOL_MB_OK);
		    	return FALSE;
			}
        }

		email_str = nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_EMAIL]);
		cmpRet = strcmp(email_str, "");

		if (cmpRet != 0)
		{		
			if (nf_mail_send_check_email(email_str) == FALSE)  //for email send
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
				return FALSE;
			}
		}
		else if (cmpRet == 0) 
		{
			if (nfui_spin_button_get_index((NFSPINBUTTON*)value[UA_ROW_EMAIL_NOTI]) == 1)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
				return FALSE;
			}
		}
		else if (!strcmp(&g_user_data[i].email, ""))
		{
			if(nfui_spin_button_get_index((NFSPINBUTTON*)value[UA_ROW_EMAIL_NOTI]) == 1)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
				return FALSE;
			}
		}

#if defined(_SUPPORT_USER_PHONE)
		phone_str = nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_PHONE]);
		cmpRet = strcmp(phone_str, "");


#if !defined(_SUPPORT_S1_STEP1)
		if (cmpRet != 0)
		{		
/*		
			if (nf_mail_send_check_email(email_str) == FALSE)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the E-mail.", NFTOOL_MB_OK);
				return FALSE;
			}
*/			
		}
		else if (cmpRet == 0) 
		{
			if (nfui_spin_button_get_index((NFSPINBUTTON*)value[UA_ROW_PHONE_NOTI]) == 1)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the phone.", NFTOOL_MB_OK);
				return FALSE;
			}
		}
		else if (!strcmp(&g_user_data[i].phone, ""))
		{
			if(nfui_spin_button_get_index((NFSPINBUTTON*)value[UA_ROW_PHONE_NOTI]) == 1)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please check the phone.", NFTOOL_MB_OK);
				return FALSE;
			}
		}
#endif
#endif
		
        if (g_popup_mode == USER_MODE_EDIT) {
    		if (!VW_UserPwd_Open(g_curwnd, "PASSWORD CHECK", -1)) return FALSE;
		}

		strcpy(&g_user_data[i].id, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_ID]));

		if (strcmp(nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_PASSWORD]), g_user_data[i].pw)) 
		{
			gettimeofday(&tv, NULL);
			g_user_data[i].pw_last_changed = tv.tv_sec;
	
			if(g_popup_mode == USER_MODE_ADD)
			{
				g_user_data[i].init_pw_changed = 0;
			}
			
			if(g_popup_mode == USER_MODE_EDIT)
			{
				init_pw = DAL_get_init_pw_changed(i);
				if(init_pw == 0) g_user_data[i].init_pw_changed = tv.tv_sec;
    		}
		}

		strcpy(&g_user_data[i].pw, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_PASSWORD]));

		if(g_edit_enable)
			strcpy(&g_user_data[i].group, nfui_spin_button_get_text((NFSPINBUTTON*)value[UA_ROW_GROUP]));

        if (strcmp(g_user_data[i].email, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_EMAIL])) != 0) {
            g_user_data[i].email_certification = 0;
        }
			
		strcpy(&g_user_data[i].email, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_EMAIL]));
		g_user_data[i].email_noti = nfui_spin_button_get_index((NFSPINBUTTON*)value[UA_ROW_EMAIL_NOTI]);
#if defined(_SUPPORT_USER_PHONE)		
		strcpy(&g_user_data[i].phone, nfui_nflabel_get_text((NFLABEL*)value[UA_ROW_PHONE]));
#if !defined(_SUPPORT_S1_STEP1)
		g_user_data[i].phone_noti = nfui_spin_button_get_index((NFSPINBUTTON*)value[UA_ROW_PHONE_NOTI]);
#endif
#endif

#if defined(_SUPPORT_DUAL_SMTPSERVER)
		for (j = 0; j < 2; j++)
		{
			if (nfui_radio_button_get_toggled((NFBUTTON*)g_server_obj[j])) 
				g_user_data[i].e_serv = j;
		}

		g_message("%s, %d, i:%d, e_serv:%d", __FUNCTION__, __LINE__, i, g_user_data[i].e_serv);
#endif


		if(ivsc.dfunc.support_auth_popup_hide) {

			for (j = 0; j < 2; j++)
			{
				if (nfui_radio_button_get_toggled((NFBUTTON*)g_auth_hide_obj[j])) 
					g_user_data[i].certi_popup_hide = j;
			}
		}

		for(j = 0 ; j < GUI_CHANNEL_CNT ; j++)
		{
			if (nfui_check_button_get_active(g_covert_check[j]))
				g_user_data[i].covert[j] = '1';	
			else
				g_user_data[i].covert[j] = '0';	
		}

        if (ivsc.dfunc.pin.support) {
    		g_user_data[i].use_pin = nfui_check_button_get_active((NFCHECKBUTTON*)g_pin_check_obj);
		    g_stpcpy(g_user_data[i].pin_number, g_pin_number);
        }

		if (ivsc.dfunc.support_qna) {
			for (j = 0; j < QNA_COUNT; j++){
				g_user_data[i].question[j] = *g_question[j];
				strcpy(g_user_data[i].answer[j], strlwr(g_answer[j]));

				g_free(g_question[j]);
				g_free(g_answer[j]);
			}
		}

		ret_val = USER_EDIT_RET_OK;
		nfui_nfobject_destroy(topwin);
	}
	
	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *topwin;
		int i;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
		  return FALSE;
	    }

		if (ivsc.dfunc.support_qna) {
			for (i = 0; i < QNA_COUNT; i++){
				*g_question[i] = g_user_data[g_user_idx].question[i];
				strcpy(g_answer[i], g_user_data[g_user_idx].answer[i]);
			}
		}

		topwin = nfui_nfobject_get_top(obj);

		ret_val = USER_EDIT_RET_CANCEL;
		nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

guint UserAddDlg_Open(NFWINDOW *parent, guint mode, gboolean edit_enable, UserManageData *user_data, gint idx, guint user_count)
{
	NFOBJECT *main_wnd, *main_fixed;
	NFOBJECT *fixed1;
	NFOBJECT *lbTemp;
	NFOBJECT *ua_btns[UAB_BUTTONS];
	NFOBJECT *obj;

	const gchar *strTitle[] = {"USER ID", 
						"PASSWORD", 
						"GROUP", 
						"EMAIL", 
						"EMAIL NOTIFY", 
#if defined(_SUPPORT_USER_PHONE)						
						"MOBILE NO.", 
#if !defined(_SUPPORT_S1_STEP1)						
						"MOBILE NOTIFY",
#endif						
#endif
#if defined(_SUPPORT_DUAL_SMTPSERVER)
						"MAIL SERVER",
#endif						
						"AUTH INFO POPUP",
						};
	const gchar *strGroup[] = {SYS_USER_STRING_ADMIN, SYS_USER_STRING_MANAGER, SYS_USER_STRING_USER};
	const gchar *strOffOn[] = {"OFF", "ON"};
	const gchar *strShow[] = {"ON", "OFF"};

	gchar strBuf[16];

	GSList *slist = NULL;
	GdkPixbuf *radio_img[NFOBJECT_STATE_COUNT];

	gboolean active;
	guint size_w, size_h;	
	guint chk_w, chk_h;
	guint pos_x, pos_y;
	guint i;
	gchar buf[64];

	ret_val = USER_EDIT_RET_CANCEL;

	g_popup_mode = mode;
	g_edit_enable = edit_enable;
	g_user_data = user_data;
	g_user_idx = idx;
	g_user_cnt = user_count;

	if (ivsc.dfunc.support_qna) {
		for (i = 0; i < QNA_COUNT; i++){
			g_question[i] = (gint*)g_malloc(sizeof(int));
			g_answer[i] = (gchar*)g_malloc(sizeof(64+1));
			memset(g_question[i], 0x00, sizeof(gint));
			memset(g_answer[i], 0x00, sizeof(64+1));
		}
	}

	if(mode == USER_MODE_ADD)
		main_wnd = nftool_create_popup_window(parent, USER_PP_POS_X, USER_PP_POS_Y, USER_PP_SIZE_WID, USER_PP_SIZE_HEI, "ADD", TRUE);
	else if(mode == USER_MODE_EDIT)
		main_wnd = nftool_create_popup_window(parent, USER_PP_POS_X, USER_PP_POS_Y, USER_PP_SIZE_WID, USER_PP_SIZE_HEI, "EDIT", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	nfui_nfobject_show(main_wnd);	
	g_curwnd = main_wnd;

	if(main_wnd == NULL)
	{
		g_warning("[GUI DEBUG] USER ADD WINDOW is NULL...");
		return USER_EDIT_RET_CANCEL;
	}

	main_fixed = ((NFWINDOW*)main_wnd)->child;

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed1, USER_FIXED_SIZE_WID, USER_FIXED_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)main_fixed, fixed1, 12, 48);
	nfui_nfobject_show(fixed1);

	pos_x = 8;
	pos_y = 8;

	for (i = 0; i < UA_ROW_MAX; i++)
	{
		if(!ivsc.dfunc.support_auth_popup_hide)
		{	if(i == UA_ROW_AUTH_POPUP_HIDE){
				break;
			}
		}

		lbTemp = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)lbTemp, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(lbTemp, PP_LABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(lbTemp, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(lbTemp);
		nfui_nffixed_put((NFFIXED*)fixed1, lbTemp, pos_x, pos_y);
	
		pos_y += 43;

		if (ivsc.dfunc.pin.support) {
			if (i == 1) pos_y += 43;
		}

		if (ivsc.dfunc.support_qna) {
			if (i == 1) pos_y += 43;
		}
	}

	pos_x += PP_LABEL_SIZE_WID;
	pos_y = 8;

	if (mode == USER_MODE_EDIT)
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_user_data[idx].id, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_regi_post_event_callback(obj, post_id_label_event_handler);	
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);	
	value[UA_ROW_ID] = obj;

	if (g_edit_enable == FALSE)
	{
		nfui_nfobject_use_focus(value[UA_ROW_ID], NFOBJECT_FOCUS_OFF);
		nfui_nfobject_disable(value[UA_ROW_ID]);
	}	

	pos_y += 43;	

	if (mode == USER_MODE_EDIT)
		obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_user_data[idx].pw, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));	
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_regi_post_event_callback(obj, post_pw_label_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);	
	value[UA_ROW_PASSWORD] = obj;

	pos_y += 43;

	if (ivsc.dfunc.pin.support) {
		memset(g_pin_number, 0x00, sizeof(g_pin_number));
		g_stpcpy(g_pin_number, g_user_data[idx].pin_number);

		if (mode == USER_MODE_EDIT) obj = nfui_checkbutton_new(g_user_data[idx].use_pin);
		else obj = nfui_checkbutton_new(TRUE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size(obj, &chk_w, &chk_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, 8, pos_y + (40-chk_h)/2);
		nfui_regi_post_event_callback(obj, post_pin_check_event_handler);
		g_pin_check_obj = obj;

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USE PIN LOGIN", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, 14+chk_w, pos_y);
	
		obj = nftool_normal_button_create_popup_type2("EDIT", 190);
		nfui_nfobject_show(obj);
		if (mode == USER_MODE_EDIT) {
			if (!g_user_data[idx].use_pin) nfui_nfobject_disable(obj);
		}
		nfui_nffixed_put((NFFIXED*)fixed1, obj, 260, pos_y+2);
		nfui_regi_post_event_callback(obj, post_pin_edit_button_event_handler);
		g_pin_edit_button = obj;

		pos_y += 43;
	}

	if (ivsc.dfunc.support_qna) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("QUESTION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, PP_LABEL_SIZE_WID, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, 8, pos_y);
	
		obj = nftool_normal_button_create_popup_type2("EDIT", 190);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, 260, pos_y+2);
		nfui_regi_post_event_callback(obj, post_question_edit_button_event_handler);
		g_question_edit_button = obj;

		pos_y += 43;
	}

	if (g_edit_enable)
	{
		obj = (NFOBJECT*)nfui_spinbutton_new(strGroup, PP_GROUP_STRING_CNT, 0);
		if (mode == USER_MODE_EDIT) nfui_spin_button_set_text_no_expose((NFSPINBUTTON*)obj, g_user_data[idx].group);		
		nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
		nfui_nfobject_show(obj);			
		value[UA_ROW_GROUP] = obj;
	}
	else
	{
		if (mode == USER_MODE_EDIT)	
			obj = (NFOBJECT*)nfui_nflabel_new_text_box(g_user_data[idx].group, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		else
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));		
		nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nfobject_support_multi_lang(obj, FALSE);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_disable(obj);
		nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
		nfui_nfobject_show(obj);			
		value[UA_ROW_GROUP] = obj;
	}

	memset(buf, 0x00, sizeof(buf));
	ssm_get_cur_group(buf);
	
	if ((mode == USER_MODE_EDIT) && strcmp(buf, "ADMIN") != 0) {
	    nfui_nfobject_disable(obj);
	}

	pos_y += 43;

	obj = nfui_nflabel_new_text_box(g_user_data[idx].email, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_spacing((NFLABEL *)obj, CONDENSED_SPACING);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_regi_post_event_callback(obj, post_email_label_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);				
	value[UA_ROW_EMAIL] = obj;

	pos_y += 43;

	if (mode == USER_MODE_EDIT)
		obj = nfui_spinbutton_new(strOffOn, 2, g_user_data[idx].email_noti);
	else
		obj = nfui_spinbutton_new(strOffOn, 2, 0);	
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);				
	value[UA_ROW_EMAIL_NOTI] = obj;

	pos_y += 43;

#if defined(_SUPPORT_USER_PHONE)
	if (mode == USER_MODE_EDIT)
		obj = nfui_nflabel_new_text_box(g_user_data[idx].phone, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	else
		obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_spacing((NFLABEL *)obj, CONDENSED_SPACING);
	nfui_nfobject_support_multi_lang(obj, FALSE);
	nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_regi_post_event_callback(obj, post_phone_label_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);				
	value[UA_ROW_PHONE] = obj;

	pos_y += 43;

#if !defined(_SUPPORT_S1_STEP1)
	if (mode == USER_MODE_EDIT)
		obj = nfui_spinbutton_new(strOffOn, 2, g_user_data[idx].phone_noti);
	else
		obj = nfui_spinbutton_new(strOffOn, 2, 0);
	nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, PP_VLABEL_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);					
	value[UA_ROW_PHONE_NOTI] = obj;

	pos_y += 43;
#endif
#endif

#if defined(_SUPPORT_DUAL_SMTPSERVER)
	radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
	radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
	radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
	radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

	pos_x = (8 + PP_LABEL_SIZE_WID);

	nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

	for (i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
		nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y + (40-size_h)/2);
		g_server_obj[i] = obj;
		
		if (i == g_user_data[idx].e_serv)
			nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

		if (i == 0) 	slist = nfui_radio_button_get_group(NF_BUTTON(obj));
		else nfui_radio_button_add_group(NF_BUTTON(obj), slist);

		pos_x += 40;

		g_sprintf(strBuf, "%d", i+1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 60, PP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

		pos_x += 60;
	}

	pos_y += 43;
#endif
		
	if(ivsc.dfunc.support_auth_popup_hide)
	{
		radio_img[0] = nfui_get_image_from_file((IMG_N_POPUP_RADIO_OFF), NULL);
		radio_img[1] = nfui_get_image_from_file((IMG_O_POPUP_RADIO_ON), NULL);
		radio_img[2] = nfui_get_image_from_file((IMG_P_POPUP_RADIO_ON), NULL);
		radio_img[3] = nfui_get_image_from_file((IMG_D_POPUP_RADIO_OFF), NULL);

		pos_x = (8 + PP_LABEL_SIZE_WID);

		nfui_get_pixbuf_size(radio_img[0], &size_w, &size_h);

		for (i = 0; i < 2; i++)
		{
			obj = (NFOBJECT*)nfui_nfbutton_new();
			nfui_nfbutton_set_image(NF_BUTTON(obj), radio_img);
			nfui_nfbutton_set_drawing_outline((NFBUTTON*)obj, FALSE);
			nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y + (40-size_h)/2);
			g_auth_hide_obj[i] = obj;
			
			if (i == g_user_data[idx].certi_popup_hide)
				nfui_radio_button_set_toggled(NF_BUTTON(obj), TRUE);

			if (i == 0) 	slist = nfui_radio_button_get_group(NF_BUTTON(obj));
			else nfui_radio_button_add_group(NF_BUTTON(obj), slist);

			pos_x += 40;

			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strShow[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, 60, PP_LABEL_SIZE_HEI);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);

			pos_x += 60;
		}

		pos_y += 43;
	}

	pos_x = 8;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("COVERT CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, USER_FIXED_SIZE_WID, PP_LABEL_SIZE_HEI);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_nfobject_show(obj);

	pos_y += 44;

	obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
	nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
	nfui_check_get_size(obj, &size_w, &size_h);
	nfui_regi_post_event_callback(obj, post_covert_check_all_event_handler);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y+(PP_LABEL_SIZE_HEI-size_h)/2);
	nfui_nfobject_show(obj);
	g_covert_check_all = obj;

	if (mode == USER_MODE_EDIT)
	{
		for (i = 0; i < GUI_CHANNEL_CNT ; i++) 
		{
			if (g_user_data[idx].covert[i] == '0') 
			{
				nfui_check_button_set_active(obj, FALSE);
				break;
			}
		}
	}
	else
	{
		nfui_check_button_set_active(obj, FALSE);
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 100, PP_LABEL_SIZE_HEI);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+40, pos_y);

	pos_y += 42;

	memset(strBuf, 0x00, sizeof(strBuf));

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (mode == USER_MODE_EDIT)
		{
			if (g_user_data[idx].covert[i] == '1') 	active = TRUE;
			else									active = FALSE;
		}
		else
		{
			active = FALSE;
		}		
	
		obj = (NFOBJECT*)nfui_checkbutton_new(active);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size(obj, &size_w, &size_h);
		nfui_nfobject_show(obj);
		nfui_regi_post_event_callback(obj, post_covert_check_event_handler);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y+(PP_LABEL_SIZE_HEI-size_h)/2);
		g_covert_check[i] = obj;

		g_sprintf(strBuf, "%d", i+1);

		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(294));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, 60, PP_LABEL_SIZE_HEI);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x+40, pos_y);

		pos_x += USER_FIXED_SIZE_WID / 4;
		
		if (i == 3 || i == 7 || i == 11 || i == 15|| i == 19|| i == 23|| i == 27|| i == 31) 
		{
			pos_x = 8;
			pos_y += (PP_LABEL_SIZE_HEI+2);
		}
		
	}

	pos_y = USER_FIXED_SIZE_HEI - 62;

	obj = nftool_normal_button_create_type1("OK", 152);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, (USER_PP_SIZE_WID/2-152) - 2, pos_y);
	nfui_regi_post_event_callback(obj, post_okbutton_event_handler);
	ua_btns[UAB_OK] = obj;

	obj = nftool_normal_button_create_type1("CANCEL", 152);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, (USER_PP_SIZE_WID/2) + 2, pos_y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
	ua_btns[UAB_CANCEL] = obj;


	nfui_regi_post_event_callback(main_fixed, post_main_bg_event_handler);
	nfui_make_key_hierarchy(main_wnd);

	if (g_edit_enable)	nfui_set_key_focus(value[UA_ROW_ID], TRUE);
	else				nfui_set_key_focus(ua_btns[UAB_OK], TRUE);

	nfui_page_close(PGID_POPUPWND, main_wnd);
	nfui_page_open(PGID_USER_ADD, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_USER_ADD, main_wnd);

	return ret_val;
}
