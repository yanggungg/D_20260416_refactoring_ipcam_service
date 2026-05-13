
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

#include "vw_live_ptzctrl.h"
#include "vw_vkeyboard_num.h"
#include "vw_zoom_pip.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "vsm.h"
#include "ssm.h"
#include "smt.h"
#include "scm.h"
#include "vw.h"


// PTZ WINDOW
#define PTZCTRL_SIZE_WID					(273)
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
#define PTZCTRL_SIZE_HEI					(567)
#else
#define PTZCTRL_SIZE_HEI					(521)
#endif

#define PTZCTRL_POS_X						(DISPLAY_ACTIVE_WIDTH - PTZCTRL_SIZE_WID - 20)
#define PTZCTRL_POS_Y						((DISPLAY_ACTIVE_HEIGHT - PTZCTRL_SIZE_HEI)/2)


// TITLE COMBOBOX
#define PTZCTRL_CAM_TITLE_X					(20)
#define PTZCTRL_CAM_TITLE_Y					(10)
#define PTZCTRL_CAM_TITLE_W					(233)
#define PTZCTRL_CAM_TITLE_H					(40)


// TABLE1 ATTR.
#define PTZCTRL_TABLE1_X					(20)
#define PTZCTRL_TABLE1_Y					(PTZCTRL_CAM_TITLE_Y + PTZCTRL_CAM_TITLE_H + 10)
#define PTZCTRL_TABLE1_H					(233)

#define PTZCTRL_TABLE1_COLS_CNT				(3)
#define PTZCTRL_TABLE1_ROWS_CNT				(3)

#define PTZCTRL_TABLE1_COLS_SPACE			(1)
#define PTZCTRL_TABLE1_ROWS_SPACE			(1)


// TABLE2 ATTR.
#define PTZCTRL_TABLE2_X					(20)
#define PTZCTRL_TABLE2_Y					(PTZCTRL_TABLE1_Y + PTZCTRL_TABLE1_H + 10)
#define PTZCTRL_TABLE2_H					(90)

#define PTZCTRL_TABLE2_COLS_CNT				(4)
#define PTZCTRL_TABLE2_ROWS_CNT				(2)

#define PTZCTRL_TABLE2_COLS_SPACE			(1)
#define PTZCTRL_TABLE2_ROWS_SPACE			(10)

#define PTZCTRL_ONEPUSH_BTN_Y				(PTZCTRL_TABLE2_Y + PTZCTRL_TABLE2_H + 4)

#define PTZCTRL_PRESET_POS_X				(20)
#define PTZCTRL_PRESET_POS_Y				(PTZCTRL_TABLE2_Y + PTZCTRL_TABLE2_ROWS_SPACE + 90)


// CLOSE BUTTON.
#define PTZCTRL_CLOSE_BTN_X					(80)
#if defined(_HDI_MODEL_UX) || defined(_DVR_MODEL_UX)
#define PTZCTRL_CLOSE_BTN_Y					(PTZCTRL_SIZE_HEI - 60)
#else
#define PTZCTRL_CLOSE_BTN_Y					(PTZCTRL_ONEPUSH_BTN_Y + 40 + 24)
#endif
#define PTZCTRL_CLOSE_BTN_W					(113)


#if defined(GUI_4CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)    		(kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH04)
#elif defined(GUI_8CH_SUPPORT)
#define GET_CHANNEL_KEY_SCOPE(kpid)    		(kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH08)
#else
#define GET_CHANNEL_KEY_SCOPE(kpid)    		(kpid >= KEYPAD_CH01 && kpid <= KEYPAD_CH16)
#endif


static NFWINDOW *g_curwnd = 0;
static gint check_cmd[16] = {0, };

static NFOBJECT *ch_obj;
static NFOBJECT *cmd_dir_obj[9];
static NFOBJECT *zoom_obj[2];
static NFOBJECT *focus_obj[2];
static NFOBJECT *onepush_obj;
static NFOBJECT *preset_obj;

static gchar *strCh[GUI_CHANNEL_CNT];
static guint g_ch;

static gboolean _post_channel_combo__event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ch;
	gint i, j, support;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		ch = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if(vsm_get_covert_state(NULL, ch)) {
			nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
			nfui_combobox_set_index_no_expose(NF_COMBOBOX(obj), g_ch);
			nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
			return FALSE;
		}

		vsm_change_sfc_by_menu_opened(ch);
		g_ch = ch;
		scm_init_ptz_param(ch);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
		support = scm_get_ipcam_ptz_supp(g_ch);

		for (j = 0; j < 3; j++)
		{
			for (i = 0; i < 3; i++)
			{
				if (!support) 
				{
					if ((i != 1) && (j != 1))
						nfui_nfobject_enable(cmd_dir_obj[i+j*3]);
				}
				else 
					nfui_nfobject_disable(cmd_dir_obj[i+j*3]);
				nfui_signal_emit(cmd_dir_obj[i+j*3], GDK_EXPOSE, TRUE);				
			}
		}

		support = scm_get_ipcam_zoom_supp(g_ch);

		for (i = 0; i < 2; i++)
		{
			if (!support) nfui_nfobject_enable(zoom_obj[i]);
			else nfui_nfobject_disable(zoom_obj[i]);
			
			nfui_signal_emit(zoom_obj[i], GDK_EXPOSE, TRUE);
		}

		support = scm_get_ipcam_focus_supp(g_ch);

		for (i = 0; i < 2; i++)
		{
			if (!support) nfui_nfobject_enable(focus_obj[i]);
			else nfui_nfobject_disable(focus_obj[i]);
			
			nfui_signal_emit(focus_obj[i], GDK_EXPOSE, TRUE);
		}		

		if (!support) nfui_nfobject_enable(onepush_obj);
		else nfui_nfobject_disable(onepush_obj);
		
		nfui_signal_emit(onepush_obj, GDK_EXPOSE, TRUE);		
#endif		
	}

	return FALSE;
}

static gboolean _post_digizoom_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;

	gint x, y, ch;

	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

		x = DISPLAY_ACTIVE_WIDTH - VW_ZoomPIP_Width();
		y = DISPLAY_ACTIVE_HEIGHT - VW_ZoomPIP_Height();
		ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

        nfui_nfobject_hide(g_curwnd);
		
		g_ch = VW_ZoomPIP_Open(g_curwnd, x, y, ch, ZOOM_PIP_NONE);
        nfui_combobox_set_index_no_expose(NF_COMBOBOX(ch_obj), g_ch);
    	vsm_change_sfc_by_menu_opened(g_ch);

        nfui_nfobject_show(g_curwnd);		

    	smt_set_service(SMT_LIVE_PTZ);
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
	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
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

	return FALSE;
}

static gboolean _post_autofocus_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

	if(evt->type == GDK_BUTTON_PRESS) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}


	}
	else if(evt->type == GDK_BUTTON_RELEASE || evt->type == GDK_LEAVE_NOTIFY) 
	{
		scm_stop_ptz_cmd();
	}

	return FALSE;
}

static gboolean post_onepush_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	static NFOBJECT *wait_mbox = NULL;
	gint ch;

	switch(event->type) {
		case GDK_BUTTON_RELEASE:
		{
			if(event->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
#if 1
			ch = nfui_combobox_get_cur_index(NF_COMBOBOX(ch_obj));

			if(scm_req_ipcam_onepush(ch) < 0) 
			{
				vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_ONEPUSH_FAIL, NFTOOL_MB_OK);
				return FALSE;
			}
#else

			if(!wait_mbox)
				wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

			if(scm_req_ipcam_onepush(g_ch_index) < 0) 
			{
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_ONEPUSH_FAIL, NFTOOL_MB_OK);
				wait_mbox = NULL;
				return FALSE;
			}
#endif			
		}
		break;
		
#if 0
		case INFY_IPCAM_ONEPUSH_BEGIN:
		break;

		case INFY_IPCAM_ONEPUSH_END:
		{		
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}		
		}
		break;

		case INFY_IPCAM_ONEPUSH_PENDING:
		break;

		case INFY_IPCAM_ONEPUSH_REQ_FAIL:
		{		
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}

			vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_ONEPUSH_FAIL, NFTOOL_MB_OK);
		}
		break;

		case INFY_IPCAM_ONEPUSH_TIMEOUT:
		{	
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}

			vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_ONEPUSH_TIMEOUT, NFTOOL_MB_OK);
		}
		break;
#endif

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_IPCAM_ONEPUSH_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_ONEPUSH_END);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_ONEPUSH_PENDING);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_ONEPUSH_REQ_FAIL);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_ONEPUSH_TIMEOUT);
		break;

		default:
		break;		
	}
	
	return FALSE;
}

static gboolean _post_preset_label_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
  	if(evt->type == GDK_2BUTTON_PRESS)
	{
		gint numTemp;
		guint x, y;

		if(evt->button.button == MOUSE_RIGTH_BUTTON)
			return FALSE;

		nfui_nfobject_get_window_pos(obj, &x, &y);

		x += ((GdkEventButton*)evt)->x;
		y += ((GdkEventButton*)evt)->y;
		
		numTemp = NumberKey_Open(g_curwnd, nfui_nflabel_get_number((NFLABEL*)obj), x, y, 255);

		if(numTemp >= 0) 
			nfui_nflabel_set_number((NFLABEL*)obj, numTemp);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
	}

	return FALSE;
}

static gboolean _post_set_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint param;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		if(vsm_get_covert_state(NULL, g_ch)) {
			nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
			return FALSE; 
		}

		param = nfui_nflabel_get_number((NFLABEL*)preset_obj);
		scm_set_ptz_preset(param);
	}

	return FALSE;
}

static gboolean _post_goto_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		gint param;

		if(evt->button.button == MOUSE_RIGTH_BUTTON) 
			return FALSE;

		if(vsm_get_covert_state(NULL, g_ch)) {
			nftool_mbox(g_curwnd, "ERROR", "Current channel is coverted.",  NFTOOL_MB_OK);
			return FALSE;
		}

		param = nfui_nflabel_get_number((NFLABEL*)preset_obj);
		scm_run_ptz_cmd_goto(param);
	}

	return FALSE;
}

static gboolean _post_close_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	guint i = 0;

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
		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
		{
			kevt = (GdkEventKey*)event;
			kpid = (KEYPAD_KID)kevt->keyval;

			if (GET_CHANNEL_KEY_SCOPE(kpid))
			{
				gint cam_num;
				
				cam_num = kpid - KEYPAD_CH01;		
				nfui_combobox_set_index(NF_COMBOBOX(ch_obj), cam_num);
				nfui_signal_emit(ch_obj, GDK_EXPOSE, TRUE);
			}
			else
			{
				switch(kpid)
				{
					case KEYPAD_EXIT:
						nfui_nfobject_destroy(obj);
					break;

					default:
					break;
				}
			}
		}
		break;		

		case GDK_DELETE:
			g_curwnd = 0;
			gtk_main_quit();
			break;

		default:
		break;
	}

	return FALSE;
}

static gboolean _pre_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    GdkDrawable *drawable = NULL;
    GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;
	guint i, x, y;

	if(event->type == GDK_EXPOSE) {
        GdkDrawable *drawable = NULL;
        GdkGC *gc;
        GdkPixbuf *pbuf = NULL;
        gint size_w, size_h;

        drawable = nfui_nfobject_get_window(obj);
        gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

        nfui_nfobject_gc_unref(gc);
	}else if(event->type == GDK_DELETE) {
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);	
	
		for(i=0; i<GUI_CHANNEL_CNT; i++) 
			ifree(strCh[i]);	
	}
	
	return FALSE;
}


gint VW_PtzCtrl_Open(NFWINDOW *parent, guint channel)
{
	NFOBJECT *ptz_win;
	NFOBJECT *main_fixed;

	NFOBJECT *ntb;
	NFOBJECT *obj;

	GdkPixbuf *ptzdir_img[9][NFOBJECT_STATE_COUNT];

	GdkPixbuf *ptz_zoomout_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ptz_zoomin_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ptz_digizoom_img[NFOBJECT_STATE_COUNT];
	
	GdkPixbuf *ptz_near_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ptz_far_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *ptz_autofocus_img[NFOBJECT_STATE_COUNT];
	
	guint ntb1_width[PTZCTRL_TABLE1_COLS_CNT] = {0, };
	guint ntb2_width[PTZCTRL_TABLE2_COLS_CNT] = {0, };

	guint size_w, size_h;

	guint i, j;
	gint support;


// <---- PTZCTRL INIT - PTZ CMD INIT / MAKE BG IMAGE
	scm_init_ptz_param(channel);
	check_cmd[channel] = 1;
	g_ch = channel;

	for (i = 0; i<GUI_CHANNEL_CNT; i++)
	{
		strCh[i] = imalloc(sizeof(gchar) * (STRING_SIZE_CAMTITLE+8));	
		j = sprintf(strCh[i], "CH%d - ", i+1);
		DAL_get_camera_title(strCh[i]+j, i);
	}

// <---- IMAGE LOAD
	ptzdir_img[0][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_LEFT), NULL);
	ptzdir_img[0][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_LEFT), NULL);
	ptzdir_img[0][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_LEFT), NULL);
	ptzdir_img[0][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_LEFT), NULL);

	ptzdir_img[1][0] = nfui_get_image_from_file((IMG_N_PTZ_UP), NULL);
	ptzdir_img[1][1] = nfui_get_image_from_file((IMG_O_PTZ_UP), NULL);
	ptzdir_img[1][2] = nfui_get_image_from_file((IMG_P_PTZ_UP), NULL);
	ptzdir_img[1][3] = nfui_get_image_from_file((IMG_D_PTZ_UP), NULL);

	ptzdir_img[2][0] = nfui_get_image_from_file((IMG_N_PTZ_UP_RIGHT), NULL);
	ptzdir_img[2][1] = nfui_get_image_from_file((IMG_O_PTZ_UP_RIGHT), NULL);
	ptzdir_img[2][2] = nfui_get_image_from_file((IMG_P_PTZ_UP_RIGHT), NULL);
	ptzdir_img[2][3] = nfui_get_image_from_file((IMG_D_PTZ_UP_RIGHT), NULL);

	ptzdir_img[3][0] = nfui_get_image_from_file((IMG_N_PTZ_LEFT), NULL);
	ptzdir_img[3][1] = nfui_get_image_from_file((IMG_O_PTZ_LEFT), NULL);
	ptzdir_img[3][2] = nfui_get_image_from_file((IMG_P_PTZ_LEFT), NULL);
	ptzdir_img[3][3] = nfui_get_image_from_file((IMG_D_PTZ_LEFT), NULL);

	ptzdir_img[4][0] = nfui_get_image_from_file((IMG_N_PTZ_CENTER), NULL);
	ptzdir_img[4][1] = nfui_get_image_from_file((IMG_O_PTZ_CENTER), NULL);
	ptzdir_img[4][2] = nfui_get_image_from_file((IMG_P_PTZ_CENTER), NULL);
	ptzdir_img[4][3] = nfui_get_image_from_file((IMG_N_PTZ_CENTER), NULL);

	ptzdir_img[5][0] = nfui_get_image_from_file((IMG_N_PTZ_RIGHT), NULL);
	ptzdir_img[5][1] = nfui_get_image_from_file((IMG_O_PTZ_RIGHT), NULL);
	ptzdir_img[5][2] = nfui_get_image_from_file((IMG_P_PTZ_RIGHT), NULL);
	ptzdir_img[5][3] = nfui_get_image_from_file((IMG_D_PTZ_RIGHT), NULL);

	ptzdir_img[6][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_LEFT), NULL);
	ptzdir_img[6][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_LEFT), NULL);
	ptzdir_img[6][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_LEFT), NULL);
	ptzdir_img[6][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_LEFT), NULL);

	ptzdir_img[7][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN), NULL);
	ptzdir_img[7][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN), NULL);
	ptzdir_img[7][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN), NULL);
	ptzdir_img[7][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN), NULL);

	ptzdir_img[8][0] = nfui_get_image_from_file((IMG_N_PTZ_DOWN_RIGHT), NULL);
	ptzdir_img[8][1] = nfui_get_image_from_file((IMG_O_PTZ_DOWN_RIGHT), NULL);
	ptzdir_img[8][2] = nfui_get_image_from_file((IMG_P_PTZ_DOWN_RIGHT), NULL);
	ptzdir_img[8][3] = nfui_get_image_from_file((IMG_D_PTZ_DOWN_RIGHT), NULL);

	ptz_zoomout_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOMOUT), NULL);
	ptz_zoomout_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOMOUT), NULL);
	ptz_zoomout_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOMOUT), NULL);
	ptz_zoomout_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOMOUT), NULL);

	ptz_zoomin_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOMIN), NULL);
	ptz_zoomin_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOMIN), NULL);
	ptz_zoomin_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOMIN), NULL);
	ptz_zoomin_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOMIN), NULL);

	ptz_digizoom_img[0] = nfui_get_image_from_file((IMG_N_PTZ_DIGIZOOM), NULL);
	ptz_digizoom_img[1] = nfui_get_image_from_file((IMG_O_PTZ_DIGIZOOM), NULL);
	ptz_digizoom_img[2] = nfui_get_image_from_file((IMG_P_PTZ_DIGIZOOM), NULL);
	ptz_digizoom_img[3] = nfui_get_image_from_file((IMG_D_PTZ_DIGIZOOM), NULL);

	ptz_near_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FOCUS_NEAR), NULL);
	ptz_near_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FOCUS_NEAR), NULL);
	ptz_near_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FOCUS_NEAR), NULL);
	ptz_near_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FOCUS_NEAR), NULL);

	ptz_far_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FOCUS_FAR), NULL);
	ptz_far_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FOCUS_FAR), NULL);
	ptz_far_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FOCUS_FAR), NULL);
	ptz_far_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FOCUS_FAR), NULL);

	ptz_autofocus_img[0] = nfui_get_image_from_file((IMG_N_PTZ_AUTOFOCUS), NULL);
	ptz_autofocus_img[1] = nfui_get_image_from_file((IMG_O_PTZ_AUTOFOCUS), NULL);
	ptz_autofocus_img[2] = nfui_get_image_from_file((IMG_P_PTZ_AUTOFOCUS), NULL);
	ptz_autofocus_img[3] = nfui_get_image_from_file((IMG_D_PTZ_AUTOFOCUS), NULL);
//
	vsm_change_sfc_by_menu_opened(channel);

// <---- WINDOW & FIXED
	ptz_win = (NFOBJECT*)nfui_nfwindow_new(parent, PTZCTRL_POS_X, PTZCTRL_POS_Y, PTZCTRL_SIZE_WID, PTZCTRL_SIZE_HEI);
	nfui_nfwindow_set_title(ptz_win, "PTZ CTRL");
	nfui_nfwindow_set_moving_area_size((NFWINDOW*)ptz_win, PTZCTRL_SIZE_HEI);
	nfui_regi_post_event_callback(ptz_win, _post_main_event_handler);
	g_curwnd = ptz_win;

	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, PTZCTRL_SIZE_WID, PTZCTRL_SIZE_HEI);
	nfui_regi_pre_event_callback(main_fixed, _pre_main_fixed_event_handler);
	nfui_nfobject_show(main_fixed);
//
// <---- TITLE DROPDOWN 
	ch_obj = nfui_combobox_new(strCh, GUI_CHANNEL_CNT, channel);
	nfui_combobox_set_skin_type(NF_COMBOBOX(ch_obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_nfobject_set_size(ch_obj, PTZCTRL_CAM_TITLE_W, PTZCTRL_CAM_TITLE_H);
	nfui_combobox_set_index_no_expose(NF_COMBOBOX(ch_obj), channel);	
	nfui_nfobject_show(ch_obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, ch_obj, PTZCTRL_CAM_TITLE_X, PTZCTRL_CAM_TITLE_Y);
	nfui_regi_post_event_callback(ch_obj, _post_channel_combo__event_handler);

// <---- TABLE1
	ntb1_width[0] = 77;
	ntb1_width[1] = 77;
	ntb1_width[2] = 77;	

	ntb = (NFOBJECT*)nfui_nftable_new(PTZCTRL_TABLE1_COLS_CNT, PTZCTRL_TABLE1_ROWS_CNT, PTZCTRL_TABLE1_COLS_SPACE, PTZCTRL_TABLE1_ROWS_SPACE, ntb1_width, 77);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb, PTZCTRL_TABLE1_X, PTZCTRL_TABLE1_Y);

	nfui_get_pixbuf_size(ptzdir_img[0][0], &size_w, &size_h);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	support = scm_get_ipcam_ptz_supp(channel);
#endif

	for (j = 0; j < 3; j++)
	{
		for (i = 0; i < 3; i++)
		{
			obj = nfui_nfbutton_new();
			nfui_nfbutton_set_image((NFBUTTON*)obj, ptzdir_img[i+j*3]);
			nfui_nfobject_set_size(obj, size_w, size_h);
			if ((i == 1) && (j == 1))	nfui_nfobject_disable(obj);
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
			if (support == -1) nfui_nfobject_disable(obj);
#endif			
			nfui_nfobject_show(obj);
			nfui_nftable_attach((NFTABLE*)ntb, obj, i, j);	
			nfui_regi_post_event_callback(obj, _post_dir_button_event_handler);
			cmd_dir_obj[i+j*3] = obj;
		}

	}

// <---- TABLE 2
	ntb2_width[0] = 68;
	ntb2_width[1] = 54;
	ntb2_width[2] = 54;	
	ntb2_width[3] = 54;	

	ntb = (NFOBJECT*)nfui_nftable_new(PTZCTRL_TABLE2_COLS_CNT, PTZCTRL_TABLE2_ROWS_CNT, PTZCTRL_TABLE2_COLS_SPACE, PTZCTRL_TABLE2_ROWS_SPACE, ntb2_width, 40);
	nfui_nfobject_modify_bg(ntb, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(ntb);
	nfui_nffixed_put((NFFIXED*)main_fixed, ntb, PTZCTRL_TABLE2_X, PTZCTRL_TABLE2_Y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ZOOM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 68, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFFIXED*)ntb, obj, 0, 0);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	support = scm_get_ipcam_zoom_supp(channel);
#endif	
	nfui_get_pixbuf_size(ptz_zoomout_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ptz_zoomout_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 0);	
	nfui_regi_post_event_callback(obj, _post_zoomout_button_event_handler);
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if (support == -1) nfui_nfobject_disable(obj);
#endif
	zoom_obj[0] = obj;

	nfui_get_pixbuf_size(ptz_zoomin_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ptz_zoomin_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 0);	
	nfui_regi_post_event_callback(obj, _post_zoomin_button_event_handler);
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if (support == -1) nfui_nfobject_disable(obj);	
#endif	
	zoom_obj[1] = obj;

	nfui_get_pixbuf_size(ptz_digizoom_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ptz_digizoom_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 0);	
	nfui_regi_post_event_callback(obj, _post_digizoom_button_event_handler);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FOCUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 68, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFFIXED*)ntb, obj, 0, 1);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	support = scm_get_ipcam_focus_supp(channel);
#endif	
	nfui_get_pixbuf_size(ptz_near_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ptz_near_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 1, 1);	
	nfui_regi_post_event_callback(obj, _post_near_button_event_handler);
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if (support == -1) nfui_nfobject_disable(obj);	
#endif	
	focus_obj[0] = obj;

	nfui_get_pixbuf_size(ptz_far_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ptz_far_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 2, 1);	
	nfui_regi_post_event_callback(obj, _post_far_button_event_handler);
#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	if (support == -1) nfui_nfobject_disable(obj);	
#endif	
	focus_obj[1] = obj;

	nfui_get_pixbuf_size(ptz_autofocus_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, ptz_autofocus_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nftable_attach((NFTABLE*)ntb, obj, 3, 1);
	nfui_regi_post_event_callback(obj, _post_autofocus_button_event_handler);

#if !defined(_HDI_MODEL_UX) && !defined(_DVR_MODEL_UX)
	support = scm_get_ipcam_focus_supp(channel);

	obj = nftool_normal_button_create_type3("ONE PUSH", ntb2_width[1]+ntb2_width[2]+ntb2_width[3]+2);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PTZCTRL_TABLE2_X+ntb2_width[0]+1, PTZCTRL_ONEPUSH_BTN_Y);
	nfui_regi_post_event_callback(obj, post_onepush_event_handler);
	if (support == -1) nfui_nfobject_disable(obj);	
	onepush_obj = obj;	
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_BEGIN);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_END);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_PENDING);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_REQ_FAIL);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_TIMEOUT);
#endif

#if 0
// <---- PRESET
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("PRESET", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_set_size(obj, 82, 40);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PTZCTRL_PRESET_POS_X, PTZCTRL_PRESET_POS_Y);

	obj= nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_INPUT);
	nfui_nflabel_set_align(obj, NFALIGN_CENTER, 0);
	nfui_nflabel_use_number((NFLABEL*)obj, TRUE);
	nfui_nflabel_set_number((NFLABEL*)obj, 0);
	nfui_regi_post_event_callback(obj, _post_preset_label_event_handler);
	nfui_nfobject_set_size(obj, 146, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PTZCTRL_PRESET_POS_X+84+2, PTZCTRL_PRESET_POS_Y);
	preset_obj = obj;

	obj = nftool_normal_button_create_popup_type1("SET", 72);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, _post_set_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PTZCTRL_PRESET_POS_X+84+2, PTZCTRL_PRESET_POS_Y+40+10);

	obj = nftool_normal_button_create_popup_type1("GO", 72);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_regi_post_event_callback(obj, _post_goto_button_event_handler);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PTZCTRL_PRESET_POS_X+84+2+72+2, PTZCTRL_PRESET_POS_Y+40+10);
#endif

// <---- CLOSE BUTTON
	obj = nftool_normal_button_create_type2("EXIT", PTZCTRL_CLOSE_BTN_W);
	nfui_nfbutton_set_font_alignment(NF_BUTTON(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, PTZCTRL_CLOSE_BTN_X, PTZCTRL_CLOSE_BTN_Y);
	nfui_regi_post_event_callback(obj, _post_close_button_event_handler);

	nfui_nfwindow_add((NFWINDOW*)ptz_win, main_fixed);
	nfui_run_main_event_handler(ptz_win);
	nfui_nfobject_show(ptz_win);
	nfui_make_key_hierarchy((NFWINDOW*)ptz_win);

	nfui_page_open(PGID_PTZ, ptz_win, ssm_get_cur_id(NULL));

	smt_set_service(SMT_LIVE_PTZ);	

	gtk_main();

	nfui_page_close(PGID_PTZ, ptz_win);

	vsm_recover_sfc_by_menu_closed();

	smt_set_service(SMT_LIVE);	

	return g_ch;
}

void VW_PtzCtrl_Destroy(NFOBJECT *obj)
{
	NFOBJECT *top = NULL;

	top = nfui_nfobject_get_top(obj);
	if(top)
		nfui_nfobject_destroy(top);
}
