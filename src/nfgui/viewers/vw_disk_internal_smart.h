#ifndef _VW_DISK_INTERNAL_SMART_H_
#define _VW_DISK_INTERNAL_SMART_H_

#include "objects/nfobject.h"

void VW_Init_DiskInternal_Smart_Page(NFOBJECT *parent);

gboolean check_int_smart_data_changed();
void save_int_smart_data();
void restore_int_smart_data();
gboolean display_internal_disk_smart(gboolean expose);
#endif
