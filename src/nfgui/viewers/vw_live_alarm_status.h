
#ifndef _VW_LIVE_ALARM_STATUS_H_
#define _VW_LIVE_ALARM_STATUS_H_


gboolean VW_Create_AlarmStatus(NFWINDOW *parent, gint x, gint y);
void VW_AlarmStatus_Show();
void VW_AlarmStatus_Hide();
gboolean VW_AlarmStatus_IsShown();

void VW_AlarmStatus_Set_Noti(guint evt_type, NF_NOTIFY_INFO *info);
#endif
