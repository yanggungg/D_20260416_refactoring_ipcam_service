#include <math.h>

#include "nf_afx.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_page_manager.h"
#include "support/color.h"
#include "support/util.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfwindow.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nflabel.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nftimelabel.h"
#include "objects/nfpiechart.h"
#include "objects/nfcombobox.h"

#include "services/vsm.h"
#include "modules/ssm.h"
#include "vw.h"
#include "uxm.h"

#include "vw_vwnd.h"

#define ZOOM_VIDEO_W_SHARE							(DISPLAY_ACTIVE_WIDTH/10)
#define ZOOM_VIDEO_H_SHARE							(DISPLAY_ACTIVE_HEIGHT/10)

#define ZOOM_MOVE_W_STEP							(ZOOM_VIDEO_W_SHARE)
#define ZOOM_MOVE_H_STEP							(ZOOM_VIDEO_H_SHARE)

#define ZOOM_MOVE_SIZE_W(step)						(ZOOM_MOVE_W_STEP*step)
#define ZOOM_MOVE_SIZE_H(step)						(ZOOM_MOVE_H_STEP*step)

#define BUTTON_SIZE                     (64)
#define FOCUS_LINE_SIZE                 (4)
#define ZOOM_SPEED                      (90)
#define D_SEC                           (3)
 
#define WIN_WIDTH                       (400)
#define WIN_HEIGHT                      (280+65)

// TABLE1 ATTR.
#define CTRL_TBL1_COLS_CNT				(3)
#define CTRL_TBL1_ROWS_CNT				(3)

#define CTRL_TBL1_COLS_SPACE			(4)
#define CTRL_TBL1_ROWS_SPACE			(2)

// TABLE2 ATTR.
#define CTRL_TBL2_COLS_CNT				(3)
#define CTRL_TBL2_ROWS_CNT				(3)

#define CTRL_TBL2_COLS_SPACE			(0)
#define CTRL_TBL2_ROWS_SPACE			(7)//11

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_parent = 0;
static NFOBJECT *g_status_label =0;

static gint     g_ch = -1;
static gint     g_temp = -1;
static gint     g_x=0;
static gint     g_y=0;
static gint     g_w=0;
static gint     g_h=0;

static NFOBJECT *cmd_dir_obj[9];
static NFOBJECT *zoom_label_obj;
static NFOBJECT *focus_label_obj;
static NFOBJECT *iris_label_obj;
static NFOBJECT *zoom_obj[2];
static NFOBJECT *focus_obj[2];
static NFOBJECT *iris_obj[2];

static NFOBJECT *g_ptz_radio[18];
 
static PtzData g_ptzData[GUI_CHANNEL_CNT];
static PtzData g_org_ptzData[GUI_CHANNEL_CNT];

static PtzPresetData g_presetData[GUI_CHANNEL_CNT];
static PtzPresetData g_org_presetData[GUI_CHANNEL_CNT];

static PtzScanData g_scanData[GUI_CHANNEL_CNT];
static PtzScanData g_org_scanData[GUI_CHANNEL_CNT];

static PtzTourData g_tourData[GUI_CHANNEL_CNT];
static PtzTourData g_org_tourData[GUI_CHANNEL_CNT];

static GdkPoint g_arrow_shape[5] = { {-2,-20}, {-11,-20}, {0,5}, {11,-20}, {2,-20} };
static GdkPoint g_center_shape[2] = { {2,0}, {-2,0} };

static GdkPoint prev_zoom_box[2];   
static gint cur_direction =0;
static gint cur_speed =0;
static gint cur_scroll = 0;    
static gint cur_drag = 0;
static gint zoom_sec = -1;
static gint drag_sec = D_SEC;

static gboolean _live_ptz_cmd_pattern(gint channel);


///////// enum ////////////

enum{
    SCAN = 16,
    TOUR,
};

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

//////////////// STATIC FUCTION ///////////////////


static gint get_direction_from_point(gint pos_x, gint pos_y, gint m_x, gint m_y)
{
    gdouble PI = 3.141592;
    gdouble x,y;
    gdouble result;
    gint direction;
    

    x = m_x - pos_x;  // ȭ�� �߽� ��ǥ pos_x,pos_y
    y = m_y - pos_y;  // ���콺 ��ǥ m_x, m_y

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
    gint speed_tm;
    gint speed;

    distance = sqrt(pow(pos_x - m_x, 2) + pow(pos_y - m_y, 2));   

    if(distance <= 0) return 0;

    speed_tm = (g_h/2)/5;
    
    speed = (distance/speed_tm) + 1;  // �� 40 pixel = 1cm  >> 1cm �� 1 speed ���� �������� ����.. ���̴�. 

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

static gboolean _erase_arrow_line()
{
    evt_send_to_local(INFY_VWND_ERASE_PTZARROW, 0, 0, 0);

    return FALSE;
}

#if 0
static gboolean _draw_zoom_box(GdkDrawable *drawable, GdkGC *gc, GdkPoint *zoom, gint mx, gint my)
{  
    gdk_gc_set_line_attributes(gc, 3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);
    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_IDX(386)));

    zoom[1].x = zoom[0].x > mx ? (zoom[0].x - mx) : (mx - zoom[0].x);
    zoom[1].y = zoom[0].y > my ? (zoom[0].y - my) : (my - zoom[0].y);   

    if(zoom[0].x > mx)  zoom[2].x = mx;             // zoom[2] �巡�׽� ������ ��ǥ 
    else                zoom[2].x = zoom[0].x;      // zoom[1] �簢�� ����, ���� 
                                                    // zoom[0] ���콺 Ŭ���� ��ǥ                   
    if(zoom[0].y > my)  zoom[2].y = my;             // mx, my  ���� ���콺 ��ǥ
    else                zoom[2].y = zoom[0].y; 

    /////////   channel size ���� ���� //////////  mouse ctrl point check 
    
    if((zoom[0].x < mx) && (zoom[0].y < my)){       // 4��и� 

        zoom[1].y = (g_h * zoom[1].x)/g_w;

        if(zoom[1].x < (mx-zoom[0].x)) zoom[1].y = (g_h * (mx-zoom[0].x))/g_w;
        if(zoom[1].y < (my-zoom[0].y)){   
            zoom[1].x = (g_w * (my-zoom[0].y))/g_h;                
            zoom[1].y = (g_h * zoom[1].x)/g_w;
        }

        if( zoom[0].y+zoom[1].y > (g_y+g_h-FOCUS_LINE_SIZE)){
            zoom[1].y = (g_y+g_h)-FOCUS_LINE_SIZE-zoom[2].y;
            zoom[1].x = (g_w *zoom[1].y)/g_h;
        }

        if( (zoom[0].x+zoom[1].x) > g_x+g_w-FOCUS_LINE_SIZE){
            zoom[1].x = ((g_x+g_w)-FOCUS_LINE_SIZE)-zoom[0].x;
            zoom[1].y = (g_h * zoom[1].x)/g_w;
        }
    }
    else if((zoom[0].x < mx) && (zoom[0].y > my)){  // 1��и� 

        zoom[1].y = (g_h * zoom[1].x)/g_w;
        zoom[2].y = zoom[0].y - zoom[1].y;

        if(zoom[1].x < (mx-zoom[0].x)){
            zoom[1].y = (g_h * (mx - zoom[0].x))/g_w;
            zoom[2].y = zoom[0].y - zoom[1].y; 
        }
        
        if(zoom[1].y < (zoom[0].y - my)){
            zoom[1].x = (g_w * (zoom[0].y-my))/g_h;
            zoom[1].y = (g_h * zoom[1].x)/g_w;
            zoom[2].y = zoom[0].y - zoom[1].y;
        }

        if((zoom[0].y - zoom[1].y) < (g_y+FOCUS_LINE_SIZE)){
            zoom[1].y = zoom[0].y-(g_y+FOCUS_LINE_SIZE);
            zoom[1].x = (g_w * zoom[1].y)/g_h;
            zoom[2].y = zoom[0].y - zoom[1].y;
        }

        if((zoom[0].x + zoom[1].x) > g_x+g_w-FOCUS_LINE_SIZE){
            zoom[1].x = g_x+g_w-FOCUS_LINE_SIZE-zoom[0].x;
            zoom[1].y = (g_h * zoom[1].x)/g_w;
            zoom[2].y = zoom[0].y - zoom[1].y;
        }
        
    }
    else if((zoom[0].x > mx) && (zoom[0].y < my)){  // 3�� �и� 

        zoom[1].y = (g_h * zoom[1].x)/g_w;

        if(zoom[1].x < (zoom[0].x - mx)){
            zoom[1].y = (g_h * (zoom[0].x -mx))/g_w;
        }
        
        if(zoom[1].y < (my - zoom[0].y)){
            zoom[1].x = (g_w *(my-zoom[0].y))/g_h;
            zoom[2].x = zoom[0].x - zoom[1].x;
            zoom[1].y = (g_h * zoom[1].x)/g_w;
        }

        if((zoom[0].y+zoom[1].y) > ((g_y+g_h)-FOCUS_LINE_SIZE)){
            zoom[1].y = (g_y+g_h)-FOCUS_LINE_SIZE-zoom[2].y;
            zoom[1].x = (g_w *zoom[1].y)/g_h;
            zoom[2].x = zoom[0].x - zoom[1].x;
        }

        if((zoom[0].x - zoom[1].x) < g_x+FOCUS_LINE_SIZE){
            zoom[2].x = g_x+FOCUS_LINE_SIZE;
            zoom[1].x = zoom[0].x - zoom[2].x;
            zoom[1].y = (g_h * zoom[1].x)/g_w;
        }        
    }
    else if((zoom[0].x > mx) && (zoom[0].y > my)){  // 2�� �и� 

        zoom[1].y = (g_h * zoom[1].x)/g_w;
        zoom[2].y = zoom[0].y - zoom[1].y;

        if(zoom[1].x < (zoom[0].x-mx)){
            zoom[1].y = (g_h * ( zoom[0].x - mx))/g_w;
            zoom[2].y = zoom[0].y - zoom[1].y;
        }
        
        if(zoom[1].y < (zoom[0].y-my)){
            zoom[1].x = (g_w * ( zoom[0].y - my))/g_h;
            zoom[2].x = zoom[0].x - zoom[1].x;
            zoom[1].y = (g_h * zoom[1].x)/g_w;
            zoom[2].y = zoom[0].y - zoom[1].y;
        }

        if((zoom[0].y-zoom[1].y) < g_y+FOCUS_LINE_SIZE){     
            zoom[2].y = g_y + FOCUS_LINE_SIZE;
            zoom[1].y = zoom[0].y - zoom[2].y;
            zoom[1].x = (g_w * zoom[1].y)/g_h;
            zoom[2].x = zoom[0].x - zoom[1].x;
        }

        if(zoom[0].x - zoom[1].x < g_x+FOCUS_LINE_SIZE){
            zoom[1].x = zoom[0].x -(g_x+FOCUS_LINE_SIZE);
            zoom[1].y = (g_h * zoom[1].x)/g_w;
            zoom[2].y = zoom[0].y-zoom[1].y;
            zoom[2].x = g_x+FOCUS_LINE_SIZE;
        }

    }
    /////////   channel size ���� ���� //////////
    
    gdk_draw_rectangle(drawable, gc, FALSE, zoom[2].x, zoom[2].y, zoom[1].x, zoom[1].y);

    nfui_nfobject_gc_unref(gc);     

    return FALSE;    
}

static gboolean _erase_zoom_box(GdkDrawable *drawable, GdkGC *gc, GdkPoint *zoom)
{
    gdk_gc_set_line_attributes(gc, 3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_BEVEL);

    gdk_gc_set_rgb_fg_color(gc, &UX_COLOR(COLOR_PRG_IDX(UX_COLOR_000000)));
    gdk_draw_rectangle(drawable, gc, FALSE, zoom[2].x, zoom[2].y, zoom[1].x, zoom[1].y);

    nfui_nfobject_gc_unref(gc);

    return FALSE;
}
#endif

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

static gboolean _draw_arrow_line(gint pos_x, gint pos_y, gint mx, gint my)
{   
    gint direction;
    gint speed;
    gfloat distance;

    gint i;
    gint dy, dx;

    GdkPoint datum_pt = {0, 0};
    GdkPoint *arrow_pt;
    gint pt_cnt;
 
    direction = get_direction_from_point(pos_x,pos_y, mx, my);
    speed = get_speed_from_point(pos_x, pos_y, mx, my);
    distance = get_distance_from_point(pos_x, pos_y, mx, my);

    dx = mx - pos_x;
    dy = my - pos_y;

    arrow_pt = imalloc(sizeof(GdkPoint)*7);
    
    for (i = 0; i < 5; i++)
    {
        arrow_pt[i] = _rotate_point(g_arrow_shape[i], datum_pt, (gfloat)dy/distance, (gfloat)dx/distance);
        arrow_pt[i].x += (pos_x+dx);
        arrow_pt[i].y += (pos_y+dy);
    }      

    if ((abs(dx) < 11) && (abs(dy) < 20))
    {
        pt_cnt = 5;
    }
    else
    {
        for (i = 0; i < 2; i++)
        {
            arrow_pt[5+i] = _rotate_point(g_center_shape[i], datum_pt, (gfloat)dy/distance, (gfloat)dx/distance);
            arrow_pt[5+i].x += pos_x;
            arrow_pt[5+i].y += pos_y;
        }

        pt_cnt = 7;        
    }

    evt_send_to_local(INFY_VWND_DRAW_PTZARROW, pt_cnt, 1, arrow_pt);

    if(cur_direction == direction && cur_speed == speed)    return 0;
    
    if(cur_direction != DIRECTION_NONE)
    {
        cur_direction =  direction;
        cur_speed = speed;
        
        if(cur_drag) return FALSE;
        
        g_timeout_add(5, _delay_drag_move, NULL);    
    }      
    else
    {
        cur_direction =  direction;
        cur_speed = speed;
        
        _move_ptz_camera();
    }

    return FALSE;
}

static gboolean _update_status_label(gint idx)
{   
    gchar strBuf[256];
    gchar strtmp[256];
    gint i;

    memset(strBuf, 0x00, sizeof(strBuf));
    
    if(idx < 0){
        g_sprintf(strBuf,"%s"," ");

        if(g_ptzData[g_ch].mode){
            g_ptzData[g_ch].mode = PTZ_MODE_OFF;
            _live_ptz_cmd_pattern(g_ch);
        }

        if(g_temp >= 0)
        {
            nfui_radio_button_unset_toggled(g_ptz_radio[g_temp]);
            nfui_signal_emit(g_ptz_radio[g_temp], GDK_EXPOSE, TRUE);
        }
    }   
    else if(idx <SCAN){
        g_sprintf(strBuf,"%s",g_presetData[g_ch].name[idx]);
    }
    else if(idx == SCAN){
        gint scan_cnt=0;
        for(i =0; i< 2; i++){
            if(!g_scanData[g_ch].number[i]) continue;
            
            memset(strtmp, 0x00, sizeof(strtmp));
            if(i==0) g_sprintf(strtmp," %d",g_scanData[g_ch].number[i]);
            else     g_sprintf(strtmp," - %d", g_scanData[g_ch].number[i]);
            strcat(strBuf, strtmp);
            scan_cnt++;
        }
        
        if(!scan_cnt)g_sprintf(strBuf,"%s", "There's no data for SCAN.");
        else         g_sprintf(strBuf,"[ SCAN ] : %s",strBuf);
    }
    else if(idx == TOUR){
        gint tour_cnt =0;
        for(i =0; i< MAX_TOUR_COUNT; i++){
            if(!g_tourData[g_ch].number[i]) continue;
            
            memset(strtmp, 0x00, sizeof(strtmp));
            if(i==0) g_sprintf(strtmp," %d",g_tourData[g_ch].number[i]);
            else     g_sprintf(strtmp," - %d", g_tourData[g_ch].number[i]);
            strcat(strBuf, strtmp);
            tour_cnt++;
        }
            
        if(!tour_cnt) g_sprintf(strBuf,"%s", "There's no data for TOUR.");
        else          g_sprintf(strBuf,"[ TOUR ] : %s",strBuf);
    }
    
    nfui_nflabel_set_text(g_status_label, strBuf);
    nfui_signal_emit(g_status_label, GDK_EXPOSE, TRUE);

    return FALSE;
}

static gint _init_supported_func(gint ch)
{
	gint support;
	gint i, j;      

	support = scm_get_ipcam_ptz_supp(ch);

	for (j = 0; j < 3; j++)
	{
		for (i = 0; i < 3; i++)
		{
			if (!support) 
				nfui_nfobject_enable(cmd_dir_obj[i+j*3]);
			else 
				nfui_nfobject_disable(cmd_dir_obj[i+j*3]);
		}
	}

#if defined(_IPX_MODEL_UX)
    support = scm_get_ipcam_calibration_supp(ch);

	if (!support) nfui_nfobject_enable(cmd_dir_obj[4]);
	else nfui_nfobject_disable(cmd_dir_obj[4]);
#else
    nfui_nfobject_enable(cmd_dir_obj[4]);
#endif

	support = scm_get_ipcam_zoom_supp(ch);

	if (!support) nfui_nfobject_enable(zoom_label_obj);
	else nfui_nfobject_disable(zoom_label_obj);

	for (i = 0; i < 2; i++)
	{
		if (!support) nfui_nfobject_enable(zoom_obj[i]);
		else nfui_nfobject_disable(zoom_obj[i]);
	}

	support = scm_get_ipcam_focus_supp(ch);

	if (!support) nfui_nfobject_enable(focus_label_obj);
	else nfui_nfobject_disable(focus_label_obj);

	for (i = 0; i < 2; i++)
	{
		if (!support) nfui_nfobject_enable(focus_obj[i]);
		else nfui_nfobject_disable(focus_obj[i]);
	}		

	support = scm_get_ipcam_iris_supp(ch);

	if (!support) nfui_nfobject_enable(iris_label_obj);
	else nfui_nfobject_disable(iris_label_obj);

	for (i = 0; i < 2; i++)
	{
		if (!support) nfui_nfobject_enable(iris_obj[i]);
		else nfui_nfobject_disable(iris_obj[i]);
	}		

    return 0;
}

static gboolean _live_ptz_ctrl_change(guint channel)
{
    gint i;

    scm_init_ptz_param(channel);

    _init_supported_func(channel);

    for(i=0 ; i<9; i++)
        nfui_signal_emit(cmd_dir_obj[i], GDK_EXPOSE, TRUE);

    nfui_signal_emit(zoom_label_obj, GDK_EXPOSE, TRUE);

    for(i=0; i<2; i++)
        nfui_signal_emit(zoom_obj[i], GDK_EXPOSE, TRUE);

    nfui_signal_emit(focus_label_obj, GDK_EXPOSE, TRUE);

    for(i=0; i<2; i++)
        nfui_signal_emit(focus_obj[i], GDK_EXPOSE, TRUE);

    nfui_signal_emit(iris_label_obj, GDK_EXPOSE, TRUE);

    for(i=0; i<2; i++)
        nfui_signal_emit(iris_obj[i], GDK_EXPOSE, TRUE);
    
    return FALSE;
}

static gboolean _live_ptz_prst_change(guint ch)
{
    gchar buf[32];
    gint i;

    for(i=0; i<18; i++)
    {
        if(i< SCAN){

            memset(buf, 0x00, sizeof(buf));     

            if(!g_presetData[ch].number[i])  g_sprintf(buf,"%s"," ");
            else g_sprintf(buf,"%d",g_presetData[ch].number[i]);

            nfui_nfbutton_set_text(g_ptz_radio[i], buf);
        
            if(g_presetData[ch].cnt-1 < i ) 
                nfui_nfobject_disable(g_ptz_radio[i]);
            else
                nfui_nfobject_enable(g_ptz_radio[i]);
                
            if(!g_presetData[ch].cnt)
                nfui_nfobject_disable(g_ptz_radio[i]);
            else
                nfui_nfobject_enable(g_ptz_radio[i]);
                
        }
        else{

            if(!g_presetData[ch].cnt) 
                nfui_nfobject_disable(g_ptz_radio[i]);
            else
                nfui_nfobject_enable(g_ptz_radio[i]);
        }

        if(i == SCAN){

            if(g_ptzData[ch].mode == PTZ_MODE_SCAN){    

                nfui_radio_button_set_toggled(g_ptz_radio[i], TRUE);
                _update_status_label(SCAN);
            }
        }
        else if(i == TOUR){

            if(g_ptzData[ch].mode == PTZ_MODE_TOUR){

                nfui_radio_button_set_toggled(g_ptz_radio[i], TRUE);
                _update_status_label(TOUR);
            }
        }      

        nfui_signal_emit(g_ptz_radio[i], GDK_EXPOSE, TRUE);
    }    

    return FALSE;
}


static gint _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);

	return 0;
}

static gboolean _live_ptz_cmd_pattern(gint channel)
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

    return FALSE;
}

static gint _init_data()
{
    gint i;

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_ptz_data(&g_ptzData[i], i);
        DAL_get_ptz_preset_data(&g_presetData[i], i);
        DAL_get_ptz_scan_data(&g_scanData[i], i);
        DAL_get_ptz_tour_data(&g_tourData[i], i);
    }

    g_memmove(g_org_ptzData, g_ptzData, sizeof(PtzData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_presetData, g_presetData, sizeof(PtzPresetData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_scanData, g_scanData, sizeof(PtzScanData)*GUI_CHANNEL_CNT);
    g_memmove(g_org_tourData, g_tourData, sizeof(PtzTourData)*GUI_CHANNEL_CNT);

      
    return 0;
}

static gboolean _init_ptz_state(gint ch)
{
    cur_direction = 0;
    cur_speed = 0;
    cur_scroll = SCROLL_NONE; 
    
    scm_stop_ptz_cmd();

    return FALSE;
}

//// CALLBACK  //////

static gboolean _post_home_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_PRESS) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

#if defined(_IPX_MODEL_UX)
		if (scm_req_ipcam_calibration(g_ch) < 0) 
		{
			vw_mbox(nfui_nfobject_get_top(obj), "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
			return FALSE;
		}
#else
        if (g_presetData[g_ch].cnt == 0) return FALSE;

        g_message("%s, %d, number:%d", __FUNCTION__, __LINE__, g_presetData[g_ch]->number[0]);            
        scm_run_ptz_cmd_goto(g_presetData[g_ch].number[0]);
#endif		
	}

	return FALSE;
}

static gboolean _post_dir_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		if (cmd_dir_obj[0] == obj) scm_run_ptz_cmd_left_up();
		else if (cmd_dir_obj[1] == obj)	scm_run_ptz_cmd_up();
		else if (cmd_dir_obj[2] == obj) scm_run_ptz_cmd_right_up();
		else if (cmd_dir_obj[3] == obj)	scm_run_ptz_cmd_left();
		else if (cmd_dir_obj[5] == obj)	scm_run_ptz_cmd_right();
		else if (cmd_dir_obj[6] == obj)	scm_run_ptz_cmd_left_down();
		else if (cmd_dir_obj[7] == obj)	scm_run_ptz_cmd_down();
		else if (cmd_dir_obj[8] == obj)	scm_run_ptz_cmd_right_down();
		
		_update_status_label(-1);
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < 9; i++)
        {
            if (cmd_dir_obj[i] == obj)
                break;
        }

		if (kpid == KEYPAD_UP) 
		{
            if ((i == 0) || (i == 1) || (i == 2))
    			return TRUE;
		}
	}

	return FALSE;
}

static gboolean _post_zoomout_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
        _update_status_label(-1);
		scm_run_ptz_cmd_zoom_out();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
            if (scm_get_ipcam_focus_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, focus_obj[0]);
    			return TRUE;
            }

            if (scm_get_ipcam_iris_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, iris_obj[0]);
    			return TRUE;
            }
          
			return TRUE;
		}
		else if (kpid == KEYPAD_UP)
		{
			return TRUE;
		}		
	}

	return FALSE;
}

static gboolean _post_zoomin_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        _update_status_label(-1);
		scm_run_ptz_cmd_zoom_in();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
            if (scm_get_ipcam_focus_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, focus_obj[1]);
    			return TRUE;
            }

            if (scm_get_ipcam_iris_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, iris_obj[1]);
    			return TRUE;
            }
          
			return TRUE;
		}
		else if (kpid == KEYPAD_UP)
		{
			return TRUE;
		}				
	}

	return FALSE;
}

static gboolean _post_near_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        _update_status_label(-1);
		scm_run_ptz_cmd_focus_near();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
            if (scm_get_ipcam_iris_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, iris_obj[0]);
    			return TRUE;
            }          
          
			return TRUE;
		}
		else if (kpid == KEYPAD_UP)
		{
            if (scm_get_ipcam_zoom_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, zoom_obj[0]);
    			return TRUE;
            }

			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean _post_far_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		_update_status_label(-1);
		scm_run_ptz_cmd_focus_far();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
            if (scm_get_ipcam_iris_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, iris_obj[1]);
    			return TRUE;
            }
          
			return TRUE;
		}
		else if (kpid == KEYPAD_UP)
		{
            if (scm_get_ipcam_zoom_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, zoom_obj[1]);
    			return TRUE;
            }

			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean _post_open_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        _update_status_label(-1);		    
		scm_run_ptz_cmd_iris_open();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
          
			return TRUE;
		}
		else if (kpid == KEYPAD_UP)
		{
            if (scm_get_ipcam_focus_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, focus_obj[0]);
    			return TRUE;
            }

            if (scm_get_ipcam_zoom_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, zoom_obj[0]);
    			return TRUE;
            }

			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean _post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}
		
		_update_status_label(-1);
		scm_run_ptz_cmd_iris_close();
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_DOWN) 
		{
          
			return TRUE;
		}
		else if (kpid == KEYPAD_UP)
		{
            if (scm_get_ipcam_focus_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, focus_obj[1]);
    			return TRUE;
            }

            if (scm_get_ipcam_zoom_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, zoom_obj[1]);
    			return TRUE;
            }

			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean _post_preset_button_select_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == NFEVENT_RADIO_GET_FOCUS_ALWAYS)
    {
        gint idx; 

        idx = nfui_radio_button_get_index((NFBUTTON*)obj);

        g_temp = idx;
        _update_status_label(idx);
        
        if(idx == SCAN)
        {
            if(g_ptzData[g_ch].mode == PTZ_MODE_SCAN)
            {
                nfui_radio_button_unset_toggled(obj);
                _update_status_label(-2);
                
                g_ptzData[g_ch].mode = PTZ_MODE_OFF;
            }
            else
                g_ptzData[g_ch].mode = PTZ_MODE_SCAN;
        }
        else if(idx ==TOUR)
        {
            if(g_ptzData[g_ch].mode == PTZ_MODE_TOUR)
            {
                nfui_radio_button_unset_toggled(obj);
                _update_status_label(-2);
                
                g_ptzData[g_ch].mode = PTZ_MODE_OFF; 
            }
            else
                g_ptzData[g_ch].mode = PTZ_MODE_TOUR;
        }
        else 
            g_ptzData[g_ch].mode = PTZ_MODE_OFF;

        _live_ptz_cmd_pattern(g_ch);

        if( idx < SCAN)
        {   
            scm_run_ptz_cmd_goto(g_presetData[g_ch].number[idx]);
        }
    }
    return FALSE;
}



static gboolean _post_zoom_temp_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_EXPOSE) 
	{
		GdkDrawable *drawable = NULL;
		GdkGC *gc = NULL;
        gint x, y;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);

		if (nfui_nfobject_is_disabled(zoom_label_obj))
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_LIVE_D, x, y,-1, -1, NFALIGN_LEFT, 0);	
        else
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_LIVE_N, x, y,-1, -1, NFALIGN_LEFT, 0);	
		    
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean _post_focus_temp_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_EXPOSE) 
	{
		GdkDrawable *drawable = NULL;
		GdkGC *gc = NULL;
        gint x, y;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);

		if (nfui_nfobject_is_disabled(focus_label_obj))
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_LIVE_D, x, y,-1, -1, NFALIGN_LEFT, 0);	
        else
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_LIVE_N, x, y,-1, -1, NFALIGN_LEFT, 0);	
		    
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

static gboolean _post_iris_temp_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if (event->type == GDK_EXPOSE) 
	{
		GdkDrawable *drawable = NULL;
		GdkGC *gc = NULL;
        gint x, y;

		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);
		nfui_nfobject_get_offset(obj, &x, &y);

		if (nfui_nfobject_is_disabled(iris_label_obj))
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_LIVE_D, x, y,-1, -1, NFALIGN_LEFT, 0);	
        else
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_LIVE_N, x, y,-1, -1, NFALIGN_LEFT, 0);	   
		    
		nfui_nfobject_gc_unref(gc);
	}

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

static gboolean _send_parent_event(GdkEvent *evt) // �ٸ� ä�� ���ý�  x,y, w, h ��ǥ �� ũ�� �� �Ҵ� .
{
    gint x, y, w, h;
    gint ch;

    evt->type = GDK_BUTTON_PRESS;
    nfui_send_event(get_live_object(), evt, TRUE);
    
    g_ch = (gint)vsm_get_current_win_id_size(&x, &y, &w, &h);

    g_x = x;
    g_y = y;
    g_w = w;
    g_h = h;    

    return FALSE;
}

static gboolean _scroll_zoom_ctrl(GdkEventScroll* event)
{
    scm_set_ptz_cmd_zoom_speed(ZOOM_SPEED);    

    switch(event->direction)
    {
        case GDK_SCROLL_UP:
        {
            if(cur_scroll == SCROLL_UP){
            
                zoom_sec = 3;  
                
                return TRUE;
                
            }else if(zoom_sec == -1){
            
                zoom_sec = 3;    
                scm_run_ptz_cmd_zoom_in();            
                cur_scroll = SCROLL_UP;
            }
            else
                return TRUE;
        
            break;
        }
        case GDK_SCROLL_DOWN:
        {
            if(cur_scroll == SCROLL_DOWN){
    
                zoom_sec = 3; 
                
                return TRUE;
                
            }else if(zoom_sec == -1){    
            
                zoom_sec = 3;   
                scm_run_ptz_cmd_zoom_out(); 
                cur_scroll = SCROLL_DOWN;  
            }
            else    
                return TRUE;
        
            break;
        }
        
    }

    return FALSE;
}

static gboolean post_main_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{   
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    gint z_x, z_y, z_w, z_h;

    static guint cur_state = 0; 

    drawable = nfui_nfobject_get_window(get_live_object());
    gc = gdk_gc_new(drawable);
  
    switch(evt->type)
    {       
        case NFOUTEVT_BUTTON_RELEASE :
        {          
            if(evt->button.button != MOUSE_RIGTH_BUTTON) return FALSE;

            cur_state &= ~(1 << RIGHT_BUTTON);
            
            _erase_arrow_line();
            _init_ptz_state(g_ch);            
            
        }
        break;

        case NFOUTEVT_BUTTON_PRESS :
        {     
            GdkEventButton *bevent; 

            bevent = (GdkEventButton*)evt;
        
            if(cur_state & (1 << FOCUS_ZONE))
            {
                //if(scm_get_ipcam_ptz_supp(g_ch) || (cur_state & (1 << RIGHT_BUTTON)))  return FALSE;
                
                if(scm_get_ipcam_ptz_supp(g_ch) || evt->button.button != MOUSE_RIGTH_BUTTON) return FALSE;

                cur_state |= (1 << RIGHT_BUTTON);
                _draw_arrow_line((g_w/2)+g_x, (g_h/2)+g_y, (gint)bevent->x, (gint)bevent->y);
            }
            else
            {
//              _vvm_refresh_vwnd_all();
                _send_parent_event(evt);
                _live_ptz_ctrl_change(g_ch);  
                _live_ptz_prst_change(g_ch);
                                
                cur_state |= (1 << FOCUS_ZONE);
                cur_state &= ~(1 << RIGHT_BUTTON);
            }            
        }
        break;
        
        case NFOUTEVT_MOTION_NOTIFY :
        {
            GdkEventMotion *mevent;
            gint mevt_x, mevt_y;

            mevent = (GdkEventMotion*)evt;

            mevt_x = (gint)mevent->x;
            mevt_y = (gint)mevent->y;

            if(((mevt_x > (g_x+FOCUS_LINE_SIZE)) && (mevt_x < (g_x+g_w-FOCUS_LINE_SIZE))) 
                && ((mevt_y > (g_y+FOCUS_LINE_SIZE)) && (mevt_y < (g_y+g_h-FOCUS_LINE_SIZE))))
            {
                 cur_state |= (1 << FOCUS_ZONE);

                 if(cur_state & (1 << RIGHT_BUTTON))
                 {
                     if(cur_drag)   drag_sec = D_SEC;

                     _erase_arrow_line();
                     _draw_arrow_line((g_w/2)+g_x, (g_h/2)+g_y, mevt_x, mevt_y);
                 }
                 else
                 {                 
                     if(scm_get_ipcam_ptz_supp(g_ch) || evt->button.button != MOUSE_RIGTH_BUTTON) return FALSE;
                     
                     if (mevent->state & GDK_BUTTON1_MASK){                     

                        if(cur_drag)  drag_sec = D_SEC;

                        _erase_arrow_line();
                        _draw_arrow_line((g_w/2)+g_x, (g_h/2)+g_y, mevt_x, mevt_y);  
                     }
                 }
            }
            else
            {
                cur_state &= ~(1 << FOCUS_ZONE);
            }              
        }
        break; 
        
        case NFOUTEVT_SCROLL :
        {
            GdkEventScroll *sevt;
            sevt = (GdkEventScroll*)evt;     
            if(!(cur_state & (1 << FOCUS_ZONE))) return FALSE;

            _update_status_label(-1);
            
            if(scm_get_ipcam_zoom_supp(g_ch))  
                return FALSE;

            if(_scroll_zoom_ctrl(sevt)) 
                return FALSE;
           
            g_timeout_add(100, _scroll_current_check, NULL);
        } 
        break;
        
        case GDK_DELETE :
        {          
            vw_autohide_unset_obj(g_curwnd);
            g_curwnd = 0;
            gtk_main_quit();
        }
        break;        
    }
    
    return FALSE;
}

static gboolean post_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_ENTER_NOTIFY)
    {   
        if(!cur_direction){ 
            #if 0
            _erase_arrow_line(drawable, gc, line, arrow);
            _init_ptz_state(g_ch);            
            _live_ptz_cmd_pattern(g_ch);
            scm_stop_ptz_cmd();
            #endif
            scm_init_ptz_param(g_ch);
        } 
    }
    else if(evt->type == GDK_LEAVE_NOTIFY)
    {

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

static gboolean post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        NFOBJECT *topwin;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;
       
//        _vvm_time_label_on();
//        _vvm_refresh_vwnd_all();

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
    }
    
    return FALSE;
}


void VW_live_ptz_ctrl_popup_Open(NFWINDOW *parent, gint win_x, gint win_y, gint ch, gint ch_x, gint ch_y, gint ch_w, gint ch_h)
{
    NFOBJECT *win;
    NFOBJECT *content_fixed;   
    NFOBJECT *fixed_temp;
    NFOBJECT *fixed;
    NFOBJECT *obj;
    NFOBJECT *ntb;

    GSList *slist = NULL;

    gchar buf[32];    
    gint pos_x, pos_y;
    gint i,j;

    guint ntb1_width[CTRL_TBL1_COLS_CNT] = {0, };
	guint ntb2_width[CTRL_TBL2_COLS_CNT] = {0, };
	guint size_w, size_h;

    GdkPixbuf *dir_img[9][NFOBJECT_STATE_COUNT];

	GdkPixbuf *zoom_out_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoom_in_img[NFOBJECT_STATE_COUNT];
	
	GdkPixbuf *focus_near_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *focus_far_img[NFOBJECT_STATE_COUNT];
	
	GdkPixbuf *iris_open_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *iris_close_img[NFOBJECT_STATE_COUNT];

    // <---- IMAGE LOAD
	dir_img[0][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_LEFT_LIVE), NULL);
	dir_img[0][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_LEFT_LIVE), NULL);
	dir_img[0][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_LEFT_LIVE), NULL);
	dir_img[0][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_LEFT_LIVE), NULL);

	dir_img[1][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_LIVE), NULL);
	dir_img[1][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_LIVE), NULL);
	dir_img[1][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_LIVE), NULL);
	dir_img[1][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_LIVE), NULL);

	dir_img[2][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_RIGHT_LIVE), NULL);
	dir_img[2][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_RIGHT_LIVE), NULL);
	dir_img[2][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_RIGHT_LIVE), NULL);
	dir_img[2][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_RIGHT_LIVE), NULL);

	dir_img[3][0] = nfui_get_image_from_file((IMG_N_PTZ_LEFT_LIVE), NULL);
	dir_img[3][1] = nfui_get_image_from_file((IMG_O_PTZ_LEFT_LIVE), NULL);
	dir_img[3][2] = nfui_get_image_from_file((IMG_P_PTZ_LEFT_LIVE), NULL);
	dir_img[3][3] = nfui_get_image_from_file((IMG_D_PTZ_LEFT_LIVE), NULL);

	dir_img[4][0] = nfui_get_image_from_file((IMG_N_PTZ_CENTER_LIVE), NULL);
	dir_img[4][1] = nfui_get_image_from_file((IMG_O_PTZ_CENTER_LIVE), NULL);
	dir_img[4][2] = nfui_get_image_from_file((IMG_P_PTZ_CENTER_LIVE), NULL);
	dir_img[4][3] = nfui_get_image_from_file((IMG_D_PTZ_CENTER_LIVE), NULL);

	dir_img[5][0] = nfui_get_image_from_file((IMG_N_PTZ_RIGHT_LIVE), NULL);
	dir_img[5][1] = nfui_get_image_from_file((IMG_O_PTZ_RIGHT_LIVE), NULL);
	dir_img[5][2] = nfui_get_image_from_file((IMG_P_PTZ_RIGHT_LIVE), NULL);
	dir_img[5][3] = nfui_get_image_from_file((IMG_D_PTZ_RIGHT_LIVE), NULL);

	dir_img[6][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_LEFT_LIVE), NULL);
	dir_img[6][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_LEFT_LIVE), NULL);
	dir_img[6][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_LEFT_LIVE), NULL);
	dir_img[6][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_LEFT_LIVE), NULL);

	dir_img[7][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_LIVE), NULL);
	dir_img[7][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_LIVE), NULL);
	dir_img[7][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_LIVE), NULL);
	dir_img[7][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_LIVE), NULL);

	dir_img[8][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_RIGHT_LIVE), NULL);
	dir_img[8][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_RIGHT_LIVE), NULL);
	dir_img[8][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_RIGHT_LIVE), NULL);
	dir_img[8][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_RIGHT_LIVE), NULL);

	zoom_out_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOMOUT_LIVE), NULL);
	zoom_out_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOMOUT_LIVE), NULL);
	zoom_out_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOMOUT_LIVE), NULL);
	zoom_out_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOMOUT_LIVE), NULL);

	zoom_in_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOMIN_LIVE), NULL);
	zoom_in_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOMIN_LIVE), NULL);
	zoom_in_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOMIN_LIVE), NULL);
	zoom_in_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOMIN_LIVE), NULL);

	focus_near_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FOCUS_NEAR_LIVE), NULL);
	focus_near_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FOCUS_NEAR_LIVE), NULL);
	focus_near_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FOCUS_NEAR_LIVE), NULL);
	focus_near_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FOCUS_NEAR_LIVE), NULL);

	focus_far_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FOCUS_FAR_LIVE), NULL);
	focus_far_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FOCUS_FAR_LIVE), NULL);
	focus_far_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FOCUS_FAR_LIVE), NULL);
	focus_far_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FOCUS_FAR_LIVE), NULL);

	iris_open_img[0] = nfui_get_image_from_file((IMG_N_PTZ_IRIS_OPEN_LIVE), NULL);
	iris_open_img[1] = nfui_get_image_from_file((IMG_O_PTZ_IRIS_OPEN_LIVE), NULL);
	iris_open_img[2] = nfui_get_image_from_file((IMG_P_PTZ_IRIS_OPEN_LIVE), NULL);
	iris_open_img[3] = nfui_get_image_from_file((IMG_D_PTZ_IRIS_OPEN_LIVE), NULL);

	iris_close_img[0] = nfui_get_image_from_file((IMG_N_PTZ_IRIS_CLOSE_LIVE), NULL);
	iris_close_img[1] = nfui_get_image_from_file((IMG_O_PTZ_IRIS_CLOSE_LIVE), NULL);
	iris_close_img[2] = nfui_get_image_from_file((IMG_P_PTZ_IRIS_CLOSE_LIVE), NULL);
	iris_close_img[3] = nfui_get_image_from_file((IMG_D_PTZ_IRIS_CLOSE_LIVE), NULL);
    
    g_parent = parent;
    g_ch = ch;
   
    g_x = ch_x;
    g_y = ch_y;
    g_w = ch_w;
    g_h = ch_h;

    if(g_curwnd) return ;

	
    win = (NFOBJECT *)nfui_nfwindow_new(parent, win_x, win_y, WIN_WIDTH, WIN_HEIGHT);
    nfui_nfwindow_use_outside_evt((NFWINDOW*)win, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_PRESS, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_2BUTTON_PRESS, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_BUTTON_RELEASE, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_MOTION_NOTIFY, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)win, GDK_SCROLL,TRUE);
    nfui_nfwindow_set_moving_area_size((NFWINDOW*)win, WIN_HEIGHT);
    nfui_nfwindow_set_moving_limit((NFWINDOW*)win, TRUE);
//    nfui_nfwindow_set_moving_effect((NFWINDOW*)win, TRUE);
    nfui_regi_post_event_callback(win, post_main_win_event_handler);
    
    g_curwnd = win;   

    if(g_curwnd == NULL) return;

    _init_data();

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_set_size(content_fixed, WIN_WIDTH, WIN_HEIGHT);
    nfui_regi_post_event_callback(content_fixed, post_fixed_event_cb);   
    nfui_nfobject_show(content_fixed);

    fixed = ( NFOBJECT*)nfui_nffixed_new(); 
    nfui_nfobject_set_size(fixed, WIN_WIDTH, WIN_HEIGHT);
    nfui_nfobject_modify_bg(fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nffixed_put((NFFIXED*)content_fixed, fixed, 0, 0);
    nfui_regi_post_event_callback(fixed, post_fixed_event_handler);
    nfui_nfobject_show(fixed); 

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LIVE PTZ CTRL", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(206));
    nfui_nfobject_modify_bg(obj,NFOBJECT_STATE_NORMAL, COLOR_IDX(604));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);    
    nfui_nfobject_set_size(obj, WIN_WIDTH-8, 36);
    nfui_nfobject_use_focus(obj,NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj,4, 4);


    pos_x = 4;
    pos_y = 44;

    nfui_get_pixbuf_size(dir_img[0][0], &size_w, &size_h);

    ntb1_width[0] = size_w;
	ntb1_width[1] = size_w;
	ntb1_width[2] = size_w;	

    ntb = (NFOBJECT*)nfui_nftable_new(CTRL_TBL1_COLS_CNT, CTRL_TBL1_ROWS_CNT, CTRL_TBL1_COLS_SPACE, CTRL_TBL1_ROWS_SPACE, ntb1_width, size_h);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)fixed, ntb, pos_x+8, pos_y);

	for (j = 0; j < 3; j++)
	{
		for (i = 0; i < 3; i++)
		{
			obj = nfui_nfbutton_new();
			nfui_nfbutton_set_image((NFBUTTON*)obj, dir_img[i+j*3]);
			nfui_nfobject_set_size(obj, size_w, size_h);
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)ntb, obj, i, j);	
			if ((i == 1) && (j == 1))   nfui_regi_post_event_callback(obj, _post_home_button_event_handler);			
			else                        nfui_regi_post_event_callback(obj, _post_dir_button_event_handler);
			cmd_dir_obj[i+j*3] = obj;
		}
	}

	pos_x += (size_w*3+2+41);
    pos_y = 44;

    nfui_get_pixbuf_size(zoom_out_img[0], &size_w, &size_h);
	ntb2_width[0] = size_w;
    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_LIVE_N, &size_w, &size_h);
	ntb2_width[1] = size_w;
    nfui_get_pixbuf_size(zoom_in_img[0], &size_w, &size_h);
	ntb2_width[2] = size_w;	

	ntb = (NFOBJECT*)nfui_nftable_new(CTRL_TBL2_COLS_CNT, CTRL_TBL2_ROWS_CNT, CTRL_TBL2_COLS_SPACE, CTRL_TBL2_ROWS_SPACE, ntb2_width, size_h);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)fixed, ntb, pos_x, pos_y);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, zoom_out_img);
	nfui_get_pixbuf_size(zoom_out_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);
	nfui_regi_post_event_callback(obj, _post_zoomout_button_event_handler);
	zoom_obj[0] = obj;

	fixed_temp = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_show(fixed_temp);
    nfui_nftable_attach((NFTABLE*)ntb, fixed_temp, 1, 0);
	nfui_regi_post_event_callback(fixed_temp, _post_zoom_temp_fixed_event_handler);

    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_LIVE_N, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ZOOM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 70, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));		
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));	
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (size_w-70)/2, 0);
	zoom_label_obj = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, zoom_in_img);
	nfui_get_pixbuf_size(zoom_in_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);
	nfui_regi_post_event_callback(obj, _post_zoomin_button_event_handler);
	zoom_obj[1] = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, focus_near_img);
	nfui_get_pixbuf_size(focus_near_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 1);
	nfui_regi_post_event_callback(obj, _post_near_button_event_handler);
	focus_obj[0] = obj;

	fixed_temp = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));		
	nfui_nfobject_show(fixed_temp);
    nfui_nftable_attach((NFTABLE*)ntb, fixed_temp, 1, 1);
	nfui_regi_post_event_callback(fixed_temp, _post_focus_temp_fixed_event_handler);

    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_LIVE_N, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FOCUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 70, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (size_w-70)/2, 0);
	focus_label_obj = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, focus_far_img);
	nfui_get_pixbuf_size(focus_far_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 1);
    nfui_regi_post_event_callback(obj, _post_far_button_event_handler);
	focus_obj[1] = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, iris_open_img);
	nfui_get_pixbuf_size(iris_open_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 2);
	nfui_regi_post_event_callback(obj, _post_open_button_event_handler);
    iris_obj[0] = obj;

	fixed_temp = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(fixed_temp, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_show(fixed_temp);
    nfui_nftable_attach((NFTABLE*)ntb, fixed_temp, 1, 2);
	nfui_regi_post_event_callback(fixed_temp, _post_iris_temp_fixed_event_handler);

    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_LIVE_N, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IRIS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 70, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));	
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (size_w-70)/2, 0);
	iris_label_obj = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, iris_close_img);
	nfui_get_pixbuf_size(iris_close_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 2);
	nfui_regi_post_event_callback(obj, _post_close_button_event_handler);    
    iris_obj[1] = obj;

    pos_x = 6; 
    pos_y = 175;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRESET/PATROL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 200, 35);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 6);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);		
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 297, 40); 
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)fixed, obj, 8, WIN_HEIGHT-48);  
    g_status_label = obj;

    pos_y +=40;
   
    for(i=0; i<18; i++)
    {
        if(i <SCAN ){
        
            obj = (NFOBJECT*) nftool_normal_button_create_popup_type2(" ",46);
            nfui_nfobject_set_size(obj, 45, 35);
            nfui_nfbutton_set_drawing_outline(obj, FALSE);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);                      
        }
        else if(i > 15){
            pos_x = 4+238;
            pos_y = 175;
            if(i == TOUR) pos_x += 74;
            
            if(i == SCAN)      obj = nftool_normal_button_create_popup_type2("SCAN",70);
            else if(i == TOUR) obj = nftool_normal_button_create_popup_type2("TOUR",70);
            nfui_nfbutton_set_font_alignment(NF_BUTTON(obj),NFALIGN_CENTER, 0);
            nfui_nfbutton_set_drawing_outline(obj, FALSE);
            nfui_nfobject_set_size(obj,70,35);
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)fixed, obj, pos_x, pos_y);
        }
        nfui_regi_post_event_callback(obj, _post_preset_button_select_event_cb);
        
        if(i == 0)
            slist = nfui_radio_button_get_group(NF_BUTTON(obj));
        else
            nfui_radio_button_add_group(NF_BUTTON(obj),slist);
        
        g_ptz_radio[i] = obj;

        if( i > 15 )  continue;

        pos_x += 48;
       
        if(i==7){
            pos_x = 6;
            pos_y += 37;
        }
    }

    pos_x = 8;
    pos_y += 40;

    obj = (NFOBJECT*)nftool_normal_button_create_type1("CLOSE",80);
	nfui_nfobject_set_size(obj, 80, 35);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, WIN_WIDTH-8-80, WIN_HEIGHT-50);
    nfui_regi_post_event_callback(obj, post_close_button_event_handler);

    _live_ptz_ctrl_change(g_ch);    
    _live_ptz_prst_change(g_ch);

    nfui_nfwindow_add((NFWINDOW*)win, fixed);
    nfui_run_main_event_handler(win);

    nfui_nfobject_show(win); 
    nfui_make_key_hierarchy((NFWINDOW*)win); 

    vw_autohide_set_obj(g_curwnd, 5);

    nfui_page_open(PGID_PTZ, win, ssm_get_cur_id(NULL));   

    gtk_main();

    nfui_page_close(PGID_PTZ, win);
}

gint get_win_height()
{
    return WIN_HEIGHT;
}

gint get_win_width()
{
    return WIN_WIDTH;
}

int VW_destroy_live_ptz_ctrl()
{
	if (!g_curwnd) return 0;

	nfui_nfobject_destroy(g_curwnd);
	return 0;
}
