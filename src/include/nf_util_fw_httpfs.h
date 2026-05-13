#ifndef __NF_UTIL_FW_HTTPFS_H__
#define __NF_UTIL_FW_HTTPFS_H__

#include "nf_common.h"
#include "nf_util_fw.h"

#define NF_FW_HTTPFS_RX_BUFFER		sizeof(NF_FW_IMAGE_LIST)

typedef enum _NF_FW_HTTPFS_CLIENT_PRGT_E
{
	NF_FW_HTTPFS_PRGT_START			= 0,
	NF_FW_HTTPFS_PRGT_CONTINUE		= 1,
	NF_FW_HTTPFS_PRGT_RET_OK		= 2,
	NF_FW_HTTPFS_PRGT_RET_FAILED	= 3
} NF_FW_HTTPFS_CLIENT_PRGT_E;

typedef enum _NF_FW_HTTPFS_CLIENT_PRGT_ERROR_E
{
	NF_FW_HTTPFS_PRGT_NO_ERROR				= 0,
	NF_FW_HTTPFS_PRGT_FILE_NOT_FOUND		= 1,
	NF_FW_HTTPFS_PRGT_FILE_INVALID			= 2,
	NF_FW_HTTPFS_PRGT_NET_REQUEST_FAIL		= 3,
	NF_FW_HTTPFS_PRGT_INVALID_FW			= 4
} NF_FW_HTTPFS_CLIENT_PRGT_ERROR_E;

typedef struct _NF_FW_HTTPFS_PRGT_T
{
	NF_FW_HTTPFS_CLIENT_PRGT_E	status;

	gint    is_error;              // OK:0 //ERR:1

	guint   current;
	guint   total;
} NF_FW_HTTPFS_PRGT;


typedef void (*fwup_httpfs_fxn_t)(NF_FW_HTTPFS_PRGT *prgt, gpointer context);

typedef struct _NF_FW_HTTPFS_CLIENT_REQ {

	gchar       url[1023+1];

	#if 0
		gchar       user_agent[63+1];
	#endif

	gint        timeout_connect_sec;
	#if 0
		gint        timeout_rx_sec;
		gint        timeout_tx_sec;
	#endif

	/* authentication */ 
	gboolean    is_auth; 
	gchar       username[63+1];
	gchar       passwd[63+1];

	/* progress cb function */
	fwup_httpfs_fxn_t	cb_func;
	gpointer            cb_arg;

	gboolean    is_debug;
	gpointer    reserved[4];
	#if 0
		gchar       out_filename[1023+1];   // for nonblocking
	#endif

} NF_FW_HTTPFS_CLIENT_REQ;

typedef struct _NF_FW_HTTPFS_CLIENT {
	NF_FW_HTTPFS_CLIENT_REQ req;

	/* neon internal*/
	ne_session  *session;
	ne_request  *request;

	ne_uri      uri;
	ne_uri      proxy;

	NF_FW_HTTPFS_PRGT prgt;

	guint64     content_size;
	guint64     read_size;

	const char        *content_length;
	
	gchar       fwver[32];

} NF_FW_HTTPFS_CLIENT;


gboolean nf_fw_httpfs_request_url(NF_FW_HTTPFS_CLIENT_REQ *req);
gboolean nf_fw_httpfs_url_check_start(NF_FW_HTTPFS_CLIENT_REQ *req);

#endif

