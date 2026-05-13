#ifndef __NF_API_PARAM_APP_H__
#define __NF_API_PARAM_APP_H__

#define NF_APP_PARAM_MAGIC			"ITXA"
#define APP_PARAM_STR_64			64

typedef enum _DEBUG_PARAM_APP_IDX_E
{
	DEBUG_PARAM_APP_IDX_DUMP		= 0,
	DEBUG_PARAM_APP_IDX_NR			= 1

} DEBUG_PARAM_APP_IDX_E;

typedef struct _ITX_DDC_INFO_DUAL_DISP_T
{   
	guint	is_prev;
	guint	output_mode;
	guint	vga;
	guint	hdmi;
	gint    dual_type;
	gint    monitor_main;
	gint    reserved;
} ITX_DDC_INFO_DUAL_DISP;

typedef enum _NF_PARAM_APP_CATE_E
{
	NF_PARAM_APP_CATE_IS_NORMAL              = 0,
	NF_PARAM_APP_CATE_IS_UPGRADE             = 1,
	NF_PARAM_APP_CATE_IS_PAL                 = 2,
	NF_PARAM_APP_CATE_DUAL                   = 3,
	NF_PARAM_APP_CATE_DUAL_TYPE              = 4,
	NF_PARAM_APP_CATE_SET_MONITOR_MAIN       = 5,
	NF_PARAM_APP_CATE_SET_RESOLUTION         = 6,
	NF_PARAM_APP_CATE_SET_RESOLUTION_HDMI    = 7,
	NF_PARAM_APP_CATE_SET_FWUP_STATUS        = 8,
	NF_PARAM_APP_CATE_SET_SERIAL             = 9,
	NF_PARAM_APP_CATE_IS_MICOM_UPGRADE		 = 10,
	NF_PARAM_APP_CATE_HDMI2_VGA_OUT          = 11

} NF_PARAM_APP_CATE;

typedef struct _NF_PARAM_APP_T
{
	gchar   magic[4];
	gint    is_normal;
	gint    is_upgrade;
	gint    is_pal;

	gchar   disk_info[4][20+40+1];
	guint   ddc_info[5];
	guchar  ddc_raw_data_vga[128+512];
	guchar  ddc_raw_data_hdmi[128+512];

	int     rsvd0[8];

	ITX_DDC_INFO_DUAL_DISP info_dual_disp;
	int     is_dual;

	gint    rsvd[4];

	guint hdmi2_vga_out; // ksi_test val_aux -> hdmi2_vga_out 변경, 0 : hdmi2, 1 : VGA
	guint val_res;          // vga
	guint val_res_hdmi;     // for dual display

	#if defined(CONFIG_FWUPGRADE_SINGLE)
		guint fwup_magic_key;
		guint fwup_status;
		guint fwup_backup_size;
		char fw_file_name[APP_PARAM_STR_64 + APP_PARAM_STR_64];
		char db_file_name[APP_PARAM_STR_64];
		guint ipaddr;
		guint gateway;
		guint netmask;
		guint dns1;
		guint dns2;
	#endif

	// guint is_enable_serial;
	guint is_micom_up;

	#if defined(SUPPORT_NAND_512M)
		guchar  reserved[168+2048];
	#elif defined(SUPPORT_NAND_256M) || defined(SUPPORT_NAND_128M)
		guchar  reserved[168];
	#endif

}NF_PARAM_APP;

gboolean nf_api_param_app_init(void);
gboolean nf_api_param_app_set(NF_PARAM_APP *app_param);
gboolean nf_api_param_app_get(NF_PARAM_APP *app_param);
gboolean nf_api_param_app_set_cate(gint cate, gint val);
gboolean nf_api_param_app_get_cate(gint cate, gint *val);
gboolean nf_api_param_app_set_upgrade_val(NF_PARAM_APP *app_param, gboolean is_upgrade);

#endif

