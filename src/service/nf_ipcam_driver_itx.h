/*
 * ITX Security
 *  System software group
 *
 *  2011-09-14 jykim
 */

#ifndef __NF_IPCAM_DRIVER_ITX_H__
#define __NF_IPCAM_DRIVER_ITX_H__

#include <nf_ptz.h>
#include <nf_ipcam_defs.h>
#include "ivca_def.h"

extern int cam_set_install_mode_off(int cam_id);
extern int cam_factory_mode(int cam_id);
extern int cam_set_osd_off(int cam_id);
extern int cam_reboot(int cam_id);
extern int cam_factory_default(int cam_id);
extern int ti368_factory_default(int cam_id);
extern int _cam_factory_default(int cam_id);
extern int cam_set_vcodec(cam_info* info_set, int cam_id);
extern int cam_set_vcodec_a2(cam_info* info_set, int cam_id);
extern int cam_set_vcodec_d1(cam_info* info_set, int cam_id);
extern int cam_set_acodec(cam_info* info_set, int cam_id);
extern int cam_set_image(image_info* info_set, int cam_id);
extern int cam_set_image_nmx(image_info* info_set, int cam_id);
extern int cam_set_dnn_adjust_d2n(image_info* info_set, int cam_id);
extern int cam_set_dnn_adjust_n2d(image_info* info_set, int cam_id);
extern int cam_set_zoom(int value, int cam_id);
extern int cam_set_focus(int value, int cam_id);
extern int cam_set_ptz_stop(PTZ_FUNCS_E e, int cam_id);
extern int cam_set_onepush(int cam_id);
extern int cam_set_origin(int cam_id);
extern int cam_set_alarm(cam_info* info_set, int cam_id);
extern int cam_set_motion_area_a2(motion_t *motion_info, int cam_id);
extern int cam_get_motion_smart(NFIPCamSetupMotionSmart *motion_info, int cam_id);
extern int cam_set_motion_smart(NFIPCamSetupMotionSmart *motion_info, int cam_id);
extern int cam_get_motion_area(NFIPCamSetupMotionArea *motion_info, int cam_id);
extern int cam_set_motion_area(NFIPCamSetupMotionArea *motion_info, int cam_id);
extern int cam_get_zoom(int*, int);
extern int cam_get_focus(int*, int);
extern int cam_set_piris(int, int);
extern int cam_get_piris(int*, int);
extern int cam_set_pmask(void* info_set, int cam_id);
extern int cam_set_corridor_mode(int, int);

extern int cam_set_initial_pw( unsigned int, unsigned short, char*, char*, char*);
extern int cam_get_model_info_raw(cam_model_info*, unsigned int, unsigned short, char*, char*, int*);
extern int cam_get_model_info_by_sysdb(cam_model_info*, int);
extern int cam_get_model_info_by_backdoor(cam_model_info*, int);
extern int cam_get_model_info(cam_model_info*, int);
extern int cam_get_af_capa(cam_info *value, int cam_id);
extern int cam_get_image(image_info* info_set, int cam_id);
extern int cam_set_roi_area(NFIPCamSetupROIArea *roi_info, int cam_id);

//captainnn
extern int cam_set_enable_va(int value, int cam_id);
extern int cam_set_reset_va( int cam_id);
extern int cam_set_va_config(ivca_rule_t* value, int cam_id);
extern int cam_set_va_option(ivca_option_t* value, int cam_id);

extern int cam_set_network(unsigned int, unsigned short, char*, char*, void*);

extern int itx_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf);

extern int cam_upload_fw(int cam_id, char *file_nm, char *file_stream, int file_len);
extern int init_remember_buf(void);
extern int itx_digest_setup_again(int cam_id, int type, char* rbuf);

extern int cam_set_focus_comp(focus_comp_info* info, int cam_id);
extern int cam_set_dc_iris_cal(int cam_id);
extern int _common_get_dc_iris_cal_status(int cam_id);
#endif
