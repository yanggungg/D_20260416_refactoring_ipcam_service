
#include "nf_afx.h"

#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/util.h"
#include "support/event_loop.h"

#include "tools/nf_ui_tool.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfcombobox.h"
#include "objects/nfimglabel.h"

#include "../support/nf_ui_page_manager.h"

#include "vw_evt_ptz_preset_popup.h"
#include "ix_mem.h"

#define PAGE_FIXED_CNT          2
#define ROW_CNT_PER_PAGE        (GUI_CHANNEL_CNT / PAGE_FIXED_CNT)

#define PP_POS_X(win_w)             ((DISPLAY_ACTIVE_WIDTH-win_w)/2)
#define PP_POS_Y(win_h)             ((DISPLAY_ACTIVE_HEIGHT-win_h)/2)

#define PP_TBL_ROWS                 (1)
#define PP_TBL_COLS                 (2)

#define PP_LABEL_HEI                (40)

#define PP_OK_BTN_W                 (192)
#define PP_OK_BTN_X(win_w)          (win_w/2-PP_OK_BTN_W-5)

#define PP_CANCEL_BTN_W             (192)
#define PP_CANCEL_BTN_X(win_w)      (win_w/2+5)

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_presetObj[GUI_CHANNEL_CNT];
static NFOBJECT *g_page_fixed[PAGE_FIXED_CNT];
static NFOBJECT *g_lb_page_num;

static PRST_INFO_T g_prst_info;
static PRST_INFO_T *g_org_prst_info;
static PtzPresetData g_prst_data[GUI_CHANNEL_CNT];

static gint _init_preset_data(PRST_INFO_T *info, guint ch_mask)
{
    gchar strBuf[64];
    gint i, j;

    memset(g_prst_data, 0x00, sizeof(PtzPresetData)*GUI_CHANNEL_CNT);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_ptz_preset_data(&g_prst_data[i], i);

        nfui_combobox_append_data(NF_COMBOBOX(g_presetObj[i]), "NONE");

        if (info->number == 0)
            nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_presetObj[i]), 0);

        for (j = 0; j < g_prst_data[i].cnt; j++)
        {
            memset(strBuf, 0x00, sizeof(strBuf));
            g_sprintf(strBuf, "%d - %s", g_prst_data[i].number[j], g_prst_data[i].name[j]);
            nfui_combobox_append_data(NF_COMBOBOX(g_presetObj[i]), strBuf);


            if (info->number[i] == g_prst_data[i].number[j])
                nfui_combobox_set_index_no_expose(NF_COMBOBOX(g_presetObj[i]), j+1);
        }

        if (ch_mask & (1 << i)) nfui_nfobject_enable(g_presetObj[i]);
        else                    nfui_nfobject_disable(g_presetObj[i]);
    }

    return 0;
}

static guint _get_preset_num(gint ch, gint idx)
{
   guint number;

    if (idx == 0)
        number = 0;
    else
        number = g_prst_data[ch].number[idx-1];    

    return number;
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

static gboolean post_prst_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    gint i, index;

    if(evt->type == NFEVENT_COMBOBOX_CHANGED) 
    {
        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (g_presetObj[i] == obj)
                break;
        }

        index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));          
        g_prst_info.number[i] = _get_preset_num(i, index);
    }

    return FALSE;
}

static gboolean _post_ok_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gint i;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        g_memmove(g_org_prst_info, &g_prst_info, sizeof(PRST_INFO_T));
    
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean _post_cancel_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;

    if(evt->type == GDK_BUTTON_RELEASE) {
        if(evt->button.button == MOUSE_RIGTH_BUTTON) {                  
            return FALSE;
        }

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean _post_main_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    GdkEventKey *kevt;
    KEYPAD_KID kpid = KEYPAD_NONE;

    switch(event->type)
    {
        case GDK_DELETE:
            g_curwnd = 0;
            gtk_main_quit();
        break;

        default:
        break;
    }

    return FALSE;
}

gboolean vw_evt_ptz_preset_popup_open(NFWINDOW *parent, PRST_INFO_T *info, guint ch_mask)
{
    NFOBJECT *win;
    NFOBJECT *fixed;
    NFOBJECT *ntb;
    NFOBJECT *obj;
    NFOBJECT *ok_btn;
    NFOBJECT *main_page_fixed;
    NFOBJECT *page_fixed[PAGE_FIXED_CNT];
    NFOBJECT *page_ntb[PAGE_FIXED_CNT];

    gint win_width, win_height;
    gint pos_x, pos_y;
    gint i, j;
    gint cnt;
    guint ntb_width[PP_TBL_COLS] = {0, };

    GdkPixbuf *pbCamImage[32];
    gchar strBuf[STRING_SIZE_CAMTITLE];
	gint size_w, size_h;
	gint page_num, row_num;
	GdkPixbuf *prev_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *next_img[NFOBJECT_STATE_COUNT];
   
    pbCamImage[0]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_01, NULL); 
    pbCamImage[1]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_02, NULL); 
    pbCamImage[2]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_03, NULL); 
    pbCamImage[3]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_04, NULL); 
    pbCamImage[4]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_05, NULL);         
    pbCamImage[5]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_06, NULL); 
    pbCamImage[6]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_07, NULL); 
    pbCamImage[7]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_08, NULL); 
    pbCamImage[8]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_09, NULL); 
    pbCamImage[9]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_10, NULL);     
    pbCamImage[10] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_11, NULL); 
    pbCamImage[11] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_12, NULL); 
    pbCamImage[12] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_13, NULL); 
    pbCamImage[13] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_14, NULL); 
    pbCamImage[14] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_15, NULL);         
    pbCamImage[15] = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_16, NULL);
    pbCamImage[16]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_17, NULL); 
    pbCamImage[17]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_18, NULL); 
    pbCamImage[18]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_19, NULL); 
    pbCamImage[19]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_20, NULL);     
    pbCamImage[20]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_21, NULL); 
    pbCamImage[21]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_22, NULL); 
    pbCamImage[22]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_23, NULL); 
    pbCamImage[23]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_24, NULL); 
    pbCamImage[24]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_25, NULL);         
    pbCamImage[25]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_26, NULL); 
    pbCamImage[26]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_27, NULL); 
    pbCamImage[27]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_28, NULL); 
    pbCamImage[28]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_29, NULL); 
    pbCamImage[29]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_30, NULL);     
    pbCamImage[30]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_31, NULL); 
    pbCamImage[31]  = nfui_get_image_from_file(IMG_CAMERA_OUTPUT_ICON_32, NULL); 

	prev_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_N_BTN), NULL);
	prev_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_O_BTN), NULL);
	prev_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_P_BTN), NULL);
	prev_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_PREV_D_BTN), NULL);

	next_img[0] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_N_BTN), NULL);
	next_img[1] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_O_BTN), NULL);
	next_img[2] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_P_BTN), NULL);
	next_img[3] = nfui_get_image_from_file((IMG_SUBMENU_TL_NEXT_D_BTN), NULL);

    ntb_width[0] = 230;
    ntb_width[1] = 350;
    
    win_width = 20 + ntb_width[0] + 2 + ntb_width[1] + 20;
    win_height = 52 + 16*(PP_LABEL_HEI+2) + 180;

    g_memmove(&g_prst_info, info, sizeof(PRST_INFO_T));
    g_org_prst_info = info;

// <---- WINDOW & FIXED
    win = nftool_create_popup_window(parent, PP_POS_X(win_width), PP_POS_Y(win_height), win_width, win_height, "PRESET EDIT", TRUE);    
    nfui_nfwindow_use_double_buffer((NFWINDOW*)win);
    nfui_regi_post_event_callback(win, _post_main_event_handler);
    g_curwnd = win;

    fixed = ((NFWINDOW*)g_curwnd)->child;

    ntb = (NFOBJECT*)nfui_nftable_new(PP_TBL_COLS, PP_TBL_ROWS, 2, 2, ntb_width, PP_LABEL_HEI);
    nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)fixed, ntb, 20, 52);

    obj = nfui_nflabel_new_with_pango_font("CHANNEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));       
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 0, 0);

    obj = nfui_nflabel_new_with_pango_font("PRESET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));        
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);
    
    size_w = 0;
    for (i = 0; i < PP_TBL_COLS; i++) {
        size_w += ntb_width[i];
    }

    main_page_fixed = (NFOBJECT*)nfui_nffixed_new();
    nfui_nfobject_modify_bg(main_page_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
    nfui_nfobject_set_size(main_page_fixed, size_w, ROW_CNT_PER_PAGE * (40 + 2) + 60);
    nfui_nfobject_show(main_page_fixed);
    nfui_nffixed_put((NFFIXED*)fixed, main_page_fixed, 20, 52+42);

    for (i = 0; i < PAGE_FIXED_CNT; i++)
    {
        g_page_fixed[i] = (NFOBJECT*)nfui_nffixed_new();
        nfui_nfobject_modify_bg(g_page_fixed[i], NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
        nfui_nfobject_set_size(g_page_fixed[i], size_w, ROW_CNT_PER_PAGE * (40 + 2));
        nfui_nffixed_put((NFFIXED*)main_page_fixed, g_page_fixed[i], 0, 0);

        page_ntb[i] = (NFOBJECT*)nfui_nftable_new(PP_TBL_COLS, ROW_CNT_PER_PAGE, 2, 2, ntb_width, 40);
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
	
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
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

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        //DAL_get_camera_title(strBuf, i);
        var_get_camtitle(strBuf, i);

        obj = (NFOBJECT*)nfui_nfimglabel_new(pbCamImage[i], strBuf);
        nfui_nfimglabel_set_align((NFIMGLABEL*)obj, NFALIGN_LEFT);
        nfui_nfimglabel_set_pango_font((NFIMGLABEL*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(116));
        nfui_nfobject_support_multi_lang(obj, FALSE);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(115));
        nfui_nfobject_show(obj);
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 0, row_num);

        obj = nfui_combobox_new(NULL, 0, 0);
        nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
        nfui_nfobject_show(obj); 
        nfui_nftable_attach((NFTABLE*)page_ntb[page_num], obj, 1, row_num);
        nfui_regi_post_event_callback(obj, post_prst_event_handler);        
        g_presetObj[i] = obj;

        row_num++;

        if (row_num == ROW_CNT_PER_PAGE) {
            row_num = 0;
            page_num++;
        }
    }

    pos_y = (win_height - 58);

    obj = nftool_normal_button_create_type1("OK", PP_OK_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, PP_OK_BTN_X(win_width), pos_y);
    nfui_regi_post_event_callback(obj, _post_ok_button_event_handler);
    ok_btn = obj;

    obj = nftool_normal_button_create_type1("CANCEL", PP_CANCEL_BTN_W);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed, obj, PP_CANCEL_BTN_X(win_width), pos_y);
    nfui_regi_post_event_callback(obj, _post_cancel_button_event_handler);

    _init_preset_data(info, ch_mask);

//    nfui_nfwindow_add((NFWINDOW*)win, fixed);
    nfui_run_main_event_handler(win);
    nfui_nfobject_show(win);
    nfui_make_key_hierarchy((NFWINDOW*)win);
    nfui_set_key_focus(ok_btn, TRUE);

    nfui_page_open(PGID_EVT_PRESET_EDIT, win, nfui_get_last_user());

    gtk_main();

    nfui_page_close(PGID_EVT_PRESET_EDIT, win);

    return FALSE;
}


