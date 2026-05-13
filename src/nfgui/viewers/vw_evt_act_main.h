#ifndef _VW_EVT_ACT_MAIN_H_
#define _VW_EVT_ACT_MAIN_H_

#include "objects/nfobject.h"
#include "vw.h"

void VW_Evt_Act_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page);
void VW_Evt_Act_Destroy(NFOBJECT *object);

void event_act_data_changed(gboolean change);
gboolean event_act_data_is_changed();
void change_obj_focus(NFOBJECT* from, NFOBJECT *to);
#endif

