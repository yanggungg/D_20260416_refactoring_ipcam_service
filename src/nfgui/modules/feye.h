/*
 * feye.h
 *  - fisheye param module
 *
 * written by parangi
 *
 */

#ifndef __FEYE_H
#define __FEYE_H

#include "nf_api_live.h"

int feye_init();

int feye_set_dewarping_channel(int ch);
int feye_set_video_param_data(int ch, NF_FISHEYE_VIDEO_PARAM *param);
int feye_set_ptz_param_data(int ch, NF_FISHEYE_PTZ_PARAM *param);

int feye_block_save_data();
int feye_flush_save_data();

#endif
