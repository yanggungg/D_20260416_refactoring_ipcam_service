#ifndef _VW_DISK_SMART_H_
#define _VW_DISK_SMART_H_

#include "objects/nfobject.h"

void VW_Init_DiskSmart_Page(NFOBJECT *parent);
gboolean VW_DiskSmart_tab_out_handler();

// internal used function
gboolean display_disk_smart(gboolean expose);
#endif
