#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfspinbutton.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"

#include "vw_sys_net_main.h"
#include "vw_sys_net_email.h"

#include "nf_util_mail.h"
#include "vw_vkeyboard.h"
#include "scm.h"
#include "ix_mem.h"


#define DEFAULT_SERVER_NAME     "smtp.dvrlink.net"
#define DEFAULT_USER                    "test"
#define DEFAULT_PASSWORD           "test4smtp"
#define DEFAULT_SMTPPORT            587

#define NE_LABLE_LEFT			(28)
#define NE_LABLE_TOP			(42)

#define NE_LABLE_WIDTH			(guint)(DISPLAY_IS_D1 ? 230:360)
#define NE_LABLE_HEIGHT			(guint)(DISPLAY_IS_D1 ? 16:40)

#define NE_CELL_WIDTH			(guint)(DISPLAY_IS_D1 ? 150:261)
#define NE_CELL_HEIGHT			NE_LABLE_HEIGHT

#define NE_COL_SPACE				(guint)(DISPLAY_IS_D1 ? 4:2)
#define NE_ROW_SPACE				(guint)(DISPLAY_IS_D1 ? 2:1)
#define NE_LABLE_HEIGHT_WITH_SPACE	(guint)(DISPLAY_IS_D1 ? 18:40)

#define NE_TEST_BUTTON_WIDTH	(guint)(DISPLAY_IS_D1 ? 80:158)

#define MAX_STRING_SIZE			(64)
#define SERVER_STRING_SIZE		(63)
#define PORT_STRING_SIZE		(5)
#define ID_STRING_SIZE			(63)
#define PW_STRING_SIZE			(16)
#define EMAIL_STRING_SIZE		(63)

#define TEST_EMAIL_SUBJECT_STRING       "NVR TEST MAIL"
#define TEST_EMAIL_CONTENTS_STRING    "This Mail is test mail by NVR machine"

enum {
	NE_ROW_SERVER = 0,
	NE_ROW_PORT,
	NE_ROW_SECURITY,
	NE_ROW_USER,
	NE_ROW_PASSWORD,
#if defined(_SUPPORT_DUAL_SMTPSERVER)
	NE_ROW_INDIVIDUAL,
#endif	
	NE_ROW_FROM,
	NE_ROW_TEST_MAIL,
#if defined(DEFALT_E_MAIL_SERVER)
	NE_ROW_DEF_SERVER,
#endif
	NUM_NE_ROWS
};

#if defined(DEFALT_E_MAIL_SERVER)
#define TEST_BUTTON_ROW_IDX     (10)
#else 
#define TEST_BUTTON_ROW_IDX     (8)
#endif

#define MAX_SERVER_CNT		(2)

#if defined(_SUPPORT_DUAL_SMTPSERVER)
#define SUPPORT_SERVER_CNT	(2)
#else
#define SUPPORT_SERVER_CNT	(1)
#endif

static EmailData emaildata[MAX_SERVER_CNT];
static EmailData org_emaildata[MAX_SERVER_CNT];

static NFOBJECT *g_value_obj[MAX_SERVER_CNT][NUM_NE_ROWS] = {0, };

static guint g_mail_test_res = 0;
static NF_MAIL_SEND_STATUS res_mail_send_status = NF_MAIL_SEND_STATUS_START;
static NFWINDOW *g_curwnd = 0;
static NFOBJECT *wait_pop = NULL;
static GError *test_err = NULL;
static gchar error_message[255];


static gboolean pre_mainbg_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
			GdkDrawable *drawable;
			GdkGC *gc;
			guint x, y;

			drawable = nfui_nfobject_get_window(obj);
			gc = nfui_nfobject_get_gc(obj);

			nfui_nfobject_get_offset(obj, &x, &y);
		}
		break;

		default :
			break;
	}

	return FALSE;
}

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint x, y;
		gint string_size;

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

		string_size = GPOINTER_TO_INT((gint)nfui_nfobject_get_data(obj, "string_size"));

		if ((obj == g_value_obj[0][NE_ROW_PORT]) ||(obj == g_value_obj[1][NE_ROW_PORT]))
		{
			gint numTemp;
			gint numRet;

			numTemp = nfui_nflabel_get_number((NFLABEL*)obj);
			numRet = NumberKey_Open(g_curwnd, numTemp, x, y, 65535);

			if (numRet == -1) return FALSE;

			nfui_nflabel_set_number((NFLABEL*)obj, numRet);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		}
		else 
		{
			gchar *strTemp;

			if ((obj == g_value_obj[0][NE_ROW_TEST_MAIL]) || (obj == g_value_obj[1][NE_ROW_TEST_MAIL]))
			{
				strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, string_size, VKEY_MAIL);
			}
			else if ((obj == g_value_obj[0][NE_ROW_PASSWORD]) || (obj == g_value_obj[1][NE_ROW_PASSWORD]))
			{
				strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, string_size, VKEY_PASSWORD);
			}
			else
			{
				strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, string_size, VKEY_NORMAL);
			}

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

static void mail_send_test_cb(gpointer user_data, NF_MAIL_SEND_STATUS status, const char *svr_msg)
{
	gchar* result[] = {"success", "fail", "start"};

	g_message("################################################################################################");
	g_message("######################################%s : status[%d]-->%s", __FUNCTION__, status, result[status]);
	g_message("################################################################################################");

	res_mail_send_status = status;
       if(svr_msg) g_sprintf(error_message, "%s", svr_msg);
}


static gboolean mail_send_test_result_update(gpointer data)
{
      gboolean res = FALSE;
      
      res = (gboolean)data;

	if(res_mail_send_status == NF_MAIL_SEND_STATUS_SUCCESS)
	{
          	if(wait_pop)
          	{
          		nftool_remove_waitbox(wait_pop);
          		wait_pop = NULL;
          	}

              res_mail_send_status = NF_MAIL_SEND_STATUS_START;
	
		NFUTIL_THREADS_ENTER();

//		nftool_mbox(g_curwnd, "NOTICE", "E-mail sending test succeed.", NFTOOL_MB_OK);
		nftool_mbox(g_curwnd, "NOTICE", "Test E-mail sending succeeded.", NFTOOL_MB_OK);

          	NFUTIL_THREADS_LEAVE();
              
		return FALSE;
	}
	else if(res == FALSE || res_mail_send_status == NF_MAIL_SEND_STATUS_FAILED)
	{
          	if(wait_pop)
          	{
          		nftool_remove_waitbox(wait_pop);
          		wait_pop = NULL;
          	}

             res_mail_send_status = NF_MAIL_SEND_STATUS_START;

	
		NFUTIL_THREADS_ENTER();

		//nftool_mbox_2_line("NOTICE", "E-mail sending test was failed.", error_message, NFTOOL_MB_OK);
             nftool_mbox(g_curwnd, "NOTICE", "E-mail sending test is failed.", NFTOOL_MB_OK);

          	NFUTIL_THREADS_LEAVE();
            
		return FALSE;		
	}


	return TRUE;
}

static gboolean post_testbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		NF_MAIL_SEND_SERVER server;
		NF_MAIL_SEND_CONTENT cont;
		gint cmp_val;
		guint port_val;
		guint ret = 0;
		gint server_idx;
		gboolean default_server_on = FALSE;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(&server, 0x00, sizeof(server));
		memset(&cont, 0x00, sizeof(cont));

		server_idx = GPOINTER_TO_INT((gint)nfui_nfobject_get_data(obj, "server_num"));
		
#if defined(DEFALT_E_MAIL_SERVER)
		default_server_on = nfui_check_button_get_active((NFCHECKBUTTON*)g_value_obj[server_idx][NE_ROW_DEF_SERVER]);
#endif
		if (default_server_on == TRUE)
		{
			port_val = nfui_nflabel_get_number((NFLABEL*)g_value_obj[server_idx][NE_ROW_PORT]);

			if (port_val <= 0 || port_val > 0xffff)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please input valid port number.", NFTOOL_MB_OK);
				return FALSE;
			}
		}
		else
		{
			cmp_val = strcmp(nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_SERVER]), "");
			
			if (cmp_val == 0)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please input server.", NFTOOL_MB_OK);
				return FALSE;
			}

			port_val = nfui_nflabel_get_number((NFLABEL*)g_value_obj[server_idx][NE_ROW_PORT]);
		
			if (port_val <= 0 || port_val > 0xffff)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please input valid port number.", NFTOOL_MB_OK);
				return FALSE;
			}

			cmp_val = strcmp(nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_USER]), "");
			
			if (cmp_val == 0)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please input user name.", NFTOOL_MB_OK);
				return FALSE;
			}

			cmp_val = strcmp(nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_PASSWORD]), "");
		
			if (cmp_val == 0)
			{
				nftool_mbox(g_curwnd, "NOTICE", "Please enter a password.", NFTOOL_MB_OK);
				return FALSE;
			}
		}

		if (nf_mail_send_check_email(nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_TEST_MAIL])) == FALSE)
		{
			nftool_mbox(g_curwnd, "NOTICE", "Please input your email address\nfor test.", NFTOOL_MB_OK);
			return FALSE;
		}
		
		if (default_server_on == TRUE)
		{
			g_sprintf(server.smtp_server, "%s", DEFAULT_SERVER_NAME);
			server.smtp_port = DEFAULT_SMTPPORT;
			server.smtp_security = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[server_idx][NE_ROW_SECURITY]);                
			g_sprintf(server.smtp_user, "%s", DEFAULT_USER);
			g_sprintf(server.smtp_passwd,"%s", DEFAULT_PASSWORD);
			g_sprintf(cont.from, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_FROM]));
			g_sprintf(cont.to[0], "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_TEST_MAIL]));
		}
		else
		{
			g_sprintf(server.smtp_server, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_SERVER]));
			server.smtp_port = nfui_nflabel_get_number((NFLABEL*)g_value_obj[server_idx][NE_ROW_PORT]);
			server.smtp_security = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[server_idx][NE_ROW_SECURITY]);                
			g_sprintf(server.smtp_user, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_USER]));
			g_sprintf(server.smtp_passwd,"%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_PASSWORD]));
			g_sprintf(cont.from, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_FROM]));
			g_sprintf(cont.to[0], "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_TEST_MAIL]));
		}
		
		cont.to_cnt = 1;
		g_sprintf(cont.subject, TEST_EMAIL_SUBJECT_STRING);
		g_sprintf(cont.contents, TEST_EMAIL_CONTENTS_STRING);
		cont.cb_func = mail_send_test_cb;
		cont.cb_arg = &cont;
    
		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "");

		ret = nf_mail_send_test(&server, &cont, &test_err);
		memset(error_message, 0x00, sizeof(error_message));              

		if (ret == FALSE) 
		{
			if (test_err) {
				g_sprintf(error_message, lookup_string("error code: %d"), test_err->code);
				g_error_free (test_err);
				test_err = NULL;
			}
		}
		
		g_timeout_add(333, mail_send_test_result_update, GUINT_TO_POINTER(ret));		
	}

	return FALSE;
}
	
static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		guint i, j;
		gboolean default_server_on = FALSE;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_memmove(emaildata, org_emaildata, sizeof(EmailData)*MAX_SERVER_CNT);

		for (i = 0; i < SUPPORT_SERVER_CNT; i++)
		{
#if defined(DEFALT_E_MAIL_SERVER)
			default_server_on = get_is_default_server_on(emaildata[i]);
			nfui_check_button_set_active((NFCHECKBUTTON*)g_value_obj[i][NE_ROW_DEF_SERVER], default_server_on);
#endif

			if(default_server_on == TRUE)
			{
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_SERVER], "");
				nfui_nflabel_set_number((NFLABEL*)g_value_obj[i][NE_ROW_PORT], (gint)emaildata[i].port);
				nfui_spin_button_set_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY], emaildata[i].security);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_USER], "");
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_PASSWORD],"");
#if defined(_SUPPORT_DUAL_SMTPSERVER)
				nfui_spin_button_set_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL], emaildata[i].individual);
#endif
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM], emaildata[i].from);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL], emaildata[i].testMail);       

				nfui_nfobject_disable(g_value_obj[i][NE_ROW_SERVER]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_PORT]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_SECURITY]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_USER]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_PASSWORD]);            
			}
			else
			{
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_SERVER], emaildata[i].server);
				nfui_nflabel_set_number((NFLABEL*)g_value_obj[i][NE_ROW_PORT], (gint)emaildata[i].port);
				nfui_spin_button_set_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY], emaildata[i].security);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_USER], emaildata[i].user);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_PASSWORD], emaildata[i].passwd);
#if defined(_SUPPORT_DUAL_SMTPSERVER)
				nfui_spin_button_set_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL], emaildata[i].individual);
#endif
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM], emaildata[i].from);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL], emaildata[i].testMail);       

				nfui_nfobject_enable(g_value_obj[i][NE_ROW_SERVER]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_PORT]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_SECURITY]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_USER]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_PASSWORD]);
			}

			for (j = 0; j < NUM_NE_ROWS; j++)
			{
				nfui_signal_emit(g_value_obj[i][j], GDK_EXPOSE, TRUE);
			}		
		}
	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint i;
		gboolean default_server_on = FALSE;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		for (i = 0; i < SUPPORT_SERVER_CNT; i++)
		{
#if defined(DEFALT_E_MAIL_SERVER)
			default_server_on = nfui_check_button_get_active((NFCHECKBUTTON*)g_value_obj[i][NE_ROW_DEF_SERVER]);
#endif

			if (default_server_on == TRUE)
			{
				g_sprintf(emaildata[i].server, "%s", DEFAULT_SERVER_NAME);
				emaildata[i].port = DEFAULT_SMTPPORT;
				emaildata[i].security = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY]);
				g_sprintf(emaildata[i].user, "%s", DEFAULT_USER);
				g_sprintf(emaildata[i].passwd, "%s", DEFAULT_PASSWORD);
#if defined(_SUPPORT_DUAL_SMTPSERVER)
				emaildata[i].individual = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL]);
#endif				
				g_sprintf(emaildata[i].from, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM]));
				g_sprintf(emaildata[i].testMail, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL]));                		
			}
			else
			{
				g_sprintf(emaildata[i].server, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_SERVER]));
				emaildata[i].port = nfui_nflabel_get_number((NFLABEL*)g_value_obj[i][NE_ROW_PORT]);
				emaildata[i].security = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY]);            
				g_sprintf(emaildata[i].user, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_USER]));
				g_sprintf(emaildata[i].passwd, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_PASSWORD]));
#if defined(_SUPPORT_DUAL_SMTPSERVER)
				emaildata[i].individual = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL]);
#endif
				g_sprintf(emaildata[i].from, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM]));
				g_sprintf(emaildata[i].testMail, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL]));        		
			}
		}

		if (memcmp(org_emaildata, emaildata, sizeof(EmailData)*MAX_SERVER_CNT))
		{
			g_memmove(org_emaildata, emaildata, sizeof(EmailData)*MAX_SERVER_CNT);
			DAL_set_email_data(&emaildata);

			nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
			scm_put_log(CHANGE_NET_EMAIL, 0, 0);
			sysnet_set_changeflag(1);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		Email_tab_out_handler();
		SystemSetupNetwork_Destroy(obj);
	}

	return FALSE;
}

static gboolean get_is_default_server_on(EmailData data)
{
	gboolean is_on = FALSE;

	if((strlen(data.server) >= 0 && strcmp(data.server, DEFAULT_SERVER_NAME) == 0) &&
		(strlen(data.user) >= 0 && strcmp(data.user, DEFAULT_USER) == 0) &&
		(strlen(data.passwd) >= 0 && strcmp(data.passwd, DEFAULT_PASSWORD) == 0)&&
		(data.port == DEFAULT_SMTPPORT))
	{
		is_on = TRUE;        
	}

	return is_on;
}

static gboolean post_default_server_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
	{
		gboolean default_server_on = FALSE;
		guint i = 0, server_idx = 0;

#if defined(DEFALT_E_MAIL_SERVER)
		for (i = 0; i < SUPPORT_SERVER_CNT; i++)
		{
			if (obj == g_value_obj[i][NE_ROW_DEF_SERVER]) break;
		}
#endif
		server_idx = i;
		
		default_server_on = nfui_check_button_get_active((NFCHECKBUTTON*)obj);

		if (default_server_on == FALSE)
		{
			nfui_nflabel_set_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_SERVER], "");
			nfui_nflabel_set_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_USER], "");
			nfui_nflabel_set_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_PASSWORD], "");

			nfui_nfobject_enable(g_value_obj[server_idx][NE_ROW_SERVER]);
			nfui_nfobject_enable(g_value_obj[server_idx][NE_ROW_PORT]);
			nfui_nfobject_enable(g_value_obj[server_idx][NE_ROW_SECURITY]);
			nfui_nfobject_enable(g_value_obj[server_idx][NE_ROW_USER]);
			nfui_nfobject_enable(g_value_obj[server_idx][NE_ROW_PASSWORD]);
		}
		else
		{
			g_sprintf(emaildata[server_idx].server, "%s", DEFAULT_SERVER_NAME);
			g_sprintf(emaildata[server_idx].user, "%s", DEFAULT_USER);
			g_sprintf(emaildata[server_idx].passwd, "%s", DEFAULT_PASSWORD);
			emaildata[server_idx].port = DEFAULT_SMTPPORT;
		
			nfui_nflabel_set_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_SERVER], "");
			nfui_nflabel_set_number((NFLABEL*)g_value_obj[server_idx][NE_ROW_PORT], (gint)emaildata[server_idx].port);
			nfui_nflabel_set_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_USER], "");
			nfui_nflabel_set_text((NFLABEL*)g_value_obj[server_idx][NE_ROW_PASSWORD], "");

			nfui_nfobject_disable(g_value_obj[server_idx][NE_ROW_SERVER]);
			nfui_nfobject_disable(g_value_obj[server_idx][NE_ROW_PORT]);
			nfui_nfobject_disable(g_value_obj[server_idx][NE_ROW_SECURITY]);
			nfui_nfobject_disable(g_value_obj[server_idx][NE_ROW_USER]);
			nfui_nfobject_disable(g_value_obj[server_idx][NE_ROW_PASSWORD]);            
		}

		for (i = 0; i < NUM_NE_ROWS; i++)
		{
			nfui_signal_emit(g_value_obj[server_idx][i], GDK_EXPOSE, TRUE);
		}
            
	}

	return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
	}

	return FALSE;
}

void init_NetEMail_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *subject[NUM_NE_ROWS];
	NFOBJECT *obj;

	const gchar *strTitle[] = { 	"SERVER", 
						"PORT", 
						"SECURITY",
						"USER", 
						"PASSWORD", 
#if defined(_SUPPORT_DUAL_SMTPSERVER)
						"SENDING INDIVIDUAL EMAILS", 
#endif						
						"FROM",
						"TEST E-MAIL ADDRESS", 
#if defined(DEFALT_E_MAIL_SERVER)					
						"DEFAULT SERVER"
#endif
	};


	const gchar *strOffOn[] = {"OFF", "ON"};
	gchar server_str[256];
	gchar tmp_str[32];
	
	guint btn_x, btn_y, btn_space;
	guint pos_x, pos_y, tmp_y;
	guint i, j;

	gboolean default_server_on = FALSE;
	guint chk_w = 0, chk_h = 0;
	NFOBJECT *fixedTemp = NULL;

	g_curwnd = nfui_nfobject_get_top(parent);

// FIXED
	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);
	nfui_regi_pre_event_callback(content_fixed, pre_mainbg_event_handler);

	memset(emaildata, 0x00, sizeof(EmailData)*MAX_SERVER_CNT);
	memset(org_emaildata, 0x00, sizeof(EmailData)*MAX_SERVER_CNT);

	DAL_get_email_data(&emaildata);

	pos_x = NE_LABLE_LEFT;

	for (i = 0; i < SUPPORT_SERVER_CNT; i++)
	{
		pos_y = NE_LABLE_TOP;

#if defined(_SUPPORT_DUAL_SMTPSERVER)
		memset(server_str, 0x00, sizeof(server_str));
		strcat(server_str, lookup_string("MAIL SERVER"));
		g_sprintf(tmp_str, (" %d"), i+1);
		strcat(server_str, tmp_str);

		obj = nfui_nfimage_new(IMG_TITLE_BG2);
		nfui_nfimage_set_text((NFIMAGE*)obj, server_str);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
		nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);

		pos_x += 20;
		pos_y += 61;
#endif

#if defined(DEFALT_E_MAIL_SERVER)
		default_server_on = get_is_default_server_on(emaildata[i]);
		tmp_y = pos_y;
		pos_y += NE_LABLE_HEIGHT + NE_LABLE_HEIGHT_WITH_SPACE;
#endif

		for (j = 0; j < NUM_NE_ROWS; j++)
		{		
#if defined(DEFALT_E_MAIL_SERVER)
			if (j == NE_ROW_DEF_SERVER) pos_y = tmp_y;
#endif
		
			obj = nfui_nflabel_new_with_pango_font(strTitle[j], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
			nfui_nfobject_set_size(obj, NE_LABLE_WIDTH, NE_LABLE_HEIGHT);
			nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
			subject[j] = obj;

			if (j == NE_ROW_SERVER)
			{
				obj = nfui_nflabel_new_text_box(emaildata[i].server, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
				nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
				nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				nfui_regi_post_event_callback(obj, post_label_event_handler);	
				g_value_obj[i][j] = obj;

				nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(SERVER_STRING_SIZE));

				if (default_server_on) 
				{
					nfui_nflabel_set_text((NFLABEL*)obj, "");
					nfui_nfobject_disable(obj);
				}
			}
			else if (j == NE_ROW_PORT)
			{
				obj = nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
				nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
				nfui_nflabel_set_number((NFLABEL*)obj, (gint)emaildata[i].port);
				nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);				
				nfui_regi_post_event_callback(obj, post_label_event_handler);
				g_value_obj[i][j] = obj;

				nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(PORT_STRING_SIZE));

				if (default_server_on) 
				{
					nfui_nfobject_disable(obj);
				}
			}
			else if (j == NE_ROW_SECURITY)
			{
				obj = nfui_spinbutton_new(strOffOn, 2, emaildata[i].security);
				nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				g_value_obj[i][j] = obj;

				if (default_server_on) 
				{
					nfui_nfobject_disable(obj);
				}
				
				pos_y += NE_LABLE_HEIGHT_WITH_SPACE;
			}
			else if (j == NE_ROW_USER)
			{
				obj = nfui_nflabel_new_text_box(emaildata[i].user, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));		
				nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
				nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
				nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				nfui_regi_post_event_callback(obj, post_label_event_handler);				
				g_value_obj[i][j] = obj;
				
				nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(ID_STRING_SIZE));

				if (default_server_on) 
				{
					nfui_nflabel_set_text((NFLABEL*)obj, "");
					nfui_nfobject_disable(obj);
				}				
			}
			else if (j == NE_ROW_PASSWORD)
			{
				obj = nfui_nflabel_new_text_box(emaildata[i].passwd, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
				nfui_nflabel_set_invisible((NFLABEL*)(obj), TRUE);
				nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
				nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				nfui_regi_post_event_callback(obj, post_label_event_handler);
				g_value_obj[i][j] = obj;
				
				nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(PW_STRING_SIZE));

				if (default_server_on) 
				{
					nfui_nflabel_set_text((NFLABEL*)obj, "");
					nfui_nfobject_disable(obj);
				}				
			}
			else if (j == NE_ROW_FROM)
			{
				obj = nfui_nflabel_new_text_box(emaildata[i].from, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
				nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
				nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				nfui_regi_post_event_callback(obj, post_label_event_handler);
				g_value_obj[i][j] = obj;
				
				nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(EMAIL_STRING_SIZE));	
			}
			else if (j == NE_ROW_TEST_MAIL)
			{
				obj = nfui_nflabel_new_text_box(emaildata[i].testMail, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
				nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
				nfui_nflabel_set_spacing((NFLABEL*)obj, SEMI_CONDENSED_SPACING);
				nfui_nfobject_support_multi_lang((NFOBJECT*)obj, FALSE);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				nfui_regi_post_event_callback(obj, post_label_event_handler);
				g_value_obj[i][j] = obj;
				
				nfui_nfobject_set_data(obj, "string_size", GINT_TO_POINTER(EMAIL_STRING_SIZE));				
			}
#if defined(DEFALT_E_MAIL_SERVER)			
			else if (j == NE_ROW_DEF_SERVER)
			{		
				fixedTemp = nfui_nffixed_new();
				nfui_nfobject_modify_bg(fixedTemp, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
				nfui_nfobject_set_size(fixedTemp, 150, NE_LABLE_HEIGHT);
				nfui_nfobject_show(fixedTemp);
				nfui_nffixed_put((NFFIXED*)content_fixed, fixedTemp, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);

				obj = nfui_checkbutton_new(default_server_on);
				nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);				
				nfui_check_get_size(obj, &chk_w, &chk_h);				
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)fixedTemp, obj, (150-chk_w)/2, (NE_LABLE_HEIGHT-chk_h)/2);
				nfui_regi_post_event_callback(obj, post_default_server_event_handler);
				g_value_obj[i][j] = obj;
			}
#endif			
#if defined(_SUPPORT_DUAL_SMTPSERVER)			
			else if (j == NE_ROW_INDIVIDUAL)
			{		
				obj = nfui_spinbutton_new(strOffOn, 2, emaildata[i].individual);
				nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_1);
				nfui_nfobject_set_size(obj, NE_CELL_WIDTH, NE_CELL_HEIGHT);
				nfui_nfobject_show(obj);
				nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x+NE_LABLE_WIDTH+NE_COL_SPACE, pos_y);								
				g_value_obj[i][j] = obj;
			}
#endif

			if (j == NE_ROW_FROM) 
			{
				nfui_nfobject_hide(subject[j]);
				nfui_nfobject_hide(g_value_obj[i][j]);
			}

			pos_y += NE_LABLE_HEIGHT_WITH_SPACE+1;
		}

		pos_x += (NE_LABLE_WIDTH+NE_COL_SPACE+NE_CELL_WIDTH-192);
		pos_y += 14;

		obj = nftool_normal_button_create_type3("TEST", 192);				
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
		nfui_regi_post_event_callback(obj, post_testbutton_event_handler);
		nfui_nfobject_set_data(obj, "server_num", GINT_TO_POINTER(i));

		pos_x += (192 + 100);
	}


	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	g_memmove(org_emaildata, emaildata, sizeof(EmailData)*MAX_SERVER_CNT);
}


gboolean Email_tab_out_handler()
{
	gint i;
	mb_type ret;
        gboolean default_server_on = FALSE;

	for (i = 0; i < SUPPORT_SERVER_CNT; i++)
	{
#if defined(DEFALT_E_MAIL_SERVER)
	        default_server_on = nfui_check_button_get_active((NFCHECKBUTTON*)g_value_obj[i][NE_ROW_DEF_SERVER]);
#endif
	        
	        if (default_server_on == TRUE)
	        {
			g_sprintf(emaildata[i].server, "%s", DEFAULT_SERVER_NAME);
			emaildata[i].port = DEFAULT_SMTPPORT;
		        emaildata[i].security = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY]);
			g_sprintf(emaildata[i].user, "%s", DEFAULT_USER);
			g_sprintf(emaildata[i].passwd, "%s", DEFAULT_PASSWORD);
#if defined(_SUPPORT_DUAL_SMTPSERVER)
		        emaildata[i].individual = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL]);
#endif			
		        g_sprintf(emaildata[i].from, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM]));
		        g_sprintf(emaildata[i].testMail, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL]));
	        }
	        else
	        {
			g_sprintf(emaildata[i].server, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_SERVER]));
			emaildata[i].port = nfui_nflabel_get_number((NFLABEL*)g_value_obj[i][NE_ROW_PORT]);		
		        emaildata[i].security = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY]);        
			g_sprintf(emaildata[i].user, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_USER]));
			g_sprintf(emaildata[i].passwd, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_PASSWORD]));
#if defined(_SUPPORT_DUAL_SMTPSERVER)
		        emaildata[i].individual = nfui_spin_button_get_index((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL]);
#endif			
		        g_sprintf(emaildata[i].from, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM]));
		        g_sprintf(emaildata[i].testMail, "%s", nfui_nflabel_get_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL]));            		
	        }
	}

	if (!memcmp(org_emaildata, emaildata, sizeof(EmailData)*MAX_SERVER_CNT))
		return FALSE;

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
	{
		g_memmove(org_emaildata, emaildata, sizeof(EmailData)*MAX_SERVER_CNT);
		DAL_set_email_data(&emaildata);
		
		scm_put_log(CHANGE_NET_EMAIL, 0, 0);
		sysnet_set_changeflag(1);
	}
	else if (ret == NFTOOL_MB_CANCEL)
	{
		g_memmove(emaildata, org_emaildata, sizeof(EmailData)*MAX_SERVER_CNT);

		for (i = 0; i < SUPPORT_SERVER_CNT; i++)
		{
			default_server_on = get_is_default_server_on(emaildata[i]);

#if defined(DEFALT_E_MAIL_SERVER)
	              nfui_check_button_set_active((NFCHECKBUTTON*)g_value_obj[i][NE_ROW_DEF_SERVER], default_server_on);
#endif
	              if (default_server_on == TRUE)
	              {
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_SERVER], "");
				nfui_nflabel_set_number((NFLABEL*)g_value_obj[i][NE_ROW_PORT], (gint)emaildata[i].port);
				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY], emaildata[i].security);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_USER], "");
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_PASSWORD],"");
#if defined(_SUPPORT_DUAL_SMTPSERVER)
				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL], emaildata[i].individual);
#endif				
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM], emaildata[i].from);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL], emaildata[i].testMail);

				nfui_nfobject_disable(g_value_obj[i][NE_ROW_SERVER]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_PORT]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_SECURITY]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_USER]);
				nfui_nfobject_disable(g_value_obj[i][NE_ROW_PASSWORD]);                                          
	              }
	              else
	              {
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_SERVER], emaildata[i].server);
				nfui_nflabel_set_number((NFLABEL*)g_value_obj[i][NE_ROW_PORT], emaildata[i].port);
				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_value_obj[i][NE_ROW_SECURITY], emaildata[i].security);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_USER], emaildata[i].user);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_PASSWORD], emaildata[i].passwd);
#if defined(_SUPPORT_DUAL_SMTPSERVER)
				nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_value_obj[i][NE_ROW_INDIVIDUAL], emaildata[i].individual);
#endif				
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_FROM], emaildata[i].from);
				nfui_nflabel_set_text((NFLABEL*)g_value_obj[i][NE_ROW_TEST_MAIL], emaildata[i].testMail);

				nfui_nfobject_enable(g_value_obj[i][NE_ROW_SERVER]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_PORT]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_SECURITY]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_USER]);
				nfui_nfobject_enable(g_value_obj[i][NE_ROW_PASSWORD]);
	              }
		}
	}

	return FALSE;
}

