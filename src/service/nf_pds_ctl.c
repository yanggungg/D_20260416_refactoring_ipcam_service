#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <glib.h>
#include <nf_api_ipcam.h>
#include <nf_notify.h>
#include <nf_common.h>
#include "nf_network.h"
#include "nf_util_netif.h"
#include "nf_debug.h"
#include "nf_pds_ctl.h"


#define PDS_DEBUG				0
#define PDS_PACKET_MAX_DATA_LEN 4096
#define PDS_PACKET_HEADER_SIZE  8
#define PDS_MAX_STRING_SIZE		128
#define PDS_KEEPALIVE_SEC		10
#define PDS_PORT_NVR_REQ		776
#define PDS_PORT_PDS_REQ		777
#define PDS_ENABLE				"/tmp/pds_enable"

#if PDS_DEBUG
    #define PDS_LOG_PRINT printf
#else
    #define PDS_LOG_PRINT(...)
#endif


typedef enum _PDS_ONOFF_E {
	PDS_ON,
	PDS_OFF
};

typedef enum _PDS_MODE_E {
	PDS_MODE_NVR_REQ = 0,
	PDS_MODE_PDS_REQ = 1,
	PDS_MODE_NR
} PDS_MODE_E;

typedef enum _PDS_PACKET_E {
	// for PDS -> NVR (777)
	PDS_PACKET_NVR_CHECK_ALIVE  = 0x00000000,	// ( no body )
	PDS_PACKET_GET_NVR_INFO   	= 0x00000001,
	PDS_PACKET_NOTI_CONN_STATUS = 0x00000002,

	// for NVR -> PDS
	PDS_PACKET_PDS_CHECK_ALIVE  = 0x00010000,	// ( no body )
	PDS_PACKET_CHANGE_NVR_INFO  = 0x00010001,
	PDS_PACKET_GET_PDS_STATUS   = 0x00010002,

	PDS_PACKET_END              = 0xffffffff
} PDS_PACKET_E;


typedef struct _PDS_PACKET_T {
	PDS_PACKET_E	type;
	unsigned int	data_len;
	char			data[PDS_PACKET_MAX_DATA_LEN];
} PDS_PACKET;

typedef struct _PDS_NVR_INFO_T {
	char	serverId[PDS_MAX_STRING_SIZE];
	char	contractNo[PDS_MAX_STRING_SIZE];
	char	domainName[PDS_MAX_STRING_SIZE];
	char	publicIp[36];
	char	localIp[36];
	int		publicHttpPort;
	int 	localHttpPort;
	int		secureHttp;
	int		publicRtspPort;
	int		localRtspPort;
	int		serviceEnable;
} PDS_NVR_INFO;

typedef struct _PDS_CTRL_FD_T {
	int conn_fd;

	int client_fd;
	int client_cnt;

	unsigned int keepalive_timer;
	unsigned int last_req_ts;
} PDS_CTRL_FD;

typedef struct _PDS_CTRL_INFO_T {
	int pds_onoff;			// _PDS_ONOFF_E
	int is_running;
	int is_sysdb_changed;
	int is_rendez_connected;
	time_t rendez_conn_last_check_time;

	PDS_CTRL_FD	nvr_req;	// 776 NVR Request Port
	PDS_CTRL_FD	pds_req;	// 777 PDS Request Port

	GThread	*thread_id;

	PDS_PACKET info_pack;
} PDS_CTRL_INFO;


static void _pds_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static int _pds_onoff_ctrl();
static int _pds_manage_thread_func(void *arg);

static int _pds_init_sock(int *fd, int port);
static int _pds_close_sock(int *fd);
static int _pds_close_sock_all(PDS_CTRL_INFO *ctrl_info);
static int _pds_init_select_fdset(PDS_CTRL_INFO *ctrl_info, fd_set *wkset);
static int _pds_check_nvr_change();
static int _pds_make_info_msg(PDS_PACKET *pack);
static int _pds_send_msg(PDS_CTRL_FD *ctrl_fd, PDS_PACKET *pack);
static int _pds_recv_msg(PDS_CTRL_FD *ctrl_fd, PDS_PACKET *pack);
static int _pds_keepalive_msg(PDS_CTRL_FD *ctrl_fd, PDS_PACKET_E type);
static int _pds_process_accept(PDS_CTRL_FD *ctrl_fd, int mode);
static int _pds_process_client_pds(PDS_CTRL_INFO *pds_info, PDS_CTRL_FD *ctrl_fd);
static int _pds_process_keepalive(PDS_CTRL_INFO *pds_info);
static int _pds_process_changed_info(PDS_CTRL_INFO *pds_info);

static int _updater_script_restart();
static int _pds_script_restart();
static int _pds_proc_kill();

static PDS_CTRL_INFO g_pds_info;
static PDS_NVR_INFO g_pds_nvr_info;


void nf_pds_ctl()
{
	static int is_init = 0;

	if (!is_init)
	{
		gulong cb_handle;

		memset(&g_pds_info, 0x00, sizeof(g_pds_info));

		cb_handle= nf_notify_connect_cb("sysdb_change", _pds_sysdb_reload_cb_func, (gpointer)NULL);
		g_assert(cb_handle > 0);

		g_pds_info.nvr_req.conn_fd = -1;
		g_pds_info.pds_req.conn_fd = -1;
		g_pds_info.nvr_req.client_fd = -1;
		g_pds_info.pds_req.client_fd = -1;
		g_pds_info.is_running = 0;
		g_pds_info.pds_onoff = 1;

		g_pds_info.thread_id = g_thread_create((GThreadFunc)_pds_manage_thread_func,
					&g_pds_info, FALSE, NULL);

		memset(&g_pds_nvr_info, 0x00, sizeof(g_pds_nvr_info));

		is_init = 1;
	}
}

int nf_pds_onestop_test()
{
	gboolean ret;
	PDS_CTRL_FD *ctrl_fd = NULL;

	g_return_val_if_fail(g_pds_info.thread_id != NULL, 0);

	ctrl_fd = &(g_pds_info.nvr_req);

	if (ctrl_fd->client_fd < 0)
		return 0;

	ret =_pds_keepalive_msg(ctrl_fd, PDS_PACKET_PDS_CHECK_ALIVE);

	if (ret == 1)
		return 1;
	return 0;
}


int nf_pds_get_connection_status()
{
	return g_pds_info.is_rendez_connected;
}


static void _pds_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);

	if (pinfo->d.params[0] == NF_SYSDB_CATE_NET)
	{
		g_pds_info.pds_onoff = 1;
		g_pds_info.is_sysdb_changed = 1;
	}
}

static int _pds_onoff_ctrl()
{
	gboolean pre_enable = FALSE;
	gboolean enable = FALSE;

	enable = nf_sysdb_get_bool("net.ddns.p2p_enable");

	if (access(PDS_ENABLE, F_OK) == 0)
		pre_enable = TRUE;
	else
		pre_enable = FALSE;

	if (pre_enable != enable)
	{
		if (enable == TRUE)
		{
			char cmd[512] = { 0, };

			snprintf(cmd, sizeof(cmd), "/bin/touch %s", PDS_ENABLE);
			proxy_system(cmd,1,3);

			_pds_script_restart();
			_pds_proc_kill();

			printf("[%s][%d] PDS is enabled.\n", __FUNCTION__, __LINE__);
		}
		else
		{
			remove(PDS_ENABLE);

			_pds_script_restart();
			_pds_proc_kill();

			printf("[%s][%d] PDS is disabled.\n", __FUNCTION__, __LINE__);
		}

		_updater_script_restart();
	}

	if (enable)
		return PDS_ON;

	return PDS_OFF;
}

static int _pds_manage_thread_func(void *arg)
{
	fd_set wkset;
	int mx, i, n;
	struct timeval tv;
	time_t pds_sync_time = 0;
	time_t tdiff;

	PDS_CTRL_INFO *pds_info = (PDS_CTRL_INFO*)arg;

	sleep(30);

	while (1)
	{
		if (pds_info->pds_onoff)
		{
			pds_info->pds_onoff = 0;

			if (_pds_onoff_ctrl() == PDS_OFF)
				pds_info->is_running = 0;
			else
				pds_info->is_running = 1;

			printf("[%s][%d] is_running(%d)\n", __FUNCTION__, __LINE__, pds_info->is_running);
		}

		if (pds_info->is_running)
		{
			if (pds_info->nvr_req.conn_fd < 0)
			{
				_pds_init_sock(&pds_info->nvr_req.conn_fd, PDS_PORT_NVR_REQ);
			}

			if (pds_info->pds_req.conn_fd < 0)
			{
				_pds_init_sock(&pds_info->pds_req.conn_fd, PDS_PORT_PDS_REQ);
				_pds_check_nvr_change();
				_pds_make_info_msg(&(pds_info->info_pack));
			}

			_pds_process_changed_info(pds_info);
		}
		else
		{
			_pds_close_sock_all(pds_info);
			pds_info->is_rendez_connected = 0;
			sleep(1);
			continue;
		}

		if (pds_info->is_rendez_connected == 0)
		{
			tdiff = time(0) - pds_info->rendez_conn_last_check_time;

			if (tdiff < 0 || 60 < tdiff)
			{
				pds_info->rendez_conn_last_check_time = time(0);
				proxy_system("killall -9 pds.bin", 1, 3);

				PDS_LOG_PRINT("[%s][%d] Relaunch pds script files.\n",
						__FUNCTION__, __LINE__);
			}
		}

		tdiff = time(0) - pds_sync_time;
		if (tdiff < 0 || 5 < tdiff)
		{
			_pds_process_keepalive(pds_info);
			pds_sync_time = time(0);
		}

		mx = _pds_init_select_fdset(pds_info, &wkset);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		n = Select(mx + 1, &wkset, 0, 0, &tv);
		if (n == -1)
		{
			_pds_close_sock_all(pds_info);
			sleep(1);
			continue;
		}
		else if (n == 0)
		{
			continue;
		}

		if (FD_ISSET(pds_info->nvr_req.conn_fd, &wkset))
			_pds_process_accept(&pds_info->nvr_req, PDS_MODE_NVR_REQ);

		if (FD_ISSET(pds_info->pds_req.conn_fd, &wkset))
			_pds_process_accept(&pds_info->pds_req, PDS_MODE_PDS_REQ);

		if (pds_info->pds_req.client_fd > 0 && FD_ISSET(pds_info->pds_req.client_fd, &wkset))
			_pds_process_client_pds(pds_info, &pds_info->pds_req);

		if (pds_info->nvr_req.client_fd > 0 && FD_ISSET(pds_info->nvr_req.client_fd, &wkset))
			_pds_process_client_pds(pds_info, &pds_info->nvr_req);
	}

	return 0;
}

static int _pds_init_sock(int *fd, int port)
{
	struct sockaddr_in addr;
	int ret;
	unsigned int len = sizeof(struct sockaddr_in);
	int value, conn_fd;

	conn_fd = *fd;

	if (conn_fd > 0)
	{
		g_warning("%s alreay conn_fd open[%d]",  __FUNCTION__, conn_fd);
		return -1;
	}

	conn_fd = Socket(AF_INET, SOCK_STREAM, 0);
	if (conn_fd == -1)
		goto error_fd;

	value = 1;
    ret = Setsockopt(conn_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int));
    if (ret == -1)
    	goto error_fd;

	g_message("%s port[%d]", __FUNCTION__, port);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ret = Bind(conn_fd, (struct sockaddr *)&addr, len);
	if (ret == -1)
		goto error_fd;

	ret = Listen(conn_fd, 2);
	if (ret == -1)
		goto error_fd;

	*fd = conn_fd;
	g_message("%s open conn_fd[%d]", __FUNCTION__, conn_fd);

	return 1;

error_fd:
	if (conn_fd > 0)
	{
		Close(conn_fd);
		*fd = -1;
	}
	return -3;
}

static int _pds_close_sock(int *fd)
{
	int conn_fd = *fd;

	if (conn_fd > 0)
	{
		Close(conn_fd);
		*fd = -1;
		return 1;
	}

	return 0;
}

static int _pds_close_sock_all(PDS_CTRL_INFO *ctrl_info)
{
	int close_cnt = 0;

	close_cnt += _pds_close_sock(&ctrl_info->nvr_req.conn_fd);
	close_cnt += _pds_close_sock(&ctrl_info->pds_req.conn_fd);
	close_cnt += _pds_close_sock(&ctrl_info->nvr_req.client_fd);
	close_cnt += _pds_close_sock(&ctrl_info->pds_req.client_fd);

	return close_cnt;
}

static int _pds_init_select_fdset(PDS_CTRL_INFO *ctrl_info, fd_set *wkset)
{
	int max = 0;

	FD_ZERO(wkset);

	if (ctrl_info->nvr_req.conn_fd > 0)
	{
		FD_SET(ctrl_info->nvr_req.conn_fd, wkset);
		if (max < ctrl_info->nvr_req.conn_fd)
			max = ctrl_info->nvr_req.conn_fd;
	}

	if (ctrl_info->nvr_req.client_fd > 0)
	{
		FD_SET(ctrl_info->nvr_req.client_fd, wkset);
		if (max < ctrl_info->nvr_req.client_fd)
			max = ctrl_info->nvr_req.client_fd;
	}

	if (ctrl_info->pds_req.conn_fd > 0)
	{
		FD_SET(ctrl_info->pds_req.conn_fd, wkset);
		if (max < ctrl_info->pds_req.conn_fd)
			max = ctrl_info->pds_req.conn_fd;
	}

	if (ctrl_info->pds_req.client_fd > 0)
	{
		FD_SET(ctrl_info->pds_req.client_fd, wkset);
		if (max < ctrl_info->pds_req.client_fd)
			max = ctrl_info->pds_req.client_fd;
	}

	return max;
}

static int _pds_check_nvr_change()
{
	int rtn = 0, port;
	char temp[128];
	NF_NETIF_GET_INFO netif_info;

	if (strcmp(g_pds_nvr_info.serverId, nf_sysdb_get_str_nocopy("sys.info.mac")) != 0)
	{
		snprintf(g_pds_nvr_info.serverId, PDS_MAX_STRING_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.mac"));
		rtn = 1;
	}

	if (strcmp(g_pds_nvr_info.contractNo, nf_sysdb_get_str_nocopy("sys.info.swver")) != 0)
	{
		snprintf(g_pds_nvr_info.contractNo, PDS_MAX_STRING_SIZE, "%s", nf_sysdb_get_str_nocopy("sys.info.swver"));
		rtn = 1;
	}

	// Domain name
	snprintf(temp, sizeof(temp), "%s.%s", nf_sysdb_get_str_nocopy("sys.info.mac"), nf_sysdb_get_str_nocopy("net.proto.ddnssvr"));
	if (strcmp(g_pds_nvr_info.domainName, temp) != 0)
	{
		snprintf(g_pds_nvr_info.domainName, "%s", temp);
		rtn = 1;
	}

	// Local IP
	memset(temp, 0x00, 128);
	nf_netif_get_info(&netif_info);
	my_inet_ntoa_r2((char *)&(netif_info.ipaddr), temp, sizeof(temp));
	if (strcmp(g_pds_nvr_info.localIp, temp) != 0)
	{
		snprintf(g_pds_nvr_info.localIp, "%s", temp);
		rtn = 1;
	}

	port = nf_sysdb_get_uint("net.proto.webport");
	if (g_pds_nvr_info.publicHttpPort != port)
	{
		g_pds_nvr_info.publicHttpPort = port;
		rtn = 1;
	}
	if (g_pds_nvr_info.localHttpPort != port)
	{
		g_pds_nvr_info.localHttpPort = port;
		rtn = 1;
	}

	if (g_pds_nvr_info.secureHttp != nf_sysdb_get_bool("net.proto.httpson"))
	{
		g_pds_nvr_info.secureHttp = nf_sysdb_get_bool("net.proto.httpson");
		rtn = 1;
	}

	port = nf_sysdb_get_uint("net.rtp.rtspport");
	if (g_pds_nvr_info.publicRtspPort != port)
	{
		g_pds_nvr_info.publicRtspPort = port;
		rtn = 1;
	}
	if (g_pds_nvr_info.localRtspPort != port)
	{
		g_pds_nvr_info.localRtspPort = port;
		rtn = 1;
	}

	if (g_pds_nvr_info.serviceEnable != nf_sysdb_get_bool("net.ddns.p2p_enable"))
	{
		g_pds_nvr_info.serviceEnable = nf_sysdb_get_bool("net.ddns.p2p_enable");
		rtn = 1;
	}

	return rtn;
}

static int _pds_make_info_msg(PDS_PACKET *pack)
{
	g_assert(pack != NULL);

	memset(pack, 0x00, sizeof(PDS_PACKET));

	pack->data_len = snprintf(pack->data, PDS_PACKET_MAX_DATA_LEN,
		"{\n"
		"	\"serverId\": \"%s\",\n"
		"	\"contractNo\": \"%s\",\n"
		"	\"domainName\": \"%s\",\n"
		"	\"publicIp\": \"%s\",\n"
		"	\"localIp\": \"%s\",\n"
		"	\"publicHttpPort\": %d,\n"
		"	\"localHttpPort\": %d,\n"
		"	\"secureHttp\": %s,\n"
		"	\"publicRtspPort\": %d,\n"
		"	\"localRtspPort\": %d,\n"
		"	\"serviceEnable\": %s\n"
		"}\n",
		g_pds_nvr_info.serverId,
		g_pds_nvr_info.contractNo,
		g_pds_nvr_info.domainName,
		g_pds_nvr_info.publicIp,
		g_pds_nvr_info.localIp,
		g_pds_nvr_info.publicHttpPort,
		g_pds_nvr_info.localHttpPort,
		(g_pds_nvr_info.secureHttp ? "true" : "false"),
		g_pds_nvr_info.publicRtspPort,
		g_pds_nvr_info.localRtspPort,
		(g_pds_nvr_info.serviceEnable ? "true" : "false")
	);

#ifdef PDS_DEBUG
	printf("%s type[%08x] buff[%s](%d)\n", __FUNCTION__, pack->type, pack->data, pack->data_len);
#endif

	return pack->data_len;
}

static int _pds_send_msg(PDS_CTRL_FD *ctrl_fd, PDS_PACKET *pack)
{
	int write_len;
	int ret;

	g_return_val_if_fail (ctrl_fd != NULL, -1);
	g_return_val_if_fail (ctrl_fd->client_fd > 0, -1);
	g_return_val_if_fail (pack != NULL, -1);

	if (pack->data_len > PDS_PACKET_MAX_DATA_LEN)
	{
		PDS_LOG_PRINT("[%s][%d] PDS_PACKET_MAX_DATA_LEN error fd[%d] data_len[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, pack->data_len);
		return -3;
	}

	write_len = PDS_PACKET_HEADER_SIZE + (pack->data_len);

	pack->type = htonl(pack->type);
	pack->data_len = htonl(pack->data_len);

	ret = Writen(ctrl_fd->client_fd, pack, write_len);
	if (ret != write_len)
	{
		PDS_LOG_PRINT("[%s][%d] PACKET_DATA written fail fd[%d] ret[%d] len[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, ret, write_len);

		pack->type = htonl(pack->type);
		pack->data_len = htonl(pack->data_len);

		return -4;
	}

	pack->type = htonl(pack->type);
	pack->data_len = htonl(pack->data_len);

	PDS_LOG_PRINT("[%s][%d] PACKET_DATA fd[%d] type[%08x] ret[%d]\n",
			__FUNCTION__, __LINE__, ctrl_fd->client_fd, pack->type, ret);

	return ret;
}

static int _pds_recv_msg(PDS_CTRL_FD *ctrl_fd, PDS_PACKET *pack)
{
	int ret;

	g_return_val_if_fail(ctrl_fd != NULL, -1);
	g_return_val_if_fail(ctrl_fd->client_fd > 0, -1);
	g_return_val_if_fail(pack != NULL, -1);

	ret = Readn(ctrl_fd->client_fd, pack, PDS_PACKET_HEADER_SIZE);

	if (ret !=  PDS_PACKET_HEADER_SIZE)
	{
		PDS_LOG_PRINT("[%s][%d] PDS_PACKET_HEADER_SIZE readn fail fd[%d] ret[%d] pack->data_len[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, ret, pack->data_len);
		return -2;
	}

	pack->type = ntohl(pack->type);
	pack->data_len = ntohl(pack->data_len);

	if (pack->data_len > PDS_PACKET_MAX_DATA_LEN)
	{
		PDS_LOG_PRINT("[%s][%d] PDS_PACKET_MAX_DATA_LEN error fd[%d] data_len[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, pack->data_len);
		return -3;
	}

	ret = Readn( ctrl_fd->client_fd, pack->data, pack->data_len);
	if (ret != pack->data_len)
	{
		PDS_LOG_PRINT("[%s][%d] PACKET_DATA readn fail fd[%d] ret[%d] data_len[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, ret, pack->data_len);
		return -4;
	}

	PDS_LOG_PRINT("[%s][%d] PACKET_DATA fd[%d] type[%08x] len[%d]\n",
			__FUNCTION__, __LINE__, ctrl_fd->client_fd, pack->type, pack->data_len);

	return ret;
}

static int _pds_keepalive_msg(PDS_CTRL_FD *ctrl_fd, PDS_PACKET_E type)
{
	PDS_PACKET kalive_pack, *pack, ret_pack;
	int ret;
	int write_len;

	g_return_val_if_fail(ctrl_fd != NULL, -1);
	g_return_val_if_fail(ctrl_fd->client_fd > 0, -1);

	kalive_pack.type = ntohl(type);
	kalive_pack.data_len = ntohl(0);

	ret = Writen(ctrl_fd->client_fd, &kalive_pack, PDS_PACKET_HEADER_SIZE);
	if (ret != PDS_PACKET_HEADER_SIZE)
	{
		PDS_LOG_PRINT("[%s][%d] PACKET_DATA written fail fd[%d] ret[%d] len[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, ret, PDS_PACKET_HEADER_SIZE);
		return -2;
	}

	ret = Readn(ctrl_fd->client_fd, &ret_pack, PDS_PACKET_HEADER_SIZE);
	if (ret != PDS_PACKET_HEADER_SIZE)
	{
		PDS_LOG_PRINT("[%s][%d] PDS_PACKET_HEADER_SIZE readn fail fd[%d] ret[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, ret);
		return -3;
	}

	if (memcmp(&ret_pack, &kalive_pack, PDS_PACKET_HEADER_SIZE) != 0)
	{
		PDS_LOG_PRINT("[%s][%d] KEEPALIVE fail fd[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd);
		nf_debug_hexdump(&kalive_pack, PDS_PACKET_HEADER_SIZE);
		nf_debug_hexdump(&ret_pack , PDS_PACKET_HEADER_SIZE);
		return -4;
	}

	PDS_LOG_PRINT("[%s][%d] KEEPALIVE OK fd[%d] type[%08x]\n",
			__FUNCTION__, __LINE__, ctrl_fd->client_fd, type);

	return 1;
}

static int _pds_process_accept(PDS_CTRL_FD *ctrl_fd, int mode)
{
	int sock;
	struct sockaddr_in addr;
	socklen_t len;

	if (ctrl_fd->client_fd > 0)
	{
		PDS_LOG_PRINT("[%s][%d] mode[%d] disconnect previous client_fd[%d]\n",
				__FUNCTION__, __LINE__, mode, ctrl_fd->client_fd);
		_pds_close_sock(&ctrl_fd->client_fd);
	}

	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	len = sizeof(struct sockaddr_in);
	sock = Accept(ctrl_fd->conn_fd, (struct sockaddr *)&addr, &len);
	if (sock < 0)
	{
		PDS_LOG_PRINT("[%s][%d] mode[%d] accept failed! errno[%d]\n",
				__FUNCTION__, __LINE__, mode, errno);
		return -1;
	}

	ctrl_fd->client_fd = sock;
	++ctrl_fd->client_cnt;

	PDS_LOG_PRINT("[%s][%d] mode[%d] accept fd[%d] count[%d]\n",
			__FUNCTION__, __LINE__, mode, sock, ctrl_fd->client_cnt);

	// set non-blocking for read api
	_sock_set_timeout(sock, (unsigned int)3);

	return  sock;
}

static int _pds_process_client_pds(PDS_CTRL_INFO *pds_info, PDS_CTRL_FD *ctrl_fd)
{
	PDS_PACKET pack, out_pack;
	int ret;

	g_return_val_if_fail(ctrl_fd != NULL, -1);
	g_return_val_if_fail(ctrl_fd->client_fd > 0, -2);

	memset(&pack, 0x00, sizeof(PDS_PACKET));

	ret = _pds_recv_msg(ctrl_fd, &pack);
	if (ret < 0)
	{
		_pds_close_sock(&ctrl_fd->client_fd);
		return ret;
	}

	if (pack.type == PDS_PACKET_GET_NVR_INFO)
	{
		pds_info->info_pack.type = PDS_PACKET_GET_NVR_INFO;
		_pds_send_msg(ctrl_fd, &(pds_info->info_pack));

	}
	else if (pack.type == PDS_PACKET_NVR_CHECK_ALIVE)
	{
		_pds_send_msg(ctrl_fd, &pack);
	}
	else if (pack.type == PDS_PACKET_NOTI_CONN_STATUS)
	{
		int connection_status = 0;
		memcpy(&connection_status, pack.data, sizeof(connection_status));
		pds_info->is_rendez_connected = connection_status;
		if (pds_info->is_rendez_connected)
			pds_info->rendez_conn_last_check_time = time(0);

		PDS_LOG_PRINT("[%s][%d] PDS_PACKET_NOTI_CONN_STATUS(%d)\n",
				__FUNCTION__, __LINE__, connection_status);
	}
	else
	{
		PDS_LOG_PRINT("[%s][%d] wrong fd[%d] packet_type[%08x]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd, pack.type);
		return -999;
	}

	return 1;
}

static int _pds_process_keepalive(PDS_CTRL_INFO *pds_info)
{
	int ret;
	PDS_CTRL_FD *ctrl_fd = NULL;

	g_return_val_if_fail(pds_info != NULL, -1);

	ctrl_fd = &pds_info->nvr_req;

	if (ctrl_fd->client_fd < 0)
		return 1;

	ret = _pds_keepalive_msg(ctrl_fd, PDS_PACKET_PDS_CHECK_ALIVE);
	if (ret < 0)
	{
		PDS_LOG_PRINT("[%s][%d] kalive fail!!  close client fd[%d]\n",
				__FUNCTION__, __LINE__, ctrl_fd->client_fd);
		_pds_close_sock(&ctrl_fd->client_fd);
		return -9;
	}

	return 1;
}

static int _pds_process_changed_info(PDS_CTRL_INFO *pds_info)
{
	int ret;
	PDS_CTRL_FD *ctrl_fd = NULL;

	g_return_val_if_fail(pds_info != NULL, -1);

	ctrl_fd = &pds_info->nvr_req;

	if (ctrl_fd->client_fd < 0)
		return 1;

	if (pds_info->is_sysdb_changed)
	{
		pds_info->is_sysdb_changed = 0;

		if (_pds_check_nvr_change())
		{
			PDS_PACKET out_pack;

			_pds_make_info_msg(&(pds_info->info_pack));

			pds_info->info_pack.type = PDS_PACKET_CHANGE_NVR_INFO;
			ret = _pds_send_msg(ctrl_fd, &pds_info->info_pack);
			if (ret > 0)
			{
				ret = _pds_recv_msg(ctrl_fd, &out_pack);
				if (ret >= 0 && out_pack.type == PDS_PACKET_CHANGE_NVR_INFO)
				{
					return 1;
				}
				else
				{
					PDS_LOG_PRINT("[%s][%d] CHANGE_NVR_INFO recv fail! ret[%d]\n",
							__FUNCTION__, __LINE__, ret);
					return -2;
				}
			}
			else
			{
				PDS_LOG_PRINT("[%s][%d] CHANGE_NVR_INFO send fail! ret[%d]\n",
						__FUNCTION__, __LINE__, ret);
				return -3;
			}
		}
	}

	return 1;
}

static int _updater_script_restart()
{
	proxy_system("killall -9 updater.sh", 1, 3);
	return 0;
}

static int _pds_script_restart()
{
	proxy_system("killall -9 pds.sh", 1, 3);
	return 0;
}

static int _pds_proc_kill()
{
	proxy_system("killall -9 pds.bin", 1, 3);
	return 0;
}

