
#include "nf_afx.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "services/scm.h"
#include "tools/nf_ui_tool.h"
#include "ssm.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_event_ftp_edit.h"
#include "vw_vkeyboard.h"

#include "nf_util_ftp.h"
#include "ix_mem.h"


#define FTP_EDIT_WIN_W      (670)
#define FTP_EDIT_WIN_H      (360)

static NFWINDOW *g_curwnd = 0;

static EA_EvtNotiFTPData *g_org_ftpData;
static EA_EvtNotiFTPData g_ftpData;


static gboolean post_host_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint x, y;
		gchar *strTemp;

        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += obj->x;
		y += obj->y + obj->height + 4;

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 31, VKEY_NORMAL);

		if (strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);
			strcpy(g_ftpData.host, strTemp);

			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_port_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint x, y;
		gint numTemp;
		gint numRet;
		gchar strbuf[256];

        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += obj->x;
		y += obj->y + obj->height + 4;

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
			g_ftpData.port = (guint)numRet;
		}
	}

	return FALSE;
}

static gboolean post_user_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint x, y;
		gchar *strTemp;

        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += obj->x;
		y += obj->y + obj->height + 4;

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 31, VKEY_NORMAL);

		if (strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);
			strcpy(g_ftpData.username, strTemp);

			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_password_label_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
		guint x, y;
		gchar *strTemp;

        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += obj->x;
		y += obj->y + obj->height + 4;

		strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 15, VKEY_PASSWORD);

		if (strTemp)
		{
			nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
			nfui_signal_emit((NFLABEL*)obj, GDK_EXPOSE, FALSE);
			strcpy(g_ftpData.passwd, strTemp);

			ifree(strTemp);
			strTemp = NULL;
		}
	}

	return FALSE;
}

static gboolean post_connection_test_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	static NFOBJECT *wait_pop = NULL;
	FTP_INFO_T req_info;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(&req_info, 0x00, sizeof(FTP_INFO_T));
		strcpy(req_info.server, g_ftpData.host);
		strcpy(req_info.user, g_ftpData.username);
		strcpy(req_info.passwd, g_ftpData.passwd);
      	req_info.port = (gint)g_ftpData.port;

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

static gboolean post_ok_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_memmove(g_org_ftpData, &g_ftpData, sizeof(EA_EvtNotiFTPData));

		top = nfui_nfobject_get_top(obj);
		if (top) nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		NFOBJECT *top = NULL;

		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		top = nfui_nfobject_get_top(obj);
		if (top) nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE)
	{
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

static gboolean post_win_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

void VW_EvtNoti_FTP_Edit(NFWINDOW *parent, gint win_x, gint win_y, EA_EvtNotiFTPData *ftpData)
{
	NFOBJECT *win;
	NFOBJECT *fixed;
	NFOBJECT *obj;
	NFOBJECT *ok_btn;
    gint pos_x, pos_y;

    g_org_ftpData = ftpData;
    g_memmove(&g_ftpData, g_org_ftpData, sizeof(EA_EvtNotiFTPData));

    if ((win_x+FTP_EDIT_WIN_W) > 1920)  win_x = 1920 - FTP_EDIT_WIN_W;
    if ((win_y+FTP_EDIT_WIN_H) > 1080)  win_y = 1080 - FTP_EDIT_WIN_H;

	win = (NFOBJECT*)nfui_nfwindow_new(parent, win_x, win_y, FTP_EDIT_WIN_W, FTP_EDIT_WIN_H);
	nfui_regi_post_event_callback(win, post_win_event_cb);
	g_curwnd = win;

	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, FTP_EDIT_WIN_W, FTP_EDIT_WIN_H);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EDIT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 4, 4);

	pos_x = 26;
	pos_y = 64;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HOST NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(ftpData->host, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 350, 40);
	nfui_regi_post_event_callback(obj, post_host_label_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);

    pos_y += (40 + 2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PORT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nflabel_set_number((NFLABEL*)obj, ftpData->port);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 350, 40);
	nfui_regi_post_event_callback(obj, post_port_label_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);

    pos_y += (40 + 2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("USER NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(ftpData->username, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_set_size(obj, 350, 40);
	nfui_regi_post_event_callback(obj, post_user_label_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);

    pos_y += (40 + 2);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PASSWORD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_set_size(obj, 250, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box(ftpData->passwd, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 20);
	nfui_nflabel_set_invisible((NFLABEL*)obj, TRUE);
	nfui_nfobject_set_size(obj, 350, 40);
	nfui_regi_post_event_callback(obj, post_password_label_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);

    pos_y += (40 + 4);

	obj = nftool_normal_button_create_popup_type1("CONNECTION TEST", 350);
	nfui_regi_post_event_callback(obj, post_connection_test_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x+250+10, pos_y);
	uxm_reg_imsg_event(obj, IRET_SCM_TST_FTP);

	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_regi_post_event_callback(obj, post_ok_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (FTP_EDIT_WIN_W/2)-174-10, FTP_EDIT_WIN_H-56);
	ok_btn = obj;

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_regi_post_event_callback(obj, post_cancel_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, (FTP_EDIT_WIN_W/2)+10, FTP_EDIT_WIN_H-56);

	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(ok_btn, TRUE);

	nfui_page_open(PGID_EVT_FTP_EDIT_POPUP, win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_EVT_FTP_EDIT_POPUP, win);
}

