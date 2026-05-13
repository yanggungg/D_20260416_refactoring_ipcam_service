#include "nf_afx.h"

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

#include "scm.h"
#include "iux_msg.h"

#include "vw_ipcam_fw_up.h"
#include "vw_ipcam_upgrade_progress.h"


#define FW_UP_WIN_SIZE_W					(692)
#define FW_UP_WIN_SIZE_H					(200 + FW_UP_FILE_CELL_H + FW_UP_CHANNEL_CELL_H)

#define FW_UP_WIN_SIZE_X					((DISPLAY_ACTIVE_WIDTH - FW_UP_WIN_SIZE_W)/2)
#define FW_UP_WIN_SIZE_Y					((DISPLAY_ACTIVE_HEIGHT - FW_UP_WIN_SIZE_H)/2)

#define FW_UP_TITLE_H						(36)

#define FW_UP_LABEL_H						(40)

// <---- DEVICE
#define FW_UP_DEV_LABEL_X					(26)
#define FW_UP_DEV_LABEL_Y					(64)
#define FW_UP_DEV_LABEL_W					(196)

#define FW_UP_DEV_CELL_X					(FW_UP_DEV_LABEL_X + FW_UP_DEV_LABEL_W + 10)
#define FW_UP_DEV_CELL_Y					(FW_UP_DEV_LABEL_Y)
#define FW_UP_DEV_CELL_W					(427)

// <---- FILE NAME
#define FW_UP_FILE_LIST_NUM					(4)
#define FW_UP_FILE_LIST_H					(41)

#define FW_UP_FILE_LABEL_X					(FW_UP_DEV_LABEL_X)
#define FW_UP_FILE_LABEL_Y					(FW_UP_DEV_LABEL_Y + FW_UP_LABEL_H + 10)
#define FW_UP_FILE_LABEL_W					(FW_UP_DEV_LABEL_W)

#define FW_UP_FILE_CELL_X					(FW_UP_DEV_CELL_X)
#define FW_UP_FILE_CELL_Y					(FW_UP_FILE_LABEL_Y)
#define FW_UP_FILE_CELL_W					(FW_UP_DEV_CELL_W)
#define FW_UP_FILE_CELL_H					(FW_UP_FILE_LIST_NUM*FW_UP_FILE_LIST_H)

// <---- UPGRADE CHANNEL
#define FW_UP_CHANNEL_LIST_ROW				(GUI_CHANNEL_CNT/4)
#define FW_UP_CHANNEL_LIST_H				(40)

#define FW_UP_CHANNEL_LABEL_X				(FW_UP_FILE_LABEL_X)
#define FW_UP_CHANNEL_LABEL_Y				(FW_UP_FILE_CELL_Y + FW_UP_FILE_CELL_H + 10)
#define FW_UP_CHANNEL_LABEL_W				(FW_UP_FILE_LABEL_W)

#define FW_UP_CHANNEL_CELL_X				(FW_UP_DEV_CELL_X)
#define FW_UP_CHANNEL_CELL_Y				(FW_UP_FILE_CELL_Y + FW_UP_FILE_CELL_H + 10)
#define FW_UP_CHANNEL_CELL_W				(FW_UP_DEV_CELL_W)
#define FW_UP_CHANNEL_CELL_H				(FW_UP_CHANNEL_LIST_ROW*FW_UP_CHANNEL_LIST_H + 2*(FW_UP_CHANNEL_LIST_ROW-1))

// <---- UPGRADE, CLOSE BUTTON
#define FW_UP_LOAD_BTN_X					(FW_UP_WIN_SIZE_W/2-179)
#define FW_UP_LOAD_BTN_Y					(FW_UP_WIN_SIZE_H - 60)
#define FW_UP_LOAD_BTN_W					(174)

#define FW_UP_CLOSE_BTN_X					(FW_UP_WIN_SIZE_W/2+5)
#define FW_UP_CLOSE_BTN_Y					(FW_UP_LOAD_BTN_Y)
#define FW_UP_CLOSE_BTN_W					(174)


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_dev_obj = NULL;
static NFOBJECT *g_fw_list_obj = NULL;
static NFOBJECT *g_ch_obj[GUI_CHANNEL_CNT];
static NFOBJECT *g_upgrade_obj = NULL;

static gint media_cnt;
static MEDIA_INFO_T	*media_info;
static gchar media_m_path[256];


static void _update_dev_list(void)
{
	guint i, cnt = 0;

	if (media_info)
	{
		scm_free_media_list(media_info);
		nfui_combobox_remove_all(g_dev_obj);
	}

	media_cnt = 0;	
	media_info = scm_new_media_list(&media_cnt);

	for (i = 0; i < media_cnt; i++)
	{
		if (scm_get_media_type(media_info[i].id) == MTYPE_USB)
		{
			nfui_combobox_append_data(g_dev_obj, media_info[i].title);
			cnt++;

			g_message("%s, %d, i:%d, id:%d, name:%s", __FUNCTION__, __LINE__, i, media_info[i].id, media_info[i].title);
		}
	}

	if (cnt == 0)
	{
		nfui_combobox_append_data(g_dev_obj, "NONE");
		nfui_nfobject_disable(g_fw_list_obj);				
	}
	else
	{
		nfui_nfobject_enable(g_fw_list_obj);				
	}

	nfui_signal_emit(g_dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(g_fw_list_obj, GDK_EXPOSE, TRUE);

	nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);

}

static void _update_itx_fwfile(guint media_id, NFOBJECT *listobj)
{
	gchar **fw_list;
	gint i, fw_cnt;

	fw_list = scm_new_ipcam_fw_list(media_id, FF_FNAME_EXT, FO_DEFAULT, &fw_cnt);

	if ((fw_cnt > 0) && (fw_list != NULL))
	{			
		for(i = 0; i < fw_cnt; i++)	
		{
			nfui_listbox_set_text(NF_LISTBOX(listobj), &(fw_list[i]));
		}

		scm_free_ipcam_fw_list(fw_list);
	}
}

static void _update_techwin_fwfile(guint media_id, NFOBJECT *listobj)
{
	gchar **fw_list;
	gint i, fw_cnt;

	fw_list = scm_new_ipcam_fw_list_techwin(media_id, FF_FNAME_EXT, FO_DEFAULT, &fw_cnt);

	if ((fw_cnt > 0) && (fw_list != NULL))
	{			
		for(i = 0; i < fw_cnt; i++)	
		{
			nfui_listbox_set_text(NF_LISTBOX(listobj), &(fw_list[i]));
		}

		scm_free_ipcam_fw_list(fw_list);
	}
}

static void _update_idis_fwfile(guint media_id, NFOBJECT *listobj)
{
	gchar **fw_list;
	gint i, fw_cnt;

	fw_list = scm_new_ipcam_fw_list_idis(media_id, FF_FNAME_EXT, FO_DEFAULT, &fw_cnt);

	if ((fw_cnt > 0) && (fw_list != NULL))
	{			
		for(i = 0; i < fw_cnt; i++)	
		{
			nfui_listbox_set_text(NF_LISTBOX(listobj), &(fw_list[i]));
		}

		scm_free_ipcam_fw_list(fw_list);
	}
}

static void _update_fw_file_list()
{
	guint i;
	gchar *dev = NULL;

	if (nfui_nfobject_is_shown(g_fw_list_obj) == FALSE)	return;

	dev = nfui_combobox_get_value(NF_COMBOBOX(g_dev_obj));

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

	memset(media_m_path, 0x00, sizeof(media_m_path));
    scm_get_mounted_path(media_info[i].id, media_m_path, 256);
    
	nfui_listbox_delete_all(NF_LISTBOX(g_fw_list_obj));

    _update_itx_fwfile(media_info[i].id, g_fw_list_obj);
    _update_techwin_fwfile(media_info[i].id, g_fw_list_obj);
    _update_idis_fwfile(media_info[i].id, g_fw_list_obj);
		
	nfui_signal_emit(g_fw_list_obj, GDK_EXPOSE, TRUE);
}

static gint _update_channel_list()
{
	char *file_name = NULL;
	gchar full_path[512];
	guint ch_mask = 0;
	gint i = 0;


    memset(full_path, 0x00, sizeof(full_path));
    
	if (nfui_nfobject_is_disabled(g_fw_list_obj) == FALSE)
	{
		file_name = nfui_listbox_get_focus_text((NFLISTBOX*)g_fw_list_obj, 0);
		g_message("%s, %d, file_name:%s", __FUNCTION__, __LINE__, file_name);

		sprintf(full_path, "%s/%s", media_m_path, file_name);
		g_message("%s, %d, full_path:%s", __FUNCTION__, __LINE__, full_path);

		if (full_path) {
			nf_ipcam_fw_capability_chk(full_path, &ch_mask);
			g_message("%s, %d, ch_mask:%08X", __FUNCTION__, __LINE__, ch_mask);
		}	
	}

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (ch_mask & (1 << i))
		{
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_obj[i], TRUE);		
			nfui_check_button_sensitive((NFCHECKBUTTON*)g_ch_obj[i], TRUE);					
		}
		else
		{
			nfui_check_button_set_active_no_expose((NFCHECKBUTTON*)g_ch_obj[i], FALSE);
			nfui_check_button_sensitive((NFCHECKBUTTON*)g_ch_obj[i], FALSE);
		}

		nfui_signal_emit(g_ch_obj[i], GDK_EXPOSE, TRUE);
	}			

	if (ch_mask) nfui_nfobject_enable(g_upgrade_obj);
	else nfui_nfobject_disable(g_upgrade_obj);
	
	nfui_signal_emit(g_upgrade_obj, GDK_EXPOSE, TRUE);			
	return 0;
}

static void _update_info_dev_changed(void)
{
	_update_dev_list();
	_update_fw_file_list();
	_update_channel_list();	
}

static gboolean post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		_update_fw_file_list();
		_update_channel_list();
	}

	return FALSE;
}

static gboolean post_fw_list_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == NFEVENT_LISTBOX_CHANGED) 
	{
		g_message("%s, %d", __FUNCTION__, __LINE__);	
		_update_channel_list();
	}

	return FALSE;
}

static gboolean post_upgradebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint file_idx = 0;
		guint ch_mask = 0;		
		gboolean ret;
		guint i;

		if(evt->button.button == 3)	return FALSE;

		file_idx = nfui_listbox_get_focus_idx((NFLISTBOX*)g_fw_list_obj);
		
		if (file_idx < 0) {
			nftool_mbox(g_curwnd, "ERROR", "Please select a file\nfor F/W upgrading.", NFTOOL_MB_OK);
			return FALSE;
		}

		g_message("%s, %d, SELECTED FILE INDEX:%d\n", __FUNCTION__, __LINE__, file_idx);

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
		{
			if (nfui_check_button_get_active(g_ch_obj[i]))
			{				
				ch_mask |= (1 << i);
			}
		}

		if (ch_mask == 0) {
			nftool_mbox(g_curwnd, "ERROR", "Please select a channel\nfor F/W upgrading.", NFTOOL_MB_OK);
			return FALSE;
		}

		ret = nftool_mbox(g_curwnd, "CONFIRM", "Do you want to upgrade firmware?", NFTOOL_MB_OKCANCEL);

		if (ret == NFTOOL_MB_OK)
		{
			gchar *dev = NULL;
			char *file_name = NULL;						
			char mount_path[128];
			char full_name[256];

			dev = nfui_combobox_get_value(NF_COMBOBOX(g_dev_obj));

			for (i = 0; i < media_cnt; i++)
			{
				if(!strcmp(dev, media_info[i].title))
					break;
			}

			if (i == media_cnt)
			{
				g_warning("%s, %d, media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, media_cnt);
				return FALSE;
			}

			scm_get_mounted_path(media_info[i].id, mount_path, 128);
			g_message("%s, %d, mount_path:%s", __FUNCTION__, __LINE__, mount_path);

			file_name = nfui_listbox_get_focus_text((NFLISTBOX*)g_fw_list_obj, 0);
			g_message("%s, %d, file_name:%s", __FUNCTION__, __LINE__, file_name);

			g_sprintf(full_name, "%s/%s", mount_path, file_name);
			g_message("%s, %d, full_name:%s", __FUNCTION__, __LINE__, full_name);

			if (scm_upgrade_ipcam_fw(ch_mask, full_name, IRET_SCM_UPGRADE_IPCAM_FW) == -1) 
			{
				nftool_mbox(g_curwnd, "NOTICE", "F/W Upgrade Fail.", NFTOOL_MB_OK);
				return FALSE;
			}

			VW_IPCamera_UPGrade_Progress_Open(g_curwnd, ch_mask, 1, 0);
			_update_channel_list();		
		}
		else if(ret == NFTOOL_MB_CANCEL)
		{
			return FALSE;
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
		nftool_destroy_setup_window(topwin);
	}

	return FALSE;
}

static gboolean pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	int result;

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

		case INFY_MEDIA_STATUS_CHANGED:
		{
			_update_info_dev_changed();
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

void VW_IPCamera_FWUpgrade_Open(NFWINDOW *parent)
{
	NFOBJECT *win;
	NFOBJECT *main_fixed;
	NFOBJECT *chk_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	gchar strBuf[8];

	guint lc_size[] = {FW_UP_FILE_CELL_W, };
	guint tbl_w[4];
	guint li_size_w, li_size_h;
	guint chk_w, chk_h;	
	guint i;

// <---- WINDOW
	win = (NFOBJECT*)nfui_nfwindow_new(parent, FW_UP_WIN_SIZE_X, FW_UP_WIN_SIZE_Y, FW_UP_WIN_SIZE_W, FW_UP_WIN_SIZE_H);
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(win, post_main_win_event_handler);
	g_curwnd = win;

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, FW_UP_WIN_SIZE_W, FW_UP_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FW UPGRADE", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, FW_UP_WIN_SIZE_W-40, FW_UP_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 20, 4);

// <---- DEVICE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEVICE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, FW_UP_DEV_LABEL_W, FW_UP_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_DEV_LABEL_X, FW_UP_DEV_LABEL_Y);

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(obj, FW_UP_DEV_CELL_W, FW_UP_LABEL_H);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_DEV_CELL_X, FW_UP_DEV_CELL_Y);
	nfui_regi_post_event_callback(obj, post_device_event_handler);
	g_dev_obj = obj;

// <---- DATA FILE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("F/W LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, FW_UP_FILE_LABEL_W, FW_UP_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_FILE_LABEL_X, FW_UP_FILE_LABEL_Y);
	
	nfui_get_image_size(IMG_N_SCROLL_UP, &li_size_w, &li_size_h);	
    lc_size[0] -= li_size_w;
	
	obj = nfui_listbox_new(1, lc_size, 40);	
	nfui_listbox_set_skin_type(NF_LISTBOX(obj), NFLISTBOX_TYPE_POPUP_1);
	nfui_listbox_set_pango_font(NF_LISTBOX(obj), nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_listbox_support_multi_lang(NF_LISTBOX(obj), FALSE);
	nfui_nfobject_set_size(obj, FW_UP_FILE_CELL_W, FW_UP_FILE_CELL_H);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(230));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_ON);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_FILE_CELL_X, FW_UP_FILE_CELL_Y);
	nfui_regi_post_event_callback(obj, post_fw_list_event_handler);
	g_fw_list_obj = obj;

// <---- UPGRADE CHANNEL

	for (i = 0; i < 4; i++)
	{
		tbl_w[i] = (FW_UP_CHANNEL_CELL_W-3)/4;
	}

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL LIST", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, FW_UP_FILE_LABEL_W, FW_UP_LABEL_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_CHANNEL_LABEL_X, FW_UP_CHANNEL_LABEL_Y);

	tbl = (NFOBJECT*)nfui_nftable_new(4, FW_UP_CHANNEL_LIST_ROW, 1, 1, tbl_w, 40);	
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)main_fixed, tbl, FW_UP_CHANNEL_CELL_X, FW_UP_CHANNEL_CELL_Y);

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		chk_fixed = nfui_nffixed_new();
		nfui_nfobject_set_size(chk_fixed, tbl_w[0], 40);
		nfui_nfobject_modify_bg(chk_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_show(chk_fixed);
		nfui_nftable_attach(tbl, chk_fixed, i%4, i/4);

		obj = nfui_checkbutton_new(FALSE);
		nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
		nfui_check_get_size(obj, &chk_w, &chk_h);
		nfui_check_button_sensitive(obj, FALSE);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)chk_fixed, obj, 0, (40-chk_h)/2);
		g_ch_obj[i] = obj;

		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "CH %d", i+1);
		
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_set_size(obj, tbl_w[0]-36, FW_UP_LABEL_H);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)chk_fixed, obj, 36, 0);
	}

// <---- UPGRADE BUTTON
	obj = nftool_normal_button_create_type1("UPGRADE", FW_UP_LOAD_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_LOAD_BTN_X, FW_UP_LOAD_BTN_Y);
	nfui_regi_post_event_callback(obj, post_upgradebutton_event_handler);
	g_upgrade_obj = obj;

// <---- CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", FW_UP_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, FW_UP_CLOSE_BTN_X, FW_UP_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	
	nfui_nfwindow_add((NFWINDOW*)win, main_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	_update_info_dev_changed();
	
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);	

	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)win);
	nfui_set_key_focus(obj, TRUE);

	nfui_page_close(PGID_POPUPWND, win);
	nfui_page_open(PGID_SYS_FWUP, win, NULL);

	gtk_main();

	nfui_page_close(PGID_SYS_FWUP, win);
}


