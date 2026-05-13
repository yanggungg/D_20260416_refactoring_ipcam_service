/*
 * ITX Security
 *  System software group
 *
 *  2012-03-27 jykim
 */

#ifndef __NF_IPCAM_DRIVER_MESSOA_H__
#define __NF_IPCAM_DRIVER_MESSOA_H__


extern int messoa_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf);

extern int i3_set_vcodec(cam_info* info_set, int cam_id);
extern int i3_set_image(image_info* info, int cam_id);
extern int i3_poll_alarm_status(int cam_id);

#endif
