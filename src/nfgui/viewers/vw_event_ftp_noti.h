#ifndef _VW_EVENT_FTP_NOTI_H_
#define _VW_EVENT_FTP_NOTI_H_

#include "objects/nfobject.h"

void VW_Init_EvtNoti_FTP_Page(NFOBJECT *parent);

gboolean check_evt_noti_ftp_changed();
void save_evt_noti_ftp_data();
void restore_evt_noti_ftp_data();
#endif

