#ifndef _VW_TIMELINE_POPUP_H_
#define _VW_TIMELINE_POPUP_H_

#include <glib.h>

void VW_Timeline_PopUp_New(NFWINDOW *parent);
void VW_Timeline_PopUp_Show(guint x, guint y, GTimeVal start_t, GTimeVal end_t, TLINE_MODE_E mode);
void VW_Timeline_PopUp_Hide();
void get_time_data(GTimeVal *s_t, GTimeVal *e_t);
int VW_Timeline_PopUp_Destroy();

#endif

