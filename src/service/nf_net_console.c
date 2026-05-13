#include "nf_common.h"
#include "nf_sysman.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

const char* config_fixed_data =
"#### MODULES ####\n"
"module(load=\"imuxsock\") # provides support for local system logging (e.g. via logger command)\n"
"module(load=\"imklog\")   # provides kernel logging support (previously done by rklogd)\n"
"module(load=\"imtcp\") # needs to be done just once\n"
"%s\n\n"

"#### GLOBAL DIRECTIVES ####\n"
"# Use default timestamp format\n"
"$ActionFileDefaultTemplate RSYSLOG_TraditionalFileFormat\n\n"

"# Include all config files in /etc/rsyslog.d/\n"
"$IncludeConfig /etc/rsyslog.d/*.conf\n\n"

"#### RULES ####\n"

"#kern.*                                                 /tmp/log/user.log\n"
"#root.*                                                 /tmp/log/user.log\n\n"

"#*.info;mail.none;authpriv.none;cron.none;local0.none	/tmp/log/user.log\n"
"#*.notice                /tmp/log/user.log\n\n"

"# Everybody gets emergency messages\n"
"#*.emerg                                                 :omusrmsg:*\n\n"

"# start log rotation via outchannel (256Kbyte size limit)\n"
"# outchannel definition\n"
"#$outchannel log_rotation, /tmp/log/user.log, 262144, /bin/sh /etc/rsyslog.d/logrotate_script\n"
"#*.* :omfile:$log_rotation\n"
"# end log rotation via outchannel\n\n"

"# remote host is: name/ip:port, e.g. 192.168.0.1:514, port optional\n"
"%s\n";

extern int proxy_system(const char *str, int mode, int timeout_sec);
char * config_input_data = "input(type=\"imtcp\" port=\"%s\")";
char * config_addr_data = "*.* @@%s:%s\n";

int _nf_net_console_config_change(char* ip_str, char* port_num);
int _nf_net_console_check_debug_mode();
void nf_net_console_ctrl();
int nf_net_console_ctrl_check(char* ip_str, char* port_num);
int nf_net_console_ctrl_init();
void _notify_cb_net_console_ctrl (NF_NOTIFY_INFO *pinfo, gpointer data);

int _nf_net_console_config_change(char* ip_str, char* port_num)
{
		FILE *fp = NULL;
		char input_buf[64] = {0};
		char addr_buf[64] = {0,};

		fp = fopen("/etc/rsyslog.conf", "w");
		if(fp == NULL)
		{
			printf("\e[%s][%d] ERROR! \e[0m\n", __FUNCTION__, __LINE__);
			return 0;
		}
		
		if(ip_str != NULL && port_num != NULL)
		{
			sprintf(input_buf, config_input_data, port_num);
			sprintf(addr_buf, config_addr_data, ip_str, port_num);
			fprintf(fp, config_fixed_data, input_buf, addr_buf);
		}
		else
			fprintf(fp, config_fixed_data, "\n", "\n");

		fclose(fp);

	return 1;
}

int _nf_net_console_check_debug_mode()
{

	char *tmp_mail = NULL;
	tmp_mail = nf_sysdb_get_str_nocopy("net.email.testmail");

	if( strcmp( tmp_mail, "choissi@debug.com" ) == 0 )
	{
		if (nf_sysman_get_serial_enable())
			return 0;
		else
			return 1;
	}
	else
		return 0;
}

int nf_net_console_ctrl_check(char* ip_str, char* port_num)
{
	int ret = 0;

#if defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_0824P4E) || \
	defined(_IPX_0824M4E) || defined(_IPX_0412M4E)
			if(_nf_net_console_check_debug_mode() == 1 && ip_str != NULL && port_num != NULL)
			{
				printf("\e[31m [%s][%d] ip modify \e[0m\n", __FUNCTION__, __LINE__);
				ret = _nf_net_console_config_change(ip_str, port_num);
				proxy_system("/etc/init.d/S01logging start &", 1, 3);

			}
			else
			{
				printf("\e[31m [%s][%d] ip null \e[0m\n", __FUNCTION__, __LINE__);
				ret = _nf_net_console_config_change(NULL, NULL);
				proxy_system("/etc/init.d/S01logging stop &", 1, 3);
			}
#endif

	return ret;
}

void _notify_cb_net_console_ctrl (NF_NOTIFY_INFO *pinfo, gpointer data)
{
	 nf_net_console_ctrl();
}

int nf_net_console_ctrl_init()
{
		static gulong _cb_handle = 0;

		if( _cb_handle != 0 )
			return 0;

		_cb_handle= nf_notify_connect_cb( "sysdb_change", _notify_cb_net_console_ctrl, NULL);
		g_message("%s connected cb_handle[%lu]",__FUNCTION__, _cb_handle);
	return 1;
}

void nf_net_console_ctrl()
{
	const char* ERROR_STR[] = 
	{
		"ERROR OCCURED",
		"SUCCESS",
	};

	int ret = -1;
	char *ip_str = NULL;
	char port_str[16] = {0,};
	unsigned int tmp_port = 0;

	ip_str = nf_sysdb_get_str_nocopy("net.email.smtpsvr");
	tmp_port =  nf_sysdb_get_uint( "net.email.smtpport");
	sprintf(port_str, "%d", tmp_port);

	ret = nf_net_console_ctrl_check(ip_str, port_str);

	printf("\e[31m [%s][%d] return : %s \e[0m\n", __FUNCTION__, __LINE__, ERROR_STR[ret]); 
}

