#ifndef _VW_SYS_REC_EVT_H_
#define _VW_SYS_REC_EVT_H_

#include "objects/nfobject.h"

void VW_Init_SysRec_Evt_Page(NFOBJECT *parent);

gboolean check_system_record_data_changed();
void save_system_record_data();
void restore_system_record_data();
#endif
