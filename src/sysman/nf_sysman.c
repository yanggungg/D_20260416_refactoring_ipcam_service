#include "nf_common.h"

#include "libsst.h"

#include "nf_qc.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_app.h"
#include "nf_api_param_fwver.h"

#include "nf_sysman.h"
#include "nf_debug.h"
#include "nf_sysdb.h"

#include "nf_notify.h"

#include "nf_network.h"
#include "nf_record.h"
#include "nf_api_disk.h"
#include "nf_watchdog.h"

#include "nf_util_time.h"
#include "nf_util_device.h"
#include "nf_util_flash.h"
#include "nf_util_fw.h"
#include "nf_api_eventlog.h"
#include "../service/nf_ipcam_hub.h"
#include "nf_ptz.h"
#include "nf_pos.h"
#include "nf_qc_audio.h"
#include "nf_ipcam_defs.h"
#include "itx_seed_algo.h"
#include "nfdal.h"

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <stdarg.h>
#include <net/if.h> 

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "sysman"

#define DEBUG_SYSMAN_JBSHELL
#define DEBUG_SYSMAN_LOG
//#define DEBUG_SYSMAN_APP_PARAM
#define DEBUG_SYSMAN_QC_LIC
//#define DEBUG_SYSMAN_QC_LIC_MAC

#ifdef DEBUG_SYSMAN_JBSHELL
	#include "jbshell.h"
#endif

int qcmode_enable; 
// int qc_audio_manual_enable;
// static void nf_sysman_print_hw_param(NF_SYSMAN_HW_PARAM *hw_param);
static void _set_sysdb_sig_type(gboolean sig_type);
// static gboolean _get_fw_param_from_nand(NF_FW_IMAGE_LIST *fw_param);
extern gboolean nf_dspcomm_sync_time(void);
extern int proxy_system(const char *str, int mode, int timeout_sec);

#if defined(_HDI_0412)
	gboolean nf_sysman_hdi_video_info_init(void);
	NF_HDI_VIDEO_INFO _hdi_video_info[NUM_ACTIVE_CH];
#endif

static NF_SYSMAN_FW_INFO _fw_param_info[16];

typedef enum _VCODE_IDX{ 
	IDX_NONE = 0, 
	IDX_ITX, IDX_GPS, IDX_VDC, 
	IDX_ORI, IDX_CBC, IDX_TKK,
	IDX_G4S, IDX_VCN, IDX_HON,
	IDX_VIT, IDX_ASP, IDX_I3D,
	IDX_CYT, IDX_DAU, IDX_IRL,
}VCODE_IDX;

static char strVcode[][4] = {
	"ITX","GPS","VDC",
	"ORI","CBC","TKK",
	"G4S","VCN","HON",
	"VIT","ASP","I3D",
	"CYT","DAU","IRL",
};

typedef enum _DEBUG_SYSMAN_IDX_E
{
	DEBUG_SYSMAN_IDX_INIT				= 0,
	DEBUG_SYSMAN_IDX_HOTKEY				= 1,
	DEBUG_SYSMAN_IDX_KENRLCMDLINE			= 2,
	DEBUG_SYSMAN_IDX_NR					= 3

}DEBUG_SYSMAN_IDX_E;

static const char *_DEBUG_SYSMAN_str[32] =
{
	"SYSMAN_IDX_INIT",
	"SYSMAN_IDX_HOTKEY",
	"SYSMAN_IDX_KERNELCMDLINE",
	"SYSMAN_IDX_NR"
};

static gint _DEBUG_SYSMAN_log[32] =
{
	1,1,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

static guint _sys_boot_time = 0;

static void nf_sysman_set_boot_time(guint time)
{
	_sys_boot_time = time;
}

guint nf_sysman_get_boot_time(void)
{
	return _sys_boot_time;
}

gboolean nf_sysman_time_check( void )
{

	GTimeVal  host_time = {0,0}, disk_time = {0,0};
	gboolean ret = 0;

	g_get_current_time( &host_time) ;
	g_message("%s host_time[%ld] 0",__FUNCTION__, host_time.tv_sec);

	ret = nf_filesystem_get_lasttime ( &disk_time);
	if(ret)
	{
		g_message("%s disk_time[%ld]",__FUNCTION__, disk_time.tv_sec );

		//if( host_time.tv_sec > NF_DATETIME_MIN && host_time.tv_sec < disk_time.tv_sec )
		if( host_time.tv_sec < disk_time.tv_sec )
		{
			g_warning("%s !!! host_time[%ld] < disk_time[%ld]",__FUNCTION__,  host_time.tv_sec, disk_time.tv_sec );
			g_warning("%s !!! host_time[%ld] < disk_time[%ld]",__FUNCTION__,  host_time.tv_sec, disk_time.tv_sec );

			ret = nf_datetime_set( disk_time.tv_sec+1 );
			g_message("%s nf_datetime_set( %ld ): %s", __FUNCTION__,
								disk_time.tv_sec, ret ? "SUCCESS" : "FAILED");

			ret = nf_dspcomm_sync_time();
			g_message("%s nf_dspcomm_sync_time(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

			g_warning("%s !!! host_time[%ld] < disk_time[%ld]",__FUNCTION__,  host_time.tv_sec, disk_time.tv_sec );
			g_warning("%s !!! host_time[%ld] < disk_time[%ld]",__FUNCTION__,  host_time.tv_sec, disk_time.tv_sec );

			g_get_current_time( &host_time);
			g_message("%s host_time[%ld] 1",__FUNCTION__, host_time.tv_sec);
		}
	}

	if( host_time.tv_sec < NF_DATETIME_MIN )
	{
		g_warning("%s !!! host_time[%ld] < NF_DATETIME_MIN[%d]",__FUNCTION__,  host_time.tv_sec, NF_DATETIME_MIN );
		g_warning("%s !!! host_time[%ld] < NF_DATETIME_MIN[%d]",__FUNCTION__,  host_time.tv_sec, NF_DATETIME_MIN );

		ret = nf_datetime_set( NF_DATETIME_MIN+1 );
		g_message("%s nf_datetime_set( %d ): %s", __FUNCTION__,
							(NF_DATETIME_MIN+1), ret ? "SUCCESS" : "FAILED");

		ret = nf_dspcomm_sync_time();
		g_message("%s nf_dspcomm_sync_time(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

		g_warning("%s !!! host_time[%ld] < NF_DATETIME_MIN[%d]",__FUNCTION__,  host_time.tv_sec, NF_DATETIME_MIN );
		g_warning("%s !!! host_time[%ld] < NF_DATETIME_MIN[%d]",__FUNCTION__,  host_time.tv_sec, NF_DATETIME_MIN );

		g_get_current_time( &host_time);
		g_message("%s host_time[%ld] 2",__FUNCTION__, host_time.tv_sec);

	}

	if( nf_sysman_get_boot_time() == 0 )
		nf_sysman_set_boot_time(host_time.tv_sec);

	return 1;
}

gboolean nf_sysman_time_set( GTimeVal tval )
{
	gboolean ret = 0;
	g_return_val_if_fail ( tval.tv_sec >= NF_DATETIME_MIN, 0 );

	ret = nf_record_stop( NULL );
	g_message("%s nf_record_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	ret = nf_network_stop( NF_DISCONN_SVR_TIME_CHANGE );
	g_message("%s nf_network_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	//gboolean nf_filesystem_stop( cb_event_fxn_t cb_event , gpointer cb_ctx, GError **error )
	ret = nf_filesystem_stop(NULL, NULL, NULL);
	g_message("%s nf_filesystem_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");


	//gboolean nf_filesystem_delete_data( guint utc_time_sec, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error )
	ret = nf_filesystem_delete_data((guint)tval.tv_sec, NULL, NULL, NULL);
	g_message("%s nf_filesystem_delete_data(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");
	if(ret)
	{
		ret = nf_datetime_set(tval.tv_sec);
		g_message("%s nf_datetime_set(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

		ret = nf_dspcomm_sync_time();
		g_message("%s nf_dspcomm_sync_time(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");
	}

	//gboolean nf_filesystem_start( gint format_flag, cb_event_fxn_t cb_event, gpointer cb_ctx, GError **error )
	ret = nf_filesystem_start(0, NULL, NULL, NULL);
	g_message("%s nf_filesystem_start(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	ret = nf_record_start(NULL);
	g_message("%s nf_record_start(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	ret = nf_network_start();
	g_message("%s nf_network_start(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	return 1;
}

static void _filesystem_stop_cb(int event, int arg, void* context)
{
	switch ( event ) {
		case NF_SST_EVT_FSSTOPBEG:
			g_message("%s %s", __FUNCTION__, sst_get_event_string(event));
			break;
		case NF_SST_EVT_FSSTOPCMPL:
			g_message("%s %s, error=%d", __FUNCTION__, sst_get_event_string(event), arg);
			((int *)context)[0] = 1;
			((int *)context)[1] = arg;
			break;
		default:
			break;
	}
}

gboolean nf_sysman_poweroff( gboolean is_reboot )
{
	gboolean ret = 0;
	int wait_cnt = 0;
	volatile int context[2] = {0,0};

	ret = nf_record_stop( NULL );
	g_message("%s nf_record_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	ret = nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
	g_message("%s nf_network_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	ret = nf_filesystem_stop( (cb_event_fxn_t)_filesystem_stop_cb, (void*)context, NULL);
	g_message("%s nf_filesystem_stop(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	while ( !context[0] )   {
		usleep(1000000); // sleep 1 sec
		g_message("%s  nf_filesystem_stop wait_cnt[%d]", __FUNCTION__, ++wait_cnt);
    }

	sync(); g_usleep(10*1000);
	sync(); g_usleep(10*1000);
	sync();	g_usleep(10*1000);

	ret = nf_sysman_sync();
	g_message("%s nf_sysman_sync(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	if( !is_reboot ) {
			nf_sysman_set_normal_boot();
			g_message("%s nf_sysman_set_normal_boot(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");
	}

	sleep(3);
	ret = nf_dev_board_reset();
	g_message("%s nf_dev_board_reset(): %s", __FUNCTION__, ret ? "SUCCESS" : "FAILED");

	return 1;
}


#if 0
	factory_default   Display + Setup + 1
	passwd_reset      Display + Setup + 2
	fwupgrade         Display + Setup + 3
	fwdngrade         Display + Setup + 4
	nf_consol_enable  Display + Setup + 5
#endif

#define NF_HOT_KEY_FACTORY_DEFAULT	"factory_default"
#define NF_HOT_KEY_PASSWD_RESET		"passwd_reset"
#define NF_HOT_KEY_FWUPGRADE		"fwupgrade"
#define NF_HOT_KEY_QA 				"nf_qa"
#define NF_HOT_KEY_CONSOL_ENABLE	"nf_consol_enable"
#define NF_HOT_KEY_NFS				"root=/dev/nfs"
#define NF_HOT_KEY_TEMP_FW_UPGRADE	"fw_upgrade_temp"
#if defined(SUPPORT_UBIFS)
	#define NF_HOT_KEY_BOOT_MTD_NUM			"root=ubi"
#else
	#define NF_HOT_KEY_BOOT_MTD_NUM			"root=/dev/mtdblock"
#endif
#define NF_HOT_KEY_FORMAT			"format"

#define FILE_PROC_CMDLINE			"/proc/cmdline"

#define NF_HW_PARAM_PANEL_TYPE		"pannel_type="
#define NF_HW_PARAM_RC_TYPE			"rc_type="
#define NF_HW_PARAM_HW_VER			"hw_ver="
#define NF_HW_PARAM_VENDOR			"vendor="
#define NF_HW_PARAM_UBL_CRC			"crc_ubl="
#define NF_HW_PARAM_FPGA_CRC		"crc_fpga="
#if defined(SCHIP_COPY_PROTECTION)
	#define NF_HW_PARAM_SERIAL_MATCH	"serial_match="
	#define NF_HW_PARAM_SERIAL_RAND		"serial_rand="
#endif

#define NF_HW_BOARD_ID					"board_id="

/** 20111010 **/
#define NF_SYSMAN_RESOLUTION_TYPE	"res="
#define NF_SYSMAN_OUTPUT_MODE_TYPE	"output_mode="

//support dual display
#define NF_SYSMAN_RESOLUTION_HDMI_TYPE	"res_hdmi="
#define NF_SYSMAN_RESOLUTION_VGA_TYPE	"res_vga="
#define NF_SYSMAN_DUAL_DISP_TYPE        "dual_type="

#define NF_SYSMAN_MODEL_CH				"num_ch="
#define NF_SYSMAN_MODEL_TYPE		"Model="
#define NF_SYSMAN_PBA_TYPE		"pba_type="

#define QC_MODE_OPTION_NTP				"ntp="
#define QC_MODE_OPTION_PCP				"pcp="
#define QC_MODE_OPTION_LANG				"lang="
#define QC_MODE_OPTION_MAC				"mac="
#define QC_MODE_OPTION_HDD				"HDD="
#define QC_MODE_OPTION_ODD				"ODD="
#define QC_MODE_OPTION_SIGNAL			"signal="
#define QC_MODE_OPTION_SERIAL			"enable_log="


static int _magic_key_init = 0;
static int _factory_default	= 0;
static int _passwd_reset = 0;
static int _fwupgrade = 0;
static int _qa = 0;
static int _consol_enable = 0;
static int _is_nfs = 0;
static int _is_temp_upgrade = 0;
static int _boot_mtd_num = 0;
static int _boot_ubi_num = 0;
static int _is_format = 0;
static int _board_id=0;

static int _qcmode_is_enable = 0;
static int _qcmode_is_factory_default = 0;
static int _qcmode_is_fwup = 0;
static char _qcmode_is_fwup_path[1024] = {0,};
static char _qcmode_is_ntp_server[256] = {0,};
static char _qcmode_option_lang[32] = {0, };
static char _qcmode_hdd_num[8] = {0,};
static char _qcmode_odd_num[8] = {0,};
static char _qcmode_signal[32] = {0,};
static int _qcmode_is_format = 0;
/*20170313*/
static int _qcmode_mac_writer = 0;

static uint _itx_default_setting = 0;
// static uint _itx_qc_live_audio = 0;

// static int _hw_param_init = 0;
// static char _hw_param_panel_type[32];
// static char _hw_param_hw_ver[32]; //jr695_comment: This variable is the firmware_model delimiter.
// static char _hw_param_vendor[32];
// static char _hw_param_ubl_crc[32];
// static char _hw_param_fpga_crc[32];
// static char _hw_param_rc_type[32];
// static char _hw_param_match_model[32]; //jr695_comment: This variable is hwparam(mac) hw_version.
#if defined(SCHIP_COPY_PROTECTION)
	static char _hw_param_serial_match[32];
	static char _hw_param_serial_rand[32];
#endif

static guint _fwver_product = 0;
static guint _fwver_procotol = 0;
static guint _fwver_minor = 0;
static guint _fwver_vendor = 0;
static gchar _fwver_option[16] = {0};

static char _nf_sysman_res[32];
static char _nf_sysman_output_mode[32];

//support dual display
static char _nf_sysman_res_hdmi[32];
static char _nf_sysman_res_vga[32];
static char _nf_sysman_dual_display[32];

// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
static char _nf_sysman_serial_num[32];
#endif
// onvif_porting

static char _nf_sysman_model[32];
static char _nf_sysman_pba_type[32];
static char _nf_sysman_model_ch[32];

static guchar _nf_sysman_qc_mac[32];

static guint nf_sysman_serial_enable = 0;


gboolean nf_sysman_hotkey_init(void)
{

	gchar *contents = NULL;
	gsize  length = 0;
	gchar *ret;
	GError *error = NULL;

	if (!g_file_get_contents ( FILE_PROC_CMDLINE , &contents, &length, &error))
	{
		g_warning("%s %s\n", __FUNCTION__,  error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return 0;
	}

	if( _magic_key_init ==0)
	{
		nf_debug_category_add( "sysman", _DEBUG_SYSMAN_str, _DEBUG_SYSMAN_log, DEBUG_SYSMAN_IDX_NR);
	}

	if(contents)
	{
		char *tmp = NULL;
		g_message("%s cmdline[%s]", __FUNCTION__, contents);

/* whenever you add the HOT_KEY, Please check below nf_sysman_hotkey_to_nand_log function */
		_factory_default	= ( NULL != strstr(contents, NF_HOT_KEY_FACTORY_DEFAULT) ) ? 1:0;
		_passwd_reset 		= ( NULL != strstr(contents, NF_HOT_KEY_PASSWD_RESET)    ) ? 1:0;
		_fwupgrade 			= ( NULL != strstr(contents, NF_HOT_KEY_FWUPGRADE)       ) ? 1:0;
		_qa 				= ( NULL != strstr(contents, NF_HOT_KEY_QA)              ) ? 1:0;
		_consol_enable 		= ( NULL != strstr(contents, NF_HOT_KEY_CONSOL_ENABLE)   ) ? 1:0;
		_is_temp_upgrade	= ( NULL != strstr(contents, NF_HOT_KEY_TEMP_FW_UPGRADE)   ) ? 1:0;

		_is_nfs 			= ( NULL != strstr(contents, NF_HOT_KEY_NFS)   ) ? 1:0;

		tmp = strstr(contents, NF_HOT_KEY_BOOT_MTD_NUM);
		if(tmp){
			#if defined(SUPPORT_UBIFS)
				_boot_ubi_num = strtol( tmp+strlen(NF_HOT_KEY_BOOT_MTD_NUM), NULL, 0 );
				if(_boot_ubi_num == NF_SYSMAM_UBIFS_PING)
					_boot_mtd_num = FW_UPGRADE_FILESYS_PING_MTD_NUM;
				else if(_boot_ubi_num == NF_SYSMAM_UBIFS_PONG)
					_boot_mtd_num = FW_UPGRADE_FILESYS_PONG_MTD_NUM;
			#else
				_boot_mtd_num = strtol( tmp+strlen(NF_HOT_KEY_BOOT_MTD_NUM), NULL, 0 );
			#endif
		}

		_is_format 			= ( NULL != strstr(contents, NF_HOT_KEY_FORMAT)   ) ? 1:0;

		#ifdef DEBUG_SYSMAN_LOG
			if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_HOTKEY ] )
			{
				g_message("%s _factory_default  [%d]", __FUNCTION__, _factory_default   );
				g_message("%s _passwd_reset     [%d]", __FUNCTION__, _passwd_reset 	    );
				g_message("%s _fwupgrade        [%d]", __FUNCTION__, _fwupgrade 		);
				g_message("%s _qa               [%d]", __FUNCTION__, _qa 			    );
				g_message("%s _consol_enable    [%d]", __FUNCTION__, _consol_enable     );
				g_message("%s _is_nfs           [%d]", __FUNCTION__, _is_nfs			);
				g_message("%s _boot_mtd_num     [%d]", __FUNCTION__, _boot_mtd_num		);
				g_message("%s _is_format        [%d]", __FUNCTION__, _is_format			);
			}
		#endif

		nf_sysman_hotkey_to_nand_log();

		g_free(contents);
	}

	_magic_key_init = 1;

	return 1;
}

gboolean nf_sysman_kcmdline_init(void)
{

	gchar *contents = NULL;
	gsize  length = 0;
	gchar *ret;
	GError *error = NULL;

	if (!g_file_get_contents ( FILE_PROC_CMDLINE , &contents, &length, &error))
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

		tmp= strstr(contents, NF_HW_BOARD_ID);
		if(tmp) {
			_board_id=(gint)strtol(tmp+strlen(NF_HW_BOARD_ID), NULL, 0);
		}

		#ifdef DEBUG_SYSMAN_LOG
			if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_KENRLCMDLINE ] )
			{
				g_message("%s _board_id         [%d]", __FUNCTION__, _board_id			);
			}      
		#endif

		g_free(contents);
	}  

	return 1;
}

gboolean nf_sysman_hotkey_is_factory_default( void )
{
	return (_factory_default)?1:0;
}
gboolean nf_sysman_hotkey_is_passwd_reset( void )
{
	return (_passwd_reset)?1:0;
}
gboolean nf_sysman_hotkey_is_fwupgrade( void )
{
	return (_fwupgrade)?1:0;
}
gboolean nf_sysman_hotkey_is_qa( void )
{
	return (_qa)?1:0;
}
gboolean nf_sysman_hotkey_is_console_en( void )
{
	return (_consol_enable)?1:0;
}
gboolean nf_sysman_hotkey_is_nfs( void )
{
	return (_is_nfs)?1:0;
}
gint nf_sysman_get_boot_mtd_num( void )
{
	return _boot_mtd_num;
}

gboolean nf_sysman_hotkey_is_temp_upgrade( void )
{
	g_message("%s _is_temp_upgrade [%d]", __FUNCTION__, _is_temp_upgrade);
	return (_is_temp_upgrade)?1:0;
}

gboolean nf_sysman_hotkey_is_format( void )
{
	return (_is_format)?1:0;
}

// void nf_sysman_set_qc_live_audio(guint is_on)
// {
// 	//ksi_test
// 	qc_audio_manual_enable = is_on;
// 	// _itx_qc_live_audio = is_on;
// 	// nf_dev_audio_set_dac(0xff); 
// }

// guint nf_sysman_get_qc_live_audio(void)
// {
// 	return _itx_qc_live_audio;
// }

#define QCMODE_CHECK_ROOT_DIR_NAME	"/mnt"
#define QCMODE_CHECK_MOUNT_DIR_NAME	"/mnt/qcusb"
#define QCMODE_CHECK_FSTYPE 		"vfat"
#define QCMODE_CHECK_TRY_DISK_CNT	16

#define QCMODE_CHECK_QC_MODE_ON					"/itx_qc_mode_on.txt"
#define QCMODE_CHECK_QC_PARAM					"/itx_qc_param.txt"
#define QCMODE_CHECK_QC_MODE_FACTORY_DEFAULT	"/itx_qc_mode_default.txt"
#define QCMODE_CHECK_QC_MODE_FW_UP				"/itx_qc_mode_fwup.txt"
#define QCMODE_CHECK_QC_MODE_FORMAT				"/itx_qc_mode_format.txt"

#define ITX_HOTKEY_CHECK_DEFAULT_SETTING		"/itx_default_setting_mode_on.txt"

static int _qc_mode_get_usb_dev( gchar *buff, gint buff_len)
{
	int i,ret;
	char tmp[1024];
	char *usb_str = NULL;

	for(i=0;i<QCMODE_CHECK_TRY_DISK_CNT;++i)
	{
		snprintf( buff, buff_len, "/sys/block/sd%c", 'a'+i);
		memset( tmp, 0x00, sizeof(tmp));
		ret = readlink( buff, tmp, sizeof(tmp));
		if(ret>0) {
			usb_str = strstr(tmp ,"usb");
			if(usb_str) {
				snprintf( buff, buff_len, "/dev/sd%c1", 'a'+i);
				return 1;
			}
		}
	}
	memset( buff, 0x00, buff_len);
	return 0;
}

uint nf_sysman_check_default_setting(void)
{
	return _itx_default_setting;
}

// gboolean nf_sysman_qcmode_init(void)
// {
// 	gint ret;
// 	char dev_name[1024];

// 	ret = mkdir(QCMODE_CHECK_ROOT_DIR_NAME, 0755);
// 	if(ret != 0)
// 		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_ROOT_DIR_NAME);

// 	ret = mkdir(QCMODE_CHECK_MOUNT_DIR_NAME, 0755);
// 	if(ret != 0)
// 		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_MOUNT_DIR_NAME);

// 	/** 1. umount ==> actually don't need **/
// 	umount(QCMODE_CHECK_MOUNT_DIR_NAME);

// 	_qc_mode_get_usb_dev(dev_name, sizeof(dev_name));

// 	/** 2. mount **/
// 	if((ret = mount(dev_name, QCMODE_CHECK_MOUNT_DIR_NAME, QCMODE_CHECK_FSTYPE , 0, 0)) !=0)
// 	{
// 		g_warning("%s mount error!!! ret[%d] [%s]->[%s]", __FUNCTION__, ret, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
// 		return FALSE;
// 	}else{
// 		g_message("%s mounted [%s]->[%s]", __FUNCTION__, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
// 	}

// 	/** 3. process **/
// 	_qcmode_is_enable = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_ON,  G_FILE_TEST_EXISTS);
// 	_qcmode_is_factory_default = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FACTORY_DEFAULT,  G_FILE_TEST_EXISTS);
// 	_qcmode_is_fwup = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FW_UP,  G_FILE_TEST_EXISTS);
// 	_qcmode_is_format = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FORMAT,  G_FILE_TEST_EXISTS);

// 	_itx_default_setting = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME ITX_HOTKEY_CHECK_DEFAULT_SETTING,  G_FILE_TEST_EXISTS);

// 	if(_qcmode_is_fwup)
// 	{
// 		gint	ret = 0;
// 		gint	contents_len = 0;
// 		gchar	*contents = NULL;

// 		ret = g_file_get_contents(QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FW_UP, &contents, &contents_len, NULL);
// 		if(ret)
// 		{
// 			snprintf( _qcmode_is_fwup_path , sizeof(_qcmode_is_fwup_path)-1,
// 						"%s/%s",QCMODE_CHECK_MOUNT_DIR_NAME, contents);
// 			g_strstrip(_qcmode_is_fwup_path);
// 			g_free(contents);

// 		}
// 	}

// 	if(_qcmode_is_enable)
// 	{
// 		gint	ret = 0;
// 		gint	contents_len = 0, limit_len = 0;
// 		gchar	*contents = NULL;

// 		ret = g_file_get_contents(QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_ON, &contents, &contents_len, NULL);
// 		if(ret)
// 		{
// 			limit_len = strstr(contents, QC_MODE_OPTION_LANG);
// 			if((contents_len = strstr(contents, QC_MODE_OPTION_NTP)) != NULL)
// 				_qcmode_mac_writer = 1;
// 			else if((contents_len = strstr(contents, QC_MODE_OPTION_PCP)) != NULL)
// 				_qcmode_mac_writer = 2;
// 			else
// 				_qcmode_mac_writer = 0;

// 			if (limit_len == NULL) {
// 				if(contents_len != NULL)
// 				{
// 					snprintf( _qcmode_is_ntp_server , sizeof(_qcmode_is_ntp_server)-1,"%s",contents+4);
// 					g_strstrip(_qcmode_is_ntp_server);
// 				}
// 			} else {
// 				if(contents_len != NULL)
// 				{
// 					snprintf( _qcmode_is_ntp_server , limit_len - (contents_len + strlen(QC_MODE_OPTION_NTP)), "%s", contents_len + strlen(QC_MODE_OPTION_NTP));
// 					g_strstrip(_qcmode_is_ntp_server);
// 				}
// 			}

// 			contents_len = strstr(contents, QC_MODE_OPTION_LANG);
// 			limit_len = strstr(contents, QC_MODE_OPTION_SIGNAL);

// 			if (limit_len == NULL) {
// 				if (contents_len != NULL)
// 				{
// 					snprintf( _qcmode_option_lang , sizeof(_qcmode_option_lang) - 1, "%s", contents_len + strlen(QC_MODE_OPTION_LANG));
// 					g_strstrip(_qcmode_option_lang);
// 				}
// 			} else {
// 				if (contents_len != NULL)
// 				{
// 					snprintf( _qcmode_option_lang , limit_len - (contents_len + strlen(QC_MODE_OPTION_LANG)), "%s", contents_len + strlen(QC_MODE_OPTION_LANG));
// 					g_strstrip(_qcmode_option_lang);
// 				}

// 				contents_len = strstr(contents, QC_MODE_OPTION_SIGNAL);
// 				if (contents_len != NULL) {
// 					snprintf( _qcmode_signal, sizeof(_qcmode_signal) - 1, "%s", contents_len + strlen(QC_MODE_OPTION_SIGNAL));
// 					g_strstrip(_qcmode_signal);
// 				}
// 			}

// 		}

// 		if (g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_PARAM,  G_FILE_TEST_EXISTS)) {
// 			ret = g_file_get_contents(QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_PARAM, &contents, &contents_len, NULL);
// 			if (ret) {
// 				#if defined(_IPX_1648P4E)|| defined(_IPX_32P4E) || defined(_IPX_0824P4E)
// 				guchar temp_mac[32]={0,};
// 				gint i = 0, j  =0;
// 				contents_len = strstr(contents, QC_MODE_OPTION_MAC);
// 				if (contents_len != NULL) {
// 					memset(_nf_sysman_qc_mac, 0x0, sizeof(_nf_sysman_qc_mac));
// 					strncpy(temp_mac, contents_len + strlen(QC_MODE_OPTION_MAC), strlen("xx:xx:xx:xx:xx:xx"));
// 					for(i = 0, j = 0; j<(strlen("xx:xx:xx:xx:xx:xx")); j++)
// 					{
// 						if(temp_mac[j] != ':')
// 						{
// 							_nf_sysman_qc_mac[i] = temp_mac[j];
// 							i++;
// 						}
// 					}
// 				}
// 				#endif

// 				contents_len = strstr(contents, QC_MODE_OPTION_HDD);
// 				if (contents_len != NULL) {
// 					strncpy(_qcmode_hdd_num, contents_len + strlen(QC_MODE_OPTION_HDD), 1);
// 				}

// 				contents_len = strstr(contents, QC_MODE_OPTION_ODD);
// 				if (contents_len != NULL) {
// 					strncpy(_qcmode_odd_num, contents_len + strlen(QC_MODE_OPTION_ODD), 1);
// 				}
// 			}
// 		}

// 		printf("\e[32m [%s] _qcmode_is_enalbe = %d\e[0m\n", __FUNCTION__, _qcmode_is_enable);
// 		g_free(contents);
// 	}
// 	g_message("%s qcmode_is_enable[%d][%s]", __FUNCTION__, _qcmode_is_enable, _qcmode_is_ntp_server);
// 	g_message("%s qcmode_is_factory_default[%d]", __FUNCTION__, _qcmode_is_factory_default);
// 	g_message("%s qcmode_is_fwup[%d][%s]", __FUNCTION__, _qcmode_is_fwup, _qcmode_is_fwup_path);
// 	g_message("%s qcmode_is_format[%d]", __FUNCTION__, _qcmode_is_format);
// 	g_message("%s _qcmode_is_ntp_server[%s]", __FUNCTION__, _qcmode_is_ntp_server);
// 	g_message("%s _qcmode_hdd_num[%s]", __FUNCTION__, _qcmode_hdd_num);
// 	g_message("%s _qcmode_odd_num[%s]", __FUNCTION__, _qcmode_odd_num);
// 	g_message("%s _qcmode_option_lang[%s]", __FUNCTION__, _qcmode_option_lang);
// 	g_message("%s _qcmode_signal[%s]", __FUNCTION__, _qcmode_signal);

//  	//while(1) sleep (1);

// 	/** 4. unmount **/
// 	if( !_qcmode_is_fwup )
// 		umount(QCMODE_CHECK_MOUNT_DIR_NAME);
// 	return 1;
// }


//#include "../nfgui/viewers/vw_qc_test.h"
extern gint vw_qc_test_info_add_line(gchar *str, gint len);
extern gint vw_qc_test_info_erase();

// int nf_sysman_qcmode_gui_printf(  const char *format, ... ){

//    va_list ap;
//    char    buffer[4096];
//    int     size;

//    va_start(ap, format);
//    size = vsprintf(buffer, (char *)format, ap);
//    va_end(ap);

//    gdk_threads_enter();
//    vw_qc_test_info_add_line( buffer, size > 128? 128:size );
//    gdk_threads_leave();
//    return( size > 128? 128:size  );
// }

static int _qcmode_gui_printf(  const char *format, ... ){

   va_list ap;
   char    buffer[4096];
   int     size;

   va_start(ap, format);
   size = vsprintf(buffer, (char *)format, ap);
   va_end(ap);

   vw_qc_test_info_add_line( buffer, size > 128? 128:size );

   return( size > 128? 128:size  );
}

static void _qcmode_gui_hexdump(gpointer p, int len){

    static char line[] =
        "00000000  xx xx xx xx  xx xx xx xx  "
        "xx xx xx xx  xx xx xx xx  yyyyyyyy yyyyyyyy";
    static char hex[] = "0123456789abcdef";
    char *s;
    int thistime;
    char *l;

    int col;
	int base = 0;

	g_message( "3--------------------------------------------------------------------------------");
//Debug("0         1         2         3         4         5         6         7");
//Debug("0123456789012345678901234567890123456789012345678901234567890123456789012345678");
//Debug("xxxxxxxx  xx xx xx xx  xx xx xx xx  xx xx xx xx  xx xx xx xx  ........ ........");

    base -= (int)p;
    for (s = p; len; len -= thistime) {
        sprintf(line, "%08x", (unsigned int)(s + base));
        line[8] = ' ';
        thistime = len > 16 ? 16 : len;

        l = line + 10;
        for (col = 0; col < thistime; col++) {
            *l++ = hex[(*s & 0xf0) >> 4];
            *l++ = hex[*s++ & 0x0f];
            l += ((col & 3) == 3) + 1;
        }
        while (col < 16) {
            *l = l[1] = ' ';
            l += ((col++ & 3) == 3) + 3;
        }

        s -= thistime;
        for (col = 0; col < thistime; col++) {
            *l++ = isprint(*s) ? *s : '.';
            l += col == 7;
            s++;
        }
        while (col < 16) {
            *l++ = ' ';
            l += col++ == 7;
        }

        _qcmode_gui_printf("%s",line);
    }

_qcmode_gui_printf("4--------------------------------------------------------------------------------");

}

static void _qcmode_gui_file(gchar* filename){

	FILE *fp = NULL;
	char buff[128];
	int print_cnt=0;

	fp = fopen( filename, "r");

	g_return_if_fail ( fp != NULL);

	memset(buff, 0x00, sizeof(buff) );
	while( fgets( buff , sizeof(buff)-1, fp ) )
	{
		_qcmode_gui_printf("%s", g_strstrip(buff) );
		if(++print_cnt>64){
			_qcmode_gui_printf("too many lines stop print");
			break;
		}
	}

	fclose(fp);
	return;

}

// static volatile int _factory_def_event_func_event = 0;
// static volatile int _factory_def_event_func_arg = 0;

// static void _factory_def_event_function(int event, int arg, void *context) {

// 	nf_sysman_qcmode_gui_printf("%s [%s](%d) arg[%d]", __FUNCTION__,  sst_get_event_string(event), event, arg );

// 	_factory_def_event_func_event = event;
// 	_factory_def_event_func_arg = arg;
// }


// static volatile int _is_factory_def_now = 0;
// static void _factory_def_thread_func ( int *_status )
// {

// 	const char	*_sysdb_cate_list[] = {
// 						"sys","net","audio","disk","cam","usr",
// 						"alarm","act","disp","rec",
// // onvif_porting
// 	#ifdef ENABLE_HNF_IPCAM
// 								"ipcam","iprec","ipact",
// 	#endif
// 						        "onvif",
// 								 NULL };
// // onvif_porting

// 	int i;
// 	char tmp[1024];

// 	nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
// 	nf_sysman_qcmode_gui_printf("%s nf_network_stop", __FUNCTION__ );
// 	sleep(2);

// 	nf_ipcam_stop();
// 	nf_sysman_qcmode_gui_printf("%s nf_ipcam_stop", __FUNCTION__ );
// 	sleep(3);
// 	while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);

// 	nf_record_stop( NULL );
// 	nf_sysman_qcmode_gui_printf("%s nf_record_stop", __FUNCTION__ );
// 	sleep(2);

// 	if( nf_filesystem_is_online()) {
// 		nf_filesystem_stop( _factory_def_event_function, NULL, NULL);
// 		nf_sysman_qcmode_gui_printf("%s nf_filesystem_stop", __FUNCTION__ );
// 		sleep(3);
// 		while( _factory_def_event_func_event != SST_EVT_FSSTOPCMPL ){
// 			//NF_SST_PROGRESS  progress;
// 			//nf_filesystem_get_progress(NF_SST_PRGT_FSSTOP, &progress);
// 			sleep(1);
// 		}
// 	}else{
// 		nf_sysman_qcmode_gui_printf("%s filesystem is already offline", __FUNCTION__ );
// 	}

// 	for(i=0; _sysdb_cate_list[i] ;++i)
// 	{
// 		//_qcmode_gui_printf("%s factory default  -> [%s]", __FUNCTION__, _sysdb_cate_list[i] );
// 		snprintf( tmp, sizeof(tmp)-1, "/bin/rm -f /NFDVR/data/nf_sysdb_%s.conf", _sysdb_cate_list[i]);
// 		proxy_system( tmp ,1,3);
// 	}
// 	nf_sysman_qcmode_gui_printf("%s factory default done", __FUNCTION__);

// 	proxy_system("/bin/rm -f /NFDVR/data/gui/login_user.itx",1,3);
// 	proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);

// 	nf_sysman_sync();
// 	nf_sysman_set_normal_boot();
// 	nf_sysman_qcmode_gui_printf("%s set normal boot", __FUNCTION__);

// 	nf_sysman_qcmode_gui_printf("%s complete", __FUNCTION__);
// 	nf_sysman_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);

// 	*_status = 2;

// 	return;

// }

// static void _qcmode_do_factory_default(gpointer param) {

// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);

// 	if( _is_factory_def_now == 0) {
// 		_is_factory_def_now = 1;
// 		if( g_thread_create((GThreadFunc)_factory_def_thread_func, &_is_factory_def_now, FALSE, NULL) == 0)
// 		{
// 			_is_factory_def_now = 0;
// 			_qcmode_gui_printf("%s thread create failed", __FUNCTION__);
// 		}
// 	}else if( _is_factory_def_now == 2) {
// 		_qcmode_gui_printf("%s complete", __FUNCTION__);
// 		_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);

// 		sleep(2);

// 		_is_factory_def_now = 0;

// 	}else {
// 		_qcmode_gui_printf("%s wait!!", __FUNCTION__);
// 	}

// 	return;
// }


// #include "nf_util_device.h"
// #include "itx_key_table_info.h"


// static volatile int _format_event_func_event = 0;
// static volatile int _format_event_func_arg = 0;

// static void _format_event_function(int event, int arg, void *context) {

// 	nf_sysman_qcmode_gui_printf("%s [%s](%d) arg[%d]", __FUNCTION__,  sst_get_event_string(event), event, arg );

// 	_format_event_func_event = event;
// 	_format_event_func_arg = arg;
// }

// static volatile int _is_format_now = 0;
// static void _format_thread_func ( int *_status )
// {
// 	const char	*_sysdb_cate_list[] = {
// 						"sys","net","audio","disk","cam","usr",
// 						"alarm","act","disp","rec",
// // onvif_porting
// 	#ifdef ENABLE_HNF_IPCAM
// 								"ipcam","iprec","ipact",
// 	#endif
// 						        "onvif",
// 								 NULL };
// // onvif_porting

// 	int i;
// 	char tmp[1024];

// 	_format_event_func_event = 0x3398;


// 	for(i=0; i<	LED_INDEX_MAX_NUM; ++i)
// 		nf_dev_keypad_led_off(i);

// 	nf_sysman_qcmode_gui_printf("%s led all OFF", __FUNCTION__);


// 	nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
// 	nf_sysman_qcmode_gui_printf("%s nf_network_stop", __FUNCTION__ );
// 	sleep(2);

// 	nf_ipcam_stop();
// 	nf_sysman_qcmode_gui_printf("%s nf_ipcam_stop", __FUNCTION__ );
// 	sleep(3);
// 	while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);

// 	nf_record_stop( NULL );
// 	nf_sysman_qcmode_gui_printf("%s nf_record_stop", __FUNCTION__ );
// 	sleep(2);

// 	if( nf_filesystem_is_online()) {
// 		nf_filesystem_stop( _format_event_function, NULL, NULL);
// 		nf_sysman_qcmode_gui_printf("%s nf_filesystem_stop", __FUNCTION__ );
// 		sleep(3);
// 		while( _format_event_func_event != SST_EVT_FSSTOPCMPL ){
// 			//NF_SST_PROGRESS  progress;
// 			//nf_filesystem_get_progress(NF_SST_PRGT_FSSTOP, &progress);
// 			sleep(1);
// 		}
// 	}else{
// 		nf_sysman_qcmode_gui_printf("%s filesystem is already offline", __FUNCTION__ );
// 	}

// 	nf_sysman_qcmode_gui_printf("%s nf_disk_format start", __FUNCTION__ );
// 	nf_disk_format( 1,0, _format_event_function, NULL, NULL);
// 	while( _format_event_func_event != SST_EVT_FORMATCMPL ) {
// 		//NF_SST_PROGRESS  progress;
// 		//nf_filesystem_get_progress(NF_SST_PRGT_FORMAT, &progress);
// 		sleep(1);
// 	}
// 	nf_sysman_qcmode_gui_printf("%s nf_disk_format done", __FUNCTION__ );

// 	for(i=0; _sysdb_cate_list[i] ;++i)
// 	{
// 		//_qcmode_gui_printf("%s factory default  -> [%s]", __FUNCTION__, _sysdb_cate_list[i] );
// 		snprintf( tmp, sizeof(tmp)-1, "/bin/rm -f /NFDVR/data/nf_sysdb_%s.conf", _sysdb_cate_list[i]);
// 		proxy_system( tmp ,1,3);
// 	}
// 	nf_sysman_qcmode_gui_printf("%s factory default done", __FUNCTION__);

// 	proxy_system("/bin/rm -f /NFDVR/data/gui/login_user.itx",1,3);
// 	proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);

// 	nf_sysman_sync();
// 	nf_sysman_set_normal_boot();
// 	nf_sysman_qcmode_gui_printf("%s set normal boot", __FUNCTION__);

// 	for(i=0; i<	LED_INDEX_MAX_NUM; ++i)
// 		nf_dev_keypad_led_on(i);
// 	nf_sysman_qcmode_gui_printf("%s led all ON", __FUNCTION__);


// 	nf_sysman_qcmode_gui_printf("%s complete", __FUNCTION__);
// 	nf_sysman_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);

// 	*_status = 2;

// 	return;
// }


// static void _qcmode_do_format(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);

// 	if( _is_format_now == 0) {
// 		_is_format_now = 1;
// 		if( g_thread_create((GThreadFunc)_format_thread_func, &_is_format_now, FALSE, NULL) == 0)
// 		{
// 			_is_format_now = 0;
// 			_qcmode_gui_printf("%s thread create failed", __FUNCTION__);
// 		}
// 	}else if( _is_format_now == 2) {
// 		_qcmode_gui_printf("%s complete", __FUNCTION__);
// 		_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);
// 	}else {
// 		_qcmode_gui_printf("%s wait!!", __FUNCTION__);
// 	}
// 	return;
// }

// static volatile int _is_test1_mode = 0;
// static void _qcmode_do_test1(gpointer param){

// 	int i;
// 	NF_UTIL_FAN_INFO  fan_info;
// 	int temper_cpu;

// 	vw_qc_test_info_erase();

// 	_qcmode_gui_printf("%s called", __FUNCTION__);

// 	memset ( &fan_info, 0x00, sizeof(fan_info));
// 	nf_dev_board_pp_fan_get_info(&fan_info);

// 	temper_cpu= (gint)(((float)fan_info.temper_cpu/(float)6.66666) + (float)fan_info.temper_cpu);

// 	_qcmode_gui_printf("%s temp cpu[%d] sys[%d]",  __FUNCTION__
// 					,temper_cpu, (int)fan_info.temper_sys);
// 	_qcmode_gui_printf("%s rpm  cpu[%d] sys[%d][%d]", __FUNCTION__
// 					, (int)fan_info.speed_cpu_fan
// 					, (int)fan_info.speed_sys_fan1
// 					, (int)fan_info.speed_sys_fan2 );


// 	if( _is_test1_mode == 0) {
// 		_is_test1_mode = 1;

// 		for(i=0; i<	LED_INDEX_MAX_NUM; ++i)
// 			nf_dev_keypad_led_on(i);

// 		_qcmode_gui_printf("%s led all ON", __FUNCTION__);

// 		nf_dev_audio_set_dac(0x0);
// 		_qcmode_gui_printf("%s SPEAKER ON( CH1->SET )", __FUNCTION__);

// 		nf_dev_mic_set_output_mask(1);
// 		nf_dev_mic_set_onoff(1);
// 		_qcmode_gui_printf("%s MIC ON( SET->CH1 )", __FUNCTION__);

// 	}else {
// 		_is_test1_mode = 0;

// 		for(i=0; i<	LED_INDEX_MAX_NUM; ++i)
// 			nf_dev_keypad_led_off(i);

// 		_qcmode_gui_printf("%s led all OFF", __FUNCTION__);

// 		nf_dev_audio_set_dac(0xff);
// 		_qcmode_gui_printf("%s SET SPEAKER OFF( CH1->SET )", __FUNCTION__);

// 		nf_dev_mic_set_output_mask(1);
// 		nf_dev_mic_set_onoff(0);
// 		_qcmode_gui_printf("%s SET MIC OFF( SET->CH1 )", __FUNCTION__);
// 	}

// 	_qcmode_gui_printf("%s complete", __FUNCTION__);

// 	return;
// }


// static volatile int _is_fwup_now = 0;
// static void _qcmode_do_fwup(gpointer param){

// 	int ret = 0, i;
// 	NF_FW_PRGT prgt;
// 	vw_qc_test_info_erase();

// 	_qcmode_gui_printf("%s called is_run[%d]", __FUNCTION__, _is_fwup_now);
// 	_qcmode_gui_printf("%s fw file (%s)", __FUNCTION__, _qcmode_is_fwup_path );

// 	if( _is_fwup_now == 0) {

// 		for(i=0;i<NF_WATCHDOG_MEMBER_NOTIFY;++i)
// 			nf_watchdog_ctrl(i, NF_WATCHDOG_KILL_SECOND, 0 );

// 		_qcmode_gui_printf("%s watchdog disable", __FUNCTION__ );
// 		sleep(2);

// 		ret = nf_fw_upgrade_thread_start( _qcmode_is_fwup_path, 1);
// 		if( ret == 1) {
// 			_is_fwup_now = 1;
// 			_qcmode_gui_printf("%s start fwup!!", __FUNCTION__ );
// 		}else{
// 			_qcmode_gui_printf("%s failed!!", __FUNCTION__ );
// 		}
// 		sleep(2);
// 	}

// 	nf_fw_state_check(&prgt);

// 	_qcmode_gui_printf("%s prgt.type    [%02x]/[%02x]\n", __FUNCTION__, prgt.type, NF_FW_PRGT_TYPE_FINISH);
// 	_qcmode_gui_printf("%s prgt.state   [%02x]/[%02x]\n",__FUNCTION__, prgt.state, NF_FW_PRGT_FNI_UPDATE_FNISH);
// 	_qcmode_gui_printf("%s prgt.is_error[%s]\n", __FUNCTION__, prgt.is_error ? "ERROR":"OK" );
// 	_qcmode_gui_printf("%s prgt.current [%d]\n", __FUNCTION__, prgt.current);
// 	_qcmode_gui_printf("%s prgt.total   [%d]\n", __FUNCTION__, prgt.total);

// 	if( prgt.is_error ) {
// 		_qcmode_gui_printf("%s error", __FUNCTION__ );
// 		_is_fwup_now = 0;
// 		sleep(1);
// 	} else {
// 		if( prgt.type == NF_FW_PRGT_TYPE_FINISH
// 			&& prgt.state == NF_FW_PRGT_FNI_UPDATE_FNISH )
// 		{
// 			_qcmode_gui_printf("%s complete", __FUNCTION__);
// 			_qcmode_gui_printf("%s fw file (%s)", __FUNCTION__, _qcmode_is_fwup_path );
// 			_qcmode_gui_printf("%s complete", __FUNCTION__);
// 			_is_fwup_now = 3;
// 		}else{
// 			_qcmode_gui_printf("%s in progress... wait", __FUNCTION__ );
// 		}
// 	}

// 	return;
// }

// #include "nf_util_netif.h"

// static volatile int _is_dhcp_now = 0;
// static void _dhcp_thread_func ( int *_status )
// {

// 	NF_NETIF_GET_INFO ret_info;

// 	nf_ipcam_stop();
// 	nf_sysman_qcmode_gui_printf("%s nf_ipcam_stop", __FUNCTION__ );
// 	sleep(3);
// 	while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);

// 	nf_sysman_qcmode_gui_printf("%s netif init start", __FUNCTION__ );
// 	nf_netif_init();
// 	nf_sysman_qcmode_gui_printf("%s netif init stop", __FUNCTION__ );

// 	nf_netif_get_info( &ret_info);
// 	nf_sysman_qcmode_gui_printf("%s ipaddr      [0x%08x]", __FUNCTION__, ntohl(ret_info.ipaddr     ));
// 	nf_sysman_qcmode_gui_printf("%s netmask     [0x%08x]", __FUNCTION__, ntohl(ret_info.netmask    ));
// 	nf_sysman_qcmode_gui_printf("%s broadcast   [0x%08x]", __FUNCTION__, ntohl(ret_info.broadcast  ));
// 	nf_sysman_qcmode_gui_printf("%s gateway     [0x%08x]", __FUNCTION__, ntohl(ret_info.gateway    ));
// 	nf_sysman_qcmode_gui_printf("%s dnsserver1  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver1 ));
// 	nf_sysman_qcmode_gui_printf("%s dnsserver2  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver2 ));

// 	nf_ipcam_start();
// 	nf_sysman_qcmode_gui_printf("%s nf_ipcam_start", __FUNCTION__ );
// 	nf_sysman_qcmode_gui_printf("%s complete", __FUNCTION__);

// 	*_status = 2;

// 	return;
// }

// static void _qcmode_do_dhcp_renew(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);

// 	if( _is_dhcp_now == 0) {
// 		_is_dhcp_now = 1;
// 		if( g_thread_create((GThreadFunc)_dhcp_thread_func, &_is_dhcp_now, FALSE, NULL) == 0)
// 		{
// 			_is_dhcp_now = 0;
// 			_qcmode_gui_printf("%s thread create failed", __FUNCTION__);
// 		}
// 	}else if( _is_dhcp_now == 2) {

// 		NF_NETIF_GET_INFO ret_info;

// 		nf_netif_get_info( &ret_info);

// 		_qcmode_gui_printf("%s ipaddr      [0x%08x]", __FUNCTION__, ntohl(ret_info.ipaddr     ));
// 		_qcmode_gui_printf("%s netmask     [0x%08x]", __FUNCTION__, ntohl(ret_info.netmask    ));
// 		_qcmode_gui_printf("%s broadcast   [0x%08x]", __FUNCTION__, ntohl(ret_info.broadcast  ));
// 		_qcmode_gui_printf("%s gateway     [0x%08x]", __FUNCTION__, ntohl(ret_info.gateway    ));
// 		_qcmode_gui_printf("%s dnsserver1  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver1 ));
// 		_qcmode_gui_printf("%s dnsserver2  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver2 ));

// 		_qcmode_gui_printf("%s complete", __FUNCTION__);
// 		_is_dhcp_now = 0;
// 		sleep(2);
// 	}else {
// 		_qcmode_gui_printf("%s wait!!", __FUNCTION__);
// 	}
// 	return;
// }

// nf_api_disk.h
// extern gboolean nf_get_nand_log(void);

// static void _qcmode_do_dump_nand(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);

// 	nf_get_nand_log();
// 	_qcmode_gui_file("/tmp/webra-info/nand.log");

// 	return;
// }

// static void _qcmode_do_dump_notify(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);
// 	_qcmode_gui_file("/tmp/webra-info/notify_dump.txt");

// 	return;
// }

// static void _qcmode_do_dump_camstat(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);
// 	_qcmode_gui_file("/tmp/webra-info/cam_stat.txt");

// 	return;
// }

// static void _qcmode_do_dump_infofs(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);
// 	_qcmode_gui_file("/tmp/webra-info/info.js");

// 	return;
// }

// static void _qcmode_do_dump_sst(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);
// 	_qcmode_gui_file("/proc/ifs");
// 	_qcmode_gui_file("/proc/ifs_io_state");
// 	return;
// }

// static void _qcmode_do_dump_mem(gpointer param){
// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);
// 	_qcmode_gui_file("/proc/cmem");
// 	_qcmode_gui_file("/proc/meminfo");
// 	return;
// }


// static void _qcmode_do_dump_qcstat(gpointer param){
// 	NF_SYSMAN_HW_PARAM hparam;

// 	vw_qc_test_info_erase();
// 	_qcmode_gui_printf("%s called", __FUNCTION__);

// 	memset( &hparam, 0x00, sizeof(NF_SYSMAN_HW_PARAM ));

// 	nf_sysman_get_hw_param( &hparam, 0 );
// 	{
// 		NF_SYSMAN_HW_PARAM *hwparam = &hparam;
// 		int cnt_qc_item;
// 		int i;

// 		if(strncmp(hwparam->magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0 )
// 		{
// 			_qcmode_gui_printf("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
// 			return;
// 		}

// 		_qcmode_gui_printf("Magic                 [%4.4s]", hwparam->magic);
// 		_qcmode_gui_printf("ETH ADDR              [%32.32s]",  hwparam->eth_addr);
// 		_qcmode_gui_printf("Front Type            [%32.32s]",  hwparam->pannel_type);
// 		_qcmode_gui_printf("RC    Type            [%32.32s]",  hwparam->rc_type);
// 		_qcmode_gui_printf("HW Version            [%32.32s]",  hwparam->hw_ver);
// 		_qcmode_gui_printf("Vendor                [%32.32s]",  hwparam->vendor);
// 		_qcmode_gui_printf("FW Write Fail         [%d]",  hwparam->is_fw_write_fail);
// 		_qcmode_gui_printf("Circular Buffer Count [%d]",  hwparam->circular_buffer_location);

// 		for(i=0; i<NF_SYSMAN_QC_CIRCULAR_BUFFER; i++)
// 		{
// 			if(strncmp(hwparam->qc_result[i].magic, NF_SYSMAN_QC_MAGIC, 4) != 0 )
// 			{
// 				_qcmode_gui_printf("%s QC Circular Buffer%dMagic Failed[%4.4s]", __FUNCTION__, i, hwparam->magic);
// 				continue;
// 			}

// 			_qcmode_gui_printf("QC Magic              [%4.4s]",  hwparam->qc_result[i].magic);
// 			_qcmode_gui_printf("Excute Time           [%48.48s]",  hwparam->qc_result[i].excute_time);

// 			for(cnt_qc_item=0; cnt_qc_item<NF_SYSMAN_QC_ITEM_MAX_NR; cnt_qc_item++)
// 				_qcmode_gui_printf("Excute Time           [%16.16s]",  hwparam->qc_result[i].item[cnt_qc_item]);

// 			_qcmode_gui_printf("\n");
// 		}
// 	}
// 	return;
// }

// gboolean nf_sysman_qcmode_sysdb_init(void)
// {
// 	int cnt = 1;

// 	if(nf_sysman_qcmode_is_enable()) {
// 		// for skip authentication
// 		nf_sysdb_set_str("usr.U0.pass","");
// 		nf_sysdb_set_bool("sys.info.passwd_enable",0);
// 		nf_sysdb_set_str("cam.C0.title","@ QCMODE @");

//  		nf_sysdb_set_bool("cam.ptz.P0.rs485", 1);
// 		nf_sysdb_set_uint("cam.ptz.P0.protocol", 0);

// 		// enable dhcp
// 		nf_sysdb_set_bool("net.proto.dhcpon", 1);

// #if	defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
//         nf_sysdb_set_bool("cam.install.mode", FALSE);
// 	nf_sysdb_set_bool("cam.install.dual_lan", FALSE);
// #endif
// 	}

// 	return 1;
// }

// gboolean nf_sysman_qcmode_2nd_init(void)
// {
// 	int cnt = 1;

// 	if(nf_sysman_qcmode_is_enable()) {

// 		vw_qc_test_connect_func(1, "TEST1", _qcmode_do_test1, NULL );
// 		vw_qc_test_connect_func(2, "QCSTAT", _qcmode_do_dump_qcstat, NULL );
// 		vw_qc_test_connect_func(3, "INFOJS", _qcmode_do_dump_infofs, NULL );
// //		vw_qc_test_connect_func(4, "CAMSTAT", _qcmode_do_dump_camstat, NULL );
// 		vw_qc_test_connect_func(5, "NAND", _qcmode_do_dump_nand, NULL );

// 		vw_qc_test_connect_func(6, "DEFAULT", _qcmode_do_factory_default, NULL );
// 		vw_qc_test_connect_func(7, "SST", _qcmode_do_dump_sst, NULL );

// 		vw_qc_test_connect_func(8, "DHCP", _qcmode_do_dhcp_renew, NULL );

// 		vw_qc_test_connect_func(10, "FORMAT", _qcmode_do_format, NULL );
// 		vw_qc_test_connect_func(11, "MEM", _qcmode_do_dump_mem, NULL );

// 		//vw_qc_test_connect_func(cnt++, "FWUP", _qcmode_do_fwup, NULL );
// 	}

// 	return 1;

// }

// gboolean nf_sysman_qcmode_is_enable( void ){ 
// 	return (_qcmode_is_enable)?1:0;
// }
// gboolean nf_sysman_qcmode_is_factory_default( void ){
// 	return (_qcmode_is_factory_default)?1:0;
// }
// gboolean nf_sysman_qcmode_is_fwup( void ){
// 	return (_qcmode_is_fwup)?1:0;
// }
// gboolean nf_sysman_qcmode_is_format( void ){
// 	return (_qcmode_is_format)?1:0;
// }

/* APP QC API*/
// #define APP_QC_DEBUG_MSG
// guint nf_sysman_hdd_num_check(void)
// {
// 	int hdd_param_num=0;
	
// 	#if defined(_IPX_0412M4) || defined(_IPX_0412M4E)
// 		return 1;
// 	#elif defined(_IPX_0824P4E) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
// 		hdd_param_num = atoi(_qcmode_hdd_num);
// 		return hdd_param_num;
// 	#else
// 		return 2;
// 	#endif
// }
// static 
// void nf_sysman_qc_set_def_signal(int mode)
// {
// 	static gboolean sig_type = FALSE;
// 	if(mode == 0)
// 	{
// 		sig_type = nf_sysdb_get_bool("sys.info.sig_type");
// 		printf("\033[0;31m %s get def sig_type = %s \033[0;39m\n", __FUNCTION__, (sig_type == TRUE) ? "PAL" : "NTSC");
// 	}
// 	else if (mode == 1)
// 	{
// 		#if 0
// 		nf_sysman_set_app_param_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, sig_type);
// 		#endif
// 		nf_sysdb_default( "sys" );
// 		nf_sysdb_load( "sys" );
// 		nf_sysdb_save( "sys" );
// 		nf_sysman_qc_set_signal();	
// 		printf("\033[0;31m %s set def sig_type = %s \033[0;39m\n", __FUNCTION__, (sig_type == TRUE) ? "PAL" : "NTSC");
// 	}

// }
// guint nf_sysman_odd_num_check(void)
// {
// 	return 0;
// }

// void nf_sysman_qc_set_signal(void)
// {
// 	if (strcmp(_qcmode_signal, "ntsc") == 0) {
// 		nf_sysman_set_app_param_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, FALSE);
// 		_set_sysdb_sig_type(FALSE);
// 	} else if (strcmp(_qcmode_signal, "pal") == 0) {
// 		nf_sysman_set_app_param_cate(NF_SYSMAN_APP_PARAM_CATE_IS_PAL, TRUE);
// 		_set_sysdb_sig_type(TRUE);
// 	}
// }

// gboolean nf_sysman_qc_get_ntp_serverip(char *serverip)
// {
// 	if(0 >= strlen(_qcmode_is_ntp_server))
// 		return FALSE;

// 	strncpy(serverip, _qcmode_is_ntp_server, strlen(_qcmode_is_ntp_server));

// 	return TRUE;
// }

// gboolean nf_sysman_qc_info_rtc(char *rtc_info)
// {
// 	if(!nf_datetime_rtc_get(rtc_info))
// 		return FALSE;

// 	return TRUE;
// }

// gboolean nf_sysman_qc_auto_test_alarm(void)
// {
// 	guint i = 0, sensor_val = 0, ret = 0, alarm_num = 0;

// 	if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A) {
// 	  #if defined(_IPX_0412M4) || defined(_IPX_0412M4E)
// 		alarm_num = 4;
// 	  #elif defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_32P4E)
// 		alarm_num = 16;
// 	  #else
// 		alarm_num = 8;
// 	  #endif
// 	} else {
// 		alarm_num = 2;
// 	}

// 	#if defined(_IPX_1648P4E)|| defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
//                 guint alarm_on_value = QC_TEST_ALARM_ON_VAL;
//                 guint alarm_off_value = QC_TEST_ALARM_OFF_VAL;
//        #endif	

// 	for (i = 0; i < NUM_RELAY_DVR; i++)
// 		nf_dev_relay_ch_on(i);

// 	g_usleep(1000 * 2000);

// 	nf_dev_sensor_get_curr_value(&sensor_val);
// #if defined(APP_QC_DEBUG_MSG)
// 	printf("\e[32m [%s] sensor val = 0x%x\e[0m\n", __FUNCTION__, sensor_val);
// #endif

//     #if defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
// 	if (sensor_val == alarm_on_value) {
// 		ret++;
// 	}
//     #endif
// 	if (alarm_num == 16) {
// 		if (sensor_val == 0xff) {
// 			ret++;
// 		}
// 	} else if (alarm_num == 8) {
// 		if (sensor_val == 0xf) {
// 			ret++;
// 		}
// 	} else if (alarm_num == 4){
// 		if (sensor_val == 0x3) {
// 			ret++;
// 		}
// 	} else {
// 		if (sensor_val == 0x1) {
// 			ret++;
// 		}
// 	}

// 	for (i = 0; i < NUM_RELAY_DVR; i++)
// 		nf_dev_relay_off(i);

// 	g_usleep(1000 * 2000);

// 	nf_dev_sensor_get_curr_value(&sensor_val);
// #if defined(APP_QC_DEBUG_MSG)
// 	printf("\e[32m [%s] sensor val = 0x%x\e[0m\n", __FUNCTION__, sensor_val);
// #endif

//     #if defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
// 	if (sensor_val == alarm_off_value) {
// 		ret++;
// 	}
//     #endif
// 	if (alarm_num == 16) {
// 		if (sensor_val == 0xff00) {
// 			ret++;
// 		}
// 	} else if (alarm_num == 8) {
// 		if (sensor_val == 0xf0) {
// 			ret++;
// 		}
// 	} else if (alarm_num == 4){
// 		if (sensor_val == 0xc) {
// 			ret++;
// 		}
// 	} else {
// 		if (sensor_val == 0x2) {
// 			ret++;
// 		}
// 	}

// #if defined(APP_QC_DEBUG_MSG)
// 	printf("\e[32m [%s] alarm_num = 0x%x ret = 0x%x\e[0m\n", __FUNCTION__, alarm_num, ret);
// #endif
// 	if (ret != 2)
// 		return FALSE;

// 	return TRUE;
// }

// static int flag_qc_audio_init = 0;
// gboolean nf_sysman_qc_auto_test_audio(void)
// {

// 	int result=0, i=0;
// //ksi_test
// 	// #if defined(_IPX_1648M4) || defined(_IPX_1648M4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
// 	// 	if (nf_sysman_get_pba_type() == NF_SYSMAN_PBA_TYPE_A)
// 	// 	{
//     //    	   if (nf_HI_aud_qc_test(flag_qc_audio_init) == -1)
// 	// 	       return FALSE;

// 	// 	   flag_qc_audio_init = 1;

// 	// 	   for (i = 0; i < QC_TEST_AUDIO_NUM; i++) {
// 	// 		if (nf_HI_aud_qc_frq_compare(i))
// 	// 			result++;
// 	// 	   }
// 	//       #if defined(APP_QC_DEBUG_MSG)
// 	// 	   printf("\e[31m >> [%s] audio test result = %d\e[0m\n", __FUNCTION__, result);
// 	//       #endif
// 	// 	   if (result == NUM_AUDIO) {
// 	// 	      return TRUE;
// 	// 	   } else {
// 	// 	      return FALSE;
// 	// 	   }
// 	// 	}
// 	// 	else
// 	// 	    return TRUE;
//     //    #elif defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
//     //    	   if (nf_HI_aud_qc_test(flag_qc_audio_init) == -1)
// 	// 	       return FALSE;

// 	// 	   flag_qc_audio_init = 1;

// 	// 	   for (i = 0; i < QC_TEST_AUDIO_NUM; i++) {
// 	// 		if (nf_HI_aud_qc_frq_compare(i))
// 	// 			result++;
// 	// 	   }
// 	//       #if defined(APP_QC_DEBUG_MSG)
// 	// 	   printf("\e[31m >> [%s] audio test result = %d\e[0m\n", __FUNCTION__, result);
// 	//       #endif
// 	// 	   if (result == NUM_AUDIO) {
// 	// 	      return TRUE;
// 	// 	   } else {
// 	// 	      return FALSE;
// 	// 	   }
// 	// #else
// 		return TRUE;
// 	// #endif
// }

// gboolean nf_sysman_qc_auto_test_rs485(void)
// {
	
// #if 1
// 	NF_PTZ_CMD ptz_cmd;
// 	nf_keyctrl_qc_test_check_reset();

// 	memset(&ptz_cmd, 0x00, sizeof(ptz_cmd));

// 	ptz_cmd.ch = 0;
// 	ptz_cmd.params[0] = 50;
// 	ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST;  // 55

// 	nf_ptz_cmd(&ptz_cmd);

// 	g_usleep(1000 * 1000);  

// 	if (!nf_keyctrl_qc_test_check()) {
// 		#if defined(APP_QC_DEBUG_MSG)
// 			printf("\e[32m [%s] rs485 test fail!!!\e[0m\n", __FUNCTION__);
// 		#endif
// 		return FALSE;
// 	}


// 	ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST_STOP;

// 	nf_ptz_cmd(&ptz_cmd);

// 	#if defined(APP_QC_DEBUG_MSG)
// 		printf("\e[32m [%s] rs485 test ok!!!\e[0m\n", __FUNCTION__);
// 	#endif
// 	#endif
// 	return TRUE;


// }

// void nf_sysman_qc_manual_test_rs485(int is_on)
// {
// 	NF_PTZ_CMD ptz_cmd;

// 	memset(&ptz_cmd, 0x00, sizeof(ptz_cmd));
// 	printf("\e[32m check check %s \e[0m \n",__FUNCTION__);
// 	ptz_cmd.ch = 0;
// 	ptz_cmd.params[0] = 50;
// 	if (is_on) {
// 		ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST;
// 	} else {
// 		ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST_STOP;
// 	}

// 	nf_ptz_cmd(&ptz_cmd);
// }

// gboolean nf_sysman_qc_manual_test_factory_key(void)
// {
// 	guchar val = 0;

// 	#if defined(_IPX_1648M4) || defined(_IPX_1648M4E)  || defined(_IPX_32M4E) || defined(_IPX_32P5)
// 		nf_dev_board_pp_get_gpio(5, 5, &val);
// 	#elif defined(_IPX_0824M4) || defined(_IPX_0824M4E)
// 		nf_dev_board_pp_get_gpio(3, 6, &val);
// 	#elif defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
// 		nf_dev_board_pp_get_gpio(5, 5, &val);
// 	#elif defined(_IPX_0824P4E)
// 		nf_dev_board_pp_get_gpio(3, 6, &val);    //check_check	
// 	#else
// 		nf_dev_board_pp_get_gpio(11, 5, &val);
// 	#endif

// 	printf("\e[33m [%s] factory key val = 0x%x\e[0m\n",
// 			__FUNCTION__, val);

// 	if (val == 0) {
// 		return TRUE;
// 	} else {
// 		return FALSE;
// 	}
// }

gboolean nf_sysman_is_support_factory_key(void)
{
	return TRUE;
}

#if defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
	#define DEV_QC_RS232_NAME "/dev/ttyAMA0"
#else 
	#define DEV_QC_RS232_NAME "/dev/ttyAMA2"
#endif

// gboolean nf_sysman_qc_auto_test_rs232(void)
// {
//  	int rs232_handle = -1, ret = -1, cnt = 0;
// 	struct termios newtio;
// 	unsigned char buf_wr[64] = "RS232TEST";
// 	unsigned char buf_rd[64] = {0, };

// 	rs232_handle = open(DEV_QC_RS232_NAME, O_RDWR | O_NOCTTY | O_NONBLOCK);

// 	if (rs232_handle < 0) {
// 		perror(DEV_QC_RS232_NAME);
// 		return -1;
// 	}

// 	memset(&newtio, 0, sizeof(newtio));
// 	newtio.c_iflag = 0;
// 	newtio.c_oflag = 0;
// 	newtio.c_cflag = CS8 | CREAD | CLOCAL;
// 	newtio.c_lflag = 0;
// 	newtio.c_cc[VMIN] = 1;
// 	newtio.c_cc[VTIME] = 5;

// 	cfsetospeed(&newtio, B115200);
// 	cfsetispeed(&newtio, B115200);

// 	tcflush(rs232_handle, TCIFLUSH);
// 	tcsetattr(rs232_handle, TCSANOW, &newtio);

// 	write(rs232_handle, buf_wr, 9);

// 	while (1) {
// 		ret = read(rs232_handle, buf_rd, 9);

// 		if (ret > 0) {
// 			if (!strcmp(buf_wr, buf_rd)) {
// 				printf("\e[32m [%s] RS232 TEST OK!!\e[0m\n", __FUNCTION__);
// 				return 1;
// 			}
// 		}

// 		cnt++;
// 		if (cnt == 10) {
// 			printf("\e[32m [%s] RS232 TEST FAIL!!\e[0m\n", __FUNCTION__);
// 			break;
// 		}

// 		usleep(10*1000);
// 	}

// 	return 0;
// }

gboolean nf_sysman_qc_auto_test_poe(void)
{
	NF_UTIL_POE_CHIP_INFO info;

	nf_dev_poe_get_chip_infomation(&info);

	printf("\e[32m [%s] POE Chip is_connected[0] = 0x%x is_connected[1] = 0x%x\e[0m\n", __FUNCTION__, info.is_connected[0], info.is_connected[1]);

#if defined(_IPX_0824) || defined(_IPX_1648P) || defined(_IPX_1648P3) || defined(_IPX_1648VE3) \
 || defined(_IPX_1648P4) || defined(_IPX_1648L4) || defined(_IPX_1648M4) || defined(_IPX_1648P4E) \
 || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
	if ((info.is_connected[0] == 1) || (info.is_connected[1] == 1))
#else
	if (info.is_connected[0] == 1)
#endif
	{
		return 1;
	}

	return 0;
}
#if defined(_IPX_1648M4) || defined(_IPX_1648P4E) || defined(_IPX_1648M4E) || defined(_IPX_32P4E) || defined(_IPX_32M4E) || defined(_IPX_32P5)
static int nf_sysman_check_link(char *ifname) {
    int state = -1;
	int ret = 0;
    int socId = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (socId < 0) 
    {
		g_warning("%s Socket failed...", __FUNCTION__);
		return -1;
    }
	
    struct ifreq if_req;
    (void) strncpy(if_req.ifr_name, ifname, sizeof(if_req.ifr_name));
    int rv = ioctl(socId, SIOCGIFFLAGS, &if_req);
    close(socId);

    if ( rv == -1)
    {
		g_warning("%s ioctl failed...", __FUNCTION__);
		return -1;
    }

	if(if_req.ifr_flags & IFF_UP)
	{
		ret |= (1<<0);
	}
	if(if_req.ifr_flags & IFF_RUNNING)
	{
		ret |= (1<<1);
	}
	return ret;
}
#endif
gboolean nf_sysman_qc_auto_test_network_switch(void)
{
	int link_mask = 0, i = 0, ret = 0;
	int wan_link = 0;
	link_mask = 0xff;

	int sleepCnt = 0;
	if(!(nf_sysman_check_link("eth0") & 0x01))
	{
		proxy_system("ifconfig eth0 down",1,3);
		proxy_system("ifconfig eth0 up",1,3);
	}

	while(nf_sysman_check_link("eth0") != 3)
	{
		if(sleepCnt >= 10)
			break;
		sleepCnt++;
		printf("\033[0;31m %s %d Waitting for linking with LAN cable\033[0;39m\n", __FUNCTION__, sleepCnt);
		g_usleep(10 * 100000); //1sec
		
	}

	wan_link = nf_sysman_check_link("eth0");
	printf("\e[33m[%s] wan_link = 0x%x  \e[0m\n", __FUNCTION__, wan_link);

	nf_dev_switch_get_link_status(&link_mask);

	printf("\e[33m[%s] link_mask = 0x%x MAX_SWITCH_PORT_NUM is [%d] \e[0m\n", __FUNCTION__, link_mask, MAX_SWITCH_PORT_NUM);

	for (i = 0; i < MAX_SWITCH_PORT_NUM; i++) {
		if (((link_mask >> i) & 0x1) == 1) {
			ret++;
		}
	}
	
	int link_state=0;
	nf_dev_board_pp_get_net_link_state(0, &link_state);
	g_message("%s  line%d link_state %d", __FUNCTION__, __LINE__, link_state);

	if ((ret == 17) && link_state && (wan_link == 3)) {
		return 1;
	}

	return 0;
}

// gboolean nf_sysman_qc_auto_test_fan(void)
// {
// 	#if defined(ENABLE_FAN_FAIL_CHECK)
// 		NF_UTIL_FAN_INFO fan_info;

// 		if (nf_dev_board_pp_fan_get_info(&fan_info)) {
// 			g_message("CPU Fan Speed       [%d]", fan_info.speed_cpu_fan);
// 			g_message("SYS Fan1 Speed      [%d]", fan_info.speed_sys_fan1);
// 			g_message("SYS Fan2 Speed      [%d]", fan_info.speed_sys_fan2);
// 			g_message("CPU Fan TEMP        [%d]", fan_info.temper_cpu);
// 			g_message("SYS Fan TEMP        [%d]", fan_info.temper_sys);
// 			g_message("Fan DUTY            CPU[%d] SYS[%d]", fan_info.cpufan_duty, fan_info.sysfan_duty);
// 			g_message("Fan MODE            CPU[%d] SYS[%d]", fan_info.cpufan_mode, fan_info.sysfan_mode);
// 			g_message("CPU Fan Threshold   [%d]", fan_info.thresh_cpu);
// 			g_message("SYS Fan Threshold   [%d]", fan_info.thresh_sys);

// 			if (fan_info.speed_sys_fan1 >= 1000 && fan_info.speed_sys_fan1 <= 5000) {
// 				return TRUE;
// 			} else {
// 				return FALSE;
// 			}
// 		}

// 		return FALSE;
// 	#else
// 		return TRUE;
// 	#endif
// }

// gboolean nf_sysman_qc_auto_test_temper(void)
// {
// 	#if defined(ENABLE_FAN_FAIL_CHECK)
// 		NF_UTIL_FAN_INFO fan_info;

// 		if (nf_dev_board_pp_fan_get_info(&fan_info)) {
// 			#if defined(APP_QC_DEBUG_MSG)
// 				g_message("CPU Fan Speed       [%d]", fan_info.speed_cpu_fan);
// 				g_message("SYS Fan1 Speed      [%d]", fan_info.speed_sys_fan1);
// 				g_message("SYS Fan2 Speed      [%d]", fan_info.speed_sys_fan2);
// 				g_message("CPU Fan TEMP        [%d]", fan_info.temper_cpu);
// 				g_message("SYS Fan TEMP        [%d]", fan_info.temper_sys);
// 				g_message("Fan DUTY            CPU[%d] SYS[%d]", fan_info.cpufan_duty, fan_info.sysfan_duty);
// 				g_message("Fan MODE            CPU[%d] SYS[%d]", fan_info.cpufan_mode, fan_info.sysfan_mode);
// 				g_message("CPU Fan Threshold   [%d]", fan_info.thresh_cpu);
// 				g_message("SYS Fan Threshold   [%d]", fan_info.thresh_sys);
// 			#endif
// 			if ((fan_info.temper_sys != 0 && fan_info.temper_sys != 255)) {
// 				return TRUE;
// 			} else {
// 				return FALSE;
// 			}
// 		}

// 		return FALSE;
// 	#else
// 		return TRUE;
// 	#endif
// }

// static gboolean _nf_sysman_qc_result_save_in_usb(NF_SYSMAN_QC_RESULT qc_result, int item_total_num)
// {
// 	gchar ret;
// 	gchar dev_name[1024];
// 	gchar file_name[256] = {0, };
// 	gchar next_mac[32] = {0, };
// 	gchar usb_mac[32] = {0, }, usb_mac_temp[32] = {0, };
// 	guint mac_temp = 0;
// 	gint file_len = 0, temp_len = 0;
// 	gchar result_str[2048] = {0, };
// 	gint result_len = 0, result_len_temp = 0, i, j;
// 	FILE *fp = NULL;
// 	unsigned char mac_hex[] = "0123456789ABCDEF";

// 	NF_SYSMAN_HW_PARAM hwparam;
	
// 	// get hw_param
// 	memset(&hwparam, 0x00, sizeof(hwparam));
// 	nf_sysman_get_hw_param(&hwparam, 0);
// 	if(strncmp(hwparam.magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0 )
// 	{
// 		g_warning("%s %d HW PARAM MAGIC ERROR!!!!", __FUNCTION__, __LINE__);
// 		memset(&hwparam, 0x00, sizeof(hwparam));
// 	}

// 	ret = mkdir(QCMODE_CHECK_ROOT_DIR_NAME, 0755);
// 	if (ret != 0)
// 		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_ROOT_DIR_NAME);

// 	ret = mkdir(QCMODE_CHECK_MOUNT_DIR_NAME, 0755);
// 	if (ret != 0)
// 		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_MOUNT_DIR_NAME);

// 	_qc_mode_get_usb_dev(dev_name, sizeof(dev_name));

// 	umount(QCMODE_CHECK_MOUNT_DIR_NAME);

// 	if ((ret = mount(dev_name, QCMODE_CHECK_MOUNT_DIR_NAME, QCMODE_CHECK_FSTYPE , 0, 0)) != 0) {
// 		g_warning("%s mount error!!! ret[%d] [%s]->[%s]", __FUNCTION__, ret, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
// 		return FALSE;
// 	} else {
// 		g_message("%s mounted [%s]->[%s]", __FUNCTION__, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
// 	}

// 	strncat(file_name, QCMODE_CHECK_MOUNT_DIR_NAME, sizeof(file_name));

// 	char temp_model[32]={0,};
// 	char temp_ch[32]={0,};
// 	char temp_hw_ver[32]={0,};
// 	#if defined(_IPX_32P4E)
// 		strcpy(temp_model,"IPXP4E_32");
// 		strcpy(temp_ch,"32");
// 		strcpy(temp_hw_ver,"IPX-P4E 32CH");
// 	#elif defined(_IPX_32M4E)
// 		strcpy(temp_model,"IPXM4E_32");
// 		strcpy(temp_ch,"32");
// 		strcpy(temp_hw_ver,"IPX-M4E 32CH");	
// 	//_IPX_32P5	
// 	#else
// 		strcpy(temp_model,_nf_sysman_model);
// 		strcpy(temp_ch,_nf_sysman_model_ch);
// 		strcpy(temp_hw_ver,nf_sysman_get_hw_ver());
// 	#endif

// 	file_len = strlen(file_name);
// 	temp_len = sprintf(file_name + file_len, "/%s_%s", temp_model, temp_ch);
// 	file_len += temp_len;

// 	if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
// 		g_mkdir(file_name, 0755);
// 	}

// 	temp_len = sprintf(file_name + file_len, "/%s_", temp_hw_ver);
// 	file_len += temp_len;

// 	temp_len = sprintf(file_name + file_len, "%s", nf_sysman_get_qc_mac());
// 	file_len += temp_len;

// 	temp_len = sprintf(file_name + file_len, ".txt");
// 	file_len += temp_len;

// 	fp = fopen(file_name, "a");
// 	if (fp == NULL) {
// 		printf("\e[32m [%s] %s FILE OPEN ERROR!!!\e[0m\n", __FUNCTION__, file_name);
// 		return FALSE;
// 	}

// 	result_len = sprintf(result_str, "=======================================================\nEXCUTE TIME:%s", qc_result.excute_time);

// 	result_len_temp = sprintf(result_str + result_len, "MODEL:%s\n", temp_model);
// 	result_len += result_len_temp;

// 	result_len_temp = sprintf(result_str + result_len, "CH:%s\n", temp_ch);
// 	result_len += result_len_temp;

// 	result_len_temp = sprintf(result_str + result_len, "HW VER:%s\n", temp_hw_ver);
// 	result_len += result_len_temp;

// 	result_len_temp = sprintf(result_str + result_len, "MAC:%s\n", nf_sysman_get_qc_mac());
// 	result_len += result_len_temp;

// 	result_len_temp = sprintf(result_str + result_len, "PANEL TYPE:%s\n", nf_sysman_get_panel_type());
// 	result_len += result_len_temp;

// 	result_len_temp = sprintf(result_str + result_len, "REMOCON TYPE:%s\n", nf_sysman_get_rc_type());
// 	result_len += result_len_temp;

// 	for (i = 0; i < item_total_num; i++) {
// 		result_len_temp = sprintf(result_str + result_len, "%s\n", qc_result.item[i]);
// 		result_len += result_len_temp;
// 	}

// 	result_len_temp = sprintf(result_str + result_len, "=======================================================\n");
// 	result_len += result_len_temp;

// #if defined(APP_QC_DEBUG_MSG)
// 	printf("\e[32m [%s] qc_result str is\n%s\e[0m\n", __FUNCTION__, result_str);
// 	printf("\e[32m [%s] qc_result str len is %d\e[0m\n", __FUNCTION__, result_len);
// #endif

// 	fwrite(result_str, result_len, 1, fp);
// 	fclose(fp);

// #if defined(_IPX_0824P4) || defined(_IPX_1648P4) \
//  || defined(_IPX_0412L4) || defined(_IPX_0824P4) || defined(_IPX_1648L4)// || defined(_IPX_1648P4E)|| defined(_IPX_32P4E) || defined(_IPX_0824P4E)
// 	memset(file_name, 0x00, sizeof(file_name));
// 	strncat(file_name, QCMODE_CHECK_MOUNT_DIR_NAME, sizeof(file_name));

// 	file_len = strlen(file_name);
// 	temp_len = sprintf(file_name + file_len, "/itx_qc_param.txt");
// 	file_len += temp_len;

// 	fp = fopen(file_name, "r");
// 	if (fp == NULL) {
// 		printf("\e[32m [%s] %s FILE OPEN ERROR!!!\e[0m\n", __FUNCTION__, file_name);
// 		return FALSE;
// 	}

// 	fread(usb_mac, sizeof("mac=xx:xx:xx:xx:xx:xx") - 1, 1, fp);

// 	fclose(fp);

// 	for (i = 4; i < sizeof("mac=xx:xx:xx:xx:xx:xx") - 1; i++) {
// 		if (hwparam.eth_addr[i - 4] != usb_mac[i]) {
// 			printf("\e[32m [%s] hwparam.eth_addr[%d] = %c, usb_mac[%d] = %c\e[0m\n", __FUNCTION__, i - 4, hwparam.eth_addr[i - 4], i, usb_mac[i]);
// 		}
// 	}

// 	for (i = 9, j =0; i < strlen(hwparam.eth_addr); i++) {
// 		if (hwparam.eth_addr[i] != ':') {
// 			usb_mac_temp[j] = hwparam.eth_addr[i];
// 			j++;
// 		}
// 	}

// 	mac_temp = strtoul(usb_mac_temp, NULL, 16);

// 	mac_temp += 1;
// 	mac_temp &= 0xffffff;

// 	usb_mac[13] = mac_hex[(mac_temp >> 20) & 0xf];
// 	usb_mac[14] = mac_hex[(mac_temp >> 16) & 0xf];
// 	usb_mac[16] = mac_hex[(mac_temp >> 12) & 0xf];
// 	usb_mac[17] = mac_hex[(mac_temp >> 8) & 0xf];
// 	usb_mac[19] = mac_hex[(mac_temp >> 4) & 0xf];
// 	usb_mac[20] = mac_hex[(mac_temp >> 0) & 0xf];

// 	fp = fopen(file_name, "r+");
// 	if (fp == NULL) {
// 		printf("\e[32m [%s] %s FILE OPEN ERROR!!!\e[0m\n", __FUNCTION__, file_name);
// 		return FALSE;
// 	}

// 	fwrite(usb_mac, strlen(usb_mac), 1, fp);

// 	fclose(fp);
// #endif
// }

// gboolean nf_sysman_qc_result_save(gchar item[][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN], int item_total_num)
// {
// 	NF_SYSMAN_HW_PARAM hw_param;
// 	NF_SYSMAN_QC_RESULT qc_reslt;
// 	NF_UTIL_PROTECT_DATA data;
// 	int item_cnt=0;

// 	// get hw_param
// 	memset(&hw_param, 0x00, sizeof(hw_param));
// 	nf_sysman_get_hw_param(&hw_param, 0);
// 	if(strncmp(hw_param.magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0 )
// 	{
// 		printf("\e[33m [%s] First QC Test\e[0m\n", __FUNCTION__);
// 		memset(&hw_param, 0x00, sizeof(hw_param));
// 	}

// 	// set QC result
// 	memset(&qc_reslt, 0x00, sizeof(qc_reslt));
// 	strncpy(qc_reslt.magic, NF_SYSMAN_QC_MAGIC, 4);
// 	nf_sysman_qc_info_rtc(qc_reslt.excute_time);
// 	if(item_total_num > NF_SYSMAN_QC_ITEM_MAX_NR)
// 	{
// 		g_warning("%s %d Total QC Result Item OVER!! Maximum [%d]", __FUNCTION__, __LINE__, NF_SYSMAN_QC_ITEM_MAX_NR);
// 		item_total_num=NF_SYSMAN_QC_ITEM_MAX_NR;
// 	}
// 	for(item_cnt=0; item_cnt<item_total_num; item_cnt++)
// 		strncpy(qc_reslt.item[item_cnt], item[item_cnt], NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN);

// 	// set hw_param
// 	hw_param.circular_buffer_location++;
// 	if((hw_param.circular_buffer_location >= NF_SYSMAN_QC_CIRCULAR_BUFFER) || (hw_param.circular_buffer_location < 0))
// 		hw_param.circular_buffer_location = 0;

// 	memcpy(&hw_param.qc_result[hw_param.circular_buffer_location], &qc_reslt, sizeof(NF_SYSMAN_QC_RESULT));

// 	memset(&data, 0x00, sizeof(data));
// 	#if defined(_IPX_1648P4E) || defined(_IPX_32P4E) || defined(_IPX_0824P4E)
// 	;
// 	#else
// 	if (nf_sysman_get_protect_data(&data) == FALSE) {
// 		// printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}
// 	#endif

// 	nf_sysman_set_hw_param(&hw_param);

// 	nf_sysman_set_protect_data(&data);

// 	_nf_sysman_qc_result_save_in_usb(qc_reslt, item_total_num);

// 	g_message("%s QC RESULT SAVE OK!", __FUNCTION__);
// 	return TRUE;
// }

// gboolean nf_sysman_qc_factory_default(void)
// {
// 	const char	*_sysdb_cate_list[] = {
// 						"sys","net","audio","disk","cam","usr",
// 						"alarm","act","disp","rec",
// 						 NULL };

// 	int i;
// 	char tmp[1024];

// 	nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
// 	sleep(2);

// 	nf_record_stop( NULL );
// 	sleep(2);

// 	for(i=0; _sysdb_cate_list[i] ;++i)
// 	{
// 		snprintf( tmp, sizeof(tmp)-1, "/bin/rm -f /NFDVR/data/nf_sysdb_%s.conf", _sysdb_cate_list[i]);
// 		proxy_system( tmp ,1,3);
// 	}

// 	proxy_system("/bin/rm -f /NFDVR/data/gui/login_user.itx",1,3);
// 	proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);

// 	nf_sysman_sync();
// 	nf_sysman_qc_set_def_signal(1); //set default type
// 	nf_sysman_sync();
// 	nf_sysman_set_normal_boot();
// 	return TRUE;

// }
/* end of APP QC API */

gboolean nf_sysman_hotkey_to_nand_log( void )
{
	if ( _factory_default )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _factory_default");
	if ( _passwd_reset )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _passwd_reset");
	if ( _fwupgrade )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _fwupgrade");
	if ( _qa )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _qa");
	if ( _consol_enable )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _consol_enable");
	if ( _is_nfs )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _is_nfs");
	if ( _is_temp_upgrade )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _is_temp_upgrade");
	if ( _is_format )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, "HOTKEY = _is_format");

	return TRUE;
}

// gboolean nf_sysman_hw_param_init(void)
// {

// 	gchar *contents = NULL;
// 	gsize  length = 0;
// 	gchar *ret;
// 	GError *error = NULL;
// 	#if defined(SCHIP_COPY_PROTECTION)
// 		NF_UTIL_PROTECT_DATA data;
// 		ITX_PROTECT_DATA_PARAM data_param;
// 	#endif

// 	memset( _hw_param_panel_type, 0x00, sizeof(_hw_param_panel_type));
// 	memset( _hw_param_rc_type, 0x00, sizeof(_hw_param_rc_type));
// 	memset( _hw_param_vendor, 0x00, sizeof(_hw_param_vendor));
// 	memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
// 	memset( _hw_param_ubl_crc, 0x00, sizeof(_hw_param_ubl_crc));

// 	#if defined(SCHIP_COPY_PROTECTION)
// 		memset( _hw_param_serial_match, 0x00, sizeof(_hw_param_serial_match));
// 		memset( _hw_param_serial_rand, 0x00, sizeof(_hw_param_serial_rand));
// 	#endif

// 	if (!g_file_get_contents ( FILE_PROC_CMDLINE , &contents, &length, &error))
// 	{
// 		g_warning("%s %s\n", __FUNCTION__,  error->message);
// 		g_error_free (error);

// 		if(contents)
// 			g_free(contents);

// 		return 0;
// 	}

// 	if(contents)
// 	{
// 		char *tmp = NULL;
// 		g_message("%s cmdline[%s]", __FUNCTION__, contents);

// 		if((tmp = strstr(contents, NF_HW_PARAM_PANEL_TYPE)))
// 		{
// 			snprintf(_hw_param_panel_type, sizeof(_hw_param_panel_type),
// 						tmp + (strlen(NF_HW_PARAM_PANEL_TYPE)));
// 			tmp = strchr(_hw_param_panel_type, ' ');
// 			if ( tmp ) {
// 				*tmp = 0x0;
// 			}
// 			g_strstrip(_hw_param_panel_type);
// 		}

// 		if((tmp = strstr(contents, NF_HW_PARAM_RC_TYPE)))
// 		{
// 			snprintf(_hw_param_rc_type, sizeof(_hw_param_rc_type),
// 						tmp + (strlen(NF_HW_PARAM_RC_TYPE)));
// 			tmp = strchr(_hw_param_rc_type, ' '); if( tmp ) *tmp = 0x0;
// 			g_strstrip(_hw_param_rc_type);
// 		}

// 		if((tmp = strstr(contents, NF_HW_PARAM_HW_VER)))
// 		{
// 			snprintf(_hw_param_match_model, sizeof(_hw_param_match_model),
// 						tmp + (strlen(NF_HW_PARAM_HW_VER)));
// 			tmp = strchr(_hw_param_match_model, ' ');
// 			if(tmp!=NULL)
// 			{
// 				tmp = strchr(tmp+1, ' ');
// 				if( tmp ) *tmp = 0x0;
// 			}
// 			g_strstrip(_hw_param_match_model);
// 		}

// 		if((tmp = strstr(contents, NF_HW_PARAM_HW_VER)))
// 		{
// 			snprintf(_hw_param_hw_ver, sizeof(_hw_param_hw_ver),
// 						tmp + (strlen(NF_HW_PARAM_HW_VER)));
// 			tmp = strchr(_hw_param_hw_ver, ' '); if( tmp ) *tmp = 0x0;
// 			g_strstrip(_hw_param_hw_ver);

// 			if (strncmp(_hw_param_hw_ver, "IPX-M4A", sizeof("IPX-M4A")) == 0) {
// 				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
// 				#if defined(_IPX_0412M4)
// 					strncpy(_hw_param_hw_ver, "IPX-M4A 4CH", sizeof("IPX-M4A 4CH"));
// 				#elif defined(_IPX_0824M4)
// 					strncpy(_hw_param_hw_ver, "IPX-M4A 8CH", sizeof("IPX-M4A 4CH"));
// 				#elif defined(_IPX_1648M4)
// 					strncpy(_hw_param_hw_ver, "IPX-M4A 16CH", sizeof("IPX-M4A 16CH"));
// 				#endif
// 			} else if (strncmp(_hw_param_hw_ver, "IPX-M4B", sizeof("IPX-M4B")) == 0) {
// 				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
// 				#if defined(_IPX_0412M4)
// 					strncpy(_hw_param_hw_ver, "IPX-M4B 4CH", sizeof("IPX-M4A 4CH"));
// 				#elif defined(_IPX_0824M4)
// 					strncpy(_hw_param_hw_ver, "IPX-M4B 8CH", sizeof("IPX-M4A 4CH"));
// 				#elif defined(_IPX_1648M4)
// 					strncpy(_hw_param_hw_ver, "IPX-M4B 16CH", sizeof("IPX-M4A 16CH"));
// 				#endif
// 			} else if (strncmp(_hw_param_hw_ver, "IPX-P4E", sizeof("IPX-P4E")) == 0) {
// 				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
// 				#if defined(_IPX_0824P4E)
// 					strncpy(_hw_param_hw_ver, "IPX-P4E 8CH", sizeof("IPX-P4E 8CH"));
// 				#elif defined(_IPX_1648P4E)
// 					strncpy(_hw_param_hw_ver, "IPX-P4E 16CH", sizeof("IPX-P4E 16CH"));
// 				#elif defined(_IPX_32P4E)
// 					strncpy(_hw_param_hw_ver, "IPX-P4E 32CH", sizeof("IPX-P4E 32CH"));
// 				#endif				
// 			} else if (strncmp(_hw_param_hw_ver, "IPX-M4AE", sizeof("IPX-M4AE")) == 0) {
// 				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
// 				#if defined(_IPX_0412M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4AE 4CH", sizeof("IPX-M4AE 4CH"));
// 				#elif defined(_IPX_0824M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4AE 8CH", sizeof("IPX-M4AE 4CH"));
// 				#elif defined(_IPX_1648M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4AE 16CH", sizeof("IPX-M4AE 16CH"));
// 				#elif defined(_IPX_32M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4AE 32CH", sizeof("IPX-M4AE 32CH"));				
// 				#endif
// 			} else if (strncmp(_hw_param_hw_ver, "IPX-M4BE", sizeof("IPX-M4BE")) == 0) {
// 				memset( _hw_param_hw_ver, 0x00, sizeof(_hw_param_hw_ver));
// 				#if defined(_IPX_0412M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4BE 4CH", sizeof("IPX-M4BE 4CH"));
// 				#elif defined(_IPX_0824M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4BE 8CH", sizeof("IPX-M4BE 4CH"));
// 				#elif defined(_IPX_1648M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4BE 16CH", sizeof("IPX-M4BE 16CH"));
// 				#elif defined(_IPX_32M4E)
// 					strncpy(_hw_param_hw_ver, "IPX-M4BE 32CH", sizeof("IPX-M4BE 32CH"));				
// 				#endif
// 			}
// 			//_IPX_32P5
			
// 		}

// 		if((tmp = strstr(contents, NF_HW_PARAM_VENDOR)))
// 		{
// 			snprintf(_hw_param_vendor, sizeof(_hw_param_vendor),
// 						tmp + (strlen(NF_HW_PARAM_VENDOR)));
// 			tmp = strchr(_hw_param_vendor, ' '); if( tmp ) *tmp = 0x0;
// 			g_strstrip(_hw_param_vendor);
// 		}

// 		if((tmp = strstr(contents, NF_HW_PARAM_UBL_CRC)))
// 		{
// 			snprintf(_hw_param_ubl_crc, sizeof(_hw_param_ubl_crc),
// 						tmp + (strlen(NF_HW_PARAM_UBL_CRC)));
// 			tmp = strchr(_hw_param_ubl_crc, ' '); if( tmp ) *tmp = 0x0;
// 			g_strstrip(_hw_param_ubl_crc);
// 		}

// 		if((tmp = strstr(contents, NF_HW_PARAM_FPGA_CRC)))
// 		{
// 			snprintf(_hw_param_fpga_crc, sizeof(_hw_param_ubl_crc),
// 						tmp + (strlen(NF_HW_PARAM_FPGA_CRC)));
// 			tmp = strchr(_hw_param_fpga_crc, ' '); if( tmp ) *tmp = 0x0;
// 			g_strstrip(_hw_param_fpga_crc);
// 		}

// 		#if defined(SCHIP_COPY_PROTECTION)
// 			memset(&data, 0x00, sizeof(data));
// 			if (nf_sysman_get_protect_data(&data) == FALSE) {
// 				// printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);   check_check
// 				strncpy(_hw_param_serial_match, "noe2pr", sizeof("noe2pr"));
// 				strncpy(_hw_param_serial_rand, "noe2pr", sizeof("noe2pr"));
// 			} else {
// 				memset(&data_param, 0x00, sizeof(data_param));
// 				memcpy(&data_param, &data.dec_data, sizeof(data_param));
// 				strncpy(_hw_param_serial_match, data_param.serial_match, sizeof(data_param.serial_match));
// 				strncpy(_hw_param_serial_rand, data_param.serial_rand, sizeof(data_param.serial_rand));
// 			}
		
// 			g_message("%s _hw_param_serial_match    [%s]", __FUNCTION__, _hw_param_serial_match);	
// 			g_message("%s _hw_param_serial_rand    [%s]", __FUNCTION__, _hw_param_serial_rand);			
// 		#endif
// #ifdef DEBUG_SYSMAN_LOG
// 		if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_HOTKEY ] )
// 		{
// 			g_message("%s _hw_param_match_model  [%s]", __FUNCTION__, _hw_param_match_model     );
// 			g_message("%s _hw_param_hw_ver  [%s]", __FUNCTION__, _hw_param_hw_ver     );
// 			g_message("%s _hw_param_vendor  [%s]", __FUNCTION__, _hw_param_vendor     );
// 			g_message("%s _hw_param_panel_type [%s]", __FUNCTION__, _hw_param_panel_type );
// 			g_message("%s _hw_param_rc_type    [%s]", __FUNCTION__, _hw_param_rc_type);
// 			g_message("%s _hw_param_ubl_crc    [%s]", __FUNCTION__, _hw_param_ubl_crc);
// 			g_message("%s _hw_param_fpga_crc    [%s]", __FUNCTION__, _hw_param_fpga_crc);
// 		}
// #endif
// 		nf_sysdb_set_str("sys.info.hwver", _hw_param_hw_ver);

// 		g_free(contents);
// 	}

// 	_hw_param_init = 1;

// 	return 1;
// }


// const gchar *nf_sysman_get_qcmode_option_lang( void )
// {
// 	return _qcmode_option_lang;
// }

// const gchar *nf_sysman_get_hw_ver( void )
// {
// 	return _hw_param_hw_ver; //get firmware version
// }
// const gchar *nf_sysman_get_match_model( void )
// {
// 	return _hw_param_match_model; //get hw version
// }
// gboolean nf_sysman_check_model_match( void )
// {
// 	#if defined(_IPX_1648P4E) || defined(_IPX_32P4E)
// 		return TRUE;
// 	#endif
// 	if(_hw_param_hw_ver !=NULL && _hw_param_match_model !=NULL)
// 	{
// 		if(!strcmp(_hw_param_hw_ver, _hw_param_match_model))
// 			return TRUE;
// 	}
// 	return FALSE;
// }
// const gchar *nf_sysman_get_vendor( void )
// {
// 	return _hw_param_vendor;
// }
// const gchar *nf_sysman_get_panel_type( void )
// {
// 	return _hw_param_panel_type;
// }

// const gchar *nf_sysman_get_rc_type( void )
// {
// 	return _hw_param_rc_type;
// }

// gboolean nf_sysman_check_is_remocon(void)
// {
// 	if (!strcmp(_hw_param_rc_type, "IRC200")) {
// 		printf("[%s] _hw_param_rc_type = %s\n", __FUNCTION__, _hw_param_rc_type);
// 		return 1;
// 	} else if (!strcmp(_hw_param_rc_type, "ICA_ITX")) {
// 		printf("[%s] _hw_param_rc_type = %s\n", __FUNCTION__, _hw_param_rc_type);
// 		return 1;
// 	}

// 	return 0;
// }

// const gchar *nf_sysman_get_ubl_crc( void )
// {
// 	return _hw_param_ubl_crc;
// }
// const gchar *nf_sysman_get_fpga_crc( void )
// {
// 	return _hw_param_fpga_crc;
// }

#if defined(SCHIP_COPY_PROTECTION)
const gchar *nf_sysman_get_serial_match( void )
{
	return _hw_param_serial_match;
}

const gchar *nf_sysman_get_serial_rand( void )
{
	return _hw_param_serial_rand;
}
#endif

// gboolean nf_sysman_fwver_init(void)
// {
// 	char tmp[128];
// 	int ret;

// 	memset( tmp, 0x00, sizeof(tmp));
// 	memset( _fwver_option, 0x00, sizeof(_fwver_option));

// 	strncpy( tmp, nf_sysdb_get_str_nocopy("sys.info.swver"), sizeof(tmp));

// 	ret = sscanf( tmp, "%d.%d.%d.%d%s", &_fwver_product,
// 										&_fwver_procotol,
// 										&_fwver_minor,
// 										&_fwver_vendor, _fwver_option);

// 	g_message("%s : swver[%s] ret[%d] [%d].[%d].[%d].[%d%s]", __FUNCTION__,
// 				tmp, ret,
// 				_fwver_product,
// 				_fwver_procotol,
// 				_fwver_minor,
// 				_fwver_vendor, _fwver_option);

// 	return TRUE;
// }

// from sys.info.swver
guint nf_sysman_get_fwver_product( void )
{
	return _fwver_product;
}

guint nf_sysman_get_fwver_protocol( void )
{
	return _fwver_procotol;
}

guint nf_sysman_get_fwver_minor( void )
{
	return _fwver_minor;
}

guint nf_sysman_get_fwver_vendor( void )
{
	return _fwver_vendor;
}

gchar *nf_sysman_get_fwver_option( void )
{
	return _fwver_option;
}

gint nf_sysman_get_vendor_from_db( void )
{
	gchar *str_vendor=NULL;

	str_vendor = nf_sysdb_get_str_nocopy("sys.info.vendor");

	printf("\e[33m [%s] str_vendor = %s\e[0m\n", __FUNCTION__, str_vendor);

	if (strncmp("TAKENAKA", str_vendor, 8) == 0) {
		return VENDOR_TAKENAKA;
	} else if (strncmp("S1", str_vendor, 2) == 0) {
		return VENDOR_S1;
	} else if (strncmp("KOBI", str_vendor, 4) == 0) {
		return VENDOR_KOBI;
	} else if (strncmp("KB_DEVICE", str_vendor, 9) == 0) {
		return VENDOR_KBDEVICE;
	} else if (strncmp("VIDECON", str_vendor, 7) == 0) {
		return VENDOR_VIDECON;
	} else if (strncmp("ITX_46GUI", str_vendor, 9) == 0) {
		return VENDOR_ITX_46GUI;
	} else if (strncmp("CBC", str_vendor, 3) == 0) {
		return VENDOR_CBC;
	} else {
		return VENDOR_UNKNOWN;
	}
}

gboolean nf_sysman_cmdline_init(void)
{
	gchar *contents = NULL;
	gsize  length = 0;
	gchar *ret;
	GError *error = NULL;

	if (!g_file_get_contents ( FILE_PROC_CMDLINE , &contents, &length, &error))
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

		if((tmp = strstr(contents, NF_SYSMAN_RESOLUTION_TYPE)))
		{
			snprintf(_nf_sysman_res, sizeof(_nf_sysman_res), tmp + (strlen(NF_SYSMAN_RESOLUTION_TYPE)));
			tmp = strchr(_nf_sysman_res, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_res);
		}

		if((tmp = strstr(contents, NF_SYSMAN_OUTPUT_MODE_TYPE)))
		{
			snprintf(_nf_sysman_output_mode, sizeof(_nf_sysman_output_mode), tmp + (strlen(NF_SYSMAN_OUTPUT_MODE_TYPE)));
			tmp = strchr(_nf_sysman_output_mode, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_output_mode);
		}

		if((tmp = strstr(contents, NF_SYSMAN_MODEL_TYPE)))
		{
			snprintf(_nf_sysman_model, sizeof(_nf_sysman_model), tmp + (strlen(NF_SYSMAN_MODEL_TYPE)));
			tmp = strchr(_nf_sysman_model, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_model);
		}

		if((tmp = strstr(contents, NF_SYSMAN_PBA_TYPE)))
		{
			snprintf(_nf_sysman_pba_type, sizeof(_nf_sysman_pba_type), tmp + (strlen(NF_SYSMAN_PBA_TYPE)));
			tmp = strchr(_nf_sysman_pba_type, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_pba_type);
		}

		//supprot dual display
		if((tmp = strstr(contents, NF_SYSMAN_RESOLUTION_HDMI_TYPE)))
		{
			snprintf(_nf_sysman_res_hdmi, sizeof(_nf_sysman_res_hdmi), tmp + (strlen(NF_SYSMAN_RESOLUTION_HDMI_TYPE)));
			tmp = strchr(_nf_sysman_res_hdmi, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_res_hdmi);
		}
		if((tmp = strstr(contents, NF_SYSMAN_RESOLUTION_VGA_TYPE)))
		{
			snprintf(_nf_sysman_res_vga, sizeof(_nf_sysman_res_vga), tmp + (strlen(NF_SYSMAN_RESOLUTION_VGA_TYPE)));
			tmp = strchr(_nf_sysman_res_vga, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_res_vga);
		}
		if((tmp = strstr(contents, NF_SYSMAN_DUAL_DISP_TYPE)))
		{
			snprintf(_nf_sysman_dual_display, sizeof(_nf_sysman_dual_display), tmp + (strlen(NF_SYSMAN_DUAL_DISP_TYPE)));
			tmp = strchr(_nf_sysman_dual_display, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_dual_display);
		}
        if((tmp = strstr(contents, NF_SYSMAN_MODEL_CH)))
		{
			snprintf(_nf_sysman_model_ch, sizeof(_nf_sysman_model_ch), tmp + (strlen(NF_SYSMAN_MODEL_CH)));
			tmp = strchr(_nf_sysman_model_ch, ' '); if( tmp ) *tmp = 0x0;
			g_strstrip(_nf_sysman_model_ch);
		}
		if((tmp = strstr(contents, QC_MODE_OPTION_SERIAL)))
		{
			snprintf(&nf_sysman_serial_enable, sizeof(_nf_sysman_model_ch), tmp + (strlen(QC_MODE_OPTION_SERIAL)));
			nf_sysman_serial_enable = atoi(&nf_sysman_serial_enable);
			printf("\033[0;35m %s nf_sysman_serial_enable : %d \033[0;39m\n", __FUNCTION__,nf_sysman_serial_enable);
		}
		//
		g_free(contents);
	}

	return TRUE;
}
// onvif_porting
#ifdef ENABLE_ONVIF_DEVICE
gchar *nf_sysman_get_serial_num(void)
{
	g_message("[%s:%d] %s\n", __FUNCTION__, __LINE__, _nf_sysman_serial_num);
	return _nf_sysman_serial_num;
}
#endif
// onvif_porting
gchar *nf_sysman_get_resolution(void)
{
	return _nf_sysman_res;
}

gchar *nf_sysman_get_output_mode(void)
{
#if defined(_IPX_32P5)
	return "hdmi"; // hdmi1, hdmi2
#else
	return _nf_sysman_output_mode;
#endif
}

gchar *nf_sysman_get_output_mode_live(void)
{
	return _nf_sysman_output_mode;
}

//support dual display
gint nf_sysman_dual_display_mode()
{
	return strtol( _nf_sysman_dual_display, NULL, 0 );
}
gchar *nf_sysman_get_hdmi_resolution(void)
{
	g_message("[%s:%d] %s\n", __FUNCTION__, __LINE__, _nf_sysman_res_hdmi);
	return _nf_sysman_res_hdmi;
}
gchar *nf_sysman_get_vga_resolution(void)
{
	g_message("[%s:%d] %s\n", __FUNCTION__, __LINE__, _nf_sysman_res_vga);
	return _nf_sysman_res_vga;
}

gchar *nf_sysman_get_system_model(void)
{
//  g_message("[%s:%d] %s\n", __FUNCTION__, __LINE__, _nf_sysman_model);
	return _nf_sysman_model;
}

guint nf_sysman_get_pba_type(void)
{
	guint ret = NF_SYSMAN_PBA_TYPE_UNKNOWN;

	if (strcmp(_nf_sysman_pba_type, "a_type" ) == 0) {
		ret = NF_SYSMAN_PBA_TYPE_A;
	} else if (strcmp(_nf_sysman_pba_type, "b_type" ) == 0) {
		ret =  NF_SYSMAN_PBA_TYPE_B;
	}

	return ret;
}

gboolean nf_sysman_sync( void )
{
	sync();
	proxy_system("/bin/sync",1,3);

	return 1;
}

gboolean nf_sysman_bdflush( void )
{
	proxy_system("echo 3 > /proc/sys/vm/drop_caches",1,3);
	return 1;
}

//gboolean nf_sysman_create_sysinfo(gchar *out_fullname)
gboolean nf_sysman_create_sysinfo(void)
{
	gchar buff[256];

	memset(buff, 0x00, sizeof(char)*256);

	proxy_system("/NFDVR/sysinfo.sh -all",1,15);

//	sprintf(buff,"mv /tmp/system_log.tar.gz %s", out_fullname);
//	proxy_system(buff,1,10);

	return 1;
}

gchar *nf_sysman_get_model_ch(void)
{
//	g_message("[%s:%d] %s\n", __FUNCTION__, __LINE__, _nf_sysman_model_ch);
	return _nf_sysman_model_ch;
}

guint *nf_sysman_get_serial_enable(void)
{
	return nf_sysman_serial_enable;
}

gboolean nf_sysman_set_delack( int ack_cnt )
{
	char buff[128];
	snprintf(buff, sizeof(buff), "sysctl -w net.ipv4.tcp_delack_segs=%d", ack_cnt);

	proxy_system(buff,1,3);
	return 1;
}


static void
_notify_cb_telnet_ctrl( NF_NOTIFY_INFO *pinfo, gpointer data ){


	if(pinfo->d.params[0] != NF_SYSDB_CATE_NET)
		return;

	nf_sysman_telnet_ctrl();

	return;
}

gboolean nf_sysman_telnet_ctrl_init( void ){

	static gulong _cb_handle = 0;

	if( _cb_handle != 0 )
		return 0;

	_cb_handle= nf_notify_connect_cb( "sysdb_change", _notify_cb_telnet_ctrl, NULL);
	g_message("%s connected cb_handle[%lu]",__FUNCTION__, _cb_handle);

	return 1;
}

gboolean nf_sysman_telnet_ctrl( void ){

	char *tmp_testmail;
	tmp_testmail = nf_sysdb_get_str_nocopy("net.email.testmail");


	if( strcmp( tmp_testmail, "debug_monitor@debug.com" ) == 0 ) {
		nf_sysman_telnet_start();
	}
	else
		nf_sysman_telnet_stop();


	return 1;
}

gboolean nf_sysman_telnet_stop( void )
{
	//proxy_system("killall -9 telnetd", 1, 3);
	proxy_system("killall -9 zabbix_agentd", 1, 3);
	//proxy_system("killall -9 iperf3", 1, 3);
	return 1;
}

gboolean nf_sysman_telnet_start( void )
{
	//proxy_system("telnetd start", 1, 3);
	proxy_system("/NFDVR/data/zabbix/zabbix_agentd", 1, 3);
	//proxy_system("iperf3 -s &", 1, 3);
	return 1;
}

int nf_sysman_capture_serial_ctrl()
{
	gboolean ret = FALSE;
	ret = nf_sysdb_get_bool("sys.diagnostic_data.enable");

	if (ret) {
		proxy_system("touch /tmp/capture_serial.conf", 1, 3);
	}
	else {
		proxy_system("rm -f /tmp/capture_serial.conf", 1, 3);
	}

	return 0;
}

static void _notify_cb_capture_serial_ctrl( NF_NOTIFY_INFO *pinfo, gpointer data )
{
	if(pinfo->d.params[0] != NF_SYSDB_CATE_SYS)
		return;

	nf_sysman_capture_serial_ctrl();

	return;
}

int nf_sysman_capture_serial_ctrl_init()
{
	static gulong _cb_handle = 0;

	if( _cb_handle != 0 )
		return 0;

	_cb_handle= nf_notify_connect_cb( "sysdb_change", _notify_cb_capture_serial_ctrl, NULL);
	g_message("%s connected cb_handle[%lu]",__FUNCTION__, _cb_handle);

	return 1;

}

gboolean nf_sysman_process_kill(gchar *processName, gchar *option)
{
	char* rbuf;
	gchar cmd[256] = {0,};
	gchar plist_file[32] = "/tmp/process_list";

	FILE *fp_plist = NULL;
	gchar pstr[256] = {0,};
	int retry_open_cnt = 0;

	g_return_val_if_fail(processName != NULL, 0);

	sprintf(cmd, "ps > %s", plist_file);
	proxy_system(cmd, 1, 3);

	for(retry_open_cnt = 0; retry_open_cnt < 3; retry_open_cnt++)
	{
		fp_plist = fopen(plist_file ,"r");
		if(fp_plist)
		{
			while(!feof(fp_plist))
			{
				gchar *pid = NULL;
				fgets(pstr, sizeof(pstr), fp_plist);
				if(strstr(pstr, processName))
				{
					if(option)
					{
						if(strstr(pstr, option))
						{
							pid = strtok_r(pstr, " ", &rbuf);
							sprintf(cmd, "kill -9 %s", pid);
							g_message("%s %d %s", __FUNCTION__, __LINE__, cmd);
							proxy_system(cmd, 1, 3);
							fclose(fp_plist);
							return TRUE;
						}
					}
					else
					{
						pid = strtok_r(pstr, " ", &rbuf);
						sprintf(cmd, "kill -9 %s", pid);
						g_message("%s %d %s", __FUNCTION__, __LINE__, cmd);
						proxy_system(cmd, 1, 3);
						fclose(fp_plist);
						return TRUE;
					}
				}
			}
			fclose(fp_plist);
		}
		g_usleep(500000);
	}

	return FALSE;
}

gboolean nf_sysman_get_mem_usage(NF_SYSMAN_USAGE_INFO* mem_info)
{
	char* rbuf;
	FILE	*fp = NULL;
	char buff[NF_SYSTEM_BUFF_SIZE] = {0,};
	char* token = NULL;
	char* str_tol_mem = NULL;
	char* str_free_mem = NULL;
	guint i = 0;

	fp = fopen( "/proc/meminfo", "r");

	if(fp == NULL)
	{
		g_warning("%s fopen failed", __FUNCTION__);
		return 0;
	}

	if(fread(buff, sizeof(char), NF_SYSTEM_BUFF_SIZE, fp) == 0)
	{
		g_warning("%s fread failed", __FUNCTION__);
		fclose(fp);
		return 0;
	}

	token = strtok_r(buff, " \n", &rbuf);

	while(token != NULL)
	{
		//g_message("%s %d %s", __FUNCTION__, i, token);

		if(i == 1)
		{
			str_tol_mem = token;
			mem_info->total = atoi(str_tol_mem);
		}
		else if(i == 4)
		{
			str_free_mem = token;
			mem_info->used = mem_info->total - atoi(str_free_mem);
			break;
		}
		token = strtok_r(NULL, " \n", &rbuf);
		i++;
	}

	//g_message("%s total_memory = %d", __FUNCTION__, mem_info->total);
	//g_message("%s used_memory = %d", __FUNCTION__, mem_info->used);

	fclose(fp);

	return 1;
}

gboolean nf_sysman_get_cpu_usage(guint* cpu_idle)
{
	char* rbuf;
	char buff[NF_SYSTEM_BUFF_SIZE] = {0,};
	FILE *fp = NULL;
	char* token = NULL;
	char* str_cpu_total_time = NULL;
	char* str_cpu_idle_time = NULL;
	guint cpu_total_time = 0;
	guint cpu_idle_time = 0;
	guint i = 0;

	fp = fopen( "/proc/stat", "r");

	if(fp == NULL)
	{
		g_warning("%s fopen failed", __FUNCTION__);
		return 0;
	}

	if(fread(buff, sizeof(char), NF_SYSTEM_BUFF_SIZE, fp) == 0)
	{
		g_warning("%s fread failed", __FUNCTION__);
		fclose(fp);
		return 0;
	}

	token = strtok_r(buff, "\n", &rbuf);
	token = strtok_r(token, " ", &rbuf);

	while(token != NULL)
	{
		//g_message("%s %d %s", __FUNCTION__, i, token);

		if(i == 4)
		{
			str_cpu_idle_time = token;
			cpu_idle_time = atoi(str_cpu_idle_time);
		}

		if(i != 0)
		{
			str_cpu_total_time = token;
			cpu_total_time += atoi(str_cpu_total_time);
		}

		token = strtok_r(NULL, " ", &rbuf);
		i++;
	}

	*cpu_idle = (cpu_idle_time * 100) / cpu_total_time;

	//g_message("%s cpu_total_time = %d", __FUNCTION__, cpu_total_time);
	//g_message("%s cpu_idle_time = %d", __FUNCTION__, cpu_idle_time);
	//g_message("%s cpu idle percent = %d", __FUNCTION__, *cpu_idle);

	fclose(fp);

	return 1;
}

gboolean nf_sysman_get_nand_usage(NF_SYSMAN_USAGE_INFO* nand_info)
{
	struct statfs lstatfs;

	if(statfs("/", &lstatfs) == -1)
	{
		g_warning("%s statfs failed", __FUNCTION__);
		return 0;
	}

	nand_info->total = lstatfs.f_blocks * (lstatfs.f_bsize / 1024);
	nand_info->used = nand_info->total - (lstatfs.f_bavail * (lstatfs.f_bsize / 1024));

	//g_message("%s nand_info->total [%d]", __FUNCTION__, nand_info->total);
	//g_message("%s nand_info->used [%d]", __FUNCTION__, nand_info->used);

	return 1;
}

#if 0			// pakkhman
gboolean nf_sysman_gui_tunning(void)
{

	if( !DISPLAY_IS_D1 )
	{
		g_warning("%s",__FUNCTION__);
			proxy_system("echo \"20000020 18\" >/sys/class/davinci_system/system/reg");
	}

	return 1;
}
#endif

/**
  after upgrade, signal( ntsc/ pal ) check
*/
gboolean nf_sysman_check_sig_type(void)
{
	gboolean is_pal=0, ret=0, sysdb_sig_type=0;
	NF_PARAM_APP app_param;

	g_message("%s called", __FUNCTION__);

	ret = nf_api_param_app_get(&app_param);
	if(ret == FALSE)
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}


	if(app_param.is_upgrade == 1)
	{
		g_message("%s is_upgrade[%d]", __FUNCTION__, app_param.is_upgrade);

		is_pal = nf_dev_board_pp_is_pal();
		sysdb_sig_type = nf_sysdb_get_bool("sys.info.sig_type");

		g_message("%s board_pp_is_pal [%d]", __FUNCTION__, is_pal);
		g_message("%s app_param.is_pal[%d]", __FUNCTION__, app_param.is_pal);
		g_message("%s sysdb sig_type  [%d]", __FUNCTION__, sysdb_sig_type);

		if(is_pal == app_param.is_pal)
		{
			if(sysdb_sig_type != is_pal)
			{
				_set_sysdb_sig_type(is_pal);
			}
		}else {		/* should format */
			if(sysdb_sig_type == is_pal)
			{
				_set_sysdb_sig_type(!is_pal);
			}
		}

		#if !defined(SUPPORT_NAND_128M)
		if (nf_api_param_app_set_upgrade_val(&app_param, 0) == 0) {
			g_warning("%s set_app_param error!!", __FUNCTION__);
			return FALSE;
		}
		#endif

	} else {
		sysdb_sig_type = nf_sysdb_get_bool("sys.info.sig_type");
		if(app_param.is_pal != sysdb_sig_type)
		{
			g_message("%s Set Siangl Type To Nand From Sysdb.. Type : %d -> %d", 
							__FUNCTION__, app_param.is_pal, sysdb_sig_type);

			nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_PAL, sysdb_sig_type);
		}
		else
			g_message("%s normal boot %d %d", __FUNCTION__, sysdb_sig_type, app_param.is_pal);

		return TRUE;
	}

	return TRUE;
}

// gboolean nf_sysman_set_app_param_upgrade_val(NF_SYSMAN_APP_PARAM *app_param, gboolean is_upgrade)
// {
// 	gint ret=0;
// 	g_message("%s called is_upgrade[%d]", __FUNCTION__, is_upgrade);

// 	app_param->is_upgrade = is_upgrade;

// 	#if defined(CONFIG_FWUPGRADE_SINGLE)
// 		if (is_upgrade == 0) {
// 			app_param->fwup_status = NF_FWUP_STATUS_DONE;
// 		}
// 	#endif

// 	ret = nf_sysman_set_app_param(app_param);
// 	if(ret == FALSE)
// 	{
// 		g_warning("%s nf_sysman_set_app_param!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	g_message("%s is_normal      [%d]", __FUNCTION__, app_param->is_normal);
// 	g_message("%s is_upgrade     [%d]", __FUNCTION__, app_param->is_upgrade);
// 	g_message("%s is_pal         [%d]", __FUNCTION__, app_param->is_pal);

// 	return TRUE;
// }

static void _set_sysdb_sig_type(gboolean sig_type)
{
	g_message("%s called [%d]", __FUNCTION__, sig_type);

	nf_sysdb_set_bool("sys.info.sig_type", sig_type);
	nf_sysdb_save("sys");
}

gboolean nf_sysman_user_set_resolution(guint val_res)
{
	nf_api_param_app_set_cate(NF_PARAM_APP_CATE_SET_RESOLUTION, val_res);

	return TRUE;
}

gboolean nf_sysman_user_set_resolution_dual(gint res_hdmi, gint res_vga)
{
	nf_api_param_app_set_cate(NF_PARAM_APP_CATE_SET_RESOLUTION, res_vga);
	nf_api_param_app_set_cate(NF_PARAM_APP_CATE_SET_RESOLUTION_HDMI, res_hdmi);

	return TRUE;
}

int nf_sysman_user_get_resolution(void)
{
	NF_PARAM_APP app_param;

	memset(&app_param, 0x0, sizeof(NF_PARAM_APP));

	if(!nf_api_param_app_get(&app_param)) {
		g_warning("%s get app param error!!", __FUNCTION__);
		return -1;
	}

	return (gint)app_param.val_res;
}

// gboolean nf_sysman_set_app_param_cate(gint cate, gint val)
// {
// 	gint ret = 0;
// 	NF_SYSMAN_APP_PARAM app_param;

// 	memset(&app_param, 0x0, sizeof(NF_SYSMAN_APP_PARAM));
// 	nf_sysman_get_app_param(&app_param);

// 	g_message("%s cate %d val %d", __FUNCTION__, cate, val);

// 	switch(cate)
// 	{
// 		case NF_SYSMAN_APP_PARAM_CATE_IS_NORMAL :
// 			app_param.is_normal = val;
// 			break;
// 		case NF_SYSMAN_APP_PARAM_CATE_IS_UPGRADE :
// 			app_param.is_upgrade = val;
// 			break;
// 		case NF_SYSMAN_APP_PARAM_CATE_IS_PAL :
// 			app_param.is_pal = val;
// 			break;
// 		case NF_SYSMAN_APP_PARAM_CATE_SET_RESOLUTION :
// 			app_param.val_res = val;
// 			break;
// 		default :
// 			g_warning("%s Unknown Cate!!", __FUNCTION__);
// 			goto nf_sysman_set_app_param_fail;
// 	}

// 	ret = nf_sysman_set_app_param(&app_param);

// 	if (ret == FALSE)
// 	{
// 		g_warning("%s nf_sysman_set_app_param!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	return TRUE;

// nf_sysman_set_app_param_fail:
// 	return FALSE;
// }

// gboolean nf_sysman_domestic_set_lic(void)   //check_check
// {
//   NF_SYSDB_LICENSE_INFO lic_info;
//     LicenseData lic_data;
//     gchar key[NF_SYSMAN_QC_MAX_LICENSE_CNT][36];
//     gchar lic_name[256];
//     gchar strBuf[256];
//     gint i, j, lic_cnt = 0, valid_key_cnt = 0;
// 	NF_SYSMAN_LIC_PARAM lic_param;
	
//     #if !(defined(_SUPPORT_DLVA) || defined(_SUPPORT_DMVA))
//     {
//     	printf("\033[0;33m %s VA is not supported. \033[0;39m\n", __FUNCTION__);
//     	return 0;
//     }
//     #endif
//     printf("\033[0;33m %s SET LICENSE START. \033[0;39m\n", __FUNCTION__);
    
//     memset(key, 0x00, sizeof(key));
//     memset(lic_name, 0x00, sizeof(lic_name));
//     memset(&lic_data, 0x00, sizeof(LicenseData));
// 	memset(&lic_param, 0x00, sizeof(NF_SYSMAN_LIC_PARAM));
//     //api ȣ�� 
// 	if(!(nf_sysman_get_lic_param(&lic_param)))
// 	{
// 		printf("\033[0;33m %s Fail get license \033[0;39m\n", __FUNCTION__);
// 		return 0;
// 	}
// 	if(lic_param.is_write != 1)
// 	{
// 		printf("\033[0;33m %s is_write[%d] \033[0;39m\n", __FUNCTION__,lic_param.is_write);
// 		return 0;
// 	}
// 	lic_cnt = lic_param.lic_cnt;
//     if (lic_cnt < 0 && lic_cnt > 16) {
//         printf("\033[0;33m %s CNT_ERROR[%d] \033[0;39m\n", __FUNCTION__,lic_cnt);
//         return 0;
//     }
// 	memcpy(key,lic_param.license,(MAX_LICENSE_CNT*36));
//     for (i = 0, j = 0; i < lic_cnt; i++)
//     {
//         memset(&lic_info, 0x00, sizeof(NF_SYSDB_LICENSE_INFO));

//         if (scm_license_decoding_key(-1, key[i], &lic_info) == 0)
//         {
//             if (strlen(lic_name) == 0) {
//                 sprintf(lic_name, "%s", lic_info.name);
//             }
//             else {
//                 sprintf(lic_name, ", %s", lic_info.name);
//             }

//             strcpy(lic_data.key_data[j].key, key[i]);
//             lic_data.key_data[j].acquired_date = ifn_get_local_timet();
//             valid_key_cnt++;
//             j++;
//         }
//     }

//     memset(strBuf, 0x00, sizeof(strBuf));

//     if (valid_key_cnt) {
//         sprintf(strBuf, "%d (%s)", valid_key_cnt, lic_name);
// 		printf("\033[0;33m %s %s. \033[0;39m\n", __FUNCTION__,strBuf);
//         lic_data.count = valid_key_cnt;
//         DAL_set_license_data(&lic_data);
//         DAL_save_db("sys");
//     }
//     else {
// 		printf("\033[0;33m %s There is not free license [%d]. \033[0;39m\n", __FUNCTION__,valid_key_cnt);
//     }
//     memset(&lic_param, 0xff, sizeof(NF_SYSMAN_LIC_PARAM));
// 	nf_sysman_set_lic_param(&lic_param);
	
//     return 0;
// }

// gboolean nf_sysman_set_lic_param(NF_SYSMAN_LIC_PARAM *lic_param)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs = NF_FLASH_PONG_APP_PARAM_MTD_LICENSE_OFFS;

// 	g_return_val_if_fail(dataBuf != NULL, 0);

// 	memset(dataBuf, 0xff, sizeof(dataBuf));
// 	memcpy(dataBuf, lic_param, sizeof(NF_SYSMAN_LIC_PARAM));

// //#ifdef DEBUG_SYSMAN_LOG
// #if 0
// 	if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_APP_PARAM ] )	
// 		nf_debug_hexdump( app_param, sizeof(NF_SYSMAN_APP_PARAM));
// #endif
	
// 	ret = nf_flash_erase_offs(NF_FLASH_PING_APP_PARAM_MTD_NUM, NULL, NULL,offs);
// 	if(!ret)
// 	{
// 		g_warning("%s Set LIC Param Nand Erase Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	ret = nf_flash_page_write(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
// 	if(!ret)
// 	{
// 		g_warning("%s Set LIC Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	return TRUE;
// }
// gboolean nf_sysman_get_lic_param(NF_SYSMAN_LIC_PARAM *lic_param)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs = 0;
// 	g_return_val_if_fail(dataBuf != NULL, 0);
// 	g_return_val_if_fail(lic_param != NULL, 0);

// 	//ret = nf_flash_read_offs(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, 0, dataBuf, NULL, NF_SYSMAN_LIC_PARAM_OFFSET);
// 	ret = nf_flash_read(NF_FLASH_PING_APP_PARAM_MTD_NUM, (int)NF_FLASH_PONG_APP_PARAM_MTD_LICENSE_OFFS, 0, dataBuf, NULL);
// 	if(!ret)
// 	{
// 		g_warning("%s Get App Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}
// #ifdef DEBUG_SYSMAN_APP_PARAM	
// 	flash_print(dataBuf, NULL, 0);
// #endif
// 	memcpy(lic_param, dataBuf, sizeof(NF_SYSMAN_APP_PARAM));
// #if 0
// //#ifdef DEBUG_SYSMAN_LOG
// 	if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_APP_PARAM ] )	
// 		nf_debug_hexdump( app_param, sizeof(NF_SYSMAN_APP_PARAM));
// #endif
// #if 0
// 	if(strncmp(app_param->magic, NF_APP_PARAM_MAGIC, 4) != 0 )
// 	{
// 		g_warning("%s magic failed[%4.4s]", __FUNCTION__, app_param->magic);
// 		/* setting default value */
		
// 		memset(app_param, 0x00, sizeof(NF_SYSMAN_APP_PARAM));
// 		strncpy(app_param->magic , NF_APP_PARAM_MAGIC, sizeof(NF_APP_PARAM_MAGIC));		
		
// 		nf_sysman_set_app_param(app_param);
		
// 		return TRUE;
// 	}
// #endif
// #if 1
// 	int i = 0;
// 	printf("********************************************\n");
// 	printf("LIC CNT [%d]\n",lic_param->lic_cnt);
// 	printf("IS WRITE [%d]\n",lic_param->is_write);
// 	for(i=0;i<lic_param->lic_cnt;i++)
// 		printf("LIC[%d] -> %s\n",i,lic_param->license[i]);
// 	printf("********************************************\n");
// #endif 
// 	return TRUE;
// }

// gboolean nf_sysman_set_app_param(NF_SYSMAN_APP_PARAM *app_param)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs =0;

// 	g_return_val_if_fail(dataBuf != NULL, 0);
// 	offs = 0;

// 	memset(dataBuf, 0xff, sizeof(dataBuf));
// 	memcpy(dataBuf, app_param, sizeof(NF_SYSMAN_APP_PARAM));

// #ifdef DEBUG_SYSMAN_LOG
// 	if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_APP_PARAM ] )
// 		nf_debug_hexdump( app_param, sizeof(NF_SYSMAN_APP_PARAM));
// #endif

// 	ret = nf_flash_erase(NF_FLASH_PING_APP_PARAM_MTD_NUM, NULL, NULL);
// 	if(!ret)
// 	{
// 		g_warning("%s Set App Param Nand Erase Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	ret = nf_flash_page_write(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
// 	if(!ret)
// 	{
// 		g_warning("%s Set App Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	return TRUE;
// }

// gboolean nf_sysman_get_app_param(NF_SYSMAN_APP_PARAM *app_param)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs = 0;

// 	g_return_val_if_fail(dataBuf != NULL, 0);
// 	g_return_val_if_fail(app_param != NULL, 0);

// 	offs = 0;

// 	ret = nf_flash_read(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
// 	if(!ret)
// 	{
// 		g_warning("%s Get App Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}
// #ifdef DEBUG_SYSMAN_APP_PARAM
// 	flash_print(dataBuf, NULL, 0);
// #endif

// 	memcpy(app_param, dataBuf, sizeof(NF_SYSMAN_APP_PARAM));

// #ifdef DEBUG_SYSMAN_LOG
// 	if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_APP_PARAM ] )
// 		nf_debug_hexdump( app_param, sizeof(NF_SYSMAN_APP_PARAM));
// #endif

// 	if(strncmp(app_param->magic, NF_APP_PARAM_MAGIC, 4) != 0 )
// 	{
// 		g_warning("%s magic failed[%4.4s]", __FUNCTION__, app_param->magic);
// 		/* setting default value */

// 		memset(app_param, 0x00, sizeof(NF_SYSMAN_APP_PARAM));
// 		strncpy(app_param->magic , NF_APP_PARAM_MAGIC, sizeof(NF_APP_PARAM_MAGIC));

// 		nf_sysman_set_app_param(app_param);

// 		return TRUE;
// 	}

// 	return TRUE;
// }

// gboolean nf_sysman_set_hw_param(NF_SYSMAN_HW_PARAM *hwparam)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs =0;


// 	g_return_val_if_fail(dataBuf != NULL, 0);
// 	offs = 0;

// 	if(strncmp(hwparam->magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0 )
// 	{
// 		g_warning("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
// 		return FALSE;
// 	}
// 	memset(dataBuf, 0xff, sizeof(dataBuf));
// 	memcpy(dataBuf, hwparam, sizeof(NF_SYSMAN_HW_PARAM));

// 	ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
// 	if(!ret)
// 	{
// 		g_warning("%s Set HW Param Nand Erase Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	ret = nf_flash_page_write(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
// 	if(!ret)
// 	{
// 		g_warning("%s Set HW Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	return TRUE;
// }

// gboolean nf_sysman_get_hw_param(NF_SYSMAN_HW_PARAM *hwparam, gboolean is_debug)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs=0;
// 	gint cnt_circular_buffer=0, cnt_qc_item=0;

// 	g_return_val_if_fail(dataBuf != NULL, 0);
// 	g_return_val_if_fail(hwparam != NULL, 0);

// 	offs=0;

// 	ret = nf_flash_read(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
// 	if(!ret)
// 	{
// 		g_warning("%s Get HW Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	memcpy(hwparam, dataBuf, sizeof(NF_SYSMAN_HW_PARAM));

// 	if(strncmp(hwparam->magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0 )
// 	{
// 		g_warning("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
// 		return FALSE;
// 	}

// 	if(is_debug)
// 	{
// 		g_message("Magic                 [%4.4s]", hwparam->magic);
// 		g_message("ETH ADDR              [%32.32s]",  hwparam->eth_addr);
// 		g_message("Front Type            [%32.32s]",  hwparam->pannel_type);
// 		g_message("RC    Type            [%32.32s]",  hwparam->rc_type);
// 		g_message("HW Version            [%32.32s]",  hwparam->hw_ver);
// 		g_message("Vendor                [%32.32s]",  hwparam->vendor);
// 		g_message("FW Write Fail         [%d]",  hwparam->is_fw_write_fail);
// 		g_message("Circular Buffer Count [%d]",  hwparam->circular_buffer_location);

// 		for(cnt_circular_buffer=0; cnt_circular_buffer<NF_SYSMAN_QC_CIRCULAR_BUFFER; cnt_circular_buffer++)
// 		{
// 			if(strncmp(hwparam->qc_result[cnt_circular_buffer].magic, NF_SYSMAN_QC_MAGIC, 4) != 0 )
// 			{
// 				g_warning("%s QC Circular Buffer%dMagic Failed[%4.4s]", __FUNCTION__, cnt_circular_buffer, hwparam->magic);
// 				continue;
// 			}

// 			g_message("QC Magic              [%4.4s]",  hwparam->qc_result[cnt_circular_buffer].magic);
// 			g_message("Excute Time           [%48.48s]",  hwparam->qc_result[cnt_circular_buffer].excute_time);

// 			for(cnt_qc_item=0; cnt_qc_item<NF_SYSMAN_QC_ITEM_MAX_NR; cnt_qc_item++)
// 				g_message("Excute Time           [%16.16s]",  hwparam->qc_result[cnt_circular_buffer].item[cnt_qc_item]);

// 			g_print("\n");
// 		}
// 	}

// 	return TRUE;
// }

// gboolean nf_sysman_get_protect_data(NF_UTIL_PROTECT_DATA *data)
// {  
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs=0;

// 	offs = NF_FLASH_PONG_HW_PARAM_MTD_PROTECTED_DATA_OFFS;

// 	ret = nf_flash_read(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
// 	if (!ret) {
// 		printf("\e[33m [%s] Get Protect Data From Nand Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	memcpy(data->enc_data, dataBuf, sizeof(data->enc_data));

// 	nf_dev_board_pp_dec_data(data);

// 	if (strncmp(data->dec_data, ITX_PROTECT_DATA_PARAM_MAGIC, 4) != 0 ) {
// 		// printf("\e[33m [%s] Magic Check Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	return TRUE;
// }

// gboolean nf_sysman_set_protect_data(NF_UTIL_PROTECT_DATA *data)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs=0;

// 	g_return_val_if_fail(dataBuf != NULL, 0);
// 	g_return_val_if_fail(data != NULL, 0);

// 	offs = NF_FLASH_PONG_HW_PARAM_MTD_PROTECTED_DATA_OFFS;

// 	memcpy(dataBuf, data->enc_data, sizeof(data->enc_data));

// 	ret = nf_flash_page_write(NF_FLASH_PONG_HW_PARAM_MTD_NUM, offs, dataBuf, NULL, 0, 0);
// 	if (!ret) {
// 		printf("\e[33m [%s] Set Protect Data Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	return TRUE;
// }

// guchar *nf_sysman_get_qc_mac(void)
// {
// 	return _nf_sysman_qc_mac;
// }

// gboolean nf_sysman_check_protect_data_param(ITX_PROTECT_DATA_PARAM *data_param, char *data)
// {
// 	int i = 0;
// 	char hw_info[30], data_info[30];
// 	int pba_type = nf_sysman_get_pba_type();

// 	printf("\e[33m [%s] data_param->eth_addr = %s\e[0m\n",
// 			__FUNCTION__, data_param->eth_addr);
// 	printf("\e[33m [%s] data_param->pannel_type = %s\e[0m\n",
// 			__FUNCTION__, data_param->pannel_type);
// 	printf("\e[33m [%s] data_param->rc_type = %s\e[0m\n",
// 			__FUNCTION__, data_param->rc_type);
// 	printf("\e[33m [%s] data_param->hw_ver = %s\e[0m\n",
// 			__FUNCTION__, data_param->hw_ver);
// 	printf("\e[33m [%s] data_param->serial_match = %s\e[0m\n",
// 			__FUNCTION__, data_param->serial_match);
// 	printf("\e[33m [%s] data_param->serial_rand = %s\e[0m\n",
// 			__FUNCTION__, data_param->serial_rand);

// 	memset(data_info, 0x00, sizeof(data_info));
// 	for (i = 0; i < 17; i++) {
// 		if (data_param->eth_addr[i] == ':') {
// 			data_info[i] = ':';
// 			continue;
// 		}
// 		data_info[i] = data[i];
// 	}

// 	if (strcasecmp(data_param->eth_addr, data_info) != 0) {
// 		printf("\e[33m [%s] Check Mac Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	if (strncmp(data_param->pannel_type, "FIPN501", sizeof("FIPN501")) != 0) {
// 		printf("\e[33m [%s] Check PT Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	if (strncmp(data_param->rc_type, "ICA_ITX", sizeof("ICA_ITX")) != 0) {
// 		printf("\e[33m [%s] Check RC Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	memset(hw_info, 0x00, sizeof(hw_info));
// 	#if defined(_IPX_1648M4)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4A 16CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4B 16CH");
// 		}
// 	#elif defined(_IPX_0824M4)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4A 8CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4B 8CH");
// 		}
// 	#elif defined(_IPX_32M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4AE 32CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4BE 32CH");
// 		}
// 	#elif defined(_IPX_1648M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4EA 16CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4EB 16CH");
// 			printf("\033[0;36m %s IMSI hw_info PBA BTYPE\033[0;39m\n", __FUNCTION__);
// 		}
// 	#elif defined(_IPX_0824M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4EA 8CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4EB 8CH");
// 		}
// 	#elif defined(_IPX_0412M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4EA 4CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4EB 4CH");
// 		}
// 	#elif defined(_IPX_1648P4E)|| defined(_IPX_32P4E)
// 		strcpy(hw_info, "IPX-P4E 16CH");
// 	#elif defined(_IPX_0824P4E)
// 		strcpy(hw_info, "IPX-P4E 8CH");
// 	//_IPX_32P5
// 	#else
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4A 4CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4B 4CH");
// 		}
// 	#endif

// 	if (strncmp(data_param->hw_ver, hw_info, sizeof(data_param->hw_ver)) != 0) {
// 		printf("\e[33m [%s] Check HW VER Fail\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}

// 	strncpy(_hw_param_hw_ver, data_param->hw_ver, sizeof(data_param->hw_ver));
// 	strncpy(_hw_param_panel_type, data_param->pannel_type, sizeof(data_param->pannel_type));
// 	strncpy(_hw_param_rc_type, data_param->rc_type, sizeof(data_param->rc_type));
// 	#if !defined(_IPX_1648P4E)|| !defined(_IPX_32P4E) || !defined(_IPX_0824P4E)
// 	snprintf(_nf_sysman_qc_mac, sizeof(_nf_sysman_qc_mac), "%c%c%c%c%c%c%c%c%c%c%c%c",
// 			data_param->eth_addr[0], data_param->eth_addr[1],
// 			data_param->eth_addr[3], data_param->eth_addr[4],
// 			data_param->eth_addr[6], data_param->eth_addr[7],
// 			data_param->eth_addr[9], data_param->eth_addr[10],
// 			data_param->eth_addr[12], data_param->eth_addr[13],
// 			data_param->eth_addr[15], data_param->eth_addr[16],
// 			data_param->eth_addr[18], data_param->eth_addr[19]);
// 	#endif
// 	printf("\e[33m [%s] HW VER : %s\e[0m\n", __FUNCTION__, nf_sysman_get_hw_ver());
// 	printf("\e[33m [%s] FRONT  : %s\e[0m\n", __FUNCTION__, nf_sysman_get_panel_type());
// 	printf("\e[33m [%s] RC     : %s\e[0m\n", __FUNCTION__, nf_sysman_get_rc_type());
// 	printf("\e[33m [%s] MAC    : %s\e[0m\n", __FUNCTION__, nf_sysman_get_qc_mac());

// 	return TRUE;
// }

gboolean nf_sysman_fwup_mount_prev_mtd(void)
{
	gint prev_mtd_no=0, ret=0;
	gchar dev_name[1024];

	memset(dev_name, 0x0, sizeof(dev_name));

	if(_is_nfs)
	{
		g_warning("%s Boot Mode is NFS..Skip fwup sysdb recover..", __FUNCTION__);
		return FALSE;
	}

	if(_boot_mtd_num == FW_UPGRADE_FILESYS_PING_MTD_NUM	||
			_boot_mtd_num == FW_UPGRADE_FILESYS_PONG_MTD_NUM)
	{
		g_message("%s _boot_mtd_num[%d]\n", __FUNCTION__, _boot_mtd_num);
	}
	else
	{
		g_warning("%s Invalid MTD NUM[%d]", __FUNCTION__, _boot_mtd_num);
		return FALSE;
	}

	#if defined(SUPPORT_UBIFS)
		if(_boot_mtd_num == FW_UPGRADE_FILESYS_PING_MTD_NUM)
			prev_mtd_no=NF_SYSMAM_UBIFS_PONG;
		else if(_boot_mtd_num == FW_UPGRADE_FILESYS_PONG_MTD_NUM)
			prev_mtd_no=NF_SYSMAM_UBIFS_PING;
	#else
		if(_boot_mtd_num == FW_UPGRADE_FILESYS_PING_MTD_NUM)
			prev_mtd_no=FW_UPGRADE_FILESYS_PONG_MTD_NUM;
		else if(_boot_mtd_num == FW_UPGRADE_FILESYS_PONG_MTD_NUM)
			prev_mtd_no=FW_UPGRADE_FILESYS_PING_MTD_NUM;
	#endif

	snprintf(dev_name, sizeof(dev_name), NF_SYSMAN_FS_DEV_NAME,  prev_mtd_no );

	g_message("%s _boot_mtd_num[%d] mtd_blk_name[%s]\n", __FUNCTION__, _boot_mtd_num, dev_name);

	/** create mnt dir and execute mount **/
	ret = mkdir(NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME, 0755);
	if(ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, NF_SYSMAN_FS_MOUNT_ROOT_DIR_NAME);

	ret = mkdir(NF_SYSMAN_FS_MOUNT_DIR_NAME, 0755);
	if(ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, NF_SYSMAN_FS_MOUNT_DIR_NAME);

	/** 1. umount ==> actually don't need **/
	umount(NF_SYSMAN_FS_MOUNT_DIR_NAME);
	/** 2. mount **/
	if((ret = mount(dev_name, NF_SYSMAN_FS_MOUNT_DIR_NAME,
							NF_SYSMAN_FS_MOUNT_FS_TYPE , 0, 0)) !=0)
	{
		g_warning("%s mount error!!! ret[%d] [%s]->[%s]", __FUNCTION__, ret, dev_name, NF_SYSMAN_FS_MOUNT_DIR_NAME);
		return FALSE;
	}else{
		g_message("%s mounted [%s]->[%s]", __FUNCTION__,
					dev_name, NF_SYSMAN_FS_MOUNT_DIR_NAME);
	}
#if 0
	/** 3. copy sysdb **/
	// ex) find ../../data/nf_sysdb_*.conf ! -name nf_sysdb_version.conf -exec cp -av {} ./ \;
//	snprintf(cp_str, STRING_LENGTH_1024-1, "find "NF_SYSDB_CONF_IMPORT_PATH_STR" ! -name nf_sysdb_version.conf "
	snprintf(cp_str, STRING_LENGTH_1024-1, "/bin/find "NF_SYSDB_CONF_IMPORT_PATH_STR" ! -name nf_sysdb_version.conf "
			"-exec cp -a {} %s \\;",
					NF_SYSMAN_FS_MOUNT_DIR_NAME, "*", NF_SYSDB_DATA_PATH_STR);
	system(cp_str);

	/** 4. sync **/
	system("sync");
	system("sync");
	system("sync");
	/** 5. umount **/
	umount(NF_SYSMAN_FS_MOUNT_DIR_NAME);
#endif

	return TRUE;
}

gboolean nf_sysman_fwup_bit_clr(void)
{
	static NF_PARAM_APP app_param;

	/** 6. upgrade bit clear **/
	if(!nf_api_param_app_get(&app_param))
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}

	if( !nf_api_param_app_set_upgrade_val(&app_param, 0) )
	{
		g_warning("%s Set App Param upgrade bit clear Error!!", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}

// static NF_SYSMAN_APP_PARAM _app_param;
// static int _app_param_load = 0;

gboolean nf_sysman_is_normal_boot(void)
{
	gint is_normal=0;

	nf_api_param_app_get_cate(NF_PARAM_APP_CATE_IS_NORMAL, &is_normal);

	if( is_normal & NF_SYSMAN_SHUTDOWN_TYPE_NORMAL) 
		return 1;
	else
		return 0;

}

gboolean nf_sysman_is_fwup_first_boot(void)
{
	gint is_upgrade=0;

	nf_api_param_app_get_cate(NF_PARAM_APP_CATE_IS_UPGRADE, &is_upgrade);

	if(is_upgrade == 1) 
		return 1;
	else
		return 0;	

}

gboolean nf_sysman_set_normal_boot(void)
{
	gint is_normal=0;

	if(!nf_api_param_app_get_cate(NF_PARAM_APP_CATE_IS_NORMAL, &is_normal)) {
		return FALSE;
	}

	is_normal |= NF_SYSMAN_SHUTDOWN_TYPE_NORMAL;

	if(!nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_NORMAL, is_normal)) {
		return FALSE;
	}
	
	g_message("%s ok!!",__FUNCTION__);
	return TRUE;
}

gboolean nf_sysman_set_standby_boot(void)
{
	gint ret;
	NF_PARAM_APP dataBuf;
	NF_PARAM_APP app_param;

	ret = nf_api_param_app_get(&dataBuf);
	if(!ret)
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	memcpy(&app_param, &dataBuf, sizeof(NF_PARAM_APP));
	app_param.is_normal |= NF_SYSMAN_SHUTDOWN_TYPE_STANDBY;

	ret = nf_api_param_app_set(&app_param);
	if(!ret)
	{
		g_warning("%s Set App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	g_message("%s ok!!",__FUNCTION__);
	return TRUE;
}

gboolean nf_sysman_clr_normal_boot(void)
{
	gint ret;
	NF_PARAM_APP dataBuf;
	NF_PARAM_APP app_param;

	ret = nf_api_param_app_get(&dataBuf);
	if(!ret)
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	memcpy(&app_param, &dataBuf, sizeof(NF_PARAM_APP));
	app_param.is_normal = 0;
	
	ret = nf_api_param_app_set(&app_param);
	if(!ret)
	{
		g_warning("%s Set App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	g_message("%s ok!!",__FUNCTION__);
	return TRUE;
}

// 0 : HDMI2, 1 : VGA
gint nf_sysman_get_hdmi2_vga_out(void)
{
	gint hdmi2_vga_out=0;

	nf_api_param_app_get_cate(NF_PARAM_APP_CATE_HDMI2_VGA_OUT, &hdmi2_vga_out);

	return hdmi2_vga_out;
}

// 0 : HDMI2, 1 : VGA
gboolean nf_sysman_set_hdmi2_vga_out(gint val)
{
	gint ret;
	NF_PARAM_APP dataBuf;
	NF_PARAM_APP app_param;

	ret = nf_api_param_app_get(&dataBuf);
	if(!ret)
	{
		g_warning("%s Get App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	memcpy(&app_param, &dataBuf, sizeof(NF_PARAM_APP));
	app_param.hdmi2_vga_out = val;
	
	ret = nf_api_param_app_set(&app_param);
	if(!ret)
	{
		g_warning("%s Set App Param Error!!!", __FUNCTION__);
		return FALSE;
	}
	
	g_message("%s ok!!",__FUNCTION__);
	return TRUE;
}

// static gboolean _get_fw_param_from_nand(NF_FW_IMAGE_LIST *fw_param)
// {
// 	guchar pingBuf[MAX_PAGE_SIZE]={0,};
// 	guchar pongBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean ret=FALSE;
// 	guint offs=0;
// 	NF_FW_IMAGE_LIST fw_param_ping;
// 	NF_FW_IMAGE_LIST fw_param_pong;
// 	NF_FW_IMAGE_LIST *fw_param_ptr=NULL;

// 	g_return_val_if_fail(pingBuf != NULL, 0);
// 	g_return_val_if_fail(pongBuf != NULL, 0);
// 	g_return_val_if_fail(fw_param != NULL, 0);

// 	memset(&fw_param_ping, 0x0, sizeof(NF_FW_IMAGE_LIST));
// 	memset(&fw_param_pong, 0x0, sizeof(NF_FW_IMAGE_LIST));
// 	offs=0;

// 	if(!nf_flash_read(NF_FLASH_PING_FW_PARAM_MTD_NUM, offs, 0, pingBuf, NULL))
// 	{
// 		g_warning("%s Get Ping FW Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	if(!nf_flash_read(NF_FLASH_PONG_FW_PARAM_MTD_NUM, offs, 0, pongBuf, NULL))
// 	{
// 		g_warning("%s Get Pong FW Param Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// #ifdef DEBUF_SYSMAN_FW_PARAM
// 	flash_print(pingBuf, NULL, 0);
// 	flash_print(pongBuf, NULL, 0);
// #endif

// 	memcpy(&fw_param_ping, pingBuf, sizeof(NF_FW_IMAGE_LIST));
// 	memcpy(&fw_param_pong, pongBuf, sizeof(NF_FW_IMAGE_LIST));

// #if 0
// #ifdef DEBUG_SYSMAN_LOG
// 	if( _DEBUG_SYSMAN_log[ DEBUG_SYSMAN_IDX_APP_PARAM ] )
// 		nf_debug_hexdump( app_param, sizeof(NF_SYSMAN_APP_PARAM));
// #endif
// #endif

// 	if(fw_param_ping.fwheader.seq > fw_param_pong.fwheader.seq)
// 		fw_param_ptr=&fw_param_ping;
// 	else
// 		fw_param_ptr=&fw_param_pong;

// 	if(strncmp(fw_param_ptr->fwheader.magic, FW_UPGRADE_NF_MAGIC_S, FW_UPGRADE_MAX_MAGIC_BUF) != 0)
// 	{
// 		g_warning("%s magic failed[%4.4s]", __FUNCTION__, fw_param_ptr->fwheader.magic);
// 		return FALSE;
// 	}

// 	memcpy(fw_param, fw_param_ptr, sizeof(NF_FW_IMAGE_LIST));

// 	return TRUE;
// }

// gboolean nf_sysman_get_fw_param_info(NF_SYSMAN_FW_INFO *fw_info)
// {
// 	g_return_val_if_fail(fw_info != NULL, 0);

// 	memcpy(fw_info, _fw_param_info, sizeof(_fw_param_info));

// 	return TRUE;
// }

// gboolean nf_sysman_fw_param_init(void)
// {
// 	NF_FW_IMAGE_LIST fw_param;
// 	guint i=0, info_type_cnt=0;

// 	memset(&fw_param, 0x0, sizeof(fw_param));

// 	_get_fw_param_from_nand(&fw_param);

// 	if(strncmp(fw_param.fwheader.magic, FW_UPGRADE_NF_MAGIC_S, strlen(FW_UPGRADE_NF_MAGIC_S)) != 0)
// 	{
// 		g_warning("%s F/W Header Magic Check Error!!!", __FUNCTION__);
// 		return FALSE;
// 	}

// 	for(i=0, info_type_cnt=0; i<fw_param.fwheader.sec_cnt; i++, info_type_cnt++)
// 	{

// #if 0	/** use anf / atm **/
// 		if(i == NF_SYSMAN_FW_INFO_TYPE_DSP)
// 			info_type_cnt++;
// #endif

// 		_fw_param_info[i].type=info_type_cnt;
// 		_fw_param_info[i].size=fw_ntohl(fw_param.fwheader.sec_header[i].ih_size);
// 		_fw_param_info[i].time=fw_param.fwheader.sec_header[i].ih_time;
// 		_fw_param_info[i].dcrc=fw_param.fwheader.sec_header[i].ih_dcrc;
// 		strncpy(_fw_param_info[i].name, fw_param.fwheader.sec_header[i].ih_name, sizeof(_fw_param_info[i].name)-1);

// #ifdef DEBUG_SYSMAN_APP_PARAM
// 		nf_fw_imgh_print(&fw_param.fwheader.sec_header[i]);
// #endif
// 	}

// 	/** last UBL **/
// 	_fw_param_info[i].type=NF_SYSMAN_FW_INFO_TYPE_UBL;
// 	_fw_param_info[i].size=0;
// 	_fw_param_info[i].time=0;
// 	_fw_param_info[i].dcrc=strtoul(_hw_param_ubl_crc, NULL, 16);
// 	strncpy(_fw_param_info[i].name, "UBL", sizeof(_fw_param_info[i].name)-1);

// 	return TRUE;
// }

#if defined(_HDI_0412)
gboolean nf_sysman_hdi_video_info_init(void)
{
	gint num_ch=0;
	gchar *contents=NULL;
	gsize  length = 0;
	gchar *ret, str[32]={0, };
	GError *error = NULL;

	if (!g_file_get_contents ( FILE_PROC_CMDLINE , &contents, &length, &error))
	{
		g_warning("%s %s\n", __FUNCTION__, error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return FALSE;
	}

	if(contents)
	{
		gchar *tmp = NULL;
		g_message("%s cmdline[%s]", __FUNCTION__, contents);

		for(num_ch=0; num_ch<NUM_ACTIVE_CH; num_ch++)
		{
			sprintf(str, HDI_VIDEO_INFO_PARAM0, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].type_name, sizeof(_hdi_video_info[num_ch].type_name),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].type_name, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].type_name);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM1, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].rate_det1, sizeof(_hdi_video_info[num_ch].rate_det1),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].rate_det1, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].rate_det1);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM2, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].rate_det0, sizeof(_hdi_video_info[num_ch].rate_det0),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].rate_det0, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].rate_det0);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM3, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].lines_per_field, sizeof(_hdi_video_info[num_ch].lines_per_field),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].lines_per_field, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].lines_per_field);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM4, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].active_lines_per_field, sizeof(_hdi_video_info[num_ch].active_lines_per_field),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].active_lines_per_field, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].active_lines_per_field);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM5, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].words_per_active_line, sizeof(_hdi_video_info[num_ch].words_per_active_line),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].words_per_active_line, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].words_per_active_line);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM6, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(_hdi_video_info[num_ch].words_per_line, sizeof(_hdi_video_info[num_ch].words_per_line),
							tmp + (strlen(str)));
				tmp = strchr(_hdi_video_info[num_ch].words_per_line, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(_hdi_video_info[num_ch].words_per_line);
			}
			sprintf(str, HDI_VIDEO_INFO_PARAM7, num_ch);
			if((tmp = strstr(contents, str)))
			{
				snprintf(str, sizeof(str), tmp + (strlen(str)));
				tmp = strchr(str, ' '); if( tmp ) *tmp = 0x0;
				g_strstrip(str);
				_hdi_video_info[num_ch].value==strtoul(str, NULL, 16);
			}

			g_message("==========================================================================");
			g_message("Receiver Debug Ch%d\n", num_ch+1);
			g_message("Active Video Area      [%s]", _hdi_video_info[num_ch].type_name);
			g_message("RATE DET[1] HD/Full HD [%s]", _hdi_video_info[num_ch].rate_det1);
			g_message("RATE DET[0] SD/HD      [%s]", _hdi_video_info[num_ch].rate_det0);
			g_message("Lines per Field        [%s]", _hdi_video_info[num_ch].lines_per_field);
			g_message("Active Lines per Field [%s]", _hdi_video_info[num_ch].active_lines_per_field);
			g_message("Words per Active Line  [%s]", _hdi_video_info[num_ch].words_per_active_line);
			g_message("Words per Line         [%s]", _hdi_video_info[num_ch].words_per_line);
			g_message("VD_STD                 [0x%02x]", _hdi_video_info[num_ch].value);
			g_message("==========================================================================\n");

		}

		g_free(contents);
	}

	return TRUE;
}

void nf_sysman_get_hdi_video_info(NF_HDI_VIDEO_INFO *hdi_video_info)
{
	memcpy(hdi_video_info, _hdi_video_info, sizeof(_hdi_video_info));
}

#endif

gint nf_sysman_get_system_type(void)
{
	gchar *type=nf_sysman_get_system_model();

	g_message("%s line%d str %s", __FUNCTION__, __LINE__, type);

	if(strcmp(type, "ANF4G_0816") == 0)
		return NF_SYSMAN_SYSTEM_TYPE0;
	else if(strcmp(type, "ANF4GE_0816") == 0)
		return NF_SYSMAN_SYSTEM_TYPE10;
	else if(strcmp(type, "ANF4G_04") == 0)
		return NF_SYSMAN_SYSTEM_TYPE0;
	else if(strcmp(type, "ANF4GE_04") == 0)
		return NF_SYSMAN_SYSTEM_TYPE10;
	else if(strcmp(type, "ATM4G_0816") == 0)
		return NF_SYSMAN_SYSTEM_TYPE0;
	else if(strcmp(type, "ATM4GE_0816") == 0)
		return NF_SYSMAN_SYSTEM_TYPE10;
	else if(strcmp(type, "ATM4G_04") == 0)
		return NF_SYSMAN_SYSTEM_TYPE0;
	else if(strcmp(type, "ATM4GE_04") == 0)
		return NF_SYSMAN_SYSTEM_TYPE10;
	else if(strcmp(type, "ANF4G_0816_AHD") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "ANF4G_04_AHD") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "ATM4G_0816_AHD") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "ATM4G_04_AHD") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "UTM4GX_04") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "UTM4GX_08") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "UTM4GX_16") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "UTM4GX_16") == 0)
		return NF_SYSMAN_SYSTEM_TYPE7;
	else if(strcmp(type, "IPXVE3") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX0824P3") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX1648P3") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX0824P3E") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX1648P3E") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPXWFQ") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX1648P4") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX0824P4") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else if(strcmp(type, "IPX0412L4") == 0)
		return NF_SYSMAN_SYSTEM_TYPE9;
	else
		return NF_SYSMAN_SYSTEM_UNKNOWN;
}

// #if defined(_SUPPORT_DUALMONITOR)
gboolean nf_sysman_set_dual_display(gboolean is_dual)
{
	nf_api_param_app_set_cate(NF_PARAM_APP_CATE_DUAL, is_dual);

	return TRUE;
}

gboolean nf_sysman_dual_disp_cmp_flag(void)
{
	// gint ret;
	// gboolean is_dual_nand=0, is_dual_db=0;
	// NF_SYSMAN_APP_PARAM app_param;

	// memset(&app_param, 0x0, sizeof(app_param));
	// ret = nf_sysman_get_app_param(&app_param);
	// if(!ret)
	// {
	// 	g_warning("%s Get App Param Error!!!", __FUNCTION__);
	// 	return FALSE;
	// }

	// is_dual_nand=app_param.is_dual;
	// is_dual_db = nf_sysdb_get_bool("disp.monitor.dualmonitor");
	// g_message("%s dual disp flah nand[%d] db[%d]", __FUNCTION__, is_dual_nand, is_dual_db);

	// if((is_dual_nand == FALSE) && (is_dual_db == TRUE))
	// {
	// 	g_message("%s Force Flag Change To True!!", __FUNCTION__);
	// 	app_param.is_dual=TRUE;

	// 	ret = nf_sysman_set_app_param(&app_param);
	// 	if(!ret)
	// 	{
	// 		g_warning("%s Set App Param Error!!!", __FUNCTION__);
	// 		return FALSE;
	// 	}
	// }

	return TRUE;
}
// #endif

guint nf_sysman_get_hub_info(int info_idx)
{
	guint info=0;
	int i=0;

	switch(info_idx)
	{
	case NF_SYSMAN_HUB_INFO_LINKED_MASK:
		for(i=0; i<4; i++)
		{
			if(hub_get_status(i) & HUB_PORT_STATE_LINKED)
				info |= (1<<i);
		}
		break;
	case NF_SYSMAN_HUB_INFO_LINKED_NUM:
		for(i=0; i<4; i++)
		{
			if(hub_get_status(i) & HUB_PORT_STATE_LINKED)
				info++;
		}
		break;
	}

	return info;
}

gboolean nf_sysman_switch_vct_result(NF_SYSMAN_SWITCH_VCT_RES *vct_info)
{
	int ret=0, recv_timeout_cnt=0, link_timeout_cnt=0, log_ret=0;
	guint dev_num=0;
	char event_log_text[256] = {0,};
	NF_UTIL_SWITCH_VCT_INFO vct_res;

	g_message("%s called", __FUNCTION__);

	memset(&vct_res, 0x00, sizeof(vct_res));

	dev_num = vct_info->dev_num;
	vct_res.port = vct_info->port;
#if 0
	if(dev_num == 0)
	{
		if(vct_res.port == 8)
			snprintf(event_log_text, sizeof(event_log_text),"VCT : NVR WAN port");
		else if(vct_res.port == 9)
			snprintf(event_log_text, sizeof(event_log_text),"VCT : NVR LAN port");
		else
			snprintf(event_log_text, sizeof(event_log_text),"VCT : NVR CAM %d port", vct_res.port+1);
	}
	else
	{
		if(vct_res.port == 8)
			snprintf(event_log_text, sizeof(event_log_text),"VCT : HUB%d WAN port", dev_num);
		else if(vct_res.port == 9)
			snprintf(event_log_text, sizeof(event_log_text),"VCT : HUB%d LAN port", dev_num);
		else
			snprintf(event_log_text, sizeof(event_log_text),"VCT : HUB%d CAM %d port", dev_num, vct_res.port+1);
	}
	log_ret = nf_eventlog_put_param( NULL, LT_NETWORK_EVENT, 0, 0, event_log_text);
	g_message("%s PUT_EVENT_LOG : %s ret[%d]", __FUNCTION__, event_log_text, log_ret);
#endif
	if(dev_num < 0 || dev_num > 4)
	{
		g_message("%s invalid dev_num[%d]", __FUNCTION__, dev_num);
		vct_res.result[0] = NF_SYSMAN_SWITCH_VCT_FAIL;
		return 0;
	}

	if(dev_num == 0)
	{
		//ret = nf_dev_board_pp_switch_vct(&vct_res);
		ret = 0;
	}
	else
	{
		while(link_timeout_cnt < 5)
		{
			if(nf_sysman_get_hub_info(HUB_PORT_STATE_LINKED))
			{
				g_message("%s HUB linked", __FUNCTION__);
				link_timeout_cnt = 0;
				break;
			}
			g_message("%s HUB not linked", __FUNCTION__);
			link_timeout_cnt++;
			sleep(1);
		}

		dev_num -= 1; // set hub index (0~3)
		hub_set_port_vct(dev_num, vct_res.port);
		g_message("%s dev_num[%d], vct_res.port [%d]", __FUNCTION__, dev_num, vct_res.port);

		while(recv_timeout_cnt < 30)
		{
			if(HUB_VCT_STATUS_DONE == hub_get_port_vct_status(dev_num, vct_res.port))
			{
				g_message("%s HUB vct info get Success", __FUNCTION__);
				hub_get_port_vct(dev_num, &vct_res);
				ret = 1;
				recv_timeout_cnt = 0;
				break;
			}

			recv_timeout_cnt++;
			sleep(1);
		}

		if(recv_timeout_cnt || link_timeout_cnt)
		{
			g_message("%s HUB vct info get Time Out", __FUNCTION__);
			vct_res.result[0] = NF_SYSMAN_SWITCH_VCT_FAIL;
			ret = 0;
		}
	}

	if((vct_res.result[0] == NF_SYSMAN_SWITCH_VCT_PASS) && (vct_res.result[1] == NF_SYSMAN_SWITCH_VCT_PASS))
	{
		vct_info->result = NF_SYSMAN_SWITCH_VCT_PASS;
		vct_info->length = 0;
	}
	else
	{
		if(vct_res.result[0] == NF_SYSMAN_SWITCH_VCT_PASS)
		{
			vct_info->result = vct_res.result[1];
			vct_info->length = vct_res.length[1];
			vct_info->cable_num = 0x24; // 00100100
		}
		else
		{
			vct_info->result = vct_res.result[0];
			vct_info->length = vct_res.length[0];
			vct_info->cable_num = 0x3; // 00000011
		}
	}

	g_message("%s end result[%d] length[%d]", __FUNCTION__, vct_info->result, vct_info->length);
	return ret;
}

#ifdef DEBUG_SYSMAN_JBSHELL

// static char sys_app_param_help[] = "sys_app_param 0 => read\n"
// 									"sys_app_param 1 [0 or 1] ==> write\n"
// 									"sys_app_param 2 ==> erase \n";
// static int sys_app_param(int argc, char **argv)
// {
// 	guchar dataBuf[MAX_PAGE_SIZE]={0,};
// 	gboolean is_write=0, ret=0;
// 	NF_SYSMAN_APP_PARAM app_param;
// 	guint offs=0;

// 	if(argc < 2){
// 		printf("%s\n", sys_app_param_help);
// 		return -1;
// 	}

// 	is_write = (gboolean)strtoul(argv[1], NULL, 0);

// 	if(is_write == 1)
// 	{
// 		gint val=0;
// 		ret = nf_flash_read(NF_FLASH_PING_APP_PARAM_MTD_NUM, offs, 0, dataBuf, NULL);
// 		if(!ret)
// 		{
// 			g_warning("%s Get App Parameter Error!!!", __FUNCTION__);
// 			return FALSE;
// 		}

// 		memcpy(&app_param, dataBuf, sizeof(app_param));

// 		if(strncmp(app_param.magic, NF_APP_PARAM_MAGIC, 4) != 0)
// 		{
// 			/* setting default value */
// 			g_print("set default value\n");
// 			strncpy(app_param.magic , NF_APP_PARAM_MAGIC, sizeof(NF_APP_PARAM_MAGIC));
// 			app_param.is_normal = 0;
// 		}

// 		if(argc > 2)
// 			val=(gint)strtoul(argv[2], NULL, 0);
// 		else
// 			return FALSE;

// 		app_param.is_normal = val;

// 		ret = nf_sysman_set_app_param(&app_param);
// 		if(!ret)
// 		{
// 			g_warning("%s Set App Param Error!!!", __FUNCTION__);
// 			return FALSE;
// 		}
// 	}
// 	else if(is_write == 2)
// 	{
// 		ret = nf_flash_erase(NF_FLASH_PING_APP_PARAM_MTD_NUM, NULL, NULL);
// 		if(!ret)
// 		{
// 			g_warning("%s Set App Param Nand Erase Error!!!", __FUNCTION__);
// 			return FALSE;
// 		}
// 	}
// 	else if(is_write == 0)
// 	{
// 		nf_sysman_get_app_param(&app_param);

// 		g_print("%s is_normal      [%d]\n", __FUNCTION__, app_param.is_normal);
// 		g_print("%s is_upgrade     [%d]\n", __FUNCTION__, app_param.is_upgrade);
// 		g_print("%s is_pal         [%d]\n", __FUNCTION__, app_param.is_pal);

// 	}
// 	else
// 	{
// 		printf("%s\n", sys_app_param_help);
// 		return -1;
// 	}

// 	return 0;
// }
// __commandlist(sys_app_param,"sys_app_param", sys_app_param_help, sys_app_param_help);

// static char sys_hw_param_help[] = "sys_hw_param";
// static int sys_hw_param(int argc, char **argv)
// {
// 	g_print("_factory_default  [%d]\n", _factory_default   );
// 	g_print("_passwd_reset     [%d]\n", _passwd_reset 	    );
// 	g_print("_fwupgrade        [%d]\n", _fwupgrade 		);
// 	g_print("_qa               [%d]\n", _qa 			    );
// 	g_print("_consol_enable    [%d]\n", _consol_enable     );
// 	g_print("_is_nfs           [%d]\n", _is_nfs     );
// 	g_print("_is_format        [%d]\n", _is_format     );

// 	g_print("_hw_param_hw_ver  [%s]\n", _hw_param_hw_ver     );
// 	g_print("_hw_param_vendor  [%s]\n", _hw_param_vendor     );
// 	g_print("_hw_param_ubl_crc [%s]\n", _hw_param_ubl_crc    );
// 	g_print("_hw_param_panel_type [%s]\n", _hw_param_panel_type );
// 	g_print("_hw_param_rc_type [%s]\n", _hw_param_rc_type );

// 	return 0;
// }
// __commandlist(sys_hw_param,"sys_hw_param", sys_hw_param_help, sys_hw_param_help);

// static char sys_fw_param_help[] = "sys_fw_param";
// static int sys_fw_param(int argc, char **argv)
// {
// 	if ( argc < 2 ) {
// 		printf("Invalid arguments\n%s\n", sys_fw_param_help);
// 		return -1;
// 	}

// 	if(strcmp(argv[1], "set") == 0)
// 		nf_sysman_fw_param_init();
// 	else if(strcmp(argv[1], "get") == 0)
// 	{
// 		NF_SYSMAN_FW_INFO fw_info[16];
// 		int i=0;

// 		memset(fw_info, 0x0, sizeof(fw_info));
// 		nf_sysman_get_fw_param_info(fw_info);

// 		for(i=0; i<16; i++)
// 		{
// 			g_message("type [%08x]", fw_info[i].type);
// 			g_message("size [%08x]", fw_info[i].size);
// 			g_message("time [%08x]", fw_info[i].time);
// 			g_message("dcrc [%08x]", fw_info[i].dcrc);
// 			g_message("name [%-32.32s]\n", fw_info[i].name);
// 		}
// 	}
// 	else
// 	{
// 		printf("Invalid arguments\n%s\n", sys_fw_param_help);
// 		return -1;
// 	}

// 	return 0;
// }
// __commandlist(sys_fw_param,"sys_fw_param", sys_fw_param_help, sys_fw_param_help);

static char sys_time_set_help[] = "sys_time_set [sec]";
static int sys_time_set(int argc, char **argv)
{
	glong tv_sec=0;
	GTimeVal tval={0,0};
	struct tm tm;
	char utc_time[32];

	if(argc < 2){
		printf("%s\n",sys_time_set_help);
		return -1;
	}

	tv_sec = (glong)strtoul(argv[1],NULL,0);

	gmtime_r(&tv_sec, &tm);
	asctime_r(&tm, utc_time);
	utc_time[strlen(utc_time)-1] = 0;

	tval.tv_sec = tv_sec;
	printf("nf_sysman_time_set(%ld/%s) ret[%d]\n", tv_sec, utc_time, nf_sysman_time_set(tval));

	return 0;
}
__commandlist(sys_time_set,"sys_time_set", sys_time_set_help, sys_time_set_help);

static char sys_hwparam_test_jbshell_cmd_help[] = "sys_hwparam [is_debug]";
static int sys_hwparam_test_jbshell_cmd(int argc, char **argv)
{
	NF_PARAM_HW hwparam;
	gboolean is_debug=FALSE;

	if(argc < 2){
		printf("%s\n", sys_hwparam_test_jbshell_cmd_help);
		return -1;
	}

	is_debug = (gboolean)strtol(argv[1], NULL, 0);

	memset(&hwparam, 0x0, sizeof(NF_PARAM_HW));
	if(!nf_api_param_hw_get_protect(&hwparam, is_debug))
	{
		g_warning("%s Get HW Param Error!!", __FUNCTION__);
		return -1;
	}
	else
		printf("%s\n", sys_hwparam_test_jbshell_cmd_help);

	return 0;
}
__commandlist(sys_hwparam_test_jbshell_cmd, "sys_hwparam", sys_hwparam_test_jbshell_cmd_help, sys_hwparam_test_jbshell_cmd_help);
#endif

int nf_sysman_qrcode_gen(char *filename, char *ext, char *msg)
{
	char cmd[512];

	g_assert(filename);
	g_assert(ext);
	g_assert(msg);

	if(strcmp(ext, "png"))
		return -1;

	snprintf( cmd, sizeof(cmd), "qrencode '%s' --foreground=080808 -t PNG -o %s.%s", msg, filename, ext);

	proxy_system( cmd, 1, 3);

	return 0;
}

//#define DEBUG_REBOOT_INFO

NF_SYSMAN_REBOOT_INFO reboot_info;

static int _analize_nand_log(struct log_data_t *nlog, int start, int end, char *string)
{
	int i;

	for( i = start; i >= end ; i-- )
	{
		if( nlog[i].ucType == LT_SYSTEM_DEBUG )
		{
			if( nlog[i].ucParam2 == LP2_SYSTEM_DEBUG_WATCHDOG )
			{
				char *p = NULL;
				int ch;

				p = strstr(nlog[i].sText, "ipcam recv err ch[");
				if(p){
					p += strlen("ipcam recv err ch[");

					while( *p == ' ' )
						p++;

					ch = atoi(p);
					ch += 1;
					sprintf(string, "CH%d", ch);

					return 1;
				}
			}
			else if( nlog[i].ucParam2 == LP2_SYSTEM_DEBUG_IFS_HOLD_SYSTEM )
			{
				char *p = NULL;

				p = strstr(nlog[i].sText, "advance_rdbit_recovery");
				if(p){
					if( strstr(p, ":STOP") )
						return 4;
				}

				p = strstr(nlog[i].sText, "IO ERR");
				if(p){
					char *id = NULL;
					int ch;

					id = strstr(nlog[i].sText, "DISK(");
					if(id){
						id += strlen("DISK(");

						while( *id == ' ' )
							id++;

						ch = atoi(id);
						ch += 1;
						sprintf(string, "ID%d", ch);

						return 2;
					}
				}

			}
	/*
			else if( nlog->ucParam2 == LP2_SYSTEM_DEBUG_UI_HOLD_SYSTEM_BACKTRACE )
			{
				return 4;

			}
	*/
		}
	}

	for( i = start; i >= end ; i-- )
	{
		if( nlog[i].ucType == LT_SYSTEM_DEBUG )
		{
			if( nlog[i].ucParam2 == LP2_SYSTEM_DEBUG_WATCHDOG )
			{
				char *p = NULL;

				p = strstr(nlog[i].sText, "Watchdog RESET");
				if(p){
					if( strstr(nlog[i].sText, "RECORD") )
					{
						return 3;
					}
				}
			}
		}
	}

	return 0;
}

gboolean nf_sysman_analize_nand_log(void)
{
	struct log_data_t log[1024], sort_log[1024];
	int k;
	int log_start = -1;
	int log_end = -1;

	int end_idx = -1;

	int log_end_time = -1;
	guint64 end_timestamp = 0;

	int ret = 0;

	GTimeVal time;

	guint64 max_logid= 0;
	int max_index = 0;

	g_message("%s => called" , __FUNCTION__);

	memset(&reboot_info, 0x0, sizeof(NF_SYSMAN_REBOOT_INFO));

	memset(log,			0x0, sizeof(struct log_data_t) * 1024);
	memset(sort_log, 	0x0, sizeof(struct log_data_t) * 1024);

	sst_get_nand_log(log);

#ifdef DEBUG_REBOOT_INFO
	printf("\n\nAll View\n");

	for(k=0;k< 1024;k++)
	{
		if(log[k].ulLogID == 0x00)
			continue;

		printf("ORI K=%d log type[%d] time[%lld] ucParam1[%d] ucParam2[%d] logid[%lld] text[%s]\n", k, log[k].ucType,log[k].ulTimeStamp,log[k].ucParam1,log[k].ucParam2,log[k].ulLogID,log[k].sText);
	}
#endif

	for(k=0;k< 1024;k++)
	{
		if(log[k].ulLogID == 0x00)
			continue;

		if(log[k].ulLogID > max_logid)
		{
			max_logid = log[k].ulLogID;
			max_index = k;
		}
	}

	if(max_logid == 0)
	{
		g_message("%s => not found max_index" , __FUNCTION__);
		return FALSE;
	}
	else
	{
		if( max_index == 1023 )
			memcpy( sort_log, log, sizeof(struct log_data_t) * 1024);
		else
		{
			if( log[max_index+1].ulLogID != 0x0 )
			{
				memcpy( sort_log, &(log[max_index+1]), sizeof(struct log_data_t) * (1024 - (max_index+1) ));
				memcpy( &(sort_log[1024 - (max_index+1)]), log, sizeof(struct log_data_t) * (max_index + 1));
			}
			else
			{
				memcpy( sort_log, log, sizeof(struct log_data_t) * (max_index+1) );
			}
		}
	}

#ifdef DEBUG_REBOOT_INFO
	printf("\n\nAll View\n");

	for(k=0;k< 1024;k++)
	{
		if(sort_log[k].ulLogID == 0x00)
			continue;

		printf("SORT K=%d log type[%d] time[%lld] ucParam1[%d] ucParam2[%d] logid[%lld] text[%s]\n", k, sort_log[k].ucType,sort_log[k].ulTimeStamp,sort_log[k].ucParam1,sort_log[k].ucParam2,sort_log[k].ulLogID,sort_log[k].sText);
	}
#endif

	gettimeofday(&time, NULL);

	time.tv_sec -= (10 * 60);

	end_timestamp = GTIMEVAL_TO_GUINT64(time);

	for( k=1023 ; k >= 0 ; k--)
	{
		if( sort_log[k].ulLogID == 0x00 )
			continue;

		if( log_start == -1 ){
			log_start = k;
		}

		if( sort_log[k].ucType == LT_SYSTEM_STARTED )
		{
			log_end = k;
			break;
		}
	}

	if( log_start == -1 )
	{
		g_message("%s => not found nand_log" , __FUNCTION__);
		return FALSE;
	}

	if( log_end == -1 )
	{
		log_end = 0;
	}

	for( k = log_start ; k >= 0 ; k--)
	{
		if(	end_timestamp < sort_log[k].ulTimeStamp )
		{
			log_end_time = k;
		}
		else
		{
			break;
		}
	}

	if( log_end_time == -1 )
	{
		g_message("%s => not within range" , __FUNCTION__);
		return FALSE;
	}

	if( log_end > log_end_time )
	{
		end_idx = log_end;
	}
	else
	{
		end_idx = log_end_time;
	}

	g_message("%s - log_start:%d, log_end:%d, log_end_time:%d, end_timestamp:%llu", __FUNCTION__, log_start, log_end, log_end_time, end_timestamp);

#ifdef DEBUG_REBOOT_INFO
	printf("\n\nSelection View\n");

	for( k = log_start; k >= end_idx ; k-- )
	{
		printf("K=%d log type[%d] time[%lld] ucParam1[%d] ucParam2[%d] logid[%lld] text[%s]\n", k, sort_log[k].ucType,sort_log[k].ulTimeStamp,sort_log[k].ucParam1,sort_log[k].ucParam2,sort_log[k].ulLogID,sort_log[k].sText);
	}
#endif

	char string[256];

	memset(string, 0x0, sizeof(string));

	ret = _analize_nand_log(sort_log, log_start, end_idx, string);

	reboot_info.error_code = ret;
	snprintf( reboot_info.content, sizeof(reboot_info.content), "%s", string);

	g_message("%s - err_code[%u], content[%s]", __FUNCTION__, reboot_info.error_code, reboot_info.content);

	return TRUE;
}

gboolean nf_sysman_get_reboot_info(NF_SYSMAN_REBOOT_INFO *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	memcpy(info, &reboot_info, sizeof(NF_SYSMAN_REBOOT_INFO));

	return TRUE;
}

static char sys_get_reboot_log_help[] = "sys_get_reboot_log";
static int sys_get_reboot_log(int argc, char **argv)
{
	NF_SYSMAN_REBOOT_INFO info;

	if( nf_sysman_get_reboot_info(&info) )
	{
		g_message("%s - err_code[%u], content[%s]", __FUNCTION__, info.error_code, info.content);
	}
	else
	{
		g_message("%s => nf_sysman_get_reboot_info[FALSE]", __FUNCTION__);
	}

	return 0;
}
__commandlist(sys_get_reboot_log, "sys_get_reboot_log", sys_get_reboot_log_help, sys_get_reboot_log_help);

static char sys_get_nand_log_help[] = "sys_get_nand_log";
static int sys_get_nand_log(int argc, char **argv)
{
	struct log_data_t log[1024];

	int k;

	sst_get_nand_log(log);

	printf("\n\n");

	for(k=0;k< 1024;k++)
	{
		if(log[k].ulLogID == 0x00)
			continue;

		printf("K=%d log type[%d] time[%lld] ucParam1[%d] ucParam2[%d] logid[%lld] text[%s]\n", k, log[k].ucType,log[k].ulTimeStamp,log[k].ucParam1,log[k].ucParam2,log[k].ulLogID,log[k].sText);
	}

	printf("\n\n");

	return 0;

}
__commandlist(sys_get_nand_log, "sys_get_nand_log", sys_get_nand_log_help, sys_get_nand_log_help);

static char sys_put_nand_log_help[] = "sys_get_nand_log [type] [ch]";
static int sys_put_nand_log(int argc, char **argv)
{
	int ch = 0;
	int type = 0;
	char str[256] = { NULL, };

	if( argc < 3 )
	{
		g_message("%s => ARGC ERROR : %s", __FUNCTION__, sys_get_nand_log_help);
	}

	type = atoi(argv[1]);

	ch = atoi(argv[2]);

	g_message("%s - TYPE=%d", __FUNCTION__, type);

	if( type == 1 )
	{
		snprintf(str, sizeof(str),  "ipcam recv err ch[%d]/[23]", ch);
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, str);
	}
	else if ( type == 2 )
	{
		snprintf(str, sizeof(str),  "DISK(%d) IO ERR : cnt=3/3, sector=211431615, disk_e_cnt=1", ch);
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_IFS_HOLD_SYSTEM, str);
	}
	else if ( type == 3 )
	{
		snprintf(str, sizeof(str),  "Watchdog RESET [ %d][RECORD] max[30] cur[31] last[1384489877]!!", ch);
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_WATCHDOG, str);
	}
	else if ( type == 4 )
	{
		snprintf(str, sizeof(str),  "advance_rdbit_recovery:3858:STOP");
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_IFS_HOLD_SYSTEM, str);
	}
	else
	{
		g_message("%s - TYPE ERROR : %d", __FUNCTION__, type);
	}

	return 0;
}
__commandlist(sys_put_nand_log, "sys_put_nand_log", sys_put_nand_log_help, sys_put_nand_log_help);

// void nf_sysman_qc_get_msg(int sock, char *str)
// {
// 	char msg[30];
// 	int msg_len;

// 	memset(msg, 0x00, sizeof(msg));
// 	msg_len = read(sock, msg, sizeof(msg));
// 	if (msg_len == -1) {
// 		printf("\e[33m [%s] Accept Fail\e[0m\n", __FUNCTION__);
// 	}

// 	strncpy(str, msg, sizeof(msg));
// 	printf("\e[33m [%s] msg = %s msg_len = %d\e[0m\n", __FUNCTION__, str, msg_len);
// }

// void nf_sysman_qc_send_msg(int clnt_sock, char *str)
// {
// 	char msg[30];
// 	int msg_len;

// 	memset(msg, 0x00, sizeof(msg));
// 	strncpy(msg, str, sizeof(msg));
// 	msg_len = write(clnt_sock, msg, sizeof(msg));
// 	if (msg_len == -1) {
// 		printf("\e[33m [%s] Write Fail\e[0m\n", __FUNCTION__);
// 	}
// 	printf("\e[33m [%s] msg = %s msg_len = %d msg_size = %d\e[0m\n", __FUNCTION__, str, msg_len, sizeof(msg));

// }

// #define NF_SYSMAN_QC_PD_ERROR_MSG_NO_USB			"NO USB"
// #define NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATADIR		"NO DATADIR"
// #define NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATA			"NO DATA"
// #define NF_SYSMAN_QC_PD_ERROR_MSG_DUPLICATION 		"DUPLICATION DATA"
// #define NF_SYSMAN_QC_PD_ERROR_MSG_WRONG_DATA 		"WRONG DATA"

// guint nf_sysman_qc_check_error_msg(char *msg)
// {
// 	int ret = NF_SYSMAN_QC_PD_ERROR_OK;

// 	if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_NO_USB, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_NO_USB)) == 0) {
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_USB;
// 	} else if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATADIR, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATADIR)) == 0) {
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATADIR;
// 	} else if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATA, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATA)) == 0) {
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATA;
// 	} else if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_DUPLICATION, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_DUPLICATION)) == 0) {
// 		ret = NF_SYSMAN_QC_PD_ERROR_DUPLICATION;
// 	}

// 	printf("\e[33m [%s] msg = %s ret = %d\e[0m\n", __FUNCTION__, msg, ret);
// 	return ret;
// }

// guint nf_sysman_qc_client(void)
// {
// 	struct sockaddr_in serv_addr;
// 	char server_name[128];
// 	char msg[30], cmd[512], hw_info[30], data_name[30], dataBuf[MAX_PAGE_SIZE];
// 	int sock, ret = -1, msg_len, data_size;
// 	char *mac_addr;
// 	NF_UTIL_PROTECT_DATA data;
// 	ITX_PROTECT_DATA_PARAM data_param;
// 	NF_SYSMAN_HW_PARAM hw_param;
// 	int pba_type = nf_sysman_get_pba_type();
// 	FILE *fp = NULL;

// 	printf("\e[33m [%s] Called \e[0m\n", __FUNCTION__);
// 	memset(server_name, 0x00, sizeof(server_name));

//     nf_sysman_qc_get_ntp_serverip(server_name);

// 	sock = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sock == -1) {
// 		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
// 	}

// 	memset(&serv_addr, 0, sizeof(serv_addr));
// 	serv_addr.sin_family = AF_INET;
// 	serv_addr.sin_addr.s_addr = inet_addr(server_name);
// 	serv_addr.sin_port = htons(8090);

// 	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
// 		perror("connect");
// 		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
// 		return NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 	}

// 	/* Make Data Folder Dir */
// 	memset(hw_info, 0x00, sizeof(hw_info));
// 	#if defined(_IPX_1648M4)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4A-16CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4B-16CH");
// 		}
// 	#elif defined(_IPX_0824M4)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4A-8CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4B-8CH");
// 		}
// 	#elif defined(_IPX_32M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4AE-32CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4BE-32CH");
// 		}
// 	#elif defined(_IPX_1648M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4EA-16CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4EB-16CH");
// 		}
// 	#elif defined(_IPX_0824M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4EA-8CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4EB-8CH");
// 		}
// 	#elif defined(_IPX_0412M4E)
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4EA-4CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4EB-4CH");
// 		}
// 	#elif defined(_IPX_1648P4E) || defined(_IPX_32P4E)
// 		strcpy(hw_info, "IPX-P4E-16CH");
// 	#elif defined(_IPX_0824P4E)
// 		strcpy(hw_info, "IPX-P4E-8CH");
// 	//_IPX_32P5
// 	#else
// 		if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 			strcpy(hw_info, "IPX-M4A-4CH");
// 		} else {
// 			strcpy(hw_info, "IPX-M4B-4CH");
// 		}
// 	#endif

// 	nf_sysman_qc_send_msg(sock, hw_info);

// 	/* Get Mac Data Name */
// 	nf_sysman_qc_get_msg(sock, msg);

// 	ret = nf_sysman_qc_check_error_msg(msg);
// 	if (ret != NF_SYSMAN_QC_PD_ERROR_OK) {
// 		printf("\e[33m [%s] ret = %d\e[0m\n", __FUNCTION__, ret);
// 		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
// 		return ret;
// 	}

// 	nf_sysman_qc_send_msg(sock, "WRITING START");

// 	/* Copy Mac Data from Server */
// 	memset(cmd, 0x00, sizeof(cmd));
// 	snprintf(cmd, sizeof(cmd) - 1, "sshpass -pitxutm3398 scp -o StrictHostKeyChecking=no -P 8822 root@%s:/mnt/qcusb/%s/%s /tmp/", server_name, hw_info, msg);
// 	printf("\e[33m [%s] cmd = %s\e[0m\n", __FUNCTION__, cmd);
// 	proxy_system(cmd, 1, 3);

// 	/* Write Mac Data & HW Param */
// 	memset(data_name, 0x00, sizeof(data_name));
// 	strncpy(data_name, msg, sizeof(msg));
// 	snprintf(cmd, sizeof(cmd) - 1, "/tmp/%s", data_name);
// 	fp = fopen(cmd, "r");
// 	if (fp == NULL) {
// 		printf("\e[33m [%s] %s Open Fail\e[0m\n", __FUNCTION__, cmd);
// 		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
// 		return NF_SYSMAN_QC_PD_ERROR_ETC;
// 	}

// 	memset(dataBuf, 0x00, sizeof(dataBuf));
// 	fseek(fp, 0, SEEK_END);
// 	data_size = ftell(fp);
// 	rewind(fp);
// 	if (data_size != fread(dataBuf, 1, data_size, fp)) {
// 		printf("\e[33m [%s] /tmp/%s Read Fail\e[0m\n", __FUNCTION__, data_name);
// 		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
// 		return NF_SYSMAN_QC_PD_ERROR_ETC;
// 	}

// 	memset(&hw_param, 0x00, sizeof(hw_param));
// 	nf_sysman_get_hw_param(&hw_param, 0);
// 	if (strncmp(hw_param.magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0) {
// 		printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
// 		ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
// 		if (!ret) {
// 			printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
// 		}
// 	} else {
// 		nf_sysman_set_hw_param(&hw_param);
// 	}

// 	memset(&data, 0x00, sizeof(data));
// 	memcpy(&data.enc_data, dataBuf, sizeof(data.enc_data));
// 	nf_sysman_set_protect_data(&data);

// 	/* Read Data From Nand */
// 	memset(&data, 0x00, sizeof(data));
// 	if (nf_sysman_get_protect_data(&data) == FALSE) {
// 		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
// 		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
// 		return NF_SYSMAN_QC_PD_ERROR_ETC;
// 	}

// 	memset(&data_param, 0x00, sizeof(data_param));
// 	memcpy(&data_param, &data.dec_data, sizeof(data_param));

// 	/* Check Data Parameters */
// 	if (nf_sysman_check_protect_data_param(&data_param, data_name) == FALSE) {
// 		printf("\e[33m [%s] Check Protect Data Fail\e[0m\n", __FUNCTION__);
// 		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
// 		return NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
// 	}

// 	nf_sysman_qc_send_msg(sock, "WRITING DONE");

// 	/* Send Msg to Notify Success */
// 	nf_sysman_qc_send_msg(sock, data_name);

// 	/* Check Error To Del Data */
// 	nf_sysman_qc_get_msg(sock, msg);
// 	if (strncmp(msg, "SUCCESS", sizeof("SUCCESS")) != 0) {
// 		memset(&hw_param, 0x00, sizeof(hw_param));
// 		nf_sysman_get_hw_param(&hw_param, 0);
// 		if (strncmp(hw_param.magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0) {
// 			printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
// 			ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
// 			if (!ret) {
// 				printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
// 			}
// 		} else {
// 			nf_sysman_set_hw_param(&hw_param);
// 		}
// 	}

// 	fclose(fp);
// 	close(sock);

// 	return NF_SYSMAN_QC_PD_ERROR_OK;
// }
// static void nf_sysman_print_hw_param(NF_SYSMAN_HW_PARAM *hw_param)
// {
// 	if(hw_param == NULL)
// 	{
// 		printf("\e[33m %s HW_PARAM is empty!!!\e[0m\n", __FUNCTION__);
// 		return;
// 	}
// 	printf("\e[33m %s magic			: %s\e[0m\n", __FUNCTION__,hw_param->magic);
// 	printf("\e[33m %s eth_addr		: %s\e[0m\n", __FUNCTION__,hw_param->eth_addr);
// 	printf("\e[33m %s pannel_type	: %s\e[0m\n", __FUNCTION__,hw_param->pannel_type);
// 	printf("\e[33m %s rc_type		: %s\e[0m\n", __FUNCTION__,hw_param->rc_type);
// 	printf("\e[33m %s hw_ver		: %s\e[0m\n", __FUNCTION__,hw_param->hw_ver);
// 	printf("\e[33m %s vendor		: %s\e[0m\n", __FUNCTION__,hw_param->vendor);
// 	#if defined(CONFIG_COPY_PROTECTION)
// 	printf("\e[33m %s serial_match	: %s\e[0m\n", __FUNCTION__,hw_param->serial_match);
// 	printf("\e[33m %s serial_rand	: %s\e[0m\n", __FUNCTION__,hw_param->serial_rand);
// 	#endif	
// }
// MAC WRITER TYPE
// typedef struct _STPACKETDATA
// {
// 	char cCode;					
// 	int nParam1;
// 	int nParam2;
// 	char cParam100[100];			//for mac_data
// 	char cParam1000[1000];			//for hw_info
// }STPacketData;

// typedef struct _LICEDATA
// {
// 	char mac[12];
// 	char lic[36];
// 	int product;
// }LicData;

// enum
// {
// 	//Client To Server
// 	CTS_REQUESTMAC = 1,			
// 	CTS_PRODUCTNAME,
// 	CTS_PRODUCTFINISH_SUCCESS,
// 	CTS_PRODUCTFINISH_FAIL,			
	
// 	//Server To Client
// 	STC_RESPONSEMAC,				//5
// 	STC_RESPONSEPRODUCTNAME,	//6
	
// 	//Set To Client
// 	SetTC_SETINFO,					//7
// 	SetTC_REQUESTMAC,				//8
// 	SetTC_ISMACSUCCESS,			//9
// 	SetTC_MACFILEPREPRARE,		//10
	
// 	//Client To Set
// 	CTSet_PREPAREINFO,				//11
// 	CTSet_SEND_MAC,				//12

// 	SetTC_SETTIME,
// 	CTSet_SEND_TIME,

// 	SetTC_REQUESTSERIAL,
// 	CTS_REQUESTSERIALNUM,
// 	STC_RESPONSESERIALNUM,
// 	CTSet_SEND_SERIAL,

// 	SetTC_ISSERIALSUCCESS,
// };

// enum{
// 	INSPECTOR_SUCCESS,						//0
// 	INSPECTOR_NOTPRODUCTINFO,
// 	INSPECTOR_NOTSERVERCONNECT,
// 	INSPECTOR_NOMACFILE,
// 	INSPECTOR_SERVER_MACFILE_LOADERROR,
// 	INSPECTOR_CLIENT_MACFILE_LOADERROR,
// 	INSPECTOR_NOTVENDERINFO,
// 	INSPECTOR_NOTSERIALINFO,
// };
// MAC WRITER END

// MAC WRITER FUNCTION
// guint nf_sysman_qc_init_stpacketdata(STPacketData *packet)
// {
// 	packet->cCode = -1;
// 	packet->nParam1 = 0;
// 	packet->nParam2 = 0;
// 	memset(packet->cParam100,0,100);
// 	memset(packet->cParam1000,0,1000);
// }
// void nf_sysman_qc_send_mac_msg(int clnt_sock, STPacketData *packet)
// {
// 	STPacketData sendPacket;
// 	int packet_len;

// 	memset(&sendPacket, 0x00, sizeof(sendPacket));
	
// 	sendPacket.cCode = packet->cCode;
// 	sendPacket.nParam1 = packet->nParam1;
// 	sendPacket.nParam2 = packet->nParam2;
// 	strncpy(sendPacket.cParam1000 , packet->cParam1000, sizeof(packet->cParam1000));
// 	strncpy(sendPacket.cParam100 , packet->cParam100, sizeof(packet->cParam100));
	
// 	packet_len = write(clnt_sock, &sendPacket, sizeof(sendPacket));
	
// 	if (packet_len == -1) {
// 		printf("\e[33m [%s] Write Fail\e[0m\n", __FUNCTION__);
// 	}
// 	//BIN DEBUG
// 	printf("\e[33m [%s] cmd = %d msg_len = %d msg_size = %d\e[0m\n", __FUNCTION__, sendPacket.cCode, packet_len, sizeof(sendPacket));
// }
// guint nf_sysman_qc_get_mac_msg(int clnt_sock, STPacketData *packet)
// {
// 	STPacketData recvPacket;
// 	int packet_len;

// 	memset(&recvPacket, 0x00, sizeof(recvPacket));

// 	packet_len = read(clnt_sock, &recvPacket, sizeof(recvPacket));
	
// 	if (packet_len == -1) {
// 		printf("\e[33m [%s] Accept Fail\e[0m\n", __FUNCTION__);
// 		return -1;
// 	}

// 	packet->cCode = recvPacket.cCode;
// 	packet->nParam1 = recvPacket.nParam1;
// 	packet->nParam2 = recvPacket.nParam2;
// 	strncpy(packet->cParam1000, recvPacket.cParam1000 , sizeof(packet->cParam1000));
// 	strncpy(packet->cParam100, recvPacket.cParam100, sizeof(packet->cParam100));

// 	//BIN DEBUG
// 	printf("\e[33m [%s] cmd = %d msg1 = %s msg2 = %s msg_len = %d\e[0m\n", __FUNCTION__, recvPacket.cCode, recvPacket.cParam100, recvPacket.cParam1000 ,packet_len);

// 	return packet_len;
// }
// guint nf_sysman_qc_check_error_mac_msg(STPacketData *packet)
// {
// 	int ret = NF_SYSMAN_QC_PD_ERROR_OK;
// 	char msg[256] = {0,};
// 	if(packet->nParam1 == INSPECTOR_NOTPRODUCTINFO)			
// 	{	
// 		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
// 		strcpy(msg,"INSPECTOR_NOTPRODUCTINFO");
// 	}
// 	else if(packet->nParam1 == INSPECTOR_NOTSERVERCONNECT)	
// 	{	
// 		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 		strcpy(msg,"INSPECTOR_NOTSERVERCONNECT");
// 	}	
// 	else if(packet->nParam1 == INSPECTOR_NOMACFILE)	
// 	{	
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATA;
// 		strcpy(msg,"INSPECTOR_NOMACFILE");
// 	}
// 	else if(packet->nParam1 == INSPECTOR_SERVER_MACFILE_LOADERROR)	
// 	{	
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATADIR;
// 		strcpy(msg,"INSPECTOR_SERVER_MACFILE_LOADERROR");
// 	}
// 	else if(packet->nParam1 == INSPECTOR_CLIENT_MACFILE_LOADERROR)	
// 	{	
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATADIR;
// 		strcpy(msg,"INSPECTOR_CLIENT_MACFILE_LOADERROR");
// 	}
// 	else{
// 		strcpy(msg,"INSPECTOR_SUCCESS");
// 	}
// 	printf("\e[33m [%s] msg = %s ret = %d\e[0m\n", __FUNCTION__, msg, ret);
// 	return ret;
// }
// #define BUF_SIZE 1024
// gboolean nf_sysman_qc_get_mac_data(int clnt_sock, char *fPath)
// {
// 	FILE *fp = NULL;
// 	int nSendData = 0;				// success : 1, fail : 0
// 	STPacketData sendPackatData;
	
// 	char buf[BUF_SIZE];
// 	int nRet = 0;
// 	int fileSize = 0;
// 	unsigned int total_received =0;
// 	unsigned int readCnt= 0;			

	
// 	fp = fopen(fPath, "w+");
// 	if (fp == NULL) {
// 		printf("\e[33m [%s] %s Open Fail\e[0m\n", __FUNCTION__, fPath);
// 		nSendData = 0;
// 		sendPackatData.cCode = SetTC_MACFILEPREPRARE;
// 		sendPackatData.nParam1 = nSendData;
// 		nf_sysman_qc_send_mac_msg(clnt_sock, &sendPackatData);
// 		return FALSE;
// 	}
// 	else
// 	{
// 		nSendData = 1;
// 		sendPackatData.cCode = SetTC_MACFILEPREPRARE;
// 		sendPackatData.nParam1 = nSendData; 
// 		nf_sysman_qc_send_mac_msg(clnt_sock, &sendPackatData);
// 	}
	
// 	readCnt = read(clnt_sock, (char*)&nRet, sizeof(int));
// 	if(nRet == -1)
// 	{
// 		printf("\e[33m [%s] File access Falied\e[0m\n", __FUNCTION__);
// 		return FALSE;
// 	}
	
// 	while(1)
// 	{
// 		readCnt = read(clnt_sock, buf, BUF_SIZE);
// 		if( readCnt <= 0)
// 			break;
// 		total_received += readCnt;
// 		if(fileSize == 0)
// 		{
// 			memcpy(&fileSize, buf, sizeof(int));
// 			if(fileSize == -1)
// 			{
// 				break;
// 			}
// 			fwrite(buf + 4,1, readCnt - 4,fp);
// 		}
// 		else
// 			fwrite(buf ,1, readCnt,fp);
// 		if( total_received == fileSize + sizeof(int))
// 			break;
// 	}
// 	fclose(fp);
// 	if(readCnt <= 0 || fileSize <= 0)
// 		return FALSE;
// 	else if( total_received == fileSize + sizeof(int))
// 		return TRUE;
// 	else
// 		return FALSE;
// }

// void nf_sysman_set_vcode(char *param)
// {
// 	VCODE_IDX idx;
// 	char *pStrVcode = NULL;
// 	pStrVcode = nf_sysdb_get_str_nocopy("sys.info.vendor");
// 	printf("\033[0;33m %s VENDOR[1] : %s \033[0;39m\n", __FUNCTION__,pStrVcode);
// 	if (strcmp(pStrVcode, "ITX") == 0)					idx = IDX_ITX;
// 	else if (strcmp(pStrVcode, "GPS") == 0)				idx = IDX_GPS;
// 	else if (strcmp(pStrVcode, "VIDECON") == 0)			idx = IDX_VDC;
// 	else if (strcmp(pStrVcode, "ORION") == 0)			idx = IDX_ORI;
// 	else if (strcmp(pStrVcode, "CBC") == 0)				idx = IDX_CBC;
// 	else if (strcmp(pStrVcode, "TAKENAKA") == 0)		idx = IDX_TKK;
// 	else if (strcmp(pStrVcode, "G4S") == 0)				idx = IDX_G4S;
// 	else if (strcmp(pStrVcode, "VICON") == 0)			idx = IDX_VCN;
// 	else if (strcmp(pStrVcode, "HONEYWELL") == 0)		idx = IDX_HON;
// 	else if (strcmp(pStrVcode, "VITEK") == 0)			idx = IDX_VIT;
// 	else if (strcmp(pStrVcode, "ASP") == 0)				idx = IDX_ASP;
// 	else if (strcmp(pStrVcode, "I3DVR") == 0)			idx = IDX_I3D;
// 	else if (strcmp(pStrVcode, "CYTE") == 0)			idx = IDX_CYT;
// 	else if (strcmp(pStrVcode, "DAYOU") == 0)			idx = IDX_DAU;
// 	else if (strcmp(pStrVcode, "IRLAB") == 0)			idx = IDX_IRL;
// 	else
// 		idx = IDX_NONE;
// 	if(idx == IDX_NONE) return;
// 	printf("\033[0;33m %s VENDOR[2] : [%s] [%d]\033[0;39m\n", __FUNCTION__,strVcode[idx-1],idx);
// 	strcat(param, "&");
// 	strcat(param, strVcode[idx-1]);
// }
// MAC WRITER END
// guint nf_sysman_qc_pc_client(void)
// {
// 	struct sockaddr_in serv_addr;
// 	char server_name[128], hw_info[30], dataBuf[MAX_PAGE_SIZE],  macFilePath[30] ,data_name[30];
// 	int sock, data_size = 0, ret = -1;
	
// 	NF_UTIL_PROTECT_DATA data;
// 	ITX_PROTECT_DATA_PARAM data_param;
// 	NF_SYSMAN_HW_PARAM hw_param;
// 	int pba_type = nf_sysman_get_pba_type();
// 	FILE *fp = NULL;

// 	int nPacketSize = sizeof(STPacketData);
// 	int received = 0;
	
// 	STPacketData sendPacket;
// 	STPacketData recvPacket;
	
// 	nf_sysman_qc_init_stpacketdata(&sendPacket);
// 	nf_sysman_qc_init_stpacketdata(&recvPacket);

// 	printf("\e[33m [%s] Called \e[0m\n", __FUNCTION__);

// 	memset(server_name, 0x00, sizeof(server_name));
// 	nf_sysman_qc_get_ntp_serverip(server_name);

// 	sock = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sock == -1) {
// 		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
// 	}

// 	memset(&serv_addr, 0, sizeof(serv_addr));
// 	serv_addr.sin_family = AF_INET;
// 	serv_addr.sin_addr.s_addr = inet_addr(server_name);
// 	serv_addr.sin_port = htons(10252);
// 	printf("\e[33m [%s] [ServerIP : %s]\e[0m\n", __FUNCTION__,server_name);
// 	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
// 		perror("connect");
// 		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
// 		ret =  NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}

// 	/* Make Data Folder Dir */
// 	#if defined(_IPX_1648M4)
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4A-16CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4A-16CH");
// 	} else {
// 		strcpy(hw_info, "IPX-M4B-16CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4B-16CH");
// 	}
// 	#elif defined(_IPX_0824M4)
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4A-8CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4A-8CH");
			
// 	} else {
// 		strcpy(hw_info, "IPX-M4B-8CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4B-8CH");
// 	}
// 	#elif defined(_IPX_32M4E) 
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4AE-32CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4AE-32CH");
// 	} else {
// 		strcpy(hw_info, "IPX-M4BE-32CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4BE-32CH");
// 	}
// 	#elif defined(_IPX_1648M4E)
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4EA-16CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4EA-16CH");
// 	} else {
// 		strcpy(hw_info, "IPX-M4EB-16CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4EB-16CH");
// 	}
// 	#elif defined(_IPX_0824M4E)
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4EA-8CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4EA-8CH");
// 	} else {
// 		strcpy(hw_info, "IPX-M4EB-8CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4EB-8CH");
// 	}
// 	#elif defined(_IPX_0412M4E)
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4EA-4CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4EA-4CH");
// 	} else {
// 		strcpy(hw_info, "IPX-M4EB-4CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4EB-4CH");
// 	}
// 	#elif defined(_IPX_1648P4E) 
// 		strcpy(hw_info, "IPX-P4E-16CH");
// 		strcpy(sendPacket.cParam1000, "IPX-P4E-16CH");
// 	#elif defined(_IPX_0824P4E)
// 		strcpy(hw_info, "IPX-P4E-8CH");
// 		strcpy(sendPacket.cParam1000, "IPX-P4E-8CH");
// 	#else
// 	if (pba_type == NF_SYSMAN_PBA_TYPE_A) {
// 		strcpy(hw_info, "IPX-M4A-4CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4A-4CH");	
// 	} else {
// 		strcpy(hw_info, "IPX-M4B-4CH");
// 		strcpy(sendPacket.cParam1000, "IPX-M4B-4CH");
// 	}
// 	#endif
// 	//_IPX_32P5
	
// 	nf_sysman_set_vcode(sendPacket.cParam1000);
// 	/*Send hw_info to Server*/
// 	sendPacket.cCode = SetTC_SETINFO;
// 	nf_sysman_qc_send_mac_msg(sock,&sendPacket);

// 	/* Get Mac Data Name */
// 	received = nf_sysman_qc_get_mac_msg(sock, &recvPacket);
// 	if(received <= 0 || received != nPacketSize) 
// 	{
// 		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}
// 	if(recvPacket.cCode != CTSet_PREPAREINFO) 
// 	{
// 		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}
// 	ret = nf_sysman_qc_check_error_mac_msg(&recvPacket);
// 	if (ret != NF_SYSMAN_QC_PD_ERROR_OK) {
// 		printf("\e[33m [%s] ret = %d\e[0m\n", __FUNCTION__, ret);
// 		sendPacket.cCode = SetTC_ISMACSUCCESS;
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		goto FAIL_CLOSE;
// 	}

// 	/*Rquesting Mac Data from Server*/
// 	memset(&sendPacket, 0, sizeof(STPacketData));
// 	sendPacket.cCode = SetTC_REQUESTMAC;
// 	nf_sysman_qc_send_mac_msg(sock,&sendPacket);

// 	memset(&sendPacket, 0, sizeof(STPacketData));
// 	sendPacket.cCode = SetTC_ISMACSUCCESS;

// 	/* Copy Mac Data from Server */
// 	memset(&recvPacket, 0, sizeof(STPacketData));
// 	received = nf_sysman_qc_get_mac_msg(sock, &recvPacket);
// 	if(received <= 0 || received != nPacketSize)  
// 	{
// 		printf("\e[33m [%s] Data receive Failed\e[0m\n", __FUNCTION__);
// 		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}
// 	if(recvPacket.cCode != CTSet_SEND_MAC) 
// 	{
// 		printf("\e[33m [%s] MacDataName receive Failed\e[0m\n", __FUNCTION__);
// 		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATA;
// 		goto FAIL_CLOSE;
// 	}
	
// 	memset(data_name, 0x00, sizeof(data_name));
// 	strncpy(data_name, recvPacket.cParam100, sizeof(data_name));
// 	snprintf(macFilePath, sizeof(macFilePath) - 1, "/tmp/%s", data_name);
// 	printf("\e[33m [%s] MAC = %s\e[0m\n", __FUNCTION__,data_name);
// 	if(!(nf_sysman_qc_get_mac_data(sock,macFilePath)))
// 	{
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		printf("\e[33m [%s] Fail that Get Mac data\e[0m\n", __FUNCTION__);
// 		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
// 		goto FAIL_CLOSE;
// 	}
	
// 	/* Write Mac Data & HW Param */
// 	fp = fopen(macFilePath, "r");
// 	if (fp == NULL) {
// 		printf("\e[33m [%s] %s Open Fail\e[0m\n", __FUNCTION__, macFilePath);
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
// 		goto FAIL_CLOSE;
// 	}

// 	memset(dataBuf, 0x00, sizeof(dataBuf));
// 	fseek(fp, 0, SEEK_END);
// 	data_size = ftell(fp);
// 	rewind(fp);
// 	if (data_size != fread(dataBuf, 1, data_size, fp)) {
// 		printf("\e[33m [%s] /tmp/%s Read Fail\e[0m\n", __FUNCTION__, data_name);
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
// 		goto FAIL_CLOSE;
// 	}

// 	memset(&hw_param, 0x00, sizeof(hw_param));
// 	nf_sysman_get_hw_param(&hw_param, 0);
// 	if (strncmp(hw_param.magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0) {
// 		printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
// 		ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
// 		if (!ret) {
// 			printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
// 		}
// 	} else {
// 		nf_sysman_set_hw_param(&hw_param);
// 	}

// 	memset(&data, 0x00, sizeof(data));
// 	memcpy(&data.enc_data, dataBuf, sizeof(data.enc_data));
// 	nf_sysman_set_protect_data(&data);

// 	/* Read Data From Nand */
// 	memset(&data, 0x00, sizeof(data));
// 	if (nf_sysman_get_protect_data(&data) == FALSE) {
// 		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
// 		//nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
// 		goto FAIL_CLOSE;
// 	}

// 	memset(&data_param, 0x00, sizeof(data_param));
// 	memcpy(&data_param, &data.dec_data, sizeof(data_param));

// 	/* Check Data Parameters */
// 	if (nf_sysman_check_protect_data_param(&data_param, data_name) == FALSE) {
// 		printf("\e[33m [%s] Check Protect Data Fail\e[0m\n", __FUNCTION__);
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		ret = NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
// 		goto FAIL_CLOSE;
// 	}
	
// 	/* Send Msg to Notify Success */
// 	printf("\e[33m [%s] Success To Write mac data\e[0m\n", __FUNCTION__);
// 	sendPacket.nParam1 = 1;
// 	nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 	ret = NF_SYSMAN_QC_PD_ERROR_OK;

// 	/*
// 	memset(&hw_param, 0x00, sizeof(hw_param));
// 	nf_sysman_get_hw_param(&hw_param, 0);
// 	if (strncmp(hw_param.magic, NF_SYSMAN_HW_PARAM_MAGIC, 4) != 0) {
// 		printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
// 		ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
// 		if (!ret) {
// 			printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
// 		}
// 	} else {
// 		nf_sysman_set_hw_param(&hw_param);
// 	}
// 	*/
// 	FAIL_CLOSE:
// 	{
// 		if(sock > 0)
// 			close(sock);
// 		if(fp != NULL)
// 			close(fp);
// 		return ret;
// 	}
// }
/*********************************
QC LICENSE
*********************************/
// int macNameSet(char* descMac, char *srcMac, int mode )
// {
// 	int idx = 0;
// 	int ret = 0;
// 	printf("\e[33m [%s] MAC %s\e[0m\n", __FUNCTION__,srcMac);
// 	if(srcMac == NULL || descMac == NULL)
// 	{
// 		printf("\e[33m [%s] MAC FAIL \e[0m\n", __FUNCTION__);
// 		return ret;
// 	}
// 	if(mode == 0)
// 	{
// 	while(srcMac[idx] != '.')
// 	{
// 		if(srcMac[idx] == '-')
// 			descMac[idx] = ':';
// 		else
// 			descMac[idx] = srcMac[idx];
// 		idx++;
// 	}
// 	}
// 	else if(mode == 1)
// 	{
// 		int descIdx = 0;
// 		while(srcMac[idx] != '\0')
// 		{
// 			if(srcMac[idx] != ':'){
// 				descMac[descIdx] = srcMac[idx];
// 				descIdx++;
// 			}
// 			idx++;
// 		}
// 		descMac[descIdx] = '\0';
// 		printf("\e[33m [%s] Mode 1 MAC %s\e[0m\n", __FUNCTION__,descMac);
// 	}
// 	return 1;
// }
// #define LICMAXCNT 16
// #define LICMEMBERCNT 3
// #define MAXPARAM1000 1000
// #define DEBUG_IMSI
// /*NAND ������ ����� ���ڿ� MAC�� ���ڷ� ��ȯ��Ű�� ���� �Լ�*/
// gint nf_sysman_qc_atoi_mac(gchar *mac, int size)
// {
// 	NF_HW_PARAM_ENC_DATA data;
// 	ITX_PROTECT_DATA_PARAM data_param;

// 	memset(&data, 0x00, sizeof(data));
// 	if (nf_sysman_get_protect_data(&data) == FALSE) {
// 		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
// 		return -1;
// 	}

// 	memset(&data_param, 0x00, sizeof(data_param));
// 	memcpy(&data_param, &data.dec_data, sizeof(data_param));
// 	int idx = 0,bit_shift=4,mac_idx = 0;
// 	char hex = 0, tmp_hex = 0; 
// 	#if defined(DEBUG_IMSI)
// 	printf("\033[0;36m %s [%s]\033[0;39m\n", __FUNCTION__,data_param.eth_addr);
// 	#endif
// 	for(idx = 0; idx < 18; idx++)
// 	{
// 		#if defined(DEBUG_IMSI)
// 		printf("\033[0;36m %s IDX[%d] [%c]\033[0;39m\n", __FUNCTION__,idx,data_param.eth_addr[idx]);
// 		#endif
// 		if(data_param.eth_addr[idx] == ':' || (idx == 17))
// 		{
// 			if(size < mac_idx)
// 			{
// 				printf("\033[0;35m %s OVERFLOW SIZE[%d] CURRENT SIZE[%d]\033[0;39m\n", __FUNCTION__,size,mac_idx);
// 				return -1;
// 			}
// 			mac[mac_idx] = hex;
// 			mac_idx++;
// 			hex = 0;
// 			bit_shift = 4;
// 			#if defined(DEBUG_IMSI)
// 			printf("\033[0;35m %s CONVERT MAC[%d] [0x%x]\033[0;39m\n", __FUNCTION__,mac_idx-1,mac[mac_idx-1]);
// 			#endif
// 			continue;
// 		}
// 		tmp_hex = data_param.eth_addr[idx];
// 		hex |= ((unsigned long)strtoul( &tmp_hex, NULL, 16) << bit_shift);
// 		bit_shift -= 4;
// 		#if defined(DEBUG_IMSI)
// 		printf("\033[0;35m %s tmp_hex[0x%c] hex[0x%x] \033[0;39m\n", __FUNCTION__,tmp_hex,hex);
// 		#endif

// 	}
// 	return 1;
// }

// static int nf_sysman_qc_lic_parse(int licCnt, char *param, char *mac,int product, LicData* arrLicData)
// {
// 	char* rbuf;
// 	char tmpParam[MAXPARAM1000] = {0,};
// 	char* tmpLicData[LICMAXCNT];
// 	int licIdx = 0 ,licMemIdx = 0, ret = 0;
// 	char *ptr;
	
// 	printf("\e[33m [%s] licCnt [%d] \e[0m\n", __FUNCTION__,licCnt);
	
// 	memcpy(tmpParam, param, MAXPARAM1000);
// 	for(licIdx =0; licIdx < licCnt; licIdx++)
// 	{
// 		if(licIdx == 0)
// 			tmpLicData[licIdx] = strtok_r( tmpParam, "\/", &rbuf);
// 		else
// 			tmpLicData[licIdx] = strtok_r( NULL , "\/", &rbuf);
// 		if(tmpLicData[licIdx] == NULL)
// 		{
// 			ret = NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
// 			goto PASEFAIL;
// 		}
// 		#if defined(DEBUG_SYSMAN_QC_LIC)
// 		printf("\e[33m [%s] [%d] %s \e[0m\n", __FUNCTION__,licIdx,tmpLicData[licIdx]);
// 		#endif
// 	}
	
// 	for(licIdx =0; licIdx < licCnt; licIdx++)
// 	{ 
// 		for(licMemIdx=0; licMemIdx < LICMEMBERCNT; licMemIdx++)
// 		{
// 			if(licMemIdx == 0)
// 				ptr = strtok_r(tmpLicData[licIdx], "\^", &rbuf);
// 			else
// 				ptr = strtok_r(NULL, "\^", &rbuf);
// 			if(tmpLicData[licIdx] == NULL)
// 			{
// 				goto PASEFAIL;
// 			}
// 			switch(licMemIdx)
// 			{
// 				case 0:
// 					memcpy(arrLicData[licIdx].mac,ptr,strlen(ptr));
// 					#if defined(DEBUG_SYSMAN_QC_LIC)
// 					printf("\e[33m [%s] arrLicData[%d] MAC %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].mac);
// 					#endif
// 					break;
// 				case 1:
// 					arrLicData[licIdx].product = atoi(ptr);
// 					#if defined(DEBUG_SYSMAN_QC_LIC)
// 					printf("\e[33m [%s] arrLicData[%d] PRODUCT %d \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].product);
// 					#endif
// 					break;
// 				case 2:
// 					memcpy(arrLicData[licIdx].lic,ptr,strlen(ptr));
// 					#if defined(DEBUG_SYSMAN_QC_LIC)
// 					printf("\e[33m [%s] arrLicData[%d] LIC %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].lic);
// 					#endif
// 					break;
// 			}
// 		}
// 		if(strncmp(mac,arrLicData[licIdx].mac,sizeof(arrLicData[licIdx].mac)) == 0 && arrLicData[licIdx].product == product)
// 			printf("\e[33m [%s] [%d] valid lic : %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].lic);
// 		else
// 		{
// 			printf("\e[33m [%s] [%d] invalid lic : %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].lic);
// 			ret = NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
// 			goto PASEFAIL;
// 		}
// 	}
// 	PASEFAIL:
// 	{
// 		return ret;
// 	}
// }

// static int nf_sysman_qc_lic_set_db(int licCnt, LicData* arrLicData)
// {
// 	int ret = 0;
// 	int licIdx = 0;
// 	time_t the_time_utc;
// 	time(&the_time_utc);
// 	GValue set_value = {0,};
	
// 	nf_sysdb_default( "sys" );
// 	nf_sysdb_lock(NF_SYSDB_CATE_SYS);
// 	char cmd[128]={0,};
//     for (licIdx = 0; licIdx < licCnt; licIdx++)
//     {
//     	g_value_init(&set_value, G_TYPE_STRING);
//     	g_value_set_string(&set_value,arrLicData[licIdx].lic);
//     	if(!nf_sysdb_set_key1("sys.lic.L%d.key", licIdx, &set_value, NULL))
//     	{
//     		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
// 			printf("\e[33m [%s] FAILED TO SET DB [sys.lic.L%d.key] \e[0m\n", __FUNCTION__,licIdx);
//     		ret = NF_SYSMAN_QC_PD_ERROR_SET_DB;
// 			goto LICSETDBEND;
//     	}
//     	g_value_unset(&set_value);
		
// 		sprintf(cmd,"sys.lic.L%d.key",licIdx);
// 		//printf("\e[33m [%s] [sys.lic.L%d.key] : %s \e[0m\n", __FUNCTION__,licIdx,nf_sysdb_get_str(cmd));
		
//     	g_value_init(&set_value, G_TYPE_UINT);
//     	g_value_set_uint(&set_value, the_time_utc);
//     	if(!nf_sysdb_set_key1("sys.lic.L%d.acquired_date", licIdx, &set_value, NULL))
//     	{
//     		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
// 			printf("\e[33m [%s] FAILED TO SET DB [sys.lic.L%d.acquired_date] \e[0m\n", __FUNCTION__,licIdx);
//     		ret = NF_SYSMAN_QC_PD_ERROR_SET_DB;
// 			goto LICSETDBEND;
//     	}
//     	g_value_unset(&set_value);
		
// 		sprintf(cmd,"sys.lic.L%d.acquired_date",licIdx);
// 		printf("\e[33m [%s] [sys.lic.L%d.acquired_date] : %d \e[0m\n", __FUNCTION__,licIdx,nf_sysdb_get_uint(cmd));
// 	}

// 	g_value_init(&set_value, G_TYPE_UINT);
// 	g_value_set_uint(&set_value, licCnt);
// 	if(!nf_sysdb_set_key0("sys.lic.key_count", &set_value, NULL))
// 	{
// 		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
// 		ret = NF_SYSMAN_QC_PD_ERROR_SET_DB;
// 		goto LICSETDBEND;
// 	}
// 	g_value_unset(&set_value);

// 	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

// 	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
// 	nf_sysdb_save( "sys" );
// 	nf_sysdb_save_flush();
// 	LICSETDBEND:
// 	{
// 		return ret;
// 	}
// }
/*
int nf_sysman_malloc_free_key( int mode, int keyCnt, char **arrKey)
{
	
	int keyIdx = 0;
	
	if(keyCnt <= 0 || mode > 2)
		return 0;
	printf("\e[33m [%s] MODE : %d keyCnt : %d \e[0m\n", __FUNCTION__,mode, keyCnt);
	if(mode == 1)
		arrKey = (char**)malloc(sizeof(char *) * keyCnt);
	if(arrKey == NULL)
		return 0;
	
	for(keyIdx = 0; keyIdx < keyCnt; keyIdx++)
	{
		printf("\e[33m [%s] for \e[0m\n", __FUNCTION__,mode, keyCnt);
		if(mode == 1)
		{
			arrKey[keyIdx] = (char*)malloc(sizeof(char) * 36);
			if(arrKey == NULL)
			{
				printf("\e[33m [%s] if(arrKey == NULL) \e[0m\n", __FUNCTION__,mode, keyCnt);
				do{
					free(arrKey[keyIdx]);
					keyIdx--;
				}while(keyIdx >= 0);
				free(arrKey);
				return 0;
			}
		}
		else if(mode == 0)
			free(arrKey[keyIdx]);
	}
	if(mode == 0)
		free(arrKey);
	return 1;
}
*/
// static int nf_sysman_cp_key_data(char (*dstData)[36], LicData *srcData, int cnt)
// {
// 	int idx = 0;
// 	if(dstData == NULL || srcData == NULL || cnt <= 0)
// 		return 0;
// 	printf("\e[33m [%s] Start COPY \e[0m\n", __FUNCTION__,dstData[idx], srcData[idx].lic,idx);
// 	for(idx=0; idx<cnt; idx++)
// 	{
// 		if(!strncpy(dstData[idx],srcData[idx].lic,36))
// 		{
// 			printf("\e[33m [%s] FAIL strncpy \e[0m\n", __FUNCTION__);
// 			return 0;
// 		}
// 		printf("\e[33m [%s] dstData[%d] : %s  srcData[%d].lic : %s \e[0m\n", __FUNCTION__,idx,dstData[idx], idx, srcData[idx].lic);
// 	}
// 	return 1;
// }

// #define DEBUG_LIC_MAC "00:11:5f:b3:07:00"
// guint nf_sysman_qc_lic_write(char key[][36])
// {
// 	struct sockaddr_in serv_addr;
// 	int sock, data_size = 0, ret = -1;
		
// 	int nPacketSize = sizeof(STPacketData);
// 	int received = 0;
// 	char cmpMac[NF_SYSMAN_HW_PARAM_MAX_LEN] = {0,}, ip[128] = {0,};
// 	int licCnt = 0;
// 	STPacketData sendPacket;
// 	STPacketData recvPacket;
// 	STPacketData packet;
	
// 	NF_SYSMAN_HW_PARAM hw_param;
// 	NF_UTIL_PROTECT_DATA data;
// 	LicData arrLicData[LICMAXCNT] = {0,};
// 	#if defined(DEBUG_SYSMAN_QC_LIC)
// 	printf("\e[33m [%s] START \e[0m\n", __FUNCTION__);
// 	#endif 
// 	memset(&serv_addr, 0, sizeof(serv_addr));
// 	memset(ip, 0, sizeof(ip)/sizeof(char));
// 	nf_sysman_qc_get_ntp_serverip(ip);
// 	serv_addr.sin_family = AF_INET;
// 	serv_addr.sin_addr.s_addr = inet_addr(ip);
// 	serv_addr.sin_port = htons(10252);
// 	sock = socket(AF_INET, SOCK_STREAM, 0);
	
// 	if (sock == -1) {
// 		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
// 	}
	
// 	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
// 		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
// 		ret =  NF_SYSMAN_QC_LIC_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}

// 	nf_sysman_qc_init_stpacketdata(&sendPacket);
// 	nf_sysman_qc_init_stpacketdata(&recvPacket);
// 	/***************************
// 	SEND REQUEST LIC MSG
// 	***************************/
// 	#if defined(DEBUG_SYSMAN_QC_LIC)
// 	printf("\e[33m [%s] SEND REQUEST LIC MSG \e[0m\n", __FUNCTION__);
// 	#endif
	
// 	memset(&data, 0x00, sizeof(NF_UTIL_PROTECT_DATA));
// 	if (nf_sysman_get_protect_data(&data) == FALSE) 
// 	{
// 		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
// 		sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 		ret = NF_SYSMAN_QC_LIC_ERROR_MAC_DATA;
// 		goto FAIL_CLOSE;
// 	}
// 	memset(&hw_param, 0x00, sizeof(hw_param));
// 	memcpy(&hw_param, &data.dec_data, sizeof(hw_param));
// 	nf_sysman_print_hw_param(&hw_param);
// 	#if defined(DEBUG_SYSMAN_QC_LIC_MAC)
// 	memcpy(hw_param.eth_addr,DEBUG_LIC_MAC,sizeof(hw_param.eth_addr));
// 	#endif
// 	macNameSet(cmpMac,hw_param.eth_addr, 1);
	
// 	strncpy(sendPacket.cParam100,cmpMac,NF_SYSMAN_HW_PARAM_MAX_LEN);
// 	sendPacket.cCode = SetTC_REQUESTSERIAL;
// 	nf_sysman_qc_send_mac_msg(sock,&sendPacket);
// 	/***************************/
	
// 	/***************************
// 	RECV REQUEST LIC MSG
// 	***************************/
// 	#if defined(DEBUG_SYSMAN_QC_LIC)
// 	printf("\e[33m [%s] RECV REQUEST LIC MSG\e[0m\n", __FUNCTION__);
// 	#endif
// 	received = nf_sysman_qc_get_mac_msg(sock, &recvPacket);
// 	if(received <= 0 || received != nPacketSize) 
// 	{
// 		ret = NF_SYSMAN_QC_LIC_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}
	
// 	nf_sysman_qc_init_stpacketdata(&packet);
// 	memcpy(&packet, &recvPacket, sizeof(STPacketData));
// 	if(packet.cCode != CTSet_SEND_SERIAL)
// 	{
// 		ret = NF_SYSMAN_QC_LIC_ERROR_NETWORK;
// 		goto FAIL_CLOSE;
// 	}
// 	if(packet.nParam1 == INSPECTOR_NOTSERIALINFO)
// 	{

// 		ret = 0;
// 		goto FAIL_CLOSE;
// 	}
// 	/**************************/

// 	/***************************
// 	SET LICENSE
// 	***************************/
// 	#if defined(DEBUG_SYSMAN_QC_LIC)
// 	printf("\e[33m [%s] SET LICENSE \e[0m\n", __FUNCTION__);
// 	printf("\e[33m [%s] LIC CNT %d \e[0m\n", __FUNCTION__,packet.nParam2);
// 	#endif
// 	licCnt = packet.nParam2;
// 	#if defined(DEBUG_SYSMAN_QC_LIC_MAC)
// 	printf("\e[33m [%s] PRODUCT %d \e[0m\n", __FUNCTION__,nf_sysman_get_fwver_product());
// 	if(ret = nf_sysman_qc_lic_parse(licCnt,packet.cParam1000,cmpMac,66000,arrLicData))
// 	#else
// 	printf("\e[33m [%s] PRODUCT %d \e[0m\n", __FUNCTION__,nf_sysman_get_fwver_product());
// 	if(ret = nf_sysman_qc_lic_parse(licCnt,packet.cParam1000,cmpMac,nf_sysman_get_fwver_product(),arrLicData))
// 	#endif
// 	{
// 		ret = NF_SYSMAN_QC_LIC_ERROR_WRONG_DATA;
// 		goto FAIL_CLOSE;	
// 	}
// 	#if 0
// 	if(ret = nf_sysman_qc_lic_set_db(licCnt,arrLicData))
// 	{
// 		ret = NF_SYSMAN_QC_LIC_ERROR_SET_DB;
// 		goto FAIL_CLOSE;	
// 	}
// 	#endif
// 	/**************************/
// 	ret = licCnt;
// 	#if 0
// 	if(!nf_sysman_malloc_free_key(1,licCnt,key))
// 	{
// 		printf("\e[33m [%s] nf_sysman_malloc_free_key\e[0m\n", __FUNCTION__);
// 		ret = NF_SYSMAN_QC_LIC_ERROR_MEMORY;
// 		goto FAIL_CLOSE;	
// 	}
// 	#endif
// 	if(!nf_sysman_cp_key_data(key,arrLicData,licCnt))
// 	{
// 		ret = NF_SYSMAN_QC_LIC_ERROR_MEMORY;
// 		printf("\e[33m [%s] nf_sysman_cp_key_data\e[0m\n", __FUNCTION__);
// 		goto FAIL_CLOSE;
// 	}
// 	/***************************
// 	SEND RESULT LIC MSG
// 	***************************/
// 	FAIL_CLOSE:
// 	{
// 		nf_sysman_qc_init_stpacketdata(&sendPacket);
// 		strncpy(sendPacket.cParam100,cmpMac,NF_SYSMAN_HW_PARAM_MAX_LEN);	
// 		sendPacket.cCode = SetTC_ISSERIALSUCCESS;
// 		if(ret >= 0) // 1 : ���� , 0 : ����.
// 			sendPacket.nParam1 = 1;
// 		else
// 			sendPacket.nParam1 = 0;
// 		nf_sysman_qc_send_mac_msg(sock,&sendPacket);	
// 		if(sock > 0)
// 			close(sock);
// 		#if defined(DEBUG_SYSMAN_QC_LIC)
// 		printf("\e[33m [%s] END\e[0m\n", __FUNCTION__);
// 		#endif
// 		return ret;
// 	}
// 	/**************************/
// }

// #define NF_SYSMAN_APP_QC_TEST_ALARM			0
// #define NF_SYSMAN_APP_QC_TEST_RS485_START	1
// #define NF_SYSMAN_APP_QC_TEST_RS485_STOP	2
// #define NF_SYSMAN_APP_QC_TEST_AUDIO			3
// #define NF_SYSMAN_APP_QC_TEST_RTC			4
// #define NF_SYSMAN_APP_QC_TEST_FAN			5
// #define NF_SYSMAN_APP_QC_TEST_TEMPER		6
// #define NF_SYSMAN_APP_QC_TEST_HDD			7
// #define NF_SYSMAN_APP_QC_TEST_ODD			8
// #define NF_SYSMAN_APP_QC_TEST_RS232  		9
// #define NF_SYSMAN_APP_QC_TEST_POE 			10
// #define NF_SYSMAN_APP_QC_TEST_LED_ALL_ON 	11
// #define NF_SYSMAN_APP_QC_TEST_LED_ALL_OFF 	12
// #define NF_SYSMAN_APP_QC_TEST_NET			13

// static char app_qc_test_help[] = "0 [alarm]\n"
// 								  "1 [rs485 start]\n"
// 								  "2 [rs485 stop]\n"
// 								  "3 [audio]\n"
// 								  "4 [rtc]\n"
// 								  "5 [fan]\n"
// 								  "6 [temper]\n"
// 								  "7 [hdd]\n"
// 								  "8 [odd]\n"
// 								  "9 [rs232]\n"
// 								  "10 [poe]\n"
// 								  "11 [led on]\n"
// 								  "12 [led off]\n"
// 								  "13 [net]\n";
// static int app_qc_test(int argc, char **argv)
// {
// 	int test_num = 0, addr, val;
// 	guchar tmp = 0;
// 	gchar buf[64];
// 	int i;

// 	memset(buf, 0x00, sizeof(buf));

// 	if( argc < 2 )
// 	{
// 		g_message("%s => ARGC ERROR : %s", __FUNCTION__, app_qc_test_help);
// 		return 0;
// 	}

// 	test_num  = (int)strtoul(argv[1], NULL, 0);
// 	switch (test_num)
// 	{
// 		case NF_SYSMAN_APP_QC_TEST_ALARM :
// 			nf_sysman_qc_client();
// #if 0
// 			if (nf_sysman_qc_auto_test_alarm()) {
// 				printf("\e[32m[%s] APP QC ALARM TEST OK !!\e[0m\n", __FUNCTION__);
// 			} else {
// 				printf("\e[32m[%s] APP QC ALARM TEST FAIL !!\e[0m\n", __FUNCTION__);
// 			}
// #endif
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_RS485_START :
// 			nf_sysman_qc_manual_test_rs485(1);
// 			printf("\e[32m[%s] APP QC RS485 TEST START !!\e[0m\n", __FUNCTION__);
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_RS485_STOP :
// 			nf_sysman_qc_manual_test_rs485(0);
// 			printf("\e[32m[%s] APP QC RS485 TEST STOP !!\e[0m\n", __FUNCTION__);
// 		break;
// 		case NF_SYSMAN_APP_QC_TEST_AUDIO :
// 			if (nf_sysman_qc_auto_test_audio()) {
// 				printf("\e[32m[%s] APP QC AUDIO TEST OK !!\e[0m\n", __FUNCTION__);
// 			} else {
// 				printf("\e[32m[%s] APP QC AUDIO TEST FAIL !!\e[0m\n", __FUNCTION__);
// 			}
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_RTC :

// 			if (nf_sysman_qc_info_rtc(buf)) {
// 				printf("\e[32m[%s] APP QC RTC OK !! INFO = %s\e[0m\n", __FUNCTION__, buf);
// 			} else {
// 				printf("\e[32m[%s] APP QC RTC FAIL !!\e[0m\n", __FUNCTION__);
// 			}
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_FAN :

// 			if (nf_sysman_qc_auto_test_fan()) {
// 				printf("\e[32m[%s] APP QC FAN TEST OK !!\e[0m\n", __FUNCTION__);
// 			} else {
// 				printf("\e[32m[%s] APP QC FAN TEST FAIL !!\e[0m\n", __FUNCTION__);
// 			}
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_TEMPER :

// 			if (nf_sysman_qc_auto_test_temper()) {
// 				printf("\e[32m[%s] APP QC TEMPER TEST OK !!\e[0m\n", __FUNCTION__);
// 			} else {
// 				printf("\e[32m[%s] APP QC TEMPER TEST FAIL !!\e[0m\n", __FUNCTION__);
// 			}
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_HDD :
// 			printf("\e[32m APP QC HDD NUM = %d\e[0m\n", nf_sysman_hdd_num_check());
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_ODD :
// 			printf("\e[32m APP QC ODD NUM = %d\e[0m\n", nf_sysman_odd_num_check());
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_RS232 :
// 			nf_sysman_qc_auto_test_rs232();
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_POE :
// 			nf_sysman_qc_auto_test_poe();
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_LED_ALL_ON :
// 			 nf_dev_keypad_led_all_on();
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_LED_ALL_OFF :
// 			nf_dev_keypad_led_all_off();
// 		break;

// 		case NF_SYSMAN_APP_QC_TEST_NET :
// 			nf_sysman_qc_auto_test_network_switch();
// 		break;
// 	}

// 	return 0;
// }
// __commandlist(app_qc_test, "app_qc_test", app_qc_test_help, app_qc_test_help);

int get_qc_mac_writer_mode(void)
{
	return _qcmode_mac_writer;
}
// int scm_get_ntp_time_pc(char *server_name, GTimeVal *time)
// {
// 	STPacketData stPatatData;
// 	int nPackatSize = sizeof(STPacketData);
// 	int received;
	
// 	struct sockaddr_in serv_addr;
// 	int sock, data_size = 0, ret = -1;
	
// 	STPacketData sendPacket;
// 	STPacketData recvPacket;
	
// 	nf_sysman_qc_init_stpacketdata(&sendPacket);
// 	nf_sysman_qc_init_stpacketdata(&recvPacket);

// 	sock = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sock == -1) {
// 		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
// 	}

// 	memset(&serv_addr, 0, sizeof(serv_addr));
// 	serv_addr.sin_family = AF_INET;
// 	serv_addr.sin_addr.s_addr = inet_addr(server_name);
// 	serv_addr.sin_port = htons(10252);
// 	printf("\e[33m [%s] [ServerIP : %s]\e[0m\n", __FUNCTION__,server_name);
// 	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
// 		perror("connect");
// 		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
// 		ret =  NF_SYSMAN_QC_PD_ERROR_NETWORK;
// 		return -2;
// 	}
	
// 	sendPacket.cCode = SetTC_SETTIME;
// 	send(sock, (char*)&sendPacket, sizeof(STPacketData), 0);

// 	received = read(sock, &recvPacket, nPackatSize);
// 	if(received <= 0 || received != nPackatSize) 
// 	{
// 		printf("received %d error !!!\n:",received);
// 		return -3;
// 	}
// 	memcpy(&stPatatData, &recvPacket, nPackatSize);
// 	if(stPatatData.cCode != CTSet_SEND_TIME)
// 	{
// 		return -4;
// 	}

// 	time->tv_sec  = atoi(stPatatData.cParam100);

// 	printf("ret->tv_sec %d \n",time->tv_sec);
	
// 	if(sock > 0)
// 		close(sock);
	
// 	return (time->tv_sec > 0 ? 0 : -1);	
// }
