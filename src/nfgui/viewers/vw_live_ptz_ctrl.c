
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
#include "objects/nfimage.h"

#include "vw_live_ptz_main.h"
#include "vw_live_ptz_internal.h"
#include "vw_live_ptz_property_popup.h"

#include "vw_vkeyboard_num.h"
#include "vw_zoom_pip.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "vsm.h"
#include "ssm.h"
#include "smt.h"
#include "scm.h"
#include "vw.h"

// TABLE1 ATTR.
#define CTRL_TBL1_COLS_CNT				(3)
#define CTRL_TBL1_ROWS_CNT				(3)

#define CTRL_TBL1_COLS_SPACE			(1)
#define CTRL_TBL1_ROWS_SPACE			(1)

// TABLE2 ATTR.
#define CTRL_TBL2_COLS_CNT				(3)
#define CTRL_TBL2_ROWS_CNT				(3)

#define CTRL_TBL2_COLS_SPACE			(0)
#define CTRL_TBL2_ROWS_SPACE			(11)


#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)    		(kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)    		(kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)    		(kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#endif

static NFOBJECT *cmd_dir_obj[9];
static NFOBJECT *zoom_label_obj;
static NFOBJECT *focus_label_obj;
static NFOBJECT *iris_label_obj;
static NFOBJECT *zoom_obj[2];
static NFOBJECT *focus_obj[2];
static NFOBJECT *iris_obj[2];
static NFOBJECT *onepush_obj;
static guint g_ch;

static NFWINDOW *g_curwnd = 0;

static PtzData *g_ptzData;
static PtzPresetData *g_presetData;


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

	support = scm_get_ipcam_onepush_supp(ch);

	if (!support) nfui_nfobject_enable(onepush_obj);
	else nfui_nfobject_disable(onepush_obj);

    return 0;
}

static gint _change_obj_focus(NFOBJECT* from, NFOBJECT *to)
{
	nfui_set_key_focus(from, FALSE);
	nfui_set_key_focus(to, TRUE);

	nfui_signal_emit(from, GDK_EXPOSE, TRUE);
	nfui_signal_emit(to, GDK_EXPOSE, TRUE);

	return 0;
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
        if (g_presetData->cnt == 0) return FALSE;

        g_message("%s, %d, number:%d", __FUNCTION__, __LINE__, g_presetData->number[0]);            
        scm_run_ptz_cmd_goto(g_presetData->number[0]);
#endif		
	}

	return FALSE;
}

static gboolean _post_property_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;
	gint retVal;

	if(evt->type == GDK_BUTTON_PRESS) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

        retVal = VW_PtzCtrl_Property_Open(g_curwnd, g_ptzData);        

        if (retVal) _live_ptz_main_cmd_pattern(g_ch);        
        
        scm_init_ptz_param(g_ch);
    }
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_UP) 
		{
            if (scm_get_ipcam_ptz_supp(g_ch) == 0)
                _change_obj_focus(obj, cmd_dir_obj[6]);                
		
			return TRUE;
		}
		else if (kpid == KEYPAD_DOWN)
		{
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

            if (scm_get_ipcam_onepush_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, onepush_obj);
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

            if (scm_get_ipcam_onepush_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, onepush_obj);
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
            if (scm_get_ipcam_onepush_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, onepush_obj);
    			return TRUE;
            }
          
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
            if (scm_get_ipcam_onepush_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, onepush_obj);
    			return TRUE;
            }
          
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

static gboolean post_onepush_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) 
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		if(scm_req_ipcam_onepush(g_ch) < 0) 
		{
			vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_ONEPUSH_FAIL, NFTOOL_MB_OK);
			return FALSE;
		}	

	}
	else if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS) 
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

		if (kpid == KEYPAD_UP) 
		{
            if (scm_get_ipcam_iris_supp(g_ch) == 0)
            {
                _change_obj_focus(obj, iris_obj[0]);
    			return TRUE;
            }

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
		else if (kpid == KEYPAD_DOWN)
		{
			return TRUE;
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
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_D, x, y,-1, -1, NFALIGN_LEFT, 0);	
        else
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_N, x, y,-1, -1, NFALIGN_LEFT, 0);	             
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
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_D, x, y,-1, -1, NFALIGN_LEFT, 0);	
        else
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_N, x, y,-1, -1, NFALIGN_LEFT, 0);	           
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
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_D, x, y,-1, -1, NFALIGN_LEFT, 0);	
        else
		    nfutil_draw_image(drawable, gc, IMG_PTZ_CTRL_ARROW_N, x, y,-1, -1, NFALIGN_LEFT, 0);	        
		nfui_nfobject_gc_unref(gc);
	}

	return FALSE;
}

gint _live_ptz_ctrl_open(NFOBJECT *parent, NFOBJECT *ctrl_fixed)
{
	NFOBJECT *ntb;
	NFOBJECT *obj;

	NFOBJECT *fixed_temp;

	GdkPixbuf *dir_img[9][NFOBJECT_STATE_COUNT];

	GdkPixbuf *zoom_out_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoom_in_img[NFOBJECT_STATE_COUNT];
	
	GdkPixbuf *focus_near_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *focus_far_img[NFOBJECT_STATE_COUNT];
	
	GdkPixbuf *iris_open_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *iris_close_img[NFOBJECT_STATE_COUNT];
	
	guint ntb1_width[CTRL_TBL1_COLS_CNT] = {0, };
	guint ntb2_width[CTRL_TBL2_COLS_CNT] = {0, };
	guint size_w, size_h;

	guint i, j;
    gint pos_x, pos_y;

    g_curwnd = parent;

// <---- IMAGE LOAD
	dir_img[0][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_LEFT), NULL);
	dir_img[0][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_LEFT), NULL);
	dir_img[0][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_LEFT), NULL);
	dir_img[0][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_LEFT), NULL);

	dir_img[1][0] = nfui_get_image_from_file((IMG_N_PTZ_UP), NULL);
	dir_img[1][1] = nfui_get_image_from_file((IMG_O_PTZ_UP), NULL);
	dir_img[1][2] = nfui_get_image_from_file((IMG_P_PTZ_UP), NULL);
	dir_img[1][3] = nfui_get_image_from_file((IMG_D_PTZ_UP), NULL);

	dir_img[2][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_RIGHT), NULL);
	dir_img[2][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_RIGHT), NULL);
	dir_img[2][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_RIGHT), NULL);
	dir_img[2][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_RIGHT), NULL);

	dir_img[3][0] = nfui_get_image_from_file((IMG_N_PTZ_LEFT), NULL);
	dir_img[3][1] = nfui_get_image_from_file((IMG_O_PTZ_LEFT), NULL);
	dir_img[3][2] = nfui_get_image_from_file((IMG_P_PTZ_LEFT), NULL);
	dir_img[3][3] = nfui_get_image_from_file((IMG_D_PTZ_LEFT), NULL);

	dir_img[4][0] = nfui_get_image_from_file((IMG_N_PTZ_CENTER), NULL);
	dir_img[4][1] = nfui_get_image_from_file((IMG_O_PTZ_CENTER), NULL);
	dir_img[4][2] = nfui_get_image_from_file((IMG_P_PTZ_CENTER), NULL);
	dir_img[4][3] = nfui_get_image_from_file((IMG_D_PTZ_CENTER), NULL);

	dir_img[5][0] = nfui_get_image_from_file((IMG_N_PTZ_RIGHT), NULL);
	dir_img[5][1] = nfui_get_image_from_file((IMG_O_PTZ_RIGHT), NULL);
	dir_img[5][2] = nfui_get_image_from_file((IMG_P_PTZ_RIGHT), NULL);
	dir_img[5][3] = nfui_get_image_from_file((IMG_D_PTZ_RIGHT), NULL);

	dir_img[6][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_LEFT), NULL);
	dir_img[6][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_LEFT), NULL);
	dir_img[6][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_LEFT), NULL);
	dir_img[6][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_LEFT), NULL);

	dir_img[7][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN), NULL);
	dir_img[7][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN), NULL);
	dir_img[7][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN), NULL);
	dir_img[7][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN), NULL);

	dir_img[8][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_RIGHT), NULL);
	dir_img[8][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_RIGHT), NULL);
	dir_img[8][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_RIGHT), NULL);
	dir_img[8][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_RIGHT), NULL);

	zoom_out_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOMOUT), NULL);
	zoom_out_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOMOUT), NULL);
	zoom_out_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOMOUT), NULL);
	zoom_out_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOMOUT), NULL);

	zoom_in_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOMIN), NULL);
	zoom_in_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOMIN), NULL);
	zoom_in_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOMIN), NULL);
	zoom_in_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOMIN), NULL);

	focus_near_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FOCUS_NEAR), NULL);
	focus_near_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FOCUS_NEAR), NULL);
	focus_near_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FOCUS_NEAR), NULL);
	focus_near_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FOCUS_NEAR), NULL);

	focus_far_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FOCUS_FAR), NULL);
	focus_far_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FOCUS_FAR), NULL);
	focus_far_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FOCUS_FAR), NULL);
	focus_far_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FOCUS_FAR), NULL);

	iris_open_img[0] = nfui_get_image_from_file((IMG_N_PTZ_IRIS_OPEN), NULL);
	iris_open_img[1] = nfui_get_image_from_file((IMG_O_PTZ_IRIS_OPEN), NULL);
	iris_open_img[2] = nfui_get_image_from_file((IMG_P_PTZ_IRIS_OPEN), NULL);
	iris_open_img[3] = nfui_get_image_from_file((IMG_D_PTZ_IRIS_OPEN), NULL);

	iris_close_img[0] = nfui_get_image_from_file((IMG_N_PTZ_IRIS_CLOSE), NULL);
	iris_close_img[1] = nfui_get_image_from_file((IMG_O_PTZ_IRIS_CLOSE), NULL);
	iris_close_img[2] = nfui_get_image_from_file((IMG_P_PTZ_IRIS_CLOSE), NULL);
	iris_close_img[3] = nfui_get_image_from_file((IMG_D_PTZ_IRIS_CLOSE), NULL);

    pos_x = 22;
    pos_y = 12;

    nfui_get_pixbuf_size(dir_img[0][0], &size_w, &size_h);

	ntb1_width[0] = size_w;
	ntb1_width[1] = size_w;
	ntb1_width[2] = size_w;	

	ntb = (NFOBJECT*)nfui_nftable_new(CTRL_TBL1_COLS_CNT, CTRL_TBL1_ROWS_CNT, CTRL_TBL1_COLS_SPACE, CTRL_TBL1_ROWS_SPACE, ntb1_width, size_h);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed, ntb, pos_x, pos_y);

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

    pos_y += (size_h*3+2+6);

	obj = nftool_normal_button_create_popup_type1("PROPERTY", size_w*3+2);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, _post_property_button_event_handler);

    pos_x += (size_w*3+2+41);
    pos_y = 12;

    nfui_get_pixbuf_size(zoom_out_img[0], &size_w, &size_h);
	ntb2_width[0] = size_w;
    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_N, &size_w, &size_h);
	ntb2_width[1] = size_w;
    nfui_get_pixbuf_size(zoom_in_img[0], &size_w, &size_h);
	ntb2_width[2] = size_w;	

	ntb = (NFOBJECT*)nfui_nftable_new(CTRL_TBL2_COLS_CNT, CTRL_TBL2_ROWS_CNT, CTRL_TBL2_COLS_SPACE, CTRL_TBL2_ROWS_SPACE, ntb2_width, size_h);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed, ntb, pos_x, pos_y);

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

    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_N, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ZOOM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 100, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));		
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));	
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (size_w-100)/2, (size_h-40)/2);
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

    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_N, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FOCUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 100, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (size_w-100)/2, (size_h-40)/2);
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

    nfui_get_image_size(IMG_PTZ_CTRL_ARROW_N, &size_w, &size_h);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("IRIS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));	
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_set_size(obj, 100, 40);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));	
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_DISABLE, COLOR_IDX(200));	
	nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)fixed_temp, obj, (size_w-100)/2, (size_h-40)/2);
	iris_label_obj = obj;
	
	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, iris_close_img);
	nfui_get_pixbuf_size(iris_close_img[0], &size_w, &size_h);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 2);
	nfui_regi_post_event_callback(obj, _post_close_button_event_handler);    
    iris_obj[1] = obj;

    pos_y += (50*3 + CTRL_TBL2_ROWS_SPACE*2 + 7);

	obj = nftool_normal_button_create_popup_type1("ONE PUSH FOCUS", 312);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ctrl_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_onepush_event_handler);
	onepush_obj = obj;

	return 0;
}

gint _live_ptz_ctrl_set_data(PtzData *ptz_data, PtzPresetData *preset_data)
{
    g_ptzData = ptz_data;
    g_presetData = preset_data;

    return 0;
}

gint _live_ptz_ctrl_init_channel(guint channel)
{
	scm_init_ptz_param(channel);
    _init_supported_func(channel);
	g_ch = channel;
	
	return 0;
}

gint _live_ptz_ctrl_change_channel(guint channel, gint expose)
{
    gint i;

	scm_init_ptz_param(channel);
    _init_supported_func(channel);

    if (expose)
    {
    	for (i = 0; i < 9; i++)
            nfui_signal_emit(cmd_dir_obj[i], GDK_EXPOSE, TRUE);

        nfui_signal_emit(zoom_label_obj, GDK_EXPOSE, TRUE);

    	for (i = 0; i < 2; i++)
            nfui_signal_emit(zoom_obj[i], GDK_EXPOSE, TRUE);

        nfui_signal_emit(focus_label_obj, GDK_EXPOSE, TRUE);

    	for (i = 0; i < 2; i++)
            nfui_signal_emit(focus_obj[i], GDK_EXPOSE, TRUE);

        nfui_signal_emit(iris_label_obj, GDK_EXPOSE, TRUE);

    	for (i = 0; i < 2; i++)
            nfui_signal_emit(iris_obj[i], GDK_EXPOSE, TRUE);

        nfui_signal_emit(onepush_obj, GDK_EXPOSE, TRUE);
    }

    g_ch = channel;
   
    return 0;
}

