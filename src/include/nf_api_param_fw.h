#ifndef __NF_API_PARAM_FW_H__
#define __NF_API_PARAM_FW_H__

typedef enum _NF_PARAM_FW_INFO_TYPE_E
{
	NF_PARAM_FW_INFO_TYPE_UBOOT	= 0,
	NF_PARAM_FW_INFO_TYPE_KERNEL	= 1,
	NF_PARAM_FW_INFO_TYPE_DSP		= 2,
	NF_PARAM_FW_INFO_TYPE_FILESYS	= 3,
	NF_PARAM_FW_INFO_TYPE_LOGO		= 4,
	NF_PARAM_FW_INFO_TYPE_FPGA		= 5,
	NF_PARAM_FW_INFO_TYPE_UBL		= 6,
	NF_PARAM_FW_INFO_TYPE_NR		= 7,
} NF_PARAM_FW_INFO_TYPE_E;

typedef struct _NF_PARAM_FW_INFO_T
{
	guint type;
	guint size;
	guint time;
	guint dcrc;
	gchar name[32];
} NF_PARAM_FW_INFO;


gboolean nf_sysman_fw_param_init(void);
gboolean nf_sysman_get_fw_param_info(NF_PARAM_FW_INFO *fw_info);

#endif

