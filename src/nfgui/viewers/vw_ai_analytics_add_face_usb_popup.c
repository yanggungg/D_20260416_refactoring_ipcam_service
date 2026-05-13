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
#include "objects/nflistbox.h"

#include "vw_ai_analytics_add_face_usb_popup.h"




////////////////////////////////////////////////////////////////
//
// private variables
//

#define POPUP_SIZE_WID		    (900)
#define POPUP_SIZE_HEI          (580)


static NFWINDOW *g_curwnd = NULL;
static NFOBJECT *g_dev_obj = NULL;
static NFOBJECT *g_facelist_obj = NULL;
static NFOBJECT *g_facelabel_obj = NULL;

static gint g_media_cnt;
static MEDIA_INFO_T	*g_media_info;

static USB_FACE_DATA_T *g_face_data;



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
		nfui_nfobject_disable(g_facelist_obj);				
	}
	else
	{
		nfui_nfobject_enable(g_facelist_obj);				
	}

	nfui_signal_emit(g_dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_facelist_obj, GDK_EXPOSE, TRUE);

	nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);
}

static void _update_face_file_list()
{
	gchar *dev = NULL;
	guint i, media_idx;

	gchar **face_list;
	gint face_cnt;

	if (nfui_nfobject_is_shown(g_facelist_obj) == FALSE) return;

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

	nfui_listbox_delete_all(NF_LISTBOX(g_facelist_obj));

	face_list = scm_new_file_list(g_media_info[media_idx].id, ".jpeg", &face_cnt);
	g_message("%s, %d, jpeg cnt:%d", __FUNCTION__, __LINE__, face_cnt);

	if ((face_cnt > 0) && (face_list != NULL))
	{			
		for(i = 0; i < face_cnt; i++)	{
			nfui_listbox_set_text(NF_LISTBOX(g_facelist_obj), &(face_list[i]));
		}
		scm_free_file_list(face_list);
	}

	face_list = scm_new_file_list(g_media_info[media_idx].id, ".jpg", &face_cnt);
	g_message("%s, %d, jpg cnt:%d", __FUNCTION__, __LINE__, face_cnt);

	if ((face_cnt > 0) && (face_list != NULL))
	{			
		for(i = 0; i < face_cnt; i++)	{
			nfui_listbox_set_text(NF_LISTBOX(g_facelist_obj), &(face_list[i]));
		}
		scm_free_file_list(face_list);
	}

	nfui_signal_emit(g_facelist_obj, GDK_EXPOSE, TRUE);
}

static void _update_info_dev_changed(void)
{
	_update_dev_list();
	_update_face_file_list();
}

static gint _get_mounted_path(gchar *mnt_path, int path_len)
{
	gchar *dev = NULL;
	guint i;

    if (nfui_nfobject_is_shown(g_facelist_obj) == FALSE) return -1;

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

static gint _get_focused_file(gchar *file_name, int name_len)
{
    if (nfui_nfobject_is_shown(g_facelist_obj) == FALSE) return -1;

    snprintf(file_name, name_len-1, "%s", nfui_listbox_get_focus_text((NFLISTBOX*)g_facelist_obj, 0));
    return 0;
}

static gint _draw_face_file_image(NFOBJECT *obj, gchar *path)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;

	gint gap_x, gap_y;

	GdkPixbuf *pbImage = NULL;
	GdkPixbuf *nbuf = NULL;

	pbImage = gdk_pixbuf_new_from_file(path, NULL);
	if (!pbImage) return -1;

	drawable = nfui_nfobject_get_window(obj);
	gc = nfui_nfobject_get_gc(obj);

	nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

	nbuf = gdk_pixbuf_scale_simple(pbImage, obj->width-8, obj->height-8, GDK_INTERP_BILINEAR);
	gdk_draw_pixbuf(drawable, gc, nbuf, 0, 0, gap_x+4, gap_y+4, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);

	g_object_unref(nbuf);
	g_object_unref(pbImage);

	nfui_nfobject_gc_unref(gc);

	return 0;
}



////////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		_update_face_file_list();
		nfui_signal_emit(g_facelabel_obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_face_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_LISTBOX_CHANGED)
    {
		nfui_signal_emit(g_facelabel_obj, GDK_EXPOSE, TRUE);
    }

	return FALSE;
}


static gboolean post_picture_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE)
    {
        GdkDrawable *drawable = NULL;
        GdkGC *gc;

        gint gap_x, gap_y;
		gchar mnt_path[128];
		gchar file_name[128];
		gchar face_path[256];

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset((NFOBJECT*)obj, &gap_x, &gap_y);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(200)));
		gdk_draw_rectangle(drawable, gc, TRUE, gap_x, gap_y, obj->width, obj->height);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(392)));
		gdk_gc_set_line_attributes(gc, 2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
		gdk_draw_rectangle(drawable, gc, FALSE, gap_x, gap_y, obj->width, obj->height);

		gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(206)));
		nfutil_draw_text_with_pango(NULL, NULL, &UX_COLOR(COLOR_IDX(200)), drawable, gc, "PHOTO", gap_x+4, gap_y+20, obj->width-8, obj->height-40, 
			nffont_get_pango_font(NFFONT_MINI_SEMI_5), &UX_COLOR(COLOR_IDX(206)), NFALIGN_CENTER, 0);

        nfui_nfobject_gc_unref(gc);

		if (nfui_listbox_get_focus_text((NFLISTBOX*)g_facelist_obj, 0))
		{
			memset(mnt_path, 0x00, sizeof(mnt_path));
			_get_mounted_path(mnt_path, sizeof(mnt_path));

			memset(file_name, 0x00, sizeof(file_name));
			_get_focused_file(file_name, sizeof(file_name));

			memset(face_path, 0x00, sizeof(face_path));
			snprintf(face_path, sizeof(face_path)-1, "%s/%s", mnt_path, file_name);
			_draw_face_file_image(obj, face_path);
		}
    }

	return FALSE;
}

static gboolean post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	gchar mnt_path[128];
	gchar file_name[128];
	gchar face_path[256];

	FILE *fp;
	GdkPixbuf *pbImage = NULL;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		if (nfui_listbox_get_focus_text((NFLISTBOX*)g_facelist_obj, 0))
		{
			memset(mnt_path, 0x00, sizeof(mnt_path));
			_get_mounted_path(mnt_path, sizeof(mnt_path));

			memset(file_name, 0x00, sizeof(file_name));
			_get_focused_file(file_name, sizeof(file_name));

			memset(face_path, 0x00, sizeof(face_path));
			snprintf(face_path, sizeof(face_path)-1, "%s/%s", mnt_path, file_name);

			pbImage = gdk_pixbuf_new_from_file(face_path, NULL);
			if (pbImage)
			{
				g_face_data->pbuf = gdk_pixbuf_scale_simple(pbImage, 240, 300, GDK_INTERP_BILINEAR);
				g_object_unref(pbImage);
			}

			fp = fopen(face_path, "r");

			if(fp){		
				fseek(fp, 0, SEEK_END);  
				g_face_data->image_len = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				g_face_data->image_data = imalloc(g_face_data->image_len);
				fread(g_face_data->image_data, 1, g_face_data->image_len, fp);
				fclose(fp);
			}			
		}

		top = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(top);
	}

	return FALSE;
}

static gboolean post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt,  gpointer data)
{
	NFOBJECT *top;

	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

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
		nfui_signal_emit(g_facelabel_obj, GDK_EXPOSE, TRUE);
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



//////////////////////////////��//////////////////////////////////
//
// public interfaces
//

gint vw_ai_analytics_add_face_usb_popup_open(NFWINDOW *parent, USB_FACE_DATA_T *face_data)
{
	NFOBJECT *main_wnd;
	NFOBJECT *main_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	guint lc_size[] = {600, };
	guint li_size_w, li_size_h;

	guint pos_x, pos_y;
	guint size_w, size_h;
	gint i;


	g_face_data = face_data;

	main_wnd = nftool_create_popup_window(parent, (1920-POPUP_SIZE_WID)/2, (1080-POPUP_SIZE_HEI)/2, POPUP_SIZE_WID, POPUP_SIZE_HEI, "Registration(USB)", TRUE);
	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);
	g_curwnd = (NFWINDOW*)main_wnd;

	main_fixed = ((NFWINDOW*)main_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

    pos_x = 20;
    pos_y = 64;

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
	nfui_nfobject_set_size(obj, 520, 40);		
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+120, pos_y);
	nfui_regi_post_event_callback(obj, post_device_event_handler);
	g_dev_obj = obj;

    pos_y += 41;

    obj = nfui_nflabel_new_with_pango_font("EXTENSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);	

    obj = nfui_nflabel_new_text_box("JPEG Image File(*.jpg,*.jpeg)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nfobject_set_size(obj, 520, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+120, pos_y);

    pos_y += 80;

	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;

	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, 600, 320);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_face_list_event_handler);
	g_facelist_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MINI_SEMI_5), COLOR_IDX(206));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 240, 320);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+620, pos_y);
	nfui_regi_post_event_callback(obj, post_picture_label_event_handler);
	g_facelabel_obj = obj;

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

	return 0;
}
