
#include "nf_afx.h"

#include "nf_api_disk.h"
#include "nf_api_cam.h"
#include "nf_util_netif.h"
#include "nf_ipcam_defs.h"

#include "tools/nf_ui_tool.h"

#include "services/scm.h"

#include "support/event_loop.h"
#include "support/color.h"
#include "support/nf_ui_image.h"
#include "support/nf_ui_font.h"
#include "support/nf_ui_page_manager.h"

#include "objects/nfobject.h"
#include "objects/nfwindow.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nfbutton.h"
#include "objects/nflabel.h"
#include "objects/nfimage.h"

#include "vw_sys_main.h"
#include "vw_system_info.h"
#include "dtf.h"
#include "uxm.h"


typedef enum
{
  SI_FW_VER = 0,
  SI_LAST_FW_DATE,
  SI_HW_VER,
//  SI_VIDEO_SIG_TYPE,
  SI_DISK_CAPACITY,
  SI_DISK_USAGE,
  SI_NUM_DISK,
  SI_IP_ADDR,
  SI_IPV6_ADDR,
  SI_MAC_ADDR,
  SI_DDNS_ADDR,
  SI_RTSP_PORT,
  SI_WEB_PORT,
  SI_VIDEO_RES,
  SI_SYSTEM_ID,
  SI_TABLE1_ROW,
}SI_TABLE1_ROW_TYPE;


typedef enum
{
  SI_CAM_CONNECT = 0,
  SI_ALARM_IN_CAM,
  SI_ALARM_IN_NVR,
  SI_ALARM_OUT_CAM,
  SI_ALARM_OUT_DVR,
  SI_TABLE2_ROW,
}SI_TABLE2_ROW_TYPE;


#define SI_LABEL_TEXT_MARGIN				(20)
#define SI_LABEL_HEIGHT						(38)

#define SI_LABEL_WIDTH						(300)
#define SI_CELL_WIDTH						(400)


// SYSTEM INFORMATION
#define SI_INFO_TITLE_X						(8)
#define SI_INFO_TITLE_Y						(0)

#define SI_MODEL_X							(SI_INFO_TITLE_X + SI_LABEL_TEXT_MARGIN)
#define SI_MODEL_Y							(SI_INFO_TITLE_Y + SI_LABEL_HEIGHT + 15)

#define SI_TABLE1_X							(SI_INFO_TITLE_X + SI_LABEL_TEXT_MARGIN)
#define SI_TABLE1_Y							(SI_MODEL_Y + SI_LABEL_HEIGHT + SI_TABLE1_ROW_SPACE)

#define SI_TABLE1_COL						(2)

#define SI_TABLE1_ROW_SPACE					(2)
#define SI_TABLE1_COL_SPACE					(0)
#define MAX_STRING_SIZE						(16)


// STATUS
#define SI_STATUS_TITLE_X					(SI_INFO_TITLE_X)
#define SI_STATUS_TITLE_Y					(SI_TABLE1_Y + (SI_LABEL_HEIGHT*SI_TABLE1_ROW) + (SI_TABLE1_ROW_SPACE*(SI_TABLE1_ROW -1)) + 15)

#define SI_TABLE2_X							(SI_STATUS_TITLE_X + SI_LABEL_TEXT_MARGIN)
#define SI_TABLE2_Y							(SI_STATUS_TITLE_Y + SI_LABEL_HEIGHT + 21)

#define SI_TABLE2_COL						(1)

#define SI_TABLE2_ROW_SPACE					(0)
#define SI_TABLE2_COL_SPACE					(0)

static NFWINDOW *g_curwnd = 0;

static NFOBJECT *info_obj[SI_TABLE1_ROW];
static NFOBJECT *model_obj[2];
static NFOBJECT *status_obj[SI_TABLE2_ROW][GUI_CHANNEL_CNT];
static NFOBJECT *status_title;
static NFOBJECT *status_tbl;

static SysInfoData org_infodata;
static SysInfoData sys_info;

static int cheat_mode = -1;
static int key_count = 0;

enum {
	CHEAT_SHOWVER       = 0,
	CHEAT_MODE
};

#define CHEAT_SHOWVER_SIZE        (8)

static guint cheat_code[CHEAT_MODE][8] = {
    { KEYPAD_UP, KEYPAD_DOWN, KEYPAD_DOWN, KEYPAD_UP, KEYPAD_UP, KEYPAD_UP, KEYPAD_DOWN, KEYPAD_DOWN },
};


static void _system_info_init(void)
{
	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);
}

static void prvUIntToIp(guint *out, guint in)
{
	out[0] = (in >> 24) & 255;
	out[1] = (in >> 16) & 255;
	out[2] = (in >> 8) & 255;
	out[3] = in & 255;
}

static gint _get_cheat_val(KEYPAD_KID kpid)
{
    gint i;

    if (key_count == 0)
    {
        for (i = 0; i < CHEAT_MODE; i++)
        {
            if (cheat_code[i][key_count] == kpid)
            {
                cheat_mode = i;
                key_count++;
                break;
            }
        }
    }
    else
    {
        if (cheat_mode == CHEAT_SHOWVER)
        {
            if (cheat_code[cheat_mode][key_count] == kpid)
            {
                key_count++;
            }
            else
            {
			    cheat_mode = -1;
                key_count = 0;
            }
        }
        else
        {
		    cheat_mode = -1;
            key_count = 0;
        }
    }

    g_message("%s, %d, cheat_mode:%d, key_count:%d", __FUNCTION__, __LINE__, cheat_mode, key_count);

    if ((cheat_mode == CHEAT_SHOWVER) && (key_count == CHEAT_SHOWVER_SIZE))
    {
	    cheat_mode = -1;
        key_count = 0;
        return CHEAT_SHOWVER;
    }

    return -1;
}

static void _info_model_update()
{
	NFOBJECT *tbl = info_obj[0]->parent;
	NFOBJECT *fixed = tbl->parent;
	guint my = SI_LABEL_HEIGHT + SI_TABLE1_ROW_SPACE;
	gint i, j;

	if(strlen(sys_info.model)) {
		nfui_nflabel_set_text((NFLABEL*)model_obj[1], sys_info.model);
		nfui_signal_emit(model_obj[1], GDK_EXPOSE, FALSE);
	}else {
		// hide model info/
		for(i=0; i<2; i++) {
			nfui_nfobject_hide(model_obj[i]);
			nfui_signal_emit(model_obj[i], GDK_EXPOSE, FALSE);
		}

		// move info table
		nfui_nffixed_move((NFFIXED*)fixed, tbl, SI_MODEL_X, SI_MODEL_Y);

		// move status obj
		nfui_nffixed_move((NFFIXED*)fixed, status_title, status_title->x, (status_title->y  - my));
		nfui_nffixed_move((NFFIXED*)fixed, status_tbl, status_tbl->x, (status_tbl->y - my));

		for(i=0; i<SI_TABLE2_ROW; i++) {
			for(j=0; j<GUI_CHANNEL_CNT; j++) {
				if(status_obj[i][j])
					nfui_nffixed_move((NFFIXED*)fixed, status_obj[i][j], status_obj[i][j]->x, (status_obj[i][j]->y - my));
			}
		}
	}
}

static void _info_fw_ver_update()
{
    gchar fwver[32];

    memset(fwver, 0x00, sizeof(fwver));
    var_get_fake_fwver(fwver, 32);

    nfui_nflabel_set_text((NFLABEL*)info_obj[SI_FW_VER], fwver);
	nfui_signal_emit(info_obj[SI_FW_VER], GDK_EXPOSE, TRUE);
}

static void _info_last_fw_date_update()
{
	char uptime[64];
	memset(uptime, 0x00, sizeof(uptime));
	if (sys_info.fwupTime) dtf_get_local_datetime(sys_info.fwupTime, uptime);
	else sprintf(uptime, "N/A");
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_LAST_FW_DATE], uptime);
	nfui_signal_emit(info_obj[SI_LAST_FW_DATE], GDK_EXPOSE, TRUE);
}

static void _info_hw_ver_update()
{
	char ver[128];
	var_get_fake_hwver(ver, 128);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_HW_VER], ver);
	nfui_signal_emit(info_obj[SI_HW_VER], GDK_EXPOSE, TRUE);
}

/*
static void _info_video_sig_type_update()
{
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_VIDEO_SIG_TYPE], "NOTHING");
	nfui_signal_emit(info_obj[SI_VIDEO_SIG_TYPE], GDK_EXPOSE, TRUE);
}*/

static void _info_disk_capacity_update()
{
	char cap[16];
	guint64 size = 0;
	DISK_CAPINFO_T d_info;

	scm_get_disk_capinfo(INTERNAL, &d_info);
	size = d_info.tsize;
	scm_get_disk_capinfo(EXTERNAL, &d_info);
	size += d_info.tsize;

	ifn_convert_storage_size(cap, size);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_DISK_CAPACITY], cap);
	nfui_signal_emit(info_obj[SI_DISK_CAPACITY], GDK_EXPOSE, TRUE);
}

static void _info_disk_usage_update()
{
	char usage[16];
	guint64 size;
	NF_NOTIFY_INFO info;

	scm_get_disk_usage_data(&info);
	size = (guint64)info.d.params[0];
	size *= 1024;		// mb size -> kb size
	ifn_convert_storage_size(usage, size);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_DISK_USAGE], usage);
	nfui_signal_emit(info_obj[SI_DISK_USAGE], GDK_EXPOSE, TRUE);
}

static void _info_num_disk_update()
{
	char diskcnt[8];
	int cnt = scm_get_disk_count();
	sprintf(diskcnt, "%d", cnt);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_NUM_DISK], diskcnt);
	nfui_signal_emit(info_obj[SI_NUM_DISK], GDK_EXPOSE, TRUE);
}

static void _info_ip_addr_update()
{
	gchar strBuf[32];

	if(scm_get_ip_addr_str(strBuf, 32) < 0)
		sprintf(strBuf, "%d.%d.%d.%d", 0, 0, 0, 0);

	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_IP_ADDR], strBuf);
	nfui_signal_emit(info_obj[SI_IP_ADDR], GDK_EXPOSE, TRUE);
}

static void _info_ipv6_addr_update()
{
	gchar strBuf[64];
    NF_NETIF_GET_INFO info;
    
	memset(strBuf, 0x00, sizeof(strBuf));
	memset(&info, 0x00, sizeof(NF_NETIF_GET_INFO));
	
	scm_get_sys_netinfo(&info);

	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_IPV6_ADDR], info.ipv6_linklocal);
	nfui_signal_emit(info_obj[SI_IPV6_ADDR], GDK_EXPOSE, TRUE);
}

static void _info_mac_addr_update()
{
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_MAC_ADDR], sys_info.macAddr);
	nfui_signal_emit(info_obj[SI_MAC_ADDR], GDK_EXPOSE, TRUE);
}

static void _info_ddns_addr_update()
{
	char ddns[256];

	scm_get_dvr_addr_str(ddns, 256);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_DDNS_ADDR], ddns);
	nfui_signal_emit(info_obj[SI_DDNS_ADDR], GDK_EXPOSE, TRUE);
}

static void _info_rtsp_port_update()
{
	gchar strBuf[32];

	sprintf(strBuf, "%d", sys_info.rtspPort);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_RTSP_PORT], strBuf);
	nfui_signal_emit(info_obj[SI_RTSP_PORT], GDK_EXPOSE, TRUE);
}

static void _info_web_port_update()
{
	gchar strBuf[32];

	sprintf(strBuf, "%d", sys_info.webPort);
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_WEB_PORT], strBuf);
	nfui_signal_emit(info_obj[SI_WEB_PORT], GDK_EXPOSE, TRUE);
}

static void _info_resolution_update()
{
	gchar strBuf[64], strRes[32], strMode[32];
	gchar *uStr;

	if(!scm_get_video_resolution(strRes)) return FALSE;
	if(!scm_get_video_output(strMode)) 	  return FALSE;

	uStr = g_ascii_strup(strMode, -1);
	sprintf(strBuf, "%s (%s)", uStr, strRes);
	g_free(uStr);

	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_VIDEO_RES], strBuf);
	nfui_signal_emit(info_obj[SI_VIDEO_RES], GDK_EXPOSE, TRUE);
}

static void _info_sysid_update()
{
	nfui_nflabel_set_text((NFLABEL*)info_obj[SI_SYSTEM_ID], sys_info.sysId);
	nfui_signal_emit(info_obj[SI_SYSTEM_ID], GDK_EXPOSE, TRUE);
}

static void _status_cam_connect_update()
{
	guint i;
	gboolean change = FALSE;
	BITMASK ch_mask = 0;

	ch_mask = scm_get_cam_conn_state();

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (ch_mask & (1 << i)) {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_CAM_CONNECT][i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_CAM_CONNECT][i], COLOR_IDX(393));
		}else {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_CAM_CONNECT][i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_CAM_CONNECT][i], COLOR_IDX(394));
		}

		if(change)
			nfui_signal_emit(status_obj[SI_CAM_CONNECT][i], GDK_EXPOSE, TRUE);
	}
}

static void _status_alarm_in_cam_update(guint ch_mask)
{
	guint i;
	gboolean change = FALSE;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (ch_mask & (1 << i)) {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_IN_CAM][i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_IN_CAM][i], COLOR_IDX(393));
		}else {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_IN_CAM][i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_IN_CAM][i], COLOR_IDX(394));
		}

		if(change)
			nfui_signal_emit(status_obj[SI_ALARM_IN_CAM][i], GDK_EXPOSE, TRUE);
	}
}

static void _status_alarm_in_dvr_update(guint ch_mask)
{
	guint i;
	gboolean change = FALSE;

#ifndef GUI_32CH_SUPPORT
	for (i = 0; i < var_get_dvr_alarmIn_cnt(); i++)
	{
		if (ch_mask & (1 << (16 + i))) {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], COLOR_IDX(393));
		}else {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], COLOR_IDX(394));
		}

		if(change)
			nfui_signal_emit(status_obj[SI_ALARM_IN_NVR][i], GDK_EXPOSE, TRUE);
	}
#else
	for (i = 0; i < var_get_dvr_alarmIn_cnt(); i++)
	{
		if (ch_mask & (1 << i)) {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], COLOR_IDX(393));
		}else {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], COLOR_IDX(394));
		}

		if(change)
			nfui_signal_emit(status_obj[SI_ALARM_IN_NVR][i], GDK_EXPOSE, TRUE);
	}
#endif
}

static void _status_alarm_out_cam_update(guint ch_mask)
{
	guint i;
	gboolean change = FALSE;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		if (ch_mask & (1 << i)) {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_OUT_CAM][i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_OUT_CAM][i], COLOR_IDX(393));
		}else {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_OUT_CAM][i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_OUT_CAM][i], COLOR_IDX(394));
		}

		if(change)
			nfui_signal_emit(status_obj[SI_ALARM_OUT_CAM][i], GDK_EXPOSE, TRUE);
	}
}

static void _status_alarm_out_dvr_update(guint ch_mask)
{
	guint i;
	gboolean change = FALSE;

	for (i = 0; i < DVR_ALARM_OUT; i++)
	{
		if (ch_mask & (1 << (i))) {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_OUT_DVR][i], IMG_CH_ALARM_ON);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_OUT_DVR][i], COLOR_IDX(393));
		}else {
			change = nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_ALARM_OUT_DVR][i], IMG_CH_ALARM_OFF);
			nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_ALARM_OUT_DVR][i], COLOR_IDX(394));
		}

		if(change)
			nfui_signal_emit(status_obj[SI_ALARM_OUT_DVR][i], GDK_EXPOSE, TRUE);
	}
}


static void _info_status_update(void)
{
	_info_model_update();
	_info_fw_ver_update();
	_info_last_fw_date_update();
	_info_hw_ver_update();
//	_info_video_sig_type_update();
	_info_disk_capacity_update();
	_info_disk_usage_update();
	_info_num_disk_update();
	_info_ip_addr_update();
	_info_ipv6_addr_update();
	_info_mac_addr_update();
	_info_ddns_addr_update();
	_info_rtsp_port_update();
	_info_web_port_update();
	_info_resolution_update();
	_info_sysid_update();

	_status_cam_connect_update();

	if(scm_req_sensor_event_data() < 0)
		g_warning("%s [%d] : scm_req_sensor_event_data returns -1", __FUNCTION__, __LINE__);

	if(scm_req_alarm_event_data() < 0)
		g_warning("%s [%d] : scm_req_alarm_event_data returns -1", __FUNCTION__, __LINE__);

}

static gboolean post_cancelbutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	}

	return FALSE;
}

static gboolean post_applybutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	GdkEventKey *kevt;
	KEYPAD_KID kpid = KEYPAD_NONE;
    	gint val;

	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

	}
	else if ((evt->type == NFEVENT_KEYPAD_PRESS) || (evt->type == NFEVENT_REMOCON_PRESS))
	{
		kevt = (GdkEventKey*)evt;
		kpid = (KEYPAD_KID)kevt->keyval;

        	val = _get_cheat_val(kpid);

		if (val == CHEAT_SHOWVER)
		{
          		nfui_nflabel_set_text((NFLABEL*)info_obj[SI_FW_VER], sys_info.swVer);
        		nfui_signal_emit(info_obj[SI_FW_VER], GDK_EXPOSE, TRUE);
		}
	}

	return FALSE;
}

static gboolean post_closebutton_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	if(evt->type == GDK_BUTTON_RELEASE)
	{
		if(evt->button.button == MOUSE_RIGTH_BUTTON) return FALSE;

		VW_SetupSystem_Destroy(obj);
	}

	return FALSE;
}

static gboolean post_cc_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	gint ch;

	switch(evt->type)
	{
		case INFY_PND_NOTIFY:
			{
				NF_NOTIFY_INFO *notify_info = (NF_NOTIFY_INFO *)data;

				ch = notify_info->d.params[1];

				if (notify_info->d.params[0] == PND_TYPE_VIDEO_START)
				{
					nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_CAM_CONNECT][ch], IMG_CH_ALARM_ON);
					nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_CAM_CONNECT][ch], COLOR_IDX(393));
					nfui_signal_emit(status_obj[SI_CAM_CONNECT][ch], GDK_EXPOSE, TRUE);
				}
				else if (notify_info->d.params[0] == PND_TYPE_UNPLUGGED)
				{
					nfui_nfimage_change_image((NFIMAGE*)status_obj[SI_CAM_CONNECT][ch], IMG_CH_ALARM_OFF);
					nfui_nfimage_set_font_color((NFIMAGE*)status_obj[SI_CAM_CONNECT][ch], COLOR_IDX(394));
					nfui_signal_emit(status_obj[SI_CAM_CONNECT][ch], GDK_EXPOSE, TRUE);
				}
			}
			break;

		case GDK_DELETE:
			uxm_unreg_imsg_event(obj, INFY_PND_NOTIFY);
			break;

		default:
			break;
	}

	return FALSE;
}

static gboolean post_aod_event_handler(NFOBJECT *obj, GdkEvent *evt, gpointer data)
{
	NF_NOTIFY_INFO *pInfo = NULL;

	switch(evt->type)
	{
		case INFY_SENSOR_NOTIFY:
			{
				pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

				if(pInfo)
				g_message("%s [%d] ::::::::::::evt->type = INFY_SENSOR_NOTIFY, param 0: %08x , param 1: %08x ",
						__FUNCTION__, __LINE__,
						pInfo->d.params[0],
						pInfo->d.params[1]);

				if(pInfo) {
					_status_alarm_in_cam_update(pInfo->d.params[1]);
					_status_alarm_in_dvr_update(pInfo->d.params[0]);
				}
			}
			break;

		case INFY_ALARM_NOTIFY:
			{
				pInfo = ((NF_NOTIFY_INFO *)((CMM_MESSAGE_T *)data)->data);

				if(pInfo)
				g_message("%s [%d] ::::::::::::evt->type = INFY_ALARM_NOTIFY, param 0: %08x , param 1: %08x ",
						__FUNCTION__, __LINE__,
						pInfo->d.params[0],
						pInfo->d.params[1]);

				if(pInfo) {
					_status_alarm_out_cam_update(pInfo->d.params[1]);
					_status_alarm_out_dvr_update(pInfo->d.params[0]);
				}
			}
			break;

		case GDK_DELETE:
			{
				uxm_unreg_imsg_event(obj, INFY_SENSOR_NOTIFY);
				uxm_unreg_imsg_event(obj, INFY_ALARM_NOTIFY);
			}
			break;

		default:
			break;
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

void VW_Init_SystemInfo_Page(NFOBJECT *parent)
{
	NFOBJECT *content_fixed;
	NFOBJECT *tbl;
	NFOBJECT *obj;

	GdkPixbuf *tmp;

	const gchar *strTitle1[] = {
		"FW VERSION",
		"LAST FW UPDATE DATE",
		"HW VERSION",
//		"VIDEO SIGNAL TYPE",
		"DISK CAPACITY",
		"DISK USAGE",
		"NUMBER OF DISKS",
		"IP ADDRESS",
		"IPv6 ADDRESS",
		"MAC ADDRESS",
		"DDNS ADDRESS",
		"RTSP SERVICE PORT",
		"WEB SERVICE PORT",
		"RESOLUTION",
		"SYSTEM ID"
	};

	const gchar *strTitle2[] = {
		"CAMERA CONNECTION",
		"ALARM IN(CAMERA)",
		"ALARM IN(NVR)",
		"ALARM OUT(CAMERA)",
		"ALARM OUT(NVR)"
	};

	guint table_w[] = {SI_LABEL_WIDTH, SI_CELL_WIDTH};
	gchar strBuf[8];
	guint i, j, size_w, size_h;
	guint x, y;


	_system_info_init();

	content_fixed = (NFOBJECT*)nfui_nffixed_new();
	nfui_nfobject_modify_bg(content_fixed, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_set_size(content_fixed, MENU_V_INNER_W, MENU_V_INNER_H);
	nfui_nfobject_show(content_fixed);
	nfui_nffixed_put((NFFIXED*)parent, content_fixed, MENU_V_INNER_X, MENU_V_INNER_Y);


// <----- SYSTEM INFORMATION TITLE.
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "SYSTEM INFORMATION");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, SI_LABEL_TEXT_MARGIN);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SI_INFO_TITLE_X, SI_INFO_TITLE_Y);


// <----- MODEL
	model_obj[0] = (NFOBJECT*)nfui_nflabel_new_with_pango_font("MODEL", nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
	nfui_nflabel_set_align((NFLABEL*)model_obj[0], NFALIGN_LEFT, 0);
	nfui_nfobject_modify_bg(model_obj[0], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_use_focus(model_obj[0], NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(model_obj[0], SI_LABEL_WIDTH, SI_LABEL_HEIGHT);
	nfui_nfobject_show(model_obj[0]);
	nfui_nffixed_put((NFFIXED*)content_fixed, model_obj[0], SI_MODEL_X, SI_MODEL_Y);

	model_obj[1] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));
	nfui_nflabel_set_skin_type((NFLABEL*)model_obj[1], NFTEXTBOX_TYPE_OUTPUT);
	nfui_nflabel_set_align((NFLABEL*)model_obj[1], NFALIGN_CENTER, 0);
	nfui_nfobject_use_focus(model_obj[1], NFOBJECT_FOCUS_OFF);
	nfui_nfobject_set_size(model_obj[1], SI_CELL_WIDTH, SI_LABEL_HEIGHT);
	nfui_nfobject_show(model_obj[1]);
	nfui_nffixed_put((NFFIXED*)content_fixed, model_obj[1], (SI_MODEL_X + SI_LABEL_WIDTH), SI_MODEL_Y);



// <----- table
	tbl = (NFOBJECT*)nfui_nftable_new(SI_TABLE1_COL, SI_TABLE1_ROW, SI_TABLE1_COL_SPACE, SI_TABLE1_ROW_SPACE, table_w, SI_LABEL_HEIGHT);
	nfui_nfobject_modify_bg(tbl, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, SI_TABLE1_X, SI_TABLE1_Y);

	for(i = 0; i < SI_TABLE1_ROW; i++) {
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle1[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);

		info_obj[i] = (NFOBJECT*)nfui_nflabel_new_text_box("", nffont_get_pango_font(NFFONT_MEDIUM_SEMI));

		nfui_nfobject_use_focus(info_obj[i], NFOBJECT_FOCUS_OFF);
		nfui_nflabel_set_skin_type((NFLABEL*)info_obj[i], NFTEXTBOX_TYPE_OUTPUT);
		nfui_nflabel_set_align((NFLABEL*)info_obj[i], NFALIGN_CENTER, 0);
		nfui_nfobject_show(info_obj[i]);
		nfui_nftable_attach((NFTABLE*)tbl, info_obj[i], 1, i);
	}
		nfui_nfobject_support_multi_lang((NFOBJECT*)info_obj[SI_SYSTEM_ID], FALSE);


// <----- STATUS TITLE.
	obj = nfui_nfimage_new(IMG_TITLE_BG);
	nfui_nfimage_set_text((NFIMAGE*)obj, "STATUS");
	nfui_nfimage_set_pango_font((NFIMAGE*)obj, nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(192));
	nfui_nfimage_set_font_alignment((NFIMAGE*)obj, NFALIGN_LEFT, SI_LABEL_TEXT_MARGIN);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)content_fixed, obj, SI_STATUS_TITLE_X, SI_STATUS_TITLE_Y);
	status_title = obj;

	tbl = (NFOBJECT*)nfui_nftable_new(SI_TABLE2_COL, SI_TABLE2_ROW, SI_TABLE2_COL_SPACE, SI_TABLE2_ROW_SPACE, table_w, SI_LABEL_HEIGHT);
	nfui_nfobject_show(tbl);
	nfui_nffixed_put((NFFIXED*)content_fixed, tbl, SI_TABLE2_X, SI_TABLE2_Y);
	status_tbl = tbl;


	tmp = nfui_get_image_from_file((IMG_CH_ALARM_OFF), NULL);
	nfui_get_pixbuf_size(tmp, &size_w, &size_h);

	for(i = 0; i < SI_TABLE2_ROW; i++) {
// <----- STATUS LABEL.
		obj = (NFOBJECT*)nfui_nflabel_new_with_pango_font(strTitle2[i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(120));
		nfui_nflabel_set_align((NFLABEL*)obj, NFALIGN_LEFT, 0);
		nfui_nfobject_modify_bg(obj, NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nfobject_use_focus(obj, NFOBJECT_FOCUS_OFF);
		nfui_nfobject_show(obj);
		nfui_nftable_attach((NFTABLE*)tbl, obj, 0, i);
	}


// <----- CAMERA CONNECT STATUS.
	x = SI_TABLE2_X + SI_LABEL_WIDTH;
	y = SI_TABLE2_Y;

	for(i = 0; i < GUI_CHANNEL_CNT; i++) {
		g_sprintf(strBuf, "%d", i + 1);
		status_obj[SI_CAM_CONNECT][i] = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)status_obj[SI_CAM_CONNECT][i], strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)status_obj[SI_CAM_CONNECT][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(status_obj[SI_CAM_CONNECT][i]);
		nfui_nfobject_modify_bg(status_obj[SI_CAM_CONNECT][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_CAM_CONNECT][i], x+(size_w+2)*i, y);
		//nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_CAM_CONNECT][i], x+(size_w+2)*i, y+(SI_LABEL_HEIGHT-size_h)/2);
	}

// <----- ALARM IN(CAMERA) STATUS.
	x = SI_TABLE2_X + SI_LABEL_WIDTH;
	y = SI_TABLE2_Y + (SI_LABEL_HEIGHT+SI_TABLE2_ROW_SPACE)*1 ;

	for(i = 0; i < GUI_CHANNEL_CNT; i++) {
		g_sprintf(strBuf, "%d", i + 1);
		status_obj[SI_ALARM_IN_CAM][i] = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)status_obj[SI_ALARM_IN_CAM][i], strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)status_obj[SI_ALARM_IN_CAM][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(status_obj[SI_ALARM_IN_CAM][i]);
		nfui_nfobject_modify_bg(status_obj[SI_ALARM_IN_CAM][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_IN_CAM][i], x+(size_w+2)*i, y);
		//nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_IN_CAM][i], x+(size_w+2)*i, y+(SI_LABEL_HEIGHT-size_h)/2);
	}

// <----- ALARM IN(NVR) STATUS.
	x = SI_TABLE2_X + SI_LABEL_WIDTH;
	y = SI_TABLE2_Y + (SI_LABEL_HEIGHT+SI_TABLE2_ROW_SPACE)*2 ;

	for(i = 0; i < var_get_dvr_alarmIn_cnt(); i++) {
		g_sprintf(strBuf, "%d", i + 1);
		status_obj[SI_ALARM_IN_NVR][i] = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)status_obj[SI_ALARM_IN_NVR][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(status_obj[SI_ALARM_IN_NVR][i]);
		nfui_nfobject_modify_bg(status_obj[SI_ALARM_IN_NVR][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_IN_NVR][i], x+(size_w+2)*i, y);
		//nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_IN_NVR][i], x+(size_w+2)*i, y+(SI_LABEL_HEIGHT-size_h)/2);
	}

// <----- ALARM OUT(CAMERA) STATUS.
	x = SI_TABLE2_X + SI_LABEL_WIDTH;
	y = SI_TABLE2_Y + (SI_LABEL_HEIGHT+SI_TABLE2_ROW_SPACE)*3;

	for(i = 0; i < GUI_CHANNEL_CNT; i++) {
		g_sprintf(strBuf, "%d", i + 1);
		status_obj[SI_ALARM_OUT_CAM][i] = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)status_obj[SI_ALARM_OUT_CAM][i], strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)status_obj[SI_ALARM_OUT_CAM][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(status_obj[SI_ALARM_OUT_CAM][i]);
		nfui_nfobject_modify_bg(status_obj[SI_ALARM_OUT_CAM][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_OUT_CAM][i], x+(size_w+2)*i, y);
		//nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_OUT_CAM][i], x+(size_w+2)*i, y+(SI_LABEL_HEIGHT-size_h)/2);
	}

// <----- ALARM OUT(NVR) STATUS.
	x = SI_TABLE2_X + SI_LABEL_WIDTH;
	y = SI_TABLE2_Y + (SI_LABEL_HEIGHT+SI_TABLE2_ROW_SPACE)*4;

	for(i = 0; i < DVR_ALARM_OUT; i++) {
		g_sprintf(strBuf, "%d", i + 1);
		status_obj[SI_ALARM_OUT_DVR][i] = nfui_nfimage_new(IMG_CH_ALARM_OFF);
		nfui_nfimage_set_text((NFIMAGE*)status_obj[SI_ALARM_OUT_DVR][i], strBuf);
		nfui_nfimage_set_pango_font((NFIMAGE*)status_obj[SI_ALARM_OUT_DVR][i], nffont_get_pango_font(NFFONT_MEDIUM_SEMI), COLOR_IDX(394));
		nfui_nfobject_show(status_obj[SI_ALARM_OUT_DVR][i]);
		nfui_nfobject_modify_bg(status_obj[SI_ALARM_OUT_DVR][i], NFOBJECT_STATE_NORMAL, COLOR_IDX(0));
		nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_OUT_DVR][i], x+(size_w+2)*i, y);
		//nfui_nffixed_put((NFFIXED*)content_fixed, status_obj[SI_ALARM_OUT_DVR][i], x+(size_w+2)*i, y+(SI_LABEL_HEIGHT-size_h)/2);
	}

	uxm_reg_imsg_event(status_obj[SI_CAM_CONNECT][0], INFY_PND_NOTIFY);
	uxm_reg_imsg_event(status_obj[SI_ALARM_OUT_DVR][0], INFY_SENSOR_NOTIFY);
	uxm_reg_imsg_event(status_obj[SI_ALARM_OUT_DVR][0], INFY_ALARM_NOTIFY);

	uxm_monitor_on_imsg_event(status_obj[SI_CAM_CONNECT][0], INFY_PND_NOTIFY);
	uxm_monitor_on_imsg_event(status_obj[SI_ALARM_OUT_DVR][0], INFY_SENSOR_NOTIFY);
	uxm_monitor_on_imsg_event(status_obj[SI_ALARM_OUT_DVR][0], INFY_ALARM_NOTIFY);

	nfui_regi_post_event_callback(status_obj[SI_CAM_CONNECT][0], post_cc_event_handler);
	nfui_regi_post_event_callback(status_obj[SI_ALARM_OUT_DVR][0], post_aod_event_handler);


// <---- CANCEL, APPLY, CLOSE BUTTON
	obj = nftool_normal_button_create_type1("CANCEL", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R3_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_cancelbutton_event_handler);

	obj = nftool_normal_button_create_type1("APPLY", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R2_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_applybutton_event_handler);

	obj = nftool_normal_button_create_type2("CLOSE", MENU_BTN_WIDTH);
	nfui_nfobject_show(obj);
	nfui_nffixed_put((NFFIXED*)parent, obj, MENU_V_BTN_R1_X, MENU_V_BTN_Y);
	nfui_regi_post_event_callback(obj, post_closebutton_event_handler);

	nfui_regi_post_event_callback(parent, post_page_event_handler);

	_info_status_update();

    cheat_mode = -1;
    key_count = 0;
}

gboolean VW_SystemInfo_tab_in_handler()
{
	SysInfoData info;

    	cheat_mode = -1;
   	 key_count = 0;

	DAL_get_sysInfo_data(&info);

	if(memcmp(&info, &sys_info, sizeof(SysInfoData)) != 0)
	{
		if (info.sysId != sys_info.sysId)
		{
			nfui_nflabel_set_text((NFLABEL*)info_obj[SI_SYSTEM_ID], info.sysId);
		}

		memcpy(&sys_info, &info, sizeof(SysInfoData));
	}

    return TRUE;
}
