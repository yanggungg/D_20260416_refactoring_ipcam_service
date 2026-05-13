

/* 
 * scm_notify.c
 *
 * written by seongho
 *
 */

#include <string.h>
#include "iux_afx.h"
#include "nf_notify.h"
#include "nf_api_live.h"
#include "scm.h"
#include "cmm.h"
#include "ssm.h"
#include "ix_mem.h"
#include "nf_sysdb.h"
#include "scm_internal.h"
#include "evt.h"
#include "ix_conf.h"

#include "ivca_def.h"
#include "libivcam.h"
#include "nf_meta_data.h"

#include "vaa.h"
#include "dvaa_itx.h"
#include "pos.h"

#include "nf_api_eventlog.h"
#include "nf_api_pos_eventlog.h"

DECLARE DBG_SYSTEM

#define DBG_LEVEL		DBG_CONF
#define DBG_MODULE		"SCM_NOTIFY"

//#define USE_MOTION_MODULAR	1
#define USE_VCA_EVT_MODULAR		1
#define USE_DVA_EVT_MODULAR		1
#define USE_DVABX_EVT_MODULAR	1


#define VMT_LOCK()		g_mutex_lock(iscm.mtx_vmt)
#define VMT_UNLOCK()	g_mutex_unlock(iscm.mtx_vmt)

#define DMT_LOCK()		g_mutex_lock(iscm.mtx_dmt)
#define DMT_UNLOCK()	g_mutex_unlock(iscm.mtx_dmt)

#define DXMT_LOCK()		g_mutex_lock(iscm.mtx_dxmt)
#define DXMT_UNLOCK()	g_mutex_unlock(iscm.mtx_dxmt)



typedef struct _DELAY_NOTIFY_INFO
{ 
	IMSG 		msgid;
	GTimeVal	timestamp; 
    gpointer	data;
} DELAY_NOTIFY_INFO; 

static WRK_ID iwrk_delay_aievt = 0; 



////////////////////////////////////////////////////////////
//
// private functions
//

static int _update_novideo_mask(NF_NOTIFY_INFO *pnotify)
{
	char buf[33];
	BITMASK novideo = var_get_novideo_mask();
	novideo &= ~(novideo & ~pnotify->d.params[0]);
	ifn_hex2btxt_le(novideo, buf);
	DMSG(7, "CURRENT NOVIDEO STATE = [%s]\n", buf);
	var_set_novideo_mask(novideo);
	return 0;
}

NF_NOTIFY_INFO *_dup_notify(NF_NOTIFY_INFO *src)
{	
	NF_NOTIFY_INFO	*dest;

	if (src->type == NF_NOTIFY_POINTER) {
		if (src->p.ptr == NULL || src->p.len == 0) return NULL;
	}
		
	dest = imalloc(sizeof(NF_NOTIFY_INFO));
	memcpy(dest, src, sizeof(NF_NOTIFY_INFO));
	if (src->type == NF_NOTIFY_POINTER) {
		gpointer tmp;
		DMSG(9, "malloc = %d", src->p.len);
		tmp = imalloc(src->p.len);
		if (tmp) {
			dest->p.ptr = tmp;
			memcpy( dest->p.ptr, src->p.ptr, src->p.len);
		}
		else {
			ifree(dest);
			dest = NULL;
		}
	}
	return dest;
}

static int _send_notify_msg(IMSG msgid, NF_NOTIFY_INFO *pnotify)
{
	NF_NOTIFY_INFO *pdata;
	pdata = _dup_notify(pnotify);
	if (pdata == NULL) return -1;
	_scm_send_struct_to_viewer(msgid, 0, pdata, _dup_notify);
	return 0;
}

static guint _get_motion_data(SCM_T *piscm)
{
	int ch;
	guint mdata = 0;

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		if (piscm->mmt[ch].motion_status)	mdata |= (1 << ch);
	}	

	return mdata;
}

static gboolean _proc_motion_modular(void *data)
{
	SCM_T *piscm = (SCM_T *)data;
	NF_NOTIFY_INFO notify;
	int ch;
	guint prev_mdata = 0, post_mdata = 0;

	prev_mdata = _get_motion_data(piscm);

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		if (piscm->mmt[ch].motion_state)
		{
			piscm->mmt[ch].motion_status = 1;
			piscm->mmt[ch].motion_cnt = 0;
		}
		else
		{
			if (piscm->mmt[ch].motion_cnt >= 4)
			{
				piscm->mmt[ch].motion_status = 0;
				piscm->mmt[ch].motion_cnt = 0;
			}
			else
				piscm->mmt[ch].motion_cnt += 1;
		}
	}

	post_mdata = _get_motion_data(piscm);

	if (prev_mdata != post_mdata)
	{
		memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
		notify.d.params[0] = post_mdata;
		_send_notify_msg(INFY_MOTION_NOTIFY, &notify);
	}

	return TRUE;
}

static gboolean _proc_vca_event_modular(void *data)
{
	SCM_T *piscm = (SCM_T *)data;
	NF_NOTIFY_INFO notify;
	int ch, rule;
	guint prev_vdata[32];

	VMT_LOCK();
	
	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		prev_vdata[ch] = piscm->vmt[ch].vca_status;
		piscm->vmt[ch].vca_status = 0;

		for (rule = 0; rule < 16; rule++)
		{
			if (piscm->vmt[ch].vca_time[rule]) 
			{
				piscm->vmt[ch].vca_status |= (1 << rule);
				piscm->vmt[ch].vca_time[rule] -= 1;
			}
		}
	}

	VMT_UNLOCK();

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		if (prev_vdata[ch] != piscm->vmt[ch].vca_status)
		{
			memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
			notify.d.params[0] = ch;
			notify.d.params[1] = piscm->vmt[ch].vca_status;
			_send_notify_msg(INFY_VCA_MODULAR_EVENT, &notify);
		}
	}

	return TRUE;
}

static gboolean _proc_dva_event_modular(void *data)
{
	SCM_T *piscm = (SCM_T *)data;
	NF_NOTIFY_INFO notify;
	int ch;
	guint prev_vdata[GUI_CHANNEL_CNT];

	DMT_LOCK();
	
	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		prev_vdata[ch] = piscm->dmt[ch].dva_status;
		piscm->dmt[ch].dva_status = 0;

		if (piscm->dmt[ch].dva_time) 
		{
			piscm->dmt[ch].dva_status = 1;
			piscm->dmt[ch].dva_time -= 1;
		}
	}

	DMT_UNLOCK();

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		if (prev_vdata[ch] != piscm->dmt[ch].dva_status)
		{
			memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
			notify.d.params[0] = ch;
			notify.d.params[1] = piscm->dmt[ch].dva_status;
			_send_notify_msg(INFY_DEEPLEARNING_MODULAR_EVENT, &notify);
		}
	}

	return TRUE;
}

static gboolean _proc_dvabx_event_modular(void *data)
{
	SCM_T *piscm = (SCM_T *)data;
	NF_NOTIFY_INFO notify;
	int ch, rule;
	guint prev_vdata[GUI_CHANNEL_CNT];
	guint prev_fr_vdata[GUI_CHANNEL_CNT];
	guint prev_lpr_vdata[GUI_CHANNEL_CNT];

	DXMT_LOCK();
	
	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		prev_vdata[ch] = piscm->dxmt[ch].dvabx_status;
		piscm->dxmt[ch].dvabx_status = 0;
		prev_fr_vdata[ch] = piscm->dxmt_fr[ch].dvabx_status;
		piscm->dxmt_fr[ch].dvabx_status = 0;
		prev_lpr_vdata[ch] = piscm->dxmt_lpr[ch].dvabx_status;
		piscm->dxmt_lpr[ch].dvabx_status = 0;

		for (rule = 0; rule < 16; rule++)
		{
			if (piscm->dxmt[ch].dvabx_time[rule]) 
			{
				piscm->dxmt[ch].dvabx_status |= (1 << rule);
				piscm->dxmt[ch].dvabx_time[rule] -= 1;
			}
		}

		if (piscm->dxmt_fr[ch].dvabx_time[0]) 
		{
			piscm->dxmt_fr[ch].dvabx_status = 1;
			piscm->dxmt_fr[ch].dvabx_time[0] -= 1;
		}	

		if (piscm->dxmt_lpr[ch].dvabx_time[0]) 
		{
			piscm->dxmt_lpr[ch].dvabx_status = 1;
			piscm->dxmt_lpr[ch].dvabx_time[0] -= 1;
		}			
	}

	DXMT_UNLOCK();

	for (ch = 0; ch < var_get_ch_count(); ch++)
	{
		if (prev_vdata[ch] != piscm->dxmt[ch].dvabx_status)
		{
			memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
			notify.d.params[0] = ch;
			notify.d.params[1] = piscm->dxmt[ch].dvabx_status;
			_send_notify_msg(INFY_AI_MODULAR_EVENT, &notify);
		}

		if (prev_fr_vdata[ch] != piscm->dxmt_fr[ch].dvabx_status)
		{
			memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
			notify.d.params[0] = ch;
			notify.d.params[1] = piscm->dxmt_fr[ch].dvabx_status;
			_send_notify_msg(INFY_AI_FACE_MODULAR_EVENT, &notify);
		}

		if (prev_lpr_vdata[ch] != piscm->dxmt_lpr[ch].dvabx_status)
		{
			memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
			notify.d.params[0] = ch;
			notify.d.params[1] = piscm->dxmt_lpr[ch].dvabx_status;
			_send_notify_msg(INFY_AI_PLATENO_MODULAR_EVENT, &notify);
		}		
	}

	return TRUE;
}

static int _check_clon_device()
{
    if (scm_is_clon_device() == 0) {
        _scm_send_msg_to_viewer(INFY_DETECTED_CLON_DEVICE, 1);
    }
    else {
        _scm_send_msg_to_viewer(INFY_DETECTED_CLON_DEVICE, 0);
    }

    return 0;
}

static int _init_gpu_mode()
{
	CamItxFisheyeData fisheye_data;
	DVAPropData dlva_data;
	gint i, gpu_mode = GPU_NONE;

	for (i = 0; i < var_get_ch_count(); i++)
	{
		memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
		DAL_get_camera_itx_fisheye_data(&fisheye_data, i);
		if (fisheye_data.act) {
			gpu_mode = GPU_FISHEYE;
			break;
		}

		if ((ivsc.dfunc.support_dlva_itx == 1) && (scm_license_is_activated_dlva() == 1))
		{
			memset(&dlva_data, 0x00, sizeof(DVAPropData));
			DAL_get_dva_prop_data(&dlva_data, i);
			if (dlva_data.active) {			
				gpu_mode = GPU_DLVA;
				break;
			}
		}
	}

	scm_set_gpu_mode_function(gpu_mode);
	return 0;
}

static int _load_fisheye_param()
{
	CamItxFisheyeData fisheye_data;
	ItxTmpFisheyeData tmpfisheye_data;

	NF_FISHEYE_VIDEO_PARAM video_param;
	NF_FISHEYE_PTZ_PARAM ptz_param;

	int dewarp_ch = -1;
	int i, j;

	feye_block_save_data();

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
		memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
		DAL_get_camera_itx_fisheye_data(&fisheye_data, i);

		memset(&video_param, 0x00, sizeof(NF_FISHEYE_VIDEO_PARAM));

		if (fisheye_data.mount_mode == 0) video_param.mnt_type = NF_FISHEYE_MOUNT_CEILING;
		else if (fisheye_data.mount_mode == 1) video_param.mnt_type = NF_FISHEYE_MOUNT_WALL;
		else if (fisheye_data.mount_mode == 2) video_param.mnt_type = NF_FISHEYE_MOUNT_GROUND;

		if (fisheye_data.default_view == 0) video_param.view_type = NF_FISHEYE_VIEW_SIGLE;
		else if (fisheye_data.default_view == 1) video_param.view_type = NF_FISHEYE_VIEW_QUAD;
		else if (fisheye_data.default_view == 2) video_param.view_type = NF_FISHEYE_VIEW_PANORAMA;

		nf_live_fisheye_set_video_param(i, &video_param);

		memset(&tmpfisheye_data, 0x00, sizeof(ItxTmpFisheyeData));
		DAL_get_itx_tmp_fisheye_data(&tmpfisheye_data, i);
		
		memset(&ptz_param, 0x00, sizeof(NF_FISHEYE_PTZ_PARAM));

		for (j = 0; j < MAX_FISHEYE_VTYPE; j++)
		{
			ptz_param.view[j].pan = tmpfisheye_data.pan[j];
			ptz_param.view[j].tilt = tmpfisheye_data.tilt[j];
			ptz_param.view[j].zoom = tmpfisheye_data.zoom[j];
		}

		nf_live_fisheye_set_ptz_param(i, &ptz_param);
    }		

    for (i = 0; i < GUI_CHANNEL_CNT; i++)
    {
        DAL_get_itx_tmp_fisheye_data(&tmpfisheye_data, i);
        if (tmpfisheye_data.dewarp == 1) {
			dewarp_ch = i;
			break;
		}
    }

	if (dewarp_ch == -1) 
	{
		nf_live_fisheye_set_enable(dewarp_ch);
	}
	else
	{
		memset(&fisheye_data, 0x00, sizeof(CamItxFisheyeData));
		DAL_get_camera_itx_fisheye_data(&fisheye_data, dewarp_ch);
		if (fisheye_data.act) nf_live_fisheye_set_enable(dewarp_ch);
		else nf_live_fisheye_set_enable(-1);
	}

	return 0;
}

static int _load_dlva_param()
{
	gchar sysdb_key[32];
	guint i, chmask = 0;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		snprintf(sysdb_key, sizeof(sysdb_key), "cam.dva.D%d.act", i);
		if (nf_sysdb_get_bool(sysdb_key)) chmask |= (1 << i);
	}

	scm_dlva_detector_live_start_chmask(chmask);
	return 0;
}

static int _set_ai_rule(guint chmask, gint sub_category)
{
	int i;

	if (sub_category != SYSDB_SUB_CATE_CAM_AI) return -1;

	g_message("%s, %d, chmask:%08X", __FUNCTION__, __LINE__, chmask);

	for (i = 0; i < var_get_ch_count(); i++) {
		if (chmask & (1 << i)) nf_ai_set_rule(i);
	}
	return 0;
}

static int _proc_delay_ai_analytics_event(WRK_ID wrkid, CMM_MESSAGE_T *pmsg) 
{
	DELAY_NOTIFY_INFO *req_info = (DELAY_NOTIFY_INFO*)pmsg->data;
	gint notify_allloc_size = (DELAY_NOTIFY_INFO*)pmsg->param;
	gint *r = req_info->data;

	GTimeVal curr_time;
	GTimeVal notify_time;
	double diffTime;

	GTimeVal stime, etime, ftime;
	gint jpeg_w, jpeg_h;
	gint jpeg_size = 0;
	gchar *out_buffer = 0;	
	gboolean retVal = 0;
	gint ch, cnt;

	gint retry_cnt = 36;
	void *p = 0;

	memcpy(&notify_time, &req_info->timestamp, sizeof(GTimeVal));

	while(retry_cnt--)
	{
		gettimeofday(&curr_time, NULL);
		diffTime = (double)(curr_time.tv_sec-notify_time.tv_sec) + (double)((curr_time.tv_usec-notify_time.tv_usec)/1000000.0);

		if (diffTime < (double)3.00) {
			usleep(100000);
			continue;
		}

		if (req_info->msgid == INFY_DELAY_AI_EVENT)
		{
			ai_rule_event_t *pevt;

			ch = r[0];
			cnt = r[1];
			pevt = r + 2;

			stime.tv_sec = (glong)pevt[0].timestamp;
			stime.tv_usec = (glong)pevt[0].timestampl;
			etime.tv_sec = stime.tv_sec+1;
			etime.tv_usec = stime.tv_usec;
			// retVal = nf_play_get_thumbnail_jpeg(ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &out_buffer, &ftime,NF_SECOND_SIZE);
			if (!retVal) break;
		}
		else if (req_info->msgid == INFY_DELAY_AI_GENERIC_EVENT)
		{
			ai_generic_event_t *pevt;

			ch = r[0];
			cnt = r[1];
			pevt = r + 2;

			stime.tv_sec = (glong)pevt[0].timestamp;
			stime.tv_usec = (glong)pevt[0].timestampl;
			etime.tv_sec = stime.tv_sec+1;
			etime.tv_usec = stime.tv_usec;
			// retVal = nf_play_get_thumbnail_jpeg(ch, stime, etime, &jpeg_w, &jpeg_h, &jpeg_size, &out_buffer, &ftime,NF_SECOND_SIZE);
			if (!retVal) break;
		}

		p = imalloc(jpeg_size+notify_allloc_size);
		if (jpeg_size) memcpy(p, out_buffer, jpeg_size);
		memcpy(p+jpeg_size, req_info->data, notify_allloc_size);

		evt_send_to_local(req_info->msgid, jpeg_size, 1, p);
		break;
	}

	if (out_buffer) free(out_buffer);
	out_buffer = 0;

	if (req_info->data) ifree(req_info->data);
	req_info->data = 0;
	return 0;
}

static int _send_delay_ai_analytics_event(IMSG msgid, NF_NOTIFY_INFO *pinfo)
{
	CMMPORT cmmpt = wrk_get_cmmport(iwrk_delay_aievt);
	gint msg_cnt;

	DELAY_NOTIFY_INFO *delay_ninfo;

	if (!pinfo) return -1;
	if (pinfo->type != NF_NOTIFY_POINTER) return -1;

	msg_cnt = cmm_get_message_count(cmmpt);
	//g_message(">>>>>>parangi %s, %d, timestamp:%d, cnt:%d", __FUNCTION__, __LINE__, pinfo->timestamp.tv_sec, msg_cnt);
	if (msg_cnt >= GUI_CHANNEL_CNT*16) return -1;

	delay_ninfo = imalloc(sizeof(DELAY_NOTIFY_INFO));

	delay_ninfo->msgid = msgid;
	memcpy(&delay_ninfo->timestamp, &pinfo->timestamp, sizeof(GTimeVal));
	delay_ninfo->data = imalloc(pinfo->p.len);
	memcpy(delay_ninfo->data, pinfo->p.ptr, pinfo->p.len);

	wrk_run_msg(iwrk_delay_aievt, IMSG_NONE, pinfo->p.len, 1, delay_ninfo);
	return 0;
}

// Notify callback function.
CALLBACK static void analog_rec_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	_scm_update_record_led(pinfo);
	if(pinfo) _send_notify_msg(INFY_ANALOG_REC_NOTIFY, pinfo);
}

CALLBACK static void title_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	var_set_camtitle();	
	if(pinfo) _send_notify_msg(INFY_CAM_TITLE_NOTIFY, pinfo);
}

CALLBACK static void net_status_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	_scm_update_network_led(pinfo);
	if(pinfo) _send_notify_msg(INFY_NET_NOTIFY, pinfo);
}

CALLBACK static void rec_status_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_ENC_NOTIFY, pinfo);
}

CALLBACK static void disk_usage_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DISK_USAGE_NOTIFY, pinfo);
}

CALLBACK static void disk_full_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DISK_FULL_NOTIFY, pinfo);
}

CALLBACK static void disk_overwr_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DISK_OW_NOTIFY, pinfo);
}

static int _send_notify_msg_all(NF_NOTIFY_INFO *pinfo)
{
	_send_notify_msg(INFY_SYSDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_NETDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_AUDDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_DSKDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_CAMDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_USRDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_ALMDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_ACTDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_DSPDB_CHANGE_NOTIFY, pinfo);
	_send_notify_msg(INFY_RECDB_CHANGE_NOTIFY, pinfo);
	return 0;
}

CALLBACK static void sysdb_change_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	SCM_T *piscm = (SCM_T *)data;
	DMSG(9, "");

	if (pinfo) {
		switch(pinfo->d.params[0]) {
		case NF_SYSDB_CATE_SYS:
			DMSG(7, "");
            _check_clon_device();
			_scm_apply_live_audio_onoff();		
			_scm_license_init();
			nvm_update_oneself_nvr_info();
			_send_notify_msg(INFY_SYSDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_NET:
			DMSG(7, "");
			_send_notify_msg(INFY_NETDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_AUDIO:
			DMSG(7, "");
			_scm_apply_live_audio_ch();
			_scm_apply_buzzer_config();
			_send_notify_msg(INFY_AUDDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_DISK:
			DMSG(7, "");
			_send_notify_msg(INFY_DSKDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_CAM:
			DMSG(7, "");
			_set_ai_rule(pinfo->d.params[1], pinfo->d.params[2]);
			var_set_camtitle();	
            _scm_apply_ipcam_poe_onoff();
			_send_notify_msg(INFY_CAMDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_USR:
			DMSG(7, "");
			ssm_update_auth();
			_scm_apply_live_audio_onoff();
			ssm_reconfig_auto_logout();
			scm_regi_password_to_hdd();
			nvm_update_oneself_nvr_info();
			_send_notify_msg(INFY_USRDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_ALARM:
			DMSG(7, "");
			_send_notify_msg(INFY_ALMDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_ACT:
			DMSG(7, "");
			_send_notify_msg(INFY_ACTDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_DISP:
			DMSG(7, "");
			posx_reload_property();
			_send_notify_msg(INFY_DSPDB_CHANGE_NOTIFY, pinfo);
			_scm_push_spot_updating(piscm);
			break;
		case NF_SYSDB_CATE_REC:
			DMSG(7, "");
			_send_notify_msg(INFY_RECDB_CHANGE_NOTIFY, pinfo);
			break;
		case NF_SYSDB_CATE_FACTORY_DEFAULT:
			DMSG(7, "");
			_set_ai_rule(0xffff, SYSDB_SUB_CATE_CAM_AI);
			_scm_send_msg_to_viewer(INFY_DEFAULT_LOAD_CSTLAYOUT, 0);
			_init_gpu_mode();			
			_load_fisheye_param();
			_load_dlva_param();
			_send_notify_msg_all(pinfo);
			break;
		case NF_SYSDB_CATE_LOAD_DATA:
			DMSG(7, "");
			_set_ai_rule(0xffff, SYSDB_SUB_CATE_CAM_AI);
			_scm_send_msg_to_viewer(INFY_DEFAULT_LOAD_CSTLAYOUT, 0);
			_init_gpu_mode();
			_load_fisheye_param();
			_load_dlva_param();
			_send_notify_msg_all(pinfo);
			break;
		}
	}
}

CALLBACK static void covert_change_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_COVERT_NOTIFY, pinfo);
}

CALLBACK static void disk_smart_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_SMART_ERROR_NOTIFY, pinfo);
}

CALLBACK static void ipcam_rec_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_IPCAM_REC_NOTIFY, pinfo);
}

CALLBACK static void vloss_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	_update_novideo_mask(pinfo);
	if(pinfo) _send_notify_msg(INFY_VLOSS_NOTIFY, pinfo);
}

CALLBACK static void novideo_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	// blank
}

CALLBACK static void sensor_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	_scm_update_alarm_led(pinfo);
	if(pinfo) _send_notify_msg(INFY_SENSOR_NOTIFY, pinfo);
}

CALLBACK static void motion_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	int ch;

//	DMSG(9, "");
	
#if defined(USE_MOTION_MODULAR)
	SCM_T *piscm = (SCM_T *)data;
	if(pinfo) {
		for (ch = 0; ch < var_get_ch_count(); ch++) {
			if (pinfo->d.params[0] & (1 << ch))
				piscm->mmt[ch].motion_state = 1;
			else	
				piscm->mmt[ch].motion_state = 0;
		}
	}	
#else	
	if(pinfo) _send_notify_msg(INFY_MOTION_NOTIFY, pinfo);
#endif	
}

CALLBACK static void alarm_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_ALARM_NOTIFY, pinfo);
}

CALLBACK static void pnd_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	int ch;

	DMSG(9, "");
	
	if(pinfo){
		if (pinfo->d.params[1] < var_get_ch_count())
			_send_notify_msg(INFY_PND_NOTIFY, pinfo);
	}
}

CALLBACK static void pnd_rate_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	int ch;

	DMSG(9, "");
	
	if(pinfo){
		if (pinfo->d.params[1] < var_get_ch_count())	
			_send_notify_msg(INFY_PND_RATE_NOTIFY, pinfo);
	}
}

CALLBACK static void wan_status_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_WAN_NOTIFY, pinfo);
}

CALLBACK static void ddns_status_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DDNS_NOTIFY, pinfo);
}

CALLBACK static void writefail_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_WRITEFAIL_NOTIFY, pinfo);
}

CALLBACK static void exhaust_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_EXHAUST_NOTIFY, pinfo);
}

CALLBACK static void nodisk_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_NODISK_NOTIFY, pinfo);
}

CALLBACK static void smart_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_SMART_WARN_NOTIFY, pinfo);
}

CALLBACK static void raid_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    static int prev_status = -1;

	if (prev_status != pinfo->d.params[0])
	{
        if (pinfo->d.params[0] == 0)
        {
            _scm_send_msg_to_viewer(INFY_RAID_CHANGE_STATUS_RECOVER, 0);
        }
        else if (pinfo->d.params[0] == 1)
        {
            _scm_send_msg_to_viewer(INFY_RAID_CHANGE_STATUS_DEGRADE, 0);
        }
        else if (pinfo->d.params[0] == 2)
        {
            _scm_send_msg_to_viewer(INFY_RAID_CHANGE_STATUS_REBUILD, 0);
        }
        else if (pinfo->d.params[0] == 3)
        {
            _scm_send_msg_to_viewer(INFY_RAID_CHANGE_STATUS_BROKEN, 0);
       }    
	}
	
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DISK_RAID_CHANGE_STATUS_NOTIFY, pinfo);

	prev_status = pinfo->d.params[0];
}

CALLBACK static void sysfan_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_SYSFAN_NOTIFY, pinfo);
}

CALLBACK static void temperature_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_TEMPERATURE_NOTIFY, pinfo);
}

CALLBACK static void set_ip_conflict_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    DMSG(9, "");
    if(pinfo) _send_notify_msg(INFY_SET_IP_CONFLICT_NOTIFY, pinfo);
}

CALLBACK static void cam_ip_conflict_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    DMSG(9, "");
    if(pinfo) _send_notify_msg(INFY_CAM_IP_CONFLICT_NOTIFY, pinfo);
}

CALLBACK static void poe_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_POE_NOTIFY, pinfo);
}

CALLBACK static void poe_port_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_POE_PORT_NOTIFY, pinfo);
}

CALLBACK static void dvrloginfail_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DVRLOGINFAIL_NOTIFY, pinfo);
}

CALLBACK static void netloginfail_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_NETLOGINFAIL_NOTIFY, pinfo);
}

CALLBACK static void net_rxtx_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_NET_RXTX, pinfo);
}

CALLBACK static void buzzer_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_BUZZER_NOTIFY, pinfo);
}

CALLBACK static void ipchange_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_IP_CHANGED_NOTIFY, pinfo);
}

CALLBACK static void pnd_hub_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_PND_HUB_NOTIFY, pinfo);
}

CALLBACK static void poe_hub_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_POE_HUB_NOTIFY, pinfo);
}

CALLBACK static void motion_raw_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
//	DMSG(9, "");
	if(pinfo) _scm_set_mraw_data(pinfo);
}

CALLBACK static void ipcam_install_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_IPCAM_INSTALL_NOTIFY, pinfo);
}

CALLBACK static void remote_s1_fwup_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_REMOTE_S1_FWUP_NOTIFY, pinfo);
}

CALLBACK static void log_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	NF_LOG_DATA *log_data;
	gpointer msg;	

	DMSG(9, "");

	msg = pinfo->p.ptr;
	log_data = (NF_LOG_DATA*)msg;
    posx_put_live_log(log_data);

//	if(pinfo) _send_notify_msg(INFY_POS_LOG_NOTIFY, pinfo);
}

CALLBACK static void pos_dev_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DETECT_POSDEV_NOTIFY, pinfo);
}

CALLBACK static void pos_status_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_POS_STATUS_NOTIFY, pinfo);
}

CALLBACK static void pos_event_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_POS_EVENT_NOTIFY, pinfo);
}

CALLBACK static void aibox_setup_change_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
    DMSG(9, "");
    if(pinfo) _send_notify_msg(INFY_AIBOX_SETUP_CHANGE_NOTIFY, pinfo);
}

CALLBACK static void vca_event_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	SCM_T *piscm = (SCM_T *)data;
	ivca_rule_event_t *pevt;
	int *p;
	int ch, t , i;
	int zid, cid;
	int pattern;

	DMSG(9, "pinfo:%p", pinfo);
	
	if ( pinfo ) {
#if USE_VCA_EVT_MODULAR
		p = pinfo->p.ptr;
		pevt = p + 2;
		ch =(gint)pevt->ch;
		
		if (p[0] >= NUM_ACTIVE_CH)
		{		
			_send_notify_msg(INFY_VCA_ANALYZE_NOTIFY, pinfo);
		}
		else 
		{
    		if (!get_vca_enable(ch)) return;
    		
			VMT_LOCK();
			
			for(i = 0; i < p[1] ; i++ , pevt++) {
				pattern = vaa_parse_event(ch, pevt, &zid, &cid);
				if (pattern!=99) piscm->vmt[pevt->ch].vca_time[pattern] = 50;	// freq : 100ms -> 5sec		
			}
		
			VMT_UNLOCK();
			
			iva_cntr_put_classic_va_event(pinfo);
			_send_notify_msg(INFY_VCA_EVENT_NOTIFY, pinfo);
		}
#endif
		DMSG(9, "");
	}		
}

CALLBACK static void vca_trackinfo_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if ( pinfo )
		_send_notify_msg(INFY_VCA_TRACKINFO_NOTIFY, pinfo);
}

CALLBACK static void vca_meta_data_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if ( pinfo )
		_send_notify_msg(INFY_VCA_META_DATA_NOTIFY, pinfo);
}

CALLBACK static void vca_counter_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if ( pinfo )
		_send_notify_msg(INFY_VCA_COUNTER_NOTIFY, pinfo);
}

CALLBACK static void ai_event_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static long event_timestamp[GUI_CHANNEL_CNT] = {0, };
	SCM_T *piscm = (SCM_T *)data;

	ai_rule_event_t *pevt;
	int *p;
	int ch, t , i;
	int zid, cid;
	int pattern;

	DMSG(9, "pinfo:%p", pinfo);

	if ( pinfo ) {
		p = pinfo->p.ptr;
		pevt = p + 2;
		ch =(gint)pevt->ch;
		
		if (p[0] >= NUM_ACTIVE_CH)
		{		
			_send_notify_msg(INFY_AI_ANALYZE_NOTIFY, pinfo);
		}
		else 
		{
    		if (!get_ai_enable(ch)) return;
    		
#if USE_DVABX_EVT_MODULAR    		
			DXMT_LOCK();
			for(i = 0; i < p[1] ; i++ , pevt++) {
				pattern = dvaa_parse_event(ch, pevt, &zid, &cid);
				if (pattern!=99) piscm->dxmt[pevt->ch].dvabx_time[pattern] = 50;	// freq : 100ms -> 5sec		
			}
			DXMT_UNLOCK();
#endif			
			iva_cntr_put_ai_dvabx_event(pinfo);
			_send_notify_msg(INFY_AI_EVENT_NOTIFY, pinfo);

			if (p[1] > 0)
			{
				if (event_timestamp[ch] != pinfo->timestamp.tv_sec) {
					// _send_delay_ai_analytics_event(INFY_DELAY_AI_EVENT, pinfo);
					event_timestamp[ch] = pinfo->timestamp.tv_sec;
				}
			}
		}
		DMSG(9, "");
	}		
}

CALLBACK static void ai_trackinfo_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if ( pinfo )
		_send_notify_msg(INFY_AI_TRACKINFO_NOTIFY, pinfo);
}

CALLBACK static void ai_counter_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if ( pinfo )
		_send_notify_msg(INFY_AI_COUNTER_NOTIFY, pinfo);
}

CALLBACK static void ai_generic_event_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static long event_timestamp[GUI_CHANNEL_CNT] = {0, };
	SCM_T *piscm = (SCM_T *)data;

	ai_generic_event_t *pevt;
	int *p;
	int ch;

	DMSG(9, "pinfo:%p", pinfo);
	
	if ( pinfo ) {
		p = pinfo->p.ptr;
		ch = p[0];
		pevt = p + 2;

#if USE_DVABX_EVT_MODULAR		
		DXMT_LOCK();
		piscm->dxmt[ch].dvabx_time[ITX_RULETYPE_GENERIC] = 50;	// freq : 100ms -> 5sec
		DXMT_UNLOCK();
#endif
		_send_notify_msg(INFY_AI_GENERIC_EVENT_NOTIFY, pinfo);

		if (p[1] > 0)
		{
			if (event_timestamp[ch] != pinfo->timestamp.tv_sec) {
				// _send_delay_ai_analytics_event(INFY_DELAY_AI_GENERIC_EVENT, pinfo);
				event_timestamp[ch] = pinfo->timestamp.tv_sec;
			}
		}
	}
		DMSG(9, "");
}		

CALLBACK static void ai_keep_alive_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	if ( pinfo )
		_send_notify_msg(INFY_AI_KEEP_ALIVE_NOTIFY, pinfo);
}

static int _read_conf(SCM_T *piscm)
{
	int ret;
	ret = icf_get_value_by_int("scm", "dmsg_notify");
	if (ret != -1) DBG_USE(ret);
	return 0;
}

CALLBACK static void vca_trackinfo_cb(int ch , int cnt, ivcam_obj_t* data)
{
	NF_NOTIFY_INFO notify;
	void *p;
	int size;

	DMSG(9, "");

	if ((ch < 0) || (ch >= GUI_CHANNEL_CNT)) {
		DMSG(9, "not supported channel : %d", ch);
		return;		
	}

	if (!cnt) {
		DMSG(9, "vca cnt zero");
		return;		
	}

	size = 2 * sizeof(int) + (size_t)cnt * sizeof(ivcam_obj_t);
	p = imalloc(size);
	
	if (p) {
		((int*)p)[0] = ch;
		((int*)p)[1] = cnt;	
		memcpy((int*)p+2, (void*)data, (size_t)cnt * sizeof(ivcam_obj_t));

		memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
		notify.type = NF_NOTIFY_POINTER;		
		notify.p.ptr = p;	
		notify.p.len = (guint)size;
		_send_notify_msg(INFY_VCA_TRACKINFO_NOTIFY, &notify);
		
		ifree(p);		
	}
}

CALLBACK static void deeplearning_object_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DEEPLEARNING_OBJECT_NOTIFY, pinfo);
}

CALLBACK static void deeplearning_event_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	static long event_timestamp[GUI_CHANNEL_CNT] = {0, };
	SCM_T *piscm = (SCM_T *)data;

	DVA_MSG *dva_event = (DVA_MSG*)pinfo->p.ptr;

	DMSG(9, "");
	DMT_LOCK();
	piscm->dmt[dva_event->ch].dva_time = 50;	// freq : 100ms -> 5sec	
	DMT_UNLOCK();
	iva_cntr_put_ai_builtin_event(pinfo);

	if(pinfo) {
		_send_notify_msg(INFY_DEEPLEARNING_EVENT_NOTIFY, pinfo);

		if (event_timestamp[dva_event->ch] != pinfo->timestamp.tv_sec) {
			// _send_delay_ai_analytics_event(INFY_DELAY_DEEPLEARNING_EVENT, pinfo);
			event_timestamp[dva_event->ch] = pinfo->timestamp.tv_sec;
		}
	}
}

CALLBACK static void deeplearning_counter_notify_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_DEEPLEARNING_COUNTER_NOTIFY, pinfo);
	}

CALLBACK static void long_distance_cb(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	DMSG(9, "");
	if(pinfo) _send_notify_msg(INFY_LONG_DISTANCE_NOTIFY, pinfo);
}


////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_init_notify(SCM_T *piscm)
{
	_read_conf(piscm);
	return 0;
}

void _scm_register_syscb()
{
	DMSG(9, "");
	// registrer noti cb
	nf_notify_connect_cb("sysdb_cam_title", title_notify_cb, NULL);
	nf_notify_connect_cb("net_status", net_status_notify_cb, NULL);  
	nf_notify_connect_cb("enc_status", rec_status_notify_cb, NULL); 
	nf_notify_connect_cb("disk_usage", disk_usage_notify_cb, NULL);
	nf_notify_connect_cb("disk_full", disk_full_notify_cb, NULL);
	nf_notify_connect_cb("disk_overwr", disk_overwr_notify_cb, NULL);
	nf_notify_connect_cb("sysdb_change", sysdb_change_notify_cb, &iscm);
	nf_notify_connect_cb("sysdb_covert", covert_change_notify_cb, NULL);
	nf_notify_connect_cb("disk_smart", disk_smart_notify_cb, NULL);
	nf_notify_connect_cb("analog_rec", analog_rec_notify_cb, NULL);
	nf_notify_connect_cb("ipcam_rec", ipcam_rec_notify_cb, NULL);
	nf_notify_connect_cb("vloss", vloss_notify_cb, NULL);
	nf_notify_connect_cb("sensor", sensor_notify_cb, NULL);
	nf_notify_connect_cb("motion", motion_notify_cb, &iscm);
	nf_notify_connect_cb("alarm", alarm_notify_cb, NULL);
	nf_notify_connect_cb("pnd_event", pnd_notify_cb, NULL);
	nf_notify_connect_cb("pnd_progress", pnd_rate_notify_cb, NULL);
	nf_notify_connect_cb("net_wan_status", wan_status_notify_cb, NULL);
	nf_notify_connect_cb("net_ddns_status", ddns_status_notify_cb, NULL);
	nf_notify_connect_cb("disk_write_fail", writefail_notify_cb, NULL);
	nf_notify_connect_cb("disk_exhaust", exhaust_notify_cb, NULL);
	nf_notify_connect_cb("disk_nodisk", nodisk_notify_cb, NULL);
	nf_notify_connect_cb("disk_smart_reqchk", smart_notify_cb, NULL);
	nf_notify_connect_cb("disk_raid", raid_notify_cb, NULL);
	nf_notify_connect_cb("sys_fan", sysfan_notify_cb, NULL);
	nf_notify_connect_cb("sys_temperature", temperature_notify_cb, NULL);
	nf_notify_connect_cb("sys_poe_status", poe_notify_cb, NULL);
	nf_notify_connect_cb("sys_poe_port", poe_port_notify_cb, NULL);	
	nf_notify_connect_cb("dvr_login_fail", dvrloginfail_notify_cb, NULL);
	nf_notify_connect_cb("net_login_fail", netloginfail_notify_cb, NULL);
	nf_notify_connect_cb("net_rxtx", net_rxtx_notify_cb, NULL);
	nf_notify_connect_cb("buzzer", buzzer_notify_cb, NULL);
	nf_notify_connect_cb("net_ip_changed", ipchange_notify_cb, NULL);
	nf_notify_connect_cb("pnd_hub_status", pnd_hub_notify_cb, NULL);	
	nf_notify_connect_cb("sys_poe_status_hub", poe_hub_notify_cb, NULL);		
	nf_notify_connect_cb("mraw_data", motion_raw_notify_cb, NULL);		
	nf_notify_connect_cb("ipcam_slist", ipcam_install_notify_cb, NULL);		
	nf_notify_connect_cb("net_s1_fw_up", remote_s1_fwup_notify_cb, NULL);		
	nf_notify_connect_cb("log", log_notify_cb, NULL);
	nf_notify_connect_cb("pos_dev", pos_dev_notify_cb, NULL);
	nf_notify_connect_cb("sys_pos_changed", pos_status_notify_cb, NULL);	
	nf_notify_connect_cb("pos_text_event", pos_event_notify_cb, NULL);
	nf_notify_connect_cb("set_ip_conflict", set_ip_conflict_notify_cb, NULL);
	nf_notify_connect_cb("cam_ip_conflict", cam_ip_conflict_notify_cb, NULL);
	nf_notify_connect_cb("aibox_db_change", aibox_setup_change_notify_cb, NULL);
	nf_notify_connect_cb("long_distance", long_distance_cb, NULL);	

	// iwrk_delay_aievt = wrk_create_worker(_proc_delay_ai_analytics_event, 0);
	// wrk_change_sleep_time(iwrk_delay_aievt, 20000);

	if (vaa_is_supported()) {
		nf_notify_connect_cb("vca_event", vca_event_notify_cb, &iscm);
		nf_notify_connect_cb("vca_trackinfo", vca_trackinfo_notify_cb, NULL);
//		nf_notify_connect_cb("vca_meta_data", vca_meta_data_notify_cb, NULL);
		nf_notify_connect_cb("vca_counter", vca_counter_notify_cb, NULL);

//		set_meta_data_callback(vca_trackinfo_cb, 0);		
	}	

	if (dvaa_is_supported()) {
		nf_notify_connect_cb("ai_event", ai_event_notify_cb, &iscm);
		nf_notify_connect_cb("ai_trackinfo", ai_trackinfo_notify_cb, NULL);
		nf_notify_connect_cb("ai_counter", ai_counter_notify_cb, NULL);

		nf_notify_connect_cb("ai_generic_event", ai_generic_event_notify_cb, &iscm);
		nf_notify_connect_cb("ai_keep_alive", ai_keep_alive_notify_cb, &iscm);
	}	

	if (ivsc.dfunc.support_dlva_itx) {
		nf_notify_connect_cb("dva_object", deeplearning_object_notify_cb, &iscm);
		nf_notify_connect_cb("dva_event", deeplearning_event_notify_cb, &iscm);
		nf_notify_connect_cb("dva_counter", deeplearning_counter_notify_cb, &iscm);
	}

	// please, add to _free_nfnotify_data() in uxm.c
}

int _scm_init_mot_timer(SCM_T *piscm)
{
#if defined(USE_MOTION_MODULAR)
	iscm.tmr_mmt = _scm_add_timeout(&iscm, 100, _proc_motion_modular, piscm);
	return iscm.tmr_mmt;
#else
	return 0;
#endif	
}

int _scm_init_vca_event_timer(SCM_T *piscm)
{
#if defined(USE_VCA_EVT_MODULAR)
	piscm->mtx_vmt = g_mutex_new();
	iscm.tmr_vmt = _scm_add_timeout(&iscm, 100, _proc_vca_event_modular, piscm);
	return iscm.tmr_vmt;
#else
	return 0;
#endif	
}

int _scm_init_dva_event_timer(SCM_T *piscm)
{
#if defined(USE_DVA_EVT_MODULAR)
	piscm->mtx_dmt = g_mutex_new();
	iscm.tmr_dmt = _scm_add_timeout(&iscm, 100, _proc_dva_event_modular, piscm);
	return iscm.tmr_dmt;
#else
	return 0;
#endif	
}

int _scm_init_dvabx_event_timer(SCM_T *piscm)
{
#if defined(USE_DVABX_EVT_MODULAR)
	piscm->mtx_dxmt = g_mutex_new();
	iscm.tmr_dxmt = _scm_add_timeout(&iscm, 100, _proc_dvabx_event_modular, piscm);
	return iscm.tmr_dxmt;
#else
	return 0;
#endif	
}

int _scm_inform_db_changing(TRANSACTION_E tra)
{
	gint index;

	for (index = 0; index < NF_SYSDB_CATE_NR; index++)
		nf_notify_fire_params("sysdb_change", index, 0, 0, 0);

	if(tra == TRA_FACTORY_DEFAULT)	
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_FACTORY_DEFAULT, 0, 0, 0);
	else
		nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_LOAD_DATA, 0, 0, 0);

	return 0;
}

int _scm_inform_smart_alarmon()
{
	nf_notify_fire_params("disk_smart", 4, 0, 0, 0);
	return 0;
}

int _scm_inform_smart_alarmoff()
{
	nf_notify_fire_params("disk_smart", 0, 0, 0, 0);
	return 0;
}

int _scm_check_novideo(SCM_T *piscm)
{
	NF_NOTIFY_INFO buf;
	BITMASK novideo = 0xFFFFFFFF;
	
	var_set_novideo_mask(novideo);

	if (nf_notify_get_update_time("vloss") != 0) {
    	scm_get_vloss_data(&buf);
    	scm_req_novideo_data();
    }

	return 0;
}

////////////////////////////////////////////////////////////
//
// notify emulating
//

#if defined(_IPX_MODEL_UX)
#include "nf_ipcam_defs.h"
#else
#include "ipx_cam_api.h"
#endif
static WRK_ID _test_pndevent = 0;
static WRK_ID _test_pndrate = 0;
static WRK_ID _test_motion = 0;
static WRK_ID _test_vloss = 0;
static WRK_ID _test_vca_track = 0;
static WRK_ID _test_vca_event = 0;

static int _proc_emul_pndevent(WRK_ID wrk, CMM_MESSAGE_T *pmsg)
{
	NF_NOTIFY_INFO info;
	int table[PND_TYPE_MAX] = {
		PND_TYPE_UNPLUGGED,
		PND_TYPE_PLUGGED,
		PND_TYPE_MAC_RESOLVED,
		PND_TYPE_IP_REQUESTED,
		PND_TYPE_IP_DONE,
		PND_TYPE_SETUP_REQUESTED,
		PND_TYPE_SETUP_DONE,
		PND_TYPE_VIDEO_START,
		PND_TYPE_TIMEOUT,
		PND_TYPE_UNKNOWN,
		PND_TYPE_UNSUPPORTED,
		PND_TYPE_CONNECTION_FAIL,
		PND_TYPE_LOGIN_FAIL,
		PND_TYPE_CONFIG_FAIL,
		PND_TYPE_STREAM_FAIL,
	};

// pattern 1
#if 0
	int pnd;
	int ch;
	int cnt = 0;

	memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
	info.d.params[0] = 0;
	while (1) {
		pnd = ifn_rand() % PND_TYPE_MAX;
		info.d.params[0] = table[pnd];
		ch = ifn_rand() % 8;
		info.d.params[1] = ch;
		
		DMSG(1, "CND = %d, EVENT = %d, CH = %d\n", ++cnt, pnd, ch);
		_send_notify_msg(INFY_PND_NOTIFY, &info);

		usleep(500000);
	}
#endif

// pattern 2
#if 0
	int pnd;
	int ch;
	int cnt = 0;

	while (1) {
		memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
		ch = ifn_rand() % 8;

		info.d.params[0] = PND_TYPE_PLUGGED;
		info.d.params[1] = ch;
	
		DMSG(7, "CND = %d, EVENT = %d, CH = %d\n", ++cnt, pnd, ch);
		_send_notify_msg(INFY_PND_NOTIFY, &info);

		for (pnd = 0; pnd < 10; ++pnd) {
			_send_notify_msg(INFY_PND_RATE_NOTIFY, &info);
			usleep(500000);
		}

		info.d.params[0] = PND_TYPE_UNPLUGGED;
		info.d.params[1] = ch;
	
		DMSG(7, "CND = %d, EVENT = %d, CH = %d\n", ++cnt, pnd, ch);
		_send_notify_msg(INFY_PND_NOTIFY, &info);


		sleep(1);
	}

#endif


	int pnd;
	int ch;
	int cnt = 0;
	FILE *fp;

	while (1) {
		fp = fopen("./data/gui/login_user.itx", "w");
		printf("%d, %p\n", ++cnt, fp);

		usleep(100000);
	}



}

static int _proc_emul_pndrate(WRK_ID wrk, CMM_MESSAGE_T *pmsg)
{
	NF_NOTIFY_INFO info;
	int ch;
	int rate;
	int cnt = 0;
	int i;
	

	memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
	info.d.params[0] = 0;
	while (1) {
		
		DMSG(7, "CNT = %d, CH = %d, RATE = %d\n", ++cnt, ch, rate);
		for (i = 0; i < 8; ++i) {
			ch = i;

			rate = ifn_rand() % 100;
			info.d.params[0] = rate;
			info.d.params[2] = cnt;
			info.d.params[1] = ch; 
			_send_notify_msg(INFY_PND_RATE_NOTIFY, &info);
		}

		usleep(50000);
	}
}


static int _proc_emul_motion(WRK_ID wrk, CMM_MESSAGE_T *pmsg)
{
	NF_NOTIFY_INFO info;
	int ch;
	int mask;
	int cnt = 0;
	
	memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
	info.d.params[0] = 0;
	while (1) {
		ch = ifn_rand() % 8;
		info.d.params[0] = 0;
		info.d.params[0] |= (1 << ch);
		
		DMSG(7, "CNT = %d, CH = %d\n", ++cnt, ch);
//		info.d.params[2] = cnt;
		_send_notify_msg(INFY_MOTION_NOTIFY, &info);

		usleep(200000);
	}
	return 0;
}

static int _proc_emul_vloss(WRK_ID wrk, CMM_MESSAGE_T *pmsg)
{
	NF_NOTIFY_INFO info;
	int ch;
	int mask;
	int cnt = 0;
	

	memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
	info.d.params[0] = 0;
	while (1) {
		mask = ifn_rand() % 256;		// 2^8
		info.d.params[0] = mask;
		
		DMSG(7, "CNT = %d, CH = %d, RATE = 0x%08x\n", ++cnt, ch, mask);
		info.d.params[2] = cnt;
		_send_notify_msg(INFY_VLOSS_NOTIFY, &info);

		usleep(50000);
	}
}

static int _proc_emul_vca_track_info(WRK_ID wrk, CMM_MESSAGE_T *pmsg)
{
	static gint x = 0;
	static gint y = 0;
	static gint w = 200;
	static gint h = 100;

	NF_NOTIFY_INFO info;
	size_t size;
	void *r;

	ivcam_obj_t m_TrackObjs[10];
	gint i;
	
	memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));

	while (1) 
	{
		if (x + w + 100 >= 1920*2) {
			x = 0;			
		}

		for (i = 0; i < 4; i++)
		{
			m_TrackObjs[i].mobj.rc.x = x;
			m_TrackObjs[i].mobj.rc.y = y + (i*300);
			m_TrackObjs[i].mobj.rc.w = w;
			m_TrackObjs[i].mobj.rc.h = h;			
		}

		size = sizeof(int)*2 + (size_t)sizeof(ivcam_obj_t)*4;	
		r = imalloc(size);
		((int *)r)[0] = 0;
		((int *)r)[1] = 4;
		memcpy((int *)r+2, (void *)m_TrackObjs, (size_t)sizeof(ivcam_obj_t)*4);	

		info.type = NF_NOTIFY_POINTER;
		info.p.ptr = r;
		info.p.len = (guint)size;		
		_send_notify_msg(INFY_VCA_TRACKINFO_NOTIFY, &info);
		ifree(r);
		
		x += 100;
		usleep(200000);
	}
	return 0;
}

static int _proc_emul_vca_event(WRK_ID wrk, CMM_MESSAGE_T *pmsg)
{
	int ch;
	int rule_id;

	while (1) 
	{
		ch = ifn_rand() % 8;
		rule_id = ifn_rand() % 7;

		VMT_LOCK();	
		iscm.vmt[ch].vca_time[rule_id] = 50;
		VMT_UNLOCK();	
		
		usleep(2000000);
	}
	return 0;
}

static int _test_wanfail()
{
	NF_NOTIFY_INFO info;
	int ch;
	static int mask = 0;
	int cnt = 0;
	

	memset(&info, 0x00, sizeof(NF_NOTIFY_INFO));
//	while (1) {
		mask = !mask;
		info.d.params[0] = mask;
		
		DMSG(7, "WAN FAIL = %d\n", info.d.params[0]);
		_send_notify_msg(INFY_WAN_NOTIFY, &info);

//		sleep(5);
//	}
}

int scm_emul_notify()
{
//	_test_pndevent = wrk_create_worker(_proc_emul_pndevent, 0);
//	_test_motion = wrk_create_worker(_proc_emul_motion, 0);
//	_test_pndrate = wrk_create_worker(_proc_emul_pndrate, 0);
//	_test_vloss = wrk_create_worker(_proc_emul_vloss, 0);
//	_test_vca_track = wrk_create_worker(_proc_emul_vca_track_info, 0);
//	_test_vca_event = wrk_create_worker(_proc_emul_vca_event, 0);

//	wrk_run_once(_test_pndevent);
//	wrk_run_once(_test_pndrate);
//	wrk_run_once(_test_motion);
//	wrk_run_once(_test_vloss);
//	wrk_run_once(_test_vca_track);
//	wrk_run_once(_test_vca_event);

//	_test_wanfail();
	return 0;
}


////////////////////////////////////////////////////////////
//
// public interfaces 
//

int scm_set_notifycb()
{
	_scm_register_syscb();
}

int scm_req_analog_data()
{
	NF_NOTIFY_INFO *pnotify;

	pnotify = nf_notify_get("analog_rec");	
	if(pnotify) {
		_send_notify_msg(INFY_ANALOG_REC_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	} 
	return -1;
}

int scm_req_ipcam_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("ipcam_rec");	
	if(pnotify) {
		_send_notify_msg(INFY_IPCAM_REC_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_vloss_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("vloss");	
	if(pnotify) {
		_update_novideo_mask(pnotify);
		_send_notify_msg(INFY_VLOSS_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_novideo_data()
{
	NF_NOTIFY_INFO notify;

	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = var_get_novideo_mask();
	_send_notify_msg(INFY_NOVIDEO_NOTIFY, &notify);
	return 0;
}

int scm_req_net_status_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_status");
	if(pnotify) {
		_send_notify_msg(INFY_NET_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_net_rxtx_status_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_rxtx");
	if(pnotify) {
		_send_notify_msg(INFY_NET_RXTX, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_disk_full_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_full");
	if(pnotify) {
		_send_notify_msg(INFY_DISK_FULL_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	} 
	return -1;
}

int scm_req_disk_usage_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_usage");
	if(pnotify) {
		_send_notify_msg(INFY_DISK_USAGE_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_disk_ow_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_overwr");
	if(pnotify) {
		_send_notify_msg(INFY_DISK_OW_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_disk_smart_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_smart");
	if(pnotify) {
		_send_notify_msg(INFY_SMART_ERROR_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_disk_raid_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_raid");

	if(pnotify) {
		_send_notify_msg(INFY_DISK_RAID_CHANGE_STATUS_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}

	return -1;
}

int scm_req_sensor_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sensor");
	if(pnotify) {
		_send_notify_msg(INFY_SENSOR_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_alarm_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("alarm");
	if(pnotify) {
		_send_notify_msg(INFY_ALARM_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

#if defined(USE_MOTION_MODULAR)
int scm_req_motion_event_data()
{
	NF_NOTIFY_INFO notify;

	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = _get_motion_data(&iscm);
	_send_notify_msg(INFY_MOTION_NOTIFY, &notify);
	return 0;	
}
#else
int scm_req_motion_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("motion");
	if(pnotify) {
		_send_notify_msg(INFY_MOTION_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}
#endif

int scm_req_pnd_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("pnd_event");
	if(pnotify) {
		_send_notify_msg(INFY_PND_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_all_pnd_event_data()
{
    int i;
	NF_NOTIFY_INFO notify;
    
    for (i = 0; i < var_get_ch_count(); i++)
    {
        memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
        notify.d.params[1] = i;
    	notify.d.params[0] = get_pnd_status(i);
    	_send_notify_msg(INFY_PND_NOTIFY, &notify);        
    }
	
	return 0;
}


int scm_req_wan_status_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_wan_status");
	if(pnotify) {
		_send_notify_msg(INFY_WAN_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_ddns_status_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_ddns_status");
	if(pnotify) {
		_send_notify_msg(INFY_DDNS_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_writefail_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_write_fail");
	if(pnotify) {
		_send_notify_msg(INFY_WRITEFAIL_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_exhaust_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_exhaust");
	if(pnotify) {
		_send_notify_msg(INFY_EXHAUST_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_nodisk_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_nodisk");
	if(pnotify) {
		_send_notify_msg(INFY_NODISK_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_smart_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_smart_reqchk");
	if(pnotify) {
		_send_notify_msg(INFY_SMART_WARN_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_sysfan_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_fan");
	if(pnotify) {
		_send_notify_msg(INFY_SYSFAN_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_termperature_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_temperature");
	if(pnotify) {
		_send_notify_msg(INFY_TEMPERATURE_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_poe_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_poe_status");
	if(pnotify) {
		_send_notify_msg(INFY_POE_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_poe_port_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_poe_port");
	if(pnotify) {
		_send_notify_msg(INFY_POE_PORT_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_dvrloginfail_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("dvr_login_fail");
	if(pnotify) {
		_send_notify_msg(INFY_DVRLOGINFAIL_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_netloginfail_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_login_fail");
	if(pnotify) {
		_send_notify_msg(INFY_NETLOGINFAIL_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_pnd_hub_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("pnd_hub_status");
	if(pnotify) {
		_send_notify_msg(INFY_PND_HUB_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_audio_ch_event_data()
{
	NF_NOTIFY_INFO notify;
	BITMASK mask = 0;
	
	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = (1 << var_get_live_audio_ch());
	_send_notify_msg(INFY_AUDIOCH_NOTIFY, &notify);
	return 0;
}

int scm_req_mic_out_event_data()
{
	NF_NOTIFY_INFO notify;
	
	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = var_get_mic_out_mask();
	_send_notify_msg(INFY_MICOUT_NOTIFY, &notify);
	return 0;
}

int scm_req_ipcam_install_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("ipcam_slist");
	if(pnotify) {
		_send_notify_msg(INFY_IPCAM_INSTALL_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_req_remote_s1_fwup_event_data()
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_s1_fw_up");
	if(pnotify) {
		_send_notify_msg(INFY_REMOTE_S1_FWUP_NOTIFY, pnotify);
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_analog_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("analog_rec");	
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	} 
	return -1;
}

int scm_get_ipcam_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("ipcam_rec");	
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_vloss_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("vloss");
	if (pnotify) {
		_update_novideo_mask(pnotify);
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_novideo_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO notify;

	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = var_get_novideo_mask();
	memcpy(buf, &notify, sizeof(NF_NOTIFY_INFO));
	return 0;
}

int scm_get_net_status_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_status");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}


int scm_get_disk_full_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_full");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	} 
	return -1;
}


int scm_get_disk_usage_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_usage");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_vloss_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("vloss");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}


int scm_get_sensor_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sensor");

	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

#if defined(USE_MOTION_MODULAR)
int scm_get_motion_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO notify;

	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = _get_motion_data(&iscm);
	memcpy(buf, &notify, sizeof(NF_NOTIFY_INFO));
	return 0;	
}
#else
int scm_get_motion_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("motion");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}
#endif

int scm_get_alarm_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("alarm");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_pnd_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("pnd_event");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_wan_status_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_wan_status");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_ddns_status_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_ddns_status");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_writefail_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_write_fail");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_exhaust_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_exhaust");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_nodisk_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_nodisk");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_smart_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_smart");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_smart_reqchk_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_smart_reqchk");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_disk_raid_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_raid");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}


int scm_get_sysfan_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_fan");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_termperature_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_temperature");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_poe_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_poe_status");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_poe_port_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("sys_poe_port");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_dvrloginfail_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("dvr_login_fail");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_netloginfail_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_login_fail");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_net_set_ipconflict_event_data(NF_NOTIFY_INFO *buf)
{
    NF_NOTIFY_INFO *pnotify = NULL;

    pnotify = nf_notify_get("set_ip_conflict");
    if(pnotify) {
        memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
        nf_notify_free(pnotify);
        return 0;
    }

    return -1;
}

int scm_get_net_cam_ipconflict_event_data(NF_NOTIFY_INFO *buf)
{
    NF_NOTIFY_INFO *pnotify = NULL;

    pnotify = nf_notify_get("cam_ip_conflict");
    if(pnotify){
        memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
        nf_notify_free(pnotify);
        return 0;
    }

    return -1;
}

int scm_get_audio_ch_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO notify;
	BITMASK mask = 0;

	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = (1 << var_get_live_audio_ch());
	memcpy(buf, &notify, sizeof(NF_NOTIFY_INFO));
	return 0;
}

int scm_get_mic_out_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO notify;

	memset(&notify, 0x00, sizeof(NF_NOTIFY_INFO));
	notify.d.params[0] = var_get_mic_out_mask();
	memcpy(buf, &notify, sizeof(NF_NOTIFY_INFO));
	return 0;
}

int scm_get_disk_ow_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("disk_overwr");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_pnd_hub_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("pnd_hub_status");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_ipcam_install_event_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("ipcam_slist");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_net_rxtx_status_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_rxtx");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_remote_s1_fwup(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("net_s1_fw_up");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_get_ai_keep_alive_data(NF_NOTIFY_INFO *buf)
{
	NF_NOTIFY_INFO *pnotify = NULL;

	pnotify = nf_notify_get("ai_keep_alive");
	if (pnotify) {
		memcpy(buf, pnotify, sizeof(NF_NOTIFY_INFO));
		nf_notify_free(pnotify);
		return 0;
	}
	return -1;
}

int scm_notify_to_system(const gchar *property_name, guint param)
{
	nf_notify_fire_params(property_name, param, 0, 0, 0);
	return 0;
}

int scm_inform_to_system(const gchar *property_name, guint param)
{
	nf_notify_fire_params(property_name, param, 0, 0, 0);
	return 0;
}

int scm_notify_to_viewer(WEBMSG_TO_VW msg)
{
	DMSG(7, "");
	switch (msg) {
	case WEBMSG_PASS_INIT:
		DMSG(7, "");
		_scm_send_msg_to_viewer(INFY_PASSWD_INIT_BY_WEB, 0);
		break;
	}
	return 0;
}

