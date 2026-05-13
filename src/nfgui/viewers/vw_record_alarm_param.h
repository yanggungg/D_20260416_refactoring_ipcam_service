#ifndef _VW_RECORD_ALARM_PARAM_H_
#define _VW_RECORD_ALARM_PARAM_H_

#include "objects/nfobject.h"

void VW_Init_RecAlarmParam_Page(NFOBJECT *parent);
void VW_RecAlarmParam_data_reloaded(gboolean reload);
void VW_RecAlarmParam_info_refresh();

#endif


