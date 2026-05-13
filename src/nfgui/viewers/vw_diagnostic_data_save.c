
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
#include "vw_system_data_save.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"


#define SDS_WIN_SIZE_W					(652)
#define SDS_WIN_SIZE_H					(252)

#define SDS_TITLE_H						(36)

#define SDS_LABEL_H						(40)

#define SDS_TABLE_COL 					(2)
#define SDS_TABLE_ROW					(2) 

#define SDS_TABLE_COL_SPACE				(10)
#define SDS_TABLE_ROW_SPACE				(3)

#define SDS_TABLE_X 					(26)
#define SDS_TABLE_Y						(64) 

#define SDS_SAVE_BTN_X					(146)
#define SDS_SAVE_BTN_Y					(SDS_WIN_SIZE_H - 74)
#define SDS_SAVE_BTN_W					(174)

#define SDS_CLOSE_BTN_X					(SDS_SAVE_BTN_X + SDS_SAVE_BTN_W + 6)
#define SDS_CLOSE_BTN_Y					(SDS_SAVE_BTN_Y)
#define SDS_CLOSE_BTN_W					(174)


#define	SYSTEM_DATA_NAME				"system_data"

#define MAX_STR_LEN						(63)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT 	*dev_obj = NULL;
static NFOBJECT 	*name_obj = NULL;
static NFOBJECT 	*saveBtn_obj = NULL;
static NFOBJECT 	*wait_pop = NULL;

static gint 		media_cnt;
static MEDIA_INFO_T	*media_info;


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

static void _get_usb_path(char *path)
{
	guint i;
	gchar *dev = NULL;
	gchar mnt_dir[128];

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

	strcpy(path, mnt_dir);
}

static gboolean post_savebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	int ret = 0;
	gchar full_file_name[256];

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		memset(full_file_name, 0, sizeof(full_file_name));
		_get_usb_path(full_file_name);

		wait_pop = nftool_mbox_wait_with_graph(g_curwnd, "WAIT", "Please wait...", "Saving diagnostic data");
		scm_export_debug_data(IRET_EXPORT_DEBUG_DATA, full_file_name);
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
			uxm_unreg_imsg_event(obj, IRET_EXPORT_DEBUG_DATA);

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

		case IRET_EXPORT_DEBUG_DATA:
		{
			gint ret = ((CMM_MESSAGE_T*)data)->param;

			g_message("[%s, %d] ret:%d", __FUNCTION__, __LINE__, ret);

			if (wait_pop)
			{
				nftool_remove_waitbox(wait_pop);
				wait_pop = NULL;
			}

			if (ret == 0) {
				nftool_mbox(g_curwnd, "NOTICE", "The diagnostic data has been saved successfully.", NFTOOL_MB_OK);
			}
			else {
				nftool_mbox(g_curwnd, "ERROR", "Failed to save diagnostic data.\nPlease try again.", NFTOOL_MB_OK);
			}
		}

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


void VW_Diagnostic_Data_Save_Open(NFWINDOW *parent)
{
	NFOBJECT *sds_win;
	NFOBJECT *main_fixed;

	NFOBJECT *ntb;
	NFOBJECT *obj;

	gchar *strLabel[] = {"DEVICE", "FILE NAME"};
	guint table_w[] = {156, 427};

	guint i;
	gint win_x, win_y;

	gchar datetime[256];
	gchar filename[256];

	memset(filename, 0x00, sizeof(filename));
	memset(datetime, 0x00, sizeof(datetime));

	if (g_curwnd) return;

	win_x = (DISPLAY_ACTIVE_WIDTH - SDS_WIN_SIZE_W) / 2;
	win_y = (DISPLAY_ACTIVE_HEIGHT - SDS_WIN_SIZE_H) / 2;

// <---- WINDOW
	sds_win = (NFOBJECT*)nfui_nfwindow_new(parent, win_x, win_y, SDS_WIN_SIZE_W, SDS_WIN_SIZE_H);
	g_curwnd = sds_win;
	nfui_nfobject_modify_bg(sds_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(sds_win, post_main_win_event_handler);

// <---- FIXED
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, SDS_WIN_SIZE_W, SDS_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);

// <---- TITLE
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SAVE DIAGNOSTIC DATA", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 500, SDS_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 76, 4);

// <---- TBL
	ntb = (NFOBJECT*)nfui_nftable_new(SDS_TABLE_COL, SDS_TABLE_ROW, SDS_TABLE_COL_SPACE, SDS_TABLE_ROW_SPACE, table_w, SDS_LABEL_H);	
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb, SDS_TABLE_X, SDS_TABLE_Y);

	for(i = 0; i < 1; i++) {
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
	g_sprintf(filename, "%s_%s", SYSTEM_DATA_NAME, datetime);	

	name_obj = (NFOBJECT*)nfui_nflabel_new_text_box(filename, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)name_obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)name_obj, NFALIGN_LEFT, 4);
	// nfui_nfobject_show(name_obj);
	nfui_nftable_attach((NFTABLE*)ntb, name_obj,  1, 1);
	nfui_regi_post_event_callback(name_obj, post_name_event_handler);

// <---- SAVE BUTTON
	saveBtn_obj = nftool_normal_button_create_type1("SAVE", SDS_SAVE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(saveBtn_obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(saveBtn_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, saveBtn_obj, SDS_SAVE_BTN_X, SDS_SAVE_BTN_Y);
	nfui_regi_post_event_callback(saveBtn_obj, post_savebutton_event_handler);

// <---- CANCEL BUTTON
	obj = nftool_normal_button_create_type1("CLOSE", SDS_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SDS_CLOSE_BTN_X, SDS_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);


	nfui_nfwindow_add((NFWINDOW*)sds_win, main_fixed);
	nfui_run_main_event_handler(sds_win);
	nfui_nfobject_show(sds_win);

	/* set for key navi */
	nfui_make_key_hierarchy((NFWINDOW*)sds_win);
	nfui_set_key_focus(saveBtn_obj, TRUE);

	_update_dev_list();
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);
	uxm_reg_imsg_event(main_fixed, IRET_EXPORT_DEBUG_DATA);

	nfui_page_open(PGID_DIAGNOSTIC_DATA_SAVE_POPUP, sds_win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_DIAGNOSTIC_DATA_SAVE_POPUP, sds_win);
}
