
#ifndef _SETUP_INSTALL_MODE_H_
#define _SETUP_INSTALL_MODE_H_

#include "objects/nfobject.h"

void init_InstallMode_page(NFOBJECT *parent);

gboolean InstallMode_tab_in_handler();
gboolean InstallMode_tab_out_handler();

#endif

