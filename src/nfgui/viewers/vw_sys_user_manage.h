#ifndef _SETUP_USER_MANAGE_SETUP_H_
#define _SETUP_USER_MANAGE_SETUP_H_

#include "objects/nfobject.h"


enum {
	USER_EDIT_RET_DELETE = 0,
	USER_EDIT_RET_OK,
	USER_EDIT_RET_CANCEL,
};

void init_UserManage_page(NFOBJECT *parent);
gboolean UserManage_tab_out_handler();
#endif

