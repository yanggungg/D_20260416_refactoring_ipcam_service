#include "nf_auth.h"

//////////////////////////////
// glib utilities
//ksi_test sysman/nf_auth.c:6:1: error: static declaration of ‘g_list_free_full’ follows non-static declaration
  static void
_g_list_free_full (GList *list, GDestroyNotify free_func)
{
  g_list_foreach (list, (GFunc) free_func, NULL);
  g_list_free (list);
}

////////////////////////////////
static void _auth_sysdbchange_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static void _auth_sysdb_refresh();

////////////////////////////////
static NfAuth *_nf_auth = NULL;
static GMutex *_nf_auth_mutex = NULL;

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//// NF GROUP Definition
G_DEFINE_TYPE(NfGroup, nf_group, NF_TYPE_OBJECT);

static GObjectClass         *parent_group_class = NULL;

/* dispose is called when the object has to release all links
 * to other objects */
  static void
nf_group_dispose (GObject * object) {
  // thread end
  G_OBJECT_CLASS (parent_group_class)->dispose (object);
}

/* finalize is called when the object has to free its resources */
  static void
nf_group_finalize (GObject * obj) {
  NfGroup *self;

  self = G_TYPE_CHECK_INSTANCE_CAST (obj, NF_TYPE_GROUP, NfGroup);

  G_OBJECT_CLASS (parent_group_class)->finalize (obj);

}

  static void
nf_group_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
  NfObject *nfobject;

  nfobject = NF_OBJECT (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

  static void
nf_group_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
  NfObject *self;

  self = NF_OBJECT (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

  static void
nf_group_print (NfGroup *nfgroup) {
  g_return_if_fail ( nfgroup != NULL );
  g_return_if_fail ( NF_IS_GROUP(nfgroup) );

  g_print("group[%p] idx[%u] name[%s] permission[%x] desc[%s]\n",
            nfgroup,
            nfgroup->index,
            nfgroup->name,
            nfgroup->permission,
            nfgroup->desc);
}

  static void
nf_group_init (NfGroup *self)
{
  //Initialize members
}

  static void
nf_group_class_init (NfGroupClass * klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_group_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = nf_group_set_property;
  gobject_class->get_property = nf_group_get_property;

  gobject_class->dispose = nf_group_dispose;
  gobject_class->finalize = nf_group_finalize;

  klass->print = nf_group_print;
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//// NF USER Definition
G_DEFINE_TYPE(NfUser, nf_user, NF_TYPE_OBJECT);

static GObjectClass         *parent_user_class = NULL;

enum {
  PROP_USER_0,
  PROP_USER_NAME,
  PROP_USER_PASSWD,
  PROP_USER_PERMISSION,
  PROP_USER_EMAIL,
  PROP_USER_EMAIL_NOTI,
  PROP_USER_DESC,
  PROP_USER_GROUP,
  PROP_USER_GRPNAME,
  PROP_USER_COVERT,
  PROP_USER_PW_LAST_CHANGED,
  PROP_USER_EXPIRED_CHECK,
  N_USER_PROPERTIES
};

static GParamSpec *obj_user_properties[N_USER_PROPERTIES] = { NULL, };

/* dispose is called when the object has to release all links
 * to other objects */
  static void
nf_user_dispose (GObject * object) {
  // thread end
  NfUser *self = NF_USER(object);

  G_OBJECT_CLASS (parent_user_class)->dispose(object);
}

/* finalize is called when the object has to free its resources */
  static void
nf_user_finalize (GObject * obj) {
  NfUser *self;
  self = G_TYPE_CHECK_INSTANCE_CAST (obj, NF_TYPE_USER, NfUser);

  g_free (self->priv->name);
  g_free (self->priv->passwd);
  g_free (self->priv->email);
  g_free (self->priv->desc);
  g_free (self->priv->covert);

  G_OBJECT_CLASS (parent_user_class)->finalize (obj);
  //parent_user_class->finalize (self);
}

  static void
nf_user_print (NfUser *nfuser) {
  g_return_if_fail ( nfuser != NULL );
  g_return_if_fail ( NF_IS_USER(nfuser) );

  g_print("user[%p] name[%s] permission[%x] grpname[%d][%s]\n",
            nfuser,
            nfuser->priv->name,
            nfuser->priv->permission,
            nfuser->priv->group->index,
            nfuser->priv->group->name);
}

  static void
nf_user_init (NfUser *self)
{
  //Initialize members
  self->priv = NF_USER_GET_PRIVATE (self);
}

  static void
nf_user_class_init (NfUserClass * klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_user_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = nf_user_set_property;
  gobject_class->get_property = nf_user_get_property;

  gobject_class->dispose = nf_user_dispose;
  gobject_class->finalize = nf_user_finalize;

  klass->print = nf_user_print;

  g_type_class_add_private (klass, sizeof(NfUserPrivate));

  obj_user_properties[PROP_USER_NAME] =
    g_param_spec_string ("name", "name", "name",
                        NULL,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_PASSWD] =
    g_param_spec_string ("passwd", "passwd", "passwd",
                        NULL,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_PERMISSION] =
    g_param_spec_int ("permission", "permission", "permission",
                        -1, G_MAXINT, 0,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_EMAIL] =
    g_param_spec_string ("email", "email", "email",
                        NULL,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_EMAIL_NOTI] =
    g_param_spec_string ("email_noti", "email_noti", "email notification",
                        FALSE,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_DESC] =
    g_param_spec_string ("desc", "desc", "desc",
                        NULL,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_GROUP] =
    g_param_spec_object ("group", "group", "group", NF_TYPE_GROUP,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_GRPNAME] =
    g_param_spec_string ("grpname", "grpname", "grpname",
                        NULL,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_COVERT] =
    g_param_spec_string ("covert", "covert", "covert",
                        NULL,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_PW_LAST_CHANGED] =
    g_param_spec_uint ("pw_last_changed", "pw_last_changed", "timestamp pw last changed",
                        0, G_MAXUINT, 0,
                        G_PARAM_READWRITE);

  obj_user_properties[PROP_USER_EXPIRED_CHECK] =
    g_param_spec_uint ("expired_check", "expired_check", "expired check",
                        0, G_MAXUINT, 0,
                        G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                    N_USER_PROPERTIES,
                                    obj_user_properties);

}

  static void
nf_user_set_property (GObject *obj,
                      guint property_id,
                      const GValue *value,
                      GParamSpec *pspec) {
  NfUser *nfuser = NF_USER(obj);
  NfUserPrivate *priv = NF_USER_GET_PRIVATE(nfuser);

  switch (property_id) {
    case PROP_USER_NAME:
      priv->name = g_value_dup_string (value);
      break;
    case PROP_USER_PASSWD:
      priv->passwd = g_value_dup_string (value);
      break;
    case PROP_USER_PERMISSION:
      priv->permission = g_value_get_int (value);
      break;
    case PROP_USER_EMAIL:
      priv->email = g_value_dup_string (value);
      break;
    case PROP_USER_EMAIL_NOTI:
      priv->email_notify = g_value_get_boolean (value);
      break;
    case PROP_USER_DESC:
      priv->desc = g_value_dup_string (value);
      break;
    case PROP_USER_GROUP:
      priv->group = g_value_get_object(value);
      break;
    case PROP_USER_COVERT:
      priv->covert = g_value_dup_string (value);
      break;
    case PROP_USER_PW_LAST_CHANGED:
      priv->pw_last_changed = g_value_get_uint (value);
      break;
    case PROP_USER_EXPIRED_CHECK:
      priv->expired_check = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
  }
}

  static void
nf_user_get_property (GObject *obj,
                      guint property_id,
                      GValue *value,
                      GParamSpec *pspec) {
  NfUser *nfuser = NF_USER(obj);
  NfUserPrivate *priv = NF_USER_GET_PRIVATE(nfuser);

  switch (property_id) {
    case PROP_USER_NAME:
      g_value_set_string(value, priv->name);
      break;
    case PROP_USER_PASSWD:
      g_value_set_string(value, priv->passwd);
      break;
    case PROP_USER_PERMISSION:
      g_value_set_int(value, priv->permission);
      break;
    case PROP_USER_EMAIL:
      g_value_set_string(value, priv->email);
      break;
    case PROP_USER_EMAIL_NOTI:
      g_value_set_boolean(value, priv->email_notify);
      break;
    case PROP_USER_DESC:
      g_value_set_string(value, priv->desc);
      break;
    case PROP_USER_GROUP:
      g_value_set_object(value, priv->group);
      break;
    case PROP_USER_COVERT:
      g_value_set_string(value, priv->covert);
      break;
    case PROP_USER_PW_LAST_CHANGED:
      g_value_set_uint(value, priv->pw_last_changed);
      break;
    case PROP_USER_EXPIRED_CHECK:
      g_value_set_uint(value, priv->expired_check);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
  }
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//// NF AUTH Definition

G_DEFINE_TYPE(NfAuth, nf_auth, NF_TYPE_OBJECT);

static GObjectClass         *parent_auth_class = NULL;

enum {
  PROP_AUTH_0,
  PROP_AUTH_SRC,
  PROP_AUTH_OPENNESS,
  PROP_AUTH_USRCNT,
  PROP_AUTH_GRPCNT,
  N_AUTH_PROPERTIES
};

static GParamSpec *obj_auth_properties[N_USER_PROPERTIES] = { NULL, };

/* dispose is called when the object has to release all links
 * to other objects */
  static void
nf_auth_dispose (GObject * object) {
  // thread end
  parent_auth_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
  static void
nf_auth_finalize (GObject * obj) {
  NfAuth *self;
  self = G_TYPE_CHECK_INSTANCE_CAST (obj, NF_TYPE_AUTH, NfAuth);

  // free member
  self->priv->userlist = (_g_list_free_full(self->priv->userlist, g_object_unref), NULL);
  self->priv->grplist = (_g_list_free_full(self->priv->grplist, g_object_unref), NULL);

  G_OBJECT_CLASS (parent_auth_class)->finalize (obj);
}

  static void
nf_auth_set_property (GObject * obj,
                      guint property_id,
                      const GValue *value,
                      GParamSpec *pspec) {
  NfAuth *nfauth = NF_AUTH(obj);
  NfAuthPrivate *priv = NF_AUTH_GET_PRIVATE(nfauth);

  switch (property_id) {
    case PROP_AUTH_SRC:
      priv->auth_src = g_value_get_enum( value );
      break;
    case PROP_AUTH_OPENNESS:
      priv->openness = g_value_get_uint( value );
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
  }
}

  static void
nf_auth_get_property (GObject * obj,
                      guint property_id,
                      GValue *value,
                      GParamSpec *pspec) {
  NfAuth *nfauth = NF_AUTH(obj);
  NfAuthPrivate *priv = NF_AUTH_GET_PRIVATE(nfauth);

  switch (property_id) {
    case PROP_AUTH_SRC:
      g_value_set_enum(value, priv->auth_src);
      break;
    case PROP_AUTH_OPENNESS:
      g_value_set_uint(value, priv->openness);
      break;
    case PROP_AUTH_USRCNT:
      g_value_set_uint(value, priv->usrcnt);
      break;
    case PROP_AUTH_GRPCNT:
      g_value_set_uint(value, priv->grpcnt);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
      break;
  }
}

  static void
nf_auth_init (NfAuth *self)
{
  self->priv = NF_AUTH_GET_PRIVATE (self);
	self->init_done = 1;

  GValueArray *array = g_value_array_new (10);

  //Initialize members
  //self->priv->
  self->priv->auth_src = AUTH_SRC_SYSDB;
  self->priv->usrcnt = 0;
  self->priv->userlist = NULL;

  gulong cb_handle = 0;
  cb_handle= nf_notify_connect_cb( "sysdb_change", _auth_sysdbchange_cb_func,  NULL);
  g_message("%s connect_cb[%ld]",__FUNCTION__, cb_handle );

  g_print("%s is called\n", __func__);
}

  static void
_nf_auth_dump_user(
    gpointer data,
    gpointer user_data) {
  g_return_if_fail ( data != NULL );

  NfUser *nfuser = (NfUser *) data;

  NF_USER_GET_CLASS(nfuser)->print(nfuser);

  return;
}

  static void
_nf_auth_dump_group(
    gpointer data,
    gpointer user_data) {
  g_return_if_fail ( data != NULL );

  NfGroup *nfgroup = (NfGroup *) data;

  NF_GROUP_GET_CLASS(nfgroup)->print(nfgroup);

  return;
}

  static void
nf_auth_print (NfAuth *ob) {
  g_return_if_fail ( ob != NULL );
  g_return_if_fail ( NF_IS_AUTH(ob) );

  NfAuth *nfauth = NF_AUTH (ob);

  g_print("===========================================\n");
  g_print("%s is called\n", __func__);
  g_print( "init_done\t\t: %d\n", nfauth->init_done );
  g_print( "auth src\t\t: %d\n", nfauth->priv->auth_src );
  g_print("-------------------------------------------\n");
  g_print( "user cnt\t\t: %d[%d]\n", g_list_length(nfauth->priv->userlist), nfauth->priv->usrcnt );
  g_list_foreach(nfauth->priv->userlist, _nf_auth_dump_user, NULL);
  g_print("-------------------------------------------\n");
  g_print( "group cnt\t\t: %d[%d]\n", g_list_length(nfauth->priv->grplist), nfauth->priv->grpcnt );
  g_list_foreach(nfauth->priv->grplist, _nf_auth_dump_group, NULL);
  g_print("===========================================\n");
}

  static void
nf_auth_class_init (NfAuthClass * klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_auth_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = nf_auth_set_property;
  gobject_class->get_property = nf_auth_get_property;

  gobject_class->dispose = nf_auth_dispose;
  gobject_class->finalize = nf_auth_finalize;

  // members
  klass->print = nf_auth_print;

  g_type_class_add_private (klass, sizeof(NfAuthPrivate));

  obj_auth_properties[PROP_AUTH_SRC] =
    g_param_spec_int ("auth_src", "auth_src", "auth src",
                        -1, G_MAXINT, 0,
                        G_PARAM_READWRITE);

  obj_auth_properties[PROP_AUTH_OPENNESS] =
    g_param_spec_uint ("openness", "openness", "openness",
                        0, G_MAXUINT, 0,
                        G_PARAM_READWRITE);

  obj_auth_properties[PROP_AUTH_USRCNT] =
    g_param_spec_uint ("usrcnt", "usrcnt", "usrcnt",
                        0, G_MAXUINT, 0,
                        G_PARAM_READABLE);

  obj_auth_properties[PROP_AUTH_GRPCNT] =
    g_param_spec_uint ("grpcnt", "grpcnt", "grpcnt",
                        0, G_MAXUINT, 0,
                        G_PARAM_READABLE);

  g_object_class_install_properties (gobject_class,
                                    N_AUTH_PROPERTIES,
                                    obj_auth_properties);

  g_print("%s is called\n", __func__);
}

  static void
_nf_obj_free(
    gpointer data,
    gpointer user_data) {
  if( data ) {
    g_object_unref(data);
  }
}

  static void
_auth_sysdb_refresh() {
  NfUser *nfuser;
  NfGroup *nfgroup;
  GList *l;

  guint i;
  char buff[128];

  guint usrcnt, grpcnt;

  g_mutex_lock(_nf_auth_mutex);

  usrcnt = nf_sysdb_get_uint("usr.UCNT");
  grpcnt = nf_sysdb_get_uint("usr.grp.GCNT");

  _nf_auth->priv->userlist = (_g_list_free_full(_nf_auth->priv->userlist, g_object_unref), NULL);
  _nf_auth->priv->grplist = (_g_list_free_full(_nf_auth->priv->grplist, g_object_unref), NULL);

  ////////////////////////////

  _nf_auth->priv->grpcnt = grpcnt;

  for (i=0; i < grpcnt; i++) {
    nfgroup = g_object_new (NF_TYPE_GROUP, NULL);

    nfgroup->index = i;

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.name", i);
    
#if 0
    // 임시 SESSION 기능.
    // DB에서 그룹명을 변경함.
    if(strcmp(nf_sysdb_get_str_nocopy(buff), "OPERATOR") == 0 ){
      nfgroup->name = "USER";
    } else {
      nfgroup->name = nf_sysdb_get_str_nocopy(buff);
    }
#else
    nfgroup->name = nf_sysdb_get_str_nocopy(buff);
#endif


#if 1
    // 임시 SESSION 기능.
    int tmp_permission = 0;
    tmp_permission += 1 << AUTH_IDX_LIVE;
    // SEARCH
    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.search", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_SEARCH;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.archive", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_ARCH;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.rec_setup", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_REC_SETUP;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.event", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_EVENT;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.audio", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_AUDIO;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.microphone", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_MIC;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.remote", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_REMOTE;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.ptz", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_PTZ;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.shutdown", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_SHUTDOWN;
    }

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.sys_setup", i);
    if(nf_sysdb_get_bool(buff)){
      tmp_permission += 1 << AUTH_IDX_SETUP;
    }

    nfgroup->permission = tmp_permission;
#else
    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.permission", i);
    nfgroup->permission = (gint)nf_sysdb_get_uint(buff);
#endif

    ////////////////////////////////
    
    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.grp.G%d.desc", i);
    nfgroup->desc = nf_sysdb_get_str_nocopy(buff);

    _nf_auth->priv->grplist = g_list_append(_nf_auth->priv->grplist, nfgroup);
  }

  ////////////////////////////

  _nf_auth->priv->usrcnt = usrcnt;

  for (i=0; i < usrcnt; i++) {
    nfuser = g_object_new (NF_TYPE_USER, NULL);

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.U%d.name", i);
    g_object_set(nfuser, "name", nf_sysdb_get_str_nocopy(buff), NULL);

    memset(buff, 0, sizeof(buff));
    snprintf(buff, sizeof(buff), "usr.U%d.pass", i);
    g_object_set(nfuser, "passwd", nf_sysdb_get_str_nocopy(buff), NULL);

    memset(buff, 0, sizeof(buff));
#if 1
    // 임시 SESSION 기능.
    snprintf(buff, sizeof(buff), "usr.U%d.grpname", i);
#else
    snprintf(buff, sizeof(buff), "usr.U%d.group", i);
#endif
    
    {
      NfGroup *pGroup;
      
#if 1
      // 임시 SESSION 기능.
      guint group_index = 4;
      gchar *grpname = nf_sysdb_get_str_nocopy(buff);   

      if(strcmp(grpname, "ADMIN") == 0 ){
        group_index = 0;        
      } else if(strcmp(grpname, "MANAGER") == 0 ){
        group_index = 1;
      } else if(strcmp(grpname, "USER") == 0 ){
        group_index = 2;
      } else {
        group_index = 4;
      }
      //////////////////////
#else
      guint group_index = nf_sysdb_get_str_nocopy(buff);
#endif
      
      
      pGroup = nf_auth_getGroupByIndex(_nf_auth, group_index);
      g_object_set(nfuser, "group", pGroup, NULL);
      g_object_set(nfuser, "permission", pGroup->permission, NULL);

      nf_user_print(nfuser);
      nf_group_print(pGroup);
    }

    // memset(buff, 0, sizeof(buff));
    // snprintf(buff, sizeof(buff), "usr.U%d.permission", i);
    // g_object_set(nfuser, "permission", nf_sysdb_get_uint(buff), NULL);

    _nf_auth->priv->userlist = g_list_append(_nf_auth->priv->userlist, nfuser);

  }

  g_mutex_unlock(_nf_auth_mutex);
}

  static gint
nf_auth_finduser(
    gconstpointer a_usr1,
    gconstpointer a_usr2) {
  int ret = 0;

  NfUser *usr1 = (NfUser *)a_usr1;
  NfUser *usr2 = (NfUser *)a_usr2;

  g_return_val_if_fail ( usr1->priv->name != NULL, -1 );
  g_return_val_if_fail ( usr2->priv->name != NULL, -1 );

  if( strcmp(usr1->priv->name, usr2->priv->name) == 0 ) {
    ret = 0;
  } else {
    ret = -1;
  }

  return ret;
}
  static gint
nf_auth_finduserpw(
    gconstpointer a_usr1,
    gconstpointer a_usr2) {
  int ret = 0;

  g_return_val_if_fail ( NF_IS_USER(a_usr1), -1 );
  g_return_val_if_fail ( NF_IS_USER(a_usr2), -1 );

  NfUser *usr1 = (NfUser *)a_usr1;
  NfUser *usr2 = (NfUser *)a_usr2;

  g_return_val_if_fail ( usr1->priv->name != NULL, -1 );
  g_return_val_if_fail ( usr2->priv->name != NULL, -1 );
  g_return_val_if_fail ( usr1->priv->passwd != NULL, -1 );
  g_return_val_if_fail ( usr2->priv->passwd != NULL, -1 );

  if( strcmp(usr1->priv->name, usr2->priv->name) == 0  &&
      strcmp(usr1->priv->passwd, usr2->priv->passwd) == 0 ) {
    ret = 0;
  } else {
    ret = -1;
  }

  return ret;
}

  static gint
_nf_auth_findgroup(
    gconstpointer pgrp,
    gconstpointer grpname) {
  int ret = 0;

  NfGroup *grp = (NfGroup *)pgrp;

  g_return_val_if_fail ( grp->name != NULL, -1 );

  if( strcmp(grp->name, grpname) == 0 ) {
    ret = 0;
  } else {
    ret = -1;
  }

  return ret;
}

  static gint
_nf_auth_finduser_group(
    gconstpointer pgrp,
    gconstpointer pusr) {
  int ret = 0;

  g_return_val_if_fail ( NF_IS_USER(pusr), -1 );;
  g_return_val_if_fail ( NF_IS_GROUP(pgrp), -1 );;

  NfUser *usr = (NfUser *)pusr;
  NfGroup *grp = (NfGroup *)pgrp;

  if( usr->priv->group->index == grp->index ) {
    ret = 0;
  } else {
    ret = -1;
  }

  return ret;
}

/**
  @in
    userid
    passwd
    permission - permission value to be compared with, if -1, ignored
  @out
  */
  gint
nf_auth_getUserAuthorized (
    NfAuth *nfauth,
    const gchar *userid,
    const gchar *passwd,
    int permission) {
  g_return_val_if_fail ( nfauth != NULL , -1);
  g_return_val_if_fail ( userid != NULL , -1);
  g_return_val_if_fail ( passwd != NULL , -1);
  g_return_val_if_fail ( NF_IS_AUTH(nfauth) , -1);

  GList *usrlist;
  GList *grplist;
  gint ret = 0;

  if( nfauth->priv->auth_src == AUTH_SRC_SYSDB ) {
    g_print("GET AUTH FROM SYSDB\n");
    NfUser *finduser = g_object_new( NF_TYPE_USER, NULL );
    g_object_set(finduser,
        "name", userid,
        "passwd", passwd,
        NULL);

    g_mutex_lock(_nf_auth_mutex);
    usrlist = g_list_find_custom( nfauth->priv->userlist, finduser, nf_auth_finduserpw );

    if( usrlist != NULL ) {
      NfUser *usr = (NfUser *) usrlist->data;
      NfGroup *grp = (NfGroup *) usr->priv->group;
      ret = usr->priv->permission & grp->permission;
      nf_user_print(usr);
      nf_group_print(grp);
    } else {
      ret = -1;
    }
    g_mutex_unlock(_nf_auth_mutex);
    g_object_unref(finduser);

  } else if ( nfauth->priv->auth_src == AUTH_SRC_LDAP ) {
#ifdef __NF_UTIL_LDAP_H__
    g_print("GET AUTH FROM LDAP\n");

    gchar *grpname;
    int ldap_group;
    int ldapret = nf_ldap_get_group_by_login(NULL, (char *)userid, (char *)passwd, &ldap_group);

    if ( ldapret == 0 ) {
      if( ldap_group >= 0) {
        NfGroup *pGroup;
        pGroup = nf_auth_getGroupByIndex(_nf_auth, ldap_group);

        ret = pGroup->permission;
        grpname = pGroup->name;
      } else {
      }
    } else if ( ldapret == -1 ) {
      // connect fail
      g_print("LDAP connection fail\n");
      ret = NF_AUTH_ERR_LDAP_CONNFAIL;
    } else if ( ldapret == -2 ) {
      // auth fail
      g_print("LDAP authorization fail\n");
      ret = NF_AUTH_ERR_LDAP_AUTHFAIL;
    } else {
      // unknown fail
      g_print("LDAP unknown fail\n");
      ret = NF_AUTH_ERR_LDAP_FAIL;
    }
#else
    g_print("LDAP is not supported\n");
#endif // __NF_UTIL_LDAP_H__
  } else {
    ret = -1;
  }

  return ret;
}

  gint
nf_auth_getUserPermission(
  NfAuth *nfauth,
  const gchar *userid) {
  GList *res;
  NfUser *usr = NULL;
  gchar *grpname;
  gint permission = 0;

  g_return_val_if_fail ( nfauth != NULL , NULL );
  g_return_val_if_fail ( NF_IS_AUTH(nfauth) , NULL );

  if( nfauth->priv->auth_src == AUTH_SRC_SYSDB ) {
    NfUser *finduser = g_object_new( NF_TYPE_USER, NULL );
    g_object_set(finduser,
        "name", userid,
        NULL);

    g_mutex_lock(_nf_auth_mutex);
    res = g_list_find_custom( nfauth->priv->userlist, finduser, nf_auth_finduser );

    if( res != NULL ) {
      usr = (NfUser *) res->data;
      permission = usr->priv->permission;
    }
    g_mutex_unlock(_nf_auth_mutex);
    g_object_unref(finduser);

  } else if ( nfauth->priv->auth_src == AUTH_SRC_LDAP ) {
#ifdef __NF_UTIL_LDAP_H__
    g_print("GET AUTH FROM LDAP\n");

    gchar *grpname;
    int ldap_group;
    /* TODO no passwd */
    int ldapret = nf_ldap_get_group_by_login(NULL, (char *)userid, (char *)passwd, &ldap_group);

    if ( ldapret == 0 ) {
      if( ldap_group >= 0) {
        group = nf_auth_getGroupByIndex(_nf_auth, ldap_group);

        grpname = group->name;
      } else {
      }
    } else if ( ldapret == -1 ) {
      // connect fail
      g_print("LDAP connection fail\n");
    } else if ( ldapret == -2 ) {
      // auth fail
      g_print("LDAP authorization fail\n");
    } else {
      // unknown fail
      g_print("LDAP unknown fail\n");
    }
#else
    g_print("LDAP is not supported\n");
#endif // __NF_UTIL_LDAP_H__
  }

  return permission;
}

  NfUser *
nf_auth_getUser (
    NfAuth *nfauth,
    const gchar *userid) {
  GList *res;
  NfUser *usr = NULL;
  gchar *grpname;

  g_return_val_if_fail ( nfauth != NULL , NULL );
  g_return_val_if_fail ( NF_IS_AUTH(nfauth) , NULL );

  if( nfauth->priv->auth_src == AUTH_SRC_SYSDB ) {
    NfUser *finduser = g_object_new( NF_TYPE_USER, NULL );
    g_object_set(finduser,
        "name", userid,
        NULL);

    g_mutex_lock(_nf_auth_mutex);
    res = g_list_find_custom( nfauth->priv->userlist, finduser, nf_auth_finduser );

    if( res != NULL ) {
      usr = (NfUser *) res->data;
    }
    g_mutex_unlock(_nf_auth_mutex);
    g_object_unref(finduser);

  } else if ( nfauth->priv->auth_src == AUTH_SRC_LDAP ) {
#ifdef __NF_UTIL_LDAP_H__
    g_print("GET AUTH FROM LDAP\n");

    gchar *grpname;
    int ldap_group;
    /* TODO no passwd */
    int ldapret = nf_ldap_get_group_by_login(NULL, (char *)userid, (char *)passwd, &ldap_group);

    if ( ldapret == 0 ) {
      if( ldap_group >= 0) {
        group = nf_auth_getGroupByIndex(_nf_auth, ldap_group);

        grpname = group->name;
      } else {
      }
    } else if ( ldapret == -1 ) {
      // connect fail
      g_print("LDAP connection fail\n");
    } else if ( ldapret == -2 ) {
      // auth fail
      g_print("LDAP authorization fail\n");
    } else {
      // unknown fail
      g_print("LDAP unknown fail\n");
    }
#else
    g_print("LDAP is not supported\n");
#endif // __NF_UTIL_LDAP_H__
  }

  return usr;
}

/*
 TODO
  */
  gboolean
nf_auth_addUser (
    NfAuth *nfauth,
    const gchar *userid,
    const gchar *passwd,
    const gchar *grpid) {
  return FALSE;
}

  NfGroup *
nf_auth_getGroupByIndex(
    NfAuth *nfauth,
    const guint index) {
  g_return_val_if_fail ( nfauth != NULL , NULL);
  g_return_val_if_fail ( NF_IS_AUTH(nfauth) , NULL);

  GList *grplist;
  grplist = g_list_nth( nfauth->priv->grplist, index);

  if ( grplist != NULL ) {
    return (NfGroup *) grplist->data;
  }

  return NULL;
}
  gchar *
nf_auth_getGroupNameByIndex(
    NfAuth *nfauth,
    const guint index) {
  NfGroup *grp = nf_auth_getGroupByIndex(nfauth, index);

  return grp->name;
}

  NfGroup *
nf_auth_getUserGroup (
    NfAuth *nfauth,
    const gchar *userid,
    const gchar *passwd) {
  GList *res;
  NfGroup *group = NULL;
  gchar *grpname;

  g_return_val_if_fail ( nfauth != NULL , NULL );
  g_return_val_if_fail ( NF_IS_AUTH(nfauth) , NULL );


  if( nfauth->priv->auth_src == AUTH_SRC_SYSDB ) {
    NfUser *finduser = g_object_new( NF_TYPE_USER, NULL );
    g_object_set(finduser,
        "name", userid,
        NULL);

    g_mutex_lock(_nf_auth_mutex);
    res = g_list_find_custom( nfauth->priv->userlist, finduser, nf_auth_finduser );

    if( res != NULL ) {
      NfUser *tmp = (NfUser *) res->data;
      group = tmp->priv->group;
    }
    g_mutex_unlock(_nf_auth_mutex);
    g_object_unref(finduser);

  } else if ( nfauth->priv->auth_src == AUTH_SRC_LDAP ) {
#ifdef __NF_UTIL_LDAP_H__
    g_print("GET AUTH FROM LDAP\n");

    gchar *grpname;
    int ldap_group;
    int ldapret = nf_ldap_get_group_by_login(NULL, (char *)userid, (char *)passwd, &ldap_group);

    if ( ldapret == 0 ) {
      if( ldap_group >= 0) {
        group = nf_auth_getGroupByIndex(_nf_auth, ldap_group);

        grpname = group->name;
      } else {
      }
    } else if ( ldapret == -1 ) {
      // connect fail
      g_print("LDAP connection fail\n");
    } else if ( ldapret == -2 ) {
      // auth fail
      g_print("LDAP authorization fail\n");
    } else {
      // unknown fail
      g_print("LDAP unknown fail\n");
    }
#else
    g_print("LDAP is not supported\n");
#endif // __NF_UTIL_LDAP_H__
  }

  return group;
}

  gint
nf_auth_getChannelMasked (
    NfAuth *nfauth,
    const gchar *userid) {
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

  static void
_auth_sysdbchange_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data) {
  char buff[128];
  gboolean ischanged;

  g_return_if_fail(pinfo != NULL);
  g_return_if_fail(_nf_auth != NULL);

  // ischanged = _nf_auth->priv->auth_src != nf_sysdb_get_uint("usr.auth.source");
  ischanged |= _nf_auth->priv->usrcnt != nf_sysdb_get_uint("usr.UCNT");
  ischanged |= TRUE; // TODO condition definition is required

  _nf_auth->priv->usrcnt = nf_sysdb_get_uint("usr.UCNT");
  // _nf_auth->priv->auth_src = nf_sysdb_get_uint("usr.auth.source");
  // _nf_auth->priv->openness = nf_sysdb_get_uint("usr.auth.openness");

  g_print("%s is called\n", __func__);

  if( ischanged ) {
    _auth_sysdb_refresh();
  }
}

  NfAuth *
get_NfAuth() {
  return _nf_auth;
}

  void
nf_auth_refresh() {
  _auth_sysdb_refresh();
}

  gint
nf_auth_main() {
  gint ret = 0;

  _nf_auth = g_object_new ( NF_TYPE_AUTH , NULL);
  _nf_auth_mutex = g_mutex_new();

#if 0 // FOR TEST
  /**TEST s**/
  NfUser *nfuser;
  nfuser = g_object_new ( NF_TYPE_USER, NULL );
  nfuser->priv->name = g_strdup_printf("admin");
  nfuser->priv->grpname = g_strdup_printf("ADMIN");
  nfuser->priv->permission = 0x7fffffff;

  nfauth->priv->userlist = g_list_append(nfauth->priv->userlist, nfuser);

  nfuser = g_object_new ( NF_TYPE_USER, NULL );
  nfuser->priv->name = g_strdup_printf("admin2");
  nfuser->priv->grpname = g_strdup_printf("ADMIN");
  nfuser->priv->permission = 0x7fffffff;

  nfauth->priv->userlist = g_list_append(nfauth->priv->userlist, nfuser);

  /**TEST e**/
#endif
  _auth_sysdb_refresh();

  NF_AUTH_GET_CLASS(_nf_auth)->print(_nf_auth);

  return ret;
}

