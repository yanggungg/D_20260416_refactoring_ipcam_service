#include "vw_image_setup_advanced.h"

#include "nf_afx.h"

#include "vw_sys_camera_main.h"
#include "vw_sys_camera_image.h"

#include "tools/nf_ui_tool.h"
#include "support/nf_ui_page_manager.h"


#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/util.h"
#include "support/color.h"

#include "objects/nfobject.h"
#include "objects/nfbutton.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"
#include "objects/nfimglabel.h"
#include "objects/nfcombobox.h"
#include "objects/nfspinbutton.h"
#include "objects/cw_slider.h"

#include "nf_api_ipcam.h"
#include "nf_api_eventlog.h"
#include "vw_vkeyboard.h"

#include "ix_mem.h"
#include "vsm.h"
#include "uxm.h"
#include "vw.h"

//#define _SUPPORT_ANTIFLICKER

#define WIN_SIZE_W			(600)
#define WIN_SIZE_H			(1080)

#define SUB_FIXED_X			(20)
#define SUB_FIXED_W			(542)

#ifdef _SUPPORT_ANTIFLICKER
#define PR_FIXED_ROWS		(6)
#else
#define PR_FIXED_ROWS		(5)
#endif
#define EX_FIXED_ROWS		(8)
#define WB_FIXED_ROWS		(2)

#define PR_FIXED_Y			(64)
#define PR_FIXED_H			(40 + 20 + 40*PR_FIXED_ROWS + 3*(PR_FIXED_ROWS-1))

#define EX_FIXED_Y			(PR_FIXED_Y + PR_FIXED_H + 15)
#define EX_FIXED_H			(40 + 20 + 40*EX_FIXED_ROWS + 3*(EX_FIXED_ROWS-1))

#define WB_FIXED_Y			(EX_FIXED_Y + EX_FIXED_H + 15)
#define WB_FIXED_H			(40 + 20 + 40*WB_FIXED_ROWS + 3*(WB_FIXED_ROWS-1))

#define LABEL_SIZE_W		(242)

#define VIDEO_SIZE_W		(1920 - WIN_SIZE_W)
#define VIDEO_SIZE_H		(1080*VIDEO_SIZE_W/1920)
#define VIDEO_SIZE_X		(0)
#define VIDEO_SIZE_Y		((1080-VIDEO_SIZE_H)/2)


static NFWINDOW *g_curwnd = 0;
static gint g_ch_index;

static NFOBJECT *video_obj;
static NFOBJECT *g_sharp_obj;
static NFOBJECT *g_antifl_obj;
static NFOBJECT *g_rotate_obj;
static NFOBJECT *g_zoom_obj[2];
static NFOBJECT *g_zoom_speed_obj;
static NFOBJECT *g_focus_obj[2];
static NFOBJECT *g_onepush_obj;
static NFOBJECT *g_calibration_obj;
static NFOBJECT *g_exposure_obj[7];
static NFOBJECT *g_dn_obj[2];
static NFOBJECT *g_wb_obj[2];

static ColorData *g_color_data;
static AdvancedData *g_advanced_data;
static NFIPCamImageProfile *g_ipcam_pf;

static	const gchar *zoomSpeed_str[] = {"1", "3", "5", "7", "10",
									"15", "20", "25", "30", "40", "50",
									 "60", "70", "80", "90", "100"};

static gint _start_preview(gint ch)
{
	vsm_live_preview_start(1 << ch, VIDEO_SIZE_X, VIDEO_SIZE_Y, VIDEO_SIZE_W, VIDEO_SIZE_H);
    
    return 0;
}

static gint _stop_preview()
{
    vsm_live_preview_stop();
    return 0;
}

static gint _show_video_obj()
{
    nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));

    return 0;
}

static gint _hide_video_obj()
{
    nfui_nfobject_modify_bg(video_obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_808080));
    nfui_signal_emit(video_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gboolean vw_cam_check_supported_func(gint ch, guint check_bit)
{
	if (g_ipcam_pf->supported & check_bit)
		return TRUE;
	else 
		return FALSE;		
}

static NFOBJECT *_get_object_matched_object(guint value)
{
	NFOBJECT *obj;

	if (value == NF_IPCAM_IMAGE_SHARPNESS) 		return g_sharp_obj;
	if (value == NF_IPCAM_IMAGE_BRIGHTNESS) 	return NULL;
	if (value == NF_IPCAM_IMAGE_CONTRAST) 		return NULL;
	if (value == NF_IPCAM_IMAGE_COLOR) 			return NULL;
	if (value == NF_IPCAM_IMAGE_TINT) 			return NULL;
	if (value == NF_IPCAM_IMAGE_EXPOSURE) 		return g_exposure_obj[0];
	if (value == NF_IPCAM_IMAGE_AGC) 			return g_exposure_obj[1];
	if (value == NF_IPCAM_IMAGE_ESHUTTER) 		return g_exposure_obj[2];	
	if (value == NF_IPCAM_IMAGE_SLOWSHUTTER) 	return g_exposure_obj[3];
	if (value == NF_IPCAM_IMAGE_MAXAGC) 		return g_exposure_obj[4];	
	
	if (value == NF_IPCAM_IMAGE_DCIRIS) 		return g_exposure_obj[5];	
	if (value == NF_IPCAM_IMAGE_BLC) 			return g_exposure_obj[6];	
	if (value == NF_IPCAM_IMAGE_CALIBRATION) 	return g_calibration_obj;	
	if (value == NF_IPCAM_IMAGE_ICF) 			return NULL;	
	if (value == NF_IPCAM_IMAGE_WB) 			return g_wb_obj[0];	
	if (value == NF_IPCAM_IMAGE_MWB) 			return g_wb_obj[1];	
	if (value == NF_IPCAM_IMAGE_WDR) 			return NULL;	
	if (value == NF_IPCAM_IMAGE_MIRRORING) 		return g_rotate_obj;	
	if (value == NF_IPCAM_IMAGE_ANTIFLICKER) 	return g_antifl_obj;	
	if (value == NF_IPCAM_IMAGE_DNN) 			return g_dn_obj[0];	
	
	if (value == NF_IPCAM_IMAGE_PIRIS) 			return NULL;	
	if (value == NF_IPCAM_IMAGE_PAN) 			return NULL;	
	if (value == NF_IPCAM_IMAGE_PRESET)			return NULL;	
	if (value == NF_IPCAM_IMAGE_ZOOM) 			return g_zoom_obj[0];	
	if (value == NF_IPCAM_IMAGE_FOCUS) 			return g_focus_obj[0];
	if (value == NF_IPCAM_IMAGE_ONEPUSH) 		return g_onepush_obj;	
	if (value == NF_IPCAM_IMAGE_DNN_TOGGLE) 	return g_dn_obj[1];	

	return NULL;
}

static gint _disable_obj(NFOBJECT *obj)
{
	if (obj)
	{			
		nfui_nfobject_disable(obj);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		return 0;
	}

	return -1;
}

static gint _enable_obj(NFOBJECT *obj)
{
	if (obj)
	{			
		nfui_nfobject_enable(obj);
		nfui_signal_emit(obj, GDK_EXPOSE, TRUE);
		return 0;
	}

	return -1;
}

static gint _control_object(NFIPCamOption option)
{
	gint i;
	NFOBJECT* matched_obj;

// disable object
	for (i = 0; i < NF_IPCAM_IMAGE_NR; i++)
	{
		if (option.disable_category & (1 << i))
		{
			matched_obj = _get_object_matched_object(1 << i);
			_disable_obj(matched_obj);
		}
	}

// enable object
	for (i = 0; i < NF_IPCAM_IMAGE_NR; i++)
	{
		if ((option.enable_category & (1 << i)) && (g_ipcam_pf->supported & (1 << i)))
		{
			matched_obj = _get_object_matched_object(1 << i);
			_enable_obj(matched_obj);
		}
	}

	return 0;	
}

static gboolean _check_number_validity(NFOBJECT *obj, gint *num)
{
	gint num_temp;

	if (obj == g_exposure_obj[2])
	{
		if (*num > g_ipcam_pf->eshutter.max) {
			vw_mbox(g_curwnd, "NOTICE", IMBX_INVALID_NUMBER_HIGH, NFTOOL_MB_OK);
			*num = g_ipcam_pf->eshutter.max;
		}

		if (*num < g_ipcam_pf->eshutter.min) {
			vw_mbox(g_curwnd, "NOTICE", IMBX_INVALID_NUMBER_LOW, NFTOOL_MB_OK);
			*num = g_ipcam_pf->eshutter.min;
		}
	}
	else if (obj == g_exposure_obj[1])
	{
		if (*num > g_ipcam_pf->agc.max) {
			vw_mbox(g_curwnd, "NOTICE", IMBX_INVALID_NUMBER_HIGH, NFTOOL_MB_OK);
			*num = g_ipcam_pf->agc.max;
		}
	}
	else
	{
		if (*num > g_ipcam_pf->sharpness.max) {
			vw_mbox(g_curwnd, "NOTICE", IMBX_INVALID_NUMBER_HIGH, NFTOOL_MB_OK);
			*num = g_ipcam_pf->sharpness.max;
		}

		if (*num < g_ipcam_pf->sharpness.min) {
			vw_mbox(g_curwnd, "NOTICE", IMBX_INVALID_NUMBER_LOW, NFTOOL_MB_OK);
			*num = g_ipcam_pf->sharpness.min;
		}		
	}

	num_temp = nfui_nflabel_get_number((NFLABEL*)(obj));

	if (num_temp == *num) {
		nftool_mbox(g_curwnd, "NOTICE", "It's a number used already.", NFTOOL_MB_OK);
		return FALSE;
	}

	return TRUE;
}

static gboolean _get_number_virtual_key(NFOBJECT *obj, gint x, gint y, gint *num)
{
	gint numTemp;
	gint ret_num;

	numTemp = nfui_nflabel_get_number((NFLABEL*)obj);

	if (obj == g_exposure_obj[2])
		ret_num = NumberKey_Open(g_curwnd, numTemp, x, y, g_ipcam_pf->eshutter.max);
	else if (obj == g_exposure_obj[1])
		ret_num = NumberKey_Open(g_curwnd, numTemp, x, y, g_ipcam_pf->agc.max);
	else
		ret_num = NumberKey_Open(g_curwnd, numTemp, x, y, g_ipcam_pf->sharpness.max);

	if (ret_num == -1) 
		return FALSE;
		
	if (!_check_number_validity(obj, &ret_num))
		return FALSE;
	
	nfui_nflabel_set_number((NFLABEL*)obj, ret_num);
	nfui_signal_emit(obj, GDK_EXPOSE, TRUE);

	*num = ret_num;

	return TRUE;
}

static void _send_command_ipcam_advanced()
{
	NFIPCamSetupImage info;
	gint index;
			
	memset(&info, 0x00, sizeof(NFIPCamSetupImage));
	
	info.ch = g_ch_index;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[0]);
	info.exposure = g_ipcam_pf->exposure[index].value;

	info.agc = nfui_nflabel_get_number((NFLABEL*)g_exposure_obj[1]);
	info.eshutter_speed = nfui_nflabel_get_number((NFLABEL*)g_exposure_obj[2]);

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[3]);
	info.slow_shutter = g_ipcam_pf->slowshutter[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[4]);
	info.max_agc = g_ipcam_pf->maxagc[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[5]);
	info.iris = g_ipcam_pf->dc_iris[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[6]);
	info.blc = g_ipcam_pf->blc[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_dn_obj[0]);
	info.day_night = g_ipcam_pf->dnn[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_dn_obj[1]);
	info.det_time = g_ipcam_pf->dnn_toggle[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_wb_obj[0]);
	info.white_balance = g_ipcam_pf->wb[index].value;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_wb_obj[1]);
	info.mwb = g_ipcam_pf->mwb[index].value;
	
	info.sharpness = nfui_nflabel_get_number((NFLABEL*)g_sharp_obj);
	info.brightness = g_color_data->bright;
	info.contrast = g_color_data->contrast;
	info.tint = g_color_data->tint;
	info.color = g_color_data->color;

	scm_set_ipcam_image_advanced(&info);
}

static void _save_data(NFOBJECT *obj)
{
	gint index;

	if (obj == g_sharp_obj)				g_advanced_data->sharpness = nfui_nflabel_get_number((NFLABEL*)obj);
	else if (obj == g_exposure_obj[1])	g_advanced_data->agc_gain = nfui_nflabel_get_number((NFLABEL*)obj);
	else if (obj == g_exposure_obj[2])	g_advanced_data->shutter_speed = nfui_nflabel_get_number((NFLABEL*)obj);		
	else if (obj == g_rotate_obj)		
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->rotate = g_ipcam_pf->mirror[index].value;
	}
	else if (obj == g_antifl_obj)		g_advanced_data->antiflicker = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);

	if (obj == g_exposure_obj[0])	
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->exposure_mode = g_ipcam_pf->exposure[index].value;	
	}
	else if (obj == g_exposure_obj[3])	
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->slow_shutter = g_ipcam_pf->slowshutter[index].value;	
	}
	else if (obj == g_exposure_obj[4])	
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->max_agc = g_ipcam_pf->maxagc[index].value;	
	}
	else if (obj == g_exposure_obj[5])	
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->iris_control = g_ipcam_pf->dc_iris[index].value;	
	}
	else if (obj == g_exposure_obj[6])	
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->blc_control = g_ipcam_pf->blc[index].value;	
	}
	else if (obj == g_wb_obj[0])		
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->wb_mode = g_ipcam_pf->wb[index].value;	
	}
	else if (obj == g_wb_obj[1])		
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->mwb_mode = g_ipcam_pf->mwb[index].value;	
	}
	else if (obj == g_dn_obj[0])		
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->day_night_mode = g_ipcam_pf->dnn[index].value;	
	}
	else if (obj == g_dn_obj[1])
	{
		index = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		g_advanced_data->day_night_duration = g_ipcam_pf->dnn_toggle[index].value;	
	}
}

static gboolean post_rotate_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		gint rotate;
		rotate = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		scm_set_ipcam_rotation(g_ch_index, g_ipcam_pf->mirror[rotate].value);
		_save_data(obj);		
	}

	return FALSE;
}

static gboolean post_antiflicker_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		gint sig;
		sig = nfui_combobox_get_cur_index((NFCOMBOBOX*)obj);
		scm_set_ipcam_antiflicker(g_ch_index, sig);
		_save_data(obj);		
	}

	return FALSE;
}

static gboolean post_onepush_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	static NFOBJECT *wait_mbox = NULL;

	switch(event->type) {
		case GDK_BUTTON_RELEASE:
		{
			if(event->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
#if 1
			if(scm_req_ipcam_onepush(g_ch_index) < 0) 
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

static gboolean post_calibration_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;
	static NFOBJECT *wait_mbox = NULL;

	switch(event->type) {
		case GDK_BUTTON_RELEASE:
		{
			if(event->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}

			if(!wait_mbox)
				wait_mbox = nftool_mbox_wait(g_curwnd, "WAIT", "Please wait...");

			if(scm_req_ipcam_calibration(g_ch_index) < 0) 
			{
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
				wait_mbox = NULL;
				return FALSE;
			}
		}
		break;

		case INFY_IPCAM_CALIBRATION_BEGIN:
		break;

		case INFY_IPCAM_CALIBRATION_END:
		{		
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}		
		}
		break;

		case INFY_IPCAM_CALIBRATION_PENDING:
		break;

		case INFY_IPCAM_CALIBRATION_REQ_FAIL:
		{		
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}

			vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_CALI_FAIL, NFTOOL_MB_OK);
		}
		break;

		case INFY_IPCAM_CALIBRATION_TIMEOUT:
		{	
			if(wait_mbox) {
				nftool_remove_waitbox((NFOBJECT*)wait_mbox);
				wait_mbox = NULL;
			}

//			vw_mbox(g_curwnd, "ERROR", IMBX_IPCAM_CALI_TIMEOUT, NFTOOL_MB_OK);
		}
		break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_BEGIN);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_END);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_PENDING);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_REQ_FAIL);
			uxm_unreg_imsg_event(obj, INFY_IPCAM_CALIBRATION_TIMEOUT);
		break;

		default:
		break;		
	}
	
	return FALSE;
}

static gboolean post_zoomout_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_zoomin_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_zoom_speed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NFOBJECT *top;
	gint index, speed;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));

		if (index < 0) return FALSE;

		speed = atoi(zoomSpeed_str[index]);
		if (speed < 0) speed = 5;	// default

/*		if (index == 0) speed = 1;
		else if (index == 1) speed = 5;
		else if (index == 2) speed = 10;
		else if (index == 3) speed = 50;
		else if (index == 4) speed = 100;
*/
		
		scm_set_ptz_cmd_zoom_speed(speed);
	}
	
	return FALSE;
}

static gboolean post_near_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean post_far_button_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
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

static gboolean _post_exposure_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint i, index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->exposure[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);		
	}
	
	return FALSE;
}

static gboolean _post_slow_shutter_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->slowshutter[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}

	return FALSE;
}

static gboolean _post_max_agc_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->maxagc[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}

	return FALSE;
}

static gboolean _post_iris_control_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->dc_iris[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}

	return FALSE;
}

static gboolean _post_blc_control_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->blc[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}

	return FALSE;
}


static gboolean _post_dn_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->dnn[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}

	return FALSE;
}

static gboolean _post_dn_toggle_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->dnn_toggle[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}

	return FALSE;
}

static gboolean _post_wb_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->wb[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}
	
	return FALSE;
}

static gboolean _post_mwb_mode_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint index;

	if(evt->type == NFEVENT_COMBOBOX_CHANGED) {	
		index = nfui_combobox_get_cur_index(NF_COMBOBOX(obj));
		_control_object(g_ipcam_pf->mwb[index]);
		_send_command_ipcam_advanced();
		_save_data(obj);				
	}
	
	return FALSE;
}

static gboolean post_number_label_common_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid=KEYPAD_NONE;

	if(evt->type == NFEVENT_KEYPAD_PRESS || evt->type == NFEVENT_REMOCON_PRESS)
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;
	}

	if(evt->type == GDK_2BUTTON_PRESS || kpid == KEYPAD_ENTER)
	{	
		NFOBJECT *top = NULL;
		gint num;
		guint i, x, y;

		if(kpid == KEYPAD_ENTER)
		{
			nfui_nfobject_get_offset(obj, &x, &y);
			top = nfui_nfobject_get_top(obj);

			x += (obj->width)/2 + top->x;
			y += obj->height + top->y;
		}
		else
		{
			if(evt->button.button == MOUSE_RIGTH_BUTTON) {					
				return FALSE;
			}
		
			nfui_nfobject_get_window_pos(obj, &x, &y);

			x += ((GdkEventButton*)evt)->x;
			y += ((GdkEventButton*)evt)->y;
		}

		if (_get_number_virtual_key(obj, x, y, &num))
		{
			_send_command_ipcam_advanced();
			_save_data(obj);
		}
	}

	return FALSE;
}

static gboolean post_main_wnd_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_DELETE)
	{
		g_curwnd = 0;
		gtk_main_quit();
	}

	return FALSE;
}

static gboolean post_main_fixed_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	switch(evt->type)
	{
		case GDK_EXPOSE :
		{
    		drawable = nfui_nfobject_get_window(obj);
    		gc = nfui_nfobject_get_gc(obj);

            nfui_nfobject_get_size(obj, &size_w, &size_h);
            pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
            nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

    		nfui_nfobject_gc_unref(gc);
    	}
		break;

		case GDK_DELETE:
		{
            nfui_nfobject_get_size(obj, &size_w, &size_h);
            nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);		
        }
		break;
			
		default :
			break;
	}

	return FALSE;
}

static gboolean _post_exit_btn_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON) {					
			return FALSE;
		}

        _hide_video_obj();
		_stop_preview();

		topwin = nfui_nfobject_get_top(obj);
		nfui_nfobject_destroy(topwin);	
	}
	
	return FALSE;
}

static gint _init_sharp(NFOBJECT *obj)
{
	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_SHARPNESS)
	{
		nfui_nflabel_set_number(obj, g_advanced_data->sharpness);
	}
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_sharp_legend(NFOBJECT *obj)
{
	gchar strBuf[32];

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_SHARPNESS)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "[%d..%d]", g_ipcam_pf->sharpness.min, g_ipcam_pf->sharpness.max);
		nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	}

	return 0;		
}

static gint _init_rotate(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_MIRRORING)
	{
		for (i = 0; i < g_ipcam_pf->mirror_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX*)obj, g_ipcam_pf->mirror[i].caption);
			if (g_ipcam_pf->mirror[i].value == g_advanced_data->rotate)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->mirror[i].value, g_advanced_data->rotate);
				def_index = i;
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->mirror_cnt, def_index);		
	}
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_antiflicker(NFOBJECT *obj)
{
	gint i;
	const gchar *sig_str[] = {"NTSC", "PAL"};

	if (vw_cam_check_supported_func(g_ch_index, NF_IPCAM_IMAGE_ANTIFLICKER))
	{
		for (i = 0; i < 2; i++)
			nfui_combobox_append_data((NFCOMBOBOX *)obj, sig_str[i]);

		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, g_advanced_data->antiflicker);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_zoom(NFOBJECT *obj)
{
	if (scm_get_ipcam_zoom_supp(g_ch_index) == 0)
		nfui_nfobject_enable(obj);
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_zoom_speed(NFOBJECT *obj)
{
	gint i;

	if (scm_get_ipcam_zoom_supp(g_ch_index) == 0)
	{
		for (i = 0; i < 16; i++)
			nfui_combobox_append_data((NFCOMBOBOX *)obj, zoomSpeed_str[i]);

		nfui_combobox_set_index_no_expose((NFCOMBOBOX*)obj, 10);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_focus(NFOBJECT *obj)
{
	if (scm_get_ipcam_focus_supp(g_ch_index) == 0)
		nfui_nfobject_enable(obj);
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_onepush(NFOBJECT *obj)
{
	if (scm_get_ipcam_onepush_supp(g_ch_index) == 0)
		nfui_nfobject_enable(obj);
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_home(NFOBJECT *obj)
{
	if (vw_cam_check_supported_func(g_ch_index, NF_IPCAM_IMAGE_CALIBRATION))
		nfui_nfobject_enable(obj);
	else
		nfui_nfobject_disable(obj);

	return 0;		
}

static gint _init_exposure_mode(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_EXPOSURE)
	{
		for (i = 0; i < g_ipcam_pf->exposure_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->exposure[i].caption);

			if (g_ipcam_pf->exposure[i].value == g_advanced_data->exposure_mode)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->exposure[i].value, g_advanced_data->exposure_mode);
				def_index = i;
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->exposure_cnt, def_index);		
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_agc_gain(NFOBJECT *obj)
{
	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_AGC)
	{
		nfui_nflabel_set_number(obj, g_advanced_data->agc_gain);
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_agc_gain_legend(NFOBJECT *obj)
{
	gchar strBuf[32];

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_AGC)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "[%d..%d]", g_ipcam_pf->agc.min, g_ipcam_pf->agc.max);
		nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	}

	return 0;		
}

static gint _init_eshutter(NFOBJECT *obj)
{
	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_ESHUTTER)
	{
		nfui_nflabel_set_number(obj, g_advanced_data->shutter_speed);
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_eshutter_legend(NFOBJECT *obj)
{
	gchar strBuf[32];

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_ESHUTTER)
	{
		memset(strBuf, 0x00, sizeof(strBuf));
		g_sprintf(strBuf, "[%d..%d]", g_ipcam_pf->eshutter.min, g_ipcam_pf->eshutter.max);
		nfui_nflabel_set_text((NFLABEL*)obj, strBuf);
	}

	return 0;		
}

static gint _init_slow_shutter(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_SLOWSHUTTER)
	{
		for (i = 0; i < g_ipcam_pf->slowshutter_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->slowshutter[i].caption);

			if (g_ipcam_pf->slowshutter[i].value == g_advanced_data->slow_shutter)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->slowshutter[i].value, g_advanced_data->slow_shutter);			
				def_index = i;
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->slowshutter_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_max_agc(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_MAXAGC)
	{
		for (i = 0; i < g_ipcam_pf->maxagc_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->maxagc[i].caption);

			if (g_ipcam_pf->maxagc[i].value == g_advanced_data->max_agc)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->slowshutter[i].value, g_advanced_data->slow_shutter);			
				def_index = i;
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);	

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->maxagc_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_iris_control(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_DCIRIS)
	{
		for (i = 0; i < g_ipcam_pf->dc_iris_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->dc_iris[i].caption);

			if (g_ipcam_pf->dc_iris[i].value == g_advanced_data->iris_control)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->dc_iris[i].value, g_advanced_data->iris_control);
				def_index = i;			
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->dc_iris_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_blc_control(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_BLC)
	{
		for (i = 0; i < g_ipcam_pf->blc_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->blc[i].caption);

			if (g_ipcam_pf->blc[i].value == g_advanced_data->blc_control)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->blc[i].value, g_advanced_data->blc_control);			
				def_index = i;			
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->blc_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_day_night(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_DNN)
	{
		for (i = 0; i < g_ipcam_pf->dnn_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->dnn[i].caption);

			if (g_ipcam_pf->dnn[i].value == g_advanced_data->day_night_mode)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->dnn[i].value, g_advanced_data->day_night_mode);			
				def_index = i;			
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->dnn_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_day_night_toggle(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_DNN_TOGGLE)
	{
		for (i = 0; i < g_ipcam_pf->dnn_toggle_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->dnn_toggle[i].caption);

			if (g_ipcam_pf->dnn_toggle[i].value == g_advanced_data->day_night_duration)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->dnn_toggle[i].value, g_advanced_data->day_night_duration);			
				def_index = i;						
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->dnn_toggle_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_wb_mode(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_WB)
	{
		for (i = 0; i < g_ipcam_pf->wb_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->wb[i].caption);

			if (g_ipcam_pf->wb[i].value == g_advanced_data->wb_mode)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->wb[i].value, g_advanced_data->wb_mode);			
				def_index = i;									
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->wb_cnt, def_index);			
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_mwb_mode(NFOBJECT *obj)
{
	gint i;
	gint def_index = -1;

	if (g_ipcam_pf->supported & NF_IPCAM_IMAGE_MWB)
	{
		for (i = 0; i < g_ipcam_pf->mwb_cnt; i++)
		{
			nfui_combobox_append_data((NFCOMBOBOX *)obj, g_ipcam_pf->mwb[i].caption);

			if (g_ipcam_pf->mwb[i].value == g_advanced_data->mwb_mode)
			{
				g_message("%s, %d, <cam:%08X, db:%08X>", __FUNCTION__, __LINE__, g_ipcam_pf->mwb[i].value, g_advanced_data->mwb_mode);			
				def_index = i;
			}
		}

		if (def_index != -1)
			nfui_combobox_set_index_no_expose((NFCOMBOBOX *)obj, def_index);

		g_message("%s, %d, <cnt:%d, def_index:%d>", __FUNCTION__, __LINE__, g_ipcam_pf->mwb_cnt, def_index);
	}
	else
		nfui_nfobject_disable(obj);

	return 0;	
}

static gint _init_relation()
{
	gint i, index;

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[0]);
	_control_object(g_ipcam_pf->exposure[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[3]);
	_control_object(g_ipcam_pf->slowshutter[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[4]);
	_control_object(g_ipcam_pf->maxagc[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[5]);
	_control_object(g_ipcam_pf->dc_iris[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_exposure_obj[6]);
	_control_object(g_ipcam_pf->blc[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_dn_obj[0]);
	_control_object(g_ipcam_pf->dnn[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_dn_obj[1]);
	_control_object(g_ipcam_pf->dnn_toggle[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_wb_obj[0]);
	_control_object(g_ipcam_pf->wb[index]);	

	index = nfui_combobox_get_cur_index((NFCOMBOBOX*)g_wb_obj[1]);
	_control_object(g_ipcam_pf->mwb[index]);	
}

void vw_open_image_setup_advanced(NFWINDOW *parent, guint ch, ColorData *color_data, AdvancedData *advanced_data, NFIPCamImageProfile *ipcam_pf)
{
	NFOBJECT *win;
	NFOBJECT *tmp_fixed;		//tmp
	NFOBJECT *bg_fixed[2];
	NFOBJECT *main_fixed;
	NFOBJECT *pr_fixed;
	NFOBJECT *ex_fixed;
	NFOBJECT *wb_fixed;	
	NFOBJECT *obj;

	GdkPixbuf *pbCamImage[32];
	GdkPixbuf *zoomout_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *zoomin_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *near_img[NFOBJECT_STATE_COUNT];
	GdkPixbuf *far_img[NFOBJECT_STATE_COUNT];	
	
	guint pos_x, pos_y;
	gint size_w, size_h;	
	gint i;

	gchar strBuf[STRING_SIZE_128];
	const gchar *ex_str[] = {"EXPOSURE MODE", "AGC GAIN", "E-SHUTTER SPEED", "SLOW SHUTTER", 
							"MAX AGC", "IRIS CONTROL", "BLC CONTROL"};
	const gchar *wb_str[] = {"WB MODE", "MWB MODE"};	

	NFIPCamStatusInfo info;

    _start_preview(ch);

// IMAGE LOAD
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

	zoomout_img[0] = nfui_get_image_from_file((IMG_N_ADVANCED_ZOOMOUT), NULL);
	zoomout_img[1] = nfui_get_image_from_file((IMG_O_ADVANCED_ZOOMOUT), NULL);
	zoomout_img[2] = nfui_get_image_from_file((IMG_P_ADVANCED_ZOOMOUT), NULL);
	zoomout_img[3] = nfui_get_image_from_file((IMG_D_ADVANCED_ZOOMOUT), NULL);

	zoomin_img[0] = nfui_get_image_from_file((IMG_N_ADVANCED_ZOOMIN), NULL);
	zoomin_img[1] = nfui_get_image_from_file((IMG_O_ADVANCED_ZOOMIN), NULL);
	zoomin_img[2] = nfui_get_image_from_file((IMG_P_ADVANCED_ZOOMIN), NULL);
	zoomin_img[3] = nfui_get_image_from_file((IMG_D_ADVANCED_ZOOMIN), NULL);

	near_img[0] = nfui_get_image_from_file((IMG_N_ADVANCED_FOCUS_NEAR), NULL);
	near_img[1] = nfui_get_image_from_file((IMG_O_ADVANCED_FOCUS_NEAR), NULL);
	near_img[2] = nfui_get_image_from_file((IMG_P_ADVANCED_FOCUS_NEAR), NULL);
	near_img[3] = nfui_get_image_from_file((IMG_D_ADVANCED_FOCUS_NEAR), NULL);

	far_img[0] = nfui_get_image_from_file((IMG_N_ADVANCED_FOCUS_FAR), NULL);
	far_img[1] = nfui_get_image_from_file((IMG_O_ADVANCED_FOCUS_FAR), NULL);
	far_img[2] = nfui_get_image_from_file((IMG_P_ADVANCED_FOCUS_FAR), NULL);
	far_img[3] = nfui_get_image_from_file((IMG_D_ADVANCED_FOCUS_FAR), NULL);

	g_ch_index = ch;
	g_color_data = color_data; 
	g_advanced_data = advanced_data;
	g_ipcam_pf = ipcam_pf;
	
	scm_init_ptz_param(ch);
	
	// window
	//win = (NFOBJECT*)nfui_nfwindow_new(GTK_WINDOW_TOPLEVEL, 0, 0, 1920, 1080);
	win = (NFOBJECT*)nfui_nfwindow_new(parent, 0, 0, 1920, 1080);
	g_curwnd = win;
	nfui_nfobject_modify_bg(win, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_regi_post_event_callback(win, post_main_wnd_event_handler);

//tmp
	tmp_fixed = (NFOBJECT*)nfui_nffixed_new();		
	nfui_nfobject_set_size(tmp_fixed, 1920, 1080);
	nfui_nfobject_modify_bg(tmp_fixed, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nffixed_put((NFFIXED*)win, tmp_fixed, 0, 0);
	nfui_nfobject_show(tmp_fixed);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_PRG_IDX(UX_COLOR_000000));
	nfui_nfobject_set_size(obj, VIDEO_SIZE_W, VIDEO_SIZE_H);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)tmp_fixed, obj, VIDEO_SIZE_X, VIDEO_SIZE_Y);	
    video_obj = obj;

	// bg_fixed1
	bg_fixed[0] = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(bg_fixed[0], VIDEO_SIZE_W, VIDEO_SIZE_Y);	
	nfui_nfobject_modify_bg(bg_fixed[0], NFOBJECT_STATE_NORMAL, COLOR_IDX(3));
	nfui_nffixed_put((NFFIXED*)tmp_fixed, bg_fixed[0], 0, 0);
	nfui_nfobject_show(bg_fixed[0]);

	// bg_fixed2
	bg_fixed[1] = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(bg_fixed[1], VIDEO_SIZE_W, 1080-VIDEO_SIZE_Y-VIDEO_SIZE_H);	
	nfui_nfobject_modify_bg(bg_fixed[1], NFOBJECT_STATE_NORMAL, COLOR_IDX(3));
	nfui_nffixed_put((NFFIXED*)tmp_fixed, bg_fixed[1], 0, VIDEO_SIZE_Y+VIDEO_SIZE_H);
	nfui_nfobject_show(bg_fixed[1]);

    scm_get_ipcam_status(ch, &info);
    
    if (info.status == NF_IPCAM_STATUS_WARN_FW_VERSION)
    {
    	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("INCOMPATIBLE F/W VERSION", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(399));
    	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);	
    	nfui_nfobject_set_size(obj, 500, 40);
    	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
    	nfui_nfobject_show(obj);
    	nfui_nffixed_put((NFFIXED*)bg_fixed[1], obj, (VIDEO_SIZE_W-500)/2, (1080-VIDEO_SIZE_Y-VIDEO_SIZE_H-40)/2);
    }
  
	// main_fixed	
	main_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(main_fixed, WIN_SIZE_W, WIN_SIZE_H);	
	nfui_nfobject_modify_bg(main_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nffixed_put((NFFIXED*)tmp_fixed, main_fixed, 1920-WIN_SIZE_W, 0);
	nfui_nfobject_show(main_fixed);
	nfui_regi_post_event_callback(main_fixed, post_main_fixed_event_handler);

	// image property fixed
	pr_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(pr_fixed, SUB_FIXED_W, PR_FIXED_H);
	nfui_nfobject_show(pr_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, pr_fixed, SUB_FIXED_X, PR_FIXED_Y);	

	// exposure fixed
	ex_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(ex_fixed, SUB_FIXED_W, EX_FIXED_H);
	nfui_nfobject_show(ex_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, ex_fixed, SUB_FIXED_X, EX_FIXED_Y);	

	// white balance fixed
	wb_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(wb_fixed, SUB_FIXED_W, WB_FIXED_H);
	nfui_nfobject_show(wb_fixed);
	nfui_nffixed_put((NFFIXED*)main_fixed, wb_fixed, SUB_FIXED_X, WB_FIXED_Y);

// <---- WINDOW SUBJECT
	//DAL_get_camera_title(strBuf, ch);
	var_get_camtitle(strBuf, ch);
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strBuf, nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, WIN_SIZE_W-74, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, 37, 4);

// IMAGE PROPERTY
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "IMAGE PROPERTY CONTROL");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 60;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SHARPNESS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);
	nfui_nflabel_use_number(obj, TRUE);
	nfui_nfobject_set_size(obj, 130, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);			
	nfui_regi_post_event_callback(obj, post_number_label_common_event_handler);
	_init_sharp(obj);
	g_sharp_obj = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);	
	nfui_nfobject_set_size(obj, 168, 32);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);	
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+130+2, pos_y+8);
	_init_sharp_legend(obj);

	pos_y += 43;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MIRROR", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x, pos_y);	

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);	
	nfui_nfobject_set_size(obj, (SUB_FIXED_W-LABEL_SIZE_W), 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
	nfui_regi_post_event_callback(obj, post_rotate_event_handler);
	_init_rotate(obj);
	g_rotate_obj = obj;

	pos_y += 43;

#ifdef _SUPPORT_ANTIFLICKER
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ANTIFLICKER", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x, pos_y);	

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, (SUB_FIXED_W-LABEL_SIZE_W), 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
	nfui_regi_post_event_callback(obj, post_antiflicker_event_handler);
	_init_antiflicker(obj);
	g_antifl_obj = obj;

	pos_y += 43;
#endif

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ZOOM", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x, pos_y);	

	nfui_get_pixbuf_size(zoomout_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, zoomout_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
	nfui_regi_post_event_callback(obj, post_zoomout_button_event_handler);
	_init_zoom(obj);	
	g_zoom_obj[0] = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, zoomin_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+size_w+4, pos_y);
	nfui_regi_post_event_callback(obj, post_zoomin_button_event_handler);
	_init_zoom(obj);
	g_zoom_obj[1] = obj;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("STEP", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, 80, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_CENTER, 4);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+(size_w+4)*2, pos_y);

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, (SUB_FIXED_W-LABEL_SIZE_W-(size_w+4)*2-80-2), 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+(size_w+4)*2+80+2, pos_y);
	nfui_regi_post_event_callback(obj, post_zoom_speed_event_handler);
	_init_zoom_speed(obj);	
	g_zoom_speed_obj = obj;

	pos_y += 43;

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("FOCUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x, pos_y);	

	nfui_get_pixbuf_size(near_img[0], &size_w, &size_h);

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, near_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
	nfui_regi_post_event_callback(obj, post_near_button_event_handler);
	_init_focus(obj);		
	g_focus_obj[0] = obj;

	obj = nfui_nfbutton_new();
	nfui_nfbutton_set_image((NFBUTTON*)obj, far_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+size_w+4, pos_y);
	nfui_regi_post_event_callback(obj, post_far_button_event_handler);
	_init_focus(obj);			
	g_focus_obj[1] = obj;

	obj = nftool_normal_button_create_type3("ONE PUSH", (SUB_FIXED_W-LABEL_SIZE_W-(size_w+4)*2));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+(size_w+4)*2, pos_y);
	nfui_regi_post_event_callback(obj, post_onepush_event_handler);
	_init_onepush(obj);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_BEGIN);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_END);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_PENDING);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_REQ_FAIL);
	uxm_reg_imsg_event(obj, INFY_IPCAM_ONEPUSH_TIMEOUT);	
	g_onepush_obj = obj;

	pos_y += 43;

	obj = nftool_normal_button_create_type3("HOME", (SUB_FIXED_W-LABEL_SIZE_W-(size_w+4)*2));
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)pr_fixed, obj, pos_x+LABEL_SIZE_W+(size_w+4)*2, pos_y);
	nfui_regi_post_event_callback(obj, post_calibration_event_handler);
	_init_home(obj);	
	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_BEGIN);
	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_END);
	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_PENDING);
	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_REQ_FAIL);
	uxm_reg_imsg_event(obj, INFY_IPCAM_CALIBRATION_TIMEOUT);	
	g_calibration_obj = obj;

// EXPOSURE CONTROL
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "EXPOSURE CONTROL");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ex_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 60;

	for (i = 0; i < 7; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(ex_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x, pos_y);

		if (i == 0)
		{
			obj = nfui_combobox_new(NULL, 0, 0);
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, (SUB_FIXED_W-LABEL_SIZE_W), 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
			nfui_regi_post_event_callback(obj, _post_exposure_mode_event_handler);	
			_init_exposure_mode(obj);
		}
		else if (i == 1)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292)); 
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);	
			nfui_nfobject_set_size(obj, 168, 32);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);				
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W+130+2, pos_y+8);				
			_init_agc_gain_legend(obj);

			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);			
			nfui_nflabel_use_number(obj, TRUE);
			nfui_nfobject_set_size(obj, 130, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
			nfui_regi_post_event_callback(obj, post_number_label_common_event_handler);		
			_init_agc_gain(obj);
		}
		else if (i == 2)
		{
			obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292)); 
			nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 10);	
			nfui_nfobject_set_size(obj, 168, 32);
			nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);				
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W+130+2, pos_y+8);
			_init_eshutter_legend(obj);
		
			obj = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
			nfui_nflabel_set_skin_type((NFLABEL*)obj, NFTEXTBOX_TYPE_POPUP_INPUT);			
			nfui_nflabel_use_number(obj, TRUE);
			nfui_nfobject_set_size(obj, 130, 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);	
			nfui_regi_post_event_callback(obj, post_number_label_common_event_handler);
			_init_eshutter(obj);
		}
		else 
		{
			obj = nfui_combobox_new("", 0, 0);			
			nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
			nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
			nfui_nfobject_set_size(obj, (SUB_FIXED_W-LABEL_SIZE_W), 40);
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);	
			
			if (i == 3) 		
			{
				nfui_regi_post_event_callback(obj, _post_slow_shutter_event_handler);
				_init_slow_shutter(obj);
			}
			else if (i == 4) 	
			{
				nfui_regi_post_event_callback(obj, _post_max_agc_event_handler);
				_init_max_agc(obj);
			}
			else if (i == 5) 	
			{
				nfui_regi_post_event_callback(obj, _post_iris_control_event_handler);			
				_init_iris_control(obj);
			}
			else if (i == 6) 	
			{
				nfui_regi_post_event_callback(obj, _post_blc_control_event_handler);			
				_init_blc_control(obj);
			}
		}
			
		g_exposure_obj[i] = obj;		
		pos_y += 43;
	}


	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DAY/NIGHT MODE", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
	nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x, pos_y);	

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 130, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
	nfui_regi_post_event_callback(obj, _post_dn_mode_event_handler);
	_init_day_night(obj);	
	g_dn_obj[0] = obj;

	obj = nfui_combobox_new(NULL, 0, 0);
	nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
	nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
	nfui_nfobject_set_size(obj, 168, 40);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)ex_fixed, obj, pos_x+LABEL_SIZE_W+130+2, pos_y);
	nfui_regi_post_event_callback(obj, _post_dn_toggle_event_handler);
	_init_day_night_toggle(obj);	
	g_dn_obj[1] = obj;	

// WHITE BALANCE CONTROL
	obj = nfui_nfimage_new(IMG_POPUP_TITLE_BG2);
	nfui_nfimage_set_text((NFIMAGE*)obj, "WHITE BALANCE CONTROL");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, 20);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)wb_fixed, obj, 0, 0);

	pos_x = 0;
	pos_y = 60;

	for (i = 0; i < 2; i++)
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(wb_str[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(292));
		nfui_nfobject_set_size(obj, LABEL_SIZE_W, 40);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 17);
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)wb_fixed, obj, pos_x, pos_y);

		obj = nfui_combobox_new(NULL, 0, 0);
		nfui_combobox_set_skin_type(NF_COMBOBOX(obj), NFCOMBOBOX_TYPE_POPUP_1);
		nfui_combobox_set_align(NF_COMBOBOX(obj), NFALIGN_CENTER, 0);
		nfui_nfobject_set_size(obj, (SUB_FIXED_W-LABEL_SIZE_W), 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)wb_fixed, obj, pos_x+LABEL_SIZE_W, pos_y);
		
		if (i == 0) 
		{
			nfui_regi_post_event_callback(obj, _post_wb_mode_event_handler);
			_init_wb_mode(obj);
		}
		else
		{
			nfui_regi_post_event_callback(obj, _post_mwb_mode_event_handler);		
			_init_mwb_mode(obj);
		}
		
		g_wb_obj[i] = obj;
		
		pos_y += 43;
	}

	_init_relation();

	obj = nftool_normal_button_create_type1("EXIT", 174);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)main_fixed, obj, (WIN_SIZE_W-174)/2, WIN_SIZE_H-65);
	nfui_regi_post_event_callback(obj, _post_exit_btn_event_handler);

//tmp
	nfui_nfwindow_add((NFWINDOW*)win, tmp_fixed);
	nfui_run_main_event_handler(win);
	nfui_nfobject_show(win);

	nfui_make_key_hierarchy(win);

	nfui_page_open(PGID_IMAGE_SETUP_ADVANCED, win, nfui_get_last_user());

	gtk_main();

	nfui_page_close(PGID_IMAGE_SETUP_ADVANCED, win);

}

