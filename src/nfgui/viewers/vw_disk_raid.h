#ifndef _VW_DISK_RAID_H_
#define _VW_DISK_RAID_H_

#include "objects/nfobject.h"

enum {
	RAID_NONE_CONF = -1,
	RAID1_CONF = 1,
	RAID5_CONF = 5
};

void VW_Init_DiskRaid_Page(NFOBJECT *parent);
gboolean VW_DiskRaid_tab_out_handler();

#endif

