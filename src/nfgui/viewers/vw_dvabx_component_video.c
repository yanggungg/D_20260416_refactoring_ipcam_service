/*
 * vw_dvabx_component_video.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
 *
 */

#include <glib.h>
#include <math.h>
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

#include "vw_dva.h"
#include "vw_dit_dva.h"
#include "vw_dvabx_component.h"

#include "dvaa.h"
#include "dvaa_itx.h"

#include "nf_api_play.h"

#define MOUSE_LEFT_BUTTON						(1)
#define MOUSE_RIGHT_BUTTON						(3)


////////////////////////////////////////////////////////////
//
// private data types
//

typedef struct DVA_MEVENT_CALLBACK
{
	DVA_MEVENT_CB_FUNC 	cb_func;
	gpointer 			user_data;
} DVA_MEVENT_CALLBACK;



////////////////////////////////////////////////////////////
//
// private variable
//

static DVA_MEVENT_CALLBACK g_mevent_cb[32][DVA_MEVENT_MAX] = {0, };



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _get_cnvs_size(DVAAID dvaaid, gint *cnvs_w, gint *cnvs_h)
{
	DITID ditid;
	
	ditid = dvaa_itx_get_dit(dvaaid);
	dit_get_cnvs_info(ditid, cnvs_w, cnvs_h);
	return 0;
}

static gint _trans_vpoint(gint vcnvs, gint rpoint, gint rcnvs)
{
	gint vpoint;	
	vpoint = (gint)((float)(rpoint*vcnvs/rcnvs));
	return vpoint;
}

static GdkPoint rotate_point(gint cnvs_w, gint cnvs_h, gint x, gint y, gint degree)
{
    float rad = degree*3.1415926/180;
    float s = sin(rad);
    float c = cos(rad);
    GdkPoint npt;

    if (degree == 0) {
        npt.x = x;
        npt.y = y;
    }
    else {
        npt.x = cnvs_h/2 + (x-cnvs_w/2) * c - (y-cnvs_h/2) * s;
        npt.y = cnvs_w/2 + (x-cnvs_w/2) * s + (y-cnvs_h/2) * c;
    }
    return npt;
}

static gint _get_scr_info(NFOBJECT *obj, DVA_SCREEN_INFO *scr_info)
{
    NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;
    gint org_width;
    gint cnvs_w, cnvs_h;
    gint degree = 0;

    top = nfui_nfobject_get_top(obj);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    if (component_data->max_ratio_w == 16) // max stream ratio -> 16:9
    {
        nfui_nfobject_get_offset(obj, &scr_info->x, &scr_info->y);
        nfui_nfobject_get_size(obj, &scr_info->w, &scr_info->h);
    }
    else // max stream ratio -> 4:3
    { 
        nfui_nfobject_get_offset(obj, &scr_info->x, &scr_info->y);
        scr_info->w = VIDEO_COMPONENT_4_3_W;
        scr_info->h = VIDEO_COMPONENT_4_3_H;
    }

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    if (component_data->corridor_mode)
    {
        org_width = scr_info->w;
        scr_info->w = scr_info->h * component_data->stream_ratio_h/component_data->stream_ratio_w;
        scr_info->x = scr_info->x+(org_width - scr_info->w)/2;
        _get_cnvs_size(dvaaid, &cnvs_h, &cnvs_w);

        if (component_data->corridor_mode == 1) degree = -90;
        else degree = -270;
    }
    else
    {
        _get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);
    }

    scr_info->scale_x = (float)cnvs_w/scr_info->w;
    scr_info->scale_y = (float)cnvs_h/scr_info->h;
    scr_info->degree = degree;
    return 0;
}

static gint _convert_send_mouse_pointer(DVAAID dvaaid, DVA_SCREEN_INFO *scrInfo, gint corridor_mode, DVA_MEVENT_PT *evt_pt, DVA_MEVENT_PT *send_pt)
{
    gint x, y, cnvs_w, cnvs_h;
    GdkPoint npt;

    if (corridor_mode) {
        _get_cnvs_size(dvaaid, &cnvs_h, &cnvs_w);
    }
    else {
        _get_cnvs_size(dvaaid, &cnvs_w, &cnvs_h);
    }

    x = _trans_vpoint(cnvs_w, evt_pt->x-scrInfo->x, scrInfo->w);
    y = _trans_vpoint(cnvs_h, evt_pt->y-scrInfo->y, scrInfo->h);

    npt = rotate_point(cnvs_w, cnvs_h, x, y, scrInfo->degree);
    send_pt->x = npt.x;
    send_pt->y = npt.y;
    return 0;
}

static gint _is_in_rule_figure(DVAAID dvaaid, DVA_SCREEN_INFO *scr_info, gint mx, gint my, gint *type, gint *zone_id)
{
	ITX_ZONEID zoneid;
	ITX_CNTRID cntrid;
	
    zoneid = dvaa_itx_detector_find_zone(dvaaid, mx, my, scr_info->scale_x, scr_info->scale_y);
    if (zoneid != -1) 
    {
        *type = 0;
        *zone_id = zoneid;
        return 1;
    }

    cntrid = dvaa_itx_detector_find_cntr(dvaaid, mx, my, scr_info->scale_x, scr_info->scale_y, scr_info->degree);
    if (zoneid != -1) 
    {
        *type = 1;
        *zone_id = cntrid;
        return 1;
    }
	
    return 0;
}

static gint _draw_video_blank(NFOBJECT *obj)
{
    NFOBJECT *top;	
    DVA_COMPONENT_DATA_T *component_data;

    GdkGC *gc;
    GdkDrawable *drawable;
    guint x, y;

    top = nfui_nfobject_get_top(obj);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    if (component_data->max_ratio_w == 4) // max stream ratio -> 4:3
    {
        if (obj->width == VIDEO_COMPONENT_4_3_W) return FALSE;

        drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset(obj, &x, &y);
        gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(186));
        gdk_draw_rectangle(drawable, gc, TRUE, x+VIDEO_COMPONENT_4_3_W, y, VIDEO_COMPONENT_16_9_W-VIDEO_COMPONENT_4_3_W, obj->height);
        nfui_nfobject_gc_unref(gc);
    }

    return 0;
}

static gint _start_live(gint pos_x, gint pos_y, DVA_COMPONENT_DATA_T *component_data)
{
    gint stream_wid, stream_hei;
    
    if (component_data->max_ratio_w == 16) // max stream ratio -> 16:9
    {
        if (component_data->stream_ratio_w == 16) // current stream ratio -> 16:9
        {
            vsm_live_preview_start_dva(component_data->preview.ch, pos_x, pos_y, VIDEO_COMPONENT_16_9_W, VIDEO_COMPONENT_16_9_H);
        }
        else 
        {
            stream_wid = VIDEO_COMPONENT_16_9_W*12/16;
            vsm_live_preview_start_dva(component_data->preview.ch, pos_x+(VIDEO_COMPONENT_16_9_W-stream_wid)/2, pos_y, stream_wid, VIDEO_COMPONENT_16_9_H);
        }
    }
    else // max stream ratio -> 4:3
    { 
        if (component_data->stream_ratio_w == 4) // current stream ratio -> 16:9
        {
            vsm_live_preview_start_dva(component_data->preview.ch, pos_x, pos_y, VIDEO_COMPONENT_4_3_W, VIDEO_COMPONENT_4_3_H);
        }
        else 
        {
            stream_hei = VIDEO_COMPONENT_4_3_H*9/12;
            vsm_live_preview_start_dva(component_data->preview.ch, pos_x, pos_y+(VIDEO_COMPONENT_4_3_H-stream_hei)/2, VIDEO_COMPONENT_4_3_W, stream_hei);
        }
    }
    return 0;
}

static gint _stop_live(DVA_COMPONENT_DATA_T *component_data)
{
    vsm_live_preview_stop();
    return 0;
}

static gint _start_playback(gint pos_x, gint pos_y, gint width, gint height, DVA_COMPONENT_DATA_T *component_data)
{
	DVAAID dvaaid;
	DITID ditid;

	GTimeVal tv_start;
	GTimeVal tv_end;

    dvaaid = dvaa_get_pb_dvaaid(component_data->preview.ch);
	ditid = dvaa_get_ditid(dvaaid);
	vw_dit_display_set_dva_ditid(ditid);

    memset(&tv_start, 0x00, sizeof(GTimeVal));
    memset(&tv_end, 0x00, sizeof(GTimeVal));

    g_message("%s, %d, x:%d, y:%d, width:%d, height:%d", __FUNCTION__, __LINE__, pos_x, pos_y, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);
    nf_play_set_smart_geometry(component_data->preview.ch, pos_x, pos_y, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);

    tv_start.tv_sec = component_data->preview.play_from;
    tv_end.tv_sec = component_data->preview.play_to;

    if (component_data->preview.play_mode == 0)
    {
        vsm_playback_smart_preview_start(component_data->preview.ch, tv_start);
    	usleep(500000);
    	vsm_playback_smart_preview_stop();
    }
    else if (component_data->preview.play_mode == 1)
    {
        vsm_playback_smart_mainview_start(component_data->preview.ch, tv_start, tv_end, NF_PLAY_SMART_SEARCH_META);
    }
    else if (component_data->preview.play_mode == 2)
    {
        vsm_playback_smart_preview_start(component_data->preview.ch, tv_start);
    }
    
    return 0;
}

static gint _stop_playback(DVA_COMPONENT_DATA_T *component_data)
{
    vw_dit_display_set_dva_ditid(0);

	vsm_playback_smart_mainview_stop(NF_PLAY_SMART_SEARCH_META);
	vsm_playback_smart_preview_stop();

	nf_play_unset_smart_geometry();
    return 0;
}





////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_video_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint property = -1;
    
    DVA_SCREEN_INFO scr_info;
    DVA_MEVENT_PT mevt_pt;
    DVA_MEVENT_PT send_pt;

	if (evt->type == GDK_EXPOSE)
    {
        NFOBJECT *top;	
        DVA_COMPONENT_DATA_T *component_data;
    
        GdkGC *gc;
        GdkDrawable *drawable;
        guint x, y;

        _draw_video_blank(obj);

	    top = nfui_nfobject_get_top(obj);
		component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
		component_data->disp_rule.force_update = 1;
    }
	else if (evt->type == GDK_DELETE)
	{
        NFOBJECT *top;	
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;
    
	    top = nfui_nfobject_get_top(obj);
		component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
		
        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
		dvaa_itx_disable_strule(dvaaid);
        dvaa_itx_remove_highlight(dvaaid);

		vw_dit_display_free_dva_diclist(component_data->disp_rule.clon);
	}
    else if (evt->type == NFEVENT_DVA_COMPONENT_PREVIEW_SYNC)
    {
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
    	DVAAID dvaaid;  

        gint win_x, win_y;
        gint off_x, off_y;
      
	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if ((component_data->preview.type == 1) && (ssm_get_covert_mask() & (1 << component_data->preview.ch)))
        {
            if (ssm_get_covert_shown_as()) nfui_nflabel_set_text((NFLABEL*)obj, "COVERT");
            else nfui_nflabel_set_text((NFLABEL*)obj, "NO VIDEO");
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
            _stop_playback(component_data);
            return FALSE;
        }

        nfui_nflabel_set_text((NFLABEL*)obj, "");
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, component_data->preview.bg_color);

        if (component_data->preview.onoff)
        {       
            win_x = top->x;
            win_y = top->y;
            nfui_nfobject_get_offset(obj, &off_x, &off_y);        

            component_data->corridor_mode = scm_get_ipcam_corridor_mode(component_data->preview.ch);
            scm_ipcam_get_max_stream_ratio(component_data->preview.ch, &component_data->max_ratio_w, &component_data->max_ratio_h);
            scm_ipcam_get_main_stream_ratio(component_data->preview.ch, &component_data->stream_ratio_w, &component_data->stream_ratio_h);

            if (component_data->preview.type == 0)  {
                _start_live(win_x+off_x, win_y+off_y, component_data);
                dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
            }
            else {
                _start_playback(win_x+off_x, win_y+off_y, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT, component_data);
                dvaaid = dvaa_get_pb_dvaaid(component_data->preview.ch);
            }

            dvaa_itx_enable_strule(dvaaid);

            if (!strcmp(component_data->algorithm_type, ALGOTYPE_DETECTOR)) dvaa_itx_active_algotithm(dvaaid, ITX_ALGOTYPE_DETECTOR);
            else if (!strcmp(component_data->algorithm_type, ALGOTYPE_FACE)) dvaa_itx_active_algotithm(dvaaid, ITX_ALGOTYPE_FACE);
            else if (!strcmp(component_data->algorithm_type, ALGOTYPE_PLATENO)) dvaa_itx_active_algotithm(dvaaid, ITX_ALGOTYPE_PLATENO);
            else dvaa_itx_active_algotithm(dvaaid, ITX_ALGOTYPE_DETECTOR);

            if (component_data->preview.calb_onoff) { 
                dvaa_deactivate_all_rule(dvaaid);
                dvaa_deactivate_meta(dvaaid, DVAA_META_BBOX);
                dvaa_activate_calb(dvaaid);
            }
            else {
                dvaa_activate_all_rule(dvaaid);
                dvaa_activate_meta(dvaaid, DVAA_META_BBOX);
                dvaa_deactivate_calb(dvaaid);
            }
        }
        else
        {
            if (component_data->preview.type == 0) _stop_live(component_data);
            else _stop_playback(component_data);
        }
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {   
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        GdkWindow *rootwin;
    	DVA_DP dp;
    	DVA_CLON preClon;
    	DVA_CLON postClon;
        gint x, y, width, height;

        if (!nfui_nfobject_is_shown(obj)) return FALSE;

	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->act_capable == 0) return FALSE;
        if (component_data->act_license == 0) return FALSE;
    	if (component_data->disp_rule.block_update) return FALSE;

    	if (component_data->disp_rule.delay_update) 
    	{
    	    if (component_data->disp_rule.delay_cnt < component_data->disp_rule.delay_max)
    	    {
    	        component_data->disp_rule.delay_cnt++;
        	    return FALSE;
    	    }

    	    component_data->disp_rule.delay_cnt = 0;
    	    component_data->disp_rule.delay_max = 0;
    	    component_data->disp_rule.delay_update = 0;    	    
        }

    	preClon.dic_cnt = component_data->disp_rule.clon.dic_cnt;
    	preClon.pdics = component_data->disp_rule.clon.pdics;
    	postClon.dic_cnt = 0;
    	postClon.pdics = 0;	
    	vw_dit_display_get_dva_diclist(&postClon);	

    	if ((vw_dit_display_compare_dva_diclist(preClon, postClon) == 1) && (component_data->disp_rule.force_update == 0)) 
    	{
    	    vw_dit_display_free_dva_diclist(preClon);
    	    component_data->disp_rule.clon.dic_cnt = postClon.dic_cnt;
    	    component_data->disp_rule.clon.pdics = postClon.pdics;    	    
    		return FALSE;
    	}

        dp.degree = 0;
    	nfui_nfobject_get_offset(obj, &x, &y);

        if (component_data->max_ratio_w == 16) // max stream ratio -> 16:9
        {
            width = VIDEO_COMPONENT_16_9_W;
            height = VIDEO_COMPONENT_16_9_H;
        }
        else // max stream ratio -> 4:3
        { 
            width = VIDEO_COMPONENT_4_3_W;
            height = VIDEO_COMPONENT_4_3_H;
        }

        dp.plt_area.x = 0;
        dp.plt_area.y = 0;
        if (component_data->corridor_mode)
        {
            dp.plt_area.width = height * component_data->stream_ratio_h/component_data->stream_ratio_w;
            dp.plt_area.height = height;

            if (component_data->corridor_mode == 1) dp.degree = 90;
            else dp.degree = 270;
        }
        else {
            dp.plt_area.width = width;
            dp.plt_area.height = height;
        }
        
        rootwin = gdk_get_default_root_window();
        dp.drawable = gdk_pixmap_new(rootwin, dp.plt_area.width, height, -1);
        dp.gc = gdk_gc_new(dp.drawable);
        
        {
            // to solve https://qts.itxm2m.com:8443/browse/SWPFOURCE-1197
            gdk_gc_set_rgb_fg_color(dp.gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
            gdk_draw_rectangle(dp.drawable, dp.gc, TRUE, 0, 0, dp.plt_area.width, height);
        }

    	vw_dit_display_dva_erase(&dp, preClon, postClon);
        vw_dit_display_free_dva_diclist(preClon);
    	vw_dit_display_dva_draw(&dp, postClon);

        if (component_data->corridor_mode) {
            gdk_draw_drawable(nfui_nfobject_get_window(obj), nfui_nfobject_get_gc(obj), dp.drawable, 0, 0, x+(width - dp.plt_area.width)/2, y, -1, -1);
        }
        else {
            gdk_draw_drawable(nfui_nfobject_get_window(obj), nfui_nfobject_get_gc(obj), dp.drawable, 0, 0, x, y, -1, -1);
        }

        g_object_unref(dp.drawable);
    	g_object_unref(dp.gc);

	    component_data->disp_rule.clon.dic_cnt = postClon.dic_cnt;
	    component_data->disp_rule.clon.pdics = postClon.pdics;
        component_data->disp_rule.force_update = 0;
    }
	else if (evt->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent; 

        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;
        gint type, zone_id;

		bevent = (GdkEventButton*)evt;

		if (bevent->button == MOUSE_LEFT_BUTTON) property = DVA_MEVENT_LEFT_PRESS;
		else if (bevent->button == MOUSE_RIGHT_BUTTON) property = DVA_MEVENT_RIGHT_PRESS;

		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;

	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        if (component_data->check_config)
        {
            _get_scr_info(obj, &scr_info);

            dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
            _convert_send_mouse_pointer(dvaaid, &scr_info, component_data->corridor_mode, &mevt_pt, &send_pt);
            if (_is_in_rule_figure(dvaaid, &scr_info, send_pt.x, send_pt.y, &type, &zone_id))
            {
                if (component_data->rule_type == RTYPE_LINE)
                {
                     if ((type != 0) || (zone_id != component_data->zone_id))
                     {
                        if ((component_data->line.forward == 0) && (component_data->line.reverse == 0))
                        {
                            nftool_mbox((NFWINDOW*)top, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
                            return FALSE;
                        }
                    }
                }
                else if (component_data->rule_type == RTYPE_AREA)
                {
                     if ((type != 0) || (zone_id != component_data->zone_id))
                     {                
                        if ((component_data->area.enter == 0) && (component_data->area.exit == 0) && \ 
                            (component_data->area.removed == 0) && (component_data->area.loitering == 0) && (component_data->area.stopped == 0))
                        {
                            nftool_mbox((NFWINDOW*)top, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
                            return FALSE;
                        }
                    }
                }
                else if (component_data->rule_type == RTYPE_COUNTER)
                {
                     if ((type != 1) || (zone_id != component_data->zone_id))
                     {                                
                        if(component_data->counter.use_counter_event == 0 && component_data->counter.use_reset_value == 0)
                        {
                            nftool_mbox((NFWINDOW*)top, "NOTICE", "Please select at least one event.", NFTOOL_MB_OK);
                            return FALSE;
                        }
                    }
                }
            }
        }		
	}
	else if (evt->type == GDK_MOTION_NOTIFY)
	{
		GdkEventMotion *mevent;

		mevent = (GdkEventMotion*)evt;

		if (mevent->state & GDK_BUTTON1_MASK) property = DVA_MEVENT_DRAG;
		else property = DVA_MEVENT_MOVE;

		mevt_pt.x = (gint)mevent->x;
		mevt_pt.y = (gint)mevent->y;
	}	
	else if (evt->type == GDK_BUTTON_RELEASE)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		property = DVA_MEVENT_RELEASE;

		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;	
	}
	else if (evt->type == GDK_2BUTTON_PRESS)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		if (bevent->button == MOUSE_LEFT_BUTTON) property = DVA_MEVENT_LEFT_2PRESS;
		else if (bevent->button == MOUSE_RIGHT_BUTTON) property = DVA_MEVENT_RIGHT_2PRESS;

		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;
	}	

	if (property >= 0)
	{
        NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;
        gint algo_type = ITX_ALGOTYPE_DETECTOR;

	    top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
        if (component_data->disable_event) return FALSE;	

        if (!strcmp(component_data->algorithm_type, ALGOTYPE_DETECTOR)) algo_type = ITX_ALGOTYPE_DETECTOR;
        else if (!strcmp(component_data->algorithm_type, ALGOTYPE_FACE)) algo_type = ITX_ALGOTYPE_FACE;
        else if (!strcmp(component_data->algorithm_type, ALGOTYPE_PLATENO)) algo_type = ITX_ALGOTYPE_PLATENO;

        if ((g_mevent_cb[algo_type][property].cb_func)) 
        {
            _get_scr_info(obj, &scr_info);

            dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
            _convert_send_mouse_pointer(dvaaid, &scr_info, component_data->corridor_mode, &mevt_pt, &send_pt);
            g_mevent_cb[algo_type][property].cb_func(&scr_info, &send_pt, g_mevent_cb[algo_type][property].user_data);
        }
	}

	return FALSE;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_dvabx_video_component_open(NFOBJECT *parent, guint opt)
{
	NFOBJECT *obj;	

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(662));
	nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_nfobject_set_size(obj, parent->width, parent->height);
	nfui_nfobject_use_hierarchy(obj, NFOBJECT_HIERARCHY_OFF);
	nfui_nffixed_put((NFFIXED*)parent, obj, 0, 0);
	nfui_regi_post_event_callback(obj, post_video_label_event_handler);	
	nfui_nfobject_show(obj);

	return 0;
}

gint vw_dvabx_video_component_show()
{

    return 0;
}

gint vw_dvabx_video_component_hide()
{

    return 0;
}

gint vw_dvabx_video_component_sync_preview(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_PREVIEW_SYNC, TRUE);
    return 0;
}

gint vw_dvabx_video_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_DVA_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_dvabx_video_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint vw_itx_dvabx_attach_mevent(ITX_ALGOTYPE_E algo_type, DVA_MEVENT_E mevt_type, DVA_MEVENT_CB_FUNC mevent_cb, gpointer user_data)
{
    if (algo_type >= ITX_ALGOTYPE_MAX) return -1;
	if (mevt_type >= DVA_MEVENT_MAX) return -1;

    g_mevent_cb[algo_type][mevt_type].cb_func = mevent_cb;
    g_mevent_cb[algo_type][mevt_type].user_data = user_data;
	return 0;
}

