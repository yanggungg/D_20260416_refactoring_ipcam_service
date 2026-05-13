/*
 * ITX Security
 *  System software group
 *
 *  2012-10-18 YiDongHyung
 */

#ifndef __NF_IPCAM_DRIVER_NVS_H__
#define __NF_IPCAM_DRIVER_NVS_H__

#include <nf_ptz.h>
#include <nf_ipcam_defs.h>

//extern int cam_set_install_mode_off(int cam_id);
//extern int cam_factory_mode(int cam_id);
//extern int cam_set_osd_off(int cam_id);
//extern int cam_reboot(int cam_id);
//extern int cam_factory_default(int cam_id);
//extern int cam_set_vcodec_a2(cam_info* info_set, int cam_id);
//extern int cam_set_vcodec_d1(cam_info* info_set, int cam_id);
//extern int cam_set_acodec(cam_info* info_set, int cam_id);

extern int nvs_set_init(int cam_id);
extern int nvs_set_vcodec(cam_info* info_set, int cam_id);
extern int nvs_set_image(image_info* info_set, int cam_id);
extern int nvs_set_pt(NF_PTZ_CMD_E, int);
extern int nvs_set_zoom(NF_PTZ_CMD_E, int);
extern int nvs_set_focus(NF_PTZ_CMD_E, int);
extern int nvs_set_iris(NF_PTZ_CMD_E, int);
extern int nvs_set_ptz_stop(PTZ_FUNCS_E, int);
extern int nvs_set_preset(int, int);
extern int nvs_clear_preset(int, int);
extern int nvs_go_preset(int, int);
extern int nvs_set_autofocus_mode(int, int);
extern int nvs_set_autoiris_mode(int, int);
extern int nvs_get_af_capa(cam_info*, int);

extern int nvs_get_model_info(cam_model_info*, int, int);
extern int nvs_video_handler_itx();
extern int nvs_close_all_sub_ch(int port_num);
extern void nvs_open_stream(int target_port);
extern void nvs_close_stream(int target_port);
extern void nvs_subch_check_physical_link_on(int target_port);

extern int nvs_itx_recv_buf_handler(int cam_id, NF_IPCAM_SETUP_TYPE_E type, char* recv_buf);

#endif
