#ifndef _SETUP_NETWORK_MAIN_H_
#define _SETUP_NETWORK_MAIN_H_

#include "objects/nfobject.h"

void sysnet_set_changeflag(guint flag);
guint sysnet_get_changeflag();


void SystemSetupNetwork_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page ); //wiggls - otm
void SystemSetupNetwork_Destroy(NFOBJECT *object);

#endif

