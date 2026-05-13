#include "nf_afx.h"

#include "scm.h"
#include "iux_msg.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"
#include "support/multi_language_support.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfprogressbar.h"
#include "viewers/objects/nfcheckbutton.h"
#include "viewers/objects/nfcombobox.h"

#include "../../service/nf_va_statistic.h"
#include "viewers/vw_statistic_export.h"

#include "modules/ssm.h"

#include "nf_util_ftp.h"
#include "scm.h"
#include "vw.h"


#include "vw_vkeyboard.h"
#include "ix_mem.h"
#include "smt.h"
#include "uxm.h"


#define STATISTIC_EX_WIN_SIZE_W			(678)
#define STATISTIC_EX_WIN_SIZE_H			(507)

#define STATISTIC_EX_POS_X				((DISPLAY_ACTIVE_WIDTH - STATISTIC_EX_WIN_SIZE_W)/4*2)
#define STATISTIC_EX_POS_Y				((DISPLAY_ACTIVE_HEIGHT - STATISTIC_EX_WIN_SIZE_H)/2)

#define MAX_TAG_STRING_SIZE			    (14)
#define MAX_MEMO_STRING_SIZE		    (24)

enum {
    EXPORT_BTN,
    CLOSE_BTN,
    BUTTON_COUNT
};    

enum{
    AVI = 0,
    DISABLE_RAW,
    FORM_ALL
};

static NFOBJECT *aeWin;

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_aeWin;
static NFOBJECT *g_export[BUTTON_COUNT];
static NFOBJECT *g_tag;
static NFOBJECT *g_memo;

static NFOBJECT *dev_obj = NULL;
static NFOBJECT *form_obj = NULL;
static NFOBJECT *wait_pop = NULL;

static gint media_cnt;
static MEDIA_INFO_T *media_info;

static guint gsrc_id = 0;


// export data 

enum{
    EXPORT_CSV =0,
    EXPORT_GRAPH,
    EXPORT_INFO,
};

static STATISTIC_EXPORT_T *g_export_info;
static guint type_state = 0;


static gint _get_media_index(gint obj_index)
{
	gint i;
	gchar *dev = NULL;

	dev = nfui_combobox_get_value(NF_COMBOBOX(dev_obj));

	for (i = 0; i < media_cnt; i++)
	{
		if(!strcmp(dev, media_info[i].title))
			break;
	}

	if (i == media_cnt)
		return -1;

	return i;
}

static int _update_dev_list(NFOBJECT *obj)
{
	int i;
	int dev_cnt = 0;
	MEDIA_TYPE_E mtype;

	if (media_info) scm_free_media_list(media_info);
		nfui_combobox_remove_all(dev_obj);

	media_cnt = 0;	
	media_info = scm_new_media_list(&media_cnt);
	  
	for (i = 0; i < media_cnt; ++i) 
	{
	    mtype = scm_get_media_type(media_info[i].id);

	    if(mtype == MTYPE_USB){
			nfui_combobox_append_data(dev_obj, media_info[i].title);
            ++dev_cnt;
        }               
    }
	    
    if (!dev_cnt){
    
		nfui_combobox_append_data(dev_obj, "NO DEVICE");
	}

	nfui_signal_emit(dev_obj, GDK_EXPOSE, TRUE);

	return 0;
}

static int _set_focus_cancel_btn(NFOBJECT *cur_focus)
{
   	nfui_set_key_focus(cur_focus, FALSE);
   	nfui_signal_emit(cur_focus, GDK_EXPOSE, TRUE);
    
	nfui_set_key_focus(g_export[CLOSE_BTN], TRUE);
	nfui_signal_emit(g_export[CLOSE_BTN], GDK_EXPOSE, TRUE);

    return 0;
}

static gboolean post_tag_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *str = NULL;
		gchar tag[32];
		gchar user[16];
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) 
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_TAG_STRING_SIZE, VKEY_NORMAL);

		if(str) {
			g_stpcpy(tag, str);
			ifree(str);

			nfui_nflabel_set_text((NFLABEL*)obj, tag);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);
		}
	}
	
	return FALSE;
}

static gboolean post_memo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;

	if (evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{
		NFOBJECT *top;
		gchar *str = NULL;
		gchar user[16];
		guint x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) 
				return FALSE;

			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		str = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, MAX_MEMO_STRING_SIZE, VKEY_NORMAL);

		if (str) {
			nfui_nflabel_set_text((NFLABEL*)obj, str);
			nfui_signal_emit(obj, GDK_EXPOSE, FALSE);

			ifree(str);
		}
	}
	
	return FALSE;
}

static int _change_export_button()
{
	gint obj_index, media_index;

	obj_index = nfui_combobox_get_cur_index(dev_obj);
	media_index = _get_media_index(obj_index);

	if (media_index == -1)
	{
		nfui_nfobject_disable(g_export[EXPORT_BTN]);
		nfui_signal_emit(g_export[EXPORT_BTN], GDK_EXPOSE, FALSE);
	}	
	else if(type_state){
	    nfui_nfobject_enable(g_export[EXPORT_BTN]);
	    nfui_signal_emit(g_export[EXPORT_BTN], GDK_EXPOSE, FALSE);
	}
	else{
	    nfui_nfobject_disable(g_export[EXPORT_BTN]);
	    nfui_signal_emit(g_export[EXPORT_BTN], GDK_EXPOSE, FALSE);
	}

	return 0;
}

static gboolean post_device_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFEVENT_COMBOBOX_CHANGED) 
	{
		_change_export_button();
	}

	return FALSE;
}

static gboolean post_csv_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint ret;
        
        ret = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

        if(ret) type_state |= (1<< EXPORT_CSV);
        else    type_state &= ~(1<< EXPORT_CSV);

        _change_export_button();
    }
    
    return FALSE;
}

static gboolean post_graph_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {
        gint ret;

        ret = nfui_check_button_get_active(NF_CHECKBUTTON(obj));

        if(ret)  type_state |= (1<< EXPORT_GRAPH);
        else     type_state &= ~(1<<EXPORT_GRAPH);

        _change_export_button();
    }
    
    return FALSE;
}

static gboolean _get_export_data(va_statistic_export_t *export_data)
{
    GTimeVal start_time, end_time;
    gint i;

    start_time.tv_sec = g_export_info->start_time;
    end_time.tv_sec = g_export_info->end_time;
    start_time.tv_usec = 0;
    end_time.tv_usec = 0;
    
    export_data->ch = g_export_info->ch;
    export_data->period = g_export_info->period;
    export_data->start_time = GTIMEVAL_TO_GUINT64(start_time);
    export_data->end_time = GTIMEVAL_TO_GUINT64(end_time);
    export_data->total_events = g_export_info->total_events;

    for(i =0; i<IVCA_MAX_ZONES; i++)
    {
        export_data->rule_list[i] = g_export_info->rule_list[i];
    }

    return 0;
}

static gboolean _export_cvs_data(gchar *path)
{
    va_statistic_export_t export_data;
    FILE *fexp;
    gint ret;

    if ((fexp = fopen(path, "wb")) == NULL)
    {
        return FALSE;
    }

    memset(&export_data, 0x00, sizeof(va_statistic_export_t));

    _get_export_data(&export_data);

    export_data.fp = fexp;    

    ret = get_va_statistic_export_data(&export_data);
     
    fclose(fexp);

    return (ret > 0 ? TRUE : FALSE);
}

static gboolean _export_graph_data(gchar *path)
{
    gint ret;

    ret = ifn_save_pixbuf(g_export_info->pbuf, path);
   
    return (ret == 0 ? TRUE : FALSE);
}

static gboolean _get_csv_file_size(FILE *f_info, gchar *path)
{
    FILE *f_csv;
    gchar *p;
    gchar strbuf[256];
    gint len;

    memset(strbuf, 0x00, sizeof(strbuf));
    
    p = strchr(path, '.');

	if(p){
	    len = p - path;
	    strncpy(strbuf, path, len);
	    g_sprintf(strbuf,"%s%s",strbuf,".csv");	    
	}
	
    if((f_csv = fopen(strbuf, "r")) != NULL)
    {
        fseek(f_csv, 0, SEEK_END);   
        fprintf(f_info, " %s %d %s\r\n", "CSV File Size : ",ftell(f_csv),"bytes"); 
        fclose(f_csv);
    }

    return 0;
}

static gboolean _get_jpg_file_size(FILE *f_info, gchar *path)
{
    FILE *f_jpg;
    gchar *p;
    gchar strbuf[256];
    gint len;

    memset(strbuf, 0x00, sizeof(strbuf));

    p = strchr(path, '.');

    if(p){
        len = p - path;
        strncpy(strbuf, path, len);
        g_sprintf(strbuf, "%s%s", strbuf, ".jpg");
    }

    if((f_jpg = fopen(strbuf, "r")) != NULL)
    {
        fseek(f_jpg, 0, SEEK_END);
        fprintf(f_info, " %s %d %s\r\n", "JPG File Size : ",ftell(f_jpg),"bytes");
        fclose(f_jpg);
    }

    return 0;
}

static gboolean _export_information_data(gchar *path)
{
    FILE *f_info;
    gchar curTime[32];
    gchar startTime[32];
    gchar endTime[32];
    gchar zoneBuf[256];
    gchar strBuf[256];
    gchar *tag;
    gchar *memo;
    gchar mac[32];
    gchar *dst[2]={"OFF","ON"};
    gint i;
    
    if((f_info = fopen(path,"wb")) == NULL)
    {
        return 0;
    }

    memset(curTime, 0x00, sizeof(curTime));    
    memset(startTime, 0x00, sizeof(startTime));
    memset(endTime, 0x00, sizeof(endTime));  
    memset(strBuf, 0x00, sizeof(strBuf));

    i = DAL_get_Dst_data();
    DAL_get_mac_address(mac);
           
    ifn_get_localtime_text(time(0), YYYYMMDD, H24, curTime);
    ifn_get_localtime_text(g_export_info->start_time, YYYYMMDD, H24, startTime);
    ifn_get_localtime_text(g_export_info->end_time, YYYYMMDD, H24, endTime); 

    tag = nfui_nflabel_get_text(g_tag);
    memo = nfui_nflabel_get_text(g_memo);

    fprintf(f_info, "%s\r\n", "[VA Statistic Information] ");
    fprintf(f_info, " %s\r\n", "VA Statistic Info ");
    fprintf(f_info, " %s %s\r\n", "Recording Device MAC address = ",mac);
    fprintf(f_info, " %s %s\r\n", "Exported at   : ",curTime);
    fprintf(f_info, " %s %s\r\n", "Start Time    : ",startTime);
    fprintf(f_info, " %s %s\r\n", "End   Time    : ",endTime);
    fprintf(f_info, " %s %d %s\r\n", "Channel Selection Information :", g_export_info->ch+1,"Channel");
    
    _get_csv_file_size(f_info, path);
    _get_jpg_file_size(f_info, path);
    
    fprintf(f_info, " %s %s\r\n", "Tag  = ",tag);
    fprintf(f_info, " %s %s\r\n", "User = ",VW_get_User_Name());
    fprintf(f_info, " %s %s\r\n", "Memo = ",memo);
    fprintf(f_info, " %s %s\r\n", "DST  = ", dst[i]);
    fprintf(f_info, " %s\r\n", "Rule & Event");

    for(i=0; i<16; i++)
    {
        memset(zoneBuf, 0x00, sizeof(zoneBuf));

        if(!(g_export_info->exist_zone & (1<<i))) continue;

        if(g_export_info->rule_list[i])
        {
            if(g_export_info->rule_list[i] & IVCA_ET_DIR_POS){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","FORWARD");
                else strcat(zoneBuf,", FORWARD");   
            }
            if(g_export_info->rule_list[i] & IVCA_ET_DIR_NEG){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","REVERSE");
                else strcat(zoneBuf, ", REVERSE");
            }
            if(g_export_info->rule_list[i] & IVCA_ET_ENTER){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","ENTER");
                else strcat(zoneBuf, ", ENTER");
            }
            if(g_export_info->rule_list[i] & IVCA_ET_EXIT){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","EXIT");
                else strcat(zoneBuf, ", EXIT");
            }
            if(g_export_info->rule_list[i] & IVCA_ET_STOPPED){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","STOPPED");
                else strcat(zoneBuf, ", STOPPED");
            }
            if(g_export_info->rule_list[i] & IVCA_ET_REMOVED){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","REMOVED");
                else strcat(zoneBuf, ", REMOVED");
            }
            if(g_export_info->rule_list[i] & IVCA_ET_LOITERED){
                if(!strlen(zoneBuf))  g_sprintf(zoneBuf,"%s","LOITERED");
                else strcat(zoneBuf, ", LOITERED");
            }

            fprintf(f_info, " %s%02d : %s\r\n", " - ZONE",i+1, zoneBuf);        
        }
    }
   
    fclose(f_info);    
    
    return 1;
}

static gboolean _get_mount_path(gchar *folderPath)
{
    gchar mount_path[128];
    gchar curTime[256];
    gint obj_index, media_index;
    gint ret;
    
    memset(mount_path, 0x00, sizeof(mount_path));
    memset(curTime, 0x00, sizeof(curTime));

    obj_index = nfui_combobox_get_cur_index(dev_obj);
	media_index = _get_media_index(obj_index);

	if (media_index == -1) return 1;
    
    scm_get_mounted_path(media_info[media_index].id, mount_path, 128);
    dtf_get_local_datetime_ex(time(0), curTime);

    g_sprintf(folderPath,"%s/%s(%s)_%s",mount_path,"STATISTIC", nfui_nflabel_get_text((NFLABEL*)g_tag),curTime);

    ret = mkdir(folderPath, 0755);

    if(ret){
        g_message("\n >>>>>> %s >> %d >> FILE >>> MKDIR FAIL >>>>> \n", __FUNCTION__,__LINE__);

        return 1;
    }

    return 0;
}

static gboolean _get_path_export_data(gchar *path, gchar* folderPath, gint type)
{
    gchar fileName[256];
    gchar startTime[32];
    gchar endTime[32];
    gint ret;
    
    memset(fileName, 0x00, sizeof(fileName));
    memset(startTime, 0x00, sizeof(startTime));
    memset(endTime, 0x00, sizeof(endTime));

    ifn_get_localtime_text_ex(g_export_info->start_time, YYYYMMDD, H24, startTime);
    ifn_get_localtime_text_ex(g_export_info->end_time, YYYYMMDD, H24, endTime); 

    g_sprintf(fileName,"CH%02d_%s_%s-%s_%s", g_export_info->ch+1, nfui_nflabel_get_text((NFLABEL*)g_tag),startTime, endTime, VW_get_User_Name());
        
    switch(type)
    {
        case EXPORT_CSV  :
        {
            g_sprintf(path, "%s/%s.csv", folderPath, fileName);    
            break;
        }
        case EXPORT_GRAPH:
        {
            g_sprintf(path, "%s/%s.jpg", folderPath, fileName);

            break;
        }
        case EXPORT_INFO:
        {
            g_sprintf(path, "%s/%s.txt", folderPath, fileName);
            break;
        }
        default:
            break;
    }
    
    return FALSE;
}

static gboolean _export_csv_file(gchar *folderPath, guint *fail_check)
{
    gchar file_path[256];
    gint ret;

    memset(file_path, 0x00, sizeof(file_path));
    _get_path_export_data(file_path, folderPath, EXPORT_CSV);

    ret = _export_cvs_data(file_path);

    if(!ret) *fail_check |= (1<< EXPORT_CSV);

    return FALSE;
}

static gboolean _export_graph_file(gchar *folderPath, guint *fail_check)
{
    gchar file_path[256];
    gint ret;

    memset(file_path, 0x00, sizeof(file_path));            
    _get_path_export_data(file_path, folderPath, EXPORT_GRAPH);

    ret = _export_graph_data(file_path);            

    if(!ret) *fail_check |= (1<< EXPORT_GRAPH);

    return FALSE;
}

static gboolean _export_information_file(gchar *folderPath, guint *fail_check)
{
    gchar file_path[256];
    gint ret;

    memset(file_path, 0x00, sizeof(file_path));
    _get_path_export_data(file_path, folderPath, EXPORT_INFO);

    ret = _export_information_data(file_path);

    if(!ret) *fail_check |= (1<< EXPORT_INFO);

    return FALSE;
}

static gboolean post_export_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{ 
    char *tag;

	if(evt->type == GDK_BUTTON_PRESS) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        gchar folderPath[256];
		guint fail_check = 0;

		tag = nfui_nflabel_get_text(g_tag);
		
		if (strlen(tag) == 0) {
			nftool_mbox(g_curwnd, "WARNING", "The tag name is empty.", NFTOOL_MB_OK);
			return FALSE;
		}

        memset(folderPath, 0x00, sizeof(folderPath));

        if(_get_mount_path(folderPath)){
            nftool_mbox(g_curwnd, "WARNING", "The export operation occured an error.\nPlease retry.", NFTOOL_MB_OK);  
                
            return FALSE;
        }

        if (type_state & (1<< EXPORT_CSV)) _export_csv_file(folderPath, &fail_check);
        if (type_state & (1<< EXPORT_GRAPH)) _export_graph_file(folderPath, &fail_check);      

        _export_information_file(folderPath, &fail_check);
        
        if(fail_check)
            nftool_mbox(g_curwnd, "WARNING", "The export operation occured an error.\nPlease retry.", NFTOOL_MB_OK);  
        else 
            nftool_mbox(g_curwnd, "INFORMATION", "The system data has been saved successfully.", NFTOOL_MB_OK);

    }
    
    return FALSE;
}

static gboolean post_close_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top = NULL;
	int ret;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		ret = scm_cancel_burning();
		if (ret == -1) {
			top = nfui_nfobject_get_top(obj);
			nfui_nfobject_destroy(top);
		}
		else {
			if (!wait_pop) wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
		}
	}

	return FALSE;
} 

static gboolean post_aeWin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	mb_type ret = -1;
	NFOBJECT *top = NULL;
	
	switch (evt->type)
	{
		case INFY_MEDIA_STATUS_CHANGED:
			_update_dev_list(dev_obj);
			_change_export_button();
			break;
              
		case GDK_DELETE:
			{
				uxm_unreg_imsg_event(g_aeWin, INFY_MEDIA_STATUS_CHANGED);
				uxm_unreg_imsg_event(g_aeWin, IRET_SCM_TST_FTP);

				if (media_info) {
					scm_free_media_list(media_info);
					media_info = 0;
				}

				g_curwnd = 0;

				gtk_main_quit();
			}
			break;

		default:
			break;
	}

	return FALSE;
}


static gboolean post_ae_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if(evt->type == GDK_EXPOSE) {
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

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	int ret;

	if(!nfui_nfobject_is_disabled(g_export[CLOSE_BTN]))
	{
		ret = scm_cancel_burning();
		if (ret != -1) 
		{	
			if (!wait_pop) wait_pop = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");
				return FALSE;
		}
		else
			return TRUE;
	}
	else
		return FALSE;
}

/////////////////////////////////////////////////////////////////////
//
//
//

void VW_StatisticExport_Open(NFWINDOW *parent, STATISTIC_EXPORT_T *export_info)
{
	NFOBJECT *aeFixed;
	NFOBJECT *obj;
    
	GdkPixbuf *prog_img[4];

	gint i;
	gint size_w, size_h;

	gchar strbuf[32];

	g_export_info = export_info;

	/* window */
	g_aeWin = (NFOBJECT*)nfui_nfwindow_new(parent, STATISTIC_EX_POS_X, STATISTIC_EX_POS_Y, STATISTIC_EX_WIN_SIZE_W, STATISTIC_EX_WIN_SIZE_H);
	g_curwnd = g_aeWin;
	nfui_regi_post_event_callback(g_aeWin, post_aeWin_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_aeWin, returnkey_proc);

	/* fixed */
	aeFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(aeFixed, STATISTIC_EX_WIN_SIZE_W, STATISTIC_EX_WIN_SIZE_H);
	nfui_regi_post_event_callback(aeFixed, post_ae_fixed_event_cb);
	nfui_nfobject_show(aeFixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STATISTIC EXPORT", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 670, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 11);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 4, 4);

	/* subtitle */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REPOSITORY", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 210, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 15, 60);
	
	// COMBO
	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 270, 40); 
	nfui_nfobject_show(obj); 
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210, 60); 
	nfui_regi_post_event_callback(obj, post_device_event_handler);
	dev_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DATA TYPE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 210, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 15, 102);
    
    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210, 102+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_csv_check_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CSV", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 100, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210+size_w+10, 102);
    
    obj = (NFOBJECT*)nfui_checkbutton_new(FALSE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
	nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210+size_w+20+100+30, 102+(40-size_h)/2);
    nfui_regi_post_event_callback(obj, post_graph_check_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("GRAPH", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 100, 40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210+size_w+20+100+size_w+10+30, 102);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("START TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 200,40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)aeFixed, obj, 15, 142);

    memset(strbuf, 0x00, sizeof(strbuf));
    ifn_get_localtime_text(export_info->start_time, YYYYMMDD, H24, strbuf);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strbuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 446, 40);;
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210, 142);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("END TIME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 200,40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)aeFixed, obj, 15, 184);

    memset(strbuf, 0x00, sizeof(strbuf));
    ifn_get_localtime_text(export_info->end_time, YYYYMMDD, H24, strbuf);
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strbuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 446, 40);;
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)aeFixed, obj, 210, 184);
    
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TAG NAME", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 641, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 15, 222);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MEMO", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 641, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)aeFixed, obj, 15, 303);
	
	g_tag = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_tag, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_set_align((NFLABEL*)g_tag, NFALIGN_LEFT, 4);
	nfui_nfobject_support_multi_lang((NFOBJECT*)g_tag, FALSE);
	nfui_nfobject_set_size(g_tag, 641, 40);
	nfui_regi_post_event_callback(g_tag, post_tag_event_handler);
	nfui_nfobject_show(g_tag);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_tag, 15, 256);

	g_memo = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)g_memo, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nfobject_set_size(g_memo, 641, 100);
	nfui_nflabel_set_align((NFLABEL*)g_memo, NFALIGN_LEFT, 4);
	nfui_regi_post_event_callback(g_memo, post_memo_event_handler);
	nfui_nfobject_show(g_memo);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_memo, 15, 337);

	g_export[EXPORT_BTN] = nftool_normal_button_create_type1("EXPORT", 182);
	nfui_regi_post_event_callback(g_export[EXPORT_BTN], post_export_button_event_cb);
	nfui_nfobject_show(g_export[EXPORT_BTN]);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_export[EXPORT_BTN], 474-10-182, 447);

	g_export[CLOSE_BTN] = nftool_normal_button_create_type1("CLOSE", 182);
	nfui_regi_post_event_callback(g_export[CLOSE_BTN], post_close_button_event_cb);
	nfui_nfobject_show(g_export[CLOSE_BTN]);
	nfui_nffixed_put((NFFIXED*)aeFixed, g_export[CLOSE_BTN], 474, 447);

	nfui_nfwindow_add((NFWINDOW*)g_aeWin, aeFixed);
	nfui_run_main_event_handler(g_aeWin);
	nfui_nfobject_show(g_aeWin);

	nfui_make_key_hierarchy((NFWINDOW*)g_aeWin);
	nfui_set_key_focus(dev_obj, TRUE);

	_update_dev_list(dev_obj);
	_change_export_button();
	
	uxm_reg_imsg_event(g_aeWin, INFY_MEDIA_STATUS_CHANGED);
	uxm_reg_imsg_event(g_aeWin, IRET_SCM_TST_FTP);

	gtk_main();

}


