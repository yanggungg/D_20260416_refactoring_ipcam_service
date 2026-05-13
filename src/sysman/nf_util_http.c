#include "nf_common.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <errno.h>
	
#define DEBUG_HTTP_CLIENT_JBSHELL

#ifdef DEBUG_HTTP_CLIENT_JBSHELL
	#include "jbshell.h"
#endif
	
#include "nf_util_http.h"
#include "nf_debug.h"

#include "unp.h"

#define HTTP_DEFAULT_PORT		80
#define HTTPS_DEFAULT_PORT		443
#define HTTP_DEFAULT_HOST		"127.0.0.1"
#define HTTP_CO_TIMEOUT			5
#define HTTP_RD_TIMEOUT			5 

/* default properties */
#define DEFAULT_LOCATION             "http://"HTTP_DEFAULT_HOST"/"
#define DEFAULT_PROXY                ""
#define DEFAULT_USER_AGENT           "nfhttpclient" 

typedef struct _HTTP_CLIENT_T {

	NF_HTTP_CLIENT_REQ	req;
			
	/* neon internal*/
	ne_session	*session;
	ne_request	*request;
  
	ne_uri		uri;
	ne_uri		proxy;	
  	
  	gchar		*location;
  	  	  	
	gchar		*auth_basic;
	
	guint64		content_size;	
	guint64		read_size;
	
	NF_HTTP_CLIENT_PRGT	prgt;
	
	char 		*content_length; 	// no require to free
	char 		*content_type;		// no require to free
			    
} HTTP_CLIENT;


static gboolean _http_client_init_sock ();
static void _http_client_close_session ( HTTP_CLIENT  *http );
static void _http_client_dispose ( HTTP_CLIENT  *http );
static gboolean _http_client_set_location ( HTTP_CLIENT *http );
static gboolean _http_client_create_session_request( HTTP_CLIENT  *http );
static int _http_client_file_reader_cb(void *userdata, const char *block, size_t length);

static gboolean _http_is_init = 0;

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "api_http"


static GQuark 
_nf_api_http_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_API_HTTP_ERROR 	_nf_api_http_error_quark()

static gboolean 
_http_client_init_sock ()
{
	if (_http_is_init) return 1;
		
//	ne_oom_callback (oom_callback);
	if (ne_sock_init () != 0)	// ssl, tls init
	{		  	
		goto init_failed;
	}	

	_http_is_init = 1;		
	return TRUE;

init_failed:
		return FALSE;
}


static void
_http_client_close_session ( HTTP_CLIENT  *http )
{ 
	g_return_if_fail( http != NULL );
	
	if (http->request) {
		ne_request_destroy (http->request);
		http->request = NULL;
	}

	if (http->session) {
		ne_close_connection (http->session);
		ne_session_destroy (http->session);
		http->session = NULL;
	}		
}


static void
_http_client_dispose ( HTTP_CLIENT  *http )
{
	g_return_if_fail( http != NULL );

	ne_uri_free (&http->proxy);
	ne_uri_free (&http->uri);
	
	if (http->location) {
    	ne_free (http->location);
    	http->location = NULL;
	}

	if (http->auth_basic) {
    	ne_free (http->auth_basic);
    	http->auth_basic = NULL;
	}
		
	_http_client_close_session ( http );

	if( http->req.is_debug )
		ne_debug_init (NULL, 0); 			
}


static gboolean
_http_client_set_location ( HTTP_CLIENT *http )
{	
	ne_uri_free (&http->uri);
	if (http->location) {
		ne_free (http->location);
		http->location = NULL;
	}
	
	if (ne_uri_parse ( http->req.url , &http->uri) != 0)
		goto parse_error;

	if (http->uri.scheme == NULL)
		http->uri.scheme = g_strdup ("http");

	if (http->uri.host == NULL)
		http->uri.host = g_strdup (DEFAULT_LOCATION);

	if (http->uri.port == 0) {
		if (!strcmp (http->uri.scheme, "https"))
			http->uri.port = HTTPS_DEFAULT_PORT;
		else
			http->uri.port = HTTP_DEFAULT_PORT;
	}

	if (!http->uri.path)
		http->uri.path = g_strdup ("");

	http->location = ne_uri_unparse (&http->uri);

	return TRUE;

	/* ERRORS */
parse_error:
	{
		if (http->location) {
			ne_free (http->location);
			http->location = NULL;
		}
		ne_uri_free (&http->uri);
		return 0;
	}
}

static void _http_client_cb_call( HTTP_CLIENT  *http, 
			NF_HTTP_CLIENT_PRGT_E	status, 
			gboolean	is_error,
			guint		current,
			guint		total)
{
	g_return_if_fail ( http != NULL );
	
	http->prgt.status = status;
	http->prgt.is_error = is_error;
	http->prgt.current = current;
	http->prgt.total = total;
	
	if(http->req.cb_func)
		http->req.cb_func( &http->prgt, http->req.cb_arg);
	
	return;			
}


static gboolean
_http_client_create_session_request( HTTP_CLIENT  *http )
{		
	gchar req_str[4096];
	gchar *tmp;

	_http_client_init_sock();
	
	if (http->req.is_debug)
		ne_debug_init (stderr, NE_DBG_HTTP);

	if( http->session || http->request )
	{
		g_warning("%s session or request is not null", __FUNCTION__);
		//_http_client_close_session( http );
		return NE_ERROR;
	}
 	 	
	http->session = ne_session_create (http->uri.scheme, 
										http->uri.host, 
										http->uri.port);
					
	g_return_val_if_fail (http->session != NULL, 0);
		
	// set timeout
/* 	SNF_MODEL_XXX
	ne_set_connect_timeout( http->session, http->req.timeout_connect_sec);
	*/
	ne_set_read_timeout( http->session, http->req.timeout_rx_sec);	

/* 	SNF_MODEL_XXX
	if(	http->uri.query )
		snprintf(req_str, sizeof(req_str)-1,"%s?%s", 
						http->uri.path, http->uri.query);
	else */ 
		snprintf(req_str, sizeof(req_str)-1,"%s", http->uri.path);
		
	
	http->request = ne_request_create (http->session, "GET", req_str);    
    if( !http->request )
    {   
    	g_warning("%s ne_request_create", __FUNCTION__);
    	//_http_client_close_session( http ); 
    	return 0;
    }
	
	if ( http->req.user_agent[0] ) {
		ne_add_request_header (http->request, "User-Agent", 
									http->req.user_agent);
	}else{	// default agent
		ne_add_request_header (http->request, "User-Agent", DEFAULT_USER_AGENT);
	}
/* SNF_MODEL_XXX
	if( http->uri.userinfo && http->req.is_auth == 0 )
	{
		char tmp[1024];
		char *user_pos = tmp, *gb_pos = NULL, *pass_pos = NULL;
		
		strncpy( tmp, http->uri.userinfo, sizeof(tmp) );
		
		gb_pos = strchr( tmp, ':' );		
		if( gb_pos ) 
		{
			pass_pos = gb_pos + 1;
			*gb_pos = '\0';
		}
		
		strncpy( http->req.username, user_pos, sizeof(http->req.username) );

		if( pass_pos ) 
			strncpy( http->req.passwd, pass_pos, sizeof(http->req.passwd) );
		
		http->req.is_auth = 1;
	}
*/
	if(http->req.is_cusx)
	{
		tmp = ne_concat(http->req.mac, ";", http->req.model, NULL);
		g_return_val_if_fail ( tmp , 0);

		ne_add_request_header (http->request, "CustomX", tmp);
		g_free(tmp);
	}

    if (http->req.is_auth ) {

		if( http->auth_basic == NULL )
		{
		   	tmp = ne_concat(http->req.username, ":", http->req.passwd, NULL);	    
		   	g_return_val_if_fail ( tmp , 0);
		   	
		   	http->auth_basic = ne_base64((unsigned char *)tmp, strlen(tmp));
		   	g_free(tmp);
		}
		    	    	    	
		tmp = ne_concat("Basic ", http->auth_basic, "\r\n", NULL); 		
		g_return_val_if_fail ( tmp , 0);
		
		ne_add_request_header (http->request, "Authorization", tmp);
		g_free(tmp);
		
	}
		 			
	return 1;
}

gboolean nf_http_client_req_check(NF_HTTP_CLIENT_REQ *req)
{	
	g_return_val_if_fail( req != NULL , 0);			

#if 0
	g_message("req->url        [%s]",req->url );
	g_message("req->user_agent [%s]",req->user_agent );
	g_message("req->username   [%s]",req->username );
	g_message("req->passwd     [%s]",req->passwd  );
	g_message("req->to_conn    [%d]",req->timeout_connect_sec );
	g_message("req->to_rx      [%d]",req->timeout_rx_sec );
	g_message("req->to_tx      [%d]",req->timeout_tx_sec );

#endif
	g_return_val_if_fail( req->url[0] != '\0' , 0);
	
	g_return_val_if_fail( req->timeout_connect_sec >0 && req->timeout_connect_sec <= 180, 0);
	g_return_val_if_fail( req->timeout_rx_sec >0 && req->timeout_rx_sec <= 180, 0);
	g_return_val_if_fail( req->timeout_tx_sec >0 && req->timeout_tx_sec <= 180, 0);
	
	if(req->is_auth)
	{
		g_return_val_if_fail( req->username[0] != '\0' , 0);		
		//g_return_val_if_fail( req->passwd[0] != '\0' , 0); 
	}	
	
	return 1;
}

char *_neon_res_to_str( gint res)
{
	char *str = NULL;
	switch(res)
	{
		case NE_OK 			:str = "NE_OK";        break; // Success"; break;
		case NE_ERROR 		:str = "NE_ERROR";     break; // Generic error; use ne_get_error(session) for message"; break;
		case NE_LOOKUP 		:str = "NE_LOOKUP";    break; // Server or proxy hostname lookup failed"; break;
		case NE_AUTH 		:str = "NE_AUTH";      break; // User authentication failed on server";  break;
		case NE_PROXYAUTH 	:str = "NE_PROXYAUTH"; break; // User authentication failed on proxy"; break;
		case NE_CONNECT		:str = "NE_CONNECT";   break; // Could not connect to server"; break;
		case NE_TIMEOUT		:str = "NE_TIMEOUT";   break; // Connection timed out"; break;
		case NE_FAILED		:str = "NE_FAILED";    break; // The precondition failed"; break;
		case NE_RETRY		:str = "NE_RETRY";     break; // Retry request (ne_end_request ONLY)"; break;
		case NE_REDIRECT	:str = "NE_REDIRECT";  break; // See ne_redirect.h"; break;
 		default : str = "NE_UNKNOWN"; break;
	}
	return str;
}


int _body_reader_cb(void *userdata, const char *block, size_t length)
{
	HTTP_CLIENT	*http = (HTTP_CLIENT	*)userdata;
	
	http->read_size += length;
		
	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 
							(guint32)http->read_size, 
							(guint32)http->content_size);
								
	return NE_OK;
}


gboolean 
nf_http_client_get_file( NF_HTTP_CLIENT_REQ *req, const char *outfilename, 
							GError **error)
{
	HTTP_CLIENT	_h;
	HTTP_CLIENT	*http = &_h;
	
	gboolean ret;
	gint res=0, fd=0;
	ne_status *http_res = NULL;	
	
	g_return_val_if_fail ( nf_http_client_req_check(req), 0 );
	
	memset( http, 0x00, sizeof(HTTP_CLIENT));		
	memcpy( &http->req, req, sizeof(NF_HTTP_CLIENT_REQ));
	
	if( !_http_client_set_location( http ) )
	{
		g_warning("%s set_location failed url[%s]", __FUNCTION__, http->req.url);
		goto req_failed;
	}
	
	if( !_http_client_create_session_request( http) )
	{
		g_warning("%s create_session_request failed", __FUNCTION__);
		goto req_failed;
	}

 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_INIT, FALSE, 0, 0);

	ne_add_response_body_reader(http->request, ne_accept_always, _body_reader_cb, http);
	
    res = ne_begin_request (http->request);	
	if( res != NE_OK )
	{		
	 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, TRUE, 0, 0);

		g_warning("%s begin_request failed res[%d](%s)", __FUNCTION__, res, _neon_res_to_str(res));
		g_set_error( error, NF_API_HTTP_ERROR, res, _neon_res_to_str(res) );
		goto req_failed;
	}
	
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, FALSE, 0, 0);
		
	http_res =  ne_get_status(http->request);	
	if(http_res->code != 200)
	{
		//http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
		
		g_warning("%s http server error[%d](%s)", __FUNCTION__, http_res->code, http_res->reason_phrase );
		g_set_error( error, NF_API_HTTP_ERROR, http_res->code, http_res->reason_phrase );
		goto req_failed;		
	}

	http->content_length = ne_get_response_header (http->request, "Content-Length");
	if (http->content_length)
		http->content_size = g_ascii_strtoull (http->content_length, NULL, 10);
	http->content_type = ne_get_response_header(http->request, "Content-Type");		
		
	if( http->req.is_debug)
		g_message("%s content length[%s] type[%s]", __FUNCTION__, 
					http->content_length, http->content_type);

	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 0, 
							(guint32)http->content_size);
			
	fd = open( outfilename , O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);	
	
	if(fd<0)	
	{
		g_warning("%s file open error[%d](%s)", __FUNCTION__, errno, strerror(errno)  );
		g_set_error( error, NF_API_HTTP_ERROR, errno, strerror(errno) );
		goto req_failed;		
	}
	
	res = ne_read_response_to_fd(http->request, fd); 
	
	close(fd);
	if(res !=0 )
	{
		g_warning("%s ne_read_response_to_fd error[%d](%s)", __FUNCTION__, res, _neon_res_to_str(res) );
		g_set_error( error, NF_API_HTTP_ERROR, EIO, strerror(EIO) );		
		goto req_failed;
	}								

	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 
							(guint32)http->content_size, 
							(guint32)http->content_size);
	
	ne_end_request(http->request);
	
	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_OK, FALSE, 0, 0);
	
	return 1;

req_failed:			
	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_FAILED, TRUE, 0, 0);
	
	return 0;
		
}

gboolean
nf_http_client_get_buff_chunked( NF_HTTP_CLIENT_REQ *req,
							char **out_buf, guint *out_len,
							GError **error )
{

	HTTP_CLIENT	_h;
	HTTP_CLIENT	*http = &_h;
	char *out = NULL;
	gboolean ret;
	gint res=0, read_len = 0, remain_len = 0;
	ne_status *http_res = NULL;

	g_return_val_if_fail ( out_buf != NULL && out_len != NULL, 0);
	g_return_val_if_fail ( *out_buf == NULL, 0);
	g_return_val_if_fail ( nf_http_client_req_check(req), 0 );

	memset( http, 0x00, sizeof(HTTP_CLIENT));
	memcpy( &http->req, req, sizeof(NF_HTTP_CLIENT_REQ));

	if( !_http_client_set_location( http ) )
	{
		g_warning("%s set_location failed url[%s]", __FUNCTION__, http->req.url);
		goto req_failed;
	}

	if( !_http_client_create_session_request( http) )
	{
		g_warning("%s create_session_request failed", __FUNCTION__);
		goto req_failed;
	}

 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_INIT, FALSE, 0, 0);

	ne_add_response_body_reader(http->request, ne_accept_always, _body_reader_cb, http);

    res = ne_begin_request (http->request);
	if( res != NE_OK )
	{
	 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, TRUE, 0, 0);

		g_warning("%s begin_request failed res[%d](%s)", __FUNCTION__, res, _neon_res_to_str(res));
		g_set_error( error, NF_API_HTTP_ERROR, res, _neon_res_to_str(res) );
		goto req_failed;
	}

 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, FALSE, 0, 0);

	http_res =  ne_get_status(http->request);
	if(http_res->code != 200)
	{
		//http://en.wikipedia.org/wiki/List_of_HTTP_status_codes

		g_warning("%s http server error[%d](%s)", __FUNCTION__, http_res->code, http_res->reason_phrase );
		g_set_error( error, NF_API_HTTP_ERROR, http_res->code, http_res->reason_phrase );
		goto req_failed;
	}

	http->content_length = ne_get_response_header (http->request, "Content-Length");
	if (http->content_length)
		http->content_size = g_ascii_strtoull (http->content_length, NULL, 10);
	http->content_type = ne_get_response_header(http->request, "Content-Type");

	if( http->req.is_debug)
		g_message("%s content length[%s] type[%s]", __FUNCTION__,
					http->content_length, http->content_type);

	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 0,
							(guint32)http->content_size);


	http->content_size =*out_len;
	if( http->content_size == 0 || http->content_size >1048576)
	{
		g_warning("%s wrong content_size[%d]", __FUNCTION__, http->content_size );
		g_set_error( error, NF_API_HTTP_ERROR, EMSGSIZE, strerror(EMSGSIZE) );
		goto req_failed;
	}

	out = g_malloc( http->content_size + 1 );
	if( !out )
	{
		g_warning("%s nomem size[%d]", __FUNCTION__, http->content_size );
		g_set_error( error, NF_API_HTTP_ERROR, ENOMEM, strerror(ENOMEM) );
		goto req_failed;
	}else{
		out[http->content_size] = 0;
	}

	remain_len = http->content_size;
	while( remain_len >0)
	{
		res = ne_read_response_block(http->request, out + read_len, remain_len );
		if(res == 0)
			break;
		else {
			remain_len -= res;
			read_len += res;
		}
	}
/*
	if( remain_len != 0 )
	{
		g_warning("%s ne_read_response_block error[%d]", __FUNCTION__, res );
		g_set_error( error, NF_API_HTTP_ERROR, EIO, strerror(EIO) );
		goto req_failed;
	}
*/
	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE,
							(guint32)http->content_size,
							(guint32)http->content_size);

	ne_end_request(http->request);

	*out_buf = out;
	*out_len = http->content_size;

	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_OK, FALSE, 0, 0);

	return 1;

req_failed:
	*out_buf = out;
	*out_len = http->content_size;
	if(out)
		g_free(out);

	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_FAILED, TRUE, 0, 0);

	return 0;

}
gboolean
nf_http_client_get_buff( NF_HTTP_CLIENT_REQ *req, 
							char **out_buf, guint *out_len,
							GError **error )
{

	HTTP_CLIENT	_h;
	HTTP_CLIENT	*http = &_h;
	char *out = NULL;	
	gboolean ret;
	gint res=0, read_len = 0, remain_len = 0;
	ne_status *http_res = NULL;	
	
	g_return_val_if_fail ( out_buf != NULL && out_len != NULL, 0);	
	g_return_val_if_fail ( *out_buf == NULL, 0);		
	g_return_val_if_fail ( nf_http_client_req_check(req), 0 );
	
	memset( http, 0x00, sizeof(HTTP_CLIENT));		
	memcpy( &http->req, req, sizeof(NF_HTTP_CLIENT_REQ));
	
	if( !_http_client_set_location( http ) )
	{
		g_warning("%s set_location failed url[%s]", __FUNCTION__, http->req.url);
		goto req_failed;
	}
	
	if( !_http_client_create_session_request( http) )
	{
		g_warning("%s create_session_request failed", __FUNCTION__);
		goto req_failed;
	}

 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_INIT, FALSE, 0, 0);

	ne_add_response_body_reader(http->request, ne_accept_always, _body_reader_cb, http);
	
    res = ne_begin_request (http->request);	
	if( res != NE_OK )
	{		
	 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, TRUE, 0, 0);

		g_warning("%s begin_request failed res[%d](%s)", __FUNCTION__, res, _neon_res_to_str(res));
		g_set_error( error, NF_API_HTTP_ERROR, res, _neon_res_to_str(res) );
		goto req_failed;
	}
	
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, FALSE, 0, 0);
		
	http_res =  ne_get_status(http->request);	
	if(http_res->code != 200)
	{
		//http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
		
		g_warning("%s http server error[%d](%s)", __FUNCTION__, http_res->code, http_res->reason_phrase );
		g_set_error( error, NF_API_HTTP_ERROR, http_res->code, http_res->reason_phrase );
		goto req_failed;		
	}

	http->content_length = ne_get_response_header (http->request, "Content-Length");
	if (http->content_length)
		http->content_size = g_ascii_strtoull (http->content_length, NULL, 10);
	http->content_type = ne_get_response_header(http->request, "Content-Type");		
		
	if( http->req.is_debug)
		g_message("%s content length[%s] type[%s]", __FUNCTION__, 
					http->content_length, http->content_type);

	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 0, 
							(guint32)http->content_size);

	if( http->content_size == 0 || http->content_size >1048576)
	{
		g_warning("%s wrong content_size[%d]", __FUNCTION__, http->content_size );
		g_set_error( error, NF_API_HTTP_ERROR, EMSGSIZE, strerror(EMSGSIZE) );		
		goto req_failed;		
	}
			
	out = g_malloc( http->content_size + 1 );
	if( !out )
	{
		g_warning("%s nomem size[%d]", __FUNCTION__, http->content_size );
		g_set_error( error, NF_API_HTTP_ERROR, ENOMEM, strerror(ENOMEM) );		
		goto req_failed;		
	}else{
		out[http->content_size] = 0;
	}
	
	remain_len = http->content_size;	
	while( remain_len >0)
	{		
		res = ne_read_response_block(http->request, out + read_len, remain_len );
		if(res == 0)
			break;
		else {
			remain_len -= res;
			read_len += res;
		}
	}

	if( remain_len != 0 )
	{
		g_warning("%s ne_read_response_block error[%d]", __FUNCTION__, res );
		g_set_error( error, NF_API_HTTP_ERROR, EIO, strerror(EIO) );				
		goto req_failed;
	}			
	
	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 
							(guint32)http->content_size, 
							(guint32)http->content_size);
	
	ne_end_request(http->request);		
	
	*out_buf = out;
	*out_len = http->content_size;
	
	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_OK, FALSE, 0, 0);
	
	return 1;

req_failed:			
	if(out)
		g_free(out);
		
	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_FAILED, TRUE, 0, 0);
	
	return 0;
	
}

// ITX Sequrinet PUSH
static gboolean
_http_client_create_session_request_post( HTTP_CLIENT  *http )
{		
	gchar req_str[4096];
	gchar *tmp;

	_http_client_init_sock();
	
	if (http->req.is_debug)
		ne_debug_init (stderr, NE_DBG_HTTP);

	if( http->session || http->request )
	{
		g_warning("%s session or request is not null", __FUNCTION__);
		//_http_client_close_session( http );
		return NE_ERROR;
	}
 	 	
	http->session = ne_session_create (http->uri.scheme, 
										http->uri.host, 
										http->uri.port);
					
	g_return_val_if_fail (http->session != NULL, 0);
		
	// set timeout
/* 	SNF_MODEL_XXX
	ne_set_connect_timeout( http->session, http->req.timeout_connect_sec);
	*/
	ne_set_read_timeout( http->session, http->req.timeout_rx_sec);	

/* 	SNF_MODEL_XXX
	if(	http->uri.query )
		snprintf(req_str, sizeof(req_str)-1,"%s?%s", 
						http->uri.path, http->uri.query);
	else */ 
		snprintf(req_str, sizeof(req_str)-1,"%s", http->uri.path);
		
	
	http->request = ne_request_create (http->session, "POST", req_str);    
    if( !http->request )
    {   
    	g_warning("%s ne_request_create", __FUNCTION__);
    	//_http_client_close_session( http ); 
    	return 0;
    }
	
	if ( http->req.user_agent[0] ) {
		ne_add_request_header (http->request, "User-Agent", 
									http->req.user_agent);
	}else{	// default agent
		ne_add_request_header (http->request, "User-Agent", DEFAULT_USER_AGENT);
	}
/* SNF_MODEL_XXX
	if( http->uri.userinfo && http->req.is_auth == 0 )
	{
		char tmp[1024];
		char *user_pos = tmp, *gb_pos = NULL, *pass_pos = NULL;
		
		strncpy( tmp, http->uri.userinfo, sizeof(tmp) );
		
		gb_pos = strchr( tmp, ':' );		
		if( gb_pos ) 
		{
			pass_pos = gb_pos + 1;
			*gb_pos = '\0';
		}
		
		strncpy( http->req.username, user_pos, sizeof(http->req.username) );

		if( pass_pos ) 
			strncpy( http->req.passwd, pass_pos, sizeof(http->req.passwd) );
		
		http->req.is_auth = 1;
	}
*/    	    
    if (http->req.is_auth ) {

		if( http->auth_basic == NULL )
		{
		   	tmp = ne_concat(http->req.username, ":", http->req.passwd, NULL);	    
		   	g_return_val_if_fail ( tmp , 0);
		   	
		   	http->auth_basic = ne_base64((unsigned char *)tmp, strlen(tmp));
		   	g_free(tmp);
		}
		    	    	    	
		tmp = ne_concat("Basic ", http->auth_basic, "\r\n", NULL); 		
		g_return_val_if_fail ( tmp , 0);
		
		ne_add_request_header (http->request, "Authorization", tmp);
		g_free(tmp);
		
	}

	ne_add_request_header (http->request, "Accept", "application/json");
	ne_add_request_header (http->request, "Content-Type", "application/json");
		 			
	return 1;
}

// ITX Sequrinet PUSH
gint
nf_http_client_post_buff( NF_HTTP_CLIENT_REQ *req, 
							char *in_buf, guint in_len,
							char **out_buf, guint *out_len,
							GError **error )
{

	HTTP_CLIENT	_h;
	HTTP_CLIENT	*http = &_h;
	char *out = NULL;	
	gboolean ret;
	gint res=0, read_len = 0, remain_len = 0;
	ne_status *http_res = NULL;	
	
	g_return_val_if_fail ( out_buf != NULL && out_len != NULL, 0);	
	g_return_val_if_fail ( *out_buf == NULL, 0);		
	g_return_val_if_fail ( nf_http_client_req_check(req), 0 );
	
	memset( http, 0x00, sizeof(HTTP_CLIENT));		
	memcpy( &http->req, req, sizeof(NF_HTTP_CLIENT_REQ));
	
	if( !_http_client_set_location( http ) )
	{
		g_warning("%s set_location failed url[%s]", __FUNCTION__, http->req.url);
		goto req_failed;
	}
	
	if( !_http_client_create_session_request_post( http) )
	{
		g_warning("%s create_session_request failed", __FUNCTION__);
		goto req_failed;
	}

	ne_set_request_body_buffer( http->request, in_buf, in_len);

 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_INIT, FALSE, 0, 0);

	ne_add_response_body_reader(http->request, ne_accept_always, _body_reader_cb, http);
	
    res = ne_begin_request (http->request);	
	if( res != NE_OK )
	{		
	 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, TRUE, 0, 0);

		g_warning("%s begin_request failed res[%d](%s)", __FUNCTION__, res, _neon_res_to_str(res));
		g_set_error( error, NF_API_HTTP_ERROR, res, _neon_res_to_str(res) );
		goto req_failed;
	}
	
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_CONNECT, FALSE, 0, 0);
		
	http_res =  ne_get_status(http->request);	
	if(http_res->code != 200 &&  http_res->code != 400 && http_res->code != 406 && http_res->code != 500)
	{
		//http://en.wikipedia.org/wiki/List_of_HTTP_status_codes
		
		g_warning("%s http server error[%d](%s)", __FUNCTION__, http_res->code, http_res->reason_phrase );
		g_set_error( error, NF_API_HTTP_ERROR, http_res->code, http_res->reason_phrase );
		goto req_failed;		
	}

	http->content_length = ne_get_response_header (http->request, "Content-Length");
	if (http->content_length)
		http->content_size = g_ascii_strtoull (http->content_length, NULL, 10);
	http->content_type = ne_get_response_header(http->request, "Content-Type");		
		
	if( http->req.is_debug)
		g_message("%s content length[%s] type[%s]", __FUNCTION__, 
					http->content_length, http->content_type);

	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 0, 
							(guint32)http->content_size);

	if( http->content_size == 0 || http->content_size >1048576)
	{
		g_warning("%s wrong content_size[%d]", __FUNCTION__, http->content_size );
		g_set_error( error, NF_API_HTTP_ERROR, EMSGSIZE, strerror(EMSGSIZE) );		
		goto req_failed;		
	}
			
	out = g_malloc( http->content_size + 1 );
	if( !out )
	{
		g_warning("%s nomem size[%d]", __FUNCTION__, http->content_size );
		g_set_error( error, NF_API_HTTP_ERROR, ENOMEM, strerror(ENOMEM) );		
		goto req_failed;		
	}else{
		out[http->content_size] = 0;
	}
	
	remain_len = http->content_size;	
	while( remain_len >0)
	{		
		res = ne_read_response_block(http->request, out + read_len, remain_len );
		if(res == 0)
			break;
		else {
			remain_len -= res;
			read_len += res;
		}
	}

	if( remain_len != 0 )
	{
		g_warning("%s ne_read_response_block error[%d]", __FUNCTION__, res );
		g_set_error( error, NF_API_HTTP_ERROR, EIO, strerror(EIO) );				
		goto req_failed;
	}			
	
	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_READ, FALSE, 
							(guint32)http->content_size, 
							(guint32)http->content_size);
	
	ne_end_request(http->request);		
	
	*out_buf = out;
	*out_len = http->content_size;
	
	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_OK, FALSE, 0, 0);
	
	return http_res->code;

req_failed:			
	if(out)
		g_free(out);
		
	_http_client_dispose( http);
 	_http_client_cb_call( http, NF_HTTP_CLIENT_PRGT_RET_FAILED, TRUE, 0, 0);
	
	return 0;
	
}

#ifdef DEBUG_HTTP_CLIENT_JBSHELL

#if 0

http_get http://192.168.100.10/phpBB_20070221.tar.gz /tmp/kkk 1
http_get http://update.websamsung.net/DVR/detroit/None/None/None/UpgradeHistory.info /tmp/UpgradeHistory.info 1
http_get http://update.websamsung.net/DVR/detroit/None/None/None/shr-detroit-pkg_v1.08_090814212744.img /tmp/shr-detroit-pkg_v1.08_090814212744.img 1
	
http_get http://192.168.100.238/cgi-bin/admin/getparam.cgi param.txt
http_get http://root:1234@192.168.100.163/cgi-bin/admin/setparam.cgi?video.keyinterval=1 /tmp/dummy.txt
http_get http://root:1234@192.168.100.163/cgi-bin/admin/setparam.cgi?video.maxframe=3&video.keyinterval=1 /tmp/dummy.txt
http_get http://192.168.100.238/cgi-bin/viewer/video.jpg?resolution=352x240 ipcam.jpg
	
#endif	

static void _http_get_cb( const NF_HTTP_CLIENT_PRGT *prgt , gpointer context)
{
	NF_HTTP_CLIENT_REQ  *req = (NF_HTTP_CLIENT_REQ *)context;

	g_message("%s status[%d] prgt[%d]/[%d] error[%d]",__FUNCTION__, 
				prgt->status, prgt->current, prgt->total, prgt->is_error);
}

static char http_get_help[] = "http_get [remote_url] [savefile] [debug:0]";
static int jbshell_http_get(int argc, char **argv)
{	
	NF_HTTP_CLIENT_REQ req;		
	char *remote, *local;	
	int debug = 0;

	if(argc < 3){
		printf("%s\n",http_get_help);
		return -1;
	}
	
	remote = argv[1];
	local = argv[2];
	
	if( argc >3) 
		debug = atoi(argv[3]);
	
	memset(&req, 0x00, sizeof(NF_HTTP_CLIENT_REQ));

	strncpy( req.url, remote, sizeof(req.url)-1);

	req.is_debug = debug;
	req.timeout_connect_sec = 3;
	req.timeout_rx_sec = req.timeout_tx_sec = 7;

	req.cb_func = _http_get_cb;
	req.cb_arg = &req;
	
	nf_http_client_get_file( &req, local, NULL);
		
	return 0;
}
__commandlist(jbshell_http_get,"http_get", http_get_help, http_get_help);


static char http_getbuff_help[] = "http_getbuff [remote_url] [ptr_len:128] [debug:0]";
static int jbshell_http_getbuff(int argc, char **argv)
{	
	NF_HTTP_CLIENT_REQ req;		
	char *remote, *buff = NULL;
	guint buff_len;
	int ret, debug = 0, ptr_len = 128;
	GError	*error = NULL;	

	if(argc < 2){
		printf("%s\n",http_getbuff_help);
		return -1;
	}
	
	remote = argv[1];
	
	if( argc >2) ptr_len = atoi(argv[2]);	
	if( argc >3) debug = atoi(argv[3]);
	
	memset(&req, 0x00, sizeof(NF_HTTP_CLIENT_REQ));

	strncpy( req.url, remote, sizeof(req.url)-1);

	req.is_debug = debug;
	req.timeout_connect_sec = 3;
	req.timeout_rx_sec = req.timeout_tx_sec = 7;

	req.cb_func = _http_get_cb;
	req.cb_arg = &req;
	
	ret = nf_http_client_get_buff( &req, &buff, &buff_len, NULL );		
	g_print("nf_http_client_get_buff ret[%d] buff[0x%08x] len[%d]\n", 
			ret, buff, buff_len);	
	if(buff)
	{
		nf_debug_hexdump(buff, (buff_len > ptr_len) ? ptr_len : buff_len);			
		g_free(buff);
	}
				
	return 0;
}
__commandlist(jbshell_http_getbuff,"http_getbuff", http_getbuff_help, http_getbuff_help);

#endif

