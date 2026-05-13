/*
 * vw_dvabx_rcol.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, Feb 15, 2019
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

#include "vw_dvabx_prop_internal.h"
#include "vw_dvabx_component.h"
#include "dvaa_itx.h"

enum {
	RULE_PAGE = 0,
	CALIBRATION_PAGE,
	ENGINE_OPTION_PAGE,
    OSD_OPTION_PAGE,
	SUBPAGE_CNT
};


////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//
static NFOBJECT *tab_page[SUBPAGE_CNT] = {NULL, };
static NFOBJECT *g_curwnd = NULL;
static NFOBJECT *g_parent = NULL;

static NFOBJECT *g_caliadrz_fixed = 0;
static NFOBJECT *g_caliresult_fixed = 0;
static NFOBJECT *g_option_fixed = 0;

static ITX_DVACALB_SHAPE g_calb_shape[32];
static ITX_DVACALB_CONF g_calb_conf[32];
static ITX_DVACALB_RESULT g_calb_result;



////////////////////////////////////////////////////////////
//
// private interfaces 
//
static _get_calb_init_data(DVAAID dvaaid)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_shape(dvaaid, i, &g_calb_shape[i]);           
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &g_calb_conf[i]);       
    }

    dvaa_itx_detector_get_calb_result(dvaaid, &g_calb_result);

    return 0;
}

static _cancel_calb_data(DVAAID dvaaid)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_set_calb_shape(dvaaid, i, &g_calb_shape[i]);
        dvaa_itx_detector_set_calb_conf(dvaaid, i, &g_calb_conf[i]);
    }

    dvaa_itx_detector_set_calb_result(dvaaid, &g_calb_result);   

    return 0;
}

static gint _get_new_calibration_idx(DVAAID dvaaid)
{
    ITX_DVACALB_CONF conf;    
    gint i;
    
    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
        if (conf.use_calb == 0) return i;
    }

    return -1;
}

static gint _get_calibration_cnt(DVAAID dvaaid)
{
    ITX_DVACALB_CONF conf;    
    gint i, cnt = 0;
   
    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
        if (conf.use_calb == 1) cnt++;
    }

    return cnt;
}

static gint _get_focus_calibration_idx(DVAAID dvaaid)
{
    ITX_DVACALB_CONF conf;    
    gint i;
    
    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
        if ((conf.use_calb == 1) && (conf.focus == 1)) return i;
    }

    return -1;
}

static gint _set_focus_calibration(DVAAID dvaaid, gint calbid)
{
    ITX_DVACALB_CONF conf;    

    dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
    conf.focus = 1;
    dvaa_itx_detector_set_calb_conf(dvaaid, calbid, &conf);

    return 0;
}

static gint _unset_focus_calibration(DVAAID dvaaid, gint calbid)
{
    ITX_DVACALB_CONF conf;    

    dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
    conf.focus = 0;
    dvaa_itx_detector_set_calb_conf(dvaaid, calbid, &conf);
    
    return 0;
}

static gint _unset_focus_all_calibration(DVAAID dvaaid)
{
    ITX_DVACALB_CONF conf;    
    gint i;

    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
        if (conf.use_calb) _unset_focus_calibration(dvaaid, i);
    }

    return 0;
}

static gint _delete_calibration(DVAAID dvaaid, gint calbid)
{
    ITX_DVACALB_CONF conf;    

    dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
    conf.use_calb = 0;
    dvaa_itx_detector_set_calb_conf(dvaaid, calbid, &conf);
    return 0;
}


static gint _delete_all_calibration(DVAAID dvaaid)
{
    ITX_DVACALB_CONF conf;    
    gint i;

    for (i = 0; i < 32; i++)
    {
        _delete_calibration(dvaaid, i);
    }
    return 0;
}

static gint _get_targetlist_from_calb_shape(DVAAID dvaaid, ivca_calib_t *vca_calb)
{
    gint i;
    ITX_DVACALB_SHAPE shape;    

    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_shape(dvaaid, i, &shape);
    
        vca_calb->targetlist[i].pt[0].x = shape.pt[0].x;
        vca_calb->targetlist[i].pt[0].y = shape.pt[0].y;
        vca_calb->targetlist[i].pt[1].x = shape.pt[1].x;
        vca_calb->targetlist[i].pt[1].y = shape.pt[1].y;
    }

    return 0;
}

static gint _get_targetlist_from_calb_conf(DVAAID dvaaid, ivca_calib_t *vca_calb)
{
    gint i;
    ITX_DVACALB_CONF conf;    

    for (i = 0; i < 32; i++)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, i, &conf);
        vca_calb->targetlist[i].height = conf.height;
    }

    return 0;
}

static gint _get_targetlist_from_calb_result(DVAAID dvaaid, ivca_calib_t *vca_calb)
{
    ITX_DVACALB_RESULT res;    

    dvaa_itx_detector_get_calb_result(dvaaid, &res);

    vca_calb->p_width = res.p_width;
    vca_calb->p_height = res.p_height;
    vca_calb->height = res.cam_height;
    vca_calb->tilt = res.cam_tilt;
    vca_calb->focal = res.focal;
    vca_calb->paramvalid = res.paramvalid;
    
    return 0;
}

static gint _set_calb_estimate_data(DVAAID dvaaid, ivca_calib_t *vca_calb)
{
    ITX_DVACALB_RESULT res;    

    dvaa_itx_detector_get_calb_result(dvaaid, &res);

    res.p_width = vca_calb->p_width;
    res.p_height = vca_calb->p_height;
    res.cam_height = vca_calb->height;
    res.cam_tilt = vca_calb->tilt;
    res.focal = vca_calb->focal;
    res.paramvalid = vca_calb->paramvalid;

    dvaa_itx_detector_set_calb_result(dvaaid, &res);
    
    return 0;
}

static gint _update_calb_adrz_component_data(DVAAID dvaaid, DVA_COMPONENT_DATA_T *component_data)
{
    ITX_DVACALB_CONF conf;    
    ITX_CALBID calbid;

    calbid = _get_focus_calibration_idx(dvaaid);
    
    if (calbid != -1)
    {
        dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
        component_data->calibration.select_icon = 1;        
        component_data->calibration.icon_height = conf.height;
    }
    else 
    {
        component_data->calibration.select_icon = 0;
    }

    component_data->calibration.icon_count = _get_calibration_cnt(dvaaid);
    vw_dvabx_calibration_adrz_component_sync_data(g_caliadrz_fixed);
    
    return 0;
}

static gint _expose_calb_adrz_component_data()
{
    vw_dvabx_calibration_adrz_component_expose_data(g_caliadrz_fixed);
    return 0;
}

static gint _update_calb_result_component_data(DVAAID dvaaid, DVA_COMPONENT_DATA_T *component_data)
{
    ITX_DVACALB_RESULT res;    

    dvaa_itx_detector_get_calb_result(dvaaid, &res);

    component_data->calibration.camera_height = res.cam_height;
    component_data->calibration.camera_tilt = res.cam_tilt;
    component_data->calibration.param_valid = res.paramvalid;
    
    vw_dvabx_calibration_result_component_sync_data(g_caliresult_fixed);
    vw_dvabx_option_component_sync_data(g_option_fixed);    
    return 0;
}

static gint _calibration_adrz_component_add(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    DVAAID dvaaid;    
    gint calb_idx;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    calb_idx = _get_new_calibration_idx(dvaaid);

    if (calb_idx == -1) {
        nftool_mbox(g_curwnd, "NOTICE", "You can set up to 32 icons.", NFTOOL_MB_OK);
        return -1;
    }

    _unset_focus_all_calibration(dvaaid);
    dvaa_itx_detector_add_calb_default_template(dvaaid, calb_idx);
    _set_focus_calibration(dvaaid, calb_idx);
    _update_calb_adrz_component_data(dvaaid, component_data);    
    _expose_calb_adrz_component_data();
	return 0;
}

static gint _calibration_adrz_component_delete(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    DVAAID dvaaid;    
    gint calb_idx;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    calb_idx = _get_focus_calibration_idx(dvaaid);
    if (calb_idx == -1) return -1;

    _delete_calibration(dvaaid, calb_idx);
    _update_calb_adrz_component_data(dvaaid, component_data);
    _expose_calb_adrz_component_data();
    return 0;
}

static gint _calibration_adrz_component_reset(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    DVAAID dvaaid;
    ITX_DVACALB_RESULT res;
    
    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    _delete_all_calibration(dvaaid);

    dvaa_itx_detector_get_calb_result(dvaaid, &res);
    res.cam_height = 0;
    res.cam_tilt = 0;
    res.focal = 0;
    res.paramvalid = 0;    
    dvaa_itx_detector_set_calb_result(dvaaid, &res);

    _update_calb_adrz_component_data(dvaaid, component_data);
    _expose_calb_adrz_component_data();
    _update_calb_result_component_data(dvaaid, component_data);    
	return 0;
}

static gboolean _delay_preview_start(gpointer data)
{
    NFOBJECT *obj = (NFOBJECT*)data;
    nfui_ui_unlock();
    vw_dvabx_video_component_sync_preview(obj);
    vw_dvabx_video_component_expose(obj);
    return FALSE;
}

static gint _calibration_adrz_component_zoom(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;
    DVAAID dvaaid;    

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_DATA);

    component_data->preview.onoff = 0;
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_808080);    
    vw_dvabx_video_component_sync_preview(component_data->video_fixed);
    vw_dvabx_video_component_expose(component_data->video_fixed);      

    vw_dvabx_calibration_zoom_open(top, component_data->preview.ch);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    component_data->calibration.pause_video = 0;
    _update_calb_adrz_component_data(dvaaid, component_data);
    _update_calb_result_component_data(dvaaid, component_data);

    component_data->preview.onoff = 1;
    component_data->preview.bg_color = COLOR_PRG_IDX(UX_COLOR_000000);    
    component_data->disp_rule.block_update = 0;
    component_data->disp_rule.delay_update = 1;
    component_data->disp_rule.delay_max = 10;
    component_data->disp_rule.delay_cnt = 0;    

    nfui_ui_lock();
    g_timeout_add(300, _delay_preview_start, component_data->video_fixed);

	return 0;
}

static gint _calibration_adrz_component_pause_video(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_DATA);

    if (component_data->calibration.pause_video)
    {
        vsm_live_preview_pause_vca();
    }
    else
    {
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);    
    }
    
	return 0;
}

static gint _calibration_adrz_component_icon_height(gpointer user_data)
{
    DVA_COMPONENT_DATA_T *component_data;
    NFOBJECT *top;
    NFOBJECT *obj;

    DVAAID dvaaid;    
    ITX_DVACALB_CONF conf;    
    ITX_DVACALB_SHAPE shape;
    ITX_CALBID calbid;
    
    obj = (NFOBJECT*)user_data;

    top = nfui_nfobject_get_top(obj);    
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
    calbid = _get_focus_calibration_idx(dvaaid);
    if (calbid == -1) return -1;
    
    dvaa_itx_detector_get_calb_conf(dvaaid, calbid, &conf);
    conf.height = component_data->calibration.icon_height;
    dvaa_itx_detector_set_calb_conf(dvaaid, calbid, &conf);

    dvaa_itx_detector_get_calb_shape(dvaaid, calbid, &shape);
    g_sprintf(shape.value, "%d", (int)component_data->calibration.icon_height);
        
    dvaa_itx_detector_set_calb_shape(dvaaid, calbid, &shape);    
    
	return 0;
}

static gint _init_calibration_component_action(NFOBJECT *top)
{
    DVA_COMPONENT_ACTION_T *component_action;

    component_action = (DVA_COMPONENT_ACTION_T*)nfui_nfobject_get_data(top, DVA_COMPONENT_ACTION);

    component_action->calibration_add = _calibration_adrz_component_add;
    component_action->calibration_delete = _calibration_adrz_component_delete;
    component_action->calibration_reset = _calibration_adrz_component_reset;
    component_action->calibration_zoom = _calibration_adrz_component_zoom;
    component_action->calibration_pause = _calibration_adrz_component_pause_video; 
    component_action->calibration_height= _calibration_adrz_component_icon_height; 

    return 0;
}



////////////////////////////////////////////////////////////
//
// handler
//

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
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_SUB_GROUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, off_x, off_y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
    else if (evt->type == GDK_DELETE)
    {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_SUB_GROUP_BG, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_calibration_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == INFY_DVAA_ITX_PRESS_CALB_ID)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;        
        DVAAID dvaaid;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        _update_calb_adrz_component_data(dvaaid, component_data);
        _expose_calb_adrz_component_data();
    }
    else if (evt->type == INFY_DVAA_ITX_PRESS_NONE_ID)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;
        DVAAID dvaaid;
                
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        _update_calb_adrz_component_data(dvaaid, component_data);
        _expose_calb_adrz_component_data();
    }    
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_PRESS_CALB_ID);
        uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_PRESS_NONE_ID);        
    }

    return FALSE;
}

static gboolean post_rule_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;        
        DVAAID dvaaid;
        
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        dvaa_itx_remove_highlight(dvaaid);

        component_data->disable_event = 1;
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);

        _dvabx_rcol_hide_page(0);
        _dvabx_engine_show_page(1);
        nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean post_calibrationadrz_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;        
        DVAAID dvaaid;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);
        
        _cancel_calb_data(dvaaid);
        _update_calb_adrz_component_data(dvaaid, component_data);
        _expose_calb_adrz_component_data();
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
    }    
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }    

    return FALSE;
}

static gboolean post_calibration_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;        
        DVAAID dvaaid;
        
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

        component_data->calibration.pause_video = 0;
        vw_dvabx_calibration_adrz_component_sync_data(g_caliadrz_fixed);
        component_data->disable_event = 1;
        component_data->preview.calb_onoff = 0;
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);

        _dvabx_rcol_hide_page(0);
        _dvabx_engine_show_page(1);
        nfui_make_key_hierarchy(g_curwnd);        
    }

    return FALSE;
}

static gboolean post_calibrationadrz_estimatebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        DVAAID dvaaid;
        gint cnt;

        ivca_calib_t vca_calb;
    
        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

        dvaaid = dvaa_get_dvaaid(component_data->preview.ch);    
        cnt = _get_calibration_cnt(dvaaid);
    
        if (cnt < 5) 
        {
            nftool_mbox((NFWINDOW*)top, "NOTICE", "You have to set at least 5 icons to move the next stage.", NFTOOL_MB_OK);
            return FALSE;
        }

        memset(&vca_calb, 0x00, sizeof(ivca_calib_t));
        _get_targetlist_from_calb_shape(dvaaid, &vca_calb);
        _get_targetlist_from_calb_conf(dvaaid, &vca_calb);
        _get_targetlist_from_calb_result(dvaaid, &vca_calb);
        vca_calb.ntargets = _get_calibration_cnt(dvaaid);

        if (vw_vca_cal_estimate(top, &vca_calb, component_data->preview.ch) == -1) 
        {
            nftool_mbox((NFWINDOW*)obj, "ERROR", "Some icons are abnormal. Please adjust the icon size or add more icons.", NFTOOL_MB_OK);
            return FALSE;
        }

        _set_calb_estimate_data(dvaaid, &vca_calb);
        
        _update_calb_result_component_data(dvaaid, component_data);        
        vw_dvabx_calibration_result_component_sync_data(g_caliresult_fixed);
        vw_dvabx_option_component_sync_data(g_option_fixed);
        
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_caliresult_fixed);
        nfui_signal_emit(g_caliresult_fixed, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_SYNC)
    {
    	NFOBJECT *top;
        DVA_COMPONENT_DATA_T *component_data;

        top = nfui_nfobject_get_top(obj);
        component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);
    
        if (component_data->act_capable && component_data->act_license) nfui_nfobject_enable(obj);
        else nfui_nfobject_disable(obj);
    }    
    else if (evt->type == NFEVENT_DVA_COMPONENT_DATA_EXPOSE)
    {
        nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
    }    

    return FALSE;
}

static gboolean post_calibrationresult_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        nfui_nfobject_hide(obj->parent);
        nfui_signal_emit(obj->parent, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_caliadrz_fixed);
        nfui_signal_emit(g_caliadrz_fixed, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_option_prevbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{		
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
		
        _dvabx_rcol_hide_page(0);
        _dvabx_engine_show_page(1);
        nfui_make_key_hierarchy(g_curwnd);
	}

	return FALSE;
}

static gboolean rule_set_page(NFOBJECT *page)
{
	NFOBJECT *fixed;
    NFOBJECT *obj;
	
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);

    _dvabx_rule_page(fixed);

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("PREVIOUS", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, fixed->height-40-10);
	nfui_regi_post_event_callback(obj, post_rule_prevbutton_event_handler);

	return TRUE;
}

static gboolean calibration_set_page(NFOBJECT *page)
{
	NFOBJECT *fixed;
	NFOBJECT *obj;
	gint opt;

    _init_calibration_component_action(nfui_nfobject_get_top(page));
	
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);
	nfui_regi_post_event_callback(fixed, post_calibration_fixed_event_handler);	
    g_caliadrz_fixed = fixed;

    uxm_reg_imsg_event(fixed, INFY_DVAA_ITX_PRESS_CALB_ID);
    uxm_reg_imsg_event(fixed, INFY_DVAA_ITX_PRESS_NONE_ID);
    
    opt = 0;
    vw_dvabx_calibration_adrz_component_open(fixed, opt);

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("PREVIOUS", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, fixed->height-40-10);
    nfui_regi_post_event_callback(obj, post_calibration_prevbutton_event_handler);
	
	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("ESTIMATE", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-245-10, fixed->height-40-10);    
	nfui_regi_post_event_callback(obj, post_calibrationadrz_estimatebutton_event_handler);


// CALIBRATION RESULT FIXED
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_hide(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);
    g_caliresult_fixed = fixed;

    opt = 0;
    vw_dvabx_calibration_result_component_open(fixed, opt);
	
	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("OK", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, fixed->width-245-10, fixed->height-40-15);    
	nfui_regi_post_event_callback(obj, post_calibrationresult_okbutton_event_handler);

    //_dvabx_rcol_load_init_calb_data(0);
    
	return TRUE;
}

static gboolean engine_option_set_page(NFOBJECT *page)
{
	NFOBJECT *fixed;	
    NFOBJECT *obj;	
    guint opt = 0;
    
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);	
    g_option_fixed = fixed;
    
    opt = 0;
    opt |= (1 << OPT_OPTION_SHOW_ENGINE);
    vw_dvabx_option_component_open(fixed, opt);	

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("PREVIOUS", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, fixed->height-40-10);
    nfui_regi_post_event_callback(obj, post_option_prevbutton_event_handler);

	return TRUE;
}

static gboolean osd_option_set_page(NFOBJECT *page)
{
	NFOBJECT *fixed;	
    NFOBJECT *obj;	
    guint opt = 0;
    
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, page->width -20, page->height -20);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(fixed);
	nfui_nffixed_put((NFFIXED*)page, fixed, 10, 10);	
    g_option_fixed = fixed;
    
    opt = 0;
    opt |= (1 << OPT_OPTION_SHOW_OSD);
    if(ivsc.vendor_code == 128) opt |= (1 << OPT_OPTION_SHOW_UNIT);

    vw_dvabx_option_component_open(fixed, opt);	

	obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("PREVIOUS", 245);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, fixed->height-40-10);
    nfui_regi_post_event_callback(obj, post_option_prevbutton_event_handler);

	return TRUE;
}


////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _dvabx_rcol_page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	guint i;
	NFOBJECT *tab;
	NFOBJECT *obj;
	
    g_parent = parent;
	g_curwnd = (NFWINDOW *)nfui_nfobject_get_top(g_parent);
		
	for (i = 0; i < SUBPAGE_CNT; i++) 
	{
		obj = (NFOBJECT*)nfui_nffixed_new();
		nfui_nfobject_set_size(obj, parent->width, parent->height);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(980));
		nfui_nffixed_put((NFFIXED*)parent, obj, 0, 0);
		nfui_regi_pre_event_callback(obj, pre_page_event_cb);
		tab_page[i] = obj;
	}

    rule_set_page(tab_page[RULE_PAGE]);
    calibration_set_page(tab_page[CALIBRATION_PAGE]);
    engine_option_set_page(tab_page[ENGINE_OPTION_PAGE]);
    osd_option_set_page(tab_page[OSD_OPTION_PAGE]);
    
    nfui_nfobject_show(tab_page[RULE_PAGE]);

    return 0;
}

gint _dvabx_rcol_show_page(gint page, gint expose)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    if (page == 0)
    {
        nfui_nfobject_show(tab_page[RULE_PAGE]);
        nfui_nfobject_hide(tab_page[CALIBRATION_PAGE]);
        nfui_nfobject_hide(tab_page[ENGINE_OPTION_PAGE]);
        nfui_nfobject_hide(tab_page[OSD_OPTION_PAGE]);

        component_data->disable_event = 0;
        component_data->preview.calb_onoff = 0;
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);
    }
    else if (page == 1)
    {
        _get_calb_init_data(dvaaid);

        nfui_nfobject_hide(g_caliresult_fixed);
        nfui_nfobject_show(g_caliadrz_fixed);

        nfui_nfobject_hide(tab_page[RULE_PAGE]);
        nfui_nfobject_show(tab_page[CALIBRATION_PAGE]);
        nfui_nfobject_hide(tab_page[ENGINE_OPTION_PAGE]);
        nfui_nfobject_hide(tab_page[OSD_OPTION_PAGE]);

        component_data->disable_event = 0;
        component_data->preview.calb_onoff = 1;
        vw_dvabx_video_component_sync_preview(component_data->video_fixed);
    }

    nfui_nfobject_show(g_parent);
    if (expose) nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint _dvabx_rcol_show_engine_option_page(gint expose)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    nfui_nfobject_hide(tab_page[RULE_PAGE]);
    nfui_nfobject_hide(tab_page[CALIBRATION_PAGE]);
    nfui_nfobject_show(tab_page[ENGINE_OPTION_PAGE]);
    nfui_nfobject_hide(tab_page[OSD_OPTION_PAGE]);

    component_data->preview.calb_onoff = 0;
    vw_dvabx_video_component_sync_preview(component_data->video_fixed);

    nfui_nfobject_show(g_parent);
    if (expose) nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint _dvabx_rcol_show_osd_option_page(gint expose)
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;
    DVAAID dvaaid;

    top = nfui_nfobject_get_top(g_parent);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);

    nfui_nfobject_hide(tab_page[RULE_PAGE]);
    nfui_nfobject_hide(tab_page[CALIBRATION_PAGE]);
    nfui_nfobject_hide(tab_page[ENGINE_OPTION_PAGE]);
    nfui_nfobject_show(tab_page[OSD_OPTION_PAGE]);

    component_data->preview.calb_onoff = 0;
    vw_dvabx_video_component_sync_preview(component_data->video_fixed);

    nfui_nfobject_show(g_parent);
    if (expose) nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint _dvabx_rcol_hide_page(gint expose)
{
    nfui_nfobject_hide(g_parent);
    if (expose) nfui_signal_emit(g_parent, GDK_EXPOSE, TRUE);
    return 0;
}

gint _dvabx_rcol_load_init_calb_data(gint ch)
{
    DVAAID dvaaid;

    dvaaid = dvaa_get_dvaaid(ch);    
    _get_calb_init_data(dvaaid);
    return 0;

}

gint _dvabx_calb_data_cancel(DVAAID dvaaid, DVA_COMPONENT_DATA_T *component_data)
{
    _cancel_calb_data(dvaaid);
    _update_calb_adrz_component_data(dvaaid, component_data);
    _expose_calb_adrz_component_data();

    return 0;
}

gint _dvabx_rcol_estimation_calb()
{
	NFOBJECT *top;
    DVA_COMPONENT_DATA_T *component_data;

    DVAAID dvaaid;
    gint cnt;

    ITX_DVACALB_RESULT calb_result;
    ivca_calib_t vca_calb;

    top = nfui_nfobject_get_top(g_caliresult_fixed);
    component_data = (DVA_COMPONENT_DATA_T*)nfui_nfobject_get_data((NFOBJECT*)top, DVA_COMPONENT_DATA);

    dvaaid = dvaa_get_dvaaid(component_data->preview.ch);    
    cnt = _get_calibration_cnt(dvaaid);

    memset(&vca_calb, 0x00, sizeof(ivca_calib_t));

    if (cnt >= 5) 
    {
        _get_targetlist_from_calb_shape(dvaaid, &vca_calb);
        _get_targetlist_from_calb_conf(dvaaid, &vca_calb);
        _get_targetlist_from_calb_result(dvaaid, &vca_calb);
        vca_calb.ntargets = _get_calibration_cnt(dvaaid);

        vw_vca_cal_estimate(top, &vca_calb, component_data->preview.ch);
        _set_calb_estimate_data(dvaaid, &vca_calb);
    }
    else
    {
        _set_calb_estimate_data(dvaaid, &vca_calb);
    }
    
    _update_calb_result_component_data(dvaaid, component_data);        
    vw_dvabx_calibration_result_component_sync_data(g_caliresult_fixed);
    vw_dvabx_option_component_sync_data(g_option_fixed);

    return 0;
}

