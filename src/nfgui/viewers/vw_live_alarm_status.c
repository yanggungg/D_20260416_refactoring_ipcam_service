
#include "nf_afx.h"
#include "iux_msg.h"

#include "scm.h"

#include "support/event_loop.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_image.h"
#include "support/color.h"
#include "support/util.h"
#include "support/nf_ui_page_manager.h"

#include "vw.h"
#include "viewers/objects/nfobject.h"
#include "viewers/objects/nfwindow.h"
#include "viewers/objects/nffixed.h"
#include "viewers/objects/nflabel.h"
#include "viewers/objects/nfbutton.h"
#include "viewers/objects/nfimage.h"

#include "vw_live_alarm_status.h"
#include "ssm.h"
#include "nf_ui_tool.h"
#include "nf_api_dlva.h"


#define ALARM_STATUS_SIZE_W					(934)
#define ALARM_STATUS_SIZE_H					(520)

enum {
	ALARM_OFF = 0,
	ALARM_ON,
	ALARM_STATUS_CNT
};

enum {
	SYSTEM_STATUS_MSG = 0,
	DISK_STATUS_MSG,
	NETWORK_STATUS_MSG,
	EVT_STATUS_MSG_CNT,
};


static NFWINDOW *g_curwnd = 0;
static NFOBJECT *g_asWin = NULL;

static NFOBJECT *g_aicObj[CAM_ALARM_IN];
static NFOBJECT *g_aidObj[DVR_ALARM_IN];
static NFOBJECT *g_aocObj[CAM_ALARM_OUT];
static NFOBJECT *g_aodObj[DVR_ALARM_OUT];

static NFOBJECT *g_motObj[GUI_CHANNEL_CNT];
static NFOBJECT *g_vlossObj[GUI_CHANNEL_CNT];
static NFOBJECT *g_aitroubleObj[GUI_CHANNEL_CNT];

static NFOBJECT *g_msgObj[EVT_STATUS_MSG_CNT];

static gint g_dvr_alarmIn= DVR_ALARM_IN;

static void init_alarm_in_status()
{
	NF_NOTIFY_INFO info;
	gint i;

	if(!scm_get_sensor_event_data(&info)) {
#ifndef GUI_32CH_SUPPORT
		for(i=0; i<CAM_ALARM_IN; i++) {
			if(info.d.params[0] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(393));
			} else {
				nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(394));
			}
		}

		for(i=0; i<g_dvr_alarmIn; i++) {
			if(info.d.params[0] & (1 << (16 + i))) {
				nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(393));
			}else {
				nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(394));
			}
		}
#else
		for(i=0; i<CAM_ALARM_IN; i++) {
			if(info.d.params[1] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(393));
			} else {
				nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(394));
			}
		}

		for(i=0; i<g_dvr_alarmIn; i++) {
			if(info.d.params[0] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(393));
			}else {
				nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(394));
			}
		}
#endif
	}
}


static void init_alarm_out_status()
{
	NF_NOTIFY_INFO info;
	gint i;

	if(!scm_get_alarm_event_data(&info)) {
		for(i=0; i<CAM_ALARM_OUT; i++) {
			if(info.d.params[1] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_aocObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aocObj[i], COLOR_IDX(393));
			}else {
				nfui_nfimage_change_image((NFIMAGE*)g_aocObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aocObj[i], COLOR_IDX(394));
			}
		}

		for(i=0; i<DVR_ALARM_OUT; i++) {
			if(info.d.params[0] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_aodObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aodObj[i], COLOR_IDX(393));
			}else {
				nfui_nfimage_change_image((NFIMAGE*)g_aodObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_aodObj[i], COLOR_IDX(394));
			}
		}
	}
}


static void init_motion_status()
{
	NF_NOTIFY_INFO info;
	gint i;

	if(!scm_get_motion_event_data(&info)) {
		for(i=0; i<GUI_CHANNEL_CNT; i++) {
			if(info.d.params[0] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_motObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_motObj[i], COLOR_IDX(393));
			}else {
				nfui_nfimage_change_image((NFIMAGE*)g_motObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_motObj[i], COLOR_IDX(394));
			}
		}
	}
}

static void init_vloss_status()
{
	NF_NOTIFY_INFO info;
	gint i;

	if(!scm_get_vloss_event_data(&info)) {
		for(i=0; i<GUI_CHANNEL_CNT; i++) {
			if(info.d.params[0] & (1 << i)) {
				nfui_nfimage_change_image((NFIMAGE*)g_vlossObj[i], IMG_CH_ALARM_ON);
				nfui_nfimage_set_font_color((NFIMAGE*)g_vlossObj[i], COLOR_IDX(393));
			}else {
				nfui_nfimage_change_image((NFIMAGE*)g_vlossObj[i], IMG_CH_ALARM_OFF);
				nfui_nfimage_set_font_color((NFIMAGE*)g_vlossObj[i], COLOR_IDX(394));
			}
		}
	}
}

static void init_system_status()
{
	NF_NOTIFY_INFO info;
	gchar buf[128];
	const gchar *msg[] = {"FAN ERROR.", "TEMPERATURE ERROR.", "POE ERROR."};
	gchar *ep = NULL;
    EA_SysSysData ssd;
   	SysManageData sdata;

    gint tot_poe;

    memset(buf, 0x00, sizeof(buf));

	if(!scm_get_sysfan_event_data(&info)) {
		if(info.d.params[0])
			ep = g_stpcpy(buf, msg[0]);
	}

	if(!scm_get_termperature_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep) 	g_sprintf(ep, "%c", 0x20);

			if(ep) 	ep = g_stpcpy(++ep, msg[1]);
			else 	ep = g_stpcpy(buf, msg[1]);
		}
	}

	if(!scm_get_poe_event_data(&info)) {
        DAL_get_SysSys_Data(&ssd, POE_FAIL_EVT_DATA);
    	DAL_get_sysManage_data(&sdata);
        tot_poe = info.d.params[3]/1000;
		if (sdata.poeLimt)
		{
        if ((tot_poe*100/sdata.poeLimt) >= ssd.failPoe)
        {
			if(ep)	g_sprintf(ep, "%c", 0x20);

			if(ep) 	ep = g_stpcpy(++ep, msg[2]);
			else 	ep = g_stpcpy(buf, msg[2]);

		}
        }
	}

	nfui_nflabel_set_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG], buf);
}

static void init_disk_status()
{
	NF_NOTIFY_INFO info;
	const gchar *msg[] = {"WRITE FAIL.", "DISK EXHAUST.", "NO DISK.", "S.M.A.R.T. ERROR.", "DISK FULL.", "S.M.A.R.T. WARN."};
	gchar buf[128];
	gchar *ep = NULL;

	memset(buf, 0x00, sizeof(buf));

	if(!scm_get_writefail_event_data(&info)) {
		if(info.d.params[0])
			ep = g_stpcpy(buf, lookup_string(msg[0]));
	}

	if(!scm_get_exhaust_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep) g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[1]));
			else   ep = g_stpcpy(buf, lookup_string(msg[1]));
		}
	}

	if(!scm_get_nodisk_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep) g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[2]));
			else   ep = g_stpcpy(buf, lookup_string(msg[2]));
		}
	}

	if(!scm_get_smart_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep) g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[3]));
			else   ep = g_stpcpy(buf, lookup_string(msg[3]));
		}
	}

	if(!scm_get_disk_full_data(&info)) {
		if(info.d.params[0]) {
			if(ep) g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[4]));
			else   ep = g_stpcpy(buf, lookup_string(msg[4]));
		}
	}

	if(!scm_get_smart_reqchk_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep) g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[5]));
			else   ep = g_stpcpy(buf, lookup_string(msg[5]));
		}
	}

	nfui_nflabel_set_text((NFLABEL*)g_msgObj[DISK_STATUS_MSG], buf);
}

static void init_net_status()
{
	NF_NOTIFY_INFO info;
	const gchar *msg[] = {"NET LOGIN FAIL.", "WAN FAIL.", "DDNS FAIL.","IP CONFLICT."};
	gchar buf[128];
	gchar *ep = NULL;

	memset(buf, 0x00, sizeof(buf));

	if(!scm_get_netloginfail_event_data(&info)) {
		if(info.d.params[0])
			ep = g_stpcpy(buf, lookup_string(msg[0]));
	}

	if(!scm_get_wan_status_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep)	g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[1]));
			else   ep = g_stpcpy(buf, lookup_string(msg[1]));
		}
	}

	if(!scm_get_ddns_status_event_data(&info)) {
		if(info.d.params[0]) {
			if(ep)	g_sprintf(ep, "%c", 0x20);

			if(ep) ep = g_stpcpy(++ep, lookup_string(msg[2]));
			else   ep = g_stpcpy(buf, lookup_string(msg[2]));
		}
	}

    if(!scm_get_net_set_ipconflict_event_data(&info)|| !scm_get_net_cam_ipconflict_event_data(&info))
    {
        if(info.d.params[0]){
            if(ep) g_sprintf(ep,"%c", 0x20);

            if(ep) ep = g_stpcpy(++ep, lookup_string(msg[3]));
            else   ep = g_stpcpy(buf, lookup_string(msg[3]));
	    }
    }

	nfui_nflabel_set_text((NFLABEL*)g_msgObj[NETWORK_STATUS_MSG], buf);
}

static void init_aibox_status()
{
	gint i;
	guint aibox_state;

	if (!ivsc.dfunc.support_aibox_itx) return;

	for (i = 0; i < GUI_CHANNEL_CNT; i++) 
	{
		aibox_state = nf_api_get_aibox_connection_status(i);

		if ((aibox_state == NF_AIBOX_CONN_FAILED) || (aibox_state == NF_AIBOX_STREAM_CONN_FAILED)) {
			nfui_nfimage_change_image((NFIMAGE*)g_aitroubleObj[i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)g_aitroubleObj[i], COLOR_IDX(393));
		}
		else {
			nfui_nfimage_change_image((NFIMAGE*)g_aitroubleObj[i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)g_aitroubleObj[i], COLOR_IDX(394));
		}
	}
}

static void init_status()
{
	init_alarm_in_status();
	init_alarm_out_status();
	init_motion_status();
	init_vloss_status();
	init_system_status();
	init_disk_status();
	init_net_status();
	init_aibox_status();
}

static gboolean post_as_window_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	switch(evt->type) {
		case NFOUTEVT_BUTTON_PRESS:
			nfui_nfobject_hide(obj);
			nfui_page_close(PGID_LIVE_ALARM_STATUS, obj);
			break;

		case NFEVENT_KEYPAD_PRESS:
		case NFEVENT_REMOCON_PRESS:
			{
				GdkEventKey *kevt;
				KEYPAD_KID kpid;

				kevt = (GdkEventKey*)evt;
				kpid = (KEYPAD_KID)kevt->keyval;

				if(kpid == KEYPAD_EXIT) {
					nfui_nfobject_hide(obj);
					nfui_page_close(PGID_LIVE_ALARM_STATUS, obj);

					return TRUE;
				}
			}
			break;
		case GDK_DELETE:
			g_curwnd = 0;
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean returnkey_proc(NFOBJECT *top, GdkEvent *event, gpointer data)
{
	return FALSE;
}

static gboolean post_as_fixed_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc;
    GdkPixbuf *pbuf = NULL;
    gint size_w, size_h;

	if (evt->type == GDK_EXPOSE)
	{
		drawable = nfui_nfobject_get_window(obj);
		gc = nfui_nfobject_get_gc(obj);

        nfui_nfobject_get_size(obj, &size_w, &size_h);
        pbuf = nfui_get_popup_pixbuf_composite(MK_IMG_POPUP_BG, size_w, size_h);
        nfutil_draw_pixbuf(drawable, gc, pbuf, (gint)obj->x, (gint)obj->y, -1, -1, NFALIGN_LEFT, 0);

		nfui_nfobject_gc_unref(gc);
	}
	else if (evt->type == GDK_DELETE)
	{
        nfui_nfobject_get_size(obj, &size_w, &size_h);
        nfui_unref_popup_pixbuf(MK_IMG_POPUP_BG, size_w, size_h);
        uxm_unreg_imsg_event(obj, IREQ_CHANGE_LANG);
	}
	else if (evt->type == IREQ_CHANGE_LANG)
	{
        init_system_status();
    	init_disk_status();
    	init_net_status();
    }

	return FALSE;
}

static gboolean post_aitrouble_popup_btn_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if (evt->type == GDK_BUTTON_RELEASE)
	{
		if (evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		vw_dvabox_keep_alive_popup_open(g_curwnd);
	}

	return FALSE;
}

static gboolean post_ok_button_event_cb(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE) {
		if(evt->button.button == MOUSE_RIGTH_BUTTON) {
			return FALSE;
		}

		nfui_nfobject_hide(g_asWin);

		nfui_page_close(PGID_LIVE_ALARM_STATUS, g_asWin);
	}

	return FALSE;
}


gboolean VW_Create_AlarmStatus(NFWINDOW *parent, gint x, gint y)
{
	NFOBJECT *asFixed = NULL;
	NFOBJECT *obj = NULL;

	GdkPixbuf *detail_img[NFOBJECT_STATE_COUNT];

	gchar *strTitle[] = {"ALARM IN(CAM)",
						"ALARM IN(NVR)",
						"MOTION EVENT",
						"VIDEO LOSS EVENT",
						"SYSTEM EVENT STATUS",
						"DISK EVENT STATUS",
						"NETWORK EVENT STATUS",
						"ALARM OUT(CAM)",
						"ALARM OUT(NVR)"};
	gchar strBuf[8];
	gint i, j;

	guint win_size_h = ALARM_STATUS_SIZE_H;
	gint pos_x, pos_y;
	

    if (GUI_CHANNEL_CNT > 16) {
        win_size_h += 80;
    }
    
    if (CAM_ALARM_IN > 16) {
        win_size_h += 40;
    }

    if (DVR_ALARM_IN > 16) {
        win_size_h += 40;
    }

    if (CAM_ALARM_OUT > 16) {
        win_size_h += 40;
    }

    if (DVR_ALARM_OUT > 16) {
        win_size_h += 40;
    }
    
	if (ivsc.dfunc.support_aibox_itx) {
		win_size_h += 80;
	}

	y = 972-win_size_h-10;

	/* window */
	g_asWin = (NFOBJECT*)nfui_nfwindow_new(parent, x, y, ALARM_STATUS_SIZE_W, win_size_h);
	g_curwnd = g_asWin;
	nfui_regi_post_event_callback(g_asWin, post_as_window_event_cb);
	nfui_nfwindow_use_outside_evt((NFWINDOW*)g_asWin, TRUE);
	nfui_nfwindow_set_mask((NFWINDOW*)g_asWin, GDK_BUTTON_PRESS, TRUE);
	nfui_nfwindow_set_returnkey_proc((NFWINDOW*)g_asWin, returnkey_proc);

	gtk_widget_add_events(((NFWINDOW*)g_asWin)->main_widget, GDK_POINTER_MOTION_HINT_MASK);

	/* fixed */
	asFixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_set_size(asFixed, ALARM_STATUS_SIZE_W, win_size_h);
	nfui_regi_post_event_callback(asFixed, post_as_fixed_event_cb);
	nfui_nfobject_show(asFixed);

	/* title */
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM STATUS", nffont_get_pango_font(NFFONT_LARGE_SEMI), COLOR_IDX(205));
	nfui_nfobject_set_size(obj, 330, 36);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 23);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, 4, 4);

	pos_x = 23;
	pos_y = 63;

	/* ALARM IN (CAM) */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM IN(CAM)", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM IN(CAM)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

    pos_x += 275 + 38;
    
	for (i = 0; i < CAM_ALARM_IN; i++)
	{
		if (i == 16) {
		    pos_x = 23 + 275 + 38;
		    pos_y += 40;
	    }
		
		g_sprintf(strBuf, "%d", i + 1);

		obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

		g_aicObj[i] = obj;
		pos_x += 32 + 2;
	}

    pos_x = 23;
    pos_y += 40;

	g_dvr_alarmIn = var_get_dvr_alarmIn_cnt();

	/* ALARM IN (NVR) */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM IN(NVR)", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM IN(NVR)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;

	for (i = 0; i < g_dvr_alarmIn; i++)
	{
		if (i == 16) {
		    pos_x = 23 + 275 + 38;
		    pos_y += 40;
	    }
		
		g_sprintf(strBuf, "%d", i + 1);

		obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

		g_aidObj[i] = obj;
		pos_x += 32 + 2;
	}

	pos_x = 23;
	pos_y += 40;

	/* MOTION */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOTION EVENT", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MOTION EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (i == 16) {
		    pos_x = 23 + 275 + 38;
		    pos_y += 40;
	    }
		
		g_sprintf(strBuf, "%d", i + 1);

		obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

		g_motObj[i] = obj;
		pos_x += 32 + 2;
	}

	pos_x = 23;
	pos_y += 40;

	/* VIDEO LOSS */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO LOSS EVENT", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("VIDEO LOSS EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (i == 16) {
		    pos_x = 23 + 275 + 38;
		    pos_y += 40;
	    }
		
		g_sprintf(strBuf, "%d", i + 1);

		obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

		g_vlossObj[i] = obj;
		pos_x += 32 + 2;
	}
	
	pos_x = 23;
	pos_y += 40;

	if (ivsc.dfunc.support_aibox_itx) 
	{
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("AI BOX FAILURE EVENT", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
		nfui_nfobject_set_size(obj, 275, 32);
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
        pos_x += 275 + 38;
		
		for(i = 0; i < GUI_CHANNEL_CNT; i++) 
		{
			if (i == 16) {
				pos_x = 23 + 275 + 38;
				pos_y += 40;
			}
			
			g_sprintf(strBuf, "%d", i + 1);
			obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
			nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
			nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
			nfui_nfobject_show(obj);
			nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
			g_aitroubleObj[i] = obj;
			
			pos_x += 32 + 2;
		}

		detail_img[0] = nfui_get_image_from_file(("bt_pop_search_n.png"), NULL);
		detail_img[1] = nfui_get_image_from_file(("bt_pop_search_o.png"), NULL);
		detail_img[2] = nfui_get_image_from_file(("bt_pop_search_p.png"), NULL);
		detail_img[3] = nfui_get_image_from_file(("bt_pop_search_d.png"), NULL);

		obj = (NFOBJECT*)nfui_nfbutton_new();
		nfui_nfbutton_set_image(NF_BUTTON(obj), detail_img);
		nfui_nfobject_set_size(obj, 42, 40);
		nfui_nfobject_show(obj);
		nfui_nffixed_put((NFFIXED*)asFixed, obj, 313 + ((32 + 2) * GUI_CHANNEL_CNT) + 8, (63 + (40 * 9) - 4));
		nfui_regi_post_event_callback(obj, post_aitrouble_popup_btn_event_handler); 

    	pos_x = 23;
    	pos_y += 40;
	}
	
	/* SYSTEM */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYSTEM EVENT STATUS", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("SYSTEM EVENT STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(395));
	nfui_nfobject_set_size(obj, 558, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
	
	g_msgObj[SYSTEM_STATUS_MSG] = obj;

	pos_x = 23;
	pos_y += 40;

	/* DISK */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK EVENT STATUS", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("DISK EVENT STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(395));
	nfui_nfobject_set_size(obj, 558, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
	
	g_msgObj[DISK_STATUS_MSG] = obj;

	pos_x = 23;
	pos_y += 40;

	/* NETWORK */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NETWORK EVENT STATUS", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("NETWORK EVENT STATUS", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;
    
	obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(395));
	nfui_nfobject_set_size(obj, 558, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 2);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);

	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
	
	g_msgObj[NETWORK_STATUS_MSG] = obj;

	pos_x = 23;
	pos_y += 40;
	
	/* ALARM OUT (CAM) */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT(CAM)", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT(CAM)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

    pos_x += 275 + 38;
    
	for (i = 0; i < CAM_ALARM_OUT; i++)
	{
		if (i == 16) {
		    pos_x = 23 + 275 + 38;
		    pos_y += 40;
	    }
		
		g_sprintf(strBuf, "%d", i + 1);

		obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

		g_aocObj[i] = obj;
		pos_x += 32 + 2;
	}

    pos_x = 23;
    pos_y += 40;

	/* ALARM OUT (NVR) */
	if(nftool_cur_language_is_japanese())
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT(NVR)", nffont_get_pango_font(NFFONT_SMALL_SEMI_1), COLOR_IDX(206));
	else
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font("ALARM OUT(NVR)", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(206));
	nfui_nfobject_set_size(obj, 275, 32);
	nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(200));
	nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);
    
    pos_x += 275 + 38;

	for (i = 0; i < DVR_ALARM_OUT; i++)
	{
		if (i == 16) {
		    pos_x = 23 + 275 + 38;
		    pos_y += 40;
	    }
		
		g_sprintf(strBuf, "%d", i + 1);

		obj = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)obj, strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(obj);

		nfui_nffixed_put((NFFIXED*)asFixed, obj, pos_x, pos_y);

		g_aodObj[i] = obj;
		pos_x += 32 + 2;
	}

	/* button */
	obj = nftool_normal_button_create_type1("OK", 192);
	nfui_regi_post_event_callback(obj, post_ok_button_event_cb);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)asFixed, obj, 351, win_size_h-60);

	uxm_reg_imsg_event(asFixed, IREQ_CHANGE_LANG);
	uxm_monitor_on_imsg_event(asFixed, IREQ_CHANGE_LANG);
	nfui_nfwindow_add((NFWINDOW*)g_asWin, asFixed);
	nfui_run_main_event_handler(g_asWin);
	nfui_nfobject_hide(g_asWin);


	init_status();

	return TRUE;
}

int VW_Destroy_AlarmStatus()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_destroy(g_curwnd);
	return 0;
}

void VW_AlarmStatus_Show()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_show(g_asWin);
	nfui_make_key_hierarchy((NFWINDOW*)g_asWin);

	nfui_page_open(PGID_LIVE_ALARM_STATUS, g_asWin, ssm_get_cur_id(NULL));
}

void VW_AlarmStatus_Hide()
{
	if (!g_curwnd) return 0;
	nfui_nfobject_hide(g_asWin);

	nfui_page_close(PGID_LIVE_ALARM_STATUS, g_asWin);
}

gboolean VW_AlarmStatus_IsShown()
{
	if (!g_curwnd) return 0;
	return nfui_nfobject_is_shown(g_asWin);
}

void VW_AlarmStatus_Set_Noti(guint evt_type, NF_NOTIFY_INFO *info)
{
	gchar *str;
	gchar *ep = NULL;
	gchar buf[128];
	gboolean expose = FALSE;
	gboolean changed = FALSE;
	gint idx = 0;
	gint i;

	g_return_if_fail(info != NULL);
	if (!g_curwnd) return 0;

	if(nfui_nfobject_is_shown(g_asWin))
		expose = TRUE;

	switch(evt_type) {
		case INFY_SENSOR_NOTIFY:
			{
				// set image
#ifndef GUI_32CH_SUPPORT
				for(i=0; i<CAM_ALARM_IN; i++) {
					if(info->d.params[0] & (1 << i)) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_aicObj[i], GDK_EXPOSE, FALSE);
				}

				for(i=0; i<g_dvr_alarmIn; i++) {
					if(info->d.params[0] & (1 << (16 + i))) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_aidObj[i], GDK_EXPOSE, FALSE);
				}
#else
				for(i=0; i<g_dvr_alarmIn; i++) {
					if(info->d.params[0] & (1 << i)) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aidObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aidObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_aidObj[i], GDK_EXPOSE, FALSE);
				}
				
				for(i=0; i<CAM_ALARM_IN; i++) {
					if(info->d.params[1] & (1 << i)) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aicObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aicObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_aicObj[i], GDK_EXPOSE, FALSE);
				}
#endif
			}
			break;

		case INFY_ALARM_NOTIFY:
			{
				// set image
				for(i=0; i<CAM_ALARM_OUT; i++) {
					if(info->d.params[1] & (1 << i)) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aocObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aocObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aocObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aocObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_aocObj[i], GDK_EXPOSE, FALSE);
				}

				for(i=0; i<DVR_ALARM_OUT; i++) {
					if(info->d.params[0] & (1 << (i))) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aodObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aodObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_aodObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_aodObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_aodObj[i], GDK_EXPOSE, FALSE);
				}
			}
			break;

		case INFY_MOTION_NOTIFY:
			{
				// set image
				for(i=0; i<GUI_CHANNEL_CNT; i++) {
					if(info->d.params[0] & (1 << i)) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_motObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_motObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_motObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_motObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_motObj[i], GDK_EXPOSE, FALSE);
				}
			}
			break;

		case INFY_VLOSS_NOTIFY:
			{
				// set image
				for(i=0; i<GUI_CHANNEL_CNT; i++) {
					if(info->d.params[0] & (1 << i)) {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_vlossObj[i], IMG_CH_ALARM_ON);
						nfui_nfimage_set_font_color((NFIMAGE*)g_vlossObj[i], COLOR_IDX(393));
					}else {
						changed = nfui_nfimage_change_image((NFIMAGE*)g_vlossObj[i], IMG_CH_ALARM_OFF);
						nfui_nfimage_set_font_color((NFIMAGE*)g_vlossObj[i], COLOR_IDX(394));
					}
					if(expose && changed) nfui_signal_emit(g_vlossObj[i], GDK_EXPOSE, FALSE);
				}
			}
			break;

		case INFY_SYSFAN_NOTIFY:
		case INFY_TEMPERATURE_NOTIFY:
		case INFY_POE_NOTIFY:
		case INFY_POE_HUB_NOTIFY:
			{
				gchar *msg[] = {"FAN ERROR.", "TEMPERATURE ERROR.", "POE ERROR."};
                EA_SysSysData ssd;
               	SysManageData sdata;
                gint tot_poe;

				memset(buf, 0x00, sizeof(buf));
				str = nfui_nflabel_get_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG]);

				if(evt_type == INFY_SYSFAN_NOTIFY) 				idx = 0;
				else if(evt_type == INFY_TEMPERATURE_NOTIFY) 	idx = 1;
				else if(evt_type == INFY_POE_NOTIFY) 			idx = 2;
				else if(evt_type == INFY_POE_HUB_NOTIFY) 		idx = 2;
				else break;

				if(info->d.params[0] != 0) {
#if 0
                    if (evt_type == INFY_POE_NOTIFY)
                    {
                        DAL_get_SysSys_Data(&ssd, POE_FAIL_EVT_DATA);
                    	DAL_get_sysManage_data(&sdata);
                        tot_poe = info->d.params[3]/1000;

                        if ((tot_poe*100/sdata.poeLimt) >= ssd.failPoe)
                        {
        					if(!g_strrstr(str, msg[idx])) {
        						if(strlen(str)) g_sprintf(buf, "%s %s", str, msg[idx]);
        						else 			g_sprintf(buf, "%s", msg[idx]);

        						nfui_nflabel_set_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG], buf);
        						if(expose) nfui_signal_emit(g_msgObj[SYSTEM_STATUS_MSG], GDK_EXPOSE, FALSE);
        					}
                        }
                    }
                    else if (evt_type == INFY_POE_HUB_NOTIFY)
                    {
                        DAL_get_SysSys_Data(&ssd, POE_FAIL_EVT_DATA);
                    	DAL_get_sysManage_data(&sdata);
                        tot_poe = info->d.params[3]/1000;

                        if ((tot_poe*100/sdata.poeHubLimt) >= ssd.failPoe)
                        {
        					if(!g_strrstr(str, msg[idx])) {
        						if(strlen(str)) g_sprintf(buf, "%s %s", str, msg[idx]);
        						else 			g_sprintf(buf, "%s", msg[idx]);

        						nfui_nflabel_set_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG], buf);
        						if(expose) nfui_signal_emit(g_msgObj[SYSTEM_STATUS_MSG], GDK_EXPOSE, FALSE);
        					}
                        }
                    }
                    else
                    {
    					if(!g_strrstr(str, msg[idx])) {
    						if(strlen(str)) g_sprintf(buf, "%s %s", str, msg[idx]);
    						else 			g_sprintf(buf, "%s", msg[idx]);

    						nfui_nflabel_set_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG], buf);
    						if(expose) nfui_signal_emit(g_msgObj[SYSTEM_STATUS_MSG], GDK_EXPOSE, FALSE);
    					}
                    }
#else
					if(!g_strrstr(str, msg[idx])) {
						if(strlen(str)) g_sprintf(buf, "%s %s", str, msg[idx]);
						else 			g_sprintf(buf, "%s", msg[idx]);

						nfui_nflabel_set_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG], buf);
						if(expose) nfui_signal_emit(g_msgObj[SYSTEM_STATUS_MSG], GDK_EXPOSE, FALSE);
					}
#endif
				}else if(info->d.params[0] == 0) {
					for(i=0; i<3; i++) {
						if(i == idx) continue;

						if(g_strrstr(str, msg[i])) {
							if(ep)	g_sprintf(ep, "%c", 0x20);

							if(!ep) ep = g_stpcpy(buf, msg[i]);
							else 	ep = g_stpcpy(++ep, msg[i]);
						}
					}

					nfui_nflabel_set_text((NFLABEL*)g_msgObj[SYSTEM_STATUS_MSG], buf);
					if(expose) nfui_signal_emit(g_msgObj[SYSTEM_STATUS_MSG], GDK_EXPOSE, FALSE);
				}
			}
			break;

		case INFY_WRITEFAIL_NOTIFY:
		case INFY_EXHAUST_NOTIFY:
		case INFY_NODISK_NOTIFY:
		case INFY_SMART_WARN_NOTIFY:
		case INFY_SMART_ERROR_NOTIFY:
		case INFY_DISK_FULL_NOTIFY:
			{
				gchar *msg[] = {"WRITE FAIL.", "DISK EXHAUST.", "NO DISK.", "S.M.A.R.T. ERROR.", "DISK FULL.", "S.M.A.R.T. WARN."};

				memset(buf, 0x00, sizeof(buf));
				str = nfui_nflabel_get_text((NFLABEL*)g_msgObj[DISK_STATUS_MSG]);

				if(evt_type == INFY_WRITEFAIL_NOTIFY) 		break;
				else if(evt_type == INFY_EXHAUST_NOTIFY) 	idx = 1;
				else if(evt_type == INFY_NODISK_NOTIFY)		idx = 2;
				else if(evt_type == INFY_SMART_ERROR_NOTIFY)	idx = 3;
				else if(evt_type == INFY_DISK_FULL_NOTIFY) 	idx = 4;
				else if(evt_type == INFY_SMART_WARN_NOTIFY)		idx = 5;
				else break;


				if(info->d.params[0] != 0) {
					if(!g_strrstr(str, lookup_string(msg[idx]))) {
						if(strlen(str)) g_sprintf(buf, "%s %s", str, lookup_string(msg[idx]));
						else 			g_sprintf(buf, "%s", lookup_string(msg[idx]));

						nfui_nflabel_set_text((NFLABEL*)g_msgObj[DISK_STATUS_MSG], buf);
						if(expose) nfui_signal_emit(g_msgObj[DISK_STATUS_MSG], GDK_EXPOSE, FALSE);
					}
				}else if(info->d.params[0] == 0) {
					for(i=0; i<6; i++) {
						if(i == idx) continue;

						if(g_strrstr(str, lookup_string(msg[i]))) {
							if(ep)	g_sprintf(ep, "%c", 0x20);

							if(!ep) ep = g_stpcpy(buf, lookup_string(msg[i]));
							else 	ep = g_stpcpy(++ep, lookup_string(msg[i]));
						}
					}

					nfui_nflabel_set_text((NFLABEL*)g_msgObj[DISK_STATUS_MSG], buf);
					if(expose) nfui_signal_emit(g_msgObj[DISK_STATUS_MSG], GDK_EXPOSE, FALSE);
				}
			}
			break;

		case INFY_WAN_NOTIFY:
		case INFY_DDNS_NOTIFY:
		case INFY_NETLOGINFAIL_NOTIFY:
		case INFY_SET_IP_CONFLICT_NOTIFY:
		case INFY_CAM_IP_CONFLICT_NOTIFY:
			{
				gchar *msg[] = {"NET LOGIN FAIL.", "WAN FAIL.", "DDNS FAIL.", "IP CONFLICT."};

				memset(buf, 0x00, sizeof(buf));
				str = nfui_nflabel_get_text((NFLABEL*)g_msgObj[NETWORK_STATUS_MSG]);

				if(evt_type == INFY_NETLOGINFAIL_NOTIFY)	idx = 0;
				else if(evt_type == INFY_WAN_NOTIFY) 		idx = 1;
				else if(evt_type == INFY_DDNS_NOTIFY)		idx = 2;
				else if(evt_type == INFY_SET_IP_CONFLICT_NOTIFY || evt_type == INFY_CAM_IP_CONFLICT_NOTIFY) idx = 3;
				else break;

				if(info->d.params[0] != 0) {
					if(!g_strrstr(str, lookup_string(msg[idx]))) {
						if(strlen(str)) g_sprintf(buf, "%s %s", str, lookup_string(msg[idx]));
						else 			g_sprintf(buf, "%s", lookup_string(msg[idx]));

						nfui_nflabel_set_text((NFLABEL*)g_msgObj[NETWORK_STATUS_MSG], buf);
						if(expose) nfui_signal_emit(g_msgObj[NETWORK_STATUS_MSG], GDK_EXPOSE, FALSE);
					}
				}else if(info->d.params[0] == 0) {
					for(i=0; i<4; i++) {
					    if(i == 3){
					        if(var_get_conflict_infor()) idx = 4;
					    }
						if(i == idx) continue;

						if(g_strrstr(str, lookup_string(msg[i]))) {
							if(ep)	g_sprintf(ep, "%c", 0x20);

							if(!ep) ep = g_stpcpy(buf, lookup_string(msg[i]));
							else 	ep = g_stpcpy(++ep, lookup_string(msg[i]));
						}
					}

					nfui_nflabel_set_text((NFLABEL*)g_msgObj[NETWORK_STATUS_MSG], buf);
					if(expose) nfui_signal_emit(g_msgObj[NETWORK_STATUS_MSG], GDK_EXPOSE, FALSE);
				}
			}
			break;

		case INFY_AI_KEEP_ALIVE_NOTIFY:
			{
				gint ch = info->d.params[0];
				guint aibox_state = info->d.params[2];

				if ((aibox_state == NF_AIBOX_CONN_FAILED) || (aibox_state == NF_AIBOX_STREAM_CONN_FAILED)) {
					changed = nfui_nfimage_change_image((NFIMAGE*)g_aitroubleObj[ch], IMG_CH_ALARM_ON);
					nfui_nfimage_set_font_color((NFIMAGE*)g_aitroubleObj[ch], COLOR_IDX(393));
				}else {
					changed = nfui_nfimage_change_image((NFIMAGE*)g_aitroubleObj[ch], IMG_CH_ALARM_OFF);
					nfui_nfimage_set_font_color((NFIMAGE*)g_aitroubleObj[ch], COLOR_IDX(394));
				}
				if(expose && changed) nfui_signal_emit(g_aitroubleObj[ch], GDK_EXPOSE, FALSE);
			}
			break;

		default:
			return;
	}
}
