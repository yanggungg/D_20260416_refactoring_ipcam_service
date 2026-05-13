#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define DEBUG_SMS_SEND_LOG

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
#include "nf_util_sms.h"

// for biz
#include "biz_send.h"
#include "nf_util_http.h"
#define S1_ID "s1netotp"
#define S1_PW "s1_smart"
#define DEBUG_HTTP_CLIENT_JBSHELL

#ifdef DEBUG_HTTP_CLIENT_JBSHELL
#include "jbshell.h"
#endif


int disableLog = 0;
#ifdef DEBUG_LOG
#define g_message ipx_printf
#endif

char* click_a_tell_string[80] = { "sendmsg", "getbalance",

};

static int _queue_init = 0;
static NfSMSSend *_nf_sms_send = NULL;

static const char *_DEBUG_SMS_SEND_str[32] =
		{ "SMS_SEND_IDX_INIT", "SMS_SEND_IDX_LIBESMTP", "SMS_SEND_IDX_QUEUE",
				"SMS_SEND_IDX_REQUEST",

				"SMS_SEND_IDX_TEST", "DEBUG_SMS_SEND_IDX_REQUEST_RAW",
				"SMS_SEND_IDX_NR" };

static gint _DEBUG_SMS_SEND_log[32] = { 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // FIXME
		};

typedef struct _NF_SMS_SEND_INTERNAL_T {
#if 0
	GTimeVal enque_tv;
	GTimeVal process_tv;
	gint retry_cnt;
#endif

	NF_SMS_SEND_DATA content;
	NF_SMS_INFO svr;
} NF_SMS_SEND_INTERNAL;

typedef enum _DEBUG_SMS_SEND_IDX_E {
	DEBUG_SMS_SEND_IDX_INIT = 0,
	DEBUG_SMS_SEND_IDX_LIBESMTP = 1,
	DEBUG_SMS_SEND_IDX_QUEUE = 2,
	DEBUG_SMS_SEND_IDX_REQUEST = 3,

	DEBUG_SMS_SEND_IDX_TEST = 4,
	DEBUG_SMS_SEND_IDX_REQUEST_RAW = 5,
	DEBUG_SMS_SEND_IDX_NR = 6,
	DEBUG_SMS_SEND_SMS_DATA = 7
} DEBUG_SMS_SEND_IDX_E;

// queue property

static void nf_sms_send_class_init(NfSMSSendClass * klass);
static void nf_sms_send_instance_init(GTypeInstance * instance,
		gpointer g_class);

static void nf_sms_send_set_property(GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec);
static void nf_sms_send_get_property(GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec);

static void nf_sms_send_dispose(GObject * object);
static void nf_sms_send_finalize(GObject * object);

static GObjectClass *parent_class = NULL;

static void sms_send_thread_func(NfSMSSend * self);
static int _sms_send_request(NF_SMS_SEND_INTERNAL* info);

static void _free_sinfo(NF_SMS_SEND_INTERNAL *sinfo);
static void _dump_sinfo(NF_SMS_SEND_INTERNAL *sinfo);
static int _check_param_content(NF_SMS_SEND_DATA *cont);
static NF_SMS_SEND_INTERNAL *_make_sinfo(NF_SMS_INFO* svr,
		NF_SMS_SEND_DATA *cont);
static int _sms_send(NF_SMS_INFO* si, NF_SMS_SEND_DATA* ssi);

static int _sms_get_account(int* svr, char* id, char* pw, char* appid,
		int* security);

// String Table
static const char* _sms_vendor_names[] = {
/*  0-1  */"BIZ PPURIO","CLICKATELL" };

#define	NUM_PROTO	(sizeof(_sms_vendor_names)/sizeof(char *))

gint nf_sms_vendor_get_count() {
	return NUM_PROTO;
}
gchar *nf_sms_vendor_get_string(gint index) {
	g_return_val_if_fail(index < NUM_PROTO, NULL);

	return _sms_vendor_names[index];
}
gint nf_sms_vendor_get_index(gchar* str) {

	int i = 0;
	g_return_val_if_fail(str != NULL, -1);
	for (i = 0; i < NUM_PROTO; i++) {

		if (!strcmp(_sms_vendor_names[i], str)) {

			return i;
		}

	}
	g_return_val_if_fail(i != NUM_PROTO, -1);

}

static
void _get_time_string(char* msg_id, int msg_len) {
	struct tm newtime;
	time_t ltime;

	ltime = time(&ltime);
	localtime_r(&ltime, &newtime);

	strftime(msg_id, msg_len, "%d%H%M", &newtime);
	//g_message("msg_id %s\n",msg_id);

}
static
int _get_message_count(char* text) {
	int mod = strlen(text) % NF_SMS_SPLIT_LIMIT;
	int index = strlen(text) / NF_SMS_SPLIT_LIMIT;
	if (mod != 0) {
		index += 1;
	}
	if (index > 4) {
		index = 4;
	}
	return index;

}
static
int _sms_send_biz(int socket_fd, NF_SMS_SEND_DATA* ssi, gboolean security) {
	int bPing = 0;    // 1/0 (Y/N)
	int bStat = 0;
	msg_t _msg;
	char msg_id[MAX_TEXT_SIZE];
	char splitedTxt[NF_SMS_SPLIT_LIMIT + 1] = { 0, };

	bPing = sendPing(socket_fd);

	if(!disableLog) g_message("%s:%d bPing[%d] sockfd[%d]", __FUNCTION__, __LINE__, bPing,
			socket_fd);

	if (bPing == -1) {
		g_message("%s, %d", __FUNCTION__, __LINE__);
		return NF_NOT_CONNECTED;
	}

	/* �޽��� ���� */
	/* �ʿ��� ������ ���� */
	/* ex. SMS �� ���, msg_type, dest_phone, send_phone, msg_body ������ ���� */
	int i = 0, j = 0;
	int length = _get_message_count(ssi->text);
	if(!disableLog)g_message("length %d ssi->receiver_cnt %d",length,ssi->receiver_cnt);
	for (i = 0; i < ssi->receiver_cnt; i++) {
		for (j = 0; j < length; j++) {
			_get_time_string(&msg_id, sizeof(msg_id));

			memset(&_msg, 0x00, sizeof(msg_t));
			strcpy(_msg.cmid, msg_id);  //   ������ id
			strcpy(_msg.msg_type, "sms"); // ������ Ÿ�� (sms / wap / fax / phone / sms_inbound / mms)
			strcpy(_msg.send_time, ""); // �߼� (����) �ð� (Unix Time, �������� ���� ��� ��� �߼�)
			strcpy(_msg.dest_phone, ssi->receiver[i]);     // �޴� ��� ��ȭ ��ȣ
			strcpy(_msg.send_phone, ssi->receiver[i]);   //  ������ ��� ��ȭ ��ȣ
			memset(splitedTxt, 0x00, NF_SMS_SPLIT_LIMIT);
			memcpy(splitedTxt, ssi->text + (NF_SMS_SPLIT_LIMIT * j),
					NF_SMS_SPLIT_LIMIT);
			strcpy(_msg.msg_body, splitedTxt);     // ������ ����

			/* �޽��� ���� */

			if(!disableLog)g_message("%s:%d splitedTxt[%s]", __FUNCTION__, __LINE__,splitedTxt);
			bStat = sendMsg(socket_fd, &_msg, security); /* @param _msg    �޽��� ����ü */
			if(!disableLog)g_message("%s:%d sendMsg ret[%d]", __FUNCTION__, __LINE__, bStat);

			if (bStat == NF_SMS_CONNECTION_ERROR) {
				// network connection error
				g_message("%s, %d", __FUNCTION__, __LINE__);
				return NF_SMS_CONNECTION_ERROR;
			}
		}
	}
	return NF_SMS_OK;
}

static
int _get_recv_data_sock(char* ipstr, char* sock_buf) {
	int sock;
	struct sockaddr_in sin;

	fd_set readfds;
	struct timeval tv;
	int state = 0;
	unsigned short http_port = 80;
	char *s, *p;
	char buf[16];

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ipstr);
	printf("ipstr : %s", ipstr);
	sin.sin_port = htons(http_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		//	free(sock_buf);
		return NF_NOT_CONNECTED;
	}

	{
		int ret;
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (ret < 0) {

			perror("setsockopt");
			close(sock);
			sock = (-1);
			//free(sock_buf);
			return NF_NOT_CONNECTED;
		}
	}

	if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		perror("connect");
		close(sock);
		sock = (-1);
		//free(sock_buf);
		return NF_NOT_CONNECTED;
	}

	if (send(sock, sock_buf, strlen(sock_buf), 0) < 0) {
		perror("send");
		close(sock);
		sock = (-1);
		//free(sock_buf);
		return NF_NOT_CONNECTED;
	}
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	state = select(sock + 1, &readfds, (fd_set*) NULL, (fd_set*) NULL, &tv);
	if (state == 0) {
		perror("select");
		close(sock);
		sock = (-1);
		//free(sock_buf);
		return NF_NOT_CONNECTED;
	}

	memset(sock_buf, 0x00, 2048);
	if (recv(sock, sock_buf, 2048, 0) < 0) {
		perror("recv");
		close(sock);
		sock = (-1);
		//free(sock_buf);
		return NF_NOT_CONNECTED;
	}

	{

		const char f_ok[] = "200 OK";
		char *errcode = sock_buf;
		errcode = strstr(errcode, f_ok);
		if (errcode == NULL) {
			//free(sock_buf);
			return NF_NOT_CONNECTED;
		} else {
			memset(sock_buf, 0x00, 2048);
			if (recv(sock, sock_buf, 2048, 0) < 0) {
				perror("recv");
				close(sock);
				sock = (-1);
				//free(sock_buf);
				return NF_NOT_CONNECTED;
			}

			close(sock);
			sock = (-1);
		}
	}
	return 0;
}

static
int _close_biz(int socket_fd) {
	if(!disableLog)g_message("%s fd[%d]", __FUNCTION__, socket_fd);
	sendEnd(socket_fd);

	// socket�ݱ�
	return 0;
}

/**
 @brief				nf_sms_send_cat_by_nf_http_direct db���� ���� �ٷ� click a tell�� ����
 @return	gboolean	%TRUE on success, %FALSE if an error occurred
 */
static int _sms_send_cat_by_nf_http(NF_SMS_INFO* si, NF_SMS_SEND_DATA* ssi) {
	char *buff = NULL;
	guint buff_len = 0;
	int ret = 0, debug = 0, ptr_len = 0;
	const char get_clickatell_raw[] =
//	"http://api.clickatell.com/http/%s?user=%s&password=%s&api_id=%s&to=%s&text=%s&from=%s";
			"api.clickatell.com/http/%s?user=%s&password=%s&api_id=%s";

	NF_HTTP_CLIENT_REQ req;
	memset(&req, 0x00, sizeof(NF_HTTP_CLIENT_REQ));

	//snprintf(req.url, 2048, get_clickatell_raw, click_a_tell_string[NF_CLICK_SEND_MSG], si->username,
	//		si->passwd, si->app_id,ssi->receiver[0],ssi->text,ssi->receiver[0]);
	snprintf(req.url, 2048, get_clickatell_raw,
			click_a_tell_string[NF_CLICK_BALANCE], si->username, si->passwd,
			si->app_id);

	printf("req.url : %s\n", req.url);
	req.is_auth = 0;
	req.is_debug = 0;
	req.timeout_connect_sec = 3;
	req.timeout_rx_sec = req.timeout_tx_sec = 7;
	req.cb_arg = &req;
	ret = nf_http_client_get_buff_chunked(&req, &buff, &buff_len, NULL);

	if(!disableLog)g_message("buff %s\n",buff);
	if(buff)
	{
			//nf_debug_hexdump(buff, (buff_len > ptr_len) ? ptr_len : buff_len);
			g_free(buff);
	}
	return ret;
}
#if 0
static
int _sms_send_cat(NF_SMS_INFO* si, NF_SMS_SEND_DATA* ssi)
{

	const char get_clickatell_raw[] =
//"GET /http/getbalance?api_id=%s&user=%s&password=%s&to=%s&text=%s&from=%s HTTP/1.1\r\n"
	"GET /%s?api_id=%s&user=%s&password=%s&to=%s&text=%s&from=%s HTTP/1.1\r\n"
	"Host: %s\r\n"
	"Connection: keep-alive\r\n"
	"Cache-Control: max-age=0\r\n"
	"User-Agent: ITX-SECURITY\r\n"
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
	"Accept-Encoding: gzip,deflate,sdch\r\n"
	"Accept-Language: ko,en-US,en;q=0.8\r\n"
	"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";

	char *sock_buf;
	char ip_str[16] = CLICKATELL_ADDRESS;
	char username[64];
	char password[64];

	sock_buf = (char*) malloc(2048);

	snprintf(sock_buf, 2048, get_clickatell_raw,"http/sendmsg",si->app_id, si->username,
			si->passwd, ssi->receiver[0],ssi->text,ssi->receiver[0]
			,"api.clickatell.com");

	_get_recv_data_sock(ip_str, sock_buf, 2048 );

	printf("sock %s",sock_buf);
	free(sock_buf);

	return 1;
}
#endif
static
char* _sms_res2str(int code) {
	char *str = NULL;
	switch (code) {
	case NF_SMS_FAIL_TO_AUTHENTICATE:
		str = "Fail to authenticate";
		break;			
	case NF_SMS_CONNECTION_ERROR:
		str = "Connection Error";
		break;
	case NF_SMS_OK:
		str = "Success";
		break;
	case NF_SMS_NOT_AVAILABLE_TIME:
		str = "Not Available Time";
		break;
	case NF_NOT_CONNECTED:
		str = "Not Connected. Check the network status.";
		break;
	case NF_FAIL_TO_MAKE_FILE_DIRECTORY:
		str = "Fail to make file directory";
		break;
	case NF_FAIL_TO_MAKE_BLACKLIST_DIRECTORY:
		str = "Fail to make blacklist directory";
		break;
	case NF_WRONG_CMD_ID:
		str = "Cmdid is wrong";
		break;
	case NF_FAIL_TO_SET_DEVICE:
		str = "Set the device e.g. sms";
		break;
	case NF_SMS_IS_BLACK_LIST:
		str = "Phone number is in the blacklist";
		break;
	case NF_SMS_DAILY_RESTRICTION_OVER:
		str = "SMS Send count was exceeding Daily resctriction count";
		break;
	case NF_FAIL_TO_AUTHENTICATE:
		str = "Fail to auth";
		break;
	case NF_ERROR_NOT_DEFINED:
		str = "Error is not defined check biz_send.";
		break;
	default:
		str = "Unkown Error!!";
		break;
	}

	if(!disableLog)g_message("%s code[%d][%s]", __FUNCTION__, code, str);
	if(!disableLog)g_message("%s code[%d][%s]", __FUNCTION__, code, str);
	return str;
}



static
int _count_check(int day, NF_SMS_INFO* si) {

	// day is changed and renew the current send count
	if (si->current_date != day) {
		si->current_date = day;
		si->current_send_count = 0;
	}

	// if max_send_count == SMS_UNLIMIT unlimited
	if (si->max_send_count != SMS_UNLIMIT
			&& si->current_send_count + 1 > si->max_send_count) {
		return NF_SMS_DAILY_RESTRICTION_OVER;
	} else {
		// set to the Database
		si->current_send_count++;
	}

	return NF_SMS_OK;
	//sav to db

}

static
int _restriction_check(NF_SMS_INFO* si) {
	char *endptr;
	char time[6] = { 0, };
	char day[2] = { 0, };
	int num;
	int ret;
	char msg_id[MAX_TEXT_SIZE];

	_get_time_string(&msg_id, sizeof(msg_id));

	strncpy(time, msg_id + 2, 4);
	strncpy(day, msg_id, 2);

	// check exceed the daily capable limit
	num = strtod(day, &endptr);
	ret = _count_check(num, si);
	if (ret != NF_SMS_OK) {
		return ret;
	}

	num = strtod(time, &endptr);
	if (si->start_avail_time <= num && si->end_avail_time >= num) {

		return NF_SMS_OK;
	} else {
		return NF_SMS_NOT_AVAILABLE_TIME;
	}
	return NF_SMS_OK;
}


/**l
 @brief				_sms_send_biz_direct DB���� ���� �Ѹ����� ��� ������
 @return	gboolean	%NF_SMS_OK on success, %error_code if an error occurred
 */
int _sms_send_biz_direct(char* ipaddress, NF_SMS_INFO* si,
		NF_SMS_SEND_DATA* ssi) {

	int sock_fd = -1; // ����
	int error_code = 0;
	// username , passwd, receiver�� ���� ��� exception.
	
	if(si->username[0] == '\0' || si->passwd[0] == '\0' || ssi->receiver[0][0] == '\0')
	{
		return NF_FAIL_TO_AUTHENTICATE;
	}
	

	if(!disableLog)g_message("%s: %s\n", si->username,si->passwd);
	if (!si->security) {

		sock_fd = sendBegin(ipaddress, BIZ_PORT, si->username, si->passwd, &error_code);
	} else {
		g_message("%s, %d", __FUNCTION__, __LINE__);

		sock_fd = sendBegin(ipaddress, BIZ_SECURITY_PORT, si->username, si->passwd, &error_code);
	}

	if (error_code < 0 || sock_fd < 0 ) {
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_close_biz(sock_fd);
		return sock_fd;
	}else {
		if(!disableLog)g_message("[summer] %s %s",ssi->text,ssi->receiver[0] );
		error_code = _sms_send_biz(sock_fd, ssi, si->security);
		g_message("%s, %d", __FUNCTION__, __LINE__);
		_close_biz(sock_fd);
	}
	return error_code;
}

/**
 @brief				nf_sms_push_event Queue �� event push �ؼ� ����ֱ�
 @return	gboolean	%NF_SMS_OK on success, %error_code if an error occurred
 */
int nf_sms_push_event(NF_SMS_INFO* si, NF_SMS_SEND_DATA* ssi) {

	gint q_len = 0;
	if (!_queue_init) {
		nf_sms_send_init(1);
		_queue_init = 1;
	}

	g_return_val_if_fail(_nf_sms_send != NULL, NF_SMS_SEND_ERROR_INIT);
	g_return_val_if_fail(ssi->receiver_cnt <= NF_SMS_SEND_MAX_TO_CNT,
			NF_SMS_SEND_ERROR_INIT);

	NF_SMS_SEND_INTERNAL * sinfo = _make_sinfo(si, ssi);
	g_return_val_if_fail(sinfo != NULL, NF_SMS_SEND_ERROR_MALLOC);

	q_len = g_async_queue_length(_nf_sms_send->queue);
	if (q_len > NF_SMS_SEND_MAX_QUEUE_LEN) {
		g_warning("%s nf_sms_send->queue FULL[%d]", __FUNCTION__, q_len);
		_free_sinfo(sinfo);
		return NF_SMS_SEND_ERROR_MAX_QUE_LEN;
	}
	g_async_queue_push(_nf_sms_send->queue, sinfo);

	return 1;

}

static void
_dump_cont( NF_SMS_SEND_DATA *content )
{

	int i=0;
	int rec_cnt = content->receiver_cnt;

	g_return_if_fail (content);
	if(!disableLog)	g_message("receiver_cnt [%d]", content->receiver_cnt);

	for(i=0;i <rec_cnt; i++) {
		if(!disableLog)g_message("receiver %d  [%s]",i, content->receiver[i]);
	}
	if(!disableLog)g_message("text         [%s]", content->text);
}

static void _dump_sinfo(NF_SMS_SEND_INTERNAL *sinfo) {
	g_return_if_fail(sinfo);
	_dump_cont(&sinfo->content);
}

static int _sms_send_request(NF_SMS_SEND_INTERNAL* info) {
	int ret = 0;
	NF_SMS_SEND_DATA* ssi = &info->content;
	NF_SMS_INFO* si = &info->svr;

	if (_DEBUG_SMS_SEND_log[DEBUG_SMS_SEND_SMS_DATA]) {
		if(!disableLog)g_message("ssi->receiver_cnt %d\n", ssi->receiver_cnt);
		if(!disableLog)g_message("ssi->receiver %s\n", ssi->receiver[0]);
		if(!disableLog)g_message("ssi->text %s\n", ssi->text);
	}

	ret = _sms_send(si, ssi);

	return ret;
}
//  g_async_queue_push (async_queue, GINT_TO_POINTER (id)); 
//	

static void _free_sinfo(NF_SMS_SEND_INTERNAL *sinfo) {
	g_return_if_fail(sinfo);
	g_free(sinfo);
	// so far null

}

static NF_SMS_SEND_INTERNAL *_make_sinfo(NF_SMS_INFO* svr,
		NF_SMS_SEND_DATA *cont) {

	int i = 0;
	NF_SMS_SEND_INTERNAL *sinfo = NULL;

	g_return_val_if_fail(cont != NULL, NULL);

	sinfo = g_malloc0(sizeof(NF_SMS_SEND_INTERNAL));
	g_return_val_if_fail(sinfo != NULL, NULL);

	sinfo->content.receiver_cnt = cont->receiver_cnt;
	for (i = 0; i < cont->receiver_cnt; i++) {
		strncpy(sinfo->content.receiver[i], cont->receiver[i],
				MAX_TEXT_SIZE - 1);
	}
	strncpy(sinfo->content.text, cont->text, NF_SMS_STRING_LIMIT - 1);
	sinfo->content.cb_func = cont->cb_func;

	if (svr != NULL) {

		strncpy(sinfo->svr.username, svr->username, MAX_SMS_SIZE - 1);
		strncpy(sinfo->svr.passwd, svr->passwd, MAX_SMS_SIZE - 1);
		strncpy( sinfo->svr.app_id, svr->app_id, MAX_SMS_SIZE-1);
		sinfo->svr.type = svr->type;
		sinfo->svr.start_avail_time = svr->start_avail_time;
		sinfo->svr.end_avail_time = svr->end_avail_time;
		sinfo->svr.max_send_count = svr->max_send_count;
		sinfo->svr.current_send_count = svr->current_send_count;
		sinfo->svr.current_date = svr->current_date;
		sinfo->svr.security = svr->security;
	} else {
		char id[64 + 1] = { 0, };
		char pw[64 + 1] = { 0, };
		char appid[64 + 1] = { 0, };
		int svr = 0;
		int security = 0;
		int err = _sms_get_account(&svr, &id, &pw, &appid, &security);

		strncpy(sinfo->svr.username, id, MAX_SMS_SIZE - 1);
		strncpy(sinfo->svr.passwd, pw, MAX_SMS_SIZE - 1);
		strncpy(sinfo->svr.app_id, appid, MAX_SMS_SIZE - 1);

		sinfo->svr.type = svr;
		sinfo->svr.start_avail_time = 0;
		sinfo->svr.end_avail_time = 2400;
		sinfo->svr.max_send_count = SMS_UNLIMIT;
		sinfo->svr.current_send_count = 0;
		sinfo->svr.current_date = 0;
		sinfo->svr.security = security;
	}

	return sinfo;
}

static void _sms_send_thread_func(NfSMSSend * self) {

	gpointer que_poped_data = NULL;

	int ret = 0;
	// wait init complete
	while (_nf_sms_send == NULL)
		g_usleep(10*1000);

	self->init_done = 1;
	while (self->thread_run) {
		NF_SMS_SEND_INTERNAL *sinfo = g_async_queue_pop(self->queue);
		if(!disableLog)g_message("%s pop_item[%x]", __FUNCTION__, sinfo);

		if (sinfo) {

#ifdef DEBUG_SMS_SEND_LOG
			if (_DEBUG_SMS_SEND_log[DEBUG_SMS_SEND_IDX_QUEUE])
				_dump_sinfo(sinfo);
#endif
			ret = _sms_send_request(sinfo);
			if (sinfo->content.cb_func) {
				sinfo->content.cb_func(ret, _sms_res2str(ret));
			}
			_free_sinfo(sinfo);
			g_usleep(10*1000);
		}
	}
	if(!disableLog)g_message("%s end", __FUNCTION__);
}

static void nf_sms_send_class_init(NfSMSSendClass * klass) {
	GObjectClass *gobject_class;
	int i;

	gobject_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	gobject_class->set_property = nf_sms_send_set_property;
	gobject_class->get_property = nf_sms_send_get_property;

	gobject_class->dispose = nf_sms_send_dispose;
	gobject_class->finalize = nf_sms_send_finalize;

}

GType nf_sms_send_get_type(void) {
	static GType nf_sms_send_type = 0;
	if (G_UNLIKELY(nf_sms_send_type == 0)) {
		static const GTypeInfo object_info = { sizeof(NfSMSSendClass), NULL,
				NULL, (GClassInitFunc) nf_sms_send_class_init, NULL, NULL,
				sizeof(NfSMSSend), 0,
				(GInstanceInitFunc) nf_sms_send_instance_init, NULL };

		nf_sms_send_type = g_type_register_static(NF_TYPE_OBJECT, "NfSMSSend",
				&object_info, 0);
	}
	return nf_sms_send_type;
}

static void nf_sms_send_instance_init(GTypeInstance* instance, gpointer g_class) {

	NfSMSSend *self = NF_SMS_SEND(instance);

	self->init_done = 0;

	// queue ����
	self->queue = g_async_queue_new();

	// notification signal emit�� thread ����
	self->thread_run = 1;
	self->thread = g_thread_create((GThreadFunc) _sms_send_thread_func, self,
			FALSE, NULL);

}

static void nf_sms_send_set_property(GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec) {
	NfObject *nfobject;

	nfobject = NF_OBJECT(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void nf_sms_send_get_property(GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec) {
	NfObject *self;

	self = NF_OBJECT(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

/* dispose is called when the object has to release all links
 * to other objects */
static void nf_sms_send_dispose(GObject * object) {
	// thread end
	parent_class->dispose(object);
}

/* finalize is called when the object has to free its resources */
static void nf_sms_send_finalize(GObject * object) {
	parent_class->finalize(object);
}

gboolean nf_sms_send_init(int wait) {
	gboolean ret = TRUE;
	gint r = 0;

	g_return_val_if_fail(_nf_sms_send == NULL, FALSE);
	_nf_sms_send = g_object_new(NF_TYPE_SMS_SEND, NULL);
	nf_debug_category_add("sms", _DEBUG_SMS_SEND_str, _DEBUG_SMS_SEND_log,
			DEBUG_SMS_SEND_IDX_NR);

	if (wait) {
		while (_nf_sms_send->init_done != 1)
			g_usleep(10*1000);
	}

	return ret;
}

static
int _sms_send(NF_SMS_INFO* gsi, NF_SMS_SEND_DATA* ssi) {
	int error_code = 0;
	char ipaddress[80] = { 0, };

	if(!disableLog)g_message("gsi->type %d\n",gsi->type); // FIX ME:
	switch (gsi->type) {
	case 0: // BIZ
		strncpy(ipaddress, BIZ_ADDRESS, strlen(BIZ_ADDRESS));
		//gsi->type = svr;
		gsi->max_send_count = SMS_UNLIMIT; // ������
		gsi->start_avail_time = 0;
		gsi->end_avail_time = 2400;
		error_code = _sms_send_biz_direct(ipaddress, gsi, ssi);
		break;
	case 1:  // ClickATell
		if(!disableLog)	g_message("gsi->app_id %s gsi->username %s gsi->passwd %s \n",gsi->app_id,gsi->username,gsi->passwd); // FIX ME:
		if(!disableLog)g_message("clickatell\n"); // FIX ME:
		gsi->max_send_count = SMS_UNLIMIT; // ������
		gsi->start_avail_time = 0;
		gsi->end_avail_time = 2400;
		gsi->security = 0;
		error_code = _sms_send_cat_by_nf_http(gsi, ssi);

		break;

	default:
		if(!disableLog)g_message("%s unknown server type[%d]", __FUNCTION__, gsi->type);
		error_code = -3;
		break;
	}
	return error_code;
}

static

int _sms_get_account(int* svr, char* id, char* pw, char* appid, int* security) {

//	'<item key="act.event.sms.server"       type="STRING"   min="0" max="64" val="BIZ PPURIO" />'
//	'<item key="act.event.sms.appid"        type="STRING"   min="0" max="64" val="" />'
//	'<item key="act.event.sms.user"         type="STRING"   min="0" max="64" val="s1netotp" />'
//	'<item key="act.event.sms.password"     type="STRING"   min="0" max="64" val="s1_smart" />'
//	'<item key="act.event.sms.sched_from"   type="UINT"     min="0" max="" val="0" />'
//	'<item key="act.event.sms.sched_to"     type="UINT"     min="0" max="" val="0" />'
//	'<item key="act.event.sms.count"        type="UINT"     min="0" max="" val="0" />'
	char *tmp_str = nf_sysdb_get_str_nocopy("act.event.sms.server");
	g_return_val_if_fail(tmp_str != NULL, NF_SMS_SEND_ERROR_MALLOC);
	*svr = nf_sms_vendor_get_index(tmp_str);

	tmp_str = nf_sysdb_get_str_nocopy("act.event.sms.user");
	g_return_val_if_fail(tmp_str != NULL, NF_SMS_SEND_ERROR_MALLOC);
	strncpy(id, tmp_str, DB_STRING_SIZE - 1);

	tmp_str = nf_sysdb_get_str_nocopy("act.event.sms.password");
	g_return_val_if_fail(tmp_str != NULL, NF_SMS_SEND_ERROR_MALLOC);
	strncpy(pw, tmp_str, DB_STRING_SIZE - 1);
	*security = 1;
	tmp_str = nf_sysdb_get_str_nocopy("act.event.sms.appid");
	g_return_val_if_fail(tmp_str != NULL, NF_SMS_SEND_ERROR_MALLOC);
	strncpy(appid, tmp_str, DB_STRING_SIZE - 1);

	return 0;
}

void _sms_send_result_cb(int ret, const char *ret_msg) {
	if(!disableLog)	g_message("%s ret[%d] ret_msg[%s]", __FUNCTION__, ret, ret_msg);
}



void _make_sms_info(NF_SMS_INFO* si) {

	char id[64 + 1] = { 0, };
	char pw[64 + 1] = { 0, };
	char appid[64 + 1] = { 0, };
	int svr = 0;
	int security = 0;

	int err = _sms_get_account(&svr, &id, &pw, &appid, &security);
	strncpy(si->username, id, MAX_SMS_SIZE - 1);
	strncpy(si->passwd, pw, MAX_SMS_SIZE - 1);
	strncpy(si->app_id, appid, MAX_SMS_SIZE - 1);

	si->type = svr;
	si->start_avail_time = 0;
	si->end_avail_time = 2400;
	si->max_send_count = SMS_UNLIMIT;
	si->current_send_count = 0;
	si->current_date = 0;
	si->security = security;
}

/**
 @brief				nf_s1_find_password DB���� ���� �Ѹ����� ��� ������
 @return	gboolean	%NF_SMS_OK on success, %error_code if an error occurred
 */
int nf_sms_find_password(NF_SMS_INFO* si, NF_SMS_SEND_DATA* ssi) {
	int error_code = 0;

	//g_return_val_if_fail ( si != NULL, -1);
	g_return_val_if_fail(ssi != NULL, -1);

	if (si != NULL) {
		error_code = _sms_send(si, ssi);
	} else {
		NF_SMS_INFO _si;
		_make_sms_info(&_si);
		error_code = _sms_send(&_si, ssi);
	}
	if (NF_SMS_OK != error_code)
		g_warning("%s send failed!! ret_code[%d]", __FUNCTION__, error_code);

	return error_code;
}

/**
 @brief				nf_s1_find_password DB���� ���� �Ѹ����� ��� ������
 @return	gboolean	%NF_SMS_OK on success, %error_code if an error occurred
 */
int nf_sms_find_s1_password(NF_SMS_SEND_DATA* ssi) {
	int error_code = 0;
	char ipaddress[80] = { 0, };

	NF_SMS_INFO si;

	char* id = S1_ID;
	char* pw = S1_PW;

	strncpy(ipaddress, BIZ_ADDRESS, strlen(BIZ_ADDRESS));

	strncpy(si.username, id, sizeof(si.username));
	strncpy(si.passwd, pw, sizeof(si.passwd));

	si.type = nf_sms_vendor_get_index("BIZ PPURIO");;
	si.max_send_count = SMS_UNLIMIT; // ������
	si.start_avail_time = 0;
	si.end_avail_time = 2400;
	si.security = 1;

	error_code = _sms_send_biz_direct(ipaddress, &si, ssi);
	if (NF_SMS_OK != error_code)
		g_warning("%s send failed!! ret_code[%d]", __FUNCTION__, error_code);

	return error_code;
}

/*******************************************************************************/

static char sms_send_event_with_svr_help[] =
		"sms_send_event_with_svr [text] [receiver]...";
static int jbshell_sms_send_event_with_svr(int argc, char **argv) {
	int res = 0;
	int i = 0;

	NF_SMS_SEND_DATA ssi;
	NF_SMS_INFO si;

	memset(&ssi, 0x00, sizeof(ssi));
	memset(&si, 0x00, sizeof(si));

	if(!disableLog)g_message("argc %d\n", argc);

	if (argc < 3) {
		if(!disableLog)g_message("%s\n", sms_send_event_with_svr_help);
		return -1;
	}

	for (i = 2; i < argc; i++) {
		strcpy(ssi.receiver[i - 2], argv[i]);
		if(!disableLog)g_message("receiver : %s\n", ssi.receiver[i - 2]);
	}
	if(!disableLog)g_message("text : %s\n", argv[0]);

	strcpy(ssi.text, argv[1]);
	ssi.receiver_cnt = argc - 2;
	ssi.cb_func = _sms_send_result_cb;


	strncpy(si.username, "s1netotp", strlen("s1netotp"));
	strncpy(si.passwd, "s1_smart111", strlen("s1_smart111"));

#if 0
	strncpy(si.username, "summersault", strlen("summersault"));
	strncpy(si.passwd, "ULeEaQCYSXUBdC", strlen("ULeEaQCYSXUBdC"));
#endif
//	strncpy(si.app_id, "3417815", strlen("3417815"));

	si.max_send_count = 100;
	si.start_avail_time = 0;
	si.end_avail_time = 2400;
	si.security = 1;
	si.type = nf_sms_vendor_get_index("BIZ PPURIO");
	if(!disableLog)g_message("si.type : %d\n", si.type);

	nf_sms_push_event(&si, &ssi);

	return 0;
}
__commandlist(jbshell_sms_send_event_with_svr,"sms_send_event_with_svr", sms_send_event_with_svr_help, sms_send_event_with_svr_help);

static char sms_send_event_help[] = "sms_send_event [text] [receiver]...";
static int jbshell_sms_send_event(int argc, char **argv) {
	int res = 0;
	int i = 0;

	NF_SMS_SEND_DATA ssi;
	memset(&ssi, 0x00, sizeof(ssi));

	if(!disableLog)g_message("argc %d\n", argc);
	if (argc < 3) {
		if(!disableLog)g_message("%s\n", sms_send_event_help);
		return -1;
	}

	for (i = 2; i < argc; i++) {
		strcpy(ssi.receiver[i - 2], argv[i]);
		if(!disableLog)g_message("receiver : %s\n", ssi.receiver[i - 2]);
	}

	if(!disableLog)g_message("text : %s\n", argv[1]);
	strcpy(ssi.text, argv[1]);
	ssi.receiver_cnt = argc - 2;

	nf_sms_push_event(NULL, &ssi);

	return 0;

}
__commandlist(jbshell_sms_send_event,"sms_send_event", sms_send_event_help, sms_send_event_help);

static char find_password_with_svr_help[] =
		"find_password_with_svr [receiver] [text]";
static int jbshell_find_password_with_svr(int argc, char **argv) {
	int res = 0;
	if(!disableLog)g_message("argc %d\n", argc);
	if (argc < 3) {
		if(!disableLog)g_message("%s\n", find_password_with_svr_help);
		return -1;
	}

	if(!disableLog)g_message("text : %s\n", argv[2]);
	if(!disableLog)g_message("receiver : %s\n", argv[1]);

	NF_SMS_SEND_DATA ssi;
	NF_SMS_INFO si;

	memset(&ssi, 0x00, sizeof(ssi));
	memset(&si, 0x00, sizeof(si));

	//strncpy(si.username, "s1netotp", strlen("s1netotp"));
	//strncpy(si.passwd, "s1_smart", strlen("s1_smart"));


	//strncpy(si.username, "itxsecurity", strlen("itxsecurity"));
	//strncpy(si.passwd, "#b6796880", strlen("#b6796880"));

	strncpy(si.username, "summersault", strlen("summersault"));
	strncpy(si.passwd, "ULeEaQCYSXUBdC", strlen("ULeEaQCYSXUBdC"));
	strncpy(si.app_id, "3417815", strlen("3417815"));
	si.max_send_count = 100;
	si.start_avail_time = 0;
	si.end_avail_time = 2400;
//	si.security = 1; for test
	si.type = nf_sms_vendor_get_index("CLICKATELL");
	if(!disableLog)g_message("si.type : %d\n", si.type);

	strcpy(ssi.receiver[0], argv[1]);
	strcpy(ssi.text, argv[2]);
	ssi.receiver_cnt = 1;
	ssi.cb_func = _sms_send_result_cb;

	res = nf_sms_find_password(&si, &ssi);

	if(!disableLog)g_message("error code = %s\n", _sms_res2str(res));
	return 0;

}
__commandlist(jbshell_find_password_with_svr,"find_password_with_svr", find_password_with_svr_help, find_password_with_svr_help);

static char find_password_help[] = "find_password [receiver] [text]...";
static int jbshell_find_password(int argc, char **argv) {
	int res = 0;
	int i = 0;

	NF_SMS_SEND_DATA ssi;
	memset(&ssi, 0x00, sizeof(ssi));

	strcpy(ssi.receiver[0], argv[1]);
	strcpy(ssi.text, argv[2]);
	ssi.receiver_cnt = 1;
	if(!disableLog)g_message("entering nf_sms_find_password\n");
	res = nf_sms_find_password(NULL, &ssi);
	if(!disableLog)g_message("error code = %s\n", _sms_res2str(res));
	return 0;

}
__commandlist(jbshell_find_password,"find_password", find_password_help, find_password_help);

static char s1_find_password_help[] = "find_s1_password  [text] [receiver] ";
static int jbshell_find_s1_password(int argc, char **argv) {
	int i = 0;
	int res = -1;

	if(!disableLog)g_message("argc %d\n", argc);
	if (argc < 3) {
		if(!disableLog)g_message("%s\n", s1_find_password_help);
		return -1;
	}


	NF_SMS_SEND_DATA ssi;
	NF_SMS_INFO si;

	memset(&ssi, 0x00, sizeof(ssi));
	memset(&si, 0x00, sizeof(si));

	for (i = 2; i < argc; i++) {
		strcpy(ssi.receiver[i - 2], argv[i]);
		if(!disableLog)g_message("receiver : %s\n", ssi.receiver[i - 2]);
	}

	if(!disableLog)g_message("text : %s\n", argv[1]);
	strcpy(ssi.text, argv[1]);
	ssi.receiver_cnt = argc - 2;

	nf_sms_find_s1_password(&ssi);

	return 0;
}
__commandlist(jbshell_find_s1_password,"find_s1_password", s1_find_password_help, s1_find_password_help);
