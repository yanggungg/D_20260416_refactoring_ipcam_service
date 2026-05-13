#ifndef _VW_EVENT_EMAIL_NOTI_H_
#define _VW_EVENT_EMAIL_NOTI_H_

#include "objects/nfobject.h"

void VW_Init_EvtNoti_Email_Page(NFOBJECT *parent);

gboolean check_evt_noti_email_changed();
void save_evt_noti_email_data();
void restore_evt_noti_email_data();
#endif

