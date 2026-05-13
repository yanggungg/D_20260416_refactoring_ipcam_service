#include "nf_common.h"

#include "nf_qc.h"
#include "nf_qc_app.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_hw_enc.h"
#include "nf_hw.h"
#include "nf_debug.h"

#if defined(SCHIP_COPY_PROTECTION)
#include "itx_seed_algo.h"
#endif

//#define DEBUG_PARAM_HW_JBSHELL
#ifdef DEBUG_PARAM_HW_JBSHELL
    #include "jbshell.h"
#endif

static int _hw_param_init = 0;
static char _hw_param_panel_type[32];
static char _hw_param_hw_ver[32];
static char _hw_param_vendor[32];
static char _hw_param_ubl_crc[32];
static char _hw_param_fpga_crc[32];
static char _hw_param_rc_type[32];
#if defined(SCHIP_COPY_PROTECTION)
	static char _hw_param_serial_match[32];
	static char _hw_param_serial_rand[32];
#endif

static const char *_DEBUG_PARAM_HW_str[8] =
{
	"PARAM_HW_IDX_DUMP",
	"PARAM_HW_IDX_NR"
};

static gint _DEBUG_PARAM_HW_log[8] =
{
	0,0,0,0, 0,0,0,0
};

/*
   Gloval Function Definition
 */
#ifdef DEBUG_PARAM_HW_JBSHELL
	static int nf_api_param_hw_jbshell(int argc, char **argv);
#endif

/*
   Extern Function Definition
 */
#if defined(SCHIP_COPY_PROTECTION)
	extern gboolean nf_dev_board_pp_dec_data(NF_HW_PARAM_ENC_DATA *data);
#endif

gboolean nf_sysman_hw_param_init(void)
{
	int pba_type = nf_hw_get_board_type();
	gchar *contents = NULL;
	gsize  length = 0;
	gboolean ret=0;
	GError *error = NULL;
	#if defined(SCHIP_COPY_PROTECTION)
		NF_HW_PARAM_ENC_DATA data;
		ITX_PROTECT_DATA_PARAM data_param;
	#endif

	ret=nf_debug_category_add( "param_hw", (const char *)_DEBUG_PARAM_HW_str, _DEBUG_PARAM_HW_log, DEBUG_PARAM_HW_IDX_NR);
	if(!ret) {
		g_warning("%s failed..", __FUNCTION__);
	}

	memset( _hw_param_panel_type, 0x00, sizeof(_hw_param_panel_type));
	memset( _hw_param_rc_type, 0x00, sizeof(_hw_param_rc_type));
	memset( _hw_param_vendor, 0x00, sizeof(_hw_param_vendor));
	memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
	memset( _hw_param_ubl_crc, 0x00, sizeof(_hw_param_ubl_crc));

	#if defined(SCHIP_COPY_PROTECTION)
		memset( _hw_param_serial_match, 0x00, sizeof(_hw_param_serial_match));
		memset( _hw_param_serial_rand, 0x00, sizeof(_hw_param_serial_rand));
	#endif

	if (!g_file_get_contents ( "/proc/cmdline" , &contents, &length, &error))
	{
		g_warning("%s %s\n", __FUNCTION__,  error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return 0;
	}

	if(contents)
	{
		char *tmp = NULL;
		g_message("%s cmdline[%s]", __FUNCTION__, contents);

		if((tmp = strstr(contents, NF_HW_PARAM_PANEL_TYPE)))
		{
			snprintf(_hw_param_panel_type, sizeof(_hw_param_panel_type),
						tmp + (strlen(NF_HW_PARAM_PANEL_TYPE)));
			tmp = strchr(_hw_param_panel_type, ' ');
			if ( tmp ) {
				*tmp = 0x0;
			}
			g_strstrip(_hw_param_panel_type);
		}

		if((tmp = strstr(contents, NF_HW_PARAM_RC_TYPE)))
		{
			snprintf(_hw_param_rc_type, sizeof(_hw_param_rc_type),
						tmp + (strlen(NF_HW_PARAM_RC_TYPE)));
			tmp = strchr(_hw_param_rc_type, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_hw_param_rc_type);
		}

		if((tmp = strstr(contents, NF_HW_PARAM_HW_VER)))
		{
			snprintf(_hw_param_hw_ver, sizeof(_hw_param_hw_ver),
						tmp + (strlen(NF_HW_PARAM_HW_VER)));
			tmp = strchr(_hw_param_hw_ver, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_hw_param_hw_ver);

			if (strncmp(_hw_param_hw_ver, "UTM7GN-16", sizeof("UTM7GN-16")) == 0)
			{
				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
				if(pba_type == HW_BOARD_TYPE_A)
					strncpy(_hw_param_hw_ver, "UTM7GNA-16", sizeof("UTM7GNA-16"));
				else
					strncpy(_hw_param_hw_ver, "UTM7GNB-16", sizeof("UTM7GNB-16"));
			}
			else if (strncmp(_hw_param_hw_ver, "UTM7GN-08", sizeof("UTM7GN-08")) == 0)
			{
				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
				if(pba_type == HW_BOARD_TYPE_A)
					strncpy(_hw_param_hw_ver, "UTM7GNA-08", sizeof("UTM7GNA-08"));
				else
					strncpy(_hw_param_hw_ver, "UTM7GNB-08", sizeof("UTM7GNB-08"));
			}
			else if (strncmp(_hw_param_hw_ver, "UTM7GN-04", sizeof("UTM7GN-04")) == 0)
			{
				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
				if(pba_type == HW_BOARD_TYPE_A)
					strncpy(_hw_param_hw_ver, "UTM7GNA-04", sizeof("UTM7GNA-04"));
				else
					strncpy(_hw_param_hw_ver, "UTM7GNB-04", sizeof("UTM7GNB-04"));
			}
		}

		if((tmp = strstr(contents, NF_HW_PARAM_VENDOR)))
		{
			snprintf(_hw_param_vendor, sizeof(_hw_param_vendor),
						tmp + (strlen(NF_HW_PARAM_VENDOR)));
			tmp = strchr(_hw_param_vendor, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_hw_param_vendor);
		}

		if((tmp = strstr(contents, NF_HW_PARAM_UBL_CRC)))
		{
			snprintf(_hw_param_ubl_crc, sizeof(_hw_param_ubl_crc),
						tmp + (strlen(NF_HW_PARAM_UBL_CRC)));
			tmp = strchr(_hw_param_ubl_crc, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_hw_param_ubl_crc);
		}
		if((tmp = strstr(contents, NF_HW_PARAM_FPGA_CRC)))
		{
			snprintf(_hw_param_fpga_crc, sizeof(_hw_param_fpga_crc),
						tmp + (strlen(NF_HW_PARAM_FPGA_CRC)));
			tmp = strchr(_hw_param_fpga_crc, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_hw_param_fpga_crc);
		}

		#if defined(SCHIP_COPY_PROTECTION)
			if((tmp = strstr(contents, NF_HW_PARAM_SERIAL_MATCH)))
			{
				snprintf(_hw_param_serial_match, sizeof(_hw_param_serial_match),
							tmp + (strlen(NF_HW_PARAM_SERIAL_MATCH)));
				tmp = strchr(_hw_param_serial_match, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hw_param_serial_match);
			}

			if((tmp = strstr(contents, NF_HW_PARAM_SERIAL_RAND)))
			{
				snprintf(_hw_param_serial_rand, sizeof(_hw_param_serial_rand),
							tmp + (strlen(NF_HW_PARAM_SERIAL_RAND)));
				tmp = strchr(_hw_param_serial_rand, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hw_param_serial_rand);
			}
			g_message("%s _hw_param_serial_match    [%s]", __FUNCTION__, _hw_param_serial_match);
			g_message("%s _hw_param_serial_rand    [%s]", __FUNCTION__, _hw_param_serial_rand);
		#endif
		#ifdef DEBUG_PARAM_HW_LOG
			if( _DEBUG_PARAM_HW_log[ DEBUG_PARAM_HW_IDX_HOTKEY ] )
			{
				g_message("%s _hw_param_hw_ver  [%s]", __FUNCTION__, _hw_param_hw_ver     );
				g_message("%s _hw_param_vendor  [%s]", __FUNCTION__, _hw_param_vendor     );
				g_message("%s _hw_param_panel_type [%s]", __FUNCTION__, _hw_param_panel_type );
				g_message("%s _hw_param_rc_type    [%s]", __FUNCTION__, _hw_param_rc_type);
				g_message("%s _hw_param_ubl_crc    [%s]", __FUNCTION__, _hw_param_ubl_crc);
				g_message("%s _hw_param_fpga_crc    [%s]", __FUNCTION__, _hw_param_fpga_crc);
			}
		#endif
		nf_sysdb_set_str("sys.info.hwver", _hw_param_hw_ver);

		g_free(contents);
	}

	_hw_param_init = 1;

	return 1;
}

const gchar *nf_api_param_hw_get_hwver(void)
{
	return _hw_param_hw_ver;
}
const gchar *nf_api_param_hw_get_vendor(void)
{
	return _hw_param_vendor;
}
const gchar *nf_api_param_hw_get_front_type(void)
{
	return _hw_param_panel_type;
}

const gchar *nf_api_param_hw_get_rc_type(void)
{
	return _hw_param_rc_type;
}

const gchar *nf_api_param_hw_get_ubl_crc(void)
{
	return _hw_param_ubl_crc;
}
const gchar *nf_api_param_hw_get_fpga_crc(void)
{
	return _hw_param_fpga_crc;
}

#if defined(SCHIP_COPY_PROTECTION)
const gchar *nf_api_param_hw_get_serial_match(void)
{
	return _hw_param_serial_match;
}   

const gchar *nf_api_param_hw_get_serial_rand(void)
{
	return _hw_param_serial_rand;
}   
#endif

gboolean nf_sysman_check_is_remocon(void)
{
	if (!strcmp(_hw_param_rc_type, "IRC200")) {
		printf("[%s] _hw_param_rc_type = %s\n", __FUNCTION__, _hw_param_rc_type);
		return 1;
	} else if (!strcmp(_hw_param_rc_type, "ICA_ITX")) {
		printf("[%s] _hw_param_rc_type = %s\n", __FUNCTION__, _hw_param_rc_type);
		return 1;
	}   
	
	return 0;
}   

gboolean nf_api_param_hw_set(NF_PARAM_HW *hwparam)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0, };
	gboolean ret=FALSE;
	guint offs =0;

	g_return_val_if_fail(dataBuf != NULL, 0);
	offs = 0;

	if(strncmp(hwparam->magic, NF_HW_PARAM_MAGIC, 4) != 0 ) {
		g_warning("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
		return FALSE;
	}
	memset(dataBuf, 0xff, sizeof(dataBuf));
	memcpy(dataBuf, hwparam, sizeof(NF_PARAM_HW));

	ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
	if(!ret) {
		g_warning("%s Set HW Param Nand Erase Error!!!", __FUNCTION__);
		return FALSE;
	}

	ret = nf_flash_page_write(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
	if(!ret) {
		g_warning("%s Set HW Param Error!!!", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

gboolean nf_api_param_hw_get(NF_PARAM_HW *hwparam, gboolean is_debug)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean ret=FALSE;
	guint offs=0;
	gint cnt_circular_buffer=0, cnt_qc_item=0;
	
	g_return_val_if_fail(dataBuf != NULL, 0);
	g_return_val_if_fail(hwparam != NULL, 0);
	
	offs=0;
	
	ret = nf_flash_read(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
	if(!ret) {
		g_warning("%s Get HW Param Error!!!", __FUNCTION__);
		return FALSE;
	}   
	
	memcpy(hwparam, dataBuf, sizeof(NF_PARAM_HW));
	
	if(strncmp(hwparam->magic, NF_HW_PARAM_MAGIC, 4) != 0 ) {
		g_warning("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
		return FALSE;
	}   
	
	if(is_debug) {
		g_message("Magic                 [%4.4s]", hwparam->magic);
		g_message("ETH ADDR              [%32.32s]",  hwparam->eth_addr);
		g_message("Front Type            [%32.32s]",  hwparam->pannel_type);
		g_message("RC    Type            [%32.32s]",  hwparam->rc_type);
		g_message("HW Version            [%32.32s]",  hwparam->hw_ver);
		g_message("Vendor                [%32.32s]",  hwparam->vendor);
		g_message("FW Write Fail         [%d]",  hwparam->is_fw_write_fail);
		g_message("Circular Buffer Count [%d]",  hwparam->circular_buffer_location);

		for(cnt_circular_buffer=0; cnt_circular_buffer<NF_SYSMAN_QC_CIRCULAR_BUFFER; cnt_circular_buffer++) {
			if(strncmp(hwparam->qc_result[cnt_circular_buffer].magic, NF_SYSMAN_QC_MAGIC, 4) != 0 ) {
				g_warning("%s QC Circular Buffer%dMagic Failed[%4.4s]", __FUNCTION__, cnt_circular_buffer, hwparam->magic);
				continue;
			}

			g_message("QC Magic              [%4.4s]",  hwparam->qc_result[cnt_circular_buffer].magic);
			g_message("Excute Time           [%48.48s]",  hwparam->qc_result[cnt_circular_buffer].excute_time);

			for(cnt_qc_item=0; cnt_qc_item<NF_SYSMAN_QC_ITEM_MAX_NR; cnt_qc_item++) {
				g_message("Excute Time           [%16.16s]",  hwparam->qc_result[cnt_circular_buffer].item[cnt_qc_item]);
			}

			g_print("\n");
		}
	}

	return TRUE;
}

gboolean nf_api_param_hw_get_protect(NF_PARAM_HW *hwparam, gboolean is_debug)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	NF_HW_PARAM_ENC_DATA dec_Buffer;
	gboolean ret=FALSE;
	guint offs=0; //M_NAND_HWPARAM_START_OFFS
	gint cnt_circular_buffer=0, cnt_qc_item=0;
	
	g_return_val_if_fail(dataBuf != NULL, 0);
	g_return_val_if_fail(hwparam != NULL, 0);
	
	ret = nf_flash_read(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
	if(!ret) {
		g_warning("%s Get HW Param Error!!!", __FUNCTION__);
		return FALSE;
	}   
	
	memcpy(&dec_Buffer.enc_data, dataBuf, sizeof(dec_Buffer.enc_data));
	nf_dev_board_pp_dec_data(&dec_Buffer);
	nf_sysman_set_vcode(hwparam->hw_ver);
	memcpy(hwparam, &dec_Buffer.dec_data, HW_PARAM_DECDATA_SIZE);
	
	if(strncmp(hwparam->magic, NF_HW_PARAM_MAGIC, 4) != 0 ) {
		g_warning("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
		return FALSE;
	}   
	
	if(is_debug) {
		g_message("Magic                 [%4.4s]", hwparam->magic);
		g_message("ETH ADDR              [%32.32s]",  hwparam->eth_addr);
		g_message("Front Type            [%32.32s]",  hwparam->pannel_type);
		g_message("RC    Type            [%32.32s]",  hwparam->rc_type);
		g_message("HW Version            [%32.32s]",  hwparam->hw_ver);
		g_message("Vendor                [%32.32s]",  hwparam->vendor);
	}   
	
	return TRUE;
}

void nf_api_param_hw_print(NF_PARAM_HW *hw_param)
{
	if(hw_param == NULL)
	{
		printf("\e[33m %s HW_PARAM is empty!!!\e[0m\n", __FUNCTION__);
		return;
	}
	printf("\e[33m %s magic         : %s\e[0m\n", __FUNCTION__,hw_param->magic);
	printf("\e[33m %s eth_addr      : %s\e[0m\n", __FUNCTION__,hw_param->eth_addr);
	printf("\e[33m %s pannel_type   : %s\e[0m\n", __FUNCTION__,hw_param->pannel_type);
	printf("\e[33m %s rc_type       : %s\e[0m\n", __FUNCTION__,hw_param->rc_type);
	printf("\e[33m %s hw_ver        : %s\e[0m\n", __FUNCTION__,hw_param->hw_ver);
	printf("\e[33m %s vendor        : %s\e[0m\n", __FUNCTION__,hw_param->vendor);
	#if defined(CONFIG_COPY_PROTECTION)
	printf("\e[33m %s serial_match  : %s\e[0m\n", __FUNCTION__,hw_param->serial_match);
	printf("\e[33m %s serial_rand   : %s\e[0m\n", __FUNCTION__,hw_param->serial_rand);
	#endif
}

#ifdef DEBUG_PARAM_HW_JBSHELL

static char nf_api_param_hw_jbshell_help[] = "sys_hw_param";
static int nf_api_param_hw_jbshell(int argc, char **argv)
{
	g_print("_factory_default  [%d]\n", _factory_default);
	g_print("_passwd_reset     [%d]\n", _passwd_reset   );
	g_print("_fwupgrade        [%d]\n", _fwupgrade      );
	g_print("_qa               [%d]\n", _qa             );
	g_print("_consol_enable    [%d]\n", _consol_enable  );
	g_print("_is_nfs           [%d]\n", _is_nfs     );
	g_print("_is_format        [%d]\n", _is_format     );

	g_print("_hw_param_hw_ver  [%s]\n", _hw_param_hw_ver     );
	g_print("_hw_param_vendor  [%s]\n", _hw_param_vendor     );
	g_print("_hw_param_ubl_crc [%s]\n", _hw_param_ubl_crc    );
	g_print("_hw_param_panel_type [%s]\n", _hw_param_panel_type );
	g_print("_hw_param_rc_type [%s]\n", _hw_param_rc_type );

	return 0;
}
__commandlist(nf_api_param_hw_jbshell,"sys_hw_param", nf_api_param_hw_jbshell_help, nf_api_param_hw_jbshell_help);

#endif

