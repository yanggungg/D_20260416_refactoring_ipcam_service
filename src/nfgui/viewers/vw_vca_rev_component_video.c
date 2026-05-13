/*
 * vw_vca_rev_component_video.c
 *
 * Written by Jungkyu. <parangi@itxsecurity.com>
 * Copyright (c) ITX security, June 14, 2014
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

#include "vw_vca.h"
#include "vw_dit_vca.h"
#include "vw_vca_rev_component.h"

#include "vaa.h"
#include "vaa_itx.h"

#include "nf_api_play.h"

#define MOUSE_LEFT_BUTTON						(1)
#define MOUSE_RIGHT_BUTTON						(3)


////////////////////////////////////////////////////////////
//
// private data types
//

typedef struct _VCA_MEVENT_CALLBACK
{
	VCA_MEVENT_CB_FUNC 	cb_func;
	gpointer 			user_data;
} VCA_MEVENT_CALLBACK;



////////////////////////////////////////////////////////////
//
// private variable
//

static VCA_MEVENT_CALLBACK g_mevent_cb[VCA_MEVENT_MAX] = {0, };



////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _get_cnvs_size(VAAID vaaid, gint *cnvs_w, gint *cnvs_h)
{
	DITID ditid;
	
	ditid = vaa_itx_get_dit(vaaid);
	dit_get_cnvs_info(ditid, cnvs_w, cnvs_h);
	return 0;
}

static gint _trans_vpoint(gint vcnvs, gint rpoint, gint rcnvs)
{
	gint vpoint;	
	vpoint = (gint)((float)(rpoint*vcnvs/rcnvs));
	return vpoint;
}

static gint _is_in_rule_figure(NFOBJECT *obj, VAAID vaaid, gint mouse_x, gint mouse_y, gint *type, gint *zone_id)
{
	ITX_ZONEID zoneid;
	ITX_CNTRID cntrid;

	gint cnvs_w, cnvs_h;
    gint off_x, off_y, video_w, video_h;
	
	gint trans_x, trans_y;
    gint i;

	_get_cnvs_size(vaaid, &cnvs_w, &cnvs_h);

	nfui_nfobject_get_offset(obj, &off_x, &off_y);
	nfui_nfobject_get_size(obj, &video_w, &video_h);

	trans_x = _trans_vpoint(cnvs_w, mouse_x-off_x, video_w);
	trans_y = _trans_vpoint(cnvs_h, mouse_y-off_y, video_h);

    zoneid = vaa_itx_find_zone(vaaid, trans_x, trans_y);
    if (zoneid != -1) 
    {
        *type = 0;
        *zone_id = zoneid;        
        return 1;
    }

    cntrid = vaa_itx_find_cntr(vaaid, trans_x, trans_y);
    if (zoneid != -1) 
    {
        *type = 1;
        *zone_id = cntrid;        
        return 1;
    }
	
    return 0;
}


static gint _start_live(gint pos_x, gint pos_y, gint width, gint height, VCA_COMPONENT_DATA_T *component_data)
{
    vsm_live_preview_start_vca(component_data->preview.ch, pos_x, pos_y, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT);

    return 0;
}

static gint _stop_live(VCA_COMPONENT_DATA_T *component_data)
{
    vsm_live_preview_stop();
    return 0;
}

static gint _start_playback(gint pos_x, gint pos_y, gint width, gint height, VCA_COMPONENT_DATA_T *component_data)
{
	VAAID vaaid;
	DITID ditid;

	GTimeVal tv_start;
	GTimeVal tv_end;

    vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
	ditid = vaa_get_ditid(vaaid);
	vw_dit_display_set_vca_ditid(ditid);

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

static gint _stop_playback(VCA_COMPONENT_DATA_T *component_data)
{
    vw_dit_display_set_vca_ditid(0);

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
    
    VCA_SCREEN_INFO scr_info;
    VCA_MEVENT_PT mevt_pt;

	if (evt->type == GDK_EXPOSE)
    {
        NFOBJECT *top;	
        VCA_COMPONENT_DATA_T *component_data;
    
	    top = nfui_nfobject_get_top(obj);
		component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

		component_data->disp_rule.force_update = 1;
    }
	else if (evt->type == GDK_DELETE)
	{
        NFOBJECT *top;	
        VCA_COMPONENT_DATA_T *component_data;
    
	    top = nfui_nfobject_get_top(obj);
		component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);
		
		vw_dit_display_free_vca_diclist(component_data->disp_rule.clon);
	}
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_PREVIEW_SYNC)
    {
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
    	VAAID vaaid;

        gint win_x, win_y;
        gint off_x, off_y;
        gint shown_as;
      
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if ((component_data->preview.type == 1) && (ssm_get_covert_mask() & (1 << component_data->preview.ch)))
        {
            shown_as = ssm_get_covert_shown_as();

            if (shown_as)
                nfui_nflabel_set_text((NFLABEL*)obj, "COVERT");
            else
                nfui_nflabel_set_text((NFLABEL*)obj, "NO VIDEO");
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

            if (component_data->preview.type == 0) 
            {
                _start_live(win_x+off_x, win_y+off_y, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT, component_data);
                vaaid = vaa_get_vaaid(component_data->preview.ch);
            }
            else
            {
                _start_playback(win_x+off_x, win_y+off_y, VIDEO_COMPONENT_WIDTH, VIDEO_COMPONENT_HEIGHT, component_data);            
                vaaid = vaa_get_pb_vaaid(component_data->preview.ch);
            }

            if (component_data->preview.rule_mode)
            {
                vaa_itx_deactivate_all_rule(vaaid);
                vaa_itx_deactivate_all_meta(vaaid);
                vaa_itx_activate_calb(vaaid);
            }
            else
            {
                vaa_itx_activate_all_rule(vaaid);
                vaa_itx_activate_all_meta(vaaid);
                vaa_itx_deactivate_calb(vaaid);
            }
        }
        else
        {
            if (component_data->preview.type == 0) 
            {
                _stop_live(component_data);
            }
            else
            {
                _stop_playback(component_data);
            }
        }
    }
    else if (evt->type == NFEVENT_VCAREV_COMPONENT_DATA_SYNC)
    {   
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        
    	VCA_DP dp;
    	VCA_CLON preClon;
    	VCA_CLON postClon;	    	

        if (!nfui_nfobject_is_shown(obj)) return FALSE;

    	dp.drawable = nfui_nfobject_get_window(obj);
    	if (!dp.drawable) return FALSE;

	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

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
    	vw_dit_display_get_vca_diclist(&postClon);	

    	if ((vw_dit_display_compare_vca_diclist(preClon, postClon) == 1) && (component_data->disp_rule.force_update == 0)) 
    	{
    	    vw_dit_display_free_vca_diclist(preClon);
    	    component_data->disp_rule.clon.dic_cnt = postClon.dic_cnt;
    	    component_data->disp_rule.clon.pdics = postClon.pdics;    	    
    		return FALSE;
    	}

    	dp.gc = nfui_nfobject_get_gc(obj);
    	nfui_nfobject_get_offset(obj, &dp.plt_area.x, &dp.plt_area.y);
    	nfui_nfobject_get_size(obj, &dp.plt_area.width, &dp.plt_area.height);

    	vw_dit_display_vca_erase(&dp, preClon, postClon);
        vw_dit_display_free_vca_diclist(preClon);

    	vw_dit_display_vca_draw(&dp, postClon);

	    component_data->disp_rule.clon.dic_cnt = postClon.dic_cnt;
	    component_data->disp_rule.clon.pdics = postClon.pdics;
        component_data->disp_rule.force_update = 0;
    	
    	nfui_nfobject_gc_unref(dp.gc);
    }
	else if (evt->type == GDK_BUTTON_PRESS)
	{
		GdkEventButton *bevent; 

        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
        gint type, zone_id;

		bevent = (GdkEventButton*)evt;

		if (bevent->button == MOUSE_LEFT_BUTTON) property = VCA_MEVENT_LEFT_PRESS;
		else if (bevent->button == MOUSE_RIGHT_BUTTON) property = VCA_MEVENT_RIGHT_PRESS;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;

	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (component_data->check_config)
        {
            if (_is_in_rule_figure(obj, (VAAID)g_mevent_cb[property].user_data, mevt_pt.x, mevt_pt.y, &type, &zone_id))
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

		if (mevent->state & GDK_BUTTON1_MASK) property = VCA_MEVENT_DRAG;
		else property = VCA_MEVENT_MOVE;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)mevent->x;
		mevt_pt.y = (gint)mevent->y;
	}	
	else if (evt->type == GDK_BUTTON_RELEASE)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		property = VCA_MEVENT_RELEASE;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;	
	}
	else if (evt->type == GDK_2BUTTON_PRESS)
	{
		GdkEventButton *bevent; 

		bevent = (GdkEventButton*)evt;

		if (bevent->button == MOUSE_LEFT_BUTTON) property = VCA_MEVENT_LEFT_2PRESS;
		else if (bevent->button == MOUSE_RIGHT_BUTTON) property = VCA_MEVENT_RIGHT_2PRESS;

		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
		mevt_pt.x = (gint)bevent->x;
		mevt_pt.y = (gint)bevent->y;
	}	

	if ((property >= 0) && (g_mevent_cb[property].cb_func))
	{
        NFOBJECT *top;
        VCA_COMPONENT_DATA_T *component_data;
     
	    top = nfui_nfobject_get_top(obj);
        component_data = (VCA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, VCA_COMPONENT_DATA);

        if (component_data->disable_event) return FALSE;
	
		g_mevent_cb[property].cb_func(&scr_info, &mevt_pt, g_mevent_cb[property].user_data);
	}
	
	return FALSE;
}




////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint vw_vca_rev_video_component_open(NFOBJECT *parent, guint opt)
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

gint vw_vca_rev_video_component_show()
{

    return 0;
}

gint vw_vca_rev_video_component_hide()
{

    return 0;
}

gint vw_vca_rev_video_component_sync_preview(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCAREV_COMPONENT_PREVIEW_SYNC, TRUE);
    return 0;
}

gint vw_vca_rev_video_component_sync_data(NFOBJECT *parent)
{
    nfui_user_signal_emit(parent, NFEVENT_VCAREV_COMPONENT_DATA_SYNC, TRUE);
    return 0;
}

gint vw_vca_rev_video_component_expose(NFOBJECT *parent)
{
    nfui_signal_emit(parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint vw_itx_vca_rev_attach_mevent(VCA_MEVENT_E mevt_type, VCA_MEVENT_CB_FUNC mevent_cb, gpointer user_data)
{
	if (mevt_type >= VCA_MEVENT_MAX) return -1;

	g_mevent_cb[mevt_type].cb_func = mevent_cb;
	g_mevent_cb[mevt_type].user_data = user_data;
	return 0;
}

