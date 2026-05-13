#ifndef _SETUP_CAM_FISHEYE_H_
#define _SETUP_CAM_FISHEYE_H_

#include "objects/nfobject.h"


void init_FisheyeSetup_page(NFOBJECT *parent);
void FisheyeSetup_Show_Preview();

gboolean FisheyeSetup_tab_out_handler();
gboolean FisheyeSetup_tab_in_handler();

#endif

