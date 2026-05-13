#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>	//for mmap
#include <fcntl.h>		//for open
#include <unistd.h>		//for lseek
#include <stdio.h>
#include <dirent.h>		//for opendir
#include <sys/mount.h>

#include "nf_common.h"
#include "nf_util_fw.h"
#include "nf_util_fw_hdd.h"
#include "nf_api_disk.h"

static int _get_hdd_dev( char *fw_path, int fw_path_len);


/*************************************** Network firmware upgrade via hdd ***************************************/
/**
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
void nf_fw_network_upgrade_hdd_mount_state(int _mode, int *_val)
{
	static int is_mounted = -1;

	if(_mode == 0)
	{
		is_mounted = *_val;
	}
	else if(_mode == 1)
	{
		*_val = is_mounted;
	}

	g_message("%s line%d mode[%d] val[%d] is_mounted[%d]", __FUNCTION__, __LINE__, _mode, *_val, is_mounted);
}

gboolean nf_fw_network_upgrade_hdd_mount(void)
{
	int ret = 0;
	char str_fw_path[256] = {0, }, str_cmd[256] = {0, };
	int length = 0;
   
	length = sizeof(str_fw_path) / sizeof(str_fw_path[0]);
   
	#if defined DEBUG_FWUP
			printf("\033[0;35m %s [DEBUG_FWUP] str_fw_path length [%d]\033[0;39m\n", __FUNCTION__, length);
	#endif
	
	if(0 == _get_hdd_dev(str_fw_path, length))
	{
		printf("%s Not found HDD", __FUNCTION__);
		return 0;
	}
   
	ret = mkdir(NET_FWUP_MOUNT_DIR_NAME, 0755);
	if(ret != 0)
		printf("%s %s exists.\n", __FUNCTION__,NET_FWUP_MOUNT_DIR_NAME);
   
	umount(NET_FWUP_MOUNT_DIR_NAME);

	if((ret = mount(str_fw_path, NET_FWUP_MOUNT_DIR_NAME, NET_FWUP_MOUNT_TYPE , 0, 0)) !=0)
	{
		printf("%s mount error. ret[%d] [%s]->[%s] start format", __FUNCTION__, ret, str_fw_path, NET_FWUP_MOUNT_DIR_NAME);

		if(EBUSY == ret)
		{
			printf("%s already mounted. ret[%d] [%s]->[%s]", __FUNCTION__, ret, str_fw_path, NET_FWUP_MOUNT_DIR_NAME);
			return 1;
		}
   
		snprintf(str_cmd,sizeof(str_cmd) / sizeof(str_cmd[0]),"mkfs.ext4 %s", str_fw_path);
		#if defined DEBUG_FWUP
			printf("\033[0;35m %s [DEBUG_FWUP] str_cmd [%s]\033[0;39m\n", __FUNCTION__, str_cmd);
		#endif
		proxy_system(str_cmd, 1, 5);

		if((ret = mount(str_fw_path, NET_FWUP_MOUNT_DIR_NAME, NET_FWUP_MOUNT_TYPE , 0, 0)) !=0)
		{
			printf("%s mount error. ret[%d] [%s]->[%s]", __FUNCTION__, ret, str_fw_path, NET_FWUP_MOUNT_DIR_NAME);
			return 0;
		}
	}
	else
	{
		printf("%s mounted [%s]->[%s]", __FUNCTION__, str_fw_path, NET_FWUP_MOUNT_DIR_NAME);
		DIR *dir;
		struct dirent *entry;
		char file_path[256];

		dir = opendir(NET_FWUP_MOUNT_DIR_NAME);
		if (dir == NULL) {
			printf("%s Failed to open directory %s\n", __FUNCTION__, NET_FWUP_MOUNT_DIR_NAME);
			return 1;
		}

		// Iterate through directory entries and delete non-log files
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_type == DT_REG) {
				if (strstr(entry->d_name, ".log") == NULL) {
					snprintf(file_path, sizeof(file_path), "%s/%s", NET_FWUP_MOUNT_DIR_NAME, entry->d_name);
					if (remove(file_path) == 0) {
						printf("%s Deleted file: %s\n", __FUNCTION__, file_path);
					} else {
						printf("%s Failed to delete file: %s\n", __FUNCTION__, file_path);
					}
				}
			}
		}

		closedir(dir);
		return 1;
	}

	return 1;

}

gboolean nf_fw_network_upgrade_hdd_init(char* fw_path, int fw_path_len)
{
	int ret = 0;
	NF_DISK_INFO tmp_disk_info;
	int result = 0;
	
	if(fw_path == NULL || fw_path_len <= 0)
	{
		printf("%s fw_path is NULL. fw_path_len[%d]", __FUNCTION__, fw_path_len);
	}
	#if 0
	#if defined DEBUG_FWUP
		printf("\033[0;35m %s [DEBUG_FWUP] str_fw_path length [%d]\033[0;39m\n", __FUNCTION__, length);
	#endif
	if(0 == _get_hdd_dev(str_fw_path, length))
	{
		printf("%s Not found HDD", __FUNCTION__);
		return 0;
	}
	
	snprintf(str_cmd,sizeof(str_cmd) / sizeof(str_cmd[0]),"mkfs.vfat %s", str_fw_path);
	#if defined DEBUG_FWUP
		printf("\033[0;35m %s [DEBUG_FWUP] str_cmd [%s]\033[0;39m\n", __FUNCTION__, str_cmd);
	#endif
	proxy_system(str_cmd, 1, 5);
	
	ret = mkdir(NET_FWUP_MOUNT_DIR_NAME, 0755);
	if(ret != 0)
		printf("%s %s exists.\n", __FUNCTION__,NET_FWUP_MOUNT_DIR_NAME);
		
	umount(NET_FWUP_MOUNT_DIR_NAME);

	if((ret = mount(str_fw_path, NET_FWUP_MOUNT_DIR_NAME, NET_FWUP_MOUNT_TYPE , 0, 0)) !=0)
	{
		printf("%s mount error. ret[%d] [%s]->[%s]", __FUNCTION__, ret, str_fw_path, NET_FWUP_MOUNT_DIR_NAME);
		return 0;
	}
	else
		printf("%s mounted [%s]->[%s]", __FUNCTION__, str_fw_path, NET_FWUP_MOUNT_DIR_NAME);
	#else
		proxy_system("rm *.nbn", 1, 5);
		proxy_system("rm lighttpd-upload-*", 1, 5);
	#endif
	if(strlen(NET_FWUP_MOUNT_DIR_NAME) > fw_path_len)
	{
		printf("%s The array size is too small", __FUNCTION__);
		return ret;
	}
	else
		strncpy(fw_path, NET_FWUP_MOUNT_DIR_NAME, strlen(NET_FWUP_MOUNT_DIR_NAME));

	nf_fw_network_upgrade_hdd_mount_state(1,&ret);

	if (ret == 0) {
		if (access(NET_FWUP_MOUNT_DIR_NAME, F_OK) == 0) {
			result = sst_disk_get_info((struct disk_info_t *)&tmp_disk_info);
			if (result != -8) {
				ret = 1;
			}
		}
	}

	return ret;
}

static int _get_hdd_dev( char *fw_path, int fw_path_len)
{
	int idx = 0,ret;
	char tmp[1024];
	char *hdd_str = NULL;
	for(idx = 'a'; idx <= 'z'; idx++)
	{
		snprintf( fw_path, fw_path_len, "/sys/block/sd%c", idx);
		memset( tmp, 0x00, sizeof(tmp));
		ret = readlink( fw_path, tmp, sizeof(tmp));
		if(ret>0) {
			hdd_str = strstr(tmp ,"ata");
			printf("\033[0;31m %s hdd_str [%s] \033[0;39m\n", __FUNCTION__, hdd_str);

			if(hdd_str) {
				snprintf( fw_path, fw_path_len, "/dev/sd%c2", idx);
				printf("\033[0;31m %s fw_path [%s] \033[0;39m\n", __FUNCTION__, fw_path);
				return 1;
			}
		}
	}
	memset( fw_path, 0x00, fw_path_len);
	return 0;
}

#if 0
/*************************************** upgrade for hdd ***************************************/
/**
	@brief                          upgrade for hdd
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
#define FW_UPGRADE_HDD_MAX_BUF              256
#define FW_UPGRADE_HDD_READ_LEN             2048    
#define FW_UPGRADE_HDD_FILE_NAME            "/nf_firmware_0.0.0.tar"
#define FW_UPGRADE_HDD_VERIFY_FILE          "/nf_firmware_0.0.0.chk"
#define FW_UPGRADE_HDD_DEV_NAME             "/dev/sd%c1"
#define FW_UPGRADE_HDD_MOUNT_DIR_NAME       "/mnt/usb_fwup"
#define FW_UPGRADE_HDD_MOUNT_FS_TYPE        "vfat"
gboolean nf_util_fw_start_upgrade_hdd()
{
	gint ret=0;
	guint src_filelen=0, dest_filelen=0;
	
	gchar dev_name[FW_UPGRADE_HDD_MAX_BUF];
	gchar fw_filename[FW_UPGRADE_HDD_MAX_BUF];
		
	FILE *fp=NULL;
	struct stat f_stat;
	int i; 
	
	ret = mkdir(FW_UPGRADE_HDD_MOUNT_DIR_NAME, 0755);
	if(ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, FW_UPGRADE_HDD_MOUNT_DIR_NAME);
		
	umount(FW_UPGRADE_HDD_MOUNT_DIR_NAME);
	
	for(i='a';i<='z';++i)
	{   
		sprintf(dev_name, FW_UPGRADE_HDD_DEV_NAME, i);
		if((fp=fopen(dev_name, "r")) == NULL)
		{
			g_warning("%s Device Open Error!!! USB Device Name [%s]",
					__FUNCTION__, dev_name);
		}
		else {
			g_message("%s fw_filename : [%s]", __FUNCTION__, dev_name);
			break;
		}
	}
   
	g_return_val_if_fail(fp != NULL, 0);
	fclose(fp);
   
	if((ret = mount(dev_name, FW_UPGRADE_HDD_MOUNT_DIR_NAME,
							FW_UPGRADE_HDD_MOUNT_FS_TYPE , 0, 0)) !=0)
	{
		g_warning("%s mount error!!! ret [%d]", __FUNCTION__, ret);
		return FALSE;
	}else{
		g_message("%s mounted [%s]->[%s]",__FUNCTION__,
					dev_name, FW_UPGRADE_HDD_MOUNT_DIR_NAME);
	}

   
	snprintf(fw_filename, sizeof(fw_filename), "%s/%s",
				FW_UPGRADE_HDD_MOUNT_DIR_NAME,
				FW_UPGRADE_HDD_FILE_NAME);

	if(stat(fw_filename, &f_stat) == -1)
	{
		g_warning("%s Cannot find src fw_filename [%s]", __FUNCTION__, fw_filename);
		ret = FALSE; goto ret_func;
	}
	src_filelen = (guint)f_stat.st_size;
	g_message("%s fwfile[%s] len[%d]",__FUNCTION__, fw_filename, src_filelen);

	//file copy
	if(nf_fw_copy_from_dev_to_hdd(fw_filename) != TRUE)
	{
		g_warning("%s Upgrade Fail!!!", __FUNCTION__);
		ret = FALSE; goto ret_func;
	}

	if(stat(FW_UPGRADE_HDD_FILE_NAME, &f_stat) == -1)
	{
		g_warning("%s Cannot find dest fw_filename[%s]", __FUNCTION__, fw_filename);
		ret = FALSE; goto ret_func;
	}

	dest_filelen = (guint)f_stat.st_size;

	//file length compare
	if(src_filelen != dest_filelen){
		g_warning("%s file length is different.. src[%d] dest[%d]",
					__FUNCTION__, src_filelen, dest_filelen);
		ret = FALSE; goto ret_func;
	}

	//verify file create.....   
	if((fp = fopen(FW_UPGRADE_HDD_VERIFY_FILE, "w")) == NULL)
	{
		g_warning("%s fw_filename[%s] create error!!!", __FUNCTION__,
						 FW_UPGRADE_HDD_VERIFY_FILE);

		ret = FALSE; goto ret_func;

	}else{
		fprintf(fp, "firmware file copy complete!!!\n");
	}

	fclose(fp);
	sync();

	ret = TRUE;

ret_func:

	if(umount(FW_UPGRADE_HDD_MOUNT_DIR_NAME) != 0)
		g_warning("%s finish umount() error!!!", __FUNCTION__);
	else
		g_message("%s umounted[%s]",__FUNCTION__, FW_UPGRADE_HDD_MOUNT_DIR_NAME);

	return ret;

}

/**
	@brief                          upgrade for hdd..   copy function from dev to hdd
	@param[in]  src                 source filename
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_copy_from_dev_to_hdd(gchar *src)
{
	FILE *src_fp=NULL, *dest_fp=NULL;
	gchar buf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	struct stat f_stat;
	guint src_filelen=0;
	gboolean ret =0;
	guint i, block_cnt=0,  extra_byte=0;

	if(stat(src, &f_stat) == -1)
	{
		g_warning("%s can't stat [%s]", __FUNCTION__, src);
		ret= FALSE; goto ret_func;
	}

	src_filelen = (guint)f_stat.st_size;

	if((src_fp=fopen(src, "r")) == NULL)
	{
		g_warning("%s Source File Open Error!!! [%s]",
					__FUNCTION__, src);

		ret= FALSE; goto ret_func;
	}

	if((dest_fp=fopen(FW_UPGRADE_HDD_FILE_NAME, "w")) == NULL)
	{
		g_warning("%s Destination File Open Error!!! [%s]",
					__FUNCTION__, FW_UPGRADE_HDD_FILE_NAME);                        //hear must modify filename
		ret= FALSE; goto ret_func;
	}

	nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
	nf_record_stop(NULL);

#if 1
	nf_dev_buzzer_on();
	g_usleep(500000*1);
	nf_dev_buzzer_off();
	g_usleep(500000*1);
	nf_dev_buzzer_on();
	g_usleep(500000*1);
	nf_dev_buzzer_off();
#endif

	block_cnt = src_filelen / FW_UPGRADE_HDD_READ_LEN;
	extra_byte = src_filelen % FW_UPGRADE_HDD_READ_LEN;

	g_message("%s filelen[%d] block_cnt[%d] extra_byte[%d]", __FUNCTION__,
					src_filelen, block_cnt, extra_byte);

	for(i=0;i<block_cnt;++i)
	{
		if(fread(buf, FW_UPGRADE_HDD_READ_LEN, 1, src_fp) != 1)
		{
			g_warning("%s fread() error!!!", __FUNCTION__);
			ret= FALSE; goto ret_func;

		}

		if(fwrite(buf, FW_UPGRADE_HDD_READ_LEN, 1, dest_fp) != 1)
		{
			g_warning("%s fwrite() error!!! ", __FUNCTION__);
			ret= FALSE; goto ret_func;
		}

		if((i % NAND_LARGE_BLOCK_SIZE_OOB) == 0)
			g_message("%s block_cnt[%d/%d]", __FUNCTION__, i, block_cnt);

		sync();
	}

	if(extra_byte >0)
	{
		if(fread(buf, extra_byte, 1, src_fp) != 1)
		{
			g_warning("%s fread() error!!!", __FUNCTION__);
			ret= FALSE; goto ret_func;
		}

		if(fwrite(buf, extra_byte, 1, dest_fp) != 1)
		{
			g_warning("%s fwrite() error!!! ", __FUNCTION__);
			ret= FALSE; goto ret_func;
		}
	}

	g_message("%s Copy Complete...",__FUNCTION__);
	sync();

#if 1
	nf_dev_buzzer_on();
	g_usleep(500000*2);
	nf_dev_buzzer_off();
	g_usleep(500000*1);
	nf_dev_buzzer_on();
	g_usleep(500000*1);
	nf_dev_buzzer_off();
#endif
	
ret_func:
	if( src_fp )
			fclose(src_fp);
	
	if( dest_fp )
			fclose(dest_fp);
	
	return TRUE;
}   


#ifdef DEBUG_JBSHELL_FLASH
static char fw_upgrade_hdd_help [] = "fw_upgrade_hdd";
static int fw_upgrade_hdd(int argc, char **argv)
{
	if ( argc < 1 ) {
		printf("Invalid arguments\n%s\n", fw_upgrade_hdd_help);
		return -1;
	}   
	
	nf_util_fw_start_upgrade_hdd();
	
	return 0;
}   
__commandlist(fw_upgrade_hdd, "fw_upgrade_hdd", "fw_upgrade_hdd", fw_upgrade_hdd_help);
#endif

#endif

