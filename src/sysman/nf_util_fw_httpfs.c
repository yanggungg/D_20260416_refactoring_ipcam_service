#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ne_request.h>
#include <libgen.h>

#include "nf_util_fw_httpfs.h"
#include "nf_util_fw.h"

#define DEBUG_JBSHELL_FLASH

#ifdef DEBUG_JBSHELL_FLASH
	#include "jbshell.h"
#endif

#define HTTP_DEFAULT_PORT			80
#define HTTPS_DEFAULT_PORT			443

#define DEBUG_FWUP

GThread *_fw_httpfs_thread;

/** Gloval Function Definition **/
static gboolean _nf_fw_httpfs_url_check_start(NF_FW_HTTPFS_CLIENT_REQ *req);
static gboolean _nf_fw_httpfs_upgrade(gchar *filename, gboolean is_nonblocking);
static gboolean _nf_fw_httpfs_chk_url(NF_FW_HTTPFS_CLIENT *httpfs,  NF_FW_IMAGE_LIST *fw_header);
static gboolean _nf_fw_httpfs_chk_fwheader(NF_FW_HTTPFS_CLIENT *httpfs, NF_FW_IMAGE_LIST *header);
static void _nf_fw_httpfs_cb_call(NF_FW_HTTPFS_CLIENT *http, NF_FW_HTTPFS_CLIENT_PRGT_E status, 
										gint is_error, guint current, guint total);

extern char *_neon_res_to_str(gint res);

/**
	Function Start!!
**/
static gboolean _nf_fw_httpfs_upgrade(gchar *filename, gboolean is_nonblocking)
{
	nf_fw_upgrade_thread_start(filename, is_nonblocking);

	return TRUE;
}

static int _nf_fw_httpfs_body_reader_cb(void *userdata, const char *block, size_t length)
{
	NF_FW_HTTPFS_CLIENT *httpfs = (NF_FW_HTTPFS_CLIENT *)userdata;
	guint total=0, percent=0;
	guint64 content_size=0;
	static gint cnt=0;
	gint is_error=0;

	total=httpfs->prgt.total;
	content_size=httpfs->content_size;
	httpfs->read_size+=length;

	percent= 60 + ((((guint)httpfs->read_size / (guint)content_size) * 100) / 5);

	is_error=NF_FW_HTTPFS_PRGT_NO_ERROR;
	_nf_fw_httpfs_cb_call( httpfs, NF_FW_HTTPFS_PRGT_CONTINUE, is_error,
							(guint32)percent, (guint32)total);

	cnt++;

	return NE_OK;
}

static void _nf_fw_httpfs_thread_func(gpointer data)
{
	NF_FW_HTTPFS_CLIENT_REQ *req=(NF_FW_HTTPFS_CLIENT_REQ *)data;

	_nf_fw_httpfs_url_check_start(req);
}

gboolean nf_fw_httpfs_request_url(NF_FW_HTTPFS_CLIENT_REQ *req)
{
	_fw_httpfs_thread = g_thread_create((GThreadFunc)_nf_fw_httpfs_thread_func, (gpointer)req, FALSE, NULL);

	if(_fw_httpfs_thread == NULL)
	{
		g_warning("%s Httpfs Firmware Upgrade Thread Create Fail!!!", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

static gboolean _nf_fw_httpfs_url_check_start(NF_FW_HTTPFS_CLIENT_REQ *req)
{
	guint			current, total=0;
	gboolean		is_debug=FALSE;
	gint			is_error=0;
	NF_FW_IMAGE_LIST fw_header;
	NF_FW_HTTPFS_CLIENT httpfs;
	NF_FW_IMAGE_HEADER header;
	char fw_file_path[64] = {0,};

	memset( &httpfs, 0x00, sizeof(NF_FW_HTTPFS_CLIENT));
	memcpy( &httpfs.req, req, sizeof(NF_FW_HTTPFS_CLIENT_REQ));

	is_debug = httpfs.req.is_debug;
	total = httpfs.prgt.total = 100;
	#if defined(DEBUG_FWUP)
			printf("\033[0;35m [HDD_FW_UP_DEBUG] %s [%s] \033[0;39m\n", __FUNCTION__, httpfs.req.url);
	#endif
	current=0;
	is_error=NF_FW_HTTPFS_PRGT_NO_ERROR;
	_nf_fw_httpfs_cb_call(&httpfs, NF_FW_HTTPFS_PRGT_START, is_error, current, total); 
	if(strstr(httpfs.req.url,NET_FWUP_MOUNT_DIR_NAME) != NULL)
	{
		sprintf(fw_file_path,"%s/%s",NET_FWUP_MOUNT_DIR_NAME,basename(httpfs.req.url));
		#if defined(HDD_FW_UP_DEBUG)
			printf("\033[0;35m [HDD_FW_UP_DEBUG] %s fw_file_path [%s] \033[0;39m\n", __FUNCTION__,fw_file_path);
		#endif
		if(!nf_fw_get_file_header(fw_file_path,&header))
		{
			return FALSE;
		}
		memcpy(&fw_header.fwheader, &header, sizeof(NF_FW_IMAGE_HEADER));
	}
	else
	{
		#if defined(HDD_FW_UP_DEBUG)
			printf("\033[0;35m [HDD_FW_UP_DEBUG] %s HTTP \033[0;39m\n", __FUNCTION__);
		#endif
	if(!_nf_fw_httpfs_chk_url(&httpfs, &fw_header))
		return FALSE;
	}

	current=90;
	_nf_fw_httpfs_cb_call(&httpfs, NF_FW_HTTPFS_PRGT_CONTINUE, is_error, current, total);
	if(!_nf_fw_httpfs_chk_fwheader(&httpfs, &fw_header))
	{
		g_warning("%s fwversion different", __FUNCTION__);
		is_error=NF_FW_HTTPFS_PRGT_INVALID_FW;
		goto nf_fw_httpfs_request_fail;
	}

	current=100;
	_nf_fw_httpfs_cb_call(&httpfs, NF_FW_HTTPFS_PRGT_RET_OK, is_error, current, total);

	return TRUE;

nf_fw_httpfs_request_fail:

	_nf_fw_httpfs_cb_call(&httpfs, NF_FW_HTTPFS_PRGT_RET_FAILED, is_error, current, total);

	return FALSE;
}

gboolean nf_fw_httpfs_url_check_start(NF_FW_HTTPFS_CLIENT_REQ *req)
{
	return _nf_fw_httpfs_url_check_start(req);
}

static gboolean _nf_fw_httpfs_chk_url(NF_FW_HTTPFS_CLIENT *httpfs,  NF_FW_IMAGE_LIST *fw_header)
{
	const char		*content_length;
	gchar 			*url;
	gchar			buffer[NF_FW_HTTPFS_RX_BUFFER]={0, };
	gint			timeout=0, status_code=0, rd_len=0, tot_len=0, res=0;
	guint			current, total=0;
	guint64			content_size=0;
	gboolean		is_debug=FALSE;
	ne_uri			uri;
	gint			is_error=0;

	total = httpfs->prgt.total;
	is_debug = httpfs->req.is_debug;
	url=httpfs->req.url;
	timeout=httpfs->req.timeout_connect_sec;
	is_error=NF_FW_HTTPFS_PRGT_NO_ERROR;

	if (ne_uri_parse ( httpfs->req.url , &uri) != 0)
	{
		is_error=NF_FW_HTTPFS_PRGT_FILE_NOT_FOUND;
		goto nf_fw_httpfs_fail;
	}

	if(!uri.scheme)
	{
		is_error=NF_FW_HTTPFS_PRGT_FILE_NOT_FOUND;
		goto nf_fw_httpfs_fail;
	}

	if (uri.port == 0) {
		if (!strcmp (uri.scheme, "https"))
			uri.port = HTTPS_DEFAULT_PORT;
		else
			uri.port = HTTP_DEFAULT_PORT;
	}

	current=20;
	_nf_fw_httpfs_cb_call(httpfs, NF_FW_HTTPFS_PRGT_CONTINUE, is_error, current, total);

	if(is_debug)
	{
		g_print("=========================================================================================================\n");
		g_print("[FW HTTPFS DBG MSG]\n");
		g_print("url             [%s]\n", url);
		g_print("uri.scheme      [%s]\n", uri.scheme);
		g_print("host            [%s]\n", uri.host);
		g_print("port            [%d]\n", uri.port);
		g_print("path            [%s]\n", uri.path);
		g_print("=========================================================================================================\n");
	}

	httpfs->session = ne_session_create(uri.scheme, uri.host, uri.port);
	if(httpfs->session == NULL)
	{
		is_error=NF_FW_HTTPFS_PRGT_NET_REQUEST_FAIL;
		goto nf_fw_httpfs_fail;
	}

	ne_set_read_timeout(httpfs->session, timeout);

	current=30;
	_nf_fw_httpfs_cb_call(httpfs, NF_FW_HTTPFS_PRGT_CONTINUE, is_error, current, total);

	#if 0
		httpfs->request = ne_request_create(httpfs->session, "HEAD", uri.path);
	#else
		httpfs->request = ne_request_create(httpfs->session, "GET", uri.path);
	#endif

	#if 1
		ne_add_response_body_reader(httpfs->request, ne_accept_always, _nf_fw_httpfs_body_reader_cb, httpfs);
		res = ne_begin_request (httpfs->request);
		if( res != NE_OK )
		{
			g_warning("%s begin_request failed res[%d](%s)", __FUNCTION__, res, _neon_res_to_str(res));

			is_error=NF_FW_HTTPFS_PRGT_NET_REQUEST_FAIL;
			goto nf_fw_httpfs_fail;
		}
	#else
		if (ne_request_dispatch(httpfs->request) != NE_OK)
		{
			ne_session_destroy(httpfs->session);

			is_error=NF_FW_HTTPFS_PRGT_NET_REQUEST_FAIL;
			goto nf_fw_httpfs_fail;
		}
	#endif

	if((status_code=ne_get_status(httpfs->request)->code) != 200)
	{
		g_print("Response Status Code Was %d.. fail!!\n", status_code);

		if(status_code == 404)		
			is_error=NF_FW_HTTPFS_PRGT_FILE_NOT_FOUND;
		else
			is_error=NF_FW_HTTPFS_PRGT_FILE_INVALID;

		goto nf_fw_httpfs_release;
	}
	else
		g_print("Response Status Code Was %d\n", status_code);

	current=40;
	_nf_fw_httpfs_cb_call(httpfs, NF_FW_HTTPFS_PRGT_CONTINUE, is_error, current, total);

	httpfs->content_length = ne_get_response_header (httpfs->request, "Content-Length");
	if (httpfs->content_length)
	{
		httpfs->content_size = g_ascii_strtoull (httpfs->content_length, NULL, 10);
		content_size=httpfs->content_size;
	}

	tot_len=NF_FW_HTTPFS_RX_BUFFER;

	if(((guint64)tot_len > content_size) || (content_size == 0))
	{
		g_print("%s Size Error!! content_size[%lld] totlen[%d]\n", __FUNCTION__, content_size, tot_len);
		goto nf_fw_httpfs_release;
	}

	memset(buffer, 0x0, (size_t)tot_len + 1);

	if (ne_get_status(httpfs->request)->klass == 2) 
	{
		//... call ne_read_response_block() loop...
		gint rd_len=0;
		while(tot_len > 0)
		{
			res = ne_read_response_block(httpfs->request, buffer+rd_len, (size_t)tot_len );

			if(res == 0)
			{
				ne_discard_response(httpfs->request);
				break;
			}
			else
			{
				tot_len -= res;
				rd_len += res;
			}
		}
	}
	else 
	{
		g_print("ne_get_status error!!\n");
		ne_discard_response(httpfs->request);
	}

	// copy firmware header
	memcpy(fw_header, buffer, sizeof(NF_FW_IMAGE_LIST));

	current=80;
	_nf_fw_httpfs_cb_call(httpfs, NF_FW_HTTPFS_PRGT_CONTINUE, is_error, current, total);

	ne_request_destroy(httpfs->request);
	ne_session_destroy(httpfs->session);

	return TRUE;

nf_fw_httpfs_release:

	ne_request_destroy(httpfs->request);
	ne_session_destroy(httpfs->session);

nf_fw_httpfs_fail:

	g_warning("Httpfs URL Check Fail!!");

	_nf_fw_httpfs_cb_call(httpfs, NF_FW_HTTPFS_PRGT_RET_FAILED, is_error, current, total);

	return FALSE;
}

static gboolean _nf_fw_httpfs_chk_fwheader(NF_FW_HTTPFS_CLIENT *httpfs, NF_FW_IMAGE_LIST *header)
{
	gboolean is_debug=httpfs->req.is_debug;
	gchar tmp_key[256]={0, }, *fwver, product[16]={0, }, proto[16]={0, }, minor[16]={0, }, vendor[16]={0, };
	gchar **str;

	sprintf(tmp_key, "sys.info.swver");
	fwver = nf_sysdb_get_str_nocopy(tmp_key);
	if(!fwver)
		goto nf_fw_httpfs_chk_fwheader_fail_type0;

	if(strncmp(header->fwheader.magic, FW_UPGRADE_NF_MAGIC_S, sizeof(header->fwheader.magic)) != 0)
	{
		g_warning("%s Firmware Magic Check Fail!!", __FUNCTION__);
		goto nf_fw_httpfs_chk_fwheader_fail_type0;
	}
	else
	{
		str=g_strsplit(fwver, ".", 4);

		strncpy(product, *str, sizeof(product));
		strncpy(proto, *(str+1), sizeof(proto));
		strncpy(minor, *(str+2), sizeof(minor));
		strncpy(vendor, *(str+3), sizeof(vendor));

		if((strncmp(product, header->fwheader.version, strlen(product)) != 0) 
					|| (strcmp(vendor, header->fwheader.vendor) != 0))
		{
			g_warning("Different Frmware Version [%s] -> [%s %s %s %s] vendor[%s -> %s]", 
							header->fwheader.version, product, proto, minor, vendor, vendor, header->fwheader.vendor);

			g_strfreev(str);

			goto nf_fw_httpfs_chk_fwheader_fail_type1;
		}

		if(is_debug)
		{
			g_print("===================================\n");
			g_print("Magic Check Success\n");
			g_print("version      [%s]\n", header->fwheader.version);
			g_print("model      [%s]\n", header->fwheader.model);
			g_print("vendor     [%s]\n", header->fwheader.vendor);
			g_print("===================================\n");
		}

		memcpy(httpfs->fwver, header->fwheader.version, sizeof(header->fwheader.version));
	}

	g_strfreev(str);

	return TRUE;

nf_fw_httpfs_chk_fwheader_fail_type0:

	memcpy(httpfs->fwver, "xxx.xxx.xxx.xxx", sizeof(header->fwheader.model));

	return FALSE;

nf_fw_httpfs_chk_fwheader_fail_type1:

	memcpy(httpfs->fwver, header->fwheader.version, sizeof(header->fwheader.version));

	return FALSE;
}

static void _nf_fw_httpfs_cb_call(NF_FW_HTTPFS_CLIENT *httpfs, NF_FW_HTTPFS_CLIENT_PRGT_E status, 
										gint is_error, guint current, guint total)
{
	httpfs->prgt.status = status;
	httpfs->prgt.is_error = is_error;
	httpfs->prgt.current = current;
	httpfs->prgt.total = total;

	if(httpfs->req.is_debug)
		g_print("CallBack Debug!! status[%d] is_error[%d] current[%d] total[%d]\n", status, is_error, current, total);

	if(httpfs->req.cb_arg)
		memcpy((gchar *)httpfs->req.cb_arg, httpfs->fwver, sizeof(httpfs->fwver));

	if(httpfs->req.cb_func)
		httpfs->req.cb_func( &httpfs->prgt, httpfs->req.cb_arg);

	return;
}

static void nf_fw_httpfs_prg_cbfunc(NF_FW_HTTPFS_PRGT *prgt, gpointer context)
{
	g_print("%s state[%d] current[%d] total[%d]\n", __FUNCTION__, prgt->status, prgt->current, prgt->total);
}

#ifdef DEBUG_JBSHELL_FLASH
static char nf_fw_httpfs_upgrade_test_help [] = "fwuph up [filename]\n"
												"      chk_url [filename]";
static int nf_fw_httpfs_upgrade_test(int argc, char **argv)
{   
	if(argc < 2)
		goto nf_fw_httpfs_jbshll_fail;

	if(strcmp(argv[1], "up") == 0)
	{
		if(!_nf_fw_httpfs_upgrade(argv[2], TRUE))
		{
			g_warning("%s Httpfs Firmware Upgrade Fail!!", __FUNCTION__);
			return -1;
		}
	}
	else if(strcmp(argv[1], "chk_url") == 0)
	{
		NF_FW_HTTPFS_CLIENT_REQ client;

		if(argc < 3)
			goto nf_fw_httpfs_jbshll_fail;

		strcpy(client.url, argv[2]);
		client.timeout_connect_sec=30;
		client.is_debug=TRUE;
		client.cb_func=nf_fw_httpfs_prg_cbfunc;
		nf_fw_httpfs_request_url(&client);
	}

	return 0;

nf_fw_httpfs_jbshll_fail:

	printf("Invalid arguments\n%s\n", nf_fw_httpfs_upgrade_test_help);
	return -1;
}
__commandlist(nf_fw_httpfs_upgrade_test, "fwuph", "fwuph", nf_fw_httpfs_upgrade_test_help);
#endif

