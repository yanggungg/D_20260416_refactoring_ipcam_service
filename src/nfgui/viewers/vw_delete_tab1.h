#ifndef __VW_DELETE_TAB1_H
#define __VW_DELETE_TAB1_H

#include "objects/nfobject.h"

void vw_init_del_tab1_page(NFOBJECT *parent);
gboolean vw_del_tab1_in_handler();
gboolean vw_del_tab1_out_handler();
gboolean vw_del_tab1_show();

#endif
