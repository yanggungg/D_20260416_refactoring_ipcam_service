#include <curl/curl.h>
#include <stdlib.h>
#include <jansson.h>
#include <glib.h>
#include "xmm.h"
#include "cmm.h"
#include "iux_afx.h"
#include "ix_queue.h"
#include "ix_func.h"

#define DMSG(level, format, args...) \
	do { \
		if (DBG_LEVEL && level && DBG_LEVEL >= level) { \
			fprintf(stderr, "[IUX:"DBG_MODULE"] %s():%d: "format"\n", __FUNCTION__, __LINE__, ##args); \
		} \
	} while (0)

#define DBG_LEVEL		0
#define DBG_MODULE		"XMM"

#define XMM_LOCK()		g_mutex_lock(ixmg.mtx)
#define XMM_UNLOCK()	g_mutex_unlock(ixmg.mtx)

typedef struct _XMM_MANAGER_T XMG_T;
struct _XMM_MANAGER_T 
{
    IXQueue *q;
    GThread *main_thd;
    GMutex *mtx;
    int sleep_time;
    int max_working_cnt;
    int cur_working_cnt;
};

typedef struct _XMM_WORK_T XMW_T;
struct _XMM_WORK_T
{
    XMM_CMD_TYPE type;
    char hostname[256];
    int http_port;
    XCT_T content;
    xmm_result_cb cb_func;
    int retry_cnt;
};

static XMG_T ixmg;

static XMM_SESSION_INFO_T ixmm[XMM_DEVICE_INFO_MAX_CNT] = {0,};
static XMM_SESSION_INFO_T master;


/////////////////////////////////////////////////
//private
/////////////////////////////////////////////////

static void _print_cookie_data(XMM_COOKIE_T *data)
{
    DMSG(9, "////////xmm cookie data/////////");
    DMSG(9, "session_id : %s", data->session_id);
    DMSG(9, "cookie_id : %s", data->id);
    DMSG(9, "grp : %s", data->grp);
    DMSG(9, "permision : %d", data->permision);
}

static void _print_session_data(XMM_SESSION_INFO_T *data)
{
    DMSG(9, "//////////////xmm session data///////////////");
    DMSG(9, "hostname : %s", data->hostname);
    DMSG(9, "id : %s", data->id);
    DMSG(9, "pw : %s", data->pw);
    DMSG(9, "port : %d", data->port);
    _print_cookie_data(&data->cookie);
    DMSG(9, "/////////////////////////////////////////////\n");
}

static size_t _xmm_request_write_data(void *ptr, size_t size, size_t nmemb, void *stream) 
{
    int req_size = (size*nmemb);
    int writeble_size = 0;
    
    XMM_CURL_RET *curl_ret = (XMM_CURL_RET *)stream;
    
    if( curl_ret->data_len + req_size <= XMM_CURL_RET_MAX_DATA_SIZE){
    	writeble_size = req_size;
    }else {
    	writeble_size = curl_ret->data_len + req_size - XMM_CURL_RET_MAX_DATA_SIZE;
    }
    	   
   if( writeble_size >0 )  {
    	memcpy( &curl_ret->data[curl_ret->data_len] , ptr, writeble_size); 
    	curl_ret->data_len += writeble_size;    	    	
    } 
    
    return writeble_size;
}

static int _send_msg_data(char *url, char *post_field, char *cookie, XMM_CURL_RET *curl_ret)
{
    CURL *curl = NULL;	
    CURLcode res;			
    struct curl_slist *headerlist = NULL;
	char err_str[CURL_ERROR_SIZE];
	int ret = -1;	
	
    curl = curl_easy_init();
    
    headerlist = curl_slist_append(headerlist, "Content-Type: application/json");

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_str);
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookie);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);			
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(post_field));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _xmm_request_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_ret);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret);
        }
		
		curl_easy_cleanup(curl);
    }
	else{
		printf("%s - curl init error", __FUNCTION__);
	}	

	if (headerlist) {
        curl_slist_free_all(headerlist);
	}

	DMSG(8, "result : %d", ret);
	
	return ret;
}

static int _send_msg_get(char *url, XMM_CURL_RET *curl_ret)
{
    CURL *curl = NULL;	
    CURLcode res;			
    struct curl_slist *headerlist = NULL;
	char err_str[CURL_ERROR_SIZE];
	int ret = -1;	
	
    curl = curl_easy_init();
    
    headerlist = curl_slist_append(headerlist, "Content-Type: application/json");

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_str);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);			
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _xmm_request_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_ret);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret);
        }

		
		curl_easy_cleanup(curl);
    }
	else{
		printf("%s - curl init error", __FUNCTION__);
	}	

	if (headerlist) {
        curl_slist_free_all(headerlist);
	}

	DMSG(8, "result : %d", ret);
	
	return ret;
}

static int _send_msg_get_with_cookie(char *url, char *post_field, char *cookie, XMM_CURL_RET *curl_ret)
{
    CURL *curl = NULL;	
    CURLcode res;			
    struct curl_slist *headerlist = NULL;
	char err_str[CURL_ERROR_SIZE];
	int ret = -1;	
	
    curl = curl_easy_init();
    
    headerlist = curl_slist_append(headerlist, "Content-Type: application/json");

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_str);
        curl_easy_setopt(curl, CURLOPT_COOKIE, cookie);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);			
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _xmm_request_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_ret);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret);
        }

		curl_easy_cleanup(curl);
    }
	else{
		printf("%s - curl init error", __FUNCTION__);
	}	

	if (headerlist) {
        curl_slist_free_all(headerlist);
	}

	DMSG(8, "result : %d", ret);
	
	return ret;
}

static int _send_msg_post(char *url, char *post_field, XMM_CURL_RET *curl_ret)
{
    CURL *curl = NULL;	
    CURLcode res;			
    struct curl_slist *headerlist = NULL;
	struct curl_httppost *form_post = NULL;
	char err_str[CURL_ERROR_SIZE];
	int ret = -1;	
	
    curl = curl_easy_init();
    
    headerlist = curl_slist_append(headerlist, "Content-Type: application/json");

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _xmm_request_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl_ret);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err_str);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);			
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ret);
        }
		
		curl_easy_cleanup(curl);
		
		if(form_post) {
			 curl_formfree(form_post);
		}
    }
	else{
		printf("%s - curl init error", __FUNCTION__);
	}	

	if (headerlist) {
        curl_slist_free_all(headerlist);
	}

	DMSG(8, "result : %d", ret);
	
	return ret;
}

static int _get_data_idx(char *hostname, int port)
{
    int i;

    DMSG(1, "target hostname : %s, port : %d", hostname, port);

    if (strlen(hostname) == 0) return -1;

    for (i = 0; i < XMM_DEVICE_INFO_MAX_CNT; i++)
    {
        DMSG(1, "idx : %d, hostname : %s, port : %d", i, ixmm[i].hostname, ixmm[i].port);
        if (!strcmp(ixmm[i].hostname, hostname) && ixmm[i].port == port) {
            DMSG(1, "Find index : %d", i);
            return i;
        }
    }

    if (i == XMM_DEVICE_INFO_MAX_CNT) {
        DMSG(1, "Can't find idx");
        return -1;
    }
}

static int _get_session_id_idx(char *hostname, int port)
{
    int i;

    DMSG(8, "target hostname : %s, port : %d", hostname, port);

    for (i = 0; i < XMM_DEVICE_INFO_MAX_CNT; i++)
    {
//        DMSG(8, "idx : %d, hostname : %s, port : %d", i, ixmm[i].hostname, ixmm[i].port);
        if (!strcmp(ixmm[i].hostname, hostname) && ixmm[i].port == port) {
            if (strlen(ixmm[i].cookie.session_id) > 0) return i;
            else return -1;
        }
    }

    if (i == XMM_DEVICE_INFO_MAX_CNT) return -1;
}

static int _get_empty_slot(char *hostname, int port)
{
    int i;

    for (i = 0; i < XMM_DEVICE_INFO_MAX_CNT; i++)
    {
        if (strlen(ixmm[i].hostname) == 0) return i;
    }

    if (i == XMM_DEVICE_INFO_MAX_CNT) return -1;
}

int _make_url(XMM_CMD_TYPE type, char *hostname, int port, char *ret_url, int url_len)
{
    memset(ret_url, 0x00, url_len);

    switch(type)
    {
        case XMM_CMD_LOGIN:
            sprintf(ret_url, "http://%s:%d/cgi-bin/webra.fcgi?login", hostname, port);
            if (strlen(ret_url) > url_len) return -1;
        break;
        
        case XMM_CMD_LOGOUT:
            sprintf(ret_url, "http://%s:%d/cgi-bin/webra.fcgi?logoff", hostname, port);
            if (strlen(ret_url) > url_len) return -1;
        break;
        
        case XMM_CMD_KEEPALIVE:
            sprintf(ret_url, "http://%s:%d/cgi-bin/webra.fcgi?check_session", hostname, port);
            if (strlen(ret_url) > url_len) return -1;
        break;
        
        case XMM_CMD_SYSCALL:
            sprintf(ret_url, "http://%s:%d/cgi-bin/webra.fcgi?notify/system", hostname, port);
            if (strlen(ret_url) > url_len) return -1;
        break;
        
        case XMM_CMD_CHK_FUNC:
            sprintf(ret_url, "http://%s:%d/cgi-bin/webra.fcgi?device", hostname, port);
            if (strlen(ret_url) > url_len) return -1;
        break;
    }

    return 0;
}

static int _make_cookie_str(int s_idx, char *ret_str, int str_len)
{
    memset(ret_str, 0x00, str_len);

    sprintf(ret_str, "grpid=%s; permission=%d; ISESSIONID=%s; userid=%s", ixmm[s_idx].cookie.grp, ixmm[s_idx].cookie.permision, ixmm[s_idx].cookie.session_id, ixmm[s_idx].cookie.id);
    
    return 0;
}

static int _parse_cookie_data(char *data, XMM_COOKIE_T *cookie)
{
    json_t *root;
    json_t *data_obj;
    json_t *session;
    json_error_t error;
    int status = 0;

    if (!data) return 0;
    
    root = json_loads(data, 0, &error);

    if (!root) {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return -1;
    }
    status = (int)json_integer_value(json_object_get(root, "status"));
    if (status != 200) {
        DMSG(8, "login fail!");
    	json_decref(root);
    	return -1;
    }
    
	data_obj = json_object_get(root, "data");
	session = json_object_get(data_obj, "_session");
	strncpy(cookie->session_id, json_string_value(json_object_get(session, "ISESSIONID")), sizeof(cookie->session_id) - 1);
	strncpy(cookie->id, json_string_value(json_object_get(session, "userid")), sizeof(cookie->id) - 1);
	strncpy(cookie->grp, json_string_value(json_object_get(session, "grpid")), sizeof(cookie->grp) - 1);
    cookie->permision = (int)json_integer_value(json_object_get(session, "permission"));

	json_decref(root);

    return 0;
}

static int _parse_supported_nvr(char *data)
{
    json_t *root;
    json_t *data_obj;
    json_t *device;
    json_t *capa;
    json_error_t error;
    int status = 0;
    int is_supported = 0;

    if (!data) return 0;
    
    root = json_loads(data, 0, &error);

    if (!root) {
        fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
        return -1;
    }
        
	data_obj = json_object_get(root, "data");
	device = json_object_get(data_obj, "device");
	capa = json_object_get(device, "capability");
	is_supported = json_boolean_value(json_object_get(capa, "support_local_api"));

    DMSG(8, "is_supported : %d", is_supported);
    
	json_decref(root);

    return is_supported;
}

static int _update_conn_status(char *hostname, int port, XMM_CONN_STATUS status)
{
    int d_idx = -1;

    d_idx = _get_data_idx(hostname, port);
    DMSG(8, "idx : %d", d_idx);
    if (d_idx == -1) {
        DMSG(8, "");
        return -1;
    }

    ixmm[d_idx].conn = status;
    DMSG(1, "host : %s, conn status : %d", ixmm[d_idx].hostname, ixmm[d_idx].conn);
    
    return 0;
}

static int _login_nvr(int idx)
{
    XMM_CURL_RET curl_ret;
    char url[1024];
    char post_field[1024];
    int ret = -1;
    int res = 0;

    memset(&curl_ret, 0x00, sizeof(XMM_CURL_RET));
    memset(url, 0x00, sizeof(url));
    memset(post_field, 0x00, sizeof(post_field));

    _make_url(XMM_CMD_LOGIN, ixmm[idx].hostname, ixmm[idx].port, url, sizeof(url));
    snprintf(post_field, sizeof(post_field) - 1, "{\"login\":{\"userid\":\"%s\",\"passwd\":\"%s\",\"maxage\":\"86400\"}}", ixmm[idx].id, ixmm[idx].pw);

    ret = _send_msg_post(url, post_field, &curl_ret);
    
    if (ret == 200) {
        DMSG(8, "\nResponse Body%s", curl_ret.data);
        res = _parse_cookie_data(curl_ret.data, &ixmm[idx].cookie);
        if (res != 0) return -1;

        _print_session_data(&ixmm[idx]);
    }
    else {
        return ret;
    }

    return 0;
}

static int _check_session_alive(int idx)
{
    XMM_CURL_RET curl_ret;
    char url[1024];
    char post_field[4096];
    char cookie[128];
    int ret = -1;

    memset(&curl_ret, 0x00, sizeof(XMM_CURL_RET));
    memset(url, 0x00, sizeof(url));
    memset(post_field, 0x00, sizeof(post_field));
    memset(cookie, 0x00, sizeof(cookie));

    DMSG(8, "Call API");
    
    _make_url(XMM_CMD_KEEPALIVE, ixmm[idx].hostname, ixmm[idx].port, url, sizeof(url));
    _make_cookie_str(idx, cookie, sizeof(cookie));
    DMSG(8, "url : %s", url);

    ret = _send_msg_get_with_cookie(url, post_field, cookie, &curl_ret);
    DMSG(8, "send result : %d", ret);

    return ret;
}

//////////////////////////////////////////////////
// xmm th manager
//

int _check_supported_remote_control(char *hostname, int port)
{
    XMM_CURL_RET curl_ret;
    char url[1024];
    int ret = -1;
    int res = 0;

    memset(&curl_ret, 0x00, sizeof(XMM_CURL_RET));
    memset(url, 0x00, sizeof(url));

    _make_url(XMM_CMD_CHK_FUNC, hostname, port, url, sizeof(url));

    ret = _send_msg_get(url, &curl_ret);
    
    if (ret == 200) {
        DMSG(8, "\nResponse Body%s", curl_ret.data);
        res = _parse_supported_nvr(curl_ret.data);
        return res;
    }
    else {
        return -1;
    }

    return 0;
}

int _send_message_data(XMM_CMD_TYPE type, char *hostname, int port, char *data)
{
    XMM_CURL_RET curl_ret;
    char url[1024];
    char post_field[4096];
    char cookie[128];
    int ret = -1;
    int l_res;
    int d_idx = -1;

    memset(&curl_ret, 0x00, sizeof(XMM_CURL_RET));
    memset(url, 0x00, sizeof(url));
    memset(post_field, 0x00, sizeof(post_field));
    memset(cookie, 0x00, sizeof(cookie));

    DMSG(8, "Call API");

    d_idx = _get_data_idx(hostname, port);
        
    if (_check_session_alive(d_idx) != 200) {
        l_res = _login_nvr(d_idx);
        if (l_res != 0) return -1;
    }
    
    _make_url(type, hostname, port, url, sizeof(url));
    if (data) strncpy(post_field, data, strlen(data));
    _make_cookie_str(d_idx, cookie, sizeof(cookie));
    DMSG(8, "url : %s", url);
    DMSG(8, "post_field : %s", post_field);

    ret = _send_msg_data(url, post_field, cookie, &curl_ret);
    
    DMSG(8, "send result : %d", ret);

    return ret;
}

static XMW_T *_make_work(XMM_CMD_TYPE type, char *hostname, int http_port, char *data, xmm_result_cb cb_func, int retry_cnt)
{
    XMW_T *pxmw = NULL;
    
    pxmw = (XMW_T*)imalloc(sizeof(XMW_T));
    if (!pxmw) {
        DMSG(8, "%s XMW_T imalloc Error!!", __FUNCTION__);
        return NULL;
    }
    
    memset(pxmw, 0x00, sizeof(XMW_T));

    pxmw->type = type;
    strcpy(pxmw->hostname, hostname);
    pxmw->http_port = http_port;
    
    if (data) {
        pxmw->content.data = (char*)imalloc(strlen(data)+1);
        if (!pxmw->content.data) {
            DMSG(8, "%s content.data imalloc Error!!", __FUNCTION__);
            ifree(pxmw);
            return NULL;
        }
        memset(pxmw->content.data, 0x00, sizeof(pxmw->content.data));
        strncpy(pxmw->content.data, data, strlen(data));
    }
    
    if (cb_func) pxmw->cb_func = cb_func;
    pxmw->retry_cnt = retry_cnt;

    return pxmw;
}

static void _push_work(XMW_T *pxmw)
{
    ix_queue_sync_push(ixmg.q, GUINT_TO_POINTER(pxmw));
}

static XMW_T *_pop_work()
{
    XMW_T *work = NULL;
    
    work = (XMW_T*)ix_queue_sync_pop(ixmg.q);

    return work;
}

static void _send_result(XMW_T *wrk, int res)
{
    XRES_T res_data;

    DMSG(8, "hostname : %s", wrk->hostname);
    DMSG(8, "res : %d", res);

    memset(&res_data, 0x00, sizeof(XRES_T));

    strcpy(res_data.hostname, wrk->hostname);
    res_data.http_port = wrk->http_port;
    res_data.result = res;

    if (wrk->cb_func) wrk->cb_func(&res_data);
}

static void _finalize_work(XMW_T *wrk)
{
    if (wrk->content.data) ifree(wrk->content.data);
    ifree(wrk);
}

static void* _proc_chk_func(void *arg) 
{
    XMW_T *wrk = (XMW_T*)arg;
    int retry_cnt = 0;
    int ret;

    do
    {
        ret = _check_supported_remote_control(wrk->hostname, wrk->http_port);
        retry_cnt++;
        DMSG(8, "hostname : %s, retry_cnt : %d", wrk->hostname, retry_cnt);
    } while ((wrk->retry_cnt != retry_cnt) && (ret == -1));

    if (ret == -1) {
        _update_conn_status(wrk->hostname, wrk->http_port, XMM_CONN_CONN_FAIL);
    }
    else if (ret == 0) {
        _update_conn_status(wrk->hostname, wrk->http_port, XMM_CONN_NOT_SUPP_FUNC);
    }

    _send_result(wrk, ret);
    _finalize_work(wrk);

    ixmg.cur_working_cnt--;
    printf("%s END\n", __FUNCTION__);
    
	g_thread_exit(NULL);
	
	return NULL;
}

static void* _proc_send_msg(void *arg) 
{
    XMW_T *wrk = (XMW_T*)arg;
    int retry_cnt = 0;
    int ret;

    do
    {
        ret = _send_message_data(wrk->type, wrk->hostname, wrk->http_port, wrk->content.data);
        retry_cnt++;
        DMSG(8, "hostname : %s, retry_cnt : %d", wrk->hostname, retry_cnt);
    } while ((wrk->retry_cnt != retry_cnt) && (ret == -1));

    if (ret == -1) {
        _update_conn_status(wrk->hostname, wrk->http_port, XMM_CONN_LOGIN_FAIL);
    }
    else if (ret != 200) {
        _update_conn_status(wrk->hostname, wrk->http_port, XMM_CONN_CONN_FAIL);
    }
    else if (ret == 200) {
        _update_conn_status(wrk->hostname, wrk->http_port, XMM_CONN_REQ_SUCC);
    }

    _send_result(wrk, ret);
    _finalize_work(wrk);

    ixmg.cur_working_cnt--;
    printf("%s END\n", __FUNCTION__);
    
	g_thread_exit(NULL);
	
	return NULL;
}
 
static void* _proc_login(void *arg) 
{
    XMW_T *wrk = (XMW_T*)arg;
    int retry_cnt = 0;
    int ret;
    int d_idx = -1;

    do
    {
        d_idx = _get_data_idx(wrk->hostname, wrk->http_port);
        if (d_idx == -1) break;
            
        if (_check_session_alive(d_idx) != 200) {
            ret = _login_nvr(d_idx);
        }
        retry_cnt++;
        DMSG(8, "hostname : %s, retry_cnt : %d", wrk->hostname, retry_cnt);
    } while ((wrk->retry_cnt != retry_cnt) && (ret == -1));

    _send_result(wrk, ret);

    _finalize_work(wrk);

    ixmg.cur_working_cnt--;
    printf("%s END\n", __FUNCTION__);
    
	g_thread_exit(NULL);
	
	return NULL;
}

// static GThread *ifn_make_thread(void *(*proc)(void *), void *data)
// {
// 	GError *error = NULL;
// 	GThread *thread = NULL;
// 	thread = g_thread_create((GThreadFunc)proc, data, FALSE, &error);
// 	if (thread == NULL) {
// 		DMSG(8, "GThread creating is fail. (err msg = %s)\n", error->message);
// 		g_error_free (error);
// 	}
// 	return thread;
// }

static void _run_work(XMW_T *work)
{
    GThread *w_thd;
    
    ixmg.cur_working_cnt++;

    if (work->type == XMM_CMD_SYSCALL)
        w_thd = ifn_make_thread(_proc_send_msg, work);
    else if (work->type == XMM_CMD_LOGIN)
        w_thd = ifn_make_thread(_proc_login, work);
    else if (work->type == XMM_CMD_CHK_FUNC)
        w_thd = ifn_make_thread(_proc_chk_func, work);
}

static int _cur_waiting_cnt()
{
    return ix_queue_sync_get_sizeof(ixmg.q);
}

static void *_proc_manager(void *data)
{
    XMW_T *work;
    gboolean empty;
    
    DMSG(8, "Working %s!\n", __FUNCTION__);
    while(1)
    {
        usleep(ixmg.sleep_time);
//        usleep(1000000);
        
        XMM_LOCK();
        empty = ix_queue_sync_is_empty(ixmg.q);
//        DMSG(9, "empty : %s\n", (empty ? "TRUE" : "FALSE"));
//        DMSG(9, "cur_working_cnt : %d", ixmg.cur_working_cnt);
        
        if (empty == TRUE) { XMM_UNLOCK(); continue; }

        if (ixmg.cur_working_cnt >= ixmg.max_working_cnt) {
            printf("_waiting_work_cnt : %d\n", _cur_waiting_cnt());
            XMM_UNLOCK(); 
            continue;
        }

        work = _pop_work();
        if (work) _run_work(work);
        
        XMM_UNLOCK();
    }

    DMSG(8, "End %s!\n", __FUNCTION__);

    g_thread_exit(NULL);

    return;
}

//////////////////////////////////////////////////
// xmm public api
//

int xmm_init()
{
    int th_id = -1;
    
    memset(ixmm, 0x00, sizeof(XMM_SESSION_INFO_T) * XMM_DEVICE_INFO_MAX_CNT);
    memset(&ixmg, 0x00, sizeof(XMG_T));
    ixmg.mtx = g_mutex_new();
    ixmg.q = ix_queue_new();
    ixmg.main_thd = ifn_make_thread(_proc_manager, NULL);
    ixmg.sleep_time = 10000;
    ixmg.max_working_cnt = 4;
    ixmg.cur_working_cnt = 0;

    return 0;
}

int xmm_cleanup()
{
    ix_queue_free(ixmg.q);
    g_mutex_free(ixmg.mtx);
}

int xmm_send_msg(char *hostname, int http_port, char *data, xmm_result_cb cb_func)
{
    XMW_T *pxmw = NULL;
    
    if (strlen(hostname) <= 0 || http_port <= 0) return -1;
    if (!data) return -1;

    XMM_LOCK();
    
    pxmw = _make_work(XMM_CMD_SYSCALL, hostname, http_port, data, cb_func, 1);
    if (!pxmw) {
        DMSG(8, "Make work Error");
        return -1;
    }
    _push_work(pxmw);
    
    XMM_UNLOCK();

    return 0;
}

int xmm_recv_message(char *data)
{
    cmm_send_message(CMMPT_NVM, IMSG_NONE, 0, 1, (void*)data);

    return 0;
}

int xmm_logout(char *hostname, int http_port, xmm_result_cb cb_func)
{
}

int xmm_update_nvr_info(char *hostname, int port, char *id, char *pw)
{
    int d_idx = -1;

    d_idx = _get_data_idx(hostname, port);
    if (d_idx == -1) {
        d_idx = _get_empty_slot(hostname, port);
        if (d_idx == -1) return -1;
        
        memset(&ixmm[d_idx], 0x00, sizeof(XMM_SESSION_INFO_T));
    }
    
    if (id && strlen(id) > 0) strcpy(ixmm[d_idx].id, id);
    if (pw && strlen(pw) > 0) {
        DMSG(8, "org_pw : %s", ixmm[d_idx].pw);
        strcpy(ixmm[d_idx].pw, pw);
        DMSG(8, "cur pw : %s, pw : %s", ixmm[d_idx].pw, pw);
    }
    if (hostname && strlen(hostname) > 0) strcpy(ixmm[d_idx].hostname, hostname);
    if (port != 0) ixmm[d_idx].port = port;
    
    DMSG(8, "d_idx : %d", d_idx);
    _print_session_data(&ixmm[d_idx]);

    return d_idx;
}

int xmm_check_supported_remote_control(char *hostname, int http_port, xmm_result_cb cb_func)
{
    XMW_T *pxmw = NULL;
    
    XMM_LOCK();
    
    pxmw = _make_work(XMM_CMD_CHK_FUNC, hostname, http_port, NULL, cb_func, 3);
    if (!pxmw) {
        DMSG(8, "Make work Error");
        return -1;
    }
    _push_work(pxmw);
    
    XMM_UNLOCK();

    return 0;
}

int xmm_update_master_nvr_info(char *hostname, int port, char *id, char *pw)
{
    DMSG(8, "");
    
    memset(&master, 0x00, sizeof(XMM_SESSION_INFO_T));
    
    if (strlen(id) > 0) strcpy(master.id, id);
    if (strlen(pw) > 0) strcpy(master.pw, pw);
    if (strlen(hostname) > 0) strcpy(master.hostname, hostname);
    if (port != 0) master.port = port;
    
    _print_session_data(&master);

    return 0;
}

int xmm_send_message_to_master(char *data)
{
    XMW_T *pxmw = NULL;
    
    if (strlen(master.hostname) <= 0 || master.port <= 0) {
        DMSG(8, "Not found master");
        return -1;
    }
    
    if (!data) {
        DMSG(1, "Data is NULL");
        return -1;
    }

    XMM_LOCK();
    
    pxmw = _make_work(XMM_CMD_SYSCALL, master.hostname, master.port, data, NULL, 3);
    if (!pxmw) {
        DMSG(8, "Make work Error");
        return -1;
    }
    _push_work(pxmw);
    
    XMM_UNLOCK();

    return 0;
}

XMM_SESSION_INFO_T *xmm_get_session_info(char *hostname, int port)
{
    XMM_SESSION_INFO_T *pinfo = NULL;
    int d_idx = -1;

    d_idx = _get_data_idx(hostname, port);
    DMSG(8, "idx : %d", d_idx);
    if (d_idx == -1) {
        DMSG(8, "");
        return NULL;
    }

    pinfo = (XMM_SESSION_INFO_T*)imalloc(sizeof(XMM_SESSION_INFO_T));
    memset(pinfo, 0x00, sizeof(XMM_SESSION_INFO_T));

    memmove(pinfo, &ixmm[d_idx], sizeof(XMM_SESSION_INFO_T));

    return pinfo;
}

int xmm_get_session_id(char *hostname, int port, char *sid, unsigned int sid_len)
{
    int d_idx = -1;

    d_idx = _get_data_idx(hostname, port);
    DMSG(8, "idx : %d", d_idx);
    if (d_idx == -1) {
        DMSG(8, "");
        return -1;
    }

    if (!sid || sid_len <= 0) return -1;

    memset(sid, 0x00, sid_len);

    if (strlen(ixmm[d_idx].cookie.session_id)) {
        strncpy(sid, ixmm[d_idx].cookie.session_id, sid_len-1);
        return 1;
    }

    return 0;
}

int xmm_update_conn_status(char *hostname, int port, XMM_CONN_STATUS status)
{
    int ret;
    
    ret = _update_conn_status(hostname, port, status);

    return ret;
}

XMM_CONN_STATUS xmm_get_conn_status(char *hostname, int port)
{
    int d_idx = -1;

    DMSG(1, "");
    
    d_idx = _get_data_idx(hostname, port);
    DMSG(8, "idx : %d", d_idx);
    if (d_idx == -1) {
        DMSG(8, "");
        return -1;
    }

    return ixmm[d_idx].conn;    
}

