#ifndef __NVM_H__
#define __NVM_H__

#include "cmm.h"

#define NVM_MAX_NVR_SLOT_CNT            (64)

typedef enum _NVM_MSG
{
    NVM_MSG_REQ_CH_LIST = 0,
    NVM_MSG_RPL_CH_LIST,
    NVM_MSG_START_REMOTE_CAMERA_INSTALL,
    NVM_MSG_FAIL_REMOTE_CAMERA_INSTALL,
    NVM_MSG_READY_REMOTE_CAMERA_INSTALL,
    NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE,
    NVM_MSG_FINALIZE_CAMERA_INSTALL_MODE_WITH_APPLY,
    NVM_MSG_APPLY_CAMERA_SETTINGS,
    NVM_MSG_REQ_CONNECT_NVR,
    NVM_MSG_RPL_CONNECT_NVR,
    NVM_MSG_TIME_SYNC,
    NVM_MSG_SUCCESS_TIME_SYNC,

    NVM_MSG_MAX_CNT
}NVM_MSG;

typedef struct _CAMERA_INFO_T CAMERA_INFO_T;
struct _CAMERA_INFO_T
{
	gchar id[64];
	gchar pw[64];
    gchar model[64];
	gchar hostname[256];
	gchar url[256];
	guchar macaddr[8];
	gint port;
	gint state;
	gint ch;	
	guint virtual_camera;
	gchar vcam_rtsp_addr[2][256]; 	
};

typedef struct _NVR_DATA_T NVR_DATA_T;
struct _NVR_DATA_T
{
    CAMERA_INFO_T cam_info[32];
    gint active_ch_cnt;
	gchar hostname[256];
	guint http_port;
    gchar sysid[64];
    gchar mac[13];
    gchar model[64];
    gchar fwver[64];
	time_t 	ka;
	time_t last_sync;
};

int nvm_init();
CMMPORT nvm_get_cmmport();
GThread *nvm_get_callid();
int nvm_add_nvr_data(NVR_DATA_T *data);
int nvm_update_oneself_nvr_info();
int nvm_update_oneself_cam_info();
NVR_DATA_T *nvm_get_nvr_data(int idx);
int nvm_get_nvr_conn_info(int idx, char *buf, int buf_len);
int nvm_set_sysid(int idx, char *sysid);
char *nvm_get_sysid(int idx);
int nvm_get_active_ch_cnt(int idx);
CAMERA_INFO_T *nvm_get_cam_info(int idx, int ch);
CAMERA_INFO_T *nvm_get_cam_info_all(int idx);
int nvm_set_cam_info_all(int idx, CAMERA_INFO_T *cam_info);
int nvm_update_cam_info(int idx, CAMERA_INFO_T *cam_info);
int nvm_get_assigned_cam_cnt(int idx);
int nvm_get_assigned_cam_cnt_by_url(char *url);
int nvm_check_empty_ch(int idx, int ch);
int nvm_set_nvr_ch_list_by_json(int idx);
int nvm_assign_cam(int idx, CAMERA_INFO_T *info, int ch);
int nvm_unassign_cam(int idx, int ch);
int nvm_reload_nvr_data(int idx);
int nvm_get_nvr_cam_list_by_data(CAMERA_INFO_T *data);
int nvm_count_nvr_by_cam(char *cam_hostname);
int nvm_apply_cam_settings(int idx);

int nvm_connect_nvr(int idx);
int nvm_check_supported_nvm(int idx, char *hostname, int port);
int nvm_export_data(int idx, char *path);
int nvm_time_sync(int idx);
int nvm_success_time_sync();

#endif
