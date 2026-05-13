#ifndef __NF_NOTIFY_H__
#define __NF_NOTIFY_H__

#include <glib.h>
#include <glib-object.h>

#include "nf_object.h"
#include "nf_notify.h"

#define NF_NOTIFY_QSIZE_FULL		128
#define NF_NOTIFY_QSIZE_THRESHOLD	64

typedef struct 	_NF_JOB_INFO
{
	char		*job_name;
	guint		job_flag;	// __CMD ��ũ�� Ȱ��; 
	
	// internal process;
	void 		(*process_cb)(struct _NF_JOB_INFO *job);
	gpointer 	process_data;
	guint		process_len;
	
	// complete callback
	void 		(*cmpl_cb)(void *data);
	gpointer 	cmpl_data;
	guint		cmpl_len;

	// parameter;
	gint		params[3];
	gint		ret;
}NF_JOB_INFO;

/* 
   NF_NOTIFY_INFO ����ü type��  
      NF_NOTIFY_PARAM    d union�� ��� 
      NF_NOTIFY_CHMAP      c union�� ��� 
      NF_NOTIFY_POINTER    p union�� ���          
*/ 
typedef enum { 
   NF_NOTIFY_PARAM, 
   NF_NOTIFY_CHMAP, 
   NF_NOTIFY_POINTER    
} NF_NOTIFY_TYPE ;


#if defined(_IPX_32P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
typedef struct    _NF_NOTIFY_INFO 
{ 
   guint      type; 
   GTimeVal   timestamp; 
   union {       
      struct    {   guint       params[8];   } d; 
      struct    {   gchar       chmap[32];   } c; 
      struct    {   guint       len;             
               gpointer    ptr;        
               guint       reserved[6]; } p; 
   }; 
} NF_NOTIFY_INFO; 
#else
typedef struct    _NF_NOTIFY_INFO 
{ 
   guint      type; 
   GTimeVal   timestamp; 
   union {       
      struct    {   guint       params[4];   } d; 
      struct    {   gchar       chmap[16];   } c; 
      struct    {   guint       len;             
               gpointer    ptr;        
               guint       reserved[2]; } p; 
   }; 
} NF_NOTIFY_INFO; 
#endif

// ���� ó���� ���� ����ϴ� ����ü
typedef void (*NF_NOTIFY_CB_FUNC) ( NF_NOTIFY_INFO *pinfo, gpointer data); 
typedef struct _NF_NOTIFY_CALLBACK
{
	NF_NOTIFY_CB_FUNC	cb_func;
	gpointer	cb_data;
	gulong		cb_handle;	
	GParamSpec	*pspec;
}NF_NOTIFY_CALLBACK;

typedef struct _NF_NOTIFY_QITEM
{
	GParamSpec		*pspec;
	NF_NOTIFY_INFO 	*pitem;
}NF_NOTIFY_QITEM;

/* type macro */
#define NF_TYPE_NOTIFY				(nf_notify_get_type ())

#define NF_IS_NOTIFY(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_NOTIFY))
#define NF_IS_NOTIFY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_NOTIFY))

#define NF_NOTIFY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_NOTIFY, NfNotifyClass))
#define NF_NOTIFY(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_NOTIFY, NfNotify))
#define NF_NOTIFY_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_NOTIFY, NfNotifyClass))

#define NF_NOTIFY_CAST(obj)			((NfNotify*)(obj))
#define NF_NOTIFY_CLASS_CAST(klass)	((NfNotifyClass*)(klass))

typedef struct _NfNotify 		NfNotify;
typedef struct _NfNotifyClass 	NfNotifyClass;

/**
 * NfNotify:
 *
 * NfDVR notify class
 */

struct _NfNotify {
	NfObject 	 	object;
	
	/*< public >*/
	gint			init_done;
	
	GAsyncQueue		*queue;			
	GThread			*thread;
	
	gint			thread_run;
	gint			thread_status;
										
	/*< public >*/ /* with LOCK */
/*
	gobject���� property�� notify�� �̿��Ϸ� �����Ϸ� ������
	������ ��ġ�� ����, �����尡 �����Ǵ� ��Ȳ ������,
	notify��⿡�� asyncqueue�� thread�� ���� �ξ 
	ó���Ѵ�.
		
*/		
	/*< private >*/	
};

struct _NfNotifyClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

gboolean 
nf_notify_init(int wait);

NF_NOTIFY_INFO *
nf_notify_new();

NF_NOTIFY_INFO *
nf_notify_new_size( guint size);

NF_NOTIFY_INFO *
nf_notify_dup( NF_NOTIFY_INFO *src);

void
nf_notify_free( NF_NOTIFY_INFO *src);


gulong 
nf_notify_connect_cb(const gchar *property_name, NF_NOTIFY_CB_FUNC cb, gpointer data);

gboolean
nf_notify_isconn_cb_byid(const gchar *property_name, gulong handler_id);

gboolean
nf_notify_isconn_cb_byfunc(const gchar *property_name, NF_NOTIFY_CB_FUNC cb);

gboolean
nf_notify_remove_cb(const gchar *property_name, gulong handler_id);

gboolean
nf_notify_fire(const gchar *property_name, NF_NOTIFY_INFO *data);

gboolean
nf_notify_fire_params(const gchar *property_name,
						guint param0, guint param1, guint param2, guint param3);

gboolean
nf_notify_fire_chmap(const gchar *property_name, gchar *chmap);
						
gboolean
nf_notify_fire_pointer(const gchar *property_name, gpointer data, gint size);

gpointer
nf_notify_get(const gchar *property_name);

guint
nf_notify_get_param0(const gchar *property_name);

guint
nf_notify_get_param1(const gchar *property_name);

guint
nf_notify_get_param_idx(const gchar *property_name, guint idx);

gulong
nf_notify_get_update_time(const gchar *property_name);
	
#endif

