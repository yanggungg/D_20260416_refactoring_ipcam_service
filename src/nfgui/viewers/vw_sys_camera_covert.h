#ifndef _VW_SYS_SETUP_CAMERA_COVERT_SETUP_H_
#define _VW_SYS_SETUP_CAMERA_COVERT_SETUP_H_

#include "objects/nfobject.h"

// RECORD MENU - CANCEL, APPLY, CLOSE BUTTON.

void init_CamCovert_page(NFOBJECT *parent);

gboolean CamCovert_tab_in_handler();
gboolean CamCovert_tab_out_handler();

#endif

