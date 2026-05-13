
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

#include "uxm.h"
#include "scm.h"
#include "ssm.h"
#include "iux_msg.h"

#include "vw.h"
#include "vw_camera_install_recorder_data_save.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"


#define CDS_WIN_SIZE_W					(652)
#define CDS_WIN_SIZE_H					(252)

#define CDS_TITLE_H						(36)

#define CDS_LABEL_H						(40)

#define CDS_TABLE_COL 					(2)
#define CDS_TABLE_ROW					(2) 

#define CDS_TABLE_COL_SPACE				(10)
#define CDS_TABLE_ROW_SPACE				(3)

#define CDS_TABLE_X 					(26)
#define CDS_TABLE_Y						(64) 

#define CDS_SAVE_BTN_X					(146)
#define CDS_SAVE_BTN_Y					(CDS_WIN_SIZE_H - 74)
#define CDS_SAVE_BTN_W					(174)

#define CDS_CLOSE_BTN_X					(CDS_SAVE_BTN_X + CDS_SAVE_BTN_W + 6)
#define CDS_CLOSE_BTN_Y					(CDS_SAVE_BTN_Y)
#define CDS_CLOSE_BTN_W					(174)


#define	RECORDER_DATA_NAME				"recorder_data"

#define MAX_STR_LEN						(63)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT 	*dev_obj = NULL;
static NFOBJECT 	*name_obj = NULL;
static NFOBJECT 	*saveBtn_obj = NULL;
static NFOBJECT 	*wait_pop = NULL;

static gint 		media_cnt;
static MEDIA_INFO_T	*media_info;
static gint g_idx = 0;


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
		}
	}

	if (cnt == 0)
	{
		nfui_combobox_append_data(dev_obj, "NONE");
		nfui_nfobject_disable(saveBtn_obj);		
	}
	else
		nfui_nfobject_enable(saveBtn_obj);			

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(saveBtn_obj, GDK_EXPOSE, TRUE);	
}


static void _get_path_DataSave(char *path)
{
	guint i;
	gchar *dev = NULL;
	gchar mnt_dir[128];
	gchar file_name[MAX_STR_LEN+1];
	gchar *pname;

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

	memset(mnt_dir, 0, sizeof(mnt_dir));
	scm_get_mounted_path(media_info[i].id, mnt_dir, 128);

	pname = nfui_nflabel_get_text((NFLABEL*)name_obj);
	strcpy(file_name, pname);
	
	g_sprintf(path, "%s/%s.json", mnt_dir, file_name);	

}

static gboolean post_savebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = -1;
	gchar full_file_name[256];
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(full_file_name, 0, sizeof(full_file_name));
		_get_path_DataSave(full_file_name);
	
        ret = nvm_export_data(g_idx, full_file_name);
        
        if (ret == 0) {
            nftool_mbox(g_curwnd, "NOTICE", "Successfully saved recorder data.", NFTOOL_MB_OK);
        }
        else {
            nftool_mbox(g_curwnd, "ERROR", "Failed to save recorder data.", NFTOOL_MB_OK);
        }
    }

	return FALSE;
}

static gboolean post_name_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	gchar *memo = NULL;

	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
	guint x, y;
	mb_type ret = NFTOOL_MB_OK;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
	    {
    		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

    		nfui_nfobject_get_window_pos(obj, &x, &y);

    		x += ((GdkEventButton*)evt)->x;
    		y += ((GdkEventButton*)evt)->y;
		}
			
		memo = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 32, VKEY_ITXSTYLE_TITLE);
        if(!memo) return FALSE;

		if(strlen(memo) <= 0) 
		{
			nftool_mbox(g_curwnd, "NOTICE", "There is no input.\nPlease enter a filename.", NFTOOL_MB_OK);
		}
        else
        {
		    nfui_nflabel_set_text((NFLABEL*)obj, memo);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
        }

		ifree(memo);
		memo = NULL;
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

			if (media_info)
			{
				scm_free_media_list(media_info);
				media_info = 0;
			}
        }
		break;

		case INFY_MEDIA_STATUS_CHANGED:
			_update_dev_list();
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


void VW_Camera_Install_Recorder_Data_Save_Open(NFWINDOW *parent, gchar *title, gint idx)
{
	NFOBJECT *sds_win;
	NFOBJECT *main_fixed;

	NFOBJECT *ntb;
	NFOBJECT *obj;

	gchar *strLabel[] = {"DEVICE", "FILE NAME"};
	guint table_w[] = {156, 427};

	guint i;

	gchar datetime[256];
	gchar filename[256];
	memset(filename, 0x00, sizeof(filename));
	memset(datetime, 0x00, sizeof(datetime));

	g_idx = idx;

// <---- WINDOW
	sds_win = (NFOBJECT*)nfui_nfwindow_new(parent, (DISPLAY_ACTIVE_WIDTH-CDS_WIN_SIZE_W)/2, (DISPLAY_ACTIVE_HEIGHT-CDS_WIN_SIZE_H)/2, CDS_WIN_SIZE_W, CDS_WIN_SIZE_H);
	g_curwnd = sds_win;
	nfui_nfobject_modify_bg(sds_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(sds_win, post_main_win_event_handler);

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, CDS_WIN_SIZE_W, CDS_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(title, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 500, CDS_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 76, 4);

// <---- TBL
	ntb = (NFOBJECT*)nfui_nftable_new(CDS_TABLE_COL, CDS_TABLE_ROW, CDS_TABLE_COL_SPACE, CDS_TABLE_ROW_SPACE, table_w, CDS_LABEL_H);	
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb, CDS_TABLE_X, CDS_TABLE_Y);

	for(i = 0; i < CDS_TABLE_ROW; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strLabel[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)ntb, obj,  0, i);
	}

	dev_obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(dev_obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_show(dev_obj);
	nfui_nftable_attach((NFTABLE*)ntb, dev_obj,  1, 0);

	dtf_get_local_datetime_ex(time(0), datetime);
	g_sprintf(filename, "%s_%s", RECORDER_DATA_NAME, datetime);	

	name_obj = (NFOBJECT*)nfui_nflabel_new_text_box(filename, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)name_obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)name_obj, NFALIGN_LEFT, 4);
	nfui_nfobject_show(name_obj);
	nfui_nftable_attach((NFTABLE*)ntb, name_obj,  1, 1);
	nfui_regi_post_event_callback(name_obj, post_name_event_handler);

// <---- SAVE BUTTON
	saveBtn_obj = nftool_normal_button_create_type1("SAVE", CDS_SAVE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(saveBtn_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(saveBtn_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, saveBtn_obj, CDS_SAVE_BTN_X, CDS_SAVE_BTN_Y);
	nfui_regi_post_event_callback(saveBtn_obj, post_savebutton_event_handler);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", CDS_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, CDS_CLOSE_BTN_X, CDS_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


	nfui_nfwindow_add((NFWINDOW*)sds_win, main_fixed);
	nfui_run_main_event_handler(sds_win);
	nfui_nfobject_show(sds_win);

	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)sds_win);
	nfui_set_key_focus(saveBtn_obj, TRUE);

	_update_dev_list();
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);	

	nfui_page_open(PGID_CAMERA_INSTALL_CHANNEL_DATA_SAVE_POPUP, sds_win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_CAMERA_INSTALL_CHANNEL_DATA_SAVE_POPUP, sds_win);
}




