#ifndef	NF_UTIL_HTTP_H
#define	NF_UTIL_HTTP_H

#include <ne_session.h>
#include <ne_request.h>
#include <ne_socket.h> 


typedef enum _NF_HTTP_CLIENT_PRGT_E
{
	NF_HTTP_CLIENT_PRGT_INIT,
	NF_HTTP_CLIENT_PRGT_CONNECT, 
	NF_HTTP_CLIENT_PRGT_READ,	
	NF_HTTP_CLIENT_PRGT_RET_OK,
	NF_HTTP_CLIENT_PRGT_RET_FAILED,
	
} NF_HTTP_CLIENT_PRGT_E;


typedef struct _NF_HTTP_CLIENT_PRGT_T
{
	NF_HTTP_CLIENT_PRGT_E	status;
	
	gboolean	is_error;              // OK:0 //ERR:1

	guint	current; 
	guint	total;
	guint	privates[4];
	
} NF_HTTP_CLIENT_PRGT;


typedef void (*http_client_fxn_t)( const NF_HTTP_CLIENT_PRGT *prgt , gpointer context); 

typedef struct _NF_HTTP_CLIENT_REQ {
    
	gchar		url[1023+1];
	gchar		user_agent[63+1];
			
	gint		timeout_connect_sec;
	gint		timeout_rx_sec;
	gint		timeout_tx_sec;
		
	/* authentication */
	gboolean	is_auth;
	gchar		username[63+1];
	gchar		passwd[63+1];

	/* CustomX Header */
	gboolean	is_cusx;
	gchar		mac[24];
	gchar		model[64];

	/* progress cb function */	
	http_client_fxn_t	cb_func;
	gpointer			cb_arg;
	
	gboolean	is_debug;
	gpointer 	reserved[4];
	
	gchar		out_filename[1023+1];	// for nonblocking
			
} NF_HTTP_CLIENT_REQ;
 

gboolean nf_http_client_req_check(NF_HTTP_CLIENT_REQ *req);

gboolean nf_http_client_get_file( NF_HTTP_CLIENT_REQ *req, 
			const char *outfilename, 
			GError **error);

gboolean nf_http_client_get_buff( NF_HTTP_CLIENT_REQ *req, 
			char **outbuf, guint *max_len, GError **error );

/* if ( ret_msg > 0 ) must be free*/
/*
int nf_ie_push_send_integer(int a, char **ret_msg);
int nf_ie_push_send_message(char *txt, char **ret_msg);
*/
	
#endif 

