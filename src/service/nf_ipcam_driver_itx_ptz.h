/*
 * ITX Security
 *  System software group
 *
 *  2011-09-14 jykim
 */

#ifndef __NF_IPCAM_DRIVER_ITX_PTZ_H__
#define __NF_IPCAM_DRIVER_ITX_PTZ_H__

#include <nf_ptz.h>
#include <nf_ipcam_defs.h>

extern int npt_ptz_init(int);
extern int npt_set_install_mode_off(int);
extern int npt_set_vcodec(cam_info*, int);
extern int npt_set_dnn_adjust_d2n(image_info* info_set, int cam_id);
extern int npt_set_dnn_adjust_n2d(image_info* info_set, int cam_id);
extern int npt_set_image(image_info*, int);
extern int npt_get_af_capa(cam_info*, int);
extern int npt_set_onepush(int cam_id);
extern int npt_set_pt(NF_PTZ_CMD_E, int);
extern int npt_set_zoom(NF_PTZ_CMD_E, int);
extern int npt_set_focus(NF_PTZ_CMD_E, int);
//extern int npt_set_iris(NF_PTZ_CMD_E, int);
extern int npt_set_iris(int, int);
extern int npt_set_origin(int);
extern int npt_set_autofocus_mode(int, int);
extern int npt_set_autoiris_mode(int, int);
extern int npt_set_ptz_stop(PTZ_FUNCS_E, int);
extern int npt_get_pan(int*, int);
extern int npt_get_tilt(int*, int);
extern int npt_get_zoom(int*, int);
extern int npt_get_focus(int*, int);
extern int npt_get_iris(int*, int);
extern int npt_set_preset(int, int);
extern int npt_clear_preset(int, int);
extern int npt_go_preset(int, int);
extern int npt_set_pmask(NFIPCamPrivacyMask* pmask_info, int cam_id);
extern int npt_goto_pmask(int pmask_no, int cam_id);

#endif
