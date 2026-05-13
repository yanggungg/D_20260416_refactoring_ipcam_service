#ifndef _SETUP_USER_ADD_H_
#define _SETUP_USER_ADD_H_

#include "objects/nfobject.h"

#define	USER_MODE_ADD	0
#define	USER_MODE_EDIT	1

guint UserAddDlg_Open(NFWINDOW *parent, guint mode, gboolean edit_enable, UserManageData *user_data, gint idx, guint user_count);

#endif

