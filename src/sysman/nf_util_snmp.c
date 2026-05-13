#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>		//for open
#include <unistd.h>		//for lseek
#include <string.h>
#include <stdlib.h>
#include <errno.h>
  
#include "nf_common.h"
#include "nf_util_snmp.h"
#include "nf_sysdb.h"
#include "nf_sysman.h"
//#include "nf_caps.h"

#define SNMPD_SCRIPT	"/etc/init.d/snmpd"

#define SNMPD_CONF_FILE	"/tmp/snmpd.conf"
#define SNMPD_DATA_FILE "/tmp/snmpd.data"
#define SNMPD_INFO_FILE "/tmp/snmpd.info"

#define SNMPTRAP_OID_SYS_TEMP ".1.3.6.1.4.1.25066.1.1.7"

/*

[root@ibuildman pelco]# snmptranslate -Tp -Oq  -IR cypherRoot
+--cypherRoot(25066)
   +--pelco-Camera-MIB(1)
      +--pelco-Camera-Diagnostic-MIB(1)
      |  +-- -R-- String    modelNumber(1)
      |  +-- -R-- String    modelName(2)
      |  +-- -R-- String    serialNumber(3)
      |  +-- -R-- String    firmwareVersion(4)
      |  +-- -R-- String    softwareVersion(5)
      |  +-- -R-- String    hardwareVersion(6)
      |  +-- -R-- Unsigned  systemTemperature(7)
      |  +-- -R-- Unsigned  powerStatus(8)
      |  +-- -R-- Unsigned  voltageStatus(9)
      |  |        Range: 0..1
      |  +-- -RW- Unsigned  rebootDevice(100)
      |           Range: 0..1
      |
      +--pelco-Camera-Accessory-MIB(3)
         |
         +--alarmTable(1)
         |  |
         |  +--alarmEntry(1)
         |     |  Index: alarmBankName
         |     |
         |     +-- -R-- String    alarmBankName(1)
         |     +-- -R-- INTEGER   alarmIndex(2)
         |     +-- -R-- INTEGER   alarmState(3)
         |
         +--relayTable(2)
            |
            +--relayEntry(1)
               |  Index: relayBankName
               |
               +-- -R-- String    relayBankName(1)
               +-- -R-- INTEGER   relayIndex(2)
               +-- -R-- INTEGER   relayState(3)
               
SNMPv2-SMI::enterprises.25066.1.1.1.0 = STRING: "ModelNumber" : nf_sysdb_get_str_nocopy("sys.info.model")
SNMPv2-SMI::enterprises.25066.1.1.2.0 = STRING: "ModelName" : nf_sysdb_get_str_nocopy("sys.info.model")
SNMPv2-SMI::enterprises.25066.1.1.3.0 = STRING: "SerialNumber" : nf_sysman_get_serial_num()
SNMPv2-SMI::enterprises.25066.1.1.4.0 = STRING: "FirmwareVersion" nf_sysdb_get_str_nocopy("sys.info.fake_swver")
SNMPv2-SMI::enterprises.25066.1.1.5.0 = STRING: "SoftwareVersion" nf_sysdb_get_str_nocopy("sys.info.fake_swver")
SNMPv2-SMI::enterprises.25066.1.1.6.0 = STRING: "HardwareVersion" nf_sysdb_get_str_nocopy("sys.info.hwver")

vi /tmp/snmpd.info

ModelNumber=a
ModelName=b
SerialNumber=c
FirmwareVersion=d
SoftwareVersion=e
HardwareVersion=f
num_alarm=1
num_relay=1

# ��ȸ �ϴ� ���
snmpwalk -v 2c -c public 192.168.100.197 .1.3.6.1.4.1.25066
snmpwalk -v 3 -u "aaaa" -l authNoPriv  -a MD5 -A "aaaa1234" 192.168.100.197 .1.3.6.1.4.1.25066
snmpwalk -v 3 -u "aaaa" -l authPriv -a MD5 -A "aaaa1234" -x DES -X "dddd1234"  192.168.100.197 .1.3.6.1.4.1.25066


# MIB trap ��Ʈ ���
-- 1.3.6.1.4.1.25066.1.1.7 
systemTemperature OBJECT-TYPE
SYNTAX Unsigned32   
MAX-ACCESS read-only
STATUS current      
DESCRIPTION  "Temperature of the camera near the lens; if a trap is configured, 
	the camera will send an SNMP trap message to the NMS when a temperature threshold is met"

# trap send
snmptrap -v 2c -c public 192.168.101.0:161 '' .1.3.6.1.4.1.25066 .1.3.6.1.4.1.25066.1.1.7 u 100

snmptrap -v 3 -u "username" -l noAuthNoPriv 192.168.101.0:161 '' .1.3.6.1.4.1.25066 .1.3.6.1.4.1.25066.1.1.7 u 100
snmptrap -v 3 -u "username" -l authNoPriv -a MD5 -A "PASSPHRASE" 192.168.101.0:161 '' .1.3.6.1.4.1.25066 .1.3.6.1.4.1.25066.1.1.7 u 100
snmptrap -v 3 -u "username" -l authPriv   -a MD5 -A "PASSPHRASE" -x DES -X "PASSPHRASE" 192.168.101.0:161 '' .1.3.6.1.4.1.25066 .1.3.6.1.4.1.25066.1.1.7 u 100

*/

static gboolean _snmpd_send_trap_v2(char *community, char *dest_host, char *oid, char *val_type, char *value)
{

	char buff[1024];
	int ret;

/*
	char *trap_addr = NULL;
	char *trap_commu = NULL;

	trap_addr = nf_sysdb_get_str_nocopy("net.snmp.v2.trap_conf.address");
	trap_commu = nf_sysdb_get_str_nocopy("net.snmp.v2.trap_conf.communitystring");
*/				

	// snmptrap -v 2c -c public 192.168.101.0:161 '' .1.3.6.1.4.1.25066 .1.3.6.1.4.1.25066.1.1.7 u 100
	snprintf(buff, sizeof(buff), "snmptrap -v 2c -c \"%s\" \"%s%s\" '' .1.3.6.1.4.1.25066 \"%s\" %s %s",
						community, 
						dest_host, strstr(dest_host,":") ? "" : ":161",
						oid, val_type, value);
												
	ret = proxy_system(buff, 1, 3);	
	return 1;
}

static gboolean _snmpd_send_trap_v3(char *username, char *amethod, char *apass, char *xmethod, char *xpass,
									char *dest_host, char *oid, char *val_type, char *value)
{
	char buff[1024];
	int ret;

/*
	char *usr_id = NULL;
	char *usr_auth = NULL;
	char *usr_auth_key = NULL;			
	char *usr_priv = NULL;
	char *usr_priv_key = NULL;
	char *receiver_addr = NULL;

	int no_auth = 0;
	int no_priv = 0;

	usr_id = nf_sysdb_get_str_nocopy("net.snmp.v3.userid");
	usr_auth = nf_sysdb_get_str_nocopy("net.snmp.v3.user_auth");
	usr_auth_key = nf_sysdb_get_str_nocopy("net.snmp.v3.user_auth_key");
	usr_priv = nf_sysdb_get_str_nocopy("net.snmp.v3.user_priv");			
	usr_priv_key = nf_sysdb_get_str_nocopy("net.snmp.v3.user_priv_key");
	receiver_addr = nf_sysdb_get_str_nocopy("net.snmp.v3.receiver_address");
*/

	if(xmethod){	// authPriv
		snprintf(buff, sizeof(buff), "snmptrap -v 3 -u \"%s\" -l authPriv -a \"%s\" -A \"%s\" -x \"%s\" -X \"%s\" \"%s%s\" '' .1.3.6.1.4.1.25066 \"%s\" %s %s",
						username, amethod, apass, xmethod, xpass,						
						dest_host, strstr(dest_host,":") ? "" : ":161",
						oid, val_type, value);
		
	} else if(amethod){ // authNoPriv
		snprintf(buff, sizeof(buff), "snmptrap -v 3 -u \"%s\" -l authNoPriv -a \"%s\" -A \"%s\"  \"%s%s\" '' .1.3.6.1.4.1.25066 \"%s\" %s %s",
						username, amethod, apass,
						dest_host, strstr(dest_host,":") ? "" : ":161",
						oid, val_type, value);		
	} else { // noAuthNoPriv
		snprintf(buff, sizeof(buff), "snmptrap -v 3 -u \"%s\" -l noAuthNoPriv  \"%s%s\" '' .1.3.6.1.4.1.25066 \"%s\" %s %s",
						username, 
						dest_host, strstr(dest_host,":") ? "" : ":161",
						oid, val_type, value);		
	}	

	ret = proxy_system(buff, 1, 3);	
	return 1;
}

static gboolean _snmpd_send_trap_common( char *oid, char *val_type, char *value )
{
	guint act = 0;			

	act = nf_sysdb_get_uint("net.snmp.version");
	if( act == 0 )		// disable snmp
		return 0;
	
	if( act == 1) {		// v2
		
		char *trap_addr = NULL;
		char *trap_commu = NULL;
		
		trap_addr = nf_sysdb_get_str_nocopy("net.snmp.v2.trap_conf.address");
		trap_commu = nf_sysdb_get_str_nocopy("net.snmp.v2.trap_conf.communitystring");
						
		if( trap_addr && strlen(trap_addr) > 0 && trap_commu )
		{			
			_snmpd_send_trap_v2(trap_commu, trap_addr, oid, val_type, value);
		}
				
	}else if( act ==2){	// v3

		char *usr_id = NULL;
		char *usr_auth = NULL;
		char *usr_auth_key = NULL;
		char *usr_priv = NULL;
		char *usr_priv_key = NULL;
		char *receiver_addr = NULL;
	
		receiver_addr = nf_sysdb_get_str_nocopy("net.snmp.v3.receiver_address");
		usr_auth = nf_sysdb_get_str_nocopy("net.snmp.v3.user_auth");
		usr_priv = nf_sysdb_get_str_nocopy("net.snmp.v3.user_priv");
		
		if( receiver_addr && strlen(receiver_addr) > 0 && usr_auth && usr_priv) {

			usr_id = nf_sysdb_get_str_nocopy("net.snmp.v3.userid");
			
			usr_auth_key = nf_sysdb_get_str_nocopy("net.snmp.v3.user_auth_key");
			usr_priv_key = nf_sysdb_get_str_nocopy("net.snmp.v3.user_priv_key");
						
			if( usr_id && usr_auth_key && usr_priv_key) {
				_snmpd_send_trap_v3(usr_id, strcmp(usr_auth,"NONE") ? usr_auth : NULL , usr_auth_key, 
										strcmp(usr_priv,"NONE") ? usr_priv : NULL , usr_priv_key,
										receiver_addr, oid, val_type, value);
			}else{
				g_warning("%s trap v3 send failed, params are missing", __FUNCTION__);
				return 0;
			}
		}		
	}
	return 1;	
}

static gboolean _snmpd_make_info_file()
{
	FILE *fp = NULL;
	
	g_message("%s start", __FUNCTION__);

	if(access(SNMPD_INFO_FILE, F_OK) == 0)
		remove(SNMPD_INFO_FILE);
	
	if ( (fp = fopen(SNMPD_INFO_FILE, "w")) == NULL ) {
		g_warning("%s file oepn error[%s]", __FUNCTION__, SNMPD_INFO_FILE);
		return 0;
	}
	
	fprintf(fp, "ModelNumber=%s\n", nf_sysdb_get_str_nocopy("sys.info.model"));
	fprintf(fp, "ModelName=%s\n", nf_sysdb_get_str_nocopy("sys.info.model"));
//	fprintf(fp, "SerialNumber=%s\n", nf_sysman_get_serial_num() );
		
	fprintf(fp, "FirmwareVersion=%s\n", nf_sysdb_get_str_nocopy("sys.info.fake_swver"));
	fprintf(fp, "SoftwareVersion=%s\n", nf_sysdb_get_str_nocopy("sys.info.fake_swver"));
	fprintf(fp, "HardwareVersion=%s\n", nf_sysdb_get_str_nocopy("sys.info.hwver"));
/*	
	{
	 	int num_alarm_in = nf_caps_get_int( nf_caps_get_root_obj() , "sys.num_alarm_in", NULL);
	 	int num_relay = nf_caps_get_int( nf_caps_get_root_obj() , "sys.num_relay", NULL);
	 		 	
		fprintf(fp, "num_alarm=%d\n", num_alarm_in);
		fprintf(fp, "num_relay=%d\n", num_relay);
	}	
*/
	fclose(fp);

	g_message("%s end", __FUNCTION__);
	
	return 1;
}


static gboolean _snmpd_sysdb_flag = FALSE;

static void _sysdb_set_change_flag( gboolean flag)
{
	_snmpd_sysdb_flag = flag;
}

static gboolean _sysdb_get_change_flag(void)
{
	return _snmpd_sysdb_flag;
}

static gboolean _snmpd_start(void)
{
	int ret;
	gchar cmd[1024];

	snprintf(cmd, sizeof(cmd), "%s start", SNMPD_SCRIPT);

	ret = proxy_system(cmd, 1, 3);

	return 0;	
}

static gboolean _snmpd_stop(void)
{
	int ret;
	gchar cmd[1024];

	snprintf(cmd, sizeof(cmd), "%s stop", SNMPD_SCRIPT);

	ret = proxy_system(cmd, 1, 3);
	
	sleep(2);

	return 0;
}

static int _generate_conf_file(void)
{
	guint act = 0;
	
	if(access(SNMPD_CONF_FILE, F_OK) == 0)
		remove(SNMPD_CONF_FILE);

	act = nf_sysdb_get_uint("net.snmp.version");

	if(	act > 0 )	
		_snmpd_make_info_file();

	if(	act > 0 )	
	{
	    FILE *fp;		
		char *comm_str = NULL;
		
		comm_str = nf_sysdb_get_str_nocopy("net.snmp.v2.communitystring");	

	    if ( (fp = fopen(SNMPD_CONF_FILE, "w")) == NULL )
	    {
	        g_warning("%s file oepn error", __FUNCTION__);
	        return 1;
	    }
		
		fprintf(fp, "agentaddress udp:161\n");
		fprintf(fp, "rocommunity %s\n", comm_str);

		if( act == 1)
		{
			char *trap_addr = NULL;
			char *trap_commu = NULL;
			
			trap_addr = nf_sysdb_get_str_nocopy("net.snmp.v2.trap_conf.address");
			trap_commu = nf_sysdb_get_str_nocopy("net.snmp.v2.trap_conf.communitystring");
			
			fprintf(fp, "trap2sink %s %s\n", trap_addr, trap_commu);
		}
		else if( act == 2 )
		{
			char *usr_id = NULL;
			char *usr_auth = NULL;
			char *usr_auth_key = NULL;			
			char *usr_priv = NULL;
			char *usr_priv_key = NULL;
			char *receiver_addr = NULL;

			int no_auth = 0;
			int no_priv = 0;

			usr_id = nf_sysdb_get_str_nocopy("net.snmp.v3.userid");
			usr_auth = nf_sysdb_get_str_nocopy("net.snmp.v3.user_auth");
			usr_auth_key = nf_sysdb_get_str_nocopy("net.snmp.v3.user_auth_key");
			usr_priv = nf_sysdb_get_str_nocopy("net.snmp.v3.user_priv");			
			usr_priv_key = nf_sysdb_get_str_nocopy("net.snmp.v3.user_priv_key");
			receiver_addr = nf_sysdb_get_str_nocopy("net.snmp.v3.receiver_address");

			fprintf(fp, "createUser %s", usr_id);

			if( !strcmp(usr_auth, "MD5") )
			{	
				fprintf(fp, " MD5 %s", usr_auth_key);
			}
			else if( !strcmp(usr_auth, "SHA") )
			{			
				fprintf(fp, " SHA %s", usr_auth_key);
			}
			else
			{
				no_auth = 1;
				no_priv = 1;
			}

			if( no_auth == 0 )
			{
				if( !strcmp(usr_priv, "DES") )
				{
					fprintf(fp, " DES %s", usr_priv_key);				

				}
				else if( !strcmp(usr_priv, "AES") )
				{
					fprintf(fp, " AES %s", usr_priv_key);
				}
				else
					no_priv = 1;
			}

			fprintf(fp, "\n");			

			if (no_auth) {
				fprintf(fp, "rwuser %s noauth\n", usr_id);
			} else {
				fprintf(fp, "rwuser %s auth\n", usr_id);
			}
			fprintf(fp, "trapsess -v 3 -u %s", usr_id);

			if( no_auth == 0 )
			{
				if( !strcmp(usr_auth, "MD5") )
				{
					fprintf(fp, " -a MD5 -A %s", usr_auth_key);
				}
				else
				{
					fprintf(fp, " -a SHA -A %s", usr_auth_key);
				}				
			}

			if( no_priv == 0 )
			{
				if( !strcmp(usr_priv, "DES") )
				{
					fprintf(fp, " -x DES -X %s", usr_priv_key);	
				}
				else
				{
					fprintf(fp, " -x AES -X %s", usr_priv_key);
				}
			}

			if(no_auth)
				fprintf(fp, " -l NoAuth");
			else
				fprintf(fp, " -l Auth");

			if(no_priv)
				fprintf(fp, "NoPriv %s\n", receiver_addr);
			else
				fprintf(fp, "Priv %s\n", receiver_addr);
		}

		fclose(fp);		
	}

	return 0;
}

static int _snmpd_reload(void)
{
	g_message("%s STRAT", __FUNCTION__);
	
	_snmpd_stop();

	_generate_conf_file();

	if(access(SNMPD_CONF_FILE, F_OK) == 0)
	{
		_snmpd_start();
	}

	g_message("%s END", __FUNCTION__);

	return 0;
}

static volatile guint _is_trap_send_sys_temp = 0;
static volatile guint _trap_value_sys_temp  = 0;

static volatile guint _is_data_update = 0;
static volatile guint _data_value_sys_temp = 0;
static volatile guint _data_value_alarm_in_mask  = 0;
static volatile guint _data_value_relay_mask  = 0;

static gboolean _snmpd_make_data_file()
{
	FILE *fp = NULL;
	
	g_message("%s start", __FUNCTION__);

	if(access(SNMPD_DATA_FILE, F_OK) == 0)
		remove(SNMPD_DATA_FILE);
	
	if ( (fp = fopen(SNMPD_DATA_FILE, "w")) == NULL ) {
		g_warning("%s file oepn error[%s]", __FUNCTION__, SNMPD_DATA_FILE);
		return 0;
	}
	
	fprintf(fp, "%u %u %d\n", _data_value_sys_temp, _data_value_alarm_in_mask, _data_value_relay_mask   );
	fclose(fp);
	
	g_message("%s end", __FUNCTION__);
	
	return 1;
}


static void _snmpd_thread(void *arg)
{
	_snmpd_reload();
		
	while(1)
	{

		if( _sysdb_get_change_flag() )
		{
			_sysdb_set_change_flag(FALSE);			
			_snmpd_reload();
		}		
		
		if( _is_trap_send_sys_temp )
		{
			char buff[128];
			
			_is_trap_send_sys_temp  = 0;
			
			snprintf(buff, sizeof(buff), "%d", _trap_value_sys_temp);		
			_snmpd_send_trap_common( SNMPTRAP_OID_SYS_TEMP, "u", buff);
		}

		if( _is_data_update )
		{
			_is_data_update = 0;
			_snmpd_make_data_file();
		}
		
		usleep(200*1000);
	}
}


static void _snmpd_sysdb_reload_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);
	g_message("%s called", __FUNCTION__);

	if( pinfo->d.params[0] == NF_SYSDB_CATE_NET )
		_sysdb_set_change_flag(TRUE);
}

static void _snmpd_sys_temp_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	g_return_if_fail(pinfo != NULL);

/*
	sys_temperature
		d.param[0]	temperature fail mask      fan�� �������ų�, ��ǳ���� ������ �ý��� �µ��� 65 �Ѿ��� ��
					0x01 CPU, 0x02 SYSTEM
		d.param[1]	cpu temperature				���� CPU �µ�
		d.param[2]	system temperature			���� System �µ�
*/		

	if( pinfo->d.params[0] )
	{
		_is_trap_send_sys_temp = 1;
		_trap_value_sys_temp = pinfo->d.params[2]; 
	}
	
	_is_data_update = 1;
	_data_value_sys_temp = pinfo->d.params[2];	
}

static void _snmpd_alarm_in_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data) // notify (sensor)
{
	g_return_if_fail(pinfo != NULL);

	_is_data_update = 1;
	_data_value_alarm_in_mask = pinfo->d.params[0];	
}

static void _snmpd_relay_cb_func(NF_NOTIFY_INFO *pinfo, gpointer data) // notify (relay)
{
	g_return_if_fail(pinfo != NULL);

	_is_data_update = 1;
	_data_value_relay_mask = pinfo->d.params[0];	
}


gboolean nf_snmpd_init(void)
{	
    pthread_t tid = 0;
	guint	cb_handle = 0;

	cb_handle= nf_notify_connect_cb( "sysdb_change", _snmpd_sysdb_reload_cb_func, NULL);
	g_message("%s notify sysdb connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sys_temperature", _snmpd_sys_temp_cb_func, NULL);
	g_message("%s notify sys_temp connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "sensor", _snmpd_alarm_in_cb_func, NULL);
	g_message("%s notify sensor(alarm_in) connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

	cb_handle= nf_notify_connect_cb( "alarm", _snmpd_relay_cb_func, NULL);
	g_message("%s notify alarm(relay) connect_cb[%d]",__FUNCTION__, cb_handle );
	g_assert(cb_handle >0);

//	_data_value_sys_temp = nf_notify_get_param2("sys_temperature");	
	_data_value_alarm_in_mask = nf_notify_get_param0("sensor");	
	_data_value_relay_mask = nf_notify_get_param0("alarm");	
	_snmpd_make_data_file();
	
    if ( pthread_create(&tid, NULL, (void*)_snmpd_thread, NULL) != 0)
    {
        return 0;
    }

    pthread_detach(tid);
	
    return 1;
}