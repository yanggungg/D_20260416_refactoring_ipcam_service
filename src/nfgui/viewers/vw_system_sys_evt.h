#ifndef _VW_SYS_SYS_EVT_H_
#define _VW_SYS_SYS_EVT_H_

#include "objects/nfobject.h"

void VW_Init_SysSys_Evt_Page(NFOBJECT *parent);

gboolean check_system_system_data_changed();
void save_system_system_data();
void restore_system_system_data();
#endif

