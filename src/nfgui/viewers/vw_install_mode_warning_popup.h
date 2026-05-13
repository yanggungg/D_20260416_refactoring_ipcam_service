#ifndef _VW_INSTALL_MODE_WARNING_POPUP_H_
#define _VW_INSTALL_MODE_WARNING_POPUP_H_

#include "nf_ui_tool.h"

enum{
	POPUP_MODE_CLOSE = 0,
	POPUP_MODE_OPEN = 1
};

mb_type VW_Open_InstallMode_Warning_Popup(NFWINDOW *parent, gint mode);

#endif
