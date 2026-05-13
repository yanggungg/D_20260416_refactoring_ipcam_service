#ifndef _VW_CAMPWD_H
#define _VW_CAMPWD_H

#include "objects/nfwindow.h"

#define CAM_PASSWD_SIZE_WID		(423)
#define CAM_PASSWD_SIZE_HEI		(236)

gboolean VW_CamPwd_Open(NFWINDOW *parent, guint x, guint y, guint cam_num);
void VW_CamPwd_Close(guint cam_num);

#endif
