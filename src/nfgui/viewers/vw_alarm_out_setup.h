#ifndef _VW_ALARM_OUT_SETUP_H_
#define _VW_ALARM_OUT_SETUP_H_

#include "objects/nfobject.h"

void VW_Init_AlarmOut_SetupPage(NFOBJECT *parent);
gboolean check_alarm_out_data_changed();
void save_alarm_out_data();
void restore_alarm_out_data();

#endif

