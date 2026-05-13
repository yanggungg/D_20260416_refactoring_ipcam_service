#ifndef _VW_SYS_SETUP_CAMERA_DEEPLEARNING_SETUP_H_
#define _VW_SYS_SETUP_CAMERA_DEEPLEARNING_SETUP_H_

#include "objects/nfobject.h"

void init_CamDeepLearning_page(NFOBJECT *parent);

gboolean CamDeepLearning_tab_in_handler();
gboolean CamDeepLearning_tab_out_handler();

#endif

