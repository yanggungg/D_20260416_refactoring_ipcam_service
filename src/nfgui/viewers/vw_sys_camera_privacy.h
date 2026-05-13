#ifndef __SETUP_PRIVACY_MASK_H__
#define __SETUP_PRIVACY_MASK_H__

#include "objects/nfobject.h"

void init_PrivacyMask_page(NFOBJECT *parent);
gboolean PrivacyMask_tab_in_handler();
gboolean PrivacyMask_tab_out_handler();
gint PrivacyMask_get_cur_tab_page();

gint _is_changed_data_PrivacyMask();
gint _save_data_PrivacyMask();
gint _restore_data_PrivacyMask();

#endif


