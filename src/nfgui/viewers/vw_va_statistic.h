#ifndef __VW_VA_STATISTIC_H
#define __VW_VA_STATISTIC_H

#include "objects/nfobject.h"

void vw_init_va_statistic_page(NFOBJECT *parent);

gboolean vw_VaStatistic_tab_out_handler();
gboolean vw_VaStatistic_tab_in_handler();
gboolean vw_VaStatistic_tab_show();

#endif
