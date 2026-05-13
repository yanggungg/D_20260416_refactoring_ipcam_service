#ifndef __WEBRA_HOST_JSON_H__
#define __WEBRA_HOST_JSON_H__

#include "webra_host.h"
#include <jansson.h>

////////////////////////////////////////////////////////////////////////////////
#define WEBRA_MAX_KEY_LENGTH    64
#define WEBRA_MAX_VALUE_LENGTH  1024
////////////////////////////////////////////////////////////////////////////////
json_t* webra_host_json_load(GString* query_string);

////////////////////////////////////////////////////////////////////////////////

void    webra_host_json_dump(json_t* json_node);
gchar*  webra_host_json_dump_string(json_t* json_node);

////////////////////////////////////////////////////////////////////////////////
json_t*      webra_host_json_get_object(json_t* json, const gchar* uri, ...);

gboolean      webra_host_json_get_boolean(json_t* json, const gchar* uri, ...);
glong         webra_host_json_get_integer(json_t* json, const gchar* uri, ...);
gulong        webra_host_json_get_unsigned_integer(json_t* json, const gchar* uri, ...);
gdouble       webra_host_json_get_real(json_t* json, const gchar* uri, ...);
const gchar*  webra_host_json_get_string(json_t* json, const gchar* uri, ...);

////////////////////////////////////////////////////////////////////////////////
gboolean webra_host_json_set_object(json_t* json,
                                     const gchar* uri,
                                     json_t* value, ...);

gboolean webra_host_json_set_boolean(json_t* json, const gchar* uri, gboolean value, ...);
gboolean webra_host_json_set_integer(json_t* json, const gchar* uri, glong value, ...);
gboolean webra_host_json_set_unsigned_integer(json_t* json, const gchar* uri, gulong value, ...);
gboolean webra_host_json_set_real(json_t* json, const gchar* uri, gdouble value, ...);
gboolean webra_host_json_set_string(json_t* json, const gchar* uri, const char* value, ...);
gboolean webra_host_json_set_array(json_t* json, const gchar* uri, ...);

////////////////////////////////////////////////////////////////////////////////
gboolean webra_host_json_array_append_string(json_t* json, const gchar* uri, const char* value, ...);
json_t* webra_host_json_get_array(json_t* json, const gchar* uri, ...);
////////////////////////////////////////////////////////////////////////////////
gboolean webra_host_json_del(json_t* json, const gchar* uri, ...);

////////////////////////////////////////////////////////////////////////////////
gboolean webra_host_json_is_boolean(json_t* json, const gchar* uri, ...);
gboolean webra_host_json_is_integer(json_t* json, const gchar* uri, ...);
gboolean webra_host_json_is_real(json_t* json, const gchar* uri, ...);
gboolean webra_host_json_is_string(json_t* json, const gchar* uri, ...);
gboolean webra_host_json_is_array(json_t* json, const gchar* uri, ...);
////////////////////////////////////////////////////////////////////////////////

#endif // __WEBRA_HOST_JSON_H__
