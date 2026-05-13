#ifndef _VW_TIMELINE_H_
#define _VW_TIMELINE_H_

#include "scm.h"

void VW_Timeline_Open(NFWINDOW *parent);
int VW_Timeline_Close();
void VW_Timeline_Show();
void VW_Timeline_Hide();
gboolean VW_Timeline_IsShown();
gboolean VW_Timeline_IsInArea(guint x, guint y);
int VW_Timeline_ChangeMode(TLINE_MODE_E mode);
int VW_Timeline_Set_Date(time_t ti, gboolean expose);
gint VW_Timeline_get_disp_mode();

#endif

