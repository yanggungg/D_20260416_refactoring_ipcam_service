#ifndef __XMM_H__
#define __XMM_H__

#define XMM_CURL_RET_MAX_DATA_SIZE      (32*1024)
#define XMM_DEVICE_INFO_MAX_CNT         (64)

typedef enum {
    XMM_CMD_LOGIN = 0,
    XMM_CMD_LOGOUT,
    XMM_CMD_KEEPALIVE,
    XMM_CMD_SYSCALL,
    XMM_CMD_CHK_FUNC
} XMM_CMD_TYPE;

typedef enum {
    XMM_CONN_NONE = 0,
    XMM_CONN_REQUEST,
    XMM_CONN_LOGIN_FAIL,
    XMM_CONN_NOT_SUPP_FUNC,
    XMM_CONN_CONN_FAIL,
    XMM_CONN_TIMEOUT,

    XMM_CONN_REQ_SUCC,
    XMM_CONN_CONN_SUCC,

    XMM_CONN_MAX
} XMM_CONN_STATUS;

typedef struct _XMM_CURL_RET_T {
	unsigned int data_len;			
	char data[XMM_CURL_RET_MAX_DATA_SIZE];
} XMM_CURL_RET;

typedef struct _XMM_COOKIE_T XMM_COOKIE_T;
struct _XMM_COOKIE_T
{
    char session_id[128];
    char id[33];
    char grp[16];
    int permision;
};

typedef struct _XMM_SESSION_INFO_T XMM_SESSION_INFO_T;
struct _XMM_SESSION_INFO_T
{
    XMM_COOKIE_T cookie;
    char id[33];
    char pw[65];
    char hostname[256];
    int port;
    XMM_CONN_STATUS conn;
};

typedef struct _XMM_CONTENTS_T XCT_T;
struct _XMM_CONTENTS_T
{
    char *data;
};

typedef struct _XMM_RESULT_DATA_T XRES_T;
struct _XMM_RESULT_DATA_T
{
    char hostname[256];
    int http_port;
    int result;
};

typedef void (*xmm_result_cb)(XRES_T *res_data);

int xmm_init();
int xmm_cleanup();
int xmm_send_msg(char *hostname, int http_port, char *data, xmm_result_cb cb_func);
int xmm_recv_message(char *data);
int xmm_logout(char *hostname, int http_port, xmm_result_cb cb_func);
int xmm_update_nvr_info(char *hostname, int port, char *id, char *pw);
int xmm_check_supported_remote_control(char *hostname, int http_port, xmm_result_cb cb_func);
int xmm_update_master_nvr_info(char *hostname, int port, char *id, char *pw);
int xmm_send_message_to_master(char *data);
XMM_SESSION_INFO_T *xmm_get_session_info(char *hostname, int port);
int xmm_get_session_id(char *hostname, int port, char *sid, unsigned int sid_len);
int xmm_update_conn_status(char *hostname, int port, XMM_CONN_STATUS status);
XMM_CONN_STATUS xmm_get_conn_status(char *hostname, int port);

#endif
