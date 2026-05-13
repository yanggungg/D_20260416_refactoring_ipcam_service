#ifndef _SETUP_DISP_SPOT_TAB1_
#define _SETUP_DISP_SPOT_TAB1_

#include "objects/nfobject.h"
#include "nf_afx.h"


void init_DispSpot_tab1_page(NFOBJECT *parent, guint spt_start, gint port_cnt, gint max_div, guint output_ch);

gboolean check_DispSpot_tab1_changed();
void save_DispSpot_tab1_data();
void restore_DispSpot_tab1_data();
#endif
