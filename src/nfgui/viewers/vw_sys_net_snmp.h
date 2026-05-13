#ifndef _SETUP_NETWORK_SNMP_H_
#define _SETUP_NETWORK_SNMP_H_

#include "objects/nfobject.h"


void init_NetSnmp_page(NFOBJECT * parent);

gboolean NetSnmp_tab_out_handler();
gboolean vw_snmp_radio_button_change(NFOBJECT *parent);

#endif
