#ifndef _VW_DISK_INFORMATION_H_
#define _VW_DISK_INFORMATION_H_

#include "objects/nfobject.h"


void VW_Init_DiskInfo_Page(NFOBJECT *parent);
gboolean VW_DiskInfo_tab_out_handler();

// internal used function
gboolean display_disk_inforamtion(gboolean expose);
#endif
