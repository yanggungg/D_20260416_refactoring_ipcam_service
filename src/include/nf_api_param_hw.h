#ifndef __NF_API_PARAM_HW_H__
#define __NF_API_PARAM_HW_H__

#include "nf_util_fw.h"
#include "nf_qc.h"

#define NF_HW_PARAM_PANEL_TYPE			"pannel_type="
#define NF_HW_PARAM_RC_TYPE				"rc_type="
#define NF_HW_PARAM_HW_VER				"hw_ver="
#define NF_HW_PARAM_VENDOR				"vendor="
#define NF_HW_PARAM_UBL_CRC				"crc_ubl="
#define NF_HW_PARAM_FPGA_CRC			"crc_fpga="
#if defined(SCHIP_COPY_PROTECTION)
	#define NF_HW_PARAM_SERIAL_MATCH		"serial_match="
	#define NF_HW_PARAM_SERIAL_RAND			"serial_rand="
#endif

typedef enum _DEBUG_PARAM_HW_IDX_E
{
	DEBUG_PARAM_HW_IDX_HOTKEY	= 0,
	DEBUG_PARAM_HW_IDX_NR		= 1

} DEBUG_PARAM_HW_IDX_E;

#define HW_PARAM_DECDATA_SIZE				164 //(magic+eth+pannel+rc+hwver+vendor) size
#define NF_HW_PARAM_STR_32					32
#define NF_HW_PARAM_MAGIC					"ITXR"

typedef struct _NF_PARAM_HW_T
{
	gchar    magic[4];							// 4
	gchar    eth_addr[NF_HW_PARAM_STR_32];			// 4 + 32 = 36
	gchar    pannel_type[NF_HW_PARAM_STR_32];		// 36 + 32 = 68
	gchar    rc_type[NF_HW_PARAM_STR_32];			// 68 + 32 = 100          /** add by pakkhman 090811 **/
	gchar    hw_ver[NF_HW_PARAM_STR_32];			// 100 + 32 = 132
	gchar    serial_match[NF_HW_PARAM_STR_32];
	gchar    serial_rand[NF_HW_PARAM_STR_32];

	u_char    reserved[44];

	// For Vendor Code
	char    vendor[NF_HW_PARAM_STR_32];

	image_header_t fpga_img_header; // For FPGA Upgrade                 // 164 + 64 = 228

	gint     is_fw_write_fail;      // For Factory Program              // 228 + 4 = 232

	// For QC
	gint circular_buffer_location;                                      // 232 + 4 = 236
	NF_SYSMAN_QC_RESULT       qc_result[NF_SYSMAN_QC_CIRCULAR_BUFFER];  // 236 + (564 * 3) = 236 + 1692 = 1928

	#if defined(SUPPORT_NAND_512M)
		guchar  reserved1[2076];            // 4096 - 2076 = 2020
	#elif defined(SUPPORT_NAND_256M)
		guchar  reserved1[11];              // 2048 - 2037 = 11
	#elif defined(SUPPORT_NAND_128M)
		guchar  reserved1[11];              // 2048 - 2037 = 11
	#endif
} NF_PARAM_HW;


const gchar *nf_api_param_hw_get_hwver(void);
const gchar *nf_api_param_hw_get_vendor(void);
const gchar *nf_api_param_hw_get_front_type(void);
const gchar *nf_api_param_hw_get_rc_type(void);
const gchar *nf_api_param_hw_get_ubl_crc(void);
const gchar *nf_api_param_hw_get_fpga_crc(void);
#if defined(_UTM5X_1648D) || defined(_UTM5X_0824D) || defined(_UTM5X_0412D)
    #define TEST_CODE_COPY_PROTECTION
#endif
#if defined(SCHIP_COPY_PROTECTION)
	const gchar *nf_api_param_hw_get_serial_match(void);
	const gchar *nf_api_param_hw_get_serial_rand(void);
#endif
gboolean nf_api_param_hw_set(NF_PARAM_HW *hwparam);
gboolean nf_api_param_hw_get(NF_PARAM_HW *hwparam, gboolean is_debug);
gboolean nf_api_param_hw_get_protect(NF_PARAM_HW *hwparam, gboolean is_debug);
void nf_api_param_hw_print(NF_PARAM_HW *app_param);

#endif

