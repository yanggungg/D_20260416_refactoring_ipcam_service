#ifndef __NF_OBJECT_H__
#define __NF_OBJECT_H__

#include <stdio.h>
#include <glib.h> 
#include <glib-object.h> 

/* type macro */
#define NF_TYPE_OBJECT					(nf_object_get_type ())

#define NF_IS_OBJECT(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_OBJECT))
#define NF_IS_OBJECT_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_OBJECT))

#define NF_OBJECT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_OBJECT, NfObjectClass))
#define NF_OBJECT(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_OBJECT, NfObject))
#define NF_OBJECT_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_OBJECT, NfObjectClass))

#define NF_OBJECT_CAST(obj)				((NfObject*)(obj))
#define NF_OBJECT_CLASS_CAST(klass)		((NfObjectClass*)(klass))

/**
 * NfObjectFlags:
 * @NF_OBJECT_DISPOSING: the object is been destroyed, do use it anymore
 * @NF_OBJECT_FLOATING:  the object has a floating reference count (e.g. its
 *  not assigned to a bin)
 * @NF_OBJECT_FLAG_LAST: subclasses can add additional flags starting from this flag
 *
 * The standard flags that an NfObject may have.
 */
typedef enum
{
  NF_OBJECT_DISPOSING = (1<<0),
  NF_OBJECT_FLOATING = (1<<1),

  /* padding */
  
  NF_OBJECT_FLAG_LAST = (1<<4)
} NfObjectFlags;

/**
 * NF_OBJECT_REFCOUNT:
 * @obj: a #NfObject
 *
 * Get access to the reference count field of the object.
 */
#define NF_OBJECT_REFCOUNT(obj)				(((GObject*)(obj))->ref_count)
/**
 * NF_OBJECT_REFCOUNT_VALUE:
 * @obj: a #NfObject
 *
 * Get the reference count value of the object.
 */
#define NF_OBJECT_REFCOUNT_VALUE(obj)		g_atomic_int_get ((gint *) &NF_OBJECT_REFCOUNT(obj))

/* we do a NF_OBJECT_CAST to avoid type checking, better call these
 * function with a valid object! */

/**
 * NF_OBJECT_GET_LOCK:
 * @obj: a #NfObject
 *
 * Acquire a reference to the mutex of this object.
 */
#define NF_OBJECT_GET_LOCK(obj)				(NF_OBJECT_CAST(obj)->lock)
/**
 * NF_OBJECT_LOCK:
 * @obj: a #NfObject to lock
 *
 * This macro will obtain a lock on the object, making serialization possible.
 * It blocks until the lock can be obtained.
 */
#define NF_OBJECT_LOCK(obj)					g_mutex_lock(NF_OBJECT_GET_LOCK(obj))
/**
 * NF_OBJECT_TRYLOCK:
 * @obj: a #Object.
 *
 * This macro will try to obtain a lock on the object, but will return with
 * FALSE if it can't get it immediately.
 */
#define NF_OBJECT_TRYLOCK(obj)				g_mutex_trylock(NF_OBJECT_GET_LOCK(obj))
/**
 * NF_OBJECT_UNLOCK:
 * @obj: a #NfObject to unlock.
 *
 * This macro releases a lock on the object.
 */
#define NF_OBJECT_UNLOCK(obj)				g_mutex_unlock(NF_OBJECT_GET_LOCK(obj))


/**
 * NF_OBJECT_NAME:
 * @obj: a #NfObject
 *
 * Get the name of this object
 */
#define NF_OBJECT_NAME(obj)					(NF_OBJECT_CAST(obj)->name)
/**
 * NF_OBJECT_PARENT:
 * @obj: a #NfObject
 *
 * Get the parent of this object
 */
#define NF_OBJECT_PARENT(obj)				(NF_OBJECT_CAST(obj)->parent)


/**
 * NF_OBJECT_FLAGS:
 * @obj: a #NfObject
 *
 * This macro returns the entire set of flags for the object.
 */
#define NF_OBJECT_FLAGS(obj)				(NF_OBJECT_CAST (obj)->flags)
/**
 * NF_OBJECT_FLAG_IS_SET:
 * @obj: a #NfObject
 * @flag: Flag to check for
 *
 * This macro checks to see if the given flag is set.
 */
#define NF_OBJECT_FLAG_IS_SET(obj,flag)		((NF_OBJECT_FLAGS (obj) & (flag)) == (flag))
/**
 * NF_OBJECT_FLAG_SET:
 * @obj: a #NfObject
 * @flag: Flag to set
 *
 * This macro sets the given bits.
 */
#define NF_OBJECT_FLAG_SET(obj,flag)		(NF_OBJECT_FLAGS (obj) |= (flag))
/**
 * NF_OBJECT_FLAG_UNSET:
 * @obj: a #NfObject
 * @flag: Flag to set
 *
 * This macro usets the given bits.
 */
#define NF_OBJECT_FLAG_UNSET(obj,flag)		(NF_OBJECT_FLAGS (obj) &= ~(flag))


/**
 * NF_OBJECT_IS_DISPOSING:
 * @obj: a #NfObject
 *
 * Check if the given object is beeing destroyed.
 */
#define NF_OBJECT_IS_DISPOSING(obj)			(NF_OBJECT_FLAG_IS_SET (obj, NF_OBJECT_DISPOSING))
/**
 * NF_OBJECT_IS_FLOATING:
 * @obj: a #NfObject
 *
 * Check if the given object is floating (has no owner).
 */
#define NF_OBJECT_IS_FLOATING(obj)			(NF_OBJECT_FLAG_IS_SET (obj, NF_OBJECT_FLOATING))


typedef struct _NfObject NfObject;
typedef struct _NfObjectClass NfObjectClass;

/**
 * NfObject:
 * @refcount: unused
 * @lock: object LOCK
 * @name: The name of the object
 * @name_prefix: used for debugging
 * @parent: this object's parent, weak ref
 * @flags: use NF_OBJECT_IS_XXX macros to access the flags
 *
 * NfDVR base object class.
 */

struct _NfObject {
  GObject 	 	object;

  /*< public >*/
  gint			refcount;

  /*< public >*/ /* with LOCK */
  GMutex        *lock;        /* object LOCK */
  gchar         *name;        /* object name */
  gchar         *name_prefix; /* used for debugging */
  NfObject      *parent;      /* this object's parent, weak ref */
  guint32		flags;

  /*< private >*/
    
};

/**
 * NF_CLASS_GET_LOCK:
 * @obj: a #NfObjectClass
 *
 * This macro will return the class lock used to protect deep_notify signal
 * emission on thread-unsafe glib versions (glib < 2.8).
 */
#define NF_CLASS_GET_LOCK(obj)		(NF_OBJECT_CLASS_CAST(obj)->lock)
/**
 * NF_CLASS_LOCK:
 * @obj: a #NfObjectClass
 *
 * Lock the class.
 */
#define NF_CLASS_LOCK(obj)			(g_static_rec_mutex_lock(NF_CLASS_GET_LOCK(obj)))
/**
 * NF_CLASS_TRYLOCK:
 * @obj: a #NfObjectClass
 *
 * Try to lock the class, returns TRUE if class could be locked.
 */
#define NF_CLASS_TRYLOCK(obj)		(g_static_rec_mutex_trylock(NF_CLASS_GET_LOCK(obj)))
/**
 * NF_CLASS_UNLOCK:
 * @obj: a #NfObjectClass
 *
 * Unlock the class.
 */
#define NF_CLASS_UNLOCK(obj)		(g_static_rec_mutex_unlock(NF_CLASS_GET_LOCK(obj)))

/*
 * NfObjectClass:
 *
 * @signal_object: is used to signal to the whole class
 */
struct _NfObjectClass {

  GObjectClass	parent_class;    

  GStaticRecMutex *lock;

  /* signals */
  void		(*parent_set)       	(NfObject *object, NfObject *parent);
  void		(*parent_unset)     	(NfObject *object, NfObject *parent);
  void		(*destroy)				(NfObject *object);

  /*< public >*/
  /* Non overridable class methods to set and get per class arguments */
  
  /* virtual methods for subclasses */

  /*< private >*/
    
};


/* normal GObject stuff */
GType		nf_object_get_type			(void);

/* name routines */
gboolean	nf_object_set_name			(NfObject *object, const gchar *name);
gchar*		nf_object_get_name			(NfObject *object);
void		nf_object_set_name_prefix	(NfObject *object, const gchar *name_prefix);
gchar*		nf_object_get_name_prefix	(NfObject *object);


/* refcounting + life cycle */
gpointer	nf_object_ref				(gpointer object);
void		nf_object_unref				(gpointer object);
void 		nf_object_sink				(gpointer object);

/* replace object pointer */
void 		nf_object_replace			(NfObject **oldobj, NfObject *newobj);

/* Object data method variants that operate on key strings. */
void	 	nf_object_set_data			(NfObject *object, const gchar *key, gpointer data);
void	 	nf_object_set_data_full		(NfObject *object, const gchar *key, gpointer data, GDestroyNotify destroy);
void	 	nf_object_remove_data		(NfObject *object, const gchar *key);
gpointer 	nf_object_get_data			(NfObject *object, const gchar *key);
void	 	nf_object_remove_no_notify	(NfObject *object, const gchar *key);
void		nf_object_set_user_data		(NfObject *object, gpointer	 data);
gpointer	nf_object_get_user_data		(NfObject *object);


/* Object data method variants that operate on key ids. */
void 		nf_object_set_data_by_id		(NfObject *object, GQuark data_id, gpointer data);
void 		nf_object_set_data_by_id_full	(NfObject *object, GQuark data_id, gpointer data, GDestroyNotify destroy);
gpointer 	nf_object_get_data_by_id		(NfObject *object, GQuark data_id);
void  		nf_object_remove_data_by_id		(NfObject *object, GQuark data_id);
void  		nf_object_remove_no_notify_by_id	(NfObject	 *object, GQuark key_id);

#define		nf_object_data_try_key			g_quark_try_string
#define		nf_object_data_force_id			g_quark_from_string


#endif /* __NF_OBJECT_H__ */
