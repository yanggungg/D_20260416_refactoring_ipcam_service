#ifndef _SETUP_SOUND_MAIN_H_
#define _SETUP_SOUND_MAIN_H_

#include "objects/nfobject.h"

void syssnd_set_changeflag(guint flag);
guint syssnd_get_changeflag();

void SystemSetupSound_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page );
void SystemSetupSound_Destroy(NFOBJECT *object);


#endif


