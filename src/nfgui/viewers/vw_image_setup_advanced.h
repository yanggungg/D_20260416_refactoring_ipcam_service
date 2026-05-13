
#ifndef _VW_IMGSET_ADVANCED_H_
#define _VW_IMGSET_ADVANCED_H_

#include "objects/nfobject.h"
#include "objects/nfwindow.h"

#include "nf_api_ipcam.h"

void vw_open_image_setup_advanced(NFWINDOW *parent, guint ch, ColorData *color_data, AdvancedData *advanced_data, NFIPCamImageProfile *ipcam_pf);
void vw_destroy_image_setup_advanced();
	
#endif

