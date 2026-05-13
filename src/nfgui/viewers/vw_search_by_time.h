#ifndef __VW_SEARCH_BY_TIME_H
#define __VW_SEARCH_BY_TIME_H

#include "objects/nfobject.h"

void vw_init_SearchByTime_page(NFOBJECT *parent);
gboolean vw_SearchByTime_tab_out_handler();
gboolean vw_SearchByTime_tab_in_handler();
gboolean vw_SearchByTime_tab_show();

#endif

