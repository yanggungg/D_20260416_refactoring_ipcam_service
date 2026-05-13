#ifndef _SETUP_CAM_CAM_TITLE_H_
#define _SETUP_CAM_CAM_TITLE_H_

#include "objects/nfobject.h"


void init_CamSetup_page(NFOBJECT *parent);

gint init_CamSetup_start_preview(NFOBJECT *parent);
gboolean CamTitle_tab_in_handler();
gboolean CamTitle_tab_out_handler();

#endif

