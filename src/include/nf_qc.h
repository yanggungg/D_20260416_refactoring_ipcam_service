#ifndef __NF_QC_H__
#define __NF_QC_H__

#define NF_SYSMAN_QC_MAGIC						"ITXQ"
#define NF_SYSMAN_QC_RS485_OK					10
#define NF_SYSMAN_QC_RS485_TEXT					"QCTEST"
#define NF_SYSMAN_QC_RS485_TEXT_LEN				6

#define NF_SYSMAN_QC_CIRCULAR_BUFFER            3
#define NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN        16
#define NF_SYSMAN_QC_ITEM_MAX_NR                32

typedef struct _NF_SYSMAN_QC_RESULT_T
{
	gchar                    magic[4];              // 4
	gchar                    excute_time[48];       // 4 + 48 = 52
	gchar                    item[NF_SYSMAN_QC_ITEM_MAX_NR][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN];      // 52 + (32 * 16) = 52 + 512 = 564
} NF_SYSMAN_QC_RESULT;

#endif

