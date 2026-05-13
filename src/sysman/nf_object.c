#include <string.h>
#include "nf_object.h"

/* Object signals and args */
enum
{
  PARENT_SET,
  PARENT_UNSET,
  DESTROY,
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_NAME
      /* FILL ME */
};

/* maps type name quark => count */
static GData *object_name_counts = NULL;

G_LOCK_DEFINE_STATIC (object_name_mutex);

static void nf_object_class_init (NfObjectClass * klass);
static void nf_object_init (GTypeInstance * instance, gpointer g_class);

static void nf_object_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_object_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_object_dispose (GObject * object);
static void nf_object_finalize (GObject * object);

static gboolean nf_object_set_name_default (NfObject * object);


static GObjectClass *parent_class = NULL;
static guint nf_object_signals[LAST_SIGNAL] = { 0 };


GType
nf_object_get_type (void)
{
  static GType nf_object_type = 0;

  if (G_UNLIKELY (nf_object_type == 0)) {
    static const GTypeInfo object_info = {
      sizeof (NfObjectClass),
      NULL,
      NULL,
      (GClassInitFunc) nf_object_class_init,
      NULL,
      NULL,
      sizeof (NfObject),
      0,
      (GInstanceInitFunc) nf_object_init,
      NULL
    };

    nf_object_type =
        g_type_register_static (G_TYPE_OBJECT, "NfObject", &object_info, 0);
  }
  return nf_object_type;
}

static void
nf_object_class_init (NfObjectClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

#ifndef NF_DISABLE_TRACE
//  _nf_object_trace = gst_alloc_trace_register (g_type_name (NF_TYPE_OBJECT));
#endif

  gobject_class->set_property = nf_object_set_property;
  gobject_class->get_property = nf_object_get_property;

  g_object_class_install_property (gobject_class, ARG_NAME,
      g_param_spec_string ("name", "Name", "The name of the object",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * NfObject::parent-set:
   * @gstobject: a #NfObject
   * @parent: the new parent
   *
   * Emitted when the parent of an object is set.
   */
  nf_object_signals[PARENT_SET] =
      g_signal_new ("parent-set", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (NfObjectClass, parent_set), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, G_TYPE_OBJECT);

  /**
   * NfObject::parent-unset:
   * @gstobject: a #NfObject
   * @parent: the old parent
   *
   * Emitted when the parent of an object is unset.
   */
  nf_object_signals[PARENT_UNSET] =
      g_signal_new ("parent-unset", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (NfObjectClass, parent_unset), NULL,
      NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, G_TYPE_OBJECT);


  /**
   * NfObject::destory:
   * @gstobject: a #NfObject
   * @parent: the old parent
   *
   * Emitted when object가 파괴 되었을 때.
   */
  nf_object_signals[DESTROY] =
      g_signal_new ("destroy", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      G_STRUCT_OFFSET (NfObjectClass, destroy), NULL, NULL, 
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);


  klass->lock = g_new0 (GStaticRecMutex, 1);

  g_static_rec_mutex_init (klass->lock);

  gobject_class->dispose = nf_object_dispose;
  gobject_class->finalize = nf_object_finalize;
}

static void
nf_object_init (GTypeInstance * instance, gpointer g_class)
{
  NfObject *self = NF_OBJECT (instance);

  self->lock = g_mutex_new ();
  self->parent = NULL;
  self->name = NULL;
 
#ifndef NF_DISABLE_TRACE
  //gst_alloc_trace_new (_nf_object_trace, object);
#endif

  self->flags = 0;
  NF_OBJECT_FLAG_SET (self, NF_OBJECT_FLOATING);
}


static void
nf_object_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  NfObject *self;

  self = NF_OBJECT (object);

  switch (prop_id) {
    case ARG_NAME:
      nf_object_set_name (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
nf_object_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  NfObject *self;

  self = NF_OBJECT (object);

  switch (prop_id) {
    case ARG_NAME:
      g_value_take_string (value, nf_object_get_name (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/**
 * nf_object_ref:
 * @object: a #NfObject to reference
 *
 * Increments the refence count on @object. This function
 * does not take the lock on @object because it relies on
 * atomic refcounting.
 *
 * This object returns the input parameter to ease writing
 * constructs like :
 *  result = nf_object_ref (object->parent);
 *
 * Returns: A pointer to @object
 */
gpointer
nf_object_ref (gpointer object)
{
  g_return_val_if_fail (object != NULL, NULL);

  g_object_ref (object);

  return object;
}

/**
 * nf_object_unref:
 * @object: a #NfObject to unreference
 *
 * Decrements the refence count on @object.  If reference count hits
 * zero, destroy @object. This function does not take the lock
 * on @object as it relies on atomic refcounting.
 *
 * The unref method should never be called with the LOCK held since
 * this might deadlock the dispose function.
 */
void
nf_object_unref (gpointer object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (((GObject *) object)->ref_count > 0);
  
  g_object_unref (object);
}


/**
 * nf_object_replace:
 * @oldobj: pointer to a place of a #NfObject to replace
 * @newobj: a new #NfObject
 *
 * Unrefs the #NfObject pointed to by @oldobj, refs @newobj and
 * puts @newobj in *@oldobj. Be carefull when calling this
 * function, it does not take any locks. You might want to lock
 * the object owning @oldobj pointer before calling this
 * function.
 *
 * Make sure not to LOCK @oldobj because it might be unreffed
 * which could cause a deadlock when it is disposed.
 */
void
nf_object_replace (NfObject ** oldobj, NfObject * newobj)
{
  g_return_if_fail (oldobj != NULL);
  g_return_if_fail (*oldobj == NULL || NF_IS_OBJECT (*oldobj));
  g_return_if_fail (newobj == NULL || NF_IS_OBJECT (newobj));

  if (G_LIKELY (*oldobj != newobj)) {
    if (newobj)
      nf_object_ref (newobj);
    if (*oldobj)
      nf_object_unref (*oldobj);

    *oldobj = newobj;
  }
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_object_dispose (GObject * object)
{
  NfObject *parent;

  //NF_CAT_LOG_OBJECT (NF_CAT_REFCOUNTING, object, "dispose");

  NF_OBJECT_LOCK (object);
  if ((parent = NF_OBJECT_PARENT (object)))
    goto have_parent;
  NF_OBJECT_PARENT (object) = NULL;
  NF_OBJECT_UNLOCK (object);

  parent_class->dispose (object);

  return;

  /* ERRORS */
have_parent:
  {
    g_critical ("\nTrying to dispose object \"%s\", but it still has a "
        "parent \"%s\".\nYou need to let the parent manage the "
        "object instead of unreffing the object directly.\n",
        NF_OBJECT_NAME (object), NF_OBJECT_NAME (parent));
    NF_OBJECT_UNLOCK (object);
    /* ref the object again to revive it in this error case */
    object = nf_object_ref (object);
    return;
  }
}

/* finalize is called when the object has to free its resources */
static void
nf_object_finalize (GObject * object)
{
  NfObject *gstobject = NF_OBJECT (object);

  //NF_CAT_LOG_OBJECT (NF_CAT_REFCOUNTING, object, "finalize");

  g_signal_handlers_destroy (object);

  g_free (gstobject->name);
  g_mutex_free (gstobject->lock);

#ifndef NF_DISABLE_TRACE
  //gst_alloc_trace_free (_nf_object_trace, object);
#endif

  parent_class->finalize (object);
}

static gboolean
nf_object_set_name_default (NfObject * object)
{
  const gchar *type_name;
  gint count;
  gchar *name, *tmp;
  gboolean result;
  GQuark q;

  /* to ensure guaranteed uniqueness across threads, only one thread
   * may ever assign a name */
  G_LOCK (object_name_mutex);

  if (!object_name_counts) {
    g_datalist_init (&object_name_counts);
  }

  q = g_type_qname (G_OBJECT_TYPE (object));
  count = GPOINTER_TO_INT (g_datalist_id_get_data (&object_name_counts, q));
  g_datalist_id_set_data (&object_name_counts, q, GINT_TO_POINTER (count + 1));

  G_UNLOCK (object_name_mutex);

  /* GstFooSink -> foosinkN */
  type_name = g_quark_to_string (q);
  if (strncmp (type_name, "Gst", 3) == 0)
    type_name += 3;
  tmp = g_strdup_printf ("%s%d", type_name, count);
  name = g_ascii_strdown (tmp, strlen (tmp));
  g_free (tmp);

  result = nf_object_set_name (object, name);
  g_free (name);

  return result;
}

/**
 * nf_object_set_name:
 * @object: a #NfObject
 * @name:   new name of object
 *
 * Sets the name of @object, or gives @object a guaranteed unique
 * name (if @name is NULL).
 * This function makes a copy of the provided name, so the caller
 * retains ownership of the name it sent.
 *
 * Returns: TRUE if the name could be set. Since Objects that have
 * a parent cannot be renamed, this function returns FALSE in those
 * cases.
 *
 * MT safe.  This function grabs and releases @object's LOCK.
 */
gboolean
nf_object_set_name (NfObject * object, const gchar * name)
{
  gboolean result;

  g_return_val_if_fail (NF_IS_OBJECT (object), FALSE);

  NF_OBJECT_LOCK (object);

  /* parented objects cannot be renamed */
  if (G_UNLIKELY (object->parent != NULL))
    goto had_parent;

  if (name != NULL) {
    g_free (object->name);
    object->name = g_strdup (name);
    NF_OBJECT_UNLOCK (object);
    result = TRUE;
  } else {
    NF_OBJECT_UNLOCK (object);
    result = nf_object_set_name_default (object);
  }
  return result;

  /* error */
had_parent:
  {
    g_warning ("parented objects can't be renamed");
    NF_OBJECT_UNLOCK (object);
    return FALSE;
  }
}

/**
 * nf_object_get_name:
 * @object: a #NfObject
 *
 * Returns a copy of the name of @object.
 * Caller should g_free() the return value after usage.
 * For a nameless object, this returns NULL, which you can safely g_free()
 * as well.
 *
 * Returns: the name of @object. g_free() after usage.
 *
 * MT safe. This function grabs and releases @object's LOCK.
 */
gchar *
nf_object_get_name (NfObject * object)
{
  gchar *result = NULL;

  g_return_val_if_fail (NF_IS_OBJECT (object), NULL);

  NF_OBJECT_LOCK (object);
  result = g_strdup (object->name);
  NF_OBJECT_UNLOCK (object);

  return result;
}

/**
 * nf_object_set_name_prefix:
 * @object:      a #NfObject
 * @name_prefix: new name prefix of @object
 *
 * Sets the name prefix of @object to @name_prefix.
 * This function makes a copy of the provided name prefix, so the caller
 * retains ownership of the name prefix it sent.
 *
 * MT safe.  This function grabs and releases @object's LOCK.
 */
void
nf_object_set_name_prefix (NfObject * object, const gchar * name_prefix)
{
  g_return_if_fail (NF_IS_OBJECT (object));

  NF_OBJECT_LOCK (object);
  g_free (object->name_prefix);
  object->name_prefix = g_strdup (name_prefix); /* NULL gives NULL */
  NF_OBJECT_UNLOCK (object);
}

/**
 * nf_object_get_name_prefix:
 * @object: a #NfObject
 *
 * Returns a copy of the name prefix of @object.
 * Caller should g_free() the return value after usage.
 * For a prefixless object, this returns NULL, which you can safely g_free()
 * as well.
 *
 * Returns: the name prefix of @object. g_free() after usage.
 *
 * MT safe. This function grabs and releases @object's LOCK.
 */
gchar *
nf_object_get_name_prefix (NfObject * object)
{
  gchar *result = NULL;

  g_return_val_if_fail (NF_IS_OBJECT (object), NULL);

  NF_OBJECT_LOCK (object);
  result = g_strdup (object->name_prefix);
  NF_OBJECT_UNLOCK (object);

  return result;
}

/*****************************************
 * NfObject object_data mechanism
 *****************************************/

void
nf_object_set_data_by_id (NfObject        *object,
			   GQuark	     data_id,
			   gpointer          data)
{
  g_return_if_fail (NF_IS_OBJECT (object));

  g_datalist_id_set_data (&G_OBJECT (object)->qdata, data_id, data);
}

void
nf_object_set_data (NfObject        *object,
		     const gchar      *key,
		     gpointer          data)
{
  g_return_if_fail (NF_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_set_data (&G_OBJECT (object)->qdata, key, data);
}

void
nf_object_set_data_by_id_full (NfObject        *object,
				GQuark		  data_id,
				gpointer          data,
				GDestroyNotify  destroy)
{
  g_return_if_fail (NF_IS_OBJECT (object));

  g_datalist_id_set_data_full (&G_OBJECT (object)->qdata, data_id, data, destroy);
}

void
nf_object_set_data_full (NfObject        *object,
			  const gchar      *key,
			  gpointer          data,
			  GDestroyNotify  destroy)
{
  g_return_if_fail (NF_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_set_data_full (&G_OBJECT (object)->qdata, key, data, destroy);
}

gpointer
nf_object_get_data_by_id (NfObject   *object,
			   GQuark       data_id)
{
  g_return_val_if_fail (NF_IS_OBJECT (object), NULL);

  return g_datalist_id_get_data (&G_OBJECT (object)->qdata, data_id);
}

gpointer
nf_object_get_data (NfObject   *object,
		     const gchar *key)
{
  g_return_val_if_fail (NF_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  return g_datalist_get_data (&G_OBJECT (object)->qdata, key);
}

void
nf_object_remove_data_by_id (NfObject   *object,
			      GQuark       data_id)
{
  g_return_if_fail (NF_IS_OBJECT (object));

  g_datalist_id_remove_data (&G_OBJECT (object)->qdata, data_id);
}

void
nf_object_remove_data (NfObject   *object,
			const gchar *key)
{
  g_return_if_fail (NF_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_remove_data (&G_OBJECT (object)->qdata, key);
}

void
nf_object_remove_no_notify_by_id (NfObject      *object,
				   GQuark          key_id)
{
  g_return_if_fail (NF_IS_OBJECT (object));

  g_datalist_id_remove_no_notify (&G_OBJECT (object)->qdata, key_id);
}

void
nf_object_remove_no_notify (NfObject       *object,
			     const gchar     *key)
{
  g_return_if_fail (NF_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_remove_no_notify (&G_OBJECT (object)->qdata, key);
}
