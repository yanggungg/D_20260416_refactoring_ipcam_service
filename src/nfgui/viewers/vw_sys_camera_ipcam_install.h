
#ifndef _SETUP_IPCAM_INSTALL_H_
#define _SETUP_IPCAM_INSTALL_H_

#include "objects/nfobject.h"

void init_IPCamInstall_page(NFOBJECT *parent);

gboolean IPCamInstall_tab_in_handler();
gboolean IPCamInstall_tab_out_handler();

#endif
