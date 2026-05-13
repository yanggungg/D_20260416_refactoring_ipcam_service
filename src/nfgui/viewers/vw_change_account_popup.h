#ifndef _VW_CHANGE_ACCOUNT_POPUP_H_
#define _VW_CHANGE_ACCOUNT_POPUP_H_

#include "nf_ui_tool.h"

enum {
	OPT_USE_ADMIN_PASS			= 0,	
	OPT_ENAHNCED_PASSWORD		= 1,
	OPT_FORCE_CHANGE_PASSWORD	= 2,
	OPT_USE_VKEY_ALPHANUMERIC	= 3,
};

typedef enum
{
	NORMAL			= 0,
	ORG_PW_CHECK	= 1,
	MODE_MAX,
}mode_type;

gint VW_Open_account_change_Popup(NFWINDOW *parent, gchar *title, guint opt, gchar* id, gchar* org_password, gchar* password, guint max_ch, mode_type mode);

#endif
