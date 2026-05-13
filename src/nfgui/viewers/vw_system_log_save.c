
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
#include "vw_system_log_save.h"

#include "vw_vkeyboard.h"
#include "ix_mem.h"

#define SLS_WIN_SIZE_W					(652)
#define SLS_WIN_SIZE_H					(180)

#define SLS_TITLE_H						(36)

#define SLS_LABEL_H						(40)

#define SLS_TABLE_COL 					(2)
#define SLS_TABLE_ROW					(2) 

#define SLS_TABLE_COL_SPACE				(10)
#define SLS_TABLE_ROW_SPACE				(3)

#define SLS_TABLE_X 					(26)
#define SLS_TABLE_Y						(64) 

#define SLS_SAVE_BTN_X					(146)
#define SLS_SAVE_BTN_Y					(SLS_WIN_SIZE_H - 64)
#define SLS_SAVE_BTN_W					(174)

#define SLS_CLOSE_BTN_X					(SLS_SAVE_BTN_X + SLS_SAVE_BTN_W + 6)
#define SLS_CLOSE_BTN_Y					(SLS_SAVE_BTN_Y)
#define SLS_CLOSE_BTN_W					(174)


#define MAX_STR_LEN						(63)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *dev_obj = NULL;
static NFOBJECT *saveBtn_obj = NULL;

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
		nfui_combobox_append_data(dev_obj, "NO DEVICE");
		nfui_nfobject_disable(saveBtn_obj);		
	}
	else
		nfui_nfobject_enable(saveBtn_obj);			

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);
	nfui_signal_emit(saveBtn_obj, GDK_EXPOSE, TRUE);	
}

static gboolean _get_path_LogSave(gchar *path)
{
    guint i;
    gchar *dev = NULL;
    gchar mnt_dir[128];
    gchar file_name[MAX_STR_LEN+1];
    gchar strBuf[128];

    dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

    for(i=0; i< media_cnt; i++)
    {
        if(!strcmp(dev, media_info[i].title)) break;
    }

    if( i== media_cnt)
    {
        g_warning("%s, %d, media_cnt:%d, didn't find usb device", __FUNCTION__, __LINE__, media_cnt);
		return;
    }

    memset(mnt_dir, 0, sizeof(mnt_dir));
    memset(strBuf,  0, sizeof(strBuf));

    ifn_get_localtime_text_ex(time(0), YYYYMMDD, H24, strBuf);

    scm_get_mounted_path(media_info[i].id, mnt_dir, 128);

    g_sprintf(file_name,"%s_%s","System_log",strBuf);

    g_sprintf(path, "%s/%s.tgz",mnt_dir, file_name);

    return FALSE;
}

static gboolean _cp_logData_to_path(char *path)
{
    FILE *fsrc, *fdes;
    int a;

    if((fsrc = fopen("/tmp/system_log.tar.gz","rb"))==NULL)         return -1;

    if((fdes = fopen(path,"wb")) == NULL)
    {
        fclose(fsrc);
        return -1;
    }

    while(1)
    {
        a = fgetc(fsrc);

        if(!feof(fsrc))
        {
            fputc(a,fdes);
        }
        else
            break;
    }

    fclose(fsrc);
    fclose(fdes);

    return FALSE;
}

static gboolean post_save_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        gchar file_name[256];
        gint ret;
                           
        memset(file_name, 0x00, sizeof(file_name));

        _get_path_LogSave(file_name);

        ret = _cp_logData_to_path(file_name);

        if (ret == 0){
			nftool_mbox(g_curwnd, "NOTICE", "The system log has been saved successfully.", NFTOOL_MB_OK);
		}
		else {
			nftool_mbox(g_curwnd, "ERROR", "It's failed to save the system log.", NFTOOL_MB_OK);
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

void VW_System_Log_Save_Open(NFWINDOW *parent)
{
	NFOBJECT *sls_win;
	NFOBJECT *main_fixed;

	NFOBJECT *ntb;
	NFOBJECT *obj;

	guint i;
	
	sls_win = (NFOBJECT*)nfui_nfwindow_new(parent,(1920/2)-(SLS_WIN_SIZE_W/2), (1080/2)-(SLS_WIN_SIZE_H/2), SLS_WIN_SIZE_W, SLS_WIN_SIZE_H);
	g_curwnd = sls_win;
	nfui_nfobject_modify_bg(sls_win, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_regi_post_event_callback(sls_win, post_main_win_event_handler);

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, SLS_WIN_SIZE_W, SLS_WIN_SIZE_H);
	nfui_regi_pre_event_callback(main_fixed, pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);
	
	if (var_get_vendor_code() == 43)
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ANALYSIS LOG", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYSTEM LOG", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));

	nfui_nfobject_set_size(obj, 500, SLS_TITLE_H);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 76, 4);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DEVICE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 150, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 30, 60);
   
	dev_obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(dev_obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(dev_obj, SLS_WIN_SIZE_W-220,40);
	nfui_nfobject_show(dev_obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, dev_obj, 190, 60);

    obj =  nftool_normal_button_create_type1("SAVE", SLS_CLOSE_BTN_W);
    nfui_nfbutton_set_font_alignment((NFBUTTON *) obj, NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, SLS_SAVE_BTN_X, SLS_SAVE_BTN_Y);
    nfui_regi_post_event_callback(obj, post_save_event_handler);
    saveBtn_obj = obj;

	obj = nftool_normal_button_create_type1("CLOSE", SLS_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, SLS_CLOSE_BTN_X, SLS_SAVE_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_nfwindow_add((NFWINDOW*)sls_win, main_fixed);
	nfui_run_main_event_handler(sls_win);
	nfui_nfobject_show(sls_win);

	nfui_make_key_hierarchy((NFWINDOW*)sls_win);
	nfui_set_key_focus(obj, TRUE);

	_update_dev_list();
	uxm_reg_imsg_event(main_fixed, INFY_MEDIA_STATUS_CHANGED);	

	nfui_page_open(PGID_SYS_SAVE_DATA_POPUP, sls_win, ssm_get_cur_id(NULL));

	gtk_main();

	nfui_page_close(PGID_SYS_SAVE_DATA_POPUP, sls_win);
}




