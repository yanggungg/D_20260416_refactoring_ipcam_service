#ifndef _VW_SYS_NET_EVT_H_
#define _VW_SYS_NET_EVT_H_

#include "objects/nfobject.h"

void VW_Init_SysNet_Evt_Page(NFOBJECT *parent);

gboolean check_system_network_data_changed();
void save_system_network_data();
void restore_system_network_data();

#endif




