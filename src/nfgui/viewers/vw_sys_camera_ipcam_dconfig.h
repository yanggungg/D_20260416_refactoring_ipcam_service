#ifndef _SETUP_IPCAM_DIRECT_CONFIG_H_
#define _SETUP_IPCAM_DIRECT_CONFIG_H_

#include "objects/nfobject.h"

void VW_Init_IPCamDirectConfig_Page(NFOBJECT *parent, gint ch, CAM_PROFILE_T *prof);

gint IPCamDirectConfig_update_channel(gint ch);

gboolean IPCamDirectConfig_tab_in_handler();
gboolean IPCamDirectConfig_tab_out_handler();
	
#endif

