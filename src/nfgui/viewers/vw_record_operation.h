
#ifndef _VW_RECORD_OPERATION_H_
#define _VW_RECORD_OPERATION_H_

#include "objects/nfobject.h"

void VW_Init_RecOperation_Page(NFOBJECT *parent);
guint VW_RecOperation_get_pre_time();
guint VW_RecOperation_get_post_time();
void VW_RecOperation_set_pre_time(guint val);
void VW_RecOperation_set_post_time(guint val);
gboolean VW_RecOperation_tab_out_handler();

#endif

