#ifndef __MOTION_SENSOR_SETUP_H__
#define __MOTION_SENSOR_SETUP_H__

#include "objects/nfobject.h"

void init_IPCam_MotSen_page(NFOBJECT *parent);

gint IPCam_MotSen_cur_tab_page();
gboolean IPCam_MotSen_tab_in_handler();
gboolean IPCam_MotSen_tab_out_handler();

#endif

