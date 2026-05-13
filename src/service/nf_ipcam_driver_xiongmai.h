/*
 * ITX Security
 *  System software group
 *
 *  2015-07-20 jykim
 */


#ifndef __NF_IPCAM_DRIVER_XIONGMAI_H__
#define __NF_IPCAM_DRIVER_XIONGMAI_H__

#include <nf_ipcam_defs.h>


#define XM_DATA_BUF_SIZE (8192)

typedef struct _IPCAM_XM_BUFS__ NF_IPCAM_XM_BUFS;
struct _IPCAM_XM_BUFS__
{
	char* pw_enc;
	char *post_api;
	char *http_api;
	char *buf;
	char **dstbuf;
};

extern int xiongmai_alarm_server_set(int cam_id);
extern int xiongmai_osd_off(int cam_id);
extern int xiongmai_prevent_periodical_reboot(int cam_id);
extern int xiongmai_stream_set(int cam_id, cam_info* info);
extern int xiongmai_motion_set(NFIPCamSetupMotionArea *motion_info, int cam_id);

extern int xiongmai_event_notification_handler(char* msg);

#endif	//__NF_IPCAM_DRIVER_XIONGMAI_H__
