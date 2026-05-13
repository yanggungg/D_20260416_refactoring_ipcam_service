#include "iux_afx.h"
#include "../support/event_loop.h"
#include "nf_ui_tool.h"
#include "../support/nf_ui_page_manager.h"

#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nfobject.h"
#include "objects/nflabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfcheckbutton.h"
#include "objects/nfbutton.h"

#include "vw_vkeyboard.h"
#include "vw_dvabx_object_filter_edit_popup.h"
#include "vw_search_by_deeplearning_filter_popup.h"


#define SAF_OBJ_START_POS_X         (15)
#define SAF_OBJ_START_POS_Y         (55)
#define SAF_OBJ_SIZE_H              (40)
#define SAF_OBJ_GAP                 (1)

#define SAF_BUTTON_SIZE_W           (150)

#define SAF_POPUP_SIZE_H            (442)
#define SAF_POPUP_SIZE_W            (790)

static NFWINDOW *g_curwnd;
static gint g_ret;
//static OPT_AI_DTR_T *g_opt = NULL;
static OPT_BUILTIN_IDZ_T *g_opt = NULL;

static NFOBJECT *g_builtin_idz_car_obj;
static NFOBJECT *g_builtin_idz_bike_obj;
static NFOBJECT *g_builtin_idz_vehicle_obj;
static NFOBJECT *g_builtin_idz_human_obj;

static gboolean _set_vehicle_all_chkbtn()
{
    gboolean ret = TRUE;

    if (!g_opt->car) ret = FALSE;
    if (!g_opt->bike) ret = FALSE;

    return ret;
}

static gboolean post_builtin_object_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;

        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_car_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_bike_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_vehicle_obj), state);
        nfui_signal_emit(g_builtin_idz_vehicle_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_builtin_object_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_car_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_bike_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_car_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_builtin_idz_bike_obj), FALSE);
        }
        nfui_signal_emit(g_builtin_idz_car_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_builtin_idz_bike_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_btn_cancel_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_ret = 0;
        
        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_btn_ok_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    NFOBJECT *top;
    gint i;
    
    if (evt->type == GDK_BUTTON_RELEASE)
    {
        if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

        g_opt->human = nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_human_obj));
        g_opt->car = nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_car_obj));
        g_opt->bike = nfui_check_button_get_active(NF_CHECKBUTTON(g_builtin_idz_bike_obj));

        g_ret = 1;

        top = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(top);
    }

    return FALSE;
}

static gboolean post_window_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_DELETE)
    {
        g_curwnd = 0;

        gtk_main_quit();
    }
    
    return FALSE;
}

gint VW_Search_By_Builtin_Idz_Filter_Popup(NFWINDOW *parent, OPT_BUILTIN_IDZ_T *opt)
{
    NFWINDOW *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint btn_w, btn_h;
    gint i;

    g_ret = 0;
    //g_opt = opt;
    g_opt = opt;

    main_wnd = nftool_create_popup_window((NFWINDOW*)parent, (DISPLAY_ACTIVE_WIDTH-SAF_POPUP_SIZE_W)/2, (DISPLAY_ACTIVE_HEIGHT-SAF_POPUP_SIZE_H)/2, 
                                         SAF_POPUP_SIZE_W, SAF_POPUP_SIZE_H, "AI SEARCH FILTER", FALSE);
    nfui_nfobject_show(main_wnd);
    nfui_regi_post_event_callback(main_wnd, post_window_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = SAF_OBJ_START_POS_X;
    pos_y = SAF_OBJ_START_POS_Y;

    obj = nfui_checkbutton_new(g_opt->human);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    g_builtin_idz_human_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HUMAN DETECTION", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    pos_x = SAF_OBJ_START_POS_X;
    pos_y += 47;

    obj = nfui_checkbutton_new(_set_vehicle_all_chkbtn());
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_builtin_object_all_event_handler);
    g_builtin_idz_vehicle_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VEHICLE DETECTION", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    pos_x += 40;
    pos_y += 41;

    obj = nfui_checkbutton_new(opt->car);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_builtin_object_group_event_handler);    
    g_builtin_idz_car_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	

    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->bike);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_builtin_object_group_event_handler);    
    g_builtin_idz_bike_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);

    pos_x = (SAF_POPUP_SIZE_W / 2) - SAF_BUTTON_SIZE_W - 1;
    pos_y = SAF_POPUP_SIZE_H - 55;
    
    obj = nftool_normal_button_create_type1("OK", SAF_BUTTON_SIZE_W);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_btn_ok_event_handler);

    pos_x = (SAF_POPUP_SIZE_W / 2) + 1;
    
    obj = nftool_normal_button_create_type1("CANCEL", SAF_BUTTON_SIZE_W);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED *)main_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_btn_cancel_event_handler);

    nfui_make_key_hierarchy((NFWINDOW*)g_curwnd);

    nfui_page_open(PGID_AI_SEARCH_FILTER_EDIT_POPUP, g_curwnd, ssm_get_cur_id(NULL));

    gtk_main();

    nfui_page_close(PGID_AI_SEARCH_FILTER_EDIT_POPUP, g_curwnd);

    return g_ret;
}
