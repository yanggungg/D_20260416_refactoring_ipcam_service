#ifndef _SETUP_CAM_PTZ_H_
#define _SETUP_CAM_PTZ_H_

#include "objects/nfobject.h"

void VW_Init_IPCamPtz_Page(NFOBJECT *parent, gint ch);

gint IPCamPtz_update_channel();

gboolean IPCamPtz_tab_in_handler();
gboolean IPCamPtz_tab_out_handler();
	
#endif

