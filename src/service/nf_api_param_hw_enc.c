#if defined(SCHIP_COPY_PROTECTION)
#include "nf_common.h"

#include "itx_seed_algo.h"

#include "nf_qc.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_hw_enc.h"

#include "nf_util_device.h"
#include "nf_util_flash.h"

#include "nf_hw.h"

/*
   Gloval Variable Definition
 */
static guchar _nf_param_hw_enc_qc_mac[32];

/*
   Extern Function Definition
 */
extern gboolean nf_dev_board_pp_dec_data(NF_HW_PARAM_ENC_DATA *data);

gboolean nf_sysman_get_protect_data(NF_HW_PARAM_ENC_DATA *data)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean ret=FALSE;
	guint offs=0;  //M_NAND_HWPARAM_START_OFFS

	ret = nf_flash_read(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
	if (!ret) {
		printf("\e[33m [%s] Get Protect Data From Nand Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}

	memcpy(data->enc_data, dataBuf, sizeof(data->enc_data));

	nf_dev_board_pp_dec_data(data);

	if (strncmp(data->dec_data, ITX_PROTECT_DATA_PARAM_MAGIC, 4) != 0 ) {
		printf("\e[33m [%s] Magic Check Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

gboolean nf_sysman_set_protect_data(NF_HW_PARAM_ENC_DATA *data)
{
	guchar dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE]={0,};
	gboolean ret=FALSE;
	guint offs=0; //M_NAND_HWPARAM_START_OFFS

	g_return_val_if_fail(dataBuf != NULL, 0);
	g_return_val_if_fail(data != NULL, 0);

	memcpy(dataBuf, data->enc_data, sizeof(data->enc_data));

	ret = nf_flash_page_write(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
	if (!ret) {
		printf("\e[33m [%s] Set Protect Data Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

gboolean nf_sysman_check_protect_data_param(ITX_PROTECT_DATA_PARAM *data_param, char *data)
{
	int i = 0;
	int pba_type=nf_hw_get_board_type();
	char hw_info[30], data_info[30];
	const gchar *str=NULL;
	
	printf("\e[33m [%s] data_param->eth_addr = %s\e[0m\n",
			__FUNCTION__, data_param->eth_addr);
	printf("\e[33m [%s] data_param->pannel_type = %s\e[0m\n",
			__FUNCTION__, data_param->pannel_type);
	printf("\e[33m [%s] data_param->rc_type = %s\e[0m\n",
			__FUNCTION__, data_param->rc_type);
	printf("\e[33m [%s] data_param->hw_ver = %s\e[0m\n",
			__FUNCTION__, data_param->hw_ver);
	printf("\e[33m [%s] data_param->serial_match = %s\e[0m\n",
			__FUNCTION__, data_param->serial_match);
	printf("\e[33m [%s] data_param->serial_rand = %s\e[0m\n",
			__FUNCTION__, data_param->serial_rand);
			
	memset(data_info, 0x00, sizeof(data_info));
	for (i = 0; i < 17; i++) {
		if (data_param->eth_addr[i] == ':') {
			data_info[i] = ':';
			continue;
		}   
		data_info[i] = data[i];
	}   
	
	if (strcasecmp(data_param->eth_addr, data_info) != 0) {
		printf("\e[33m [%s] Check Mac Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}   
	
	if (strncmp(data_param->pannel_type, "FIPN501", sizeof("FIPN501")) != 0) {
		printf("\e[33m [%s] Check PT Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}   
	
	if (strncmp(data_param->rc_type, "ICA_ITX", sizeof("ICA_ITX")) != 0) {
		printf("\e[33m [%s] Check RC Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}   
	
	memset(hw_info, 0x00, sizeof(hw_info));
	#if defined(_UTM7G_1648D)
		strcpy(hw_info, "UTM7GN-16");
	//if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "TAKENAKA") == 0) strcpy(hw_info, "UTM7GN-T-16");
	#elif defined(_UTM7G_0824D)
		strcpy(hw_info, "UTM7GN-08");
	#elif defined(_UTM7G_0412D)
		strcpy(hw_info, "UTM7GN-04")
	#elif defined(_ANF8G_1648D)
		strcpy(hw_info, "ANF8GN-16");
	#elif defined(_IPX_32P5)
		strcpy(hw_info, "IPXP5-32");//IPXP5-32
	#endif
	if (strncmp(data_param->hw_ver, hw_info, sizeof(data_param->hw_ver)) != 0) {
		printf("\e[33m [%s] Check HW VER Fail\e[0m\n", __FUNCTION__);
		return FALSE;
	}

	//hw_ver name modify(a/b type display)
	#if defined(_UTM7G_1648D)
		if(pba_type == HW_BOARD_TYPE_A)
			strcpy(data_param->hw_ver, "UTM7GNA-16");
		else
			strcpy(data_param->hw_ver, "UTM7GNB-16");
	#elif defined(_UTM7G_0824D)
		if(pba_type == HW_BOARD_TYPE_A)
			strcpy(data_param->hw_ver, "UTM7GNA-08");
		else
			strcpy(data_param->hw_ver, "UTM7GNB-08");
	#elif defined(_UTM7G_0412D)
		if(pba_type == HW_BOARD_TYPE_A)
			strcpy(data_param->hw_ver, "UTM7GNA-04");
		else
			strcpy(data_param->hw_ver, "UTM7GNB-04");
	#elif defined(_ANF8G_1648D)

	#elif defined(_IPX_32P5)
	
	#endif

	str=nf_api_param_hw_get_hwver();
	strncpy((char *)str, data_param->hw_ver, sizeof(data_param->hw_ver));
	str=nf_api_param_hw_get_front_type();
	strncpy((char *)str, data_param->pannel_type, sizeof(data_param->pannel_type));
	str=nf_api_param_hw_get_rc_type();
	strncpy((char *)str, data_param->rc_type, sizeof(data_param->rc_type));
	snprintf(_nf_param_hw_enc_qc_mac, sizeof(_nf_param_hw_enc_qc_mac), "%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
			data_param->eth_addr[0], data_param->eth_addr[1],
			data_param->eth_addr[3], data_param->eth_addr[4],
			data_param->eth_addr[6], data_param->eth_addr[7],
			data_param->eth_addr[9], data_param->eth_addr[10],
			data_param->eth_addr[12], data_param->eth_addr[13],
			data_param->eth_addr[15], data_param->eth_addr[16],
			data_param->eth_addr[18], data_param->eth_addr[19]);

	printf("\e[33m [%s] HW VER : %s\e[0m\n", __FUNCTION__, nf_api_param_hw_get_hwver());
	printf("\e[33m [%s] FRONT  : %s\e[0m\n", __FUNCTION__, nf_api_param_hw_get_front_type());
	printf("\e[33m [%s] RC     : %s\e[0m\n", __FUNCTION__, nf_api_param_hw_get_rc_type());
	printf("\e[33m [%s] MAC    : %s\e[0m\n", __FUNCTION__, nf_param_hw_enc_get_qc_mac());

	return TRUE;
}

guchar *nf_param_hw_enc_get_qc_mac(void)
{
	return _nf_param_hw_enc_qc_mac;
}

#endif

