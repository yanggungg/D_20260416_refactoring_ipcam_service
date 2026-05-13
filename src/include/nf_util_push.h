#ifndef	__NF_UTIL_PUSH_H
#define	__NF_UTIL_PUSH_H


/* type macro */
#define NF_TYPE_UTIL_PUSH					(nf_util_push_get_type ())

#define NF_IS_UTIL_PUSH(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_UTIL_PUSH))
#define NF_IS_UTIL_PUSH_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_UTIL_PUSH))

#define NF_UTIL_PUSH_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_UTIL_PUSH, NfUtilPushClass))
#define NF_UTIL_PUSH(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_UTIL_PUSH, NfUtilPush))
#define NF_UTIL_PUSH_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_UTIL_PUSH, NfUtilPushClass))

#define NF_UTIL_PUSH_CAST(obj)			((NfUtilPush*)(obj))
#define NF_UTIL_PUSH_CLASS_CAST(klass)	((NfUtilPushClass*)(klass))

typedef struct _NfUtilPush 		NfUtilPush;
typedef struct _NfUtilPushClass 	NfUtilPushClass;

/**
 * NfUtilPush:
 *
 * NfDVR util_push class
 */
struct _NfUtilPush {
	NfObject 	 	object;
	
	/*< public >*/	
	gint			init_done;
	
	GAsyncQueue		*queue;	

	gint			request_cnt;
	gint			process_cnt;
	gint			error_cnt;

	GThread			*thread;	
	gint			thread_run;
	gint			thread_status;
			
	/*< public >*/ /* with LOCK */

	/*< private >*/	
};

struct _NfUtilPushClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

typedef enum _NF_UTIL_PUSH_ERROR_E {
	NF_UTIL_PUSH_ERROR_SUCCESS	= 0,
	NF_UTIL_PUSH_ERROR_FAILED	= 1,	
	NF_UTIL_PUSH_ERROR_FROM		= 2,
	NF_UTIL_PUSH_ERROR_TO 		= 3,

	NF_UTIL_PUSH_ERROR_SUBJECT	= 4,
	NF_UTIL_PUSH_ERROR_CONTENTS	= 5,
	NF_UTIL_PUSH_ERROR_IMAGE_NAME	= 6,
	NF_UTIL_PUSH_ERROR_IMAGE_DATA	= 7,

	NF_UTIL_PUSH_ERROR_IMAGE_SIZE	= 8,
	NF_UTIL_PUSH_ERROR_SERVER	= 9,
	NF_UTIL_PUSH_ERROR_PORT		= 10,
	NF_UTIL_PUSH_ERROR_USERNAME	= 11,

	NF_UTIL_PUSH_ERROR_PASSWORD	= 12,
	NF_UTIL_PUSH_ERROR_SECURITY	= 13,
	NF_UTIL_PUSH_ERROR_MALLOC	= 14,
	NF_UTIL_PUSH_ERROR_MAX_QUE_LEN	= 15,

	NF_UTIL_PUSH_ERROR_NETWORK	= 17,
	NF_UTIL_PUSH_ERROR_SERVER_CONF	= 18,
	NF_UTIL_PUSH_ERROR_INIT		= 19,
	NF_UTIL_PUSH_ERROR_NR,
} NF_UTIL_PUSH_ERROR_E;



	
typedef enum _NF_UTIL_PUSH_STATUS_E {
	NF_UTIL_PUSH_STATUS_SUCCESS	= 0,
	NF_UTIL_PUSH_STATUS_FAILED	= 1,	
	NF_UTIL_PUSH_STATUS_START	= 2
} NF_UTIL_PUSH_STATUS;

// hand off
typedef void (*NF_UTIL_PUSH_CB) ( gpointer user_data, 
									NF_UTIL_PUSH_STATUS status,
									const char *svr_msg ); 

#define NF_UTIL_PUSH_MAX_QUEUE_LEN 	32

#define NF_UTIL_PUSH_MAX_SUBJECT		256
#define NF_UTIL_PUSH_MAX_CONTENTS		4096
#define NF_UTIL_PUSH_MAX_IMAGE_NAME		256

typedef struct _NF_UTIL_PUSH_CONTENT_T	{
		
	char			subject[NF_UTIL_PUSH_MAX_SUBJECT];
	char			contents[NF_UTIL_PUSH_MAX_CONTENTS];

	char			image_name[NF_UTIL_PUSH_MAX_IMAGE_NAME];
	unsigned int	image_size;
	guchar 			*image_data;
		
	NF_UTIL_PUSH_CB	cb_func;
	gpointer		cb_arg;
	
} NF_UTIL_PUSH_CONTENT;

gboolean 
nf_util_push_init(int wait);

gboolean 
nf_util_push_request(NF_UTIL_PUSH_CONTENT *cont, GError **error);

gint
nf_util_push_get_queue_length();

/*
gboolean 
nf_util_push_send_integer(int param1,  GError **error );

gboolean 
nf_util_push_send_text(char *alert_text, char *text , GError **error );
*/

#endif

