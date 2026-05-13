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
static OPT_AI_DTR_T *g_opt = NULL;

static NFOBJECT *g_aibox_dtr_rule_obj;
static NFOBJECT *g_aibox_dtr_forward_obj;
static NFOBJECT *g_aibox_dtr_reverse_obj;
static NFOBJECT *g_aibox_dtr_intrusion_obj;
static NFOBJECT *g_aibox_dtr_remove_obj;
static NFOBJECT *g_aibox_dtr_loiter_obj;
static NFOBJECT *g_aibox_dtr_stop_obj;
static NFOBJECT *g_aibox_dtr_object_obj;
static NFOBJECT *g_aibox_dtr_human_obj;
static NFOBJECT *g_aibox_dtr_car_obj;
static NFOBJECT *g_aibox_dtr_bike_obj;
static NFOBJECT *g_aibox_manual_check_obj;
static NFOBJECT *g_aibox_manual_label_obj;


static gboolean _set_evt_all_chkbtn()
{
    gboolean ret = TRUE;

    if (!g_opt->evt.forward) ret = FALSE;
    if (!g_opt->evt.reverse) ret = FALSE;
    if (!g_opt->evt.intrusion) ret = FALSE;
    if (!g_opt->evt.removed) ret = FALSE;
    if (!g_opt->evt.loitering) ret = FALSE;
    if (!g_opt->evt.stopped) ret = FALSE;

    return ret;
}

static gboolean _set_obj_all_chkbtn()
{
    gboolean ret = TRUE;

    if (!g_opt->obj.human) ret = FALSE;
    if (!g_opt->obj.car) ret = FALSE;
    if (!g_opt->obj.bike) ret = FALSE;
    if (!g_opt->obj.custom) ret = FALSE;

    return ret;
}

static gboolean post_aibox_object_filter_edit_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == GDK_BUTTON_RELEASE)
    {   
        gchar filter_string[256]; 
        gchar exception_string[64]; 

        memset(filter_string, 0x00, sizeof(filter_string));
        snprintf(filter_string, sizeof(filter_string), "%s", nfui_nflabel_get_text((NFLABEL*)g_aibox_manual_label_obj));

        memset(exception_string, 0x00, sizeof(exception_string));
        snprintf(exception_string, sizeof(exception_string), "%s", "person,car,bike,");

        vw_dvabx_object_filter_edit_popup_open(g_curwnd, 1300, 310, exception_string, filter_string, sizeof(filter_string)-sizeof(exception_string));

        nfui_nflabel_set_text((NFLABEL*)g_aibox_manual_label_obj, filter_string);
        nfui_signal_emit(g_aibox_manual_label_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_aibox_object_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;

        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_human_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_car_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_bike_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_manual_check_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_object_obj), state);
        nfui_signal_emit(g_aibox_dtr_object_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_aibox_object_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_human_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_car_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_bike_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_manual_check_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_human_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_car_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_bike_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_manual_check_obj), FALSE);
        }
        nfui_signal_emit(g_aibox_dtr_human_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_car_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_bike_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_manual_check_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_aibox_rule_group_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state = TRUE;

        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_forward_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_remove_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj))) state = FALSE;
        if (!nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_stop_obj))) state = FALSE;

        nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_rule_obj), state);
        nfui_signal_emit(g_aibox_dtr_rule_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_aibox_rule_all_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
    if (evt->type == NFEVENT_CHECKBUTTON_CHANGED)
    {   
        gboolean state;
        state = nfui_check_button_get_active((NFCHECKBUTTON*)obj);
        
        if (state) {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_forward_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_remove_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj), TRUE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_stop_obj), TRUE);
        }
        else {
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_forward_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_remove_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj), FALSE);
            nfui_check_button_set_active_no_expose(NF_CHECKBUTTON(g_aibox_dtr_stop_obj), FALSE);
        }
        nfui_signal_emit(g_aibox_dtr_forward_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_reverse_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_intrusion_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_remove_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_loiter_obj, GDK_EXPOSE, TRUE);
        nfui_signal_emit(g_aibox_dtr_stop_obj, GDK_EXPOSE, TRUE);
    }

    return FALSE;
}

static gboolean post_label_keyword_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    guint x, y;
    gchar *strTemp = NULL;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) {
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}
	
    if (evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
    {
        nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;

        strTemp = VirtualKey_Open(g_curwnd, nfui_nflabel_get_text((NFLABEL*)obj), x, y, 64, VKEY_NORMAL);

        if (strTemp)
        {
            nfui_nflabel_set_text((NFLABEL*)obj, strTemp);
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

            ifree(strTemp);
            strTemp = NULL;
        }
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

        g_opt->evt.forward = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_forward_obj));
        g_opt->evt.reverse = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_reverse_obj));
        g_opt->evt.intrusion = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_intrusion_obj));
        g_opt->evt.removed = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_remove_obj));
        g_opt->evt.loitering = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_loiter_obj));
        g_opt->evt.stopped = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_stop_obj));

        g_opt->obj.human = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_human_obj));
        g_opt->obj.car = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_car_obj));
        g_opt->obj.bike = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_dtr_bike_obj));
        g_opt->obj.custom = nfui_check_button_get_active(NF_CHECKBUTTON(g_aibox_manual_check_obj));
        strcpy(g_opt->obj.strcustom, nfui_nflabel_get_text((NFLABEL*)g_aibox_manual_label_obj));
        
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

gint VW_Search_By_Ai_Detector_Filter_Popup(NFWINDOW *parent, OPT_AI_DTR_T *opt)
{
    NFWINDOW *main_wnd;
    NFOBJECT *main_fixed;
    NFOBJECT *obj;

    gint pos_x, pos_y;
    gint btn_w, btn_h;
    gint i;

    g_ret = 0;
    g_opt = opt;

    main_wnd = nftool_create_popup_window((NFWINDOW*)parent, (DISPLAY_ACTIVE_WIDTH-SAF_POPUP_SIZE_W)/2, (DISPLAY_ACTIVE_HEIGHT-SAF_POPUP_SIZE_H)/2, 
                                         SAF_POPUP_SIZE_W, SAF_POPUP_SIZE_H, "AI SEARCH FILTER", FALSE);
    nfui_nfobject_show(main_wnd);
    nfui_regi_post_event_callback(main_wnd, post_window_event_handler);
    g_curwnd = main_wnd;

    main_fixed = ((NFWINDOW*)main_wnd)->child;

    pos_x = SAF_OBJ_START_POS_X;
    pos_y = SAF_OBJ_START_POS_Y;

    obj = nfui_checkbutton_new(_set_evt_all_chkbtn());
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_all_event_handler);
    g_aibox_dtr_rule_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("EVENT", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    pos_x += 40;
    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->evt.forward);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_group_event_handler);    
    g_aibox_dtr_forward_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FORWARD", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	

    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->evt.reverse);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_group_event_handler);    
    g_aibox_dtr_reverse_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REVERSE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	

    pos_x += 240;
    pos_y = SAF_OBJ_START_POS_Y + 41; 

    obj = nfui_checkbutton_new(opt->evt.intrusion);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_group_event_handler);    
    g_aibox_dtr_intrusion_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INTRUSION", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	    

    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->evt.removed);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_group_event_handler);    
    g_aibox_dtr_remove_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("REMOVED", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  

    pos_x += 240;
    pos_y = SAF_OBJ_START_POS_Y + 41; 

    obj = nfui_checkbutton_new(opt->evt.loitering);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_group_event_handler);    
    g_aibox_dtr_loiter_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("LOITERING", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	

    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->evt.stopped);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_rule_group_event_handler);    
    g_aibox_dtr_stop_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STOPPED", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  

    pos_x = SAF_OBJ_START_POS_X;
    pos_y += 47;

    obj = nfui_checkbutton_new(_set_obj_all_chkbtn());
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_object_all_event_handler);
    g_aibox_dtr_object_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("OBJECT", nffont_get_pango_font(NFFONT_MEDIUM_NORMAL), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 280, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);   

    pos_x += 40;
    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->obj.human);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_object_group_event_handler);    
    g_aibox_dtr_human_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HUMAN", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	

    obj = nfui_checkbutton_new(opt->obj.car);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+240, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_object_group_event_handler);    
    g_aibox_dtr_car_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("CAR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+280, pos_y);
    nfui_nfobject_show(obj);  	

    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->obj.bike);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_object_group_event_handler);    
    g_aibox_dtr_bike_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("BIKE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, 200, SAF_OBJ_SIZE_H);
    nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(291));
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);

    pos_y += 41; 

    obj = nfui_checkbutton_new(opt->obj.custom);
    nfui_check_button_set_skin_type(NF_CHECKBUTTON(obj), NFCHECK_TYPE_NORMAL);
    nfui_check_get_size(obj, &btn_w, &btn_h);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x, pos_y+(SAF_OBJ_SIZE_H-btn_h)/2);
    nfui_regi_post_event_callback(obj, post_aibox_object_group_event_handler);    
    g_aibox_manual_check_obj = obj;

    obj = (NFOBJECT*)nfui_nflabel_new_text_box(opt->obj.strcustom, nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
    nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_OUTPUT);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 4);
    nfui_nfobject_set_size(obj, 360, SAF_OBJ_SIZE_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+40, pos_y);
    nfui_nfobject_show(obj);  	
    g_aibox_manual_label_obj = obj;

    obj = nftool_normal_button_create_popup_type1("EDIT", 120);
    nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, pos_x+402, pos_y);
    nfui_regi_post_event_callback(obj, post_aibox_object_filter_edit_event_handler);

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
