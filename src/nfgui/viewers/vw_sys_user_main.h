#ifndef _SETUP_USER_MAIN_H_
#define _SETUP_USER_MAIN_H_


#include "objects/nfobject.h"

#define SYS_USER_STRING_ADMIN		"ADMIN"
#define SYS_USER_STRING_MANAGER		"MANAGER"
#define SYS_USER_STRING_USER		"USER"

typedef enum {
	USER_AUTH_SETUP = 0,
	USER_AUTH_ALARM_OFF,
	USER_AUTH_PTZ,
	USER_AUTH_SEARCH,
	USER_AUTH_ARCHIVE,
	USER_AUTH_PANIC,
	USER_AUTH_REMOTE,

	USER_AUTH_FOR_POWEROFF,
	USER_AUTH_ONLY_ADMIN,
	USER_AUTH_ONLY_ADMIN_ALWAYS,	// no db check.

	USER_AUTH_ALL,
	USER_AUTH_ALL_ALWAYS,	// no db check

} ua_type;

#ifdef __CBC_UI__
typedef enum{
	USER_AUTH_ADMIN = 0,
	USER_AUTH_MANAGER,
	USER_AUTH_USER
} USER_AUTH_GROUP;
#endif

void sysuser_set_changeflag(guint flag);
guint sysuser_get_changeflag();


void SystemSetupUser_Open(NFWINDOW *parent, NFOBJECT *prev_wnd, gint page );
void SystemSetupUser_Destroy(NFOBJECT *object);

#ifdef __CBC_UI__
/*inline*/ USER_AUTH_GROUP get_current_user_auth();
#endif

#endif

