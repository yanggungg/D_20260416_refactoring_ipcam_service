#ifndef __NF_UTIL_MAIL_H__
#define __NF_UTIL_MAIL_H__

/* type macro */
#define NF_TYPE_MAIL_SEND					(nf_mail_send_get_type ())

#define NF_IS_MAIL_SEND(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_MAIL_SEND))
#define NF_IS_MAIL_SEND_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_MAIL_SEND))

#define NF_MAIL_SEND_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_MAIL_SEND, NfMailSendClass))
#define NF_MAIL_SEND(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_MAIL_SEND, NfMailSend))
#define NF_MAIL_SEND_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_MAIL_SEND, NfMailSendClass))

#define NF_MAIL_SEND_CAST(obj)			((NfMailSend*)(obj))
#define NF_MAIL_SEND_CLASS_CAST(klass)	((NfMailSendClass*)(klass))

typedef struct _NfMailSend 		NfMailSend;
typedef struct _NfMailSendClass 	NfMailSendClass;

/**
 * NfMailSend:
 *
 * NfDVR mail_send class
 */
struct _NfMailSend {
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

struct _NfMailSendClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

typedef enum _NF_MAIL_SEND_ERROR_E {
	NF_MAIL_SEND_ERROR_SUCCESS	= 0,
	NF_MAIL_SEND_ERROR_FAILED	= 1,	
	NF_MAIL_SEND_ERROR_FROM		= 2,
	NF_MAIL_SEND_ERROR_TO 		= 3,

	NF_MAIL_SEND_ERROR_SUBJECT	= 4,
	NF_MAIL_SEND_ERROR_CONTENTS	= 5,
	NF_MAIL_SEND_ERROR_IMAGE_NAME	= 6,
	NF_MAIL_SEND_ERROR_IMAGE_DATA	= 7,

	NF_MAIL_SEND_ERROR_IMAGE_SIZE	= 8,
	NF_MAIL_SEND_ERROR_SERVER	= 9,
	NF_MAIL_SEND_ERROR_PORT		= 10,
	NF_MAIL_SEND_ERROR_USERNAME	= 11,

	NF_MAIL_SEND_ERROR_PASSWORD	= 12,
	NF_MAIL_SEND_ERROR_SECURITY	= 13,
	NF_MAIL_SEND_ERROR_MALLOC	= 14,
	NF_MAIL_SEND_ERROR_MAX_QUE_LEN	= 15,

	NF_MAIL_SEND_ERROR_NETWORK	= 17,
	NF_MAIL_SEND_ERROR_SERVER_CONF	= 18,
	NF_MAIL_SEND_ERROR_INIT		= 19,
	NF_MAIL_SEND_ERROR_NR,
} NF_MAIL_SEND_ERROR_E;


typedef enum _NF_MAIL_SEND_STATUS_E {
	NF_MAIL_SEND_STATUS_SUCCESS	= 0,
	NF_MAIL_SEND_STATUS_FAILED	= 1,	
	NF_MAIL_SEND_STATUS_START	= 2
} NF_MAIL_SEND_STATUS;

// hand off
typedef void (*NF_MAIL_SEND_CB) ( gpointer user_data, 
									NF_MAIL_SEND_STATUS status,
									const char *svr_msg ); 

#define NF_MAIL_SEND_MAX_QUEUE_LEN 	32

#define NF_MAIL_SEND_MAX_SERVER			64
#define NF_MAIL_SEND_MAX_USER			64
#define NF_MAIL_SEND_MAX_PASSWD			32

#define NF_MAIL_SEND_MAX_FROM			64
#define NF_MAIL_SEND_MAX_TO				64

#define NF_MAIL_SEND_MAX_TO_CNT 		32

#define NF_MAIL_SEND_MAX_SUBJECT		256
#define NF_MAIL_SEND_MAX_CONTENTS		4096
#define NF_MAIL_SEND_MAX_IMAGE_NAME		256
#define NF_MAIL_SEND_MAX_ALARM_NAME_DB_KEY		64

#define NF_MAIL_SEND_MAX_IMAGE_SIZE		(256*1024)


typedef struct _NF_MAIL_SEND_SERVER_T	{

	char			smtp_server[NF_MAIL_SEND_MAX_SERVER];
	int				smtp_port;
	int				smtp_security;
	char			smtp_user[NF_MAIL_SEND_MAX_USER];
	char			smtp_passwd[NF_MAIL_SEND_MAX_PASSWD];
							
} NF_MAIL_SEND_SERVER;

typedef struct _NF_MAIL_SEND_CONTENT_T	{

	char			from[NF_MAIL_SEND_MAX_FROM];	
	int				to_cnt;
	char			to[NF_MAIL_SEND_MAX_TO_CNT][NF_MAIL_SEND_MAX_TO];
	char			subject[NF_MAIL_SEND_MAX_SUBJECT];
	char			contents[NF_MAIL_SEND_MAX_CONTENTS];

	char			image_name[NF_MAIL_SEND_MAX_IMAGE_NAME];
	unsigned int	image_size;
	guchar 			*image_data;
		
	NF_MAIL_SEND_CB	cb_func;
	gpointer		cb_arg;
	
	int				is_dvr_event;

	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		int             email_serv;
	#endif
} NF_MAIL_SEND_CONTENT;

gboolean 
nf_mail_send_init(int wait);

gboolean nf_mail_send_check_email(char *email);

gboolean 
nf_mail_send_request(NF_MAIL_SEND_CONTENT *cont, GError **error);

gboolean 
nf_mail_send_test(NF_MAIL_SEND_SERVER *server, NF_MAIL_SEND_CONTENT *content, GError **error);

gint
nf_mail_send_get_queue_length();

#endif
