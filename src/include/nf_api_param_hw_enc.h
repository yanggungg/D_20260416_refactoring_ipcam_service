#ifndef __NF_API_PARAM_HW_ENC_H__
#define __NF_API_PARAM_HW_ENC_H__

#if defined(SCHIP_COPY_PROTECTION)
typedef struct _NF_HW_PARAM_ENC_DATA
{
	unsigned char enc_data[256];
	unsigned char dec_data[256];
	unsigned int enc_len;
	unsigned int dec_len;
} NF_HW_PARAM_ENC_DATA;

gboolean nf_sysman_get_protect_data(NF_HW_PARAM_ENC_DATA *data);
guchar *nf_param_hw_enc_get_qc_mac(void);
#endif

#endif

