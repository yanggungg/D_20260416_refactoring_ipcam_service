#ifndef _VW_RECORD_PANIC_H_
#define _VW_RECORD_PANIC_H_

#include "objects/nfobject.h"

void VW_Init_RecPanic_Page(NFOBJECT *parent);
gboolean VW_RecPanic_tab_out_handler();
void VW_RecPanic_info_refresh();


#endif


