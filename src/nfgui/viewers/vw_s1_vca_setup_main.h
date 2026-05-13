#ifndef _VW_S1_VCA_SETUP_MAIN_H_
#define _VW_S1_VCA_SETUP_MAIN_H_

#include "objects/nfobject.h"
#include "vw_vca.h"

void VW_Init_S1_VCA_Main_Page(NFOBJECT *parent);

gboolean VW_S1_VCA_Main_tab_in_handler();
gboolean VW_S1_VCA_Main_tab_out_handler();

gint VW_S1_VCA_attach_mevent(VCA_MEVENT_E mevt_type, VCA_MEVENT_CB_FUNC mevent_cb, gpointer user_data);
gint VW_S1_VCA_attach_select_ruleid(VCA_SELECT_CB_FUNC select_cb, gpointer user_data);

#endif

