
#ifndef __VW_CALENDAR_H
#define __VW_CALENDAR_H

#include "scm.h"

void VW_Calendar_Open(NFWINDOW *parent);

void VW_Calendar_Show(time_t ti);
void VW_Calendar_Hide();
gboolean VW_Calendar_IsShown();
int VW_Calendar_Close();


#endif

