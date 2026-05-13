/*
 * vw_vca_rev_calibration_zoom.c
 *
 * Written by JungKyu. <parangi22@itxsecurity.com>
 * Copyright (c) ITX security, June 16, 2014
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
#include "viewers/objects/nfcombobox.h"
#include "objects/nfspinbutton.h"

#include "vaa.h"
#include "vaa_itx.h"

#include "vw_dit_vca.h"
#include "vw_vca_rev_calibration_zoom_setup.h"
#include "vw_vca_rev_component.h"


#define WIN_WIDTH                   (460)
#define WIN_HEIGHT                  (380)



////////////////////////////////////////////////////////////
//
// private data types
//





////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFWINDOW *g_parent = 0;

static gint g_cur_ch = 0;

static ITX_VACALB_SHAPE g_calb_shape[32];
static ITX_VACALB_CONF g_calb_conf[32];
static ITX_VACALB_RESULT g_calb_result;

static NFOBJECT *g_add_btn;
static NFOBJECT *g_del_btn;
static NFOBJECT *g_reset_btn;
static NFOBJECT *g_height_spin;


////////////////////////////////////////////////////////////
//
// private interfaces 
//

static _get_calb_init_data(VAAID vaaid)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_shape(vaaid, i, &g_calb_shape[i]);
        vaa_itx_get_calb_conf(vaaid, i, &g_calb_conf[i]);
    }

    vaa_itx_get_calb_result(vaaid, &g_calb_result);

    return 0;
}

static _cancel_calb_data(VAAID vaaid)
{
    gint i;

    for (i = 0; i < 32; i++)
    {
        vaa_itx_set_calb_shape(vaaid, i, &g_calb_shape[i]);
        vaa_itx_set_calb_conf(vaaid, i, &g_calb_conf[i]);
    }

    vaa_itx_set_calb_result(vaaid, &g_calb_result);   

    return 0;
}

static gint _get_new_calibration_idx(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;
    
    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if (conf.use_calb == 0) return i;
    }

    return -1;
}

static gint _get_calibration_cnt(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i, cnt = 0;
   
    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if (conf.use_calb == 1) cnt++;
    }

    return cnt;
}

static gint _get_focus_calibration_idx(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;
    
    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if ((conf.use_calb == 1) && (conf.focus == 1)) return i;
    }

    return -1;
}

static gint _set_icon_height(VAAID vaaid, gint calbid, gint icon_height)
{
    ITX_VACALB_CONF conf;    
    ITX_VACALB_SHAPE shape;
    
    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.height = icon_height;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);

    vaa_itx_get_calb_shape(vaaid, calbid, &shape);
    g_sprintf(shape.value, "%d", icon_height);
    vaa_itx_set_calb_shape(vaaid, calbid, &shape);
    return 0;
}

static gint _set_focus_calibration(VAAID vaaid, gint calbid)
{
    ITX_VACALB_CONF conf;    

    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.focus = 1;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);

    return 0;
}

static gint _unset_focus_calibration(VAAID vaaid, gint calbid)
{
    ITX_VACALB_CONF conf;    

    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.focus = 0;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);
    
    return 0;
}

static gint _unset_focus_all_calibration(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;

    for (i = 0; i < 32; i++)
    {
        vaa_itx_get_calb_conf(vaaid, i, &conf);
        if (conf.use_calb) _unset_focus_calibration(vaaid, i);
    }

    return 0;
}

static gint _delete_calibration(VAAID vaaid, gint calbid)
{
    ITX_VACALB_CONF conf;    

    vaa_itx_get_calb_conf(vaaid, calbid, &conf);
    conf.use_calb = 0;
    vaa_itx_set_calb_conf(vaaid, calbid, &conf);
    return 0;
}


static gint _delete_all_calibration(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    gint i;

    for (i = 0; i < 32; i++)
    {
        _delete_calibration(vaaid, i);
    }
    return 0;
}

static gint _update_calb_object_data(VAAID vaaid)
{
    ITX_VACALB_CONF conf;    
    ITX_CALBID calb_idx;
    gint spin_index;

    if (_get_calibration_cnt(vaaid) < 32) nfui_nfobject_enable(g_add_btn);
    else nfui_nfobject_disable(g_add_btn);

    if (_get_calibration_cnt(vaaid) > 0) nfui_nfobject_enable(g_reset_btn);
    else nfui_nfobject_disable(g_reset_btn);

    calb_idx = _get_focus_calibration_idx(vaaid);
    
    if (calb_idx != -1)
    {
        nfui_nfobject_enable(g_del_btn);
    
        vaa_itx_get_calb_conf(vaaid, calb_idx, &conf);

        gdouble index ;
        if(get_unit_change_yard())
        {
            index = ifn_unit_change((gdouble) conf.height, 1);
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_height_spin, index-50);
        }
        else 
            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)g_height_spin, conf.height-50);
        nfui_nfobject_enable(g_height_spin);
    }
    else 
    {
        nfui_nfobject_disable(g_del_btn);
        nfui_nfobject_disable(g_height_spin);        
    }

    return 0;
}


////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == NFOUTEVT_BUTTON_PRESS || evt->type == NFOUTEVT_BUTTON_RELEASE || \ 
	    evt->type == NFOUTEVT_MOTION_NOTIFY || evt->type == NFOUTEVT_SCROLL) 
	{
        if (evt->type == NFOUTEVT_BUTTON_PRESS) evt->type = GDK_BUTTON_PRESS;
        else if (evt->type == NFOUTEVT_BUTTON_RELEASE) evt->type = GDK_BUTTON_RELEASE;
        else if (evt->type == NFOUTEVT_MOTION_NOTIFY) evt->type = GDK_MOTION_NOTIFY;        
        else if (evt->type == NFOUTEVT_SCROLL) evt->type = GDK_SCROLL;
        
        nfui_send_event(g_parent, evt, TRUE);
	}
	else if(evt->type == GDK_DELETE) 
	{
//        _vca_rev_calibration_zoom_close();
		g_curwnd = 0;
	}

	return FALSE;
}

static gboolean post_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
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
    else if (evt->type == INFY_VAA_ITX_PRESS_CALB_ID)
    {
        VAAID vaaid;    
    
        vaaid = vaa_get_vaaid(g_cur_ch);
        _update_calb_object_data(vaaid);

        nfui_signal_emit(g_add_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_del_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_reset_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_height_spin, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == INFY_VAA_ITX_PRESS_NONE_ID)
    {
        VAAID vaaid;    
    
        vaaid = vaa_get_vaaid(g_cur_ch);
        _update_calb_object_data(vaaid);

        nfui_signal_emit(g_add_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_del_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_reset_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_height_spin, GDK_EXPOSE, TRUE);
    }
    else if (evt->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_VAA_ITX_PRESS_CALB_ID);
        uxm_unreg_imsg_event(obj, INFY_VAA_ITX_PRESS_NONE_ID);
    
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
    }

	return FALSE;
}

static gboolean post_icon_height_spin_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_SPINBUTTON_CHANGED)
    {       
        gdouble tmp;
        gint value;
        VAAID vaaid;    
        gint calb_idx;

        tmp = atof(nfui_spin_button_get_text((NFSPINBUTTON*)obj));
        value = (tmp*100);
               
        if(get_unit_change_yard())
        {
            gdouble index;
            index = ifn_unit_change(value,0);
            value = (index);
        }
        else
        {
            if(tmp > 9.99){
                nfui_spin_button_set_index((NFSPINBUTTON *)obj, 0);
                value = 50;
            }
        }
        
        vaaid = vaa_get_vaaid(g_cur_ch);
        calb_idx = _get_focus_calibration_idx(vaaid);
        if (calb_idx == -1) return FALSE;

        _set_icon_height(vaaid, calb_idx, value);
    }
    else if(evt->type == GDK_2BUTTON_PRESS)
    {
        guint x, y;
        NFOBJECT *top;
    	gchar buf[5];
    	gchar *strTemp;
        gint tmp, value;
        gint numTemp;
        VAAID vaaid;    
        gint calb_idx;
        
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
        
        top = nfui_nfobject_get_top(obj);
        
        nfui_nfobject_get_window_pos(obj, &x, &y);
        x += ((GdkEventButton*)evt)->x;
        y += ((GdkEventButton*)evt)->y;

        tmp = nfui_spin_button_get_index((NFSPINBUTTON*)obj);

		memset(buf, 0x00, sizeof(buf));	
        gcvt((50 + tmp)*0.01, 3, buf);
        
		strTemp = Real_NumberKey_Str_Open(top, buf, x, y, "9.99");  

        if(strTemp)
        {
            numTemp = (atof(strTemp)*100.00 + 0.5);
            
            if ((numTemp < 50) || (numTemp > 999)) return FALSE;

            nfui_spin_button_set_index_no_expose((NFSPINBUTTON*)obj, numTemp-50);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            if(get_unit_change_yard())
            {
                gdouble index;
                index = ifn_unit_change(numTemp,0);
                numTemp = (index);
            }

            vaaid = vaa_get_vaaid(g_cur_ch);
            calb_idx = _get_focus_calibration_idx(vaaid);
            if (calb_idx == -1) return FALSE;

            _set_icon_height(vaaid, calb_idx, numTemp);  
            ifree(strTemp);
        }
    }    
	return FALSE;
}

static gboolean post_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{        
        VAAID vaaid;    
        gint calb_idx;
    
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_vaaid(g_cur_ch);
        calb_idx = _get_new_calibration_idx(vaaid);

        if (calb_idx == -1) {
            nftool_mbox(g_curwnd, "NOTICE", "You can set up to 32 icons.", NFTOOL_MB_OK);
            return FALSE;
        }

        _unset_focus_all_calibration(vaaid);
        vaa_itx_add_calb_default_template(vaaid, calb_idx);
        _set_focus_calibration(vaaid, calb_idx);
        _update_calb_object_data(vaaid);

        nfui_signal_emit(g_add_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_del_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_reset_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_height_spin, GDK_EXPOSE, TRUE);        
	}

	return FALSE;
}

static gboolean post_delbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{
        VAAID vaaid;    
        gint calb_idx;
	
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_vaaid(g_cur_ch);
        calb_idx = _get_focus_calibration_idx(vaaid);
        if (calb_idx == -1) return FALSE;

        _delete_calibration(vaaid, calb_idx);
        _update_calb_object_data(vaaid);

        nfui_signal_emit(g_add_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_del_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_reset_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_height_spin, GDK_EXPOSE, TRUE);                
	}

	return FALSE;
}

static gboolean post_resetbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{        
        VAAID vaaid;
        ITX_VACALB_RESULT res;
        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_vaaid(g_cur_ch);
        _delete_all_calibration(vaaid);
        _update_calb_object_data(vaaid);

        nfui_signal_emit(g_add_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_del_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_reset_btn, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_height_spin, GDK_EXPOSE, TRUE);
        
        vaa_itx_get_calb_result(vaaid, &res);
        res.cam_height = 0;
        res.cam_tilt = 0;
        res.focal = 0;
        res.paramvalid = 0;    
        vaa_itx_set_calb_result(vaaid, &res);
	}

	return FALSE;
}

static gboolean post_pausebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "PAUSE VIDEO") == 0)
        {
            nf_live_stop();       
            nf_live_set_freeze(1);

            nfui_nfbutton_set_text((NFBUTTON*)obj, "PLAY VIDEO");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    			
        }
        else if (strcmp(nfui_nfbutton_get_text((NFBUTTON*)obj), "PLAY VIDEO") == 0)
        {
            _vca_rev_calibration_zoom_restart();
            nf_live_set_freeze(0);			

            nfui_nfbutton_set_text((NFBUTTON*)obj, "PAUSE VIDEO");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    
        }
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{
        NFOBJECT *topwin;
        VAAID vaaid;        
        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        vaaid = vaa_get_vaaid(g_cur_ch);    
        _cancel_calb_data(vaaid);

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(topwin);
        nfui_nfobject_destroy(topwin);
        _vca_rev_calibration_zoom_close();
	}

	return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE) 
	{
        NFOBJECT *topwin;
        
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_hide(topwin);
        nfui_nfobject_destroy(topwin);
        _vca_rev_calibration_zoom_close();
	}

	return FALSE;
}


////////////////////////////////////////////////////////////
//
// protected interfaces 
//







////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_vca_rev_calibration_zoom_setup_open(NFOBJECT *parent, gint ch)
{
    NFOBJECT *win;
    NFOBJECT *obj;
    NFOBJECT *fixed;

    gint pos_x, pos_y, size_w, size_h;
    gint label_h;
    gchar strBuf[4096];
    gchar lfBuf[4096];

    VAAID vaaid;

    g_parent = parent;
    g_cur_ch = ch;


	win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, WIN_WIDTH, WIN_HEIGHT);
	nfui_nfwindow_set_title((NFWINDOW*)win, "VCA ZOOM SETUP");
	nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_RELEASE, TRUE);	
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_MOTION_NOTIFY, TRUE);	
	nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_SCROLL, TRUE);		
//    nfui_nfwindow_set_modal((NFWINDOW*)win, FALSE);
    nfui_nfwindow_set_moving_area_size((NFWINDOW*)win, WIN_HEIGHT);
    nfui_nfwindow_set_moving_limit((NFWINDOW*)win, TRUE);
	nfui_regi_post_event_callback(win, post_main_win_event_handler);
	g_curwnd = win;

	/* fixed */
	fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WIN_WIDTH, WIN_HEIGHT);
	nfui_regi_post_event_callback(fixed, post_fixed_event_cb);
	nfui_nfobject_show(fixed);

    uxm_reg_imsg_event(fixed, INFY_VAA_ITX_PRESS_CALB_ID);
    uxm_reg_imsg_event(fixed, INFY_VAA_ITX_PRESS_NONE_ID);

    pos_x = 10;
    pos_y = 10;
    
    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "<%s>", lookup_string("NOTE"));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, WIN_WIDTH-20, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);   
    
    pos_y += 50;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "1. %s", lookup_string("A minimum of 5 icons should be set."));

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(strBuf, WIN_WIDTH-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));
    label_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, WIN_WIDTH-20, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "2. %s", lookup_string("All objects must be on the same plane."));
    label_h = nfutil_string_height(0, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, 0);
    
    pos_y += 30;

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(strBuf, WIN_WIDTH-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, WIN_WIDTH-20, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

    pos_y += 30;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "3. %s", lookup_string("Icons must be evenly distributed on the screen."));

    memset(lfBuf, 0x00, sizeof(lfBuf));
    nfutil_get_line_feed_string(strBuf, WIN_WIDTH-20, nffont_get_pango_font(NFFONT_SMALL_SEMI), lfBuf, sizeof(lfBuf));

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(lfBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, WIN_WIDTH-20, 50);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT_UP, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    
    pos_y += 50 + 10;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("ADD", 140);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
    nfui_regi_post_event_callback(obj, post_addbutton_event_handler);
    g_add_btn = obj;
    
    pos_x += 145;
    
    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("DELETE", 140);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
    nfui_regi_post_event_callback(obj, post_delbutton_event_handler);	
    g_del_btn = obj;
    
    pos_x += 145;
    
    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("RESET", 140);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
    nfui_regi_post_event_callback(obj, post_resetbutton_event_handler);		
    g_reset_btn = obj;
    
    pos_x = 10;
    pos_y += 45;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type2("PAUSE VIDEO", 160);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
    nfui_regi_post_event_callback(obj, post_pausebutton_event_handler);		
    
    pos_y += 50;

    memset(strBuf, 0x00, sizeof(strBuf));
    g_sprintf(strBuf, "%s : ", lookup_string("Selected icon's estimated height"));
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 315, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);    
    
    pos_x += 315;
    
    obj = nfui_spinbutton_new_value_with_range(1.75, 0.50, 9.99, 0.01);
    if(get_unit_change_yard())obj= nfui_spinbutton_new_value_with_range(5.74, 0.50, 32.77, 0.01);
    nfui_spinbutton_set_skin_type((NFSPINBUTTON*)obj, NFSPINBUTTON_TYPE_POPUP_SMALL_1);
    nfui_nfobject_set_size(obj, 85, 30);
    nfui_nfobject_show(obj);                
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y); 
    nfui_regi_post_event_callback(obj, post_icon_height_spin_event_handler);  
    g_height_spin = obj;
    
    pos_x += 90;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("m", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(205));
    nfui_nfobject_modify_bg((NFOBJECT*)obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(obj, 10, 30);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER_DOWN, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
    if(get_unit_change_yard())  nfui_nflabel_set_text((NFLABEL*)obj,"ft");
    else              nfui_nflabel_set_text((NFLABEL *) obj, "m");     
    
    pos_x = 10;
    pos_y += 50;

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("CANCEL", 210);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 10, pos_y); 
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = (NFOBJECT*)nftool_normal_button_create_popup_type1("OK", 210);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, WIN_WIDTH-10-210, pos_y); 
    nfui_regi_post_event_callback(obj, post_okbutton_event_handler);

// ADD MAIN FIXED	
	nfui_nfwindow_add((NFWINDOW*)win, fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);

    vaaid = vaa_get_vaaid(ch);    
    _get_calb_init_data(vaaid);
    _update_calb_object_data(vaaid);    

    return 0;
}

