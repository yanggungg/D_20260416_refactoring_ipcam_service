#ifndef __WEBRA_HOST_AUTH_H__
#define __WEBRA_HOST_AUTH_H__

#include "webra_host.h"
#include "webra_host_db.h"
#include "nf_auth.h"
#include "nf_session.h"

// bitmask
typedef enum _WEBRA_GROUP_E {
  WEBRA_GROUP_NONE    = 1 << 0,
  WEBRA_GROUP_USER    = 1 << 1,
  WEBRA_GROUP_MANAGER = 1 << 2,
  WEBRA_GROUP_ADMIN   = 1 << 3
} WEBRA_GROUP;

// must be synced with 'NFDVR/src/include/nf_auth.h'
// bitmask

typedef enum _WEBRA_AUTHORITY_E {
  WEBRA_AUTH_MASK_LIVE = 0,
  WEBRA_AUTH_MASK_SEARCH,
  WEBRA_AUTH_MASK_ARCHIVING,
  WEBRA_AUTH_MASK_RECORD_SETUP,
  WEBRA_AUTH_MASK_EVENT_CONTROL,
  WEBRA_AUTH_MASK_AUDIO,
  WEBRA_AUTH_MASK_MIC,
  WEBRA_AUTH_MASK_REMOTE_LOG_IN,
  WEBRA_AUTH_MASK_SHUTDOWN,
  WEBRA_AUTH_MASK_PTZ,
  WEBRA_AUTH_MASK_USER_MOD,
  WEBRA_AUTH_MASK_FACTORY,
  WEBRA_AUTH_MASK_SYSTEM_SETUP,
  WEBRA_AUTH_MASK_DB_READ,
  WEBRA_AUTH_MASK_DB_WRITE,
  WEBRA_AUTH_MASK_OPEN,
} WEBRA_AUTHORITY_E;

#define AUTH_BITS(foo) ((unsigned int)(1 << foo))
 
typedef enum _WEBRA_AUTH {
  WEBRA_AUTH_NOUSE          = 0,
  WEBRA_AUTH_LIVE           = 1 << 0,
  WEBRA_AUTH_SEARCH         = 1 << 1,
  WEBRA_AUTH_ARCHIVING      = 1 << 2,
  WEBRA_AUTH_RECORD_SETUP   = 1 << 3,
  WEBRA_AUTH_EVENT_CONTROL  = 1 << 4,
  WEBRA_AUTH_AUDIO          = 1 << 5,
  WEBRA_AUTH_MIC            = 1 << 6,
  WEBRA_AUTH_REMOTE_LOG_IN  = 1 << 7,
  WEBRA_AUTH_SHUTDOWN       = 1 << 8,
  WEBRA_AUTH_PTZ            = 1 << 9,
  WEBRA_AUTH_USER_MOD       = 1 << 10,
  WEBRA_AUTH_FACTORY        = 1 << 11,
  WEBRA_AUTH_SYSTEM_SETUP   = 1 << 12,
  WEBRA_AUTH_DB_READ        = 1 << 13,
  WEBRA_AUTH_DB_WRITE       = 1 << 14,
  WEBRA_AUTH_NONE           = 0xffffffff,
} WEBRA_AUTH;

// defined in src/webra_host_api_auth.c
extern gchar *authority_string[];

gboolean webra_host_api_isallowed(json_t *permissions);

#endif // __WEBRA_HOST_AUTH_H__

