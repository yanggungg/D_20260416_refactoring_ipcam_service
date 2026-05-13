#include <glib.h>
#include "iux_afx.h"
#include "nf_api_openmode.h"
#include "nfdal.h"
#include "cmm.h"
#include "xmm.h"
#include "nvm.h"
#include "nvm_json.h"
#include "ix_func.h"


DECLARE DBG_SYSTEM;

//describe  DBG_LEVEL
//9 : loop message
//8 : global print
//7 : private print
//5 : api call print
//1 : 1time print
//0 : off

#define DBG_LEVEL		0
#define DBG_MODULE		"NVM"

#define NVM_LOCK()		g_mutex_lock(invm.mtx)
#define NVM_UNLOCK()	g_mutex_unlock(invm.mtx)


////////////////////////////////////
//data struct
////////////////////////////////////

typedef struct _NVM_T_ NVM_T;
struct _NVM_T_
{
	GMutex *mtx;
	GThread *th;
    NVR_DATA_T nvr_data[NVM_MAX_NVR_SLOT_CNT];
	CMMPORT	cmmpt;
	int		opt_ka;
};


////////////////////////////////////////////////////////////
//
// private variable
//

static NVM_T invm;

////////////////////////////////////
//private
////////////////////////////////////

static void _print_cam_info(CAMERA_INFO_T *data, int dbg_lv)
{
    DMSG(dbg_lv, "camera id : %s", data->id);
    DMSG(dbg_lv, "camera pw : %s", data->pw);
    DMSG(dbg_lv, "camera ch : %d", data->ch);
    DMSG(dbg_lv, "camera addr : %s", data->url);
    DMSG(dbg_lv, "camera port : %d", data->port);
    DMSG(dbg_lv, "camera model : %s", data->model);
    DMSG(dbg_lv, "camera state : %d", data->state);
    DMSG(dbg_lv, "camera vcam : %d", data->virtual_camera);
    DMSG(dbg_lv, "camera vcam_addr1 : %s", data->vcam_rtsp_addr[0]);
    DMSG(dbg_lv, "camera vcam_addr2 : %s", data->vcam_rtsp_addr[1]);
    DMSG(dbg_lv, "//////////////CAM %d end///////////////", data->ch);
}

static void _print_nvr_data(NVR_DATA_T *data, int nvr_lv, int cam_lv)
{
    int i;
    
    DMSG(nvr_lv, "//////////////nvm nvr data///////////////");
    DMSG(nvr_lv, "nvr addr : %s", data->hostname);
    DMSG(nvr_lv, "nvr port : %d", data->http_port);
    
    DMSG(nvr_lv, "//////////////nvm camera data///////////////");
    for (i= 0; i < data->active_ch_cnt; i++)
    {
        DMSG(cam_lv, "//////////////CAM %d start///////////////", i+1);
        _print_cam_info(&data->cam_info[i], cam_lv);
    }
}

static NFOpenmodeCamInfo *_find_matched_info_by_ch(NFOpenmodeDeviceList *dlist, gint ch)
{
	NFOpenmodeCamInfo *info;
	gint i;

	info = dlist->head;
	if (info == 0) return NULL;

	for (i = 0; i < dlist->entry_cnt; i++)
	{
		if (info->ch == ch) return info;		
		
		info = info->next;
	}

	return NULL;
}

static void _send_notify(IMSG msgid, int param, bool dyn_data, void *data)
{
    cmm_send_message(CMMPT_EVT, msgid, param, dyn_data, data);
    if (CMMPT_WEB) cmm_send_message(CMMPT_WEB, msgid, param, dyn_data, data);
}

static int _set_oneself_nvr_data()
{
    NFOpenmodeDeviceList *dlist;
	NFOpenmodeCamInfo *info = NULL;
	NVR_DATA_T data;
	SysInfoData sys_info;
	UserManageData udata;
	gint i, j;
	gchar fwver[32];
	
	memset(&data, 0x00, sizeof(NVR_DATA_T));
	
    data.active_ch_cnt = GUI_CHANNEL_CNT;
    var_get_fake_fwver(fwver, sizeof(fwver));
    strcpy(data.fwver, fwver);
    
	memset(&sys_info, 0, sizeof(SysInfoData));
	DAL_get_sysInfo_data(&sys_info);

    strcpy(data.model, sys_info.model);
    strcpy(data.sysid, sys_info.sysId);
    scm_get_ip_addr_str(data.hostname, 256);
    data.http_port = sys_info.webPort;

	memset(&udata, 0x00, sizeof(UserManageData));
    DAL_get_userManage_data(&udata, 0);  
    
    xmm_update_nvr_info(data.hostname, data.http_port, udata.id, udata.pw);
	
    memset(&invm.nvr_data[0], 0x00, sizeof(NVR_DATA_T));
    g_memmove(&invm.nvr_data[0], &data, sizeof(NVR_DATA_T));

    _print_nvr_data(&invm.nvr_data[0], 8, 8);

	return 0;
}

static int _find_empty_nvr_slot()
{
    int i;

    for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {
        if (strlen(invm.nvr_data[i].hostname) == 0) break;
    }

    if (i == NVM_MAX_NVR_SLOT_CNT) return -1;

    return i;
}

static int _check_empty_cam_slot(int idx, int ch)
{
    if (strlen(invm.nvr_data[idx].cam_info[ch].url) <= 0) return 1;

    return 0;
}

static int _get_nvr_idx(NVR_DATA_T *data)
{
    int i;

    for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {
        if (strcmp(invm.nvr_data[i].hostname, data->hostname) == 0) return i;
    }

    if (i == NVM_MAX_NVR_SLOT_CNT) return -1;
}

static int _check_exist(NVR_DATA_T *data)
{
    if (_get_nvr_idx(data) != -1) return 1;
    
    return 0;
}

static int _update_oneself_cam_info()
{
    NFOpenmodeDeviceList *dlist;
	NFOpenmodeCamInfo *info = NULL;
	CAMERA_INFO_T data[GUI_CHANNEL_CNT];
	gint i, j;
	
	memset(data, 0x00, sizeof(CAMERA_INFO_T) * GUI_CHANNEL_CNT);

    dlist = nf_openmode_get_ch_list();
	if (dlist == 0) return -1;
    
	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		info = _find_matched_info_by_ch(dlist, i);
	
		if ((info) && (info->state != OPENMODE_CAM_STATE_INIT))
		{
            strcpy(data[i].id, info->u);
            strcpy(data[i].pw, info->p);
            strcpy(data[i].model, info->model);
            strcpy(data[i].hostname, info->hostname);
            strcpy(data[i].url, info->hostname);
            for (j = 0; j < 8; j++) {
                data[i].macaddr[j] = info->macaddr[j];
            }
            data[i].port = info->http_port;
            data[i].state = info->state;
            data[i].ch = info->ch;
    		data[i].virtual_camera = info->virtual_camera;
    		if (info->virtual_camera) {
        		strcpy(data[i].vcam_rtsp_addr[0], info->vcam_rtsp_addr[0]);
        		strcpy(data[i].vcam_rtsp_addr[1], info->vcam_rtsp_addr[1]);
    		}
    		else {
        		strcpy(data[i].vcam_rtsp_addr[0], "");
        		strcpy(data[i].vcam_rtsp_addr[1], "");
    		}
		}
		else
		{
            strcpy(data[i].id, "");
            strcpy(data[i].pw, "");
            strcpy(data[i].model, "");
            strcpy(data[i].hostname, "");
            strcpy(data[i].url, "");
            for (j = 0; j < 8; j++) {
                data[i].macaddr[j] = info->macaddr[j];
            }
            data[i].port = 0;
            data[i].state = OPENMODE_CAM_STATE_INIT;
            data[i].ch = -1;
    		data[i].virtual_camera = info->virtual_camera;
    		strcpy(data[i].vcam_rtsp_addr[0], "");
    		strcpy(data[i].vcam_rtsp_addr[1], "");
		}
	}
	
    memset(invm.nvr_data[0].cam_info, 0x00, sizeof(CAMERA_INFO_T) * GUI_CHANNEL_CNT);
    g_memmove(invm.nvr_data[0].cam_info, data, sizeof(CAMERA_INFO_T) * GUI_CHANNEL_CNT);

	return 0;
}

static int _set_oneself_cam_info()
{
    IPCamLoginData info;
    int i, j;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
    	memset(&info, 0x00, sizeof(IPCamLoginData));
		DAL_get_ipcam_login_data(&info, i);
		
		strcpy(invm.nvr_data[0].cam_info[i].id, info.id);
		strcpy(invm.nvr_data[0].cam_info[i].pw, info.password);
		strcpy(invm.nvr_data[0].cam_info[i].model, info.model);
		strcpy(invm.nvr_data[0].cam_info[i].hostname, info.hostname);
		strcpy(invm.nvr_data[0].cam_info[i].url, info.hostname);
		for (j = 0; j < 8; j++) {
		    invm.nvr_data[0].cam_info[i].macaddr[j] = info.mac[j];
		}
		invm.nvr_data[0].cam_info[i].port = info.http_port;
		invm.nvr_data[0].cam_info[i].virtual_camera = info.virtual_camera;
		if (info.virtual_camera) {
    		strcpy(invm.nvr_data[0].cam_info[i].vcam_rtsp_addr[0], info.vcam_rtsp_addr[0]);
    		strcpy(invm.nvr_data[0].cam_info[i].vcam_rtsp_addr[1], info.vcam_rtsp_addr[1]);
		}
		invm.nvr_data[0].cam_info[i].ch = i;
		
		_print_cam_info(&invm.nvr_data[0].cam_info[i], 8);
	}

    return 0;
}

static int _add_list(NVR_DATA_T *data)
{
    int i;

    if (_check_exist(data)) return -2;
    
    i = _find_empty_nvr_slot();

    if (i == -1) return -1;
    if (i == 0) {
        _update_oneself_cam_info();
        i++;
    }

    g_memmove(&invm.nvr_data[i], data, sizeof(NVR_DATA_T));
    
    return 0;
}

static int _get_nvr_data_cnt()
{
    int i;
    int cnt = 0;

    NVM_LOCK();

    for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {
        if (strlen(invm.nvr_data[i].hostname) > 0) cnt++;
    }

    NVM_UNLOCK();

    return cnt;
}

static int _update_nvr_data(NVR_DATA_T *ndata)
{
    int idx;
    
    NVM_LOCK();

    idx = _get_nvr_idx(ndata);
    DMSG(7, "idx : %d", idx);
    if (idx == -1) {
        NVM_UNLOCK();
        return -1;
    }

    g_memmove(&invm.nvr_data[idx], ndata, sizeof(NVR_DATA_T));
    
    NVM_UNLOCK();

    return idx;
}

static gint _save_db()
{
	NFOpenmodeDeviceList *dlist = NULL;
	NFOpenmodeCamInfo *info = NULL;
    IPCamLoginData post_info[GUI_CHANNEL_CNT];
	
	gint i, ch;
	gchar str_buf[32];

    memset(str_buf, 0x00, sizeof(str_buf));
    memset(&post_info, 0x00, sizeof(IPCamLoginData) * GUI_CHANNEL_CNT);

	dlist = nf_openmode_get_ch_list();
	if (dlist == 0) return -1;

	for (i = 0; i < GUI_CHANNEL_CNT; i++)
	{
		info = _find_matched_info_by_ch(dlist, i);

		if ((info) && (info->state != OPENMODE_CAM_STATE_INIT))
		{	
			strncpy(post_info[i].id, info->u, 31);
			strncpy(post_info[i].password, info->p, 31);
			strncpy(post_info[i].hostname, info->hostname, 63);

			g_sprintf(str_buf, "%02x%02x%02x%02x%02x%02x", 
			           info->macaddr[0], info->macaddr[1], info->macaddr[2], info->macaddr[3], info->macaddr[4], info->macaddr[5]);

			strncpy(post_info[i].mac, str_buf, 12);

			post_info[i].http_port = info->http_port;
			post_info[i].rtsp_port = info->rtsp_port;
            post_info[i].virtual_camera = info->virtual_camera;
			post_info[i].vcam_cnt = info->vcam_cnt;

			strncpy(post_info[i].vcam_rtsp_addr[0], info->vcam_rtsp_addr[0], 255);
    		strncpy(post_info[i].vcam_rtsp_addr[1], info->vcam_rtsp_addr[1], 255);			

            if(info->virtual_camera)
            {                               
    			strncpy(post_info[i].model, info->model, 63);
            }
		}
		else
		{
//			strcpy(post_info[i].id, "");
//			strcpy(post_info[i].password, "");
			strcpy(post_info[i].hostname, "");
			post_info[i].http_port = 0;
		}
		
		DAL_set_ipcam_login_data(post_info[i], i);
	}

	DAL_save_setup_db(NFSETUP_WINDOW_CAMERA);

	return 0;
}

//�޼��� ó��
static int _process_message(CMM_MESSAGE_T *pmsg)
{
    NVR_DATA_T *ndata = NULL;
    NVM_MSG nvm_msg = NVM_MSG_MAX_CNT;

    if (!pmsg->data) return 0;
    
    DMSG(9, "pmsg->data : %s", pmsg->data);
    nvm_msg = nvm_json_get_msg(pmsg->data);

	switch (nvm_msg) 
	{
	    case NVM_MSG_REQ_CONNECT_NVR:
	    {
	        NVR_DATA_T *oneself;
	        char *jdata;
	        int ret = 0;
            char sender_addr[256];
            char sender_id[65];
            char sender_pw[33];
            int sender_port = 0;

            DMSG(5, "NVM_MSG_REQ_CONNECT_NVR");
            
            memset(sender_addr, 0x00, sizeof(sender_addr));
            memset(sender_id, 0x00, sizeof(sender_id));
            memset(sender_pw, 0x00, sizeof(sender_pw));

	        nvm_json_get_sender(pmsg->data, sender_addr, 256, &sender_port, sender_id, sender_pw);
	        xmm_update_nvr_info(sender_addr, sender_port, sender_id, sender_pw);
	        xmm_update_master_nvr_info(sender_addr, sender_port, sender_id, sender_pw);
	        
	        nf_openmode_init_detection_list();
	        _update_oneself_cam_info();
	        nf_openmode_cancel();
	        nf_openmode_finalize_installation();

	        oneself = nvm_get_nvr_data(0);
	        jdata = nvm_json_convert_ndata_to_json(oneself, NVM_MSG_RPL_CONNECT_NVR);
            xmm_send_msg(sender_addr, sender_port, jdata, NULL);

	        if (oneself) ifree(oneself);
            if (jdata) free(jdata);
        }
	    break;
	    
	    case NVM_MSG_RPL_CONNECT_NVR:
	    {
            NVR_DATA_T *ndata = NULL;
            int idx;

            DMSG(5, "NVM_MSG_RPL_CONNECT_NVR");

            ndata = nvm_json_convert_json_to_ndata(pmsg->data);

            if (ndata)
            {
                idx = _update_nvr_data(ndata);
                _print_nvr_data(ndata, 8, 8);
                xmm_update_conn_status(ndata->hostname, ndata->http_port, XMM_CONN_CONN_SUCC);
                _send_notify(IRET_NVM_UPDATE_CONN_STATUS, idx, 0, NULL);
            }
	        
        	if (ndata) ifree(ndata);
        }
	    break;
	    
	    case NVM_MSG_START_REMOTE_CAMERA_INSTALL:
	    {
            char sender_addr[256];
            char sender_id[65];
            char sender_pw[33];
            int sender_port = 0;

            DMSG(5, "NVM_MSG_START_REMOTE_CAMERA_INSTALL");
            
            memset(sender_addr, 0x00, sizeof(sender_addr));
            memset(sender_id, 0x00, sizeof(sender_id));
            memset(sender_pw, 0x00, sizeof(sender_pw));

	        nvm_json_get_sender(pmsg->data, sender_addr, 256, &sender_port, sender_id, sender_pw);
	        xmm_update_nvr_info(sender_addr, sender_port, sender_id, sender_pw);
	        xmm_update_master_nvr_info(sender_addr, sender_port, sender_id, sender_pw);
	        
            _send_notify(INFY_OPENMODE_ENTER_INSTALL_BY_NVM, 0, 0, NULL);
	    }
	    break;

	    case NVM_MSG_FAIL_REMOTE_CAMERA_INSTALL:
	    {
            NVR_DATA_T *ndata = NULL;
            int idx;
            
            DMSG(5, "NVM_MSG_FAIL_REMOTE_CAMERA_INSTALL");
            
            ndata = nvm_json_convert_json_to_ndata(pmsg->data);
            _print_nvr_data(ndata, 8, 8);

            idx = _get_nvr_idx(ndata);
            
            _send_notify(INFY_OPENMODE_FAIL_INSTALL_BY_NVM, idx, 0, NULL);
            
        	if (ndata) ifree(ndata);
	    }
	    break;

	    case NVM_MSG_READY_REMOTE_CAMERA_INSTALL:
	    {
            NVR_DATA_T *ndata = NULL;
            int idx;

            DMSG(5, "NVM_MSG_READY_REMOTE_CAMERA_INSTALL");
            
            ndata = nvm_json_convert_json_to_ndata(pmsg->data);
            _print_nvr_data(ndata, 8, 8);

            if (ndata)
            {
                idx = _update_nvr_data(ndata);
                _send_notify(INFY_OPENMODE_READY_INSTALL_BY_NVM, 1, 0, NULL);
            }
	        
        	if (ndata) ifree(ndata);
        }
        break;
	    
	    case NVM_MSG_REQ_CH_LIST:
	    {
	        NVR_DATA_T *oneself;
	        char *jdata;
	        int ret = 0;
            char sender_addr[256];
            char sender_id[65];
            char sender_pw[33];
            int sender_port = 0;

            DMSG(5, "NVM_MSG_REQ_CH_LIST");
            
            memset(sender_addr, 0x00, sizeof(sender_addr));
            memset(sender_id, 0x00, sizeof(sender_id));
            memset(sender_pw, 0x00, sizeof(sender_pw));

	        nvm_json_get_sender(pmsg->data, sender_addr, 256, &sender_port, sender_id, sender_pw);

	        oneself = nvm_get_nvr_data(0);
	        jdata = nvm_json_convert_ndata_to_json(oneself, NVM_MSG_RPL_CH_LIST);
            xmm_send_msg(sender_addr, sender_port, jdata, NULL);

	        if (oneself) ifree(oneself);
            if (jdata) free(jdata);
        }	        
	    break;

	    case NVM_MSG_RPL_CH_LIST:
        {
            NVR_DATA_T *ndata = NULL;
            int idx;

            DMSG(5, "NVM_MSG_RPL_CH_LIST");
            
            ndata = nvm_json_convert_json_to_ndata(pmsg->data);
            _print_nvr_data(ndata, 8, 8);

            if (ndata)
            {
                idx = _update_nvr_data(ndata);
                _send_notify(INFY_NVM_DATA_IS_CHANGED, idx, 0, NULL);
            }
	        
        	if (ndata) ifree(ndata);
        }
	    break;

	    case NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE:
	    {
            DMSG(5, "NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE");

	        nf_openmode_cancel();
        	nf_openmode_finalize_installation();
            _send_notify(INFY_OPENMODE_LEAVE_INSTALL_BY_NVM, 0, 0, NULL);
	    }
	    break;

	    case NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE_WITH_APPLY:
	    {
            NVR_DATA_T *ndata = NULL;
	        NFOpenmodeDeviceList *dlist;
	        NFOpenmodeCamInfo *info;
	        OPENMODE_STATE_VIRTUAL_CAM_ADD state;
	        int i, j;
            
            DMSG(5, "NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE_WITH_APPLY");
            
            ndata = nvm_json_convert_json_to_ndata(pmsg->data);
            _print_nvr_data(ndata, 8, 8);

            NVM_LOCK();
	        g_memmove(invm.nvr_data[0].cam_info, ndata->cam_info, sizeof(CAMERA_INFO_T) * GUI_CHANNEL_CNT);
	        _print_nvr_data(&invm.nvr_data[0], 8, 8);
            NVM_UNLOCK();
            
	        dlist = nf_openmode_get_list();
	        if (!dlist) break;

	        info = dlist->head;

            while(info != NULL) {
                nf_openmode_set_channel_no_noti(info->index, -1);
                info = info->next;
            }
            
            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (strlen(invm.nvr_data[0].cam_info[i].url) == 0) continue;
                
                if (invm.nvr_data[0].cam_info[i].virtual_camera) nf_openmode_add_virtual_camera(invm.nvr_data[0].cam_info[i].vcam_rtsp_addr[0], invm.nvr_data[0].cam_info[i].vcam_rtsp_addr[1], "VCAM", &state);
                else nf_openmode_add_device_manual(invm.nvr_data[0].cam_info[i].hostname, invm.nvr_data[0].cam_info[i].port);
    	        info = dlist->head;
    	        
                for (j = 0; j < dlist->entry_cnt; j++)
                {
    	            if (strcmp(invm.nvr_data[0].cam_info[i].url, info->hostname) == 0) 
    	            {
        	            DMSG(7, "SAMESAME!!!! i : %d, j : %d", i, j);
    	                nf_openmode_set_channel_no_noti(info->index, invm.nvr_data[0].cam_info[i].ch);
    	                nf_openmode_set_login_info(info->index, invm.nvr_data[0].cam_info[i].id, invm.nvr_data[0].cam_info[i].pw);
    	                break;
    	            }
    	            
                    info = info->next;
                }
            }

	        _save_db();
	        nf_openmode_apply();
        	nf_openmode_finalize_installation();
            _send_notify(INFY_OPENMODE_LEAVE_INSTALL_BY_NVM, 0, 0, NULL);
	        
        	if (ndata) ifree(ndata);
	    }
	    break;

	    case NVM_MSG_APPLY_CAMERA_SETTINGS:
	    {
            NVR_DATA_T *ndata = NULL;
	        NFOpenmodeDeviceList *dlist;
	        NFOpenmodeCamInfo *info;
	        OPENMODE_STATE_VIRTUAL_CAM_ADD state;
	        int i, j;
            
            DMSG(5, "NVM_MSG_APPLY_CAMERA_SETTINGS");
            
            ndata = nvm_json_convert_json_to_ndata(pmsg->data);
            _print_nvr_data(ndata, 8, 8);

            NVM_LOCK();
	        g_memmove(invm.nvr_data[0].cam_info, ndata->cam_info, sizeof(CAMERA_INFO_T) * GUI_CHANNEL_CNT);
	        _print_nvr_data(&invm.nvr_data[0], 8, 8);
            NVM_UNLOCK();
            
	        dlist = nf_openmode_get_list();
	        if (!dlist) break;

	        info = dlist->head;

            while(info != NULL) {
                nf_openmode_set_channel_no_noti(info->index, -1);
                info = info->next;
            }
            
            for (i = 0; i < GUI_CHANNEL_CNT; i++)
            {
                if (strlen(invm.nvr_data[0].cam_info[i].url) == 0) continue;
                
                if (invm.nvr_data[0].cam_info[i].virtual_camera) nf_openmode_add_virtual_camera(invm.nvr_data[0].cam_info[i].vcam_rtsp_addr[0], invm.nvr_data[0].cam_info[i].vcam_rtsp_addr[1], "VCAM", &state);
                else nf_openmode_add_device_manual(invm.nvr_data[0].cam_info[i].hostname, invm.nvr_data[0].cam_info[i].port);
    	        info = dlist->head;
    	        
                for (j = 0; j < dlist->entry_cnt; j++)
                {
    	            if (strcmp(invm.nvr_data[0].cam_info[i].url, info->hostname) == 0) 
    	            {
        	            DMSG(7, "SAMESAME!!!! i : %d, j : %d", i, j);
    	                nf_openmode_set_channel_no_noti(info->index, invm.nvr_data[0].cam_info[i].ch);
    	                nf_openmode_set_login_info(info->index, invm.nvr_data[0].cam_info[i].id, invm.nvr_data[0].cam_info[i].pw);
    	                break;
    	            }
    	            
                    info = info->next;
                }                
            }

	        _save_db();
	        nf_openmode_apply();
	        
        	if (ndata) ifree(ndata);
	    }
	    break;

	    case NVM_MSG_TIME_SYNC:
	    {
	        unsigned int target_time = 0;

	        target_time = nvm_json_get_time(pmsg->data);
	        
	        scm_change_time(target_time, time(0), IRET_SCM_CHANGE_SYSTEM_TIME);
	    }
	    break;

	    case NVM_MSG_SUCCESS_TIME_SYNC:
	    {
            NVR_DATA_T *ndata = NULL;
            int idx;

            DMSG(5, "NVM_MSG_SUCCESS_TIME_SYNC");
            
            ndata = nvm_json_convert_json_to_ndata(pmsg->data);
            _print_nvr_data(ndata, 8, 8);

            if (ndata)
            {
                idx = _get_nvr_idx(ndata);
                invm.nvr_data[idx].last_sync = time(0);
                DMSG(7, "invm.nvr_data[%d].hostname : %s, last_sync : %d", idx, invm.nvr_data[idx].hostname, invm.nvr_data[idx].last_sync);
                _send_notify(INFY_NVM_DATA_IS_CHANGED, idx, 0, NULL);
            }
	        
        	if (ndata) ifree(ndata);
        }
	    break;

	    default:
            DMSG(5, "msg : %d, UNKNOWN MESSAGE(T_T)", nvm_msg);
	    break;
	}

	if (pmsg->dyn_data && pmsg->data) free(pmsg->data);

	return 0;
}	


static int _is_keepalive_on()
{
	return (invm.opt_ka == 1);
}

static int _is_30sec_past()
{
	static time_t _ka_prev = 0;
	time_t cur = time(0);

	if (cur - _ka_prev > 30) {
		_ka_prev = time(0);
		return 1;
	}
	return 0;
}

static int _is_connected(int idx)
{

}

static int _is_empty_slot(int idx)
{
	return (strlen(invm.nvr_data[idx].hostname) == 0);
}

static int _is_ka_timeout(int idx)
{
	time_t basetime = time(0) - 30;	// 30sec before current time
	if (invm.nvr_data[idx].ka == 0) return 1;
	if (invm.nvr_data[idx].ka - basetime > 5) return 1;		// 5sec over
	return 0;
}

static int _has_disconnected()
{
	int i;
	time_t cur;
	for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; ++i) {
		if (_is_empty_slot(i)) continue;
		if (!_is_connected(i)) continue;
		if (_is_ka_timeout(i)) return 1;
	}
	return 0;
}

static int _stamp_ka(int idx)
{
	invm.nvr_data[idx].ka = time(0);
	return 0;
}

static int _clear_ka(int idx)
{
	invm.nvr_data[idx].ka = 0;
	return 0;
}

static int _send_keepalive_msg()
{
	int i;
	int ret;
	for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; ++i) {
		if (_is_empty_slot(i)) continue;
		if (!_is_connected(i)) continue;

		_clear_ka(i);
		//ret = xmm_send_keepalive(0, invm.nvr_data[i].hostname, invm.nvr_data[i].port);
		
	}
	return 0;
}

static int _check_keepalive()
{
	int ret;
	if (_is_keepalive_on()) {
		if (_is_30sec_past()) {
			ret = _has_disconnected();
			if (ret) {
				//_send_notify(infy_nvm_reload, 0, 0, null);
			}
			else {
				_send_keepalive_msg();
			}
		}
	}
	return 0;
}

static int _find_nvr(char *hostname)
{
	int i;
	for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; ++i) {
		if (strlen(invm.nvr_data[i].hostname) == 0) continue;
		if (strcmp(invm.nvr_data[i].hostname, hostname) == 0) return i;
	}
	return -1;
}


//�޼��� ����
static int _service_proc()
{
    CMM_MESSAGE_T buf;
    
    while(1)
    {
        usleep(100000);
        if (cmm_get_message(&buf) == 0) {
            _process_message(&buf);
        }

		_check_keepalive();
    }

    return 0;
}

static int _init_thread(NVM_T *pinvm)
{
	pinvm->th = ifn_make_thread(_service_proc, NULL);
	cmm_mount_on_thread(pinvm->th);
	pinvm->cmmpt = pinvm->th;
	DMSG(7, "NVM CMMPT : [0x%08x]\n", pinvm->cmmpt);
	return 0;
}

static void _chk_supported_remote_cb(XRES_T *res_data)
{
    NVR_DATA_T data;
    memset(&data, 0x00, sizeof(NVR_DATA_T));
    
    NVM_LOCK();
    DMSG(7, "hostname : %s, res : %d", res_data->hostname, res_data->result);
    
    if (res_data->result == 1) {
        strcpy(data.hostname, res_data->hostname);
        data.http_port = res_data->http_port;
        _add_list(&data);
        xmm_update_nvr_info(data.hostname, data.http_port, "", "");
    }
    
    NVM_UNLOCK();
}

static void _connect_nvr_cb(XRES_T *res_data)
{
    int idx;

    idx = _find_nvr(res_data->hostname);
    _send_notify(IRET_NVM_UPDATE_CONN_STATUS, idx, 0, NULL);
}

static void _start_remote_cam_install_cb(XRES_T *res_data)
{
    DMSG(7, "hostname : %s, res : %d", res_data->hostname, res_data->result);
    if (res_data->result == 200)
        _send_notify(IRET_NVM_START_CAMERA_INSTALL, 1, 0, NULL);
    else
        _send_notify(IRET_NVM_START_CAMERA_INSTALL, 0, 0, NULL);
}

static void _apply_cam_settings_cb(XRES_T *res_data)
{
    DMSG(7, "hostname : %s, res : %d", res_data->hostname, res_data->result);
    if (res_data->result == 200)
        _send_notify(IRET_NVM_APPLY_CAMERA_SETTING, 1, 0, NULL);
    else
        _send_notify(IRET_NVM_APPLY_CAMERA_SETTING, 0, 0, NULL);
}

static void _apply_and_finalize_cb(XRES_T *res_data)
{
    DMSG(7, "hostname : %s, res : %d", res_data->hostname, res_data->result);
    if (res_data->result == 200)
        _send_notify(IRET_NVM_APPLY_AND_FINALIZE, 1, 0, NULL);
    else
        _send_notify(IRET_NVM_APPLY_AND_FINALIZE, 0, 0, NULL);
}

////////////////////////////////////
//public
////////////////////////////////////

//�ʱ�ȭ
int nvm_init()
{
    memset(&invm, 0x00, sizeof(NVM_T));
    invm.mtx = g_mutex_new();
    _init_thread(&invm);
    _set_oneself_nvr_data();
    _set_oneself_cam_info();

    return 0;
}

CMMPORT nvm_get_cmmport()
{
    return invm.cmmpt;
}

GThread *nvm_get_callid()
{
    return invm.th;
}

int nvm_turnon_keepalive()
{
	invm.opt_ka = 1;
	return 0;
}

int nvm_add_nvr_data(NVR_DATA_T *data)
{
    DMSG(5, "Call API");
    NVM_LOCK();

    _add_list(data);
    _print_nvr_data(data, 8, 8);
    NVM_UNLOCK();

    return 0;
}

int nvm_check_supported_nvm(int idx, char *hostname, int port)
{
    int ret = 0;

    DMSG(5, "Call API");
    ret = xmm_check_supported_remote_control(hostname, port, _chk_supported_remote_cb);

    return ret;
}

int nvm_connect_nvr(int idx)
{
    NVR_DATA_T *target_data;
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");

    if (strlen(invm.nvr_data[idx].hostname) <= 0) return -1;
    
    target_data = nvm_get_nvr_data(idx);    
    if (!target_data) return -1;
    
    jdata = nvm_json_convert_ndata_to_json(target_data, NVM_MSG_REQ_CONNECT_NVR);
    if (!jdata) {
        ifree(target_data);
        return -1;
    }
    
    xmm_update_conn_status(target_data->hostname, target_data->http_port, XMM_CONN_REQUEST);
    xmm_send_msg(target_data->hostname, target_data->http_port, jdata, _connect_nvr_cb);

    ifree(target_data);
    free(jdata);

    return ret;
}

int nvm_update_oneself_nvr_info()
{
    DMSG(5, "Call API");
    NVM_LOCK();
    
    _set_oneself_nvr_data();
    
    NVM_UNLOCK();

    return 0;
}

int nvm_update_oneself_cam_info()
{
    DMSG(5, "Call API");
    NVM_LOCK();
    
    _update_oneself_cam_info();
    
    NVM_UNLOCK();

    return 0;
}

NVR_DATA_T *nvm_get_nvr_data(int idx)
{
    NVR_DATA_T *data;

    DMSG(5, "Call API");
    NVM_LOCK();
    DMSG(7, "idx : %d", idx);
    
    data = (NVR_DATA_T*)imalloc(sizeof(NVR_DATA_T));
    if (data == NULL) {
        NVM_UNLOCK();
        return NULL;
    }
    memset(data, 0x00, sizeof(NVR_DATA_T));

    g_memmove(data, &invm.nvr_data[idx], sizeof(NVR_DATA_T));
    
    NVM_UNLOCK();

    return data;
}

int nvm_get_nvr_conn_info(int idx, char *buf, int buf_len)
{
    memset(buf, 0x00, buf_len);

    DMSG(5, "Call API");
    NVM_LOCK();
    strcpy(buf, invm.nvr_data[idx].hostname);
    NVM_UNLOCK();

    return invm.nvr_data[idx].http_port;
}

int nvm_set_sysid(int idx, char *sysid)
{
    DMSG(5, "Call API");

    NVM_LOCK();
    strcpy(invm.nvr_data[idx].sysid, sysid);
    NVM_UNLOCK();

    return 0;
}

char *nvm_get_sysid(int idx)
{
    char *sysid = NULL;
    
    DMSG(5, "Call API");

    NVM_LOCK();
    if (strlen(invm.nvr_data[idx].sysid) > 0) {
        sysid = imalloc(sizeof(invm.nvr_data[idx].sysid) + 1);
        if (!sysid) {
            NVM_UNLOCK();
            return NULL;
        }
        memset(sysid, 0x00, sizeof(sysid));
        
        strcpy(sysid, invm.nvr_data[idx].sysid);
    }
    NVM_UNLOCK();

    return sysid;
}

int nvm_get_active_ch_cnt(int idx)
{
    DMSG(5, "Call API");
    DMSG(7, "active_ch_cnt : %d", invm.nvr_data[idx].active_ch_cnt);
    return invm.nvr_data[idx].active_ch_cnt;
}

CAMERA_INFO_T *nvm_get_cam_info(int idx, int ch)
{
    CAMERA_INFO_T *data;
    
    DMSG(5, "Call API");
    NVM_LOCK();
    if (strlen(invm.nvr_data[idx].cam_info[ch].url) <= 0) {
        NVM_UNLOCK();
        return NULL;
    }
    
    data = (CAMERA_INFO_T*)imalloc(sizeof(CAMERA_INFO_T));
    if (!data) {
        NVM_UNLOCK();
        return NULL;
    }
    memset(data, 0x00, sizeof(CAMERA_INFO_T));
    
    g_memmove(data, &invm.nvr_data[idx].cam_info[ch], sizeof(CAMERA_INFO_T));
    NVM_UNLOCK();

    return data;
}

CAMERA_INFO_T *nvm_get_cam_info_all(int idx)
{
    CAMERA_INFO_T *data;
    
    DMSG(5, "Call API");
    NVM_LOCK();
    
    data = (CAMERA_INFO_T*)imalloc(sizeof(CAMERA_INFO_T) * invm.nvr_data[idx].active_ch_cnt);
    if (!data) {
        DMSG(7, "imalloc fail!");
        NVM_UNLOCK();
        return NULL;
    }
    memset(data, 0x00, sizeof(CAMERA_INFO_T) * invm.nvr_data[idx].active_ch_cnt);
    
    g_memmove(data, invm.nvr_data[idx].cam_info, sizeof(CAMERA_INFO_T) * invm.nvr_data[idx].active_ch_cnt);
    NVM_UNLOCK();

    return data;
}

int nvm_update_cam_info(int idx, CAMERA_INFO_T *cam_info)
{
    int i;
    
    DMSG(5, "Call API");
    DMSG(5, "nvr idx : %d", idx);

    if (!cam_info) return -1;
    
    NVM_LOCK();

    for (i = 0; i < invm.nvr_data[idx].active_ch_cnt; i++)
    {
        if (strcmp(invm.nvr_data[idx].cam_info[i].url, cam_info->url) == 0) {
    		strcpy(invm.nvr_data[idx].cam_info[i].id, cam_info->id);
    		strcpy(invm.nvr_data[idx].cam_info[i].pw, cam_info->pw);
    		strcpy(invm.nvr_data[idx].cam_info[i].model, cam_info->model);
    		invm.nvr_data[idx].cam_info[i].state = cam_info->state;
    		
             _print_cam_info(&invm.nvr_data[idx].cam_info[i], 8);
        }
    }

    NVM_UNLOCK();

    return 0;
}

int nvm_set_cam_info_all(int idx, CAMERA_INFO_T *cam_info)
{
    int i;
    
    DMSG(5, "Call API");

    if (!cam_info) return -1;
    
    NVM_LOCK();

    g_memmove(invm.nvr_data[idx].cam_info, cam_info, sizeof(CAMERA_INFO_T) * invm.nvr_data[idx].active_ch_cnt);
    
    if (idx == 0)
    {
        NFOpenmodeDeviceList *dlist;
        NFOpenmodeCamInfo *info;
        int i, j;
        
        dlist = nf_openmode_get_list();
        if (!dlist) {
            NVM_UNLOCK();
            return 0;
        }

        info = dlist->head;
	        
        for (j = 0; j < dlist->entry_cnt; j++)
        {
            DMSG(7, "info_cam_info[%d].url : %s", j, info->hostname);
	        for (i = 0; i < GUI_CHANNEL_CNT; i++)
	        {
	            DMSG(7, "invm_cam_info[%d].url : %s", i, invm.nvr_data[0].cam_info[i].url);
	            if ((strlen(invm.nvr_data[0].cam_info[i].url) && strcmp(invm.nvr_data[0].cam_info[i].url, info->hostname) == 0) &&
	                (invm.nvr_data[0].cam_info[i].port == info->http_port))
	            {
    	            DMSG(7, "SAMESAME!!!! i : %d, j : %d", i, j);
	                nf_openmode_set_channel_no_noti(info->index, invm.nvr_data[0].cam_info[i].ch);
	                break;
	            }
	        }
	        if (i == GUI_CHANNEL_CNT) nf_openmode_set_channel_no_noti(info->index, -1);
	        
            info = info->next;
        }
    }

    NVM_UNLOCK();

    return 0;
}

int nvm_get_assigned_cam_cnt(int idx)
{
    int i;
    int cnt = 0;

    for (i = 0; i < invm.nvr_data[idx].active_ch_cnt; i++)
    {
        if ((strlen(invm.nvr_data[idx].cam_info[i].url) > 0) && (invm.nvr_data[idx].cam_info[i].ch != -1)) cnt++;
    }

    return cnt;
}

int nvm_get_assigned_cam_cnt_by_url(char *url)
{
    int i, j;
    int cnt = 0;

    for (i = 0; i < NVM_MAX_NVR_SLOT_CNT; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if (strcmp(invm.nvr_data[i].cam_info[j].url, url) == 0) cnt++;
        }
    }

    return cnt;
}

int nvm_check_empty_ch(int idx, int ch)
{
    int res = 0;
    
    DMSG(5, "Call API");
    NVM_LOCK();
    res = _check_empty_cam_slot(idx, ch);
    NVM_UNLOCK();

    return res;
}

int nvm_assign_cam(int idx, CAMERA_INFO_T *info, int ch)
{
    NVM_LOCK();
    g_memmove(&invm.nvr_data[idx].cam_info[ch], info, sizeof(CAMERA_INFO_T));
    invm.nvr_data[idx].cam_info[ch].ch = ch;
    NVM_UNLOCK();
    
    return 0;    
}

int nvm_unassign_cam(int idx, int ch)
{
    NVM_LOCK();
    if (idx == 0)
    {
        NFOpenmodeDeviceList *dlist;
        NFOpenmodeCamInfo *info;
        int i;
        
        dlist = nf_openmode_get_ch_list();
        if (!dlist) {
            NVM_UNLOCK();
            return 0;
        }

        info = dlist->head;
	        
        for (i = 0; i < dlist->entry_cnt; i++)
        {
            DMSG(7, "info_cam_info[%d].url : %s", i, info->hostname);
            if (strlen(invm.nvr_data[idx].cam_info[ch].url) && strcmp(invm.nvr_data[idx].cam_info[ch].url, info->hostname) == 0) 
            {
	            DMSG(7, "SAMESAME!!!! ch : %d, i : %d", ch, i);
                nf_openmode_set_channel_no_noti(info->index, -1);
                break;
            }
	        
            info = info->next;
        }
    }
    
    memset(&invm.nvr_data[idx].cam_info[ch], 0x00, sizeof(CAMERA_INFO_T));
    invm.nvr_data[idx].cam_info[ch].ch = -1;
        
    NVM_UNLOCK();

    return 0;
}

int nvm_swap_ch(int idx, int from_ch, int to_ch)
{
    CAMERA_INFO_T tmp;
    
    NVM_LOCK();

    memset(&tmp, 0x00, sizeof(CAMERA_INFO_T));

    g_memmove(&tmp, &invm.nvr_data[idx].cam_info[to_ch], sizeof(CAMERA_INFO_T));
    g_memmove(&invm.nvr_data[idx].cam_info[to_ch], &invm.nvr_data[idx].cam_info[from_ch], sizeof(CAMERA_INFO_T));
    g_memmove(&invm.nvr_data[idx].cam_info[from_ch], &tmp, sizeof(CAMERA_INFO_T));

    invm.nvr_data[idx].cam_info[from_ch].ch = from_ch;
    invm.nvr_data[idx].cam_info[to_ch].ch = to_ch;
    
    NVM_UNLOCK();

    return 0;
}

int nvm_reload_nvr_data(int idx)
{
    NVR_DATA_T *target_data;
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");
    target_data = nvm_get_nvr_data(idx);
    jdata = nvm_json_convert_ndata_to_json(target_data, NVM_MSG_REQ_CH_LIST);
    xmm_send_msg(target_data->hostname, target_data->http_port, jdata, NULL);


    ifree(target_data);
    free(jdata);

    return ret;
}

int nvm_start_remote_cam_install(int idx)
{
    NVR_DATA_T *target_data;
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");
    target_data = nvm_get_nvr_data(idx);
    jdata = nvm_json_convert_ndata_to_json(target_data, NVM_MSG_START_REMOTE_CAMERA_INSTALL);
    xmm_send_msg(target_data->hostname, target_data->http_port, jdata, _start_remote_cam_install_cb);

    ifree(target_data);
    free(jdata);

    return ret;
}

int nvm_fail_remote_cam_install()
{
    NVR_DATA_T *ndata;
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");
    ndata = nvm_get_nvr_data(0);
    jdata = nvm_json_convert_ndata_to_json(ndata, NVM_MSG_FAIL_REMOTE_CAMERA_INSTALL);
    ret = xmm_send_message_to_master(jdata);

    ifree(ndata);
    free(jdata);

    return ret;
}

int nvm_ready_remote_cam_install()
{
    NVR_DATA_T *ndata;
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");
    ndata = nvm_get_nvr_data(0);
    jdata = nvm_json_convert_ndata_to_json(ndata, NVM_MSG_READY_REMOTE_CAMERA_INSTALL);
    ret = xmm_send_message_to_master(jdata);

    ifree(ndata);
    free(jdata);

    return ret;
}

int nvm_apply_cam_settings(int idx)
{
    int ret = -1;
    int status;
    
    DMSG(5, "Call API");
    
    if (idx == 0)
    {
        NFOpenmodeDeviceList *dlist;
        NFOpenmodeCamInfo *info;
        int i, j;
        
        dlist = nf_openmode_get_list();
        if (!dlist) return ret;

        info = dlist->head;

        for (i = 0; i < dlist->entry_cnt; i++) {
            nf_openmode_set_channel_no_noti(info->index, -1);
            info = info->next;
        }

        for (i = 0; i < GUI_CHANNEL_CNT; i++)
        {
            DMSG(7, "invm_cam_info[%d].url : %s", i, invm.nvr_data[0].cam_info[i].url);
            info = dlist->head;
            for (j = 0; j < dlist->entry_cnt; j++)
            {
                if ((strlen(invm.nvr_data[0].cam_info[i].url) && strcmp(invm.nvr_data[0].cam_info[i].url, info->hostname) == 0) &&
                    (invm.nvr_data[0].cam_info[i].port == info->http_port))
                {
                    nf_openmode_set_channel_no_noti(info->index, invm.nvr_data[0].cam_info[i].ch);
                    info = info->next;
                    break;
                }
                
                info = info->next;
            }
        }
/*	        
        for (j = 0; j < dlist->entry_cnt; j++)
        {
            DMSG(7, "info_cam_info[%d].url : %s", j, info->hostname);
	        for (i = 0; i < GUI_CHANNEL_CNT; i++)
	        {
	            DMSG(7, "invm_cam_info[%d].url : %s", i, invm.nvr_data[0].cam_info[i].url);
	            if ((strlen(invm.nvr_data[0].cam_info[i].url) && strcmp(invm.nvr_data[0].cam_info[i].url, info->hostname) == 0) &&
	                (invm.nvr_data[0].cam_info[i].port == info->http_port))
	            {
	                nf_openmode_set_channel_no_noti(info->index, invm.nvr_data[0].cam_info[i].ch);
	                break;
	            }
	        }
	        if (i == GUI_CHANNEL_CNT) nf_openmode_set_channel_no_noti(info->index, -1);
	        
            info = info->next;
        }
*/
        ret = 0;
    }
    else
    {
        NVR_DATA_T *ndata;
        char *jdata;

        ndata = nvm_get_nvr_data(idx);
        _print_nvr_data(ndata, 8, 8);
        jdata = nvm_json_convert_ndata_to_json(ndata, NVM_MSG_APPLY_CAMERA_SETTINGS);
        xmm_send_msg(ndata->hostname, ndata->http_port, jdata, _apply_cam_settings_cb);

        ifree(ndata);
        free(jdata);
    }

    return ret;
}

int nvm_finalize_remote_cam_install(int idx)
{
    NVR_DATA_T *target_data;
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");
    if (idx == 0) return 0;
    
    target_data = nvm_get_nvr_data(idx);
    jdata = nvm_json_convert_ndata_to_json(target_data, NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE);
    xmm_send_msg(target_data->hostname, target_data->http_port, jdata, NULL);

    ifree(target_data);
    free(jdata);

    return ret;
}

int nvm_finalize_remote_cam_install_with_apply(int idx)
{
    int ret = -1;
    int status;

    if (idx == 0)
    {
        NFOpenmodeDeviceList *dlist;
        NFOpenmodeCamInfo *info;
        int i, j;
        
        dlist = nf_openmode_get_list();
        if (!dlist) return ret;

        info = dlist->head;
	        
        for (j = 0; j < dlist->entry_cnt; j++)
        {
            DMSG(7, "info_cam_info[%d].url : %s", j, info->hostname);
	        for (i = 0; i < GUI_CHANNEL_CNT; i++)
	        {
	            DMSG(7, "invm_cam_info[%d].url : %s", i, invm.nvr_data[0].cam_info[i].url);
	            if ((strlen(invm.nvr_data[0].cam_info[i].url) && strcmp(invm.nvr_data[0].cam_info[i].url, info->hostname) == 0) &&
	                (invm.nvr_data[0].cam_info[i].port == info->http_port))
	            {
    	            DMSG(7, "SAMESAME!!!! i : %d, j : %d", i, j);
	                nf_openmode_set_channel_no_noti(info->index, invm.nvr_data[0].cam_info[i].ch);
	                break;
	            }
	        }
	        if (i == GUI_CHANNEL_CNT) nf_openmode_set_channel_no_noti(info->index, -1);
	        
            info = info->next;
        }
        
        ret = 0;
    }
    else
    {
        NVR_DATA_T *target_data;
        char *jdata;

        target_data = nvm_get_nvr_data(idx);
        jdata = nvm_json_convert_ndata_to_json(target_data, NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE_WITH_APPLY);
        xmm_send_msg(target_data->hostname, target_data->http_port, jdata, _apply_and_finalize_cb);

        ifree(target_data);
        free(jdata);
    }

    return ret;
}

int nvm_export_data(int idx, char *path)
{
    int ret;
    
    DMSG(5, "Call API");
    
    NVM_LOCK();
    
    ret = nvm_json_export_ndata(&invm.nvr_data[idx], path);

    NVM_UNLOCK();

    return ret;
}

int nvm_time_sync(int idx)
{
    NVR_DATA_T *target_data;
    char *jdata;
    int res;

    target_data = nvm_get_nvr_data(idx);
    jdata = nvm_json_set_time(NVM_MSG_TIME_SYNC, time(0));
    res = xmm_send_msg(target_data->hostname, target_data->http_port, jdata, NULL);

    ifree(target_data);
    free(jdata);

    return res;
}

int nvm_success_time_sync()
{
    char *jdata;
    int ret = 0;

    DMSG(5, "Call API");
    jdata = nvm_json_set_msg(NVM_MSG_SUCCESS_TIME_SYNC);
    ret = xmm_send_message_to_master(jdata);

    free(jdata);

    return ret;
}

