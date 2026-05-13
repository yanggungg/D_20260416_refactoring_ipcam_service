#include <math.h>

#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/event_loop.h"
#include "support/nf_ui_page_manager.h"
#include "support/util.h"

#include "tools/nf_ui_function.h"
#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"

#include "vw_live_ptz_main.h"
#include "vw_live_ptz_internal.h"
#include "vw_vkeyboard_num.h"
#include "vw_live_ptz_rmc_prst.h"
#include "vw_zoom_pip.h"

#include "ix_mem.h"
#include "nfdal.h"
#include "vsm.h"
#include "ssm.h"
#include "smt.h"
#include "scm.h"
#include "vw.h"


#define D_SEC                   (3)

#define VIDEO_SIZE_W            (1488)
#define VIDEO_SIZE_H            (848)

#define VIDEO_FIXED_POS_X       (1920-VIDEO_SIZE_W)
#define VIDEO_FIXED_POS_Y       (0)
#define VIDEO_FIXED_W		    (VIDEO_SIZE_W)
#define VIDEO_FIXED_H		    (VIDEO_SIZE_H)

#define VIDEO_CENTER_POS_X      (VIDEO_SIZE_W/2 + VIDEO_FIXED_POS_X)
#define VIDEO_CENTER_POS_Y      (VIDEO_SIZE_H/2 + VIDEO_FIXED_POS_Y)

#define PRST_FIXED_POS_X        (0)
#define PRST_FIXED_POS_Y        (0)
#define PRST_FIXED_W	    	(432)
#define PRST_FIXED_H	    	(1080)

#define CTRL_FIXED_POS_X        (PRST_FIXED_W)
#define CTRL_FIXED_POS_Y        (VIDEO_SIZE_H)
#define CTRL_FIXED_W		    (570)
#define CTRL_FIXED_H		    (243)

#define STATUS_FIXED_POS_X      (CTRL_FIXED_POS_X+CTRL_FIXED_W)
#define STATUS_FIXED_POS_Y      (VIDEO_SIZE_H)
#define STATUS_FIXED_W		    (918)
#define STATUS_FIXED_H		    (243)

#define ZOOM_SPEED              (90)


#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)    (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)    (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)    (kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#endif

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_video_obj;
static guint g_ch;

static PtzData g_ptzData[GUI_CHANNEL_CNT];
static PtzData g_org_ptzData[GUI_CHANNEL_CNT];

static PtzPresetData g_presetData[GUI_CHANNEL_CNT];
static PtzPresetData g_org_presetData[GUI_CHANNEL_CNT];

static PtzScanData g_scanData[GUI_CHANNEL_CNT];
static PtzScanData g_org_scanData[GUI_CHANNEL_CNT];

static PtzTourData g_tourData[GUI_CHANNEL_CNT];
static PtzTourData g_org_tourData[GUI_CHANNEL_CNT];

static EA_AlmSenData g_asd[ALARM_IN_COUNT];
static EA_AlmSenData g_org_asd[ALARM_IN_COUNT];

static EA_MotSenData g_msd[GUI_CHANNEL_CNT];
static EA_MotSenData g_org_msd[GUI_CHANNEL_CNT];

static EA_VLossData g_vld[GUI_CHANNEL_CNT];
static EA_VLossData g_org_vld[GUI_CHANNEL_CNT];

static EA_PosData g_psd[GUI_CHANNEL_CNT];
static EA_PosData g_org_psd[GUI_CHANNEL_CNT];

static GdkPoint arrow[3];
static GdkPoint line[2];
static gint cur_direction =0;
static gint cur_speed =0;
static gint cur_scroll = 0;
static gint cur_drag = 0;
static gint zoom_sec = -1;
static gint drag_sec = D_SEC;
static gint g_alarmIn = ALARM_IN_COUNT;


// 0 : move object, 1 : control pan-tilt
static gint g_ptz_ctrl_mode = 0;

// 0 : none, 1 : +10
static gint g_keypad_ch_prefix = 0;

enum {
    DIRECTION_NONE =0,
    DIRECTION_RIGHT,
    DIRECTION_RIGHT_UP,
    DIRECTION_UP,
    DIRECTION_LEFT_UP,
    DIRECTION_LEFT,
    DIRECTION_LEFT_DOWN,
    DIRECTION_DOWN,
    DIRECTION_RIGHT_DOWN,
};

enum {
    FOCUS_ZONE =0,
    RIGHT_BUTTON,
    CLICK_2BUTTON,
};

enum {
    SCROLL_NONE =0,
    SCROLL_UP,
    SCROLL_DOWN,
};

static gint _init_data()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_ptz_data(&g_ptzData[i], i);
        DAL_get_ptz_preset_data(&g_presetData[i], i);
        DAL_get_ptz_scan_data(&g_scanData[i], i);
        DAL_get_ptz_tour_data(&g_tourData[i], i);

        DAL_get_MotSen_data(&g_msd[i], i);
        DAL_get_VLoss_Data(&g_vld[i], i);
        DAL_get_posevent_data(&g_psd[i], i);
    }

	for(i=0; i<g_alarmIn; i++)
		DAL_get_almSen_data(&g_asd[i], i);

    g_memmove(g_org_ptzData, g_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_presetData, g_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_scanData, g_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_tourData, g_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT);

	g_memmove(g_org_asd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn);
    g_memmove(g_org_msd, g_msd, sizeof(EA_MotSenData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_vld, g_vld, sizeof(EA_VLossData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_psd, g_psd, sizeof(EA_PosData)*GUI_CHANNEL_CNT);

    return 0;
}

static gint _is_changed_data()
{
    gint ret = 0;

	if (memcmp(g_org_ptzData, g_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT))
		ret = 1;

	if (memcmp(g_org_presetData, g_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT))
		ret = 1;

	if (memcmp(g_org_scanData, g_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT))
		ret = 1;

	if (memcmp(g_org_tourData, g_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT))
		ret = 1;

	if (memcmp(g_org_asd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn))
		ret = 1;

	if (memcmp(g_org_msd, g_msd, sizeof(EA_MotSenData)*GUI_CHANNEL_CNT))
		ret = 1;

	if (memcmp(g_org_vld, g_vld, sizeof(EA_VLossData)*GUI_CHANNEL_CNT))
		ret = 1;

	if (memcmp(g_org_psd, g_psd, sizeof(EA_PosData)*GUI_CHANNEL_CNT))
		ret = 1;

    return ret;
}

static gint _save_data()
{
    gint cam_changed = 0, evt_changed = 0;

	if (memcmp(g_org_ptzData, g_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT))
	{
        DAL_set_ptz_data_all(g_ptzData, GUI_CHANNEL_CNT);
        cam_changed = 1;
	}

	if (memcmp(g_org_presetData, g_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT))
	{
        DAL_set_ptz_preset_data_all(g_presetData, GUI_CHANNEL_CNT);
        cam_changed = 1;
	}

	if (memcmp(g_org_scanData, g_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT))
	{
        DAL_set_ptz_scan_data_all(g_scanData, GUI_CHANNEL_CNT);
        cam_changed = 1;
	}

	if (memcmp(g_org_tourData, g_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT))
	{
        DAL_set_ptz_tour_data_all(g_tourData, GUI_CHANNEL_CNT);
        cam_changed = 1;
	}

    if (cam_changed)
    {
        DAL_notify_fire_DB_change(NF_SYSDB_CATE_CAM);
        scm_put_log(CHANGE_CAM_PTZ, 0, 0);
    }

	if (memcmp(g_org_asd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn))
	{
        DAL_set_almSen_data_all(g_asd, g_alarmIn);
        scm_put_log(CHANGE_EVT_SENSOR, 0, 0);
        evt_changed = 1;
	}

	if (memcmp(g_org_msd, g_msd, sizeof(EA_MotSenData)*GUI_CHANNEL_CNT))
	{
        DAL_set_MotSen_data_all(g_msd, GUI_CHANNEL_CNT);
        scm_put_log(CHANGE_EVT_MOTION, 0, 0);
        evt_changed = 1;
	}

	if (memcmp(g_org_vld, g_vld, sizeof(EA_VLossData)*GUI_CHANNEL_CNT))
	{
        DAL_set_VLoss_Data_all(g_vld, GUI_CHANNEL_CNT);
        scm_put_log(CHANGE_EVT_VLOSS, 0, 0);
        evt_changed = 1;
	}

	if (memcmp(g_org_psd, g_psd, sizeof(EA_PosData)*GUI_CHANNEL_CNT))
	{
        DAL_set_posevent_data_all(g_psd, GUI_CHANNEL_CNT);
//        scm_put_log(CHANGE_EVT_VLOSS, 0, 0);
        evt_changed = 1;
	}

    if (cam_changed || evt_changed)
    {
		nftool_mbox_auto(g_curwnd, 2, "NOTICE", "Configuration has been saved.");

		if (cam_changed)   DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);
		if (evt_changed)   DAL_save_setup_db(NFSETUP_WINDOW_EVENT);
    }

    g_memmove(g_ptzData, g_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_presetData, g_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_scanData, g_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_tourData, g_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT);

	g_memmove(g_org_asd, g_asd, sizeof(EA_AlmSenData)*g_alarmIn);
    g_memmove(g_org_msd, g_msd, sizeof(EA_MotSenData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_vld, g_vld, sizeof(EA_VLossData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_psd, g_psd, sizeof(EA_PosData)*GUI_CHANNEL_CNT);

    return 0;
}

static gint _recover_data()
{
    gint i;

    PtzData tmp_ptzData[GUI_CHANNEL_CNT];
    PtzPresetData tmp_presetData[GUI_CHANNEL_CNT];
    PtzScanData tmp_scanData[GUI_CHANNEL_CNT];
    PtzTourData tmp_tourData[GUI_CHANNEL_CNT];

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_ptz_data(&tmp_ptzData[i], i);
        DAL_get_ptz_preset_data(&tmp_presetData[i], i);
        DAL_get_ptz_scan_data(&tmp_scanData[i], i);
        DAL_get_ptz_tour_data(&tmp_tourData[i], i);
    }

	if (memcmp(tmp_ptzData, g_org_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT))
        DAL_set_ptz_data_all(g_org_ptzData, GUI_CHANNEL_CNT);

	if (memcmp(tmp_presetData, g_org_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT))
        DAL_set_ptz_preset_data_all(g_org_presetData, GUI_CHANNEL_CNT);

	if (memcmp(tmp_scanData, g_org_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT))
        DAL_set_ptz_scan_data_all(g_org_scanData, GUI_CHANNEL_CNT);

	if (memcmp(tmp_tourData, g_org_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT))
        DAL_set_ptz_tour_data_all(g_org_tourData, GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
        DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_PTZ, i);

    g_memmove(g_ptzData, g_org_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT);
    g_memmove(g_presetData, g_org_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT);
    g_memmove(g_scanData, g_org_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT);
    g_memmove(g_tourData, g_org_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT);

	g_memmove(g_asd, g_org_asd, sizeof(EA_AlmSenData)*g_alarmIn);
    g_memmove(g_msd, g_org_msd, sizeof(EA_MotSenData)*GUI_CHANNEL_CNT);
    g_memmove(g_vld, g_org_vld, sizeof(EA_VLossData)*GUI_CHANNEL_CNT);
    g_memmove(g_psd, g_org_psd, sizeof(EA_PosData)*GUI_CHANNEL_CNT);

    return 0;
}

static gboolean _clear_video_color(gpointer data)
{
    nfui_nflabel_set_text((NFLABEL*)g_video_obj, "");
	nfui_nfobject_modify_bg(g_video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_signal_emit(g_video_obj, GDK_EXPOSE, TRUE);
	return FALSE;
}

static gint get_direction_from_point(gint pos_x, gint pos_y, gint m_x, gint m_y)
{
    gdouble PI = 3.141592;
    gdouble x,y;
    gdouble result;
    gint direction;


    x = m_x - pos_x;  // 화면 중심 좌표 pos_x,pos_y
    y = m_y - pos_y;  // 마우스 좌표 m_x, m_y

    result = atan2(y,x)*180/PI;
    if(result < 0 ) result += 360;

    if((result >= 0 && result < 22.5) || ( result >= 337.5 )) direction = DIRECTION_RIGHT;
    else if(result >= 22.5 && result < 67.5)    direction = DIRECTION_RIGHT_DOWN;
    else if(result >= 67.5 && result < 112.5)   direction = DIRECTION_DOWN;
    else if(result >= 112.5 && result < 157.5)  direction = DIRECTION_LEFT_DOWN;
    else if(result >= 157.5 && result < 202.2)  direction = DIRECTION_LEFT;
    else if(result >= 202.5 && result < 247.5)  direction = DIRECTION_LEFT_UP;
    else if(result >= 247.5 && result < 292.5)  direction = DIRECTION_UP;
    else if(result >= 292.5 && result < 337.5)  direction = DIRECTION_RIGHT_UP;
    else direction = DIRECTION_NONE;

    return direction;
}

static gint get_speed_from_point(gint pos_x, gint pos_y, gint m_x, gint m_y)
{
    gdouble x,y;
    gdouble distance;
    gint speed;
    gint speed_tm;

    distance = sqrt(pow(pos_x - m_x, 2) + pow(pos_y - m_y, 2));

    if(distance <= 0) return 0;

    speed_tm = (VIDEO_SIZE_H/2)/5;

    speed = (distance/speed_tm) + 1;  // 약 40 pixel = 1cm  >> 1cm 당 1 speed 값을 가지도록 설정.. 긷이당.

    if(speed > 5) speed  = 5;

    return speed*10;
}

static gfloat get_distance_from_point(gint pos_x, gint pos_y, gint m_x, gint m_y)
{
   gfloat distance;

   distance = sqrt(pow(pos_x - m_x, 2) + pow(pos_y - m_y, 2));

   return distance;
}

static GdkPoint _rotate_point(GdkPoint point, GdkPoint datum_point, gdouble cost, gdouble sint)
{
    GdkPoint returned;

    returned.x = (int)ceil(((point.x - datum_point.x) * cost) +  ((point.y - datum_point.y) * sint)+ datum_point.x);
    returned.y = (int)ceil(((datum_point.x - point.x) * sint) +  ((point.y - datum_point.y) * cost)+ datum_point.y);

    return returned;
}

static gboolean _move_ptz_camera()
{
    scm_set_ptz_cmd_pt_speed(cur_speed);

    switch(cur_direction)
    {
        case DIRECTION_LEFT_UP   :  scm_run_ptz_cmd_left_up();
            break;
        case DIRECTION_UP        :  scm_run_ptz_cmd_up();
            break;
        case DIRECTION_RIGHT_UP  :  scm_run_ptz_cmd_right_up();
            break;
        case DIRECTION_LEFT      :  scm_run_ptz_cmd_left();
            break;
        case DIRECTION_RIGHT     :  scm_run_ptz_cmd_right();
            break;
        case DIRECTION_LEFT_DOWN :  scm_run_ptz_cmd_left_down();
            break;
        case DIRECTION_DOWN      :  scm_run_ptz_cmd_down();
            break;
        case DIRECTION_RIGHT_DOWN:  scm_run_ptz_cmd_right_down();
            break;
    }

    return FALSE;
}

static gboolean _delay_drag_move(gpointer data)
{
    drag_sec--;
    cur_drag = 1;

    if(drag_sec) return TRUE;

    drag_sec = D_SEC;
    cur_drag = 0;

    scm_stop_ptz_cmd();
    _move_ptz_camera();

    return FALSE;
}

static gboolean _erase_arrow_line(GdkDrawable *drawable, GdkGC *gc, GdkPoint *line, GdkPoint *arrow)
{
    GdkRectangle cr;

    cr.x = VIDEO_FIXED_POS_X;
    cr.y = VIDEO_FIXED_POS_Y;
    cr.width = VIDEO_SIZE_W;
    cr.height = VIDEO_SIZE_H;
    gdk_gc_set_clip_rectangle(gc, &cr);

    gdk_gc_set_line_attributes(gc, 5, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_polygon(drawable, gc, TRUE, arrow, 3);
    gdk_draw_line(drawable, gc, line[0].x, line[0].y, line[1].x, line[1].y);

    cr.x = VIDEO_FIXED_POS_X;
    cr.y = VIDEO_FIXED_POS_Y;
    gdk_drawable_get_size(drawable, &cr.width, &cr.height);
    gdk_gc_set_clip_rectangle(gc, &cr);

    return FALSE;
}

static gboolean _draw_arrow_line(GdkDrawable *drawable, GdkGC *gc, GdkPoint *line, GdkPoint *arrow, gint pos_x, gint pos_y, gint mx, gint my)
{
    gint direction;
    gint speed;
    gfloat distance;

    gint i;
    gfloat dy, dx;

    GdkRectangle cr;

    cr.x = VIDEO_FIXED_POS_X;
    cr.y = VIDEO_FIXED_POS_Y;
    cr.width = VIDEO_SIZE_W;
    cr.height = VIDEO_SIZE_H;
    gdk_gc_set_clip_rectangle(gc, &cr);

    direction = get_direction_from_point(pos_x,pos_y, mx, my);
    speed = get_speed_from_point(pos_x, pos_y, mx, my);
    distance = get_distance_from_point(pos_x, pos_y, mx, my);

    line[0].x = pos_x;
    line[0].y = pos_y;
    line[1].x = mx;
    line[1].y = my;

    arrow[0].x = pos_x;
    arrow[0].y = pos_y+5;
    arrow[1].x = pos_x-14;
    arrow[1].y = pos_y-25;
    arrow[2].x = pos_x+14;
    arrow[2].y = pos_y-25;

    dy = line[1].y - line[0].y;
    dx = line[1].x - line[0].x;

    for (i = 0 ; i< 3; i++)
    {
        arrow[i] = _rotate_point(arrow[i], line[0], dy/distance, dx/distance);
        arrow[i].x += dx;
        arrow[i].y += dy;
    }

    gdk_gc_set_line_attributes(gc, 5, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_FF7F00)));

    gdk_draw_polygon(drawable, gc, TRUE, arrow, 3);
    gdk_draw_line(drawable, gc, line[0].x, line[0].y, line[1].x, line[1].y);

    cr.x = VIDEO_FIXED_POS_X;
    cr.y = VIDEO_FIXED_POS_Y;
    gdk_drawable_get_size(drawable, &cr.width, &cr.height);
    gdk_gc_set_clip_rectangle(gc, &cr);

    if(cur_speed == speed && cur_direction == direction)  return 0;

    if(cur_direction != DIRECTION_NONE)
    {
        cur_speed = speed;
        cur_direction = direction;

        if(cur_drag) return FALSE;

        g_timeout_add(5, _delay_drag_move, NULL);
    }
    else
    {
        cur_speed = speed;
        cur_direction = direction;

        _move_ptz_camera();
    }

    return FALSE;
}

static gboolean _init_ptz_state(gint ch)
{
    cur_direction = 0;
    cur_speed = 0;
    cur_scroll = SCROLL_NONE;

    scm_stop_ptz_cmd();

    return FALSE;
}

static gboolean _scroll_current_check(gpointer data)
{
    zoom_sec--;

    if(zoom_sec) return TRUE;

    scm_stop_ptz_cmd();
    cur_scroll = SCROLL_NONE;
    zoom_sec = -1;

    return FALSE;
}

static gboolean _scroll_zoom_ctrl(GdkEventScroll* event)
{
    scm_set_ptz_cmd_zoom_speed(ZOOM_SPEED); //  mouse wheel zoom speed

    switch(event->direction)
    {
        case GDK_SCROLL_UP :
        {
            if(cur_scroll == SCROLL_UP){

                zoom_sec = 3;
                return TRUE;

            }else if(zoom_sec == -1){

                zoom_sec = 3;
                cur_scroll = SCROLL_UP;

                scm_run_ptz_cmd_zoom_in();
            }
            else
                return TRUE;

            break;
        }
        case GDK_SCROLL_DOWN :
        {
            if(cur_scroll == SCROLL_DOWN){

                zoom_sec = 3;
                return TRUE;

            }else if(zoom_sec == -1){

                zoom_sec = 3;
                cur_scroll = SCROLL_DOWN;

                scm_run_ptz_cmd_zoom_out();
            }
            else
                return TRUE;

            break;
        }
    }

    return FALSE;
}


static gboolean _post_video_obj_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    static guint cur_state = 0;


    drawable = nfui_nfobject_get_window(obj);
    gc = gdk_gc_new(drawable);

    switch(event->type)
    {
        static gint mx, my;

        case GDK_BUTTON_RELEASE:
        {
            if(cur_state & (1 << RIGHT_BUTTON))
            {
                cur_state &= ~(1 << RIGHT_BUTTON);

                _erase_arrow_line(drawable, gc, line, arrow);
                _init_ptz_state(g_ch);
            }

            break;
        }
        case GDK_LEAVE_NOTIFY:
        {
            if(cur_state & (1 << RIGHT_BUTTON))
            {
                cur_state &= ~(1 << RIGHT_BUTTON);
                _erase_arrow_line(drawable, gc, line, arrow);
                _init_ptz_state(g_ch);
            }
            break;
        }
        case GDK_MOTION_NOTIFY:
        {
            GdkEventMotion *mevent;
            gint mevt_x, mevt_y;

            mevent = (GdkEventMotion*)event;

            mevt_x = (gint)mevent->x;
            mevt_y = (gint)mevent->y;

            mx = mevt_x;
            my = mevt_y;

            if (mevent->state & GDK_BUTTON3_MASK){

                if(scm_get_ipcam_ptz_supp(g_ch)) return FALSE;

                cur_state |= (1 << RIGHT_BUTTON);

                if(cur_drag)  drag_sec = D_SEC;

                _erase_arrow_line(drawable, gc, line, arrow);
                _draw_arrow_line(drawable, gc, line, arrow, VIDEO_CENTER_POS_X, VIDEO_CENTER_POS_Y, mx, my);
            }
            else
            {
                if(cur_state & (1 << RIGHT_BUTTON))
                {
                    cur_state &= ~(1 << RIGHT_BUTTON);
                    _erase_arrow_line(drawable, gc, line, arrow);
                    _init_ptz_state(g_ch);
                }
            }
            break;
        }
        case GDK_BUTTON_PRESS:
        {
            if(scm_get_ipcam_ptz_supp(g_ch)) return FALSE;
            if(event->button.button != MOUSE_RIGTH_BUTTON) return FALSE;

            cur_state |= (1 << RIGHT_BUTTON);

            _draw_arrow_line(drawable, gc, line, arrow, VIDEO_CENTER_POS_X, VIDEO_CENTER_POS_Y, mx, my);

            break;
        }
        case GDK_SCROLL:
        {
            GdkEventScroll *sevt;
            sevt = (GdkEventScroll*)event;

            if(scm_get_ipcam_zoom_supp(g_ch))
                return FALSE;

            if(_scroll_zoom_ctrl(sevt))
                return FALSE;

            g_timeout_add(100, _scroll_current_check, NULL);

            break;
        }
    }
	return FALSE;
}

static gboolean _pre_prst_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (event->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if (event->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

static gboolean _pre_ctrl_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (event->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if (event->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

static gboolean _pre_status_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (event->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if (event->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

static gboolean _post_main_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    guint cam_num;

	switch(event->type)
	{
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
        	GdkEventKey *kevt;
        	KEYPAD_KID kpid;

        	kevt = (GdkEventKey*)event;
        	kpid = (KEYPAD_KID)kevt->keyval;

		if (GET_CHANNEL_KEY_SCOPE(kpid))
		{
			cam_num = kpid - KEYPAD_CH01;
			if (g_ch != cam_num) _live_ptz_main_change_channel(cam_num, 1);
		}else
		{
    			switch(kpid)
    			{
    				case KEYPAD_SETUP:
    				case RMC_MENU:
    				{
                  	  		g_ptz_ctrl_mode = 0;
                  	 		 return TRUE;
                	}
					break;
    				case KEYPAD_PTZ:
    				{
                   			g_ptz_ctrl_mode = 1;
                   			return TRUE;
                	}
					break;
    				case KEYPAD_UP:
					{
                   			if (g_ptz_ctrl_mode)
                    		{
                        			scm_run_ptz_cmd_up();
                        			return TRUE;
                   			}
					}
					break;
    				case KEYPAD_DOWN:
					{
                   			if (g_ptz_ctrl_mode)
                    		{
                        			scm_run_ptz_cmd_down();
                       				return TRUE;
               	     		}
					}
					break;
    				case KEYPAD_LEFT:
					{
              	     		if (g_ptz_ctrl_mode)
              	      		{
              	         			scm_run_ptz_cmd_left();
              	        			return TRUE;
              	    		}
					}
					break;
    				case KEYPAD_RIGHT:
					{
             	      		if (g_ptz_ctrl_mode)
             	     		{
             	      			    scm_run_ptz_cmd_right();
             	          			return TRUE;
             	   		 	}
					}
					break;
					case KEYPAD_FF:
    				case RMC_ZOOMIN:
					{
            	   	scm_run_ptz_cmd_zoom_in();
					}
					break;
					case KEYPAD_REW:
    				case RMC_ZOOMOUT:
					{
            	       		scm_run_ptz_cmd_zoom_out();
					}
					break;
    				case RMC_NEAR:
					{
         	           		scm_run_ptz_cmd_focus_near();
					}
					break;
    				case RMC_FAR:
					{
                	   		 scm_run_ptz_cmd_focus_far();
					}
					break;
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

        	kevt = (GdkEventKey*)event;
        	kpid = (KEYPAD_KID)kevt->keyval;

        		switch(kpid)
        		{
        			case KEYPAD_UP:     case KEYPAD_DOWN:
        		    case KEYPAD_RIGHT:  case KEYPAD_LEFT:
        		    {
                        if (g_ptz_ctrl_mode)
                        {
                		    scm_stop_ptz_cmd();
                            return TRUE;
                        }
                    }
                    break;
        			case KEYPAD_FF:     case KEYPAD_REW:
        			case RMC_ZOOMIN:    case RMC_ZOOMOUT:
        		    case RMC_NEAR:      case RMC_FAR:
        		    {
               		    scm_stop_ptz_cmd();
                    }
                    break;
        			case RMC_PRESET:
    				{
                        gint prst_num;

                        prst_num = VW_PtzCtrl_rmc_prst_Open(g_curwnd);
                        if(prst_num) scm_run_ptz_cmd_goto(prst_num);
    				}
    				break;
/*
        			case KEYPAD_PANIC:
    				{
                		scm_toggle_panic_record();
    				}
    				break;
*/
        			case KEYPAD_EXIT:
    				{
                        _live_ptz_main_close();
                        return TRUE;
    				}
    				break;
        			default:
    				break;
        		}
        }
		break;

		case GDK_DELETE:
		{
			g_curwnd = 0;
			gtk_main_quit();
        }
		break;

		default:
		break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

gint VW_Live_Ptz_Main_Open(NFWINDOW *parent, guint channel)
{
	NFOBJECT *ptz_win;
	NFOBJECT *main_fixed;

	NFOBJECT *ctrl_fixed;
	NFOBJECT *prst_fixed;
	NFOBJECT *status_fixed;

	NFOBJECT *obj;
	NFOBJECT *focus_obj;

    vsm_live_stop();
    g_ch = channel;
    g_ptz_ctrl_mode = 1;
	g_alarmIn = var_get_alarmIn_cnt();

    ptz_win = nfui_nfwindow_new(parent, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
	nfui_nfwindow_set_title(ptz_win, "PTZ CTRL");
	nfui_nfobject_modify_bg(ptz_win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_regi_post_event_callback(ptz_win, _post_main_event_handler);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)ptz_win, returnkey_proc);
	g_curwnd = ptz_win;

	main_fixed = nfui_nffixed_new();
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_nfobject_show(main_fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("Please wait...", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_PRG_IDX(UX_COLOR_FFFFFF));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
	nfui_nfobject_set_size(obj, VIDEO_FIXED_W, VIDEO_FIXED_H);
	//nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nflabel_set_drawing_outline((NFLABEL*)obj, FALSE);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, VIDEO_FIXED_POS_X, VIDEO_FIXED_POS_Y);
	nfui_regi_post_event_callback(obj, _post_video_obj_event_handler);
    g_video_obj = obj;

	prst_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(prst_fixed, PRST_FIXED_W, PRST_FIXED_H);
	nfui_nfobject_modify_bg(prst_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(prst_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, prst_fixed, PRST_FIXED_POS_X, PRST_FIXED_POS_Y);
	nfui_regi_pre_event_callback(prst_fixed, _pre_prst_fixed_event_handler);

	ctrl_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(ctrl_fixed, CTRL_FIXED_W, CTRL_FIXED_H);
	nfui_nfobject_modify_bg(ctrl_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ctrl_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, ctrl_fixed, CTRL_FIXED_POS_X, CTRL_FIXED_POS_Y);
	nfui_regi_pre_event_callback(ctrl_fixed, _pre_ctrl_fixed_event_handler);

	status_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(status_fixed, STATUS_FIXED_W, STATUS_FIXED_H);
	nfui_nfobject_modify_bg(status_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(status_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, status_fixed, STATUS_FIXED_POS_X, STATUS_FIXED_POS_Y);
	nfui_regi_pre_event_callback(status_fixed, _pre_status_fixed_event_handler);

    _init_data();

    _live_ptz_prst_set_data(&g_ptzData[channel], &g_presetData[channel], &g_scanData[channel], &g_tourData[channel]);
    _live_ptz_ctrl_set_data(&g_ptzData[channel], &g_presetData[channel]);

    _live_ptz_prst_set_evt_data(g_asd, g_msd, g_vld, g_psd);

    _live_ptz_prst_open(ptz_win, prst_fixed);
    _live_ptz_ctrl_open(ptz_win, ctrl_fixed);
    _live_ptz_status_open(ptz_win, status_fixed);

    _live_ptz_prst_init_channel(channel);
    _live_ptz_ctrl_init_channel(channel);
    _live_ptz_status_init_channel(channel);

	nfui_nfwindow_add((NFWINDOW*)ptz_win, main_fixed);
	nfui_run_main_event_handler(ptz_win);
	nfui_nfobject_show(ptz_win);
	nfui_make_key_hierarchy((NFWINDOW*)ptz_win);

	focus_obj = (NFOBJECT*)(nfui_nfobject_get_data(status_fixed,"FOCUS_OBJ"));
	nfui_set_key_focus(focus_obj, TRUE);
	nfui_page_open(PGID_PTZ, ptz_win, ssm_get_cur_id(NULL));
	smt_set_service(SMT_LIVE_PTZ);

	g_timeout_add(1000, _clear_video_color, NULL);
  	vsm_live_preview_start(1 << g_ch, VIDEO_FIXED_POS_X, VIDEO_FIXED_POS_Y, VIDEO_FIXED_W, VIDEO_FIXED_H);

	gtk_main();

	nfui_page_close(PGID_PTZ, ptz_win);
	vsm_live_preview_stop();
    vsm_live_start();
	smt_set_service(SMT_LIVE);

	return g_ch;
}

void VW_Live_Ptz_Main_Destroy()
{
	if (g_curwnd) nfui_nfobject_destroy(g_curwnd);
}

gint _live_ptz_main_change_channel(guint channel, gint expose)
{
    vsm_live_preview_start(1 << channel, VIDEO_FIXED_POS_X, VIDEO_FIXED_POS_Y, VIDEO_FIXED_W, VIDEO_FIXED_H);

    _live_ptz_prst_set_data(&g_ptzData[channel], &g_presetData[channel], &g_scanData[channel], &g_tourData[channel]);
    _live_ptz_ctrl_set_data(&g_ptzData[channel], &g_presetData[channel]);

    _live_ptz_prst_change_channel(channel, expose);
    _live_ptz_ctrl_change_channel(channel, expose);
    _live_ptz_status_change_channel(channel, expose);

    g_ch = channel;

    return 0;
}

gint VW_Live_Ptz_Main_Get_Channel(void)
{
	return g_ch;
}

gint _live_ptz_main_cmd_pattern(gint channel)
{
    gint is_changed=0;

    PtzData tmp_ptzData;
    PtzPresetData tmp_presetData;
    PtzScanData tmp_scanData;
    PtzTourData tmp_tourData;

    DAL_get_ptz_data(&tmp_ptzData, channel);
    DAL_get_ptz_preset_data(&tmp_presetData, channel);
    DAL_get_ptz_scan_data(&tmp_scanData, channel);
    DAL_get_ptz_tour_data(&tmp_tourData, channel);

    if (memcmp(&tmp_ptzData, &g_ptzData[channel], sizeof(PtzData)))
    {
        DAL_set_ptz_data(g_ptzData[channel], channel);
        is_changed = 1;
    }

    if (memcmp(&tmp_presetData, &g_presetData[channel], sizeof(PtzPresetData)))
    {
        DAL_set_ptz_preset_data(g_presetData[channel], channel);
        is_changed = 1;
    }

    if (memcmp(&tmp_scanData, &g_scanData[channel], sizeof(PtzScanData)))
    {
        DAL_set_ptz_scan_data(g_scanData[channel], channel);
        is_changed = 1;
    }

    if (memcmp(&tmp_tourData, &g_tourData[channel], sizeof(PtzTourData)))
    {
        DAL_set_ptz_tour_data(g_tourData[channel], channel);
        is_changed = 1;
    }

    if (is_changed)
        DAL_notify_fire_DB_sync(NF_SYSDB_TMP_CHANGE_EVENTID_PTZ, channel);

    return 0;
}

gint _live_ptz_main_close()
{
    gint is_changed = 0;
	mb_type ret;

    is_changed = _is_changed_data();

    if (!is_changed)
    {
        nfui_nfobject_destroy(g_curwnd);
        return 0;
    }

	ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?",
							NFTOOL_MB_OKCANCEL);

	if (ret == NFTOOL_MB_OK)
        _save_data();
	else if (ret == NFTOOL_MB_CANCEL)
        _recover_data();

	nfui_nfobject_destroy(g_curwnd);

    return 0;
}

gint _live_ptz_main_show(gint channel)
{
	nfui_nfobject_modify_bg(g_video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
    nfui_nfobject_show(g_curwnd);
    smt_set_service(SMT_LIVE_PTZ);
    return 0;
}

gint _live_ptz_main_hide()
{
	nfui_nfobject_modify_bg(g_video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_929292));
    nfui_signal_emit(g_video_obj, GDK_EXPOSE, TRUE);
    nfui_nfobject_hide(g_curwnd);
    return 0;
}

