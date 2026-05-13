#ifndef _SETUP_CAM_PTZ_H_
#define _SETUP_CAM_PTZ_H_

#include "objects/nfobject.h"


void init_PtzSetup_page(NFOBJECT *parent);

gboolean PtzSetup_tab_out_handler();
gboolean PtzSetup_tab_in_handler();

#endif

