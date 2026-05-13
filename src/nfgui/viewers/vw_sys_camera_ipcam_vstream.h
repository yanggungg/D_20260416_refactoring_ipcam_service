#ifndef _SETUP_IPCAM_VIDEO_STREAM_H_
#define _SETUP_IPCAM_VIDEO_STREAM_H_

#include "objects/nfobject.h"

void VW_Init_IPCamVideoStream_Page(NFOBJECT *parent, gint ch);

gint IPCamVideoStream_update_channel();

gboolean IPCamVideoStream_tab_in_handler();
gboolean IPCamVideoStream_tab_out_handler();

gint IPCamVideoStream_refresh_data();
	
#endif

