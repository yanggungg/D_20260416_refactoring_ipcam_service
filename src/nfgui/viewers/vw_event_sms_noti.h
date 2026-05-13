#ifndef _VW_EVENT_SMS_NOTI_H_
#define _VW_EVENT_SMS_NOTI_H_

#include "objects/nfobject.h"

void VW_Init_EvtNoti_SMS_Page(NFOBJECT *parent);

gboolean check_evt_noti_sms_changed();
void save_evt_noti_sms_data();
void restore_evt_noti_sms_data();
#endif

