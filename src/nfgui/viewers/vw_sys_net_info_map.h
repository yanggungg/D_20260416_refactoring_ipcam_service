
#ifndef _VW_SYS_NET_INFO_MAP_
#define _VW_SYS_NET_INFO_MAP_

#include "objects/nfobject.h"

void vw_init_NetInfoMap_Page(NFOBJECT *parent);
gboolean NetInfoMap_tab_out_handler();
gboolean NetInfoMap_tab_in_handler();
gboolean NetInfoMap_update_ddns_addr(void);
gboolean NetInfoMap_update_sequrinet_status();

#endif



