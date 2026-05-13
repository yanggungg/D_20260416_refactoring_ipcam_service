#ifndef _VW_DISK_RAID_INTERNAL_H_
#define _VW_DISK_RAID_INTERNAL_H_

#include "objects/nfobject.h"

void VW_Init_DiskInternal_Raid_Page(NFOBJECT *parent);
gboolean display_internal_raid(gboolean expose);
gboolean check_disk_int_raid_conf_changed();
gboolean apply_disk_int_raid_conf();

#endif

