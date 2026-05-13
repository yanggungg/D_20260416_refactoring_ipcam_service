#ifndef	NF_UTIL_FTP_H
#define	NF_UTIL_FTP_H

#include <glib.h>
#include <glib-object.h>

#include "nf_object.h"

typedef struct _NF_FTP_CLIENT_REQ_T {

	char	server[255+1];
	gint	port;

	char	user[63+1];
	char	passwd[63+1];

	gint	is_pasv;
	gint	is_anon;
				
	gint	timeout_connent_sec;
	gint	timeout_rx_sec;
	gint	timeout_tx_sec;
	
	gpointer reserved[4];
					
} NF_FTP_CLIENT_REQ;

gpointer nf_ftp_client_connect( NF_FTP_CLIENT_REQ *req );

gboolean nf_ftp_client_close( gpointer handle );

gboolean nf_ftp_client_get_filesize( gpointer handle, char *remote );
gboolean nf_ftp_client_get_file( gpointer handle, char *remote, char *local, gboolean is_binary);
gboolean nf_ftp_client_put_file( gpointer handle, char *remote, char *local, gboolean is_binary);
gboolean nf_ftp_client_list( gpointer handle, char *path );
gboolean nf_ftp_client_mkdir( gpointer handle, char *path );
gboolean nf_ftp_client_rmdir( gpointer handle, char *path );
 
typedef enum _NF_FTP_CLIENT_ERROR_E
{
	NF_FTP_CLIENT_ERROR_NONE = 0,	
	NF_FTP_CLIENT_ERROR_PARAMETER = 1,
	NF_FTP_CLIENT_ERROR_CONN = 2,
	NF_FTP_CLIENT_ERROR_AUTH = 3,
	NF_FTP_CLIENT_ERROR_NR
} NF_FTP_CLIENT_ERROR_E;
 
gboolean nf_ftp_client_connect_test( NF_FTP_CLIENT_REQ *req, GError **error);


/* type macro */
#define NF_TYPE_NET_SEND					(nf_net_send_get_type ())

#define NF_IS_NET_SEND(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj),	NF_TYPE_NET_SEND))
#define NF_IS_NET_SEND_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	NF_TYPE_NET_SEND))

#define NF_NET_SEND_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	NF_TYPE_NET_SEND, NfNetSendClass))
#define NF_NET_SEND(obj)					(G_TYPE_CHECK_INSTANCE_CAST ((obj),	NF_TYPE_NET_SEND, NfNetSend))
#define NF_NET_SEND_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST ((klass),	NF_TYPE_NET_SEND, NfNetSendClass))

#define NF_NET_SEND_CAST(obj)			((NfNetSend*)(obj))
#define NF_NET_SEND_CLASS_CAST(klass)	((NfNetSendClass*)(klass))

typedef struct _NfNetSend 		NfNetSend;
typedef struct _NfNetSendClass 	NfNetSendClass;

/**
 * NfNetSend:
 *
 * NfDVR net_send class
 */
struct _NfNetSend {
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

struct _NfNetSendClass {
  NfObjectClass	parent_class;    
  
  /* signals */

  /*< public >*/
  
  /*< private >*/
    
};

typedef enum _NF_NET_SEND_SERVER_TYPE_E {
	NF_NET_SEND_SERVER_TYPE_FTP		= 0,
	NF_NET_SEND_SERVER_TYPE_HTTP	= 1,
	NF_NET_SEND_SERVER_TYPE_TCP		= 2,
	NF_NET_SEND_SERVER_TYPE_UDP		= 3,
	NF_NET_SEND_SERVER_TYPE_TFTP	= 4
	
} NF_NET_SEND_SERVER_TYPE;


typedef enum _NF_NET_SEND_ERROR_E {
	NF_NET_SEND_ERROR_SUCCESS	= 0,
	NF_NET_SEND_ERROR_FAILED	= 1,	
	NF_NET_SEND_ERROR_FROM		= 2,
	NF_NET_SEND_ERROR_TO 		= 3,

	NF_NET_SEND_ERROR_SUBJECT	= 4,
	NF_NET_SEND_ERROR_CONTENTS	= 5,
	NF_NET_SEND_ERROR_IMAGE_NAME	= 6,
	NF_NET_SEND_ERROR_IMAGE_DATA	= 7,

	NF_NET_SEND_ERROR_IMAGE_SIZE	= 8,
	NF_NET_SEND_ERROR_SERVER	= 9,
	NF_NET_SEND_ERROR_PORT		= 10,
	NF_NET_SEND_ERROR_USERNAME	= 11,

	NF_NET_SEND_ERROR_PASSWORD	= 12,
	NF_NET_SEND_ERROR_SECURITY	= 13,
	NF_NET_SEND_ERROR_MALLOC	= 14,
	NF_NET_SEND_ERROR_MAX_QUE_LEN	= 15,

	NF_NET_SEND_ERROR_NETWORK	= 17,
	NF_NET_SEND_ERROR_SERVER_CONF	= 18,
	NF_NET_SEND_ERROR_INIT		= 19,
	NF_NET_SEND_ERROR_NR,
} NF_NET_SEND_ERROR_E;


typedef enum _NF_NET_SEND_STATUS_E {
	NF_NET_SEND_STATUS_SUCCESS	= 0,
	NF_NET_SEND_STATUS_FAILED	= 1,	
	NF_NET_SEND_STATUS_START	= 2
} NF_NET_SEND_STATUS;

// hand off
typedef void (*NF_NET_SEND_CB) ( gpointer user_data, 
									NF_NET_SEND_STATUS status,
									const char *svr_msg ); 

#define NF_NET_SEND_MAX_QUEUE_LEN 		16

#define NF_NET_SEND_MAX_SERVER			64
#define NF_NET_SEND_MAX_USER			64
#define NF_NET_SEND_MAX_PASSWD			32

#define NF_NET_SEND_MAX_FROM			64
#define NF_NET_SEND_MAX_TO				64

#define NF_NET_SEND_MAX_SUBJECT			256
#define NF_NET_SEND_MAX_CONTENTS		4096
#define NF_NET_SEND_MAX_WEBRA_LINK		1024

#define NF_NET_SEND_MAX_IMAGE_NAME		256
#define NF_NET_SEND_MAX_IMAGE_SIZE		(256*1024)

#define NF_NET_SEND_MAX_DIR				64

typedef struct _NF_NET_SEND_SERVER_T {

	gint			type;	

	char			server[NF_NET_SEND_MAX_SERVER];
	int				port;
	int				security;
	char			user[NF_NET_SEND_MAX_USER];
	char			passwd[NF_NET_SEND_MAX_PASSWD];
										
} NF_NET_SEND_SERVER;

typedef struct _NF_NET_SEND_CONTENT_T {

	gint			type;	
	
	char			from[NF_NET_SEND_MAX_FROM];	
	
	char			subject[NF_NET_SEND_MAX_SUBJECT];
	char			contents[NF_NET_SEND_MAX_CONTENTS];
	
	char			webra_link[NF_NET_SEND_MAX_WEBRA_LINK];
	
	char			image_name[NF_NET_SEND_MAX_IMAGE_NAME];
	unsigned int	image_size;
	guchar 			*image_data;

	char			video_name[NF_NET_SEND_MAX_IMAGE_NAME];	
	gint			video_ch;
	guint64			video_start_time;
	guint64			video_end_time;
			
	NF_NET_SEND_CB	cb_func;
	gpointer		cb_arg;
	
	int				is_dvr_event;

	// for ftp		
	char			ftp_dir[NF_NET_SEND_MAX_DIR];
		
} NF_NET_SEND_CONTENT;

gboolean 
nf_net_send_init(int wait);

gboolean 
nf_net_send_request(NF_NET_SEND_CONTENT *cont, GError **error);

gint
nf_net_send_get_queue_length();
 
#endif 
