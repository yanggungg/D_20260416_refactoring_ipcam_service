#ifndef __WEBRA_HOST_VALUE_H__
#define __WEBRA_HOST_VALUE_H__

#include "webra_host.h"

typedef struct _WEBRA_HOST_VALUE_MAP_T {
  const gchar*  key;
  gint          value;
} WEBRA_HOST_VALUE_MAP;

#define webra_get_index_by_key(_pair, _key) webra_host_value_get_index_by_key(_pair, _key)
#define webra_get_value_by_key(_pair, _key) webra_host_value_get_value_by_key(_pair, _key)
#define webra_get_key_by_value(_pair, _value) webra_host_value_get_key_by_value(_pair, _value)

////////////////////////////////////////////////////////////////////////////////
gint          webra_host_value_get_index_by_key(WEBRA_HOST_VALUE_MAP* pair, const gchar* key);

gint          webra_host_value_get_value_by_key(WEBRA_HOST_VALUE_MAP*, const gchar*);
const gchar*  webra_host_value_get_key_by_value(WEBRA_HOST_VALUE_MAP*, gint);
////////////////////////////////////////////////////////////////////////////////

#endif //__WEBRA_HOST_VALUE_H__