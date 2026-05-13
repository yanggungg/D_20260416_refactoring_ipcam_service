#ifndef _SETUP_CAM_COLOR_SETUP_H_
#define _SETUP_CAM_COLOR_SETUP_H_

#include "objects/nfobject.h"

void init_ColorSetup_page(NFOBJECT *parent);

gint init_ColorSetup_start_preview(NFOBJECT *parent);
gboolean ColorSetup_tab_in_handler();
gboolean ColorSetup_tab_out_handler();

void nf_ui_SetDataToObjects_colorsetup();

#endif

