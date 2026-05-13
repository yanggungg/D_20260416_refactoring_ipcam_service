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
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfprogressbar.h"
#include "viewers/objects/nfcombobox.h"

#include "viewers/vw_arch_export.h"

//#include "nf_ui_arch_burn.h"

#include "modules/ssm.h"

#include "nf_util_ftp.h"
#include "scm.h"
#include "vw.h"


// TODO: need new vkey
#include "vw_vkeyboard.h"
#include "vw_vkeyboard2.h"
#include "ix_mem.h"
#include "smt.h"
#include "uxm.h"


#define ARCH_EX_WIN_SIZE_W			(678)
#define ARCH_EX_WIN_SIZE_H			(505)   //505 when raw support. 450 not support

#define ARCH_EX_POS_X				((DISPLAY_ACTIVE_WIDTH - ARCH_EX_WIN_SIZE_W)/4*2)
#define ARCH_EX_POS_Y				((DISPLAY_ACTIVE_HEIGHT - ARCH_EX_WIN_SIZE_H)/2)

#define MAX_PW_STRING_SIZE			16
#define MAX_TAG_STRING_SIZE			14
#define MAX_MEMO_STRING_SIZE		24

enum {
	BURNING_BTN,
	ERASE_N_BURNING_BTN,
	CANCEL_BUTTON,
	BUTTON_CNT
};

static NFOBJECT *aeWin;

static BURN_INFO *g_burn_info;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_aeWin;
static NFOBJECT *g_prog;
static NFOBJECT *g_ex_btns[BUTTON_CNT];
static NFOBJECT *g_tag;
static NFOBJECT *g_memo;
static NFOBJECT *g_progress_label;
static NFOBJECT *g_pw;
static NFOBJECT *g_pw_conf;

static NFOBJECT *dev_obj = NULL;
static NFOBJECT *form_obj = NULL;
static NFOBJECT *wait_pop = NULL;

static gint media_cnt;
MEDIA_INFO_T *media_info;

static guint gsrc_id = 0;
static gchar g_str_pw[MAX_PW_STRING_SIZE] = {0,};
static gchar g_str_pw2[MAX_PW_STRING_SIZE] = {0,};
static gint g_pre_idx = 0;



static gint _get_media_index(gint obj_index)
{
	gint i;
	gchar *dev = NULL;

	dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

	for (i = 0; i < media_cnt; i++)
	{
		if(!strcmp(dev, media_info[i].title))
			break;
	}

	if (i == media_cnt)
		return -1;

	return i;
}

static gboolean _check_form_raw_encryption()
{
	gint data_form;

	data_form = nfui_combobox_get_cur_index((NFCOMBOBOX*)form_obj);

	if( data_form == 2 ) // Raw(Encryption)
		return TRUE;

	return FALSE;
}

static void _enable_burning_button()
{
	gint obj_index, media_index;

	if(nfui_nfobject_is_disabled(g_ex_btns[BURNING_BTN])) {
		nfui_nfobject_enable(g_ex_btns[BURNING_BTN]);
		nfui_signal_emit(g_ex_btns[BURNING_BTN], GDK_EXPOSE, FALSE);
	}

	obj_index = nfui_combobox_get_cur_index(dev_obj);
	media_index = _get_media_index(obj_index);

	if (media_index == -1) {
		nfui_nfobject_disable(g_ex_btns[BURNING_BTN]);
		nfui_nfobject_disable(g_ex_btns[ERASE_N_BURNING_BTN]);
	}
	else {
		if(strcmp(media_info[media_index].title, "FTP") == 0)
		{
			nfui_nfobject_enable(g_ex_btns[BURNING_BTN]);
			nfui_nfobject_disable(g_ex_btns[ERASE_N_BURNING_BTN]);
		}	
		else {
			nfui_nfobject_enable(g_ex_btns[BURNING_BTN]);
			nfui_nfobject_enable(g_ex_btns[ERASE_N_BURNING_BTN]);
		}
	}

	nfui_signal_emit(g_ex_btns[BURNING_BTN], GDK_EXPOSE, FALSE);
	nfui_signal_emit(g_ex_btns[ERASE_N_BURNING_BTN], GDK_EXPOSE, FALSE);
}

static void _disable_burning_button()
{
	if(!nfui_nfobject_is_disabled(g_ex_btns[BURNING_BTN])) {
		nfui_nfobject_disable(g_ex_btns[BURNING_BTN]);
		nfui_signal_emit(g_ex_btns[BURNING_BTN], GDK_EXPOSE, FALSE);
	}

	if(!nfui_nfobject_is_disabled(g_ex_btns[ERASE_N_BURNING_BTN])) {
		nfui_nfobject_disable(g_ex_btns[ERASE_N_BURNING_BTN]);
		nfui_signal_emit(g_ex_btns[ERASE_N_BURNING_BTN], GDK_EXPOSE, FALSE);
	}
}

static void _enable_cancel_button()
{
	if(nfui_nfobject_is_disabled(g_ex_btns[CANCEL_BUTTON])) 
	{
		nfui_nfobject_enable(g_ex_btns[CANCEL_BUTTON]);
		nfui_signal_emit(g_ex_btns[CANCEL_BUTTON], GDK_EXPOSE, FALSE);
	}
}

static void _disable_cancel_button()
{
	if(!nfui_nfobject_is_disabled(g_ex_btns[CANCEL_BUTTON])) {
		nfui_nfobject_disable(g_ex_btns[CANCEL_BUTTON]);
		nfui_signal_emit(g_ex_btns[CANCEL_BUTTON], GDK_EXPOSE, FALSE);
	}
}

static void _enable_device_combo()
{
	if(nfui_nfobject_is_disabled(dev_obj)) {
		nfui_nfobject_enable(dev_obj);
		// mantis 11276
		nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	}
}

static void _disable_device_combo()
{
	if(!nfui_nfobject_is_disabled(dev_obj)) {
		nfui_nfobject_disable(dev_obj);
		nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	}
}

static void _enable_tag_label()
{
	if(nfui_nfobject_is_disabled(g_tag)) {
		nfui_nfobject_enable(g_tag);
		nfui_signal_emit(g_tag, GDK_EXPOSE, FALSE);
	}
}

static void _disable_tag_label()
{
	if(!nfui_nfobject_is_disabled(g_tag)) {
		nfui_nfobject_disable(g_tag);
		nfui_signal_emit(g_tag, GDK_EXPOSE, FALSE);
	}
}

static void _enable_dformat()
{
	if(nfui_nfobject_is_disabled(form_obj)) {
		nfui_nfobject_enable(form_obj);
		nfui_signal_emit(form_obj, GDK_EXPOSE, FALSE);
	}
}

static void _disable_dformat()
{
	if(!nfui_nfobject_is_disabled(form_obj)) {
		nfui_nfobject_disable(form_obj);
		nfui_signal_emit(form_obj, GDK_EXPOSE, TRUE);
	}
}

static void _enable_memo_label()
{
	if(nfui_nfobject_is_disabled(g_memo)) {
		nfui_nfobject_enable(g_memo);
		nfui_signal_emit(g_memo, GDK_EXPOSE, FALSE);
	}
}

static void _disable_memo_label()
{
	if(!nfui_nfobject_is_disabled(g_memo)) {
		nfui_nfobject_disable(g_memo);
		nfui_signal_emit(g_memo, GDK_EXPOSE, FALSE);
	}
}

static void _enable_pw_label()
{
    if (g_burn_info->type == NF_ARCH_TYPE_SNAP) return;

    if (nfui_nfobject_is_disabled(g_pw)) {
		nfui_nfobject_enable(g_pw);
		nfui_signal_emit(g_pw, GDK_EXPOSE, FALSE);
    }

    if (nfui_nfobject_is_disabled(g_pw_conf)) {
		nfui_nfobject_enable(g_pw_conf);
		nfui_signal_emit(g_pw_conf, GDK_EXPOSE, FALSE);
    }
}

static void _disable_pw_label()
{
    if (g_burn_info->type == NF_ARCH_TYPE_SNAP) return;

    if (!nfui_nfobject_is_disabled(g_pw)) {
		nfui_nfobject_disable(g_pw);
		nfui_signal_emit(g_pw, GDK_EXPOSE, FALSE);
    }

    if (!nfui_nfobject_is_disabled(g_pw_conf)) {
		nfui_nfobject_disable(g_pw_conf);
		nfui_signal_emit(g_pw_conf, GDK_EXPOSE, FALSE);
    }
}

static int _enable_widgets()
{
	_enable_device_combo();
	_enable_tag_label();
	_enable_dformat();
	_enable_memo_label();
	_enable_burning_button();
	_enable_cancel_button();

	if( _check_form_raw_encryption() )
		_enable_pw_label();

	return 0;
}

static int _reset_progress_widget()
{
	nfui_nflabel_set_text(g_progress_label, "PROGRESS");
	nfui_signal_emit(g_progress_label, GDK_EXPOSE, TRUE);

	nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prog, 0);
	nfui_signal_emit(g_prog, GDK_EXPOSE, FALSE);
	return 0;
}

static int _update_dev_list(NFOBJECT *obj)
{
	int i;
	int dev_cnt = 0;
	MEDIA_TYPE_E mtype;

	if (media_info) scm_free_media_list(media_info);
		nfui_combobox_remove_all(dev_obj);

	nfui_combobox_append_data(dev_obj, "USB(NO DEVICE)");

	media_cnt = 0;	
	media_info = scm_new_media_list(&media_cnt);

	for (i = 0; i < media_cnt; ++i)
    {
		mtype = scm_get_media_type(media_info[i].id);
			
		if (mtype == MTYPE_FTP || mtype == MTYPE_ODD) {
			nfui_combobox_append_data(dev_obj, media_info[i].title);
			++dev_cnt;
		}
		else if (mtype == MTYPE_USB)
    	{	
		    nfui_combobox_remove_data(dev_obj, "USB(NO DEVICE)");
			nfui_combobox_append_data(dev_obj, media_info[i].title);
			++dev_cnt;
		}
	}

	if (dev_cnt == 0) nfui_combobox_append_data(dev_obj, "NO DEVICE");

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);

	return 0;
}

static int _set_focus_cancel_btn(NFOBJECT *cur_focus)
{
   	nfui_set_key_focus(cur_focus, FALSE);
   	nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
    
	nfui_set_key_focus(g_ex_btns[CANCEL_BUTTON], TRUE);
	nfui_signal_emit(g_ex_btns[CANCEL_BUTTON], GDK_EXPOSE, TRUE);

    return 0;
}

static gboolean post_pw_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint x, y;
        gchar buf[64];

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

		if(DAL_get_enahnced_password())
			str = VirtualKey_Open2(g_curwnd, "", "", x, y, MAX_PW_STRING_SIZE, VKEY2_PASSWORD);
		else
			str = VirtualKey_Open(g_curwnd, "", x, y, MAX_PW_STRING_SIZE, VKEY_PASSWORD);

		if(str) {
			g_stpcpy(tag, str);
			ifree(str);

			if (strlen(tag) == 0)
			{
			    if (obj == g_pw) {
                    g_sprintf(buf, "(%s)", lookup_string("Please input PW."));
                    nfui_nflabel_set_text((NFLABEL*)g_pw, buf);
			        nfui_nflabel_set_invisible((NFLABEL*)g_pw, FALSE);
			    }
			    else {
                    g_sprintf(buf, "(%s)", lookup_string("Please input confirm PW."));
                    nfui_nflabel_set_text((NFLABEL*)g_pw_conf, buf);
			        nfui_nflabel_set_invisible((NFLABEL*)g_pw_conf, FALSE);
			    }
			}
			else
			{
			    if (obj == g_pw) {
			        nfui_nflabel_set_text((NFLABEL*)g_pw, tag);
			        nfui_nflabel_set_invisible((NFLABEL*)g_pw, TRUE);
			    }
			    else {
			        if( nfui_nflabel_get_invisible((NFLABEL*)g_pw) == FALSE ) // g_pw empty
					{
						nftool_mbox(g_curwnd, "WARNING", "Please input PW.", NFTOOL_MB_OK);
						return FALSE;
					}
			        else if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_pw), tag) != 0)
			        {
			            nftool_mbox(g_curwnd, "ERROR", "Password doesn't match confirmation.", NFTOOL_MB_OK);
			            return FALSE;
			        }
			        else
			        {
    			        nfui_nflabel_set_text((NFLABEL*)g_pw_conf, tag);
    			        nfui_nflabel_set_invisible((NFLABEL*)g_pw_conf, TRUE);
			        }
			    }
			}
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}

	return FALSE;
}

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

		if (scm_is_enable_choissi_debug())
			str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_TAG_STRING_SIZE, VKEY_NORMAL);
		else
			str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_TAG_STRING_SIZE, VKEY_ALPHANUMERIC);

		if(str) {
			g_stpcpy(tag, str);
			ifree(str);

			nfui_nflabel_set_text((NFLABEL*)obj, tag);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			ssm_get_cur_id(user);
			scm_set_arch_info(tag, user, nfui_nflabel_get_text((NFLABEL*)g_memo));
		}
	}
	
	return FALSE;
}

static gboolean post_memo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

		str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_MEMO_STRING_SIZE, VKEY_NORMAL);

		if (str) {
			nfui_nflabel_set_text((NFLABEL*)obj, str);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			ssm_get_cur_id(user);

			scm_set_arch_info(nfui_nflabel_get_text((NFLABEL*)g_tag), user, str);
			ifree(str);
		}
	}
	
	return FALSE;
}

static int _change_erase_n_burning_button()
{
	gint obj_index, media_index;

	obj_index = nfui_combobox_get_cur_index(dev_obj);
	media_index = _get_media_index(obj_index);

	if (media_index == -1)
	{
		nfui_nfobject_disable(g_ex_btns[BURNING_BTN]);
		nfui_nfobject_disable(g_ex_btns[ERASE_N_BURNING_BTN]);
		
		nfui_signal_emit(g_ex_btns[BURNING_BTN], GDK_EXPOSE, FALSE);
		nfui_signal_emit(g_ex_btns[ERASE_N_BURNING_BTN], GDK_EXPOSE, FALSE);
	}	
	else if (strcmp(media_info[media_index].title, "FTP") == 0)
	{
		nfui_nfobject_enable(g_ex_btns[BURNING_BTN]);
		nfui_nfobject_disable(g_ex_btns[ERASE_N_BURNING_BTN]);

		nfui_signal_emit(g_ex_btns[BURNING_BTN], GDK_EXPOSE, FALSE);		
		nfui_signal_emit(g_ex_btns[ERASE_N_BURNING_BTN], GDK_EXPOSE, FALSE);
	}
	else
	{
		nfui_nfobject_enable(g_ex_btns[BURNING_BTN]);	
		nfui_nfobject_enable(g_ex_btns[ERASE_N_BURNING_BTN]);

		nfui_signal_emit(g_ex_btns[BURNING_BTN], GDK_EXPOSE, FALSE);		
		nfui_signal_emit(g_ex_btns[ERASE_N_BURNING_BTN], GDK_EXPOSE, FALSE);
	}

	return 0;
}

static gboolean 
post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		_change_erase_n_burning_button();
	}

	return FALSE;
}

static gint verify_connecting_ftp()
{
	static ArchFtpData archftp_data;
	FTP_INFO_T req_info;

	memset(&archftp_data, 0x00, sizeof(ArchFtpData));
	DAL_get_archftp_data(&archftp_data);

	memset(&req_info, 0x00, sizeof(FTP_INFO_T));
	
	g_sprintf(req_info.server, "%s", archftp_data.host);
	req_info.port = archftp_data.port;
	g_sprintf(req_info.user, "%s", archftp_data.username);
	g_sprintf(req_info.passwd, "%s", archftp_data.passwd);

	scm_req_test_ftp(&req_info, IRET_SCM_TST_FTP);

	if (!wait_pop) wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	return 0;
}

static gint _start_burning(gint media_index)
{
    gchar *pw;
	int data_form = nfui_combobox_get_cur_index((NFCOMBOBOX*)form_obj);

	scm_set_arch_data_format(data_form);
	
	if (g_burn_info->type != NF_ARCH_TYPE_SNAP)
	{
		pw = nfui_nflabel_get_text((NFLABEL*)g_pw);
		scm_set_arch_data_cipher(data_form, pw, MAX_PW_STRING_SIZE);
	}

	nftool_mbox(g_curwnd, "WARNING", "Do not unplug the power or network cables or\nremove the USB device while backup is in progress.", NFTOOL_MB_OK);

	if(scm_start_burning(g_burn_info->type, g_burn_info->arch_id, media_info[media_index].id, FALSE) != BRN_SUCCESS) 
	{
		nftool_mbox(g_curwnd, "WARNING", "The burning operation occured an error.\nPlease retry.", NFTOOL_MB_OK);
		return FALSE;
	}

    _set_focus_cancel_btn(g_ex_btns[BURNING_BTN]);
	_disable_device_combo();
	_disable_tag_label();
	_disable_dformat();
	_disable_memo_label();
	_disable_burning_button();
	_disable_cancel_button();

	if (g_burn_info->type != NF_ARCH_TYPE_SNAP)
		_disable_pw_label();

	return 0;
}

static gint _start_erase_burning(gint media_index)
{
    gchar *pw;
	int data_form = nfui_combobox_get_cur_index((NFCOMBOBOX*)form_obj);

	scm_set_arch_data_format(data_form);
			
	if (g_burn_info->type != NF_ARCH_TYPE_SNAP)
	{
		pw = nfui_nflabel_get_text((NFLABEL*)g_pw);
		scm_set_arch_data_cipher(data_form, pw, MAX_PW_STRING_SIZE);
	}

	nftool_mbox(g_curwnd, "WARNING", "Do not unplug the power or network cables or\nremove the USB device while backup is in progress.", NFTOOL_MB_OK);

	if(scm_start_burning(g_burn_info->type, g_burn_info->arch_id, media_info[media_index].id, TRUE) != BRN_SUCCESS)
	{
		nftool_mbox(g_curwnd, "WARNING", "The burning operation occured an error.\nPlease retry.", NFTOOL_MB_OK);
		return FALSE;
	}

    _set_focus_cancel_btn((g_ex_btns[ERASE_N_BURNING_BTN]));
	_disable_device_combo();
	_disable_tag_label();
	_disable_dformat();
	_disable_memo_label();
	_disable_burning_button();
	_disable_cancel_button();

	if (g_burn_info->type != NF_ARCH_TYPE_SNAP)
		_disable_pw_label();

	return 0;
}

static gboolean post_form_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED)
    {
        gint idx;
        gchar buf[64];

        idx = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        if (idx == g_pre_idx) return FALSE;

        g_pre_idx = idx;

        if (idx == 2)   //avi
        {
            nfui_nflabel_set_text((NFLABEL*)g_pw, lookup_string("Please input PW."));
            nfui_nflabel_set_text((NFLABEL*)g_pw_conf, lookup_string("Please input confirm PW."));
            nfui_nflabel_set_skin_type((NFLABEL*)g_pw, NFTEXTBOX_TYPE_POPUP_INPUT);
            nfui_nflabel_set_skin_type((NFLABEL*)g_pw_conf, NFTEXTBOX_TYPE_POPUP_INPUT);
            nfui_nfobject_enable(g_pw);
            nfui_nfobject_enable(g_pw_conf);
        }
        else
        {
            nfui_nflabel_set_text((NFLABEL*)g_pw, lookup_string("Supports only RAW(ENCRYPTION) format."));
            nfui_nflabel_set_text((NFLABEL*)g_pw_conf, "");
            nfui_nflabel_set_skin_type((NFLABEL*)g_pw, NFTEXTBOX_TYPE_POPUP_OUTPUT);
            nfui_nflabel_set_skin_type((NFLABEL*)g_pw_conf, NFTEXTBOX_TYPE_POPUP_OUTPUT);
			nfui_nflabel_set_invisible((NFLABEL*)g_pw, FALSE);
            nfui_nflabel_set_invisible((NFLABEL*)g_pw_conf, FALSE);
            nfui_nfobject_disable(g_pw);
            nfui_nfobject_disable(g_pw_conf);
        }
        nfui_make_key_hierarchy(g_curwnd);
        nfui_signal_emit(g_pw, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_pw_conf, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_burn_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	char *tag;

	if(evt->type == GDK_BUTTON_PRESS) 
	{
		gint obj_index, media_index;
		int data_form = nfui_combobox_get_cur_index((NFCOMBOBOX*)form_obj);

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		obj_index = nfui_combobox_get_cur_index(dev_obj);
		media_index = _get_media_index(obj_index);

		if (media_index == -1) return FALSE;

		if(data_form == 2)
		{
			if( nfui_nflabel_get_invisible((NFLABEL*)g_pw) == FALSE ) // g_pw empty
			{
				nftool_mbox(g_curwnd, "WARNING", "Please input PW.", NFTOOL_MB_OK);
				return FALSE;
			}
			else if( nfui_nflabel_get_invisible((NFLABEL*)g_pw_conf) == FALSE ) // g_pw_conf empty
			{
				nftool_mbox(g_curwnd, "WARNING", "Please input confirm PW.", NFTOOL_MB_OK);
				return FALSE;
			}
	        else if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_pw), nfui_nflabel_get_text((NFLABEL*)g_pw_conf)) != 0)
	        {
				nftool_mbox(g_curwnd, "ERROR", "Password doesn't match confirmation.", NFTOOL_MB_OK);
				return FALSE;
	        }
		}

		tag = nfui_nflabel_get_text(g_tag);
		if (strlen(tag) == 0) {
			nftool_mbox(g_curwnd, "WARNING", "The tag name is empty.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		if(strcmp(media_info[media_index].title, "FTP") == 0) {
			verify_connecting_ftp();
			return FALSE;
		}

		_start_burning(media_index);
	}

	return FALSE;
}

static gboolean post_erase_n_burn_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret = -1;
	char *tag;

	if(evt->type == GDK_BUTTON_PRESS)
	{
		gint obj_index, media_index;
		int data_form = nfui_combobox_get_cur_index((NFCOMBOBOX*)form_obj);

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}

		obj_index = nfui_combobox_get_cur_index(dev_obj);
		media_index = _get_media_index(obj_index);

		if (media_index == -1) return FALSE;

       if(data_form == 2)
		{
			if( nfui_nflabel_get_invisible((NFLABEL*)g_pw) == FALSE ) // g_pw empty
			{
				nftool_mbox(g_curwnd, "WARNING", "Please input PW.", NFTOOL_MB_OK);
				return FALSE;
			}
			else if( nfui_nflabel_get_invisible((NFLABEL*)g_pw_conf) == FALSE ) // g_pw_conf empty
			{
				nftool_mbox(g_curwnd, "WARNING", "Please input confirm PW.", NFTOOL_MB_OK);
				return FALSE;
			}
	        else if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_pw), nfui_nflabel_get_text((NFLABEL*)g_pw_conf)) != 0)
	        {
				nftool_mbox(g_curwnd, "ERROR", "Password doesn't match confirmation.", NFTOOL_MB_OK);
				return FALSE;
	        }
		}

		tag = nfui_nflabel_get_text(g_tag);
		if (strlen(tag) == 0) {
			nftool_mbox(g_curwnd, "WARNING", "The tag name is empty.", NFTOOL_MB_OK);
			return FALSE;
		}

		ret = nftool_mbox(g_curwnd, "NOTICE", "All data in the selected storage will be deleted.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);
		if (ret == NFTOOL_MB_CANCEL) return FALSE;
		
		_start_erase_burning(media_index);
	}

	return FALSE;
} 

static gboolean post_cancel_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	int ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		ret = scm_cancel_burning();
		if (ret == -1) {
			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
		}
		else {
			if (!wait_pop) wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
		}
	}

	return FALSE;
} 

static gboolean post_aeWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret = -1;
	NFOBJECT *top = NULL;
	
	switch (evt->type)
	{
		case INFY_MEDIA_STATUS_CHANGED:
			_update_dev_list(dev_obj);
			_change_erase_n_burning_button();
			break;

		case INFY_BURN_EXTRACTING:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->data;
			gchar msg_buf[80];
			
			// work-around
			if (rate > 100) rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (EXTRACTING %d %%)"), rate);

			nfui_nflabel_set_text(g_progress_label, msg_buf);
			nfui_signal_emit(g_progress_label, GDK_EXPOSE, TRUE);

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prog, rate);
			nfui_signal_emit(g_prog, GDK_EXPOSE, FALSE);
		}
		break;
		
		case INFY_BURN_ERASING:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->data;
			gchar msg_buf[80];
			
			// work-around
			if (rate > 100) rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (ERASING %d %%)"), rate);

			nfui_nflabel_set_text(g_progress_label, msg_buf);
			nfui_signal_emit(g_progress_label, GDK_EXPOSE, TRUE);

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prog, rate);
			nfui_signal_emit(g_prog, GDK_EXPOSE, FALSE);
		}
		break;		
		
		case INFY_BURN_PROG:
		{
			guint rate = ((CMM_MESSAGE_T *)data)->data;
			gchar msg_buf[80];
			
			// work-around
			if (rate > 100) rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (BURNING %d %%)"), rate);

			nfui_nflabel_set_text(g_progress_label, msg_buf);
			nfui_signal_emit(g_progress_label, GDK_EXPOSE, TRUE);

			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prog, rate);
			nfui_signal_emit(g_prog, GDK_EXPOSE, FALSE);

			if(evt->type == INFY_BURN_PROG)
				_enable_cancel_button();
		}
		break;

		case INFY_BURN_SUCCESS:
		{
			gchar msg_buf[80];
			gint rate = 100;

			memset(msg_buf, 0x00, sizeof(msg_buf));
			g_sprintf(msg_buf, lookup_string("PROGRESS (BURNING %d %%)"), rate);

			nfui_nflabel_set_text(g_progress_label, msg_buf);
			nfui_signal_emit(g_progress_label, GDK_EXPOSE, TRUE);
		
			nfui_nfprogressbar_set_rate((NFPROGRESSBAR*)g_prog, rate);
			nfui_signal_emit(g_prog, GDK_EXPOSE, FALSE);

			ret = nftool_mbox(g_curwnd, "NOTICE", "It has been successfully burned.", NFTOOL_MB_OK);
			if (ret == NFTOOL_MB_OK) nfui_nfobject_destroy(obj);
		}
			break;

		case INFY_BURN_ERROR:
			{
				BRN_CODE_E status = (BRN_CODE_E)((CMM_MESSAGE_T *)data)->data;
				switch (status) {
				case BRN_CODE_INV_COMMAND:
				case BRN_CODE_INV_PARAM:
				case BRN_CODE_FAIL:
				case BRN_CODE_FAIL_WRITING:
					ret = nftool_mbox(g_curwnd, "WARNING", "The burning operation occured an error.\nPlease retry.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_INV_MEDIA:
				case BRN_CODE_INV_DEV:
					ret = nftool_mbox(g_curwnd, "WARNING", "Can't write on device.\nPlease insert another device.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK)	{
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_NOTERASABLE_MEDIA:
					ret = nftool_mbox(g_curwnd, "WARNING", "Can't erase and continue burning.\nPlease insert another device\nor change burning type.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK)	{
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_FULL_MEDIA:
					ret = nftool_mbox(g_curwnd, "WARNING", "No space to write.\nPlease insert another device.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_NEXT_MEDIA:
					ret = nftool_mbox(g_curwnd, "NOTICE", "The device is full.\nPlease insert another device.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK)	{
						_reset_progress_widget();
						_enable_widgets();
					}
					break;

				case BRN_CODE_CANCELED:
					nfui_nfobject_destroy(obj);
					break;	

				case BRN_CODE_FTP_CONN:
					ret = nftool_mbox(g_curwnd, "WARNING", "It is failed for testing of connection\nto the FTP server.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_FTP_AUTH:
					ret = nftool_mbox(g_curwnd, "WARNING", "It is failed for testing of the authentication.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;

				case BRN_CODE_FTP_FAIL:
					ret = nftool_mbox(g_curwnd, "WARNING", "It is failed for testing of reading and writing.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_FAIL_MULTISESSION:
					ret = nftool_mbox(g_curwnd, "WARNING", "The disk has a problem.\nPlease change or erase the disc.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_NOTSUPP_MULTISESSION:
					ret = nftool_mbox(g_curwnd, "WARNING", "It's unable to support multi session media.\nPlease insert another media.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	

				case BRN_CODE_UNKNOWN_BUG:
				default:
					ret = nftool_mbox(g_curwnd, "WARNING", "Unknown error.", NFTOOL_MB_OK);
					if (ret == NFTOOL_MB_OK) {
						_reset_progress_widget();
						_enable_widgets();
					}
					break;	
				}
			}
			break;

		case INFY_BURN_CANCEL:
//			nfui_nfobject_hide(wait_pop);
            if (wait_pop) {
            	nftool_remove_waitbox(wait_pop);
            	wait_pop = 0;
			}
			scm_end_burning();

			_enable_widgets();
			_reset_progress_widget();
			break;

		case IRET_SCM_TST_FTP:
			{
				gint obj_index, media_index;
				gint ret = ((CMM_MESSAGE_T *)data)->param;

				g_message("%s, %d, IRET_SCM_TST_FTP : %d", __FUNCTION__, __LINE__, ret);

		        if (wait_pop) {
			        nftool_remove_waitbox(wait_pop);
	            	wait_pop = 0;
		        }
		        
				switch (ret) 
				{
					case FTP_CODE_CONN:
						nftool_mbox(g_curwnd, "ERROR", "It has been failed for testing of connection\nto the FTP server.", NFTOOL_MB_OK);
						break;
					case FTP_CODE_AUTH:
						nftool_mbox(g_curwnd, "ERROR", "It has been failed for testing of the authentication.", NFTOOL_MB_OK);
						break;
					case BRN_CODE_FAIL_WRITING:					
					case FTP_CODE_FAIL:
						nftool_mbox(g_curwnd, "ERROR", "It has been failed for testing of reading and writing.", NFTOOL_MB_OK);
						break;
				}				

				if (ret != FTP_SUCCESS) return FALSE;

				obj_index = nfui_combobox_get_cur_index(dev_obj);
				media_index = _get_media_index(obj_index);

				if (media_index == -1) return FALSE;
		
				_start_burning(media_index);		
			}
			break;

		case GDK_DELETE:
			{
				uxm_unreg_imsg_event(g_aeWin, INFY_MEDIA_STATUS_CHANGED);
				uxm_unreg_imsg_event(g_aeWin, INFY_BURN_ERASING);
				uxm_unreg_imsg_event(g_aeWin, INFY_BURN_EXTRACTING);
				uxm_unreg_imsg_event(g_aeWin, INFY_BURN_PROG);
				uxm_unreg_imsg_event(g_aeWin, INFY_BURN_ERROR);
				uxm_unreg_imsg_event(g_aeWin, INFY_BURN_SUCCESS);
				uxm_unreg_imsg_event(g_aeWin, INFY_BURN_CANCEL);
				uxm_unreg_imsg_event(g_aeWin, IRET_SCM_TST_FTP);

				scm_end_burning();
				if (media_info) {
					scm_free_media_list(media_info);
					media_info = 0;
				}

				g_curwnd = 0;

				gtk_main_quit();
			}
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean 
post_ae_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
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

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	int ret;

	if(!nfui_nfobject_is_disabled(g_ex_btns[CANCEL_BUTTON]))
	{
		ret = scm_cancel_burning();
		if (ret != -1) 
		{	
			if (!wait_pop) wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
				return FALSE;
		}
		else
			return TRUE;
	}
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////
//
//
//

void VW_ArchExport_Open(NFWINDOW *parent, BURN_INFO *burn_info)
{
	NFOBJECT *aeFixed;
	NFOBJECT *obj;

	GdkPixbuf *prog_img[4];

	gint i, pos_x, pos_y;
	guint wnd_h;
	gchar buf[64];

	const gchar *formStr[] = {"AVI", "RAW", "RAW(ENCRYPTION)"};
	
	// for test
	memset(g_str_pw, 0x00, sizeof(g_str_pw));
	memset(g_str_pw2, 0x00, sizeof(g_str_pw2));
	g_pre_idx = 0;

	g_burn_info = burn_info;
	wnd_h = ARCH_EX_WIN_SIZE_H;
/*
	if (burn_info->type != NF_ARCH_TYPE_SNAP) wnd_h = ARCH_EX_WIN_SIZE_H + 64;
	else wnd_h = ARCH_EX_WIN_SIZE_H;
*/

	/* window */
	g_aeWin = (NFOBJECT*)nfui_nfwindow_new(parent, ARCH_EX_POS_X, ARCH_EX_POS_Y, ARCH_EX_WIN_SIZE_W, wnd_h);
	g_curwnd = g_aeWin;
	nfui_regi_post_event_callback(g_aeWin, post_aeWin_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_aeWin, returnkey_proc);


	/* fixed */
	aeFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(aeFixed, ARCH_EX_WIN_SIZE_W, ARCH_EX_WIN_SIZE_H);
	nfui_regi_post_event_callback(aeFixed, post_ae_fixed_event_cb);
	nfui_nfobject_show(aeFixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EXPORT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 670, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 4, 4);

	/* subtitle */
	pos_x = 15;
	pos_y = 61;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEVICE NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 310, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);

	pos_y = 85;

	obj = nfui_combobox_new(NULL, 0, 0);
	dev_obj = obj;
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 310, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(dev_obj, post_device_event_handler);

	pos_x = 346;
	pos_y = 61;
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DATA FORMAT", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 310, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);
	if (burn_info->type == NF_ARCH_TYPE_SNAP) nfui_nfobject_hide(obj);

    pos_y = 85;

	obj = nfui_combobox_new(formStr, 1, 0);
	form_obj = obj;
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, 310, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_form_event_handler);
	if (burn_info->type == NF_ARCH_TYPE_SNAP) nfui_nfobject_hide(obj);

    if (burn_info->type != NF_ARCH_TYPE_SNAP)
    {
//        pos_x = 15;
//        pos_y += 57;

    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SET PASSWORD", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    	nfui_nfobject_set_size(obj, 310, 22);
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
//    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);

        pos_y += 24;

    	g_pw = (NFOBJECT*)nfui_nflabel_new_text_box("Supports only RAW(ENCRYPTION) format.", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)g_pw, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    	nfui_nflabel_set_align((NFLABEL*)g_pw, NFALIGN_LEFT, 4);
		nfui_nflabel_set_invisible((NFLABEL*)g_pw, FALSE);
    	nfui_nfobject_set_size(g_pw, 310, 40);
    	nfui_regi_post_event_callback(g_pw, post_pw_event_handler);
		nfui_nfobject_disable(g_pw);
//    	nfui_nfobject_show(g_pw);
    	nfui_nffixed_put((NFFIXED*)aeFixed, g_pw, pos_x, pos_y);

        pos_x = 346;

    	g_pw_conf = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
    	nfui_nflabel_set_skin_type((NFLABEL*)g_pw_conf, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    	nfui_nflabel_set_align((NFLABEL*)g_pw_conf, NFALIGN_LEFT, 4);
		nfui_nflabel_set_invisible((NFLABEL*)g_pw, FALSE);
    	nfui_nfobject_set_size(g_pw_conf, 310, 40);
    	nfui_regi_post_event_callback(g_pw_conf, post_pw_event_handler);
		nfui_nfobject_disable(g_pw_conf);
//    	nfui_nfobject_show(g_pw_conf);
    	nfui_nffixed_put((NFFIXED*)aeFixed, g_pw_conf, pos_x, pos_y);
    }

    pos_x = 15;
    pos_y += 57;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TAG NAME", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 641, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);

    pos_y += 24;

	g_tag = (NFOBJECT*)nfui_nflabel_new_text_box(burn_info->tag, nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_tag, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)g_tag, NFALIGN_LEFT, 4);
	nfui_nfobject_set_size(g_tag, 641, 40);
	nfui_regi_post_event_callback(g_tag, post_tag_event_handler);
	nfui_nfobject_show(g_tag);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_tag, pos_x, pos_y);

	if (strlen(burn_info->tag)) {
		nfui_nflabel_set_skin_type((NFLABEL*)g_tag, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus((NFLABEL*)g_tag, 0);
	}

    pos_y += 57;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MEMO", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 641, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);

	// memo 
	pos_y += 24;

	g_memo = (NFOBJECT*)nfui_nflabel_new_text_box(burn_info->memo, nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_memo, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(g_memo, 641, 100);
	nfui_nflabel_set_align((NFLABEL*)g_memo, NFALIGN_LEFT, 4);
	nfui_regi_post_event_callback(g_memo, post_memo_event_handler);
	nfui_nfobject_show(g_memo);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_memo, pos_x, pos_y);

	if (strlen(burn_info->memo)) {
		nfui_nflabel_set_skin_type((NFLABEL*)g_memo, NFTEXTBOX_TYPE_POPUP_OUTPUT);
		nfui_nfobject_use_focus((NFLABEL*)g_memo, 0);
	}

	// progress label
	//g_progress_label
	pos_y += 100 + 19;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PROGRESS",
	nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	g_progress_label = obj;
	nfui_nfobject_set_size(obj, 600, 22);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, pos_x, pos_y);
	
	// progressbar
	pos_y += 30;

	prog_img[0] = nfui_get_image_from_file((IMG_PROGRESS_BG), NULL);
	prog_img[1] = nfui_get_image_from_file((IMG_PROGRESS_HEAD), NULL);
	prog_img[2] = nfui_get_image_from_file((IMG_PROGRESS_MIDDLE), NULL);
	prog_img[3] = nfui_get_image_from_file((IMG_PROGRESS_TAIL), NULL);

	g_prog = nfui_nfprogressbar_new_with_images(prog_img[0], prog_img[1], prog_img[2], prog_img[3]);
	nfui_nfobject_show(g_prog);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_prog, pos_x, pos_y);

	/* button */
	pos_y += 31;

	g_ex_btns[BURNING_BTN] = nftool_normal_button_create_type1("BURN", 182);
	nfui_regi_post_event_callback(g_ex_btns[BURNING_BTN], post_burn_button_event_cb);
	nfui_nfobject_show(g_ex_btns[BURNING_BTN]);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_ex_btns[BURNING_BTN], pos_x, pos_y);

    pos_x = 203;

	g_ex_btns[ERASE_N_BURNING_BTN] = nftool_normal_button_create_type1("ERASE & BURN", 265);
	nfui_regi_post_event_callback(g_ex_btns[ERASE_N_BURNING_BTN], post_erase_n_burn_button_event_cb);
	nfui_nfobject_show(g_ex_btns[ERASE_N_BURNING_BTN]);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_ex_btns[ERASE_N_BURNING_BTN], pos_x, pos_y);

    pos_x = 474;

	g_ex_btns[CANCEL_BUTTON] = nftool_normal_button_create_type1("CANCEL", 182);
	nfui_regi_post_event_callback(g_ex_btns[CANCEL_BUTTON], post_cancel_button_event_cb);
	nfui_nfobject_show(g_ex_btns[CANCEL_BUTTON]);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_ex_btns[CANCEL_BUTTON], pos_x, pos_y);

	nfui_nfwindow_add((NFWINDOW*)g_aeWin, aeFixed);
	nfui_run_main_event_handler(g_aeWin);
	nfui_nfobject_show(g_aeWin);

	nfui_make_key_hierarchy((NFWINDOW*)g_aeWin);

	_update_dev_list(dev_obj);
	_change_erase_n_burning_button();
	
	uxm_reg_imsg_event(g_aeWin, INFY_MEDIA_STATUS_CHANGED);
	uxm_reg_imsg_event(g_aeWin, INFY_BURN_ERASING);
	uxm_reg_imsg_event(g_aeWin, INFY_BURN_EXTRACTING);
	uxm_reg_imsg_event(g_aeWin, INFY_BURN_PROG);
	uxm_reg_imsg_event(g_aeWin, INFY_BURN_ERROR);
	uxm_reg_imsg_event(g_aeWin, INFY_BURN_SUCCESS);
	uxm_reg_imsg_event(g_aeWin, INFY_BURN_CANCEL);
	uxm_reg_imsg_event(g_aeWin, IRET_SCM_TST_FTP);

	smt_set_service(SMT_ARCHIVE_BURN);
	nfui_page_open(PGID_ARCH_BURN, g_aeWin, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_ARCH_BURN, g_aeWin);
	smt_return_to_previous();
}
