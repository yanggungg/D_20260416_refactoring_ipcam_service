#ifndef __VW_ARCHIVING_TAB1_H
#define __VW_ARCHIVING_TAB1_H

#include "objects/nfobject.h"

void vw_init_arch_tab1_page(NFOBJECT *parent);
void vw_arch_set_from_utime(guint utime);
void vw_arch_set_to_utime(guint utime);
gboolean vw_arch_tab1_in_handler();
gboolean vw_arch_tab1_out_handler();
gboolean vw_arch_tab1_show();

#endif



















