#include "nf_common.h"

#include "nf_util_fw.h"
#include "nf_qc.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_fw.h"

//#define DEBUF_PARAM_FW
//#define DEBUG_PARAM_FW_JBSHELL
#ifdef DEBUG_PARAM_FW_JBSHELL
    #include "jbshell.h"
#endif

/*
   Gloval Variable Definition
 */
static NF_PARAM_FW_INFO _fw_param_info[16];

/*
   Gloval Function Definition
 */
#ifdef DEBUG_PARAM_FW_JBSHELL
	static int nf_api_param_fw_jbshell(int argc, char **argv);
#endif
static gboolean _nf_api_param_fw_from_nand(NF_FW_IMAGE_LIST *fw_param);

gboolean nf_api_param_fwparam_init(void)
{
	NF_FW_IMAGE_LIST fw_param;
	guint i=0, info_type_cnt=0;
	const gchar *str=NULL;

	memset(&fw_param, 0x0, sizeof(fw_param));

	_nf_api_param_fw_from_nand(&fw_param);

	if(strncmp(fw_param.fwheader.magic, FW_UPGRADE_NF_MAGIC_S, strlen(FW_UPGRADE_NF_MAGIC_S)) != 0) {
		g_warning("%s F/W Header Magic Check Error!!!", __FUNCTION__);
		return FALSE;
	}

	for(i=0, info_type_cnt=0; i<fw_param.fwheader.sec_cnt; i++, info_type_cnt++) {

		#if 0   /** use anf / atm **/
			if(i == NF_PARAM_FW_INFO_TYPE_DSP)
				info_type_cnt++;
		#endif

		_fw_param_info[i].type=info_type_cnt;
		_fw_param_info[i].size=fw_ntohl(fw_param.fwheader.sec_header[i].ih_size);
		_fw_param_info[i].time=fw_param.fwheader.sec_header[i].ih_time;
		_fw_param_info[i].dcrc=fw_param.fwheader.sec_header[i].ih_dcrc;
		strncpy(_fw_param_info[i].name, fw_param.fwheader.sec_header[i].ih_name, sizeof(_fw_param_info[i].name)-1);

		#ifdef DEBUG_PARAM_FW   
			nf_fw_imgh_print(&fw_param.fwheader.sec_header[i]);
		#endif
	}

	/** last UBL **/
	_fw_param_info[i].type=NF_PARAM_FW_INFO_TYPE_UBL;
	_fw_param_info[i].size=0;
	_fw_param_info[i].time=0;
	str=nf_api_param_hw_get_ubl_crc();
	_fw_param_info[i].dcrc=strtoul(str, NULL, 16);
	strncpy(_fw_param_info[i].name, "UBL", sizeof(_fw_param_info[i].name)-1);

	return TRUE;
}

gboolean nf_sysman_get_fw_param_info(NF_PARAM_FW_INFO *fw_info)
{
	g_return_val_if_fail(fw_info != NULL, 0);
	
	memcpy(fw_info, _fw_param_info, sizeof(_fw_param_info));
	
	return TRUE;
}  

static gboolean _nf_api_param_fw_from_nand(NF_FW_IMAGE_LIST *fw_param)
{
	guchar pingBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	guchar pongBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean ret=FALSE;
	guint offs=0;
	NF_FW_IMAGE_LIST fw_param_ping;
	NF_FW_IMAGE_LIST fw_param_pong;
	NF_FW_IMAGE_LIST *fw_param_ptr=NULL;

	g_return_val_if_fail(pingBuf != NULL, 0);
	g_return_val_if_fail(pongBuf != NULL, 0);
	g_return_val_if_fail(fw_param != NULL, 0);

	memset(&fw_param_ping, 0x0, sizeof(NF_FW_IMAGE_LIST));
	memset(&fw_param_pong, 0x0, sizeof(NF_FW_IMAGE_LIST));
	offs=0;

	if(!nf_flash_read(NF_FLASH_PING_FW_PARAM_MTD_NUM, offs, 0, pingBuf, NULL)) {
		g_warning("%s Get Ping FW Param Error!!!", __FUNCTION__);
		return FALSE;
	}

	if(!nf_flash_read(NF_FLASH_PONG_FW_PARAM_MTD_NUM, offs, 0, pongBuf, NULL)) {
		g_warning("%s Get Pong FW Param Error!!!", __FUNCTION__);
		return FALSE;
	}

	#ifdef DEBUF_PARAM_FW
		flash_print(pingBuf, NULL, 0);
		flash_print(pongBuf, NULL, 0);
	#endif

	memcpy(&fw_param_ping, pingBuf, sizeof(NF_FW_IMAGE_LIST));
	memcpy(&fw_param_pong, pongBuf, sizeof(NF_FW_IMAGE_LIST));

	if(fw_param_ping.fwheader.seq > fw_param_pong.fwheader.seq) {
		fw_param_ptr=&fw_param_ping;
	}
	else {
		fw_param_ptr=&fw_param_pong;
	}

	if(strncmp(fw_param_ptr->fwheader.magic, FW_UPGRADE_NF_MAGIC_S, FW_UPGRADE_MAX_MAGIC_BUF) != 0) {
		g_warning("%s magic failed[%4.4s]", __FUNCTION__, fw_param_ptr->fwheader.magic);
		return FALSE;
	}

	memcpy(fw_param, fw_param_ptr, sizeof(NF_FW_IMAGE_LIST));

	return TRUE;
}

#ifdef DEBUG_PARAM_FW_JBSHELL

static char nf_api_param_fw_jbshell_help[] = "sys_fw_param";
static int nf_api_param_fw_jbshell(int argc, char **argv)
{
	if ( argc < 2 ) {
		printf("Invalid arguments\n%s\n", nf_api_param_fw_jbshell_help);
		return -1;
	}

	if(strcmp(argv[1], "set") == 0) {
		nf_sysman_fw_param_init();
	}
	else if(strcmp(argv[1], "get") == 0) {
		NF_PARAM_FW_INFO fw_info[16];
		int i=0;

		memset(fw_info, 0x0, sizeof(fw_info));
		nf_sysman_get_fw_param_info(fw_info);

		for(i=0; i<16; i++) {
			g_message("type [%08x]", fw_info[i].type);
			g_message("size [%08x]", fw_info[i].size);
			g_message("time [%08x]", fw_info[i].time);
			g_message("dcrc [%08x]", fw_info[i].dcrc);
			g_message("name [%-32.32s]\n", fw_info[i].name);
		}
	}
	else {
		printf("Invalid arguments\n%s\n", nf_api_param_fw_jbshell_help);
		return -1;
	}

	return 0;
}
__commandlist(nf_api_param_fw_jbshell, "sys_fw_param", nf_api_param_fw_jbshell_help, nf_api_param_fw_jbshell_help);

#endif

