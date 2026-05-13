
#ifndef _VW_SYS_CAMERA_VCA_SCHD_H_
#define _VW_SYS_CAMERA_VCA_SCHD_H_

#include "objects/nfobject.h"

void VW_VCACfg_Schd_init_page(NFOBJECT *parent);

gboolean vw_vca_check_schd_data_changed(void);
void vw_vca_save_schd_data(void);
void vw_vca_restore_schd_data(gboolean expose);

#endif	/* _VW_SYS_CAMERA_VCA_SCHD_H_ */

