#ifndef _VW_SYS_SETUP_CAMERA_DEEPLEARNING_SCHEDULE_H_
#define _VW_SYS_SETUP_CAMERA_DEEPLEARNING_SCHEDULE_H_

#include "objects/nfobject.h"

void init_CamDeepLearning_Schedule_Page(NFOBJECT *parent);

gboolean CamDeepLearning_Schedule_tab_in_handler();
gboolean CamDeepLearning_Schedule_tab_out_handler();

#endif

