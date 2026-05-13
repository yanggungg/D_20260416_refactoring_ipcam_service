#include <sys/mount.h>
#include <libgen.h>

#include "nf_common.h"
#include "nf_util_fw_single.h"
#include "nf_api_param_app.h"
#include "nf_sysman.h"
#include "nf_util_fw.h"

#if defined(CONFIG_FWUPGRADE_SINGLE)
/** gloval variable definition **/
extern int proxy_system(const char *str, int mode, int timeout_sec);
extern gchar _nf_fw_filename[FW_UPGRADE_MAX_BUFFER];

static guint _nf_fwup_status=0;
static guint g_magic_key=NF_FWUP_MAGIC_KEY_RAMDISK;
static char _nf_fwup_data_path[256]={0, };

gboolean nf_fw_update_clear(void)
{
	gchar tmp[256]={0,};

	g_message("%s Upgrade data directory remove [%s]!!", __FUNCTION__, _nf_fwup_data_path);

	if(strstr(_nf_fwup_data_path, "NFDVR"))
	{
		sprintf(tmp, "rm -rvf %s/nf_sysdb_*.conf", _nf_fwup_data_path);
		proxy_system(tmp, 1, 3);
		sprintf(tmp, "rm -rvf %s/log_000.log", _nf_fwup_data_path);
		proxy_system(tmp, 1, 3);
		sprintf(tmp, "rm -rvf %s/progress_*.bmp", _nf_fwup_data_path);
		proxy_system(tmp, 1, 3);
		sprintf(tmp, "rm -rvf %s/dgss_data.cfg.gz", _nf_fwup_data_path);
		proxy_system(tmp, 1, 3);
	}
	else
	{
		sprintf(tmp, "rm -rvf %s", _nf_fwup_data_path);
		proxy_system(tmp, 1, 3);
	}

	if(!nf_sysman_fwup_bit_clr())
	{
		g_warning("%s Upgrade bit clear Error!!", __FUNCTION__);
		return FALSE;
	}
	return TRUE;
}

guint nf_fw_update_check(void)
{
	g_message("%s status[0x%x]", __FUNCTION__, _nf_fwup_status);
	return _nf_fwup_status;
}

void nf_fw_update_check_clear(void)
{
	_nf_fwup_status = NF_FWUP_STATUS_DONE;
}

gboolean nf_fw_update_verify(char* data_path)
{
	NF_PARAM_APP app_param;
	gchar buff[256]={0,};
	int i,ret;
	char tmp[1024];
	char *usb_str = NULL;
	FILE *fp=NULL;
	guint magic_key=0;

	ret = nf_api_param_app_get(&app_param);
	if(!ret)
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_FAIL;
		return FALSE;
	}
	else
		g_message("%s Get App Param Pass!!!", __FUNCTION__);

	if(strstr(app_param.fw_file_name, "http"))
	{
		sprintf(data_path, "%s", NF_FW_INSTALL_FILE_DIR);
		memcpy(_nf_fwup_data_path, data_path, strlen(data_path));

		if(app_param.fwup_status != NF_FWUP_STATUS_UPGRADE_NET_DONE)
		{
			g_warning("%s FW UPGRADE FAIL!!!", __FUNCTION__);
			_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_NET_FAIL;
			return FALSE;
		}
		else
			g_message("%s FW Upgrade Pass!!", __FUNCTION__);

		_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_NET_DONE;
	}
	else if(app_param.fwup_magic_key == 0xffffffff)
	{
		sprintf(data_path, "%s", NF_FW_INSTALL_FILE_DIR);
		memcpy(_nf_fwup_data_path, data_path, strlen(data_path));

		if(app_param.fwup_status != NF_FWUP_STATUS_UPGRADE_RAM_DONE)
		{
			g_warning("%s FW UPGRADE FAIL!!!", __FUNCTION__);
			_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_RAM_FAIL;
			return FALSE;
		}

		_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_RAM_DONE;
	}
	else if(app_param.fwup_magic_key == 0xfffffffe)
	{
		sprintf(data_path, "%s", app_param.db_file_name);

		if(app_param.fwup_status != NF_FWUP_STATUS_UPGRADE_NET_DOWN_DONE)
		{
			g_warning("%s FW UPGRADE FAIL!!!", __FUNCTION__);
			_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_NET_DOWN_FAIL;
			return FALSE;
		}

		_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_NET_DOWN_DONE;
	}
	else
	{
		sprintf(data_path, "%s/%x", NF_SYSMAN_FS_MOUNT_DIR_NAME, app_param.fwup_magic_key);
		memcpy(_nf_fwup_data_path, data_path, strlen(data_path));

		if(app_param.fwup_status != NF_FWUP_STATUS_UPGRADE_DONE)
		{
			g_warning("%s FW UPGRADE FAIL!!!", __FUNCTION__);
			_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_FAIL;
			return FALSE;
		}

		for(i=0;i<NF_FW_CHECK_TRY_DISK_CNT;++i)
		{
			snprintf( buff, sizeof(buff), "/sys/block/sd%c", 'a'+i);
			memset( tmp, 0x00, sizeof(tmp));
			ret = readlink( buff, tmp, sizeof(tmp));
			if(ret>0) {
				usb_str = strstr(tmp ,"usb");
				if(usb_str) {
					snprintf( buff, sizeof(buff), "/dev/sd%c1", 'a'+i);
					break;
				}
			}
		}

		if(i >= NF_FW_CHECK_TRY_DISK_CNT)
		{
			g_warning("%s DB file not found!!!", __FUNCTION__);
			_nf_fwup_status = NF_FWUP_STATUS_DB_FAIL;
			return FALSE;
		}

		ret = mkdir(NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME, 0755);
		if(ret != 0)
			g_warning("%s %s exists..", __FUNCTION__, NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME);

		ret = mkdir(NF_SYSMAN_FS_MOUNT_DIR_NAME, 0755);
		if(ret != 0)
			g_warning("%s %s exists..", __FUNCTION__, NF_SYSMAN_FS_MOUNT_DIR_NAME);

		if((ret = mount(buff, NF_SYSMAN_FS_MOUNT_DIR_NAME, "vfat" , 0, 0)) !=0)
		{
			g_warning("%s mount error!!! ret[%d] [%s]->[%s]", __FUNCTION__, ret, buff, NF_SYSMAN_FS_MOUNT_DIR_NAME);
			_nf_fwup_status = NF_FWUP_STATUS_DB_FAIL;
			return FALSE;
		}else{
			g_message("%s mounted [%s]->[%s]", __FUNCTION__, buff, NF_SYSMAN_FS_MOUNT_DIR_NAME);
		}

		if(access(data_path, F_OK))
		{
			g_warning("%s data directory not found!!  [%s]", __FUNCTION__, data_path);
			_nf_fwup_status = NF_FWUP_STATUS_DB_FAIL;
			return FALSE;
		}

		_nf_fwup_status = NF_FWUP_STATUS_UPGRADE_DONE;
	}

	return TRUE;
}

gboolean nf_fw_data_backup(const gchar *mnt_path)
{
	gboolean ret=0;
	gchar *fw_version=NULL;
	gchar tmp_key[256]={0,};
	gint i;

	g_return_val_if_fail(mnt_path != NULL, 0);

	g_message("[%s]  mnt_path[%s]", __FUNCTION__, mnt_path);
	if(strstr(mnt_path, "http") || strstr(mnt_path, "ramdisk") || (strstr(mnt_path, NET_FWUP_MOUNT_DIR_NAME)))
	{
		g_magic_key = NF_FWUP_MAGIC_KEY_RAMDISK;
		/** data file copy **/
		sprintf(tmp_key, "cp -a %s %s/log_000.log", "/NFDVR/log/log_000.log", NF_FW_INSTALL_FILE_DIR);
		proxy_system(tmp_key, 1, 3);
		sprintf(tmp_key, "cp -a %s %s", "/NFDVR/data/nf_sysdb_*.conf", NF_FW_INSTALL_FILE_DIR);
		proxy_system(tmp_key, 1, 3);

		#if 0		// For S1
			sprintf(tmp_key, "cp -a %s %s", "/NFDVR/data/dgss_data.cfg.gz", NF_FW_INSTALL_FILE_DIR);
			proxy_system(tmp_key, 1, 3);
		#endif

		sprintf(tmp_key, "cp -avrf %s %s/", "/NFDVR/data/IPKI", NF_FW_INSTALL_FILE_DIR);
		proxy_system(tmp_key, 1, 3);
		
		if (access("/NFDVR/webra/conf/ssl_public_key.pem", F_OK) == 0) {
			sprintf(tmp_key, "cp -a %s %s", "/NFDVR/webra/conf/ssl_public_key.pem", NF_FW_INSTALL_FILE_DIR);
			proxy_system(tmp_key, 1, 3);
		}
		else {
			g_message("[%s %d] Nothing file : ssl_public_key.pem", __FUNCTION__, __LINE__);
		}
		
		if (access("/NFDVR/webra/conf/ssl_private_key.key", F_OK) == 0) {
			sprintf(tmp_key, "cp -a %s %s", "/NFDVR/webra/conf/ssl_private_key.key", NF_FW_INSTALL_FILE_DIR);
			proxy_system(tmp_key, 1, 3);
		}
		else {
			g_message("[%s %d] Nothing file : ssl_private_key.key", __FUNCTION__, __LINE__);
		}
		
		if (access("/NFDVR/webra/conf/sslcertification.pem", F_OK) == 0) {
			sprintf(tmp_key, "cp -a %s %s", "/NFDVR/webra/conf/sslcertification.pem", NF_FW_INSTALL_FILE_DIR);
			proxy_system(tmp_key, 1, 3);
		}
		else {
			g_message("[%s %d] Nothing file : sslcertification.pem", __FUNCTION__, __LINE__);
		}
		
		/** install file copy **/
		// #if defined(MODEL_DVR)
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.uyvy");
				sprintf(tmp_key, "%s", "/NFDVR/data/fwup_bg.uyvy");
			#else
				sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.bmp");
			#endif
			if(access(tmp_key, F_OK))
			{
				g_warning("%s %s not found!!", __FUNCTION__, tmp_key);
				return FALSE;
			}
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "cp -a %s %s/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.uyvy", 
				//					NF_FW_INSTALL_FILE_DIR, NF_FW_INSTALL_FILE_PROG_BG);
				sprintf(tmp_key, "cp -a %s %s/%s", "/NFDVR/data/fwup_bg.uyvy", 
									NF_FW_INSTALL_FILE_DIR, NF_FW_INSTALL_FILE_PROG_BG);
			#else
				sprintf(tmp_key, "cp -a %s %s/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.bmp", 
									NF_FW_INSTALL_FILE_DIR, NF_FW_INSTALL_FILE_PROG_BG);
			#endif
			proxy_system(tmp_key, 1, 3);
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.uyvy");
				sprintf(tmp_key, "%s", "/NFDVR/data/fwup_txt.uyvy");
			#else
				sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.bmp");
			#endif
			if(access(tmp_key, F_OK))
			{
				g_warning("%s %s not found!!", __FUNCTION__, tmp_key);
				return FALSE;
			}
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "cp -a %s %s/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.uyvy", 
				//					NF_FW_INSTALL_FILE_DIR, NF_FW_INSTALL_FILE_PROG_TXT);
				sprintf(tmp_key, "cp -a %s %s/%s", "/NFDVR/data/fwup_txt.uyvy", 
									NF_FW_INSTALL_FILE_DIR, NF_FW_INSTALL_FILE_PROG_TXT);
			#else
				sprintf(tmp_key, "cp -a %s %s/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.bmp", 
									NF_FW_INSTALL_FILE_DIR, NF_FW_INSTALL_FILE_PROG_TXT);
			#endif
			proxy_system(tmp_key, 1, 3);
		// #endif
	}
	else
	{
		/** fw upgrade magic directory create **/
		for( i = 0; i < 4; i++ )
				g_magic_key |= (guint)((random() & 0xff) << (8*i));
		sprintf(tmp_key, "%s/%x", mnt_path, g_magic_key);
		printf("[%s][%d] tmp_key[%s]", __FUNCTION__, __LINE__, tmp_key);
		ret = mkdir(tmp_key, 0755);
		if(ret != 0)
		{
			g_warning("%s %s exists..", __FUNCTION__, tmp_key);
			return FALSE;
		}
		/** fw upgrade magic directory create end **/
		g_message("%s line%d FWUP Debug!! g_magic_key[0x%08x]", __FUNCTION__, __LINE__, g_magic_key);

		/** data file copy **/
		sprintf(tmp_key, "cp -a %s %s/%x/log_000.log", "/NFDVR/log/log_000.log", mnt_path, g_magic_key);
		printf("[%s][%d] tmp_key[%s]", __FUNCTION__, __LINE__, tmp_key);
		proxy_system(tmp_key, 1, 3);
		sprintf(tmp_key, "cp -a %s %s/%x", "/NFDVR/data/nf_sysdb_*.conf", mnt_path, g_magic_key);
		printf("[%s][%d] tmp_key[%s]", __FUNCTION__, __LINE__, tmp_key);
		proxy_system(tmp_key, 1, 3);
		// #if defined(MODEL_DVR)
			/** install file copy **/
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.uyvy");
				sprintf(tmp_key, "%s", "/NFDVR/data/fwup_bg.uyvy");
			#else
				sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.bmp");
			#endif
			if(access(tmp_key, F_OK))
			{
					g_warning("%s %s not found!!", __FUNCTION__, tmp_key);
				return FALSE;
			}
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "cp -a %s %s/%x/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.uyvy", 
				//					mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_BG);
				sprintf(tmp_key, "cp -a %s %s/%x/%s", "/NFDVR/data/fwup_bg.uyvy", 
									mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_BG);
			#else
				sprintf(tmp_key, "cp -a %s %s/%x/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg.bmp", 
									mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_BG);
			#endif
			proxy_system(tmp_key, 1, 3);
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.uyvy");
				sprintf(tmp_key, "%s", "/NFDVR/data/fwup_txt.uyvy");
			#else
				sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.bmp");
			#endif
			if(access(tmp_key, F_OK))
			{
					g_warning("%s %s not found!!", __FUNCTION__, tmp_key);
				return FALSE;
			}
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "cp -a %s %s/%x/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.uyvy", 
				//					mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_TXT);
				sprintf(tmp_key, "cp -a %s %s/%x/%s", "/NFDVR/data/fwup_txt.uyvy", 
									mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_TXT);
				proxy_system(tmp_key, 1, 3);
				//sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg_fail.uyvy");
				sprintf(tmp_key, "%s", "/NFDVR/data/fwup_bg_fail.uyvy");
			#else
				sprintf(tmp_key, "cp -a %s %s/%x/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_txt.bmp", 
									mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_TXT);
				proxy_system(tmp_key, 1, 3);
				sprintf(tmp_key, "%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg_fail.bmp");
			#endif
			if(access(tmp_key, F_OK))
			{
				g_warning("%s %s not found!!", __FUNCTION__, tmp_key);
				return FALSE;
			}
			#if defined(CHIP_NVT)
				//sprintf(tmp_key, "cp -a %s %s/%x/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg_fail.uyvy", 
				//					mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_FAIL);
				sprintf(tmp_key, "cp -a %s %s/%x/%s", "/NFDVR/data/fwup_bg_fail.uyvy", 
									mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_FAIL);
			#else
				sprintf(tmp_key, "cp -a %s %s/%x/%s", NF_FW_INSTALL_FILE_PROG_PATH"/fwup_bg_fail.bmp", 
									mnt_path, g_magic_key, NF_FW_INSTALL_FILE_PROG_FAIL);
			#endif
			proxy_system(tmp_key, 1, 3);
		// #endif

		fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");
		memset(tmp_key, 0x00, sizeof(tmp_key));
		sprintf(tmp_key, "%s/%s.ndb", mnt_path, fw_version);
		if(!nf_sysdb_export(tmp_key))
			return FALSE;
	}

	return TRUE;
}

gboolean nf_fw_write_upgrade_info(gchar *fw_filename)
{
	NF_PARAM_APP app_param;
	gchar *fw_version=NULL;
	gchar tmp_key[256]={0,};
	gchar *curr_lang=NULL;
	gchar *cmp_lang=NULL;
	gint lang_cnt=0, i;
	guint status=0;

	g_message("%s filename[%s]", __FUNCTION__, fw_filename);

	sleep(2); // wait UI status text 

	if( nf_api_param_app_get(&app_param) != TRUE )
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		nf_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_THREAD_CREAT, FW_UPGRADE_PRGT_ERROR, 0, 0);
		return FALSE;
	}
	else
	{
		//init app_param
		app_param.is_pal=0;
		app_param.fwup_magic_key=0;
		app_param.fwup_backup_size=0;
		app_param.fwup_status=0;
		app_param.ipaddr=0;
		app_param.gateway=0;
		app_param.netmask=0;
		app_param.dns1=0;
		app_param.dns2=0;

		memset(app_param.fw_file_name, 0x00, sizeof(app_param.fw_file_name));
		memset(app_param.db_file_name, 0x00, sizeof(app_param.db_file_name));
		memset(app_param.reserved, 0x00, sizeof(app_param.reserved));
	}

	#if defined(CONFIG_FWUPGRADE_NET_DOWN)
		if(nf_fw_chk_version(fw_filename) == FALSE) {

			status=NF_FWUP_STATUS_NET_FAIL;

			goto nf_fw_write_upgrade_info_fail;
		}
	#endif

	sprintf(tmp_key , "sys.info.sig_type");
	app_param.is_pal = nf_sysdb_get_bool(tmp_key);

	#if defined(CONFIG_FWUPGRADE_NET_DOWN)
		g_magic_key=NF_FWUP_MAGIC_KEY_NET;
	#endif

	app_param.fwup_magic_key=g_magic_key;

	#if defined(CONFIG_FWUPGRADE_NET_DOWN)
		strcpy(app_param.fw_file_name, NF_FW_NET_DOWN_FILE_NAME);
	#else
		strcpy(app_param.fw_file_name, fw_filename);
	#endif

	g_message("%s line%d FWUP Debug!! g_magic_key[0x%08x]", __FUNCTION__, __LINE__, g_magic_key);
	if(strstr(fw_filename, "http")) {
		gboolean is_dhcp=0;
		gchar *mac_buff = NULL;
		gchar *model_buff = NULL;

		#if defined(CONFIG_FWUPGRADE_NET_DOWN)		// For Clair Model
			app_param.fwup_status=NF_FWUP_STATUS_UPGRADE_NET_DOWN;
		#else
			app_param.fwup_status=NF_FWUP_STATUS_UPGRADE_NET;
		#endif
		is_dhcp = nf_sysdb_get_bool( "net.proto.dhcpon");
		if(is_dhcp) {
			app_param.ipaddr=0xffffffff;
		}
		else {
			app_param.ipaddr=nf_sysdb_get_uint("net.proto.ipaddr");
			app_param.gateway=nf_sysdb_get_uint("net.proto.gateway");
			app_param.netmask=nf_sysdb_get_uint("net.proto.subnet");
			app_param.dns1 = nf_sysdb_get_uint("net.proto.dns1");
			app_param.dns2 = nf_sysdb_get_uint("net.proto.dns2");
		}
		
		fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");
		memset(tmp_key, 0x00, sizeof(tmp_key));
		sprintf(tmp_key, "%s.ndb", fw_version);
		strncpy(app_param.db_file_name, tmp_key, APP_PARAM_STR_64);

		mac_buff = nf_sysdb_get_str_nocopy("sys.info.mac");
		if(mac_buff == NULL) {
			g_warning("%s sys.info.mac error", __FUNCTION__);

			status=NF_FWUP_STATUS_NET_FAIL;

			goto nf_fw_write_upgrade_info_fail;
		}
		model_buff = nf_sysdb_get_str_nocopy("sys.info.model");
		if(model_buff == NULL) {
			g_warning("%s sys.info.model error", __FUNCTION__);

			status=NF_FWUP_STATUS_NET_FAIL;

			goto nf_fw_write_upgrade_info_fail;
		}
		memset(tmp_key, 0x00, sizeof(tmp_key));
		sprintf(tmp_key, "\"%s;%s\"", mac_buff, model_buff);
		strcpy(app_param.reserved, tmp_key);
	}
	else {
		if(g_magic_key == NF_FWUP_MAGIC_KEY_RAMDISK) {
			gchar *mnt_path=NULL;

			mnt_path = dirname(_nf_fw_filename);
			fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");
			memset(tmp_key, 0x00, sizeof(tmp_key));
			sprintf(tmp_key, "%s/%s.ndb", mnt_path, fw_version);
			if(!nf_sysdb_export(tmp_key)) {
				g_warning("[NF_UTIL_FW_SINGLE][INFO][RAM] sysdb export fail.. key[%s]", tmp_key);
			
				status=NF_FWUP_STATUS_RAM_FAIL;

				goto nf_fw_write_upgrade_info_fail;
			}

			app_param.fwup_status=NF_FWUP_STATUS_UPGRADE_RAM;
		}
		#if defined(CONFIG_FWUPGRADE_NET_DOWN)
		else if(g_magic_key == NF_FWUP_MAGIC_KEY_NET)
		{
			gchar *mnt_path=NULL;
			char tmp[1024]={0, };

			mnt_path = dirname(_nf_fw_filename);
			fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");
			memset(tmp_key, 0x00, sizeof(tmp_key));
			sprintf(tmp_key, "%s/%s.ndb", mnt_path, fw_version);
			if(!nf_sysdb_export(tmp_key)) {
				g_warning("[NF_UTIL_FW_SINGLE][INFO][NET] sysdb export fail.. key[%s]", tmp_key);
			
				status=NF_FWUP_STATUS_NET_FAIL;

				goto nf_fw_write_upgrade_info_fail;
			}

			app_param.fwup_status=NF_FWUP_STATUS_UPGRADE_NET_DOWN;
			strncpy(app_param.db_file_name, tmp_key, APP_PARAM_STR_64);

			sprintf(tmp, "cp -av /NFDVR/log/log_000.log %s/log_000.log", NF_FW_NET_DOWN_DIR_NAME);
			proxy_system(tmp, 1, 3);
		}
		#endif
		else
		{
			app_param.fwup_status=NF_FWUP_STATUS_UPGRADE;
			fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");
			memset(tmp_key, 0x00, sizeof(tmp_key));
			sprintf(tmp_key, "%s.ndb", fw_version);
			strncpy(app_param.db_file_name, tmp_key, APP_PARAM_STR_64);
		}
	}

	#if 1
		g_message("%s FW upgrade status 0x%x", __FUNCTION__, app_param.fwup_status);
		g_message("%s Language index %d", __FUNCTION__, app_param.fwup_backup_size);
		g_message("%s FW file name %s", __FUNCTION__, app_param.fw_file_name);
		g_message("%s DB file name %s", __FUNCTION__, app_param.db_file_name);
	#endif

	if(nf_api_param_app_set_upgrade_val(&app_param, 1) != TRUE) {
		g_warning("%s set app param upgrade value error!!!", __FUNCTION__);

		if(strstr(fw_filename, "http")) {
			status=NF_FWUP_STATUS_NET_FAIL;
		}
		else {
			if(g_magic_key == NF_FWUP_MAGIC_KEY_RAMDISK) {
				status=NF_FWUP_STATUS_RAM_FAIL;
			}
			else if(g_magic_key == NF_FWUP_MAGIC_KEY_NET) {
				status=NF_FWUP_STATUS_NET_FAIL;
			}
			else {
				status=NF_FWUP_STATUS_FAIL;
			}
		}

		goto nf_fw_write_upgrade_info_fail;
	}

	#if 1
		{
			memset(&app_param, 0xff, sizeof(app_param));

			nf_api_param_app_get(&app_param);
			#if 1
				g_message("%s Verify!! FW upgrade status 0x%x", __FUNCTION__, app_param.fwup_status);
				g_message("%s Verify!! Language index %d", __FUNCTION__, app_param.fwup_backup_size);
				g_message("%s Verify!! FW file name %s", __FUNCTION__, app_param.fw_file_name);
				g_message("%s Verify!! DB file name %s", __FUNCTION__, app_param.db_file_name);
				printf("magic key           [0x%08x]\n", app_param.fwup_magic_key);
				printf("fwup_status         [0x%08x]\n", app_param.fwup_status);
				printf("size                [%d]\n", app_param.fwup_backup_size);
				printf("fw_file_name        [%s]\n", app_param.fw_file_name);
				printf("db_file_name        [%s]\n", app_param.db_file_name);
				printf("ipaddr              [0x%08x]\n", app_param.ipaddr);
				printf("gateway             [0x%08x]\n", app_param.gateway);
				printf("netmask             [0x%08x]\n", app_param.netmask);
				printf("dns1                [0x%08x]\n", app_param.dns1);
				printf("dns2                [0x%08x]\n", app_param.dns2);
			#endif
		}
	#endif

	nf_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_UPDATE_FNISH, FW_UPGRADE_PRGT_OK, 0, 0);

	return TRUE;

nf_fw_write_upgrade_info_fail:

	app_param.fwup_status=status;
	if( nf_api_param_app_set_upgrade_val(&app_param, 0) != TRUE)
	{
		g_warning("%s set app param upgrade value error!!!", __FUNCTION__);
		goto nf_fw_write_upgrade_info_fail;
	}

	nf_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_THREAD_CREAT, FW_UPGRADE_PRGT_ERROR, 0, 0);
	return FALSE;
}

#if defined(CONFIG_FWUPGRADE_NET_DOWN)
gboolean nf_fw_chk_version(char *filename)
{
	gchar str[256] = {0,};
	gint ret=0;
	FILE *fp=NULL;
	NF_FW_IMAGE_LIST list;
	gchar *fw_version=NULL;

	umount(NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME);

	ret = mkdir(NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME, 0755);
	if(ret != 0) {
		g_warning("%s %s exists..", __FUNCTION__, NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME);
	}

	sprintf(str, "mount -o loop %s %s", NF_FW_NET_DOWN_FILE_NAME, NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME);
	ret = system(str);
	if (ret) {
		return FALSE;
	}

	if((fp=fopen("/mnt/38100.nbn", "r")) == NULL) {
		g_warning("%s FW File Open Error!!!", __FUNCTION__);
		return FALSE;
	}

	memset(&list, 0x0, sizeof(NF_FW_IMAGE_LIST));
	if(fread(&list, sizeof(NF_FW_IMAGE_LIST), 1, fp) != 1) {
		g_warning("%s fread() error!!! ", __FUNCTION__);
		return FALSE;
	}

	if(strncmp(list.fwheader.magic, FW_UPGRADE_NF_MAGIC_S, sizeof(list.fwheader.magic)) !=0) {
		g_warning("%s Not Match Start Magic. Magic Name [%s]",
					__FUNCTION__, list.fwheader.magic);
		return FALSE;
	}
	else
		g_message("%s NF Firmware Image Start MAGIC Check Success!!!", __FUNCTION__);

	if(!nf_sysman_hotkey_is_nfs()) {
		fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");

		g_message("db [%s] flash [%s]", fw_version, list.fwheader.version);
		if(strncmp(fw_version, list.fwheader.version, 5) != 0) {
			g_warning("%s Not Match F/W Version. fw version [%s]",
					__FUNCTION__, list.fwheader.version);
			return FALSE;
		}
		else {
			g_message("%s Match F/W Version. fw version [%s]",
					__FUNCTION__, list.fwheader.version);
		}
	}

	return TRUE;
}
#endif

#endif

