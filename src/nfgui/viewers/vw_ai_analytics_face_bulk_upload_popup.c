/*
 * vw_timeline_deeplearning.c
 * 	- timeline dlva viewer
 *	- dependencies :
 *		
 *
 * Written by Jungkyu Park. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 11, 2018
 *
 */

#include <time.h>

#include "gui/nf_afx.h"
#include <gtk/gtk.h>

#include <glib.h> 
#include <glib-object.h>
#include <glib/gprintf.h>

#include "nf_common.h"
#include "nf_notify.h"


#include "scm.h"
#include "vsm.h"
#include "modules/ssm.h"

#include "support/event_loop.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"
#include "viewers/objects/nftable.h"
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nflistbox.h"

#include "vw_vkeyboard.h"
#include "vw_ai_analytics_face_bulk_upload_popup.h"

#include "nf_api_dlva.h"




////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (580)
#define POPUP_SIZE_HEI          (700)


static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_dev_obj = NULL;
static NFOBJECT *g_dirlist_obj = NULL;

static gint g_media_cnt;
static MEDIA_INFO_T	*g_media_info;

static gint g_dev_ch;
static guint g_aibox_ipaddr;
static gint g_possible_face_cnt;

static gint g_retVal = 0;



////////////////////////////////////////////////////////////////
//
// private interfaces
//

static void _update_dev_list(void)
{
	guint i, cnt = 0;

	if (g_media_info)
	{
		scm_free_media_list(g_media_info);
		nfui_combobox_remove_all(g_dev_obj);
	}

	g_media_cnt = 0;	
	g_media_info = scm_new_media_list(&g_media_cnt);

	for (i = 0; i < g_media_cnt; i++)
	{
		if (scm_get_media_type(g_media_info[i].id) == MTYPE_USB)
		{
			nfui_combobox_append_data(g_dev_obj, g_media_info[i].title);
			cnt++;
		}
	}

	if (cnt == 0)
	{
		nfui_combobox_append_data(g_dev_obj, "NONE");
		nfui_nfobject_disable(g_dirlist_obj);				
	}
	else
	{
		nfui_nfobject_enable(g_dirlist_obj);				
	}

	nfui_signal_emit(g_dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_dirlist_obj, GDK_EXPOSE, TRUE);

	nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
}

static void _update_face_dir_list()
{
	gchar *dev = NULL;
	guint i, media_idx;

	gchar **dir_list;
	gint dir_cnt;

	if (nfui_nfobject_is_shown(g_dirlist_obj) == FALSE) return;

	dev = nfui_combobox_get_value(NF_COMBOBOX(g_dev_obj));

	for (i = 0; i < g_media_cnt; i++)
	{
		if(!strcmp(dev, g_media_info[i].title))
			break;
	}
	media_idx = i;

	if (i == g_media_cnt)
	{
		g_warning("%s, %d, g_media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, g_media_cnt);
		return;
	}

	nfui_listbox_delete_all(NF_LISTBOX(g_dirlist_obj));

	dir_list = scm_new_dir_list(g_media_info[media_idx].id, &dir_cnt);
	g_message("%s, %d, dir cnt:%d", __FUNCTION__, __LINE__, dir_cnt);

	if ((dir_cnt > 0) && (dir_list != NULL))
	{			
		for(i = 0; i < dir_cnt; i++)	{
			nfui_listbox_set_text(NF_LISTBOX(g_dirlist_obj), &(dir_list[i]));
		}
		scm_free_dir_list(dir_list);
	}

	nfui_signal_emit(g_dirlist_obj, GDK_EXPOSE, TRUE);
}

static void _update_info_dev_changed(void)
{
	_update_dev_list();
	_update_face_dir_list();
}

static gint _get_mounted_path(gchar *mnt_path, int path_len)
{
	gchar *dev = NULL;
	guint i;

    if (nfui_nfobject_is_shown(g_dirlist_obj) == FALSE) return -1;

	dev = nfui_combobox_get_value(NF_COMBOBOX(g_dev_obj));

	for (i = 0; i < g_media_cnt; i++)
	{
		if(!strcmp(dev, g_media_info[i].title))
			break;
	}

	if (i == g_media_cnt)
	{
		g_warning("%s, %d, media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, g_media_cnt);
		return -1;
	}

	scm_get_mounted_path(g_media_info[i].id, mnt_path, path_len);

    return 0;
}

static gint _get_focused_directory(gchar *dir_name, int name_len)
{
    if (nfui_nfobject_is_shown(g_dirlist_obj) == FALSE) return -1;

    snprintf(dir_name, name_len-1, "%s", nfui_listbox_get_focus_text((NFLISTBOX*)g_dirlist_obj, 0));
    return 0;
}



////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		_update_face_dir_list();
	}

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	gchar mnt_path[128];
	gchar dir_name[128];
	gchar fullpath[256];

	gint retVal = 0;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		if (!nfui_listbox_get_focus_text((NFLISTBOX*)g_dirlist_obj, 0)) return FALSE;

		memset(mnt_path, 0x00, sizeof(mnt_path));
		_get_mounted_path(mnt_path, sizeof(mnt_path));
		memset(dir_name, 0x00, sizeof(dir_name));
		_get_focused_directory(dir_name, sizeof(dir_name));

		memset(fullpath, 0x00, sizeof(fullpath));
		snprintf(fullpath, sizeof(fullpath)-1, "%s/%s", mnt_path, dir_name);
		g_message("%s, %d, fullpath:%s", __FUNCTION__, __LINE__, fullpath);

		retVal = vw_ai_analytics_face_bulk_progress_popup_open(g_curwnd, g_dev_ch, g_aibox_ipaddr, g_possible_face_cnt, fullpath);

		if (retVal == 0)
		{
			g_retVal = 0;

			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
		}
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		g_retVal = -1;

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
	{

	}
	else if (evt->type == INFY_MEDIA_STATUS_CHANGED)
	{
		_update_info_dev_changed();
	}	
	else if (evt->type == GDK_DELETE)
	{
		if (g_media_info)
		{
			scm_free_media_list(g_media_info);
			g_media_info = 0;
		}

		uxm_unreg_imsg_event(obj, INFY_MEDIA_STATUS_CHANGED);	
	}

	return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_DELETE)
	{
		gtk_main_quit();
	}

	return FALSE;
}



//////////////////////////////¤º//////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_face_bulk_upload_popup_open(NFWINDOW *parent, gint ch, guint ipaddr, gint face_cnt)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;
	
	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i, j;

	guint lc_size[] = {480, };
	guint li_size_w, li_size_h;	

	g_dev_ch = ch;
	g_aibox_ipaddr = ipaddr;
	g_possible_face_cnt = face_cnt;

	g_retVal = -1;


	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "BULK UPLOAD", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 60;

    obj = nfui_nflabel_new_with_pango_font("Ex) (Directory) / (Group) / (Name).jpg", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 540, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 36;

    obj = nfui_nflabel_new_with_pango_font("1. (Directory) : Directory to perform bulk upload", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 540, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 36;

    obj = nfui_nflabel_new_with_pango_font("2. (Group) : Group directory, upload as name of group", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 540, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 36;

    obj = nfui_nflabel_new_with_pango_font("3. (Name).jpg : Face file, upload with the name of the photo", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 540, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);

	pos_y += 70;

    obj = nfui_nflabel_new_with_pango_font("DEVICE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = nfui_combobox_new(0, 0, 0);
	nfui_combobox_set_skin_type((NFCOMBOBOX*)obj, NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align((NFCOMBOBOX*)obj, NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nfobject_set_size(obj, 420, 40);		
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+120, pos_y);
	nfui_regi_post_event_callback(obj, post_device_event_handler);
	g_dev_obj = obj;

	pos_y += 52;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 540, 320);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	//nfui_regi_post_event_callback(obj, post_dir_list_event_handler);
	g_dirlist_obj = obj;


	obj = nftool_normal_button_create_type1("OK", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2-174-4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_ok_button_event_handler);

	obj = nftool_normal_button_create_type1("CANCEL", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, POPUP_SIZE_WID/2+4, POPUP_SIZE_HEI-56);
	nfui_regi_post_event_callback(obj, post_cancel_button_event_handler);
	
	nfui_nfobject_show(main_wnd);
	nfui_make_key_hierarchy(main_wnd);
	nfui_set_key_focus(obj, TRUE);	

	_update_info_dev_changed();
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);

	nfui_page_open(PGID_POPUPWND, main_wnd, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_POPUPWND, main_wnd);

	return g_retVal;
}
