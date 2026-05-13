#ifndef __WEBRA_HOST_SCM_H__
#define __WEBRA_HOST_SCM_H__

#include "webra_host.h"
#include "iux_afx.h"
#include "cmm.h"
#include "ix_mem.h"

////////////////////////////////////////////////////////////////////////////////
typedef struct _WEBRA_HOST_SCM_MSG_T {
  gint            index;
  CMM_MESSAGE_T*  msg;
  gint            ttl;
} WEBRA_HOST_SCM_MSG_T;

#if WEBRA_FEATURE_SCM == TRUE
////////////////////////////////////////////////////////////////////////////////

#define WEBRA_HOST_SCM_MAX_INDEX        1000
#define WEBRA_HOST_SCM_MAX_RETURN_SIZE  30
#define WEBRA_HOST_SCM_MSG_TTL          5

////////////////////////////////////////////////////////////////////////////////
gboolean webra_host_scm_receiver_main();

gboolean webra_host_scm_reciver_is_nfnotify(IMSG msgid);

gint    webra_host_scm_reciver_get_first_index();
GList*  webra_host_scm_reciver_get_list(gint index);
GList*  webra_host_scm_reciver_get_list_next(GList* list);

void      webra_host_scm_lock();
void      webra_host_scm_unlock();
gboolean  webra_host_scm_is_init();

#endif // WEBRA_FEATURE_SCM == 1

////////////////////////////////////////////////////////////////////////////////
CMMPORT web_uxm_get_cmmport();
CALLID  web_uxm_get_callid();
////////////////////////////////////////////////////////////////////////////////

#endif // __WEBRA_HOST_SCM_H__
