#ifndef _SETUP_IPCAM_IMAGE_SET_H_
#define _SETUP_IPCAM_IMAGE_SET_H_

#include "objects/nfobject.h"

void VW_Init_IPCamImageSet_Page(NFOBJECT *parent, gint ch, CAM_PROFILE_T *prof);

gint IPCamImageSet_update_channel(gint ch);

gint IPCamImageSet_video_show();
gint IPCamImageSet_video_hide();

gboolean IPCamImageSet_tab_in_handler();
gboolean IPCamImageSet_tab_out_handler();
	
#endif

