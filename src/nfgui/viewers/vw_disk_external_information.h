#ifndef _VW_DISK_EXTERNAL_INFORMATION_H_
#define _VW_DISK_EXTERNAL_INFORMATION_H_

#include "objects/nfobject.h"


void VW_Init_DiskExternal_Info(NFOBJECT *parent);

// internal used function
gboolean display_external_disk_information(gboolean expose);

#endif

