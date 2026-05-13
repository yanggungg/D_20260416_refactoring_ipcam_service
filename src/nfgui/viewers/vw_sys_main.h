#ifndef _SETUP_SYSTEM_MAIN_H_
#define _SETUP_SYSTEM_MAIN_H_


#include "objects/nfobject.h"

void VW_SetupSystem_set_changeflag(guint flag);
guint VW_SetupSystem_get_changeflag();


void VW_SetupSystem_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page); //wiggls - otm
void VW_SetupSystem_Destroy(NFOBJECT *object);


#endif

