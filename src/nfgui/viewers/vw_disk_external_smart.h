#ifndef _VW_DISK_EXTERNAL_SMART_H_
#define _VW_DISK_EXTERNAL_SMART_H_

#include "objects/nfobject.h"

void VW_Init_DiskExternal_Smart_Page(NFOBJECT *parent);

gboolean check_ext_smart_data_changed();
void save_ext_smart_data();
void restore_ext_smart_data();
gboolean display_external_disk_smart();
#endif
