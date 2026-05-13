#ifndef _SETUP_DISPLAY_MAIN_H_
#define _SETUP_DISPLAY_MAIN_H_

#include "objects/nfobject.h"
#include "objects/nfwindow.h"


void sysdisp_set_changeflag(guint flag);
guint sysdisp_get_changeflag();

void SystemSetupDisp_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page) ;
void SystemSetupDisp_Destroy(NFOBJECT *object);


#endif

