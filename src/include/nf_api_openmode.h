/* set tabspace4 */
/*******************************************************************************
*  (c) COPYRIGHT 2010 ITXSecurity                                              *
*  ALL RIGHT RESERVED                                                          *
*                                                                              *
********************************************************************************

DESCRIPTION: Including declarations and structures to support IPX open mode
             scenario.

................................................................................

REVISION HISTORY:

Date       Name           Description
__________ ______________ ______________________________________________________
15/01/2013 Jae-young Kim  Created. (Based on nmf 2.0 api )
*/

#ifndef __NF_API_OPENMODE_H__
#define __NF_API_OPENMODE_H__

#include <glib.h>
#include "nf_api_dlva.h"

typedef enum __OPENMODE_RTN_ENUM_
{
	OPENMODE_RTN_OK = 0,
	OPENMODE_RTN_FAIL
} OPENMODE_RTN_ENUM;

typedef enum __OPENMODE_STATE_ENUM_
{
	OPENMODE_STATE_CLOSE = 0,
	OPENMODE_STATE_INIT,
	OPENMODE_STATE_RUNNING,
	OPENMODE_STATE_SCANNING
} OPENMODE_STATE_ENUM;

typedef enum __OPENMODE_CAM_STATE_ENUM_
{
	OPENMODE_CAM_STATE_INIT = 0,
	OPENMODE_CAM_STATE_DISCOVERED,
	OPENMODE_CAM_STATE_REQ_IP,
	OPENMODE_CAM_STATE_DEV_INFO,
	OPENMODE_CAM_STATE_DEV_INFO_ONVIF,
	OPENMODE_CAM_STATE_ASSIGN_CH,
	OPENMODE_CAM_STATE_OK,

	OPENMODE_CAM_STATE_INVALID_IP,
	OPENMODE_CAM_STATE_PW_CHANGE,

	OPENMODE_CAM_STATE_CONN_FAIL,
	OPENMODE_CAM_STATE_LOGIN_FAIL,
	OPENMODE_CAM_STATE_STREAM_FAIL,
	OPENMODE_CAM_STATE_UNSUPPORTED,
	OPENMODE_CAM_STATE_VIRTUAL_CAMERA,
	OPENMODE_CAM_STATE_STREAM_OPEN_REQ,

	OPENMODE_CAM_STATE_CLOSING
} OPENMODE_CAM_STATE_ENUM;

enum __OPENMODE_SORT_DIRECTION_ENUM_
{
	OPENMODE_SORT_ASC = 0,
	OPENMODE_SORT_DESC
};

/* virtual cam add state, in use nf_openmode_add_virtual_camera() */
typedef enum __OPENMODE_STATE_VIRTUAL_CAM_ADD_
{
	OPENMODE_STATE_VIRTUAL_CAM_ADD_OK = 0,
	OPENMODE_STATE_VIRTUAL_CAM_ADD_FAIL,
	OPENMODE_STATE_VIRTUAL_CAM_ADD_MAIN_ADDR_FAIL,
	OPENMODE_STATE_VIRTUAL_CAM_ADD_SECOND_ADDR_FAIL,
} OPENMODE_STATE_VIRTUAL_CAM_ADD;

/* virtual cam supported */
typedef enum __OPENMODE_STATE_VIRTUAL_CAM_
{
	OPENMODE_STATE_VIRTUAL_UNSUPPORTED = 0,
	OPENMODE_STATE_VIRTUAL_SUPPORTED
} OPENMODE_STATE_VIRTUAL_CAM;

typedef enum __OPENMODE_DEV_CATEGORY_
{
	OPENMODE_DEV_CAM = 0,
	OPENMODE_DEV_NVR = 1,
	OPENMODE_DEV_AIBOX = 2,
} OPENMODE_DEV_CATEGORY;

typedef struct _NFOpenmodeVirtualCamera NFOpenmodeVirtualCamera;
struct _NFOpenmodeVirtualCamera
{
	gchar u_id[64];
	gchar u_password[64];
	gchar host[256];
	guint ipaddr;
	guint rtsp_port;
};


typedef struct _NFOpenmodeCamInfo NFOpenmodeCamInfo;
struct _NFOpenmodeCamInfo
{
	gint index;
	gint state;

	gint ch;
	guint xid;

	gchar hostname[256];
	gchar gwstr[16];
	gchar maskstr[16];
	gchar dns1str[16];
	gchar dns2str[16];
	gchar eth_dev[16];

	gint is_dhcp;
	guint ipaddr;
	guint gw;
	guint mask;
	guint dns1;
	guint dns2;
	guint http_port;
	guint rtsp_port;
	guchar macaddr[8];

	gint ipaddr_pri;

	gchar model[64];
	gchar model_std[64];
	gchar firmware_version[64];
	gchar firmware_version2[64];
	gchar vendor[64];
	gchar sdkver[64];

	gchar tail[256];
	gchar media_xaddr[256];
	gchar token[64];

	gchar u[64];
	gchar p[64];

	gint auth;
	gint use_ssl;
	gchar u_done[64];
	gchar p_done[64];

	gchar preview_rtsp[256];

	/* virtual camera */
	guint virtual_camera;
	guint vcam_cnt;
	gchar vcam_rtsp_addr[2][256]; 
	guint vcam_audio_flag;

	gchar zoom_module_name[64];
	gchar zoom_module_fwver[64];

	gchar capa_version[16];

    pthread_mutex_t state_mutex;

    OPENMODE_DEV_CATEGORY dev_cate;

	NFOpenmodeCamInfo *prev;
	NFOpenmodeCamInfo *next;
};

typedef struct _NFOpenmodeDeviceList NFOpenmodeDeviceList;
struct _NFOpenmodeDeviceList
{
	gint assigned_cnt;
	gint entry_cnt;
	gint recognized_cnt;
	gint setup_needed_cnt;

	NFOpenmodeCamInfo *head;
	NFOpenmodeCamInfo *tail;
};

typedef struct _NFOpenmodeSetupNetwork NFOpenmodeSetupNetwork;
struct _NFOpenmodeSetupNetwork
{
	guint is_dhcp;
	guint ipaddr;
	guint mask;
	guint gw;
	guint dns1;
	guint dns2;

	guint http_port;
	guint rtsp_port;
};

typedef struct _NFOpenmodeSetupPorts NFOpenmodeSetupPorts;
struct _NFOpenmodeSetupPorts
{
	guint http_port;
	guint rtsp_port;
};

jpeg_image_data nf_openmode_get_snapshot_image(NFOpenmodeCamInfo *iter);

/* To show list */
NFOpenmodeDeviceList* nf_openmode_get_ch_list(void);
NFOpenmodeDeviceList* nf_openmode_get_list(void);

/* Stopping video/audio streaming */
void nf_openmode_stop_streaming(void);

/* To build-up initial channel list */
void nf_openmode_init_detection_list(void);
void nf_openmode_cancel(void);
void nf_openmode_apply(void);
void nf_openmode_finalize_installation(void);

/* Search */
void nf_openmode_scan_camera(void);

/* Range search */
/* void nf_openmode_scan_camera_range(guint s, guint e); */

/* Manual add */
gint nf_openmode_add_device_manual(gchar* host, guint port);

/* Virtual Camera(RTSP addres) */
gint nf_openmode_add_virtual_camera(gchar* host_main, gchar* host_second, gchar* model_name, OPENMODE_STATE_VIRTUAL_CAM_ADD *v_state);

/* To setup camera */
OPENMODE_RTN_ENUM nf_openmode_change_password(gint index, gchar *p);
OPENMODE_RTN_ENUM nf_openmode_set_login_info(gint index, gchar *u, gchar *p);
OPENMODE_RTN_ENUM nf_openmode_request_ip_assign(int, NFOpenmodeSetupNetwork*);
OPENMODE_RTN_ENUM nf_openmode_request_port_assign(int, NFOpenmodeSetupPorts*);
void nf_openmode_set_preview(gint index, gint is_chlist);
void nf_openmode_set_channel(int index, int ch);
void nf_openmode_set_channel_no_noti(int index, int ch);

/* openmode list manip. */
OPENMODE_RTN_ENUM nf_custommode_sort_by_lan(int dir);
OPENMODE_RTN_ENUM nf_openmode_sort_by_model(int dir);
OPENMODE_RTN_ENUM nf_openmode_sort_by_ip(int dir);
OPENMODE_RTN_ENUM nf_openmode_sort_by_status(int dir);
OPENMODE_RTN_ENUM nf_openmode_sort_by_ch(int dir);

extern int nf_openmode_is_installing(void);

/* for the bug fix */
NFOpenmodeCamInfo* nf_openmode_get_dlist_info_by_chlist_index(int index);



/* Internal APIs: Do not call */
void nf_openmode_init(void);
void nf_openmode_start(void);
void nf_openmode_stop(void);
void nf_openmode_finalize(void);
void nf_openmode_show_list(void);
void nf_openmode_show_ch_list(void);
void nf_openmode_show_entry(gchar*, gint index);
void nf_openmode_empty_list(void);

extern int search_ipcam_model(NFOpenmodeCamInfo* entry);

#endif // __NF_API_OPENMODE_H__

