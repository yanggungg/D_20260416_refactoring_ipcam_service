#ifndef _SETUP_IPCAM_FOCUS_H_
#define _SETUP_IPCAM_FOCUS_H_

#include "objects/nfobject.h"

void VW_Init_IPCamFocus_Page(NFOBJECT *parent, gint ch);

gint IPCamFocus_update_channel();

gboolean IPCamFocus_tab_in_handler();
gboolean IPCamFocus_tab_out_handler();
	
#endif

