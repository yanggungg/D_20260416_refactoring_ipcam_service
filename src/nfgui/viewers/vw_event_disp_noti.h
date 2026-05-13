#ifndef _VW_EVENT_DISP_NOTI_H_
#define _VW_EVENT_DISP_NOTI_H_

#include "objects/nfobject.h"

void VW_Init_EvtNoti_Display_Page(NFOBJECT *parent);

gboolean check_evt_noti_disp_changed();
void save_evt_noti_disp_data();
void restore_evt_noti_disp_data();
#endif

