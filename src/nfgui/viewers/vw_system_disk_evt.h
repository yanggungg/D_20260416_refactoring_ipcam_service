#ifndef _VW_SYS_DISK_EVT_H_
#define _VW_SYS_DISK_EVT_H_

#include "objects/nfobject.h"

void VW_Init_SysDisk_Evt_Page(NFOBJECT *parent);

gboolean check_system_disk_data_changed();
void save_system_disk_data();
void restore_system_disk_data();
#endif
