#ifndef _VW_DISK_INTERNAL_INFORMATION_H_
#define _VW_DISK_INTERNAL_INFORMATION_H_

#include "objects/nfobject.h"


void VW_Init_DiskInternal_Info(NFOBJECT *parent);

// internal used function
gboolean display_internal_disk_information(gboolean expose);

#endif

