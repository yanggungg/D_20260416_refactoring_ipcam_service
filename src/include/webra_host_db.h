#ifndef __WEBRA_HOST_DB_H__
#define __WEBRA_HOST_DB_H__

#include "nf_sysdb.h"
#include "nf_notify.h"
#include "webra_host.h"
#include "webra_host_return.h"
#include "webra_host_json.h"
#include "webra_host_value.h"

////////////////////////////////////////////////////////////////////////////////
#define MAX_DB_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 1024

#define WEBRA_DONT_SAVE_DB -1
////////////////////////////////////////////////////////////////////////////////

gboolean  webra_host_db_init();

WEBRA_HOST_RETURN_CODE  webra_host_db_get(json_t* query);
WEBRA_HOST_RETURN_CODE  webra_host_db_set(json_t* query);
WEBRA_HOST_RETURN_CODE  webra_host_db_sysdbfire(json_t* query);
WEBRA_HOST_RETURN_CODE  webra_host_db_logput(json_t* query);

gboolean  webra_host_db_save(NF_SYSDB_CATE_E category);
////////////////////////////////////////////////////////////////////////////////
gboolean  webra_db_get_boolean(gchar* key, ...);
guint     webra_db_get_uint(gchar* key, ...);
gint      webra_db_get_int(gchar* key, ...);
gchar*    webra_db_get_string(gchar* key, ...);
const gchar*    webra_db_get_key(WEBRA_HOST_VALUE_MAP*, gchar* key, ...);

gboolean  webra_db_set_boolean(gchar* key, gboolean value, ...);
gboolean  webra_db_set_uint(gchar* key, guint value, ...);
gboolean  webra_db_set_int(gchar* key, gint value, ...);
gboolean  webra_db_set_string(gchar* key, gchar* value, ...);
gboolean  webra_db_set_key(WEBRA_HOST_VALUE_MAP*, gchar* key, gchar* value, ...);
////////////////////////////////////////////////////////////////////////////////

//gboolean  webra_host_db_save(NF_SYSDB_CATE_E category_num);
gint      webra_host_db_get_id_index(gchar* id);
gchar*    webra_host_db_get_user_group(gchar* id);
////////////////////////////////////////////////////////////////////////////////
#endif // __WEBRA_HOST_DB_H__
