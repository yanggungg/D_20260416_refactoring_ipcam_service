
#include "nf_afx.h"

#include "nf_api_ipcam.h"

#include "nf_ptz.h"
#include "nf_api_live.h"

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

#include "vw_system_data_load.h"
#include "vw_sys_main.h"

#include "vw_vkeyboard.h"


#define SDL_WIN_SIZE_W					(652)
#define SDL_WIN_SIZE_H					(598)

#define SDL_TITLE_H						(36)

#define SDL_LABEL_H						(40)

// <---- DEVICE
#define SDL_DEV_LABEL_X					(26)
#define SDL_DEV_LABEL_Y					(64)
#define SDL_DEV_LABEL_W					(156)

#define SDL_DEV_CELL_X					(SDL_DEV_LABEL_X + SDL_DEV_LABEL_W + 10)
#define SDL_DEV_CELL_Y					(SDL_DEV_LABEL_Y)
#define SDL_DEV_CELL_W					(427)

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
#define SDL_LOAD_BTN_X					(146)
#define SDL_LOAD_BTN_Y					(SDL_WIN_SIZE_H - 74)
#define SDL_LOAD_BTN_W					(174)

#define SDL_CLOSE_BTN_X					(SDL_LOAD_BTN_X + SDL_LOAD_BTN_W + 6)
#define SDL_CLOSE_BTN_Y					(SDL_LOAD_BTN_Y)
#define SDL_CLOSE_BTN_W					(174)


#define MAX_STR_LEN						(63)




static NFWINDOW *g_curwnd = 0;
static NFOBJECT 	*dev_obj = NULL;
static NFOBJECT 	*df_list_obj = NULL;
static NFOBJECT 	*load_obj = NULL;

static NFOBJECT 	*wait_pop = NULL;

static gint 		media_cnt;
static MEDIA_INFO_T	*media_info;

static gboolean g_ret = FALSE;





static void _create_waitbox(void)
{
	wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
}

static void _remove_waitbox(void)
{
	if(wait_pop)
	{
		nftool_remove_waitbox(wait_pop);
		wait_pop = NULL;
	}
}

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
		nfui_nfobject_disable(df_list_obj);				
		nfui_nfobject_disable(load_obj);
	}
	else
	{
		nfui_nfobject_enable(df_list_obj);				
		nfui_nfobject_enable(load_obj);
	}

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(df_list_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(load_obj, GDK_EXPOSE, TRUE);
	
}

static void _update_data_file_list(void)
{
	guint i;
	gchar *dev = NULL;

	gchar **df_list;
	gint df_cnt;

	if (nfui_nfobject_is_shown(df_list_obj) == FALSE)	return;

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

	df_list = scm_new_db_list(media_info[i].id, &df_cnt);

	nfui_listbox_delete_all(NF_LISTBOX(df_list_obj));

	if ((df_cnt > 0) && (df_list != NULL))
	{	
		for(i = 0; i < df_cnt; i++)	
			nfui_listbox_set_text(NF_LISTBOX(df_list_obj), &(df_list[i]));

		ifree (df_list);
	}
		
	nfui_signal_emit(df_list_obj, GDK_EXPOSE, TRUE);

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

static void _get_path_DataLoad(char *path)
{
}

static gboolean _get_path_of_data(gchar *path)
{
	gchar dir[128];
	gchar *file = NULL;
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

		memset(dir, 0x00, sizeof(dir));
		if(scm_get_mounted_path(media_info[i].id, dir, 128) < 0) {
			g_warning("%s :::: RETURN -1" , __FUNCTION__);
			return FALSE;
		}

		file = nfui_listbox_get_focus_text((NFLISTBOX*)df_list_obj, 0);
		if(!file) {
			g_warning("%s :::: RETURN NULL", __FUNCTION__);
			return FALSE;
		}

		g_sprintf(path, "%s/%s", dir, file);
		g_message("%s :::: full path : %s/%s ", __FUNCTION__, dir, file);

		return TRUE;
	}

	return FALSE;
}

static gboolean post_loadbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gchar full_path[512];
		gint retValue;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
			
		memset(full_path, 0x00, sizeof(full_path));
		if(_get_path_of_data(full_path)) {
    		retValue = nftool_mbox(g_curwnd, "NOTICE", "The currently logged-in account will be logged out.\nDo you want to continue?", NFTOOL_MB_OKCANCEL);

    		if (retValue == NFTOOL_MB_CANCEL) return FALSE;
    		
			g_message("%s :::: full path : %s ", __FUNCTION__, full_path);

			smt_set_service(SMT_SYSDB_LOAD);
			scm_import_db(full_path, IRET_SYSTEM_DATA_LOAD_NOTIFY);	

			_create_waitbox();
		}
	}

	return FALSE;
}

static gboolean post_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{




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

		case GDK_BUTTON_RELEASE:
		break;					

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
		
			uxm_unreg_imsg_event(obj, INFY_MEDIA_STATUS_CHANGED);
			uxm_unreg_imsg_event(obj, IRET_SYSTEM_DATA_LOAD_NOTIFY);			

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

		case IRET_SYSTEM_DATA_LOAD_NOTIFY:
		{
			CMM_MESSAGE_T *pmsg = (CMM_MESSAGE_T *)data;
			GTimeVal last_temp;
			gint i;

			printf("data load ret = %d\n", pmsg->param);

			if(pmsg->param == 0) {
				g_get_current_time(&last_temp);
				for(i=0; i<MAX_USER_COUNT; i++)
					DAL_set_pw_last_changed_time(last_temp.tv_sec, i);

				DAL_save_db("usr");
			}

			_remove_waitbox();

			smt_return_to_previous();
			VW_SetupSystem_Destroy(obj);

			g_ret = TRUE;
		}
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

gboolean VW_System_Data_Load_Open(NFWINDOW *parent, gchar *title, guint pos_x, guint pos_y)
{
	NFOBJECT *sds_win;
	NFOBJECT *main_fixed;
	NFOBJECT *obj;

	guint lc_size[] = {SDL_FILE_CELL_W, };
	guint li_size_w, li_size_h;
	guint i;

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
	nfui_nfobject_set_size(obj, 500, SDL_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 76, 4);

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
	
	df_list_obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(df_list_obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(df_list_obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(df_list_obj), FALSE);
    nfui_listbox_set_drawing_outline((NFLISTBOX*)df_list_obj, TRUE);
	nfui_nfobject_use_focus(df_list_obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_set_size(df_list_obj, SDL_FILE_CELL_W, SDL_FILE_CELL_H);
	nfui_nfobject_show(df_list_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, df_list_obj, SDL_FILE_CELL_X, SDL_FILE_CELL_Y);

// <---- SAVE BUTTON
	load_obj = nftool_normal_button_create_type1("LOAD", SDL_LOAD_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(load_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(load_obj);
	nfui_regi_post_event_callback(load_obj, post_loadbutton_event_handler);
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
	uxm_reg_imsg_event(main_fixed, IRET_SYSTEM_DATA_LOAD_NOTIFY);

	nfui_page_open(PGID_SYS_LOAD_DATA_POPUP, sds_win, ssm_get_cur_id(NULL));

	g_ret = FALSE;

	gtk_main();

	nfui_page_close(PGID_SYS_LOAD_DATA_POPUP, sds_win);

	return g_ret;
}



