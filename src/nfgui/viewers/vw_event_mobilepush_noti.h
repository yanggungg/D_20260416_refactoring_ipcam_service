#ifndef _VW_EVENT_MOBILEPUSH_NOTI_H_
#define _VW_EVENT_MOBILEPUSH_NOTI_H_

#include "objects/nfobject.h"

void VW_Init_EvtNoti_MobilePush_Page(NFOBJECT *parent);

gboolean check_evt_noti_buzzer_changed();
void save_evt_noti_buzzer_data();
void restore_evt_noti_buzzer_data();
#endif

