#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <openssl/ssl.h>
#include <auth-client.h>
#include <libesmtp.h>

#include <sys/ioctl.h>
#include <linux/sockios.h>

#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_util_mail.h"
#include "nf_debug.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_mail"

#define ENABLE_MAIL_SEND_EVENTLOG
#define ENABLE_MAIL_SEND_CB_FUNC

#define DEBUG_MAIL_SEND_LOG
#define	DEBUG_MAIL_SEND_JBSHELL
//#define DEBUG_MAIL_SEND_MONITOR_CB
//#define DEBUG_DIRECT_MAIL_SEND

static GQuark 
_nf_mail_send_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_MAIL_SEND_ERROR 	_nf_mail_send_error_quark()

#ifdef DEBUG_MAIL_SEND_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_MAIL_SEND_IDX_E
{ 
	DEBUG_MAIL_SEND_IDX_INIT				= 0,
	DEBUG_MAIL_SEND_IDX_LIBESMTP			= 1,
	DEBUG_MAIL_SEND_IDX_QUEUE				= 2,
	DEBUG_MAIL_SEND_IDX_REQUEST				= 3,

	DEBUG_MAIL_SEND_IDX_TEST				= 4,	
	DEBUG_MAIL_SEND_IDX_REQUEST_RAW			= 5,	
	DEBUG_MAIL_SEND_IDX_NR					= 6	
}DEBUG_MAIL_SEND_IDX_E;

static const char *_DEBUG_MAIL_SEND_str[32] =
{
	"MAIL_SEND_IDX_INIT",
	"MAIL_SEND_IDX_LIBESMTP",
	"MAIL_SEND_IDX_QUEUE",	
	"MAIL_SEND_IDX_REQUEST",
	
	"MAIL_SEND_IDX_TEST",		
	"DEBUG_MAIL_SEND_IDX_REQUEST_RAW",		
	"MAIL_SEND_IDX_NR"
};

static gint _DEBUG_MAIL_SEND_log[32] = 
{
	1,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 
};


typedef struct _NF_MAIL_SEND_INTERNAL_T	{
#if 0
	GTimeVal	enque_tv;	
	GTimeVal	process_tv;
	gint		retry_cnt;
#endif			
	NF_MAIL_SEND_SERVER		server;
	NF_MAIL_SEND_CONTENT	content;
					
} NF_MAIL_SEND_INTERNAL;

/* Object signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	PROP_0,		
	LAST_PROP	
	/* FILL ME */
};

static void nf_mail_send_class_init (NfMailSendClass * klass);
static void nf_mail_send_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_mail_send_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_mail_send_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_mail_send_dispose (GObject * object);
static void nf_mail_send_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfMailSend	*_nf_mail_send = NULL;

static void mail_send_thread_func (NfMailSend * self);
static int _mail_send_request(NF_MAIL_SEND_INTERNAL* info);


#if defined(ENABLE_EMAIL_DUAL_SERVER)
	static gboolean _sysdb_get_smtp_info( NF_MAIL_SEND_SERVER *svr, gint svr_no );
#else
	static gboolean _sysdb_get_smtp_info( NF_MAIL_SEND_SERVER *svr );
#endif
static void  	_free_minfo( NF_MAIL_SEND_INTERNAL *minfo);
static void  	_dump_minfo( NF_MAIL_SEND_INTERNAL *minfo);
static int _check_param_content(NF_MAIL_SEND_CONTENT *cont);
static int _check_param_server(NF_MAIL_SEND_SERVER *svr);

static NF_MAIL_SEND_INTERNAL *_make_minfo( NF_MAIL_SEND_SERVER *svr, NF_MAIL_SEND_CONTENT *cont ); 

GType
nf_mail_send_get_type (void)
{
	static GType nf_mail_send_type = 0;

	if (G_UNLIKELY (nf_mail_send_type == 0)) {		
		static const GTypeInfo object_info = {
			sizeof (NfMailSendClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_mail_send_class_init,
			NULL,
			NULL,
			sizeof (NfMailSend),
			0,
			(GInstanceInitFunc) nf_mail_send_instance_init,
			NULL
		};

		nf_mail_send_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfMailSend", &object_info, 0);
	}
	
	return nf_mail_send_type;
}

static void
nf_mail_send_class_init (NfMailSendClass * klass)
{	
	GObjectClass *gobject_class;
	int i;
		
	gobject_class = G_OBJECT_CLASS (klass);
		
	parent_class = g_type_class_peek_parent (klass);
	
	gobject_class->set_property = nf_mail_send_set_property;
	gobject_class->get_property = nf_mail_send_get_property;
			
	gobject_class->dispose = nf_mail_send_dispose;
	gobject_class->finalize = nf_mail_send_finalize;

}

#define NUM_SEND_THREAD_COUNT 4

static void
nf_mail_send_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfMailSend *self = NF_MAIL_SEND (instance);
	int i=0;
				
	self->init_done	= 0;
	
	// queue ����
	self->queue = g_async_queue_new();
 		 
	// notification signal emit�� thread ����
	self->thread_run = 1;

	auth_client_init();
	
	for(i=0;i<NUM_SEND_THREAD_COUNT ;++i)
		self->thread = g_thread_create(	(GThreadFunc)mail_send_thread_func, 
									self, FALSE, NULL);
																		
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_mail_send_dispose (GObject * object)
{		
	// thread end
	parent_class->dispose (object);  
}

/* finalize is called when the object has to free its resources */
static void
nf_mail_send_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_mail_send_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
 	NfObject *nfobject;

	nfobject = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nf_mail_send_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
	NfObject *self;

	self = NF_OBJECT (object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
  }
}

GStaticMutex _mail_send_mutex = G_STATIC_MUTEX_INIT;
//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	

gint nf_get_nf_mail_send_value() //for Booting Event Timing 
{
	gint retry = 1;
	
	if(_nf_mail_send == NULL)
		return retry;
	else
		return 0;
}

static void
mail_send_thread_func (NfMailSend * self)
{
	gpointer	que_poped_data = NULL;	
	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_mail_send == NULL ) g_usleep(10*1000);
		
	self->init_done = 1;
	
	while(self->thread_run)
	{
		NF_MAIL_SEND_INTERNAL *minfo  = g_async_queue_pop( self->queue);
		if(minfo)
		{

#ifdef DEBUG_MAIL_SEND_LOG
			if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_QUEUE] )
				_dump_minfo(minfo);
#endif
			g_static_mutex_lock(&_mail_send_mutex);
			_mail_send_request(minfo);
			_free_minfo(minfo);
			g_static_mutex_unlock(&_mail_send_mutex);
			
			g_usleep(10*1000);			
		}						
	}
	g_message("%s end", __FUNCTION__);
}


/**
	@brief				mail_send �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred					
*/
gboolean 
nf_mail_send_init( int wait )
{
	gboolean ret = TRUE;
	gint r = 0;
		
	g_return_val_if_fail (_nf_mail_send == NULL, FALSE);	
	
	_nf_mail_send = g_object_new ( NF_TYPE_MAIL_SEND , NULL);

	nf_debug_category_add( "mail", _DEBUG_MAIL_SEND_str, _DEBUG_MAIL_SEND_log, DEBUG_MAIL_SEND_IDX_NR);
																																
	if( wait )
	{
		while( _nf_mail_send->init_done != 1)
			g_usleep(10*1000);
	}
		
	return ret;
}

static char send_stmp_server[NF_MAIL_SEND_MAX_SERVER];

#if defined(ENABLE_EMAIL_DUAL_SERVER)
static gboolean
_sysdb_get_smtp_info( NF_MAIL_SEND_SERVER *server , gint svr_no )
#else
static gboolean
_sysdb_get_smtp_info( NF_MAIL_SEND_SERVER *server )
#endif
{
#if defined(ENABLE_EMAIL_DUAL_SERVER)
	char *smtp_server=NULL, *smtp_user=NULL, *smtp_passwd=NULL;
	int smtp_port=0, smtp_security=0;

	#if 0
		g_return_val_if_fail (server, 0);
	#endif

	if ( svr_no == 0 ) {
		smtp_server = nf_sysdb_get_str_nocopy( "net.email.smtpsvr");
		smtp_user = nf_sysdb_get_str_nocopy( "net.email.id");
		smtp_passwd = nf_sysdb_get_str_nocopy( "net.email.passwd");

		smtp_port =  nf_sysdb_get_uint( "net.email.smtpport");
		smtp_security =  nf_sysdb_get_bool( "net.email.ssl");
	}else{
		smtp_server = nf_sysdb_get_str_nocopy( "net.email_2nd.smtpsvr");
		smtp_user = nf_sysdb_get_str_nocopy( "net.email_2nd.id");
		smtp_passwd = nf_sysdb_get_str_nocopy( "net.email_2nd.passwd");

		smtp_port =  nf_sysdb_get_uint( "net.email_2nd.smtpport");
		smtp_security =  nf_sysdb_get_bool( "net.email_2nd.ssl");
	}

#else
	g_return_val_if_fail (server, 0);

	char *smtp_server = nf_sysdb_get_str_nocopy( "net.email.smtpsvr");
	char *smtp_user = nf_sysdb_get_str_nocopy( "net.email.id");
	char *smtp_passwd = nf_sysdb_get_str_nocopy( "net.email.passwd");

	int smtp_port =  nf_sysdb_get_uint( "net.email.smtpport");
	int smtp_security =  nf_sysdb_get_bool( "net.email.ssl");
#endif


	g_return_val_if_fail( smtp_server, 0);
	g_return_val_if_fail( smtp_user, 0);
	g_return_val_if_fail( smtp_passwd, 0);

	memset( server, 0x00, sizeof(NF_MAIL_SEND_SERVER));

	memset(send_stmp_server, 0x00, NF_MAIL_SEND_MAX_SERVER);
	strncpy(send_stmp_server, smtp_server, NF_MAIL_SEND_MAX_SERVER-1);

	strncpy( server->smtp_server, smtp_server,	NF_MAIL_SEND_MAX_SERVER-1 );
	strncpy( server->smtp_user, 	smtp_user, 		NF_MAIL_SEND_MAX_USER-1 );
	strncpy( server->smtp_passwd, smtp_passwd, 	NF_MAIL_SEND_MAX_PASSWD-1 );

	server->smtp_port = smtp_port;
	server->smtp_security = smtp_security;

#ifdef DEBUG_MAIL_SEND_LOG
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_REQUEST] )
		nf_debug_hexdump(  server, sizeof(NF_MAIL_SEND_SERVER));
#endif

	return 1;

}

static void 
_free_minfo( NF_MAIL_SEND_INTERNAL *minfo )
{
	g_return_if_fail (minfo);
	
	if( minfo->content.image_data) 
		g_free( minfo->content.image_data);	
		
	g_free( minfo);
				
}

static void 
_dump_svr( NF_MAIL_SEND_SERVER *server )
{
	int user_count;
	
	g_return_if_fail (server);
	
	g_message("smtp_server  [%s]", server->smtp_server	);
	g_message("smtp_user    [%s]", server->smtp_user	);
	g_message("smtp_passwd  [%s]", server->smtp_passwd	);
	g_message("smtp_port    [%d] smtp_ssl[%d]", 
				server->smtp_port, 
				server->smtp_security );                       
}

static void 
_dump_cont( NF_MAIL_SEND_CONTENT *content )
{
	int user_count;
	
	g_return_if_fail (content);
						                            
	g_message("from         [%s]", content->from			);
	g_message("to_cnt       [%d]", content->to_cnt);
	for( user_count = 0; user_count < content->to_cnt; ++user_count )
	{
		g_message("to[%d]        [%s]", user_count, content->to[user_count] );
	}
	g_message("subject      [%s]", content->subject		);
	g_message("image_size   [%d]", content->image_size	);
	g_message("image_name   [%s]", content->image_name	);
	g_message("cb_func      [0x%08x]", content->cb_func	    );
	g_message("cb_arg       [0x%08x]", content->cb_arg	    );
	g_message("is_dvr_event [%d]", content->is_dvr_event  );
	g_message("contents     [%s]", content->contents  );

}

static void 
_dump_minfo( NF_MAIL_SEND_INTERNAL *minfo )
{
	int user_count;
	
	g_return_if_fail (minfo);
	
	_dump_svr(&minfo->server);
	_dump_cont(&minfo->content);
}

gboolean nf_mail_send_check_email(char *email)
{

  int i, find_at = 0, at_count=0, dot_count=0;
  if(email == NULL) return 0;

  if( strnlen(email,NF_MAIL_SEND_MAX_FROM) > NF_MAIL_SEND_MAX_FROM) return 0;

  for(i = 0; email[i] != '\0'; ++i){
    switch(email[i]) {
      case '@' : {
        ++at_count;
        if(at_count != 1 || i == 0) return 0;
        if(email[i+1] == '\0') return 0;
        	
        find_at = 1;
        break;
      }
      case '.' : {                
        if(find_at)
        	++dot_count;
        if(email[i-1] == '@' || email[i-1] == '.') return 0;
        if(email[i+1] == '\0') return 0;
        break;
      }
      default  : {
             if((email[i] >= '0') && (email[i] <= '9')) break;
        else if((email[i] >= 'A') && (email[i] <= 'Z')) break;
        else if((email[i] >= 'a') && (email[i] <= 'z')) break;
        else if((email[i] == '-') || (email[i] == '_')) break;
        else return 0;
      }
    }
  }

  if( find_at == 0 || dot_count < 1 || dot_count > 3) return 0;
  return 1;

}

static int 
_check_param_server(NF_MAIL_SEND_SERVER *svr)
{
	
	int len = 0, user_len = 0, pass_len = 0;

#ifdef DEBUG_MAIL_SEND_LOG
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_REQUEST_RAW] )
		nf_debug_hexdump(  svr, sizeof(NF_MAIL_SEND_SERVER));

	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_REQUEST] )
		_dump_svr( svr);
				
#endif
	
	len = strnlen(svr->smtp_server, NF_MAIL_SEND_MAX_SERVER);
	g_return_val_if_fail( len >0 && len < NF_MAIL_SEND_MAX_SERVER , NF_MAIL_SEND_ERROR_SERVER);
	
	user_len =  strnlen(svr->smtp_user, NF_MAIL_SEND_MAX_USER);
	pass_len =  strnlen(svr->smtp_passwd, NF_MAIL_SEND_MAX_PASSWD);
	
	g_return_val_if_fail( user_len < NF_MAIL_SEND_MAX_USER, NF_MAIL_SEND_ERROR_USERNAME);
	g_return_val_if_fail( pass_len < NF_MAIL_SEND_MAX_PASSWD, NF_MAIL_SEND_ERROR_PASSWORD);
	
	g_return_val_if_fail( svr->smtp_port>0 && svr->smtp_port<0xffff, NF_MAIL_SEND_ERROR_PORT);

#if 0	
	if(svr->smtp_security)
	{
		g_return_val_if_fail( user_len>0 && pass_len>0, NF_MAIL_SEND_ERROR_SECURITY);
	}
#endif
	
	return NF_MAIL_SEND_ERROR_SUCCESS;
}

static int 
_check_param_content(NF_MAIL_SEND_CONTENT *cont)
{
	int i;

#ifdef DEBUG_MAIL_SEND_LOG
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_REQUEST_RAW] )
		nf_debug_hexdump(  cont, sizeof(NF_MAIL_SEND_CONTENT));
		
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_REQUEST] )
		_dump_cont( cont );
				
#endif

	g_return_val_if_fail( strnlen(cont->from, NF_MAIL_SEND_MAX_FROM) < NF_MAIL_SEND_MAX_FROM, NF_MAIL_SEND_ERROR_FROM);

#if 0	
	g_return_val_if_fail( nf_mail_send_check_email(cont->from), NF_MAIL_SEND_ERROR_FROM);
#endif
		
	g_return_val_if_fail( cont->to_cnt < NF_MAIL_SEND_MAX_TO_CNT , NF_MAIL_SEND_ERROR_TO);
	g_return_val_if_fail( cont->to_cnt != 0 , NF_MAIL_SEND_ERROR_TO);	
	for(i=0;i<cont->to_cnt;++i)
	{
		g_return_val_if_fail( strnlen(cont->to[i], NF_MAIL_SEND_MAX_TO) < NF_MAIL_SEND_MAX_TO, NF_MAIL_SEND_ERROR_TO);		
		g_return_val_if_fail( nf_mail_send_check_email(cont->to[i]), NF_MAIL_SEND_ERROR_TO);
	}
		
	g_return_val_if_fail( strnlen(cont->subject, NF_MAIL_SEND_MAX_SUBJECT) < NF_MAIL_SEND_MAX_SUBJECT , NF_MAIL_SEND_ERROR_SUBJECT);
	g_return_val_if_fail( strnlen(cont->contents, NF_MAIL_SEND_MAX_CONTENTS) < NF_MAIL_SEND_MAX_CONTENTS , NF_MAIL_SEND_ERROR_CONTENTS);
	g_return_val_if_fail( strnlen(cont->image_name, NF_MAIL_SEND_MAX_IMAGE_NAME) < NF_MAIL_SEND_MAX_IMAGE_NAME , NF_MAIL_SEND_ERROR_IMAGE_NAME);
	
	if( cont->image_size == 0 )
		g_return_val_if_fail( cont->image_data == NULL, NF_MAIL_SEND_ERROR_IMAGE_DATA);	
	else				
		g_return_val_if_fail( cont->image_data != NULL, NF_MAIL_SEND_ERROR_IMAGE_DATA);
		
	g_return_val_if_fail( cont->image_size < NF_MAIL_SEND_MAX_IMAGE_SIZE, NF_MAIL_SEND_ERROR_IMAGE_SIZE);
	
	return NF_MAIL_SEND_ERROR_SUCCESS;
}

static 
NF_MAIL_SEND_INTERNAL *_make_minfo( NF_MAIL_SEND_SERVER *svr, NF_MAIL_SEND_CONTENT *cont )
{
	NF_MAIL_SEND_INTERNAL	*minfo = NULL;
	gpointer image_data = NULL;
	int i;
	char server[256], domain[256];
	
	g_return_val_if_fail ( svr != NULL, NULL);
	g_return_val_if_fail ( cont != NULL, NULL);
	 	
	if( cont->image_size > 0)
	{
		image_data = g_malloc( cont->image_size );
		g_return_val_if_fail (image_data, 0);
	}
	
	minfo = g_malloc0( sizeof(NF_MAIL_SEND_INTERNAL) );
	if(!minfo)
	{
		if(image_data) g_free(image_data);
		g_return_val_if_fail( minfo, 0);			
	}

	memset(send_stmp_server, 0x00, NF_MAIL_SEND_MAX_SERVER);
	strncpy(send_stmp_server, svr->smtp_server, NF_MAIL_SEND_MAX_SERVER-1);
	
	strncpy( minfo->server.smtp_server, svr->smtp_server, 	NF_MAIL_SEND_MAX_SERVER-1 );	
	strncpy( minfo->server.smtp_user, svr->smtp_user, 	NF_MAIL_SEND_MAX_USER-1 );	
	strncpy( minfo->server.smtp_passwd, svr->smtp_passwd, 	NF_MAIL_SEND_MAX_PASSWD-1 );
		
	minfo->server.smtp_port = svr->smtp_port;
	minfo->server.smtp_security = svr->smtp_security;
	

#if 1
	//NF_MAIL_SEND_MAX_FROM
	if( strchr( svr->smtp_user, '@' ) )	{
		strncpy( minfo->content.from, svr->smtp_user, 	NF_MAIL_SEND_MAX_FROM-1 );				
	}else{
		memset( server, 0x00, sizeof(server) );
		memset( domain, 0x00, sizeof(domain) );	
		
		sscanf(svr->smtp_server,  "%[^.].%s", server, domain);	
		snprintf( minfo->content.from, sizeof(minfo->content.from)-1,
				"%s@%s", svr->smtp_user, domain);
	}
	
#else
	strncpy( minfo->content.from, cont->from, 	NF_MAIL_SEND_MAX_FROM-1 );		
#endif	

	minfo->content.to_cnt = cont->to_cnt;
	for(i=0;i<cont->to_cnt;++i)
		strncpy( minfo->content.to[i], cont->to[i], 	NF_MAIL_SEND_MAX_TO-1 );	
		
	strncpy( minfo->content.subject, cont->subject, 	NF_MAIL_SEND_MAX_SUBJECT-1 );	
	strncpy( minfo->content.contents, cont->contents, 	NF_MAIL_SEND_MAX_CONTENTS-1 );	
	
	minfo->content.image_size =  cont->image_size;
	strncpy( minfo->content.image_name, cont->image_name, 	NF_MAIL_SEND_MAX_IMAGE_NAME-1 );		

	if( cont->image_size > 0)
	{
		memcpy(image_data, cont->image_data, cont->image_size );
		minfo->content.image_data = image_data;
	}
	
	minfo->content.cb_func = cont->cb_func;
	minfo->content.cb_arg = cont->cb_arg;
		
	return minfo;
}


 
static gint _send_request(NF_MAIL_SEND_SERVER *svr, NF_MAIL_SEND_CONTENT *cont)
{
	printf("check_chck >>>>>>> point0 [%s], [%d] \n", __FUNCTION__, __LINE__); 				
	gint q_len = 0;
	gint i = 0, ret = 0;
	
	NF_MAIL_SEND_INTERNAL	*minfo = NULL;
	gpointer image_data = NULL;
		
	g_return_val_if_fail (_nf_mail_send != NULL, NF_MAIL_SEND_ERROR_INIT);	
	
	g_return_val_if_fail (cont, NF_MAIL_SEND_ERROR_FAILED);
	g_return_val_if_fail (svr, NF_MAIL_SEND_ERROR_FAILED);

	ret = _check_param_content(cont);
	g_return_val_if_fail ( ret == 0 , ret );

	ret = _check_param_server(svr);
	g_return_val_if_fail ( ret == 0 , ret);

	minfo = _make_minfo(svr, cont);
	g_return_val_if_fail( minfo , NF_MAIL_SEND_ERROR_MALLOC);

#ifdef DEBUG_MAIL_SEND_LOG
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_TEST] )
		_dump_minfo(minfo);
#endif

	q_len = g_async_queue_length(_nf_mail_send->queue);
	if(q_len > NF_MAIL_SEND_MAX_QUEUE_LEN)
	{
		g_warning("%s nf_mail_send->queue FULL[%d]",__FUNCTION__, q_len);		
		_free_minfo(minfo);
		return NF_MAIL_SEND_ERROR_MAX_QUE_LEN;
	}

#ifdef DEBUG_DIRECT_MAIL_SEND
	g_static_mutex_lock(&_mail_send_mutex);
	_mail_send_request(minfo);
	_free_minfo(minfo);
	g_static_mutex_unlock(&_mail_send_mutex);
#else
	g_async_queue_push( _nf_mail_send->queue, minfo);
#endif
		
	return NF_MAIL_SEND_ERROR_SUCCESS; // 0
}

gboolean 
nf_mail_send_request(NF_MAIL_SEND_CONTENT *cont, GError **error)
{
	gint err_code = NF_MAIL_SEND_ERROR_SUCCESS;
	NF_MAIL_SEND_SERVER server;

	#if defined(ENABLE_EMAIL_DUAL_SERVER)
		if( _sysdb_get_smtp_info( &server , cont->email_serv ) != 1 )	// error
	#else
		if( _sysdb_get_smtp_info( &server ) != 1 )
	#endif
	{
		err_code = NF_MAIL_SEND_ERROR_SERVER_CONF;
		goto ret_error;
	}

	err_code = _send_request(&server, cont);
	if(err_code != NF_MAIL_SEND_ERROR_SUCCESS)
		goto ret_error;

	return 1;

ret_error:

	g_warning("%s failed ret[%d]",__FUNCTION__, err_code);
	g_set_error (error,
					NF_MAIL_SEND_ERROR ,	/* error domain */
					err_code ,					/* error code */
					"send mail error code[%d]",	/* error message format string */
					err_code);

	return 0;
}


gboolean 
nf_mail_send_test(NF_MAIL_SEND_SERVER *svr, NF_MAIL_SEND_CONTENT *cont, GError **error)
{
					

	gint err_code;			
		
	err_code = _send_request( svr, cont);
	if(err_code != NF_MAIL_SEND_ERROR_SUCCESS)
		goto ret_error;
	
	return 1;
	
ret_error:
			
	g_warning("%s failed ret[%d]",__FUNCTION__, err_code);	
	g_set_error (error,
					NF_MAIL_SEND_ERROR ,	/* error domain */
					err_code ,					/* error code */
					"send mail error code[%d]",	/* error message format string */
					err_code);																		
	return 0;		
}

gint
nf_mail_send_get_queue_length()
{
	g_return_val_if_fail (_nf_mail_send != NULL, 0);	
	
	return g_async_queue_length(_nf_mail_send->queue);
}


/******************************************************************************/

// image_path	: absolute path of image file 
// image		: image buffer it should be allocated enouth to hold image file
// image_size	: [in]: buffer size, [out]: image size
static int _read_image_file(char* image_path, char* image, int* image_size) 
{
	FILE* fp_image = NULL;
	struct stat st;
	size_t read_size = 0;

	if ( (stat(image_path, &st) == -1) || ((st.st_mode & S_IFMT)!=S_IFREG) )  {
		printf("read_image_file(): invalid image file\n");
		return -1;
	}
	
	if(*image_size < st.st_size)	{
		printf("read_image_file(): image buffer isn't enough\n");
		return -2;
	}

	fp_image = fopen(image_path, "r");
	if(NULL==fp_image)	{
		printf("read_image_file(): error while open file \n");
		fclose(fp_image);
		return -3;
	}
	
	printf("image_size : %d\n", *image_size);
	read_size = fread(image, 1, *image_size, fp_image);

	if(read_size != 0) {
		*image_size = read_size;
	} else {
		printf("fread reads 0 byte : %s\n", strerror(errno));
		fclose(fp_image);
		return -4;
	}

	fclose(fp_image);

	return 0;
}

static char* _get_file_name (char *pathname)
{
	char *fname = NULL;
	if (pathname) {
		fname = strrchr (pathname, '/') + 1;
	}
	return fname;
}

const char *readlinefp_cb(void **buf, int *len, void *arg);
void monitor_cb(const char *buf, int buflen, int writing, void *arg);
void print_recipient_status(smtp_recipient_t recipient, const char *mailbox, void *arg);
int authinteract(auth_client_request_t request, char **result, int fields, void *arg);
int tlsinteract(char *buf, int buflen, int rwflag, void *arg);
void event_cb(smtp_session_t session, int event_no, void *arg, ...);

static int write_image_base64(FILE* fp_mail, char* data, unsigned long int length);
static int write_mail_contents(FILE* fp_mail, NF_MAIL_SEND_INTERNAL* info);

static int init_base64_encode_table (void);

typedef struct _login_info_t {
	char id[128];
	char pw[128];
} login_info_t;
static login_info_t login_info;

static unsigned char ssl_base64_dtable[256];
static unsigned char ssl_reverse_table[256];

#define MAX_SMTP_ERROR_CODE 10

static int esmtp_monitor_error[MAX_SMTP_ERROR_CODE];
static int esmtp_monitor_error_index;
static void smtp_error_code_table_clear(void);

#define MAX_MAIL_SIZE		(1024*1024)


/* Callback function to read the message from a file.  Since libESMTP
   does not provide callbacks which translate line endings, one must
   be provided by the application.

   The message is read a line at a time and the newlines converted
   to \r\n.  Unfortunately, RFC 822 states that bare \n and \r are
   acceptable in messages and that individually they do not constitute a
   line termination.  This requirement cannot be reconciled with storing
   messages with Unix line terminations.  RFC 2822 rescues this situation
   slightly by prohibiting lone \r and \n in messages.

   The following code cannot therefore work correctly in all situations.
   Furthermore it is very inefficient since it must search for the \n.
 */
#define BUFLEN	8192
const char *readlinefp_cb(void **buf, int *len, void *arg)
{
	int octets;

	if (*buf == NULL)
		*buf = malloc(BUFLEN);

	if (len == NULL) {
		rewind((FILE *) arg);
		return NULL;
	}

	if (fgets(*buf, BUFLEN - 2, (FILE *) arg) == NULL)
		octets = 0;
	else {
		char *p = strchr(*buf, '\0');
		if (p[-1] == '\n' && p[-2] != '\r') {
			strcpy(p - 1, "\r\n");
			p++;
		}
		octets = p - (char *)*buf;
	}
	*len = octets;
	return *buf;
}

void monitor_cb(const char *buf, int buflen, int writing, void *arg)
{
	FILE *fp = arg;
	if (writing == SMTP_CB_HEADERS) {
		fputs("H: ", fp);
		fwrite(buf, 1, buflen, fp);
		return;
	}

	char tmp[8];
	int code = 0;
	memset(tmp, 0, 8);
	strncpy(tmp, buf, 3);
	code = atoi(tmp);

	/* 
	 * It's hard to determine what code should we consider error or not.
	 * Because shortage understanding of SMTP. and various SMTP server's in Internet. 
	 * We can't count on all of the situation. 
	 *
	 * For example of weird case.., 
	 * Accessing mail.itxsecurity.com with incorrect pw is OK. 
	 * Some recipient side mail server process the mail spam, but some does not. 
	 * What do you think about how should we deal with this reponse code ?? 
	 *
	 * If you find any mis-understanding in here or you have concrete idea how smtp works,
	 * please let me have an honor to hear from you about your wisdom : hwlee@itxsecurity.com 
	 * */
	switch(code) {
		case 553:	
			printf("monitor Auth Failure\n");
			break;
	}
	
	fputs(writing ? "C: " : "S: ", fp);
	fwrite(buf, 1, buflen, fp);
	if (buf[buflen - 1] != '\n')
		putc('\n', fp);
}

/* Callback to request user/password info.  Not thread safe. */
int authinteract(auth_client_request_t request, char **result, int fields, void *arg)
{
	printf("## authinteract(): fields :%d\n", fields);
	result[0] = login_info.id;
	if(fields == 2)
		result[1] = login_info.pw;

	return 1;
}

int tlsinteract(char *buf, int buflen, int rwflag, void *arg)
{
	char *pw;
	int len;

	printf("## tlsinteract(): \n");
	//pw = getpass("certificate password");	
	pw = "certificate password";
	len = strlen(pw);
	if (len + 1 > buflen)
		return 0;
	strcpy(buf, pw);
	return len;
}

int handle_invalid_peer_certificate(long vfy_result)
{
	const char *k = "rare error";
	switch (vfy_result) {
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:			k = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT";				break;
		case X509_V_ERR_UNABLE_TO_GET_CRL:					k = "X509_V_ERR_UNABLE_TO_GET_CRL";						break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:	k = "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE";		break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:	k = "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE";		break;
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:	k = "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY";	break;
		case X509_V_ERR_CERT_SIGNATURE_FAILURE:				k = "X509_V_ERR_CERT_SIGNATURE_FAILURE";				break;
		case X509_V_ERR_CRL_SIGNATURE_FAILURE:				k = "X509_V_ERR_CRL_SIGNATURE_FAILURE";					break;
		case X509_V_ERR_CERT_NOT_YET_VALID:					k = "X509_V_ERR_CERT_NOT_YET_VALID";					break;
		case X509_V_ERR_CERT_HAS_EXPIRED:					k = "X509_V_ERR_CERT_HAS_EXPIRED";						break;
		case X509_V_ERR_CRL_NOT_YET_VALID:					k = "X509_V_ERR_CRL_NOT_YET_VALID";						break;
		case X509_V_ERR_CRL_HAS_EXPIRED:					k = "X509_V_ERR_CRL_HAS_EXPIRED";						break;
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:		k = "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD";		break;
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:		k = "X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD";			break;
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:		k = "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD";		break;
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:		k = "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD";		break;
		case X509_V_ERR_OUT_OF_MEM:							k = "X509_V_ERR_OUT_OF_MEM";							break;
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:		k = "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT";			break;
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:			k = "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN";				break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:	k = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY";		break;
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:	k = "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE";		break;
		case X509_V_ERR_CERT_CHAIN_TOO_LONG:				k = "X509_V_ERR_CERT_CHAIN_TOO_LONG";					break;
		case X509_V_ERR_CERT_REVOKED:						k = "X509_V_ERR_CERT_REVOKED";							break;
		case X509_V_ERR_INVALID_CA:							k = "X509_V_ERR_INVALID_CA";							break;
		case X509_V_ERR_PATH_LENGTH_EXCEEDED:				k = "X509_V_ERR_PATH_LENGTH_EXCEEDED";					break;
		case X509_V_ERR_INVALID_PURPOSE:					k = "X509_V_ERR_INVALID_PURPOSE";						break;
		case X509_V_ERR_CERT_UNTRUSTED:						k = "X509_V_ERR_CERT_UNTRUSTED";						break;
		case X509_V_ERR_CERT_REJECTED:						k = "X509_V_ERR_CERT_REJECTED";							break;
	}
	printf("send mail server %s SMTP_EV_INVALID_PEER_CERTIFICATE: %ld: %s\n", send_stmp_server, vfy_result, k);

	if (strcmp(send_stmp_server, "smtppw.dvrlink.net") == 0 || strcmp(send_stmp_server, "SMTPPW.DVRLINK.NET") == 0) {
		return 0;
	}

	return 1;		/* Accept the problem */
}

void event_cb(smtp_session_t session, int event_no, void *arg, ...)
{
	va_list alist;
	int *ok;

	va_start(alist, arg);
	switch (event_no) {
	case SMTP_EV_CONNECT:
	case SMTP_EV_MAILSTATUS:
	case SMTP_EV_RCPTSTATUS:
	case SMTP_EV_MESSAGEDATA:
	case SMTP_EV_MESSAGESENT:
	case SMTP_EV_DISCONNECT:
//		printf("event_cb() %d\n", event_no);
		break;
	case SMTP_EV_WEAK_CIPHER:
		{
			int bits;
			bits = va_arg(alist, long);
			ok = va_arg(alist, int *);
			printf("SMTP_EV_WEAK_CIPHER, bits=%d - accepted.\n",
			       bits);
			*ok = 1;
			break;
		}
	case SMTP_EV_STARTTLS_OK:
		puts("SMTP_EV_STARTTLS_OK - TLS started here.");
		break;
	case SMTP_EV_INVALID_PEER_CERTIFICATE:{
			long vfy_result;
			vfy_result = va_arg(alist, long);
			ok = va_arg(alist, int *);
			*ok = handle_invalid_peer_certificate(vfy_result);
			break;
		}
	case SMTP_EV_NO_PEER_CERTIFICATE:{
			ok = va_arg(alist, int *);
			puts("SMTP_EV_NO_PEER_CERTIFICATE - accepted.");
			*ok = 1;
			break;
		}
	case SMTP_EV_WRONG_PEER_CERTIFICATE:{
			ok = va_arg(alist, int *);
			puts("SMTP_EV_WRONG_PEER_CERTIFICATE - accepted.");
// ok 포인터로 libesmtp 라이브러에 전달 함. 결과 값이 1이면 email 전송 하는 구조.
// 인증서 문제시 이메일 전송을 하지 않게 한다.
#if 0
			*ok = 1;
#endif
			break;
		}
	case SMTP_EV_NO_CLIENT_CERTIFICATE:{
			ok = va_arg(alist, int *);
			puts("SMTP_EV_NO_CLIENT_CERTIFICATE - accepted.");
			*ok = 1;
			break;
		}
	default:
		printf("Got event: %d - ignored.\n", event_no);
	}
	va_end(alist);
}

static gint _util_mail_get_lang_id(void)
{
	const char *db_lang = nf_sysdb_get_str_nocopy("disp.osd.lang");

	g_return_val_if_fail( db_lang != NULL, 0);

	if( strcasecmp( db_lang, "Korean") == 0 )
		return 0;
	else
		return 0;
}


static int write_mail_contents(FILE* fp_mail, NF_MAIL_SEND_INTERNAL* info)
{
	int i=0;

	fprintf(fp_mail,	"From: NVR<%s>\n", info->content.from);
	//fprintf(fp_mail,	"Return-Path: <%s>\n", info->content.from);	
	fprintf(fp_mail, 	"To: <%s>", info->content.to[0]);
	for(i=1; i<info->content.to_cnt; i++)	{
		fprintf(fp_mail, ", <%s>", info->content.to[i]);
	}	
	fprintf(fp_mail,	"\nSubject: %s\n", info->content.subject);
		
	fprintf(fp_mail, 	"MIME-Version: 1.0\n"
						"Content-Type: MULTIPART/MIXED; BOUNDARY=\"----_=misnaled\"\n"
						"Content-Transfer-Encoding: 8bit\n\n"
						"This is a MIME multipart message.\n"
						"\n\n\n------_=misnaled\n" );
						
	if( _util_mail_get_lang_id() )
	{
		fprintf(fp_mail, "Content-Type: text/plain; charset=\"euc-kr\"\n"
						 "Content-Transfer-Encoding: 8bit\n\n");
	}
	else
	{
	fprintf(fp_mail, "Content-Type: text/plain; charset=\"us-ascii\"\n"
					 "Content-Transfer-Encoding: 8bit\n\n");
	}
					 
	fprintf(fp_mail, "%s\n",	info->content.contents);
														
	if( info->content.image_size > 0)
	{
		fprintf(fp_mail, "\n\n\n------_=misnaled\n" );	
		fprintf(fp_mail, "Content-Type: image/jpeg; name=\"%s\"\n", info->content.image_name ? info->content.image_name : "none");
		fprintf(fp_mail, "Content-Transfer-Encoding: base64\n\n");

		init_base64_encode_table();
	
		write_image_base64(fp_mail, info->content.image_data, info->content.image_size);

	}
	fprintf(fp_mail, "\n\n\n------_=misnaled--\n\n\n\n");
	
	return 0;
}

static int init_base64_encode_table (void)
{
	int tmp;
	int ch;
	unsigned char *chp;
	static int initialized = 0;
	if( 1 == initialized )	{
		return 0;
	}
	initialized = 1;

	for (tmp = 0; tmp < 26; tmp++)	{
		ssl_base64_dtable[tmp] = 'A' + tmp;
		ssl_base64_dtable[26 + tmp] = 'a' + tmp;
	}

	for (tmp = 0; tmp < 10; tmp++)	{
		ssl_base64_dtable[52 + tmp] = '0' + tmp;
	}

	ssl_base64_dtable[62] = '+';
	ssl_base64_dtable[63] = '/';

	for (ch = 0; ch < 256; ch++)	{
		chp = (unsigned char*)strchr ((char*)ssl_base64_dtable, ch);

		if (chp)
			ssl_reverse_table[ch] = chp - ssl_base64_dtable;
		else
			ssl_reverse_table[ch] = -1;
	}
	return 0;
}


/* libESMTP doens't support MIME, so it's required to additional code to send a mail with image file */
//static int write_image_base64(FILE* fp_mail, sock, char *data, unsigned long int length)
static int write_image_base64(FILE* fp_mail, char* data, unsigned long int length)
{
	int finished = 0, outbytes = 0, width = 0, cnt;
	unsigned char igroup[3], ogroup[2056];
	unsigned long int ptr = 0;

	memset (ogroup, 0, sizeof (char) * 2056);


	while (!finished)   {
		igroup[0] = igroup[1] = igroup[2] = 0;
		for (cnt = 0; cnt < 3; cnt++)   {
			if (ptr == length)  {
				finished = 1;
				break;
			}
			igroup[cnt] = data[ptr++];
		}

		if (cnt == 0)
			break;

		/* This is the encoding stuff - courtesy of John Walker */
		ogroup[outbytes] = ssl_base64_dtable[igroup[0] >> 2];
		ogroup[outbytes + 1] = ssl_base64_dtable[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
		ogroup[outbytes + 2] = ssl_base64_dtable[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
		ogroup[outbytes + 3] = ssl_base64_dtable[igroup[2] & 0x3F];

		/* Replace characters in output stream with "=" pad
		 * characters if fewer than three characters were
		 * read from the end of the input stream. */
		if (cnt < 3)    {
			ogroup[outbytes + 3] = '=';
			if (cnt < 2)    {
				ogroup[outbytes + 2] = '=';
			}
		}
		outbytes += 4;
		width += 4;

		/* After 72 characters in a line we need a linebreak */
		if (width > 72) {
			ogroup[outbytes++] = '\r';
			ogroup[outbytes++] = '\n';
			width = 0;
		}

		/* If we have more than 2K of data, we send it */
		if (outbytes >= 2048)   {
		//	ssl_send_cmd (tls, sock, (char*)ogroup);
			fprintf(fp_mail, "%s", (char*)ogroup);
			/* We reset the pointer into our outbuffer, too */
			outbytes = 0;
			memset (ogroup, 0, sizeof (char) * 2056);
		}
	}

	ogroup[outbytes++] = '\r';
	ogroup[outbytes++] = '\n';
	ogroup[outbytes] = '\0';

//	printf("ssl send base 64 : %d \n", outbytes);
	fprintf(fp_mail, "%s", (char*)ogroup);

	return 0;
}

static void smtp_error_code_table_clear() 
{
	int i;
	for(i=0; i<MAX_SMTP_ERROR_CODE ; i++){
		esmtp_monitor_error[i] = 0;
	}
	esmtp_monitor_error_index = 0;
}

/* Callback to prnt the recipient status */
void print_recipient_status(smtp_recipient_t recipient, const char *mailbox, void *arg)
{
	const smtp_status_t *status = NULL;
	status = smtp_recipient_status(recipient);
	
	g_return_if_fail(status);
	g_return_if_fail(mailbox);
	
	g_message("%s mbox[%s] status[%d](%s)", __FUNCTION__, 
				mailbox, status->code, (status->text != NULL) ? status->text : "");
	
}

#ifdef ENABLE_MAIL_SEND_EVENTLOG	

#include "nf_api_eventlog.h"
	
/* Callback to prnt the recipient status */
void log_recipient_status(smtp_recipient_t recipient, const char *mailbox, void *arg)
{
	const smtp_status_t *status = NULL;
	status = smtp_recipient_status(recipient);
	gboolean ret = 0;
	
	g_return_if_fail(status);
	g_return_if_fail(mailbox);
#if 0	
	g_message("%s mbox[%s] status[%d](%s)", __FUNCTION__, 
				mailbox, status->code, (status->text != NULL) ? status->text : "");
#endif
	if( status->code == 250)	// Recipient ok
	{
		ret = nf_eventlog_put_param( NULL, LT_SYSTEM_EVENT, 1, LP2_SYSTEM_EVENT_EMAIL_SENT, mailbox);
	}
}
#endif


static int _mail_send_request(NF_MAIL_SEND_INTERNAL* info)
{
	smtp_session_t session = NULL;
	smtp_message_t message = NULL;
	smtp_recipient_t recipient = NULL;
	auth_context_t authctx;
	const smtp_status_t *status;
	struct sigaction sa;
	FILE *fp_mail = NULL;
	int i;
	int ret = 0;

	char host[256], err_msg[256];
	char buffer[MAX_MAIL_SIZE];
	
#ifdef ENABLE_MAIL_SEND_CB_FUNC
	if( info->content.cb_func )
	{
		info->content.cb_func(info->content.cb_arg, 
					NF_MAIL_SEND_STATUS_START,
					"Start Sending" );
	}
#endif
	
	memset(buffer, 0, MAX_MAIL_SIZE);
	memset(err_msg, 0, sizeof(err_msg) );

	fp_mail = fmemopen (buffer, MAX_MAIL_SIZE, "w+"); 
	if (NULL==fp_mail) {
		g_warning("%s fmemopen return NULL %d", __FUNCTION__, (errno));			// don't forget strerror is not thread-safe
		
		ret = -1; snprintf( err_msg, sizeof(err_msg)-1, "fmemopen failed");
		goto send_failed;
	}

	smtp_error_code_table_clear();
	write_mail_contents(fp_mail, info);

	strncpy(login_info.id, info->server.smtp_user, sizeof(login_info.id));
	strncpy(login_info.pw, info->server.smtp_passwd, sizeof(login_info.pw));

	/* This program sends only one message at a time.  
	Create an SMTP session and add a message to it. */
	//auth_client_init();
	session = smtp_create_session();
	message = smtp_add_message(session);
	
	g_assert(session);
	g_assert(message);
	
	if( session == NULL || message== NULL)
	{
		ret = -2; snprintf( err_msg, sizeof(err_msg)-1, "session or message failed");
		goto send_failed;
	}
	
	
#ifdef DEBUG_MAIL_SEND_LOG
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_LIBESMTP] )
		smtp_set_monitorcb(session, monitor_cb, stdout, 1);	
		// if you want to monitor mail session 
#endif

#if 1
	smtp_starttls_enable(session, Starttls_ENABLED);	// optional
#else		
	if(info->server.smtp_security)
	{
		smtp_starttls_enable(session, Starttls_REQUIRED);		
	}
#endif

	if (access("/tmp/starttls_disable", F_OK) == 0) {
		smtp_starttls_enable(session, Starttls_DISABLED);
		g_message("%s starttls_disabled test\n", __FUNCTION__);
	}

	/* Set the host running the SMTP server.  LibESMTP has a default port
	   number of 587, however this is not widely deployed so the port
	   is specified as 25 along with the default MTA host. */
	snprintf(host, sizeof(host), "%s:%d", info->server.smtp_server, info->server.smtp_port);
		
#ifdef DEBUG_MAIL_SEND_LOG
	if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_LIBESMTP] )
		g_message("%s host[%s]", __FUNCTION__, host);
#endif
		
	smtp_set_server(session, host);

	/* Do what's needed at application level to use authentication.	 */
	authctx = auth_create_context();
	g_assert(authctx);
	
	if( authctx == NULL )
	{
		ret = -3; snprintf( err_msg, sizeof(err_msg)-1, "auth ctx failed");
		goto send_failed;
	}

			
	auth_set_mechanism_flags(authctx, AUTH_PLUGIN_PLAIN, 0);
	auth_set_interact_cb(authctx, authinteract, NULL);

	/* Use our callback for X.509 certificate passwords.  
	If STARTTLS is not in use or disabled in configure,
	the following is harmless. */
	smtp_starttls_set_password_cb(tlsinteract, NULL);
	smtp_set_eventcb(session, event_cb, NULL);

	/* Now tell libESMTP it can use the SMTP AUTH extension */
	smtp_auth_set_context(session, authctx);

	/* Set the reverse path for the mail envelope.  (NULL is ok) */
	smtp_set_reverse_path(message, info->content.from);

	/* RFC 2822 doesn't require recipient headers but a 
	To: header would be nice to have if not present. */
	smtp_set_header(message, "To", NULL, NULL);

	/* Set the Subject: header.  For no reason, 
	we want the supplied subject to override any subject line 
	in the message headers. */
	smtp_set_header(message, "Subject", info->content.subject);
	smtp_set_header_option(message, "Subject", Hdr_OVERRIDE, 1);

	smtp_set_messagecb(message, readlinefp_cb, fp_mail);
//	smtp_set_message_fp(message, fp_mail);	// when crlf is 0 

	for (i=0; i<info->content.to_cnt; i++) {
		recipient = smtp_add_recipient(message, info->content.to[i]);
	}

	/* Initiate a connection to the SMTP server and transfer the message. */
	if (!smtp_start_session(session)) {
		char buf[128];
		g_warning("%s smtp_start_session error[%s]", __FUNCTION__, 
							smtp_strerror(smtp_errno(), buf, sizeof(buf) ) );
		ret = -4; snprintf( err_msg, sizeof(err_msg)-1, "%s", buf);
		goto send_failed;
				
	} else {
		/* Report on the success or otherwise of the mail transfer.  */
		status = smtp_message_transfer_status(message);
			
#ifdef DEBUG_MAIL_SEND_LOG
		if( _DEBUG_MAIL_SEND_log[DEBUG_MAIL_SEND_IDX_LIBESMTP] )
			g_message("%s status[%d](%s)", __FUNCTION__, 
							status->code, (status->text != NULL) ? status->text : "");

		smtp_enumerate_recipients(message, print_recipient_status, NULL);
#endif				

#ifdef ENABLE_MAIL_SEND_EVENTLOG		
		smtp_enumerate_recipients(message, log_recipient_status, NULL);
#endif		

#ifdef ENABLE_MAIL_SEND_CB_FUNC
		if( info->content.cb_func && status )
		{
			info->content.cb_func(info->content.cb_arg, 
					( status->code == 250 ) ? NF_MAIL_SEND_STATUS_SUCCESS : NF_MAIL_SEND_STATUS_FAILED,
					( status->text != NULL ) ? status->text : "" );
		}
#endif
		goto send_ok;	// success or failed, but process ok
	}
//	printf("recipient %d\n", smtp_recipient_reset_status(recipient));

send_failed:

#ifdef ENABLE_MAIL_SEND_CB_FUNC
	if( info->content.cb_func )
	{
		info->content.cb_func(info->content.cb_arg, 
					NF_MAIL_SEND_STATUS_FAILED, 
					err_msg );
	}
#endif

send_ok:
	
	if(fp_mail) fclose(fp_mail);

	/* Free resources consumed by the program. */
	if(session) smtp_destroy_session(session);
	if(authctx) auth_destroy_context(authctx);
	//auth_client_exit();

	if (access("/tmp/starttls_disable", F_OK) == 0) {
		proxy_system("rm -f /tmp/starttls_disable",1,3);
	}

	g_message("%s ret[%d] msg[%s]", __FUNCTION__, ret, err_msg);
	
	return ret;
}

#if 0

sysdb-Message: 01575 [GParamString    ][STRING    ] name[net.email.from                  ] []
sysdb-Message: 01576 [GParamString    ][STRING    ] name[net.email.id                    ] []
sysdb-Message: 01577 [GParamString    ][STRING    ] name[net.email.passwd                ] []
sysdb-Message: 01578 [GParamUInt      ][UINT      ] name[net.email.smtpport              ] [25]
sysdb-Message: 01579 [GParamString    ][STRING    ] name[net.email.smtpsvr               ] []
sysdb-Message: 01580 [GParamBoolean   ][BOOL      ] name[net.email.ssl                   ] [FALSE]
sysdb-Message: 01581 [GParamString    ][STRING    ] name[net.email.testmail              ] []

sysdb_set net.email.smtpsvr	 s   SMTP.DVRLINK.NET
sysdb_set net.email.from     s   choissi@itxsecurity.com
sysdb_set net.email.testmail s   choissi@itxsecurity.com
sysdb_set net.email.id       s   test
sysdb_set net.email.passwd   s   test4smtp

sysdb_set net.email.smtpsvr	 s   mail.itxsecurity.com
sysdb_set net.email.id       s   choissi@itxsecurity.com

sysdb_set net.email.smtpsvr	 s   smtp.gmail.com
sysdb_set net.email.id       s   choissi
sysdb_set net.email.passwd   s   

sysdb_set net.email.testmail s   choissi@ajou.ac.kr

sysdb_set usr.U0.email  s   choissi@itxsecurity.com

#endif

static NF_MAIL_SEND_INTERNAL _conf_dvrlink = {
/* smtp_server[256];*/	{"SMTP.DVRLINK.NET",
/* smtp_port;		*/		25,
/* smtp_security;   */		0,
/* smtp_user[256];  */		"test",
/* smtp_passwd[256];*/		"test4smtp"
						},
						{
/* from[256];	    */		"test@dvrlink.net",
/* to_cnt;          */		1,
/* to[to_cnt][256]; */		{ "choissi@itxsecurity.com" },
/* subject[256];    */		"SMTP test subject",
/* contents[4096];  */		"SMTP test contents",
/* image_name[256]; */		"",
/* image_size;      */		0,
/* *image_data;     */		NULL,
/* callback func 	*/		NULL,
/* callback arg		*/		NULL,
/* is_dvr_event		*/		0
						}
};

#ifdef DEBUG_MAIL_SEND_JBSHELL

static char *_send_status_string[] = {
	"SUCCESS",
	"FAILED",
	"START"
};

void _mail_send_test_cb( gpointer user_data, 
							NF_MAIL_SEND_STATUS status, 
							const char* svr_msg)
{
	NF_MAIL_SEND_CONTENT *pCont = (NF_MAIL_SEND_CONTENT *)user_data;
	
	//g_return_if_fail(pCont);		
	g_message("%s to[%s] status[%s] svr_msg[%s]", __FUNCTION__, 
			pCont->to[0], 
			(status <= NF_MAIL_SEND_STATUS_START) ? _send_status_string[status] : "UNKNOWN", 
			svr_msg);	
}

static char mail_send_help[] = "mail_send [subject] [email]";
static int mail_send(int argc, char **argv)
{		
	//nf_debug_hexdump(&_conf_dvrlink, sizeof(NF_MAIL_SEND_INTERNAL));		
	_conf_dvrlink.content.cb_func = _mail_send_test_cb;
	_conf_dvrlink.content.cb_arg = &_conf_dvrlink.content;

	if( argc>1)
		snprintf( _conf_dvrlink.content.subject, 
					sizeof(_conf_dvrlink.content.subject), 
					argv[1] );
					
	if( argc>2)
		snprintf( _conf_dvrlink.content.to[0], 
					sizeof(_conf_dvrlink.content.to[0]), 
					argv[2] );			

	_dump_minfo( &_conf_dvrlink );
	
	_mail_send_request(&_conf_dvrlink);
	return 0;
}
__commandlist(mail_send, "mail_send",  mail_send_help, mail_send_help);

#endif
