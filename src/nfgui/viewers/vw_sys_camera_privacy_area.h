#ifndef __SETUP_PRIVACY_MASK_AREA_H__
#define __SETUP_PRIVACY_MASK_AREA_H__

#include "objects/nfobject.h"

void init_PrivacyMask_Area_Page(NFOBJECT *parent);

void PrivacyMask_Area_Show_Preview();
void PrivacyMask_Area_Stop_Preview();

gint _set_data_PrivacyMask_Area(PrivacyData *data);

#endif


