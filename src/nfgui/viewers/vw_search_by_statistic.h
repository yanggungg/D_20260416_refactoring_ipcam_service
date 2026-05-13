#ifndef __VW_SEARCH_BY_STATISTIC_H
#define __VW_SEARCH_BY_STATISTIC_H

#include "objects/nfobject.h"

void vw_init_SearchBy_statistic_page(NFOBJECT *parent);

gboolean vw_VaStatistic_tab_out_handler();
gboolean vw_VaStatistic_tab_in_handler();
gboolean vw_VaStatistic_tab_show();

#endif
