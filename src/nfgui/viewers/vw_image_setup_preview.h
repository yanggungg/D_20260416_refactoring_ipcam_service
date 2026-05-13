
#ifndef _VW_IMGSET_SETUP_PREVIEW_H_
#define _VW_IMGSET_SETUP_PREVIEW_H_

#include "objects/nfobject.h"
#include "objects/nfwindow.h"

#include "nf_api_ipcam.h"

void vw_open_image_setup_preview(NFWINDOW *parent, guint ch, ColorData *data, NFIPCamImageProfile *ipcam_pf);
	
#endif

