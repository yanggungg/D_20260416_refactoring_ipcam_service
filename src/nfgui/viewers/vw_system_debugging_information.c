#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nftimelabel.h"
#include "objects/nfpiechart.h"

#include "services/vsm.h"

#include "modules/ssm.h"

#include "vw_system_debugging_information.h"
#include "vw_set_date_time.h"
#include "vw.h"

#include "uxm.h"

#define WIN_WIDTH                   (700)
#define WIN_HEIGHT                  (500)

#define TIME_X                      (10)
#define TIME_Y                      (44)    

#define IPADRESS_IN_X               (10)
#define IPADRESS_IN_Y               (TIME_Y+36)

#define IPADRESS_EX_X               (10)
#define IPADRESS_EX_Y               (IPADRESS_IN_Y+36)

#define NAND_LABEL_X                (30)
#define NAND_LABEL_Y                (44+100)//90x

#define MEMORY_LABEL_X              ((WIN_WIDTH/3)+20)
#define MOMORY_LABEL_Y              (NAND_LABEL_Y)

#define CPU_LABEL_X                 (((WIN_WIDTH/3)*2)+20)
#define CPU_LABEL_Y                 (NAND_LABEL_Y)

#define PIE_CHART_WIDTH             (WIN_WIDTH/4+20)

#define PIE_CHART1_X                (PIE_CHART_WIDTH/2-30)
#define PIE_CHART1_Y                ((WIN_HEIGHT/2)-20)//70

#define PIE_CHART2_X                ((WIN_WIDTH/3)+PIE_CHART1_X)
#define PIE_CHART2_Y                (PIE_CHART1_Y)

#define PIE_CHART3_X                (((WIN_WIDTH/3)*2)+PIE_CHART1_X)
#define PIE_CHART3_Y                (PIE_CHART1_Y)

#define NAND_DATA1_COLOR_X          (30)
#define NAND_DATA1_COLOR_Y          ((PIE_CHART1_Y+(PIE_CHART_WIDTH/2)+40)+15)
#define NAND_DATA1_X                (NAND_DATA1_COLOR_X+20)
#define NAND_DATA1_Y                (NAND_DATA1_COLOR_Y)

#define NAND_DATA2_COLOR_X          (NAND_DATA1_COLOR_X)
#define NAND_DATA2_COLOR_Y          ((NAND_DATA1_COLOR_Y+40)-5)
#define NAND_DATA2_X                (NAND_DATA1_X)
#define NAND_DATA2_Y                (NAND_DATA2_COLOR_Y)

#define MEMORY_DATA1_COLOR_X        ((WIN_WIDTH/3)+NAND_DATA1_COLOR_X)
#define MEMORY_DATA1_COLOR_Y        (NAND_DATA1_COLOR_Y)
#define MEMORY_DATA1_X              (MEMORY_DATA1_COLOR_X+20)
#define MEMORY_DATA1_Y              (NAND_DATA1_COLOR_Y)

#define MEMORY_DATA2_COLOR_X        (MEMORY_DATA1_COLOR_X)
#define MEMORY_DATA2_COLOR_Y        (NAND_DATA2_COLOR_Y)
#define MEMORY_DATA2_X              (MEMORY_DATA1_X)
#define MEMORY_DATA2_Y              (NAND_DATA2_COLOR_Y)

#define CPU_DATA1_COLOR_X           (((WIN_WIDTH/3)*2)+20)
#define CPU_DATA1_COLOR_Y           (NAND_DATA1_COLOR_Y)
#define CPU_DATA1_X                 (CPU_DATA1_COLOR_X+20)
#define CPU_DATA1_Y                 (NAND_DATA1_COLOR_Y)

#define CPU_DATA2_COLOR_X           (CPU_DATA1_COLOR_X)
#define CPU_DATA2_COLOR_Y           (NAND_DATA2_COLOR_Y)
#define CPU_DATA2_X                 (CPU_DATA1_X)
#define CPU_DATA2_Y                 (NAND_DATA2_COLOR_Y)

#define STR_SERVICE(use_ssl)   ((use_ssl == 1) ? "https://" : "http://")

typedef struct _NF_SYSMAN_USAGE_INFO_T
{
	guint total;
	guint used;
	
} NF_SYSMAN_USAGE_INFO;

typedef enum {

    DB_EXPORT = 0,
    LOG_EXPORT,
    
}SaveType;

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,

	NUM_DATE_FORMATS,
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_parent = 0;
static NFOBJECT *g_time = 0; 

static NF_SYSMAN_USAGE_INFO nand_info;
static NF_SYSMAN_USAGE_INFO mem_info;
static guint cpu_info;

static guint t_timer = 0;
static guint g_time_update = 0;


static gboolean _update_time(gpointer data)
{
    NFTIMELABEL* time_obj;
    GTimeVal tv;
    GTimeVal tv_temp;

    memset(&tv, 0x00, sizeof(GTimeVal));
    memset(&tv_temp, 0x00, sizeof(GTimeVal));

    //time_obj = (NFTIMELABEL*)data;

    g_get_current_time(&tv);

    NFUTIL_THREADS_ENTER();
    
    nfui_nftimelabel_get_datetime(g_time, &tv_temp);

    if(tv.tv_sec != tv_temp.tv_sec)
    {
        nfui_nftimelabel_set_datetime_expose(g_time, &tv);
    }
    NFUTIL_THREADS_LEAVE();

    return TRUE; 
}

static nftl_df_type _prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if(db_index == DF_YMD)			ret = NFTL_DF_YMD;
	else if(db_index == DF_MDY)		ret = NFTL_DF_MDY;
	else if(db_index == DF_DMY)		ret = NFTL_DF_DMY;
	else	ret = NFTL_DF_HIDE;

	return ret;
}

static gboolean _get_debug_info()
{
    gint i;

    memset(&mem_info, 0x00, sizeof(NF_SYSMAN_USAGE_INFO));
    memset(&nand_info, 0x00, sizeof(NF_SYSMAN_USAGE_INFO));
    cpu_info = 0;

    if(!nf_sysman_get_mem_usage(&mem_info)) g_message("\n get_fail_memory_debugging information \n");
    if(!nf_sysman_get_nand_usage(&nand_info)) g_message("\n get_fail_nand_debugging information \n");
    if(!nf_sysman_get_cpu_usage(&cpu_info)) g_message("\n get_fail_cpu_debugging information \n");
    
    nfui_user_signal_emit(g_curwnd, NFEVENT_DEBUG_INFOR_DATA_SYNC, TRUE);
    
    return TRUE;
}

static void _get_path_DataSave(char *path, SaveType type)
{
    guint i;
    gchar *dev = NULL;
    gchar mnt_dir[128];
    gchar datetime[256];
    gchar filename[256];

    MEDIA_INFO_T *media_info;
    gint media_cnt;

    media_cnt = 0;
    media_info = scm_new_media_list(&media_cnt);
    for(i = 0; i<media_cnt; i++)
    {
        if(scm_get_media_type(media_info[i].id) == MTYPE_USB)
            break;
    }

    if( i == media_cnt ) return ;

    memset(filename, 0x00, sizeof(filename));
    memset(datetime, 0x00, sizeof(datetime));

    dtf_get_local_datetime_ex(time(0), datetime);

    memset(mnt_dir, 0, sizeof(mnt_dir));
    scm_get_mounted_path(media_info[i].id, mnt_dir, 128);

    if(type == DB_EXPORT){
        g_sprintf(filename,"%s_%s","DB_data", datetime);
        g_sprintf(path,"%s/%s.ndb", mnt_dir, filename);
    }
    else if( type == LOG_EXPORT){
        g_sprintf(filename,"%s_%s","LOG_data",datetime);   
        g_sprintf(path,"%s/%s.log",mnt_dir, filename);
    }
}

static gboolean _cp_logdata_to_path(char *path)
{
    FILE *fsrc, *fdes;
    int a;
    if((fsrc = fopen("/tmp/webra-info/nand.log","rb"))==NULL)         return -1;
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


static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFOUTEVT_BUTTON_PRESS || evt->type == NFOUTEVT_BUTTON_RELEASE || \
        evt->type == NFOUTEVT_MOTION_NOTIFY)
    {
        if(evt->type == NFOUTEVT_BUTTON_PRESS) evt->type = GDK_BUTTON_PRESS;
        else if (evt->type == NFOUTEVT_BUTTON_RELEASE) evt->type = GDK_BUTTON_RELEASE;
        else if (evt->type == NFOUTEVT_MOTION_NOTIFY) evt->type = GDK_MOTION_NOTIFY;
        
        nfui_send_event(g_parent, evt, TRUE);
    }
    else if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}


static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf   *pbuf = NULL;
    gint size_w, size_h;

    if(evt->type == GDK_EXPOSE)
    {
        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);
        nfui_nfobject_gc_unref(gc);
    }
    else if(evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }

    return FALSE;
}

static gboolean post_timelabel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        if (!g_time_update) return FALSE;
        
    	g_source_remove(g_time_update);
    	g_time_update = 0;
    }

    return FALSE;
}


static gboolean post_piechartNand_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DEBUG_INFOR_DATA_SYNC)
    {
        gint rate = 0;
        
        rate = ((gdouble)nand_info.used/(gdouble)nand_info.total)*100;

        if(nfui_nfpiechart_get_ratio_value(obj, 0) != rate)
        {           
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, 100);
            nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0, rate);

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_piechartMemory_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DEBUG_INFOR_DATA_SYNC)
    {
        gint rate = 0;

        rate = ((gdouble)mem_info.used/(gdouble)mem_info.total)*100;

        if(nfui_nfpiechart_get_ratio_value(obj, 0) != rate)
        {            
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, 100);
            nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0, rate);

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}

static gboolean post_piechartCpu_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DEBUG_INFOR_DATA_SYNC)
    {
        gint value = 0;

        value = 100 - cpu_info;

        if(nfui_nfpiechart_get_ratio_value(obj, 0) != value)
        {            
            nfui_nfpiechart_set_max_value((NFPIECHART *)obj, 100);
            nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0, value);

            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }

    return FALSE;
}


static gboolean post_nand_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DEBUG_INFOR_DATA_SYNC)
    {   
        gchar strbuf[32];
        gint rate;

        memset(strbuf, 0x00, sizeof(strbuf));

        rate = ((gdouble)nand_info.used/(gdouble)nand_info.total)*100;
        
        g_sprintf(strbuf,"%.1f/%.1fGB(%d%%)",((double)nand_info.used/1048576), (double)nand_info.total/1048576, rate);

        if(strcmp(nfui_nflabel_get_text(obj), strbuf))
        {
            nfui_nflabel_set_text((NFLABEL *)obj,strbuf);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    
    return FALSE;
}

static gboolean post_memory_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DEBUG_INFOR_DATA_SYNC)
    {
        static gchar strtmp[32];
        gint rate;
        gchar strbuf[32];

        memset(strbuf, 0x00, sizeof(strbuf));

        rate = ((gdouble)mem_info.used/(gdouble)mem_info.total)*100;

        g_sprintf(strbuf,"%.1f/%.1fMB(%d%%)",(double)mem_info.used/1024,(double)mem_info.total/1024,rate);

        if(strcmp(nfui_nflabel_get_text(obj), strbuf))
        {
            nfui_nflabel_set_text((NFLABEL *)obj, strbuf);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    
    return FALSE;
}

static gboolean post_cpu_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_DEBUG_INFOR_DATA_SYNC)
    { 
        static gchar strtmp[32];
        gint rate;
        gchar strbuf[32];

        memset(strbuf, 0x00, sizeof(strbuf));

        rate = 100- cpu_info;

        g_sprintf(strbuf,lookup_string("USAGE : %d%%"), rate);

        if(strcmp(nfui_nflabel_get_text(obj), strbuf))
        {
            nfui_nflabel_set_text((NFLABEL *)obj, strbuf);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
    }
    
    return FALSE;
}

static gboolean post_logexposrt_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {        
        int ret;
        gchar file_path[256];

        nf_get_nand_log();                

        memset(file_path, 0, sizeof(file_path));

        _get_path_DataSave(file_path, LOG_EXPORT);

        ret = _cp_logdata_to_path(file_path);

        if(!ret)
        {   
            nftool_mbox(g_curwnd, "INFORMATION", "The log data has been saved successfully.", NFTOOL_MB_OK);
        }
        else
        {
            nftool_mbox(g_curwnd, "INFORMATION", "Fail to save the log data.", NFTOOL_MB_OK);
        }
    }
    
    return FALSE;
}


static gboolean post_dbexport_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        int ret;
        gchar file_path[256];

        memset(file_path, 0, sizeof(file_path));
        _get_path_DataSave(file_path, DB_EXPORT);

        ret = scm_export_db(file_path);
        if(ret == 0)
        {                                          
            nftool_mbox(g_curwnd, "INFORMATION", "The system data has been saved successfully.", NFTOOL_MB_OK);
        }
        else
        {
            nftool_mbox(g_curwnd, "INFORMATION", "It's failed to save the system data.", NFTOOL_MB_OK);
        }
    	
    }

    return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_curwnd = 0;

        if(t_timer)        g_source_remove(t_timer), t_timer =0;
        if(g_time_update)  g_source_remove(g_time_update), g_time_update = 0;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(topwin);
        nfui_nfobject_destroy(topwin);
    }
    
    return FALSE;
}

void System_Debugging_Information_Open(NFWINDOW *parent)
{
    NFOBJECT *win;
    NFOBJECT *content_fixed;
    NFOBJECT *fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y, size_w, size_h;

    DateTimeData dtdata;
    guint tformat;


    IPSetupData ipdata;
    DDNSData ddnsdata;	
	NF_NETIF_GET_INFO netif_info;

    gchar strBuf[256];
	gchar strTemp[128];
    //gchar ex_addr[40];

    guint port_num;
    gint use_ssl;
    
    g_parent = parent;
    
    if(g_curwnd) return ;

	
    win = (NFOBJECT *)nfui_nfwindow_new(parent, (1920/2)-(WIN_WIDTH/2),(1080/2)-(WIN_HEIGHT/2), WIN_WIDTH, WIN_HEIGHT);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_RELEASE, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_MOTION_NOTIFY, TRUE);
    //nfui_nfwindow_set_modal((NFWINDOW*)win, FALSE);
    nfui_nfwindow_set_moving_area_size((NFWINDOW*)win, WIN_HEIGHT);
    nfui_nfwindow_set_moving_limit((NFWINDOW*)win, TRUE);
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    
    g_curwnd = win;   

    if(g_curwnd == NULL) return;

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(content_fixed, WIN_WIDTH, WIN_HEIGHT);
    nfui_regi_post_event_callback(content_fixed, post_fixed_event_cb);   
    nfui_nfobject_show(content_fixed);
    

    fixed = ( NFOBJECT*)nfui_nffixed_new(); 
    nfui_nfobject_set_size(fixed, WIN_WIDTH, WIN_HEIGHT);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed, 0, 0);
    nfui_nfobject_show(fixed);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYSTEM DEBUGGING INFORMATION", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(193));
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(604));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);    
    nfui_nfobject_set_size(obj, WIN_WIDTH-8, 36);
    nfui_nfobject_use_focus(obj,NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,4, 4);

    ////////////// TIME 

    /*
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("2015-04-08 11:07:23", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL),COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,WIN_WIDTH, 36);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, TIME_X, TIME_Y);
    */

    memset(&dtdata, 0x00, sizeof(DateTimeData));
    
    DAL_get_dateTime_data(&dtdata);
    DAL_get_dateTime_format(NULL, &tformat);

    g_time = (NFOBJECT*) nfui_nftimelabel_new();
    nfui_nftimelabel_set_fg_color((NFTIMELABEL*)g_time, COLOR_IDX(292));
    nfui_nftimelabel_set_bg_color((NFTIMELABEL*)g_time, COLOR_IDX(200));
    nfui_nftimelabel_set_mode((NFTIMELABEL*)g_time, _prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
    nfui_nftimelabel_set_size((NFTIMELABEL*)g_time, 400, 36);   
    nfui_nfobject_use_focus(g_time, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(g_time);
    nfui_nffixed_put((NFFIXED*)fixed, g_time, TIME_X+130, TIME_Y);
    nfui_regi_post_event_callback(g_time, post_timelabel_event_handler);
       
    /////////////// IP ADDRESS 
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IP ADDRESS", nffont_get_pango_font(NFFONT_SMALL_SEMI),COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 30);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj,210, 36);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, IPADRESS_IN_X, IPADRESS_IN_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI),COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj,300, 36);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, IPADRESS_IN_X+210, IPADRESS_IN_Y);

    DAL_get_ipSetup_data(&ipdata);
	DAL_get_ddns_data(&ddnsdata);
	nf_netif_get_info(&netif_info);

    //port_num = nf_ipcam_get_http_port(0); //ch
    use_ssl = nf_ipcam_is_using_ssl(0);

	memset(strBuf, 0x00, sizeof(strBuf));  
	memset(strTemp, 0x00, sizeof(strTemp));

    strcpy(strBuf, ": ");

    strcat(strBuf, STR_SERVICE(use_ssl));
	g_sprintf(strTemp, "%d.%d.%d.%d", ((netif_info.ipaddr & 0xff000000)>>24), ((netif_info.ipaddr & 0xff0000)>>16),
                        			((netif_info.ipaddr & 0xff00)>>8), (netif_info.ipaddr & 0xff));
    strcat(strBuf, strTemp);
	if (ipdata.dhcp) strcat(strBuf, "(DHCP)");
    
    nfui_nflabel_set_text((NFLABEL*)obj, strBuf);

    /////////////// WEB SERVICE PORT

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("WEB SERVICE PORT", nffont_get_pango_font(NFFONT_SMALL_SEMI),COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 30);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj,210, 36);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, IPADRESS_EX_X, IPADRESS_EX_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_SMALL_SEMI),COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj,300, 36);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, IPADRESS_EX_X+210, IPADRESS_EX_Y);

    memset(strBuf, 0x00, sizeof(strBuf));
	//memset(ex_addr, 0x00, sizeof(ex_addr));
    //var_get_external_addr(ex_addr, 40);
        
    //if (strlen(ex_addr))
    //{
    //    g_sprintf(strBuf,"%s%s:%d", STR_SERVICE(use_ssl), ex_addr, ipdata.webPort);
        g_sprintf(strBuf,": %d", ipdata.webPort); 
        nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
    //}
	//else {
    //    g_sprintf(strBuf,"N/A");
    //    nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	//}
    
    /////////////////
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAND", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 60);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,140,40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, NAND_LABEL_X, NAND_LABEL_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MEMORY", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, MEMORY_LABEL_X, MOMORY_LABEL_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CPU", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,40);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, CPU_LABEL_X, CPU_LABEL_Y);


    obj = nfui_nfpiechart_new(PIE_CHART_WIDTH, 2, 200); // 100 max value
    nfui_nfpiechart_set_arcbg_color((NFPIECHART *)obj, 665); // 669
    nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0, 90);
    nfui_nfpiechart_set_chart_color((NFPIECHART *)obj, 0, 604);
    nfui_nfpiechart_set_pango_font((NFPIECHART *) obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, PIE_CHART1_X,PIE_CHART1_Y);
    nfui_regi_post_event_callback(obj, post_piechartNand_event_handler);
    
    obj = nfui_nfpiechart_new(PIE_CHART_WIDTH, 2, 200); // 100 max value
    nfui_nfpiechart_set_arcbg_color((NFPIECHART *)obj, 665); // 669
    nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0, 90);
    nfui_nfpiechart_set_chart_color((NFPIECHART *)obj, 0, 604);
    nfui_nfpiechart_set_pango_font((NFPIECHART *) obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, PIE_CHART2_X,PIE_CHART2_Y);
    nfui_regi_post_event_callback(obj, post_piechartMemory_event_handler);    


    obj = nfui_nfpiechart_new(PIE_CHART_WIDTH, 2, 90); // 100 max value
    nfui_nfpiechart_set_arcbg_color((NFPIECHART *)obj, 665); // 604
    nfui_nfpiechart_set_chart_value((NFPIECHART *)obj, 0, 10);
    nfui_nfpiechart_set_chart_color((NFPIECHART *)obj, 0, 604);
    nfui_nfpiechart_set_pango_font((NFPIECHART *) obj, nffont_get_pango_font(NFFONT_MINI_NORMAL_3), COLOR_IDX(262));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)fixed, obj, PIE_CHART3_X,PIE_CHART3_Y);
    nfui_regi_post_event_callback(obj, post_piechartCpu_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new("");
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, 604); // 604
    nfui_nfobject_set_size(obj, 13, 13);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, NAND_DATA1_COLOR_X, NAND_DATA1_COLOR_Y+(30-13)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NAND USAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,30);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, NAND_DATA1_X, NAND_DATA1_Y); 

    obj = (NFOBJECT*)nfui_nflabel_new("");
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, 604); // 669
    nfui_nfobject_set_size(obj, 13, 13);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, NAND_DATA2_COLOR_X, NAND_DATA2_COLOR_Y+(30-13)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("1.1/7.9GB(10%)", nffont_get_pango_font(NFFONT_MINI_NORMAL_4), COLOR_IDX(292));
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,30);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, NAND_DATA2_X, NAND_DATA2_Y); 
    nfui_regi_post_event_callback(obj, post_nand_event_handler);
    
    obj = (NFOBJECT*)nfui_nflabel_new("");
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, 604); // 669
    nfui_nfobject_set_size(obj, 13, 13);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, MEMORY_DATA1_COLOR_X, MEMORY_DATA1_COLOR_Y+(30-13)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MEMORY USAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,30);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, MEMORY_DATA1_X, MEMORY_DATA1_Y); 

    obj = (NFOBJECT*)nfui_nflabel_new("");
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, 604); // 669
    nfui_nfobject_set_size(obj, 13, 13);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, MEMORY_DATA2_COLOR_X, MEMORY_DATA2_COLOR_Y+(30-13)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("2.1/7.9MB(30%)", nffont_get_pango_font(NFFONT_MINI_NORMAL_4), COLOR_IDX(292));
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,30);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, MEMORY_DATA2_X, MEMORY_DATA2_Y); 
    nfui_regi_post_event_callback(obj, post_memory_event_handler);

    obj = (NFOBJECT*)nfui_nflabel_new("");
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, 604); // 669
    nfui_nfobject_set_size(obj, 13, 13);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, CPU_DATA1_COLOR_X, CPU_DATA1_COLOR_Y+(30-13)/2);   

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CPU USAGE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,30);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, CPU_DATA1_X, CPU_DATA1_Y); 

    obj = (NFOBJECT*)nfui_nflabel_new("");
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, 604); // 669
    nfui_nfobject_set_size(obj, 13, 13);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, CPU_DATA2_COLOR_X, CPU_DATA2_COLOR_Y+(30-13)/2);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("85% (1.3GHz)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nfobject_support_multi_lang(obj, FALSE);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj,200,30);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, CPU_DATA2_X, CPU_DATA2_Y); 
    nfui_regi_post_event_callback(obj, post_cpu_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("LOG EXPORT",140);
    nfui_nfobject_set_size(obj, 140, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 10, WIN_HEIGHT-50);
    //nfui_nffixed_put((NFFIXED*)fixed, obj, WIN_WIDTH-10-140-10-140-10-140, WIN_HEIGHT-50);
    nfui_regi_post_event_callback(obj, post_logexposrt_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("DB EXPORT",140);
    nfui_nfobject_set_size(obj, 140, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, 10+140+10, WIN_HEIGHT-50);
    //nfui_nffixed_put((NFFIXED*)fixed, obj, WIN_WIDTH-10-140-10-140, WIN_HEIGHT-50);
    nfui_regi_post_event_callback(obj, post_dbexport_event_handler);
    
    obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("CLOSE",140);
	nfui_nfobject_set_size(obj, 140, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, WIN_WIDTH-10-140, WIN_HEIGHT-50);
    nfui_regi_post_event_callback(obj, post_okbutton_event_handler);
    
    t_timer = g_timeout_add(500, _get_debug_info, NULL);
    g_time_update = g_timeout_add(500, _update_time, NULL);

    nfui_nfwindow_add((NFWINDOW*)win, fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);
    
}
