#ifndef __WEBRA_HOST_RETURN_H__
#define __WEBRA_HOST_RETURN_H__

#include "webra_host.h"
#include "webra_host_json.h"
#include "nf_sysman.h"

////////////////////////////////////////////////////////////////////////////////
typedef enum _WEBRA_HOST_RETURN_CODE_E {
  WEBRA_HOST_RETURN_SUCCESS = 200,

  WEBRA_HOST_RETURN_NO_AUTH = 401,

  WEBRA_HOST_NOT_INITIALIZED_PASSWORD = 403,
  
  WEBRA_HOST_RETURN_INTERNEL_ERROR     = 500,
  WEBRA_HOST_RETURN_NO_API             = 501,
  WEBRA_HOST_RETURN_INVALID_PARAM      = 505,

  WEBRA_HOST_RETURN_DVR_IN_SETUP       = 506,
  WEBRA_HOST_RETURN_DVR_IN_ARCH        = 507,
  WEBRA_HOST_RETURN_REC_INVALID        = 508,
  WEBRA_HOST_RETURN_NOT_LIVE           = 509,
  WEBRA_HOST_RETURN_INCORRECT_PASSWORD = 510,
  WEBRA_HOST_NOT_INITIALIZED_PASSWORD_OLD  = 511,
  WEBRA_HOST_NOT_FOUND_USER_SESSION    = 512,

  WEBRA_HOST_RETURN_TARGET_AIBOX_IS_OFFLINE = 513,

  WEBRA_HOST_NOT_ADMIN                 = 514,
  WEBRA_HOST_NOT_INITIALIZED_QUESTION  = 515,
  WEBRA_HOST_NOT_INITIALIZED_ANSWER    = 516,
  WEBRA_HOST_NOT_EXIST_ID              = 517,

  WEBRA_HOST_RETURN_NR
} WEBRA_HOST_RETURN_CODE;


////////////////////////////////////////////////////////////////////////////////
void      webra_host_return_init(void);
void      webra_host_return_set(const gchar* key);
void      webra_host_return_free(void);
json_t*   webra_host_return_get_list(void);

void      webra_return_boolean(const gchar* key, gboolean value, ...);
void      webra_return_int(const gchar* key, gint value, ...);
void      webra_return_uint(const gchar* key, guint value, ...);
void      webra_return_char(const gchar* key, gchar value, ...);
void      webra_return_long(const gchar* key, glong value, ...);
void      webra_return_string(const gchar* key, const gchar* value, ...);

//void      webra_return_debug_msg();
////////////////////////////////////////////////////////////////////////////////
void      webra_host_return_log(const gchar* format, ...);
void      webra_host_return_http_status(WEBRA_HOST_RETURN_CODE http_status);
////////////////////////////////////////////////////////////////////////////////
#endif // __WEBRA_HOST_RETURN_H__
