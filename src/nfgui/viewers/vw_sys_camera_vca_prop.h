
#ifndef _VW_SYS_CAMERA_VCA_PROP_H_
#define _VW_SYS_CAMERA_VCA_PROP_H_

#include "objects/nfobject.h"

void VW_VCACfg_Prop_init_page(NFOBJECT *parent);

gboolean vw_vca_check_prop_data_changed(void);
void vw_vca_save_prop_data(void);
void vw_vca_restore_prop_data(gboolean expose);

#endif	/* _VW_SYS_CAMERA_VCA_PROP_H_ */

