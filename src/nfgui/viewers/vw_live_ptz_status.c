
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
#include "vw_live_ptz_full.h"
#include "vw_live_audio_output.h"
#include "vw_zoom_pip.h"

#include "ix_mem.h"
#include "nfdal.h"
#include "vsm.h"
#include "ssm.h"
#include "smt.h"
#include "scm.h"
#include "vw.h"
#include "uxm.h"

#define TBL_ROW_CNT         (5)
#define TBL_COL_CNT         (33)

#define TBL_ROW_SPACE       (4)
#define TBL_COL_SPACE       (2)

static NFOBJECT *rec_obj[GUI_CHANNEL_CNT];
static NFOBJECT *cam_alarm_in_obj[CAM_ALARM_IN];
static NFOBJECT *dvr_alarm_in_obj[DVR_ALARM_IN];
static NFOBJECT *cam_alarm_out_obj[CAM_ALARM_OUT];
static NFOBJECT *dvr_alarm_out_obj[DVR_ALARM_OUT];
static NFOBJECT *exit_obj;
static NFOBJECT *auxiliary_combo_obj;
static NFOBJECT *auxiliary_run_obj;


static GdkPixbuf *alarm_on_img[4];
static GdkPixbuf *alarm_off_img[4];

static GdkPixbuf *rec_no_img[4];
static GdkPixbuf *rec_pre_img[4];
static GdkPixbuf *rec_cont_img[4];
static GdkPixbuf *rec_mot_img[4];
static GdkPixbuf *rec_alarm_img[4];
static GdkPixbuf *rec_panic_img[4];

static NFWINDOW *g_curwnd = 0;
static NFOBJECT *auxiliary_fixed;
static guint g_ch;
static gint g_dvr_alarmIn= DVR_ALARM_IN;


static gint _init_supported_func(guint ch)
{
	gint support;

    support = scm_get_ipcam_auxiliary_supp(ch);
    if(!support){
        nfui_nfobject_enable((NFOBJECT*)auxiliary_run_obj);
        nfui_nfobject_enable((NFOBJECT*)auxiliary_combo_obj);
    }
    else{
        nfui_nfobject_disable((NFOBJECT*) auxiliary_run_obj);
        nfui_nfobject_disable((NFOBJECT*) auxiliary_combo_obj);
    }

    nfui_signal_emit(auxiliary_run_obj, GDK_EXPOSE, TRUE);
    nfui_signal_emit(auxiliary_combo_obj, GDK_EXPOSE, TRUE);

    return 0;
}

static gint _set_rec_status(NF_NOTIFY_INFO *pInfo)
{
    gint i;

	if(pInfo == NULL) return -1;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		switch(pInfo->c.chmap[i]) {
			case 'p':
        		nfui_nfbutton_set_image(NF_BUTTON(rec_obj[i]), rec_pre_img);
			break;
			case 'T':
        		nfui_nfbutton_set_image(NF_BUTTON(rec_obj[i]), rec_cont_img);
			break;
			case 'A':
        		nfui_nfbutton_set_image(NF_BUTTON(rec_obj[i]), rec_alarm_img);
			break;
			case 'M':
        		nfui_nfbutton_set_image(NF_BUTTON(rec_obj[i]), rec_mot_img);
			break;
			case 'P':
        		nfui_nfbutton_set_image(NF_BUTTON(rec_obj[i]), rec_panic_img);
			break;
			case ' ':
        		nfui_nfbutton_set_image(NF_BUTTON(rec_obj[i]), rec_no_img);
			break;
		}
    }

	return 0;
}

static gint _set_sensor_status(NF_NOTIFY_INFO *pInfo)
{
    gint i;

#ifndef GUI_32CH_SUPPORT
	for (i=0; i<CAM_ALARM_IN; i++)
	{
		if(pInfo->d.params[0] & (1 << i))
		    nfui_nfbutton_set_image(NF_BUTTON(cam_alarm_in_obj[i]), alarm_on_img);
		else
		    nfui_nfbutton_set_image(NF_BUTTON(cam_alarm_in_obj[i]), alarm_off_img);
	}

	for (i=0; i<g_dvr_alarmIn; i++)
	{
		if(pInfo->d.params[0] & (1 << (16 + i)))
		    nfui_nfbutton_set_image(NF_BUTTON(dvr_alarm_in_obj[i]), alarm_on_img);
		else
		    nfui_nfbutton_set_image(NF_BUTTON(dvr_alarm_in_obj[i]), alarm_off_img);
	}
#else
	for (i=0; i<CAM_ALARM_IN; i++)
	{
		if(pInfo->d.params[1] & (1 << i))
		    nfui_nfbutton_set_image(NF_BUTTON(cam_alarm_in_obj[i]), alarm_on_img);
		else
		    nfui_nfbutton_set_image(NF_BUTTON(cam_alarm_in_obj[i]), alarm_off_img);
	}

	for (i=0; i<g_dvr_alarmIn; i++)
	{
		if(pInfo->d.params[0] & (1 << i))
		    nfui_nfbutton_set_image(NF_BUTTON(dvr_alarm_in_obj[i]), alarm_on_img);
		else
		    nfui_nfbutton_set_image(NF_BUTTON(dvr_alarm_in_obj[i]), alarm_off_img);
	}
#endif

	return 0;
}

static gint _set_alarm_status(NF_NOTIFY_INFO *pInfo)
{
    gint i;

    for (i=0; i<CAM_ALARM_OUT; i++)
    {
    	if(pInfo->d.params[1] & (1 << i))
    	    nfui_nfbutton_set_image(NF_BUTTON(cam_alarm_out_obj[i]), alarm_on_img);
    	else
    	    nfui_nfbutton_set_image(NF_BUTTON(cam_alarm_out_obj[i]), alarm_off_img);
    }

    for (i=0; i<DVR_ALARM_OUT; i++)
    {
    	if(pInfo->d.params[0] & (1 << (i)))
    	    nfui_nfbutton_set_image(NF_BUTTON(dvr_alarm_out_obj[i]), alarm_on_img);
    	else
    	    nfui_nfbutton_set_image(NF_BUTTON(dvr_alarm_out_obj[i]), alarm_off_img);
    }

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

static gboolean post_panic_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

		scm_toggle_panic_record();
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_UP)
		{
			return TRUE;
		}
	}
	else if(event->type == INFY_PANIC_REC_STATUS)
    {
        if (((CMM_MESSAGE_T *)data)->param)
        	nfui_check_button_set_active((NFCHECKBUTTON*)obj, TRUE);
        else
        	nfui_check_button_set_active((NFCHECKBUTTON*)obj, FALSE);
    }
    else if (event->type == GDK_DELETE)
    {
        uxm_unreg_imsg_event(obj, INFY_PANIC_REC_STATUS);
    }

	return FALSE;
}

static gboolean post_full_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	gint x, y, ch;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        vsm_live_preview_start(1 << g_ch, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
        vsm_live_preview_stop();
        _live_ptz_main_hide();

        ch = VW_PTZ_Full_Open(g_curwnd, g_ch);

        _live_ptz_main_show(ch);
        _live_ptz_main_change_channel(ch, 0);
	}

	return FALSE;
}

static gboolean post_zoom_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	gint x, y, ch;

	if(event->type == GDK_BUTTON_RELEASE)
	{
		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

		x = DISPLAY_ACTIVE_WIDTH - VW_ZoomPIP_Width();
		y = DISPLAY_ACTIVE_HEIGHT - VW_ZoomPIP_Height();

        vsm_live_preview_start(1 << g_ch, 0, 0, DISPLAY_ACTIVE_WIDTH, DISPLAY_ACTIVE_HEIGHT);
        vsm_live_preview_stop();
        _live_ptz_main_hide();

		ch = VW_ZoomPIP_Open(g_curwnd, x, y, g_ch, ZOOM_PIP_NONE);

        _live_ptz_main_show(ch);
        _live_ptz_main_change_channel(ch, 0);
	}

	return FALSE;
}

static gboolean post_audio_out_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE)
	{
        gint x, y;

		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

		nfui_nfobject_get_offset(obj, &x, &y);

		if (VW_AudioOutput_IsShown())
            VW_AudioOutput_Hide();
		else
            VW_AudioOutput_Show(x, 590);
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_DOWN)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_rec_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE)
	{
        gint i;

		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            if (rec_obj[i] == obj)
                break;
        }

        if (g_ch != i) _live_ptz_main_change_channel(i, 1);
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_UP)
        {
			return TRUE;
        }
        else if (kpid == KEYPAD_DOWN)
		{
#if 1
            _change_obj_focus(obj, exit_obj);
#else
            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (rec_obj[i] == obj)
                    break;
            }

            if (i < CAM_ALARM_IN)
                _change_obj_focus(obj, cam_alarm_in_obj[i]);
            else
                _change_obj_focus(obj, cam_alarm_in_obj[CAM_ALARM_IN-1]);
#endif
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_cam_alarm_in_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint ch;

	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;


	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < CAM_ALARM_IN; i++)
        {
            if (cam_alarm_in_obj[i] == obj)
                break;
        }

        if (kpid == KEYPAD_UP)
        {
            if (i < GUI_CHANNEL_CNT)
                _change_obj_focus(obj, rec_obj[i]);
            else
                _change_obj_focus(obj, rec_obj[GUI_CHANNEL_CNT-1]);
			return TRUE;
		}
        else if (kpid == KEYPAD_DOWN)
		{
            if (i < g_dvr_alarmIn)
                _change_obj_focus(obj, dvr_alarm_in_obj[i]);
            else
                _change_obj_focus(obj, dvr_alarm_in_obj[g_dvr_alarmIn-1]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_dvr_alarm_in_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint ch;

	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;


	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < g_dvr_alarmIn; i++)
        {
            if (dvr_alarm_in_obj[i] == obj)
                break;
        }

        if (kpid == KEYPAD_UP)
        {
            if (i < CAM_ALARM_IN)
                _change_obj_focus(obj, cam_alarm_in_obj[i]);
            else
                _change_obj_focus(obj, cam_alarm_in_obj[CAM_ALARM_IN-1]);
			return TRUE;
		}
        else if (kpid == KEYPAD_DOWN)
		{
            if (i < CAM_ALARM_OUT)
                _change_obj_focus(obj, cam_alarm_out_obj[i]);
            else
                _change_obj_focus(obj, cam_alarm_out_obj[CAM_ALARM_OUT-1]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_cam_alarm_out_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE) {
        NF_NOTIFY_INFO info;
        gint i;

		if(event->button.button == MOUSE_RIGTH_BUTTON)
		    return FALSE;

        memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
        scm_get_sensor_event_data(&info);

        for (i = 0; i < CAM_ALARM_OUT; i++)
        {
            if (cam_alarm_out_obj[i] == obj)
                break;
        }

#if 0
        if (info.d.params[0] & (1 << i))
            nf_action_relay_test(FALSE, i, FALSE);
        else
            nf_action_relay_test(FALSE, i, TRUE);
#endif
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        for (i = 0; i < CAM_ALARM_OUT; i++)
        {
            if (cam_alarm_out_obj[i] == obj)
                break;
        }

        if (kpid == KEYPAD_UP)
        {
            if (i < g_dvr_alarmIn)
                _change_obj_focus(obj, dvr_alarm_in_obj[i]);
            else
                _change_obj_focus(obj, dvr_alarm_in_obj[g_dvr_alarmIn-1]);
			return TRUE;
		}
        else if (kpid == KEYPAD_DOWN)
		{
            if (i < DVR_ALARM_OUT)
                _change_obj_focus(obj, dvr_alarm_out_obj[i]);
            else
                _change_obj_focus(obj, dvr_alarm_out_obj[DVR_ALARM_OUT-1]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean _pre_status_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (event->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)1118, (gint)1025, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if (event->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG_NO_LINE, size_w, size_h);
	}

	return FALSE;
}

static gboolean post_dvr_alarm_out_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
	if(event->type == GDK_BUTTON_RELEASE) {
        NF_NOTIFY_INFO info;
        gint i;

		if(event->button.button == MOUSE_RIGTH_BUTTON)
		    return FALSE;

        memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
        scm_get_sensor_event_data(&info);

        for (i = 0; i < DVR_ALARM_OUT; i++)
        {
            if (dvr_alarm_out_obj[i] == obj)
                break;
        }

#if 0
        if (info.d.params[0] & (1 << (16 + i)))
            nf_action_relay_test(FALSE, (16 + i), FALSE);
        else
            nf_action_relay_test(FALSE, (16 + i), TRUE);
#endif
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_UP)
        {
            for (i = 0; i < DVR_ALARM_OUT; i++)
            {
                if (dvr_alarm_out_obj[i] == obj)
                    break;
            }

            if (i < CAM_ALARM_OUT)
                _change_obj_focus(obj, cam_alarm_out_obj[i]);
            else
                _change_obj_focus(obj, cam_alarm_out_obj[CAM_ALARM_OUT-1]);
			return TRUE;
		}
        else if (kpid == KEYPAD_DOWN)
		{
            _change_obj_focus(obj, exit_obj);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean post_ptz_combo_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint i , index;
    if(event->type == NFEVENT_COMBOBOX_CHANGED)
    {
        index =  nfui_combobox_get_cur_index((NFCOMBOBOX*)auxiliary_combo_obj);
    }
    return FALSE;
}
static gboolean post_start_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint index;
    if(event->type == GDK_BUTTON_RELEASE)
    {
         if(event->button.button == MOUSE_RIGTH_BUTTON)  return FALSE;

         index = nfui_combobox_get_cur_index((NFCOMBOBOX*)auxiliary_combo_obj);

         nf_ipcam_send_auxiliary_command(g_ch, AUXILIARY_CATEGORY_PTZ, index,0);

    }
    return FALSE;
}

static gboolean init_supported_func_combobox_list(guint ch)
{
    gint ret,i;

    NFIPCamAuxiliary info ;

    memset(&info, 0x00, sizeof(NFIPCamAuxiliary));

    nfui_combobox_remove_all((NFCOMBOBOX*)auxiliary_combo_obj);

    ret = nf_ipcam_get_auxiliary_commands(ch,AUXILIARY_CATEGORY_PTZ,&info, 0);

    if(!ret){

        for(i=0; i< info.size; i++)
        {
            nfui_combobox_append_data((NFCOMBOBOX*)auxiliary_combo_obj,info.commands[i]);
        }
    }

    nfui_signal_emit((NFOBJECT*)auxiliary_combo_obj, GDK_EXPOSE, TRUE);

    return FALSE;

}

static gboolean post_exit_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
   	NFOBJECT *topwin;

	if(event->type == GDK_BUTTON_RELEASE) {
		if(event->button.button == MOUSE_RIGTH_BUTTON)	return FALSE;

        _live_ptz_main_close();
	}
	else if(event->type == NFEVENT_KEYPAD_PRESS || event->type == NFEVENT_REMOCON_PRESS)
	{
		GdkEventKey *kevt;
		KEYPAD_KID kpid;
        gint i, enable_idx = -1;

		kevt = (GdkEventKey*)event;
		kpid = (KEYPAD_KID)kevt->keyval;

        if (kpid == KEYPAD_UP)
        {
            _change_obj_focus(obj, rec_obj[GUI_CHANNEL_CNT-1]);
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean _post_status_fixed_event_handler(NFOBJECT *obj, GdkEvent *event, gpointer data)
{
    gint i;
	NF_NOTIFY_INFO *pInfo = NULL;

	if (event->type == INFY_ANALOG_REC_NOTIFY)
	{
		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        _set_rec_status(pInfo);

		for (i = 0; i < GUI_CHANNEL_CNT; i++)
            nfui_signal_emit(rec_obj[i], GDK_EXPOSE, TRUE);
	}
	else if (event->type == INFY_SENSOR_NOTIFY)
	{
		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        _set_sensor_status(pInfo);

		for (i = 0; i < CAM_ALARM_IN; i++)
            nfui_signal_emit(cam_alarm_in_obj[i], GDK_EXPOSE, TRUE);

		for (i = 0; i < g_dvr_alarmIn; i++)
            nfui_signal_emit(dvr_alarm_in_obj[i], GDK_EXPOSE, TRUE);
	}
	else if (event->type == INFY_ALARM_NOTIFY)
	{
		pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

        _set_alarm_status(pInfo);

		for (i = 0; i < CAM_ALARM_OUT; i++)
            nfui_signal_emit(cam_alarm_out_obj[i], GDK_EXPOSE, TRUE);

		for (i = 0; i < DVR_ALARM_OUT; i++)
            nfui_signal_emit(dvr_alarm_out_obj[i], GDK_EXPOSE, TRUE);
	}
    else if (event->type == GDK_DELETE)
    {
    	uxm_unreg_imsg_event(obj, INFY_ANALOG_REC_NOTIFY);
    	uxm_unreg_imsg_event(obj, INFY_SENSOR_NOTIFY);
    	uxm_unreg_imsg_event(obj, INFY_ALARM_NOTIFY);
    }

	return FALSE;
}

gint _live_ptz_status_open(NFOBJECT *parent, NFOBJECT *status_fixed)
{
	NFOBJECT *tbl = NULL;
	NFOBJECT *obj = NULL;

	gint i, j;
    gint pos_x, pos_y;
    gint width, height;
    gint size_w, size_h;

	guint tbl_w[TBL_COL_CNT] = {0, };
    gchar strBuf[8];

    GdkPixbuf *panic_inact_img[4];
    GdkPixbuf *panic_act_img[4];
    GdkPixbuf *full_img[4];
    GdkPixbuf *zoom_img[4];
    GdkPixbuf *audio_img[4];

	const guint rec_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(419), COLOR_IDX(420), COLOR_IDX(421), COLOR_IDX(422)};
//	const guint off_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(424), COLOR_IDX(425), COLOR_IDX(426), COLOR_IDX(427)};
//	const guint on_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(428), COLOR_IDX(429), COLOR_IDX(430), COLOR_IDX(427)};
	const guint off_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(424), COLOR_IDX(424), COLOR_IDX(424), COLOR_IDX(424)};
	const guint on_font_color[NFOBJECT_STATE_COUNT] = {COLOR_IDX(428), COLOR_IDX(428), COLOR_IDX(428), COLOR_IDX(428)};

    NF_NOTIFY_INFO info;

    gboolean is_panic;

    g_curwnd = parent;

	panic_inact_img[0] = nfui_get_image_from_file((IMG_N_PTZ_PANIC_INACTIVE), NULL);
	panic_inact_img[1] = nfui_get_image_from_file((IMG_O_PTZ_PANIC_INACTIVE), NULL);
	panic_inact_img[2] = nfui_get_image_from_file((IMG_P_PTZ_PANIC_INACTIVE), NULL);
	panic_inact_img[3] = nfui_get_image_from_file((IMG_D_PTZ_PANIC_INACTIVE), NULL);

	panic_act_img[0] = nfui_get_image_from_file((IMG_N_PTZ_PANIC_ACTIVE), NULL);
	panic_act_img[1] = nfui_get_image_from_file((IMG_O_PTZ_PANIC_ACTIVE), NULL);
	panic_act_img[2] = nfui_get_image_from_file((IMG_P_PTZ_PANIC_ACTIVE), NULL);
	panic_act_img[3] = nfui_get_image_from_file((IMG_D_PTZ_PANIC_ACTIVE), NULL);

	full_img[0] = nfui_get_image_from_file((IMG_N_PTZ_FULL), NULL);
	full_img[1] = nfui_get_image_from_file((IMG_O_PTZ_FULL), NULL);
	full_img[2] = nfui_get_image_from_file((IMG_P_PTZ_FULL), NULL);
	full_img[3] = nfui_get_image_from_file((IMG_D_PTZ_FULL), NULL);

	zoom_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ZOOM), NULL);
	zoom_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ZOOM), NULL);
	zoom_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ZOOM), NULL);
	zoom_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ZOOM), NULL);

	audio_img[0] = nfui_get_image_from_file((IMG_N_PTZ_AUDIO), NULL);
	audio_img[1] = nfui_get_image_from_file((IMG_O_PTZ_AUDIO), NULL);
	audio_img[2] = nfui_get_image_from_file((IMG_P_PTZ_AUDIO), NULL);
	audio_img[3] = nfui_get_image_from_file((IMG_D_PTZ_AUDIO), NULL);

#if 1
	alarm_on_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_ON), NULL);
	alarm_on_img[1] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_ON), NULL);
	alarm_on_img[2] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_ON), NULL);
	alarm_on_img[3] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_ON), NULL);

	alarm_off_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_OFF), NULL);
	alarm_off_img[1] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_OFF), NULL);
	alarm_off_img[2] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_OFF), NULL);
	alarm_off_img[3] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_OFF), NULL);
#else
	alarm_on_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_ON), NULL);
	alarm_on_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ALARM_ON), NULL);
	alarm_on_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ALARM_ON), NULL);
	alarm_on_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ALARM_ON), NULL);

	alarm_off_img[0] = nfui_get_image_from_file((IMG_N_PTZ_ALARM_OFF), NULL);
	alarm_off_img[1] = nfui_get_image_from_file((IMG_O_PTZ_ALARM_OFF), NULL);
	alarm_off_img[2] = nfui_get_image_from_file((IMG_P_PTZ_ALARM_OFF), NULL);
	alarm_off_img[3] = nfui_get_image_from_file((IMG_D_PTZ_ALARM_OFF), NULL);
#endif

	rec_no_img[0] = nfui_get_image_from_file((IMG_N_PTZ_REC_NO), NULL);
	rec_no_img[1] = nfui_get_image_from_file((IMG_O_PTZ_REC_NO), NULL);
	rec_no_img[2] = nfui_get_image_from_file((IMG_P_PTZ_REC_NO), NULL);
	rec_no_img[3] = nfui_get_image_from_file((IMG_P_PTZ_REC_NO), NULL);

	rec_pre_img[0] = nfui_get_image_from_file((IMG_N_PTZ_REC_PRE), NULL);
	rec_pre_img[1] = nfui_get_image_from_file((IMG_O_PTZ_REC_PRE), NULL);
	rec_pre_img[2] = nfui_get_image_from_file((IMG_P_PTZ_REC_PRE), NULL);
	rec_pre_img[3] = nfui_get_image_from_file((IMG_P_PTZ_REC_PRE), NULL);

	rec_cont_img[0] = nfui_get_image_from_file((IMG_N_PTZ_REC_CONT), NULL);
	rec_cont_img[1] = nfui_get_image_from_file((IMG_O_PTZ_REC_CONT), NULL);
	rec_cont_img[2] = nfui_get_image_from_file((IMG_P_PTZ_REC_CONT), NULL);
	rec_cont_img[3] = nfui_get_image_from_file((IMG_P_PTZ_REC_CONT), NULL);

	rec_mot_img[0] = nfui_get_image_from_file((IMG_N_PTZ_REC_MOT), NULL);
	rec_mot_img[1] = nfui_get_image_from_file((IMG_O_PTZ_REC_MOT), NULL);
	rec_mot_img[2] = nfui_get_image_from_file((IMG_P_PTZ_REC_MOT), NULL);
	rec_mot_img[3] = nfui_get_image_from_file((IMG_P_PTZ_REC_MOT), NULL);

	rec_alarm_img[0] = nfui_get_image_from_file((IMG_N_PTZ_REC_ALARM), NULL);
	rec_alarm_img[1] = nfui_get_image_from_file((IMG_O_PTZ_REC_ALARM), NULL);
	rec_alarm_img[2] = nfui_get_image_from_file((IMG_P_PTZ_REC_ALARM), NULL);
	rec_alarm_img[3] = nfui_get_image_from_file((IMG_P_PTZ_REC_ALARM), NULL);

	rec_panic_img[0] = nfui_get_image_from_file((IMG_N_PTZ_REC_PANIC), NULL);
	rec_panic_img[1] = nfui_get_image_from_file((IMG_O_PTZ_REC_PANIC), NULL);
	rec_panic_img[2] = nfui_get_image_from_file((IMG_P_PTZ_REC_PANIC), NULL);
	rec_panic_img[3] = nfui_get_image_from_file((IMG_P_PTZ_REC_PANIC), NULL);

	g_dvr_alarmIn = var_get_dvr_alarmIn_cnt();

    pos_x = 22;
    pos_y = 12;

    nfui_get_pixbuf_size(panic_inact_img[0], &size_w, &size_h);

    is_panic = scm_get_panic_record();

	obj = (NFOBJECT*)nfui_checkbutton_new(is_panic);
	nfui_check_button_set_skin_type((NFCHECKBUTTON*)obj, NFCHECK_TYPE_DEF);
	nfui_check_button_set_inactive_image((NFCHECKBUTTON*)obj, panic_inact_img);
	nfui_check_button_set_active_image((NFCHECKBUTTON*)obj, panic_act_img);
	nfui_nfobject_set_size(obj, size_w, size_h);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_panic_event_handler);

	uxm_reg_imsg_event(obj, INFY_PANIC_REC_STATUS);
	uxm_monitor_on_imsg_event(obj, INFY_PANIC_REC_STATUS);

    pos_y += (size_h+5);

   	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), full_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_full_event_handler);

    pos_y += (size_h+5);

   	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), zoom_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_zoom_event_handler);

    pos_y += (size_h+5);

   	obj = (NFOBJECT*)nfui_nfbutton_new();
	nfui_nfbutton_set_image(NF_BUTTON(obj), audio_img);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x, pos_y);
    nfui_regi_post_event_callback(obj, post_audio_out_event_handler);


    for (i = 0; i < TBL_COL_CNT; i++)
    {
        if(i == 0)
        {
            tbl_w[i] = 229;
        }
        else
        {
            nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);
            tbl_w[i] = size_w;
        }
    }

    pos_x = 123;
    pos_y = 10;

	tbl = (NFOBJECT*)nfui_nftable_new(TBL_COL_CNT, TBL_ROW_CNT, TBL_COL_SPACE, TBL_ROW_SPACE, tbl_w, 32);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)status_fixed, tbl, pos_x, pos_y);

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("RECORD STATUS", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 0);

    nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", i+1);
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(rec_no_img, strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)rec_font_color);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_show(obj);
        nfui_regi_post_event_callback(obj, post_rec_event_handler);
    	rec_obj[i] = obj;

    	if (i < 16) nfui_nftable_attach((NFTABLE*)tbl, obj, i+1, 0);
    	else nfui_nftable_attach((NFTABLE*)tbl, obj, i+1-16, 1);
    }

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM IN(CAMERA)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
//    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 1);

    nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);

    for (i = 0; i < CAM_ALARM_IN; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", i+1);
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(alarm_off_img, strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)off_font_color);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
//    	nfui_nftable_attach((NFTABLE*)tbl, obj, i+1, 1);
        nfui_regi_post_event_callback(obj, post_cam_alarm_in_event_handler);
    	cam_alarm_in_obj[i] = obj;
    }

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM IN(NVR)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
//    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 2);

    nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);

    for (i = 0; i < g_dvr_alarmIn; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", i+1);
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(alarm_off_img, strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)off_font_color);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
//    	nfui_nftable_attach((NFTABLE*)tbl, obj, i+1, 2);
        nfui_regi_post_event_callback(obj, post_dvr_alarm_in_event_handler);
    	dvr_alarm_in_obj[i] = obj;
    }

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT(CAMERA)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
//    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 3);

    nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);

    for (i = 0; i < CAM_ALARM_OUT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", i+1);
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(alarm_off_img, strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)off_font_color);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
//    	nfui_nftable_attach((NFTABLE*)tbl, obj, i+1, 3);
    	cam_alarm_out_obj[i] = obj;
        nfui_regi_post_event_callback(obj, post_cam_alarm_out_event_handler);
    }

	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT(NVR)", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
//    nfui_nftable_attach((NFTABLE*)tbl, obj, 0, 4);

    nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);

    for (i = 0; i < DVR_ALARM_OUT; i++)
    {
        memset(strBuf, 0x00, sizeof(strBuf));
        g_sprintf(strBuf, "%d", i+1);
		obj = (NFOBJECT*)nfui_nfbutton_new_with_param(alarm_off_img, strBuf);
		nfui_nfbutton_set_pango_font((NFBUTTON*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), (guint*)off_font_color);
		nfui_nfobject_set_size(obj, (guint)size_w, (guint)size_h);
		nfui_nfobject_disable(obj);
		nfui_nfobject_show(obj);
//    	nfui_nftable_attach((NFTABLE*)tbl, obj, i+1, 4);
        nfui_regi_post_event_callback(obj, post_dvr_alarm_out_event_handler);
    	dvr_alarm_out_obj[i] = obj;
    }

    obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AUXILIARY FUNCTION", nffont_get_pango_font(NFFONT_SMALL_SEMI), COLOR_IDX(292));
    nfui_nfobject_set_size(obj, 200 ,40);
    nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
    nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)status_fixed, obj,pos_x, 243-12-40);

    nfui_get_pixbuf_size(rec_no_img[0], &size_w, &size_h);

    pos_x = 918-22-117;
    pos_y = 243-12-44;

    obj = nfui_combobox_new(NULL,0,0);
    nfui_combobox_set_skin_type((NFCOMBOBOX *)obj,NFCOMBOBOX_TYPE_POPUP_2);
    nfui_combobox_set_align((NFCOMBOBOX *)obj, NFALIGN_CENTER, 0);
    nfui_nfobject_set_size(obj, 260, 40);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x-420,pos_y+4);
    auxiliary_combo_obj = obj;

    obj = nftool_normal_button_create_popup_type1("RUN",100);
    nfui_nfobject_show(obj);
    nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x-150,pos_y+4);
    nfui_regi_post_event_callback(obj, post_start_event_handler);
    auxiliary_run_obj = obj;

	obj = nftool_normal_button_create_type1("EXIT", 117);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)status_fixed, obj, pos_x, pos_y);
	nfui_regi_post_event_callback(obj, post_exit_event_handler);
    exit_obj = obj;

	nfui_nfobject_set_data(status_fixed, "FOCUS_OBJ", (gpointer)obj);

    memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_analog_data(&info);
    _set_rec_status(&info);

    memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_sensor_event_data(&info);
    _set_sensor_status(&info);

    memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
    scm_get_alarm_event_data(&info);
    _set_alarm_status(&info);

	uxm_reg_imsg_event(status_fixed, INFY_ANALOG_REC_NOTIFY);
	uxm_reg_imsg_event(status_fixed, INFY_SENSOR_NOTIFY);
	uxm_reg_imsg_event(status_fixed, INFY_ALARM_NOTIFY);

	nfui_regi_post_event_callback(status_fixed, _post_status_fixed_event_handler);

    return 0;
}


gint _live_ptz_status_init_channel(guint channel)
{
    gint ret;
    g_ch = channel;

    _init_supported_func(g_ch);
    init_supported_func_combobox_list(g_ch);

    return 0;
}

gint _live_ptz_status_change_channel(guint channel, gint expose)
{
    g_ch = channel;

    _init_supported_func(g_ch);
    init_supported_func_combobox_list(g_ch);

    return 0;
}

