#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>	//for mmap
#include <fcntl.h>		//for open
#include <unistd.h>		//for lseek
#include <stdio.h>
#include <dirent.h>		//for opendir
#include <sys/mount.h>
#include <libgen.h>

#include "nf_util_fw.h"
#if defined(CONFIG_FWUPGRADE_SINGLE)
    #include "nf_util_fw_single.h"
#endif
#include "nf_util_fw_utils.h"
#include "nf_util_fw_crc.h"
#include "nf_record.h"
#include "nf_network.h"
#include "nf_util_device.h"
#include "nf_sysman.h"
#include "nf_api_eventlog.h"
#include "nf_api_param_app.h"
#include "nf_api_param_fwver.h"
#include "nf_watchdog.h"

#define FW_UPGRADE_LIST_SIZE				sizeof(NF_FW_IMAGE_LIST)
#define FW_UPGRADE_IMG_HEADER_SIZE			sizeof(image_header_t)
//#define DEBUG_FW_UPGRADE_PRGT
#define DEBUG_FWUP
#define NF_FW_TURNING_CRC_CHECK				// 20121113
#define NF_FW_UPGRADE_LED
#define DEBUG_JBSHELL_FLASH

#ifdef DEBUG_JBSHELL_FLASH
	#include "jbshell.h"
#endif

/*	function declare	*/
static void nf_fw_upgrade_thread_func();
static void _fw_set_prgt_state(gshort type, short state, gint is_error, guint current, guint total);
static int _get_hdd_dev( char *fw_path, int fw_path_len);

/** extern function definition **/
extern int proxy_system(const char *str, int mode, int timeout_sec);

/** gloval variable definition **/
gchar _nf_fw_filename[FW_UPGRADE_MAX_BUFFER];
GThread *_fw_thread;
NF_FW_PRGT _prgt;
#ifdef NF_FW_UPGRADE_LED
guint _led_cnt;
#endif

/**
	@brief                          firmware running state check
	@param[in]  state     			firmware running state
*/
void nf_fw_state_check(NF_FW_PRGT *ui_prgt)
{
	g_return_if_fail(ui_prgt != NULL);
	
	#if defined(DEBUG_FW_UPGRADE_PRGT)
		g_print("%s _prgt.type       [%02x]\n", __FUNCTION__, _prgt.type);
		g_print("%s _prgt.state      [%02x]\n",__FUNCTION__, _prgt.state);
		g_print("%s _prgt.is_error   [%d]\n", __FUNCTION__, _prgt.is_error);
		g_print("%s _prgt.current    [%d]\n", __FUNCTION__, _prgt.current);
		g_print("%s _prgt.total      [%d]\n", __FUNCTION__, _prgt.total);
	#endif
			
	memcpy(ui_prgt, &_prgt, sizeof(NF_FW_PRGT));	
}

/**
	@brief                          firmware thread function
*/
static void nf_fw_upgrade_thread_func()
{
	#if defined(CONFIG_FWUPGRADE_SINGLE)
		gchar *fw_filename;

		#if defined DEBUG_FWUP
			printf("\033[0;35m %s  [IMSI] CONFIG_FWUPGRADE_SINGLE \033[0;39m\n", __FUNCTION__);
		#endif

		if (strstr(_nf_fw_filename, "http")) {
			fw_filename = _nf_fw_filename;
		} else if(strstr(_nf_fw_filename, NET_FWUP_MOUNT_DIR_NAME)) {
			fw_filename = _nf_fw_filename;
		} else {
			fw_filename = basename(_nf_fw_filename);
		}

		if (fw_filename != NULL) {
			nf_fw_write_upgrade_info(fw_filename);
		} else {
			g_warning("%s FW File name dose not exist!!!", __FUNCTION__);
			_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_THREAD_CREAT, FW_UPGRADE_PRGT_ERROR, 0, 0);
		}
	#else
		nf_fw_start_upgrade(_nf_fw_filename);
	#endif
}

/**
	@brief                          thread function start for FW Upgrade
	@param[in]  fw_filename     	firmware file name    
	@param[in]  is_nonblocking		1 is nonblocking mode, 0 is blocking mode
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_upgrade_thread_start(gchar *fw_filename, gboolean is_nonblocking)
{
	gboolean ret=0;

	g_return_val_if_fail(fw_filename != NULL, 0);

	strncpy(_nf_fw_filename, fw_filename, sizeof(_nf_fw_filename));

	#if defined DEBUG_FWUP
	printf("\033[0;35m %s  [IMSI] fw_filename [%s] \033[0;39m\n", __FUNCTION__,_nf_fw_filename);
	#endif

	g_message("%s Firmware Filename ==> [%s]", __FUNCTION__, _nf_fw_filename);

	#if defined(ENABLE_NABTO)
	backup_nabto();
	#endif
	memset(&_prgt, 0x0, sizeof(NF_FW_PRGT));

	_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_START, FW_UPGRADE_PRGT_OK, 0, 0);
	nf_fw_update_check_clear();
	
	if(is_nonblocking)
	{
		g_message("%s Upgrade is Non-Blocking Mode.", __FUNCTION__);

		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_THREAD_CREAT, FW_UPGRADE_PRGT_OK, 0, 0);

		_fw_thread = g_thread_create((GThreadFunc)nf_fw_upgrade_thread_func, NULL, FALSE, NULL);

		if(_fw_thread == NULL)
		{
			g_warning("%s FW Upgrade Thread Create Fail!!!", __FUNCTION__);
			_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_THREAD_CREAT, FW_UPGRADE_PRGT_ERROR, 0, 0);
			return FALSE;
		}
	}
	else
	{
		g_message("%s Upgrade is Blocking Mode.", __FUNCTION__);
		ret = nf_fw_start_upgrade(_nf_fw_filename);
		if(ret == FALSE)
		{
			_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_START, FW_UPGRADE_PRGT_ERROR, 0, 0);
			g_warning("%s Upgrade Fail!!!", __FUNCTION__);
			return FALSE;
		}
	}

	return TRUE;
}

/**
	@brief                          firmware upgrade
	@param[in]  fw_filename			firmware filename
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_start_upgrade(gchar *fw_filename)
{
	FILE *fp=NULL;
	glong data_len=0, crc_check_len=0, offset=0, next_offset=0;
	glong offset_uboot=0, data_len_uboot=0;
	gint i=0, autoplace=0, writeoob=0, ret=0, is_first=0, img_type=0;
	gboolean is_ping = FALSE, is_uboot_up=FALSE;
	NF_FW_IMAGE_LIST file_imglist, nand_imglist;
	struct stat f_stat;
	guint *ptr=NULL, ping_seq=0, pong_seq=0, checksum=0;
	char mtddev[NAND_LARGE_BLOCK_SIZE_OOB] = {0,};
	gchar end_magic_buf[FW_UPGRADE_MAX_MAGIC_BUF]={0,};
	NF_FW_IMAGE_LIST_TABLE img_list_table[IMAGE_TABLE_MAX_NUM];

	g_return_val_if_fail(fw_filename != NULL, 0);

	/* sequence number check */
	_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_SEQ, FW_UPGRADE_PRGT_OK, 0, 0);
	ret = nf_fw_get_seq(&ping_seq, &pong_seq, &is_first);
	if(ret == -1)
	{
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_SEQ, FW_UPGRADE_PRGT_ERROR, 0, 0);
		g_warning("%s Sequence read error!!!\n", __FUNCTION__);
		return FALSE;
	}	

	if(ret == FW_UPGRADE_PING)
		g_message("%s ******* Upgarade part is ping *******", __FUNCTION__);
	else
		g_message("%s ******* Upgarade part is pong *******", __FUNCTION__);

	_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_OPEN, FW_UPGRADE_PRGT_OK, 0, 0);
	if(stat(fw_filename, &f_stat) == -1)
	{
		g_warning("%s Cannot Find Filename [%s]", __FUNCTION__, fw_filename);
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_OPEN, FW_UPGRADE_PRGT_ERROR, 0, 0);
		return FALSE;
	}

	if((fp=fopen(fw_filename, "r")) == NULL)
	{
		g_warning("%s File Open Error!!! Image Name [%s]", 
					__FUNCTION__, fw_filename);
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_OPEN, FW_UPGRADE_PRGT_ERROR, 0, 0);
		return FALSE;
	}
	
	fseek(fp, f_stat.st_size - (glong)sizeof(img_list_table), SEEK_CUR);
	if(fread(&img_list_table, sizeof(img_list_table), 1, fp) != 1)		//read tail size
	{
		g_warning("%s tail fread() error!!! ", __FUNCTION__);
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_READ, FW_UPGRADE_PRGT_ERROR, 0, 0);
		goto nf_fw_upgrade_fail;
	}

	fseek(fp, 0, SEEK_SET);		// return to 0 file pointer
	
	_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_READ, FW_UPGRADE_PRGT_OK, 0, 0);
	memset(&file_imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	if(fread(&file_imglist, FW_UPGRADE_LIST_SIZE, 1, fp) != 1)			//move fp -> FW_UPGRADE_LIST_SIZE == cur fp ==>uboot
	{
		g_warning("%s fread() error!!! ", __FUNCTION__);
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_READ, FW_UPGRADE_PRGT_ERROR, 0, 0);
		goto nf_fw_upgrade_fail;
	}

	/************************* Start Magic Check ***************************/
	_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_S_MAGIC_CHECK, FW_UPGRADE_PRGT_OK, 0, 0);
	if(strncmp(file_imglist.fwheader.magic, FW_UPGRADE_NF_MAGIC_S, sizeof(file_imglist.fwheader.magic)) !=0)
	{
		g_warning("%s Not Match Start Magic. Magic Name [%s]",
					__FUNCTION__, file_imglist.fwheader.magic);
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_S_MAGIC_CHECK, FW_UPGRADE_PRGT_ERROR, 0, 0);
		goto nf_fw_upgrade_fail;
	}
	else
		g_message("%s NF Firmware Image Start MAGIC Check Success!!!", __FUNCTION__);

	// 20120925 Added by pakkhman
	/************************* F/W Version Check ***************************/
	{
		gchar *fw_version=NULL;
		if(!nf_sysman_hotkey_is_nfs())
		{
			fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");

			if(strncmp(fw_version, file_imglist.fwheader.version, 5) != 0)
			{
				g_warning("%s Not Match F/W Version. fw version [%s]",
						__FUNCTION__, file_imglist.fwheader.version);
				fclose(fp);
				_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_S_MAGIC_CHECK, FW_UPGRADE_PRGT_ERROR, 0, 0);
				return FALSE;
			}
		}
	}

	g_warning("%s ===== Watchdog Disable!! =====", __FUNCTION__);
	nf_watchdog_ctrl(NF_WATCHDOG_MEMBER_MAIN_TIMER, NF_WATCHDOG_TIME_SEC  , 0 );
	nf_watchdog_ctrl(NF_WATCHDOG_MEMBER_ACTION, NF_WATCHDOG_TIME_SEC  , 0 );
	nf_watchdog_ctrl(NF_WATCHDOG_MEMBER_EVENT, NF_WATCHDOG_TIME_SEC  , 0 );
	nf_watchdog_ctrl(NF_WATCHDOG_MEMBER_NOTIFY, NF_WATCHDOG_TIME_SEC  , 0 );
	

	g_message("%s ===== Starting to Write to Nand !!! =====", __FUNCTION__);

	if((ping_seq < pong_seq) || (ping_seq == pong_seq))
	{
		is_ping = FW_UPGRADE_PING;
		g_message("%s Write is Ping", __FUNCTION__);
	}
	else
	{
		is_ping = FW_UPGRADE_PONG;
		g_message("%s Write is Pong", __FUNCTION__);
	}
	
	/************************* FW File Write ***************************/
	g_message("%s tot sec_cnt [%d]\n", __FUNCTION__, file_imglist.fwheader.sec_cnt);
	for(i=0; i<(gint)file_imglist.fwheader.sec_cnt; i++)
	{
		gint mtd_num=0, img_type=0, is_header_write=0;
		guint crc_check_len=0;
		gshort prgt_type=0;
		
		if(ping_seq < pong_seq)
			mtd_num = img_list_table[i].ping_mtd_num;
		else
			mtd_num = img_list_table[i].pong_mtd_num;
		
		if((mtd_num == FW_UPGRADE_UBL_MTD_NUM) || (mtd_num > FW_UPGRADE_MAX_MTD_NUM))
		{
			g_warning("%s invalid mtd num [%d]\n", __FUNCTION__, mtd_num);
			_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_WRITE, FW_UPGRADE_PRGT_ERROR, 0, 0);
			goto nf_fw_upgrade_fail;
		}

		if(img_list_table[i].img_type == NF_FW_TYPE_UBOOT)
			offset_uboot=ftell(fp);

		data_len = (glong)img_list_table[i].data_len;
		autoplace = (glong)img_list_table[i].autoplace;
		writeoob = 	(glong)img_list_table[i].writeoob;
		
		if(img_list_table[i].img_type > FW_UPGRADE_MAX_IMG_TYPE_NUM)
		{
			g_warning("%s invalid img_type [%d]\n", __FUNCTION__, img_list_table[i].img_type);
			_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_WRITE, FW_UPGRADE_PRGT_ERROR, 0, 0);
			goto nf_fw_upgrade_fail;
		}

		prgt_type = (gshort)(img_list_table[i].img_type + 1);		/* +1 means " not match tool type" */

		if(!img_list_table[i].is_header_write)
			fseek(fp, FW_UPGRADE_IMG_HEADER_SIZE, SEEK_CUR);
		
		if((nf_flash_erase(mtd_num, NULL, NULL)) != TRUE)	/* first mtd erase */
		{
			_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_ERASE, FW_UPGRADE_PRGT_ERROR, 0, 0);
			g_warning("%s Nand erase error!!! mtd num [%d]", __FUNCTION__, mtd_num);
			goto nf_fw_upgrade_fail;
		}
		
		_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_WRITE, FW_UPGRADE_PRGT_OK, 0, 0);
		//write data to nand flash
		g_message("%s Write FW file ===== %s=====", __FUNCTION__, file_imglist.fwheader.sec_header[i].ih_name);
#if !defined(SUPPORT_UBIFS)
		if(img_list_table[i].img_type == NF_FW_TYPE_FILESYS)
		{
			g_message("Filesystem Yaffs Format Write!!!!!!!!!!!!!!!!!!!!!");
			if((nf_flash_fw_yaffs_write(mtd_num, fp, data_len, autoplace, writeoob, prog_cb_func, NULL)) != TRUE)
			{
				g_warning("%s [%s] image write error!!!", 
							__FUNCTION__, file_imglist.fwheader.sec_header[i].ih_name);
				g_warning("%s Firmware Upgrade Fail!!!", __FUNCTION__);

				_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_WRITE, FW_UPGRADE_PRGT_ERROR, 0, 0);
				goto nf_fw_upgrade_fail;
			}
		}
		else
#endif
		{
			if((nf_flash_fw_write(mtd_num, fp, data_len, autoplace, writeoob, prog_cb_func, NULL)) != TRUE)
			{
				g_warning("%s [%s] Image Write Error!!!", 
							__FUNCTION__, file_imglist.fwheader.sec_header[i].ih_name);
				g_warning("%s Firmware Upgrade Fail!!!", __FUNCTION__);
			
				_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_WRITE, FW_UPGRADE_PRGT_ERROR, 0, 0);
				goto nf_fw_upgrade_fail;
			}
		}

		checksum = img_list_table[i].checksum;
		img_type = img_list_table[i].img_type;
		if(img_type == NF_FW_TYPE_UBOOT)			/* uboot upgrade check */
			is_uboot_up = TRUE;
		crc_check_len = img_list_table[i].crc_check_len;
		is_header_write = img_list_table[i].is_header_write;
#if 1
		if(nf_fw_crc_check(mtd_num, checksum, crc_check_len, img_type, prgt_type, is_header_write) != TRUE)		/* CRC Check */
		{
			g_warning("%s %s CRC Check Error!!!", __FUNCTION__,
						file_imglist.fwheader.sec_header[i].ih_name);

			_fw_set_prgt_state(prgt_type, NF_FW_PRGT_IMG_CRC_CHECK, FW_UPGRADE_PRGT_ERROR, 0, 0);
			goto nf_fw_upgrade_fail;
		}
		else
			g_message("%s ===== %s  CRC Check was passed!!! =====", __FUNCTION__,
						file_imglist.fwheader.sec_header[i].ih_name);
#endif
	}
	_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_START, FW_UPGRADE_PRGT_OK, 0, 0);
	
	/************************* END Magic Check ***************************/
	/* last.. end magic check */
	offset = ftell(fp);
	
	_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_E_MAGIC_CHECK, FW_UPGRADE_PRGT_OK, 0, 0);

	if(fread(end_magic_buf, FW_UPGRADE_MAX_MAGIC_BUF, 1, fp) != 1)      //read tail size
	{
		g_warning("%s End Magic Buf fread() error!!! ", __FUNCTION__);
		goto nf_fw_upgrade_fail;
	}
	
	if(strncmp(end_magic_buf, FW_UPGRADE_NF_MAGIC_E, FW_UPGRADE_MAX_MAGIC_BUF))
	{
		g_warning("%s End Magic Different!!!", __FUNCTION__);
		goto nf_fw_upgrade_fail;
	}
	else
		g_message("%s ====== End Magic Check Passed ======", __FUNCTION__);

	g_message("%s ===== Finished to Write to Nand !!! =====", __FUNCTION__);
	
	/************************* Finish Check ***************************/
	/*
	   FW_UPGRADE_LIST_SIZE copy..
	*/
	memset(&nand_imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&nand_imglist, &file_imglist, FW_UPGRADE_LIST_SIZE);	

	/*
		To Do ==> seqence number update
	*/
	_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_SEQ_UPDATE, FW_UPGRADE_PRGT_OK, 0, 0);
	g_message("%s ====== Sequence Number Update ======", __FUNCTION__);
	ret = nf_fw_set_seq(&ping_seq, &pong_seq, is_first, &nand_imglist);
	if(!ret)
	{
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_SEQ_UPDATE, FW_UPGRADE_PRGT_ERROR, 0, 0);
		goto nf_fw_upgrade_fail;
	}

	/*
		 Setting Update variable 
	*/
	if(is_uboot_up)
	{
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_SET_UPDATE_VARIABLE, FW_UPGRADE_PRGT_OK, 0, 0);
		if(	nf_fw_update_uboot(fp, offset_uboot, img_list_table) != TRUE)
		{
			_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_SET_UPDATE_VARIABLE, FW_UPGRADE_PRGT_ERROR, 0, 0);
			g_warning("%s Updata Upgrade Fail!!!", __FUNCTION__);
			goto nf_fw_upgrade_fail;
		}
		#if 0
			if(	nf_fw_set_update_variable() != TRUE)
			{
				_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_SET_UPDATE_VARIABLE, FW_UPGRADE_PRGT_ERROR, 0, 0);
				g_warning("%s Updata Variable Setting Fail!!!", __FUNCTION__);
				goto nf_fw_upgrade_fail;
			}
		#endif
	}

	/*
		Setting App Param
	*/
	if( nf_fw_set_app_param() != TRUE)
	{
		/* share update variable */
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_SET_UPDATE_VARIABLE, FW_UPGRADE_PRGT_ERROR, 0, 0);
		g_warning("%s App Param Setting Error!!!", __FUNCTION__);
		goto nf_fw_upgrade_fail;
	}


	_fw_set_prgt_state(NF_FW_PRGT_TYPE_FINISH, NF_FW_PRGT_FNI_UPDATE_FNISH, FW_UPGRADE_PRGT_OK, 0, 0);
	g_message("%s ====== Sequence Number Update Finish ======", __FUNCTION__);
	g_message("%s ======> Firmware Upgrade Finish <======", __FUNCTION__);
	
	fclose(fp);
	return TRUE;

nf_fw_upgrade_fail:
	fclose(fp);
	return FALSE;
}

gboolean nf_fw_set_app_param(void)
{
	NF_PARAM_APP app_param;
	gchar tmp_key[256]={0,};

	g_message("%s called", __FUNCTION__);

	if( nf_api_param_app_get(&app_param) != TRUE )
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}

	sprintf(tmp_key , "sys.info.sig_type");
	app_param.is_pal = nf_sysdb_get_bool(tmp_key);

	if( nf_api_param_app_set_upgrade_val(&app_param, 1) != TRUE)
	{
		g_warning("%s set app param upgrade value error!!!", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

/** 
    @brief							Update Uboot
    @return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_update_uboot(FILE *fp, glong offset_uboot, NF_FW_IMAGE_LIST_TABLE *img_list_table)
{
	gboolean ret=FALSE;
	image_header_t imgh;
	glong data_len=0;
	gint autoplace=0, writeoob=0;

	g_message("%s ====== Uboot Upgrade Start!! ======", __FUNCTION__);
	fseek(fp, offset_uboot, SEEK_SET);

	memset(&imgh, 0x0, FW_UPGRADE_IMG_HEADER_SIZE);

	if(fread((char *)&imgh, FW_UPGRADE_IMG_HEADER_SIZE, 1, fp) != 1)
	{
		g_warning("%s Uboot Data Read Fail!! ", __FUNCTION__);
		goto nf_fw_upgrade_fail;
	}

	data_len = (glong)img_list_table[NF_FW_TYPE_UBOOT].data_len;
	autoplace = (glong)img_list_table[NF_FW_TYPE_UBOOT].autoplace;
	writeoob = 	(glong)img_list_table[NF_FW_TYPE_UBOOT].writeoob;

	if(!nf_flash_erase(FW_UPGRADE_UBOOT_MTD_NUM, NULL, NULL))
	{
		g_warning("%s Flash Erase Error!!", __FUNCTION__);
		return FALSE;
	}

	if((nf_flash_fw_write(FW_UPGRADE_UBOOT_MTD_NUM, fp, data_len, autoplace, writeoob, NULL, NULL)) != TRUE)
	{
		g_warning("%s Flash MTD0 Write Error!!", __FUNCTION__);
		return FALSE;
	}
	
	g_message("%s ====== Uboot Upgrade Finish!! ======", __FUNCTION__);

	return TRUE;

nf_fw_upgrade_fail:
	return FALSE;
}

#if 0
/** 
    @brief							set update variable in nand flash (for uboot upgrade)
    @return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_set_update_variable(void)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0, };
	NF_FW_UPGRADE_CHECK fwcheck;
	gboolean ret=FALSE;
	guint offs=0;

	ret = nf_flash_erase(FW_UPGRADE_RESERVED_MTD_PONG_NUM, NULL, NULL);
	if(!ret)
	{
		g_warning("%s Nand Flash Erase Error!!!", __FUNCTION__);
		return FALSE;
	}

	offs = 0;
	strncpy(fwcheck.magic, FW_UPGRADE_NF_MAGIC_U, sizeof(fwcheck.magic));
	fwcheck.is_upgrade = TRUE;

	memset(dataBuf, 0xff, FW_UPGRADE_NAND_PAGE_SIZE);
	memcpy(dataBuf, &fwcheck, sizeof(NF_FW_UPGRADE_CHECK));
	
	ret = nf_flash_page_write(FW_UPGRADE_RESERVED_MTD_PONG_NUM, offs, dataBuf, NULL, 0, 0);
	if(!ret)
	{
		g_warning("%s Nand Flash Erase Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	return TRUE;
}
#endif

gboolean nf_fw_get_file_header(char *filename, NF_FW_IMAGE_HEADER *header)
{
	FILE *fp=NULL;
	NF_FW_IMAGE_LIST fw_header;

	g_return_val_if_fail(filename != NULL, 0);
	g_return_val_if_fail(header != NULL, 0);

	if((fp=fopen(filename, "r")) == NULL)
	{
		g_warning("%s File Open Error!!! Image Name [%s]",
					__FUNCTION__, filename);
		return FALSE;
	}
	#if 0
		else
			g_message("%s File Open Success!! filename[%s]", __FUNCTION__, filename);
	#endif

	if(fread(&fw_header, sizeof(NF_FW_IMAGE_LIST), 1, fp) != 1)
	{
		g_warning("%s fw header fread() error!!! ", __FUNCTION__);
		fclose(fp);
		return FALSE;
	}

	if(strncmp(fw_header.fwheader.magic, FW_UPGRADE_NF_MAGIC_S, strlen(FW_UPGRADE_NF_MAGIC_S)))
	{
		g_warning("%s FW Header Magic Different!!!", __FUNCTION__);
		fclose(fp);
		return FALSE;
	}
	#if 0
		else
			g_message("%s Magic Check Success!! %4.4s", __FUNCTION__, fw_header.fwheader.magic);
	#endif

	fclose(fp);

	memcpy(header, &fw_header.fwheader, sizeof(NF_FW_IMAGE_HEADER));

	return TRUE;
}

/** 
    @brief                          fw call back function
    @param[in]	data				
    @param[in]  context				
*/
void nf_fw_cb_func(NF_UTIL_FLASH_PROGRESS *data, gpointer context)
{
#ifdef DEBUG_FW_UPGRADE_PRGT
	g_message("%s tot_block_cnt [%d] cur_block_cnt [%d] bad_block_cnt [%d]",
				__FUNCTION__, data->tot_block_cnt, data->cur_block_cnt, data->bad_block_cnt);
#endif
	_prgt.total = data->tot_block_cnt;
	_prgt.current = data->cur_block_cnt;

#ifdef NF_FW_UPGRADE_LED
	_led_cnt++;
	nf_fw_continue_led(_led_cnt);
#endif
}

void nf_fw_continue_led(guint cnt)
{
#ifdef USE_DEV_KEYPAD
	if(cnt & 1) {
		nf_dev_keypad_led_on(38);
	}else{
		nf_dev_keypad_led_off(38);
	}
#endif
}

/** 
    @brief							set progress bar variable
    @param[in]	type				progress type
    @param[in]	state				progress state
    @param[in]	is_error			error value
    @param[in]	current				current block count		it's for UI progress bar
    @param[in]	total				total block count		it's for UI progress bar
*/
static void _fw_set_prgt_state(gshort type, short state, gint is_error, guint current, guint total)
{
	NF_UTIL_FLASH_PROGRESS flash_prgt;
	
	_prgt.type = type;
	_prgt.state = state;
	_prgt.is_error = is_error;
	_prgt.current = current;
	_prgt.total = total;

#ifdef NF_FW_UPGRADE_LED
	_led_cnt++;
	nf_fw_continue_led(_led_cnt);
#endif

}

void nf_fw_set_prgt_state(gshort type, short state, gint is_error, guint current, guint total)
{
	_fw_set_prgt_state(type, state, is_error, current, total);
}

/** 
    @brief                          do crc check after read from nand 
    @param[in]  img_fliename        upgrade image filename
    @return     gboolean            %TRUE on success, %FALSE if an error occurred
    
    firmware image sector ==> [header][uboot][kernel][dsp][filesys][end magic]
*/
gboolean nf_fw_crc_check(gint mtd_num, guint file_checksum, guint filesize, gint img_type, gint prgt_type, gboolean is_header_write)
{
	guint checksum=0;

	checksum = (guint)fwup_chk_crc(0, mtd_num, filesize, is_header_write, img_type, prgt_type);

	if(checksum !=  file_checksum)
	{
		g_warning("%s CRC Error!!! Checksum Nand[%08x] Checksum File[%08x]\n",
				   __FUNCTION__, fw_ntohl(checksum),  file_checksum);
		return FALSE;
	}

	return TRUE;
}

/**
	@brief							firmware list
	@param[in]  path				directory path
	@param[in]  fwimglist			save list infomation
	@param[in]  req_cnt				request cnt.. this decides to save how many in fwimglist..
	@param[in]  cnt					firmware list count
	@return     gboolean			%TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_list(gchar *path, NF_FW_IMAGE_LIST *fwimglist, gint req_cnt, gint *ret_cnt)
{
	DIR *dp;
	FILE *fp=NULL;
	struct dirent *dirp;
	struct stat	f_stat;
	gint ret=0, i=0;
	gchar filename[FW_UPGRADE_MAX_BUFFER]={0,};

	for(i=0; i < req_cnt; i++)
		memset(&fwimglist[i], 0x0, FW_UPGRADE_LIST_SIZE);

	g_message("%s\nFirmware Image Directory Name [%s]", __FUNCTION__, path);

	if(!(dp = opendir(path)))
	{
		g_warning("%s Directory open error!!! Directory name [%s]",
					__FUNCTION__, path);
		return FALSE;
	}

	*ret_cnt = 0;	//init return count to 0
	
	while((dirp=readdir(dp)) != NULL)
	{
		if(strcmp(dirp->d_name,".") == 0 || strcmp(dirp->d_name,"..") == 0)
			continue;

		if((strlen(dirp->d_name)+ strlen(path)+1) > FW_UPGRADE_MAX_BUFFER)
		{
			g_warning("%s filename is too long.. max length [%d] filename [%s/%s] file length [%d]\n",
						__FUNCTION__, FW_UPGRADE_MAX_BUFFER, path, dirp->d_name,
						strlen(path)+strlen(dirp->d_name)+1);
			continue;
		}

		sprintf(filename,"%s/%s", path, dirp->d_name);

		if(stat(filename, &f_stat) == -1)
		{
			g_warning("%s can't stat [%s]", __FUNCTION__, filename);
			continue;
		}   

		if(stat(filename, &f_stat) == -1)
		{
			g_warning("%s Cannot Find Filename [%s]", __FUNCTION__, dirp->d_name);
			continue;
		}

		if(S_ISDIR(f_stat.st_mode))
			continue;
		else
		{
			//compare string
			if(strncmp(dirp->d_name, FW_UPGRADE_NAME_TITLE, 11) == 0)
			{
				if((fp = fopen(filename, "r")) == NULL)
					g_warning("%s File open error!!! filename [%s]", __FUNCTION__, filename); 
			
				if(f_stat.st_size < (glong)FW_UPGRADE_LIST_SIZE)
				{
					g_warning("%s Invalidate File.. filename [%s] filesize[%ld]",
								__FUNCTION__, dirp->d_name, f_stat.st_size);
					fclose(fp);
				}
				else
				{
					if((*ret_cnt) < req_cnt)
					{
						if(fread(&fwimglist[*ret_cnt], FW_UPGRADE_LIST_SIZE, 1, fp) != 1)
							g_warning("%s fread() error!!! filename [%s]", __FUNCTION__, dirp->d_name);
						else
							(*ret_cnt)++;
					}
					else
					{
						g_warning("%s Current Count excessed Request Count.. req_cnt [%d] cur_cnt [%d]",
									__FUNCTION__, req_cnt, *ret_cnt);
						fclose(fp);
						closedir(dp);
						return FALSE;
					}
				}
			}
		}	//end if.. S_ISDIR
	}
	
	fclose(fp);
	closedir(dp);

	return TRUE;
}

/**
	@brief                          update sequence number
	@param[in]  ping_seq			ping sequence number
	@param[in]  pong_seq			pong sequence number
	@return     gboolean            %TRUE on success, %FALSE if an error occurred
*/
gboolean nf_fw_set_seq(guint *ping_seq, guint *pong_seq, gint is_first, NF_FW_IMAGE_LIST *nand_imglist)
{
	NF_FW_IMAGE_LIST imglist;
	gboolean ret=FALSE;
	guchar dataBuf[FW_UPGRADE_NAND_PAGE_SIZE] = {0,};
	guchar oobBuf[FW_UPGRADE_NAND_OOB_SIZE] = {0,};

	if(is_first)
	{
		ret = nf_flash_erase(FW_UPGRADE_MTD_PONG_NUM, NULL, NULL);
		if(!ret)
		{
			g_warning("%s flash erase error!!!", __FUNCTION__);
			return FALSE;
		}
		strcpy(nand_imglist->fwheader.magic , "ITXS");
		nand_imglist->fwheader.seq = *pong_seq;
		memcpy(dataBuf, (void*)nand_imglist, FW_UPGRADE_LIST_SIZE);
		ret = nf_flash_page_write(FW_UPGRADE_MTD_PONG_NUM, 0, dataBuf, oobBuf, 0,0);
		if(!ret)
		{
			g_warning("%s flash write error!!!", __FUNCTION__);
			return FALSE;
		}

		nf_flash_erase(FW_UPGRADE_MTD_PING_NUM, NULL, NULL);
//		memset(&nand_imglist, 0xff, FW_UPGRADE_LIST_SIZE);
		nand_imglist->fwheader.seq = *ping_seq;
		memcpy(dataBuf, (void*)nand_imglist, FW_UPGRADE_LIST_SIZE);
		ret = nf_flash_page_write(FW_UPGRADE_MTD_PING_NUM, 0, dataBuf, oobBuf, 0,0);
		if(!ret)
		{
			g_warning("%s flash write error!!!", __FUNCTION__);
			return FALSE;
		}
	}
	else
	{
		if(*ping_seq > *pong_seq)
		{
			ret = nf_flash_erase(FW_UPGRADE_MTD_PONG_NUM, NULL, NULL);
			if(!ret)
			{
				g_warning("%s Nand Flash Erase Error!!!", __FUNCTION__);
				return FALSE;
			}

//			memset(&imglist, 0xff, FW_UPGRADE_LIST_SIZE);
			*pong_seq = *ping_seq+1;
			nand_imglist->fwheader.seq = *pong_seq;
			strcpy(nand_imglist->fwheader.magic , "ITXS");
			memcpy(dataBuf, (void*)nand_imglist, FW_UPGRADE_LIST_SIZE);
			ret = nf_flash_page_write(FW_UPGRADE_MTD_PONG_NUM, 0, dataBuf, oobBuf, 0,0);
			if(!ret)
			{
				g_warning("%s Nand Flash Write Error!!!", __FUNCTION__);
				return FALSE;
			}
		}
		else
		{
			ret = nf_flash_erase(FW_UPGRADE_MTD_PING_NUM, NULL, NULL);
			if(!ret)
			{
				g_warning("%s Nand Flash Erase Error!!!", __FUNCTION__);
				return FALSE;
			}
//			memset(&imglist, 0xff, FW_UPGRADE_LIST_SIZE);
			strcpy(nand_imglist->fwheader.magic , "ITXS");
			*ping_seq = *pong_seq+1;
			nand_imglist->fwheader.seq = *ping_seq;
			memcpy(dataBuf, (void*)nand_imglist, FW_UPGRADE_LIST_SIZE);
			ret = nf_flash_page_write(FW_UPGRADE_MTD_PING_NUM, 0, dataBuf, oobBuf, 0,0);
			if(!ret)
			{
				g_warning("%s Nand Flash Write Error!!!", __FUNCTION__);
				return FALSE;
			}
		}
	}
	
	// sequence verify...
	memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
	ret = nf_flash_read(FW_UPGRADE_MTD_PING_NUM, 0, 0,dataBuf, oobBuf);
	if(!ret)
	{
		g_warning("%s Nand Flash Read Error!!!", __FUNCTION__);
		return FALSE;
	}
	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
	*ping_seq = imglist.fwheader.seq;

	memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
	ret = nf_flash_read(FW_UPGRADE_MTD_PONG_NUM, 0, 0,dataBuf, oobBuf);
	if(!ret)
	{
		g_warning("%s Nand Flash Read Error!!!", __FUNCTION__);
		return FALSE;
	}
	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
	*pong_seq = imglist.fwheader.seq;
	
	g_message("%s After Update Sequence Number ==> Ping[%u] Pong[%u]\n",
		   			__FUNCTION__, *ping_seq, *pong_seq);
	
	if ( *ping_seq > *pong_seq )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_FW_UPGRADE_PING, "Upgraded ping>pong");
	else
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_FW_UPGRADE_PONG, "Upgraded pong>ping");

	return TRUE;
}

/**
	@brief                          current sequence number check
	@param[in]  ping_seq			ping sequence number
	@param[in]  pong_seq			pong sequence number
	@return     gint				0 : upgrade is processed first
									1 : upgrade in first area
									2 : upgrade in sencond aread 
									-1: error!!
*/
gint nf_fw_get_seq(guint *ping_seq, guint *pong_seq, gint *is_first)
{
	NF_FW_IMAGE_LIST imglist;
	guchar dataBuf[FW_UPGRADE_NAND_PAGE_SIZE] = {0,};
	guchar oobBuf[FW_UPGRADE_NAND_OOB_SIZE] = {0,};
	gint i=0, ret=0;

	//ping read
	memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
	ret = nf_flash_read(FW_UPGRADE_MTD_PING_NUM, 0, 0,dataBuf, oobBuf);
	if(!ret)
	{
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_SEQ, FW_UPGRADE_PRGT_ERROR, 0, 0);
		return -1;
	}
		
	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
	*ping_seq = imglist.fwheader.seq;

	//pong read
	memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
	nf_flash_read(FW_UPGRADE_MTD_PONG_NUM, 0, 0,dataBuf, oobBuf);
	if(!ret)
	{
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_SEQ, FW_UPGRADE_PRGT_ERROR, 0, 0);
		return -1;
	}
	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
	*pong_seq = imglist.fwheader.seq;

	if((*ping_seq == 0xffffffff) && (*pong_seq == 0xffffffff))
	{
		g_message("%s Upgrade is first...", __FUNCTION__);
		g_message("%s Current Sequence Number ==> ping_seq [%u] pong_seq [%u]\n",
		   		__FUNCTION__, *ping_seq, *pong_seq);
	
		*ping_seq = 10001;
		*pong_seq = 10000;
		*is_first = TRUE;
		return FW_UPGRADE_PING;
	}
	else
	{
		g_message("%s Current Sequence Number ==> ping_seq [%u] pong_seq [%u]\n",
		   		__FUNCTION__, *ping_seq, *pong_seq);
		
		*is_first = FALSE;

		if(*ping_seq >*pong_seq)
			return FW_UPGRADE_PONG;
		else if(*ping_seq == *pong_seq)
		{
			_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_SEQ, FW_UPGRADE_PRGT_ERROR, 0, 0);
			g_warning("%s Sequence Number is same!!! ping_seq [%u] pong_seq [%u]",
				   		__FUNCTION__, *ping_seq, *pong_seq);
			return -1;
		}
		else
			return FW_UPGRADE_PING;
	}
}

/**
	@brief                          define jbshell
*/
#ifdef DEBUG_JBSHELL_FLASH
#if 0
#include "iux_afx.h"
extern int _start_fw_upgrade(TRANSACTION tra, const char *full_file_name);
#endif
static char nf_fw_upgrade_help [] = "fw_upgrade [fileanme] [is_nonblocking]\n"
									"           ui [filename]\n";
static int fw_upgrade(int argc, char **argv)
{
	gboolean is_nonblocking=TRUE;
	gchar filename[FW_UPGRADE_MAX_BUFFER]={0, };

	if(argc < 3) {
		printf("Invalid arguments\n%s\n", nf_fw_upgrade_help);
		return -1;
	}
	
	if(strncmp(argv[1], "ui", 2) == 0)
	{
		#if 0
			_start_fw_upgrade(TRA_FW_UPGRADE, argv[2]);
		#else
			g_message("%s Blocked For Build Error!!", __FUNCTION__);
		#endif
		return TRUE;
	}
	else
	{
		strncpy(filename, argv[1], FW_UPGRADE_MAX_BUFFER);

		is_nonblocking = (gboolean)strtoul(argv[2], NULL, 0);
	
		nf_fw_upgrade_thread_start(filename, is_nonblocking);
	}

	return 0;
}
__commandlist(fw_upgrade, "fw_upgrade", "fw_upgrade [filename] [is_nonblocking]", nf_fw_upgrade_help);

static char fw_validate_check_help [] = "fw_validate_check [1 or 2] ==> 1 is ping , 2 is pong";
static int fw_validate_check(int argc, char **argv)
{
	gint is_ping=0;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", fw_validate_check_help);
		return -1;
	}

	is_ping = (gint)strtoul(argv[1], NULL, 0);
	
	g_return_val_if_fail(is_ping >=1 || is_ping <=2, 0);
#if 0
	if(nf_fw_crc_check("/FWIMAGE/nf_firmware_1.0.0", is_ping))
		g_print("Image File is good..\n");
	else
		g_print("Image File is bad..\n");
#endif
	return 0;
}
__commandlist(fw_validate_check, "fw_validate_check", "fw_validate_check [Filename]", fw_validate_check_help);

static char nf_fw_list_help [] = "fw_list [Directory]";
static int fw_list(int argc, char **argv)
{
	NF_FW_IMAGE_LIST fwimglist[5];
	gint ret_cnt=0, req_cnt=0, i=0;

	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", nf_fw_list_help);
		return -1;
	}
	
	req_cnt = (gint)strtoul(argv[2], NULL, 0);
	
	if(req_cnt > 5)
	{
		g_warning("%s Current Buffer Max Value is 5..", __FUNCTION__);
		return -1;
	}
	
	nf_fw_list(argv[1], fwimglist, req_cnt, &ret_cnt);

	for(i=0; i<req_cnt; i++)
		g_message("%s [%d] filename ==> [%s]", __FUNCTION__, i, fwimglist[i].filename);

	return 0;
}
__commandlist(fw_list, "fw_list", "fw_list [Directory]", nf_fw_list_help);

static char fw_file_validate_check_help [] = "fw_file_validate_check [Filename]";
static int fw_file_validate_check(int argc, char **argv)
{
	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", fw_file_validate_check_help);
		return -1;
	}
	
	if(nf_fw_file_validate_check(argv[1]))
		g_print("Image File is good..\n");
	else
		g_print("Image File is bad..\n");

	return 0;
}
__commandlist(fw_file_validate_check, "fw_file_validate_check", "fw_file_validate_check [Filename]", fw_file_validate_check_help);

#endif


#ifdef DEBUG_JBSHELL_FLASH
static char fw_prgt_state_print_help [] = "fw_prgt_print";
static int fw_prgt_state_print(int argc, char **argv)
{
	NF_UTIL_FLASH_PROGRESS flash_prgt;

	if ( argc < 1 ) {
		printf("Invalid arguments\n%s\n", fw_prgt_state_print_help);
		return -1;
	}
	
	g_print("===========================================\n");
	g_print("type       [%02x]\n", _prgt.type);
	g_print("state      [%02x]\n", _prgt.state);
	g_print("is_error   [%d]\n", _prgt.is_error);
	g_print("current    [%d]\n", _prgt.current);
	g_print("total      [%d]\n", _prgt.total);
	g_print("===========================================\n");
	
		
	return 0;
}
__commandlist(fw_prgt_state_print, "fw_prgt_print", "fw_prgt_print", fw_prgt_state_print_help);
#endif

#ifdef DEBUG_JBSHELL_FLASH
gboolean nf_fw_bmp_write(gchar* bmp_filename)
{
	FILE* fp=NULL;
	gboolean autoplace=0, writeoob=0;

	if((fp=fopen(bmp_filename, "r")) == NULL)
	{
		g_warning("%s File Open Error!!! Image Name [%s]",
					__FUNCTION__, bmp_filename);
		return FALSE;
	}

	nf_flash_erase(FW_UPGRADE_LOGO_PING_MTD_NUM, NULL, NULL);

	if((nf_flash_write(FW_UPGRADE_LOGO_PING_MTD_NUM, bmp_filename, autoplace, writeoob, NULL, NULL)) != TRUE)
		g_warning("%s Loge Image Write Error!!! Filename [%s]", __FUNCTION__, bmp_filename);

	fclose(fp);
	return TRUE;
}

static char fw_bmp_write_help [] = "fw_bmp_write [bmp name]";
static int fw_bmp_write(int argc, char **argv)
{
	if ( argc < 1 ) {
		printf("Invalid arguments\n%s\n", fw_bmp_write_help);
		return -1;
	}

	if(!nf_fw_bmp_write(argv[1]))
	{
		g_warning("%s Loge Image Write Error", __FUNCTION__);
		return FALSE;
	}

	return 0;
}
__commandlist(fw_bmp_write, "fw_bmp_write", "fw_bmp_write", fw_bmp_write_help);

#if 0
static char fw_run_thread_help [] = "fw_run_thread";
static int fw_run_thread(int argc, char **argv)
{
	if ( argc < 0 ) {
		printf("Invalid arguments\n%s\n", fw_bmp_write_help);
		return -1;
	}

	nf_fw_upgrade_thread_start();

	return 0;
}
__commandlist(fw_run_thread, "fw_run_thread", "fw_run_thread", fw_run_thread);
#endif

#endif
#if 0
gboolean nf_file_fix_page_align(FILE* fp, guint *imglen)
{
	guint remain=0;
	guchar temp[FW_UPGRADE_NAND_PAGE_SIZE] = {0,};

	memset(temp, 0xff, FW_UPGRADE_NAND_PAGE_SIZE);	
	*imglen = lseek(fp, 0, SEEK_END);

	if((*imglen % FW_UPGRADE_NAND_PAGE_SIZE) != 0)
	{
		g_warning("%s Not page align..", __FUNCTION__);
		
		remain = *imglen % FW_UPGRADE_NAND_PAGE_SIZE;
		g_print("remain [%d]    [%d]\n", remain, FW_UPGRADE_NAND_PAGE_SIZE - remain);

		if(fwrite(temp, FW_UPGRADE_NAND_PAGE_SIZE - remain, 1, fp) != 1)
		{
			g_warning("%s fwrite() error!!! ", __FUNCTION__);
			return FALSE;
		}
	
		
	}


	lseek (fp, 0, SEEK_SET);
	return TRUE;
}
#endif

/**
	@brief							firmware file validate check	
	@param[in]  img_fliename		upgrade image filename
	@return     gboolean			%TRUE on success, %FALSE if an error occurred
	
	firmware image sector ==> [header][uboot][kernel][dsp][filesys][end magic]
*/
//#define NF_FW_DEBUG_VALIDATE_CHECK
gboolean nf_fw_file_validate_check(const gchar *img_filename)
{
	FILE *fp=NULL;
	gint ret=0;
	guint i=0;
	gulong size=0, cur_offset=0, crc_offset=0;
	glong end_magic_offset=0, start_offs=0;
	gchar crc_header_chk[FW_UPGRADE_IMG_HEADER_SIZE]={0,};
	gchar magic_e_buf[5]={0, };
	struct stat	f_stat;

	NF_FW_IMAGE_LIST imglist;
	NF_FW_IMAGE_LIST_TABLE img_list_table[IMAGE_TABLE_MAX_NUM];

	g_return_val_if_fail(img_filename != NULL, 0);

	//image file name length check
	if(strlen(img_filename) > FW_UPGRADE_MAX_BUFFER)
	{
		g_warning("%s Img Name too long [%d]", __FUNCTION__, strlen(img_filename));
		return FALSE;
	}
	
   	//file exist check	
	if(stat(img_filename, &f_stat) == -1)
	{
		g_warning("%s Cannot find File Name [%s]", __FUNCTION__, img_filename);
		return FALSE;
	}
	
	if(f_stat.st_size < (glong)FW_UPGRADE_IMG_HEADER_SIZE)
	{
		g_warning("%s File Size is too short.. image filename [%s] filesize[%ld]",
				__FUNCTION__, img_filename, f_stat.st_size);
		return FALSE;
	}
	
	g_message("%s Imgage File Name [%s] File Size [%ld]", __FUNCTION__, img_filename, f_stat.st_size);
	
	//image file open	
	if((fp=fopen(img_filename, "r")) == NULL)
	{
		g_warning("%s File Open Error!!! Image File Name [%s]", 
					__FUNCTION__, img_filename);
		return FALSE;
	}
	
	fseek(fp, f_stat.st_size - (glong)sizeof(img_list_table), SEEK_CUR);
	if(fread(&img_list_table, sizeof(img_list_table), 1, fp) != 1)		//read tail size
	{
		g_warning("%s tail fread() error!!! ", __FUNCTION__);
		_fw_set_prgt_state(NF_FW_PRGT_TYPE_PRE, NF_FW_PRGT_PRE_FILE_READ, FW_UPGRADE_PRGT_ERROR, 0, 0);
		fclose(fp);
		return FALSE;
	}

	fseek(fp, 0, SEEK_SET);		// return to 0 file pointer

	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	if((ret = (gint)fread(&imglist, (size_t)FW_UPGRADE_LIST_SIZE, 1, fp)) != 1)		//read NF_FW_IMAGE_HEADER
	{
		g_warning("%s header fread() error!!! ret_cnt [%d]", __FUNCTION__, ret);
		fclose(fp);
		return FALSE;
	}

	//////////////////// Start Magic Check ////////////////////
	if(strcmp(imglist.fwheader.magic, FW_UPGRADE_NF_MAGIC_S) !=0)
	{
		g_warning("%s Not Match Start Magic. Magic Name [%s]",
				__FUNCTION__, imglist.fwheader.magic);
		fclose(fp);
		return FALSE;
	}
	else
		g_print("NF Firmware Image Start MAGIC Check Success!!!\n");

	#if 1
	/************************* F/W Version Check ***************************/
	{
		gchar *fw_version=NULL;
		gchar *fw_option=NULL;
		gchar *file_option=NULL;
		if(!nf_sysman_hotkey_is_nfs())
		{
			fw_version = nf_sysdb_get_str_nocopy("sys.info.swver");

			if(strncmp(fw_version, imglist.fwheader.version, 5) != 0)
			{
				g_warning("%s Not Match F/W Version. fw version [%s]",
						__FUNCTION__, imglist.fwheader.version);
				fclose(fp);
				return FALSE;
			}

			fw_option=strrchr(fw_version, '.');
			fw_option++;
			
			file_option=strrchr(imglist.fwheader.version, '.');
			file_option++;

			if(strcmp(fw_option, file_option)!=0)
			{
				g_warning("%s Not Match F/W OPTION. fw option [%s]",
						__FUNCTION__, file_option);
				fclose(fp);
				return FALSE;
			}
		}
	}
	#endif
	
	////////////////////	CRC Check ////////////////////
	rewind(fp);		//move file pointer to 0
	fseek(fp, FW_UPGRADE_LIST_SIZE, SEEK_CUR);	//move file pointer to u-boot start

	for(i=0; i<imglist.fwheader.sec_cnt; i++)
	{
		image_header_t crc_chk_imh;
		guint checksum=0, ret_checksum=0, data_len=0;
		gint img_type=0;

		//1. header crc check
		memset(&crc_chk_imh, 0x0, FW_UPGRADE_IMG_HEADER_SIZE);
		if(fread(&crc_chk_imh, FW_UPGRADE_IMG_HEADER_SIZE, 1, fp) != 1)
		{
			g_warning("%s CRC Header Check fread() error!!!", __FUNCTION__);
			fclose(fp);
			return FALSE;
		}
		
#ifdef NF_FW_DEBUG_VALIDATE_CHECK
		nf_fw_imgh_print(&crc_chk_imh);
		g_message("%s Header CRC Check Image Name [%s]", __FUNCTION__, crc_chk_imh.ih_name);
#endif
		g_print("[%-32.32s] check...\n", crc_chk_imh.ih_name);
		
		checksum = ntohl(crc_chk_imh.ih_hcrc);
		crc_chk_imh.ih_hcrc = htonl(0);

		if((ret_checksum = header_crc_check(0, (guchar*)&crc_chk_imh, FW_UPGRADE_IMG_HEADER_SIZE)) != checksum)
		{
			g_warning("%s Header CRC is different!!! checksum [%08x] ret_checksum [%08x]",
						__FUNCTION__, checksum, ret_checksum);
			fclose(fp);
			return FALSE;
		}
#ifdef NF_FW_DEBUG_VALIDATE_CHECK
		else
			g_message("%s [%s] Header CRC Check OK!!! ret_chksum[%08x]", __FUNCTION__, crc_chk_imh.ih_name, ret_checksum);
#endif

		/*
		   here To Do....
		   2. data crc check...
		*/
#ifdef NF_FW_DEBUG_VALIDATE_CHECK
		g_print("imglist.fwheader.sec_header[%d].ih_size [%d]\n", i, ntohl(imglist.fwheader.sec_header[i].ih_size));
#endif

		data_len = img_list_table[i].crc_check_len;
		checksum = img_list_table[i].checksum;
		img_type=img_list_table[i].img_type;

		if((ret_checksum=data_crc_check(0, fp, data_len, img_type)) != checksum)
		{
			g_warning("%s Data CRC is different!!! checksum [%08x] ret_checksum [%08x] CRC Check Fail...",
						__FUNCTION__, checksum, ret_checksum);
			fclose(fp);
			return FALSE;
		}
		else
		{
			g_print("...ok\n");
#ifdef NF_FW_DEBUG_VALIDATE_CHECK
			g_message("%s [%s] Data CRC Check OK!!! ret_chksum[%08x]", __FUNCTION__, crc_chk_imh.ih_name, ret_checksum);
#endif
		}
	}

	//////////////////// End Magic Check ////////////////////
	for(i=0; i<imglist.fwheader.sec_cnt; i++)
	{
		if(i == 0)
			end_magic_offset = (glong)(FW_UPGRADE_LIST_SIZE + FW_UPGRADE_IMG_HEADER_SIZE
								+ fw_ntohl(imglist.fwheader.sec_header[i].ih_size));
		else
			end_magic_offset += (glong)(FW_UPGRADE_IMG_HEADER_SIZE + fw_ntohl(imglist.fwheader.sec_header[i].ih_size));
	}

	fseek(fp, start_offs, SEEK_SET);						//offset to 0	== rewind(fp)	
	fseek(fp, end_magic_offset, SEEK_CUR);		//move offset to end maigs's start location

	if(fread(magic_e_buf, sizeof(magic_e_buf)-1, 1, fp) !=1)	//you must read 4byte.. otherwise it excesses filesize
	{
		g_warning("%s End Magic Read Error!!! buffer [%s]", __FUNCTION__, magic_e_buf);
		fclose(fp);
		return FALSE;
	}

	if(strncmp(magic_e_buf, FW_UPGRADE_NF_MAGIC_E, 4) != 0)
	{
		g_warning("%s Not Match End Magic. Magic Name [%s]",
				__FUNCTION__, magic_e_buf);
		fclose(fp);
		return FALSE;
	}
	else
		g_print("NF Firmware Image End MAGIC Check Success!!!\n");

	fclose(fp);
	return TRUE;
}

//just for test
gint nf_fw_seq_update_test(void)
{
	gint ret=0, is_first=0;
	guint ping_seq=0, pong_seq=0;
	NF_FW_IMAGE_LIST imglist;
	guchar dataBuf[FW_UPGRADE_NAND_PAGE_SIZE] = {0,};
	guchar oobBuf[FW_UPGRADE_NAND_OOB_SIZE] = {0,};
	gint i=0;

	//ping read
	memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
	nf_flash_read(FW_UPGRADE_MTD_PING_NUM, 0, 0,dataBuf, oobBuf);
	
	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
	ping_seq = imglist.fwheader.seq;

	//pong read
	memset(dataBuf, 0x0, FW_UPGRADE_NAND_PAGE_SIZE);
	nf_flash_read(FW_UPGRADE_MTD_PONG_NUM, 0, 0,dataBuf, oobBuf);
	memset(&imglist, 0x0, FW_UPGRADE_LIST_SIZE);
	memcpy(&imglist, dataBuf, FW_UPGRADE_LIST_SIZE);
	pong_seq = imglist.fwheader.seq;

	if((ping_seq == 0xffffffff) && (pong_seq == 0xffffffff))
	{
		g_message("%s Upgrade is first...", __FUNCTION__);
		g_message("%s Current Sequence Number ==> ping_seq [%u] pong_seq [%u]\n",
				__FUNCTION__, ping_seq, pong_seq);

		ping_seq = 1;
		pong_seq = 0;
		is_first = TRUE;
	}
	else
	{
		g_message("%s Current Sequence Number ==> ping_seq [%u] pong_seq [%u]\n",
				__FUNCTION__, ping_seq, pong_seq);
		is_first = FALSE;
	}

//	nf_fw_get_seq(&ping_seq, &pong_seq, &is_first);
	nf_fw_set_seq(&ping_seq, &pong_seq, is_first, &imglist);
	
	return 0;
}

void nf_fw_start_upgrade_beep(void)
{
    nf_dev_buzzer_on();
    g_usleep(500000*2);
    nf_dev_buzzer_off();
    g_usleep(500000*2);
    nf_dev_buzzer_on();
    g_usleep(500000*2);
    nf_dev_buzzer_off();
}

void nf_fw_finish_upgrade_beep(void)
{
	printf("%s called\n", __FUNCTION__);
    nf_dev_buzzer_on();
    g_usleep(500000*2);
    nf_dev_buzzer_off();
    g_usleep(500000*2);
    nf_dev_buzzer_on();
    g_usleep(500000*2);
    nf_dev_buzzer_off();
    g_usleep(500000*2);
    nf_dev_buzzer_on();
    g_usleep(500000*2);
    nf_dev_buzzer_off();
}

/******************************************************************************/

#include "nf_util_http.h"

static GStaticMutex _fw_prgt_mutex = G_STATIC_MUTEX_INIT;
static NF_HTTP_CLIENT_PRGT _fw_prgt;
static GThread *_fw_tid = NULL;

static char *_hdd_dev_list[] = {
		"/dev/hda2",
		"/dev/hdb2",
		"/dev/hdc2",
		"/dev/hdd2",
		NULL,
};

#define HDD_MOUNT_PATH 	"/mnt/hdd_upgrade" 

static void _http_get_cb( const NF_HTTP_CLIENT_PRGT  *prgt , gpointer context)
{
	NF_HTTP_CLIENT_REQ  *req = (NF_HTTP_CLIENT_REQ *)context;
	static	guint64 save_sleep_check = 0;
	guint64 sleep_check;
	
	if(prgt->status != NF_HTTP_CLIENT_PRGT_READ)		
	{
		g_message("%s status[%d] prgt[%d]/[%d] error[%d]",__FUNCTION__, 
				prgt->status, prgt->current, prgt->total, prgt->is_error);
	}
	
	g_static_mutex_lock (&_fw_prgt_mutex);
	memcpy( &_fw_prgt, prgt, sizeof(NF_HTTP_CLIENT_PRGT) );
	g_static_mutex_unlock (&_fw_prgt_mutex);
	
	sleep_check = prgt->current/131072;
	if( save_sleep_check !=  sleep_check)
	{
		g_usleep(100000);
		save_sleep_check= sleep_check;
	}
	
}

static gboolean _pre_process_download( void )
{
	char cmd_buff[1024];	
	char *dev_name = NULL;			
	int	 ret=0, i=0, hdd_idx=-1;
	
	for(i=0; i<4; ++i)
	{
		if( g_file_test( _hdd_dev_list[i], G_FILE_TEST_EXISTS ) )
		{
			hdd_idx = i;
			dev_name = _hdd_dev_list[i];
			g_message("%s find[%s]", __FUNCTION__, dev_name );
			break;
		}
	}	
	
	g_return_val_if_fail( hdd_idx != -1, 0);
	
	umount( HDD_MOUNT_PATH );
	
	snprintf(cmd_buff, sizeof(cmd_buff)-1, "/NFDVR/mkdosfs -F 32 %s", dev_name );
	proxy_system(cmd_buff, 1, 5);
	
	rmdir( HDD_MOUNT_PATH );
	mkdir( HDD_MOUNT_PATH, 0755 );
	
	ret = mount( dev_name, HDD_MOUNT_PATH , "vfat", 0, NULL);
	if(ret != 0)
	{
		g_warning("%s mount error %d(%s)", __FUNCTION__,
				 errno, strerror(errno) );
				 				 		
		g_return_val_if_fail( ret != 0, 0);
	}	
	g_message("%s mount ok [%s]", __FUNCTION__, HDD_MOUNT_PATH);		
	
	return 1;	
}
	
static void _download_thread_func( gpointer arg )
{
	NF_HTTP_CLIENT_REQ *req = (NF_HTTP_CLIENT_REQ *)arg;		
		
	g_message("%s start", __FUNCTION__);
    {
        int policy;
        struct sched_param sched;
        pthread_t thread;
        
        policy = SCHED_FIFO;
        thread = pthread_self();
		sched.sched_priority = sched_get_priority_min(policy);
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT;
		//sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_URGENT+1;
        g_message("%s pthread_setschedparam ret = %d", __FUNCTION__, pthread_setschedparam (thread, policy, &sched));
        g_message("%s pthread_getschedparam ret = %d", __FUNCTION__, pthread_getschedparam (thread, &policy, &sched));
        g_message("%s XXX ugienf set realtime policy = %d", __FUNCTION__, policy);
    }
 	
	nf_http_client_get_file( req, req->out_filename, NULL);
		
	g_free(arg);	
	_fw_tid = NULL;
}

gboolean 
nf_fw_network_upgrade_prgt( NF_HTTP_CLIENT_PRGT *prgt )
{
	g_return_val_if_fail( prgt != NULL, 0 );
	
	g_static_mutex_lock (&_fw_prgt_mutex);
	memcpy( prgt, &_fw_prgt, sizeof(NF_HTTP_CLIENT_PRGT) );
	g_static_mutex_unlock (&_fw_prgt_mutex);	
	
	return 1;
}
	
gboolean 
nf_fw_network_upgrade_download( const char *fw_file, gboolean nb_flag, GError **error )
{

	NF_HTTP_CLIENT_REQ req;		
	char *dev_name = NULL;			
	int	 ret = 0, i, hdd_idx = -1;
	
	g_return_val_if_fail( _fw_tid == NULL, 0);	// check a already running download 
	
	g_return_val_if_fail( fw_file != NULL, 0);			
	g_return_val_if_fail( _pre_process_download() == 1, 0);
	
	memset(&req, 0x00, sizeof(NF_HTTP_CLIENT_REQ));
	memset(&_fw_prgt, 0x00, sizeof(NF_HTTP_CLIENT_PRGT) );
	
//	snprintf( req.url, sizeof(req.url) -1, "%s/%s", NF_FW_SVR_FW_PATH , fw_file);
	snprintf( req.url, sizeof(req.url)-1,  NF_FW_SVR_FW_PATH"%s", 
				nf_api_param_fwver_get_product(),
				nf_api_param_fwver_get_protocol(),
				nf_api_param_fwver_get_vendor(), 
				nf_api_param_fwver_get_option2(),
				fw_file );

	g_message("%s url[%s]",__FUNCTION__, req.url);
	
	req.is_debug = 0;
	req.timeout_connect_sec = 3;
	req.timeout_rx_sec = req.timeout_tx_sec = 5;

	req.cb_func = _http_get_cb;
	req.cb_arg = &req;

	snprintf(req.out_filename, 
				sizeof(req.out_filename)-1,
				"%s/%s", HDD_MOUNT_PATH, fw_file);

	g_message("%s dest[%s]",__FUNCTION__, req.out_filename);

	unlink(req.out_filename);

	if( nb_flag ){

		gpointer arg = g_malloc( sizeof(NF_HTTP_CLIENT_REQ) );

		g_return_val_if_fail( arg != NULL, 0);

		memcpy(arg, &req, sizeof(NF_HTTP_CLIENT_REQ));

		_fw_tid = g_thread_create((GThreadFunc)_download_thread_func, arg, FALSE, NULL);

		return _fw_tid ? 1:0;

	}else{
		return  nf_http_client_get_file( &req, req.out_filename, error);
	}
}

gboolean 
nf_fw_network_upgrade_info_download( GError **error )
{
	NF_HTTP_CLIENT_REQ req;		
	gint ret=0;
	
	memset(&req, 0x00, sizeof(NF_HTTP_CLIENT_REQ));

	//strncpy( req.url, NF_FW_SVR_VERINFO_FILE , sizeof(req.url)-1);
	snprintf( req.url, sizeof(req.url)-1, NF_FW_SVR_VERINFO_FILE,
				nf_api_param_fwver_get_product(),
				nf_api_param_fwver_get_protocol(),
				nf_api_param_fwver_get_vendor(),
				nf_api_param_fwver_get_option2());
	
	g_message("%s url[%s]",__FUNCTION__, req.url);

	req.is_debug = 0;
	req.timeout_connect_sec = 3;
	req.timeout_rx_sec = req.timeout_tx_sec = 3;

	unlink(NF_FW_LOCAL_VERINFO_FILE);

	g_message("%s dest[%s]",__FUNCTION__, NF_FW_LOCAL_VERINFO_FILE);

	return nf_http_client_get_file( &req, NF_FW_LOCAL_VERINFO_FILE, error);
}

gboolean 
nf_fw_network_upgrade_check(  NF_FW_NETWORK_VERINFO *ver )
{
	FILE *fp = NULL;
	NF_FW_NETWORK_VERINFO	verinfo;
	
	gint current_major_ver = (gint)nf_api_param_fwver_get_protocol();
	gint current_minor_ver = (gint)nf_api_param_fwver_get_minor();
	gint is_upgrade=0, row_cnt=0;
	
	fp = fopen( NF_FW_LOCAL_VERINFO_FILE, "r");	
	
	g_return_val_if_fail ( fp != NULL, 0);

	
	memset(ver, 0x00, sizeof(verinfo) );
		
	while( fgets( ver->reserved , sizeof(verinfo.reserved)-1, fp ) )
	{
		char *token, *save;
		int  idx=0;
						
		save = ver->reserved;
		token = strsep( &save, "," );
		do {			
			//printf("[%d][%s]\n",i, token);
			if(token)
			{					
				if(idx==0) ver->version    		= token;
				else if(idx==1) ver->date        = token;
				else if(idx==2) ver->time        = token;
				else if(idx==3) ver->is_bridge   = token;
				else if(idx==4) ver->is_urgent   = token;
				else if(idx==5) ver->fw_file     = token;
				else if(idx==6) ver->rel_file	 = token;
				else if(idx==7) ver->reserved1   = token;
				else if(idx==8) ver->reserved2   = token;
				else if(idx==9) ver->reserved3	 = token;
				else {
					printf("idx overflow token[%10.10s]\n", token);
					break;
				}					
				++idx;
			}
			else
				break;
				
		}while( (token = strsep(&save, ","))  );

#if 0
{
		printf("\n\nidx[%d][%d]\n", row_cnt++, idx);
		
		printf("verinfo.version   [%s]\n", (verinfo.version    ) ? verinfo.version   : "null" );
		printf("verinfo.date      [%s]\n", (verinfo.date       ) ? verinfo.date      : "null" );
		printf("verinfo.time      [%s]\n", (verinfo.time       ) ? verinfo.time      : "null" );
		printf("verinfo.is_bridge [%s]\n", (verinfo.is_bridge  ) ? verinfo.is_bridge : "null" );
		printf("verinfo.is_urgent [%s]\n", (verinfo.is_urgent  ) ? verinfo.is_urgent : "null" );
		printf("verinfo.fw_file   [%s]\n", (verinfo.fw_file    ) ? verinfo.fw_file   : "null" );
		printf("verinfo.rel_file  [%s]\n", (verinfo.rel_file   ) ? verinfo.rel_file	 : "null" );
		printf("verinfo.reserved1 [%s]\n", (verinfo.reserved1  ) ? verinfo.reserved1 : "null" );
		printf("verinfo.reserved2 [%s]\n", (verinfo.reserved2  ) ? verinfo.reserved2 : "null" );
		printf("verinfo.reserved3 [%s]\n", (verinfo.reserved3  ) ? verinfo.reserved3 : "null" );
}
#endif
		if( idx == 10 	// field count
				&& ver->version 
				&& (ver->version[0] == 'V' ||  ver->version[0] == 'v') )
		{
			int ret, ver1, ver2;
									
			ret = sscanf( &ver->version[1], "%d.%d",  &ver1, &ver2);
			if(ret == 2)
			{
				ver->major_ver =  ver1;
				ver->minor_ver =  ver2;
			}
#if 0
			printf("dvr[%d.%d]   verinfo[%d.%d]\n", 
						current_major_ver, current_minor_ver,
						ver->major_ver, ver->minor_ver);
#endif
			// detect higher fw entry
			if( ver->major_ver > current_major_ver
				|| (ver->major_ver ==  current_major_ver 
						&& ver->minor_ver > current_minor_ver) )
			{
				is_upgrade = 1;	
												
				if( ver->is_bridge[0] == 'Y' || ver->is_bridge[0] == 'y' )
					goto find_finish;
			}									
		}
		
		memset(&verinfo, 0x00, sizeof(verinfo) );
	}

find_finish:
				
	fclose(fp);
	return is_upgrade;
			
}


#ifdef DEBUG_JBSHELL_FLASH

static int _is_upgrade = 0;
static NF_FW_NETWORK_VERINFO	_verinfo;

static char fw_net_check_help[] = "fw_net_check";
static int jbshell_fw_net_check(int argc, char **argv)
{	
		
	GError	*error = NULL;	
	int ret = 0;
		
	_is_upgrade = 0;
	
	ret = nf_fw_network_upgrade_info_download( NULL );
	if( ret != 1 )
	{
		if(error){
			g_warning("%s error[%d][%s]", __FUNCTION__, error->code, error->message);
			g_error_free( error);
		}else{
			g_warning("%s nf_fw_network_upgrade_info_download ret[%d]", __FUNCTION__, ret);
		}		
		return 0;
	}

	ret = nf_fw_network_upgrade_check (&_verinfo);
	if(ret == 1)
	{	
		printf("is_upgrade[%d] [%d.%d][%s]\n", ret,
				_verinfo.major_ver, _verinfo.minor_ver,
				(_verinfo.fw_file ) ? _verinfo.fw_file   : "null" );
					
		_is_upgrade = 1;
	}
	
	return 0;
}
__commandlist(jbshell_fw_net_check,"fw_net_check", fw_net_check_help, fw_net_check_help);

static char fw_net_run_help[] = "fw_net_run";
static int jbshell_fw_net_run(int argc, char **argv)
{	
	
	GError	*error = NULL;	
	int ret = 0;
		
	ret = nf_fw_network_upgrade_download( _verinfo.fw_file, 1, &error );
	if(ret != 0)
	{
		if(error){
			g_warning("%s error[%d][%s]", __FUNCTION__, 
							error->code, error->message);
			g_error_free( error);
		}
	}
		
	return 0;	
}
__commandlist(jbshell_fw_net_run,"fw_net_run", fw_net_run_help, fw_net_run_help);

static char fw_net_prgt_help[] = "fw_net_prgt";
static int jbshell_fw_net_prgt(int argc, char **argv)
{
		
	NF_HTTP_CLIENT_PRGT p;
	NF_HTTP_CLIENT_PRGT *prgt = &p;
	
	nf_fw_network_upgrade_prgt( prgt );
	
	g_message("%s status[%d] prgt[%d]/[%d] error[%d]",__FUNCTION__, 
				prgt->status, prgt->current, prgt->total, prgt->is_error);
					
	return 0;
}
__commandlist(jbshell_fw_net_prgt,"fw_net_prgt", fw_net_prgt_help, fw_net_prgt_help);

static char fw_jbshell_chk_fwheader_file_help[] = "fw_chk_file";
static int fw_jbshell_chk_fwheader_file(int argc, char **argv)
{
	NF_FW_IMAGE_HEADER header;

	if(argv[1] == NULL)
		goto nf_fw_jbshell_err_msg;

	nf_fw_get_file_header(argv[1], &header);

	g_message("model        [%s]", header.model); 
	g_message("vendor       [%s]", header.vendor); 
	g_message("lang         [%s]", header.lang); 
	g_message("option       [%s]", header.option); 
	g_message("version      [%s]", header.version); 
					
	return 0;

nf_fw_jbshell_err_msg:
    printf("Invalid arguments\n%s\n", fw_jbshell_chk_fwheader_file_help);
    return -1;

}
__commandlist(fw_jbshell_chk_fwheader_file, "fw_chk_file", fw_jbshell_chk_fwheader_file_help, fw_jbshell_chk_fwheader_file_help);

#endif

#if 0
static void nf_fw_fs_stop(void)
{
	gint ret=0;
#if 0 
	g_message("NF Network Stop Start!!!");
    nf_network_stop(NF_DISCONN_SVR_FW_UPGRADE);
    sleep(3);
#endif

	g_message("NF Record Stop Start!!!");
	nf_record_stop( NULL );

	g_message("NF Filesystem Stop Start!!!");
	ret = nf_filesystem_stop(NULL, NULL, NULL);
	g_message("%s nf_filesystem_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

}
#endif

/************************ MTD OFFSET****************************
0x00000000-0x00100000 : "reserved" 
0x00100000-0x00200000 : "reserved" 
0x00200000-0x00280000 : "boot_param" 
0x00280000-0x00300000 : "app_param" 
0x00300000-0x00400000 : "fw_param" 
0x00400000-0x00600000 : "u-boot" 
0x00600000-0x00800000 : "logoimage" 
0x00800000-0x00c00000 : "kernel" 
0x00c00000-0x01000000 : "dspimage" 
0x01000000-0x08000000 : "filesystem" 

0x08000000-0x08100000 : "reserved" 
0x08100000-0x08200000 : "reserved" 
0x08200000-0x08280000 : "boot_param" 
0x08280000-0x08300000 : "app_param" 
0x08300000-0x08400000 : "fw_param" 
0x08400000-0x08600000 : "u-boot" 
0x08600000-0x08800000 : "logoimage" 
0x08800000-0x08c00000 : "kernel" 
0x08c00000-0x09000000 : "dspimage" 
0x09000000-0x10000000 : "filesystem"
****************************************************************/

/*EOF*/
