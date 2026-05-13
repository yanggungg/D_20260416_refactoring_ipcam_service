#ifndef __NF_SYSMAN_H__
#define __NF_SYSMAN_H__

#include <glib.h>
#include "nf_util_fw.h"

gboolean nf_sysman_time_set( GTimeVal tval );
gboolean nf_sysman_time_check( void );

#if 1
gboolean nf_sysman_poweroff( gboolean is_reboot );
#else
gboolean nf_sysman_poweroff( void );
#endif

// gboolean nf_sysman_domestic_set_lic(void);
gboolean nf_sysman_hotkey_init( void );
gboolean nf_sysman_kcmdline_init(void);
gboolean nf_sysman_hotkey_is_factory_default( void );
gboolean nf_sysman_hotkey_is_passwd_reset( void );
gboolean nf_sysman_hotkey_is_fwupgrade( void );
gboolean nf_sysman_hotkey_is_qa( void );
gboolean nf_sysman_hotkey_is_console_en( void );
gboolean nf_sysman_hotkey_is_nfs( void );
gint nf_sysman_get_boot_mtd_num( void );
gboolean nf_sysman_hotkey_is_temp_upgrade( void );
gboolean nf_sysman_hotkey_is_format( void );

// gboolean nf_sysman_qcmode_init( void );
// gboolean nf_sysman_qcmode_sysdb_init(void);
// gboolean nf_sysman_qcmode_2nd_init(void);
// void nf_sysman_qc_set_signal(void);

// gboolean nf_sysman_qcmode_is_enable( void );
// gboolean nf_sysman_qcmode_is_factory_default( void );
// gboolean nf_sysman_qcmode_is_fwup( void );
// gboolean nf_sysman_qcmode_is_format( void );
// int nf_sysman_qcmode_gui_printf(  const char *format, ... );

uint nf_sysman_check_default_setting(void);

// gboolean nf_sysman_hw_param_init( void );
// const gchar *nf_sysman_get_qcmode_option_lang( void );
// const gchar *nf_sysman_get_hw_ver( void );
// const gchar *nf_sysman_get_match_model( void );
// gboolean nf_sysman_check_model_match( void );
// const gchar *nf_sysman_get_vendor( void );
// const gchar *nf_sysman_get_panel_type( void );
// const gchar *nf_sysman_get_rc_type( void );
// gboolean nf_sysman_check_is_remocon(void);
// const gchar *nf_sysman_get_ubl_crc( void );
// const gchar *nf_sysman_get_fpga_crc( void );

// gboolean nf_sysman_fwver_init( void );
guint nf_sysman_get_fwver_product( void );
guint nf_sysman_get_fwver_protocol( void );
guint nf_sysman_get_fwver_minor( void );
guint nf_sysman_get_fwver_vendor( void );
gchar *nf_sysman_get_fwver_option( void );
gint nf_sysman_get_vendor_from_db( void );
#if defined(SCHIP_COPY_PROTECTION)
	const gchar *nf_sysman_get_serial_match( void );
	const gchar *nf_sysman_get_serial_rand( void );
#endif

gboolean nf_sysman_sync( void );
gboolean nf_sysman_bdflush( void );
//gboolean nf_sysman_create_sysinfo(gchar *out_fullname);
gboolean nf_sysman_create_sysinfo(void);
gboolean nf_sysman_set_delack( int ack_cnt ); // 2014-02-10 ���� 2:57:03 choissi for  hw burst test

gboolean nf_sysman_telnet_ctrl_init();
gboolean nf_sysman_telnet_ctrl( void );
gboolean nf_sysman_telnet_stop( void );
gboolean nf_sysman_telnet_start( void );

gboolean nf_sysman_process_kill(gchar *processName, gchar *option);

#define NF_SYSTEM_BUFF_SIZE	1024

typedef struct _NF_SYSMAN_USAGE_INFO_T
{
	guint total;
	guint used;
} NF_SYSMAN_USAGE_INFO;

gboolean nf_sysman_get_mem_usage(NF_SYSMAN_USAGE_INFO* mem_info);
gboolean nf_sysman_get_cpu_usage(guint* cpu_idle);
gboolean nf_sysman_get_nand_usage(NF_SYSMAN_USAGE_INFO* nand_info);

#if defined (USE_DEV_PD69104B1)
	gboolean nf_dev_sysman_verify_open_mode(void);
#endif

typedef enum _NF_SYSMAN_PBA_TYPE_E
{
	NF_SYSMAN_PBA_TYPE_A = 0,
	NF_SYSMAN_PBA_TYPE_B = 1,
	NF_SYSMAN_PBA_TYPE_UNKNOWN = 2,
	NF_SYSMAN_PBA_TYPE_NR

} NF_SYSMAN_PBA_TYPE_E;

typedef enum _NF_DVR_STATUS_E
{
  NF_DVR_STATUS_INIT = 0,

  NF_DVR_STATUS_LIVE = 1,
  NF_DVR_STATUS_ZOOM = 2,				// ZOOM

  NF_DVR_STATUS_PLAYBACK = 3,
  NF_DVR_STATUS_RUN_PLAYBACK = 4,		// PLAY
  NF_DVR_STATUS_ARCHIVE = 5,
  NF_DVR_STATUS_RUN_ARCHIVE = 6,

  NF_DVR_STATUS_SETUP = 7,

  NF_DVR_STATUS_SHUTDOWN = 8,

  NF_DVR_STATUS_LIVE_RUN_ARCHIVE = 9,	// choissi 2009-11-24 ���� 2:13:54
  NF_DVR_STATUS_LIVE_AUTO_SYNC = 10,	// choissi 2009-11-24 ���� 2:13:54
  NF_DVR_STATUS_LIVE_FWUPGRADE = 11,	// choissi 2012-12-10 ���� 8:42:17

  NF_DVR_STATUS_PTZ = 12,

  NF_DVR_STATUS_NR

} NF_DVR_STATUS_E;

// typedef enum _NF_SYSMAN_DEF_VENDOR_E
// {
// 	VENDOR_VIDECON      = 28,
// 	VENDOR_S1           = 30,
// 	VENDOR_CBC			= 32,
// 	VENDOR_TAKENAKA     = 65,
// 	VENDOR_KOBI         = 76,
// 	VENDOR_KBDEVICE     = 78,
// 	VENDOR_ITX_46GUI    = 1400,

// 	VENDOR_UNKNOWN      = 0xFF,
// } NF_SYSMAN_DEF_VENDOR;

typedef enum _NF_SYSMAN_MODEL_E
{
	MODEL_IPXM4_4		= 0,
	MODEL_IPXM4_8		= 1,
	MODEL_IPXM4_16		= 2,
	MODEL_IPXP4E_8		= 3,
	MODEL_IPXP4E_16		= 4,	
	MODEL_IPXM4E_16		= 5,

	MODEL_UNKNOWN		= 0xFF,
} NF_SYSMAN_MODEL;

#define NF_APP_PARAM_MAGIC			"ITXA"
enum _NF_APP_PARAM_RES_E
{
	//AUTO
	ITX_DDC_PRI_UNKNOWN				= 0,
	// NTSC
	ITX_DDC_PRI_800_600_60          = 22,
	ITX_DDC_PRI_1080P_25			= 24,
	ITX_DDC_PRI_1280_1024_60		= 25,
	ITX_DDC_PRI_720P_60				= 26,
	ITX_DDC_PRI_1080P_50			= 27,       // For NTSC
	ITX_DDC_PRI_1080P_30			= 28,
	ITX_DDC_PRI_1080P_60			= 29,
	ITX_DDC_PRI_2560_1440_30		= 30,
	ITX_DDC_PRI_2560_1600_60		= 31,
	ITX_DDC_PRI_3840_2160_30		= 32,
	ITX_DDC_PRI_3840_2160_60		= 33,

	// PAL
	ITX_DDC_PRI_800_600_60_PAL      = 23,
	ITX_DDC_PRI_1280_1024_60_PAL	= 34,
	ITX_DDC_PRI_720P_60_PAL			= 35,
	ITX_DDC_PRI_1080P_30_PAL		= 36,
	ITX_DDC_PRI_1080P_60_PAL		= 37,
	ITX_DDC_PRI_1080P_25_PAL		= 38,
	ITX_DDC_PRI_1080P_50_PAL		= 39,       // For PAL
	ITX_DDC_PRI_2560_1440_30_PAL	= 40,
	ITX_DDC_PRI_2560_1600_60_PAL	= 41,
	ITX_DDC_PRI_3840_2160_30_PAL	= 42,
	ITX_DDC_PRI_3840_2160_60_PAL	= 43,
	ITX_DDC_PRI_2560_1440_25_PAL	= 44,
	ITX_DDC_PRI_2560_1600_50_PAL	= 45,
	ITX_DDC_PRI_3840_2160_25_PAL	= 46,
	ITX_DDC_PRI_3840_2160_50_PAL	= 47,

	ITX_DDC_PRI_NR
};

#if defined(ENABLE_MDIN_EDID)
typedef struct _ITX_MDIN_EDID_FRMT_INFO_T
{
	guchar frmt;
	gushort mode;
} ITX_MDIN_EDID_FRMT_INFO;
#endif

// typedef struct _ITX_DDC_INFO_DUAL_DISP_T
// {
// 	guint is_prev;
// 	guint output_mode;
// 	guint vga;
// 	guint hdmi;
// 	int   dual_type;
// 	int   monitor_main;
// 	int   reserved;
// } ITX_DDC_INFO_DUAL_DISP;

typedef enum _NF_SYSMAN_MONITOR_RES_E
{
	MONITOR_RES_800_600_60          = 0,
	MONITOR_RES_1280_1024_60        = 1,
	MONITOR_RES_720P_60             = 2,
	MONITOR_RES_1080P_25            = 3,
	MONITOR_RES_1080P_30            = 4,
	MONITOR_RES_1080P_50            = 5,
	MONITOR_RES_1080P_60            = 6,
	MONITOR_RES_2560_1440_30		= 7,
	MONITOR_RES_2560_1600_60		= 8,
	MONITOR_RES_3840_2160_30		= 9,
	MONITOR_RES_3840_2160_60		= 10,

	MONITOR_RES_UNKNOWN				= 11,

	MONITOR_RES_MAX

} NF_SYSMAN_MONITOR_RES;

typedef enum _NF_SYSMAN_MONITOR_DISPLAY_E
{
	MONITOR_DIS_VGA					= 0,
	MONITOR_DIS_HDMI				= 1,

	MONITOR_DIS_UNKNOWN				= 2		// parameter null

} NF_SYSMAN_MONITOR_DISPLAY;

typedef struct _NF_SYSMAN_MONITOR_INTO_T
{
	gint is_main;
	gint res_hdmi;
	gint res_vga;

} NF_SYSMAN_MONITOR_INTO;

typedef enum _NF_SYSMAN_DUAL_DISP_TYPE_E
{
	NF_SYSMAN_DUAL_DISP_TYPE_NONE	= 0,
	NF_SYSMAN_DUAL_DISP_TYPE_TIE	= 1,
	NF_SYSMAN_DUAL_DISP_TYPE_DUAL	= 2,
	NF_SYSMAN_DUAL_DISP_TYPE_CF		= 3,        // advertisement

	NF_SYSMAN_DUAL_DISP_TYPE_MAX
} NF_SYSMAN_DUAL_DISP_TYPE;

typedef enum _NF_SYSMAN_SHUTDOWN_TYPE_E
{
	NF_SYSMAN_SHUTDOWN_TYPE_ABNORMAL	= 0x00,
	NF_SYSMAN_SHUTDOWN_TYPE_NORMAL		= 0x01,
	NF_SYSMAN_SHUTDOWN_TYPE_STANDBY		= 0x02,

} NF_SYSMAN_SHUTDOWN_TYPE;

typedef enum _NF_SYSMAN_APP_PARAM_CATE_E
{
	NF_SYSMAN_APP_PARAM_CATE_IS_NORMAL 				= 0,
	NF_SYSMAN_APP_PARAM_CATE_IS_UPGRADE 			= 1,
	NF_SYSMAN_APP_PARAM_CATE_IS_PAL					= 2,
	NF_SYSMAN_APP_PARAM_CATE_SET_RESOLUTION			= 3

} NF_SYSMAN_APP_PARAM_CATE;

#define NF_SYSMAN_QC_MAX_LICENSE_CNT 16
typedef struct _NF_SYSMAN_LIC_PARAM_T
{
	int lic_cnt;
	char is_write;
	char license[NF_SYSMAN_QC_MAX_LICENSE_CNT][36];	//size 576
	char reserved[1467];    //2048 - 581
}NF_SYSMAN_LIC_PARAM;

// #define MAX_APP_PARAM_LEN 64
// typedef struct _NF_SYSMAN_APP_PARAM_T
// {
// 	gchar	magic[4];
// 	gint	is_normal;
// 	gint	is_upgrade;
// 	gint	is_pal;

// 	gchar	disk_info[4][20+40+1];
// 	guint	ddc_info[5];
// 	guchar  ddc_raw_data_vga[128+512];
// 	guchar  ddc_raw_data_hdmi[128+512];

// 	int     rsvd0[8];

// 	ITX_DDC_INFO_DUAL_DISP info_dual_disp;
// 	int     is_dual;
// 	int rsvd1[4];

// 	guint	val_aux;
// 	guint	val_res;
// 	guint	val_res_hdmi;

// 	#if defined(SUPPORT_NAND_SINGLE_PARTITION)
// 		guint fwup_magic_key;
// 		guint fwup_status;
// 		guint fwup_backup_size;
// 		char fw_file_name[MAX_APP_PARAM_LEN + MAX_APP_PARAM_LEN];
// 		char db_file_name[MAX_APP_PARAM_LEN];
// 		guint ipaddr;
// 		guint gateway;
// 		guint netmask;
// 		guint dns1;
// 		guint dns2;
// 	#endif

// 	guint is_enable_serial;
// 	int is_uboot_up;
// 	#if defined(SUPPORT_NAND_512M)
// 		guchar reserved[388 + 2048];
// 	#elif defined(SUPPORT_NAND_256M)
// 		#if defined(SUPPORT_NAND_SINGLE_PARTITION)
// 			guchar reserved[164];
// 		#else
// 			guchar reserved[388];
// 		#endif
// 	#elif defined(SUPPORT_NAND_128M)
// 		guchar  reserved[164];
// 	#endif
// } NF_SYSMAN_APP_PARAM;

#if defined(SUPPORT_UBIFS)
	#define NF_SYSMAM_UBIFS_PING	0
	#define NF_SYSMAM_UBIFS_PONG	1
	#define NF_SYSMAN_FS_DEV_NAME					"/dev/ubi%d_0"
	#define	NF_SYSMAN_FS_MOUNT_FS_TYPE				"ubifs"
#else
	#define NF_SYSMAN_FS_DEV_NAME					"/dev/mtdblock%d"
	#define	NF_SYSMAN_FS_MOUNT_FS_TYPE				"yaffs2"
#endif
#define NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME		"/mnt"
#define NF_SYSMAN_FS_MOUNT_DIR_NAME				"/mnt/prev_mtd"

typedef enum _NF_SYSMAN_FW_INFO_TYPE_E
{
    NF_SYSMAN_FW_INFO_TYPE_UBOOT    =   0,
    NF_SYSMAN_FW_INFO_TYPE_KERNEL   =   1,
    NF_SYSMAN_FW_INFO_TYPE_DSP      =   2,
    NF_SYSMAN_FW_INFO_TYPE_FILESYS  =   3,
    NF_SYSMAN_FW_INFO_TYPE_LOGO     =   4,
    NF_SYSMAN_FW_INFO_TYPE_FPGA     =   5,
    NF_SYSMAN_FW_INFO_TYPE_UBL      =   6,
    NF_SYSMAN_FW_INFO_TYPE_NR       =   7
} NF_SYSMAN_FW_INFO_TYPE_E;

typedef struct _NF_SYSMAN_FW_INFO_T
{
	guint type;
	guint size;
	guint time;
	guint dcrc;
	gchar name[32];
}NF_SYSMAN_FW_INFO;

////////////////////////////////////////////////////////////////////////////////////////
// Decoder
typedef enum _NF_SYSMAN_DECODER_IDX__E
{
	NF_SYSMAN_DECODER_CX26824       = 0,
	NF_SYSMAN_DECODER_CX26844       = 1,
	NF_SYSMAN_DECODER_CX26828       = 2,
	NF_SYSMAN_DECODER_CX26848       = 3,
	NF_SYSMAN_DECODER_CX25930       = 4,
	NF_SYSMAN_DECODER_IDEN1100      = 5,
	NF_SYSMAN_DECODER_TW2968        = 6,
	NF_SYSMAN_DECODER_TW2964        = 7,
	NF_SYSMAN_DECODER_TP2802        = 8,
	NF_SYSMAN_DECODER_NVP191X       = 9,
	NF_SYSMAN_DECODER_NVP6124       = 10,
	NF_SYSMAN_DECODER_EN334         = 11,

	NF_SYSMAN_DECODER_UNDEFINED     = 12
} NF_SYSMAN_DECODER_ITX;

typedef enum _NF_SYSMAN_SYSTEM_TYPE_E
{
	NF_SYSMAN_SYSTEM_TYPE0      = 0,        // TVI / AHD
	NF_SYSMAN_SYSTEM_TYPE1      = 1,        // TVI / HDSDI
	NF_SYSMAN_SYSTEM_TYPE2      = 2,        // AHD / HDSDI
	NF_SYSMAN_SYSTEM_TYPE3      = 3,        // NVR / AHD
	NF_SYSMAN_SYSTEM_TYPE4      = 4,        // NVR / TVI
	NF_SYSMAN_SYSTEM_TYPE5      = 5,        // NVR / HDSDI
	NF_SYSMAN_SYSTEM_TYPE6      = 6,        // TVI Only
	NF_SYSMAN_SYSTEM_TYPE7      = 7,        // AHD Only
	NF_SYSMAN_SYSTEM_TYPE8      = 8,        // HDSDI Only
	NF_SYSMAN_SYSTEM_TYPE9      = 9,        // NVR Only
	NF_SYSMAN_SYSTEM_TYPE10     = 10,       // EX-SDI Only

	NF_SYSMAN_SYSTEM_UNKNOWN    = 0xFF

} NF_SYSMAN_SYSTEM_TYPE;

/////////////////////////////////////////////////////////////////////////////////////////
// HW Parameter
// #define NF_SYSMAN_QC_CIRCULAR_BUFFER			3
// #define NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN		16
// #define NF_SYSMAN_QC_ITEM_MAX_NR				32

// #define NF_SYSMAN_QC_MAGIC						"ITXQ"
// #define NF_SYSMAN_QC_RS485_OK					10
// #define NF_SYSMAN_QC_RS485_TEXT					"QCTEST"
// #define NF_SYSMAN_QC_RS485_TEXT_LEN				6
// typedef struct _NF_SYSMAN_QC_RESULT_T
// {
// 	gchar                    magic[4];
// 	gchar                    excute_time[48];
// 	gchar                    item[NF_SYSMAN_QC_ITEM_MAX_NR][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN];
// } NF_SYSMAN_QC_RESULT;

// #define NF_SYSMAN_HW_PARAM_MAX_LEN				32
// #define NF_SYSMAN_HW_PARAM_MAGIC				"ITXB"
// typedef struct _NF_SYSMAN_HW_PARAM_T
// {
// 	gchar    magic[4];
// 	gchar    eth_addr[NF_SYSMAN_HW_PARAM_MAX_LEN];
// 	gchar    pannel_type[NF_SYSMAN_HW_PARAM_MAX_LEN];
// 	gchar    rc_type[NF_SYSMAN_HW_PARAM_MAX_LEN];          /** add by pakkhman 090811 **/
// 	gchar    hw_ver[NF_SYSMAN_HW_PARAM_MAX_LEN];
// 	gchar    vendor[NF_SYSMAN_HW_PARAM_MAX_LEN];

// 	image_header_t fpga_img_header;

// 	gint     is_fw_write_fail;

// 	gint circular_buffer_location;
// 	NF_SYSMAN_QC_RESULT       qc_result[NF_SYSMAN_QC_CIRCULAR_BUFFER];

// 	gint     is_eth_write;
// 	#if defined(SUPPORT_NAND_512M)
// 		guchar reserved[116 + 2048];										// 2048 - 1932 = 116
// 	#elif defined(SUPPORT_NAND_256M)
// 		guchar reserved[116];										// 2048 - 1932 = 116
// 	#elif defined(SUPPORT_NAND_128M)
// 		guchar reserved[116];												// 2048 - 1932 = 116
// 	#endif
// } NF_SYSMAN_HW_PARAM;
/////////////////////////////////////////////////////////////////////////////////////////

// gboolean nf_sysman_get_app_param(NF_SYSMAN_APP_PARAM *app_param);
// gboolean nf_sysman_set_hw_param(NF_SYSMAN_HW_PARAM *hwparam);
// gboolean nf_sysman_get_hw_param(NF_SYSMAN_HW_PARAM *hwparam, gboolean is_debug);
gboolean nf_sysman_set_normal_boot(void);
gboolean nf_sysman_clr_normal_boot(void);
gboolean nf_sysman_is_normal_boot(void);
gboolean nf_sysman_is_fwup_first_boot(void);
gboolean nf_sysman_set_standby_boot(void);
gboolean nf_sysman_fwup_mount_prev_mtd(void);
gboolean nf_sysman_fwup_bit_clr(void);
gboolean nf_sysman_check_sig_type(void);

// 0 : HDMI2, 1 : VGA
gint nf_sysman_get_hdmi2_vga_out(void);
// 0 : HDMI2, 1 : VGA
gboolean nf_sysman_set_hdmi2_vga_out(gint val);

// gboolean nf_sysman_set_app_param(NF_SYSMAN_APP_PARAM *app_param);
// gboolean nf_sysman_set_app_param_upgrade_val(NF_SYSMAN_APP_PARAM *app_param, gboolean is_upgrade);
// gboolean nf_sysman_set_app_param_cate(gint cate, gint val);
gboolean nf_sysman_user_set_resolution(guint val_res);
gboolean nf_sysman_user_set_resolution_dual(gint res_hdmi, gint res_vga);

int nf_sysman_user_get_resolution(void);
gboolean nf_sysman_fw_param_init(void);
// gboolean nf_sysman_get_fw_param_info(NF_SYSMAN_FW_INFO *fw_info);
gboolean nf_sysman_cmdline_init(void);
gchar *nf_sysman_get_resolution(void);
gchar *nf_sysman_get_output_mode(void);
gchar *nf_sysman_get_output_mode_live(void);
guint *nf_sysman_get_serial_enable(void);

//support dual display
gint nf_sysman_dual_display_mode();
gchar *nf_sysman_get_hdmi_resolution(void);
gchar *nf_sysman_get_vga_resolution(void);

guint nf_sysman_get_pba_type(void);

#if defined(_SUPPORT_DUALMONITOR)
	gboolean nf_sysman_set_dual_display(gboolean is_dual);
	gboolean nf_sysman_dual_disp_cmp_flag(void);
#endif

gboolean nf_sysman_gui_tunning(void);

#if defined(_HDI_0412)
#define HDI_VIDEO_INFO_PARAM0		"CH%d_INFO_PARAM0="
#define HDI_VIDEO_INFO_PARAM1		"CH%d_INFO_PARAM1="
#define HDI_VIDEO_INFO_PARAM2		"CH%d_INFO_PARAM2="
#define HDI_VIDEO_INFO_PARAM3		"CH%d_INFO_PARAM3="
#define HDI_VIDEO_INFO_PARAM4		"CH%d_INFO_PARAM4="
#define HDI_VIDEO_INFO_PARAM5		"CH%d_INFO_PARAM5="
#define HDI_VIDEO_INFO_PARAM6		"CH%d_INFO_PARAM6="
#define HDI_VIDEO_INFO_PARAM7		"CH%d_INFO_PARAM7="

typedef struct _NF_HDI_VIDEO_INFO_T
{
	gchar type_name[32];
	gchar rate_det1[8];
	gchar rate_det0[8];
	gchar lines_per_field[8];
	gchar active_lines_per_field[8];
	gchar words_per_active_line[8];
	gchar words_per_line[8];
	guchar value;

} NF_HDI_VIDEO_INFO;

gboolean nf_sysman_hdi_video_info_init(void);
void nf_sysman_get_hdi_video_info(NF_HDI_VIDEO_INFO *hdi_video_info);
#endif  /*_HDI_0412*/

#if 0
gboolean nf_sysman_set_dvr_status(  NF_DVR_STATUS_E status );
NF_DVR_STATUS_E nf_sysman_get_dvr_status( void );
#endif

int nf_sysman_qrcode_gen(char *filename, char *ext, char *msg);

typedef struct _NF_SYSMAN_REBOOT_INFO{
	guint error_code;
	gchar content[256];
} NF_SYSMAN_REBOOT_INFO;

gboolean nf_sysman_get_reboot_info(NF_SYSMAN_REBOOT_INFO *info);

typedef enum _NF_SYSMAN_HUB_INFO_E
{
	NF_SYSMAN_HUB_INFO_LINKED_MASK	= 0,
	NF_SYSMAN_HUB_INFO_LINKED_NUM	= 1,
} NF_SYSMAN_HUB_INFO_E;

guint nf_sysman_get_hub_info(int info_idx);

typedef enum _NF_SYSMAN_SWITCH_VCT_RESULT_E
{
	NF_SYSMAN_SWITCH_VCT_PASS      = 0,
	NF_SYSMAN_SWITCH_VCT_FAIL      = 1,
	NF_SYSMAN_SWITCH_VCT_OPEN      = 2,
	NF_SYSMAN_SWITCH_VCT_SHORT     = 3,
} NF_SYSMAN_SWITCH_VCT_RESULT_E;

typedef struct _NF_SYSMAN_SWITCH_VCT_RES
{
	guint dev_num; // 0:NVR, 1:hub1, 2:hub2, 3:hub3, 4:hub4
	guint port; // 0~7:CAM, 8:WAN, 9:LAN
	char result;
	int length;
	guint cable_num; // bitmask
} NF_SYSMAN_SWITCH_VCT_RES;

// typedef enum _NF_SYSMAN_QC_PD_ERROR_E
// {
// 	NF_SYSMAN_QC_PD_ERROR_OK			= 0,
// 	NF_SYSMAN_QC_PD_ERROR_NETWORK		= 1,
// 	NF_SYSMAN_QC_PD_ERROR_NO_USB		= 2,
// 	NF_SYSMAN_QC_PD_ERROR_NO_DATADIR	= 3,
// 	NF_SYSMAN_QC_PD_ERROR_NO_DATA		= 4,
// 	NF_SYSMAN_QC_PD_ERROR_DUPLICATION	= 5,
// 	NF_SYSMAN_QC_PD_ERROR_WRONG_DATA	= 6,
// 	NF_SYSMAN_QC_PD_ERROR_ETC			= 7,
// 	NF_SYSMAN_QC_PD_ERROR_SET_DB		= 8,
// 	NF_SYSMAN_QC_PD_ERROR_NR			= 9,
// } NF_SYSMAN_QC_PD_ERROR_E;
// typedef enum _NF_SYSMAN_QC_LIC_ERROR_E
// {
// 	NF_SYSMAN_QC_LIC_ERROR_NETWORK			= -1,
// 	NF_SYSMAN_QC_LIC_ERROR_WRONG_DATA		= -2,
// 	NF_SYSMAN_QC_LIC_ERROR_MAC_DATA			= -3,
// 	NF_SYSMAN_QC_LIC_ERROR_NO_DATA			= -4,
// 	NF_SYSMAN_QC_LIC_ERROR_SET_DB			= -5,
// 	NF_SYSMAN_QC_LIC_ERROR_MEMORY			= -6,
// } NF_SYSMAN_QC_LIC_ERROR_E;
// guint nf_sysman_qc_client(void);
// guint nf_sysman_qc_pc_client(void);
int get_qc_mac_writer_mode(void);
// guint nf_sysman_qc_lic_write(char key[][36]);
int nf_sysman_malloc_free_key( int mode, int keyCnt, char **arrKey);
gboolean nf_sysman_analize_nand_log(void);
gboolean nf_sysman_switch_vct_result(NF_SYSMAN_SWITCH_VCT_RES *vct_info);

/* APP_QC_TEST */
#if defined(_IPX_1648M4)
     #define QC_TEST_ALARM_ON_VAL           0x1
     #define QC_TEST_ALARM_OFF_VAL          0x2
     #define QC_TEST_AUDIO_NUM              1
#elif defined(_IPX_1648M4E) || defined(_IPX_32M4E)
     #define QC_TEST_ALARM_ON_VAL           0x1
     #define QC_TEST_ALARM_OFF_VAL          0x2
     #define QC_TEST_AUDIO_NUM              1	
#elif defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
     #define QC_TEST_ALARM_ON_VAL           0x3ff55
     #define QC_TEST_ALARM_OFF_VAL          0xaa
     #define QC_TEST_AUDIO_NUM              1
     #define DEV_QC_RS232_NAME "/dev/ttyAMA0"
#elif defined(_IPX_0824P4E)
     #define QC_TEST_ALARM_ON_VAL           0x30055
     #define QC_TEST_ALARM_OFF_VAL          0xaa
     #define QC_TEST_AUDIO_NUM              1    
#else //_IPX_32P5
	#define QC_TEST_ALARM_ON_VAL            0x1
	#define QC_TEST_ALARM_OFF_VAL           0x2
	#define QC_TEST_AUDIO_NUM               4
	#define QC_TEST_ALARM_NUM               2
	#define QC_TEST_FACTORY_KEY_GPIO_BASE   2 //imsi
	#define QC_TEST_FACTORY_KEY_GPIO_PIN    20 //imsi
#endif


/* APP_QC_TEST */
// gboolean nf_sysman_qc_auto_test_alarm(void);
// gboolean nf_sysman_qc_auto_test_rs485(void);
// gboolean nf_sysman_qc_auto_test_audio(void);
// gboolean nf_sysman_qc_auto_test_rs232(void);
gboolean nf_sysman_qc_auto_test_poe(void);
// gboolean nf_sysman_qc_auto_test_fan(void);
// gboolean nf_sysman_qc_auto_test_temper(void);
gboolean nf_sysman_qc_auto_test_network_switch(void);
// guint nf_sysman_hdd_num_check(void);
// guint nf_sysman_odd_num_check(void);
// gboolean nf_sysman_qc_info_rtc(char *rtc_info);
// void nf_sysman_qc_manual_test_rs485(int is_on);
// gboolean nf_sysman_qc_result_save(gchar item[][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN], int item_total_num);
// gboolean nf_sysman_qc_get_ntp_serverip(char *serverip);
// gboolean nf_sysman_qc_factory_default(void);
// gboolean nf_sysman_qc_manual_test_factory_key(void);
gboolean nf_sysman_is_support_factory_key(void);
// void nf_sysman_set_qc_live_audio(guint is_on);
// guint nf_sysman_get_qc_live_audio(void);
// guchar *nf_sysman_get_qc_mac(void);
// gint nf_sysman_qc_atoi_mac(gchar *mac, int size);

/* END OF APP_QC_TEST */
#endif
