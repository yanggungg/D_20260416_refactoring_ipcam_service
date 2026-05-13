
#ifndef _VW_LIVE_PTZ_MAIN_H_
#define _VW_LIVE_PTZ_MAIN_H_

#include "support/nf_ui_image.h"
#include "support/nf_ui_color.h"

gint VW_Live_Ptz_Main_Open(NFWINDOW *parent, guint channel);
void VW_Live_Ptz_Main_Destroy();

gint VW_Live_Ptz_Main_Get_Channel(void);
#endif
