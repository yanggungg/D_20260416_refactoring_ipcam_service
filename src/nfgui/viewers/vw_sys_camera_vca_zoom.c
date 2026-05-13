#include <string.h>

#include "nf_afx.h"
#include "nf_common.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "tools/nf_ui_tool.h"

#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "objects/nfspinbutton.h"
//#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfbutton.h"

#include "modules/var.h"
#include "modules/ssm.h"

#include "vsm.h"
#include "vw_sys_camera_vca_zoom.h"
#include "vw_sys_camera_vca_zoom_setup.h"
#include "vw_vca_internal.h"

#include "nf_api_live.h"
#include "ivca_def.h"

// ZOOM
#define ZOOM_AREA_SIZE_W								(1920 * 2)
#define ZOOM_AREA_SIZE_H								(1080 * 2)

#define ZOOM_AREA_W_SHARE							(ZOOM_AREA_SIZE_W/ZOOM_MAX_STEP)
#define ZOOM_AREA_H_SHARE							(ZOOM_AREA_SIZE_H/ZOOM_MAX_STEP)

#define ZOOM_AREA_X(step)	    					((ZOOM_AREA_SIZE_W-ZOOM_AREA_W(step))/2)
#define ZOOM_AREA_Y(step)	    					((ZOOM_AREA_SIZE_H-ZOOM_AREA_H(step))/2)
#define ZOOM_AREA_W(step)	    					(ZOOM_AREA_W_SHARE*step)
#define ZOOM_AREA_H(step)	    					(ZOOM_AREA_H_SHARE*step)

#define ZOOM_AREA_X_STEP						    (ZOOM_AREA_W_SHARE/2)
#define ZOOM_AREA_Y_STEP							(ZOOM_AREA_H_SHARE/2)
#define ZOOM_AREA_W_STEP							(ZOOM_AREA_W_SHARE)
#define ZOOM_AREA_H_STEP							(ZOOM_AREA_H_SHARE)

#define ZOOM_MAX_STEP								(10)
#define ZOOM_MIN_STEP								(1)
#define ZOOM_INIT_STEP      						(10)

#define ZOOM_VIDEO_W_SHARE							(DISPLAY_ACTIVE_WIDTH/ZOOM_MAX_STEP)
#define ZOOM_VIDEO_H_SHARE							(DISPLAY_ACTIVE_HEIGHT/ZOOM_MAX_STEP)

#define ZOOM_MOVE_X_STEP							(ZOOM_VIDEO_W_SHARE/2)
#define ZOOM_MOVE_Y_STEP							(ZOOM_VIDEO_H_SHARE/2)
#define ZOOM_MOVE_W_STEP							(ZOOM_VIDEO_W_SHARE)
#define ZOOM_MOVE_H_STEP							(ZOOM_VIDEO_H_SHARE)

#define ZOOM_MOVE_POS_X(step)						((DISPLAY_ACTIVE_WIDTH-(ZOOM_MOVE_W_STEP*step))/2)
#define ZOOM_MOVE_POS_Y(step)						((DISPLAY_ACTIVE_HEIGHT-(ZOOM_MOVE_H_STEP*step))/2)
#define ZOOM_MOVE_SIZE_W(step)						(ZOOM_MOVE_W_STEP*step)
#define ZOOM_MOVE_SIZE_H(step)						(ZOOM_MOVE_H_STEP*step)

#define COORD_RATIO									(0.5)

typedef struct _ZOOM_DRAW_AREA_DATA_T {
    gint x;     // area_x
    gint y;     // area_y
    gint w;     // area_w
    gint h;     // area_h

    gint step;  //
}ZOOM_DRAW_AREA_DATA_T;

typedef struct _ZOOM_DRAW_TARGET_T{
	ivca_calib_target_t c_target;
    guint target_index;             // index of targets list
}ZOOM_DRAW_TARGET_T;

typedef enum {
	ZOOM_CMD_START = 0,
	ZOOM_CMD_STOP,
	ZOOM_CMD_ZOOMIN,
	ZOOM_CMD_ZOOMOUT,
    ZOOM_CMD_MOVE
//	ZOOM_CMD_MOVE_LEFT,
//	ZOOM_CMD_MOVE_RIGHT,
//	ZOOM_CMD_MOVE_UP,
//	ZOOM_CMD_MOVE_DOWN,
}ZoomCmd;

typedef enum {
	ZOOM_DIR_NONE = 0,
	ZOOM_DIR_LEFT,
	ZOOM_DIR_RIGHT,
	ZOOM_DIR_UP,
	ZOOM_DIR_DOWN
}ZoomMoveDir;

enum{
    ZOOM_IN = 0,
    ZOOM_OUT
};

typedef struct _DRAW_TARGET_MANAGER_T {
    guint ch;
    gint sel_idx;			/* Selected calib target index. */

    GdkPoint evt_point;
    ivca_calib_t calibData;
    ZOOM_DRAW_AREA_DATA_T zoom_area;

    ZOOM_DRAW_TARGET_T target[IVCA_MAX_CALIB_TARGETS];
    ZOOM_DRAW_TARGET_T drawed_target[IVCA_MAX_CALIB_TARGETS];

}DRAW_TARGET_MANAGER;

static DRAW_TARGET_MANAGER idrt;

static NFWINDOW *g_curwnd = 0;
static guint g_tid;

static gboolean retval = FALSE;

extern int g_live_freeze;
int g_live_ch;
static g_sel_target_pt;

static gboolean cal_zoom_area_size(gint resize)
{
	ZOOM_DRAW_AREA_DATA_T tmp;
	gint gap;

	tmp.x = idrt.zoom_area.x;
	tmp.y = idrt.zoom_area.y;
	tmp.w = idrt.zoom_area.w;
	tmp.h = idrt.zoom_area.h;
	tmp.step = idrt.zoom_area.step;

	if(resize == ZOOM_OUT) {

		if(tmp.x <= ZOOM_AREA_X(ZOOM_MAX_STEP) && tmp.y <= ZOOM_AREA_Y(ZOOM_MAX_STEP) && tmp.w >= ZOOM_AREA_W(ZOOM_MAX_STEP) && tmp.h >= ZOOM_AREA_H(ZOOM_MAX_STEP)) 
			return FALSE;

		if(idrt.zoom_area.step < ZOOM_MAX_STEP)
			tmp.step++;
		else 
			return FALSE;

		if(idrt.zoom_area.step < ZOOM_MAX_STEP) {
			tmp.x -= ZOOM_AREA_X_STEP;
			tmp.y -= ZOOM_AREA_Y_STEP;
			tmp.w += ZOOM_AREA_W_STEP;
			tmp.h += ZOOM_AREA_H_STEP;
		}else if(idrt.zoom_area.step == ZOOM_MAX_STEP) {
			tmp.x = ZOOM_AREA_X(ZOOM_MAX_STEP);
			tmp.y = ZOOM_AREA_Y(ZOOM_MAX_STEP);
			tmp.w = ZOOM_AREA_W(ZOOM_MAX_STEP);
			tmp.h = ZOOM_AREA_H(ZOOM_MAX_STEP);
		}
	}
	else if(resize == ZOOM_IN) {
		if(tmp.w <= ZOOM_AREA_W_SHARE && tmp.h <= ZOOM_AREA_H_SHARE)
			return FALSE;

		if(idrt.zoom_area.step > ZOOM_MIN_STEP)
			tmp.step--;
		else
			return FALSE;

		tmp.x += ZOOM_AREA_X_STEP;
		tmp.y += ZOOM_AREA_Y_STEP;
		tmp.w -= ZOOM_AREA_W_STEP;
		tmp.h -= ZOOM_AREA_H_STEP;
	}

	if(tmp.x <= 0) {
		tmp.x = ZOOM_AREA_X(ZOOM_MAX_STEP);
	}

	if(tmp.x + tmp.w > ZOOM_AREA_X(ZOOM_MAX_STEP) + ZOOM_AREA_W(ZOOM_MAX_STEP)) {
		gap = (tmp.x + tmp.w) - (ZOOM_AREA_X(ZOOM_MAX_STEP) + ZOOM_AREA_W(ZOOM_MAX_STEP));
		tmp.x -= gap;
	}

	if(tmp.y <= 0) {
		tmp.y = ZOOM_AREA_Y(ZOOM_MAX_STEP);
	}

	if(tmp.y + tmp.h > ZOOM_AREA_Y(ZOOM_MAX_STEP) + ZOOM_AREA_H(ZOOM_MAX_STEP)) {
		gap = (tmp.y + tmp.h) - (ZOOM_AREA_Y(ZOOM_MAX_STEP) + ZOOM_AREA_H(ZOOM_MAX_STEP)); 
		tmp.y -= gap;
	}

	idrt.zoom_area = tmp;

    //g_message("[%s, %d] tmp_x(%d) tmp_y(%d) tmp_w(%d) tmp_h(%d)", __FUNCTION__, __LINE__, tmp.x, tmp.y, tmp.w, tmp.h);

	return TRUE;
}

static gboolean cal_zoom_box_coord(gint x, gint y)
{
	ZOOM_DRAW_AREA_DATA_T tmp;
	gint move_x = 0, move_y = 0;

	tmp.x = idrt.zoom_area.x;
	tmp.y = idrt.zoom_area.y;
	tmp.w = idrt.zoom_area.w;
	tmp.h = idrt.zoom_area.h;
	tmp.step = idrt.zoom_area.step;

	if(idrt.zoom_area.step == ZOOM_MAX_STEP) 
        return FALSE;

	if(idrt.zoom_area.w == ZOOM_AREA_W(ZOOM_MAX_STEP)
			&& idrt.zoom_area.h == ZOOM_AREA_W(ZOOM_MAX_STEP))
        return FALSE;


	move_x = (idrt.evt_point.x - x) * idrt.zoom_area.w / DISPLAY_ACTIVE_WIDTH;
	move_y = (idrt.evt_point.y - y) * idrt.zoom_area.h / DISPLAY_ACTIVE_HEIGHT;

	if((move_x + tmp.x) < 0) 
		move_x = 0;
	if((move_x + tmp.x) > DISPLAY_ACTIVE_WIDTH)
		move_x = DISPLAY_ACTIVE_WIDTH - tmp.x;
	
	if((move_y + tmp.y) < 0) 
		move_y = 0;
	if((move_y + tmp.y) > DISPLAY_ACTIVE_HEIGHT)
		move_y = DISPLAY_ACTIVE_HEIGHT - tmp.y;

    tmp.x += move_x;
    tmp.y += move_y;

	idrt.zoom_area = tmp;

    idrt.evt_point.x -= (move_x*DISPLAY_ACTIVE_WIDTH/idrt.zoom_area.w);
    idrt.evt_point.y -= (move_y*DISPLAY_ACTIVE_HEIGHT/idrt.zoom_area.h);

				

	return TRUE;
}

static _init_zoom_setup()
{

    if( idrt.calibData.ntargets < 5)
        set_estimate_button(FALSE);
    else
        set_estimate_button(TRUE);

   set_text_result_value(idrt.calibData.height, idrt.calibData.tilt, idrt.calibData.focal);
}


gboolean zoom_cmd(ZoomCmd cmd)
{
	gint ret = 0;
	gint pos_sx = 0, pos_sy = 0;
	gint pos_ex = 0, pos_ey = 0;
	gint pos_nx = 0, pos_ny = 0;
	gint size_w = 0, size_h = 0;

    if(cmd >= ZOOM_CMD_ZOOMIN) {
        pos_sx = nf_live_zoom_get_pos_sx(); 
        pos_sy = nf_live_zoom_get_pos_sy();

        pos_ex = nf_live_zoom_get_pos_ex(); 
        pos_ey = nf_live_zoom_get_pos_ey();

        size_w = nf_live_zoom_get_pos_dx(); 
        size_h = nf_live_zoom_get_pos_dy();
    }

	switch(cmd) {
        case ZOOM_CMD_START:
            {
                ret = nf_live_zoom_start_without_pip(idrt.ch, 
                                         ZOOM_MOVE_POS_X(ZOOM_INIT_STEP), 
                                         ZOOM_MOVE_POS_Y(ZOOM_INIT_STEP), 
                                         ZOOM_MOVE_SIZE_W(ZOOM_INIT_STEP), 
                                         ZOOM_MOVE_SIZE_H(ZOOM_INIT_STEP),
                                         TRUE);
                // SETUP UI
                VW_VCA_Zoom_Open_Setup(g_curwnd);
                _init_zoom_setup();

                return ret;
            }
            break;

        case ZOOM_CMD_ZOOMIN:
            {
				pos_nx = idrt.zoom_area.x * COORD_RATIO;
				pos_ny = idrt.zoom_area.y * COORD_RATIO;

				pos_sx += (pos_nx - pos_sx);
				pos_sy += (pos_ny - pos_sy);
				size_w -= ZOOM_MOVE_W_STEP;
				size_h -= ZOOM_MOVE_H_STEP;
			}
			break;

        case ZOOM_CMD_ZOOMOUT:
            {
				pos_nx = idrt.zoom_area.x * COORD_RATIO;
				pos_ny = idrt.zoom_area.y * COORD_RATIO;

				pos_sx -= (pos_sx - pos_nx);
				pos_sy -= (pos_sy - pos_ny);
				size_w += ZOOM_MOVE_W_STEP;
				size_h += ZOOM_MOVE_H_STEP;

				if(size_w >= DISPLAY_ACTIVE_WIDTH) 	pos_sx = 0;
				if(size_h >= DISPLAY_ACTIVE_HEIGHT) pos_sy = 0;

				if((pos_sx + size_w) > DISPLAY_ACTIVE_WIDTH)
					size_w -= (pos_sx + size_w - DISPLAY_ACTIVE_WIDTH);
				if((pos_sy + size_h) > DISPLAY_ACTIVE_HEIGHT)
					size_h -= (pos_sy + size_h - DISPLAY_ACTIVE_HEIGHT);
			}
			break;

		case ZOOM_CMD_MOVE:
            {
				pos_nx = idrt.zoom_area.x * COORD_RATIO;
				pos_ny = idrt.zoom_area.y * COORD_RATIO;

                if(pos_sx > pos_nx) pos_sx -= (pos_sx - pos_nx);
                else 				pos_sx += (pos_nx - pos_sx);


                if(pos_sy > pos_ny) pos_sy -= (pos_sy - pos_ny);
                else 				pos_sy += (pos_ny - pos_sy);

                if(pos_sx < 0) pos_sx = 0;
                if((pos_sx + size_w) > DISPLAY_ACTIVE_WIDTH) pos_sx -= ((pos_sx + size_w) - DISPLAY_ACTIVE_WIDTH);

                if(pos_sy < 0) pos_sy = 0;
                if((pos_sy + size_h) > DISPLAY_ACTIVE_HEIGHT) pos_sy -= ((pos_sy + size_h) - DISPLAY_ACTIVE_HEIGHT);
            }
            break;

        default :
            break;
    }

	ret = nf_live_zoom_move(pos_sx, pos_sy, size_w, size_h);
	
	if(ret) return TRUE;
	else	return FALSE;
}

static void _convert_calibData_to_drawPoint(ivca_calib_target_t *draw_target, 
                                            ivca_calib_target_t *calib_target)
{
    int i;

    for( i = 0; i < 2; i++)
    {
        draw_target->pt[i].x = (calib_target->pt[i].x - idrt.zoom_area.x) * DISPLAY_ACTIVE_WIDTH / (idrt.zoom_area.w);
        draw_target->pt[i].y = (calib_target->pt[i].y - idrt.zoom_area.y) * DISPLAY_ACTIVE_HEIGHT/ (idrt.zoom_area.h);
    }
    draw_target->height = calib_target->height;

}

static void _convert_drawPoint_to_calibData(ivca_calib_target_t *calib_target,
                                            ivca_calib_target_t *draw_target)
{
    int i;

    for( i = 0; i < 2; i++)
    {
        calib_target->pt[i].x = draw_target->pt[i].x * idrt.zoom_area.w / DISPLAY_ACTIVE_WIDTH + idrt.zoom_area.x;
        calib_target->pt[i].y = draw_target->pt[i].y * idrt.zoom_area.h / DISPLAY_ACTIVE_HEIGHT + idrt.zoom_area.y;

    }
    calib_target->height = draw_target->height;
}

static gint _is_select_target_point(gint x, gint y, ivca_calib_target_t *draw_target)
{
    int i;
    gint diff_x, diff_y;
	gint tx = 8 * ZOOM_AREA_SIZE_W /DISPLAY_ACTIVE_WIDTH;
	gint ty = 8 * ZOOM_AREA_SIZE_H /DISPLAY_ACTIVE_HEIGHT;

    for( i = 0; i < 2; i++)
    {
        diff_x = idrt.target[idrt.sel_idx].c_target.pt[i].x - x;
        diff_y = idrt.target[idrt.sel_idx].c_target.pt[i].y - y;

        if((abs(diff_x) < tx) && ((abs(diff_y) < ty)))
            break;
    }

    if(i < 2)
        return i;
    else
        return -1;
}

static gint _select_target(gint x, gint y)
{
    int i;
	gint tx = 8 * ZOOM_AREA_SIZE_W /DISPLAY_ACTIVE_WIDTH;

    for(i = 0; i < idrt.calibData.ntargets; i++)
    {
		if ( vw_vca_polygon_test(x, y, tx, 2, &idrt.drawed_target[i]))
            break;
    }

    if(i == idrt.calibData.ntargets)
        return -1;
    else
        return i;

}	/* _select_rule(... */




static void draw_targets(NFOBJECT *obj, GdkDrawable *drawable, GdkGC *gc)
{
    int i;

    if( idrt.calibData.ntargets == 0 )
    {
        for(i = 0; i < 32; i++)
        {
            if(memcmp(&idrt.target[i], &idrt.drawed_target[i], sizeof(ZOOM_DRAW_TARGET_T)))
                vw_vca_draw_target(obj, drawable, gc, ERASE_TARGET, &idrt.drawed_target[i].c_target);
        }
    }
    
    else
    {
        for(i = 0; i < idrt.calibData.ntargets + 1; i++)
        {
            if(memcmp(&idrt.target[i], &idrt.drawed_target[i], sizeof(ZOOM_DRAW_TARGET_T)))
                vw_vca_draw_target(obj, drawable, gc, ERASE_TARGET, &idrt.drawed_target[i].c_target);

            if( i == idrt.sel_idx)
                vw_vca_draw_target(obj, drawable, gc, SELECT_TARGET, &idrt.target[i].c_target);
            else if(i == idrt.calibData.ntargets)
                vw_vca_draw_target(obj, drawable, gc, ERASE_TARGET, &idrt.drawed_target[i].c_target);
            else
                vw_vca_draw_target(obj, drawable, gc, DRAW_TARGET, &idrt.target[i].c_target);
        }
    }
    memcpy(idrt.drawed_target, idrt.target, sizeof(ZOOM_DRAW_TARGET_T) * IVCA_MAX_CALIB_TARGETS);
}

static gint _add_target()
{
    gint idx;

    if(idrt.calibData.ntargets == IVCA_MAX_CALIB_TARGETS)
    {
        nftool_mbox(g_curwnd, "NOTICE", "Cannot add more target.", NFTOOL_MB_OK);
        return FALSE;
    }

    idx = idrt.calibData.ntargets;

    vw_vca_add_target(&idrt.target[idx].c_target);
    idrt.calibData.ntargets++;

    _convert_drawPoint_to_calibData(&idrt.calibData.targetlist[idx], &idrt.target[idx].c_target );


    idrt.sel_idx = idx;

    if( idrt.calibData.ntargets >= 5)
        set_estimate_button(TRUE);

    return idrt.calibData.ntargets;
}

static gint _modify_target(gint x, gint y)
{
    int i;
    gint move_x, move_y;
    //gint sel_pt;

	move_x = idrt.evt_point.x - x;
	move_y = idrt.evt_point.y - y;
    //sel_pt =  _is_select_target_point(x, y, &idrt.target[idrt.sel_idx]);
    //if(sel_pt >= 0)
   
    if(g_sel_target_pt >= 0)
    {
        idrt.target[idrt.sel_idx].c_target.pt[g_sel_target_pt].x = idrt.evt_point.x;
        idrt.target[idrt.sel_idx].c_target.pt[g_sel_target_pt].y = idrt.evt_point.y;
        //idrt.target[idrt.sel_idx].c_target.pt[sel_pt].x -= move_x;
        //idrt.target[idrt.sel_idx].c_target.pt[sel_pt].y -= move_y;

    }
    else
    {
        for( i = 0; i < 2; i++)
        {
            idrt.target[idrt.sel_idx].c_target.pt[i].x -= move_x;
            idrt.target[idrt.sel_idx].c_target.pt[i].y -= move_y;
        }
    }
    idrt.evt_point.x = x;
    idrt.evt_point.y = y;

    _convert_drawPoint_to_calibData(&idrt.calibData.targetlist[idrt.sel_idx], &idrt.target[idrt.sel_idx].c_target );
    return 0;
}


static gint _delete_target()
{
    gint idx;
    int i;

    idx = vw_vca_del_cal_target(&idrt.calibData, idrt.sel_idx);

    for(i = 0; i < idrt.calibData.ntargets + 1; i++)
    {
        _convert_calibData_to_drawPoint(&idrt.target[i].c_target, &idrt.calibData.targetlist[i]);
    }


    //memset(&idrt.target[idrt.sel_idx].c_target, 0x00, sizeof(ivca_calib_target_t));

    //_convert_calibData_to_drawPoint(&idrt.target[idrt.sel_idx].c_target, &idrt.calibData);

    idrt.sel_idx = idx;

    if( idrt.calibData.ntargets < 5)
        set_estimate_button(FALSE);

    return idrt.calibData.ntargets;
}

static gint _reset_target()
{
    gint idx;

    idrt.calibData.ntargets = 0;

    memset(&idrt.target, 0x00, sizeof(ZOOM_DRAW_TARGET_T) * IVCA_MAX_CALIB_TARGETS);
    memset(&idrt.calibData, 0x00, sizeof(idrt.calibData));

    idrt.sel_idx = -1;

    set_text_result_value(idrt.calibData.height, idrt.calibData.tilt, idrt.calibData.focal);
    set_estimate_button(FALSE);

    return 0;

}

static gint _estimate_target(NFOBJECT *obj)
{
	gint idx;

    idx = vw_vca_cal_estimate(obj, &idrt.calibData, idrt.ch);
    if(idx >=0 )
    {
        idrt.calibData.paramvalid = TRUE;

        set_text_result_value(idrt.calibData.height, idrt.calibData.tilt, idrt.calibData.focal);
    }
    else
    {
		nftool_mbox((NFWINDOW*)obj, "ERROR", "Some icons are abnormal. Please adjust the icon size or add more icons.", NFTOOL_MB_OK);
    }

    return 0;
}

static gint _set_height_target(int value)
{
    if(idrt.sel_idx >= 0)
    {
        idrt.calibData.targetlist[idrt.sel_idx].height = IVCA_MIN_CALIB_HEIGHT + 5 * value;
    }

    return 0;
}

static gboolean _proc_timer_draw(gpointer *data)
{
    NFOBJECT *obj = (NFOBJECT *)data;
	GdkDrawable *drawable;
	GdkGC *gc;

    if(!g_tid)
        return FALSE;

	drawable = nfui_nfobject_get_window(obj);
    gc= gdk_gc_new(drawable);

	draw_targets(obj, drawable, gc);
	
	return TRUE;
}


static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {

        case GDK_DELETE:
            {
                if(g_tid != 0)
                {
                    g_source_remove(g_tid);
                    g_tid = 0;
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
	switch(evt->type) {
		case GDK_EXPOSE:
            break;
		case GDK_BUTTON_PRESS:
            {
                if ( evt->button.button != MOUSE_LEFT_BUTTON )
                    break;

                idrt.evt_point.x = (int)((GdkEventButton*)evt)->x;
                idrt.evt_point.y = (int)((GdkEventButton*)evt)->y;
                idrt.sel_idx = _select_target(idrt.evt_point.x, idrt.evt_point.y);
                if(idrt.sel_idx >= 0)
                    g_sel_target_pt =  _is_select_target_point(idrt.evt_point.x, idrt.evt_point.y, &idrt.target[idrt.sel_idx]);
                else
                    g_sel_target_pt = -1;
            }
            break;
        case GDK_SCROLL:
            {
				GdkEventScroll *sevt;
                int i;

				sevt = (GdkEventScroll*)evt;
				if(sevt->direction == GDK_SCROLL_UP) {
					if(!cal_zoom_area_size(ZOOM_IN)) 
						break;

					if(!zoom_cmd(ZOOM_CMD_ZOOMIN))
						g_message("<%s> not action!", __FUNCTION__);
				}
				else if(sevt->direction == GDK_SCROLL_DOWN) {
					if(!cal_zoom_area_size(ZOOM_OUT)) 
						break;

					if(!zoom_cmd(ZOOM_CMD_ZOOMOUT))
						g_message("<%s> not action!", __FUNCTION__);
				}

                for(i = 0; i < idrt.calibData.ntargets; i++)
                {
                    _convert_calibData_to_drawPoint(&idrt.target[i].c_target, &idrt.calibData.targetlist[i]);
                }

			}
			break;
        case GDK_MOTION_NOTIFY:
            {
                GdkEventMotion *mevt;
                int i;

				mevt = (GdkEventMotion*)evt;
				if(!(mevt->state & GDK_BUTTON1_MASK))
					break;

                if(idrt.sel_idx >= 0)
                {
                    _modify_target((gint)mevt->x, (gint)mevt->y);
					break;
                }

                if(!cal_zoom_box_coord((gint)mevt->x, (gint)mevt-> y))
                    break;

                if(zoom_cmd(ZOOM_CMD_MOVE)){

	                for(i = 0; i < idrt.calibData.ntargets; i++)
	                    _convert_calibData_to_drawPoint(&idrt.target[i].c_target, &idrt.calibData.targetlist[i]);

	           		 }
        		}
            break;

		case GDK_BUTTON_RELEASE:
            {
                idrt.evt_point.x = 0;
                idrt.evt_point.y = 0;
            }
            break;

        default:
            break;

	}

	return FALSE;
}



void static _init(guint ch, ivca_calib_t *calib, gint select_idx)
{
    int i;

    idrt.ch = ch;
    idrt.sel_idx = select_idx;

    memcpy(&idrt.calibData, calib, sizeof(idrt.calibData));


    // drawing area
    idrt.zoom_area.x = 0;
    idrt.zoom_area.y = 0;
    idrt.zoom_area.w = ZOOM_AREA_SIZE_W;
    idrt.zoom_area.h = ZOOM_AREA_SIZE_H;
    idrt.zoom_area.step = ZOOM_INIT_STEP;

    // move zoom area
    idrt.evt_point.x = 0;
    idrt.evt_point.y = 0;

	memset(&idrt.drawed_target, 0x00, sizeof(ZOOM_DRAW_TARGET_T) * IVCA_MAX_CALIB_TARGETS);
	memset(&idrt.target, 0x00, sizeof(ZOOM_DRAW_TARGET_T) * IVCA_MAX_CALIB_TARGETS);

    for(i = 0; i < idrt.calibData.ntargets; i++)
    {
        //memcpy(&idrt.target[i].c_target, &idrt.calibData.targetlist[i], sizeof(idrt.target[i]));
        _convert_calibData_to_drawPoint(&idrt.target[i].c_target, &idrt.calibData.targetlist[i]);
        idrt.target[i].target_index = i;
    }

    return;
}

void _vca_zoom_init()
{

    	int i;
		
	idrt.zoom_area.x = 0;
	idrt.zoom_area.y = 0;
	idrt.zoom_area.w = ZOOM_AREA_SIZE_W;
	idrt.zoom_area.h = ZOOM_AREA_SIZE_H;
	idrt.zoom_area.step = ZOOM_INIT_STEP;

	// move zoom area
	idrt.evt_point.x = 0;
	idrt.evt_point.y = 0;

	 for(i = 0; i < idrt.calibData.ntargets; i++)
	    {
	        _convert_calibData_to_drawPoint(&idrt.target[i].c_target, &idrt.calibData.targetlist[i]);
	        idrt.target[i].target_index = i;
	 	}

	nf_live_zoom_start_without_pip(idrt.ch, 
                                         ZOOM_MOVE_POS_X(ZOOM_INIT_STEP), 
                                         ZOOM_MOVE_POS_Y(ZOOM_INIT_STEP), 
                                         ZOOM_MOVE_SIZE_W(ZOOM_INIT_STEP), 
                                         ZOOM_MOVE_SIZE_H(ZOOM_INIT_STEP),
                                         TRUE);

}

gint zoom_add_target()
{
    gint ret;
    ret = _add_target();

    return ret;
}

gint zoom_delete_target()
{
    gint ret;
    ret = _delete_target();

    return ret;
}

gint zoom_reset_target()
{
    gint ret;
    ret = _reset_target();

    return ret;
}

gint zoom_estimate_target(NFOBJECT *obj)
{
    gint ret;
    ret = _estimate_target(obj);

    return ret;
}

gint zoom_set_height_target(int value)
{
    gint ret;
    ret = _set_height_target(value);

    return ret;
}



gint zoom_save_targets()
{
	retval = TRUE;

    return 0;
}

gint get_target_num()
{
    return idrt.calibData.ntargets;
}


gint zoom_wnd_close()
{
    nfui_nfobject_destroy((NFOBJECT*)g_curwnd);
}

gint VW_VCA_Zoom_Open(NFOBJECT *parent, guint ch, ivca_calib_t *calib_data, gint select_idx)
{
    NFOBJECT *main_wnd = NULL;
	NFOBJECT *main_fixed;

    _init(ch, calib_data, select_idx);
	g_live_ch = ch;
	g_live_freeze = 0;

    main_wnd = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
    nfui_nfwindow_set_modal(main_wnd, FALSE);
    nfui_nfwindow_set_title((NFWINDOW*)main_wnd, "VCA ZOOM");

	nfui_regi_post_event_callback(main_wnd, post_main_win_event_handler);


	nfui_nfobject_modify_bg(main_wnd, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    g_curwnd = (NFWINDOW*)main_wnd;

    main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);
    nfui_nfobject_show(main_fixed);
    nfui_nfwindow_add((NFWINDOW*)main_wnd, main_fixed);

    nfui_run_main_event_handler(main_wnd);

	nfui_nfobject_show(main_wnd);

    vsm_change_sfc_by_zoom_opened(ch);
    zoom_cmd(ZOOM_CMD_START);
	nfui_page_open(PGID_VCA_ZOOM, main_wnd, ssm_get_cur_id(NULL));

	g_tid = g_timeout_add(40, _proc_timer_draw, main_wnd);

    gtk_main();

    vsm_recover_sfc_by_zoom_closed();
	nfui_page_close(PGID_VCA_ZOOM, main_wnd);

    if(retval == TRUE)
        memcpy(calib_data, &idrt.calibData, sizeof(idrt.calibData));

    retval = FALSE;

    return idrt.sel_idx;
}

