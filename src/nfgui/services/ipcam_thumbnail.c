#include "evt.h"
#include "scm.h"
#include "scm_internal.h"
#include "ipcam_thumbnail.h"

//private define
#define IPCAM_TH_MAX_BUF_SIZE               1024

enum {
    IIS_NOTYET = 0,
    IIS_SUCCESS,
    IIS_FAIL
};

//private data
static CAM_THUMBNAIL_T g_cam_thumb[IPCAM_TH_MAX_BUF_SIZE];
static gint g_loop_idx = 0;
static gint g_designate_idx = -1;

//private func

static gint _get_idx(gchar *hostname, gint port)
{
    gint i;
    
    for (i = 0; i < IPCAM_TH_MAX_BUF_SIZE; i++)
    {
        if (strlen(g_cam_thumb[i].hostname) <= 0) continue;
        
        if ((strcmp(hostname, g_cam_thumb[i].hostname) == 0) && (port == g_cam_thumb[i].port)) return i;
    }

    if (i == IPCAM_TH_MAX_BUF_SIZE) return -1;

    return i;
}

static gint _find_thumbnail_slot(NFOpenmodeCamInfo *info)
{
    gint i;
    
    for (i = 0; i < IPCAM_TH_MAX_BUF_SIZE; i++)
    {
        if (strcmp(info->hostname, g_cam_thumb[i].hostname) == 0) break;
        if (g_cam_thumb[i].jpec_size <= 0) break;
    }

    if (i == IPCAM_TH_MAX_BUF_SIZE) return -1;

    return i;
}

static size_t _get_thumbnail_data(NFOpenmodeCamInfo *info, jpeg_image_data *ret_jdata)
{
    jpeg_image_data jdata = {NULL, 0};

    if ((info->dev_cate != OPENMODE_DEV_CAM) || 
        (info->state == OPENMODE_CAM_STATE_INIT) || 
        (strlen(info->hostname) <= 0)) 
    {
        return 0;
    }
    
    if (info->state == OPENMODE_CAM_STATE_DEV_INFO || 
        info->state == OPENMODE_CAM_STATE_DEV_INFO_ONVIF || 
        info->state == OPENMODE_CAM_STATE_VIRTUAL_CAMERA)
    {
        jdata = nf_openmode_get_snapshot_image(info);

        if (jdata.size > 0) {
            ret_jdata->memory = jdata.memory;
            ret_jdata->size = jdata.size;
        }
    }

    return ret_jdata->size;
}

static NFOpenmodeCamInfo *_find_matched_info_by_idx(NFOpenmodeDeviceList *dlist, gint f_index)
{
	NFOpenmodeCamInfo *info;
	gint i;

	if (f_index >= dlist->entry_cnt) return NULL;

	info = dlist->head;

	for (i = 0; i < dlist->entry_cnt; i++)
	{
		if (info->index == f_index) return info;		
		
		info = info->next;
	}

	return NULL;
}

static int _proc_get_cam_thumb(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
    NFOpenmodeDeviceList *dlist = NULL;
    NFOpenmodeCamInfo *info = NULL;
    CAM_THUMBNAIL_T *tdata;
    jpeg_image_data jdata;
    gint i;
    gint e_idx;

    dlist = nf_openmode_get_list();
    if (!dlist || dlist->entry_cnt <= 0) {
        g_message("[%s, %d] list is NULL!", __FUNCTION__, __LINE__);
        return -1;
    }

    if (g_designate_idx == -1) {
        info = _find_matched_info_by_idx(dlist, g_loop_idx);
//        g_message("[%s, %d] g_loop_idx : %d", __FUNCTION__, __LINE__, g_loop_idx);

        if (!info) {
            g_message("[%s, %d] info is NULL!", __FUNCTION__, __LINE__);
            g_loop_idx = 0;
            return -1;
        }
        
        g_loop_idx++;
        if (g_loop_idx == IPCAM_TH_MAX_BUF_SIZE) {
            g_loop_idx = 0;
        }
    }
    else {
        info = _find_matched_info_by_idx(dlist, g_designate_idx);
        g_designate_idx = -1;
        
        if (!info) {
            g_message("[%s, %d] info is NULL!", __FUNCTION__, __LINE__);
            return -1;
        }
    }

//    g_message("[%s, %d] hostname : %s", __FUNCTION__, __LINE__, info->hostname);

    if ((info->dev_cate != OPENMODE_DEV_CAM) || (info->state == OPENMODE_CAM_STATE_INIT)) {
        return -1;
    }

    if ((e_idx = _find_thumbnail_slot(info)) != -1) 
    {
        jdata.memory = NULL;
        jdata.size = 0;
//        g_message("[%s, %d] e_idx : %d", __FUNCTION__, __LINE__, e_idx);

        if (_get_thumbnail_data(info, &jdata)) 
        {
//            g_message("[%s, %d] g_cam_thumb[%d].jpec_size : %d", __FUNCTION__, __LINE__, e_idx, g_cam_thumb[e_idx].jpec_size);

/*
            if (g_cam_thumb[e_idx].jpec_data) {
                ifree(g_cam_thumb[e_idx].jpec_data);
                g_cam_thumb[e_idx].jpec_data = NULL;
            }
            
            g_cam_thumb[e_idx].jpec_data = (gchar*)imalloc(sizeof(gchar) * jdata.size);
            g_memmove(g_cam_thumb[e_idx].jpec_data, jdata.memory, sizeof(gchar) * jdata.size);
            
            g_cam_thumb[e_idx].jpec_size = jdata.size;
            strcpy(g_cam_thumb[e_idx].hostname, info->hostname);
            strcpy(g_cam_thumb[e_idx].model, info->model);
            g_cam_thumb[e_idx].port = info->http_port;
            for (i = 0; i < 8; i++) {
                g_cam_thumb[e_idx].macaddr[i] = info->macaddr[i];
            }
*/
            tdata = (CAM_THUMBNAIL_T*)imalloc(sizeof(CAM_THUMBNAIL_T));
            
            tdata->jpec_data = (gchar*)imalloc(sizeof(gchar) * jdata.size);
            g_memmove(tdata->jpec_data, jdata.memory, sizeof(gchar) * jdata.size);
            
            tdata->jpec_size = jdata.size;
            strcpy(tdata->hostname, info->hostname);
            strcpy(tdata->model, info->model);
            tdata->port = info->http_port;
            for (i = 0; i < 8; i++) {
                tdata->macaddr[i] = info->macaddr[i];
            }

            cmm_send_message(CMMPT_EVT, INFY_GET_IPCAM_THUMBNAIL, e_idx, 0, (void*)tdata);
        }
        
        if (jdata.memory) free(jdata.memory);
    }

	return 0;
}

//public func
  
int iis_thumb_manager_start()
{
	if (iscm.wrk_iis_thumb) return -1;
	
    g_loop_idx = 0;
    memset(g_cam_thumb, 0x00, sizeof(CAM_THUMBNAIL_T) * IPCAM_TH_MAX_BUF_SIZE);

	iscm.wrk_iis_thumb = wrk_create_worker(_proc_get_cam_thumb, 0);
	wrk_change_sleep_time(iscm.wrk_iis_thumb, 1000*300);
	wrk_run_loop(iscm.wrk_iis_thumb, IMSG_NONE, &iscm, 0, 0);

    return 0;
}

int iis_thumb_manager_stop()
{
    gint i;

    wrk_destroy_worker(iscm.wrk_iis_thumb);
    iscm.wrk_iis_thumb = 0;

    for (i = 0; i < IPCAM_TH_MAX_BUF_SIZE; i++) {
        if (g_cam_thumb[i].jpec_data) {
            ifree(g_cam_thumb[i].jpec_data);
            g_cam_thumb[i].jpec_data = NULL;
        }
    }
    
    return 0;
}

