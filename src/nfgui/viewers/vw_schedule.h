#ifndef _SETUP_SYSTEM_SCHEDULE_H_
#define _SETUP_SYSTEM_SCHEDULE_H_

#include "objects/nfobject.h"
#include "objects/cw_calendar.h"
#include "objects/nfobject.h"
#include "objects/nffixed.h"
#include "objects/nftable.h"
#include "objects/nflabel.h"
#include "objects/nfbutton.h"
#include "objects/cw_hol_calendar.h"


void vw_Init_SysSchedule_page(NFOBJECT *parent);
void vw_arch_set_from_utime(guint utime);
void vw_arch_set_to_utime(guint utime);
gboolean vw_SysSchedule_in_handler();
gboolean vw_SysSchedule_out_handler();
gboolean vw_SysSchedule_show();

#endif



















