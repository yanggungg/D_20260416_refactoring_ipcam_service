#ifndef __WEBRA_API_H__
#define __WEBRA_API_H__

#include <glib.h>
#include <glib-object.h>

#include "webra_host_socket.h"

#include "webra_host_api_param.h"
#include "webra_feature.h"
#include "webra_host_api_auth.h"
#include "webra_host_db.h"
#include "webra_host_return.h"
#include "webra_host_json.h"
#include "webra_host_value.h"
#include "nf_sysdb.h"
#include "nf_auth.h"
#include "nf_common.h"
#include "nf_sysman.h"

////////////////////////////////////////////////////////////////////////////////
#define WEBRA_MAX_API_LENGTH 256

#define WEBRA_CLIENT_HOST   webra_host_socket_get_header_remote_host()
#define WEBRA_CLIENT_PORT   webra_host_socket_get_header_remote_port()
#define WEBRA_CLIENT_SESSID   webra_host_socket_get_header_sessid()
#define WEBRA_CLIENT_USERID   webra_host_socket_get_header_userid()
#define WEBRA_CLIENT_USERKEY  webra_hostr_socket_get_header_usrkey()

#define MAX_DATA_SIZE ((1024*64) * 2) * 4
////////////////////////////////////////////////////////////////////////////////

typedef struct _WEBRA_API_PROC_T {
  WEBRA_API_PARAM         *param;
  WEBRA_HOST_RETURN_CODE  (*func)(void);
  NF_SYSDB_CATE_E         db_category;
  gint                    permission_needed;
  gint                    allowed_group;
} WEBRA_API_PROC;

typedef struct _WEBRA_API_CMD_T {
  gchar           *name;
  WEBRA_API_PROC  *proc;
} WEBRA_API_CMD;

////////////////////////////////////////////////////////////////////////////////
gboolean                webra_api_init();
WEBRA_HOST_RETURN_CODE  webra_api_main(json_t* query_json);
////////////////////////////////////////////////////////////////////////////////

gboolean webra_host_api_get_authority(gchar* api_name, AUTH_IDX auth_idx);
gboolean webra_host_api_get_authority_mask(gchar* api_name, gint *mask);
gboolean webra_host_api_set_authority(gchar* api_name, AUTH_IDX auth_idx, gboolean flag);
gboolean webra_host_api_set_authority_mask(gchar* api_name, gint mask);

gint webra_host_api_scm_completed_check();
gint webra_host_api_setup_save_check();
////////////////////////////////////////////////////////////////////////////////

#endif //__WEBRA_API_H__
