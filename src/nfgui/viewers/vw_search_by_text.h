#ifndef __VW_SEARCH_BY_TEXT_H
#define __VW_SEARCH_BY_TEXT_H

#include "objects/nfobject.h"

void vw_init_SearchByText_page(NFOBJECT *parent, time_t from_time, time_t to_time);
gboolean vw_SearchByText_tab_out_handler();
gboolean vw_SearchByText_tab_in_handler();
gboolean vw_SearchByText_tab_show();
gboolean vw_SearchByText_set_interval(time_t from_time, time_t to_time);

#endif

