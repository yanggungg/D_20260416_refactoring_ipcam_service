
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nflistbox.h"
#include "ix_mem.h"

#include "modules/ssm.h"

#include "uxm.h"
#include "scm.h"
#include "smt.h"
#include "iux_msg.h"

#include "vw_sys_net_ssl_load_file_popup.h"


#define SDL_WIN_SIZE_W					(852)
#define SDL_WIN_SIZE_H					(598)

#define SDL_TITLE_H						(36)
#define SDL_LABEL_H						(40)

// <---- DEVICE
#define SDL_DEV_LABEL_X					(26)
#define SDL_DEV_LABEL_Y					(64)
#define SDL_DEV_LABEL_W					(156)

#define SDL_DEV_CELL_X					(SDL_DEV_LABEL_X + SDL_DEV_LABEL_W + 10)
#define SDL_DEV_CELL_Y					(SDL_DEV_LABEL_Y)
#define SDL_DEV_CELL_W					(627)

// <---- FILE NAME
#define SDL_FILE_LIST_NUM				(12)
#define SDL_FILE_LIST_H					(30)

#define SDL_FILE_LABEL_X				(SDL_DEV_LABEL_X)
#define SDL_FILE_LABEL_Y				(SDL_DEV_LABEL_Y + SDL_LABEL_H + 3)
#define SDL_FILE_LABEL_W				(SDL_DEV_LABEL_W)

#define SDL_FILE_CELL_X					(SDL_DEV_CELL_X)
#define SDL_FILE_CELL_Y					(SDL_FILE_LABEL_Y)
#define SDL_FILE_CELL_W					(SDL_DEV_CELL_W)
#define SDL_FILE_CELL_H					(SDL_FILE_LIST_NUM*SDL_FILE_LIST_H+23)

// <---- LOAD, CLOSE BUTTON
#define SDL_LOAD_BTN_X					((SDL_WIN_SIZE_W)/2 - SDL_LOAD_BTN_W - 3 )
#define SDL_LOAD_BTN_Y					(SDL_WIN_SIZE_H - 74)
#define SDL_LOAD_BTN_W					(174)

#define SDL_CLOSE_BTN_X					((SDL_WIN_SIZE_W)/2 + 3)
#define SDL_CLOSE_BTN_Y					(SDL_LOAD_BTN_Y)
#define SDL_CLOSE_BTN_W					(174)


#define MAX_STR_LEN						(63)




static NFWINDOW 	*g_curwnd = 0;
static NFOBJECT 	*dev_obj = NULL;
static NFOBJECT 	*ssl_list_obj = NULL;
static NFOBJECT 	*load_obj = NULL;

static gint 		media_cnt;
static MEDIA_INFO_T	*media_info;
static SSL_MODE_E g_mode;

static gchar *g_dir = NULL;
static gchar *g_file = NULL;
static gboolean g_ret = FALSE;


static void _update_dev_list(void)
{
	guint i, cnt = 0;

	if (media_info)
	{
		scm_free_media_list(media_info);
		nfui_combobox_remove_all(dev_obj);
	}

	media_cnt = 0;	
	media_info = scm_new_media_list(&media_cnt);

	for (i = 0; i < media_cnt; i++)
	{
		if (scm_get_media_type(media_info[i].id) == MTYPE_USB)
		{
			nfui_combobox_append_data(dev_obj, media_info[i].title);
			cnt++;

			g_message("%s, %d, i:%d, id:%d, name:%s", __FUNCTION__, __LINE__, i, media_info[i].id, media_info[i].title);
		}
	}

	if (cnt == 0)
	{
		nfui_combobox_append_data(dev_obj, "NONE");
		nfui_nfobject_disable(ssl_list_obj);				
		nfui_nfobject_disable(load_obj);
	}
	else
	{
		nfui_nfobject_enable(ssl_list_obj);				
		nfui_nfobject_enable(load_obj);
	}

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(ssl_list_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(load_obj, GDK_EXPOSE, TRUE);
	
}

static void _update_ssl_dcv_file(guint media_id, NFOBJECT *listobj)
{
    gchar **ssl_list;
    gint i, ssl_cnt;

    ssl_list = scm_new_ssl_dcv_list(media_id, &ssl_cnt);

    if ((ssl_cnt > 0) && (ssl_list != NULL))
    {
        for(i = 0; i < ssl_cnt; i++)
        {
            nfui_listbox_set_text(NF_LISTBOX(listobj), &(ssl_list[i]));
        }

        scm_free_ssl_list(ssl_list);
    }
}

static void _update_ssl_icrt_file(guint media_id, NFOBJECT *listobj)
{
    gchar **ssl_list;
    gint i, ssl_cnt;

    ssl_list = scm_new_ssl_icrt_list(media_id, &ssl_cnt);

    if ((ssl_cnt > 0) && (ssl_list != NULL))
    {
        for(i = 0; i < ssl_cnt; i++)
        {
            nfui_listbox_set_text(NF_LISTBOX(listobj), &(ssl_list[i]));
        }

        scm_free_ssl_list(ssl_list);
    }
}

static void _update_ssl_pem_file(guint media_id, NFOBJECT *listobj)
{
    gchar **ssl_list;
    gint i, ssl_cnt;

    ssl_list = scm_new_ssl_pem_list(media_id, &ssl_cnt);

    if ((ssl_cnt > 0) && (ssl_list != NULL))
    {
        for(i = 0; i < ssl_cnt; i++)
        {
            nfui_listbox_set_text(NF_LISTBOX(listobj), &(ssl_list[i]));
        }

        scm_free_ssl_list(ssl_list);
    }
}

static void _update_ssl_crt_file(guint media_id, NFOBJECT *listobj)
{
    gchar **ssl_list;
    gint i, ssl_cnt;

    ssl_list = scm_new_ssl_crt_list(media_id, &ssl_cnt);

    if ((ssl_cnt > 0) && (ssl_list != NULL))
    {
        for(i = 0; i < ssl_cnt; i++)
        {
            nfui_listbox_set_text(NF_LISTBOX(listobj), &(ssl_list[i]));
        }

        scm_free_ssl_list(ssl_list);
    }
}

static void _update_ssl_key_file(guint media_id, NFOBJECT *listobj)
{
    gchar **ssl_list;
    gint i, ssl_cnt;

    ssl_list = scm_new_ssl_key_list(media_id, &ssl_cnt);

    if ((ssl_cnt > 0) && (ssl_list != NULL))
    {
        for(i = 0; i < ssl_cnt; i++)
        {
            nfui_listbox_set_text(NF_LISTBOX(listobj), &(ssl_list[i]));
        }

        scm_free_ssl_list(ssl_list);
    }
}

static void _update_data_file_list(void)
{
	guint i;
	gchar *dev = NULL;

	gchar **df_list;
	gint df_cnt;

	if (nfui_nfobject_is_shown(ssl_list_obj) == FALSE)	return;

	dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

	for (i = 0; i < media_cnt; i++)
	{
		if(!strcmp(dev, media_info[i].title))
			break;
	}

	if (i == media_cnt)
	{
		g_warning("%s, %d, media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, media_cnt);
		return;
	}

	g_message("%s, %d, i:%d, id:%d, name:%s", __FUNCTION__, __LINE__, i, media_info[i].id, media_info[i].title);

	nfui_listbox_delete_all(NF_LISTBOX(ssl_list_obj));
	
	if (g_mode == SSL_MODE_DCV) 
	{
		_update_ssl_dcv_file(media_info[i].id, ssl_list_obj);
	}
	else if (g_mode == SSL_MODE_BUILTIN) 
	{
		_update_ssl_icrt_file(media_info[i].id, ssl_list_obj);
	}
	else {
		_update_ssl_pem_file(media_info[i].id, ssl_list_obj);
		_update_ssl_crt_file(media_info[i].id, ssl_list_obj);
		_update_ssl_key_file(media_info[i].id, ssl_list_obj);
	}
    
	nfui_signal_emit(ssl_list_obj, GDK_EXPOSE, TRUE);

}

static void _update_info_dev_changed(void)
{
	_update_dev_list();
	_update_data_file_list();
}

static gboolean post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
		_update_data_file_list();
	}

	return FALSE;
}

static gboolean _get_path_of_data(gchar *dir, gchar *file)
{
	gchar t_dir[256];
	gchar *t_file = NULL;
	guint i;
	gchar *dev = NULL;

	if(media_info) {
		dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

		for (i = 0; i < media_cnt; i++)
		{
			if(!strcmp(dev, media_info[i].title))
				break;
		}

		if (i == media_cnt)
		{
			g_warning("%s, %d, media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, media_cnt);
			return;
		}

		memset(t_dir, 0x00, sizeof(t_dir));
		if(scm_get_mounted_path(media_info[i].id, t_dir, 256) < 0) {
			g_warning("%s :::: RETURN -1" , __FUNCTION__);
			return FALSE;
		}

		t_file = nfui_listbox_get_focus_text((NFLISTBOX*)ssl_list_obj, 0);
		if(!t_file) {
			g_warning("%s :::: RETURN NULL", __FUNCTION__);
			return FALSE;
		}

		g_message("%s :::: full path : %s/%s ", __FUNCTION__, t_dir, t_file);
		strcpy(dir, t_dir);
		strcpy(file, t_file);

		return TRUE;
	}

	return FALSE;
}

static gboolean post_ok_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *topwin;
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        gchar dir[256];
        gchar file[256];
        gint retValue;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        memset(dir, 0x00, sizeof(dir));
        memset(file, 0x00, sizeof(file));
        if(_get_path_of_data(dir, file)) 
        {
            strcpy(g_dir, dir);
            strcpy(g_file, file);
            
            g_ret = TRUE;

            topwin = nfui_nfobject_get_top(obj);
            nfui_nfobject_destroy(topwin);
        }
    }

    return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *topwin;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);

		g_ret = FALSE;
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

    switch(event->type) {
        case GDK_EXPOSE:
        {
            drawable = nfui_nfobject_get_window(obj);
            gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

            nfui_nfobject_gc_unref(gc);
        }
        break;

        case GDK_DELETE:
        {
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);

            uxm_unreg_imsg_event(obj, INFY_MEDIA_STATUS_CHANGED);

            if (media_info)
            {
                scm_free_media_list(media_info);
                media_info = 0;
            }			
        }
        break;

        case INFY_MEDIA_STATUS_CHANGED:
            _update_info_dev_changed();
        break;

        default:
        break;
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

gboolean VW_SSL_Load_File_Open(NFWINDOW *parent, gchar *title, guint pos_x, guint pos_y, gchar *dir, gchar *fname, SSL_MODE_E mode)
{
	NFOBJECT *sds_win;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;

	guint lc_size[] = {SDL_FILE_CELL_W, };
	guint li_size_w, li_size_h;
	guint i;

	g_dir = dir;
	g_file = fname;
	g_mode = mode;

// <---- WINDOW
	sds_win = (NFOBJECT*)nfui_nfwindow_new(parent, pos_x, pos_y, SDL_WIN_SIZE_W, SDL_WIN_SIZE_H);
	g_curwnd = sds_win;
	nfui_nfobject_modify_bg(sds_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(sds_win, post_main_win_event_handler);

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, SDL_WIN_SIZE_W, SDL_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, SDL_WIN_SIZE_W - 8, SDL_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 4, 4);

// <---- DEVICE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEVICE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SDL_DEV_LABEL_W, SDL_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SDL_DEV_LABEL_X, SDL_DEV_LABEL_Y);

	dev_obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(dev_obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(dev_obj, SDL_DEV_CELL_W, SDL_LABEL_H);
	nfui_nfobject_show(dev_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, dev_obj, SDL_DEV_CELL_X, SDL_DEV_CELL_Y);
	nfui_regi_post_event_callback(dev_obj, post_device_event_handler);

// <---- DATA FILE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FILE NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, SDL_FILE_LABEL_W, SDL_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SDL_FILE_LABEL_X, SDL_FILE_LABEL_Y);
	
	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;
	
	ssl_list_obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(ssl_list_obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(ssl_list_obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(ssl_list_obj), FALSE);
    nfui_listbox_set_drawing_outline((NFLISTBOX*)ssl_list_obj, TRUE);
	nfui_nfobject_use_focus(ssl_list_obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_set_size(ssl_list_obj, SDL_FILE_CELL_W, SDL_FILE_CELL_H);
	nfui_nfobject_show(ssl_list_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, ssl_list_obj, SDL_FILE_CELL_X, SDL_FILE_CELL_Y);

// <---- SAVE BUTTON
	load_obj = nftool_normal_button_create_type1("OK", SDL_LOAD_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(load_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(load_obj);
	nfui_regi_post_event_callback(load_obj, post_ok_event_handler);
	nfui_nffixed_put((NFFIXED*)main_fixed, load_obj, SDL_LOAD_BTN_X, SDL_LOAD_BTN_Y);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", SDL_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SDL_CLOSE_BTN_X, SDL_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_nfwindow_add((NFWINDOW*)sds_win, main_fixed);
	nfui_run_main_event_handler(sds_win);
	nfui_nfobject_show(sds_win);

	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)sds_win);
	nfui_set_key_focus(load_obj, TRUE);

	_update_info_dev_changed();
	
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);	

	nfui_page_open(PGID_FIND_SSL_CERTIFICATE_POPUP, sds_win, ssm_get_cur_id(NULL));

	g_ret = FALSE;

	gtk_main();

	nfui_page_close(PGID_FIND_SSL_CERTIFICATE_POPUP, sds_win);

	return g_ret;
}
