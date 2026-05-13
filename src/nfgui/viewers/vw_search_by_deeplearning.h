#ifndef __VW_SEARCH_BY_DEEPLEARNING_H
#define __VW_SEARCH_BY_DEEPLEARNING_H

#include "objects/nfobject.h"

void vw_init_SearchByDeepLearning_page(NFOBJECT *parent, time_t from_time, time_t to_time, gchar *data);
gboolean vw_SearchByDeepLearning_tab_in_handler();
gboolean vw_SearchByDeepLearning_tab_out_handler();
gboolean vw_SearchByDeepLearning_tab_show();
gboolean vw_SearchByDeepLearning_set_interval(time_t from_time, time_t to_time);

#endif

