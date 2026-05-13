#ifndef _VW_MOTION_SENSOR_SETUP_H_
#define _VW_MOTION_SENSOR_SETUP_H_

#include "objects/nfobject.h"

void init_MotSenSetup_Page(NFOBJECT *parent);

// TEMPORARY API
void MotSen_Show_Preview();
void MotSen_Pause_Preview();
void MotSen_Stop_Preview();

#endif
