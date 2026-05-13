#include <string.h>

#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"

#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfscrolledfixed.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nftile.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"


#include "tools/nf_ui_tool.h"
#include "ix_mem.h"

#include "vw_record_main.h"
#include "vw_record_continuous.h"
#include "vw_record_data_internal.h"
#include "vw_record_continuous_param.h"
#include "vw_record_param_sub.h"
#include "vw_passage_popup.h"


#define CONT_PARAM_TIME_LABEL_X             (4+292+2)
#define CONT_PARAM_TIME_LABEL_Y             (0)
#define CONT_PARAM_TIME_LABEL_W             (48)
#define CONT_PARAM_TIME_LABEL_H             (40)

#define CONT_PARAM_TIME_LABEL_COLS_SPACE    (2)

#define CONT_PARAM_TILE_X                   (CONT_PARAM_TIME_LABEL_X - 1)
#define CONT_PARAM_TILE_Y                   (CONT_PARAM_TIME_LABEL_Y+CONT_PARAM_TIME_LABEL_H+2)
#define CONT_PARAM_TILE_W                   ((CONT_PARAM_TIME_LABEL_W+2) * 24)
#define CONT_PARAM_TILE_H                   (CONT_PARAM_TIME_LABEL_H)

#define CONT_PARAM_DAY_MENU_W_SUM           (284)

#define CONT_PARAM_DAY_LABEL_X              (CONT_PARAM_TABLE_X)
#define CONT_PARAM_DAY_LABEL_Y              (CONT_PARAM_TILE_Y)
#define CONT_PARAM_DAY_LABEL_W              (60)
#define CONT_PARAM_DAY_LABEL_H              (40)

#define CONT_PARAM_DAY_DROPDOWN_X           (CONT_PARAM_TABLE_X + CONT_PARAM_DAY_LABEL_W)
#define CONT_PARAM_DAY_DROPDOWN_Y           (CONT_PARAM_DAY_LABEL_Y)
#define CONT_PARAM_DAY_DROPDOWN_W           (CONT_PARAM_DAY_MENU_W_SUM - CONT_PARAM_DAY_LABEL_W)
#define CONT_PARAM_DAY_DROPDOWN_H           (CONT_PARAM_DAY_LABEL_H)

#define CONT_PARAM_TABLE_COL_CNT            (5)
#define CONT_PARAM_TABLE_ROW_CNT            (GUI_CHANNEL_CNT)

#define CONT_PARAM_TABLE_X                  (4)
#define CONT_PARAM_TABLE_Y                  (CONT_PARAM_TILE_Y+CONT_PARAM_TILE_H+27)
#define CONT_PARAM_TABLE_CELL_H             (39)

#define CONT_PARAM_TABLE_COLS_SPACE         (2)
#define CONT_PARAM_TABLE_ROWS_SPACE         (2)

#define CONT_PARAM_HELP_BTN_X               (CONT_PARAM_TABLE_X)
#define CONT_PARAM_HELP_BTN_Y               (MENU_V_SUBTAB_INNER_H - 40 - 4)

#define HELP_STR1 "If you want to set the time interval, drag the mouse to the time table at the top of the screen and a recording information setting window should appear."
#define HELP_STR2 "In the recording information settings window, you can set the desired resolution and quality, FPS, audio recording, and whether to save or not."
#define HELP_STR3 "If there is a different setting value, the time table will be displayed in a different color."
#define HELP_STR4 "If you hover the mouse pointer over the top of the time table, the corresponding time zone information will be displayed for the recording settings."
#define HELP_STR5 "Change the mode of schedule in operation mode page to change hourly, daily setup."


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *lbSize[GUI_CHANNEL_CNT];
static NFOBJECT *lbFPS[GUI_CHANNEL_CNT];
static NFOBJECT *lbQuality[GUI_CHANNEL_CNT];
static NFOBJECT *lbAudio[GUI_CHANNEL_CNT];

static NFOBJECT *days_combo;
static NFOBJECT *days_label;

static gboolean isReloaded = FALSE;


static void _set_size_data_text_color(gint ch)
{
    gboolean val;
    guint fg_color;

    val = get_continuous_size_data_modify(ch);

    fg_color = nfui_nflabel_get_pango_font_color(lbSize[ch]);

    if (val)
    {
        if (fg_color != 190)
        {
            nfui_nflabel_modify_fg(lbSize[ch], COLOR_IDX(190));
            nfui_signal_emit(lbSize[ch], GDK_EXPOSE, TRUE);
        }
    }
    else
    {
        if (fg_color != 921)
        {
            nfui_nflabel_modify_fg(lbSize[ch], 921);
            nfui_signal_emit(lbSize[ch], GDK_EXPOSE, TRUE);
        }
    }
}

static void _set_fps_data_text_color(gint ch)
{
    gboolean val;
    guint fg_color;

    val = get_continuous_fps_data_modify(ch);

    fg_color = nfui_nflabel_get_pango_font_color(lbFPS[ch]);

    if (val)
    {
        if (fg_color != 190)
        {
            nfui_nflabel_modify_fg(lbFPS[ch], COLOR_IDX(190));
            nfui_signal_emit(lbFPS[ch], GDK_EXPOSE, TRUE);
        }
    }
    else
    {
        if (fg_color != 921)
        {
            nfui_nflabel_modify_fg(lbFPS[ch], COLOR_IDX(921));
            nfui_signal_emit(lbFPS[ch], GDK_EXPOSE, TRUE);
        }
    }
}

static void _set_param_data_text_color()
{
    gint ch;

    for(ch = 0; ch < GUI_CHANNEL_CNT; ch++) 
    {
        _set_size_data_text_color(ch);
        _set_fps_data_text_color(ch);
    }
}


static void display_continuous_rec_param_info(gboolean redraw) 
{
    gint i;
    gchar *size, *fps, *qual, *audio;

    for(i=0; i<GUI_CHANNEL_CNT; i++) {

        size = nfui_nflabel_get_text((NFLABEL*)lbSize[i]);
        fps = nfui_nflabel_get_text((NFLABEL*)lbFPS[i]);
        qual = nfui_nflabel_get_text((NFLABEL*)lbQuality[i]);
        audio = nfui_nflabel_get_text((NFLABEL*)lbAudio[i]);

        if(strcmp(size, continuous_info[i][PARAM_SIZE_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbSize[i], continuous_info[i][PARAM_SIZE_COL]);
            nfui_nflabel_modify_fg((NFLABEL*)lbSize[i], COLOR_IDX(921));

            if(redraw) 
                nfui_signal_emit(lbSize[i], GDK_EXPOSE, TRUE);
        }

        if(strcmp(fps, continuous_info[i][PARAM_FPS_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbFPS[i], continuous_info[i][PARAM_FPS_COL]);
            nfui_nflabel_modify_fg((NFLABEL*)lbFPS[i], COLOR_IDX(921));

            if(redraw) 
                nfui_signal_emit(lbFPS[i], GDK_EXPOSE, TRUE);
        }

        if(strcmp(qual, continuous_info[i][PARAM_QUAL_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbQuality[i], continuous_info[i][PARAM_QUAL_COL]);

            if(redraw) 
                nfui_signal_emit(lbQuality[i], GDK_EXPOSE, TRUE);
        }

        if(strcmp(audio, continuous_info[i][PARAM_AUDIO_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbAudio[i], continuous_info[i][PARAM_AUDIO_COL]);

            if(redraw) 
                nfui_signal_emit(lbAudio[i], GDK_EXPOSE, TRUE);
        }
    }
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {

        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
    
		if(is_changed_continuous_data()) {		
        
            if(!set_record_continuous_db(REC_PARAM_DATA_CHANGED))
                g_error("%s : set record continuous error", __FUNCTION__);

            vw_send_notify_change_record_data();
            nftool_mbox_auto(g_curwnd, 1, "NOTICE", "Configuration has been saved.");
        }
    }

    return FALSE;
}


static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_BUTTON_RELEASE)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }
    
        if(get_record_schedMode()) {
            gchar *day = NULL;
            guint set_days;

            day = nfui_combobox_get_value(NF_COMBOBOX(days_combo));
            set_days = get_days(day);
        
            if(!get_record_continuous_db(set_days, 0))
                g_error("%s : get record continuous error", __FUNCTION__);

            display_continuous_rec_param_info(TRUE);
        }else {
            if(!get_record_continuous_db(REC_DAILY_PARAM, 0))
                g_error("%s : get record continuous error", __FUNCTION__);

            display_continuous_rec_param_info(TRUE);
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

        VW_RecContinuous_tab_out_handler();
        VW_RecordSetup_Destroy(obj);
    }

    return FALSE;
}


static gboolean tile_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *day = NULL;
    guint set_days, sche_mode;
    
    guint s_row = 0, s_col = 0;
    guint e_row = 0, e_col = 0;

    guint color_n = 0;

    switch(evt->type) {
        case NFEVENT_TILE_INIT:
        break;

        case NFEVENT_TILE_START_SELECT:
            NF_TILE(obj)->select_n = FOCUS_STATE_COLOR;
        break;

        case NFEVENT_TILE_MOVE_SELECT:
        {       
            nfui_tile_get_end_area(NF_TILE(obj), &e_row, &e_col);

            sche_mode = get_record_schedMode();

            if ( sche_mode ) {
                day = nfui_combobox_get_value(NF_COMBOBOX(days_combo));
                set_days = get_days(day);
            }

            if( sche_mode )
                get_record_continuous_param_data(set_days, e_col);
            else 
                get_record_continuous_param_data(REC_DAILY_PARAM, e_col);

            _set_param_data_text_color();
            display_continuous_rec_param_info(TRUE);
        }
        break;

        case NFEVENT_TILE_END_SELECT:
        {
            nfui_tile_get_selectArea(NF_TILE(obj), &s_row, &s_col, &e_row, &e_col);

            if(VW_RecParam_Submenu_Open(g_curwnd, continuous_info, REC_PARAM_NORMAL_OPEN))
            {   
                sche_mode = get_record_schedMode();

                if (sche_mode)
                {
                    day = nfui_combobox_get_value(NF_COMBOBOX(days_combo));
                    set_days = get_days(day);

                    set_record_continuous_param_info(set_days, s_col, e_col);
                }
                else
                {
                    set_record_continuous_param_info(REC_DAILY_PARAM, s_col, e_col);
                }

                display_continuous_rec_param_info(TRUE);
            }

			color_n = NORMAL_STATE_COLOR;
			nfui_tile_draw_color(NF_TILE(obj), color_n, s_row, s_col, e_row, e_col);
        }
        break;

        default:
        break;
    }

    return FALSE;
}

static gboolean days_combo_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gchar *day = NULL;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED){
    
        day = nfui_combobox_get_value(NF_COMBOBOX(days_combo));
        
        if(day) {
            get_record_continuous_param_data(get_days(day), 0);
            display_continuous_rec_param_info(TRUE);
        }
    }

    return FALSE;
}

static gboolean post_help_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_BUTTON_RELEASE || kpid == KEYPAD_ENTER)
    {
        NFOBJECT *top;
        guint x, y;
        PARAGRAPH_STR *para;
        gint i, cnt = 0;

        if (evt->button.button == MOUSE_RIGTH_BUTTON)  
            return FALSE;

        nfui_nfobject_get_offset(obj, &x, &y);
        top = nfui_nfobject_get_top(obj);

        x += top->x;
        y += top->y + obj->height + 2;
        
        para = imalloc(sizeof(PARAGRAPH_STR));
        
        para->body[cnt++] = g_strdup(HELP_STR1);
        para->body[cnt++] = g_strdup(HELP_STR2);
//        para->body[cnt++] = g_strdup(HELP_STR3);
        para->body[cnt++] = g_strdup(HELP_STR4);
        para->body[cnt++] = g_strdup(HELP_STR5);        
        para->body_cnt = cnt;

        vw_passage_popup_open(g_curwnd, x, y, DIR_TOP_RIGHT, &para, 1);

        for (i = 0; i < para->intro_cnt; i++)
        {
            if (para->intro[i]) g_free(para->intro[i]);
        }

        for (i = 0; i < para->body_cnt; i++)
        {
            if (para->body[i]) g_free(para->body[i]);
        }

        ifree(para);
    }

    return FALSE;
}

static gboolean param_page_parent_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    GdkGC *gc;
    GdkDrawable *drawable;
    guint x, y;

    gchar *day = NULL;
    guint set_days;

    if(evt->type == GDK_EXPOSE) {
        if(get_record_schedMode()) {            //weekly
            if(days_combo->show == NFOBJECT_HIDE) {
                nfui_nfobject_show(days_label);
                nfui_nfobject_show(days_combo);             
                nfui_make_key_hierarchy(nfui_nfobject_get_top(days_combo));

                day = nfui_combobox_get_value(NF_COMBOBOX(days_combo));

                get_record_continuous_param_data(get_days(day), 0);
                display_continuous_rec_param_info(FALSE);
            }

            if(isReloaded) {    
                display_continuous_rec_param_info(FALSE);
                isReloaded = FALSE;
            }
        } else {                            //daily
            if(days_combo->show == NFOBJECT_SHOW) {
                nfui_nfobject_hide(days_label); 
                nfui_nfobject_hide(days_combo);
                nfui_make_key_hierarchy(nfui_nfobject_get_top(days_combo));

                get_record_continuous_param_data(REC_DAILY_PARAM, 0);
                display_continuous_rec_param_info(FALSE);
            }

            if(isReloaded) {    
                display_continuous_rec_param_info(FALSE);
                isReloaded = FALSE;
            }
            
        }   
    }else if(evt->type == GDK_DELETE) {
        if(isReloaded)
            isReloaded = FALSE;

        g_curwnd = 0;
    }

    return FALSE;
}


void VW_Init_RecContParam_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *scrolled_fixed;
    NFOBJECT *ntb1;
    NFOBJECT *obj;
    NFOBJECT *nftile;
    NFOBJECT *cam_ch[GUI_CHANNEL_CNT];

    GdkPixbuf *pbCamImage[32];
    GdkColor select_color[4] = {UX_COLOR(801),  
                                UX_COLOR(801),
                                UX_COLOR(801),
                                UX_COLOR(801)};

    gchar *strTitle[CONT_PARAM_TABLE_COL_CNT] = {"", "RESOLUTION", "FPS", "QUALITY", "AUDIO"};
    gchar *days[8] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT", "HOL"};
    gchar strBuf[STRING_SIZE_64];
    gchar image_name[64];
    
    guint width[CONT_PARAM_TABLE_COL_CNT] = {0, };
    gint i;
    gint pos_x, pos_y;


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
        nfui_nfobject_set_size(obj, CONT_PARAM_TIME_LABEL_W, CONT_PARAM_TIME_LABEL_H);
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)content_fixed, obj, CONT_PARAM_TIME_LABEL_X + (CONT_PARAM_TIME_LABEL_COLS_SPACE+CONT_PARAM_TIME_LABEL_W)*i, CONT_PARAM_TIME_LABEL_Y);
    }


// TIME TILE
    nftile = (NFOBJECT*)nfui_tile_new(1, 24);
    nfui_tile_set_color(NF_TILE(nftile), NFTILE_STATE_NORMAL, &UX_COLOR(801)); 
    nfui_tile_set_color(NF_TILE(nftile), NFTILE_STATE_FOCUS, &UX_COLOR(802)); 
    nfui_tile_set_color(NF_TILE(nftile), NFTILE_STATE_SELECT, select_color); 
    nfui_tile_can_reSelect(NF_TILE(nftile), TRUE);
    nfui_nfobject_set_size(nftile, CONT_PARAM_TILE_W, CONT_PARAM_TILE_H);
    nfui_regi_post_event_callback(nftile, tile_event_handler);
    nfui_nfobject_show(nftile);
    nfui_nffixed_put((NFFIXED*)content_fixed, nftile, CONT_PARAM_TILE_X, CONT_PARAM_TILE_Y);


// RECORD TABLE
    width[0] = 292;
    width[1] = 300;
    width[2] = 218;
    width[3] = 298;
    width[4] = 298;

    pos_x = CONT_PARAM_TABLE_X;

    for(i=0; i<CONT_PARAM_TABLE_COL_CNT; i++)
    {
        if (i != 0)
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));      
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_set_size(obj, width[i], CONT_PARAM_TABLE_CELL_H);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, CONT_PARAM_TABLE_Y);
        }

        pos_x += width[i] + 2;
    }
    
    scrolled_fixed = (NFOBJECT*)nfui_nfscrolledfixed_new(NFSCROLLED_POLICY_NEVER, NFSCROLLED_POLICY_AUTOMATIC);
    nfui_nfscrolledfixed_set_skin_type((NFSCROLLEDFIXED*)scrolled_fixed, NFSCROLLEDFIXED_TYPE_1);
    nfui_nfscrolledfixed_set_vscroll_speed((NFSCROLLEDFIXED*)scrolled_fixed, (16 * 41)/3, (16 * 41)/10, (16 * 41)/20);
    nfui_nfscrolledfixed_set_vscroll_offset((NFSCROLLEDFIXED*)scrolled_fixed, 0);
    nfui_nfobject_modify_bg(scrolled_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(COLOR_PRG_IDX(UX_COLOR_8594A6)));
    nfui_nfobject_set_size(scrolled_fixed, width[0] + width[1] + width[2] + width[3] + width[4] + (2*5) + 60, 16 * 41);
    nfui_nfobject_show(scrolled_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, scrolled_fixed, CONT_PARAM_TABLE_X, CONT_PARAM_TABLE_Y + 41);

    ntb1 = (NFOBJECT*)nfui_nftable_new(CONT_PARAM_TABLE_COL_CNT, CONT_PARAM_TABLE_ROW_CNT, CONT_PARAM_TABLE_COLS_SPACE, CONT_PARAM_TABLE_ROWS_SPACE, width, CONT_PARAM_TABLE_CELL_H);
    nfui_nfobject_show(ntb1);
    nfui_nfscrolledfixed_put((NFSCROLLEDFIXED*)scrolled_fixed, ntb1, 0, 0);

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        var_get_camtitle(strBuf, i);
        
        cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        lbSize[i] = (NFOBJECT*)nfui_nflabel_new_text_box(continuous_info[i][PARAM_SIZE_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbSize[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
        lbFPS[i] = (NFOBJECT*)nfui_nflabel_new_text_box(continuous_info[i][PARAM_FPS_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbFPS[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
        lbQuality[i] = (NFOBJECT*)nfui_nflabel_new_text_box(continuous_info[i][PARAM_QUAL_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbQuality[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);
        lbAudio[i] = (NFOBJECT*)nfui_nflabel_new_text_box(continuous_info[i][PARAM_AUDIO_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbAudio[i], NFTEXTBOX_TYPE_SUBTAB_OUTPUT);

        nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));      

        nfui_nfobject_support_multi_lang(cam_ch[i], FALSE);
        
        nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_use_focus(lbSize[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_use_focus(lbFPS[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_use_focus(lbQuality[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_use_focus(lbAudio[i], NFOBJECT_FOCUS_OFF);

        nfui_nfobject_show(cam_ch[i]);
        nfui_nfobject_show(lbSize[i]);
        nfui_nfobject_show(lbFPS[i]);
        nfui_nfobject_show(lbQuality[i]);
        nfui_nfobject_show(lbAudio[i]);

        nfui_nftable_attach((NFTABLE*)ntb1, cam_ch[i], 0, i);
        nfui_nftable_attach((NFTABLE*)ntb1, lbSize[i], 1, i);
        nfui_nftable_attach((NFTABLE*)ntb1, lbFPS[i], 2, i);
        nfui_nftable_attach((NFTABLE*)ntb1, lbQuality[i], 3, i);
        nfui_nftable_attach((NFTABLE*)ntb1, lbAudio[i], 4, i);

    }


// DAY LABEL
    days_label = (NFOBJECT*)nfui_nflabel_new_with_pango_font("YOIL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(968));
    nfui_nfobject_use_focus(days_label, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_set_size(days_label, CONT_PARAM_DAY_LABEL_W, CONT_PARAM_DAY_LABEL_H);
    nfui_nflabel_set_align((NFLABEL*)days_label, NFALIGN_LEFT, 0);
    nfui_nfobject_modify_bg(days_label, NFOBJECT_STATE_NORMAL, COLOR_IDX(186));
    nfui_nfobject_hide(days_label);
    nfui_nffixed_put((NFFIXED*)content_fixed, days_label, CONT_PARAM_DAY_LABEL_X, CONT_PARAM_DAY_LABEL_Y);


// DAY DROP DOWN
    days_combo = nfui_combobox_new(days, 8, 0);
    nfui_combobox_set_skin_type(NF_COMBOBOX(days_combo), NFCOMBOBOX_TYPE_SUBTAB_1);     
    nfui_nfobject_set_size(days_combo, CONT_PARAM_DAY_DROPDOWN_W, CONT_PARAM_DAY_DROPDOWN_H);
    nfui_nffixed_put((NFFIXED*)content_fixed, days_combo, CONT_PARAM_DAY_DROPDOWN_X, CONT_PARAM_DAY_DROPDOWN_Y);
    nfui_nfobject_hide(days_combo);
    nfui_regi_post_event_callback(days_combo, days_combo_event_handler);
	
	if (!ivsc.dfunc.support_usrdef_holiday) {
		nfui_combobox_remove_data(NF_COMBOBOX(days_combo) , "HOL");
	}
// HELP BUTTON
    obj = nftool_normal_button_create_subtab_type1("HELP", 95);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, CONT_PARAM_HELP_BTN_X, CONT_PARAM_HELP_BTN_Y);
    nfui_regi_post_event_callback(obj, post_help_button_event_handler);

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

// PARENT EVENT HANDLER
    nfui_regi_pre_event_callback(parent, param_page_parent_event_handler);
}

void VW_RecContParam_data_reloaded(gboolean reload)
{
    if(isReloaded != reload)
        isReloaded = reload;
}

void VW_RecContParam_info_refresh()
{
    gint i;
    gchar *size, *fps, *qual, *audio;

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        size = nfui_nflabel_get_text((NFLABEL*)lbSize[i]);
        fps = nfui_nflabel_get_text((NFLABEL*)lbFPS[i]);

        if(strcmp(size, continuous_info[i][PARAM_SIZE_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbSize[i], continuous_info[i][PARAM_SIZE_COL]);
            nfui_signal_emit((NFLABEL*)lbSize[i], GDK_EXPOSE, TRUE);
        }

        _set_size_data_text_color(i);

        if(strcmp(fps, continuous_info[i][PARAM_FPS_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbFPS[i], continuous_info[i][PARAM_FPS_COL]);
            nfui_signal_emit((NFLABEL*)lbFPS[i], GDK_EXPOSE, TRUE);
        }

        _set_fps_data_text_color(i);
    }
}



