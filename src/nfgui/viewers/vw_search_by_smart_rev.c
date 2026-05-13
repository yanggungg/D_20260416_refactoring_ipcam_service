/*
 * vw_search_by_smart_rev.c
 *
 * Written by eunhye. <eun@itxsecurity.com>
 * Copyright (c) ITX security, June 22, 2014
 *
 */

#include <glib.h>
#include "iux_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/nf_ui_color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "modules/ssm.h"
#include "modules/smt.h"
#include "modules/var.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfbutton.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfvklabel.h"
#include "objects/nftab.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"
#include "objects/nfcombobox.h"
#include "objects/nftimelabel.h"

#include "vw_search_by_smart_rev.h"
#include "vw_vca_rev_component.h"
#include "vw_search_by_smart_rev_internal.h"
#include "vw_set_date_time.h"
#include "vw_smart_search_statistic.h"

#include "vsm.h"
#include "vaa.h"
#include "vaa_itx.h"


#define	_FIXED1_X		0
#define	_FIXED1_Y		0
#define	_FIXED1_W		1036
#define	_FIXED1_H		943

#define	_FIXED2_X		1896-_FIXED2_W-20
#define	_FIXED2_Y		_FIXED1_Y
#define	_FIXED2_W		810
#define	_FIXED2_H		_FIXED1_H

#define VIDEO_WIDTH           (960)
#define VIDEO_HEIGHT          (540)


enum {
	SEARCH_RULE_TAB = 0,
	SEARCH_RESULT_TAB,

	SEARCH_SUBTAB_CNT
};

enum {
	DF_YMD = 0,
	DF_MDY,
	DF_DMY,
	NUM_DATE_FORMATS,
};


////////////////////////////////////////////////////////////
//
// private data types
//




////////////////////////////////////////////////////////////
//
// private variable
//
static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_fixed = 0;
static NFOBJECT *g_ch_combo = 0;
static NFOBJECT *from_obj;
static NFOBJECT *to_obj;
static NFOBJECT *from_btn_obj;
static NFOBJECT *to_btn_obj;
static NFOBJECT *g_search_btn_obj;
static NFOBJECT *g_stop_btn_obj;
static NFOBJECT *g_import_btn_obj;
static NFOBJECT *g_statistic_btn_obj;
static NFOBJECT *g_play_btn_obj;
static NFOBJECT *g_close_btn_obj;

static NFOBJECT *g_smart_fixed1;
static NFOBJECT *g_smart_fixed2;
static NFOBJECT *g_tab;
static NFOBJECT *g_tab_page[SEARCH_SUBTAB_CNT];

static NFOBJECT *g_rule_check;
static NFOBJECT *g_rule_label;
static NFOBJECT *g_vstatus_label;
static NFOBJECT *g_play_btn[5];

static GTimeVal g_from_tv;
static GTimeVal g_to_tv;

static guint g_rule_draw_tid = 0;
static guint g_van_status_tid = 0;
static guint g_preview_status_tid = 0;


////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _init_component_data(NFOBJECT *win, gint ch)
{
    VCA_COMPONENT_DATA_T *component_data;

    component_data = imalloc(sizeof(VCA_COMPONENT_DATA_T));

    component_data->act_capable = 1;
    component_data->act_license = 1;

    component_data->preview.type = 1;
    component_data->preview.play_mode = 0;
    component_data->preview.ch = ch;
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_FF0000);    

    component_data->unit_setup = 0;
    
    component_data->skip_calibration = 0;    
    component_data->calibration.icon_height = 170;
    component_data->calibration.pause_video = 0;
    component_data->calibration.camera_height = 12.00;
    component_data->calibration.camera_tilt = 35;
    
    component_data->rule_type = 0;
   
    strcpy(component_data->line.name, "");
    component_data->line.active = 0;
    component_data->line.forward = 0;
    component_data->line.reverse = 0;
    component_data->line.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F);
    component_data->line.use_filter_color = 0;
    component_data->line.filter_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->line.use_filter_size = 0;
    component_data->line.filter_color_percnt = 1;
    component_data->line.filter_min_size_w = 0;
    component_data->line.filter_max_size_w = 0;
    component_data->line.filter_min_size_h = 0;
    component_data->line.filter_max_size_h = 0;
    component_data->line.use_filter_speed = 0;
    component_data->line.filter_min_speed = 0;
    component_data->line.filter_max_speed = 0;

    strcpy(component_data->area.name, "");
    component_data->area.active = 0;
    component_data->area.enter = 0;
    component_data->area.exit = 0;
    component_data->area.removed = 0;
    component_data->area.loitering = 0;
    component_data->area.stopped = 0;    
    component_data->area.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF003F);
    component_data->area.stopped_val = 5;
    component_data->area.removed_val = 5;
    component_data->area.loitering_val = 5;
    component_data->area.use_filter_color = 0;
    component_data->area.filter_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->area.use_filter_size = 0;
    component_data->area.filter_color_percnt = 1;
    component_data->area.filter_min_size_w = 0;
    component_data->area.filter_max_size_w = 0;
    component_data->area.filter_min_size_h = 0;
    component_data->area.filter_max_size_h = 0;
    component_data->area.use_filter_speed = 0;
    component_data->area.filter_min_speed = 0;
    component_data->area.filter_max_speed = 0;
    
    component_data->use_counter = 0;
    strcpy(component_data->counter.name, "");
    component_data->counter.active = 0;
    component_data->counter.display_color_idx = COLOR_PRG_IDX(UX_COLOR_FF0000);
    component_data->counter.use_counter_event = 0;
    component_data->counter.counter_event_val = 100;
    component_data->counter.use_reset_value = 0;
    component_data->counter.up_source = -1;
    component_data->counter.down_source = -1;
 
    component_data->option.shadow_removal = 0;
    component_data->option.track_reference = 1;
    component_data->option.minimum_object = 0;
    component_data->option.minimum_w = 1000;
    component_data->option.minimum_h = 2000;
    component_data->option.disp_obj_box = 0;
    component_data->option.disp_obj_traj = 0;
    component_data->option.disp_obj_id = 0;
    component_data->option.disp_obj_w = 0;
    component_data->option.disp_obj_h = 0;
    component_data->option.disp_obj_speed = 0;
    component_data->option.disp_rules = 0;       
    
    nfui_nfobject_set_data(win, VCA_COMPONENT_DATA, component_data);
    return 0;
}


static gint _init_component_action(NFOBJECT *win, gint ch)
{
    VCA_COMPONENT_ACTION_T *component_action;

    component_action = imalloc(sizeof(VCA_COMPONENT_ACTION_T));

    
    nfui_nfobject_set_data(win, VCA_COMPONENT_ACTION, component_action);
    return 0;
}

static gint _set_component_video_fixed(NFOBJECT *top, NFOBJECT *video_fixed)
{
    VCA_COMPONENT_DATA_T *component_data;

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    component_data->video_fixed = video_fixed;

    return 0;
}

static gboolean _draw_vca_rule(gpointer data)
{
    vw_vca_rev_video_component_sync_data(g_video_fixed);
	return TRUE;
}

static gboolean _init_vca_rule(gpointer data)
{
	if (!g_rule_draw_tid) g_rule_draw_tid = g_timeout_add(100, _draw_vca_rule, 0);
	return FALSE;
}


static nftl_df_type prvTransDateFormat(gint db_index)
{
	nftl_df_type ret;

	if(db_index == DF_YMD)			ret = NFTL_DF_YMD;
	else if(db_index == DF_MDY)		ret = NFTL_DF_MDY;
	else if(db_index == DF_DMY)		ret = NFTL_DF_DMY;
	else	ret = NFTL_DF_HIDE;

	return ret;
}

static void _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);
}

static gboolean _open_rule_page(NFOBJECT *page)
{
	NFOBJECT *fixed;
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	//nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);

    _search_by_smart_rev_rule_page(fixed);    
	return TRUE;
}

static gboolean _open_search_result_page(NFOBJECT *page)
{
	NFOBJECT *fixed;
	
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	//nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);

    _search_by_smart_rev_result_page(fixed);

	return TRUE;
}

static gint _proc_set_object_status_ready()
{
    GdkRectangle area;
    gint off_x, off_y;
    gint i;

    _search_by_smart_rev_result_clear();

    nfui_check_button_set_active(g_rule_check, TRUE);
	nfui_nfobject_disable(g_rule_check);
    nfui_signal_emit(g_rule_check, GDK_EXPOSE, TRUE);	

    for (i = 0; i < 5; i++)
    {
    	nfui_nfobject_disable(g_play_btn[i]);
        nfui_signal_emit(g_play_btn[i], GDK_EXPOSE, TRUE);	
    }

    nfui_nfobject_disable(g_stop_btn_obj);
    nfui_signal_emit(g_stop_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_ch_combo);
    nfui_signal_emit(g_ch_combo, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(from_btn_obj);
    nfui_signal_emit(from_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(to_btn_obj);
    nfui_signal_emit(to_btn_obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_disable(g_play_btn_obj);
    nfui_signal_emit(g_play_btn_obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_enable(g_close_btn_obj);
    nfui_signal_emit(g_close_btn_obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_disable(g_statistic_btn_obj);
    nfui_signal_emit(g_statistic_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_import_btn_obj);
    nfui_signal_emit(g_import_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_get_offset(g_smart_fixed2, &off_x, &off_y);   
    area.x = off_x;
    area.y = off_y;
    area.width = g_smart_fixed2->width;
    area.height = g_smart_fixed2->height;

    gdk_window_begin_paint_rect(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, &area);
    nfui_nftab_regi_page((NFTAB *)g_tab, g_tab_page[SEARCH_RULE_TAB], SEARCH_RULE_TAB);   
    nfui_nftab_regi_page((NFTAB *)g_tab, g_tab_page[SEARCH_RESULT_TAB], SEARCH_RESULT_TAB);   
    nfui_nftab_change_page((NFTAB*)g_tab, SEARCH_RULE_TAB);
    nfui_nftab_unregi_page((NFTAB *)g_tab, SEARCH_RESULT_TAB);    
    nfui_signal_emit(g_smart_fixed2, GDK_EXPOSE, TRUE);        
    gdk_window_end_paint(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window);
    
	VW_Search_config_smart(FALSE);
    nfui_make_key_hierarchy(g_curwnd);    

    return 0;
}

static gint _proc_set_object_status_start()
{
    GdkRectangle area;
    gint off_x, off_y;

    NFOBJECT *obj;

    nfui_nfobject_enable(g_stop_btn_obj);
    nfui_signal_emit(g_stop_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_disable(g_ch_combo);
    nfui_signal_emit(g_ch_combo, GDK_EXPOSE, TRUE);

    nfui_nfobject_disable(from_btn_obj);
    nfui_signal_emit(from_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_disable(to_btn_obj);
    nfui_signal_emit(to_btn_obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_disable(g_play_btn_obj);
    nfui_signal_emit(g_play_btn_obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_disable(g_close_btn_obj);
    nfui_signal_emit(g_close_btn_obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_disable(g_statistic_btn_obj);
    nfui_signal_emit(g_statistic_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_disable(g_import_btn_obj);
    nfui_signal_emit(g_import_btn_obj, GDK_EXPOSE, TRUE);

    obj = _get_searchbysmart_filter();
    nfui_nfobject_disable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

    obj = _get_searchbysmart_loglist();
    nfui_nfobject_disable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

    obj = _get_searchbysmart_order();
    nfui_nfobject_disable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_get_offset(g_smart_fixed2, &off_x, &off_y);   
    area.x = off_x;
    area.y = off_y;
    area.width = g_smart_fixed2->width;
    area.height = g_smart_fixed2->height;

    gdk_window_begin_paint_rect(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, &area);
    nfui_nftab_regi_page((NFTAB *)g_tab, g_tab_page[SEARCH_RESULT_TAB], SEARCH_RESULT_TAB);   
    nfui_nftab_change_page((NFTAB*)g_tab, SEARCH_RESULT_TAB);
    nfui_nftab_unregi_page((NFTAB *)g_tab, SEARCH_RULE_TAB);    
    nfui_signal_emit(g_smart_fixed2, GDK_EXPOSE, TRUE);        
    gdk_window_end_paint(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window);
    
	VW_Search_config_smart(TRUE);
    nfui_make_key_hierarchy(g_curwnd);    

    return 0;
}

static gint _proc_set_object_status_compelte()
{
    GdkRectangle area;
    gint off_x, off_y;

    NFOBJECT *obj;

    nfui_nfobject_disable(g_stop_btn_obj);
    nfui_signal_emit(g_stop_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_ch_combo);
    nfui_signal_emit(g_ch_combo, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(from_btn_obj);
    nfui_signal_emit(from_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(to_btn_obj);
    nfui_signal_emit(to_btn_obj, GDK_EXPOSE, TRUE); 

    nfui_nfobject_disable(g_play_btn_obj);
    nfui_signal_emit(g_play_btn_obj, GDK_EXPOSE, TRUE); 

    nfui_nfobject_enable(g_close_btn_obj);
    nfui_signal_emit(g_close_btn_obj, GDK_EXPOSE, TRUE); 

    nfui_nfobject_disable(g_import_btn_obj);
    nfui_signal_emit(g_import_btn_obj, GDK_EXPOSE, TRUE);

    nfui_nfobject_enable(g_statistic_btn_obj);
    nfui_signal_emit(g_statistic_btn_obj, GDK_EXPOSE, TRUE);  

    obj = _get_searchbysmart_filter();
    nfui_nfobject_enable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

    obj = _get_searchbysmart_loglist();
    nfui_nfobject_enable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

    obj = _get_searchbysmart_order();
    nfui_nfobject_enable(obj);
    nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

    nfui_nfobject_get_offset(g_smart_fixed2, &off_x, &off_y);   
    area.x = off_x;
    area.y = off_y;
    area.width = g_smart_fixed2->width;
    area.height = g_smart_fixed2->height;

    gdk_window_begin_paint_rect(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window, &area);
    nfui_nftab_regi_page((NFTAB *)g_tab, g_tab_page[SEARCH_RULE_TAB], SEARCH_RULE_TAB);   
    nfui_nftab_regi_page((NFTAB *)g_tab, g_tab_page[SEARCH_RESULT_TAB], SEARCH_RESULT_TAB);   
    nfui_nftab_change_page((NFTAB*)g_tab, SEARCH_RESULT_TAB);    
    nfui_signal_emit(g_smart_fixed2, GDK_EXPOSE, TRUE);        
    gdk_window_end_paint(GTK_WIDGET(((NFWINDOW*)g_curwnd)->main_widget)->window);

	VW_Search_config_smart(FALSE);
    nfui_make_key_hierarchy(g_curwnd);

    return 0;
}

#define CONTROL_TIME_FOR_SMART_SEARCH 30

static gint _proc_start_smart_search(VCA_COMPONENT_DATA_T *component_data)
{
    VAAID vaaid;
    VCAData vcadata;
    VCARule vcarule;

	GTimeVal from_tv;
	GTimeVal to_tv;
	gint ch;
	
    memset(&vcadata, 0x00, sizeof(VCAData));
    memset(&vcarule, 0x00, sizeof(VCARule));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
    vaa_itx_export_db(vaaid, &vcadata);
    memcpy(&vcarule.zonelist, &vcadata.zonelist, sizeof(VCAZoneData));
    memcpy(&vcarule.cntrlist, &vcadata.cntrlist, sizeof(VCACntrData));

	nfui_nftimelabel_get_datetime(from_obj, &from_tv);		
	nfui_nftimelabel_get_datetime(to_obj, &to_tv);

	scm_analyze_video_range_dal(component_data->preview.ch, from_tv.tv_sec, to_tv.tv_sec, &vcarule);

    component_data->disable_event = 1;
    component_data->preview.onoff = 1;
    component_data->preview.play_mode = 1;
    component_data->preview.play_from = from_tv.tv_sec - CONTROL_TIME_FOR_SMART_SEARCH; //captainnn for smart search
    component_data->preview.play_to = to_tv.tv_sec;		
	vw_vca_rev_video_component_sync_preview(g_video_fixed);

    return 0;
}

static gint _proc_pause_smart_search(VCA_COMPONENT_DATA_T *component_data)
{
    vsm_playback_smart_mainview_pause(NF_PLAY_SMART_SEARCH_META);
    return 0;
}

static gint _proc_resume_smart_search(VCA_COMPONENT_DATA_T *component_data)
{
    vsm_playback_smart_mainview_resume(NF_PLAY_SMART_SEARCH_META);
    return 0;
}

static gint _proc_stop_smart_search(VCA_COMPONENT_DATA_T *component_data)
{
    component_data->preview.onoff = 0;
    vw_vca_rev_video_component_sync_preview(g_video_fixed);

    scm_cancel_analyze();

    nfui_nfbutton_set_text((NFBUTTON*)g_search_btn_obj, "SEARCH");
    nfui_signal_emit(g_search_btn_obj, GDK_EXPOSE, TRUE);

    _proc_set_object_status_compelte();
    return 0;
}

static gint _update_enable_button_obj()
{
    gint update = 0;

    if (nfui_nfobject_is_disabled(g_rule_check) == TRUE)
    {
        nfui_nfobject_enable(g_rule_check);
        nfui_nfobject_enable(g_play_btn[0]);
        nfui_nfobject_enable(g_play_btn[1]);
        nfui_nfobject_enable(g_play_btn[2]);
        nfui_nfobject_enable(g_play_btn[3]);
        nfui_nfobject_enable(g_play_btn[4]);        
        update = 1;
    }  

    return update;
}

static gint _update_disable_button_obj()
{
    gint update = 0;

    if (nfui_nfobject_is_disabled(g_rule_check) == FALSE)
    {
        nfui_nfobject_disable(g_rule_check);
        nfui_nfobject_disable(g_play_btn[0]);
        nfui_nfobject_disable(g_play_btn[1]);
        nfui_nfobject_disable(g_play_btn[2]);
        nfui_nfobject_disable(g_play_btn[3]);
        nfui_nfobject_disable(g_play_btn[4]);        
        update = 1;
    }  

    return update;
}

static gint _update_text_vstatus_label(gchar *strBuf)
{
    gint update = 0;

    if (strcmp(nfui_nflabel_get_text((NFLABEL*)g_vstatus_label), strBuf) != 0)
    {
        nfui_nflabel_set_text((NFLABEL*)g_vstatus_label, strBuf);
        update = 1;
    }

    return update;    
}

static gint _get_meta_prcnt_string(gchar *str)
{
    GTimeVal curr_tv;
	GTimeVal from_tv;
	GTimeVal to_tv;
	
	gfloat per;
	gchar tmp[64];

    curr_tv = vsm_playback_get_smarttime();
    
    if (curr_tv.tv_sec != 0)
    {
    	nfui_nftimelabel_get_datetime(from_obj, &from_tv);
    	nfui_nftimelabel_get_datetime(to_obj, &to_tv);
        
	if(curr_tv.tv_sec < from_tv.tv_sec)
		return 0;
        
        per = (gfloat)(curr_tv.tv_sec-from_tv.tv_sec) * 100 / (gfloat)(to_tv.tv_sec-from_tv.tv_sec);

        dtf_get_local_datetime(curr_tv.tv_sec, tmp);
        g_sprintf(str, "%s (%.2f%%)", tmp, per);
    }

    return 0;
}

static gint _get_preview_time_string(gchar *str)
{
    GTimeVal tv;

    tv = vsm_playback_get_previewtime();
    
    if (tv.tv_sec != 0)
    {
        dtf_get_local_datetime(tv.tv_sec, str);
    }

    return 0;
}

static gboolean _tmr_check_search_status(gpointer data)
{
    NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;

    DIR_RATE_E dir_rate;

	if (vsm_playback_get_smart_dir_rate() == DR_STOP)
	{   
        top = nfui_nfobject_get_top(g_search_btn_obj);	
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        _proc_stop_smart_search(component_data);
        g_van_status_tid = 0;
        return FALSE;
	}

	return TRUE;
}

static gboolean _tmr_check_preview_status(gpointer data)
{
    NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    
    GTimeVal tv;
    gchar strBuf[128];

    gint meta_start = 0;
    gint preview_start = 0;
    gint text_expose = 0;   
    gint enable_expose = 0;   

    top = nfui_nfobject_get_top(g_vstatus_label);	
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    if (component_data->preview.onoff == 1)
    {
        if (component_data->preview.play_mode == 1)
        {
            enable_expose =  _update_disable_button_obj();

            memset(strBuf, 0x00, sizeof(strBuf));
            _get_meta_prcnt_string(strBuf);
            text_expose = _update_text_vstatus_label(strBuf);
        }
        else if (component_data->preview.play_mode == 2)
        {
            enable_expose =  _update_enable_button_obj();

            memset(strBuf, 0x00, sizeof(strBuf));
            _get_preview_time_string(strBuf);
            text_expose = _update_text_vstatus_label(strBuf);
        }
    }
    else
    {
        enable_expose =  _update_disable_button_obj();
        text_expose = _update_text_vstatus_label(" ");
    }

    if (text_expose) 
        nfui_signal_emit(g_vstatus_label, GDK_EXPOSE, TRUE);

    if (enable_expose) 
    {
        nfui_signal_emit(g_rule_check, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_play_btn[0], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_play_btn[1], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_play_btn[2], GDK_EXPOSE, TRUE);    
        nfui_signal_emit(g_play_btn[3], GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_play_btn[4], GDK_EXPOSE, TRUE);
    }

	return TRUE;
}

static gint _open_rule_import_popup(NFOBJECT *video_fixed, VCA_COMPONENT_DATA_T *component_data)
{
    component_data->disp_rule.block_update = 1; 
    vw_vca_rev_video_component_sync_data(video_fixed);
    component_data->disable_event = 1;
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
    vw_vca_rev_video_component_sync_preview(video_fixed);
    vw_vca_rev_video_component_expose(video_fixed);      
    
    vw_search_by_smart_import_rule_popup_open(g_curwnd, component_data->preview.ch, component_data->preview.play_from);

    component_data->disable_event = 0;
    component_data->preview.onoff = 1;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
    component_data->disp_rule.block_update = 0;
    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 5;
    component_data->disp_rule.delay_cnt = 0;      
    vw_vca_rev_video_component_sync_preview(video_fixed);    
    vw_vca_rev_video_component_expose(video_fixed);
    
    return 0;
}

////////////////////////////////////////////////////////////
//
// handler
//

static gboolean pre_subtab_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint cur_page;
	gint new_page;
	mb_type ret;

    NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;    

    GTimeVal from_tv;
    
	if(evt->type == NFEVENT_TAB_BEFORE_CHANGE)
	{
		cur_page = nfui_nftab_get_cur_page((NFTAB*)obj);
		new_page = nfui_nftab_get_new_page((NFTAB*)obj);

		if(cur_page == new_page)	return FALSE;

		switch(new_page) 
		{
			case SEARCH_RULE_TAB:
			{    
                nfui_nftimelabel_get_datetime(from_obj, &from_tv);

                top = nfui_nfobject_get_top(obj);
                component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

                component_data->disable_event = 0;
                component_data->preview.onoff = 1;
                component_data->preview.play_mode = 0;  
                component_data->preview.play_from = from_tv.tv_sec;
                component_data->preview.play_to = from_tv.tv_sec + 1000000;    
                component_data->disp_rule.block_update = 0; 
                vw_vca_rev_video_component_sync_preview(g_video_fixed);
                vw_vca_rev_video_component_expose(g_video_fixed);        

                nfui_user_signal_emit(g_smart_fixed1, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
                nfui_user_signal_emit(g_smart_fixed2, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);

                _proc_set_object_status_ready();
            }
			break;
			case SEARCH_RESULT_TAB:
			{      

            }
			break;

			default:
			break;
		}
	}

	return FALSE;
}

static gboolean pre_page_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;
    GdkDrawable *drawable = NULL;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;		
    gint off_x, off_y;
		
	if(evt->type == GDK_EXPOSE) {
		drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
		gc = gdk_gc_new(drawable);

		nfui_nfobject_get_offset(obj, &off_x, &off_y);
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_TAB_SUB_GROUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, obj->x+off_x, obj->y+off_y-60, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_TAB_SUB_GROUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_video_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_EXPOSE) 
	{	

	}
	else if (evt->type == GDK_DELETE)  
	{
	    vw_dit_display_set_vca_ditid(0);

    	if (g_rule_draw_tid) 
    	{
    		g_source_remove(g_rule_draw_tid);
    		g_rule_draw_tid = 0;
    	}	    

    	if (g_van_status_tid) 
    	{
    		g_source_remove(g_van_status_tid);
    		g_van_status_tid = 0;
    	}	    

    	if (g_preview_status_tid) 
    	{
    		g_source_remove(g_preview_status_tid);
    		g_preview_status_tid = 0;
    	}	    
	}

	return FALSE;
}

static gboolean post_from_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	VCA_COMPONENT_DATA_T *component_data;

	GTimeVal from_tv;
	GTimeVal to_tv;
	GTimeVal temp_tv;

	gint x, y;
	gint i;

    GdkRectangle area;
    gint off_x, off_y;
	
	memset(&temp_tv, 0x00, sizeof(GTimeVal));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

	if (evt->type == GDK_BUTTON_RELEASE) 
	{	
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);
	
		x += top->x + obj->width + 4;
		y += top->y;
		
		nfui_nftimelabel_get_datetime(from_obj, &from_tv);		
		nfui_nftimelabel_get_datetime(to_obj, &to_tv);
		to_tv.tv_sec -= 1;
		temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "FROM", x, y, from_tv.tv_sec, SDT_TYPE_SEC, NF_LOWER_TIMELIMIT, to_tv.tv_sec);
		
		if (temp_tv.tv_sec != 0)
		{	
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &temp_tv);
			nfui_signal_emit((NFOBJECT*)from_obj, GDK_EXPOSE, TRUE);
			g_from_tv.tv_sec = temp_tv.tv_sec;

            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            component_data->disable_event = 0;
            component_data->preview.onoff = 1;    
            component_data->preview.play_mode = 0;
            component_data->preview.play_from = temp_tv.tv_sec;
            component_data->preview.play_to = temp_tv.tv_sec + 1000000;       
            vw_vca_rev_video_component_sync_preview(g_video_fixed);	
            vw_vca_rev_video_component_expose(g_video_fixed);        

            _proc_set_object_status_ready();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_UP) 
		{
			_change_obj_focus(obj, g_search_btn_obj);
			return TRUE;
		}		
	}
	
	return FALSE;
}

static gboolean post_to_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	guint i;
	int x, y;
	NFOBJECT *top;
	VCA_COMPONENT_DATA_T *component_data;

	GTimeVal from_tv;
	GTimeVal to_tv;
	GTimeVal temp_tv;

    GdkRectangle area;
    gint off_x, off_y;

	memset(&temp_tv, 0x00, sizeof(GTimeVal));
	memset(&from_tv, 0x00, sizeof(GTimeVal));
	memset(&to_tv, 0x00, sizeof(GTimeVal));

	if (evt->type == GDK_BUTTON_RELEASE) 
	{	
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
	
		nfui_nfobject_get_offset(obj, &x, &y);
		top = nfui_nfobject_get_top(obj);
	
		x += top->x + obj->width + 4;
		y += top->y;
		
		nfui_nftimelabel_get_datetime(from_obj, &from_tv);		
		nfui_nftimelabel_get_datetime(to_obj, &to_tv);
		from_tv.tv_sec += 1;
		temp_tv.tv_sec = VW_Set_DateTime_Open(g_curwnd, "TO", x, y, to_tv.tv_sec, SDT_TYPE_SEC, from_tv.tv_sec, NF_UPPER_TIMELIMIT);		

		if (temp_tv.tv_sec != 0)
		{	
			nfui_nftimelabel_set_datetime((NFTIMELABEL*)to_obj, &temp_tv);
			nfui_signal_emit((NFOBJECT*)to_obj, GDK_EXPOSE, TRUE);
			g_to_tv.tv_sec = temp_tv.tv_sec;

            component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

            component_data->disable_event = 0;
            component_data->preview.onoff = 1;    
            component_data->preview.play_mode = 0;
            component_data->preview.play_from = from_tv.tv_sec;
            component_data->preview.play_to = from_tv.tv_sec + 1000000;       
            vw_vca_rev_video_component_sync_preview(g_video_fixed);	
            vw_vca_rev_video_component_expose(g_video_fixed);        

            _proc_set_object_status_ready();
		}
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if(kpid == KEYPAD_DOWN) 
		{
			_change_obj_focus(obj, g_ch_combo);
			return TRUE;
		}		
	}
	
	return FALSE;
}

static gboolean post_dis_rule_check_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED) 
    {      
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;    
        gint status;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        status = nfui_check_button_get_active(obj);       
        component_data->disp_rule.block_update = !status; 
        vw_vca_rev_video_component_expose(g_video_fixed);        
        vw_vca_rev_video_component_sync_data(g_video_fixed);
    }
    
    return FALSE;
}

static gboolean post_channel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {      
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;    
        gint ch;

    	GTimeVal from_tv;
    	GTimeVal to_tv;

		VAAID vaaid;

        GdkRectangle area;
        gint off_x, off_y;
    
        ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

        vaaid = vaa_get_pb_vaaid(ch);
    	vaa_itx_raiseup(vaaid);
 
		nfui_nftimelabel_get_datetime(from_obj, &from_tv);		
		nfui_nftimelabel_get_datetime(to_obj, &to_tv);

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        component_data->disable_event = 0;
        component_data->preview.ch = ch;    
        component_data->preview.onoff = 1;
        component_data->preview.play_mode = 0;  
        component_data->preview.play_from = from_tv.tv_sec;
        component_data->preview.play_to = from_tv.tv_sec + 1000000;    
        vw_vca_rev_video_component_sync_preview(g_video_fixed);
        vw_vca_rev_video_component_expose(g_video_fixed);        

        nfui_user_signal_emit(g_smart_fixed1, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
        nfui_user_signal_emit(g_smart_fixed2, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);

        _proc_set_object_status_ready();
    }
    
    return FALSE;
}

static gboolean post_searchbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (ssm_get_covert_mask() & (1 << component_data->preview.ch))
        {
            nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
            return FALSE;
        }

        if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "SEARCH") == 0)
        {   
            _search_by_smart_rev_result_clear();
        
            _proc_set_object_status_start();        
            _proc_start_smart_search(component_data);

            nfui_nfbutton_set_text((NFBUTTON*)obj, "PAUSE");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    

            g_van_status_tid = g_timeout_add(250, _tmr_check_search_status, NULL);
        }
        else if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "PAUSE") == 0)
        {
            _proc_pause_smart_search(component_data);

            nfui_nfbutton_set_text((NFBUTTON*)obj, "RESUME");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
        else if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "RESUME") == 0)
        {
            _proc_resume_smart_search(component_data);

            nfui_nfbutton_set_text((NFBUTTON*)obj, "PAUSE");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
        }
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
			return TRUE;
		}		
	}
	else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        VAAID vaaid;
        ITX_VAZONE_CONF conf;
        gint i;

        top = nfui_nfobject_get_top(obj);	
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
        vaaid = vaa_get_pb_vaaid(component_data->preview.ch);        

        nfui_nfobject_disable(obj);

        for (i = 0; i < 16; i++)
        {
            vaa_itx_get_zone_conf(vaaid, i, &conf);
            
            if (conf.use_zone)
            {
                nfui_nfobject_enable(obj);
                break;
            }
        }

        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean post_stopbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        mb_type ret;
	
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);	
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        _proc_pause_smart_search(component_data);

        ret = nftool_mbox((NFWINDOW *)obj->parent, "WARNING", "You cannot resume the search after stop.\nDo you want to stop?", NFTOOL_MB_OKCANCEL);
        
        if (ret == NFTOOL_MB_CANCEL) 
        {
            _proc_resume_smart_search(component_data);        
            return FALSE;
        }

    	if (g_van_status_tid) 
    	{
    		g_source_remove(g_van_status_tid);
    		g_van_status_tid = 0;
    	}	    
        
        _proc_stop_smart_search(component_data);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
			return TRUE;
		}		
	}

	return FALSE;
}

static gboolean post_play_rew_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        vsm_playback_change_dir_rate(DIR_DS_BWD);
    }

    return FALSE;    
}

static gboolean post_play_back_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        vsm_playback_change_dir_rate(DIR_BWD);
    }

    return FALSE;    
}

static gboolean post_play_pause_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        vsm_playback_change_dir_rate(DIR_PAUSE);
    }

    return FALSE;    
}

static gboolean post_play_forward_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        vsm_playback_change_dir_rate(DIR_FWD);
    }

    return FALSE;    
}

static gboolean post_play_ff_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
    {
        vsm_playback_change_dir_rate(DIR_DS_FWD);
    }

    return FALSE;    
}

static gboolean post_statistic_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        VCA_COMPONENT_DATA_T *component_data;
        NFOBJECT *top;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data =(VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        vw_init_smart_search_statistic_page(g_curwnd, g_from_tv, g_to_tv, component_data->preview.ch);

    }
    else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
    {
        GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
			return TRUE;
		}		
    }    
    return FALSE;
}

static gboolean post_importbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
        _open_rule_import_popup(component_data->video_fixed, component_data);
        nfui_user_signal_emit(g_tab_page[SEARCH_RULE_TAB], NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);    
        nfui_user_signal_emit(_get_searchbysmart_search_btn(), NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);            
    }

	return FALSE;
}    

static gboolean post_playbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
    	NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;

        GTimeVal tv;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        component_data->preview.onoff = 0;    
        vw_vca_rev_video_component_sync_preview(component_data->video_fixed);

        tv.tv_sec = component_data->preview.play_from;
        tv.tv_usec = 0;

        VW_Search_start_playback();
    	vsm_playback_start((guint)(1 << component_data->preview.ch), tv, PLAYBACK_NORMAL);
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
			return TRUE;
		}		
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
  	   	if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
  	   	
		VW_Search_Destroy();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
			return TRUE;
		}		
	}

	return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _set_searchbysmart_ready()
{
	NFOBJECT *top;
	VCA_COMPONENT_DATA_T *component_data;

	GTimeVal from_tv;

	top = nfui_nfobject_get_top(from_obj);

	nfui_nftimelabel_get_datetime((NFTIMELABEL*)from_obj, &from_tv);

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    component_data->preview.onoff = 1;    
    component_data->preview.play_mode = 0;
    component_data->preview.play_from = from_tv.tv_sec;
    component_data->preview.play_to = from_tv.tv_sec + 1000000;       
    vw_vca_rev_video_component_sync_preview(g_video_fixed);	
    vw_vca_rev_video_component_expose(g_video_fixed);        
    
    _proc_set_object_status_ready();
    return 0;
}

gint _set_searchbysmart_stop_by_time(time_t time)
{
	NFOBJECT *top;
	VCA_COMPONENT_DATA_T *component_data;

	GTimeVal from_tv;

	if (g_van_status_tid) 
	{
		g_source_remove(g_van_status_tid);
		g_van_status_tid = 0;
	}	    

    from_tv.tv_sec = time;
    from_tv.tv_usec = 0;

	nfui_nftimelabel_set_datetime((NFTIMELABEL*)from_obj, &from_tv);
	nfui_signal_emit(from_obj, GDK_EXPOSE, TRUE);

	top = nfui_nfobject_get_top(from_obj);

    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

    component_data->preview.onoff = 1;    
    component_data->preview.play_mode = 0;
    component_data->preview.play_from = from_tv.tv_sec;
    vw_vca_rev_video_component_sync_preview(g_video_fixed);	
    vw_vca_rev_video_component_expose(g_video_fixed);        

    _proc_stop_smart_search(component_data);
    return 0;
}

NFOBJECT *_get_searchbysmart_search_btn()
{
    return g_search_btn_obj;
}

NFOBJECT *_get_searchbysmart_playback_btn()
{
    return g_play_btn_obj;
}

NFOBJECT *_get_searchbysmart_statistic_btn()
{
    return g_statistic_btn_obj;
}

/*
 :  VCA_Rev_prop main_fixed.

      --------------------------------------------
      |  ---------------------------------------  |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |        FIXED1         |     FIXED2    | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      | |                       |               | |
      |  ---------------------------------------  |
      |                                           |
      |             PARENT                        |
      |-------------------------------------------|  
*/

////////////////////////////////////////////////////////////
//
// public interfaces
//

void vw_init_SearchBySmart_rev_page(NFOBJECT *parent, time_t from_time, time_t to_time)
{
	NFOBJECT *fixed1;
    NFOBJECT *fixed2;
    NFOBJECT *video_fixed;
    NFOBJECT *obj;

    GdkPixbuf *play_back[NFOBJECT_STATE_COUNT];
    GdkPixbuf *play_forward[NFOBJECT_STATE_COUNT];
    GdkPixbuf *play_rew[NFOBJECT_STATE_COUNT];
    GdkPixbuf *play_ff[NFOBJECT_STATE_COUNT];
    GdkPixbuf *play_pause[NFOBJECT_STATE_COUNT];
    GdkPixbuf *play_full[NFOBJECT_STATE_COUNT];
	GdkPixbuf *datetime_img[NFOBJECT_STATE_COUNT];
    
    gint pos_x, pos_y, i;
    gint size_w, size_h;
	gchar strCh[STRING_SIZE_CAMTITLE];
	gchar *strImage_h[2] = { MKB_IMG_SUBTAB_DIR_H_N_235, MKB_IMG_SUBTAB_DIR_H_S_235};
	guint tabcolor[3] = {COLOR_IDX(976), COLOR_IDX(978), COLOR_IDX(977)};

	DateTimeData dtdata;
	guint tformat;
	GTimeVal time_val = {0, 0};

    VCA_COMPONENT_DATA_T *component_data;
	VAAID vaaid;
    
	const gchar *strButton[] = {"PLAYBACK", "CLOSE"};
	const gchar *strTabTitle[] = {"RULE", "SEARCH RESULT"};


	g_curwnd = (NFWINDOW*)nfui_nfobject_get_top(parent);

    play_back[0] = nfui_get_image_from_file(IMG_N_PLAY_PB_BACK, NULL);
    play_back[1] = nfui_get_image_from_file(IMG_O_PLAY_PB_BACK, NULL);
    play_back[2] = nfui_get_image_from_file(IMG_P_PLAY_PB_BACK, NULL);
    play_back[3] = nfui_get_image_from_file(IMG_D_PLAY_PB_BACK, NULL);

    play_forward[0] = nfui_get_image_from_file(IMG_N_PLAY_PB_FORWARD, NULL);
    play_forward[1] = nfui_get_image_from_file(IMG_O_PLAY_PB_FORWARD, NULL);
    play_forward[2] = nfui_get_image_from_file(IMG_P_PLAY_PB_FORWARD, NULL);
    play_forward[3] = nfui_get_image_from_file(IMG_D_PLAY_PB_FORWARD, NULL);

    play_rew[0] = nfui_get_image_from_file(IMG_N_PLAY_PB_REW, NULL);
    play_rew[1] = nfui_get_image_from_file(IMG_O_PLAY_PB_REW, NULL);
    play_rew[2] = nfui_get_image_from_file(IMG_P_PLAY_PB_REW, NULL);
    play_rew[3] = nfui_get_image_from_file(IMG_D_PLAY_PB_REW, NULL);

    play_ff[0] = nfui_get_image_from_file(IMG_N_PLAY_PB_FF, NULL);
    play_ff[1] = nfui_get_image_from_file(IMG_O_PLAY_PB_FF, NULL);
    play_ff[2] = nfui_get_image_from_file(IMG_P_PLAY_PB_FF, NULL);
    play_ff[3] = nfui_get_image_from_file(IMG_D_PLAY_PB_FF, NULL);

    play_pause[0] = nfui_get_image_from_file(IMG_N_PLAY_PB_PAUSE, NULL);
    play_pause[1] = nfui_get_image_from_file(IMG_O_PLAY_PB_PAUSE, NULL);
    play_pause[2] = nfui_get_image_from_file(IMG_P_PLAY_PB_PAUSE, NULL);
    play_pause[3] = nfui_get_image_from_file(IMG_D_PLAY_PB_PAUSE, NULL);

    play_full[0] = nfui_get_image_from_file(IMG_N_PLAY_PB_FULL, NULL);
    play_full[1] = nfui_get_image_from_file(IMG_O_PLAY_PB_FULL, NULL);
    play_full[2] = nfui_get_image_from_file(IMG_P_PLAY_PB_FULL, NULL);
    play_full[3] = nfui_get_image_from_file(IMG_D_PLAY_PB_FULL, NULL);


	memset(&dtdata, 0x00, sizeof(DateTimeData));
	
	DAL_get_dateTime_data(&dtdata);
	DAL_get_dateTime_format(NULL, &tformat);

	_init_component_data(nfui_nfobject_get_top(parent), 0);
    _init_component_action(nfui_nfobject_get_top(parent), 0);
   
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data(nfui_nfobject_get_top(parent), VCA_COMPONENT_DATA);
    component_data->preview.play_from = from_time;
    component_data->preview.play_to = to_time;

	fixed1 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed1, _FIXED1_W, _FIXED1_H);
    nfui_nfobject_modify_bg(fixed1, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(fixed1);
    nfui_nffixed_put((NFFIXED*) parent, fixed1, _FIXED1_X, _FIXED1_Y);
    g_smart_fixed1 = fixed1;

    pos_x = 35;
    pos_y = 30;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += 120;

    obj = nfui_combobox_new(0, 0, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_1);
    nfui_nfobject_set_size(obj, 240, 40);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    nfui_regi_post_event_callback(obj, post_channel_event_handler);
    g_ch_combo = obj;

    for (i = 0; i < GUI_CHANNEL_CNT; i++) 
    {
        memset(strCh, 0x00, sizeof(strCh)); 
        var_get_camtitle(strCh, i);
        nfui_combobox_append_data((NFCOMBOBOX*)obj, strCh);
    }

    pos_x += 350;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FROM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 120, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);

    pos_x += 120;

	datetime_img[0] = nfui_get_image_from_file((IMG_BT_DATETIME_N), NULL);
	datetime_img[1] = nfui_get_image_from_file((IMG_BT_DATETIME_O), NULL);	
	datetime_img[2] = nfui_get_image_from_file((IMG_BT_DATETIME_P), NULL);	
	datetime_img[3] = nfui_get_image_from_file((IMG_BT_DATETIME_D), NULL);	

	nfui_get_pixbuf_size(datetime_img[0], &size_w, &size_h);

	time_val.tv_sec = from_time;
	g_from_tv.tv_sec = from_time;
	g_from_tv.tv_sec = from_time;
	obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(129));
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(128));
	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, 300, 40);			
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)obj, &time_val);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    from_obj = obj;
    
    pos_x += 300;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_from_event_handler);
	from_btn_obj = obj;

    pos_x = 505;
    pos_y += 50; 

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("TO", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 3, 40);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_nfobject_show(obj);
    
    pos_x += 120;

	time_val.tv_sec = to_time;	
	g_to_tv.tv_sec = to_time;
	obj = (NFOBJECT*)nfui_nftimelabel_new();
	nfui_nftimelabel_set_fg_color((NFTIMELABEL*)obj, COLOR_IDX(129));
	nfui_nftimelabel_set_bg_color((NFTIMELABEL*)obj, COLOR_IDX(128));
	nfui_nftimelabel_set_mode((NFTIMELABEL*)obj, prvTransDateFormat((gint)(dtdata.dateFormat)), tformat+1);
	nfui_nftimelabel_set_size((NFTIMELABEL*)obj, 300, 40);			
	nfui_nftimelabel_set_datetime((NFTIMELABEL*)obj, &time_val);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    to_obj = obj;
    
    pos_x += 300;

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), datetime_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_to_event_handler);
    to_btn_obj = obj;

    video_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(video_fixed, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nfui_nfobject_modify_bg(video_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_nfobject_show(video_fixed);
    nfui_nffixed_put((NFFIXED*)fixed1, video_fixed, 36, 179);
    nfui_regi_post_event_callback(video_fixed, post_video_fixed_event_handler);
    g_video_fixed = video_fixed;

    vw_vca_rev_video_component_open(video_fixed, 0);
    _set_component_video_fixed(nfui_nfobject_get_top(parent), video_fixed);	

    pos_x = 35;
    pos_y = 179 + VIDEO_COMPONENT_HEIGHT + 20;
    
    obj = (NFOBJECT*)nfui_checkbutton_new(TRUE);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_POPUP_NORMAL);
    nfui_check_get_size(NF_CHECKBUTTON(obj), &size_w, &size_h);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_dis_rule_check_event_handler);
    g_rule_check = obj;
    
    pos_x += size_w;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISPLAY RULE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 200, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj,  pos_x, pos_y);
    g_rule_label = obj;
    
    pos_x += 250;
    
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(" ", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(122));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, 350, 30);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj,  pos_x, pos_y);
    g_vstatus_label = obj;
    
    pos_x = 722;
    pos_y -= 10;
    
	nfui_get_pixbuf_size(play_rew[0], &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), play_rew);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_play_rew_event_cb);
    g_play_btn[0] = obj;
    
    pos_x += (size_w+1);
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), play_back);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_play_back_event_cb);
    g_play_btn[1] = obj;
    
    pos_x += (size_w+1);

 	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), play_pause);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_play_pause_event_cb);
    g_play_btn[2] = obj;
    
    pos_x += (size_w+1);

 	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), play_forward);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_play_forward_event_cb);
    g_play_btn[3] = obj;
    
    pos_x += (size_w+1);

 	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), play_ff);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_disable(obj);
    nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_play_ff_event_cb);
    g_play_btn[4] = obj;

	obj = nftool_normal_button_create_type3("SEARCH", 203);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, fixed1->width-203*2-15, fixed1->height-44-20);
	nfui_regi_post_event_callback(obj, post_searchbutton_event_handler);
	g_search_btn_obj = obj;

	obj = nftool_normal_button_create_type3("STOP", 203);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed1, obj, fixed1->width-203-10, fixed1->height-44-20);
	nfui_regi_post_event_callback(obj, post_stopbutton_event_handler);
    g_stop_btn_obj = obj;

    pos_x = 24-8 + VIDEO_COMPONENT_WIDTH + 10;

	fixed2 = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(fixed2, _FIXED2_W, _FIXED2_H);
    nfui_nfobject_modify_bg(fixed2, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_show(fixed2);
    nfui_nffixed_put((NFFIXED*)parent, fixed2, _FIXED2_X, _FIXED2_Y);
    g_smart_fixed2 = fixed2;

    pos_x = 0;
    pos_y = 20;

    obj = (NFOBJECT*)nfui_nftab_new(SEARCH_SUBTAB_CNT, strImage_h, 235, 43, NFTAB_DIR_H, strTabTitle, tabcolor);
	nfui_nftab_set_pango_font((NFTAB*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nftab_set_margin((NFTAB*)obj, 5);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);
    nfui_regi_pre_event_callback(obj, pre_subtab_event_handler);
    g_tab = obj;
	
    pos_y += 40;

	for(i=0; i<SEARCH_SUBTAB_CNT; i++) {
		obj = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(obj, fixed2->width, fixed2->height - 80);
		nfui_nftab_regi_page((NFTAB*)g_tab, obj, i);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nffixed_put((NFFIXED*)fixed2, obj, pos_x, pos_y);
		nfui_regi_pre_event_callback(obj, pre_page_event_cb);
		g_tab_page[i] = obj;
	}

    _open_rule_page(g_tab_page[SEARCH_RULE_TAB]);	
	_open_search_result_page(g_tab_page[SEARCH_RESULT_TAB]);

    nfui_nftab_unregi_page((NFTAB *)g_tab, SEARCH_RESULT_TAB);	
	nfui_nfobject_show(g_tab_page[SEARCH_RULE_TAB]);

    vaaid = vaa_get_pb_vaaid(0);
    vaa_itx_raiseup(vaaid);

    nfui_user_signal_emit(g_smart_fixed1, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
    nfui_user_signal_emit(g_smart_fixed2, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
    	
	obj = nftool_normal_button_create_type1("IMPORT RULE", 226);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, 0, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_importbutton_event_handler);
    g_import_btn_obj = obj;
    	
	obj = nftool_normal_button_create_type1(strButton[0], MENU_BTN_WIDTH);
	nfui_nfobject_disable(obj);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R2_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_playbutton_event_handler);
	g_play_btn_obj = obj;

	obj = nftool_normal_button_create_type1("STATISTIC", MENU_BTN_WIDTH);
	nfui_nfobject_disable(obj);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R3_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_statistic_event_handler);
	g_statistic_btn_obj = obj;

	obj = nftool_normal_button_create_type2(strButton[1], MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_H_BTN_R1_X, MENU_H_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);	
    g_close_btn_obj = obj;

	g_timeout_add(1000, _init_vca_rule, 0);
	g_preview_status_tid = g_timeout_add(200, _tmr_check_preview_status, NULL);
}

gboolean vw_SearchBySmart_rev_tab_in_handler(void)
{
	NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    
	while (1) {
		usleep(100000);
		if (!nfui_nfthumbnail_check_running()) break;
	}

    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
    component_data->preview.onoff = 1;    
    component_data->preview.play_mode = 0;
    component_data->preview.play_to = component_data->preview.play_from + 1000000;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);
    vw_vca_rev_video_component_sync_preview(g_video_fixed);

	return FALSE;
}

gboolean vw_SearchBySmart_rev_tab_out_handler(void)
{
	NFOBJECT *top;
    VCA_COMPONENT_DATA_T *component_data;
    
    top = nfui_nfobject_get_top(g_video_fixed);
    component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
    
    component_data->preview.onoff = 0;    
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);
    vw_vca_rev_video_component_sync_preview(g_video_fixed);
    vw_vca_rev_video_component_expose(g_video_fixed);

	return FALSE;
}	

gboolean vw_SearchBySmart_rev_tab_show(void)
{
	vw_SearchBySmart_rev_tab_in_handler();
	return FALSE;
}

