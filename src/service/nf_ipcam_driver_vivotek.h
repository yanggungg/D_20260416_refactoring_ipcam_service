/*
 * ITX Security
 *  System software group
 *
 *  2012-03-17 jykim
 */

#ifndef __NF_IPCAM_DRIVER_VIVOTEK_H__
#define __NF_IPCAM_DRIVER_VIVOTEK_H__

#include <nf_ipcam_defs.h>


extern int vivotek_set_video(cam_info* info_set, int cam_id);


extern int vivotek_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf);

#endif	// __NF_IPCAM_DRIVER_VIVOTEK_H__
