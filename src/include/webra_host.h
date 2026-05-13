#ifndef __WEBRA_HOST_H__
#define __WEBRA_HOST_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>

#include "webra_feature.h"
#include "webra_def.h"
#include "webra_host_log.h"

#define WEBRA_MAX_USER 120
#define LOGIN_LIMIT_TIME 60000

// #define LANG_KOR_AUTH_CODE_SEND_MESSAGE "인증번호[%s]를 입력해 주세요."
static guint WEBBASE_LOGIN_FAIL_COUNT_TABLE[WEBRA_MAX_USER];

////////////////////////////////////////////////////////////////////////////////
typedef struct _WEBRA_HOST_THREAD_INFO_T {
  gboolean thread_init;
  gboolean thread_run;
  GThread* thread;
  GThread* session_checker_thread;
  GThread* health_checker_thread;
} WEBRA_HOST_THREAD_INFO;

typedef struct _WEBRA_SESSION_STORED_DATA_T {
  gchar sessionid[128];
  guint dlva_channel_data;
  glong last_kicked_time;
} WEBRA_SESSION_STORED_DATA;
////////////////////////////////////////////////////////////////////////////////

extern enum _SESSION_STORED_VALUE_E {
  CATE_STORED_DLVA_CH
} SESSION_STORED_VALUE;


typedef enum _WEBRA_AI_CAM_TYPE
{
	WEBRA_CAM_AI_TYPE_NONE = 0,
	WEBRA_CAM_AI_TYPE_HAS_AIBOX_MODULE = 1, // PRO,ULTRA
	WEBRA_CAM_AI_TYPE_RULE_EDITABLE_USE_CAM_ENGINE = 2, // Clair2
	WEBRA_CAM_AI_TYPE_RULE_EDITABLE_USE_NVR_ENGINE = 3 // Clair1
} WEBRA_AI_CAM_TYPE;

////////////////////////////////////////////////////////////////////////////////
// format : const gchar* , printf format.
// result : gchar*
#define WEBRA_HOST_VA_PRINT(last_arg, format, result) \
  va_list va_arg; \
  va_start(va_arg, last_arg); \
  g_vsnprintf(result, sizeof(result), format, va_arg); \
  va_end(va_arg);

////////////////////////////////////////////////////////////////////////////////

GThread*  webra_host_get_thread();
gboolean webra_host_add_session_store_list(char* sessionid);
gboolean webra_host_session_kick(char* param_sessionid);
void set_change_symbolic_link(int tof);
void _change_fileupload_base_dir(int option);
void set_change_upgrade_prepare_fwup_validate(int _val);
void set_boot_event_status(int _val);
int get_change_upgrade_prepare_fwup_validate();

gint webra_logo_init(void);
int get_change_symbolic_link();
int webra_host_main();
////////////////////////////////////////////////////////////////////////////////
#endif // __WEBRA_HOST_H__
