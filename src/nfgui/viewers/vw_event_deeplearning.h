#ifndef _VW_EVENT_DEEPLEARNING_H_
#define _VW_EVENT_DEEPLEARNING_H_

#include "objects/nfobject.h"

void VW_Init_DeepLearning_Evt_Page(NFOBJECT *parent);

gboolean VW_DeepLearning_Evt_tab_in_handler();
gboolean VW_DeepLearning_Evt_tab_out_handler();

#endif

