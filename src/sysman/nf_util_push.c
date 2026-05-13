#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <glib.h>
// #include <gst/gst.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <errno.h> //For errno - the error number
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <time.h>
#include "nf_common.h"
#include "nf_common_util.h"

#include "nf_util_http.h"
#include "nf_util_push.h"


#include "nf_debug.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "util_push"

#define DEBUG_UTIL_PUSH_SEND_LOG
#define	DEBUG_UTIL_PUSH_SEND_JBSHELL
//#define DEBUG_MAIL_SEND_MONITOR_CB
//#define DEBUG_DIRECT_MAIL_SEND

#define PPP g_message("PUSH - %s - %d", __FUNCTION__, __LINE__);

static GQuark
_nf_util_push_send_error_quark (void)
{
	return g_quark_from_static_string ( G_LOG_DOMAIN "-error-quark");
}

#define NF_UTIL_PUSH_SEND_ERROR 	_nf_util_push_send_error_quark()



#ifdef DEBUG_UTIL_PUSH_SEND_JBSHELL
	#include "jbshell.h"
#endif


typedef enum _DEBUG_UTIL_PUSH_IDX_E
{
	DEBUG_UTIL_PUSH_IDX_INIT				= 0,
	DEBUG_UTIL_PUSH_IDX_NR					= 1
}DEBUG_UTIL_PUSH_IDX_E;

static const char *_DEBUG_UTIL_PUSH_str[32] =
{
	"DEBUG_UTIL_PUSH_IDX_INIT",
	"DEBUG_UTIL_PUSH_IDX_LIBESMTP",
	"DEBUG_UTIL_PUSH_IDX_QUEUE",
	"DEBUG_UTIL_PUSH_IDX_REQUEST",

	"DEBUG_UTIL_PUSH_IDX_TEST",
	"DEBUG_UTIL_PUSH_IDX_REQUEST_RAW",
	"DEBUG_UTIL_PUSH_IDX_NR"
};

static gint _DEBUG_UTIL_PUSH_log[32] =
{
	1,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};


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

static void nf_util_push_class_init (NfUtilPushClass * klass);
static void nf_util_push_instance_init (GTypeInstance * instance, gpointer g_class);

static void nf_util_push_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void nf_util_push_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void nf_util_push_dispose (GObject * object);
static void nf_util_push_finalize (GObject * object);

static GObjectClass *parent_class = NULL;
static NfUtilPush	*_nf_util_push = NULL;

static void _nf_util_push_thread_func (NfUtilPush * self);

#define PUSH_SERVER		"ipex.sequrinet.com"

GType
nf_util_push_get_type (void)
{
	static GType nf_util_push_type = 0;

	if (G_UNLIKELY (nf_util_push_type == 0)) {
		static const GTypeInfo object_info = {
			sizeof (NfUtilPushClass),
			NULL,
			NULL,
			(GClassInitFunc) nf_util_push_class_init,
			NULL,
			NULL,
			sizeof (NfUtilPush),
			0,
			(GInstanceInitFunc) nf_util_push_instance_init,
			NULL
		};

		nf_util_push_type =
			g_type_register_static (NF_TYPE_OBJECT, "NfUtilPush", &object_info, 0);
	}

	return nf_util_push_type;
}

static void
nf_util_push_class_init (NfUtilPushClass * klass)
{
	GObjectClass *gobject_class;
	int i;

	gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->set_property = nf_util_push_set_property;
	gobject_class->get_property = nf_util_push_get_property;

	gobject_class->dispose = nf_util_push_dispose;
	gobject_class->finalize = nf_util_push_finalize;
}

static void
nf_util_push_instance_init (GTypeInstance* instance, gpointer g_class)
{
	NfUtilPush *self = NF_UTIL_PUSH (instance);

	self->init_done	= 0;

	// queue ����
	self->queue = g_async_queue_new();

	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create(	(GThreadFunc)_nf_util_push_thread_func,
									self, FALSE, NULL);
}

/* dispose is called when the object has to release all links
 * to other objects */
static void
nf_util_push_dispose (GObject * object)
{
	// thread end
	parent_class->dispose (object);
}

/* finalize is called when the object has to free its resources */
static void
nf_util_push_finalize (GObject * object)
{
	parent_class->finalize (object);
}


static void
nf_util_push_set_property (GObject * object, guint prop_id,
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
nf_util_push_get_property (GObject * object, guint prop_id,
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

/**
	@brief				util_push �ʱ�ȭ
	@return	gboolean	%TRUE on success, %FALSE if an error occurred
*/
gboolean
nf_util_push_init( int wait )
{
	gboolean ret = TRUE;
	gint r = 0;

	g_return_val_if_fail (_nf_util_push == NULL, FALSE);

	_nf_util_push = g_object_new ( NF_TYPE_UTIL_PUSH , NULL);

	nf_debug_category_add( "push", _DEBUG_UTIL_PUSH_str, _DEBUG_UTIL_PUSH_log, DEBUG_UTIL_PUSH_IDX_NR);

	if( wait )
	{
		while( _nf_util_push->init_done != 1)
			g_usleep(10*1000);
	}

	return ret;
}

static void _util_push_request(NF_UTIL_PUSH_CONTENT *cont)
{
	NF_HTTP_CLIENT_REQ req;

	int ret;
	int in_len = 0, ret_len = 0;
	char in_buf[4096];
	char *mac = NULL;
	char *ret_buf = NULL;

	memset(&req, 0x00, sizeof(NF_HTTP_CLIENT_REQ));

	//snprintf( req.url, sizeof(req.url)-1, "http://%s/push?method=sendText", PUSH_SERVER);
    snprintf( req.url, sizeof(req.url)-1, "http://%s/service/v2/push/send/message", PUSH_SERVER );
	req.is_debug = 0;
	req.timeout_connect_sec = 5;
	req.timeout_rx_sec = req.timeout_tx_sec = 5;

	in_len = strlen(cont->contents);

	g_message("PUSH API - %s\n\n", cont->contents);

	ret = nf_http_client_post_buff(&req, cont->contents, in_len, &ret_buf, &ret_len, NULL);

#if 1
	if(ret_buf)
		g_message("PUSH RET=%d, STR=%s", ret, ret_buf);
	else
		g_message("PUSH RET=%d", ret);
#endif

	if( ret_buf )
		g_free(ret_buf);
}

static void
_nf_util_push_thread_func (NfUtilPush * self)
{
	gpointer	que_poped_data = NULL;
	g_message("%s start", __FUNCTION__);

	// wait init complete
	while( _nf_util_push == NULL ) g_usleep(10*1000);

	self->init_done = 1;

	while(self->thread_run)
	{
		que_poped_data  = g_async_queue_pop( self->queue);
		if(que_poped_data)
		{

#ifdef DEBUG_UTIL_PUSH_LOG
			if( _DEBUG_UTIL_PUSH_log[DEBUG_UTIL_PUSH_IDX_QUEUE] )
			{

			}
#endif
			_util_push_request(que_poped_data);

			g_free(que_poped_data);
			g_usleep(10*1000);
		}
	}
	g_message("%s end", __FUNCTION__);
}

gboolean
nf_util_push_request(NF_UTIL_PUSH_CONTENT *cont, GError **error)
{
	gint err_code = NF_UTIL_PUSH_ERROR_SUCCESS;
	gint q_len = 0;
/*
	g_return_val_if_fail(_nf_util_push != NULL, NF_UTIL_PUSH_ERROR_INIT);
	g_return_val_if_fail(cont != NULL, NF_UTIL_PUSH_ERROR_MALLOC);
*/
	if(!_nf_util_push){
		err_code = NF_UTIL_PUSH_ERROR_INIT;
		goto ret_error;
	}

	if(!cont){
		err_code = NF_UTIL_PUSH_ERROR_MALLOC;
		goto ret_error;
	}

	q_len = g_async_queue_length(_nf_util_push->queue);

	if (q_len > NF_UTIL_PUSH_MAX_QUEUE_LEN) {
		err_code = NF_UTIL_PUSH_ERROR_MAX_QUE_LEN;
	}
	else
	{
		NF_UTIL_PUSH_CONTENT *data = g_malloc(sizeof(NF_UTIL_PUSH_CONTENT));
		memcpy(data, cont, sizeof(NF_UTIL_PUSH_CONTENT));

		g_async_queue_push(_nf_util_push->queue, data);
		err_code = NF_UTIL_PUSH_ERROR_SUCCESS;
	}

	if(err_code != NF_UTIL_PUSH_ERROR_SUCCESS)
		goto ret_error;

	return 1;

ret_error:

	g_warning("%s failed ret[%d]",__FUNCTION__, err_code);
	g_set_error (error,
					NF_UTIL_PUSH_SEND_ERROR ,	/* error domain */
					err_code ,					/* error code */
					"util push error code[%d]",	/* error message format string */
					err_code);

	return 0;
}

/*
http://14.63.169.122/push?method=send
POST
Accept:application/json
Content-Type:application/json
{
	"macAddress" : "xxxxx",
	"messageCode" : 0
}


{
  "mac" : "112233445566",
  "msg": "MOTION : %1 %2 %3",
  "type": "MOTION",
  "params" : [
        1,
        "abcde",
        3
  ],
  "date" : {
    "unixtime" : 12345678,
    "datestr" : "2014-03-28 05:14:35"
  }
}

http://14.63.169.122/push?method=sendText
POST
Accept:application/json
Content-Type:application/json
{
  "macAddress" : "xxxx",
  "alert" : "ǥ��� �޽���" ,
  "data" : ""
}
*/

#ifdef DEBUG_UTIL_PUSH_SEND_JBSHELL

static char push_send_int_help[] = "push_send_int [int]";
static int jbshell_push_send_int(int argc, char **argv)
{

	if(argc < 2){
		printf("%s\n",push_send_int_help);
		return -1;
	}


	return 0;
}
__commandlist(jbshell_push_send_int,"push_send_int", push_send_int_help, push_send_int_help);



static char push_send_text_help[] = "push_send_text [alert] [data]";
static int jbshell_push_send_text(int argc, char **argv)
{

	if(argc < 3){
		printf("%s\n",push_send_text_help);
		return -1;
	}


	return 0;
}
__commandlist(jbshell_push_send_text,"push_send_text", push_send_text_help, push_send_text_help);


#endif
