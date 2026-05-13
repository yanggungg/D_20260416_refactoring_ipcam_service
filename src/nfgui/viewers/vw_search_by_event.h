#ifndef __VW_SEARCH_BY_EVENT_H
#define __VW_SEARCH_BY_EVENT_H

#include "objects/nfobject.h"

void vw_init_SearchByEvent_page(NFOBJECT *parent, time_t from_time, time_t to_time);
gboolean vw_SearchByEvent_tab_out_handler();
gboolean vw_SearchByEvent_tab_in_handler();
gboolean vw_SearchByEvent_tab_show();
gboolean vw_SearchByEvent_set_interval(time_t from_time, time_t to_time);

#endif

