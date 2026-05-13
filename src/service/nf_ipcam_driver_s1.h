/*
 * ITX Security
 *  System software group
 *
 *  2012-03-17 jykim
 */

#ifndef __NF_IPCAM_DRIVER_S1_H__
#define __NF_IPCAM_DRIVER_S1_H__

#include <nf_ipcam_defs.h>

extern int s1_get_event_cap(motion_t* info, int cam_id);

extern int s1_get_motion_area(int cam_id);
extern int s1_set_motion_area(NFIPCamSetupMotionArea* info, int cam_id);
extern int s1_get_preset_motion(int cam_id);
extern int s1_set_preset_motion(int cam_id);

extern int s1_get_mirror_cap(int* capability, int cam_id);
extern int s1_get_mirror_val(int cam_id);
extern int s1_set_mirror_val(cam_info* info, int cam_id);

extern int s1_get_onepush_cap(int* capability, int cam_id);
extern int s1_set_onepush(int cam_id);

extern int s1_factory_clear(int cam_id, const char* user, const char* pass, char* macaddr);
extern int s1_factory_default(int cam_id);

#endif	// __NF_IPCAM_DRIVER_S1_H__
