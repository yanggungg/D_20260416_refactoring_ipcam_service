#ifndef _VW_EVENT_ANALYSYS_H_
#define _VW_EVENT_ANALYSIS_H_

#include "objects/nfobject.h"

void VW_Init_Analysis_Evt_Page(NFOBJECT *parent);

gboolean VW_Analysis_Evt_tab_in_handler();
gboolean VW_Analysis_Evt_tab_out_handler();

#endif

