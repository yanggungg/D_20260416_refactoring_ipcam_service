#ifndef __SETUP_MOTION_SENSOR_H__
#define __SETUP_MOTION_SENSOR_H__

#include "objects/nfobject.h"

void init_MotSen_page(NFOBJECT *parent);

gboolean MotSen_tab_in_handler();
gboolean MotSen_tab_out_handler();

#endif


