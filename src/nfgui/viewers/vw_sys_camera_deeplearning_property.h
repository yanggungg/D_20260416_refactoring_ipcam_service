#ifndef _VW_SYS_SETUP_CAMERA_DEEPLEARNING_PROPERTY_H_
#define _VW_SYS_SETUP_CAMERA_DEEPLEARNING_PROPERTY_H_

#include "objects/nfobject.h"


////////////////////////////////////////////////////////////
//
// protected interfaces
//

gint _analysis_builtin_prop_init_page(NFOBJECT *parent);
gint _analysis_builtin_prop_show_page(gint ch, AiAnalysisActData *analysis_data);
gint _analysis_builtin_prop_hide_page(gint ch);

gint _analysis_builtin_prop_load_changed_data(gint ch);
gint _analysis_builtin_prop_is_changed_data();

gint _analysis_builtin_prop_save_data();
gint _analysis_builtin_prop_cancel_data(gint ch, gint expose);

#endif

