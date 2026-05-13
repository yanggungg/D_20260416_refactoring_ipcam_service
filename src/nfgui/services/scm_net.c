/*
 * scm_net.c
 * 	- scm network service
 *	- dependency :
 *
 * Written by Sekeun Shin. <etazeus@itxsecurity.com>
 * Copyright (c) ITX security, Jan 6, 2011
 *
 */

#include "scm_internal.h"
#include "nf_object.h"
#include "nf_util_mail.h"
#include "nf_util_netif.h"
#include "nf_util_ftp.h"
#include "nf_network.h"
#include "nf_issm_ctl.h"
#include "../../service/ddns2_manager.h"
#include "../modules/wrk.h"
#include "scm.h"
#include "iux_afx.h"
#include "ix_mem.h"
#include "nfdal.h"
#include "ix_func.h"
#include "vfs.h"
#include "nf_api_ipcam.h"
#include "nf_netsvr.h"
#include "nf_util_sms.h"
#include <openssl/pem.h>

#define DBG_LEVEL		9
#define DBG_MODULE		"SCM_NET"

#define IS_SUCCESS()			(result == 0)
#define IS_UPNP_SUCCESS()		(result == 101)

////////////////////////////////////////////////////////////
//
// public data type 
//

typedef struct _EMAIL_SENDING_WORK_T {
	char 	*fmt_email_info;
	char 	delimiter;
	IMSG	ret_msg;
} EMAIL_SENDIG_WORK_T;


////////////////////////////////////////////////////////////
//
// private functions
//

static void _cb_email_test(gpointer user_data, NF_MAIL_SEND_STATUS status, const char *svr_msg)
{
	return;
}

static int _test_email(char *fmt_email_info, char delimiter)
{
	char   smtp_port[8];
	char   smtp_security[8];
	NF_MAIL_SEND_SERVER server;
	NF_MAIL_SEND_CONTENT cont;
	gboolean ret;


/*
 * these codes maybe contain some bug related to memory size 
 * 
 *
 */

	ifn_find_token(fmt_email_info, delimiter, 0, server.smtp_server);
	ifn_find_token(fmt_email_info, delimiter, 1, smtp_port);
	server.smtp_port = atoi(smtp_port);
	ifn_find_token(fmt_email_info, delimiter, 2, smtp_security);
	server.smtp_security = atoi(smtp_security);
	ifn_find_token(fmt_email_info, delimiter, 3, server.smtp_user);
	ifn_find_token(fmt_email_info, delimiter, 4, server.smtp_passwd);
	ifn_find_token(fmt_email_info, delimiter, 5, cont.from);
	ifn_find_token(fmt_email_info, delimiter, 6, cont.to[0]);
	ifn_find_token(fmt_email_info, delimiter, 7, cont.subject);
	ifn_find_token(fmt_email_info, delimiter, 8, cont.contents);

	cont.cb_func = _cb_email_test;
	cont.cb_arg = NULL;
	ret = nf_mail_send_test(&server, &cont, NULL);
	return (ret ? 0 : -1);
}

static int _proc_email_sending(int param,  void *data)
{
	_test_email((char *)data, param); 
	return 0;
}

static int _get_sys_netinfo(NF_NETIF_GET_INFO *info)
{
	memset(info, 0x00, sizeof(NF_NETIF_GET_INFO));
	nf_netif_get_info(info);
	DMSG(1, "IP       [%X]", info->ipaddr);
	DMSG(1, "GATEWAY  [%X]", info->gateway);
	DMSG(1, "SUBNET   [%X]", info->netmask);
	DMSG(1, "DNS1     [%X]", info->dnsserver1);
	DMSG(1, "DNS2     [%X]", info->dnsserver2);
	//IPv6
	DMSG(1, "====================IPv6====================");
	DMSG(1, "LINK-LOCAL       [%s]", info->ipv6_linklocal);
	DMSG(1, "ADDRESS1  [%s/%d]", info->ipv6_addr[0], info->ipv6_prefix[0]);
	DMSG(1, "ADDRESS2  [%s/%d]", info->ipv6_addr[1], info->ipv6_prefix[1]);
	DMSG(1, "ADDRESS3  [%s/%d]", info->ipv6_addr[2], info->ipv6_prefix[2]);
	DMSG(1, "ADDRESS4  [%s/%d]", info->ipv6_addr[3], info->ipv6_prefix[3]);
	DMSG(1, "GATEWAY  [%s]", info->ipv6_gateway);
	DMSG(1, "DNS1     [%s]", info->ipv6_dns[0]);
	DMSG(1, "DNS2     [%s]", info->ipv6_dns[1]);
	return 0;
}

static int _get_ipaddr_str(char *buf, int buf_len)
{
	NF_NETIF_GET_INFO info;
	
	memset(buf, 0x00, buf_len);
	memset(&info, 0x00, sizeof(NF_NETIF_GET_INFO));
	_get_sys_netinfo(&info);
	if (info.ipaddr == 0) return  -1;
	else {
		sprintf(buf, "%d.%d.%d.%d", 
				(info.ipaddr >> 24) & 255,
				(info.ipaddr >> 16) & 255,
				(info.ipaddr >> 8) & 255,
				info.ipaddr & 255);

		DMSG(1, "%s", buf);
	}
	return 0;
}

static int _get_mac(char *buf, int buf_len)
{
	NF_NETIF_GET_INFO ret_net_info;
	if (buf_len < 32) return -1;

	memset(buf, 0x00, 32);
	nf_netif_get_info(&ret_net_info);   
	sprintf(buf,"%02x%02x%02x%02x%02x%02x", 
			(guchar)ret_net_info.mac_addr[0],
			(guchar)ret_net_info.mac_addr[1], 
			(guchar)ret_net_info.mac_addr[2], 
			(guchar)ret_net_info.mac_addr[3], 
			(guchar)ret_net_info.mac_addr[4], 
			(guchar)ret_net_info.mac_addr[5]);

	return 0;
}

static int _make_outbound_alias(int port, char *alias, int alias_len)
{
	DDNSData dd;
	char buf[128];
	char host[128];

	memset(host, 0x00, sizeof(host));
	memset(alias, 0x00, alias_len);
	memset(&dd, 0x00, sizeof(dd));
	DAL_get_ddns_data(&dd);

	DMSG(1, "");
	if (strlen(dd.server) > 0 && dd.enable) {
		if (strlen(dd.host_name) == 0) _get_mac(host, 128);
		else strcpy(host, dd.host_name);
		sprintf(alias, "http://%s.%s:%d", host, dd.server, port);
	}
	else {
		if (_get_ipaddr_str(buf, 128) < 0) sprintf(alias, "http://localhost:%d", port);
		else sprintf(alias, "http://%s:%d", buf, port);
		DMSG(1, "ALIAS=[%s]", alias);
	}

	return 0;	
}

static int _make_inbound_alias(int port, char *alias, int alias_len)
{
	char buf[128];

	memset(alias, 0x00, alias_len);

	DMSG(1, "");
	if (_get_ipaddr_str(buf, 128) < 0) sprintf(alias, "http://localhost:%d", port);
	else sprintf(alias, "http://%s:%d", buf, port);
	DMSG(1, "ALIAS=[%s]", alias);

	return 0;	
}

static int _make_dvr_addr_with_host(char *host, char *addr, int addr_len)
{
	DDNSData dd;
	char buf[128];
	char mac[32];

	memset(addr, 0x00, addr_len);
	memset(&dd, 0x00, sizeof(dd));
	DAL_get_ddns_data(&dd);

	DMSG(1, "");
	if (strlen(dd.server) > 0) {
		sprintf(addr, "%s.%s", host, dd.server);
	}
	else {
		if (_get_ipaddr_str(buf, 128) < 0) sprintf(addr, "localhost");
		else sprintf(addr, "%s", buf);
		DMSG(1, "ADDR=[%s]", addr);
	}

	return 0;	

}

static int _make_dvr_addr(char *addr, int addr_len)
{
	DDNSData dd;
	char buf[128];
	char mac[32];
	char ex_addr[40];

	memset(addr, 0x00, addr_len);
	memset(&dd, 0x00, sizeof(dd));
	memset(&ex_addr, 0x00, sizeof(ex_addr));
	
	DAL_get_ddns_data(&dd);

    if (var_get_supported_ddns() == 1)
    {
    	if (strlen(dd.server) > 0) 
    	{
    		if (strlen(dd.host_name) > 0) 
    		{
                if (!strcmp(dd.server, "s1.co.kr"))
                    {
                        strcpy(addr, dd.serverip);
                    }
                else
        		    sprintf(addr, "%s.%s", dd.host_name, dd.server);
            }
    		else 
    		{
    			_get_mac(mac, 32);
    			sprintf(addr, "%s.%s", mac, dd.server);
    		}
    	}
    	else {
    		if (_get_ipaddr_str(buf, 128) < 0) sprintf(addr, "localhost");
    		else sprintf(addr, "%s", buf);
    	}
    }
    else
    {
        var_get_external_addr(ex_addr, 40);
	    sprintf(addr, "%s", ex_addr);
    }

	DMSG(1, "ADDR=[%s]", addr);

	return 0;	

}

static int _make_dvr_name(char *name, int name_len)
{
	DDNSData dd;
	char mac[32];

	memset(name, 0x00, name_len);
	memset(&dd, 0x00, sizeof(dd));
	DAL_get_ddns_data(&dd);

	if (strlen(dd.host_name) > 0) {
		strcpy(name, dd.host_name);
	}
	else {
		_get_mac(mac, 32);
		strcpy(name, mac);
	}
	return 0;
}

static int _proc_get_wan_ip(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ret;
	char *buf = imalloc(sizeof(char) * 40);
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	memset(buf, 0x00, 40);
//	ret = nf_upnp_get_external_ip(buf);	
	DMSG(9, "");
	ret = nf_get_external_ip(buf);	
	DMSG(9, "");

	if (ret != 0) memset(buf, 0x00, 40);

	ack_ret = (ret == 0) ? 0 : -1;
	piscm->chart[tra].result = (void *)buf;

	return ack_ret;
}

static int _register_rtsp_port(int rtsp_port)
{
	int ret;
	char buf[256];
	memset(buf, 0x00, sizeof(buf));
	// buf won't be not used, because it is not accurate
	return nf_upnp_port_forwording (rtsp_port, 0, buf);

	// return value
	//		RES_UPNP_SUCCESS = 101,	// success
	//		RES_UPNP_IGD_NOT_PORT_FWD = 102,
	//		RES_UPNP_DEV_NOT_IGD = 103,
	//		RES_UPNP_EQUAL_PORT = 104,
	//		RES_UPNP_PORT_DEL_FAIL = 105,
	//		RES_UPNP_NOT_IGD = 106,
	//		RES_UPNP_NOT_DEV = 107,
	//		RES_UPNP_PORT_DEL_ERROR = 108,
}

static int _remove_rtsp_port(int rtsp_port)
{
	return nf_upnp_port_delete(rtsp_port, 0);
}

static int _test_rtsp_port()
{
	return nf_network_port_test_netsvr(NF_NETWORK_PORT_TEST_ADDRESS_LOCAL, NULL);
}

static int _register_web_port(int web_port)
{
	char buf[128];
	memset(buf, 0x00, sizeof(buf));
	// buf won't be not used, because it is not accurate
	return nf_upnp_port_forwording (web_port, 1, buf);

	// return value
	//		RES_UPNP_SUCCESS = 101,	// success
	//		RES_UPNP_IGD_NOT_PORT_FWD = 102,
	//		RES_UPNP_DEV_NOT_IGD = 103,
	//		RES_UPNP_EQUAL_PORT = 104,
	//		RES_UPNP_PORT_DEL_FAIL = 105,
	//		RES_UPNP_NOT_IGD = 106,
	//		RES_UPNP_NOT_DEV = 107,
	//		RES_UPNP_PORT_DEL_ERROR = 108,
}

static int _remove_web_port(int web_port)
{
	return nf_upnp_port_delete(web_port, 1);
}

static int _test_web_port()
{
	return nf_network_port_test_websvr( NF_NETWORK_PORT_TEST_ADDRESS_LOCAL,  NULL);
}


// SKSHIN, special code because it is not declared
//int net_ddns_cur_status(void);
static int _test_ddns(char *server, char *host_name)
{
    char ip_addr[32];
    char strHostName[256];

    memset(ip_addr, 0, sizeof(ip_addr));
    memset(strHostName, 0, sizeof(strHostName));
    sprintf(strHostName,"%s.%s", host_name, server);    
	return get_dns_ip_addr(ip_addr, 0, strHostName, strlen(strHostName));
}

static int _register_ddns2(SCM_T *piscm, char *host_name)
{
	int ddns2_ret;
	DDNS2_PARRAM send_par;

	memset(&send_par, 0x00, sizeof(send_par));
	strcpy(send_par.owner_name, "c");
	_scm_get_cur_lang(send_par.lang, 32);
	strcpy(send_par.host_name, host_name);
	send_par.video_type = 0;		// always 0
	send_par.disk_count = var_get_detected_disk_count();
	_scm_get_disk_space(piscm, &send_par.disk_size, &send_par.disk_filled);

	ddns2_ret = (int)nf_ddns2_register(&send_par);
	return ddns2_ret;
}

static int _register_fujiko(SCM_T *piscm, char *host_name, char *id, char *pwd)
{
	int ddns2_ret;

	ddns2_ret = (int)fujiko_ddns_force_register(host_name, id, pwd);
	return ddns2_ret;
}

static int _register_s1(SCM_T *piscm)
{
	int s1_ret;

	s1_ret = (int)s1_ddns_force_register();
	return s1_ret;
}

static int _proc_test_ftp(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	NF_FTP_CLIENT_REQ req;
	FTP_INFO_T *info;
	FTP_CODE_E ret_val = 0;
	GError *err = NULL;
	
	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	info = (FTP_INFO_T *)piscm->chart[tra].void_data;

	memset(&req, 0x00, sizeof(NF_FTP_CLIENT_REQ));
	memcpy(req.server, info->server, 256);
	req.port = info->port;
	memcpy(req.user, info->user, 64);
	memcpy(req.passwd, info->passwd, 64);

	DMSG(1, "server : %s", req.server);
	DMSG(1, "port : %d", req.port);
	DMSG(1, "user : %s", req.user);
	DMSG(1, "password : %s", req.passwd);	

	if (req.user[0] == '\0') req.is_anon = 1;

	nf_arch_set_ftp_info(&req, 0);
	if (!nf_arch_ftp_test(0, 0, &err)) {
		if (err) {
			ret_val = err->code;	
			g_error_free(err);
		}
	}

	DMSG(1, "%d", ret_val);
	return ret_val;
}

static int _scm_cleanup_wan_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_wan);
	piscm->wrk_wan = 0;
	return 0;
}

static int _scm_cleanup_rtsp_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_rtsp);
	piscm->wrk_rtsp = 0;
	return 0;
}

static int _scm_cleanup_web_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_web);
	piscm->wrk_web = 0;
	return 0;
}

static int _scm_cleanup_unimo_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_unimo);
	piscm->wrk_unimo = 0;
	return 0;
}

static int _scm_cleanup_ddns_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_ddns);
	piscm->wrk_ddns = 0;
	return 0;
}

static int _scm_cleanup_ddnstest_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_ddnstest);
	piscm->wrk_ddnstest = 0;
	return 0;
}

static int _scm_cleanup_ddnsStatus_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_ddnsStatus);
	piscm->wrk_ddnsStatus = 0;
	return 0;
}

static int _scm_cleanup_ftp_wrk(SCM_T *piscm)
{
	DMSG(1, "");
	wrk_destroy_worker(piscm->wrk_ftp);
	piscm->wrk_ftp = 0;
	return 0;
}

static RS_SRVSTOP_E _get_discon_reason(TRANSACTION_E tra)
{
	RS_SRVSTOP_E reason = 0;

	switch (tra) {
	case TRA_FW_UPGRADE: 		reason = RS_FWUP; 		break;
	case TRA_FACTORY_DEFAULT: 	reason = RS_FACDEF; 	break;
	case TRA_TIME_CHANGE:		reason = RS_TIMECHANGE; break;
	case TRA_DB_IMPORT: 		reason = RS_DBIMPORT; 	break;
	case TRA_IP_CHANGE:			reason = RS_IPCHANGE; 	break;
	case TRA_FORMAT:			reason = RS_FORMAT; 	break;
	case TRA_SHUTDOWN:			reason = RS_SHUTDOWN;	break;
	}

	return reason;
}

static int _proc_register_rtsp(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;
	int port;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	port = piscm->chart[tra].int_data;

	DMSG(1, "");
	ack_ret = _register_rtsp_port(port);	
	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_remove_rtsp(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;
	int port;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	port = piscm->chart[tra].int_data;

	ack_ret = _remove_rtsp_port(port);	
	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_test_rtsp(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	DMSG(1, "");
	ack_ret = _test_rtsp_port();	
	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_register_web(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;
	char *buf = imalloc(sizeof(char) * 255);
	int port;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	port = piscm->chart[tra].int_data;

	DMSG(1, "");
	ack_ret = _register_web_port(port);	
	DMSG(1, "");
	if (ack_ret == 101) _make_outbound_alias(port, buf, 255);	// success
	else memset(buf, 0x00, 255);
	piscm->chart[tra].result = (void *)buf;

	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_remove_web(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;
	char *buf = imalloc(sizeof(char) * 255);
	int port;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	port = piscm->chart[tra].int_data;

	DMSG(1, "");
	ack_ret = _remove_web_port(port);	
	DMSG(1, "");
	if (ack_ret == 101) _make_inbound_alias(port, buf, 255);	// success
	else memset(buf, 0x00, 255);
	piscm->chart[tra].result = (void *)buf;

	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_test_web(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	DMSG(1, "");
	ack_ret = _test_web_port();	
	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_register_unimo(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;
    UNIMOData *pInfo;
    NF_DDNS_COMMON_REG_PARAM regParam;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	pInfo = (UNIMOData*)piscm->chart[tra].alloc_data;

	DMSG(1, "");

	nf_ddns_register_unimo_checker(pInfo->server, pInfo->port);

	return 0;
}

static int _proc_register_ddns(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;
    DDNS_INFO_T *pInfo;
    NF_DDNS_COMMON_REG_PARAM regParam;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;
	pInfo = (DDNS_INFO_T*)piscm->chart[tra].alloc_data;

	DMSG(1, "");

    memset(&regParam, 0, sizeof(NF_DDNS_COMMON_REG_PARAM));

	DMSG(1, ">>>> server : %s", pInfo->server);
	DMSG(1, ">>>> hostname : %s", pInfo->hostname);
	DMSG(1, ">>>> id : %s", pInfo->id);
	DMSG(1, ">>>> pwd : %s", pInfo->pwd);	

    strcpy(regParam.ddns_server, pInfo->server);
    strcpy(regParam.hostname, pInfo->hostname);
    strcpy(regParam.username, pInfo->id);
    strcpy(regParam.passwd, pInfo->pwd);
    
    ack_ret = ddns_common_force_register(&regParam);

	DMSG(1, "%d", ack_ret);
	return ack_ret;
}

static int _proc_test_ddns(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;
    DDNS_INFO_T *pInfo;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	DMSG(1, "");
	pInfo = (DDNS_INFO_T*)piscm->chart[tra].alloc_data;
	ack_ret = _test_ddns(pInfo->server, pInfo->hostname);
	DMSG(1, "%d", ack_ret);
	return ack_ret == 1 ? 0 : -1;
}

static int _proc_get_ddns_status(WRK_ID wrkid, CMM_MESSAGE_T *pmsg)
{
	SCM_T *piscm = (SCM_T *)pmsg->param;
    NF_DDNS_STATUS *status = imalloc(sizeof(NF_DDNS_STATUS));
	int ack_ret = 0;

	CMMACK_T cmmack;
	TRANSACTION_E tra;

	DMSG(9, "");
	wrk_get_cmmack(wrkid, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	DMSG(1, "");
    ddns_get_status(status);
	DMSG(1, "");

	piscm->chart[tra].result = (void *)status;
	return ack_ret;
}

////////////////////////////////////////////////////////////
//
// protected interfaces
//

int _scm_init_ddns_hostname()
{
	DDNSData ddnsdata;
	gchar mac_string[32];

	memset(&ddnsdata, 0x00, sizeof(ddnsdata));  
	_get_mac(mac_string, 32);
	DAL_get_ddns_data(&ddnsdata);

	if ((strlen(ddnsdata.host_name) == 0)) {
		strncpy(ddnsdata.host_name, mac_string, sizeof(mac_string));
		DAL_set_ddns_data(&ddnsdata);
	}
	return 0;
}

int _scm_work_ip_change(SCM_T *piscm, TRANSACTION_E tra)
{
	nf_netif_init();

	nf_issm_ctl(ISSM_STOP, __FUNCTION__);
	nf_issm_ctl(ISSM_START, __FUNCTION__);

	_scm_work_net_start(piscm, tra);
	return 0;
}

int _scm_work_ipcam_ctrl(SCM_T *piscm, TRANSACTION_E tra)
{
	switch (tra) {
	case TRA_IP_CHANGE:
	case TRA_DB_IMPORT:
	case TRA_FACTORY_DEFAULT:
		DMSG(9, "");
		nf_ipcam_ip_changed();
		break;
	}
		
	return 0;
}


HANDLER int _scm_on_network_work_cmpl(SCM_T *piscm, CMM_MESSAGE_T *pmsg)
{
	WRK_ID wrk_id = (WRK_ID)pmsg->data;
	TRANSACTION_E tra;
	CMMACK_T cmmack;
	int result = pmsg->param;

	wrk_get_cmmack(wrk_id, &cmmack);
	tra = (TRANSACTION_E)cmmack.data;

	switch (tra) {
	case TRA_GET_WAN_IP:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_wan_wrk(piscm);
		break;
	case TRA_REG_RTSPPORT:
	case TRA_RMV_RTSPPORT:
	case TRA_TST_RTSPPORT:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_rtsp_wrk(piscm);
		break;
	case TRA_REG_WEBPORT:
	case TRA_RMV_WEBPORT:
	case TRA_TST_WEBPORT:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_web_wrk(piscm);
		break;
	case TRA_REG_UNIMO:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_unimo_wrk(piscm);
		break;
	case TRA_REG_DDNS:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_ddns_wrk(piscm);
		break;
	case TRA_TST_DDNS:
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_ddnstest_wrk(piscm);
		break;
	case TRA_GET_DDNS_STATUS:	
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_ddnsStatus_wrk(piscm);
		break;		
	case TRA_TST_FTP:	
		DMSG(9, "");
		_scm_finalize_tra(piscm, tra, result);
		_scm_cleanup_ftp_wrk(piscm);
		break;				
	}
	return 0;
}

static void _send_sms_result_cb(int msgInt, const char* svr_msg)
{
	g_message("%s, %d, msgInt : %d", __FUNCTION__, __LINE__, msgInt);
	_scm_send_data_to_viewer(INFY_SMS_TEST_RESULT, msgInt, 0);
}

static void _send_mail_result_cb(gpointer user_data, NF_MAIL_SEND_STATUS status, const char *svr_msg)
{
	if ((status == NF_MAIL_SEND_STATUS_SUCCESS) || (status == NF_MAIL_SEND_STATUS_FAILED))
	{
		g_message("%s, %d", __FUNCTION__, __LINE__);	
		_scm_send_data_to_viewer(INFY_MAIL_TEST_RESULT, status, 0);
	}
}

static void _send_mail_verification_result_cb(gpointer user_data, NF_MAIL_SEND_STATUS status, const char *svr_msg)
{
	if ((status == NF_MAIL_SEND_STATUS_SUCCESS) || (status == NF_MAIL_SEND_STATUS_FAILED))
	{
		g_message("%s, %d", __FUNCTION__, __LINE__);	
		_scm_send_data_to_viewer(INFY_EMAIL_VERIFICATION_RESULT, status, 0);
	}
}

////////////////////////////////////////////////////////////
//
// public interfaces
//

int scm_apply_netinfo_by_db(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_IP_CHANGE;
	RS_SRVSTOP_E reason = _get_discon_reason(tra);
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	_scm_push_notification(INFY_NETCHANGE_API_BEGIN, tra);
	_scm_work_service_stop(&iscm, tra, reason);
	return 0;
}

int scm_register_rtsp_port(int port, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_REG_RTSPPORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_REG_RTSP, (void *)tra };
	if (iscm.wrk_rtsp) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = port;

	iscm.wrk_rtsp = wrk_create_worker(_proc_register_rtsp, &cmmack);
	wrk_run_once_param(iscm.wrk_rtsp, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_remove_rtsp_port(int port, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_RMV_RTSPPORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_RMV_RTSP, (void *)tra };
	if (iscm.wrk_rtsp) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = port;

	iscm.wrk_rtsp = wrk_create_worker(_proc_remove_rtsp, &cmmack);
	wrk_run_once_param(iscm.wrk_rtsp, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_test_rtsp_port(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_TST_RTSPPORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_TST_RTSP, (void *)tra };
	if (iscm.wrk_rtsp) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_rtsp = wrk_create_worker(_proc_test_rtsp, &cmmack);
	wrk_run_once_param(iscm.wrk_rtsp, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_register_web_port(int port, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_REG_WEBPORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_REG_WEB, (void *)tra };
	if (iscm.wrk_web) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = port;

	iscm.wrk_web = wrk_create_worker(_proc_register_web, &cmmack);
	wrk_run_once_param(iscm.wrk_web, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_remove_web_port(int port, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_RMV_WEBPORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_RMV_WEB, (void *)tra };
	if (iscm.wrk_web) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].int_data = port;

	iscm.wrk_web = wrk_create_worker(_proc_remove_web, &cmmack);
	wrk_run_once_param(iscm.wrk_web, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_test_web_port(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_TST_WEBPORT;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_TST_WEB, (void *)tra };
	if (iscm.wrk_web) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());

	iscm.wrk_web = wrk_create_worker(_proc_test_web, &cmmack);
	wrk_run_once_param(iscm.wrk_web, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_register_ddns(IMSG ret_msg)
{
    DDNS_INFO_T info;
    DDNSData data;
	TRANSACTION_E tra = TRA_REG_DDNS;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_REG_DDNS, (void *)tra };
	if (iscm.wrk_ddns) return 0;

    memset(&info, 0, sizeof(DDNS_INFO_T));
    memset(&data, 0, sizeof(DDNSData));

    DAL_get_ddns_data(&data);
    ifn_tolower(data.server);    
    strcpy(info.server, data.server);    
    strcpy(info.hostname, data.host_name);
    strcpy(info.id, data.id);
    strcpy(info.pwd, data.passwd);    

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(DDNS_INFO_T));	
	memcpy(iscm.chart[tra].alloc_data, &info, sizeof(DDNS_INFO_T));

	iscm.wrk_ddns = wrk_create_worker(_proc_register_ddns, &cmmack);
	wrk_run_once_param(iscm.wrk_ddns, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_register_ddns_with_info(DDNS_INFO_T *info, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_REG_DDNS;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_REG_DDNS, (void *)tra };
	if (iscm.wrk_ddns) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(DDNS_INFO_T));	
	memcpy(iscm.chart[tra].alloc_data, info, sizeof(DDNS_INFO_T));

	iscm.wrk_ddns = wrk_create_worker(_proc_register_ddns, &cmmack);
	wrk_run_once_param(iscm.wrk_ddns, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_register_unimo_with_info(UNIMOData *info, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_REG_UNIMO;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_REG_UNIMO, (void *)tra };
	if (iscm.wrk_unimo) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(UNIMOData));	
	memcpy(iscm.chart[tra].alloc_data, info, sizeof(UNIMOData));

	iscm.wrk_unimo = wrk_create_worker(_proc_register_unimo, &cmmack);
	wrk_run_once_param(iscm.wrk_unimo, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_test_ddns(IMSG ret_msg)
{
    DDNS_INFO_T info;
    DDNSData data;
	TRANSACTION_E tra = TRA_TST_DDNS;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_TST_DDNS, (void *)tra };
	if (iscm.wrk_ddnstest) return 0;

    memset(&info, 0, sizeof(DDNS_INFO_T));
    memset(&data, 0, sizeof(DDNSData));

    DAL_get_ddns_data(&data);
    ifn_tolower(data.server);
    strcpy(info.server, data.server);    
    strcpy(info.hostname, data.host_name);
    strcpy(info.id, data.id);
    strcpy(info.pwd, data.passwd);
    
	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(DDNS_INFO_T));	
	memcpy(iscm.chart[tra].alloc_data, &info, sizeof(DDNS_INFO_T));
	
	iscm.wrk_ddnstest = wrk_create_worker(_proc_test_ddns, &cmmack);
	wrk_run_once_param(iscm.wrk_ddnstest, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_test_ddns_with_info(DDNS_INFO_T *info, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_TST_DDNS;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_TST_DDNS, (void *)tra };
	if (iscm.wrk_ddnstest) return 0;
    
	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].alloc_data = imalloc(sizeof(DDNS_INFO_T));	
	memcpy(iscm.chart[tra].alloc_data, info, sizeof(DDNS_INFO_T));
	
	iscm.wrk_ddnstest = wrk_create_worker(_proc_test_ddns, &cmmack);
	wrk_run_once_param(iscm.wrk_ddnstest, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_req_ddns_status(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_GET_DDNS_STATUS;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_GET_DDNS_STATUS, (void *)tra };
	if (iscm.wrk_ddnsStatus) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.wrk_ddnsStatus = wrk_create_worker(_proc_get_ddns_status, &cmmack);
	wrk_run_once_param(iscm.wrk_ddnsStatus, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

int scm_check_mailaddr(char *addr)
{
	if (nf_mail_send_check_email(addr) == FALSE) return -1;
		
    return 0;
}

int scm_test_email(MAIL_SERVER_T *server, MAIL_CONTENT_T *content)
{
	NF_MAIL_SEND_SERVER server_info;
	NF_MAIL_SEND_CONTENT content_info;
	GError *err = NULL;

	memset(&server_info, 0x00, sizeof(NF_MAIL_SEND_SERVER));
	memset(&content_info, 0x00, sizeof(NF_MAIL_SEND_CONTENT));

	strcpy(server_info.smtp_server, server->server_name);
	strcpy(server_info.smtp_user, server->user);
	strcpy(server_info.smtp_passwd, server->passwd);
	server_info.smtp_port = server->port;
	server_info.smtp_security = server->security;

	strcpy(content_info.subject, "TEST MAIL");
	strcpy(content_info.contents, "This Mail is test mail by machine");
	strcpy(content_info.from, content->from);
	strcpy(content_info.to[0], content->to);
	content_info.to_cnt = 1;
	content_info.cb_func = _send_mail_result_cb;

	if (nf_mail_send_test(&server_info, &content_info, &err) == FALSE)
	{
		if (err) {
			g_message("%s, %d, error_code:%d", __FUNCTION__, __LINE__, err->code);
			g_error_free (err);
			err = NULL;
		}
	
		return -1;
	}    

    return 0;
}

int scm_req_wan_ip(IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_GET_WAN_IP;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_GET_WANIP, (void *)tra };
	if (iscm.wrk_wan) return 0;

	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.wrk_wan = wrk_create_worker(_proc_get_wan_ip, &cmmack);
	wrk_run_once_param(iscm.wrk_wan, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

REMOTE_USER_T *scm_new_remote_user_list(int *ret_cnt)
{
	return rmt_new_remote_user_info(ret_cnt);
}

int scm_free_remote_user_list(REMOTE_USER_T *list)
{
	rmt_free_remote_user_info(list);
	return 0;
}


SESSION_LIST_T *scm_new_net_session_list(int *ret_cnt)
{
	SESSION_LIST_T *tmp_p;

	tmp_p = rmt_new_session_list(ret_cnt);

	return  tmp_p;
}

int scm_free_net_session_list(SESSION_LIST_T *list)
{
	rmt_free_session_list(list);
	return 0;
}

int scm_get_net_session_count()
{
	SESSION_LIST_T *tmp = NULL;
	int cnt = 0;
	tmp = scm_new_net_session_list(&cnt);
	if (tmp) scm_free_net_session_list(tmp);

	DMSG(1, "count = %d", cnt);
	return cnt;
}

int scm_get_remote_user_info(char *user_id, REMOTE_USER_T *info)
{
	int i;
	int cnt;
	REMOTE_USER_T *rinfo = rmt_new_remote_user_info(&cnt);
	if (!rinfo) return -1;

	for (i = 0; i < cnt; ++i) {
		if (strcmp(rinfo[i].user_id, user_id) == 0) {
			memcpy(info, &rinfo[i], sizeof(REMOTE_USER_T));
			ifree(rinfo);
			return 0;
		}
	}

	ifree(rinfo);
	return -1;
}

int scm_is_connected_remote_user(char *user_id)
{
	int i;
	int cnt;
	REMOTE_USER_T *rinfo = rmt_new_remote_user_info(&cnt);
	if (!rinfo) return 0;

	for (i = 0; i < cnt; ++i) {
		if (strcmp(rinfo[i].user_id, user_id) == 0) {
			ifree(rinfo);
			return 1;
		}
	}

	ifree(rinfo);
	return 0;
}

int scm_disconnect_remote_user(char *user_id)
{
	SSID id;
	int i, j, k;
	int cnt;
	REMOTE_USER_T *rinfo = rmt_new_remote_user_info(&cnt);
	if (!rinfo) return 0;

	for (i = 0; i < cnt; ++i) {
		if (strcmp(rinfo[i].user_id, user_id) == 0) {
			for (j = 0; j < 4; ++j) {
				for (k = 0; k < rinfo[i].ssinfo[j].cnt; ++k) {
					rmt_disconnect_session(rinfo[i].ssinfo[j].id[k]);
				}
			}
			break;
		}
	}

	ifree(rinfo);
	return 0;
}

int scm_disconnect_session(SSID id)
{
	rmt_disconnect_session(id);
	return 0;
}

int scm_req_test_ftp(FTP_INFO_T *finfo, IMSG ret_msg)
{
	TRANSACTION_E tra = TRA_TST_FTP;
	CMMACK_T cmmack = { CMMPT_SCM, iRET_WRK_TST_FTP, (void *)tra };

	if (iscm.wrk_ftp) return 0;

	DMSG(9, "");
	_scm_ready_tra(&iscm, tra, ret_msg, IUX_CALLER());
	iscm.chart[tra].void_data = imalloc(sizeof(FTP_INFO_T));
	memcpy(iscm.chart[tra].void_data, finfo, sizeof(FTP_INFO_T));
	
	iscm.wrk_ftp = wrk_create_worker(_proc_test_ftp, &cmmack);
	wrk_run_once_param(iscm.wrk_ftp, IMSG_NONE, &iscm, 0, 0);
	return 0;
}

FTP_CODE_E scm_test_ftp(FTP_INFO_T *finfo)
{
	NF_FTP_CLIENT_REQ req;
	GError *err = NULL;
	FTP_CODE_E ret_val = 0;

	memset(&req, 0x00, sizeof(NF_FTP_CLIENT_REQ));
	memcpy(req.server, finfo->server, 256);
	req.port = finfo->port;
	memcpy(req.user, finfo->user, 64);
	memcpy(req.passwd, finfo->passwd, 64);

	if (req.user[0] == '\0') req.is_anon = 1;

	nf_arch_set_ftp_info(&req, 0);
	if (!nf_arch_ftp_test(0, 0, &err)) {
		if (err) {
			ret_val = err->code;	
			g_error_free(err);
		}
	}

	return ret_val;
}

int scm_restart_rtsp()
{
	nf_issm_ctl(ISSM_STOP, __FUNCTION__);
	nf_issm_ctl(ISSM_START, __FUNCTION__);
	return 0;
}

int scm_get_sys_netinfo(NF_NETIF_GET_INFO *info)
{
	DMSG(1, "");
	_get_sys_netinfo(info);
	return 0;
}

int scm_get_mac_addr_str(char *buf, int buf_len)
{
	return _get_mac(buf, buf_len);
}

int scm_get_dvr_addr_str(char *addr, int addr_len)
{
	_make_dvr_addr(addr, addr_len);
	return 0;
}

int scm_replace_host_in_addr(char *host_name, char *addr, int addr_len)
{
	_make_dvr_addr_with_host(host_name, addr, addr_len);
	return 0;
}

int scm_get_dvr_name(char *name, int name_len)
{
	_make_dvr_name(name, name_len);
	return 0;
}

int scm_get_ip_addr_str(char *addr, int addr_len)
{
	DMSG(1, "");
	return _get_ipaddr_str(addr, addr_len);
}

int scm_apply_ipfilter()
{
	gboolean ret = nf_netif_ipfilter_init();
	return (ret == TRUE);
}

int scm_test_sms(SMS_SERVER_T *serverinfo, SMS_RECEIVER_T *receiverinfo)
{
	NF_SMS_INFO info;
	NF_SMS_SEND_DATA data;

	memset(&info, 0x00, sizeof(NF_SMS_INFO));
	memset(&data, 0x00, sizeof(NF_SMS_SEND_DATA));

//		strcpy(sms_info.server, serverinfo->server);

	if (!strcmp(serverinfo->server, "BIZ PPURIO")) info.type = 0;
	else if (!strcmp(serverinfo->server, "CLICKATELL")) info.type = 1;
	else return -1;
	
	if (!strlen(serverinfo->user) || !strlen(serverinfo->password)) 
	{
		_scm_send_data_to_viewer(INFY_SMS_TEST_RESULT, -2, 0);
		return 0;
	}

	strcpy(info.username, serverinfo->user);		
	strcpy(info.passwd, serverinfo->password);		
	strcpy(info.app_id, serverinfo->appid);
	info.max_send_count = 100;
	info.start_avail_time = 0;
	info.end_avail_time = 2400;
	info.security = 0;

	strcpy(data.receiver[0], receiverinfo->number);
	strcpy(data.text, "TEST SMS");
	data.receiver_cnt = 1;
	data.cb_func = _send_sms_result_cb;

	nf_sms_push_event(&info, &data);
	return 0;
}

int scm_send_mail_verification_code(gchar *from, gchar *to, gchar *code)
{
	NF_MAIL_SEND_SERVER server_info;
	NF_MAIL_SEND_CONTENT content_info;
	GError *test_err = NULL;

	memset(&server_info, 0x00, sizeof(NF_MAIL_SEND_SERVER));
	memset(&content_info, 0x00, sizeof(NF_MAIL_SEND_CONTENT));

	strcpy(server_info.smtp_server, "SMTPPW.DVRLINK.NET");
	strcpy(server_info.smtp_user, "findpass");
	strcpy(server_info.smtp_passwd, "find@itx3398");
	server_info.smtp_port = 25;
	server_info.smtp_security = 0;

	strcpy(content_info.subject, "verification code");
	strcpy(content_info.contents, code);
	strcpy(content_info.from, from);
	strcpy(content_info.to[0], to);
	content_info.to_cnt = 1;
	content_info.cb_func = _send_mail_verification_result_cb;

	if (nf_mail_send_test(&server_info, &content_info, &test_err) == FALSE)
	{
		if (test_err) 
		{
			g_message("%s, %d, error_code:%d", __FUNCTION__, __LINE__, test_err->code);
			g_error_free (test_err);
			test_err = NULL;
		}
	
		return -1;
	}
	
	return 0;
}

int scm_sned_s1_sms_verification_code(gchar *receiver, gchar *code)
{
	NF_SMS_SEND_DATA data;
	gint err_code;

	memset(&data, 0x00, sizeof(NF_SMS_SEND_DATA));

	strcpy(data.receiver[0], receiver);
	strcpy(data.text, code);
	data.receiver_cnt = 1;

	return nf_sms_find_s1_password(&data);
}

gboolean scm_get_onestop_test()
{
	return nf_network_onestop_test();
}

int scm_set_qrcode(char *filename)
{
	int ret;
	char qr_url[256] = {0,};
	int url_len = 256;

	var_get_qr_url(qr_url, url_len);
	ret = nf_sysman_qrcode_gen(filename,"png",qr_url);

	return (ret == 0) ? 0 : -1;
}

int scm_set_qrcode_use_URL(char *filename, char *qr_url)
{
	int ret;

	ret = nf_sysman_qrcode_gen(filename,"png",qr_url);

	return (ret == 0) ? 0 : -1;
}

gboolean scm_net_conflict_get_ipset(gchar *ipset)
{
    gchar str_buf[64];
    gint ret;
    NF_NETIF_MAC ret_mac;

    memset(str_buf, 0x00, sizeof(str_buf));

    ret = nf_network_get_mac_conflict_ipset(&ret_mac);

        
    if(ret){
        g_sprintf(str_buf, "%02x : %02x : %02x : %02x : %02x : %02x",
            ret_mac.mac_addr[0]&0xff, ret_mac.mac_addr[1]&0xff, ret_mac.mac_addr[2]&0xff, ret_mac.mac_addr[3]&0xff, ret_mac.mac_addr[4]&0xff, ret_mac.mac_addr[5]&0xff);
            
        strcpy(ipset, str_buf);
    }
    
    return (ret == 1) ? 1 : 0;
}

gboolean scm_net_conflict_get_ipcam(guint ch, gchar *ipcam)
{
    gchar str_buf[64];
    gint ret;
    NF_NETIF_MAC ret_mac;

    memset(str_buf, 0x00, sizeof(str_buf));

    ret = nf_network_get_mac_conflict_ipcam(ch, &ret_mac);


    if(ret){
        g_sprintf(str_buf, "%02x : %02x : %02x : %02x : %02x : %02x",
            ret_mac.mac_addr[0]&0xff, ret_mac.mac_addr[1]&0xff,ret_mac.mac_addr[2]&0xff, ret_mac.mac_addr[3]&0xff, ret_mac.mac_addr[4]&0xff, ret_mac.mac_addr[5]&0xff);

        strcpy(ipcam, str_buf);
    }
    
    return (ret == 1) ? 1 : 0;
}

int scm_get_compressed_ipv6_addr(char *in_addr, char *out_addr)
{
    int ret;
    
    ret = nf_netif_compress_ipv6_addr(in_addr, out_addr);

    return (ret == 1) ? 1 : 0;
}

int scm_is_wan_connected()
{
	DMSG(1, "CALL API");
	gboolean ret = nf_network_is_connected_internet();
	DMSG(1, "RET = [%d]", ret);
	return (int)ret;
}
