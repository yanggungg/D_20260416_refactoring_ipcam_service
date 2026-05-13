/*
 * ITX Security
 *  System software group
 *
 *  2013-01-31 Author YiDongHyung
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <glib.h>
#include <nf_api_ipcam.h>
#include <nf_ipcam_defs.h>
#include <nf_notify.h>
#include <nf_common.h>
#include "nf_ipcam_hub.h"
#include "jbshell.h"

extern gboolean nf_notify_fire_params(const gchar*, guint, guint, guint, guint);
extern int is_extention_macaddr(unsigned char* mac);

static int g_hub_sock = -1;
static pthread_t hub_recv_th;

IPX_HUB_MANAGE_TABLE		g_hub_manage_table[MAX_HUB_NUM];
IPX_HUB_PORT_MANAGE_TABLE	g_hub_port_manage_table[MAX_HUB_NUM][AVAILABLE_MAX_CH];
IPX_VHUB_PORT_MANAGE_TABLE	g_vhub_port_manage_table[AVAILABLE_MAX_CH];

IPX_HUB_MANAGE_TABLE		g_bef_hub_manage_table[MAX_HUB_NUM];
IPX_HUB_PORT_MANAGE_TABLE	g_bef_hub_port_manage_table[MAX_HUB_NUM][AVAILABLE_MAX_CH];
IPX_VHUB_PORT_MANAGE_TABLE	g_bef_vhub_port_manage_table[AVAILABLE_MAX_CH];

IPX_HUB_FW_VERSION_TABLE	g_hub_fw_version_table[MAX_HUB_NUM_SUPPORTED];

IPX_HUB_VCT_INFO			g_hub_vct_info[MAX_HUB_NUM][10];

extern void vhub_manager_init(void)
{
	gulong cb_handle = 0;
	
	printf("[%s] start\n", __FUNCTION__);
	
	cb_handle= nf_notify_connect_cb( "sysdb_change", hub_sysdb_reload_cb_func, NULL);
	g_message("%s reload sysdb connect_cb[%ld]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);
	
	pthread_create(&hub_recv_th, NULL, (void*)&hub_recv_func, NULL);
	printf("[%s] end\n", __FUNCTION__);
}

static void hub_recv_func()
{
	int ret1, ret2, ret3;
	HUB_DATA_T recv_msg;
	HUB_PORT_STATUS_T recv_hub_port_status;
	HUB_VCT_INFO_T recv_hub_vct_info;

	ret1 = hub_recv_func_init();
	if (ret1 < 0) {
		printf("[%s] Failed:hub_recv_func_init()\n", __FUNCTION__);
		return;
	}

	/** iHUB main loop **/
	while(1) {
		hub_recv_func_check_data_changed();

		// Trigger event for wan link/unlink
		hub_recv_func_check_wan_link();

		ret1 = hub_recv_func_receive_msg(&recv_msg, &recv_hub_port_status, &recv_hub_vct_info);
		if (ret1 == 1) {
			hub_recv_func_process_msg(&recv_msg, &recv_hub_port_status, &recv_hub_vct_info);
		}

		ret2 = hub_recv_func_process_req();
		hub_recv_func_process_timeout();
		hub_recv_func_process_job();

		ret3 = vhub_process_req();

		if (ret1 < 0 && !ret2 && !ret3) {
			usleep(20*1000);
			continue;
		}
		else {
			usleep(10*1000);
			continue;
		}
	}
}

static int hub_recv_func_init()
{
	int i;
	int ret;

	char fw_path[256] = {0};
	struct stat buf;
	
	hub_manage_table_init();
	memset(g_hub_port_manage_table, 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*MAX_HUB_NUM*AVAILABLE_MAX_CH);
	vhub_init();

	// 지??하??HUB FW Version??보 ??싱.
	for (i = 0; i < MAX_HUB_NUM_SUPPORTED; i++) {
		unsigned char sysdb_key[24];
		unsigned char hubFWVersion_dvr[24];
		int dvr_product_code;
		int dvr_proto_code;
		int dvr_minor_code;
		int dvr_buyer_code;

		memset(sysdb_key, 0x00, sizeof(sysdb_key));
		memset(hubFWVersion_dvr, 0x00, sizeof(hubFWVersion_dvr));

		snprintf(sysdb_key, sizeof(sysdb_key), "sys.info.hubver%d", i + 1);
		snprintf(hubFWVersion_dvr, sizeof(hubFWVersion_dvr), "%s", nf_sysdb_get_str_nocopy(sysdb_key));
		ret = hub_parse_fwver(hubFWVersion_dvr, &dvr_product_code, &dvr_proto_code, &dvr_minor_code, &dvr_buyer_code);

		if (ret) {
			printf("[%s] Failed:hub_parse_fwver()\n", __FUNCTION__);
			return -1;
		}
		if(dvr_product_code == HUB_PRODUCT_VER_1ST)
		{
			snprintf(fw_path, sizeof(fw_path), "%s/%d.bin", HUB_FW_IMAGE_PATH, HUB_PRODUCT_VER_1ST);
		}
		else if(dvr_product_code == HUB_PRODUCT_VER_2ND)
		{
			snprintf(fw_path, sizeof(fw_path), "%s/%d.nbn", HUB_FW_IMAGE_PATH, HUB_PRODUCT_VER_2ND);
		}
		else
		{
			snprintf(fw_path, sizeof(fw_path), "%s/%d.nbn", HUB_FW_IMAGE_PATH, HUB_PRODUCT_VER_3RD);
		}

		memset(&buf, 0x00, sizeof(buf));
		if(stat(fw_path, &buf) == 0)
		{
			snprintf(g_hub_fw_version_table[i].fw_path, sizeof(g_hub_fw_version_table[i].fw_path), "%s", fw_path);
			g_hub_fw_version_table[i].fw_have = HUB_TRUE;
		}
		else
		{
			g_hub_fw_version_table[i].fw_path[0] = '\0';
			g_hub_fw_version_table[i].fw_have = HUB_FALSE;
		}

		memcpy(g_hub_fw_version_table[i].hubFWVersion_dvr, hubFWVersion_dvr, sizeof(hubFWVersion_dvr));
		g_hub_fw_version_table[i].dvr_product_code = dvr_product_code;
		g_hub_fw_version_table[i].dvr_proto_code = dvr_proto_code;
		g_hub_fw_version_table[i].dvr_minor_code = dvr_minor_code;
		g_hub_fw_version_table[i].dvr_buyer_code = dvr_buyer_code;

		/*
		printf("[%s] hubFWVersion_dvr:[%s]\n", __FUNCTION__, hubFWVersion_dvr);
		printf("[%s] sysdb_key:[%s]\n", __FUNCTION__, sysdb_key);
		printf("[%s] dvr_product_code:[%d]\n", __FUNCTION__, dvr_product_code);
		printf("[%s] dvr_proto_code:[%d]\n", __FUNCTION__, dvr_proto_code);
		printf("[%s] dvr_minor_code:[%d]\n", __FUNCTION__, dvr_minor_code);
		printf("[%s] dvr_buyer_code:[%d]\n", __FUNCTION__, dvr_buyer_code);
		printf("[%s] hubFWVersion_dvr:[%s]\n", __FUNCTION__, g_hub_fw_version_table[i].hubFWVersion_dvr);
		printf("[%s] fw_path:[%s]\n", __FUNCTION__, g_hub_fw_version_table[i].fw_path);
		*/
	}

	ret = hub_open_sock();
	if (ret < 0) {
		printf("[%s] Failed:hub_open_sock()\n", __FUNCTION__);
		return -1;
	}

	return 1;
}

static int hub_make_fwver_strs(char *buf, size_t buf_size)
{
	int idx = 0;
	size_t off = 0;
	size_t len = 0;

	if(buf == NULL || buf_size == 0)
	{
		return -1;
	}

	for(idx = 0; idx < MAX_HUB_NUM_SUPPORTED; idx++)
	{
		if( (g_hub_fw_version_table[idx].fw_have == TRUE) && 
		     ((buf_size - off) > (strlen(g_hub_fw_version_table[idx].hubFWVersion_dvr)+1)) )
		{
			len = snprintf(buf+off, buf_size-off, "%s&", g_hub_fw_version_table[idx].hubFWVersion_dvr);
			off += len;
		}
	}

	if(off != 0)
	{
		buf[off-1] = '\0';
	}

	return 0;
}

static void hub_recv_func_check_wan_link()
{
	static int hub_link_status_pre = -1;
	static int hub_link_status_cur = -1;
	int port = HUB_PORT;

	nf_dev_switch_get_link_status(&port);
	if (port == 0)
		hub_link_status_cur = 0;
	else
		hub_link_status_cur = 1;

	if (hub_link_status_pre != hub_link_status_cur) {
		hub_link_status_pre = hub_link_status_cur;

		if (!hub_link_status_cur) {
			hub_disconnect_all_hubs();
		}
	}
}

static int hub_recv_func_receive_msg(HUB_DATA_T* p_recv_msg, HUB_PORT_STATUS_T* p_recv_port_status, HUB_VCT_INFO_T* p_recv_vct_info)
{
	struct sockaddr_in cin;
	size_t cin_len;

	HUB_DATA_T hub_sock_buf;
	int len, ret;

	memset(&hub_sock_buf,		0x00, sizeof(HUB_DATA_T));
	memset(p_recv_msg,			0x00, sizeof(HUB_DATA_T));
	memset(p_recv_port_status,	0x00, sizeof(HUB_PORT_STATUS_T));

	cin_len = sizeof(cin);
	len = recvfrom(g_hub_sock, (void*)&hub_sock_buf, sizeof(HUB_DATA_T), MSG_DONTWAIT, (struct sockaddr*)&cin, &cin_len);
	if (len <= 0)
		return -1;

	// non-blocking????른 ??인.
	if (hub_sock_buf.code == 0)
		return -1;

	if (ntohl(hub_sock_buf.code) == CODE_PORT_STATUS) {
		memcpy(p_recv_port_status, &hub_sock_buf, sizeof(HUB_PORT_STATUS_T));
		p_recv_port_status->code			= ntohl(p_recv_port_status->code);
		p_recv_port_status->portNum			= ntohl(p_recv_port_status->portNum);
		p_recv_port_status->is_discovery	= ntohl(p_recv_port_status->is_discovery);
		p_recv_port_status->is_active		= ntohl(p_recv_port_status->is_active);
		p_recv_port_status->port_class		= ntohl(p_recv_port_status->port_class);
		p_recv_port_status->func_status		= ntohl(p_recv_port_status->func_status);
		p_recv_port_status->consumption		= ntohl(p_recv_port_status->consumption);
		p_recv_port_status->voltage			= ntohl(p_recv_port_status->voltage);
		p_recv_port_status->current_mA		= ntohl(p_recv_port_status->current_mA);

		p_recv_msg->code = p_recv_port_status->code;
		memcpy(p_recv_msg->hubmac, p_recv_port_status->hubmac, 6);
	}
	else if (ntohl(hub_sock_buf.code) == CODE_HUB_VCT_RES) {
		memcpy(p_recv_vct_info, &hub_sock_buf, sizeof(HUB_VCT_INFO_T));
		p_recv_vct_info->code			= ntohl(p_recv_vct_info->code);
		p_recv_vct_info->portNum		= ntohl(p_recv_vct_info->portNum);
		p_recv_vct_info->length[0]		= ntohl(p_recv_vct_info->length[0]);
		p_recv_vct_info->length[1]   	= ntohl(p_recv_vct_info->length[1]);
		p_recv_vct_info->length[2]		= ntohl(p_recv_vct_info->length[2]);
		p_recv_vct_info->length[3]		= ntohl(p_recv_vct_info->length[3]);

		p_recv_msg->code = p_recv_vct_info->code;
		memcpy(p_recv_msg->hubmac, p_recv_vct_info->hubmac, 6);
	}
	else {
		int checksum;

		memcpy(p_recv_msg, &hub_sock_buf, sizeof(HUB_DATA_T));
		p_recv_msg->code	= ntohl(hub_sock_buf.code);
		p_recv_msg->portNum	= ntohl(hub_sock_buf.portNum);
		p_recv_msg->data1	= ntohl(hub_sock_buf.data1);
		p_recv_msg->data2	= ntohl(hub_sock_buf.data2);
		p_recv_msg->chksum	= ntohs(hub_sock_buf.chksum);

		checksum = hub_make_checksum(&hub_sock_buf);

		if (checksum != p_recv_msg->chksum) {
			printf("[%s] CHECKSUM_MISMATCH(rcv_len:%d calculated_len:%d rcv:%d, calculated:%d)\n",
				__FUNCTION__, len, sizeof(HUB_DATA_T), p_recv_msg->chksum, checksum);
			return 0;
		}
	}

	//hub_msg_print(p_recv_msg);

	ret = hub_recv_func_receive_msg_filter(p_recv_msg, p_recv_port_status);
	return ret;
}

static int	hub_recv_func_receive_msg_filter(HUB_DATA_T* p_recv_msg, HUB_PORT_STATUS_T* p_recv_port_status)
{
	int idx;
	unsigned char hubmac[6];
	memcpy(hubmac, p_recv_msg->hubmac, sizeof(hubmac));

	idx = hub_manage_table_search(p_recv_msg->hubmac);

	// ??속 ??작 메시지가 ??니?????록??HUB MAC???받아??임.
	if (!(p_recv_msg->code == CODE_POLL_NVR || p_recv_msg->code == CODE_ALIVE)) {
		if (idx == -1) {
			printf("[%s] hubmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__,
					p_recv_msg->hubmac[0], p_recv_msg->hubmac[1],p_recv_msg->hubmac[2],
					p_recv_msg->hubmac[3], p_recv_msg->hubmac[4],p_recv_msg->hubmac[5]);
			printf("[%s] This msg's hubmac is not registerd at hub_manage_table.\n", __FUNCTION__);
			hub_msg_print(p_recv_msg);
			return 0;
		}

		if (g_hub_manage_table[idx].status == 0) {
			printf("[%s] hubmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__,
					p_recv_msg->hubmac[0], p_recv_msg->hubmac[1],p_recv_msg->hubmac[2],
					p_recv_msg->hubmac[3], p_recv_msg->hubmac[4],p_recv_msg->hubmac[5]);
			printf("[%s] This msg's hubmac is not running at hub_manage_table.\n", __FUNCTION__);
			hub_msg_print(p_recv_msg);
			return 0;
		}

		if (g_hub_manage_table[idx].status == HUB_PORT_STATE_RECONNECT_REQ ) {
			printf("[%s] hubmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__,
					p_recv_msg->hubmac[0], p_recv_msg->hubmac[1],p_recv_msg->hubmac[2],
					p_recv_msg->hubmac[3], p_recv_msg->hubmac[4],p_recv_msg->hubmac[5]);
			printf("[%s] This msg's hubmac is target of HUB_PORT_STATE_RECONNECT_REQ.\n", __FUNCTION__);
			hub_msg_print(p_recv_msg);
			return 0;
		}
	}
	else {
		if (idx != -1 && p_recv_msg->code == CODE_ALIVE) {
			if (g_hub_manage_table[idx].status == HUB_PORT_STATE_RECONNECT_REQ ) {
				printf("[%s] hubmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__,
						p_recv_msg->hubmac[0], p_recv_msg->hubmac[1],p_recv_msg->hubmac[2],
						p_recv_msg->hubmac[3], p_recv_msg->hubmac[4],p_recv_msg->hubmac[5]);
				printf("[%s] This msg's hubmac is target of HUB_PORT_STATE_RECONNECT_REQ.\n", __FUNCTION__);
				hub_msg_print(p_recv_msg);
				return 0;
			}
			return 1;
		}

		if (!is_extention_macaddr(hubmac)) {
			/*
			// for debug
			printf("[%s] hubmac:[%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__,
					p_recv_msg->hubmac[0], p_recv_msg->hubmac[1],p_recv_msg->hubmac[2],
					p_recv_msg->hubmac[3], p_recv_msg->hubmac[4],p_recv_msg->hubmac[5]);
			printf("[%s] This msg's hubmac is not registerd at lan mactable.\n", __FUNCTION__);
			hub_msg_print(p_recv_msg);
			*/
			return 0;
		}
	}

	/*
	// ??속 ??작 메시지??면 FWVER????라????속??Physical Port??인.
	else {
		int port_restrict = 0;
		int hub_product_code, hub_proto_code, hub_minor_code, hub_buyer_code;
		int ret, i, cnt = 0;

		// 구버??HUB
		if (p_recv_msg->fwver[0] == NULL) {
			port_restrict = 0;
		}
		else {
			ret = hub_parse_fwver(p_recv_msg->fwver, &hub_product_code, &hub_proto_code, &hub_minor_code, &hub_buyer_code);
			if (ret) {
				printf("[%s] This msg's fwver is not parsed.\n", __FUNCTION__);
				return 0;
			}

			for (i = 0; i < MAX_HUB_NUM_SUPPORTED; i++) {
				if (g_hub_fw_version_table[i].dvr_product_code == hub_product_code &&
					g_hub_fw_version_table[i].dvr_proto_code == hub_proto_code) {
					break;
				}
			}

			if (i == MAX_HUB_NUM_SUPPORTED) {
				printf("[%s] Not supported hub fwver.\n", __FUNCTION__);
				return 0;
			}

			port_restrict = 1;
		}

		if (port_restrict == 0) {
			if (!is_extention_macaddr(p_recv_port_status->hubmac)) {
				printf("[%s] This msg's hubmac is not registerd at wan mactable.\n", __FUNCTION__);
				return 0;
			}

			//printf("[%s] OK\n", __FUNCTION__);
		}
		else if (port_restrict == 1) {
			if (!is_extention_macaddr(p_recv_port_status->hubmac))
				printf("[%s] This msg's hubmac is not registerd at wan mactable.\n", __FUNCTION__);
		}
	}
	*/

	return 1;
}

static void	hub_recv_func_process_msg(HUB_DATA_T* p_recv_msg, HUB_PORT_STATUS_T* p_recv_port_status, HUB_VCT_INFO_T* p_recv_vct_info)
{
	switch (p_recv_msg->code) {
	case CODE_POLL_NVR:
		hub_handler_poll(p_recv_msg);
		break;
	case CODE_IP_SET:
		hub_handler_ipset(p_recv_msg);
		break;
	case CODE_FWUP_RES:
		hub_handler_fwup_res(p_recv_msg);
		break;
	case CODE_FILE_TRANS_DONE:
	case CODE_FILE_TRANS_FAIL_SZ:
	case CODE_FILE_TRANS_FAIL_CRC:
	case CODE_FILE_TRANS_FAIL_NAND:
	case CODE_FILE_TRANS_FAIL:
	case CODE_FILE_TRANS_TIMEOUT:
		hub_handler_fwup_done(p_recv_msg);
		break;
	case CODE_ALIVE:
		hub_handler_alive(p_recv_msg);
		break;
	case CODE_DEVICE_MACADDR_RES:
		hub_handler_macaddr(p_recv_msg);
		break;
	case CODE_DEVICE_LINKED:
		hub_handler_linked(p_recv_msg);
		break;
	case CODE_DEVICE_UNLINKED:
		hub_handler_unlinked(p_recv_msg);
		break;
	case CODE_UNLINK_RES:
		hub_handler_unlink_res(p_recv_msg);
		break;
	case CODE_REBOOT_POE_RES:
		hub_handler_poe_reset_res(p_recv_msg);
		break;
	case CODE_HUB_POE_OFF_RES:
		hub_handler_poe_off_res(p_recv_msg);
		break;
	case CODE_HUB_POE_ON_RES:
		hub_handler_poe_on_res(p_recv_msg);
		break;
	case CODE_CAMFWUP_RES:
		hub_handler_camfwup_res(p_recv_msg);
		break;
	case CODE_PORT_STATUS:
		hub_handler_port_status(p_recv_port_status);
		break;
	case CODE_CONN_ORDER:
		hub_handler_conn_order(p_recv_msg);
		break;
	case CODE_HUB_VCT_RES:
		hub_handler_vct_info(p_recv_vct_info);
		break;
	}

	return;
}

static int hub_recv_func_process_req()
{
	static int fwup_req_cnt = 0;
	static int fwup_req_check_time = 0;
	static unsigned char fwup_req_hub_mac[6];
	struct timespec now_time;

	int i;
	int req_cnt = 0;

	clock_gettime(CLOCK_REALTIME, &now_time);

	// Make/Send req msg.
	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (!memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6))
			continue;

		if ((g_hub_manage_table[i].status & HUB_PORT_STATE_FWUP_WAIT) && !fwup_req_cnt) {
			fwup_req_cnt++;
			fwup_req_check_time = now_time.tv_sec;
			memcpy(fwup_req_hub_mac, g_hub_manage_table[i].macaddr, sizeof(fwup_req_hub_mac));
			g_hub_manage_table[i].status = HUB_PORT_STATE_UBOOT_REQ;
		}

		if (g_hub_manage_table[i].status & HUB_PORT_STATE_UBOOT_REQ) {
			hub_handler_req_fwup(i);
			req_cnt++;
		}

		if (g_hub_manage_table[i].status & HUB_PORT_STATE_FWUP_COMP) {
			fwup_req_cnt = 0;
			fwup_req_check_time = 0;
			memset(fwup_req_hub_mac, 0x00, sizeof(fwup_req_hub_mac));
			printf("[%s] FWUP Complete.\n", __FUNCTION__);
			hub_manage_table_set_unlink(i);
		}

		if (g_hub_manage_table[i].status & HUB_PORT_STATE_RECONNECT_REQ) {
			if (now_time.tv_sec < g_hub_manage_table[i].check_time + 10) {
				hub_handler_req_reconnect(i);
				req_cnt++;
			}
		}

		if (g_hub_manage_table[i].status & HUB_PORT_STATE_CAMFWUP_REQ) {
			if (now_time.tv_sec < g_hub_manage_table[i].check_time + 10) {
				hub_handler_req_camfwup(i);
				req_cnt++;
			}
		}
	}

	// Check fw upgrade timeout.
	if (fwup_req_cnt && fwup_req_check_time + 90 < now_time.tv_sec) {
		i = hub_manage_table_search(fwup_req_hub_mac);
		if (i != -1) {
			printf("[%s] FWUP Timeout!\n", __FUNCTION__);
			hub_manage_table_set_unlink(i);
		}

		fwup_req_cnt = 0;
		fwup_req_check_time = 0;
		memset(fwup_req_hub_mac, 0x00, sizeof(fwup_req_hub_mac));
	}

	return req_cnt;
}

static void hub_recv_func_process_timeout()
{
	int i;
	struct timespec now_time;

	clock_gettime(CLOCK_REALTIME, &now_time);

	// Check Timeout case
	for (i = 0; i < MAX_HUB_NUM; i++) {
		// Check req timeout
		if (g_hub_manage_table[i].status & HUB_PORT_STATE_RECONNECT_REQ) {
			if (g_hub_manage_table[i].check_time + 13 < now_time.tv_sec) {
				printf("[%s] reconnect req timeout!\n", __FUNCTION__);
				hub_manage_table_set_unlink(i);
			}
		}

		// Check check_time of hub_manage_table
		if (g_hub_manage_table[i].status & (HUB_PORT_STATE_LINKED|HUB_PORT_CONNECTION_COMPLETE)) {
			if (g_hub_manage_table[i].check_time + 10 < now_time.tv_sec) {
				printf("[%s] paring is broken.\n", __FUNCTION__);
				hub_manage_table_set_unlink(i);
			}
		}
	}
}

static void	hub_recv_func_process_job()
{

}

static void hub_recv_func_check_data_changed()
{
	static int init = 0;
	int i, j;

	if (!init) {
		memset(g_bef_hub_manage_table, 		0x00, sizeof(IPX_HUB_MANAGE_TABLE)*MAX_HUB_NUM);
		memset(g_bef_hub_port_manage_table,	0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*MAX_HUB_NUM*AVAILABLE_MAX_CH);
		memset(g_bef_vhub_port_manage_table,0x00, sizeof(IPX_VHUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);

		init = 1;
	}

	for (i = 0; i < MAX_HUB_NUM; i++) {
		g_bef_hub_manage_table[i].check_time = g_hub_manage_table[i].check_time;
		for (j = 0; j < AVAILABLE_MAX_CH; j++)
			g_bef_hub_port_manage_table[i][j].recv_cnt = g_hub_port_manage_table[i][j].recv_cnt;
	}

	if (memcmp(g_bef_hub_manage_table, g_hub_manage_table, sizeof(IPX_HUB_MANAGE_TABLE)*MAX_HUB_NUM)) {
		hub_manage_table_print();
		memcpy(g_bef_hub_manage_table, g_hub_manage_table, sizeof(IPX_HUB_MANAGE_TABLE)*MAX_HUB_NUM);
	}

	if (memcmp(g_bef_hub_port_manage_table, g_hub_port_manage_table, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*MAX_HUB_NUM*AVAILABLE_MAX_CH)) {
		hub_port_manage_table_print();
		memcpy(g_bef_hub_port_manage_table, g_hub_port_manage_table, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*MAX_HUB_NUM*AVAILABLE_MAX_CH);
	}

	if (memcmp(g_bef_vhub_port_manage_table, g_vhub_port_manage_table, sizeof(IPX_VHUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH)) {
		vhub_table_print();
		memcpy(g_bef_vhub_port_manage_table, g_vhub_port_manage_table, sizeof(IPX_VHUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);
	}
}

static int hub_open_sock()
{
	struct sockaddr_in sin;
	const int sockopt_flag = 1;

	g_hub_sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (g_hub_sock < 0) {
		printf("[%s] recv socket alloc fail\n", __FUNCTION__);
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(RECV_PORT_FROM_HUB);

#ifdef DUAL_LAN_NETWORK
	// Wating HUB Port IP Assigne
	while(get_hub_interface_addr() == 0)
	{
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec =1000000 * 100; // 100ms
		clock_nanosleep(CLOCK_MONOTONIC, 0, &req, NULL);
	}
#endif

	if (bind(g_hub_sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		printf("[%s] recv socket bind fail\n", __FUNCTION__);
		return -1;
	}

	setsockopt(g_hub_sock, SOL_SOCKET, SO_BROADCAST, &sockopt_flag, sizeof(sockopt_flag));

#ifdef DUAL_LAN_NETWORK
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), HUB_ETH_DEVICE);
	if (setsockopt(g_hub_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0)
	{
		printf("[%s:%d] socketopt fail\n", __FUNCTION__, __LINE__);
	}
#endif

	return g_hub_sock;
}

static int hub_send_msg_broadcast(HUB_DATA_T* p_send_msg)
{
	struct sockaddr_in sin;
	int chksum;
	int ret;

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_BROADCAST;
	sin.sin_port = htons(SEND_PORT_TO_HUB);

	chksum = hub_make_checksum(p_send_msg);
	p_send_msg->chksum = (unsigned short)chksum;
	p_send_msg->chksum = htons(p_send_msg->chksum);

	//hub_send_msg_print(p_send_msg);

	ret = sendto(g_hub_sock, (void*)p_send_msg, sizeof(HUB_DATA_T), 0, (struct sockaddr*)&sin, sizeof(sin));

	//printf("[%s] ret : %d\n", __FUNCTION__, ret);
	return ret;
}

static int hub_send_msg_target(HUB_DATA_T* p_send_msg, unsigned int p_target_ip)
{
	struct sockaddr_in sin;
	int chksum;
	int ret;

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = p_target_ip;
	sin.sin_port = htons(SEND_PORT_TO_HUB);

	chksum = hub_make_checksum(p_send_msg);
	p_send_msg->chksum = (unsigned short)chksum;
	p_send_msg->chksum = htons(p_send_msg->chksum);

	//hub_send_msg_print(p_send_msg);

	ret = sendto(g_hub_sock, (void*)p_send_msg, sizeof(HUB_DATA_T), 0, (struct sockaddr*)&sin, sizeof(sin));

	//printf("[%s] ret : %d\n", __FUNCTION__, ret);
	return ret;
}

static void hub_send_msg_print(HUB_DATA_T* p_send_msg)
{
	unsigned int x;
	printf("[%s] Start\n", __FUNCTION__);

	printf("code     : [%d]\n", ntohl(p_send_msg->code));
	printf("portNUM  : [%d]\n", ntohl(p_send_msg->portNum));

	printf("hubmac   : [%02x:%02x:%02x:%02x:%02x:%02x]\n",
			p_send_msg->hubmac[0],
			p_send_msg->hubmac[1],
			p_send_msg->hubmac[2],
			p_send_msg->hubmac[3],
			p_send_msg->hubmac[4],
			p_send_msg->hubmac[5]);

	x = p_send_msg->data1;
	printf("data1    : [%02x:%02x:%02x:%02x]\n",
			(int) ((x >> 24) & 0xff),
			(int) ((x >> 16) & 0xff),
			(int) ((x >> 8) & 0xff),
			(int) ((x >> 0) & 0xff));

	x = p_send_msg->data2;
	printf("data2    : [%02x:%02x:%02x:%02x]\n",
			(int) ((x >> 24) & 0xff),
			(int) ((x >> 16) & 0xff),
			(int) ((x >> 8) & 0xff),
			(int) ((x >> 0) & 0xff));

	printf("mac      : [%02x:%02x:%02x:%02x:%02x:%02x]\n",
			p_send_msg->mac[0],
			p_send_msg->mac[1],
			p_send_msg->mac[2],
			p_send_msg->mac[3],
			p_send_msg->mac[4],
			p_send_msg->mac[5]);

}

static void hub_msg_print(HUB_DATA_T* p_send_msg)
{
	unsigned int x;
	printf("[%s] Start\n", __FUNCTION__);

	printf("code     : [%d]\n", p_send_msg->code);
	printf("portNUM  : [%d]\n", p_send_msg->portNum);

	printf("hubmac   : [%02x:%02x:%02x:%02x:%02x:%02x]\n",
			p_send_msg->hubmac[0],
			p_send_msg->hubmac[1],
			p_send_msg->hubmac[2],
			p_send_msg->hubmac[3],
			p_send_msg->hubmac[4],
			p_send_msg->hubmac[5]);

	x = p_send_msg->data1;
	printf("data1    : [%02x:%02x:%02x:%02x]\n",
			(int) ((x >> 24) & 0xff),
			(int) ((x >> 16) & 0xff),
			(int) ((x >> 8) & 0xff),
			(int) ((x >> 0) & 0xff));

	x = p_send_msg->data2;
	printf("data2    : [%02x:%02x:%02x:%02x]\n",
			(int) ((x >> 24) & 0xff),
			(int) ((x >> 16) & 0xff),
			(int) ((x >> 8) & 0xff),
			(int) ((x >> 0) & 0xff));

	printf("mac      : [%02x:%02x:%02x:%02x:%02x:%02x]\n",
			p_send_msg->mac[0],
			p_send_msg->mac[1],
			p_send_msg->mac[2],
			p_send_msg->mac[3],
			p_send_msg->mac[4],
			p_send_msg->mac[5]);

}

static int hub_send_ext_target(HUB_EXT_T* p_send_ext, unsigned int p_target_ip)
{
	struct sockaddr_in sin;
	int chksum;
	int ret;

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = p_target_ip;
	sin.sin_port = htons(SEND_PORT_TO_HUB);

	ret = sendto(g_hub_sock, (void*)p_send_ext, sizeof(HUB_DATA_T), 0, (struct sockaddr*)&sin, sizeof(sin));

	printf("[%s] ret : %d\n", __FUNCTION__, ret);
	return ret;
}

static int hub_make_checksum(HUB_DATA_T* h_data)
{
	unsigned char *p = NULL;
	int i = 0, len;
	int checksum = 0;

    p = (unsigned char*)h_data;
    len = sizeof(HUB_DATA_T);

    for (i = 0; i < len; i++) {
    	if (52 <= i && i <= 53) continue;
    	checksum += (*(p+i));
    }
    return checksum;
}

static int hub_make_ipaddr(unsigned int* p_ipaddr)
{
#ifdef DUAL_LAN_NETWORK
	const unsigned int class_c_ipbase = get_hub_info() & get_host_netmask();
#else
	const unsigned int class_c_ipbase = get_host_info() & get_host_netmask();
#endif
	int i, j, cnt;
	unsigned char tt;

	printf("[%s] start\n", __FUNCTION__);

	if (class_c_ipbase == 0)
		return 0;

	// Create
	for (i = 100; i < 120; i++) {
		cnt = 0;

		for (j = 0; j < MAX_HUB_NUM; j++) {
			if (memcmp(g_hub_manage_table[j].macaddr, hub_manage_table_empty_entry, 6)) {
				if (((g_hub_manage_table[j].assigned_ip >> 24) & 0xff) == i)
					cnt++;
			}
		}

		if (!cnt) {
			*p_ipaddr = class_c_ipbase | (i << 24);
			break;
		}
	}

	// Print
	{
		unsigned int x;
		x = htonl(*p_ipaddr);
		printf("[%s] ipaddr[%d] : %d.%d.%d.%d\n", __FUNCTION__, *p_ipaddr,
				(int) ((x >> 24) & 0xff),
				(int) ((x >> 16) & 0xff),
				(int) ((x >> 8) & 0xff),
				(int) ((x >> 0) & 0xff));
	}

	printf("[%s] end\n", __FUNCTION__);
	return 1;
}

static int hub_check_fwver(int p_idx)
{
	unsigned char hubFWVersion_hub[24];
	unsigned int ret = 0;

	int i;
	int hub_product_code, hub_proto_code, hub_minor_code, hub_buyer_code;

	static int nfs_fwup_cnt[MAX_HUB_NUM] = { 0, };

	printf("[%s] Start\n", __FUNCTION__);

	snprintf(hubFWVersion_hub, sizeof(hubFWVersion_hub), "%s", g_hub_manage_table[p_idx].fw_version_hub);

	ret = hub_parse_fwver(hubFWVersion_hub, &hub_product_code, &hub_proto_code, &hub_minor_code, &hub_buyer_code);
	if (ret)
		goto RETURN_HUB_CHECK_FWVER;

	// 28200.1.1.100
	printf("[%s] HUBFWVersion(from hub): %s %d %d %d %d\n", __FUNCTION__, hubFWVersion_hub, hub_product_code, hub_proto_code, hub_minor_code, hub_buyer_code);

	for (i = 0; i < MAX_HUB_NUM_SUPPORTED; i++) {
		if (g_hub_fw_version_table[i].dvr_product_code == hub_product_code &&
			g_hub_fw_version_table[i].dvr_proto_code == hub_proto_code) {
			break;
		}
	}

	if (i == MAX_HUB_NUM_SUPPORTED) {
		ret = 1;
		goto RETURN_HUB_CHECK_FWVER;
	}

	memcpy(g_hub_manage_table[p_idx].fw_version_dvr, g_hub_fw_version_table[i].hubFWVersion_dvr, sizeof(g_hub_manage_table[p_idx].fw_version_dvr));

	// ihub4 dev exception, ihub3 is all connect
	if (hub_product_code == HUB_PRODUCT_VER_2ND)
	{
		ret = 0;
		goto RETURN_HUB_CHECK_FWVER;
	}

	if (g_hub_fw_version_table[i].dvr_minor_code != hub_minor_code) {
		int fw_up_count = 0;
		for (i = 0; i < MAX_HUB_NUM; i++) {
			if (g_hub_manage_table[i].status & (HUB_PORT_STATE_FWUP_WAIT|HUB_PORT_STATE_FWUP_START|HUB_PORT_STATE_UBOOT_REQ|HUB_PORT_STATE_FWUP_COMP))
				fw_up_count++;
		}

		if (fw_up_count)
			ret = 3; // skip
		else
			ret = 2;

		goto RETURN_HUB_CHECK_FWVER;
	}

	// If this hub is 1st version hub, must set matching_chs to HUB_MATCHING_CHS_FIXED.
	if (hub_product_code == HUB_PRODUCT_VER_1ST) {
		if (!(g_hub_manage_table[p_idx].matching_chs & HUB_MATCHING_CHS_FIXED)) {
			unsigned char sysdb_key_hub_chs[256];
			g_hub_manage_table[p_idx].matching_chs |= HUB_MATCHING_CHS_FIXED;
			snprintf(sysdb_key_hub_chs, 	256, "sys.hub.H%d.chs",		p_idx);

			nf_sysdb_lock(NF_SYSDB_CATE_SYS);
			nf_sysdb_set_uint(sysdb_key_hub_chs, g_hub_manage_table[p_idx].matching_chs);
			nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
			nf_sysdb_save("sys");
			nf_sysdb_save_flush();
		}
	}

RETURN_HUB_CHECK_FWVER:

	// rtnVal = 1.  If Model Types are not matched, disconnect hub.
	// rtnVal = 2.  Start hub fwup.
	// rtnVal = 0.  Connect.

/*
	{
		if (nfs_fwup_cnt[p_idx]%3 == 0) {
			ret = 2;
		}
		nfs_fwup_cnt[p_idx]++;
	}
*/
	printf("[%s] Return Value : %d\n", __FUNCTION__, ret);
	printf("[%s] End\n", __FUNCTION__);

	return ret;
}

static int hub_parse_fwver(char* str_fwver, int* p_product_code, int* p_proto_code, int* p_minor_code, int* p_buyer_code)
{
	int i = 0, j = 0;
	int str_fwver_len = 0;
	int count_of_dot = 0;
	int pre_part_pos = 0;

	int retVal = 0;

	str_fwver_len = strlen(str_fwver);

	// 28200.1.1.100
	while (i < str_fwver_len) {
		if (str_fwver[i] == '.') {
			count_of_dot++;

			if (count_of_dot == 1) {
				*p_product_code = hub_fwver_part_to_int(str_fwver, pre_part_pos, i);
			}
			if (count_of_dot == 2) {
				*p_proto_code = hub_fwver_part_to_int(str_fwver, pre_part_pos, i);
			}
			if (count_of_dot == 3) {
				*p_minor_code = hub_fwver_part_to_int(str_fwver, pre_part_pos, i);
			}

			pre_part_pos = i + 1;
		}
		else {
			if (str_fwver[i] < '0' || str_fwver[i] > '9') {
				retVal = 1;
				return retVal;
			}
		}
		i++;
	}

	if (i != pre_part_pos)
		*p_buyer_code = hub_fwver_part_to_int(str_fwver, pre_part_pos, i);

	if (count_of_dot != 3) {
		retVal = 1;
		return retVal;
	}

	return retVal;
}

static int hub_fwver_part_to_int(char* str_fwver, int start, int end)
{
	int i;
	int retVal = 0;

	for (i = start; i < end; i++)
		retVal = retVal * 10 + str_fwver[i] - 48;

	return retVal;
}

static void hub_tftpd_up()
{
	int result = 0;
#ifdef DUAL_LAN_NETWORK
	unsigned int sub_ip = get_hub_sub_network();
#else
	unsigned int sub_ip = get_sub_network();
#endif
	char sys_cmd_buffer[256];

	printf("[%s] start\n", __FUNCTION__);
	memset(sys_cmd_buffer, 0x00, 256);
	snprintf(sys_cmd_buffer, 256, "utftpd -l %d.%d.%d.0/24 %s &",
			sub_ip&(0xff), (sub_ip&(0xff00))>>8, (sub_ip&(0xff0000))>>16, HUB_FW_IMAGE_PATH);

	result = proxy_system(sys_cmd_buffer, 1, 3);
	printf("[%s] tftp server start:%d\n", __FUNCTION__, result);
	printf("[%s] end\n", __FUNCTION__);
}

static void hub_tftpd_down()
{
	int result = 0;

	printf("[%s] start\n", __FUNCTION__);
	result = proxy_system("killall utftpd", 1, 3);
	printf("[%s] tftp server end:%d\n", __FUNCTION__, result);
	printf("[%s] end\n", __FUNCTION__);
}

static void hub_disconnect_all_hubs()
{
	int i;
	printf("[%s] start\n", __FUNCTION__);

	for (i = 0; i < MAX_HUB_NUM; i++) {
		hub_manage_table_set_unlink(i);
	}

	hub_tftpd_down();

	printf("[%s] end\n", __FUNCTION__);
}

/*
 * HUB_MANAGE_TABLE control functions
 */
static void hub_manage_table_init()
{
	int i;
	unsigned char sysdb_key_hub_macaddr[256];
	unsigned char sysdb_key_hub_chs[256];
	gchar* hub_macaddr;
	guint hub_chs;
	struct timespec now_time;

	printf("[%s] start\n", __FUNCTION__);

	memset(g_hub_manage_table, 0x00, sizeof(IPX_HUB_MANAGE_TABLE)*MAX_HUB_NUM);
	clock_gettime(CLOCK_REALTIME, &now_time);

	for (i = 0; i < MAX_HUB_NUM; i++) {
		// '<item key="sys.hub.H0.macaddr"		type="STRING"	min="0" max="9"	val="" />'
		// '<item key="sys.hub.H0.chs"			type="UNIT"		min="0" max="9"	val="0" />'
		memset(sysdb_key_hub_macaddr, 0x00, sizeof(sysdb_key_hub_macaddr));
		memset(sysdb_key_hub_chs, 0x00, sizeof(sysdb_key_hub_chs));

		snprintf(sysdb_key_hub_macaddr, 256, "sys.hub.H%d.macaddr",	i);
		snprintf(sysdb_key_hub_chs, 	256, "sys.hub.H%d.chs",		i);

		hub_macaddr = nf_sysdb_get_str_nocopy(sysdb_key_hub_macaddr);
		hub_chs = nf_sysdb_get_uint(sysdb_key_hub_chs);

		printf("[%s] (idx:%d)\n", __FUNCTION__, i);
		printf("[%s] (macaddr:%s)\n", __FUNCTION__, hub_macaddr);
		printf("[%s] (hub_chs:%d)\n", __FUNCTION__, hub_chs);

		convertMacAddrStringIntoByte(hub_macaddr, g_hub_manage_table[i].macaddr);
		g_hub_manage_table[i].matching_chs = hub_chs;
		g_hub_manage_table[i].check_time = now_time.tv_sec;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_manage_table_print()
{
	int i;

	printf("[%s] start\n", __FUNCTION__);
	printf("===============================================================\n");
	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (!memcmp(&g_bef_hub_manage_table[i], &g_hub_manage_table[i], sizeof(IPX_HUB_MANAGE_TABLE)))
			continue;

		printf("[hub-%02d] ", i);

		printf("%02x:%02x:%02x:%02x:%02x:%02x ",
				g_hub_manage_table[i].macaddr[0],
				g_hub_manage_table[i].macaddr[1],
				g_hub_manage_table[i].macaddr[2],
				g_hub_manage_table[i].macaddr[3],
				g_hub_manage_table[i].macaddr[4],
				g_hub_manage_table[i].macaddr[5]);

		printf("%03d ", g_hub_manage_table[i].status);

		switch (g_hub_manage_table[i].matching_chs) {
		case HUB_MATCHING_CHS_NONE:
			printf("%s ","00~00");
			break;
		case HUB_MATCHING_CHS_1_8:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_9_16:
			printf("%s ","09~16");
			break;
		case HUB_MATCHING_CHS_1_4:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_5_8:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_9_12:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_13_16:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_1_8|HUB_MATCHING_CHS_FIXED:
			printf("%s ","F1~08");
			break;
		case HUB_MATCHING_CHS_9_16|HUB_MATCHING_CHS_FIXED:
			printf("%s ","F9~16");
			break;
		}

		printf("%d ", g_hub_manage_table[i].check_time);

		{
			unsigned int x;
			x = htonl(g_hub_manage_table[i].assigned_ip);
			printf("%d.%d.%d.%d\n",
					(int) ((x >> 24) & 0xff),
					(int) ((x >> 16) & 0xff),
					(int) ((x >> 8) & 0xff),
					(int) ((x >> 0) & 0xff));
		}
	}
	printf("***************************************************************\n");
	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (!memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6))
			continue;

		printf("[hub-%02d] ", i);

		printf("%02x:%02x:%02x:%02x:%02x:%02x ",
				g_hub_manage_table[i].macaddr[0],
				g_hub_manage_table[i].macaddr[1],
				g_hub_manage_table[i].macaddr[2],
				g_hub_manage_table[i].macaddr[3],
				g_hub_manage_table[i].macaddr[4],
				g_hub_manage_table[i].macaddr[5]);

		printf("%03d ", g_hub_manage_table[i].status);

		switch (g_hub_manage_table[i].matching_chs) {
		case HUB_MATCHING_CHS_NONE:
			printf("%s ","00~00");
			break;
		case HUB_MATCHING_CHS_1_8:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_9_16:
			printf("%s ","09~16");
			break;
		case HUB_MATCHING_CHS_1_4:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_5_8:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_9_12:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_13_16:
			printf("%s ","01~08");
			break;
		case HUB_MATCHING_CHS_1_8|HUB_MATCHING_CHS_FIXED:
			printf("%s ","F1~08");
			break;
		case HUB_MATCHING_CHS_9_16|HUB_MATCHING_CHS_FIXED:
			printf("%s ","F9~16");
			break;
		}

		printf("%d ", g_hub_manage_table[i].check_time);

		{
			unsigned int x;
			x = htonl(g_hub_manage_table[i].assigned_ip);
			printf("%d.%d.%d.%d\n",
					(int) ((x >> 24) & 0xff),
					(int) ((x >> 16) & 0xff),
					(int) ((x >> 8) & 0xff),
					(int) ((x >> 0) & 0xff));
		}
	}
	printf("===============================================================\n");
	printf("[%s] end\n", __FUNCTION__);
}

static int hub_manage_table_add(HUB_DATA_T* p_recv_msg)
{
	struct timespec now_time;
	unsigned char sysdb_key_hub_macaddr[256];
	unsigned char sysdb_key_hub_chs[256];
	unsigned char buff_mac_str[32];
	unsigned int hub_ip;

	int table_idx;
	int ret = -1;

	printf("[%s] start\n", __FUNCTION__);
	printf("[%s] HUB_MAX_CH:[%d]\n", __FUNCTION__, p_recv_msg->data2);

	table_idx = hub_manage_table_search(p_recv_msg->hubmac);
	clock_gettime(CLOCK_REALTIME, &now_time);

	if (table_idx == -1) {
		table_idx = hub_manage_table_find_empty_idx();

		// HUB 16??????? ??결 ????태. 추?? 불??.
		if (table_idx == -1) {
			ret = -1;
			goto HUB_MANAGE_TABLE_ADD_END;
		}

		// Network ??팅 ??업 미완?????태.
		if (!hub_make_ipaddr(&hub_ip)) {
			ret = -1;
			goto HUB_MANAGE_TABLE_ADD_END;
		}

		snprintf(sysdb_key_hub_macaddr, 256, "sys.hub.H%d.macaddr",	table_idx);
		snprintf(sysdb_key_hub_chs, 	256, "sys.hub.H%d.chs",		table_idx);
		g_hub_manage_table[table_idx].hub_max_ch = p_recv_msg->data2;

		// theweak
		//printf("[%s] Add hub:[%d] mac:[%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__, table_idx, p_macaddr[0], p_macaddr[1], p_macaddr[2], p_macaddr[3], p_macaddr[4], p_macaddr[5]);

		memcpy(g_hub_manage_table[table_idx].macaddr, p_recv_msg->hubmac, 6);
#if 0
		g_hub_manage_table[table_idx].matching_chs
			= hub_manage_table_check_matching_chs(g_hub_manage_table[table_idx].matching_chs);
#else
		g_hub_manage_table[table_idx].matching_chs
			= hub_manage_table_check_matching_chs(HUB_MATCHING_CHS_NONE, table_idx);
#endif
		g_hub_manage_table[table_idx].check_time = now_time.tv_sec;
		g_hub_manage_table[table_idx].assigned_ip = hub_ip;

		memset(buff_mac_str, 0x00, sizeof(buff_mac_str));
		snprintf(buff_mac_str, sizeof(buff_mac_str),"%02x%02x%02x%02x%02x%02x",
				p_recv_msg->hubmac[0],p_recv_msg->hubmac[1],p_recv_msg->hubmac[2],
				p_recv_msg->hubmac[3],p_recv_msg->hubmac[4],p_recv_msg->hubmac[5]);

		printf("[%s] converted mac str : [%s] len : [%d]\n", __FUNCTION__, buff_mac_str, strlen(buff_mac_str));
		nf_sysdb_lock(NF_SYSDB_CATE_SYS);
		nf_sysdb_set_str(sysdb_key_hub_macaddr, buff_mac_str);
		nf_sysdb_set_uint(sysdb_key_hub_chs, g_hub_manage_table[table_idx].matching_chs);
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		nf_sysdb_save("sys");
		nf_sysdb_save_flush();
	}
	else {
		int pre_matching_chs = g_hub_manage_table[table_idx].matching_chs;

		if (g_hub_manage_table[table_idx].assigned_ip == 0) {
			// Network ??팅 ??업 미완?????태.
			if (!hub_make_ipaddr(&hub_ip)) {
				ret = -1;
				goto HUB_MANAGE_TABLE_ADD_END;
			}
			g_hub_manage_table[table_idx].assigned_ip = hub_ip;
		}

		g_hub_manage_table[table_idx].matching_chs
			= hub_manage_table_check_matching_chs(pre_matching_chs, table_idx);

		if (g_hub_manage_table[table_idx].matching_chs != pre_matching_chs) {
			snprintf(sysdb_key_hub_chs, 256, "sys.hub.H%d.chs",	table_idx);

			nf_sysdb_lock(NF_SYSDB_CATE_SYS);
			nf_sysdb_set_uint(sysdb_key_hub_chs, g_hub_manage_table[table_idx].matching_chs);
			nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
			nf_sysdb_save("sys");
			nf_sysdb_save_flush();
		}

		g_hub_manage_table[table_idx].check_time = now_time.tv_sec;
	}
	printf("[%s] hub_ip:[%d]\n", __FUNCTION__, hub_ip);
	ret = table_idx;

HUB_MANAGE_TABLE_ADD_END:
	// -1   : table is full.
	// 0~15 : entry added. ret value is entry index.
	printf("[%s] end. return val:%d\n", __FUNCTION__, ret);
	return ret;
}

static int hub_manage_table_search(unsigned char* p_macaddr)
{
	int i;
	int ret = -1;

	//printf("[%s] start\n", __FUNCTION__);

	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (!memcmp(g_hub_manage_table[i].macaddr, p_macaddr, 6)) {
			ret = i;
			goto HUB_MANAGE_TABLE_SEARCH_END;
		}
	}

HUB_MANAGE_TABLE_SEARCH_END:
	// -1   : not match.
	// 0~15 : matched entry's index.
	//printf("[%s] end. return val:%d\n", __FUNCTION__, ret);
	return ret;
}

static int hub_manage_table_check_matching_chs(int p_matching_chs, int table_idx)
{
	int i;
	int ret = 0, match_chs=0;
	int max_channel = NUM_ACTIVE_CH;
	unsigned char sysdb_key_hub_ch_mask[256] = {0,};
	guint ch_mask=0;
#if 0
	// If this model is 4/8 channel model, return HUB_MATCHING_CHS_1_8|HUB_MATCHING_CHS_FIXED.(MUST)
	if (max_channel <= 8)
		return HUB_MATCHING_CHS_1_8|HUB_MATCHING_CHS_FIXED;
	else
		return HUB_MATCHING_CHS_9_16;

	/** not used **/
	if (p_matching_chs != 0)
		return p_matching_chs;

	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6)) {
			if (g_hub_manage_table[i].status) {
				if (g_hub_manage_table[i].matching_chs & HUB_MATCHING_CHS_9_16)
					return HUB_MATCHING_CHS_1_8;
			}
		}
	}

	return HUB_MATCHING_CHS_9_16;
#else
	if(p_matching_chs == HUB_MATCHING_CHS_NONE)
	{
		if(g_hub_manage_table[table_idx].hub_max_ch == 4)
		{
			if (max_channel <= 8)
			{
				match_chs = HUB_MATCHING_CHS_1_4;
				ch_mask = HUB_MATCHING_CH_MASK_1_4;
			}
			else
			{
				match_chs = HUB_MATCHING_CHS_9_12;
				ch_mask = HUB_MATCHING_CH_MASK_9_12;
			}
		}
		else
		{
			if (max_channel <= 4)
			{
				match_chs = HUB_MATCHING_CHS_1_4;
				ch_mask = HUB_MATCHING_CH_MASK_1_4;
			}
			else if (max_channel <= 8)
			{
				match_chs = HUB_MATCHING_CHS_1_8;
				ch_mask = HUB_MATCHING_CH_MASK_1_8;
			}
			else
			{
				match_chs = HUB_MATCHING_CHS_9_16;
				ch_mask = HUB_MATCHING_CH_MASK_9_16;
			}
		}
		snprintf(sysdb_key_hub_ch_mask, 256, "sys.hub.H%d.chmask",	table_idx);
		nf_sysdb_lock(NF_SYSDB_CATE_SYS);
		nf_sysdb_set_uint(sysdb_key_hub_ch_mask, ch_mask);
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		nf_sysdb_save("sys");
		nf_sysdb_save_flush();

		return match_chs;
	}
	else
	{
		snprintf(sysdb_key_hub_ch_mask, 256, "sys.hub.H%d.chmask",	table_idx);
		ch_mask = nf_sysdb_get_uint(sysdb_key_hub_ch_mask);
		
		if(ch_mask == HUB_MATCHING_CH_MASK_1_8)
		{
			return HUB_MATCHING_CHS_1_8;
		}
		else if(ch_mask == HUB_MATCHING_CH_MASK_9_16)
		{
			return HUB_MATCHING_CHS_9_16;
		}
		else if(ch_mask == HUB_MATCHING_CH_MASK_1_4)
		{
			return HUB_MATCHING_CHS_1_4;
		}
		else if(ch_mask == HUB_MATCHING_CH_MASK_5_8)
		{
			return HUB_MATCHING_CHS_5_8;
		}
		else if(ch_mask == HUB_MATCHING_CH_MASK_9_12)
		{
			return HUB_MATCHING_CHS_9_12;
		}
		else if(ch_mask == HUB_MATCHING_CH_MASK_13_16)
		{
			return HUB_MATCHING_CHS_13_16;
		}
		else
		{
			if(g_hub_manage_table[table_idx].hub_max_ch == 4)
			{
				if (max_channel <= 8)
					return HUB_MATCHING_CHS_1_4;
				else
					return HUB_MATCHING_CHS_9_12;
			}
			else
			{
				if (max_channel <= 4)
					return HUB_MATCHING_CHS_1_4;
				else if (max_channel <= 8)
					return HUB_MATCHING_CHS_1_8;
				else
					return HUB_MATCHING_CHS_9_16;
			}
		}
	}
#endif
}

static int hub_manage_table_find_empty_idx()
{
	int i;
	int ret = -1;
	struct timespec now_time;

	printf("[%s] start\n", __FUNCTION__);

	// Find empty entry idx
	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (!memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6))
		{
			ret = i;
			goto HUB_MANAGE_TABLE_FIND_EMPTY_IDX_END;
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);

	// Find invalid entry idx
	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (g_hub_manage_table[i].status == HUB_PORT_STATE_UNLINKED) {
			ret = i;
			goto HUB_MANAGE_TABLE_FIND_EMPTY_IDX_END;
		}
	}

HUB_MANAGE_TABLE_FIND_EMPTY_IDX_END:
	// -1   : not match.
	// 0~15 : matched entry's index.
	printf("[%s] end. return val:%d\n", __FUNCTION__, ret);
	return ret;
}

static void	hub_manage_table_set_unlink(int p_hub_idx)
{
	int i;
	int link_count = 0;

	g_hub_manage_table[p_hub_idx].status = HUB_PORT_STATE_UNLINKED;
	g_hub_manage_table[p_hub_idx].assigned_ip = 0;
	memset(&g_hub_port_manage_table[p_hub_idx][0], 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);
	vhub_delete_by_hub_macaddr(g_hub_manage_table[p_hub_idx].macaddr);

	for (i = 0; i < MAX_HUB_NUM; i++) {
		g_hub_manage_table[i].conn_below &= ~(1 << p_hub_idx);

		if (g_hub_manage_table[i].status != HUB_PORT_STATE_UNLINKED)
			link_count++;
	}

#if MAKE_NOTIFY_FIRE
	if (link_count == 0)
		nf_notify_fire_params("pnd_hub_status", (guint)p_hub_idx, (guint)IPX_HUB_STATUS_UNLINKED, 0, 0);
#endif
}

static void convertMacAddrStringIntoByte(const char *pszMACAddress, unsigned char* pbyAddress)
{
	int iConunter = 0;

	printf("[%s] p1:%s\n", __FUNCTION__, pszMACAddress);
	printf("[%s] p1:%s\n", __FUNCTION__, pbyAddress);

	for (iConunter = 0; iConunter < 6; ++iConunter) {
		unsigned int iNumber = 0;
		char ch;

		ch = tolower(*pszMACAddress++);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
			return;

		iNumber = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);

		ch = tolower(*pszMACAddress);

		if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
			return;

		iNumber <<= 4;
		iNumber += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);

		pbyAddress[iConunter] = (unsigned char) iNumber;

		if (iConunter < 5)
			++pszMACAddress;
	}
}

static void hub_port_manage_table_print()
{
	int i, j;

	printf("[%s] start\n", __FUNCTION__);
	printf("===============================================================\n");
	for (i = 0; i < MAX_HUB_NUM; i++) {
		for (j = 0; j < AVAILABLE_MAX_CH; j++) {
			if (!memcmp(&g_bef_hub_port_manage_table[i][j], &g_hub_port_manage_table[i][j], sizeof(IPX_HUB_PORT_MANAGE_TABLE)))
				continue;

			printf("[hub-%02d]:[%02d] ", i, j);
			printf("%03d ", g_hub_port_manage_table[i][j].status);
			printf("%02x:%02x:%02x:%02x:%02x:%02x ",
					g_hub_port_manage_table[i][j].macaddr[0],
					g_hub_port_manage_table[i][j].macaddr[1],
					g_hub_port_manage_table[i][j].macaddr[2],
					g_hub_port_manage_table[i][j].macaddr[3],
					g_hub_port_manage_table[i][j].macaddr[4],
					g_hub_port_manage_table[i][j].macaddr[5]);
			printf("%03d ", g_hub_port_manage_table[i][j].recv_cnt);
			printf("%03d %d\n", g_hub_port_manage_table[i][j].req_status, g_hub_port_manage_table[i][j].req_seq);
		}
	}
	printf("***************************************************************\n");
	for (i = 0; i < MAX_HUB_NUM; i++) {
		int line = 0;

		if (!memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6))
			continue;

		for (j = 0; j < AVAILABLE_MAX_CH; j++) {
			if (!memcmp(g_hub_port_manage_table[i][j].macaddr, hub_manage_table_empty_entry, 6)
				&& g_hub_port_manage_table[i][j].status == 0
				&& g_hub_port_manage_table[i][j].recv_cnt == 0)
				continue;

			line++;

			printf("[hub-%02d]:[%02d] ", i, j);
			printf("%03d ", g_hub_port_manage_table[i][j].status);
			printf("%02x:%02x:%02x:%02x:%02x:%02x ",
					g_hub_port_manage_table[i][j].macaddr[0],
					g_hub_port_manage_table[i][j].macaddr[1],
					g_hub_port_manage_table[i][j].macaddr[2],
					g_hub_port_manage_table[i][j].macaddr[3],
					g_hub_port_manage_table[i][j].macaddr[4],
					g_hub_port_manage_table[i][j].macaddr[5]);
			printf("%03d ", g_hub_port_manage_table[i][j].recv_cnt);
			printf("%03d %d\n", g_hub_port_manage_table[i][j].req_status, g_hub_port_manage_table[i][j].req_seq);
		}

		if (line)
			printf("---------------------------------------------------------------\n");
	}
	printf("===============================================================\n");
	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_poll(HUB_DATA_T* p_recv_msg)
{
	HUB_DATA_T buffer;
	unsigned int hub_ip;
	int idx;
	struct timespec now_time;
	char fwver_strs[65] = {0,};

	printf("[%s] start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx != -1) {
		if (g_hub_manage_table[idx].status & HUB_PORT_CONNECTION_COMPLETE) {
			printf("[%s] already connected.\n", __FUNCTION__);
			return;
		}
	}

	idx = hub_manage_table_add(p_recv_msg);
	if (idx == -1) {
		printf("[%s] hub_manage_table_add() failed\n", __FUNCTION__);
		return;
	}
	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;
	g_hub_manage_table[idx].status = HUB_PORT_STATE_LINKED;

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.code = htonl(CODE_POLL_ACK);
	buffer.data1 = g_hub_manage_table[idx].assigned_ip;
#ifdef DUAL_LAN_NETWORK
	buffer.data2 = get_hub_info();
#else
	buffer.data2 = get_host_info();
#endif
	memcpy(buffer.hubmac, p_recv_msg->hubmac, sizeof(buffer.hubmac));
	nf_netif_get_mac_str(buffer.mac);

	memset(fwver_strs, 0x00, sizeof(fwver_strs));
	hub_make_fwver_strs(fwver_strs, sizeof(fwver_strs));
	memcpy(buffer.all_fwver, fwver_strs, sizeof(buffer.all_fwver));

	if (hub_send_msg_broadcast(&buffer) < 0) {
		printf("[%s] hub_send_msg_broadcast() failed\n", __FUNCTION__);
		return;
	}

#if MAKE_NOTIFY_FIRE
	nf_notify_fire_params("pnd_hub_status", (guint)idx, (guint)IPX_HUB_STATUS_CONNECTING, 0, 0);
#endif

	printf("[%s] end\n", __FUNCTION__);
}

#define HUB_FWUP_FAIL_INFO	HUB_FW_IMAGE_PATH"/hub_fwup_fail_info"
#define HUB_FWUP_MAX_CNT	3

static int hub_handler_fwup_retry_check(int idx, unsigned char* p_macaddr)
{
	static int retry_cnt[MAX_HUB_NUM] = {0,};
	static unsigned char hubmac[MAX_HUB_NUM][6] = {{0,},};
	FILE *fp=NULL;
	unsigned char fwup_fail_mac[6] = {0,};
	int fwup_fail_fwver = 0;
	int mode = R_OK | W_OK;
	char fwup_fail_info_file_path[256] = {0,};

	unsigned char hubFWVersion_hub[24] = {0,};
	int hub_product_code, hub_proto_code, hub_minor_code, hub_buyer_code;

	sprintf(fwup_fail_info_file_path, "%s_%d", HUB_FWUP_FAIL_INFO, idx);
	g_message("%s %d idx[%d] mac[%x:%x:%x:%x:%x:%x]", __FUNCTION__, __LINE__, idx, 
			p_macaddr[0], p_macaddr[1], p_macaddr[2], p_macaddr[3], p_macaddr[4], p_macaddr[5]);
	if(access(fwup_fail_info_file_path, mode) == 0)
	{
		if((fp=fopen(fwup_fail_info_file_path, "r")) == NULL)
		{
			g_warning("%s Device Open Error!!! [%s]", __FUNCTION__, fwup_fail_info_file_path);
		}
		if(fp)
		{
			if(fread(fwup_fail_mac, 6, 1, fp) != 1)
			{
				g_warning("%s fread() mac error!!!", __FUNCTION__);
			}
			if(fread(&fwup_fail_fwver, 4, 1, fp) != 1)
			{
				g_warning("%s fread() fwver error!!!", __FUNCTION__);
			}
			fclose(fp);	
		}
		
		if(!memcmp(fwup_fail_mac, p_macaddr, 6))
		{
			memcpy(hubmac[idx], p_macaddr, 6);
			retry_cnt[idx] = 0;
			remove(fwup_fail_info_file_path);
			g_message("[%s:%d] hub fw upgrade fail retry cnt[%d] ", __FUNCTION__, __LINE__, retry_cnt[idx]); 
			return 0;
		}
		else
		{
			snprintf(hubFWVersion_hub, sizeof(hubFWVersion_hub), "%s", g_hub_manage_table[idx].fw_version_hub);
			hub_parse_fwver(hubFWVersion_hub, &hub_product_code, &hub_proto_code, &hub_minor_code, &hub_buyer_code);
			if (fwup_fail_fwver == hub_minor_code) 
			{
				g_message("%s %s", __FUNCTION__, hubFWVersion_hub);
				return 1;
			}
			else
			{
				retry_cnt[idx]++;
				remove(fwup_fail_info_file_path);
				g_message("[%s:%d] hub fw upgrade fail retry cnt[%d] ", __FUNCTION__, __LINE__, retry_cnt[idx]); 
				return 0;
			}
		}
	}
	else
	{
		if(!memcmp(hubmac[idx], p_macaddr, 6))
		{
			memcpy(hubmac[idx], p_macaddr, 6);
			retry_cnt[idx] = 0;
			g_message("[%s:%d] hub fw upgrade fail retry cnt[%d] ", __FUNCTION__, __LINE__, retry_cnt[idx]); 
			return 0;
		}
		else
		{
			if(retry_cnt[idx] < HUB_FWUP_MAX_CNT)
			{
				retry_cnt[idx]++;
				g_message("[%s:%d] hub fw upgrade fail retry cnt[%d] ", __FUNCTION__, __LINE__, retry_cnt[idx]); 
				return 0;
			}
			else
			{
				snprintf(hubFWVersion_hub, sizeof(hubFWVersion_hub), "%s", g_hub_manage_table[idx].fw_version_hub);
				hub_parse_fwver(hubFWVersion_hub, &hub_product_code, &hub_proto_code, &hub_minor_code, &hub_buyer_code);

				if((fp=fopen(fwup_fail_info_file_path, "w")) == NULL)
				{
					g_warning("%s Device Open Error!!! [%s]", __FUNCTION__, fwup_fail_info_file_path);
				}
				if(fp)
				{
					if(fwrite(hubmac[idx], 6, 1, fp) != 1)
					{
						g_warning("%s fwrite() mac error!!! ", __FUNCTION__);
					}
					if(fwrite(&hub_minor_code, 4, 1, fp) != 1)
					{
						g_warning("%s fwrite() fwver error!!! ", __FUNCTION__);
					}
					fclose(fp);		
				}
				retry_cnt[idx] = 0;
				return 1;
			}
		}
	}
	return 0;
}

static void hub_handler_ipset(HUB_DATA_T* p_recv_msg)
{
	unsigned int check_hub_fwver;
	int idx;
	HUB_DATA_T buffer;
	struct timespec now_time;

	printf("[%s] start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx != -1) {
		if (g_hub_manage_table[idx].status & HUB_PORT_CONNECTION_COMPLETE) {
			printf("[%s] already connected.\n", __FUNCTION__);
			return;
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;
	memcpy(g_hub_manage_table[idx].fw_version_hub, p_recv_msg->fwver, sizeof(p_recv_msg->fwver));

	check_hub_fwver = hub_check_fwver(idx);
		
	/** to do **/
	if(check_hub_fwver == 2)
	{
		if(hub_handler_fwup_retry_check(idx, p_recv_msg->hubmac))
			check_hub_fwver = 0;
	}

	// Connect.
	if (check_hub_fwver == 0) {
		g_hub_manage_table[idx].status |= HUB_PORT_CONNECTION_COMPLETE;
#if MAKE_NOTIFY_FIRE
		nf_notify_fire_params("pnd_hub_status", (guint)idx, (guint)IPX_HUB_STATUS_LINKED, 0, 0);
#endif
	}
	// If Model Types are not matched, disconnect hub.
	else if (check_hub_fwver == 1) {
		hub_manage_table_set_unlink(idx);
		return;
	}
	// Start hub fwup.
	else if (check_hub_fwver == 2) {
		g_hub_manage_table[idx].status = HUB_PORT_STATE_FWUP_WAIT;
	}
	else if (check_hub_fwver == 3){
		return;
	}

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.code = htonl(CODE_IP_SET_ACK);
#ifdef DUAL_LAN_NETWORK
	buffer.data1 = get_hub_info();
#else
	buffer.data1 = get_host_info();
#endif
	buffer.data2 = htonl(check_hub_fwver);
	memcpy(buffer.hubmac, p_recv_msg->hubmac, sizeof(buffer.hubmac));
	nf_netif_get_mac_str(buffer.mac);

	if (hub_send_msg_broadcast(&buffer) < 0) {
		printf("[%s] hub_send_msg_broadcast() failed\n", __FUNCTION__);
		return;
	}

	// Set POE On/Off
	{
		int i, s = 0, e = 8;
		char key[64];

		if (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_9_16)
		{
			s += 8;
			e += 8;
		}

		for (i = s; i < e; i++)
		{
			snprintf(key,64,"cam.C%d.poe_on_off",i);
			if (!nf_sysdb_get_bool(key))
			{
				g_hub_port_manage_table[idx][i].req_status = HUB_CH_PORT_STATUS_POE_OFF_REQ;
				g_hub_port_manage_table[idx][i].req_seq = now_time.tv_sec;
			}
		}
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_fwup_res(HUB_DATA_T* p_recv_msg)
{
	char fw_name[30];
	char fw_location[100];
	unsigned char hubFWVersion_dvr[24];
	unsigned char hubFWVersion_hub[24];

	int len = 0;
	int result = 0;
	int fw_size = 0;
	unsigned int send_ip;
	int idx;
	struct timespec now_time;

	FILE *fp = NULL;
	char *fw_data = NULL;
	HUB_EXT_T buffer;

	int hub_product_code, hub_proto_code, hub_minor_code, hub_buyer_code;
	int ret;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (g_hub_manage_table[idx].status == HUB_PORT_STATE_FWUP_START)
		goto HUB_HANDLER_FWUP_RES_END;

	g_hub_manage_table[idx].status = HUB_PORT_STATE_FWUP_START;

	ret = hub_parse_fwver(g_hub_manage_table[idx].fw_version_hub, &hub_product_code, &hub_proto_code, &hub_minor_code, &hub_buyer_code);
	printf("[%s] g_hub_manage_table[idx].fw_version_hub : %s\n", __FUNCTION__, g_hub_manage_table[idx].fw_version_hub);
	printf("[%s] hub_product_code : %d\n", __FUNCTION__, hub_product_code);

	if (hub_product_code == HUB_PRODUCT_VER_1ST)
		snprintf(fw_name, sizeof(fw_name), "%d.bin", hub_product_code);
	else
		snprintf(fw_name, sizeof(fw_name), "%d.nbn", hub_product_code);

	snprintf(fw_location, sizeof(fw_location), "%s/%s", HUB_FW_IMAGE_PATH, fw_name);

	printf("[%s] HUBFWVersion(from hub): %s\n", __FUNCTION__, g_hub_manage_table[idx].fw_version_hub);
	printf("[%s] fw_name: %s\n", __FUNCTION__, fw_name);
	printf("[%s] fw_location: %s\n", __FUNCTION__, fw_location);

	fp = fopen(fw_location, "rb");
	fseek(fp, 0L, SEEK_END);
	fw_size = ftell(fp);
	fclose(fp);
	fp = NULL;
	printf("[%s] fw_size : %d\n", __FUNCTION__, fw_size);

	if (hub_product_code == HUB_PRODUCT_VER_1ST) {
		fw_data = (char*)malloc(0x50000);
		if (!fw_data) {
			printf("[%s] malloc() failed. Out of memory.\n", __FUNCTION__);
			goto HUB_HANDLER_FWUP_RES_END;
		}

		fp = fopen(fw_location, "rb");
		if (fp == NULL) {
			printf("[%s] fw image file open fail\n", __FUNCTION__);
			free(fw_data);
			goto HUB_HANDLER_FWUP_RES_END;
		}
		result = fread(fw_data, 1, fw_size, fp);
		fclose(fp);
		if (result < fw_size) {
			printf("[%s] crc error 1\n", __FUNCTION__);
			free(fw_data);
			goto HUB_HANDLER_FWUP_RES_END;
		}
		result = crc32(0, fw_data, fw_size);
		free(fw_data);
	}

	hub_tftpd_up();

	printf("[%s] FW_START Packet Info\n", __FUNCTION__);
	printf("[%s] buffer.code: %d\n", __FUNCTION__, CODE_TFTP_READY);
	printf("[%s] buffer.data_len: %d\n", __FUNCTION__, fw_size);
	printf("[%s] buffer.crc: %d\n", __FUNCTION__, result);
	printf("[%s] buffer.fname_len: %d\n", __FUNCTION__, strlen(fw_name));
	printf("[%s] buffer.fname: %s\n", __FUNCTION__, fw_name);

	memset(&buffer, 0x00, sizeof(HUB_EXT_T));
	buffer.code = htonl(CODE_TFTP_READY);
	buffer.data_len = htonl(fw_size);
	buffer.crc = htonl(result);
	buffer.fname_len = htonl(strlen(fw_name));
	snprintf(buffer.fname, 32, fw_name);
	memcpy(buffer.hubmac, g_hub_manage_table[idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[idx].assigned_ip;
	if (hub_send_ext_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		goto HUB_HANDLER_FWUP_RES_END;
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

HUB_HANDLER_FWUP_RES_END:
	printf("[%s] End\n", __FUNCTION__);
}

static void hub_handler_fwup_done(HUB_DATA_T* p_recv_msg)
{
	int idx;
	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx == -1) {
		printf("[%s] hub_manage_table_search() failed\n", __FUNCTION__);
		return;
	}
	g_hub_manage_table[idx].status = HUB_PORT_STATE_FWUP_COMP;
	hub_tftpd_down();

	printf("[%s] End\n", __FUNCTION__);
}

static void hub_handler_alive(HUB_DATA_T* p_recv_msg)
{
	int idx;
	int alive_scenario = 0; // 0 : ok or corrected link info, 1 : re-connect
	int link_changed = 0;
	struct timespec now_time;

	idx = hub_manage_table_search(p_recv_msg->hubmac);

	if (idx == -1)
		alive_scenario = 2; // re-connect, added to hub_manage_table
	else if (g_hub_manage_table[idx].status == HUB_PORT_STATE_UNLINKED)
		alive_scenario = 1; // re-connect
	else
		alive_scenario = 0; // check linkinfo

	clock_gettime(CLOCK_REALTIME, &now_time);

	if (alive_scenario == 0) {
		int hub_add_matching_ch_num, hub_add_matching_ch_num_end;
		int hub_link_info;
		int i, isLinked;

	#if 0
		hub_add_matching_ch_num = g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8;
	#else
		hub_add_matching_ch_num = hub_start_ch_num(idx);
		hub_add_matching_ch_num_end = hub_end_ch_num(idx);
	#endif
		hub_link_info = htonl(p_recv_msg->data1) << hub_add_matching_ch_num;

		for (i = 0 + hub_add_matching_ch_num ; i < hub_add_matching_ch_num_end; i++) {
			isLinked = (hub_link_info & (1 << i) ? 1 : 0);
			if (!isLinked && g_hub_port_manage_table[idx][i].status) {
				link_changed++;

				printf("[%s] delete hubidx[%d] port[%d]\n", __FUNCTION__, idx, i);
				memset(&g_hub_port_manage_table[idx][i], 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE));
				g_hub_port_manage_table[idx][i].status = HUB_CH_PORT_STATUS_UNLINKED;
				vhub_delete_by_hub_macaddr_and_ch(g_hub_manage_table[idx].macaddr, i);
			}
			if (isLinked && !g_hub_port_manage_table[idx][i].status) {
				link_changed++;
				g_hub_port_manage_table[idx][i].status = HUB_CH_PORT_STATUS_LINKED;
				g_hub_port_manage_table[idx][i].recv_cnt = 0;
				printf("[%s] insert hubidx[%d] port[%d]\n", __FUNCTION__, idx, i);
				vhub_insert(i);
			}
		}

		g_hub_manage_table[idx].check_time = now_time.tv_sec;
	}
	else if (alive_scenario == 1) { // hub_manage_table????보 그???????용.
		g_hub_manage_table[idx].status = HUB_PORT_STATE_RECONNECT_REQ;
		g_hub_manage_table[idx].check_time = now_time.tv_sec;
		vhub_delete_by_hub_macaddr(g_hub_manage_table[idx].macaddr);
		memset(&g_hub_port_manage_table[idx], 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);
	}
	else if (alive_scenario == 2) { // hub_manage_table??추??.
		idx = hub_manage_table_add(p_recv_msg);
		if (idx == -1)
			return;

		g_hub_manage_table[idx].status = HUB_PORT_STATE_RECONNECT_REQ;
		g_hub_manage_table[idx].check_time = now_time.tv_sec;
		vhub_delete_by_hub_macaddr(g_hub_manage_table[idx].macaddr);
		memset(&g_hub_port_manage_table[idx], 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);
	}

	if (alive_scenario || link_changed) {
		printf("[%s] Start\n", __FUNCTION__);
		printf("[%s] alive_scenario : %d\n", __FUNCTION__, alive_scenario);
		printf("[%s] idx : %d\n", __FUNCTION__, idx);
		printf("[%s] End\n", __FUNCTION__);
	}
}

static void hub_handler_macaddr(HUB_DATA_T* p_recv_msg)
{
	int idx, ch = 0;
	int hub_product_code, hub_proto_code, hub_minor_code, hub_buyer_code;
	struct timespec now_time;
	int ret;

	//printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif

	ret = hub_parse_fwver(g_hub_manage_table[idx].fw_version_hub, &hub_product_code, &hub_proto_code, &hub_minor_code, &hub_buyer_code);

	// HUB Type 1
	if (hub_product_code == HUB_PRODUCT_VER_1ST)
	{
		if (memcmp(p_recv_msg->mac, g_hub_port_manage_table[idx][ch].macaddr, 6)) {
			memcpy(g_hub_port_manage_table[idx][ch].macaddr, p_recv_msg->mac, 6);
			g_hub_port_manage_table[idx][ch].recv_cnt = 0;
		}
		else if (memcmp(p_recv_msg->mac, hub_manage_table_empty_entry, 6)) {
			if (g_hub_port_manage_table[idx][ch].recv_cnt > 6) {
				g_hub_port_manage_table[idx][ch].status |= HUB_CH_PORT_STATUS_MACADDR;
				vhub_set_macaddr(idx, ch);
			}
			else
				g_hub_port_manage_table[idx][ch].recv_cnt++;
		}
	}

	// HUB Type 3 & HUB Type 4
	if (hub_product_code == HUB_PRODUCT_VER_2ND || hub_product_code == HUB_PRODUCT_VER_3RD)
	{
		if (g_hub_port_manage_table[idx][ch].status & HUB_CH_PORT_STATUS_LINKED)
		{
			if (memcmp(p_recv_msg->mac, g_hub_port_manage_table[idx][ch].macaddr, 6))
			{
				memcpy(g_hub_port_manage_table[idx][ch].macaddr, p_recv_msg->mac, 6);
				g_hub_port_manage_table[idx][ch].recv_cnt = 7;
				g_hub_port_manage_table[idx][ch].status |= HUB_CH_PORT_STATUS_MACADDR;
				vhub_set_macaddr(idx, ch);
			}
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	/*
	printf("[%s] (ch:%d) (mac:%02x:%02x:%02x:%02x:%02x:%02x)\n", __FUNCTION__, ch,
			p_recv_msg->mac[0],
			p_recv_msg->mac[1],
			p_recv_msg->mac[2],
			p_recv_msg->mac[3],
			p_recv_msg->mac[4],
			p_recv_msg->mac[5]);
	*/

	//printf("[%s] End\n", __FUNCTION__);
}

static void hub_handler_linked(HUB_DATA_T* p_recv_msg)
{
	int idx, ch;
	unsigned int send_ip;
	HUB_DATA_T buffer;
	struct timespec now_time;

	printf("[%s] start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_recv_msg->portNum);
	buffer.code = htonl(CODE_DEVICE_LINKED_ACK);
	memcpy(buffer.hubmac, g_hub_manage_table[idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif
	g_hub_port_manage_table[idx][ch].status = HUB_CH_PORT_STATUS_LINKED;
	g_hub_port_manage_table[idx][ch].recv_cnt = 0;
	vhub_insert(ch);

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_unlinked(HUB_DATA_T* p_recv_msg)
{
	int idx, ch;
	unsigned int send_ip;
	HUB_DATA_T buffer;
	struct timespec now_time;

	printf("[%s] start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_recv_msg->portNum);
	buffer.code = htonl(CODE_DEVICE_UNLINKED_ACK);
	memcpy(buffer.hubmac, g_hub_manage_table[idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif
	memset(&g_hub_port_manage_table[idx][ch], 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE));
	g_hub_port_manage_table[idx][ch].status = HUB_CH_PORT_STATUS_UNLINKED;
	vhub_delete_by_hub_macaddr_and_ch(g_hub_manage_table[idx].macaddr, ch);

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_unlink_res(HUB_DATA_T* p_recv_msg)
{
	int idx;
	int ch = 0;
	struct timespec now_time;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx == -1)
		return;

#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif
	if (memcmp(g_vhub_port_manage_table[ch].hub_macaddr, p_recv_msg->hubmac, 6))
		return;

	g_vhub_port_manage_table[ch].status &= ~(HUB_CH_PORT_STATUS_UNLINK_REQ);

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_poe_reset_res(HUB_DATA_T* p_recv_msg)
{
	int idx;
	int ch = 0;
	struct timespec now_time;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx == -1)
		return;

#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif
	if (memcmp(g_vhub_port_manage_table[ch].hub_macaddr, p_recv_msg->hubmac, 6))
		return;

	g_vhub_port_manage_table[ch].status &= ~(HUB_CH_PORT_STATUS_POE_REQ);

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_poe_off_res(HUB_DATA_T* p_recv_msg)
{
	int idx;
	int ch = 0;
	struct timespec now_time;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx == -1)
	{
		printf("[%s][%d]\n", __FUNCTION__, __LINE__);
		return;
	}

#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif

	printf("[%s][%d] idx : %d\n", __FUNCTION__, __LINE__, idx);
	printf("[%s][%d] ch  : %d\n", __FUNCTION__, __LINE__, ch);

	g_hub_port_manage_table[idx][ch].req_status = 0;

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_poe_on_res(HUB_DATA_T* p_recv_msg)
{
	int idx;
	int ch = 0;
	struct timespec now_time;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx == -1)
	{
		printf("[%s][%d]\n", __FUNCTION__, __LINE__);
		return;
	}

#if 0
	ch = p_recv_msg->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_recv_msg->portNum + hub_start_ch_num(idx);
#endif

	printf("[%s][%d] idx : %d\n", __FUNCTION__, __LINE__, idx);
	printf("[%s][%d] ch  : %d\n", __FUNCTION__, __LINE__, ch);

	g_hub_port_manage_table[idx][ch].req_status = 0;

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_camfwup_res(HUB_DATA_T* p_recv_msg)
{
	int idx;
	int ch = 0;
	struct timespec now_time;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	if (idx == -1)
		return;

	g_hub_manage_table[idx].status &= ~(HUB_PORT_STATE_CAMFWUP_REQ);

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_port_status(HUB_PORT_STATUS_T* p_port_status)
{
	int idx;
	int ch = 0;
	struct timespec now_time;
	NF_UTIL_POE_INFO info;

	//printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_port_status->hubmac);
#if 0
	ch = p_port_status->portNum + (g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8 ? 0 : 8);
#else
	ch = p_port_status->portNum + hub_start_ch_num(idx);
#endif

	/*
	printf("[Hub-%02d] : [%02x:%02x:%02x:%02x:%02x:%02x]\n", idx,
			p_port_status->hubmac[0],
			p_port_status->hubmac[1],
			p_port_status->hubmac[2],
			p_port_status->hubmac[3],
			p_port_status->hubmac[4],
			p_port_status->hubmac[5]);
	printf("code\t: %d\n",			p_port_status->code);
	printf("portNum\t: %d\n",		p_port_status->portNum);
	printf("is_discovery\t: %d\n",	p_port_status->is_discovery);
	printf("is_active\t: %d\n",		p_port_status->is_active);
	printf("port_class\t: %d\n",	p_port_status->port_class);
	printf("func_status\t: %d\n",	p_port_status->func_status);
	printf("consumption\t: %d\n",	p_port_status->consumption);
	printf("voltage\t: %u\n", 		p_port_status->voltage);
	printf("current_mA\t: %u\n", 	p_port_status->current_mA);
	 */

	info.is_discovery	= p_port_status->is_discovery;
	info.is_active		= p_port_status->is_active;
	info.port_class		= p_port_status->port_class;
	info.func_status	= p_port_status->func_status;
	info.consumption	= p_port_status->consumption;
	info.voltage		= p_port_status->voltage;
	info.current_mA		= p_port_status->current_mA;

/*POE error temporary debug*/
#if defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
#else
	if (NUM_ACTIVE_CH >= 16)
		_event_set_hub_port_info(ch - 8, info);
#endif

	clock_gettime(CLOCK_REALTIME, &now_time);
	g_hub_manage_table[idx].check_time = now_time.tv_sec;

	//printf("[%s] End\n", __FUNCTION__);
}

static void hub_handler_conn_order(HUB_DATA_T* p_recv_msg)
{
	int idx, below_idx;

	//printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_recv_msg->hubmac);
	below_idx = hub_manage_table_search(p_recv_msg->mac);

	if (idx == -1 || below_idx == -1) {
		printf("[%s] not found. idx:[%d] below_idx[%d]\n", __FUNCTION__, idx, below_idx);
		return;
	}

	g_hub_manage_table[idx].conn_below |= 1 << below_idx;

	/*
	printf("[%s] hubmac : %02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,
			p_recv_msg->hubmac[0], p_recv_msg->hubmac[1], p_recv_msg->hubmac[2],
			p_recv_msg->hubmac[3], p_recv_msg->hubmac[4], p_recv_msg->hubmac[5]);
	printf("[%s] mac    : %02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,
			p_recv_msg->mac[0], p_recv_msg->mac[1], p_recv_msg->mac[2],
			p_recv_msg->mac[3], p_recv_msg->mac[4], p_recv_msg->mac[5]);
	*/

	vhub_get_hub_conn_order();

	//printf("[%s] End\n", __FUNCTION__);
}

static void hub_handler_vct_info(HUB_VCT_INFO_T* p_vct_info)
{
	int idx;
	int port = 0;
	struct timespec now_time;
	NF_UTIL_POE_INFO info;

	printf("[%s] Start\n", __FUNCTION__);

	idx = hub_manage_table_search(p_vct_info->hubmac);
	port = p_vct_info->portNum;

	g_hub_vct_info[idx][port].result[0] = p_vct_info->result[0];
	g_hub_vct_info[idx][port].result[1] = p_vct_info->result[1];
	g_hub_vct_info[idx][port].result[2] = p_vct_info->result[2];
	g_hub_vct_info[idx][port].result[3] = p_vct_info->result[3];

	g_hub_vct_info[idx][port].length[0] = p_vct_info->length[0];
	g_hub_vct_info[idx][port].length[1] = p_vct_info->length[1];
	g_hub_vct_info[idx][port].length[2] = p_vct_info->length[2];
	g_hub_vct_info[idx][port].length[3] = p_vct_info->length[3];

	g_hub_vct_info[idx][port].status = HUB_VCT_STATUS_DONE;
	
	printf("[%s] End\n", __FUNCTION__);
}

static void hub_handler_req_fwup(int p_idx)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(1);
	buffer.code = htonl(CODE_FWUP_REQ);
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[p_idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_req_reconnect(int p_idx)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.code = htonl(CODE_HUB_RECONNECT_REQ);
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	if (hub_send_msg_broadcast(&buffer) < 0) {
		printf("[%s] hub_send_msg_broadcast() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_req_unlink(int p_idx, int p_ch, int p_seq)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_ch);
	buffer.code = htonl(CODE_UNLINK_REQ);
	buffer.data1 = p_seq;
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[p_idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_req_poe_reset(int p_idx, int p_ch, int p_seq)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_ch);
	buffer.code = htonl(CODE_REBOOT_POE_REQ);
	buffer.data1 = p_seq;
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[p_idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}
}

static void hub_handler_req_poe_off(int p_idx, int p_ch)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_ch);
	buffer.code = htonl(CODE_HUB_POE_OFF_REQ);
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[p_idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_req_poe_on(int p_idx, int p_ch)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_ch);
	buffer.code = htonl(CODE_HUB_POE_ON_REQ);
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[p_idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_req_camfwup(int p_idx)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.code = htonl(CODE_CAMFWUP_REQ);
	buffer.data1 = g_hub_manage_table[p_idx].check_time;
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	if (hub_send_msg_broadcast(&buffer) < 0) {
		printf("[%s] hub_send_msg_broadcast() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void hub_handler_req_vct(int p_idx, int p_port)
{
	HUB_DATA_T buffer;
	unsigned int send_ip;

	printf("[%s] start\n", __FUNCTION__);

	memset(&buffer, 0x00, sizeof(HUB_DATA_T));
	buffer.portNum = htonl(p_port);
	buffer.code = htonl(CODE_HUB_VCT_REQ);
	memcpy(buffer.hubmac, g_hub_manage_table[p_idx].macaddr, sizeof(buffer.hubmac));

	send_ip = g_hub_manage_table[p_idx].assigned_ip;
	if (hub_send_msg_target(&buffer, send_ip) < 0) {
		printf("[%s] hub_send_msg_target() failed\n", __FUNCTION__);
		return;
	}

	printf("[%s] end\n", __FUNCTION__);
}

static void vhub_init()
{
	memset(g_vhub_port_manage_table, 0x00, sizeof(IPX_VHUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);
}

static void vhub_table_print()
{
	int i;

	printf("[%s] start\n", __FUNCTION__);
	printf("===============================================================\n");
	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		if (!memcmp(&g_bef_vhub_port_manage_table[i], &g_vhub_port_manage_table[i], sizeof(IPX_VHUB_PORT_MANAGE_TABLE)))
			continue;

		printf("[vhub-%02dch] ", i);
		printf("%03d ", g_vhub_port_manage_table[i].status);
		printf("%02x:%02x:%02x:%02x:%02x:%02x ",
				g_vhub_port_manage_table[i].hub_macaddr[0],
				g_vhub_port_manage_table[i].hub_macaddr[1],
				g_vhub_port_manage_table[i].hub_macaddr[2],
				g_vhub_port_manage_table[i].hub_macaddr[3],
				g_vhub_port_manage_table[i].hub_macaddr[4],
				g_vhub_port_manage_table[i].hub_macaddr[5]);

		printf("%02x:%02x:%02x:%02x:%02x:%02x ",
				g_vhub_port_manage_table[i].cam_macaddr[0],
				g_vhub_port_manage_table[i].cam_macaddr[1],
				g_vhub_port_manage_table[i].cam_macaddr[2],
				g_vhub_port_manage_table[i].cam_macaddr[3],
				g_vhub_port_manage_table[i].cam_macaddr[4],
				g_vhub_port_manage_table[i].cam_macaddr[5]);

		printf("%03d\n", g_vhub_port_manage_table[i].req_seq);
	}
	printf("***************************************************************\n");
	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		if (!memcmp(g_vhub_port_manage_table[i].hub_macaddr, hub_manage_table_empty_entry, 6)
			&& !memcmp(g_vhub_port_manage_table[i].cam_macaddr, hub_manage_table_empty_entry, 6)
			&& g_vhub_port_manage_table[i].status == 0
			&& g_vhub_port_manage_table[i].req_seq == 0)
			continue;

		printf("[vhub-%02dch] ", i);
		printf("%03d ", g_vhub_port_manage_table[i].status);
		printf("%02x:%02x:%02x:%02x:%02x:%02x ",
				g_vhub_port_manage_table[i].hub_macaddr[0],
				g_vhub_port_manage_table[i].hub_macaddr[1],
				g_vhub_port_manage_table[i].hub_macaddr[2],
				g_vhub_port_manage_table[i].hub_macaddr[3],
				g_vhub_port_manage_table[i].hub_macaddr[4],
				g_vhub_port_manage_table[i].hub_macaddr[5]);

		printf("%02x:%02x:%02x:%02x:%02x:%02x ",
				g_vhub_port_manage_table[i].cam_macaddr[0],
				g_vhub_port_manage_table[i].cam_macaddr[1],
				g_vhub_port_manage_table[i].cam_macaddr[2],
				g_vhub_port_manage_table[i].cam_macaddr[3],
				g_vhub_port_manage_table[i].cam_macaddr[4],
				g_vhub_port_manage_table[i].cam_macaddr[5]);

		printf("%03d\n", g_vhub_port_manage_table[i].req_seq);
	}
	printf("===============================================================\n");
	printf("[%s] end\n", __FUNCTION__);
}

static int vhub_process_req()
{
	struct timespec now_time;
	int req_cnt = 0;
	int i, j, idx, ch, seq;

	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		if (g_vhub_port_manage_table[i].status & HUB_CH_PORT_STATUS_UNLINK_REQ) {
			idx = hub_manage_table_search(g_vhub_port_manage_table[i].hub_macaddr);
			ch = i % 8;
			seq = g_vhub_port_manage_table[i].req_seq;
			hub_handler_req_unlink(idx, ch, seq);
			req_cnt++;
		}

		if (g_vhub_port_manage_table[i].status & HUB_CH_PORT_STATUS_POE_REQ) {
			idx = hub_manage_table_search(g_vhub_port_manage_table[i].hub_macaddr);
			ch = i % 8;
			seq = g_vhub_port_manage_table[i].req_seq;
			hub_handler_req_poe_reset(idx, ch, seq);
			req_cnt++;
		}
	}

	clock_gettime(CLOCK_REALTIME, &now_time);

	for (i = 0; i < MAX_HUB_NUM; i ++)
	{
		for (j = 0; j < AVAILABLE_MAX_CH; j++)
		{
			if (g_hub_port_manage_table[i][j].req_status & HUB_CH_PORT_STATUS_POE_OFF_REQ)
			{
				if (g_hub_port_manage_table[i][j].req_seq != now_time.tv_sec)
				{
					printf("[%s][%d] idx : %d\n", __FUNCTION__, __LINE__, i);
					printf("[%s][%d] ch  : %d\n", __FUNCTION__, __LINE__, j);

					ch = j % 8;
					hub_handler_req_poe_off(i, ch);
					req_cnt++;
					g_hub_port_manage_table[i][j].req_seq = now_time.tv_sec;
				}
			}

			if (g_hub_port_manage_table[i][j].req_status & HUB_CH_PORT_STATUS_POE_ON_REQ)
			{
				if (g_hub_port_manage_table[i][j].req_seq != now_time.tv_sec)
				{
					printf("[%s][%d] idx : %d\n", __FUNCTION__, __LINE__, i);
					printf("[%s][%d] ch  : %d\n", __FUNCTION__, __LINE__, j);

					ch = j % 8;
					hub_handler_req_poe_on(i, ch);
					req_cnt++;
					g_hub_port_manage_table[i][j].req_seq = now_time.tv_sec;
				}
			}
		}
		
		for(j=0; j<10; j++)
		{
			if(g_hub_vct_info[i][j].status == HUB_VCT_STATUS_REQ)
			{
				hub_handler_req_vct(i, j);
				g_hub_vct_info[i][j].status = HUB_VCT_STATUS_WAIT;
				req_cnt++;
			}
		}
	}

	return req_cnt;
}

static void vhub_insert(int p_ch)
{
	int i;

	if (p_ch < 0 || 15 < p_ch) {
		printf("[%s] error. (p_ch:%d)\n", __FUNCTION__, p_ch);
		return;
	}

	// ??당 ch????결??카메????으???pass
	if (g_vhub_port_manage_table[p_ch].status)
		return;

	for (i= 0; i < MAX_HUB_NUM; i++) {
		// ??록??HUB???처리.
		if (!memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6))
			continue;

		// ??결??HUB???처리.
		if (!(g_hub_manage_table[i].status & HUB_PORT_CONNECTION_COMPLETE))
			continue;

		// ??당 ch????당??는 port????결??카메????으???pass
		if (!g_hub_port_manage_table[i][p_ch].status)
			continue;

		g_vhub_port_manage_table[p_ch].status = HUB_CH_PORT_STATUS_LINKED;
		memcpy(g_vhub_port_manage_table[p_ch].hub_macaddr, g_hub_manage_table[i].macaddr, 6);

		if (g_hub_port_manage_table[i][p_ch].status & HUB_CH_PORT_STATUS_MACADDR) {
			g_vhub_port_manage_table[p_ch].status |= HUB_CH_PORT_STATUS_MACADDR;
			memcpy(g_vhub_port_manage_table[p_ch].cam_macaddr, g_hub_port_manage_table[i][p_ch].macaddr, 6);
		}

		break;
	}
}

static void vhub_set_macaddr(int p_idx, int p_ch)
{
	if (memcmp(g_vhub_port_manage_table[p_ch].hub_macaddr, g_hub_manage_table[p_idx].macaddr, 6))
		return;

	memcpy(g_vhub_port_manage_table[p_ch].cam_macaddr, g_hub_port_manage_table[p_idx][p_ch].macaddr, 6);
	g_vhub_port_manage_table[p_ch].status |= HUB_CH_PORT_STATUS_MACADDR;
}

static int vhub_delete_by_hub_macaddr_and_ch(unsigned char* p_hub_macaddr, int p_ch)
{
	if (!memcmp(p_hub_macaddr, g_vhub_port_manage_table[p_ch].hub_macaddr, 6)) {
		memset(&g_vhub_port_manage_table[p_ch], 0x00, sizeof(IPX_VHUB_PORT_MANAGE_TABLE));
		vhub_insert(p_ch);
		return 1;
	}

	return 0;
}

static void vhub_delete_by_hub_macaddr(unsigned char* p_hub_macaddr)
{
	int i;

	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		if (!memcmp(p_hub_macaddr, g_vhub_port_manage_table[i].hub_macaddr, 6)) {
			memset(&g_vhub_port_manage_table[i], 0x00, sizeof(IPX_VHUB_PORT_MANAGE_TABLE));
			vhub_insert(i);
		}
	}
}

static void hub_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	unsigned char sysdb_key_hub_ch_mask[256] = {0,};
	int sysdb_chg_flag=0, i;
	guint ch_mask=0;

	g_message("%s called", __FUNCTION__);

	g_return_if_fail(pinfo != NULL);

	if(pinfo->d.params[0] == NF_SYSDB_CATE_SYS)
	{
		for(i=0; i<MAX_HUB_NUM_SUPPORTED; i++)
		{
			snprintf(sysdb_key_hub_ch_mask, 256, "sys.hub.H%d.chmask", i);
			ch_mask = nf_sysdb_get_uint(sysdb_key_hub_ch_mask);

			if(ch_mask == HUB_MATCHING_CH_MASK_1_8)
			{
				if(g_hub_manage_table[i].matching_chs != HUB_MATCHING_CHS_1_8)
				{
					sysdb_chg_flag=1;
					break;
				}
			}
			else if(ch_mask == HUB_MATCHING_CH_MASK_9_16)
			{
				if(g_hub_manage_table[i].matching_chs != HUB_MATCHING_CHS_9_16)
				{
					sysdb_chg_flag=1;
					break;
				}
			}
			else if(ch_mask == HUB_MATCHING_CH_MASK_1_4)
			{
				if(g_hub_manage_table[i].matching_chs != HUB_MATCHING_CHS_1_4)
				{
					sysdb_chg_flag=1;
					break;
				}
			}
			else if(ch_mask == HUB_MATCHING_CH_MASK_5_8)
			{
				if(g_hub_manage_table[i].matching_chs != HUB_MATCHING_CHS_5_8)
				{
					sysdb_chg_flag=1;
					break;
				}
			}
			else if(ch_mask == HUB_MATCHING_CH_MASK_9_12)
			{
				if(g_hub_manage_table[i].matching_chs != HUB_MATCHING_CHS_9_12)
				{
					sysdb_chg_flag=1;
					break;
				}
			}
			else if(ch_mask == HUB_MATCHING_CH_MASK_13_16)
			{
				if(g_hub_manage_table[i].matching_chs != HUB_MATCHING_CHS_13_16)
				{
					sysdb_chg_flag=1;
					break;
				}
			}
		}
	}

	if(sysdb_chg_flag)
	{
		g_message("%s iHUB Mapping Changed. All iHUB Reconnect", __FUNCTION__);
		hub_disconnect_all_hubs();
	}
}

static int hub_start_ch_num(int idx)
{
	int max_channel = NUM_ACTIVE_CH;

	if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8)
		return 0; 
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_9_16)
		return 8;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_4)
		return 0;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_5_8)
		return 4;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_9_12)
		return 8;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_13_16)
		return 12;
	else
	{
		if (max_channel <= 4)
			return 0;
		else if (max_channel <= 8)
			return 0;
		else
			return 8;
	}
}

static int hub_end_ch_num(int idx)
{
	int max_channel = NUM_ACTIVE_CH;

	if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_8)
		return 8; 
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_9_16)
		return 16;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_1_4)
		return 4;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_5_8)
		return 8;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_9_12)
		return 12;
	else if(g_hub_manage_table[idx].matching_chs & HUB_MATCHING_CHS_13_16)
		return 16;
	else
	{
		if (max_channel <= 4)
			return 4;
		else if (max_channel <= 8)
			return 8;
		else
			return 16;
	}
}

extern int vhub_get_port_linkinfo()
{
	int i;
	int ret = 0;

	for (i = 0; i < AVAILABLE_MAX_CH; i++)
		ret |= (g_vhub_port_manage_table[i].status == 0 ? 0 : 1) << i;

	return ret;
}

extern int vhub_get_port_macaddr(int p_ch, unsigned char* p_macaddr)
{
	if (p_ch < 0 || NUM_ACTIVE_CH <= p_ch)
		return 0;

	if (!(g_vhub_port_manage_table[p_ch].status & HUB_CH_PORT_STATUS_MACADDR))
		return 0;

	p_macaddr[0] = g_vhub_port_manage_table[p_ch].cam_macaddr[0];
	p_macaddr[1] = g_vhub_port_manage_table[p_ch].cam_macaddr[1];
	p_macaddr[2] = g_vhub_port_manage_table[p_ch].cam_macaddr[2];
	p_macaddr[3] = g_vhub_port_manage_table[p_ch].cam_macaddr[3];
	p_macaddr[4] = g_vhub_port_manage_table[p_ch].cam_macaddr[4];
	p_macaddr[5] = g_vhub_port_manage_table[p_ch].cam_macaddr[5];

	return 1;
}

extern void vhub_get_hub_conn_order()
{
	int i, j, t_idx, t_bitmask;
	int bit_count[MAX_HUB_NUM] = { 0, };

	for (i = 0; i < MAX_HUB_NUM; i++) {
		for (j = 0; j < MAX_HUB_NUM; j++) {
			if (g_hub_manage_table[i].conn_below & (1<<j))
				bit_count[i]++;
		}
	}

	while (1) {
		for (i = 0; i < MAX_HUB_NUM; i++) {
			if (bit_count[i] != -1) {
				t_idx = i;
				break;
			}
		}

		if (i == MAX_HUB_NUM)
			break;

		for (i = 0; i < MAX_HUB_NUM; i++) {
			if (t_idx == i)
				continue;

			if (bit_count[t_idx] < bit_count[i])
				t_idx = i;
		}

		t_bitmask = g_hub_manage_table[t_idx].conn_below | (1 << t_idx);

		for (i = 0; i < MAX_HUB_NUM; i++) {
			if (t_idx == i)
				continue;

			if (g_hub_manage_table[i].conn_below == t_bitmask) {
				break;
			}
		}

		g_hub_manage_table[t_idx].pre_hub_idx = i;
		bit_count[t_idx] = -1;
	}

	/*
	for (i = 0; i < MAX_HUB_NUM; i++) {
		printf("[%s] idx[%d] pre_hub_idx[%d] conn_below[%d] hubmac[%02x:%02x:%02x:%02x:%02x:%02x]\n",
				__FUNCTION__, i, g_hub_manage_table[i].pre_hub_idx, g_hub_manage_table[i].conn_below,
				g_hub_manage_table[i].macaddr[0], g_hub_manage_table[i].macaddr[1], g_hub_manage_table[i].macaddr[2],
				g_hub_manage_table[i].macaddr[3], g_hub_manage_table[i].macaddr[4], g_hub_manage_table[i].macaddr[5]);
	}
	*/
}

extern void vhub_set_port_unlink(int p_ch)
{
	struct timespec now_time;

	printf("[%s] unlink(ch:%d)\n", __FUNCTION__, p_ch);

	if (p_ch < 0 || NUM_ACTIVE_CH <= p_ch)
		return;

	if (!memcmp(g_vhub_port_manage_table[p_ch].hub_macaddr, hub_manage_table_empty_entry, 6))
		return;

	clock_gettime(CLOCK_REALTIME, &now_time);

	g_vhub_port_manage_table[p_ch].status |= HUB_CH_PORT_STATUS_UNLINK_REQ;
	g_vhub_port_manage_table[p_ch].req_seq = now_time.tv_sec;
}

extern void vhub_set_port_poe_reset(int p_ch)
{
	struct timespec now_time;

	printf("[%s] poe reset(ch:%d)\n", __FUNCTION__, p_ch);

	if (p_ch < 0 || NUM_ACTIVE_CH <= p_ch)
		return;

	if (!memcmp(g_vhub_port_manage_table[p_ch].hub_macaddr, hub_manage_table_empty_entry, 6))
		return;

	clock_gettime(CLOCK_REALTIME, &now_time);

	g_vhub_port_manage_table[p_ch].status |= HUB_CH_PORT_STATUS_POE_REQ;
	g_vhub_port_manage_table[p_ch].req_seq = now_time.tv_sec;
}

extern void vhub_set_port_poe_off(int p_ch)
{
	struct timespec now_time;
	int is_hub_ch_range_1_8;
	int i, j;

	//printf("[%s] poe off(ch:%d)\n", __FUNCTION__, p_ch);

	if (p_ch < 0 || NUM_ACTIVE_CH <= p_ch)
		return;

	clock_gettime(CLOCK_REALTIME, &now_time);

	for (i = 0; i < MAX_HUB_NUM; i++)
	{
		if (!(g_hub_manage_table[i].status &HUB_PORT_CONNECTION_COMPLETE))
			continue;

	#if 0
		is_hub_ch_range_1_8 = g_hub_manage_table[i].matching_chs & HUB_MATCHING_CHS_1_8 ? 1 : 0;

		if (!is_hub_ch_range_1_8 && p_ch < 8)
			continue;
		if (is_hub_ch_range_1_8 && p_ch >= 8)
			continue;
	#else
		if (p_ch < hub_start_ch_num(i) || p_ch >= hub_end_ch_num(i))
			continue;
	#endif

		g_hub_port_manage_table[i][p_ch].req_status = HUB_CH_PORT_STATUS_POE_OFF_REQ;
		g_hub_port_manage_table[i][p_ch].req_seq = now_time.tv_sec;
	}
}

extern void vhub_set_port_poe_on(int p_ch)
{
	struct timespec now_time;
	int is_hub_ch_range_1_8;
	int i, j;

	//printf("[%s] poe on(ch:%d)\n", __FUNCTION__, p_ch);

	if (p_ch < 0 || NUM_ACTIVE_CH <= p_ch)
		return;

	clock_gettime(CLOCK_REALTIME, &now_time);

	for (i = 0; i < MAX_HUB_NUM; i++)
	{
		if (!(g_hub_manage_table[i].status &HUB_PORT_CONNECTION_COMPLETE))
			continue;

	#if 0
		is_hub_ch_range_1_8 = g_hub_manage_table[i].matching_chs & HUB_MATCHING_CHS_1_8 ? 1 : 0;

		if (!is_hub_ch_range_1_8 && p_ch < 8)
			continue;
		if (is_hub_ch_range_1_8 && p_ch >= 8)
			continue;
	#else
		if (p_ch < hub_start_ch_num(i) || p_ch >= hub_end_ch_num(i))
			continue;
	#endif

		g_hub_port_manage_table[i][p_ch].req_status = HUB_CH_PORT_STATUS_POE_ON_REQ;
		g_hub_port_manage_table[i][p_ch].req_seq = now_time.tv_sec;
	}
}

extern void vhub_set_camfwup()
{
	int i;
	struct timespec now_time;

	printf("[%s] Start\n", __FUNCTION__);
	clock_gettime(CLOCK_REALTIME, &now_time);

	for (i = 0; i < MAX_HUB_NUM; i++) {
		if (!memcmp(g_hub_manage_table[i].macaddr, hub_manage_table_empty_entry, 6))
			continue;

		if (!g_hub_manage_table[i].status)
			continue;

		printf("[%s] camfwup on:[%d]\n", __FUNCTION__, i);
		g_hub_manage_table[i].status = HUB_PORT_STATE_CAMFWUP_REQ;
		g_hub_manage_table[i].check_time = now_time.tv_sec;
	}
	printf("[%s] End\n", __FUNCTION__);
}

extern void vhub_set_data_clear()
{
	memset(g_hub_manage_table, 0x00, sizeof(IPX_HUB_MANAGE_TABLE)*MAX_HUB_NUM);
	memset(g_hub_port_manage_table, 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*MAX_HUB_NUM*AVAILABLE_MAX_CH);
	memset(g_vhub_port_manage_table, 0x00, sizeof(IPX_VHUB_PORT_MANAGE_TABLE)*AVAILABLE_MAX_CH);

	hub_tftpd_down();
}

extern void vhub_set_data_rebuild()
{
	hub_manage_table_init();
	memset(g_hub_port_manage_table, 0x00, sizeof(IPX_HUB_PORT_MANAGE_TABLE)*MAX_HUB_NUM*AVAILABLE_MAX_CH);
	vhub_init();
}

extern int vhub_get_hub_macaddr(unsigned char *p_hub_macaddr)
{
	int i;

	for (i = 0; i < MAX_HUB_NUM; i++)
	{
		if (g_hub_manage_table[i].status & HUB_PORT_CONNECTION_COMPLETE)
		{
			memcpy(p_hub_macaddr, g_hub_manage_table[i].macaddr, sizeof(g_hub_manage_table[i].macaddr));
			return 1;
		}
	}

	return 0;
}

extern int vhub_get_hub_fwver(unsigned char *p_hub_fwver)
{
	int i;

	for (i = 0; i < MAX_HUB_NUM; i++)
	{
		if (g_hub_manage_table[i].status & HUB_PORT_CONNECTION_COMPLETE)
		{
			memcpy(p_hub_fwver, g_hub_manage_table[i].fw_version_hub, sizeof(g_hub_manage_table[i].fw_version_hub));
			return 1;
		}
	}

	return 0;
}

// theweak : dummy
extern int ipx_hub_current_link_status(int ch)
{
	int linkinfo;
	linkinfo = g_vhub_port_manage_table[ch].status == 0 ? 0 : 1;
	return linkinfo;
}

extern int hub_find_port(unsigned char* mac)
{
	int i, j,ret;
	unsigned char vhub_port_mac[6];

	for (i = 0; i < AVAILABLE_MAX_CH; i++) {
		ret = vhub_get_port_macaddr(i, vhub_port_mac);

		if (ret) {
			if (!memcmp(mac, vhub_port_mac, sizeof(vhub_port_mac))) {
				return i;
			}
		}
	}

	for (i = 0; i < MAX_HUB_NUM; i++)
	{
		for (j = 0; j < AVAILABLE_MAX_CH; j++)
		{
			if (!memcmp(mac, g_hub_port_manage_table[i][j].macaddr, sizeof(g_hub_port_manage_table[i][j].macaddr)))
			{
				g_hub_port_manage_table[i][j].recv_cnt = 7;
			}
		}
	}
	return -1;
}

extern void hub_fw_upgrade(int type)
{
	return;
}

extern void hub_poe_reboot(int ch)
{
	vhub_set_port_poe_reset(ch);
}
extern void hub_unlink_request(int ch)
{
	vhub_set_port_unlink(ch);
}
extern void hub_camfwup_request()
{
	printf("[%s] Start\n", __FUNCTION__);
	vhub_set_camfwup();
	printf("[%s] End\n", __FUNCTION__);
	return;
}

extern void hub_set_port_vct(int dev_num, int port)
{
	g_hub_vct_info[dev_num][port].status = HUB_VCT_STATUS_REQ; 
}

extern void hub_get_port_vct(guint dev_num, NF_UTIL_SWITCH_VCT_INFO* vct_res)
{
	vct_res->result[0] = g_hub_vct_info[dev_num][vct_res->port].result[0]; 
	vct_res->result[1] = g_hub_vct_info[dev_num][vct_res->port].result[1]; 
	vct_res->result[2] = g_hub_vct_info[dev_num][vct_res->port].result[2]; 
	vct_res->result[3] = g_hub_vct_info[dev_num][vct_res->port].result[3]; 
	
	vct_res->length[0] = g_hub_vct_info[dev_num][vct_res->port].length[0]; 
	vct_res->length[1] = g_hub_vct_info[dev_num][vct_res->port].length[1]; 
	vct_res->length[2] = g_hub_vct_info[dev_num][vct_res->port].length[2]; 
	vct_res->length[3] = g_hub_vct_info[dev_num][vct_res->port].length[3];
	
	g_hub_vct_info[dev_num][vct_res->port].status = HUB_VCT_STATUS_OFF; 
}

extern int hub_get_port_vct_status(int dev_num, int port)
{
	return g_hub_vct_info[dev_num][port].status;
}

extern int hub_get_status(int dev_num)
{
	return g_hub_manage_table[dev_num].status;
}

extern int hub_fw_ver_chg(int idx, char *fw_ver)
{
	int ret = 0;
	unsigned char hubFWVersion_dvr[24] = {0,};
	int dvr_product_code;
	int dvr_proto_code;
	int dvr_minor_code;
	int dvr_buyer_code;

	snprintf(hubFWVersion_dvr, sizeof(hubFWVersion_dvr), "%s", fw_ver);
	ret = hub_parse_fwver(hubFWVersion_dvr, &dvr_product_code, &dvr_proto_code, &dvr_minor_code, &dvr_buyer_code);

	if (ret) {
		printf("[%s] Failed:hub_parse_fwver()\n", __FUNCTION__);
		return -1;
	}

	memcpy(g_hub_fw_version_table[idx].hubFWVersion_dvr, hubFWVersion_dvr, sizeof(hubFWVersion_dvr));
	g_hub_fw_version_table[idx].dvr_product_code = dvr_product_code;
	g_hub_fw_version_table[idx].dvr_proto_code = dvr_proto_code;
	g_hub_fw_version_table[idx].dvr_minor_code = dvr_minor_code;
	g_hub_fw_version_table[idx].dvr_buyer_code = dvr_buyer_code;

	return 0;
}

extern int hub_fw_have_chg(char *have_str)
{
	int idx = 0;
	for(idx = 0; idx < MAX_HUB_NUM_SUPPORTED; idx++)
	{
		if(*(have_str+idx) == '0')
		{
			g_hub_fw_version_table[idx].fw_have = HUB_FALSE;
		}
		else
		{
			g_hub_fw_version_table[idx].fw_have = HUB_TRUE;
		}
	}

	return 0;
}


