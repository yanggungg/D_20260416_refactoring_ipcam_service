#ifndef _VW_DISK_RAID_EXTERNAL_H_
#define _VW_DISK_RAID_EXTERNAL_H_

#include "objects/nfobject.h"

void VW_Init_DiskExternal_Raid_Page(NFOBJECT *parent);
gboolean display_external_raid(gboolean expose);
gboolean check_disk_ext_raid_conf_changed();
gboolean apply_disk_ext_raid_conf();

#endif
