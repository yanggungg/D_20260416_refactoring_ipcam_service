#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "services/scm.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nftimespin.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "objects/nfspinbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"


#include "vw_archiving.h"
#include "vw_vkeyboard.h"

#include "nf_util_ftp.h"
#include "ix_mem.h"
#include "ssm.h"


//MAX_MEMO_STRING_SIZE
#define AR_T4_MAX_LANGTH		24

// CONNECTION TEST BUTTON
#define AR_T4_CONN_BTN_SIZE		(230)
#define AR_T4_CONN_BTN_X		(138+479-AR_T4_CONN_BTN_SIZE)
#define AR_T4_CONN_BTN_Y		(277)

// CONNECTION TEST BUTTON

#define MAX_STRING_SIZE			(64)
#define	SERVER_STRING_SIZE		(63)
#define	PORT_STRING_SIZE		(5)
#define	ID_STRING_SIZE			(63)
#define	PW_STRING_SIZE			(16)
#define	DIRECTORY_STRING_SIZE	(63)

//#define _SUPPORT_PERIOD


enum {
	HOST_NAME = 0,
	PORT,
	USER_NAME,
	PASSWORD,
	DIR_PATH,
	PERIOD,
};

static ArchFtpData archftp_data;
static ArchFtpData org_archftp_data;

static NFOBJECT *account[6];
static NFWINDOW *g_curwnd = 0;

//static FtpData org_ftpData;

static gboolean
arch_tab4_pre_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE:
		break;

		case GDK_DELETE:

		break;

		default :

		break;
	}

	return FALSE;
}

static gboolean
art4_account_label_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i;
	gchar *text_value;
	guint xpos;
	guint ypos;

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
		gchar *strTemp;
		guint x, y;
		gint string_size;
		gchar strbuf[256];

		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);

		x += top->x + obj->width + 4;
		y += top->y;

		if (obj == account[PORT]) {
			gint numTemp;
			gint numRet;

			numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
			numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

			if (numRet == -1) return FALSE;

            memset(strbuf, 0x00, sizeof(strbuf));
            sprintf(strbuf, lookup_string("The setting range is exceeded. (%d ~ %d)"), 1, 65535);
            
    		if (numRet == 0) 
    		{
    			nftool_mbox(g_curwnd, "NOTICE", strbuf, NFTOOL_MB_OK);
    		}
			else
			{
				nfui_nflabel_set_number((NFLABEL*)obj, numRet);
				nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);
			}
		}
		else if (obj == account[PASSWORD])
		{
			gint i;

			string_size = 15;
			strTemp = VirtualKey_Open(g_curwnd, archftp_data.passwd, x, y, string_size, VKEY_PASSWORD);

			if (strTemp)
			{
				sprintf(archftp_data.passwd, "%s", strTemp);

				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

				ifree(strTemp);
				strTemp = NULL;
			}
		}
		else
		{
			if (obj == account[DIR_PATH])
			{
				string_size = 31;
				strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, string_size, VKEY_FTP_DIR);
			}
			else
			{
				string_size = 31;
				strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, string_size, VKEY_NORMAL);
			}

			if (strTemp)
			{
				nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
				nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);

				ifree(strTemp);
				strTemp = NULL;
			}
		}
	}

	return FALSE;
}

static gboolean
art4_conntest_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT *wait_pop = NULL;
	FTP_INFO_T req_info;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
	    }

		memset(&req_info, 0x00, sizeof(FTP_INFO_T));
		g_sprintf(req_info.server, "%s", nfui_nflabel_get_text((NFLABEL*)account[HOST_NAME]));
      	req_info.port = nfui_nflabel_get_number((NFLABEL*)account[PORT]);
		g_sprintf(req_info.user, "%s", nfui_nflabel_get_text((NFLABEL*)account[USER_NAME]));
		g_sprintf(req_info.passwd, "%s", archftp_data.passwd);

		scm_req_test_ftp(&req_info, IRET_SCM_TST_FTP);
		if (!wait_pop) wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
	}
	else if (evt->type == IRET_SCM_TST_FTP)
	{
		gint ret = ((CMM_MESSAGE_T *)data)->param;

		g_message("%s, %d, IRET_SCM_TST_FTP : %d", __FUNCTION__, __LINE__, ret);

        if (wait_pop) {
	        nftool_remove_waitbox(wait_pop);
        	wait_pop = 0;
        }

		switch (ret)
		{
			case FTP_SUCCESS:
				nftool_mbox(g_curwnd, "SUCCESS", "All of configurations are well set.\nIt's possible to upload to the server.", NFTOOL_MB_OK);
				break;
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
	}
	else if (evt->type == GDK_DELETE)
	{
		uxm_unreg_imsg_event(obj, IRET_SCM_TST_FTP);
	}

	return FALSE;

}

static gboolean
art4_cancel_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
	}

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		guint i;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
		}

		g_memmove(&archftp_data, &org_archftp_data, sizeof(ArchFtpData));

		nfui_nflabel_set_text(account[HOST_NAME], archftp_data.host);
		nfui_nflabel_set_number(account[PORT], archftp_data.port);
		nfui_nflabel_set_text(account[USER_NAME], archftp_data.username);

		gchar strTemp[20];

		memset(strTemp, 0x00, sizeof(strTemp));
		sprintf(strTemp, "%s", archftp_data.passwd);

		nfui_nflabel_set_text((NFLABEL*)account[PASSWORD], strTemp);

		if(archftp_data.is_anon)
		{
            nfui_nflabel_set_text((NFLABEL*)account[USER_NAME], "");
            nfui_nflabel_set_text((NFLABEL*)account[PASSWORD], "");

          	nfui_nfobject_disable(account[USER_NAME]);
          	nfui_nfobject_disable(account[PASSWORD]);
		}
		else
		{
          	nfui_nfobject_enable(account[USER_NAME]);
          	nfui_nfobject_enable(account[PASSWORD]);
		}

		nfui_nflabel_set_text(account[DIR_PATH], archftp_data.dir_path);
#ifdef _SUPPORT_PERIOD
		nfui_combobox_set_index(account[PERIOD], archftp_data.period);
#endif




#ifdef _SUPPORT_PERIOD
		for(i=0; i<=PERIOD; i++)
#else
		for(i=0; i<PERIOD; i++)
#endif

		{
			nfui_signal_emit(account[i], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;

}

static gboolean
art4_apply_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
		{
			return FALSE;
	    }

		g_sprintf(archftp_data.host, "%s", nfui_nflabel_get_text((NFLABEL*)account[HOST_NAME]));
      	archftp_data.port = nfui_nflabel_get_number((NFLABEL*)account[PORT]);
    	g_sprintf(archftp_data.username, "%s", nfui_nflabel_get_text((NFLABEL*)account[USER_NAME]));
    	//g_sprintf(archftp_data.passwd, "%s",
		//				nfui_nflabel_get_text((NFLABEL*)account[PASSWORD]));
    	g_sprintf(archftp_data.dir_path, "%s", nfui_nflabel_get_text((NFLABEL*)account[DIR_PATH]));

#ifdef _SUPPORT_PERIOD
		archftp_data.period = nfui_combobox_get_cur_index(account[PERIOD]);
#endif

		if(memcmp(&org_archftp_data, &archftp_data, sizeof(ArchFtpData)))
		{
			g_memmove(&org_archftp_data, &archftp_data, sizeof(ArchFtpData));
			DAL_set_archftp_data(&archftp_data);
			DAL_save_setup_db(NFSETUP_WINDOW_NETWORK);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");

			//sysarch_set_changeflag(1);
		}

	}

	return FALSE;
}

static gboolean
art4_close_btn_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		mb_type ret;

		g_sprintf(archftp_data.host, "%s", nfui_nflabel_get_text((NFLABEL*)account[HOST_NAME]));
      	archftp_data.port = nfui_nflabel_get_number((NFLABEL*)account[PORT]);
    	g_sprintf(archftp_data.username, "%s", nfui_nflabel_get_text((NFLABEL*)account[USER_NAME]));
    	//g_sprintf(archftp_data.passwd, "%s",
		//				nfui_nflabel_get_text((NFLABEL*)account[PASSWORD]));
    	g_sprintf(archftp_data.dir_path, "%s", nfui_nflabel_get_text((NFLABEL*)account[DIR_PATH]));

#ifdef _SUPPORT_PERIOD
		archftp_data.period = nfui_combobox_get_cur_index(account[PERIOD]);
#endif


		if( !memcmp(&org_archftp_data, &archftp_data, sizeof(ArchFtpData)) )
		{
			VW_Archiving_Close();
			return FALSE;
		}

		ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?",
							NFTOOL_MB_OKCANCEL);

		if(ret == NFTOOL_MB_OK)
		{
			g_memmove(&org_archftp_data, &archftp_data, sizeof(ArchFtpData));
			DAL_set_archftp_data(&archftp_data);
		}
		else if(ret == NFTOOL_MB_CANCEL)
		{
			g_memmove(&org_archftp_data, &archftp_data, sizeof(ArchFtpData));
		}

		VW_Archiving_Close();

	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}


////////////////////////////////////////////////////////////////////
//
//
//
//

void vw_init_arch_tab4_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *obj;
	gint i, j;
	gchar *strCh[4] =  {"2 WEEKS",
						"3 WEEKS",
						"4 WEEKS",
						"5 WEEKS"};

	g_curwnd = nfui_nfobject_get_top(parent);

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_H_INNER_W, MENU_H_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);


// TITLE BAR LABEL
	const gchar *title_name[] = {"FTP", "E-MAIL"};
	gint title_xpos[2] = {138, 138};
	gint title_ypos[2] = {0, 368};
	int k = 1;

#ifdef _SUPPORT_PERIOD
	k = 2;
#endif

	for( i = 0 ; i < k ; i++ )
	{
		obj = nfui_nfimage_new(IMG_TITLE_BG);
		nfui_nfimage_set_text((NFIMAGE*)obj, title_name[i]);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, title_xpos[i], title_ypos[i]);
	}

// FTP ACCOUNT LABEL
	const gchar *ftp_name[] = { "HOST NAME",
								"PORT",
								"USER NAME",
								"PASSWORD",
								"DIRECTORY"};
	guint ftp_xpos = 138;
	guint ftp_ypos = 61;

	memset(&archftp_data, 0x00, sizeof(ArchFtpData));
	memset(&org_archftp_data, 0x00, sizeof(ArchFtpData));

	DAL_get_archftp_data(&archftp_data);
	//archftp_data.is_anon = FALSE;

	for( i  = 0 ; i < 5 ; i++ )
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(ftp_name[i],
									nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_set_size(obj, 220, 40);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, ftp_xpos, ftp_ypos);
		ftp_ypos += 42;
	}

	account[HOST_NAME] = nfui_nflabel_new_text_box(archftp_data.host, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)account[HOST_NAME], NFTEXTBOX_TYPE_INPUT);

	account[PORT] = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)account[PORT], NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_use_number((NFLABEL*)account[PORT], TRUE);
	nfui_nflabel_set_number((NFLABEL*)account[PORT], archftp_data.port);

	account[USER_NAME] = nfui_nflabel_new_text_box(archftp_data.username, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)account[USER_NAME], NFTEXTBOX_TYPE_INPUT);

	account[PASSWORD] = nfui_nflabel_new_text_box(archftp_data.passwd, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)account[PASSWORD], NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_invisible((NFLABEL *) account[PASSWORD], TRUE );

	account[DIR_PATH] = nfui_nflabel_new_text_box(archftp_data.dir_path, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)account[DIR_PATH], NFTEXTBOX_TYPE_INPUT);

	if(archftp_data.is_anon)
	{
        nfui_nflabel_set_text((NFLABEL*)account[USER_NAME], "");
        nfui_nflabel_set_text((NFLABEL*)account[PASSWORD], "");

		nfui_nfobject_disable(account[USER_NAME]);
		nfui_nfobject_disable(account[PASSWORD]);
	}
	else
	{
		nfui_nfobject_enable(account[USER_NAME]);
		nfui_nfobject_enable(account[PASSWORD]);
	}

	ftp_ypos = 61;
	for(i=0 ; i < 5; i++)
	{
		nfui_nflabel_set_align((NFLABEL*)account[i], NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(account[i], 271, 40);
		nfui_nfobject_use_focus(account[i], NFOBJECT_FOCUS_ON);
		nfui_nfobject_show(account[i]);
		nfui_nffixed_put((NFFIXED*)content_fixed, account[i], ftp_xpos + 250, ftp_ypos);
		nfui_regi_post_event_callback(account[i], art4_account_label_handler);
		ftp_ypos += 42;
	}

	nfui_nfobject_set_data(account[HOST_NAME], "string_size",
									GINT_TO_POINTER(SERVER_STRING_SIZE));
	nfui_nfobject_set_data(account[PORT], "string_size",
									GINT_TO_POINTER(PORT_STRING_SIZE));
	nfui_nfobject_set_data(account[USER_NAME], "string_size",
									GINT_TO_POINTER(ID_STRING_SIZE));
	nfui_nfobject_set_data(account[PASSWORD], "string_size",
									GINT_TO_POINTER(PW_STRING_SIZE));
	nfui_nfobject_set_data(account[DIR_PATH], "string_size",
									GINT_TO_POINTER(DIRECTORY_STRING_SIZE));
	nfui_nfobject_support_multi_lang(account[DIR_PATH], FALSE);

	// FTP CONNECT TEST BUTTON
	obj = nftool_normal_button_create_type3("CONNECTION TEST", AR_T4_CONN_BTN_SIZE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, AR_T4_CONN_BTN_X, AR_T4_CONN_BTN_Y);
	nfui_regi_post_event_callback(obj, art4_conntest_btn_handler);
	uxm_reg_imsg_event(obj, IRET_SCM_TST_FTP);

#ifdef _SUPPORT_PERIOD
// this ver doen't support this. skshin
	// DOWNLOAD PERIOD
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DOWNLOAD PERIOD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(obj, 271, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, 138, 429);

	account[PERIOD] = nfui_combobox_new(strCh, 4, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(account[PERIOD]), NFCOMBOBOX_TYPE_1);
	nfui_nfobject_set_size(account[PERIOD], 200, 40);
	nfui_combobox_set_index(account[PERIOD], archftp_data.period);
	nfui_nfobject_show(account[PERIOD]);
	nfui_nffixed_put((NFFIXED*)content_fixed, account[PERIOD], 409, 429);
#endif

	// below button
	const gchar *below_btn[] = {"CANCEL", "APPLY", "CLOSE"};

	obj = nftool_normal_button_create_type1(below_btn[0], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R3_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, art4_cancel_btn_handler);

	obj = nftool_normal_button_create_type1(below_btn[1], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, art4_apply_btn_handler);

	obj = nftool_normal_button_create_type2(below_btn[2], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, art4_close_btn_handler);

	// event handler
	nfui_regi_pre_event_callback(content_fixed, arch_tab4_pre_fixed_event_handler);
	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(&org_archftp_data, &archftp_data, sizeof(ArchFtpData));

}

gboolean vw_arch_tab4_out_handler()
{
	mb_type ret;

	g_sprintf(archftp_data.host, "%s", nfui_nflabel_get_text((NFLABEL*)account[HOST_NAME]));
  	archftp_data.port = nfui_nflabel_get_number((NFLABEL*)account[PORT]);
	g_sprintf(archftp_data.username, "%s", nfui_nflabel_get_text((NFLABEL*)account[USER_NAME]));
	//g_sprintf(archftp_data.passwd, "%s",
	//				nfui_nflabel_get_text((NFLABEL*)account[PASSWORD]));
	g_sprintf(archftp_data.dir_path, "%s", nfui_nflabel_get_text((NFLABEL*)account[DIR_PATH]));

#ifdef _SUPPORT_PERIOD
	archftp_data.period = nfui_combobox_get_cur_index(account[PERIOD]);
#endif


	if( !memcmp(&org_archftp_data, &archftp_data, sizeof(ArchFtpData)) )
	{
		return FALSE;
	}

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?",
						NFTOOL_MB_OKCANCEL);

	if(ret == NFTOOL_MB_OK)
	{
		g_memmove(&org_archftp_data, &archftp_data, sizeof(ArchFtpData));
		DAL_set_archftp_data(&archftp_data);
	}
	else if(ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(&org_archftp_data, &archftp_data, sizeof(ArchFtpData));
	}

	return FALSE;
}

