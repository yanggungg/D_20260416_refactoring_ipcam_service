#ifndef _VW_ALARM_OUT_SCHED_H_
#define _VW_ALARM_OUT_SCHED_H_

#include "objects/nfobject.h"

void VW_Nvr_Init_AlarmOut_SchedPage(NFOBJECT *parent);

gboolean check_nvr_alarm_sched_data_changed();
void save_nvr_alarm_sched_data();
void restroe_nvr_alarm_sched_data();
#endif

