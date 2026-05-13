
#include <string.h>

#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "nf_ui_tool.h"

#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"

#include "vw_record_main.h"
#include "vw_record_alarm.h"
#include "vw_record_data_internal.h"
#include "vw_record_alarm_sched.h"
#include "vw_record_sched_control.h"
#include "vw_copy_sched.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define ALARM_SCHE_TIME_LABEL_COLS_SPACE    (2)

#define ALARM_SCHE_DAY_MENU_W_SUM           (284)

#define ALARM_SCHE_DAY_LABEL_X              (4)
#define ALARM_SCHE_DAY_LABEL_Y              (0)
#define ALARM_SCHE_DAY_LABEL_W              (60)
#define ALARM_SCHE_DAY_LABEL_H              (40)

#define ALARM_SCHE_DAY_DROPDOWN_X           (ALARM_SCHE_TABLE_X + ALARM_SCHE_DAY_LABEL_W)
#define ALARM_SCHE_DAY_DROPDOWN_Y           (ALARM_SCHE_DAY_LABEL_Y)
#define ALARM_SCHE_DAY_DROPDOWN_W           (ALARM_SCHE_DAY_MENU_W_SUM - ALARM_SCHE_DAY_LABEL_W)
#define ALARM_SCHE_DAY_DROPDOWN_H           (ALARM_SCHE_DAY_LABEL_H)

#define ALARM_SCHE_CH_LABEL_W               (232)

#define ALARM_SCHE_TABLE_COL_CNT            (1)
#define ALARM_SCHE_TABLE_ROW_CNT            (GUI_CHANNEL_CNT)

#define ALARM_SCHE_TIME_LABEL_X             (ALARM_SCHE_DAY_LABEL_X+ALARM_SCHE_CH_LABEL_W+2)
#define ALARM_SCHE_TIME_LABEL_Y             (ALARM_SCHE_DAY_LABEL_Y+ALARM_SCHE_DAY_LABEL_H+27)
#define ALARM_SCHE_TIME_LABEL_W             (48)
#define ALARM_SCHE_TIME_LABEL_H             (40)

#define ALARM_SCHE_TABLE_X                  (ALARM_SCHE_DAY_LABEL_X)
#define ALARM_SCHE_TABLE_Y                  (ALARM_SCHE_DAY_LABEL_Y+ALARM_SCHE_DAY_LABEL_H+68)
#define ALARM_SCHE_TABLE_CELL_H             (39)

#define ALARM_SCHE_TABLE_COLS_SPACE         (0)
#define ALARM_SCHE_TABLE_ROWS_SPACE         (2)

#define ALARM_SCHE_TILE2_ROW_CNT            (ROW_CNT_PER_PAGE)
#define ALARM_SCHE_TILE2_COL_CNT            (24)

#define ALARM_SCHE_TILE2_X                  (ALARM_SCHE_CH_LABEL_W+2)
#define ALARM_SCHE_TILE2_Y                  (ALARM_SCHE_TABLE_Y-1)
#define ALARM_SCHE_TILE2_W                  (((48+2) * 24))
#define ALARM_SCHE_TILE2_H                  ((ALARM_SCHE_TABLE_CELL_H+2)*ALARM_SCHE_TILE2_ROW_CNT)


#define ALARM_SCHE_COPY_BTN_X               (MENU_V_SUBTAB_INNER_W - 226 - 7)
#define ALARM_SCHE_COPY_BTN_Y               (MENU_V_SUBTAB_INNER_H - 40 - 4)


#define ALARM_SCHE_ON_LABEL_W               (80)
#define ALARM_SCHE_ON_LABEL_H               (26)
#define ALARM_SCHE_ON_LABEL_X               (MENU_V_SUBTAB_INNER_W - ALARM_SCHE_ON_LABEL_W - 7)
#define ALARM_SCHE_ON_LABEL_Y               (ALARM_SCHE_DAY_LABEL_Y + ALARM_SCHE_DAY_LABEL_H)

#define ALARM_SCHE_ON_LEGEND_W              (22)
#define ALARM_SCHE_ON_LEGEND_H              (22)
#define ALARM_SCHE_ON_LEGEND_X              (ALARM_SCHE_ON_LABEL_X - 6 - ALARM_SCHE_ON_LEGEND_W)
#define ALARM_SCHE_ON_LEGEND_Y              (ALARM_SCHE_ON_LABEL_Y)

#define ALARM_SCHE_OFF_LABEL_W              (80)
#define ALARM_SCHE_OFF_LABEL_H              (26)
#define ALARM_SCHE_OFF_LABEL_X              (ALARM_SCHE_ON_LEGEND_X - ALARM_SCHE_OFF_LABEL_W)
#define ALARM_SCHE_OFF_LABEL_Y              (ALARM_SCHE_ON_LABEL_Y)

#define ALARM_SCHE_OFF_LEGEND_W             (22)
#define ALARM_SCHE_OFF_LEGEND_H             (22)
#define ALARM_SCHE_OFF_LEGEND_X             (ALARM_SCHE_OFF_LABEL_X - ALARM_SCHE_OFF_LEGEND_W - 7)
#define ALARM_SCHE_OFF_LEGEND_Y             (ALARM_SCHE_ON_LABEL_Y)



enum {
    CMRSB_CANCEL = 0,
    CMRSB_APPLY,
    CMRSB_CLOSE,
    CMRSB_BUTTONS
};


static guint set_days = REC_SUN_PARAM; 


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *days_combo;
static NFOBJECT *days_label;
static NFOBJECT *copy_btn;
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;
static NFOBJECT *g_tile[PAGE_FIXED_CNT];


static void redraw_tile_modeArea(NFTILE *tile, guint day)
{
    guint i, j;
    guint s_ch, e_ch;
    guint ch;
    guint time = 24;
    guint color_n = 1;
    gchar buf[64];
    NFTILE *tmp_tile;
    guint tile_idx;


    tile_idx = GPOINTER_TO_INT(nfui_nfobject_get_data((NFOBJECT*)tile, "tile index"));
    if (tile_idx == PAGE_FIXED_CNT) return;

    s_ch = tile_idx * ALARM_SCHE_TILE2_ROW_CNT;
    e_ch = s_ch + ALARM_SCHE_TILE2_ROW_CNT;

    if(nfui_tile_drawable(NF_TILE(tile))) {
        for(ch = s_ch; ch < e_ch; ch++) {
            for(j=0; j<time; j++) {
                get_record_alarm_sche_data(day, ch, j);

                if(!g_ascii_strcasecmp(alarm_info[ch][PARAM_MODE_COL], "0")) 
                    color_n = SELECT_STATE_COLOR_2;
                else if(!g_ascii_strcasecmp(alarm_info[ch][PARAM_MODE_COL], "1")) 
                    color_n = SELECT_STATE_COLOR_3;

                i = ch - s_ch;
                if (nfui_nfobject_is_shown((NFOBJECT*)tile)) nfui_tile_draw_color(NF_TILE(tile), color_n, i, j, i, j);
                else nfui_tile_no_draw_color(NF_TILE(tile), color_n, i, j, i, j);
            }
        }
    }
}

static gboolean tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint s_row = 0, s_col = 0;
    guint e_row = 0, e_col = 0;
    guint i, j;
    guint color_n = 0;
    guint mode;
    gchar buf[10];
    gint tile_idx, ch;


    switch(evt->type) {
        case NFEVENT_TILE_INIT:
        {
            if(get_record_schedMode()) 
                redraw_tile_modeArea(NF_TILE(obj), set_days);
            else        
                redraw_tile_modeArea(NF_TILE(obj), REC_DAILY_PARAM);
        }       
        break;
        
        case NFEVENT_TILE_START_SELECT:
            NF_TILE(obj)->select_n = SELECT_STATE_COLOR_1;
        break;
                        
        case NFEVENT_TILE_MOVE_SELECT:
        break;

        case NFEVENT_TILE_END_SELECT:
        {
            gint x, y;
            gint mx, my;
            int off_x, off_y;

            nfui_tile_get_selectArea(NF_TILE(obj), &s_row, &s_col, &e_row, &e_col);
            x = (50 * (e_col + 1));
            y = (41 * (e_row + 1)); 

            nfui_nfobject_get_offset(obj, &off_x, &off_y);
            x += off_x;
            y += off_y;

            gdk_display_get_pointer(gdk_display_get_default(),
                                NULL, &mx, &my, NULL);

            if (mx > x - 50 && mx < x + 50) x = mx;
            if (my > y - 41 && my < y + 41) y = my;

            gdk_display_get_pointer(gdk_display_get_default(),
                                    NULL, &mx, &my, NULL);
            mode = VW_RecSched_Control_Page(g_curwnd, x, y);

            if (mode == SCHE_CANCEL_EVT)
            {
                if(get_record_schedMode())
                    redraw_tile_modeArea(NF_TILE(obj), set_days);
                else
                    redraw_tile_modeArea(NF_TILE(obj), REC_DAILY_PARAM);
            }
            else
            {

                switch(mode) {
                    case SCHE_OFF_EVT:
                        color_n = SELECT_STATE_COLOR_2;
                    break;
                    case SCHE_ON_EVT:
                        color_n = SELECT_STATE_COLOR_3;
                    break;
                }

                memset(buf, 0, sizeof(buf));
                g_sprintf(buf, "%d", mode);

                tile_idx = GPOINTER_TO_INT(nfui_nfobject_get_data(obj, "tile index"));

                for(i = s_row; i <= e_row; i++) 
                {
                    ch = i + (ALARM_SCHE_TILE2_ROW_CNT * tile_idx);
                    
                    for(j = s_col; j <= e_col; j++) 
                    {
                        if(get_record_schedMode())  
                            get_record_alarm_sche_data(set_days, ch, j);
                        else                        
                            get_record_alarm_sche_data(REC_DAILY_PARAM, ch, j);

                        alarm_info[ch][PARAM_MODE_COL] = buf;

                        if(get_record_schedMode())  
                            set_record_alarm_sche_info(set_days, ch, j);
                        else                    
                            set_record_alarm_sche_info(REC_DAILY_PARAM, ch, j);
                    }
                }

                nfui_tile_draw_color(NF_TILE(obj), color_n, s_row, s_col, e_row, e_col);
            }
        }
        break;

        default:
        break;
    }

    return FALSE;
}

static gboolean post_prev_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];
    
    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == 0) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i--;
        
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);
    	
        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean post_next_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i;
    gchar buf[64];

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
          return FALSE;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++) {
            if (nfui_nfobject_is_shown((NFOBJECT*)g_page_fixed[i])) {
                break;
            }
        }

        if (i == PAGE_FIXED_CNT) return FALSE;

        if (i == (PAGE_FIXED_CNT - 1)) return FALSE;
        
		nfui_on_backscr(obj);
		nfui_rflip(obj);

        nfui_nfobject_hide(g_page_fixed[i]);

        i++;
                
        memset(buf, 0x00, sizeof(buf));
        g_sprintf(buf, "%d / %d", i + 1, PAGE_FIXED_CNT);
        nfui_nflabel_set_text(g_lb_page_num, buf);
        nfui_signal_emit(g_lb_page_num, GDK_EXPOSE, TRUE);

        nfui_nfobject_show(g_page_fixed[i]);
        nfui_signal_emit(g_page_fixed[i], GDK_EXPOSE, TRUE);

        nfui_make_key_hierarchy((NFWINDOW*)nfui_nfobject_get_top(obj));

		nfui_flip(obj);
		nfui_off_backscr(obj);
    }

    return FALSE;
}

static gboolean days_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *tile = NULL;
    gchar *day = NULL;
    guint i, j;
    guint time = 24;
    gint color_n = 0;
    gint tile_cnt;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) {

        day = nfui_combobox_get_value(NF_COMBOBOX(obj));

        if(day) {
            set_days = get_days(day);

            for (i = 0; i < PAGE_FIXED_CNT; i++)
            {
                redraw_tile_modeArea(NF_TILE(g_tile[i]), set_days);
            }
        }
    }

    return FALSE;
}


static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
    
        if(is_changed_alarm_data()) {
            if(!set_record_alarm_db(REC_SCHED_DATA_CHANGED)) {
                g_warning("%s : set record alarm error", __FUNCTION__);
            }

            vw_send_notify_change_record_data();
            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        }
    }

    return FALSE;
}

static gboolean post_copy_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        guint day_mask = 0;
        gint ch, day, time;

        if(evt->button.button == MOUSE_RIGTH_BUTTON) 
            return FALSE;
	
    	if(set_days == 8){
    		day_mask = VW_Copy_Sched(g_curwnd, set_days-1);
    	}else{
        	day_mask = VW_Copy_Sched(g_curwnd, set_days);
    	}
    	
        for(ch = 0; ch < GUI_CHANNEL_CNT; ch++) 
        {
            for(time = 0; time < 24; time++) 
            {
                get_record_alarm_sche_data(set_days, ch, time);

                for (day = REC_SUN_PARAM; day <= REC_SAT_PARAM; day++) 
                {
                    if (day == set_days) continue;
                    if (day_mask & (1 << day))  
                        set_record_alarm_sche_info(day, ch, time);
                }
                
                if (day_mask & (1 << (REC_USR_HOL_PARAM) )) {
                        set_record_alarm_sche_info(REC_USR_HOL_PARAM, ch, time);
        		}
            }
        }
    }

    return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *tile = NULL;
    gint day_param;
    gint i;

    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
    
        if(get_record_schedMode()) {
            if(!get_record_alarm_db(set_days, 0)) 
                g_warning("%s : get record alarm error", __FUNCTION__);

            day_param = set_days;
        }else {
            if(!get_record_alarm_db(REC_DAILY_PARAM, 0)) {
                g_warning("%s : get record alarm error", __FUNCTION__);
            }

            day_param = REC_DAILY_PARAM;
        }

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            redraw_tile_modeArea(NF_TILE(g_tile[i]), day_param);
        }
    }
    
    return FALSE;
}


static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
    
        VW_RecAlarm_tab_out_handler();      
        VW_RecordSetup_Destroy(obj);
    }

    return FALSE;
}


static gboolean sched_page_parent_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *tile = NULL;
    GdkDrawable *drawable;
    GdkGC *gc;
    GdkColor off_color = UX_COLOR(800); 
    GdkColor on_color = UX_COLOR(801);  
    gint color_n = 0;
    guint time = 24;
    guint i;
    guint legend_x;
    guint x, y;
    gint day_param;


    if(evt->type == GDK_EXPOSE) {

        drawable = nfui_nfobject_get_window((NFOBJECT*)obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_offset(obj, &x, &y);
    
        gdk_gc_set_rgb_fg_color(gc, &off_color);                    // OFF
        gdk_draw_rectangle(drawable, gc, TRUE, x+ALARM_SCHE_OFF_LEGEND_X, y+ALARM_SCHE_OFF_LEGEND_Y, ALARM_SCHE_OFF_LEGEND_W, ALARM_SCHE_OFF_LEGEND_H);

        gdk_gc_set_rgb_fg_color(gc, &on_color);                     // ON
        gdk_draw_rectangle(drawable, gc, TRUE, x+ALARM_SCHE_ON_LEGEND_X, y+ALARM_SCHE_ON_LEGEND_Y, ALARM_SCHE_ON_LEGEND_W, ALARM_SCHE_ON_LEGEND_H);

        nfui_nfobject_gc_unref(gc);

        if(get_record_schedMode()) {
            if(days_combo->show == NFOBJECT_HIDE) {
                nfui_nfobject_show(days_label);         
                nfui_nfobject_show(days_combo);
                nfui_nfobject_show(copy_btn);
                nfui_make_key_hierarchy(nfui_nfobject_get_top(days_combo));
            }
            day_param = set_days;
        }else {
            if(days_combo->show == NFOBJECT_SHOW) {
                nfui_nfobject_hide(days_label); 
                nfui_nfobject_hide(days_combo);
                nfui_nfobject_hide(copy_btn);               
                nfui_make_key_hierarchy(nfui_nfobject_get_top(days_combo));
            }
            day_param = REC_DAILY_PARAM;
        }       

        for (i = 0; i < PAGE_FIXED_CNT; i++)
        {
            redraw_tile_modeArea(NF_TILE(g_tile[i]), day_param);
        }
    }
    else if(evt->type == GDK_DELETE) {
        if(set_days != REC_SUN_PARAM)
            set_days = REC_SUN_PARAM; 

        g_curwnd = 0;
    }
    

    return FALSE;
}


void VW_Init_RecAlarmSched_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *ntb;  
    NFOBJECT *nftile;
    NFOBJECT *obj;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    GdkPixbuf *pbCamImage[32];
    GdkColor tile_color = UX_COLOR(801);
    GdkColor ttile_color[4] = {UX_COLOR(TODO_COLOR),    
                                UX_COLOR(800),  
                                UX_COLOR(801),  
                                UX_COLOR(TODO_COLOR)};

    gchar *days[8] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "HOL"};
    gchar strBuf[STRING_SIZE_64];

    guint width[] = {ALARM_SCHE_CH_LABEL_W};
    guint x, y;
    guint i,space;
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];


    g_curwnd = nfui_nfobject_get_top(parent);


// CAMERA IMAGE LOAD
    pbCamImage[0] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
    pbCamImage[1] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
    pbCamImage[2] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
    pbCamImage[3] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL); 
    pbCamImage[4] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);      
    pbCamImage[5] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
    pbCamImage[6] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
    pbCamImage[7] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL); 
    pbCamImage[8] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
    pbCamImage[9] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);  
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);         
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL); 
    pbCamImage[16] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL);
    pbCamImage[18] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);  
    pbCamImage[20] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL); 
    pbCamImage[21] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL);
    pbCamImage[24] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);      
    pbCamImage[25] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL); 
    pbCamImage[26] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL);
    pbCamImage[28] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);  
    pbCamImage[30] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL); 
    pbCamImage[31] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL); 

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    content_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_set_size(content_fixed, MENU_V_SUBTAB_INNER_W, MENU_V_SUBTAB_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_SUBTAB_INNER_X, MENU_V_SUBTAB_INNER_Y);
    

// TIME LABEL
    for (i = 0; i < 24; i++)
    {
        sprintf(strBuf, "%02d", i); 
        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(obj, ALARM_SCHE_TIME_LABEL_W, ALARM_SCHE_TIME_LABEL_H);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, ALARM_SCHE_TIME_LABEL_X + (ALARM_SCHE_TIME_LABEL_COLS_SPACE+ALARM_SCHE_TIME_LABEL_W)*i, ALARM_SCHE_TIME_LABEL_Y);
    }

// DAY LABEL
    days_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("YOIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_use_focus(days_label, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(days_label, ALARM_SCHE_DAY_LABEL_W, ALARM_SCHE_DAY_LABEL_H);
    nfui_nflabel_set_align((NFLABEL*)days_label, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(days_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_hide(days_label);
    nfui_nffixed_put((NFFIXED*)content_fixed, days_label, ALARM_SCHE_DAY_LABEL_X, ALARM_SCHE_DAY_LABEL_Y);


// DAY DROP DOWN
    days_combo = nfui_combobox_new(days, 8, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(days_combo), NFCOMBOBOX_TYPE_SUBTAB_1);
    nfui_nfobject_set_size(days_combo, ALARM_SCHE_DAY_DROPDOWN_W, ALARM_SCHE_DAY_DROPDOWN_H);
    nfui_nfobject_hide(days_combo);
    nfui_nffixed_put((NFFIXED*)content_fixed, days_combo, ALARM_SCHE_DAY_DROPDOWN_X, ALARM_SCHE_DAY_DROPDOWN_Y);
    nfui_regi_post_event_callback(days_combo, days_combo_event_handler);

    if (!ivsc.dfunc.support_usrdef_holiday) {
    	nfui_combobox_remove_data(NF_COMBOBOX(days_combo) , "HOL");
    }

// LEGEND LABEL - ON, OFF
    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OFF", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, ALARM_SCHE_OFF_LABEL_W, ALARM_SCHE_OFF_LABEL_H);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ALARM_SCHE_OFF_LABEL_X, ALARM_SCHE_OFF_LABEL_Y);

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ON", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(968));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(obj, ALARM_SCHE_ON_LABEL_W, ALARM_SCHE_ON_LABEL_H);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ALARM_SCHE_ON_LABEL_X, ALARM_SCHE_ON_LABEL_Y);


// TABLE - CH
    size_w = 0;
    for (i = 0; i < ALARM_SCHE_TABLE_COL_CNT; i++) {
        size_w += width[i];
    }
    size_w += ALARM_SCHE_TILE2_W;

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 60);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, ALARM_SCHE_TABLE_X, ALARM_SCHE_TABLE_Y);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(ALARM_SCHE_TABLE_COL_CNT, ROW_CNT_PER_PAGE, ALARM_SCHE_TABLE_COLS_SPACE, 1, width, 40);
        nfui_nfobject_show(page_ntb[i]);
        nfui_nffixed_put((NFFIXED*)g_page_fixed[i], page_ntb[i], 0, 0);
    }
    nfui_nfobject_show(g_page_fixed[0]);

	nfui_get_pixbuf_size(prev_img[0], &size_w, &size_h);
	
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), prev_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) - (size_w + 60), main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_prev_event_handler);
    
	memset(strBuf, 0x00, sizeof(strBuf));
	g_sprintf(strBuf, "1 / %d", PAGE_FIXED_CNT);
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(obj, 100, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width - 100) / 2, main_page_fixed->height - size_h);
    g_lb_page_num = obj;
    
	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), next_img);
	nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_page_fixed, obj, (main_page_fixed->width / 2) + 60, main_page_fixed->height - size_h);
	nfui_regi_post_event_callback(obj, post_next_event_handler);


// MAKE CH LABEL
    page_num = row_num = 0;

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        var_get_camtitle(strBuf, i);
        
        obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));            
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);  

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

// TIME TILE
    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        nftile = nfui_tile_new(ALARM_SCHE_TILE2_ROW_CNT, ALARM_SCHE_TILE2_COL_CNT);
        nfui_nfobject_set_size(nftile, ALARM_SCHE_TILE2_W, ALARM_SCHE_TILE2_H);
        nfui_tile_set_color(NF_TILE(nftile), NFTILE_STATE_FOCUS, &UX_COLOR(802)); 
        nfui_tile_set_color(NF_TILE(nftile), NFTILE_STATE_SELECT, ttile_color); 
        nfui_nfobject_show(nftile);
    	nfui_nffixed_put((NFFIXED*)g_page_fixed[i], nftile, ALARM_SCHE_TILE2_X, 0);
        nfui_regi_post_event_callback(nftile, tile_event_handler);
        g_tile[i] = nftile;
        
        nfui_nfobject_set_data((NFOBJECT*)nftile, "tile index", GINT_TO_POINTER(i));
    }


// COPY SCHEDULE TO BUTTON
    obj = nftool_normal_button_create_type1("COPY SCHEDULE TO", 226);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_hide(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, ALARM_SCHE_COPY_BTN_X, ALARM_SCHE_COPY_BTN_Y);
    nfui_regi_post_event_callback(obj, post_copy_button_event_handler);
    copy_btn = obj;



// CANCEL, APPLY, CLOSE BUTTON
    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R3_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R2_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_SUBTAB_BTN_R1_X, MENU_V_SUBTAB_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

    nfui_regi_pre_event_callback(content_fixed, sched_page_parent_event_handler);
}


guint VW_RecAlarmParam_get_sched_day()
{
    if(get_record_schedMode())
        return set_days;
    else
        return REC_DAILY_PARAM;
}

