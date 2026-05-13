#ifndef __WEBRA_API_PARAM_H__
#define __WEBRA_API_PARAM_H__

#include "webra_host_api_valid.h"
#include "webra_host_json.h"

////////////////////////////////////////////////////////////////////////////////

typedef enum _WEBRA_API_PARAM_TYPE_E {
  WEBRA_PARAM_START = -1,

  // TYPE
  WEBRA_PARAM_NONE = 0, // Don't use none param
  WEBRA_PARAM_BOOLEAN,
  WEBRA_PARAM_INT,
  WEBRA_PARAM_UINT,
  WEBRA_PARAM_CHAR,
  WEBRA_PARAM_LONG,
  WEBRA_PARAM_STRING,

  // add param type here
  WEBRA_PARAM_NR
} WEBRA_API_PARAM_TYPE;

typedef enum _WEBRA_API_PARAM_OPTIONAL_E {
  WEBRA_NOT_OPTIONAL = 0,
  WEBRA_IS_OPTIONAL
} WEBRA_API_PARAM_OPTIONAL;

typedef struct _WEBRA_API_PARAM_T {
  const gchar*              name;
  WEBRA_API_PARAM_TYPE      type;
  WEBRA_API_PARAM_OPTIONAL  optional;
  gint                      min;
  gint                      max;
  WEBRA_API_VALID_TYPE      validate; // validate_method

  // hidden value
  GValue                val;
  gboolean              valid;
} WEBRA_API_PARAM;

////////////////////////////////////////////////////////////////////////////////

gboolean  webra_host_set_param(WEBRA_API_PARAM* param_info, json_t* param);
gboolean  webra_host_free_param(WEBRA_API_PARAM* param_info);

gint      webra_host_param_length(WEBRA_API_PARAM*);
gboolean  webra_param_is_valid(gchar* key, ...);

gboolean  webra_param_boolean(gchar* key, ...);
gint      webra_param_int(gchar* key, ...);
guint     webra_param_uint(gchar* key, ...);
gchar     webra_param_char(gchar* key, ...);
glong     webra_param_long(gchar* key, ...);
gchar*    webra_param_string(gchar* key, ...);
GValue*   webra_param_gvalue(gchar* key, ...);

json_t*   webra_get_param_json();

////////////////////////////////////////////////////////////////////////////////
//APP_VALID_ERROR_CODE app_server_validate_param(APP_SERVER_PARAM*);
gboolean  webra_param_valid_boolean(WEBRA_API_PARAM param, gboolean value);
gboolean  webra_param_valid_int(WEBRA_API_PARAM param, gint value);
gboolean  webra_param_valid_uint(WEBRA_API_PARAM param, guint value);
gboolean  webra_param_valid_char(WEBRA_API_PARAM param, const gchar* value);
gboolean  webra_param_valid_long(WEBRA_API_PARAM param, gint64 value);
gboolean  webra_param_valid_string(WEBRA_API_PARAM param, const gchar* value);
gboolean  webra_param_valid_gvalue(WEBRA_API_PARAM param, GValue* value);
////////////////////////////////////////////////////////////////////////////////

#endif // __WEBRA_API_PARAM_H__
