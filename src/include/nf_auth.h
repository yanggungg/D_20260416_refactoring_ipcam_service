#ifndef __NF_AUTH_H__
#define __NF_AUTH_H__

#include <glib.h>
#include <glib-object.h>

#include "nf_object.h"
#include "nf_notify.h"
#if 0
#include "nf_util_ldap.h"
#endif

G_BEGIN_DECLS


#include "nf_common.h"

typedef enum {
  AUTH_SRC_SYSDB = 0,
  AUTH_SRC_LDAP,
  AUTH_SRC_NR
} AUTH_SRC;

typedef enum {
  AUTH_IDX_LIVE = 0,
  AUTH_IDX_SEARCH,
  AUTH_IDX_ARCH,
  AUTH_IDX_REC_SETUP,
  AUTH_IDX_EVENT,
  AUTH_IDX_AUDIO,
  AUTH_IDX_MIC,
  AUTH_IDX_REMOTE,
  AUTH_IDX_SHUTDOWN,
  AUTH_IDX_PTZ,
  AUTH_IDX_USER_MOD = 10,
  AUTH_IDX_FACTORY,
  AUTH_IDX_SETUP,
  AUTH_IDX_DB_READ,
  AUTH_IDX_DB_WRITE,
  AUTH_IDX_NR
} AUTH_IDX;

#define AUTH(TYPE) (1<<AUTH_IDX_##)

static gchar *AUTH_NAME[] = {
  "Auth Live",
  "Auth Search",
  "Auth Archive",
  "Auth Record Setup",
  "Auth Event",
  "Auth Audio",
  "Auth Mic",
  "Auth Remote",
  "Auth Shutdown",
  "Auth PTZ",
  "Auth UserMod",
  "Auth Factory",
  "Auth Setup",
  "_Auth End_"
};

#define USERNAME_MAXLEN 128
#define USERPASS_MAXLEN 128

#if 0
typedef struct _USER_GROUP_T {
  gchar grpname[USERNAME_MAXLEN];
  gint  group_auth; // Bitmask of AUTH

} USER_GROUP_T;

typedef struct _USER_T {
  gchar username[USERNAME_MAXLEN];
  gint  user_auth; // TODO: Bitmask of AUTH
  gchar userpasswd[USERPASS_MAXLEN];
} USER_T;
#endif

#define MAX_GROUP 200
#define MAX_USER 200

#define NF_AUTH_ERR_OK 0
#define NF_AUTH_ERR_NOUSER -1
#define NF_AUTH_ERR_NOAUTH -2
#define NF_AUTH_ERR_LDAP_CONNFAIL -3
#define NF_AUTH_ERR_LDAP_AUTHFAIL -4
#define NF_AUTH_ERR_LDAP_FAIL -5

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////OBJECT/////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/* type macro */
#define NF_TYPE_GROUP            (nf_group_get_type ())

#define NF_IS_GROUP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  NF_TYPE_GROUP))
#define NF_IS_GROUP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_GROUP))

#define NF_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  NF_TYPE_GROUP, NfGroup))
#define NF_GROUP_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_GROUP, NfGroupClass))
#define NF_GROUP_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_GROUP, NfGroupClass))

#define NF_GROUP_CAST(obj)        ((NfGroup*)(obj))
#define NF_GROUP_CLASS_CAST(klass)    ((NfGroupClass*)(klass))

#define NF_GROUP_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE(obj, NF_TYPE_GROUP, NfGroupPrivate))

typedef struct _NfGroup     NfGroup;
typedef struct _NfGroupClass   NfGroupClass;
typedef struct _NfGroupPrivate   NfGroupPrivate;

struct _NfGroup {
  NfObject        object;

  /* signals */

  /*< public >*/
  guint index;
  gchar *name;
  gint permission;
  gchar *desc;

  /*< private >*/
};

struct _NfGroupClass {
  NfObjectClass  parent_class;

  void (*print) (NfGroup *nfgroup);
};



/////////////////////////////////////////////

/* type macro */
#define NF_TYPE_USER            (nf_user_get_type ())

#define NF_IS_USER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  NF_TYPE_USER))
#define NF_IS_USER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_USER))

#define NF_USER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  NF_TYPE_USER, NfUser))
#define NF_USER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_USER, NfUserClass))
#define NF_USER_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_USER, NfUserClass))

#define NF_USER_CAST(obj)        ((NfUser*)(obj))
#define NF_USER_CLASS_CAST(klass)    ((NfUserClass*)(klass))

#define NF_USER_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE(obj, NF_TYPE_USER, NfUserPrivate))

typedef struct _NfUser     NfUser;
typedef struct _NfUserClass   NfUserClass;
typedef struct _NfUserPrivate   NfUserPrivate;

struct _NfUser {
  NfObject        object;

  /* signals */

  /*< public >*/

  /*< private >*/
  NfUserPrivate *priv;
};

struct _NfUserClass {
  NfObjectClass  parent_class;

  void (*print) (NfUser *nfuser);
};

struct _NfUserPrivate {
  gchar *name;
  gchar *passwd;
  gint permission;
  gchar *email;
  gboolean email_notify;
  gchar *desc;
  NfGroup *group;
  gchar *covert;
  guint pw_last_changed;
  guint expired_check;
};

  static void
nf_user_set_property (GObject *obj,
                      guint param_id,
                      const GValue *value,
                      GParamSpec *spec);

  static void
nf_user_get_property (GObject *obj,
                      guint param_id,
                      GValue *value,
                      GParamSpec *spec);

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////OBJECT/////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/* type macro */
#define NF_TYPE_AUTH            (nf_auth_get_type ())

#define NF_IS_AUTH(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  NF_TYPE_AUTH))
#define NF_IS_AUTH_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_AUTH))

#define NF_AUTH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  NF_TYPE_AUTH, NfAuth))
#define NF_AUTH_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_AUTH, NfAuthClass))
#define NF_AUTH_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_AUTH, NfAuthClass))

#define NF_AUTH_CAST(obj)        ((NfAuth*)(obj))
#define NF_AUTH_CLASS_CAST(klass)    ((NfAuthClass*)(klass))

#define NF_AUTH_GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE(obj, NF_TYPE_AUTH, NfAuthPrivate))

typedef struct _NfAuth     NfAuth;
typedef struct _NfAuthClass   NfAuthClass;
typedef struct _NfAuthPrivate   NfAuthPrivate;

struct _NfAuth {
  NfObject        object;

  /* signals */

  /*< public >*/
  guint       init_done;

  /*< private >*/
  NfAuthPrivate    *priv;
};

struct _NfAuthClass {
  NfObjectClass  parent_class;

  void (*print) (NfAuth *nfauth);
};

struct _NfAuthPrivate {
  AUTH_SRC    auth_src;
  guint        openness;
  guint       usrcnt;
  GList       *userlist;
  guint       grpcnt;
  GList       *grplist;
};


//////////////
// Function Prototype
gint
nf_auth_getUserPermission(
    NfAuth *nfauth,
    const gchar *userid);
    
gint nf_auth_getUserAuthorized (
    NfAuth *nfauth,
    const gchar *userid,
    const gchar *passwd,
    int permission);

  NfUser *
nf_auth_getUser (
    NfAuth *nfauth,
    const gchar *userid);

  NfGroup *
nf_auth_getUserGroup (
    NfAuth *nfauth,
    const gchar *userid,
    const gchar *passwd);

NfAuth *get_NfAuth();
NfGroup *nf_auth_getGroupByIndex(
    NfAuth *nfauth,
    const guint index);

gchar *nf_auth_getGroupNameByIndex(
    NfAuth *nfauth,
    const guint index);

gint nf_auth_main(void);

G_END_DECLS

#endif // __NF_AUTH_H__

