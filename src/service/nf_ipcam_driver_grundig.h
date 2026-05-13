/*
 * ITX Security
 *  System software group
 *
 *  2012-03-17 jykim
 */

#ifndef __NF_IPCAM_DRIVER_GRUNDIG_H__
#define __NF_IPCAM_DRIVER_GRUNDIG_H__

#include <nf_ipcam_defs.h>


extern int grundig_hdwdr_set_resolution(int cam_id);
extern int grundig_vseries_set_resolution(int cam_id);
extern int grundig_ipptz_set_resolution(int cam_id);
extern int grundig_fhdms_set_resolution(int cam_id);
extern int grundig_set_gop(int cam_id);
extern int grundig_set_audio(cam_info *info, int cam_id);

extern int grundig_ipptz_set_preset(int num, int cam_id);
extern int grundig_ipptz_go_preset(int num, int cam_id);
extern int grundig_ipptz_set_pt(NF_PTZ_CMD_E ptz_cmd, int cam_id);
extern int grundig_ipptz_set_zoom(NF_PTZ_CMD_E ptz_cmd, int cam_id);
extern int grundig_ipptz_set_focus(NF_PTZ_CMD_E ptz_cmd, int cam_id);
extern int grundig_ipptz_set_origin(int cam_id);
extern int grundig_ipptz_set_autofocus_mode(int enable, int cam_id);
extern int grundig_ipptz_set_ptz_stop(PTZ_FUNCS_E e, int cam_id);

extern int grundig_set_motion_area(NFIPCamSetupMotionArea* minfo, int cam_id);
extern int grundig_common_set_ptz_property(int cam_id);
extern int grundig_common_set_audio_property(int cam_id);

extern int grundig_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf);
extern int grundig_common_get_onthefly(int cam_id);
extern int grundig_common_adjust_vcodec(cam_info* info_set, int cam_id);
#endif	//__NF_IPCAM_DRIVER_GRUNDIG_H__
