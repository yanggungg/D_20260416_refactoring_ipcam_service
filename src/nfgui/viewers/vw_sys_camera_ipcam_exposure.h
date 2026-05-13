#ifndef _SETUP_IPCAM_EXPOSURE_H_
#define _SETUP_IPCAM_EXPOSURE_H_

#include "objects/nfobject.h"

void VW_Init_IPCamExposure_Page(NFOBJECT *parent, gint ch, CAM_PROFILE_T *prof);

gint IPCamExposure_update_channel(gint ch);

gint IPCamExposure_video_show();
gint IPCamExposure_video_hide();

gboolean IPCamExposure_tab_in_handler();
gboolean IPCamExposure_tab_out_handler();

#endif

