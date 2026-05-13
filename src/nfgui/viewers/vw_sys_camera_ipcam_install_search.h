
#ifndef _SETUP_IPCAM_INSTALL_MODE_H_
#define _SETUP_IPCAM_INSTALL_MODE_H_

#include "objects/nfobject.h"


#define STR_EMPTY			""
#define STR_OK				"OK"
#define STR_INVALID_IP		"INVALID IP"
#define STR_NEED_CHANGE_PW	"NEED TO CHANGE PASSWORD"
#define STR_CONNECT_FAIL	"CONNECTION FAIL"
#define STR_UNSUPPORTED		"UNSUPPORTED"
#define STR_LOGIN_FAIL		"LOGIN FAIL"
#define STR_STREAM_FAIL		"STREAM FAIL"
#define STR_CONNECTING		"CONNECTING"


gint VW_Open_IPCamInstallSearch_Page(NFOBJECT *parent);

#endif

