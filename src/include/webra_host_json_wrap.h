#ifndef __WEBRA_HOST_JSON_WRAP_H__
#define __WEBRA_HOST_JSON_WRAP_H__

#include "webra_host.h"
#include "webra_host_json.h"

////////////////////////////////////////////////////////////////////////////////
#ifdef __JANSSON_WRAP__

GList *global_json_list;

#define define_wrapper(X,Y,Z) \
  json_t *__real_json_##X(Y Z); \
  json_t *__wrap_json_##X(Y Z) \
  { \
    json_t *o = __real_json_##X(Z); \
    global_json_list_append(o, __func__, __FILE__); \
    return o; \
  }

////////////////////////////////////////////////////////////////////////////////
void global_json_list_dump();
int  global_json_list_append(json_t *json, char *caller, char *callerf);
int  getRefcountSum(json_t *json);

json_t *__real_json_loads(const char *string, size_t flags, json_error_t *error);
json_t *__wrap_json_loads(const char *string, size_t flags, json_error_t *error);

void __real_json_delete(json_t *json);
void __wrap_json_delete(json_t *json);

#endif // __JANSSON_WRAP__

////////////////////////////////////////////////////////////////////////////////

#endif // __WEBRA_HOST_JSON_WRAP_H__
