#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "nf_common.h"
#include "nf_util_wpa.h"

#define WPA_HEAD	"network={\nssid=\"dot1x\"\nkey_mgmt=IEEE8021X\n"
#define WPA_TLSV12	"phase1=\"tls_disable_tlsv1_0=1 tls_disable_tlsv1_1=1 tls_disable_tlsv1_2=0\"\n"
#define WPA_END		"}"
#define WPA_EAPOL_V2 "eapol_version=2\n"

#define WPA_CONF	"/tmp/wpa/wpa_supplicant.conf"
#define WPA_DIR		"/tmp/wpa"
#define WPA_CA_CERT	"/NFDVR/data/IPKI?nac/ca_cert_link"
#define WPA_CLIENT_CERT "/NFDVR/data/IPKI/nac/client_cert_link"
#define WPA_PRIVATE_KEY "/NFDVR/data/IPKI/nac/client_key_link"


// DB 
#define DB_KEY_USE			"net.8021x.use"
#define DB_KEY_EAP_TYPE 	"net.8021x.eap_type"
#define DB_KEY_EAP_VERSION 	"net.8021x.eapol_ver"
#define DB_KEY_ID			"net.8021x.id"
#define DB_KEY_PW			"net.8021x.pw"
#define DB_KEY_PRIVATE_KEY_PW "net.8021x.prikey_pw"
#define DB_KEY_ANONYMOUS	"net.8021x.anoymous"
#define DB_KEY_INNER_AUTH	"net.8021x.inner_auth"

static int _wpa_start(void);
static int _wpa_stop(void);

// cert file generate
static gboolean _generate_cert_file(void)
{
	FILE *fp = NULL;
	// skip, need to talk UI-team
	//
	
	// if certs exist, remove it to regenerate cert file
	
	// ca_cert
	if(access(WPA_CA_CERT, F_OK) == 0)
		remove(WPA_CA_CERT);
	
	// client_cert
	if(access(WPA_CLIENT_CERT, F_OK) == 0)
		remove(WPA_CLIENT_CERT);

	// private_key
	if(access(WPA_PRIVATE_KEY, F_OK) == 0)
		remove(WPA_PRIVATE_KEY);

	return 1;
}

static gboolean _wpa_conf_gen_md5(void)
{
	char *uname = NULL;
	char *passwd = NULL;
	guint eapol_version = 0;

	FILE *fp;
	if((fp=fopen(WPA_CONF, "w")) == NULL)
	{
		printf("[%s:%d] %s file open error\n", __func__, __LINE__, WPA_CONF);
		return 0;
	}

	// ID,PW -> GET DB
	uname = nf_sysdb_get_str_nocopy(DB_KEY_ID);
	passwd = nf_sysdb_get_str_nocopy(DB_KEY_PW);
	eapol_version = nf_sysdb_get_uint(DB_KEY_EAP_VERSION);
	// test
	//uname = "md5id";
	//passwd = "md5pw";
	
	if(eapol_version != 0)
		fprintf(fp, "%s", WPA_EAPOL_V2);

	fprintf(fp, "%s", WPA_HEAD);
	fprintf(fp, "eap=MD5\n");
	fprintf(fp, "identity=\"%s\"\n", uname);
	fprintf(fp, "password=\"%s\"\n", passwd);
	
	fprintf(fp, "%s", WPA_END);

	
	fclose(fp);
	return 1;
}

static gboolean _wpa_conf_gen_tls(void)
{
	char *uname = NULL;
	char *private_key_pw = NULL;
	char *client_cert = NULL;
	char *client_key = NULL;
	char *ca_cert = NULL;
	guint eapol_version = 0;
	FILE *fp;

	if((fp=fopen(WPA_CONF, "w")) == NULL)
	{
		printf("[error] %s not open \n", __func__);
		return 0;
	}
	
	// GET DB
	uname = nf_sysdb_get_str_nocopy(DB_KEY_ID);
	if(strlen(uname)==0)
		uname = "Unknown";
	private_key_pw = nf_sysdb_get_str_nocopy(DB_KEY_PRIVATE_KEY_PW);
	eapol_version = nf_sysdb_get_uint(DB_KEY_EAP_VERSION);
	// ca_cert, client_key ,client_cert is file, check path
	
	
	// test
	// uname = "md5id";
	// private_key_pw = "md5pw";
	// if(uname && private_key_pw)
	if(1)
	{
		if(eapol_version != 0)
			fprintf(fp, "%s", WPA_EAPOL_V2);

		fprintf(fp, "%s", WPA_HEAD);
		fprintf(fp, "eap=TLS\n");
		fprintf(fp, "identity=\"%s\"\n", uname);
		fprintf(fp, "client_cert=\"%s\"\n", WPA_CLIENT_CERT);
		fprintf(fp, "private_key=\"%s\"\n", WPA_PRIVATE_KEY);
		fprintf(fp, "private_key_passwd=\"%s\"\n", private_key_pw);
		fprintf(fp, "%s", WPA_TLSV12);

		if(access(WPA_CA_CERT, F_OK) == 0)
		{
			fprintf(fp, "ca_cert=\"%s\"\n", WPA_CA_CERT);
		}
		
		fprintf(fp, "%s", WPA_END);
	}

	fclose(fp);
	return 1;
}

static gboolean _wpa_conf_gen_ttls(void)
{
	// skip
	return 1;
}

static gboolean _wpa_conf_gen_peap(void)
{
	//skip
	return 1;
}

static gboolean _generate_conf_file(void)
{
	guint proto;
	if(access(WPA_CONF, F_OK)==0)
		remove(WPA_CONF);

	// GET DB
	proto = nf_sysdb_get_uint("net.8021x.eap_type");

	if( proto == NF_WPA_EAP_MD5 )
		_wpa_conf_gen_md5();
	else if ( proto == NF_WPA_EAP_TLS )
		_wpa_conf_gen_tls();
	else if ( proto == NF_WPA_EAP_TTLS )
		_wpa_conf_gen_ttls();
	else if ( proto == NF_WPA_EAP_PEAP )
		_wpa_conf_gen_peap();
	else
		return 0;

	return 1;
}

gboolean nf_wpa_start(void )
{
	printf("%s\n", __func__);
	int ret;
	char cmd[512];
	guint enable;

	_wpa_stop();

	enable = nf_sysdb_get_uint("net.8021x.use");
	if(enable)
	{
		if(access(WPA_DIR, F_OK) == -1)
		{
			mkdir(WPA_DIR, 0744);
		}
		_generate_conf_file();
		printf("[khkh] Start WPA SUPPLICANT \n");
		_wpa_start();
	}
	else
	{
		printf("[khkh] NO SELECT WPA SUPPLICANT \n");
	}
	return 1;
}

static int _wpa_stop(void)
{
	char cmd[512];
	int ret;
	
	sprintf(cmd, "killall -9 wpa_supplicant");
	ret = proxy_system(cmd, 1, 3);
	return ret;
}

static int _wpa_start(void)
{
	char cmd[512];
	int ret;
	guint is_duallan = 0;
	is_duallan = nf_sysdb_get_bool("cam.install.dual_lan");
	
	if(is_duallan)
		sprintf(cmd, "wpa_supplicant -B -Dwired -ieth0 -c %s -dd", WPA_CONF);
	else
		sprintf(cmd, "wpa_supplicant -B -Dwired -ieth0 -bbr0 -c %s -dd", WPA_CONF);
	ret = proxy_system(cmd, 1, 3);
	sleep(10);
	return ret;
}
