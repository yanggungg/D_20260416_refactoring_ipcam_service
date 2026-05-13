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



// SETUP WINDOW
#define SETUP_WIN_SIZE_W        410
#define SETUP_WIN_SIZE_H        520
#define TARGET_LABEL_SIZE_W     230
#define TARGET_LABEL_SIZE_H     40
#define TARGET_VALUE_SIZE_W     150
#define TARGET_VALUE_SIZE_H     TARGET_LABEL_SIZE_H

enum {
    RESULT_CAMERA_HEIGHT = 0,
    RESULT_TILT_ANGLE,
    RESULT_FOCAL_LENGTH,
    RESULT_NUMBER_OF_TARGET,

    RESULT_VALUES
};

static NFWINDOW *g_vcaSetupWnd = 0;
static NFOBJECT *g_wnd_par;

static NFOBJECT *g_tar_height;
static NFOBJECT *g_lb_result[RESULT_VALUES];
static NFOBJECT *g_estimate_btn;
static NFOBJECT *g_live_btn;

extern int g_live_freeze = 0;
extern int g_live_ch;

static gboolean post_addbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        gint targets;
        gchar strBuf[16];

        targets = zoom_add_target();

        g_sprintf(strBuf, "%d",targets);

        nfui_nflabel_set_text((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], strBuf);
        nfui_signal_emit((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], GDK_EXPOSE,TRUE);
	}

	return FALSE;
}

static gboolean post_delbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        gint targets;
        char strBuf[16];

        targets = zoom_delete_target();

        g_sprintf(strBuf, "%d",targets);
        nfui_nflabel_set_text((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], strBuf);
        nfui_signal_emit((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], GDK_EXPOSE,TRUE);
	}

	return FALSE;
}

static gboolean post_tar_height_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if ( evt->type == NFEVENT_SPINBUTTON_CHANGED ) 
    {
        int index;

        index = nfui_spin_button_get_index(obj);
        zoom_set_height_target(index);

    }

    return FALSE;
}



static gboolean post_estimatebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        zoom_estimate_target(obj->parent);
	}

	return FALSE;
}


static gboolean post_live_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		if(g_live_freeze == 0){
			g_live_freeze = 1; // freeze
			nf_live_stop();
			nf_live_set_freeze(g_live_freeze);
            
            nfui_nfbutton_set_text((NFBUTTON*)obj, "PLAY VIDEO");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    
        }
		else{
			g_live_freeze = 0; // live
			nf_live_set_freeze(g_live_freeze);
			_vca_zoom_init();
			
            nfui_nfbutton_set_text((NFBUTTON*)obj, "PAUSE VIDEO");
            nfui_signal_emit(obj, GDK_EXPOSE, TRUE);    
        }		
	}

	return FALSE;
}




static gboolean post_resetbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;
        gint targets;
        char strBuf[16];

        targets = zoom_reset_target();

        g_sprintf(strBuf, "%d",targets);
        nfui_nflabel_set_text((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], strBuf);
        nfui_signal_emit((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], GDK_EXPOSE,TRUE);

	}

	return FALSE;
}

static gboolean post_okbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        NFOBJECT *topwin;
        gint ret;

        zoom_save_targets();

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{

	if(evt->type == GDK_BUTTON_RELEASE) {
        NFOBJECT *topwin;
		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

        topwin = nfui_nfobject_get_top(obj);
        nfui_nfobject_destroy(topwin);
	}

	return FALSE;
}




static gboolean post_cal_setup_win_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case GDK_MOTION_NOTIFY:
			{
                /*
				GdkEventMotion *mevt;

				mevt = (GdkEventMotion*)evt;
				if(!(mevt->state & GDK_BUTTON1_MASK))
                {
					break;
                }
                nfui_nfobject_move(obj, (gint)mevt->x, (gint)mevt->y);
                */
            }
			break;
		
		case GDK_BUTTON_PRESS:
			{
				GdkEventButton *bevt; 

				if(evt->button.button == MOUSE_RIGTH_BUTTON 
						|| evt->button.button == MOUSE_MIDDLE_BUTTON) 
					return FALSE;
			}
			break;

        case GDK_DELETE:
            {
                zoom_wnd_close();
            }
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_cal_setup_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
			case GDK_BUTTON_PRESS:
			{
				GdkEventButton *bevt; 

				if(evt->button.button == MOUSE_RIGTH_BUTTON 
						|| evt->button.button == MOUSE_MIDDLE_BUTTON) 
					return FALSE;
			}
			break;

		default:
			break;
	}

	return FALSE;
}

void set_text_result_value(gfloat height, gfloat tilt, gfloat focal)
{
    char strBuf[32];
    int cam_tilt;

    sprintf(strBuf, "%.2f", height);
	nfui_nflabel_set_text(g_lb_result[RESULT_CAMERA_HEIGHT], strBuf);

    cam_tilt = tilt;
	sprintf(strBuf, "%d", cam_tilt);
	nfui_nflabel_set_text(g_lb_result[RESULT_TILT_ANGLE], strBuf);

	sprintf(strBuf, "%f", focal);
	nfui_nflabel_set_text(g_lb_result[RESULT_FOCAL_LENGTH], strBuf);
	
	nfui_signal_emit(g_lb_result[RESULT_CAMERA_HEIGHT], GDK_EXPOSE,TRUE);
	nfui_signal_emit(g_lb_result[RESULT_TILT_ANGLE], GDK_EXPOSE,TRUE);
	nfui_signal_emit(g_lb_result[RESULT_FOCAL_LENGTH], GDK_EXPOSE,TRUE);
}

void set_estimate_button(gboolean opt)
{
    if(opt)
    {
        if(nfui_nfobject_is_disabled(g_estimate_btn))
            nfui_nfobject_enable(g_estimate_btn);
    }
    else
    {
        if(!nfui_nfobject_is_disabled(g_estimate_btn))
            nfui_nfobject_disable(g_estimate_btn);
    }
    nfui_signal_emit(g_estimate_btn, GDK_EXPOSE, TRUE);
}

static void _init()
{
    gint targets;
    gchar strBuf[16];

    targets = get_target_num();

    g_sprintf(strBuf, "%d",targets);
    nfui_nflabel_set_text((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], strBuf);
    nfui_signal_emit((NFLABEL*)g_lb_result[RESULT_NUMBER_OF_TARGET], GDK_EXPOSE,TRUE);
}

void VW_VCA_Zoom_Open_Setup(NFWINDOW *parent)
{
    NFOBJECT *setup_wnd = NULL;
    NFOBJECT *main_fixed = NULL;
    NFOBJECT *obj = NULL;

    const gchar *strTitle[] = {"CAMERA HEIGHT", "CAMERA TILT", "FOCAL LENGTH", "NUMBER OF TARGET"};
	static gchar *target_height[200];

    guint pos_x, pos_y, i, j;

    setup_wnd = (NFOBJECT*)nftool_create_popup_window(parent, 0, 0, SETUP_WIN_SIZE_W, SETUP_WIN_SIZE_H-86, "ZOOM", FALSE);
	nfui_regi_post_event_callback(setup_wnd, post_cal_setup_win_event_handler);
    g_vcaSetupWnd = (NFWINDOW*)setup_wnd;
    nfui_nfwindow_set_modal(setup_wnd, FALSE);
    nfui_nfwindow_set_moving_area_size((NFWINDOW*)setup_wnd, SETUP_WIN_SIZE_H);
    nfui_nfwindow_set_moving_limit((NFWINDOW*)setup_wnd, TRUE);
    nfui_nfwindow_set_mask((NFWINDOW*)setup_wnd, GDK_BUTTON_PRESS, TRUE);

    main_fixed = ((NFWINDOW*)setup_wnd)->child;
	nfui_regi_post_event_callback(main_fixed, post_cal_setup_fixed_event_handler);


    pos_y = 56;
    obj = nftool_normal_button_create_type1("ADD", 110);
    nfui_regi_post_event_callback(obj, post_addbutton_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 15, pos_y);  
    nfui_nfobject_show(obj);
    nfui_set_key_focus(obj, TRUE);


    obj = nftool_normal_button_create_type1("DELETE", 110);
    nfui_regi_post_event_callback(obj, post_delbutton_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 130, pos_y);  
    nfui_nfobject_show(obj);

    obj = nftool_normal_button_create_type1("RESET", 110);

    nfui_regi_post_event_callback(obj, post_resetbutton_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 245, pos_y);  
    nfui_nfobject_show(obj);

    pos_y += 50;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("HEIGHT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, TARGET_LABEL_SIZE_W, TARGET_LABEL_SIZE_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 15, pos_y);

	for (i = 0 , j = IVCA_MIN_CALIB_HEIGHT; j < IVCA_MAX_CALIB_HEIGHT; i++, j +=5 ) {
		target_height[i] = imalloc(32);
		if ( target_height[i] ) {
			sprintf(target_height[i], "%u", j);
		}
	}

	g_tar_height = (NFOBJECT*)nfui_spinbutton_new((gchar**)target_height, i, 25); 
	nfui_spinbutton_set_skin_type(g_tar_height, NFSPINBUTTON_TYPE_1);
	nfui_nfobject_set_size(g_tar_height, TARGET_VALUE_SIZE_W, TARGET_LABEL_SIZE_H);
    nfui_nfobject_show(g_tar_height);
    nfui_regi_post_event_callback(g_tar_height, post_tar_height_cb); 
    nfui_nffixed_put((NFFIXED*)main_fixed, g_tar_height, 15+TARGET_LABEL_SIZE_W, pos_y);

	for (i = 0 , j = IVCA_MIN_CALIB_HEIGHT; j < IVCA_MAX_CALIB_HEIGHT; i++, j +=5 ) 
        ifree(target_height[i]);
	
    pos_y += 47;

	obj = nftool_normal_button_create_type1("PAUSE VIDEO", TARGET_VALUE_SIZE_W);
    nfui_regi_post_event_callback(obj, post_live_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj,  15, pos_y);  
    nfui_nfobject_show(obj);
    g_live_btn = obj;

    obj = nftool_normal_button_create_type1("ESTIMATE", TARGET_VALUE_SIZE_W);
    nfui_regi_post_event_callback(obj, post_estimatebutton_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj,  15+TARGET_LABEL_SIZE_W, pos_y);  
    nfui_nfobject_show(obj);
    g_estimate_btn = obj;

    pos_y += 70;

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("3D CALIBRATION RESULT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_set_size(obj, SETUP_WIN_SIZE_W-15, TARGET_LABEL_SIZE_H);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 15, pos_y);

    for( i = 0; i < RESULT_VALUES; i++)
    {
        pos_y += 43;

        obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle[i], nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
        nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
        nfui_nfobject_set_size(obj, TARGET_LABEL_SIZE_W, TARGET_LABEL_SIZE_H);
        nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
        nfui_nfobject_show(obj);
        nfui_nffixed_put((NFFIXED*)main_fixed, obj, 15, pos_y);

        g_lb_result[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_SMALL_SEMI));
        nfui_nflabel_set_skin_type((NFLABEL*)g_lb_result[i], NFTEXTBOX_TYPE_OUTPUT);
        nfui_nflabel_set_align((NFLABEL*)g_lb_result[i], NFALIGN_CENTER, 0);
        nfui_nfobject_use_focus(g_lb_result[i], NFOBJECT_FOCUS_OFF);
        nfui_nfobject_set_size(g_lb_result[i], TARGET_VALUE_SIZE_W, TARGET_VALUE_SIZE_H);
        nfui_nffixed_put((NFFIXED*)main_fixed, g_lb_result[i], 15 + TARGET_LABEL_SIZE_W, pos_y);

        if(i < 2)
            nfui_nfobject_show(g_lb_result[i]);
        else
        {
            nfui_nfobject_hide(obj);
            nfui_nfobject_hide(g_lb_result[i]);
            pos_y -= 43;
        }
    }
	
    pos_y += 70;

    obj = nftool_normal_button_create_type1("OK", 110);
    nfui_regi_post_event_callback(obj, post_okbutton_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 100, pos_y);  
    nfui_nfobject_show(obj);

    obj = nftool_normal_button_create_type1("CANCEL", 110);
    nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);
    nfui_nffixed_put((NFFIXED*)main_fixed, obj, 130 + 110, pos_y);  
    nfui_nfobject_show(obj);


    nfui_nfobject_show(setup_wnd);
	nfui_make_key_hierarchy((NFWINDOW*)setup_wnd);

    _init();

}



