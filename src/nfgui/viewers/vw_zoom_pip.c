/*
 * vw_zoom_pip.c
 * written by seongho
 */
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
#include "viewers/objects/nfcombobox.h"
#include "viewers/objects/nfbutton.h"

#include "modules/var.h"
#include "modules/ssm.h"

#include "vsm.h"
#include "vw_zoom_pip.h"

#include "nf_api_live.h"
#include "nf_api_play.h"

#include "smt.h"

#define _ZOOM_STEP_LIMIT

#define ZOOM_PIP_POS_X                              (g_zpWin->x)
#define ZOOM_PIP_POS_Y                              (g_zpWin->y)
#define ZOOM_PIP_SIZE_W                             (576)
#define ZOOM_PIP_SIZE_H                             (392)
/*
#define ZOOM_PIP_SIZE_W                             (640)
#define ZOOM_PIP_SIZE_H                             (428)
*/

#define ZOOM_AREA_SIZE_W                            (ZOOM_PIP_SIZE_W)
#define ZOOM_AREA_SIZE_H                            (ZOOM_PIP_SIZE_H - ZOOM_BAR_SIZE_H)

#define ZOOM_BAR_SIZE_W                             (ZOOM_PIP_SIZE_W)
#define ZOOM_BAR_SIZE_H                             (68)

#define ZOOM_BOX_LINE_SIZE                          (3)
#define ZOOM_BOX_W_SHARE                            (ZOOM_AREA_SIZE_W/ZOOM_MAX_STEP)
#define ZOOM_BOX_H_SHARE                            (ZOOM_AREA_SIZE_H/ZOOM_MAX_STEP)
#define ZOOM_BOX_W_REST                             (ZOOM_AREA_SIZE_W%ZOOM_MAX_STEP) 
#define ZOOM_BOX_H_REST                             (ZOOM_AREA_SIZE_H%ZOOM_MAX_STEP) 

//#define ZOOM_BOX_POS_X(step)                      (((ZOOM_MAX_STEP-step)*ZOOM_BOX_SIZE_W(step))+ZOOM_BOX_W_REST/2)
//#define ZOOM_BOX_POS_Y(step)                      (((ZOOM_MAX_STEP-step)*ZOOM_BOX_SIZE_H(step))+ZOOM_BOX_H_REST/2)
#define ZOOM_BOX_POS_X(step)                        ((ZOOM_AREA_SIZE_W-ZOOM_BOX_SIZE_W(step))/2)
#define ZOOM_BOX_POS_Y(step)                        ((ZOOM_AREA_SIZE_H-ZOOM_BOX_SIZE_H(step))/2)
#define ZOOM_BOX_SIZE_W(step)                       (ZOOM_BOX_W_SHARE*step)
#define ZOOM_BOX_SIZE_H(step)                       (ZOOM_BOX_H_SHARE*step)

#define ZOOM_BOX_X_STEP                             (ZOOM_BOX_W_SHARE/2)
#define ZOOM_BOX_Y_STEP                             (ZOOM_BOX_H_SHARE/2)
#define ZOOM_BOX_W_STEP                             (ZOOM_BOX_W_SHARE)
#define ZOOM_BOX_H_STEP                             (ZOOM_BOX_H_SHARE)

#define ZOOM_MAX_STEP                               (10)
#if defined(_ZOOM_STEP_LIMIT)
#define ZOOM_MIN_STEP                               (4)
#else
#define ZOOM_MIN_STEP                               (1)
#endif
#define ZOOM_INIT_STEP                              (10)

#define ZOOM_VIDEO_W_SHARE                          (DISPLAY_ACTIVE_WIDTH/ZOOM_MAX_STEP)
#define ZOOM_VIDEO_H_SHARE                          (DISPLAY_ACTIVE_HEIGHT/ZOOM_MAX_STEP)

#define ZOOM_MOVE_X_STEP                            (ZOOM_VIDEO_W_SHARE/2)
#define ZOOM_MOVE_Y_STEP                            (ZOOM_VIDEO_H_SHARE/2)
#define ZOOM_MOVE_W_STEP                            (ZOOM_VIDEO_W_SHARE)
#define ZOOM_MOVE_H_STEP                            (ZOOM_VIDEO_H_SHARE)

#define ZOOM_MOVE_POS_X(step)                       ((DISPLAY_ACTIVE_WIDTH-(ZOOM_MOVE_W_STEP*step))/2)
#define ZOOM_MOVE_POS_Y(step)                       ((DISPLAY_ACTIVE_HEIGHT-(ZOOM_MOVE_H_STEP*step))/2)
#define ZOOM_MOVE_SIZE_W(step)                      (ZOOM_MOVE_W_STEP*step)
#define ZOOM_MOVE_SIZE_H(step)                      (ZOOM_MOVE_H_STEP*step)

#define COORD_RATIO                                 (3.3)

#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)                 (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)                 (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)                 (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#endif


typedef struct {
    gint mode;
    gint ch;
    gint step;
    gdouble px;
    gdouble py;
    GdkRectangle area;
}ZOOMPIP_DATA;


typedef enum {
    ZOOM_CMD_START = 0,
    ZOOM_CMD_STOP,
    ZOOM_CMD_CHANGE_CH,
    ZOOM_CMD_ZOOMIN,
    ZOOM_CMD_ZOOMOUT,
    ZOOM_CMD_MOVE_LEFT,
    ZOOM_CMD_MOVE_RIGHT,
    ZOOM_CMD_MOVE_UP,
    ZOOM_CMD_MOVE_DOWN,
    ZOOM_CMD_JUMP,
}ZoomCmd;

typedef enum {
    ZOOM_DIR_NONE = 0,
    ZOOM_DIR_LEFT,
    ZOOM_DIR_RIGHT,
    ZOOM_DIR_UP,
    ZOOM_DIR_DOWN
}ZoomMoveDir;

enum {
    ZOOM_BOX_INCREASE = 0,
    ZOOM_BOX_DECREASE
};

static NFWINDOW *g_curwnd = 0;
static ZOOMPIP_DATA g_zpData;
static OsdData g_osddata;

static NFOBJECT *g_zpWin = NULL; 
static NFOBJECT *g_zpArea;
static NFOBJECT *g_combo;


static void init_zoom_box();
static gboolean cal_zoom_box_coord(ZoomMoveDir dir, gint x, gint y);
static gboolean cal_zoom_box_size(gint resize);
static void cal_zoom_box_position(GdkRectangle *rect, gdouble x, gdouble y);
static void draw_init_zoom_box();
static void draw_zoom_box();
static void erase_zoom_box();
static gboolean move_zoom_box(ZoomMoveDir dir, gint x, gint y);
static gboolean press_zoom_box(gdouble x, gdouble y);
static gboolean resize_zoom_box(gint resize);
static gboolean is_movable_zoom_box(gint x, gint y);
static ZoomMoveDir get_mouse_move_dir(gint x, gint y);
static gdouble press_x, press_y;


static /*inline*/ void set_zoom_ch(gint ch)
{
    g_zpData.ch = ch; 
}

static /*inline*/ void set_video_mode(VDO_MODE_E mode)
{
    g_zpData.mode = mode;
}

static /*inline*/ gboolean live_zoom_mode()
{
    if(g_zpData.mode == 0)
        return TRUE;

    return FALSE;
}

static /*inline*/ gboolean playback_zoom_mode()
{
    if(g_zpData.mode == 1)
        return TRUE;

    return FALSE;
}

static void init_zoom_box()
{
    g_zpData.area.x = ZOOM_BOX_POS_X(ZOOM_INIT_STEP);
    g_zpData.area.y = ZOOM_BOX_POS_Y(ZOOM_INIT_STEP);
    g_zpData.area.width = ZOOM_BOX_SIZE_W(ZOOM_INIT_STEP);
    g_zpData.area.height = ZOOM_BOX_SIZE_H(ZOOM_INIT_STEP);

    //g_message("%s >>>>> zbox init x: %d y:%d w: %d h: %d",
    //      __FUNCTION__,
    //      ZOOM_BOX_POS_X(ZOOM_INIT_STEP),
    //      ZOOM_BOX_POS_Y(ZOOM_INIT_STEP),
    //      ZOOM_BOX_SIZE_W(ZOOM_INIT_STEP),
    //      ZOOM_BOX_SIZE_H(ZOOM_INIT_STEP));   

    g_zpData.step = ZOOM_INIT_STEP;
}

static gboolean cal_zoom_box_coord(ZoomMoveDir dir, gint x, gint y)
{
    GdkRectangle tmp;

    tmp.x = g_zpData.area.x;
    tmp.y = g_zpData.area.y;
    tmp.width = g_zpData.area.width;
    tmp.height = g_zpData.area.height;
    
    switch(dir) {
        case ZOOM_DIR_LEFT:  
            {
                if(x > 0) {
                    if(g_zpData.step == 1 || g_zpData.step == 2) {
                        if(x <= tmp.x)
                            goto go_left;
                    }

                    if((tmp.x + tmp.width/2) - x < ZOOM_BOX_W_STEP)
                        return FALSE;
                }

                go_left:
                tmp.x -= ZOOM_BOX_X_STEP;
            }
            break;

        case ZOOM_DIR_RIGHT: 
            {
                if(x > 0) {
                    if(g_zpData.step == 1 || g_zpData.step == 2) {
                        if(x >= tmp.x + tmp.width)
                            goto go_right;
                    }

                    if(x - (tmp.x + tmp.width/2) < ZOOM_BOX_W_STEP)
                        return FALSE;
                }

                go_right:
                tmp.x += ZOOM_BOX_X_STEP;
            }
            break;

        case ZOOM_DIR_UP:    
            {
                if(y > 0) {
                    if(g_zpData.step == 1 || g_zpData.step == 2) {
                        if(y <= tmp.y)
                            goto go_up;
                    }

                    if((tmp.y + tmp.height/2) - y < ZOOM_BOX_H_STEP)
                        return FALSE;
                }

                go_up:
                tmp.y -= ZOOM_BOX_Y_STEP;
            }
            break;

        case ZOOM_DIR_DOWN:  
            {
                if(y > 0) {
                    if(g_zpData.step == 1 || g_zpData.step == 2) {
                        if(y >= tmp.y + tmp.height)
                            goto go_down;
                    }

                    if(y - (tmp.y + tmp.height/2) < ZOOM_BOX_H_STEP)
                        return FALSE;
                }

                go_down:
                tmp.y += ZOOM_BOX_Y_STEP;
            }
            break;

        default:
            break;
    }

    if(tmp.x <= 0)                               
        tmp.x = ZOOM_BOX_POS_X(ZOOM_MAX_STEP);

    if((tmp.x + tmp.width) >= ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP))  
        tmp.x -= ((tmp.x + tmp.width) - ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP));

    if(tmp.y <= 0)                               
        tmp.y = ZOOM_BOX_POS_Y(ZOOM_MAX_STEP);

    if((tmp.y + tmp.height) >= ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP)) 
        tmp.y -= ((tmp.y + tmp.height) - ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP));

    g_zpData.area = tmp;

    return TRUE;
}

static gboolean cal_zoom_box_size(gint resize)
{
    GdkRectangle tmp;
    gint gap;


    tmp.x = g_zpData.area.x;
    tmp.y = g_zpData.area.y;
    tmp.width = g_zpData.area.width;
    tmp.height = g_zpData.area.height;

    if(resize == ZOOM_BOX_INCREASE) {
        if(tmp.x <= ZOOM_BOX_POS_X(ZOOM_MAX_STEP) && tmp.y <= ZOOM_BOX_POS_Y(ZOOM_MAX_STEP) && tmp.width >= ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP) && tmp.height >= ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP)) 
            return FALSE;

        if(g_zpData.step < ZOOM_MAX_STEP)
            g_zpData.step++;
        else 
            return FALSE;

        if(g_zpData.step < ZOOM_MAX_STEP) {
            tmp.x -= ZOOM_BOX_X_STEP;
            tmp.y -= ZOOM_BOX_Y_STEP;
            tmp.width += ZOOM_BOX_W_STEP;
            tmp.height += ZOOM_BOX_H_STEP;
        }else if(g_zpData.step == ZOOM_MAX_STEP) {
            tmp.x = ZOOM_BOX_POS_X(ZOOM_MAX_STEP);
            tmp.y = ZOOM_BOX_POS_Y(ZOOM_MAX_STEP);
            tmp.width = ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP);
            tmp.height = ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP);
        }
    }
    else if(resize == ZOOM_BOX_DECREASE) {
        if(tmp.width <= ZOOM_BOX_W_SHARE && tmp.height <= ZOOM_BOX_H_SHARE)
            return FALSE;

        if(g_zpData.step > ZOOM_MIN_STEP)
            g_zpData.step--;
        else
            return FALSE;

        tmp.x += ZOOM_BOX_X_STEP;
        tmp.y += ZOOM_BOX_Y_STEP;
        tmp.width -= ZOOM_BOX_W_STEP;
        tmp.height -= ZOOM_BOX_H_STEP;
    }

    if(tmp.x <= 0) {
        tmp.x = ZOOM_BOX_POS_X(ZOOM_MAX_STEP);
    }

    if(tmp.x + tmp.width > ZOOM_BOX_POS_X(ZOOM_MAX_STEP) + ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP)) {
        gap = (tmp.x + tmp.width) - (ZOOM_BOX_POS_X(ZOOM_MAX_STEP) + ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP));
        tmp.x -= gap;
    }

    if(tmp.y <= 0) {
        tmp.y = ZOOM_BOX_POS_Y(ZOOM_MAX_STEP);
    }

    if(tmp.y + tmp.height > ZOOM_BOX_POS_Y(ZOOM_MAX_STEP) + ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP)) {
        gap = (tmp.y + tmp.height) - (ZOOM_BOX_POS_Y(ZOOM_MAX_STEP) + ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP)); 
        tmp.y -= gap;
    }

    g_zpData.area = tmp;

    return TRUE;
}

static void cal_zoom_box_position(GdkRectangle *rect, gdouble x, gdouble y)
{
    rect->x = x - (rect->width/2);
    rect->y = y - (rect->height/2);

    //g_message("%s : x %d  y %d --> new x %d , new y %d ::::::::::::::::::::::", __FUNCTION__, x, y, rect->x, rect->y);

    if(rect->x <= 0)                                 
        rect->x = ZOOM_BOX_POS_X(ZOOM_MAX_STEP);

    if((rect->x + rect->width) >= ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP))  
        rect->x -= ((rect->x + rect->width) - ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP));

    if(rect->y <= 0)                                 
        rect->y = ZOOM_BOX_POS_Y(ZOOM_MAX_STEP);

    if((rect->y + rect->height) >= ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP)) 
        rect->y -= ((rect->y + rect->height) - ZOOM_BOX_SIZE_H(ZOOM_MAX_STEP));
}

static void draw_init_zoom_box()
{
    erase_zoom_box();
    init_zoom_box();
    draw_zoom_box();
}

static void draw_zoom_box()
{
    GdkDrawable *drawable;
    GdkColor line_color = UX_COLOR(386);
    GdkGC *line_gc;

    drawable = nfui_nfobject_get_window((NFOBJECT*)g_zpArea);
    line_gc = nfui_nfobject_get_gc((NFOBJECT*)g_zpArea);
    gdk_gc_set_rgb_fg_color(line_gc, &line_color);

    gdk_gc_set_line_attributes(line_gc,
            ZOOM_BOX_LINE_SIZE,
            GDK_LINE_SOLID,
            GDK_CAP_NOT_LAST,
            GDK_JOIN_BEVEL);

    gdk_draw_rectangle(drawable,
            line_gc,
            FALSE,
            g_zpData.area.x, g_zpData.area.y,
            g_zpData.area.width, g_zpData.area.height);

    nfui_nfobject_gc_unref(line_gc);
}

static void erase_zoom_box()
{
    GdkDrawable *drawable;
    GdkGC *line_gc;

    drawable = nfui_nfobject_get_window((NFOBJECT*)g_zpArea);
    line_gc = nfui_nfobject_get_gc((NFOBJECT*)g_zpArea);
    gdk_gc_set_rgb_fg_color(line_gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));

    gdk_gc_set_line_attributes(line_gc,
            ZOOM_BOX_LINE_SIZE,
            GDK_LINE_SOLID,
            GDK_CAP_NOT_LAST,
            GDK_JOIN_BEVEL);

    gdk_draw_rectangle(drawable,
            line_gc,
            FALSE,
            g_zpData.area.x, g_zpData.area.y,
            g_zpData.area.width, g_zpData.area.height);

    nfui_nfobject_gc_unref(line_gc);
}

static gboolean move_zoom_box(ZoomMoveDir dir, gint x, gint y)
{
    gboolean ret = TRUE;

    erase_zoom_box();

    ret = cal_zoom_box_coord(dir, x, y);
    draw_zoom_box();

    return ret;
}

static gboolean press_zoom_box(gdouble x, gdouble y)
{
    GdkRectangle  rect;
    gboolean ret = FALSE;

    if(g_zpData.step == ZOOM_MAX_STEP) 
        return FALSE;

    erase_zoom_box();

    rect = g_zpData.area;
    cal_zoom_box_position(&rect, x, y);

    //g_message("::::::::::::::::::::::  %d %d %d %d", rect.x, rect.y, ZOOM_BOX_POS_X(ZOOM_MAX_STEP), ZOOM_BOX_POS_Y(ZOOM_MAX_STEP));
    g_zpData.px = x;
    g_zpData.py = y;
    g_zpData.area = rect;

    ret = TRUE;

    draw_zoom_box();
    
    return ret;
}

static gboolean resize_zoom_box(gint resize)
{
    gboolean ret = TRUE;

    erase_zoom_box();

    ret = cal_zoom_box_size(resize);
    //g_message("%s :  returns %s ", __FUNCTION__, (ret ? "TRUE":"FALSE"));

    draw_zoom_box();

    return ret;
}

static gboolean is_movable_zoom_box(gint x, gint y)
{
    if(g_zpData.area.x <= x && (g_zpData.area.x + g_zpData.area.width) >= x) {
        if(g_zpData.area.y <= y && (g_zpData.area.y + g_zpData.area.height) >= y) {
            return TRUE;
        }
    }

    return FALSE;
}

static ZoomMoveDir get_mouse_move_dir(gint x, gint y)
{
    ZoomMoveDir dir = ZOOM_DIR_NONE;
    ZoomMoveDir x_dir = ZOOM_DIR_NONE;
    ZoomMoveDir y_dir = ZOOM_DIR_NONE;
    gint cur_x = 0, cur_y = 0;
    gint move_x = 0, move_y = 0;

    if(g_zpData.step == ZOOM_MAX_STEP) 
        return dir;
    
    if(g_zpData.area.width == ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP)
            && g_zpData.area.height == ZOOM_BOX_SIZE_W(ZOOM_MAX_STEP))
        return dir;

    cur_x = g_zpData.area.x + (g_zpData.area.width/2);
    cur_y = g_zpData.area.y + (g_zpData.area.height/2);

    move_x = cur_x - x;
    move_y = cur_y - y;

    if(move_x < 0)       x_dir = ZOOM_DIR_RIGHT;
    else if(move_x > 0)  x_dir = ZOOM_DIR_LEFT;
    
    if(move_y < 0)       y_dir = ZOOM_DIR_DOWN;
    else if(move_y > 0)  y_dir = ZOOM_DIR_UP;   
    
    if(move_x < 0)       move_x *= -1;
    if(move_y < 0)       move_y *= -1;  

    if(move_x > move_y)      dir = x_dir;
    else if(move_x < move_y) dir = y_dir;   
#if 0       
    if(dir == 0)g_message("%s 3>>>>>>>>>>>>>>>> move NONE move_x :%d move_y : %d ", __FUNCTION__, move_x, move_y);
    if(dir == 1)g_message("%s 3>>>>>>>>>>>>>>>> move LEFT move_x :%d move_y : %d ", __FUNCTION__, move_x, move_y);
    if(dir == 2)g_message("%s 3>>>>>>>>>>>>>>>> move RIGHT move_x :%d move_y : %d ", __FUNCTION__,move_x, move_y);
    if(dir == 3)g_message("%s 3>>>>>>>>>>>>>>>> move UP move_x :%d move_y : %d ", __FUNCTION__, move_x, move_y);
    if(dir == 4)g_message("%s 3>>>>>>>>>>>>>>>> move DOWN move_x :%d move_y : %d ", __FUNCTION__, move_x, move_y);
#endif

    return dir;
}

static gboolean zoom_cmd(ZoomCmd cmd)
{
    gint pos_sx = 0, pos_sy = 0;
    gint pos_ex = 0, pos_ey = 0;
    gint pos_nx = 0, pos_ny = 0;
    gint size_w = 0, size_h = 0;
    gint ret = 0;

    if(cmd >= ZOOM_CMD_ZOOMIN) {
        if(live_zoom_mode()) {
            pos_sx = nf_live_zoom_get_pos_sx(); 
            pos_sy = nf_live_zoom_get_pos_sy();

            pos_ex = nf_live_zoom_get_pos_ex(); 
            pos_ey = nf_live_zoom_get_pos_ey();

            size_w = nf_live_zoom_get_pos_dx(); 
            size_h = nf_live_zoom_get_pos_dy();
        }else {
            pos_sx = nf_play_zoom_get_pos_sx(); 
            pos_sy = nf_play_zoom_get_pos_sy();

            pos_ex = nf_play_zoom_get_pos_ex(); 
            pos_ey = nf_play_zoom_get_pos_ey();

            size_w = nf_play_zoom_get_pos_dx(); 
            size_h = nf_play_zoom_get_pos_dy();
        }
    }

    switch(cmd) {
        case ZOOM_CMD_START:
            {
                // TODO: set mode
                if(live_zoom_mode()) {
                    ret = nf_live_zoom_start(g_zpData.ch, 
                                            ZOOM_MOVE_POS_X(ZOOM_INIT_STEP), 
                                            ZOOM_MOVE_POS_Y(ZOOM_INIT_STEP), 
                                            ZOOM_MOVE_SIZE_W(ZOOM_INIT_STEP), 
                                            ZOOM_MOVE_SIZE_H(ZOOM_INIT_STEP), 
                                            ZOOM_PIP_POS_X, 
                                            ZOOM_PIP_POS_Y, 
                                            ZOOM_AREA_SIZE_W,
                                            ZOOM_AREA_SIZE_H);

                    /*
                    g_message("%s >>>>> zbox ch %d\nx: %d\ny:%d\nw: %d\nh: %d\npx: %d\npy: %d\npw: %d\nph: %d\n >>>>>",
                            __FUNCTION__,
                            g_zpData.ch,
                            ZOOM_MOVE_POS_X(ZOOM_INIT_STEP), 
                            ZOOM_MOVE_POS_Y(ZOOM_INIT_STEP), 
                            ZOOM_MOVE_SIZE_W(ZOOM_INIT_STEP), 
                            ZOOM_MOVE_SIZE_H(ZOOM_INIT_STEP), 
                            ZOOM_PIP_POS_X, 
                            ZOOM_PIP_POS_Y, 
                            ZOOM_AREA_SIZE_W,
                            ZOOM_AREA_SIZE_H);
                    */
                }else {
                    ret = nf_play_zoom_start(g_zpData.ch, 
                                            ZOOM_MOVE_POS_X(ZOOM_INIT_STEP), 
                                            ZOOM_MOVE_POS_Y(ZOOM_INIT_STEP), 
                                            ZOOM_MOVE_SIZE_W(ZOOM_INIT_STEP), 
                                            ZOOM_MOVE_SIZE_H(ZOOM_INIT_STEP), 
                                            ZOOM_PIP_POS_X, 
                                            ZOOM_PIP_POS_Y, 
                                            ZOOM_AREA_SIZE_W,
                                            ZOOM_AREA_SIZE_H);
                }

//g_message("%s : ZOOM START : %d %d %d %d ", __FUNCTION__, ZOOM_MOVE_POS_X(ZOOM_INIT_STEP), ZOOM_MOVE_POS_Y(ZOOM_INIT_STEP), ZOOM_MOVE_SIZE_W(ZOOM_INIT_STEP), ZOOM_MOVE_SIZE_H(ZOOM_INIT_STEP));
            }
            return ret;

        case ZOOM_CMD_STOP:
            if(live_zoom_mode()) ret = nf_live_zoom_stop();
            else                 ret = nf_play_zoom_stop();
            return ret;

        case ZOOM_CMD_CHANGE_CH:
            if(live_zoom_mode()) ret = nf_live_zoom_channel_change(g_zpData.ch);
            else                 ret = FALSE;
            return  ret;

        case ZOOM_CMD_ZOOMIN:
            {
                pos_nx = g_zpData.area.x * COORD_RATIO;
                pos_ny = g_zpData.area.y * COORD_RATIO;

                pos_sx += (pos_nx - pos_sx);
                pos_sy += (pos_ny - pos_sy);
                size_w -= ZOOM_MOVE_W_STEP;
                size_h -= ZOOM_MOVE_H_STEP;

                //g_message("%s >>>>> zbox in real x: %d y:%d w: %d h: %d",
                //      __FUNCTION__,
                //      pos_sx, pos_sy,
                //      size_w, size_h);    
            }
            break;

        case ZOOM_CMD_ZOOMOUT:
            {
                pos_nx = g_zpData.area.x * COORD_RATIO;
                pos_ny = g_zpData.area.y * COORD_RATIO;

                pos_sx -= (pos_sx - pos_nx);
                pos_sy -= (pos_sy - pos_ny);
                size_w += ZOOM_MOVE_W_STEP;
                size_h += ZOOM_MOVE_H_STEP;

                if(size_w >= DISPLAY_ACTIVE_WIDTH)  pos_sx = 0;
                if(size_h >= DISPLAY_ACTIVE_HEIGHT) pos_sy = 0;

                if((pos_sx + size_w) > DISPLAY_ACTIVE_WIDTH)
                    size_w -= (pos_sx + size_w - DISPLAY_ACTIVE_WIDTH);
                if((pos_sy + size_h) > DISPLAY_ACTIVE_HEIGHT)
                    size_h -= (pos_sy + size_h - DISPLAY_ACTIVE_HEIGHT);

                //g_message("%s >>>>> zbox out real x: %d y:%d w: %d h: %d",
                //      __FUNCTION__,
                //      pos_sx, pos_sy,
                //      size_w, size_h);    
            }
            break;

        case ZOOM_CMD_MOVE_LEFT:
            {
#if 0
                if(pos_sx == 0) 
                    return FALSE;

                pos_sx -= ZOOM_MOVE_X_STEP; 
#endif
                pos_nx = g_zpData.area.x * COORD_RATIO;

                pos_sx -= (pos_sx - pos_nx);
            }
            break;

        case ZOOM_CMD_MOVE_RIGHT:
            {
#if 0
                if(pos_ex == DISPLAY_ACTIVE_WIDTH) 
                    return FALSE;

                pos_sx += ZOOM_MOVE_X_STEP;
#endif
                pos_nx = g_zpData.area.x * COORD_RATIO;

                pos_sx += (pos_nx - pos_sx);
            }
            break;

        case ZOOM_CMD_MOVE_UP:
            {
#if 0
                if(pos_sy == 0) 
                    return FALSE;

                pos_sy -= ZOOM_MOVE_Y_STEP;
#endif
                pos_ny = g_zpData.area.y * COORD_RATIO;

                pos_sy -= (pos_sy - pos_ny);
            }
            break;

        case ZOOM_CMD_MOVE_DOWN:
            {
#if 0
                if(pos_ey == DISPLAY_ACTIVE_HEIGHT) 
                    return FALSE;

                pos_sy += ZOOM_MOVE_Y_STEP;
#endif
                pos_ny = g_zpData.area.y * COORD_RATIO;

                pos_sy += (pos_ny - pos_sy);
            }
            break;

        case ZOOM_CMD_JUMP:
        {
            pos_nx = (g_zpData.px * COORD_RATIO) - ((g_zpData.area.width/2) * COORD_RATIO);
            pos_ny = (g_zpData.py * COORD_RATIO) - ((g_zpData.area.height/2) * COORD_RATIO);
            
            // new pos x
            if(pos_sx > pos_nx) pos_sx -= (pos_sx - pos_nx);
            else                pos_sx += (pos_nx - pos_sx);

            if(pos_sx < 0) pos_sx = 0;
            if((pos_sx + size_w) > DISPLAY_ACTIVE_WIDTH) pos_sx -= ((pos_sx + size_w) - DISPLAY_ACTIVE_WIDTH);

            // new pos y
            if(pos_sy > pos_ny) pos_sy -= (pos_sy - pos_ny);
            else                pos_sy += (pos_ny - pos_sy);

            if(pos_sy < 0) pos_sy = 0;
            if((pos_sy + size_h) > DISPLAY_ACTIVE_HEIGHT) pos_sy -= ((pos_sy + size_h) - DISPLAY_ACTIVE_HEIGHT);
        }
        break;

        default:
            return FALSE;
    }
    //g_message("%s [DOWN]:::::::::::: start x: %d start y: [%d] end x: %d end y: %d size w: %d size h: %d ", __FUNCTION__, pos_sx, pos_sy, pos_ex, pos_ey, size_w, size_h);

    if(live_zoom_mode()) ret = nf_live_zoom_move(pos_sx, pos_sy, size_w, size_h);
    else                 ret = nf_play_zoom_move(pos_sx, pos_sy, size_w, size_h);
    
    if(ret) return TRUE;
    else    return FALSE;
}

static gint _live_pip_show_callback(gpointer data)
{
    NFOBJECT *ch_obj = (NFOBJECT*)data;
    gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)ch_obj);
    
    g_message("%s, %d", __FUNCTION__, __LINE__);

    nf_live_pip_show();
    return 0;
}

static gint _live_pip_hide_callback(gpointer data)
{
    NFOBJECT *ch_obj = (NFOBJECT*)data;
    gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)ch_obj);

    g_message("%s, %d", __FUNCTION__, __LINE__);

    nf_live_pip_hide();
    return 0;
}

static gint _playback_pip_show_callback(gpointer data)
{
    NFOBJECT *ch_obj = (NFOBJECT*)data;
    gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)ch_obj);
    
    g_message("%s, %d", __FUNCTION__, __LINE__);

    nf_play_pip_show(ch);        
    return 0;
}

static gint _playback_pip_hide_callback(gpointer data)
{
    NFOBJECT *ch_obj = (NFOBJECT*)data;
    gint ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)ch_obj);

    g_message("%s, %d", __FUNCTION__, __LINE__);

    nf_play_pip_hide(ch);        
    return 0;
}

static gboolean post_zp_area_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
        case GDK_EXPOSE:
            draw_zoom_box();
            break;

        case GDK_ENTER_NOTIFY:
            break;

        case GDK_LEAVE_NOTIFY:
            break;

        case GDK_MOTION_NOTIFY:
            {
                GdkEventMotion *mevt;
                ZoomMoveDir dir;

                mevt = (GdkEventMotion*)evt;
                if(!(mevt->state & GDK_BUTTON1_MASK))
                    break;
#if 0
                if(!is_movable_zoom_box((gint)mevt->x, (gint)mevt->y))
                    break;
#endif

                dir = get_mouse_move_dir((gint)mevt->x, (gint)mevt->y);
                if(dir == ZOOM_DIR_NONE)  break;

                if(!move_zoom_box(dir, (gint)mevt->x, (gint)mevt->y))
                    break;

                if(dir == ZOOM_DIR_LEFT)       zoom_cmd(ZOOM_CMD_MOVE_LEFT);
                else if(dir == ZOOM_DIR_RIGHT) zoom_cmd(ZOOM_CMD_MOVE_RIGHT);
                else if(dir == ZOOM_DIR_UP)    zoom_cmd(ZOOM_CMD_MOVE_UP);
                else if(dir == ZOOM_DIR_DOWN)  zoom_cmd(ZOOM_CMD_MOVE_DOWN);
            }
            break;
        
        case GDK_BUTTON_PRESS:
            {
                GdkEventButton *bevt; 

                if(evt->button.button == MOUSE_RIGTH_BUTTON 
                        || evt->button.button == MOUSE_MIDDLE_BUTTON) 
                    return FALSE;

                bevt = (GdkEventButton *)evt;
                if(!press_zoom_box(bevt->x, bevt->y))
                    break;

                zoom_cmd(ZOOM_CMD_JUMP);
            }
            break;
            

        case GDK_SCROLL:
            {
                GdkEventScroll *sevt;

                sevt = (GdkEventScroll*)evt;
                if(sevt->direction == GDK_SCROLL_UP) {
                    if(!resize_zoom_box(ZOOM_BOX_DECREASE)) 
                        break;

                    //g_message("%s : ZOOM IN STEP %d ", __FUNCTION__, g_zpData.step);

                    if(!zoom_cmd(ZOOM_CMD_ZOOMIN))
                        g_message("<%s> not action!", __FUNCTION__);
                }
                else if(sevt->direction == GDK_SCROLL_DOWN) {
                    if(!resize_zoom_box(ZOOM_BOX_INCREASE)) 
                        break;

                    //g_message("%s : ZOOM OUT STEP %d ", __FUNCTION__, g_zpData.step);

                    if(!zoom_cmd(ZOOM_CMD_ZOOMOUT))
                        g_message("<%s> not action!", __FUNCTION__);
                }
            }
            break;

        case GDK_DELETE:
            {
//              memset(&g_zpData, 0x00, sizeof(ZOOMPIP_DATA));
            }
            break;

        default:
            break;
    }

    return FALSE;
}

static gboolean post_zoom_out_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        if(resize_zoom_box(ZOOM_BOX_INCREASE)) {
            if(!zoom_cmd(ZOOM_CMD_ZOOMOUT))
                g_message("<%s> not action!", __FUNCTION__);
                return FALSE;
        }
    }

    return FALSE;
}

static gboolean post_zoom_in_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;

        if(resize_zoom_box(ZOOM_BOX_DECREASE)) {
            if(!zoom_cmd(ZOOM_CMD_ZOOMIN))
            {
                g_message("<%s> not action!", __FUNCTION__);
                return FALSE;
            }
        }
    }

    return FALSE;
}

static gboolean post_ch_combo_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {
        gint ch;
        guint prop;

        ch = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
        prop = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_zpWin, "pip property"));

        if(vsm_get_covert_state(NULL, ch)) {
            nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);

            nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, g_zpData.ch);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            return FALSE;
        }

        // reset 
        if(g_zpData.ch == ch) return FALSE;

        draw_init_zoom_box();

        set_zoom_ch(ch);
        if(!(prop & ZOOM_PIP_LIVE))
            vsm_change_sfc_by_zoom_changed(ch);

        if(!zoom_cmd(ZOOM_CMD_CHANGE_CH))
        {
            g_message("<%s> not action!", __FUNCTION__);
            return FALSE;
        }
    }

    return FALSE;
}

static gboolean post_zp_sbar_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_EXPOSE) {
        GdkDrawable *drawable = NULL;
        GdkGC *gc = NULL;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfutil_draw_image(drawable, gc, IMG_ZOOM_CTRL_BG, (gint)obj->x, (gint)obj->y, (gint)obj->width, (gint)obj->height, NFALIGN_LEFT, 0);
    
        nfui_nfobject_gc_unref(gc);
    }

    return FALSE;
}

static gboolean post_exit_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE){
        zoom_cmd(ZOOM_CMD_STOP);
        nfui_nfobject_destroy(g_zpWin);
    }

    return FALSE;
}

static gboolean post_zp_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
        default:
            break;
    }

    return FALSE;
}

static gboolean post_zp_win_event(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    switch(evt->type) {
        case NFEVENT_KEYPAD_PRESS:
        case NFEVENT_REMOCON_PRESS:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;
                NFOBJECT *top;
                gint ret = 0;

                kevt = (GdkEventKey*)evt;
                kpid = (KEYPAD_KID)kevt->keyval;

                if (GET_CHANNEL_KEY_SCOPE(kpid))
                {
                    if (kpid >= GUI_CHANNEL_CNT)
                        break;

                    if (playback_zoom_mode())
                        break;

                    nfui_combobox_set_index((NFCOMBOBOX*)g_combo, kpid);
                    nfui_signal_emit(g_combo, GDK_EXPOSE, TRUE);
                }
                else
                {
                    switch(kpid) {
                        case KEYPAD_FF:
                        case RMC_ZOOMIN:
                            {
                                if(!resize_zoom_box(ZOOM_BOX_DECREASE)) 
                                    break;

                                if(!zoom_cmd(ZOOM_CMD_ZOOMIN))
                                {
                                    g_message("<%s> not action!", __FUNCTION__);
                                    return FALSE;
                                }
                            }
                            break;

                        case KEYPAD_REW:
                        case RMC_ZOOMOUT:
                            {
                                if(!resize_zoom_box(ZOOM_BOX_INCREASE)) 
                                    break;

                                if(!zoom_cmd(ZOOM_CMD_ZOOMOUT))
                                {
                                    g_message("<%s> not action!", __FUNCTION__);
                                    return FALSE;
                                }
                            }
                            break;
                        case KEYPAD_UP:
                            {
                                if(!move_zoom_box(ZOOM_DIR_UP, -1, -1))
                                    break;

                                if(!zoom_cmd(ZOOM_CMD_MOVE_UP))
                                {
                                    g_message("<%s> not action!", __FUNCTION__);
                                    return FALSE;
                                }
                            }
                            break;

                        case KEYPAD_DOWN:
                            {
                                if(!move_zoom_box(ZOOM_DIR_DOWN, -1, -1))
                                    break;
                                
                                if(!zoom_cmd(ZOOM_CMD_MOVE_DOWN))
                                {
                                    g_message("<%s> not action!", __FUNCTION__);
                                    return FALSE;
                                }
                            }
                            break;

                        case KEYPAD_LEFT:
                            {
                                if(!move_zoom_box(ZOOM_DIR_LEFT, -1, -1))
                                    break;

                                if(!zoom_cmd(ZOOM_CMD_MOVE_LEFT))
                                {
                                    g_message("<%s> not action!", __FUNCTION__);
                                    return FALSE;
                                }
                            }
                            break;

                        case KEYPAD_RIGHT:
                            {
                                if(!move_zoom_box(ZOOM_DIR_RIGHT, -1, -1))
                                    break;

                                if(!zoom_cmd(ZOOM_CMD_MOVE_RIGHT))
                                {
                                    g_message("<%s> not action!", __FUNCTION__);
                                    return FALSE;
                                }
                            }
                            break;  
                        case KEYPAD_EXIT:
                            return TRUE;

                        default:
                            break;
                    }
                }
            }
            break;

        case NFEVENT_KEYPAD_RELEASE:
        case NFEVENT_REMOCON_RELEASE:
            {
                GdkEventKey *kevt;
                KEYPAD_KID kpid;

                kevt = (GdkEventKey*)evt;
                kpid = (KEYPAD_KID)kevt->keyval;

                if(kpid == KEYPAD_EXIT)
                {
                    zoom_cmd(ZOOM_CMD_STOP);
                    nfui_nfobject_destroy(obj);

                    return TRUE;
                }
            }
            break;

        case GDK_DELETE:
            {
                if (g_osddata.zoompip) vw_autohide_unset_obj(g_zpWin);

                if(g_zpWin != NULL)
                    g_zpWin = NULL;

                g_curwnd = 0;
                gtk_main_quit();
            }
            break;

        default:
            break;
    }
    return FALSE;
}


gint VW_ZoomPIP_Open(NFWINDOW *parent, gint x, gint y, guint ch, ZoomPIPProp property)
{
    NFOBJECT *zpFixed;
    NFOBJECT *zpSBar;
    NFOBJECT *obj;

    GdkPixbuf *zi_img[NFOBJECT_STATE_COUNT];
    GdkPixbuf *zo_img[NFOBJECT_STATE_COUNT];

    guint prop = property;
    gint pos_x, pos_y, size_w, size_h;
    gint cam_ch = var_get_ch_count();
    gint index;
    gint i;
    gchar camTitle[STRING_SIZE_CAMTITLE+8];

    if(g_zpWin)
        return FALSE;

    // init zoom pip data
    memset(&g_zpData, 0x00, sizeof(ZOOMPIP_DATA));

    init_zoom_box();

    if (strcmp(parent->strTitle, "PTZ CTRL") == 0)
    {
        set_video_mode(VMODE_LV);
    }
    else
    {
        set_video_mode(vsm_get_vmode());
    }
    
    
    /* window */
    g_zpWin = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, ZOOM_PIP_SIZE_W, ZOOM_PIP_SIZE_H);
    g_curwnd = g_zpWin;
    nfui_nfwindow_set_title(g_zpWin, "DIGITAL ZOOM");
    nfui_nfobject_modify_bg(g_zpWin, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    gtk_widget_add_events(((NFWINDOW*)g_zpWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);
    //gtk_widget_add_events(((NFWINDOW*)g_zpWin)->main_widget, GDK_ALL_EVENTS_MASK);
    nfui_regi_post_event_callback(g_zpWin, post_zp_win_event);
    nfui_nfobject_set_data(g_zpWin, "pip property", GUINT_TO_POINTER(prop));

    /* fixed */
    zpFixed = (NFOBJECT*)nfui_nffixed_new();
    //nfui_regi_post_event_callback(zpFixed, post_zp_fixed_event_cb);
    nfui_nfobject_set_size(zpFixed, ZOOM_PIP_SIZE_W, ZOOM_PIP_SIZE_H);
    nfui_nfobject_show(zpFixed);

    g_zpArea = (NFOBJECT*)nfui_nffixed_new();
    nfui_regi_post_event_callback(g_zpArea, post_zp_area_event_cb);
    nfui_nfobject_use_focus(g_zpArea, TRUE);
    nfui_nfobject_set_size(g_zpArea, ZOOM_AREA_SIZE_W, ZOOM_AREA_SIZE_H);
    nfui_nfobject_show(g_zpArea);
    nfui_nffixed_put((NFFIXED*)zpFixed, g_zpArea, 0, 0);

    zpSBar = (NFOBJECT*)nfui_nffixed_new();
    nfui_regi_post_event_callback(zpSBar, post_zp_sbar_event_cb);
    nfui_nfobject_set_size(zpSBar, ZOOM_BAR_SIZE_W, ZOOM_BAR_SIZE_H);
    nfui_nfobject_show(zpSBar);
    nfui_nffixed_put((NFFIXED*)zpFixed, zpSBar, 0, (ZOOM_PIP_SIZE_H - ZOOM_BAR_SIZE_H));


    /* combo */
    obj = nfui_combobox_new(NULL, 0, ch);
    nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
    nfui_nfobject_set_size(obj, 255, 40);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_regi_post_event_callback(obj, post_ch_combo_event_cb);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)zpSBar, obj, 10, 14);
    g_combo = obj;

    
    // set cam title
    for(i=0; i<cam_ch; i++) {
        index = sprintf(camTitle, "CH%d - ", i+1);

        //DAL_get_camera_title(&camTitle[index] , (guint)i);
        var_get_camtitle(&camTitle[index] , (guint)i);

        nfui_combobox_append_data((NFCOMBOBOX*)obj, camTitle);
    }
    nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, ch);

    if(playback_zoom_mode()) nfui_nfobject_disable(obj);

    pos_x = (10 + 255 + 8);
    pos_y = 14;

    /* button */
    zo_img[0] = nfui_get_image_from_file((IMG_N_ZOOM_OUT_BG), NULL);
    zo_img[1] = nfui_get_image_from_file((IMG_O_ZOOM_OUT_BG), NULL);
    zo_img[2] = nfui_get_image_from_file((IMG_P_ZOOM_OUT_BG), NULL);
    zo_img[3] = nfui_get_image_from_file((IMG_D_ZOOM_OUT_BG), NULL);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image((NFBUTTON*)obj, zo_img);
    nfui_regi_post_event_callback(obj, post_zoom_out_event_cb);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)zpSBar, obj, pos_x, pos_y);

    zi_img[0] = nfui_get_image_from_file((IMG_N_ZOOM_IN_BG), NULL);
    zi_img[1] = nfui_get_image_from_file((IMG_O_ZOOM_IN_BG), NULL);
    zi_img[2] = nfui_get_image_from_file((IMG_P_ZOOM_IN_BG), NULL);
    zi_img[3] = nfui_get_image_from_file((IMG_D_ZOOM_IN_BG), NULL);

    nfui_get_pixbuf_size(zi_img[0], &size_w, &size_h);
    pos_x += (size_w + 1);

    obj = (NFOBJECT*)nfui_nfbutton_new();
    nfui_nfbutton_set_image((NFBUTTON*)obj, zi_img);
    nfui_regi_post_event_callback(obj, post_zoom_in_event_cb);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)zpSBar, obj, pos_x, pos_y);


    pos_x += (size_w + 8);

    obj = nftool_normal_button_create_type2("EXIT", 112);
    nfui_regi_post_event_callback(obj, post_exit_button_event_cb);
    nfui_nfobject_use_focus(obj, FALSE);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)zpSBar, obj, pos_x, pos_y);


    nfui_nfwindow_add((NFWINDOW*)g_zpWin, zpFixed);
    nfui_run_main_event_handler(g_zpWin);

    // property
    if(property & ZOOM_PIP_MODALESS) 
        gtk_window_set_modal(GTK_WINDOW(((NFWINDOW*)g_zpWin)->main_widget), FALSE);

    if(property & ZOOM_PIP_MOVING) {
        nfui_nfwindow_set_moving_area_pos_y((NFWINDOW*)g_zpWin, (ZOOM_PIP_SIZE_H - ZOOM_BAR_SIZE_H));
        nfui_nfwindow_set_moving_area_size((NFWINDOW*)g_zpWin, ZOOM_BAR_SIZE_H);
    }

    nfui_nfobject_show(g_zpWin);
    nfui_make_key_hierarchy((NFWINDOW*)g_zpWin);
//  nfui_set_key_focus(obj, TRUE);  

    if (strcmp(parent->strTitle, "PTZ CTRL") == 0)
    {
        if(!(property & ZOOM_PIP_LIVE)) vsm_change_sfc_by_zoom_opened_ptz(ch);
    }
    else
    {
        if(!(property & ZOOM_PIP_LIVE)) vsm_change_sfc_by_zoom_opened(ch);
    }

    set_zoom_ch((gint)ch);
    zoom_cmd(ZOOM_CMD_START);

    memset(&g_osddata, 0x00, sizeof(OsdData));
    DAL_get_osd_data(&g_osddata);

    if(live_zoom_mode()) 
    {
        if (g_osddata.zoompip)
        {
            vw_autohide_set_obj(g_zpWin, g_osddata.zoompip);
            vw_autohide_set_show_callback(_live_pip_show_callback, (gpointer)g_combo);
            vw_autohide_set_hide_callback(_live_pip_hide_callback, (gpointer)g_combo);
        }
        smt_set_service(SMT_LIVE_ZOOM);
    }
    else
    {
        if (g_osddata.zoompip)
        {
            vw_autohide_set_obj(g_zpWin, g_osddata.zoompip);
            vw_autohide_set_show_callback(_playback_pip_show_callback, (gpointer)g_combo);
            vw_autohide_set_hide_callback(_playback_pip_hide_callback, (gpointer)g_combo);
        }

        smt_set_service(SMT_PLAYBACK_ZOOM);
    }
    nfui_page_open(PGID_DIGIZOOM, g_zpWin, ssm_get_cur_id(NULL));

    gtk_main();

    if (strcmp(parent->strTitle, "PTZ CTRL") == 0)
    {
        if(!(property & ZOOM_PIP_LIVE)) vsm_recover_sfc_by_zoom_closed_ptz();
    }
    else
    {
        if(!(property & ZOOM_PIP_LIVE)) vsm_recover_sfc_by_zoom_closed();
    }


    nfui_page_close(PGID_DIGIZOOM, g_zpWin);

    if(live_zoom_mode()) 
        smt_set_service(SMT_LIVE);  
    else
        smt_set_service(SMT_PLAYBACK);
    
    return g_zpData.ch;
}

void VW_ZoomPIP_Show()
{
    g_return_if_fail(g_zpWin != NULL); 

    nfui_nfobject_show(g_zpWin);
    nfui_make_key_hierarchy((NFWINDOW*)g_zpWin);
}

void VW_ZoomPIP_Move(guint x, guint y)
{
    guint prop;

    g_return_if_fail(g_zpWin != NULL); 

    prop = GPOINTER_TO_UINT(nfui_nfobject_get_data(g_zpWin, "pip property"));
    if(!(prop & ZOOM_PIP_MOVING))
        return;

    nfui_nfobject_move(g_zpWin, x, y);

    nfui_nfobject_show(g_zpWin);
    nfui_make_key_hierarchy((NFWINDOW*)g_zpWin);
}

void VW_ZoomPIP_Hide()
{
    g_return_if_fail(g_zpWin != NULL); 

    nfui_nfobject_hide(g_zpWin);
}

gboolean VW_ZoomPIP_Is_Shown()
{
    g_return_val_if_fail(g_zpWin != NULL, FALSE); 

    return nfui_nfobject_is_shown(g_zpWin);
}

/*inline*/ guint VW_ZoomPIP_Width()
{
    return ZOOM_PIP_SIZE_W;
}

/*inline*/ guint VW_ZoomPIP_Height()
{
    return ZOOM_PIP_SIZE_H;
}

void VW_ZoomPIP_Destroy()
{
    if (g_curwnd) 
    {
        zoom_cmd(ZOOM_CMD_STOP);
        nfui_nfobject_destroy(g_curwnd);
    }
}
