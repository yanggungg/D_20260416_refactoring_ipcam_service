
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
#include "objects/nfvklabel.h"

#include "modules/ssm.h"

#include "vw_userpwd.h"
#include "vw_live_statusbar.h"
#include "vw_live_start_menu.h"
#include "vw_pass_expired_dlg.h"
#include "vw_user_authentication.h"
#include "vw_change_account_popup.h"
#include "vw_vkeyboard.h"

#include "iux_msg.h"
#include "iux_afx.h"
#include "vsm.h"
#include "nfdal.h"
#include "uxm.h"
#include "evt.h"
#include "ix_mem.h"


#define PWD_FAIL_MSG            "The password has been entered incorrectly %d times.\nThis system will reboot."
#define DEFAULT_PWD_SIZE_W		(705)
#define DEFAULT_PWD_SIZE_H		(415)
#define PIN_PWD_SIZE_W			(400)
#define PIN_PWD_SIZE_H			(600)
#define PWD_POS_X(w)			((DISPLAY_ACTIVE_WIDTH - w)/4*2)
#define PWD_POS_Y(h)			((DISPLAY_ACTIVE_HEIGHT - h)/2)

#define USER_ID_STRING_SIZE		(16)
#define PASSWORD_STRING_SIZE	(32)
#define PASSWORD_FAIL_CNT		(5)
#define PWD_LOCK_TIME			(30) // second

typedef enum _KEY_CNT_E_ KEY_CNT_E;
enum _KEY_CNT_E_{
	BACKSPACE_KEY = 13,
	DELETE_KEY 	  = 27,
	ENTER_KEY 	  = 39,
	SPACE_KEY 	  = 50,
	SHIFT_KEY 	  = 51,
	KEY_CNT 
};

typedef enum _PWD_MODE_E_ PWD_MODE_E;
enum _PWD_MODE_E_{
	LOG_ON = 0,
	KEY_LOCK,
	KEY_UNLOCK,
	SHUTDOWN,
	AUTOLOGOUT,
	AUTH,
	BROKEN_RAID,
    DOUBLE_LOGIN,
    DOUBLE_LOGIN2,
};

typedef enum {
    CHOOSE_ID_MODE = 0,
    DIRECT_INPUT_MODE
}ID_INPUT_MODE_E;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_user_obj = NULL;
static NFOBJECT *g_pwd_obj = NULL;
static NFOBJECT *g_titleObj = NULL;
static NFOBJECT *g_keyBtn[KEY_CNT];
static NFOBJECT *g_ok_btn = NULL;
static NFOBJECT *g_sel_obj = NULL;

static NFOBJECT *invmbox= NULL;

static gchar g_pwdTitle[32];
static gint g_pwdMode = 0;		// 0:login, 1:key lock, 2:key unlock
static gboolean g_retVal = FALSE;
static gint fail_cnt = 0;
static gint g_check_auth;

static gint g_pw_changed_by_web = 0;
static gchar g_cur_password[NFUI_MAX_PW_SIZE];
static ID_INPUT_MODE_E g_pwd_id_input_mode = CHOOSE_ID_MODE;
static ID_INPUT_MODE_E g_pin_id_input_mode = CHOOSE_ID_MODE;

static NFOBJECT *g_pin_user_obj = NULL;
static NFOBJECT *g_pin_label[8];
static NFOBJECT *g_pin_key_obj[10];
static gchar g_input_pin_number[16];
static gint g_input_pin_pos = 0;
static gint g_switch_default_password = 0;

static gint get_pressed_key_index(NFOBJECT *obj);
static void create_backspace_image(GdkPixbuf **dest, GdkPixbuf **src);


static /*inline*/ void set_user_pwd_mode(gint mode);
static /*inline*/ gint get_user_pwd_mode();



static /*inline*/ void set_user_pwd_mode(gint mode)
{
	g_pwdMode = mode;
}

static /*inline*/ gint get_user_pwd_mode()
{
	return g_pwdMode;
}

static /*inline*/ void set_user_pwd_return_val(gboolean val)
{
	g_retVal = val;
}

static void set_fail_cnt_init()
{
    fail_cnt = 0;
}

static gboolean lock_userpwd()
{
	NFOBJECT *top;
	NFOBJECT *wait_pop;

	if(++fail_cnt >= PASSWORD_FAIL_CNT) {
		set_fail_cnt_init();

		set_user_pwd_return_val(FALSE);

		return TRUE;
	}

	return FALSE;
}

static gint get_pressed_key_index(NFOBJECT *obj)
{
	gint index = 0;

	while(index < KEY_CNT) {
		if(g_keyBtn[index] == obj) 
			return index;	

		++index;
	}

	return -1;
}

static gint _is_need_pw_force_changed(gint id_idx)
{
	guint init_pw = 0;
 
	if (id_idx == 0) return 0;
	if(!DAL_get_passwd_change_enable()) return 0;

	init_pw = DAL_get_init_pw_changed(id_idx);
	if(init_pw != 0) return 0;

	return 1;
} 

static gint _proc_renew_pw_change(gint id_idx, gchar *new_passwd)
{
	UserManageData userdata;
	gint ret;
	guint opt = 0;
	guint max_ch = 0;

	memset(&userdata, 0x00, sizeof(UserManageData));

	DAL_get_userManage_data(&userdata, id_idx);

	if (DAL_get_enahnced_password()) 
	{
		opt |= (1 << OPT_ENAHNCED_PASSWORD);
		max_ch = NFUI_MAX_ENHANCED_PW_SIZE;
	}
	else 
	{
		max_ch = NFUI_MAX_PW_SIZE;
	}

	ret = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, userdata.id, NULL, new_passwd, max_ch, 0);

	if (ret == 0) return 0;
	if (strcmp(userdata.pw, new_passwd) == 0) 
	{
		nftool_mbox(g_curwnd, "NOTICE", "The Renew Password option has been enabled.\nThe password must be changed in order to log-in.", NFTOOL_MB_OK);
		return 0;
	}

	userdata.pw_last_changed = time(0);
	userdata.init_pw_changed = time(0);
	g_stpcpy(userdata.pw, new_passwd);

	DAL_set_userManage_data(userdata, id_idx);
	DAL_save_setup_db(NFSETUP_WINDOW_USER);
	nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

	return 1;
}

static gint _proc_pw_change(gint id_idx, gchar *new_passwd)
{
	UserManageData userdata;
	gint ret;
	guint opt = 0;
	guint max_ch = 0;

	memset(&userdata, 0x00, sizeof(UserManageData));

	DAL_get_userManage_data(&userdata, id_idx);

	if (DAL_get_enahnced_password()) 
	{
		opt |= (1 << OPT_ENAHNCED_PASSWORD);
		max_ch = NFUI_MAX_ENHANCED_PW_SIZE;
	}
	else 
	{
		max_ch = NFUI_MAX_PW_SIZE;
	}

	ret = VW_Open_account_change_Popup(g_curwnd, "CHANGE PASSWORD", opt, userdata.id, userdata.pw, new_passwd, max_ch, ORG_PW_CHECK);
	if (ret == 0) return 0;
	if (strcmp(userdata.pw, new_passwd) == 0) return 0;

	userdata.pw_last_changed = time(0);
	g_stpcpy(userdata.pw, new_passwd);

	DAL_set_userManage_data(userdata, id_idx);
	DAL_save_setup_db(NFSETUP_WINDOW_USER);
	nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

	return 1;
}

static gchar _shift_translate(gchar in_buf)
{
	gchar out_buf = in_buf;

	switch(in_buf) 
	{
		//case '\'': out_buf = '~';   break;
		case '1':  out_buf = '!';   break;
		case '2':  out_buf = '@';   break;
		case '3':  out_buf = '#';   break;
		case '4':  out_buf = '$';   break;
		case '5':  out_buf = '%';   break;
		case '6':  out_buf = '^';   break;
		case '7':  out_buf = '&';   break;
		case '8':  out_buf = '*';   break;
		case '9':  out_buf = '(';   break;
		case '0':  out_buf = ')';   break;
		case '-':  out_buf = '_';   break;
		case '=':  out_buf = '+';   break;
		case '\\':  out_buf = '|';  break;
		case '.':  out_buf = ':';   break;
		
		//case '~':  out_buf = '\'';  break;
		case '!':  out_buf = '1';   break;
		case '@':  out_buf = '2';   break;
		case '#':  out_buf = '3';   break;
		case '$':  out_buf = '4';   break;
		case '%':  out_buf = '5';   break;
		case '^':  out_buf = '6';   break;
		case '&':  out_buf = '7';   break;
		case '*':  out_buf = '8';   break;
		case '(':  out_buf = '9';   break;
		case ')':  out_buf = '0';   break;
		case '_':  out_buf = '-';   break;
		case '+':  out_buf = '=';   break;
		case '|':  out_buf = '\\';  break;
		case ':':  out_buf = '.';   break;
		
		default:
		{
			if (g_ascii_isalpha(in_buf)) 
			{
				if (g_ascii_islower(in_buf)) out_buf = in_buf-32;
				else out_buf = in_buf+32;
			}
		}
		break;
	}

	return out_buf;
}

static gchar _shift_translate_S1(gchar in_buf)
{
	gchar out_buf = in_buf;

	switch(in_buf) 
	{
		case '\`': out_buf = '~';   break;
		case '1':  out_buf = '!';   break;
		case '2':  out_buf = '@';   break;
		case '3':  out_buf = '#';   break;
		case '4':  out_buf = '$';   break;
		case '5':  out_buf = '%';   break;
		case '6':  out_buf = '^';   break;
		case '7':  out_buf = '&';   break;
		case '8':  out_buf = '*';   break;
		case '9':  out_buf = '(';   break;
		case '0':  out_buf = ')';   break;
		case '-':  out_buf = '_';   break;
		case '=':  out_buf = '+';   break;
		case '{':  out_buf = '[';   break;
		case '}':  out_buf = ']';   break;		
		case '\\':  out_buf = '|';  break;
		case '.':  out_buf = ':';   break;
		case '\"':  out_buf = '\'';   break;
		case '<':  out_buf = ',';   break;
		case '>':  out_buf = ';';   break;
		case '?':  out_buf = '/';   break;
		
		case '~':  out_buf = '\`';  break;
		case '!':  out_buf = '1';   break;
		case '@':  out_buf = '2';   break;
		case '#':  out_buf = '3';   break;
		case '$':  out_buf = '4';   break;
		case '%':  out_buf = '5';   break;
		case '^':  out_buf = '6';   break;
		case '&':  out_buf = '7';   break;
		case '*':  out_buf = '8';   break;
		case '(':  out_buf = '9';   break;
		case ')':  out_buf = '0';   break;
		case '_':  out_buf = '-';   break;
		case '+':  out_buf = '=';   break;
		case '[':  out_buf = '{';   break;
		case ']':  out_buf = '}';   break;				
		case '|':  out_buf = '\\';  break;
		case ':':  out_buf = '.';   break;
		case '\'':  out_buf = '\"';   break;
		case ',':  out_buf = '<';   break;
		case ';':  out_buf = '>';   break;
		case '/':  out_buf = '?';   break;
		
		default:
		{
			if (g_ascii_isalpha(in_buf)) 
			{
				if (g_ascii_islower(in_buf)) out_buf = in_buf-32;
				else out_buf = in_buf+32;
			}
		}
		break;
	}

	return out_buf;
}

static gint _check_user_auth(gint user_idx, gint check_auth)
{
    UserAuthData data;
    gchar group[16];
    gint i;

    memset(group, 0x00, sizeof(group));
    memset(&data, 0x00, sizeof(UserAuthData));
    
    DAL_get_user_group(group, user_idx);

    for (i = 0; i < 4; i++) {
        DAL_get_userAuth_data(&data, i);
        if (strcmp(group, data.grp_name) == 0) break;
    }

    if (i == 4) return 0;

    switch(check_auth)
    {
        case USR_AUTH_SEARCH:
            return data.search;
            
        case USR_AUTH_ARCHIVE:
            return data.archive;

        case -1:
            return 1;

        default:
            return 0;
    }

    return 0;
}

static gint _get_input_pin_number(gchar *pin_number)
{
	gchar *p;
	gint i;
	
	for (i = 0; i < ivsc.dfunc.pin.digit; i++)
	{
		p = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_pin_label[i]);
		pin_number[i] = p[0];
	}
	return 0;
}

static gint _reset_input_pin_number()
{
	gint i;

	nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[0], 1);

	for (i = 0; i < ivsc.dfunc.pin.digit; i++)
	{
		nfui_nfvklabel_set_string((NFVKLABEL*)g_pin_label[i], "");
		if (i != 0) nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[i], 0);
		nfui_signal_emit(g_pin_label[i], GDK_EXPOSE, TRUE);
	}
	return 0;
}

static gboolean post_sel_obj_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    if (event->type == GDK_BUTTON_PRESS)
    {
        NFOBJECT *tobj;
        
        tobj = nfui_get_cur_focus(g_curwnd);

        if (tobj == g_user_obj)
        {
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_user_obj, TRUE);
            nfui_signal_emit(g_user_obj, GDK_EXPOSE, TRUE);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pwd_obj, FALSE);
            nfui_signal_emit(g_pwd_obj, GDK_EXPOSE, TRUE);
            g_sel_obj = g_user_obj;
        }
        else if (tobj == g_pwd_obj)
        {
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pwd_obj, TRUE);
            nfui_signal_emit(g_pwd_obj, GDK_EXPOSE, TRUE);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_user_obj, FALSE);
            nfui_signal_emit(g_user_obj, GDK_EXPOSE, TRUE);
            g_sel_obj = g_pwd_obj;
        }
        else
        {
            g_message("###yanggungg : %s, %d, ELSE_OBJ!!!", __FUNCTION__, __LINE__);
        }
    }
}

static gboolean post_func_key_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_PRESS) {
		gchar strTmp[PASSWORD_STRING_SIZE+1];
		gint key_index = -1;

		if(event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		key_index = get_pressed_key_index(obj);
		if(key_index < 0) return FALSE;

		switch(key_index) 
		{
			case BACKSPACE_KEY:
			{
				if (nfui_nfvklabel_erase((NFVKLABEL*)g_sel_obj, VK_ERASE_BACKSPACE) == 0)
					nfui_signal_emit(g_sel_obj, GDK_EXPOSE, FALSE);
			}
			break;
			
			case DELETE_KEY:
			{
				if (nfui_nfvklabel_erase((NFVKLABEL*)g_sel_obj, VK_ERASE_DEL) == 0)
					nfui_signal_emit(g_sel_obj, GDK_EXPOSE, FALSE);
			}
			break;

			case SPACE_KEY:
			{
				if (nfui_nfvklabel_input_character((NFVKLABEL*)g_sel_obj, ' ') == 0)
					nfui_signal_emit(g_sel_obj, GDK_EXPOSE, FALSE);
			}
			break;

			case SHIFT_KEY:
			{
				gint i;
				gchar inBuf[4];
				gchar outBuf[4];				

				memset(inBuf, 0x00, sizeof(inBuf));
				memset(outBuf, 0x00, sizeof(outBuf));
				
				for (i = 0; i < KEY_CNT; i++) 
				{
					if (i == 13 || i == 27 || i == 39 || i == 50 || i == 51) continue;

					strcpy(inBuf, nfui_nfbutton_get_text((NFBUTTON*)(g_keyBtn[i])));
					
					outBuf[0] = _shift_translate_S1(inBuf[0]);

					nfui_nfbutton_set_text((NFBUTTON*)(g_keyBtn[i]), outBuf);
					nfui_signal_emit(g_keyBtn[i], GDK_EXPOSE, TRUE);
				}
			}
			break;

			default:
			return FALSE;
		}
	}

	return FALSE;
}

static void create_backspace_image(GdkPixbuf **dest, GdkPixbuf **src)
{
	gdk_pixbuf_composite(src[0], dest[0], 8, 8, 36, 36, 8.0, 8.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(src[1], dest[1], 8, 8, 36, 36, 8.0, 8.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(src[2], dest[2], 8, 8, 36, 36, 8.0, 8.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
	gdk_pixbuf_composite(src[3], dest[3], 8, 8, 36, 36, 8.0, 8.0, 1.0, 1.0, GDK_INTERP_BILINEAR, 255);
}


static gboolean post_key_event_cb(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_PRESS) {
		gint key_index = -1;
		gchar strPwd[PASSWORD_STRING_SIZE+1];
		gchar keyVal[PASSWORD_STRING_SIZE+1];

		if(event->button.button == MOUSE_RIGTH_BUTTON) 			 
			return FALSE;

		key_index = get_pressed_key_index(obj);

		if(key_index < 0) return FALSE;
		
		strcpy(keyVal, nfui_nfbutton_get_text((NFBUTTON*)(obj)));

		if (nfui_nfvklabel_input_character((NFVKLABEL*)g_sel_obj, keyVal[0]) == 0)
			nfui_signal_emit(g_sel_obj, GDK_EXPOSE, FALSE);	
	}

	return FALSE;
}

static gboolean ok_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE) {
		NFOBJECT *top = NULL;
		gchar *id;
		gchar *idbuf;
		gchar password[NFUI_MAX_ENHANCED_PW_SIZE + 1];		
		guint id_idx = 0;

        if (g_pwd_id_input_mode == CHOOSE_ID_MODE) {
			id = nfui_combobox_get_value((NFCOMBOBOX*)g_user_obj);
            id_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_user_obj);
        }
        else {
            id = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_user_obj);
            id_idx = ssm_get_usr_idx(id);

			if (strlen(id) == 0) {
				nftool_mbox_handle(g_curwnd, "NOTICE", "Please input user ID.", NFTOOL_MB_OK, &invmbox);
				nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
				return FALSE;
			}
			else if (ssm_get_usr_idx(id) == -1) {
				nftool_mbox_handle(g_curwnd, "NOTICE", "User ID has not been registered.", NFTOOL_MB_OK, &invmbox);
				nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
				return FALSE;
			}
			else if (get_user_pwd_mode() != LOG_ON && get_user_pwd_mode() != BROKEN_RAID && get_user_pwd_mode() != DOUBLE_LOGIN2) {
				gchar login_user[USER_ID_STRING_SIZE];
				memset(login_user, 0x00, sizeof(login_user));
				if (strlen(ssm_get_cur_id(login_user)) == 0) strcpy(login_user, "ADMIN");

				if (strcmp(login_user, id) != 0) {
					nftool_mbox_handle(g_curwnd, "NOTICE", "User authentication failed.", NFTOOL_MB_OK, &invmbox);
					nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
					return FALSE;
				}
			}
        }

		memset(password, 0x00, sizeof(password));
		strcpy(password, nfui_nfvklabel_get_all_str((NFVKLABEL*)g_pwd_obj));

		switch(get_user_pwd_mode()) 
		{
			case LOG_ON:
			{
				if (ssm_check_id_passwd(id, password))
				{
					gchar new_password[64];	
					gint ret;

                    memset(g_cur_password, 0x00, sizeof(g_cur_password));
                	strcpy(g_cur_password, password);

                    uxm_reg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);

					if (_is_need_pw_force_changed(id_idx)) 
					{				
						memset(new_password, 0x00, sizeof(new_password));
						ret = _proc_renew_pw_change(id_idx, new_password);
							
						if (ret == 0)
						{
                    		nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
                    		nfui_signal_emit(g_pwd_obj, GDK_EXPOSE, TRUE);
                            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);						
							return FALSE;
						}
						strcpy(password, new_password);
					}

					if (ssm_get_remain_day_to_expire_user(id) == 0)
					{
						ret = VW_CheckPasswordExpired_id(g_curwnd, id);

						if (ret == 0)
						{
                    		nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
                    		nfui_signal_emit(g_pwd_obj, GDK_EXPOSE, TRUE);
                            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);						
							return FALSE;
						}

                       	//Don't show message for a day. 
						if (ssm_get_remain_day_to_expire_user(id) == -1)
						{
							ssm_logon(id, password);

							VW_Live_StatusBar_Menu_Enable();
							VW_Live_StatusBar_Set_User(id);

							LiveStart_Popup_Menu_Enable();
							scm_put_log(OPEN_LIVE, 0, 0);
							vsm_set_covert_by_user(id);
							set_user_pwd_return_val(TRUE);

							idbuf = imalloc(sizeof(char) * 32);
							strcpy(idbuf, id);
							evt_send_to_local(INFY_USER_LOGON, 0, 1, idbuf);

                            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);						

							top = nfui_nfobject_get_top(obj);
							nfui_nfobject_destroy(top);
							return FALSE;
						}
							
						memset(new_password, 0x00, sizeof(new_password));
						ret = _proc_pw_change(id_idx, new_password);
							
						if (ret == 0)
						{
                    		nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
                    		nfui_signal_emit(g_pwd_obj, GDK_EXPOSE, TRUE);
                            uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);						
							return FALSE;
						}							
						strcpy(password, new_password);
					}		

					ssm_logon(id, password);

					VW_Live_StatusBar_Menu_Enable();
					VW_Live_StatusBar_Set_User(id);

					LiveStart_Popup_Menu_Enable();

					scm_put_log(OPEN_LIVE, 0, 0);
					vsm_set_covert_by_user(id);
					set_user_pwd_return_val(TRUE);

					idbuf = imalloc(sizeof(char) * 32);
					strcpy(idbuf, id);
					evt_send_to_local(INFY_USER_LOGON, 0, 1, idbuf);

                    uxm_unreg_imsg_event(obj, INFY_USRDB_CHANGE_NOTIFY);						

					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_destroy(top);
					
					set_fail_cnt_init();
				}
				else
				{
				    scm_put_log_t(LOCAL_LOGON_FAIL, 0, 0, id);
					nf_event_logon_fail_check(id, 1, 1);
					
					if(!lock_userpwd())
					{
						break;
					}
					else
					{
                		top = nfui_nfobject_get_top((NFOBJECT*)g_curwnd);
                		nfui_nfobject_hide(top);
                		nfui_nfobject_destroy(top);

                		nftool_mbox_auto(NULL, PWD_LOCK_TIME, "WARNING", "Please retry after 30 sec.");
					}
				}
			}
			return FALSE;

			case KEY_LOCK:
			{
				if(ssm_is_cur_user(id, password)) {
					nfui_nflabel_set_text((NFLABEL*)g_titleObj, "KEY UNLOCK");
                    if (g_pwd_id_input_mode == DIRECT_INPUT_MODE) {
                        nfui_nfvklabel_set_string((NFVKLABEL*)g_user_obj, "");
                    }
					nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");

					set_user_pwd_mode(KEY_UNLOCK);

					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_hide(top);

					nffunc_change_led_by_mask(0x0000001F, 0xFFFEFFFF);

					set_user_pwd_return_val(TRUE);
                    set_fail_cnt_init();
				}else
					break;
			}
			return FALSE;

			case KEY_UNLOCK:
			{
				if(ssm_is_cur_user(id, password)) {
					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_destroy(top);

					set_user_pwd_return_val(TRUE);
                    set_fail_cnt_init();
				}else 
					break;
			}
			return FALSE;

			case SHUTDOWN:
			{
				if(ssm_is_cur_user(id, password)) {
					ssm_shutdown();	
					
    				set_user_pwd_return_val(TRUE);

					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_destroy(top);

					set_user_pwd_return_val(TRUE);
                    set_fail_cnt_init();                    
				}
				else {
					if(!lock_userpwd())
					{
						break;
					}
					else
					{
                		top = nfui_nfobject_get_top((NFOBJECT*)g_curwnd);
                		nfui_nfobject_hide(top);
                		nfui_nfobject_destroy(top);

                		nftool_mbox_auto(NULL, PWD_LOCK_TIME, "WARNING", "Please retry after 30 sec.");
					}
				}
			}
			return FALSE;

			case AUTOLOGOUT:
			break;

			case AUTH:
			{
				if(ssm_check_id_passwd(id, password)) {
					nfui_nflabel_set_text((NFLABEL*)g_titleObj, "PASSWORD CHECK");
					nfui_nflabel_set_text((NFLABEL*)g_pwd_obj, "");

					set_user_pwd_mode(AUTH);

					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_hide(top);

					set_user_pwd_return_val(TRUE);
                    set_fail_cnt_init();
                    
					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_destroy(top);
				}
				else {
					if(!lock_userpwd())
					{
						break;
					}
					else
					{
                		top = nfui_nfobject_get_top((NFOBJECT*)g_curwnd);
                		nfui_nfobject_hide(top);
                		nfui_nfobject_destroy(top);

                		nftool_mbox_auto(NULL, PWD_LOCK_TIME, "WARNING", "Please retry after 30 sec.");
					}
				}
			}
			return FALSE;

            case BROKEN_RAID:
            {
                if(ssm_check_id_passwd(id, password)) {
                    nfui_nflabel_set_text((NFLABEL*)g_titleObj, "DELETE RAID");
                    nfui_nflabel_set_text((NFLABEL*)g_pwd_obj, "");

                    set_user_pwd_mode(BROKEN_RAID);

                    top = nfui_nfobject_get_top(obj);
                    nfui_nfobject_hide(top);

                    set_user_pwd_return_val(TRUE);
                    set_fail_cnt_init();
                    
                    top = nfui_nfobject_get_top(obj);
                    nfui_nfobject_destroy(top);
                }
                else 
                {
                    if(!lock_userpwd())
					{
						break;
					}
					else
					{
					    gchar buf[256];

					    g_sprintf(buf, lookup_string(PWD_FAIL_MSG), PASSWORD_FAIL_CNT);
                		
                		top = nfui_nfobject_get_top((NFOBJECT*)g_curwnd);
                		nfui_nfobject_hide(top);
                		nfui_nfobject_destroy(top);
                		
                		nftool_mbox(NULL, "WARNING", buf, NFTOOL_MB_OK);

					}
                }
            }

            case DOUBLE_LOGIN:
            case DOUBLE_LOGIN2:
            {
                if(ssm_check_id_passwd(id, password)) 
                {
                    if (!_check_user_auth(ssm_get_usr_idx(id), g_check_auth) && g_pwd_id_input_mode == DIRECT_INPUT_MODE)
                    {
                        switch(g_check_auth)
                        {
                            case USR_AUTH_SEARCH:
                                nftool_mbox_handle(g_curwnd, "WARNING", "No authority", NFTOOL_MB_OK, &invmbox);
                            break;

                            case USR_AUTH_ARCHIVE:
                                nftool_mbox_handle(g_curwnd, "WARNING", "No authority", NFTOOL_MB_OK, &invmbox);
                            break;
                        }
                        set_user_pwd_return_val(FALSE);
                    }
                    else
                    {
                        set_user_pwd_return_val(TRUE);
                    }
                    
                    top = nfui_nfobject_get_top(obj);
                    nfui_nfobject_hide(top);

                    set_fail_cnt_init();
                    
                    top = nfui_nfobject_get_top(obj);
                    nfui_nfobject_destroy(top);
                }
                else 
                {
                    if(!lock_userpwd())
                        break;
                }
            }
            return FALSE;

			default:
			break;
		}

        if (strlen(password) == 0)
            nftool_mbox_handle(g_curwnd, "NOTICE", "Please input PW.", NFTOOL_MB_OK, &invmbox);
        else
		nftool_mbox_handle(g_curwnd, "NOTICE", "Invalid password.", NFTOOL_MB_OK, &invmbox);
        invmbox = 0;
		
	    nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
	}
    else if (event->type == INFY_USRDB_CHANGE_NOTIFY)
    {
        UserManageData userdata;
        gint id_idx;
        NFOBJECT *top;

        id_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_user_obj);

        memset(&userdata, 0x00, sizeof(UserManageData));
        DAL_get_userManage_data(&userdata, id_idx);     
        if (strcmp(userdata.pw, g_cur_password) == 0) return FALSE;

		ssm_logout();
        evt_send_to_local(INFY_LEAVE_LIVE, 0, 0, 0);
    }

	return FALSE;
}

static gboolean forgot_password_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;
	UserManageData userdata;

	if (event->type == GDK_BUTTON_RELEASE) 
	{
		gint usr_idx = 0;
	
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (g_user_obj->type == NFOBJECT_TYPE_NFCOMBOBOX) 
			usr_idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_user_obj);
		g_message("%s, %d, usr_idx:%d", __FUNCTION__, __LINE__, usr_idx);

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);

		memset(&userdata, 0x00, sizeof(UserManageData));
		DAL_get_userManage_data(&userdata, usr_idx);

		if(ivsc.dfunc.support_forgot_pw_warning_box) {
		if ((userdata.email_certification == 0)) {
			nftool_mbox(g_curwnd, "WARNING", "To protect your data, unauthenticated e-mail addresses\nmay not be used for recovering a forgotten password.", NFTOOL_MB_OK);
			return FALSE;
		}
		}
	
		VW_Open_User_Authen_Popup(top->parent, usr_idx, g_pwd_id_input_mode);
	}

	return FALSE;
}

static gboolean post_pin_user_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    GdkEventKey *kevt;
    KEYPAD_KID kpid;

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
            gchar buf[256];

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

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text(obj), x, y, USER_ID_STRING_SIZE, VKEY_ALPHANUMERIC);

        if (strTemp) {
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
            ifree(strTemp);
            strTemp = NULL;
        }
	}

	return FALSE;
}

static gboolean post_pin_key_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		NFOBJECT *top;
		gchar login_user[USER_ID_STRING_SIZE];
		gchar strBuf[8];
		gchar *idbuf;
		gchar *id;
		guint id_idx = 0;
		gint i;

		UserManageData userdata;

		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < 10; i++) {
			if (g_pin_key_obj[i] == obj) break;
		}
		if (i == 10) return FALSE;

		memset(strBuf, 0x00, sizeof(strBuf));
		sprintf(strBuf, "%d", i);
		nfui_nfvklabel_set_string((NFVKLABEL*)g_pin_label[g_input_pin_pos], strBuf);

		if (g_input_pin_pos != ivsc.dfunc.pin.digit - 1) 
		{
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_input_pin_pos], 0);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_input_pin_pos+1], 1);
			nfui_signal_emit(g_pin_label[g_input_pin_pos], GDK_EXPOSE, TRUE);
			nfui_signal_emit(g_pin_label[g_input_pin_pos+1], GDK_EXPOSE, TRUE);
			g_input_pin_pos++;
		}
		else 
		{
			g_input_pin_pos = 0;
			memset(g_input_pin_number, 0x00, sizeof(g_input_pin_number));
			_get_input_pin_number(g_input_pin_number);

			if (g_pin_id_input_mode == CHOOSE_ID_MODE) {
				id = nfui_combobox_get_value((NFCOMBOBOX*)g_pin_user_obj);
			}
			else {
				id = nfui_nflabel_get_text((NFLABEL*)g_pin_user_obj);

				if (strlen(id) == 0) {
					nftool_mbox_handle(g_curwnd, "NOTICE", "Please input user ID.", NFTOOL_MB_OK, &invmbox);
					_reset_input_pin_number();
					return FALSE;
				}
				else if (ssm_get_usr_idx(id) == -1) {
					nftool_mbox_handle(g_curwnd, "NOTICE", "User ID has not been registered.", NFTOOL_MB_OK, &invmbox);
					_reset_input_pin_number();
					return FALSE;
				}
				else if (get_user_pwd_mode() != LOG_ON && get_user_pwd_mode() != BROKEN_RAID && get_user_pwd_mode() != DOUBLE_LOGIN2) {
					memset(login_user, 0x00, sizeof(login_user));
					if (strlen(ssm_get_cur_id(login_user)) == 0) strcpy(login_user, "ADMIN");

					if (strcmp(login_user, id) != 0) {
						nftool_mbox_handle(g_curwnd, "NOTICE", "User authentication failed.", NFTOOL_MB_OK, &invmbox);
						_reset_input_pin_number();
						return FALSE;
					}
				}
			}

			id_idx = ssm_get_usr_idx(id);

			memset(&userdata, 0x00, sizeof(UserManageData));
			DAL_get_userManage_data(&userdata, id_idx);
			if (userdata.use_pin == 0) {
				nftool_mbox_handle(g_curwnd, "NOTICE", "User authentication failed.", NFTOOL_MB_OK, &invmbox);
				_reset_input_pin_number();
				return FALSE;
			}

			if (strcmp(userdata.pin_number, g_input_pin_number) == 0) 
			{
				if (get_user_pwd_mode() == LOG_ON) {
					ssm_logon(userdata.id, userdata.pw);

					VW_Live_StatusBar_Menu_Enable();
					VW_Live_StatusBar_Set_User(userdata.id);
					LiveStart_Popup_Menu_Enable();

					scm_put_log(OPEN_LIVE, 0, 0);
					vsm_set_covert_by_user(userdata.id);

					idbuf = imalloc(sizeof(char) * 32);
					strcpy(idbuf, userdata.id);
					evt_send_to_local(INFY_USER_LOGON, 0, 1, idbuf);
				}
				else if (get_user_pwd_mode() == KEY_LOCK) {
					nfui_nflabel_set_text((NFLABEL*)g_titleObj, "KEY UNLOCK");
					set_user_pwd_mode(KEY_UNLOCK);
					nffunc_change_led_by_mask(0x0000001F, 0xFFFEFFFF);
				}
				else if (get_user_pwd_mode() == SHUTDOWN) {
					ssm_shutdown();
				}

				if (get_user_pwd_mode() == KEY_LOCK) {
					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_hide(top);
				}
				else {
					top = nfui_nfobject_get_top(obj);
					nfui_nfobject_destroy(top);
				}

				set_user_pwd_return_val(TRUE);
				set_fail_cnt_init();
			}
			else 
			{
				if (get_user_pwd_mode() == LOG_ON || get_user_pwd_mode() == SHUTDOWN) {
					scm_put_log_t(LOCAL_LOGON_FAIL, 0, 0, userdata.id);
					nf_event_logon_fail_check(userdata.id, 1, 1);
				}

				if (get_user_pwd_mode() == KEY_LOCK || get_user_pwd_mode() == KEY_UNLOCK) {
					_reset_input_pin_number();
					nftool_mbox(g_curwnd, "WARNING", "Incorrect pin number.", NFTOOL_MB_OK);
				}
				else if (get_user_pwd_mode() == BROKEN_RAID) {
					if (!lock_userpwd())
					{
						_reset_input_pin_number();
						nftool_mbox(g_curwnd, "WARNING", "Incorrect pin number.", NFTOOL_MB_OK);
					}
					else
					{
						gchar buf[256];
						g_sprintf(buf, lookup_string(PWD_FAIL_MSG), PASSWORD_FAIL_CNT);
						top = nfui_nfobject_get_top((NFOBJECT*)g_curwnd);
						nfui_nfobject_hide(top);
						nfui_nfobject_destroy(top);
						nftool_mbox(NULL, "WARNING", buf, NFTOOL_MB_OK);
					}
				}
				else {
					if (!lock_userpwd())
					{
						_reset_input_pin_number();
						nftool_mbox(g_curwnd, "WARNING", "Incorrect pin number.", NFTOOL_MB_OK);
					}
					else
					{
						top = nfui_nfobject_get_top((NFOBJECT*)g_curwnd);
						nfui_nfobject_hide(top);
						nfui_nfobject_destroy(top);
						nftool_mbox_auto(NULL, PWD_LOCK_TIME, "WARNING", "Please retry after 30 sec.");
					}
				}
			}
		}
	}

	return FALSE;
}

static gboolean post_pin_switch_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		NFOBJECT *top;

		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_switch_default_password = 1;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_pin_backspace_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (g_input_pin_pos == 0) return FALSE;

		nfui_nfvklabel_set_string((NFVKLABEL*)g_pin_label[g_input_pin_pos-1], "");
		nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_input_pin_pos], 0);
		nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pin_label[g_input_pin_pos-1], 1);
		nfui_signal_emit(g_pin_label[g_input_pin_pos], GDK_EXPOSE, TRUE);
		nfui_signal_emit(g_pin_label[g_input_pin_pos-1], GDK_EXPOSE, TRUE);
		g_input_pin_pos--;
	}

	return FALSE;
}

static gboolean post_pin_forgot_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_BUTTON_PRESS) 
	{
		NFOBJECT *top;

		if (event->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_switch_default_password = 1;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean cancel_button_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *top;

	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		top = nfui_nfobject_get_top(obj);

		if(get_user_pwd_mode() == KEY_UNLOCK)
		{
			nfui_nfobject_hide(top);
            nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");
			nffunc_change_led_by_mask(0x0000001F, 0xFFFEFFFF);

			return FALSE;
		}

		nfui_nfobject_destroy(top);
		set_user_pwd_return_val(FALSE);
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	if(get_user_pwd_mode() == KEY_UNLOCK)
	{
		nfui_nfobject_hide(top);
		nfui_nfvklabel_set_string((NFVKLABEL*)g_pwd_obj, "");		
		nffunc_change_led_by_mask(0x0000001F, 0xFFFEFFFF);

		return FALSE;
	}
	else if(get_user_pwd_mode() == SHUTDOWN)
	{
		return FALSE;
	}

	return TRUE;
}

static gboolean pwd_dlg_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == NFEVENT_REMOCON_RELEASE || event->type == NFEVENT_KEYPAD_RELEASE) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		gchar *strPwd;
		gchar buf[8];

        strPwd = nfui_nfvklabel_get_all_str((NFVKLABEL*)g_pwd_obj);

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid) {
			case KEYPAD_CH01:
			case KEYPAD_CH02:
			case KEYPAD_CH03:
			case KEYPAD_CH04:
			case KEYPAD_CH05:
			case KEYPAD_CH06:
			case KEYPAD_CH07:
			case KEYPAD_CH08:
			case KEYPAD_CH09:
			{
				NFOBJECT* cur_focus = NULL;

				nfui_clear_key_focus(obj);

				cur_focus = nfui_get_cur_focus(obj);

                if (cur_focus != g_ok_btn) {
                    if (cur_focus) {                
                        nfui_clear_key_focus(cur_focus);
					nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
                    }

                    nfui_set_key_focus(g_ok_btn, TRUE);
                    nfui_signal_emit(g_ok_btn, GDK_EXPOSE, TRUE);
                }

				g_sprintf(buf, "%d", kpid + 1);

				if (nfui_nfvklabel_input_character((NFVKLABEL*)g_sel_obj, buf[0]) == 0)
					nfui_signal_emit(g_sel_obj, GDK_EXPOSE, FALSE);
			}
			break;

			case KEYPAD_CH00:
			case RMC_NUM_0:
			{
				NFOBJECT* cur_focus = NULL;

				nfui_clear_key_focus(obj);
				cur_focus = nfui_get_cur_focus(obj);

                if (cur_focus != g_ok_btn) {
                    if (cur_focus) {                
                        nfui_clear_key_focus(cur_focus);
                        nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
                    }

                    nfui_set_key_focus(g_ok_btn, TRUE);
                    nfui_signal_emit(g_ok_btn, GDK_EXPOSE, TRUE);
                }

                nfui_set_key_focus(g_ok_btn, TRUE);
                nfui_signal_emit(g_ok_btn, GDK_EXPOSE, TRUE);

				g_sprintf(buf, "%d", 0);

				if (nfui_nfvklabel_input_character((NFVKLABEL*)g_sel_obj, buf[0]) == 0)
					nfui_signal_emit(g_sel_obj, GDK_EXPOSE, FALSE);
			}
			break;

			case KEYPAD_EXIT:
				{
					NFOBJECT *top;

					top = nfui_nfobject_get_top(obj);

					if(get_user_pwd_mode() == KEY_UNLOCK) 
					{
						//nfui_nfobject_hide(top);
					}
					else
					{
						set_user_pwd_return_val(FALSE);

						nfui_nfobject_destroy(top);
					}
				}
				return TRUE;

			default:
				return FALSE;
		}
	}
	else if (event->type == WND_HIDE) 
	{
		if (g_user_obj->type == NFOBJECT_TYPE_NFCOMBOBOX) {
			if (NF_COMBOBOX(g_user_obj)->submenu) {
				nfui_nfobject_hide(NF_COMBOBOX(g_user_obj)->submenu);
				nfui_page_close(PGID_COMBO_MENU, NF_COMBOBOX(g_user_obj)->submenu); 
			}
		}

        VW_CheckPasswordExpired_Close();
        VW_Close_account_change_Popup();

	}
	else if(event->type == GDK_DELETE) 
	{
#if defined(_SUPPORT_LOGIN_AUTOHIDE)
		vw_autohide_unset_obj(g_curwnd);
#endif	
        invmbox = NULL;
        g_sel_obj = NULL;
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean pwd_pin_dlg_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == NFEVENT_REMOCON_RELEASE || event->type == NFEVENT_KEYPAD_RELEASE) {
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

		switch(kpid) 
		{
			case KEYPAD_EXIT:
			{
				NFOBJECT *top;

				top = nfui_nfobject_get_top(obj);
				set_user_pwd_return_val(FALSE);
				nfui_nfobject_destroy(top);
			}
			return TRUE;

			default:
			return FALSE;
		}
	}
	else if (event->type == WND_HIDE) 
	{
        VW_CheckPasswordExpired_Close();
        VW_Close_account_change_Popup();
	}
	else if(event->type == GDK_DELETE) 
	{
#if defined(_SUPPORT_LOGIN_AUTOHIDE)
		vw_autohide_unset_obj(g_curwnd);
#endif	
        invmbox = NULL;
        g_sel_obj = NULL;
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean pwd_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(event->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
    
		nfui_nfobject_gc_unref(gc);
	}
	else if (event->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
	}	

	return FALSE;
}

static gboolean login_off() 
{
	gchar passwd[PASSWORD_STRING_SIZE+1];

	// password off
	if(!DAL_get_passwd_enable()) {
		if(DAL_get_user_passwd(passwd, 0)) {
			ssm_logon("ADMIN", passwd);

			VW_Live_StatusBar_Menu_Enable();
			VW_Live_StatusBar_Set_User("ADMIN");

			LiveStart_Popup_Menu_Enable();

			scm_put_log(OPEN_LIVE, 0, 0);

			vsm_set_covert_by_user("ADMIN");

			return TRUE;
		}
	}

	return FALSE;
}

static gint _default_password_window(NFWINDOW *parent, PAGEID pid, gchar *title, gint check_auth)
{
	NFOBJECT *pwd_dlg = NULL;
	NFOBJECT *pwd_fixed = NULL;
	NFOBJECT *obj = NULL;
	NFOBJECT *ok_btn = NULL;
	NFOBJECT *id_label;
	NFOBJECT *pw_label;	

	GdkPixbuf *bgBtn1_W44[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn2_W44[NFOBJECT_STATE_COUNT];
//	GdkPixbuf *bgBtn3_W44[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_W140[NFOBJECT_STATE_COUNT];
	GdkPixbuf *bgBtn_W92[NFOBJECT_STATE_COUNT];
	GdkPixbuf *backspace[NFOBJECT_STATE_COUNT];
	GdkPixbuf *dropdown_img[NFOBJECT_STATE_COUNT];
	
	gchar *keyVal[KEY_CNT] = {"~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "", 
								  "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "{", "}", "\\", "DEL",
								  "a", "s", "d", "f", "g", "h", "j", "k", "l", ".", "\"", "ENTER",
								  "z", "x", "c", "v", "b", "n", "m", "<", ">", "?", "SPACE", "SHIFT"};

	guint key_fg_1[NFOBJECT_STATE_COUNT] = {COLOR_IDX(278), COLOR_IDX(279), COLOR_IDX(280), COLOR_IDX(281)}; 
	guint key_fg_2[NFOBJECT_STATE_COUNT] = {COLOR_IDX(282), COLOR_IDX(283), COLOR_IDX(285), COLOR_IDX(286)}; 
	guint user_cnt = 0;
	gchar user_id[USER_ID_STRING_SIZE];
	gint i;


	// key buttons
	bgBtn1_W44[0] = nfui_get_image_from_file((IMG_N_KEY_01), NULL); 
	bgBtn1_W44[1] = nfui_get_image_from_file((IMG_O_KEY_01), NULL); 
	bgBtn1_W44[2] = nfui_get_image_from_file((IMG_P_KEY_01), NULL); 
	bgBtn1_W44[3] = nfui_get_image_from_file((IMG_D_KEY_01), NULL); 

	bgBtn2_W44[0] = nfui_get_image_from_file((IMG_N_KEY_02), NULL); 
	bgBtn2_W44[1] = nfui_get_image_from_file((IMG_O_KEY_02), NULL); 
	bgBtn2_W44[2] = nfui_get_image_from_file((IMG_P_KEY_02), NULL); 
	bgBtn2_W44[3] = nfui_get_image_from_file((IMG_D_KEY_02), NULL); 
/*
	bgBtn3_W44[0] = nfui_get_image_from_file((IMG_N_KEY_BACKSPACE), NULL); 
	bgBtn3_W44[1] = nfui_get_image_from_file((IMG_O_KEY_BACKSPACE), NULL); 
	bgBtn3_W44[2] = nfui_get_image_from_file((IMG_P_KEY_BACKSPACE), NULL); 
	bgBtn3_W44[3] = nfui_get_image_from_file((IMG_D_KEY_BACKSPACE), NULL); 
*/
	backspace[0] = nfui_get_image_from_file((MK_IMG_N_KEY_BACKSPACE), NULL); 
	backspace[1] = nfui_get_image_from_file((MK_IMG_O_KEY_BACKSPACE), NULL); 
	backspace[2] = nfui_get_image_from_file((MK_IMG_P_KEY_BACKSPACE), NULL); 
	backspace[3] = nfui_get_image_from_file((MK_IMG_D_KEY_BACKSPACE), NULL); 

	bgBtn_W140[0] = nfui_get_image_from_file((IMG_N_KEY_ENTER), NULL); 
	bgBtn_W140[1] = nfui_get_image_from_file((IMG_O_KEY_ENTER), NULL); 
	bgBtn_W140[2] = nfui_get_image_from_file((IMG_P_KEY_ENTER), NULL); 
	bgBtn_W140[3] = nfui_get_image_from_file((IMG_D_KEY_ENTER), NULL); 

	bgBtn_W92[0] = nfui_get_image_from_file((IMG_N_KEY_03), NULL); 
	bgBtn_W92[1] = nfui_get_image_from_file((IMG_O_KEY_03), NULL); 
	bgBtn_W92[2] = nfui_get_image_from_file((IMG_P_KEY_03), NULL); 
	bgBtn_W92[3] = nfui_get_image_from_file((IMG_D_KEY_03), NULL); 
	
//	create_backspace_image(bgBtn3_W44, backspace);

	pwd_dlg = (NFOBJECT*)nfui_nfwindow_new(parent, PWD_POS_X(DEFAULT_PWD_SIZE_W), PWD_POS_Y(DEFAULT_PWD_SIZE_H), DEFAULT_PWD_SIZE_W, DEFAULT_PWD_SIZE_H);
	nfui_nfwindow_set_title(pwd_dlg, "USER PASSWORD");
	nfui_regi_post_event_callback(pwd_dlg, pwd_dlg_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)pwd_dlg, returnkey_proc);
	g_curwnd = pwd_dlg;

	// fixed
	pwd_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(pwd_fixed, DEFAULT_PWD_SIZE_W, DEFAULT_PWD_SIZE_H);
	nfui_regi_post_event_callback(pwd_fixed, pwd_fixed_event_handler);
	nfui_nfobject_show(pwd_fixed);

	// title label
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, pwd_fixed->width-40, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 20, 7);
	g_titleObj = obj;

	memset(g_pwdTitle, 0x00, sizeof(g_pwdTitle));
	strncpy(g_pwdTitle, title, strlen(title));


	// user id & password
	for(i=0; i<2; i++) {
		if(i == 0) obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		if(i == 1) obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, 150, 40);		
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_RIGHT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);

		if(i == 0)		id_label = obj;
		else if(i == 1) pw_label = obj;
	}
	
    // user id label
    if (g_pwd_id_input_mode == CHOOSE_ID_MODE)
    {
		g_user_obj = nfui_combobox_new(NULL, 0, 0);
		nfui_combobox_set_skin_type(NF_COMBOBOX(g_user_obj), NFCOMBOBOX_TYPE_POPUP_1);
		nfui_nfobject_support_multi_lang(g_user_obj, FALSE);
        nfui_nfobject_set_size(g_user_obj, 325, 40);
		nfui_nfobject_show(g_user_obj);
    }
    else
    {
    	g_user_obj = (NFOBJECT*)nfui_nfvklabel_new_str("", USER_ID_STRING_SIZE);
    	nfui_nfvklabel_set_invisible((NFVKLABEL*)g_user_obj, 0);
    	nfui_nfvklabel_set_pango_font((NFVKLABEL*)g_user_obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
    	nfui_nfobject_modify_bg(g_user_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
        nfui_nfobject_set_size(g_user_obj, 325, 40);
		nfui_nfobject_show(g_user_obj);
		nfui_nfobject_support_multi_lang(g_user_obj, FALSE);
		nfui_regi_post_event_callback(g_user_obj, post_sel_obj_event_cb);
    }

    //pw label
	g_pwd_obj = (NFOBJECT*)nfui_nfvklabel_new_str("", 16);
	nfui_nfvklabel_set_invisible((NFVKLABEL*)g_pwd_obj, 1);
	nfui_nfvklabel_set_pango_font((NFVKLABEL*)g_pwd_obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
	nfui_nfobject_support_multi_lang(g_pwd_obj, FALSE);
	nfui_nfobject_modify_bg(g_pwd_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
    nfui_nfobject_set_size(g_pwd_obj, 325, 40);
	nfui_nfobject_show(g_pwd_obj);
	nfui_regi_post_event_callback(g_pwd_obj, post_sel_obj_event_cb);
	
    //input mode
    if (g_pwd_id_input_mode == CHOOSE_ID_MODE)
    {
        if (get_user_pwd_mode() == LOG_ON || get_user_pwd_mode() == AUTOLOGOUT) 
        {
    		user_cnt = DAL_get_user_count();
    		for(i=0; i<user_cnt; i++) {
    			memset(user_id, 0x00, sizeof(user_id));

    			DAL_get_user_id(user_id, (guint)i);
    			nfui_combobox_append_data(NF_COMBOBOX(g_user_obj), user_id);

    			printf("user_count : %d , user_id[%d] : %s \n", user_cnt, i, user_id);
    		}
    	}
        else if (get_user_pwd_mode() == DOUBLE_LOGIN2) 
        {
            user_cnt = DAL_get_user_count();

            for(i=0; i<user_cnt; i++) 
            {
                memset(user_id, 0x00, sizeof(user_id));
                DAL_get_user_id(user_id, (guint)i);

                if (strcmp(ssm_get_cur_id(NULL), user_id) == 0) continue;
                if ((!_check_user_auth(i, check_auth))) continue;
                
                nfui_combobox_append_data(NF_COMBOBOX(g_user_obj), user_id);

                printf("user_count : %d , user_id[%d] : %s \n", user_cnt, i, user_id);
            }
        }
        else 
    	{
    	    memset(user_id, 0x00, sizeof(user_id));
    	    
    	    if (strlen(ssm_get_cur_id(user_id)) != 0) {
    	        nfui_combobox_append_data(NF_COMBOBOX(g_user_obj), user_id);
    	    }
    	    else {
                nfui_combobox_append_data(NF_COMBOBOX(g_user_obj), "ADMIN");
    	    }
        }
        
        g_sel_obj = g_pwd_obj;
    	nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pwd_obj, TRUE);
    }
    else
    {
        g_sel_obj = g_user_obj;
    	nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pwd_obj, FALSE);
    	
        if (get_user_pwd_mode() == DOUBLE_LOGIN)
        {
            memset(user_id, 0x00, sizeof(user_id));
            ssm_get_cur_id(user_id);
            nfui_nfvklabel_set_string((NFVKLABEL*)g_user_obj, user_id);
            nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_user_obj, FALSE);
            nfui_nfobject_set_size(g_user_obj, 325, 40);
            nfui_nfvklabel_modify_fg((NFVKLABEL*)g_user_obj, COLOR_IDX(139));
            nfui_nfobject_modify_bg(g_user_obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(138));
            nfui_nfobject_use_focus(g_user_obj, NFOBJECT_FOCUS_OFF);
                        
            g_sel_obj = g_pwd_obj;
        	nfui_nfvklabel_set_use_cursor((NFVKLABEL*)g_pwd_obj, TRUE);
        }
    }

    if (ivsc.dfunc.support_find_passwd)
    {
    	if (!strcmp(title, "LOG IN"))
    	{
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, id_label, 15, 52);
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, pw_label, 15, 95);
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, g_user_obj, 185, 52);
    	nfui_nfobject_set_size(g_user_obj, 300, 40);
    	nfui_nfobject_set_size(g_pwd_obj, 300, 40);	
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, g_pwd_obj, 185, 95);

    	obj = nftool_normal_button_create_popup_type2("Forgot Password?", 200);
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, DEFAULT_PWD_SIZE_W-15-200, 95+2);
    	nfui_regi_post_event_callback(obj, forgot_password_button_event_handler);
    	}
    	else
    	{
    		nfui_nffixed_put((NFFIXED*)pwd_fixed, id_label, 92, 52);
    		nfui_nffixed_put((NFFIXED*)pwd_fixed, pw_label, 92, 95);
    		nfui_nffixed_put((NFFIXED*)pwd_fixed, g_user_obj, 262, 52);
    		nfui_nfobject_set_size(g_user_obj, 325, 40);
    		nfui_nfobject_set_size(g_pwd_obj, 325, 40);	
    		nfui_nffixed_put((NFFIXED*)pwd_fixed, g_pwd_obj, 262, 95);
    	}
	}
    else
    {
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, id_label, 92, 52);
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, pw_label, 92, 95);
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, g_user_obj, 262, 52);
    	nfui_nfobject_set_size(g_user_obj, 325, 40);
    	nfui_nfobject_set_size(g_pwd_obj, 325, 40);	
    	nfui_nffixed_put((NFFIXED*)pwd_fixed, g_pwd_obj, 262, 95);
	}
	
	for(i=0; i<KEY_CNT; i++) {
		if(i == 13) 	 g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(backspace, keyVal[i]);	
		else if(i == 27) g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn2_W44, keyVal[i]);	
		else if(i == 39) {
			g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_W140, keyVal[i]);	
			nfui_nfobject_support_multi_lang(g_keyBtn[i], FALSE);
			nfui_nfobject_disable(g_keyBtn[i]);
		}
		else if(i == 50) g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_W92, keyVal[i]);	
		else if(i == 51) g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn_W92, keyVal[i]);	
		else 			 g_keyBtn[i] = (NFOBJECT*)nfui_nfbutton_new_with_param(bgBtn1_W44, keyVal[i]);

		if(i == 13 || i == 27 || i == 39 || i == 50 || i == 51) {
			nfui_nfbutton_set_pango_font((NFBUTTON*)g_keyBtn[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_2);
			if(i == 39) nfui_regi_post_event_callback(g_keyBtn[i], ok_button_event_handler);
			else 		nfui_regi_post_event_callback(g_keyBtn[i], post_func_key_event_cb);
		} else													{
			nfui_nfbutton_set_pango_font((NFBUTTON*)g_keyBtn[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)key_fg_1);
			nfui_regi_post_event_callback(g_keyBtn[i], post_key_event_cb);
		}
		nfui_nfobject_show(g_keyBtn[i]);

		if(i < 14) 		 nfui_nffixed_put((NFFIXED*)pwd_fixed, g_keyBtn[i], 15 + (48 * i), 143);
		else if(i < 28)	 nfui_nffixed_put((NFFIXED*)pwd_fixed, g_keyBtn[i], 15 + (48 * (i % 14)), 191);
		else if(i < 40)  nfui_nffixed_put((NFFIXED*)pwd_fixed, g_keyBtn[i], 15 + (48 * (i % 28)), 239);
		else if(i == 51) nfui_nffixed_put((NFFIXED*)pwd_fixed, g_keyBtn[i], 15 + (48 * ((i % 40) + 1)), 287);
		else 			 nfui_nffixed_put((NFFIXED*)pwd_fixed, g_keyBtn[i], 15 + (48 * (i % 40)), 287);
	}

	// button
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, ok_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 154, 346);
	g_ok_btn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", 192);
	nfui_regi_post_event_callback(obj, cancel_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 352, 346);

	nfui_nfwindow_add((NFWINDOW*)pwd_dlg, pwd_fixed);
	nfui_run_main_event_handler(pwd_dlg);
	nfui_nfobject_show(pwd_dlg);
	nfui_make_key_hierarchy((NFWINDOW*)pwd_dlg);

	if (get_user_pwd_mode() == KEY_UNLOCK) {
		nfui_nfobject_hide(pwd_dlg);
	}

#if defined(_SUPPORT_LOGIN_AUTOHIDE)
	vw_autohide_set_obj(g_curwnd, 10);
#endif

	nfui_page_open(pid, pwd_dlg, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(pid, pwd_dlg);

	return 0;
}

static gint _pin_password_window(NFWINDOW *parent, PAGEID pid, gchar *title)
{
	NFOBJECT *pwd_dlg = NULL;
	NFOBJECT *pwd_fixed = NULL;
	NFOBJECT *ntb;
	NFOBJECT *obj;

	GdkPixbuf *btn_pbuf1[NFOBJECT_STATE_COUNT];
	GdkPixbuf *btn_pbuf2[NFOBJECT_STATE_COUNT];

	GdkPixbuf *switch_icon[NFOBJECT_STATE_COUNT];
	GdkPixbuf *backspace_icon[NFOBJECT_STATE_COUNT];

	ICONPOSITION icon_pos;

	guint btn_fg[] = {COLOR_IDX(704), COLOR_IDX(705), COLOR_IDX(705), COLOR_IDX(704)};

	guint user_cnt = 0;
	UserManageData userdata;
	gchar user_id[USER_ID_STRING_SIZE];
	gint pin_label_w = 42;
	gint pin_label_h = 42;
	gint btn_size_w = 80;
	gint btn_size_h = 52;
	gint btn_col_gap = 16;
	gint btn_row_gap = 16;
	gint pin_len = ivsc.dfunc.pin.digit;

	gint pos_x, pos_y;	
	guint i, j;

	gchar strBuf[64];

	
	if (ivsc.dfunc.pin.digit > 6) g_assert(0);


	btn_pbuf1[0] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_N_BUTTON, btn_size_w, IMG_N_POPUP52_BTN_LEFT, IMG_N_POPUP52_BTN_MIDDLE, IMG_N_POPUP52_BTN_RIGHT);
	btn_pbuf1[1] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_O_BUTTON, btn_size_w, IMG_O_POPUP52_BTN_LEFT, IMG_O_POPUP52_BTN_MIDDLE, IMG_O_POPUP52_BTN_RIGHT);
	btn_pbuf1[2] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_P_BUTTON, btn_size_w, IMG_P_POPUP52_BTN_LEFT, IMG_P_POPUP52_BTN_MIDDLE, IMG_P_POPUP52_BTN_RIGHT);
	btn_pbuf1[3] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_D_BUTTON, btn_size_w, IMG_D_POPUP52_BTN_LEFT, IMG_D_POPUP52_BTN_MIDDLE, IMG_D_POPUP52_BTN_RIGHT);

	btn_pbuf2[0] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_N_BUTTON2, btn_size_w, IMG_N_POPUP52_BTN2_LEFT, IMG_N_POPUP52_BTN2_MIDDLE, IMG_N_POPUP52_BTN2_RIGHT);
	btn_pbuf2[1] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_O_BUTTON, btn_size_w, IMG_O_POPUP52_BTN_LEFT, IMG_O_POPUP52_BTN_MIDDLE, IMG_O_POPUP52_BTN_RIGHT);
	btn_pbuf2[2] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_P_BUTTON, btn_size_w, IMG_P_POPUP52_BTN_LEFT, IMG_P_POPUP52_BTN_MIDDLE, IMG_P_POPUP52_BTN_RIGHT);
	btn_pbuf2[3] = (GdkPixbuf*)nf_ui_create_image_button_no_alpha(MK_IMG_PIN_D_BUTTON, btn_size_w, IMG_D_POPUP52_BTN_LEFT, IMG_D_POPUP52_BTN_MIDDLE, IMG_D_POPUP52_BTN_RIGHT);

	switch_icon[0] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);
	switch_icon[1] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);
	switch_icon[2] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);
	switch_icon[3] = nfui_get_image_from_file((IMG_USER_SWITCH_ICON), NULL);

	backspace_icon[0] = nfui_get_image_from_file((IMG_N_KEY_BACK), NULL);
	backspace_icon[1] = nfui_get_image_from_file((IMG_O_KEY_BACK), NULL);
	backspace_icon[2] = nfui_get_image_from_file((IMG_P_KEY_BACK), NULL);
	backspace_icon[3] = nfui_get_image_from_file((IMG_D_KEY_BACK), NULL);


	memset(g_input_pin_number, 0x00, sizeof(g_input_pin_number));	
	g_input_pin_pos = 0;

	pwd_dlg = (NFOBJECT*)nfui_nfwindow_new(parent, PWD_POS_X(PIN_PWD_SIZE_W), PWD_POS_Y(PIN_PWD_SIZE_H), PIN_PWD_SIZE_W, PIN_PWD_SIZE_H);
	nfui_nfwindow_set_title(pwd_dlg, "USER PASSWORD");
	nfui_regi_post_event_callback(pwd_dlg, pwd_pin_dlg_event_handler);
	g_curwnd = pwd_dlg;

	// fixed
	pwd_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(pwd_fixed, PIN_PWD_SIZE_W, PIN_PWD_SIZE_H);
	nfui_regi_post_event_callback(pwd_fixed, pwd_fixed_event_handler);
	nfui_nfobject_show(pwd_fixed);

	// title label
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(205));
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_tooltip(obj, FALSE);
	nfui_nfobject_set_size(obj, pwd_fixed->width-40, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 20, 7);
	g_titleObj = obj;

	memset(g_pwdTitle, 0x00, sizeof(g_pwdTitle));
	strncpy(g_pwdTitle, title, strlen(title));

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER ID", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 120, 40);		
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 20, 62);

    if (g_pin_id_input_mode == CHOOSE_ID_MODE)
    {
		g_pin_user_obj = nfui_combobox_new(NULL, 0, 0);
		nfui_combobox_set_skin_type(NF_COMBOBOX(g_pin_user_obj), NFCOMBOBOX_TYPE_POPUP_1);
		nfui_nfobject_support_multi_lang(g_pin_user_obj, FALSE);
        nfui_nfobject_set_size(g_pin_user_obj, pwd_fixed->width-160, 40);
		nfui_nfobject_show(g_pin_user_obj);
        nfui_nffixed_put((NFFIXED*)pwd_fixed, g_pin_user_obj, 140, 62);

        if(get_user_pwd_mode() == LOG_ON || get_user_pwd_mode() == BROKEN_RAID) 
        {
    		user_cnt = DAL_get_user_count();
    		for(i=0; i<user_cnt; i++) {
    			memset(&userdata, 0x00, sizeof(UserManageData));
				DAL_get_userManage_data(&userdata, i);
				if (userdata.use_pin) nfui_combobox_append_data(NF_COMBOBOX(g_pin_user_obj), userdata.id);
    		}
    	}
        else 
    	{
    	    memset(user_id, 0x00, sizeof(user_id));    	    
			if (strlen(ssm_get_cur_id(user_id)) == 0) strcpy(user_id, "ADMIN");
			nfui_combobox_append_data(NF_COMBOBOX(g_pin_user_obj), user_id);
        }
    }
    else
    {
		g_pin_user_obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
		nfui_nflabel_set_skin_type((NFLABEL*)g_pin_user_obj, NFTEXTBOX_TYPE_POPUP_INPUT);
		nfui_nflabel_set_align((NFLABEL*)g_pin_user_obj, NFALIGN_LEFT, 8);
        nfui_nfobject_set_size(g_pin_user_obj, pwd_fixed->width-160, 40);
		nfui_nfobject_show(g_pin_user_obj);
		nfui_nfobject_support_multi_lang(g_pin_user_obj, FALSE);
        nfui_nffixed_put((NFFIXED*)pwd_fixed, g_pin_user_obj, 140, 62);
		nfui_regi_post_event_callback(g_pin_user_obj, post_pin_user_event_handler);
    }

	obj = nfui_nflabel_new_with_pango_font("Please enter pin number.", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(340));
	nfui_nfobject_set_size(obj, pwd_fixed->width-32, 40);
	nfui_nflabel_set_align(obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 16, 114);
	nfui_nfobject_show(obj);

	for (i = 0; i < pin_len; i++) 
	{
		obj = (NFOBJECT*)nfui_nfvklabel_new_str("", 1);
		nfui_nfvklabel_set_invisible((NFVKLABEL*)obj, TRUE);
		nfui_nfvklabel_set_pango_font((NFVKLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(221));
		nfui_nfvklabel_set_align((NFVKLABEL*)obj, NFALIGN_CENTER, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(220));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_set_size(obj, pin_label_w, pin_label_h);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, 40+pin_label_w*i+(pwd_fixed->width-80-pin_label_w*pin_len)/(pin_len-1)*i, 160);
		g_pin_label[i] = obj;

		if (i != 0) {
			nfui_nfvklabel_set_use_cursor((NFVKLABEL*)obj, 0);
		}
	}

	for (i = 0; i < 3; i++) 
	{
		for (j = 0; j < 3; j++) 
		{
			memset(strBuf, 0x00, sizeof(strBuf));
			g_sprintf(strBuf, "%d", i*3+j+1);
			obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf1, strBuf);
			nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
			nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
			nfui_nfobject_set_size(obj, btn_size_w, btn_size_h);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, (pwd_fixed->width-btn_size_w*3-btn_col_gap*2)/2+(btn_size_w+btn_col_gap)*j, 240+(btn_size_h+btn_row_gap)*i);
			nfui_regi_post_event_callback(obj, post_pin_key_button_event_handler);
			g_pin_key_obj[i*3+j+1] = obj;
		}
	}

	memset(&icon_pos, 0x00, sizeof(ICONPOSITION));
	icon_pos.x = (btn_size_w-gdk_pixbuf_get_width(switch_icon[0]))/2;
	icon_pos.y = (btn_size_h-gdk_pixbuf_get_height(switch_icon[0]))/2;

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf2, "");
	nfui_nfbutton_set_icon_image((NFBUTTON*)obj, switch_icon);
	nfui_nfbutton_set_icon_position(NF_BUTTON(obj), icon_pos);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
	nfui_nfobject_set_size(obj, btn_size_w, btn_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, (pwd_fixed->width-btn_size_w*3-btn_col_gap*2)/2, 240+(btn_size_h+btn_row_gap)*3);
	nfui_regi_post_event_callback(obj, post_pin_switch_button_event_handler);

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf2, "0");
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
	nfui_nfobject_set_size(obj, btn_size_w, btn_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, (pwd_fixed->width-btn_size_w*3-btn_col_gap*2)/2+(btn_size_w+btn_col_gap), 240+(btn_size_h+btn_row_gap)*3);
	nfui_regi_post_event_callback(obj, post_pin_key_button_event_handler);
	g_pin_key_obj[0] = obj;

	memset(&icon_pos, 0x00, sizeof(ICONPOSITION));
	icon_pos.x = (btn_size_w-gdk_pixbuf_get_width(backspace_icon[0]))/2;
	icon_pos.y = (btn_size_h-gdk_pixbuf_get_height(backspace_icon[0]))/2;

	obj = (NFOBJECT*)nfui_nfbutton_new_with_param(btn_pbuf2, "");
	nfui_nfbutton_set_icon_image((NFBUTTON*)obj, backspace_icon);
	nfui_nfbutton_set_icon_position(NF_BUTTON(obj), icon_pos);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfbutton_set_pango_font(NF_BUTTON(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI), btn_fg);
	nfui_nfobject_set_size(obj, btn_size_w, btn_size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, (pwd_fixed->width-btn_size_w*3-btn_col_gap*2)/2+(btn_size_w+btn_col_gap)*2, 240+(btn_size_h+btn_row_gap)*3);
	nfui_regi_post_event_callback(obj, post_pin_backspace_button_event_handler);

	obj = nftool_normal_button_create_type1("Forgot PIN?", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, pwd_fixed->width/2-164, pwd_fixed->height-64);
	nfui_regi_post_event_callback(obj, post_pin_forgot_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 160);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pwd_fixed, obj, pwd_fixed->width/2+4, pwd_fixed->height-64);
	nfui_regi_post_event_callback(obj, cancel_button_event_handler);

	nfui_nfwindow_add((NFWINDOW*)pwd_dlg, pwd_fixed);
	nfui_run_main_event_handler(pwd_dlg);
	nfui_nfobject_show(pwd_dlg);
	nfui_make_key_hierarchy((NFWINDOW*)pwd_dlg);

	if (get_user_pwd_mode() == KEY_UNLOCK) {
		nfui_nfobject_hide(pwd_dlg);
	}

#if defined(_SUPPORT_LOGIN_AUTOHIDE)
	vw_autohide_set_obj(g_curwnd, 10);
#endif

	nfui_page_open(pid, pwd_dlg, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(pid, pwd_dlg);


	return 0;
}

gboolean VW_UserPwd_Open(NFWINDOW *parent, gchar *title, gint check_auth)
{
	PAGEID pid;
	guint user_cnt = 0;
    UserManageData userdata;
	gchar user_id[USER_ID_STRING_SIZE];
	gint i;
	gint is_pin_check = 0;

	set_user_pwd_return_val(FALSE);

	if(!strcmp(title, "KEY LOCK")) 				set_user_pwd_mode(KEY_LOCK);
	else if(!strcmp(title, "KEY UNLOCK")) 		set_user_pwd_mode(KEY_UNLOCK);
	else if(!strcmp(title, "SHUTDOWN")) 		set_user_pwd_mode(SHUTDOWN);
	else if(!strcmp(title, "REBOOT")) 	    	set_user_pwd_mode(SHUTDOWN);
	else if(!strcmp(title, "PASSWORD CHECK")) 	set_user_pwd_mode(AUTH);
    else if(!strcmp(title, "DELETE RAID"))  	set_user_pwd_mode(BROKEN_RAID);
    else if(!strcmp(title, "DOUBLE LOGIN"))  	set_user_pwd_mode(DOUBLE_LOGIN);
    else if(!strcmp(title, "DOUBLE LOGIN2"))  	set_user_pwd_mode(DOUBLE_LOGIN2);
	else 										set_user_pwd_mode(LOG_ON);

	// reset user auth
	if(get_user_pwd_mode() == LOG_ON) 
	{
		ssm_init_auth();

		if (login_off()) {
            set_user_pwd_return_val(TRUE);		
			return g_retVal;
    	}
	}
	
	if (get_user_pwd_mode() == KEY_LOCK) 
	{
		pid = PGID_USERPWD_AUTOLOGOUT;
		nffunc_led_on(NFLED_RETURN);
	}
	else if (get_user_pwd_mode() == KEY_UNLOCK) 
	{
		pid = PGID_USERPWD_AUTOLOGOUT;
		nffunc_change_led_by_mask(0x0000001F, 0xFFFEFFFF);
	}
	else if (get_user_pwd_mode() == LOG_ON || get_user_pwd_mode() == SHUTDOWN || get_user_pwd_mode() == AUTH || get_user_pwd_mode() == BROKEN_RAID) 
	{
		pid = PGID_USERPWD;
	}

	g_message("%s, %d, pid:%d", __FUNCTION__, __LINE__, pid);

    g_pwd_id_input_mode = DAL_get_id_input_mode();
	g_pin_id_input_mode = DAL_get_pin_id_input_mode();
    g_check_auth = check_auth;

	g_switch_default_password = 0;

	if (get_user_pwd_mode() != DOUBLE_LOGIN && get_user_pwd_mode() != DOUBLE_LOGIN2)
	{
		if (get_user_pwd_mode() == LOG_ON || get_user_pwd_mode() == BROKEN_RAID)
		{
			user_cnt = DAL_get_user_count();
			for (i = 0; i < user_cnt; i++)
			{
				memset(&userdata, 0x00, sizeof(UserManageData));
				DAL_get_userManage_data(&userdata, i);
				if (userdata.use_pin && strlen(userdata.pin_number))
				{
					is_pin_check = 1;
					break;
				}
			}
		}
		else
		{
			memset(user_id, 0x00, sizeof(user_id));
			ssm_get_cur_id(user_id);
			if (strlen(user_id) == 0)
			{
				memset(&userdata, 0x00, sizeof(UserManageData));
				DAL_get_userManage_data(&userdata, 0);
				if (userdata.use_pin && strlen(userdata.pin_number))
					is_pin_check = 1;
			}
			else
			{
				user_cnt = DAL_get_user_count();
				for (i = 0; i < user_cnt; i++)
				{
					memset(&userdata, 0x00, sizeof(UserManageData));
					DAL_get_userManage_data(&userdata, i);
					if (strcmp(userdata.id, user_id) == 0)
					{
						if (userdata.use_pin && strlen(userdata.pin_number))
							is_pin_check = 1;
						break;
					}
				}
			}
		}
	}

	if (is_pin_check) 
	{		
		_pin_password_window(parent, pid, title);
		if (g_switch_default_password) _default_password_window(parent, pid, title, check_auth);
	}
	else {
		_default_password_window(parent, pid, title, check_auth);
	}

	return g_retVal;
}

