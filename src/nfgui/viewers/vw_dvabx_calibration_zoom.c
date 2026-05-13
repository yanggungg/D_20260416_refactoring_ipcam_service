/*
 * vw_dvabx_calibration_zoom.c
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
#include "viewers/objects/nfcombobox.h"

#include "dvaa.h"
#include "dvaa_itx.h"

#include "vw_dit_dva.h"
#include "vw_dvabx_component.h"
#include "vw_dvabx_calibration_zoom.h"
#include "vw_dvabx_calibration_zoom_setup.h"


#define WIN_WIDTH                   (1920)
#define WIN_HEIGHT                  (1080)

#define MAX_STEP                    (10)
#define MIN_STEP                    (1)
#define INIT_STEP                   (10)

#define AREA_SIZE_W                 (1918)
#define AREA_SIZE_H                 (1080)

#define AREA_W_SHARE                (AREA_SIZE_W/MAX_STEP)
#define AREA_H_SHARE                (AREA_SIZE_H/MAX_STEP)

#define AREA_X_STEP                 (AREA_W_SHARE/2)
#define AREA_Y_STEP                 (AREA_H_SHARE/2)
#define AREA_W_STEP                 (AREA_W_SHARE)
#define AREA_H_STEP                 (AREA_H_SHARE)

#define VIDEO_W_SHARE               (DISPLAY_ACTIVE_WIDTH/MAX_STEP)
#define VIDEO_H_SHARE               (DISPLAY_ACTIVE_HEIGHT/MAX_STEP)

#define MOVE_W_STEP                 (VIDEO_W_SHARE)
#define MOVE_H_STEP                 (VIDEO_H_SHARE)

#define MOVE_POS_X(step)            ((DISPLAY_ACTIVE_WIDTH-(MOVE_W_STEP*step))/2)
#define MOVE_POS_Y(step)            ((DISPLAY_ACTIVE_HEIGHT-(MOVE_H_STEP*step))/2)
#define MOVE_SIZE_W(step)           (MOVE_W_STEP*step)
#define MOVE_SIZE_H(step)           (MOVE_H_STEP*step)



////////////////////////////////////////////////////////////
//
// private data types
//

typedef enum {
    CMD_ZOOMIN = 0,
    CMD_ZOOMOUT,
    CMD_MOVE
}ZoomCmd;

typedef struct _AREA_DATA_T {
    gint x;
    gint y;
    gint w;
    gint h;
    gint step;
}AREA_DATA_T;

typedef struct _DVA_MEVENT_CALLBACK
{
	DVA_MEVENT_CB_FUNC 	cb_func;
	gpointer 			user_data;
} DVA_MEVENT_CALLBACK;

typedef struct _DVA_VMAP_CALLBACK
{
	DVA_VMAP_CB_FUNC 	cb_func;
	gpointer 			user_data;
} DVA_VMAP_CALLBACK;




////////////////////////////////////////////////////////////
//
// private variable
//

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_fixed = 0;

static gint g_cur_step = INIT_STEP;
static GdkPoint g_evt_point;
static guint g_draw_tid = 0;

static gint g_force_update = 0;
static gint g_vmap_update = 0;
static DVA_CLON g_dva_clon;

static DVA_MEVENT_CALLBACK g_mevent_cb[DVA_MEVENT_MAX] = {0, };
static DVA_VMAP_CALLBACK g_vmap_cb = {0, };
static gint g_select_icon = 0;

static gint g_cur_ch = 0;


////////////////////////////////////////////////////////////
//
// private interfaces 
//

static gint _proc_zoom_in()
{
    DVA_VMAP_INFO vaae_vmap;
    
    gint pos_sx = 0, pos_sy = 0;
    gint size_w = 0, size_h = 0;

    pos_sx = nf_live_zoom_get_pos_sx(); 
    pos_sy = nf_live_zoom_get_pos_sy();
    size_w = nf_live_zoom_get_pos_dx(); 
    size_h = nf_live_zoom_get_pos_dy();

    if (g_cur_step == MIN_STEP) return -1;
    if (size_w <= AREA_W_SHARE && size_h <= AREA_H_SHARE) return -1;

    pos_sx += AREA_X_STEP;
    pos_sy += AREA_Y_STEP;
    size_w -= MOVE_W_STEP;
    size_h -= MOVE_H_STEP;

    if (size_w <= AREA_W_SHARE && size_h <= AREA_H_SHARE) return -1;
    
    if (!nf_live_zoom_move(pos_sx, pos_sy, size_w, size_h)) return -1;

    vaae_vmap.use = 1;
    vaae_vmap.x = pos_sx;
    vaae_vmap.y = pos_sy;
    vaae_vmap.w = size_w;
    vaae_vmap.h = size_h;    
    g_vmap_cb.cb_func(&vaae_vmap, g_vmap_cb.user_data);

    g_cur_step--;
    return 0;
}

static gint _proc_zoom_out()
{
    DVA_VMAP_INFO vaae_vmap;
    
    gint pos_sx = 0, pos_sy = 0;
    gint size_w = 0, size_h = 0;
    gint gap;

    pos_sx = nf_live_zoom_get_pos_sx(); 
    pos_sy = nf_live_zoom_get_pos_sy();
    size_w = nf_live_zoom_get_pos_dx(); 
    size_h = nf_live_zoom_get_pos_dy();

    if (g_cur_step == MAX_STEP) return -1;
    if (pos_sx <= 0 && pos_sy <= 0 && size_w >= AREA_SIZE_W && size_h >= AREA_SIZE_H) return -1;

    if (g_cur_step+1 < MAX_STEP) 
    {
        pos_sx -= AREA_X_STEP;
        pos_sy -= AREA_Y_STEP;
        size_w += AREA_W_STEP;
        size_h += AREA_H_STEP;
    }
    else if (g_cur_step+1 == MAX_STEP) 
    {
        pos_sx = 0;
        pos_sy = 0;
        size_w = AREA_SIZE_W;
        size_h = AREA_SIZE_H;
    }

    if (pos_sx <= 0) pos_sx = 0;
    if (pos_sy <= 0) pos_sy = 0;

    if (pos_sx + size_w > AREA_SIZE_W) 
    {
        gap = pos_sx + size_w - AREA_SIZE_W;
        pos_sx -= gap;
    }

    if (pos_sy + size_h > AREA_SIZE_H) 
    {
        gap = pos_sy + size_h - AREA_SIZE_H; 
        pos_sy -= gap;
    }

    if (pos_sx <= 0 && pos_sy <= 0 && size_w >= AREA_SIZE_W && size_h >= AREA_SIZE_H) return -1;

    if (!nf_live_zoom_move(pos_sx, pos_sy, size_w, size_h)) return -1;

    vaae_vmap.use = 1;
    vaae_vmap.x = pos_sx;
    vaae_vmap.y = pos_sy;
    vaae_vmap.w = size_w;
    vaae_vmap.h = size_h;    
    g_vmap_cb.cb_func(&vaae_vmap, g_vmap_cb.user_data);

    g_cur_step++;
    return 0;
}

static gint _proc_zoom_drag(gint mouse_x, gint mouse_y)
{
    DVA_VMAP_INFO vaae_vmap;

    gint pos_sx = 0, pos_sy = 0;
    gint size_w = 0, size_h = 0;

    pos_sx = nf_live_zoom_get_pos_sx(); 
    pos_sy = nf_live_zoom_get_pos_sy();
    size_w = nf_live_zoom_get_pos_dx(); 
    size_h = nf_live_zoom_get_pos_dy();

    if (g_cur_step == MAX_STEP) return -1;
    if (size_w == AREA_SIZE_W && size_h == AREA_SIZE_H) return -1;

    pos_sx += (g_evt_point.x - mouse_x);
    pos_sy += (g_evt_point.y - mouse_y);

    if (pos_sx <= 0) return -1;
    if (pos_sy <= 0) return -1;
    if (pos_sx+size_w > AREA_SIZE_W) return -1;
    if (pos_sy+size_h > AREA_SIZE_H) return -1;
    
    if (!nf_live_zoom_move(pos_sx, pos_sy, size_w, size_h)) return -1;

    vaae_vmap.use = 1;
    vaae_vmap.x = pos_sx;
    vaae_vmap.y = pos_sy;
    vaae_vmap.w = size_w;
    vaae_vmap.h = size_h;    
    g_vmap_cb.cb_func(&vaae_vmap, g_vmap_cb.user_data);
    
    return 0;
}


static gboolean _draw_dva_calb(gpointer data)
{      
	DVA_DP dp;
	DVA_CLON preClon, postClon;
	DVA_CLON tmpClon;	    	
    DVA_VMAP display_vmap;

    if (!nfui_nfobject_is_shown(g_video_fixed)) return TRUE;

	dp.drawable = nfui_nfobject_get_window(g_video_fixed);
	if (!dp.drawable) return TRUE;

	preClon.dic_cnt = g_dva_clon.dic_cnt;
	preClon.pdics = g_dva_clon.pdics;
	postClon.dic_cnt = 0;
	postClon.pdics = 0;	
	vw_dit_display_get_dva_diclist(&postClon);	

	if ((vw_dit_display_compare_dva_diclist(preClon, postClon) == 1) && (g_force_update == 0) && (g_vmap_update == 0)) 
	{
	    vw_dit_display_free_dva_diclist(preClon);
	    g_dva_clon.dic_cnt = postClon.dic_cnt;
	    g_dva_clon.pdics = postClon.pdics;    	    
		return TRUE;
	}

	dp.gc = nfui_nfobject_get_gc(g_video_fixed);
	nfui_nfobject_get_offset(g_video_fixed, &dp.plt_area.x, &dp.plt_area.y);
	nfui_nfobject_get_size(g_video_fixed, &dp.plt_area.width, &dp.plt_area.height);

    if (g_vmap_update)
    {
        tmpClon.dic_cnt = 0;    
        tmpClon.pdics = 0;
        
    	vw_dit_display_dva_erase(&dp, preClon, tmpClon);

        display_vmap.use = 1;
        display_vmap.x = nf_live_zoom_get_pos_sx();
        display_vmap.y = nf_live_zoom_get_pos_sy();
        display_vmap.w = nf_live_zoom_get_pos_dx();
        display_vmap.h = nf_live_zoom_get_pos_dy();
        
        vw_dit_display_set_dva_vmap(&display_vmap);
        vw_dit_display_free_dva_diclist(preClon);
    }
    else
    {
    	vw_dit_display_dva_erase(&dp, preClon, postClon);
        vw_dit_display_free_dva_diclist(preClon);
    }  

	vw_dit_display_dva_draw(&dp, postClon);

    g_dva_clon.dic_cnt = postClon.dic_cnt;
    g_dva_clon.pdics = postClon.pdics;    	
    g_force_update = 0;
    g_vmap_update = 0;

	nfui_nfobject_gc_unref(dp.gc);

	return TRUE;
}

static gboolean _init_dva_rule(gpointer data)
{
    if (!g_draw_tid) g_draw_tid = g_timeout_add(100, _draw_dva_calb, 0);
    return FALSE;
}




////////////////////////////////////////////////////////////
//
// handler
//

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) 
	{
        case GDK_DELETE:
        {
            DVA_VMAP display_vmap;
            DVA_VMAP_INFO vaae_vmap;
        
            display_vmap.use = 0;           
            vw_dit_display_set_dva_vmap(&display_vmap);                   
        	vw_dit_display_set_dva_ditid(0);        
        	vw_dit_display_free_dva_diclist(g_dva_clon);
        	g_dva_clon.dic_cnt = 0;
        	g_dva_clon.pdics = 0;        	

            vaae_vmap.use = 0;
            g_vmap_cb.cb_func(&vaae_vmap, g_vmap_cb.user_data);
        
    		if (g_draw_tid) 
    		{
    			g_source_remove(g_draw_tid);
    			g_draw_tid = 0;
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

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    DVA_SCREEN_INFO scr_info;
    DVA_MEVENT_PT mevt_pt;

    switch (evt->type) 
    {
        case GDK_EXPOSE:
        {
            g_force_update = 1;
        }
        break;
        
        case GDK_BUTTON_PRESS:
        {
    		GdkEventButton *bevt; 

    		bevt = (GdkEventButton*)evt;        
            if (bevt->button != MOUSE_LEFT_BUTTON) return FALSE;

    		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
    		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
    		mevt_pt.x = (gint)bevt->x;
    		mevt_pt.y = (gint)bevt->y;	

            g_mevent_cb[DVA_MEVENT_LEFT_PRESS].cb_func(&scr_info, &mevt_pt, g_mevent_cb[DVA_MEVENT_LEFT_PRESS].user_data);

            g_evt_point.x = (gint)bevt->x;
            g_evt_point.y = (gint)bevt->y;
        }
        break;
        
        case GDK_SCROLL:
        {
            GdkEventScroll *sevt;
            gint i;

#if 0
            sevt = (GdkEventScroll*)evt;

            if (sevt->direction == GDK_SCROLL_UP) 
            {           
                if (_proc_zoom_in() == 0) 
                {
                    g_vmap_update = 1;
                }
            }
            else if(sevt->direction == GDK_SCROLL_DOWN) 
            {    			           
                if (_proc_zoom_out() == 0) 
                {
                    g_vmap_update = 1;
                }
            }
#endif            
        }
        break;
        
        case GDK_MOTION_NOTIFY:
        {
            GdkEventMotion *mevt;
            gint i;
            
            mevt = (GdkEventMotion*)evt;            
            if (!(mevt->state & GDK_BUTTON1_MASK)) return FALSE;            

            if (g_select_icon == 1)
            {
        		nfui_nfobject_get_offset(obj, &scr_info.x, &scr_info.y);
        		nfui_nfobject_get_size(obj, &scr_info.w, &scr_info.h);
        		mevt_pt.x = (gint)mevt->x;
        		mevt_pt.y = (gint)mevt->y;
            
                g_mevent_cb[DVA_MEVENT_DRAG].cb_func(&scr_info, &mevt_pt, g_mevent_cb[DVA_MEVENT_DRAG].user_data);
            }
            else
            {
                if (_proc_zoom_drag((gint)mevt->x, (gint)mevt->y) == 0)
                {
                    g_vmap_update = 1;            
        		}
            }

            g_evt_point.x = (gint)mevt->x;
            g_evt_point.y = (gint)mevt->y;
        }
        break;

        case GDK_BUTTON_RELEASE:
        {
            g_evt_point.x = 0;
            g_evt_point.y = 0;
        }
        break;

        case INFY_DVAA_ITX_PRESS_CALB_ID:
        {
            g_select_icon = 1;
        }
        break;

        case INFY_DVAA_ITX_PRESS_NONE_ID:
        {
            g_select_icon = 0;
        }
        break;

        case GDK_DELETE:
        {
            uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_PRESS_CALB_ID);
            uxm_unreg_imsg_event(obj, INFY_DVAA_ITX_PRESS_NONE_ID);
        }
        break;

        default:
        break;
    }

    return FALSE;
}



////////////////////////////////////////////////////////////
//
// protected interfaces 
//

gint _dvabx_calibration_zoom_close()
{
    nfui_nfobject_hide((NFOBJECT*)g_curwnd);
    nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
    return 0;
}

gint _dvabx_calibration_zoom_restart()
{
    nf_live_zoom_start_without_pip(g_cur_ch, MOVE_POS_X(INIT_STEP), MOVE_POS_Y(INIT_STEP), 
															MOVE_SIZE_W(INIT_STEP), MOVE_SIZE_H(INIT_STEP),
															FALSE);
    g_vmap_update = 1;
    return 0;
}





////////////////////////////////////////////////////////////
//
// public interfaces
//

gint vw_dvabx_calibration_zoom_open(NFOBJECT *parent, gint ch)
{
    NFOBJECT *win;
    NFOBJECT *fixed;   
    NFOBJECT *obj;
       
    gint pos_x, pos_y;
    gchar strBuf[16];

	DVAAID dvaaid;
	DITID ditid;


    g_cur_ch = ch;

    g_force_update = 0;
    g_vmap_update = 0;

    g_dva_clon.dic_cnt = 0;
    g_dva_clon.pdics = 0;

    g_cur_step = INIT_STEP;    
    g_evt_point.x = 0;
    g_evt_point.y = 0;

    g_select_icon = 0;

    win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, WIN_WIDTH, WIN_HEIGHT);
//    nfui_nfwindow_set_modal(win, FALSE);    
    nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    g_curwnd = win;

    fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(fixed, WIN_WIDTH, WIN_HEIGHT);
	nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));    
    nfui_nfobject_show(fixed);
    nfui_regi_post_event_callback(fixed, post_main_fixed_event_handler);
    g_video_fixed = fixed;

    uxm_reg_imsg_event(fixed, INFY_DVAA_ITX_PRESS_CALB_ID);
    uxm_reg_imsg_event(fixed, INFY_DVAA_ITX_PRESS_NONE_ID);

// ADD MAIN FIXED   
    nfui_nfwindow_add((NFWINDOW*)win, fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);

    nfui_page_open(PGID_VCA_ZOOM, win, nfui_get_last_user());

    vw_dvabx_calibration_zoom_setup_open(win, ch); 
    
    vsm_change_sfc_by_zoom_opened(ch);
	nf_live_set_freeze(0);			
    nf_live_zoom_start_without_pip(ch, MOVE_POS_X(INIT_STEP), MOVE_POS_Y(INIT_STEP), 
																	MOVE_SIZE_W(INIT_STEP), MOVE_SIZE_H(INIT_STEP),
																	TRUE);

	dvaaid = dvaa_get_dvaaid(ch);
	ditid = dvaa_get_ditid(dvaaid);
	vw_dit_display_set_dva_ditid(ditid);

    g_timeout_add(1000, _init_dva_rule, 0);

    gtk_main();    

    vsm_recover_sfc_by_zoom_closed();
    nfui_page_close(PGID_VCA_ZOOM, win);

    return 0;
}

gint vw_dvabx_calibration_zoom_attach_mevent(DVA_MEVENT_E mevt_type, DVA_MEVENT_CB_FUNC mevent_cb, gpointer user_data)
{
	if (mevt_type >= DVA_MEVENT_MAX) return -1;

	g_mevent_cb[mevt_type].cb_func = mevent_cb;
	g_mevent_cb[mevt_type].user_data = user_data;
	return 0;
}

gint vw_dvabx_calibration_zoom_set_vmap(DVA_VMAP_CB_FUNC vmap_cb, gpointer user_data)
{
	g_vmap_cb.cb_func = vmap_cb;
	g_vmap_cb.user_data = user_data;
    return 0;
}

