#ifndef _SETUP_CAMERA_MAIN_H_
#define _SETUP_CAMERA_MAIN_H_

#include "objects/nfobject.h"
#include "objects/nfwindow.h"

void syscam_set_changeflag(guint flag);
guint syscam_get_changeflag();

void sysusr_set_syncflag(guint flag);
guint sysusr_get_syncflag();

void sysact_set_changeflag(guint flag);
guint sysact_get_changeflag();

void SystemSetupCam_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page) ;
void SystemSetupCam_Destroy(NFOBJECT *object);

#endif

