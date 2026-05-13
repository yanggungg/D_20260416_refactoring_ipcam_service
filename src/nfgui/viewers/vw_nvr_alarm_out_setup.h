#ifndef _VW_ALARM_OUT_SETUP_H_
#define _VW_ALARM_OUT_SETUP_H_

#include "objects/nfobject.h"

void VW_Nvr_Init_AlarmOut_SetupPage(NFOBJECT *parent);
gboolean check_nvr_alarm_out_data_changed();
void save_nvr_alarm_out_data();
void restore_nvr_alarm_out_data();

#endif

