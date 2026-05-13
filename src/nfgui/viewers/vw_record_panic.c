#include <string.h>

#include "nf_afx.h"

#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/event_loop.h"
#include "support/color.h"

#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfimglabel.h"
#include "objects/nfimage.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/nfcombobox.h"


#include "tools/nf_ui_tool.h"

#include "vw_record_main.h"
#include "vw_record_data_internal.h"
#include "vw_record_panic.h"
#include "vw_record_param_sub.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define REC_PANIC_TABLE_COL_CNT             (5)
#define REC_PANIC_TABLE_ROW_CNT             (GUI_CHANNEL_CNT)

#define REC_PANIC_TABLE_X                   (4)
#define REC_PANIC_TABLE_Y                   (68)
#define REC_PANIC_TABLE_CELL_H              (39)

#define REC_PANIC_TABLE_COLS_SPACE          (2)
#define REC_PANIC_TABLE_ROWS_SPACE          (2)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *lbSize[GUI_CHANNEL_CNT];
static NFOBJECT *lbFPS[GUI_CHANNEL_CNT];
static NFOBJECT *lbQuality[GUI_CHANNEL_CNT];
static NFOBJECT *lbAudio[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static gboolean isReloaded = FALSE;


static void _set_size_data_text_color(gint ch)
{
    gboolean val;
    guint fg_color;

    val = get_panic_size_data_modify(ch);

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
        if (fg_color != 139)
        {
            nfui_nflabel_modify_fg(lbSize[ch], COLOR_IDX(139));
            nfui_signal_emit(lbSize[ch], GDK_EXPOSE, TRUE);
        }
    }
}

static void _set_fps_data_text_color(gint ch)
{
    gboolean val;
    guint fg_color;

    val = get_panic_fps_data_modify(ch);

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
        if (fg_color != 139)
        {
            nfui_nflabel_modify_fg(lbFPS[ch], COLOR_IDX(139));
            nfui_signal_emit(lbFPS[ch], GDK_EXPOSE, TRUE);
        }
    }
}

void display_panic_rec_info(gboolean redraw) 
{
    gint i;
    gchar *size, *fps, *qual, *audio;

    for(i=0; i<GUI_CHANNEL_CNT; i++) {

        size = nfui_nflabel_get_text((NFLABEL*)lbSize[i]);
        fps = nfui_nflabel_get_text((NFLABEL*)lbFPS[i]);
        qual = nfui_nflabel_get_text((NFLABEL*)lbQuality[i]);
        audio = nfui_nflabel_get_text((NFLABEL*)lbAudio[i]);

        if(strcmp(size, panic_info[i][PARAM_SIZE_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbSize[i], panic_info[i][PARAM_SIZE_COL]);
            nfui_nflabel_modify_fg((NFLABEL*)lbSize[i], COLOR_IDX(139));

            if(redraw) 
                nfui_signal_emit(lbSize[i], GDK_EXPOSE, TRUE);
        }

        if(strcmp(fps, panic_info[i][PARAM_FPS_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbFPS[i], panic_info[i][PARAM_FPS_COL]);
            nfui_nflabel_modify_fg((NFLABEL*)lbFPS[i], COLOR_IDX(139));

            if(redraw) 
                nfui_signal_emit(lbFPS[i], GDK_EXPOSE, TRUE);
        }

        if(strcmp(qual, panic_info[i][PARAM_QUAL_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbQuality[i], panic_info[i][PARAM_QUAL_COL]);

            if(redraw) 
                nfui_signal_emit(lbQuality[i], GDK_EXPOSE, TRUE);
        }

        if(strcmp(audio, panic_info[i][PARAM_AUDIO_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbAudio[i], panic_info[i][PARAM_AUDIO_COL]);

            if(redraw) 
                nfui_signal_emit(lbAudio[i], GDK_EXPOSE, TRUE);
        }
    }
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

static gboolean post_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    guint i;
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
    {
        kevt = (GdkEventKey*)evt;
        kpid = (KEYPAD_KID)kevt->keyval;
    }

    if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

            if(VW_RecParam_Submenu_Open(g_curwnd, panic_info, REC_PARAM_NORMAL_OPEN)) {
                set_record_panic_info();
                display_panic_rec_info(TRUE);
            }
    }
    return FALSE;
}

static gboolean post_setup_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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
        if(evt->button.button == MOUSE_RIGTH_BUTTON)
            return FALSE;

            if(VW_RecParam_Submenu_Open(g_curwnd, panic_info, REC_PARAM_NORMAL_OPEN)) {
                set_record_panic_info();
                display_panic_rec_info(TRUE);
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
    
        if(is_changed_panic_data()) {
            set_record_panic_db();
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
    
        get_record_panic_db();
        display_panic_rec_info(TRUE);
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

        VW_RecPanic_tab_out_handler();
        VW_RecordSetup_Destroy(obj);
    }

    return FALSE;
}

static gboolean post_page_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if(evt->type == GDK_DELETE)
    {
        g_curwnd = 0;
    }

    return FALSE;
}

void VW_Init_RecPanic_Page(NFOBJECT *parent)
{
    NFOBJECT *content_fixed;
    NFOBJECT *obj;
    NFOBJECT *cam_ch[GUI_CHANNEL_CNT];
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    GdkPixbuf *pbCamImage[32];

    gchar *strTitle[REC_PANIC_TABLE_COL_CNT] = {"", "RESOLUTION", "FPS", "QUALITY", "AUDIO"};
    gchar strBuf[STRING_SIZE_CAMTITLE];
    gchar image_name[64];
    
    guint width[REC_PANIC_TABLE_COL_CNT] = {0, };
    gint pos_x = 0, pos_y = 0, size_h, size_w;
    gint i;
    
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
    nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
    nfui_nfobject_show(content_fixed);
    nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);


// RECORD TABLE
    width[0] = 292;
    width[1] = 300;
    width[2] = 218;
    width[3] = 298;
    width[4] = 298;

    pos_x = REC_PANIC_TABLE_X;

    for(i=0; i<REC_PANIC_TABLE_COL_CNT; i++)
    {
        if (i != 0)
        {
            obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));      
            nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
            nfui_nfobject_set_size(obj, width[i], REC_PANIC_TABLE_CELL_H);
            nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
            nfui_nfobject_show(obj);
            nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, REC_PANIC_TABLE_Y);
        }

        pos_x += width[i] + 2;
    }
   
    size_w = 0;
    for (i = 0; i < REC_PANIC_TABLE_COL_CNT; i++) {
        size_w += width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 1) + 80);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)content_fixed, main_page_fixed, REC_PANIC_TABLE_X, REC_PANIC_TABLE_Y + 41);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 1));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(REC_PANIC_TABLE_COL_CNT, ROW_CNT_PER_PAGE, 2, 1, width, 40);
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

    page_num = row_num = 0;

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        var_get_camtitle(strBuf, i);
        
        cam_ch[i] = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfobject_modify_bg(cam_ch[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        lbSize[i] = (NFOBJECT*)nfui_nflabel_new_text_box(panic_info[i][PARAM_SIZE_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbSize[i], NFTEXTBOX_TYPE_OUTPUT);
        lbFPS[i] = (NFOBJECT*)nfui_nflabel_new_text_box(panic_info[i][PARAM_FPS_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbFPS[i], NFTEXTBOX_TYPE_OUTPUT);
        lbQuality[i] = (NFOBJECT*)nfui_nflabel_new_text_box(panic_info[i][PARAM_QUAL_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbQuality[i], NFTEXTBOX_TYPE_OUTPUT);
        lbAudio[i] = (NFOBJECT*)nfui_nflabel_new_text_box(panic_info[i][PARAM_AUDIO_COL], nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)lbAudio[i], NFTEXTBOX_TYPE_OUTPUT);

        nfui_nfobject_use_focus(cam_ch[i], NFOBJECT_FOCUS_OFF);
        nfui_nfimglabel_set_align((NFIMGLABEL*)cam_ch[i], NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)cam_ch[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));      

        nfui_nfobject_support_multi_lang(cam_ch[i], FALSE);

        nfui_regi_post_event_callback(lbSize[i], post_label_event_handler);
        nfui_regi_post_event_callback(lbFPS[i], post_label_event_handler);
        nfui_regi_post_event_callback(lbQuality[i], post_label_event_handler);
        nfui_regi_post_event_callback(lbAudio[i], post_label_event_handler);

        nfui_nfobject_show(cam_ch[i]);
        nfui_nfobject_show(lbSize[i]);
        nfui_nfobject_show(lbFPS[i]);
        nfui_nfobject_show(lbQuality[i]);
        nfui_nfobject_show(lbAudio[i]);

        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], cam_ch[i], 0, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], lbSize[i], 1, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], lbFPS[i], 2, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], lbQuality[i], 3, row_num);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], lbAudio[i], 4, row_num);
        
        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

// SETUP BTN
    pos_x = REC_PANIC_TABLE_X;
    pos_x += width[0] + REC_PANIC_TABLE_COLS_SPACE;
    pos_x += width[1] + REC_PANIC_TABLE_COLS_SPACE;
    pos_x += width[2] + REC_PANIC_TABLE_COLS_SPACE;
    pos_x += width[3] + REC_PANIC_TABLE_COLS_SPACE;
    pos_x += width[4] + REC_PANIC_TABLE_COLS_SPACE;
    pos_x -= MENU_BTN_WIDTH;

    pos_y = REC_PANIC_TABLE_Y + 41 + main_page_fixed->height + 2;
        
    obj = nftool_normal_button_create_subtab_type1("SETUP", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)content_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_setup_button_event_handler);


// CANCEL, APPLY, CLOSE BUTTON
    obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

    obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

    obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);    
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
    nfui_regi_post_event_callback(obj, post_closebutton_event_handler);
    

    nfui_regi_post_event_callback(parent, post_page_event_handler);

}


gboolean VW_RecPanic_tab_out_handler()
{
    mb_type ret;

    if((is_changed_panic_data())) {
        ret = nftool_mbox(g_curwnd, "CONFIRM", "Configuration has been changed.\nDo you want to save?", NFTOOL_MB_OKCANCEL);
        
        if(ret == NFTOOL_MB_OK)
        {
            set_record_panic_db();
            vw_send_notify_change_record_data();            
        }
        else
        {
            get_record_panic_db();
            display_panic_rec_info(FALSE);
        }
    }
    return FALSE;
}

void VW_RecPanic_info_refresh()
{
    gint i;
    gchar *size, *fps, *qual, *audio;

    for(i=0; i<GUI_CHANNEL_CNT; i++)
    {
        size = nfui_nflabel_get_text((NFLABEL*)lbSize[i]);
        fps = nfui_nflabel_get_text((NFLABEL*)lbFPS[i]);

        if(strcmp(size, panic_info[i][PARAM_SIZE_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbSize[i], panic_info[i][PARAM_SIZE_COL]);
            nfui_signal_emit((NFLABEL*)lbSize[i], GDK_EXPOSE, TRUE);
        }

        _set_size_data_text_color(i);

        if(strcmp(fps, panic_info[i][PARAM_FPS_COL]) != 0) {
            nfui_nflabel_set_text((NFLABEL*)lbFPS[i], panic_info[i][PARAM_FPS_COL]);
            nfui_signal_emit((NFLABEL*)lbFPS[i], GDK_EXPOSE, TRUE);
        }

        _set_fps_data_text_color(i);
    }
}

