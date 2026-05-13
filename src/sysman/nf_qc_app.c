#include "nf_common.h"
#include "libsst.h"

#include "nf_qc.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_hw_enc.h"
#include "nf_api_param_app.h"
#include "nf_api_param_fwver.h"

#include "nf_qc_app.h"
#include "nf_ptz.h"
#include "nf_hw.h"

#include "nf_network.h"
#include "nf_watchdog.h"
#include "nf_util_device.h"
#include "nfdal.h"

#if defined(SCHIP_COPY_PROTECTION)
	#include "itx_seed_algo.h"
#endif

#define DEBUG_QC_APP_JBSHELL
#ifdef DEBUG_QC_APP_JBSHELL
    #include "jbshell.h"
#endif


#define QCMODE_CHECK_ROOT_DIR_NAME  "/mnt"
#define QCMODE_CHECK_MOUNT_DIR_NAME "/mnt/qcusb"
#define QCMODE_CHECK_FSTYPE         "vfat"
#define QCMODE_CHECK_TRY_DISK_CNT   16

#define QCMODE_CHECK_QC_MODE_ON                 "/itx_qc_mode_on.txt"
#define QCMODE_CHECK_QC_PARAM                   "/itx_qc_param.txt"
#define QCMODE_CHECK_QC_MODE_FACTORY_DEFAULT    "/itx_qc_mode_default.txt"
#define QCMODE_CHECK_QC_MODE_FW_UP              "/itx_qc_mode_fwup.txt"
#define QCMODE_CHECK_QC_MODE_FORMAT             "/itx_qc_mode_format.txt"

#define QC_MODE_OPTION_NTP              "ntp="
#define QC_MODE_OPTION_PCP              "pcp="
#define QC_MODE_OPTION_LANG             "lang="
#define QC_MODE_OPTION_HDD              "HDD="
#define QC_MODE_OPTION_ODD              "ODD="
#define QC_MODE_OPTION_SIGNAL           "signal="

typedef enum _VCODE_IDX{
	IDX_NONE = 0,
	IDX_ITX, IDX_GPS, IDX_VDC,
	IDX_ORI, IDX_CBC, IDX_TKK,
	IDX_G4S, IDX_VCN, IDX_HON,
	IDX_VIT, IDX_ASP, IDX_I3D,
	IDX_CYT, IDX_DAU, IDX_KBD,
	IDX_VTR,
}VCODE_IDX;

static char strVcode[][4] = {
	"ITX","GPS","VDC",
	"ORI","CBC","TKK",
	"G4S","VCN","HON",
	"VIT","ASP","I3D",
	"CYT","DAU","KBD",
	"VTR",
};

int qc_audio_manual_enable;

static int _qcmode_mac_writer=0;

static int _qcmode_is_enable=0;
static int _qcmode_is_factory_default=0;
static int _qcmode_is_fwup=0;
static int _qcmode_is_format=0;

static char _qcmode_is_fwup_path[1024] = {0,};
static char _qcmode_is_ntp_server[256] = {0,};
static char _qcmode_option_lang[32] = {0, };
static char _qcmode_hdd_num[8] = {0,};
static char _qcmode_odd_num[8] = {0,};
static char _qcmode_signal[32] = {0,};
static int _qc_mode_loopout_chk = 0;


static void _nf_qc_app_set_sysdb_sig_type(gboolean sig_type);
static void _qcmode_do_fwup(gpointer param);
static int _qc_mode_get_usb_dev( gchar *buff, gint buff_len);

static int _qcmode_gui_printf(  const char *format, ... );
static int _qcmode_gui_printf_no_update(  const char *format, ... );
static int _qcmode_gui_update(void);
static void _qcmode_gui_hexdump(gpointer p, int len);
static void _qcmode_gui_file(gchar* filename);

static void _factory_def_event_function(int event, int arg, void *context);
static void _factory_def_thread_func (int *_status);
static void _qcmode_do_factory_default(gpointer param);

static void _format_event_function(int event, int arg, void *context);
static void _format_thread_func ( int *_status );
static void _qcmode_do_format(gpointer param);

static void _qcmode_do_test1(gpointer param);
static void _qcmode_do_fwup(gpointer param);
static void _dhcp_thread_func ( int *_status );
static void _qcmode_do_dhcp_renew(gpointer param);

static void _qcmode_do_dump_nand(gpointer param);
static void _qcmode_do_dump_notify(gpointer param);
static void _qcmode_do_dump_camstat(gpointer param);
static void _qcmode_do_dump_infofs(gpointer param);
static void _qcmode_do_dump_sst(gpointer param);
static void _qcmode_do_dump_mem(gpointer param);
static void _qcmode_do_dump_qcstat(gpointer param);

static void nf_sysman_qc_set_def_signal(int mode);
static gboolean _nf_sysman_qc_result_save_in_usb(NF_SYSMAN_QC_RESULT qc_result, int item_total_num);

#if defined(SCHIP_COPY_PROTECTION)
static int nf_sysman_qc_lic_parse(int licCnt, char *param, char *mac,int product, LicData* arrLicData);
static int nf_sysman_qc_lic_set_db(int licCnt, LicData* arrLicData);
static int nf_sysman_cp_key_data(char (*dstData)[36], LicData *srcData, int cnt);
#endif

gboolean nf_sysman_qcmode_init(void)
{
	gint ret;
	char dev_name[1024];
	gboolean is_chip=FALSE;

	ret = mkdir(QCMODE_CHECK_ROOT_DIR_NAME, 0755);
	if(ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_ROOT_DIR_NAME);

	ret = mkdir(QCMODE_CHECK_MOUNT_DIR_NAME, 0755);
	if(ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_MOUNT_DIR_NAME);

	/** 1. umount ==> actually don't need **/
	umount(QCMODE_CHECK_MOUNT_DIR_NAME);

	_qc_mode_get_usb_dev(dev_name, sizeof(dev_name));

	/** 2. mount **/
	if((ret = mount(dev_name, QCMODE_CHECK_MOUNT_DIR_NAME, QCMODE_CHECK_FSTYPE , 0, 0)) !=0)
	{
		g_warning("%s mount error!!! ret[%d] [%s]->[%s]", __FUNCTION__, ret, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
		return FALSE;
	}else{
		g_message("%s mounted [%s]->[%s]", __FUNCTION__, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
	}

	/** 3. process **/
	_qcmode_is_enable = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_ON,  G_FILE_TEST_EXISTS);
	_qcmode_is_factory_default = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FACTORY_DEFAULT,  G_FILE_TEST_EXISTS);
	_qcmode_is_fwup = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FW_UP,  G_FILE_TEST_EXISTS);
	_qcmode_is_format = g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FORMAT,  G_FILE_TEST_EXISTS);

	if(_qcmode_is_fwup) {
		gint    ret = 0;
		gint    contents_len = 0;
		gchar   *contents = NULL;

		ret = g_file_get_contents(QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_FW_UP, &contents, &contents_len, NULL);
		if(ret)
		{
			snprintf( _qcmode_is_fwup_path , sizeof(_qcmode_is_fwup_path)-1,
						"%s/%s",QCMODE_CHECK_MOUNT_DIR_NAME, contents);
			g_strstrip(_qcmode_is_fwup_path);
			g_free(contents);

		}
	}

	if(_qcmode_is_enable) {
		gint    ret = 0;
		gint    contents_len = 0, limit_len = 0;
		gchar   *contents = NULL;

		ret = g_file_get_contents(QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_MODE_ON, &contents, &contents_len, NULL);
		if(ret)
		{
			//contents_len = strstr(contents, QC_MODE_OPTION_NTP);  
			limit_len = strstr(contents, QC_MODE_OPTION_LANG);
			if((contents_len = strstr(contents, QC_MODE_OPTION_NTP)) != NULL)
				_qcmode_mac_writer = 1;
			else if((contents_len = strstr(contents, QC_MODE_OPTION_PCP)) != NULL)
				_qcmode_mac_writer = 2;
			else
				_qcmode_mac_writer = 0;
			if (limit_len == NULL) {
				if(contents_len != NULL)
				{
					snprintf( _qcmode_is_ntp_server , sizeof(_qcmode_is_ntp_server)-1,"%s",contents+4);
					g_strstrip(_qcmode_is_ntp_server);
				}
			} else {
				if(contents_len != NULL)
				{
					snprintf( _qcmode_is_ntp_server , limit_len - (contents_len + strlen(QC_MODE_OPTION_NTP)), 
								"%s", contents_len + strlen(QC_MODE_OPTION_NTP));
					g_strstrip(_qcmode_is_ntp_server);
				}
			}

			contents_len = strstr(contents, QC_MODE_OPTION_LANG);
			limit_len = strstr(contents, QC_MODE_OPTION_SIGNAL);

			if (limit_len == NULL) {
				if (contents_len != NULL)
				{
					snprintf( _qcmode_option_lang , sizeof(_qcmode_option_lang) - 1, "%s", contents_len + strlen(QC_MODE_OPTION_LANG));
					g_strstrip(_qcmode_option_lang);
				}
			} else {
				if (contents_len != NULL)
				{
					snprintf( _qcmode_option_lang , limit_len - (contents_len + strlen(QC_MODE_OPTION_LANG)), 
								"%s", contents_len + strlen(QC_MODE_OPTION_LANG));
					g_strstrip(_qcmode_option_lang);
				}

				contents_len = strstr(contents, QC_MODE_OPTION_SIGNAL);
				if (contents_len != NULL) {
					snprintf( _qcmode_signal, sizeof(_qcmode_signal) - 1, "%s", contents_len + strlen(QC_MODE_OPTION_SIGNAL));              
					g_strstrip(_qcmode_signal);
				}
			}
		}

		if (g_file_test( QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_PARAM,  G_FILE_TEST_EXISTS)) {
			ret = g_file_get_contents(QCMODE_CHECK_MOUNT_DIR_NAME QCMODE_CHECK_QC_PARAM, &contents, &contents_len, NULL);
			if (ret) {
				contents_len = strstr(contents, QC_MODE_OPTION_HDD);
				if (contents_len != NULL) {
					strncpy(_qcmode_hdd_num, contents_len + strlen(QC_MODE_OPTION_HDD), 2);
				}

				contents_len = strstr(contents, QC_MODE_OPTION_ODD);
				if (contents_len != NULL) {
					strncpy(_qcmode_odd_num, contents_len + strlen(QC_MODE_OPTION_ODD), 1);
				}
			}
		}

		// ksi_test nf_dev_drv_ext_cntl_loopout_chk_chip(&is_chip);
		_qc_mode_loopout_chk = is_chip;

		printf("\e[32m [%s] _qcmode_is_enalbe = %d\e[0m\n", __FUNCTION__, _qcmode_is_enable);
		g_free(contents);
	}

	g_message("%s qcmode_is_enable[%d][%s]", __FUNCTION__, _qcmode_is_enable, _qcmode_is_ntp_server);
	g_message("%s qcmode_is_factory_default[%d]", __FUNCTION__, _qcmode_is_factory_default);
	g_message("%s qcmode_is_fwup[%d][%s]", __FUNCTION__, _qcmode_is_fwup, _qcmode_is_fwup_path);
	g_message("%s qcmode_is_format[%d]", __FUNCTION__, _qcmode_is_format);
	g_message("%s _qcmode_is_ntp_server[%s]", __FUNCTION__, _qcmode_is_ntp_server);
	g_message("%s _qcmode_hdd_num[%s]", __FUNCTION__, _qcmode_hdd_num);
	g_message("%s _qcmode_odd_num[%s]", __FUNCTION__, _qcmode_odd_num);
	g_message("%s _qcmode_option_lang[%s]", __FUNCTION__, _qcmode_option_lang);
	g_message("%s _qcmode_signal[%s]", __FUNCTION__, _qcmode_signal);
	g_message("%s _qcmode_loopout_enable[%d]", __FUNCTION__, _qc_mode_loopout_chk);

	//while(1) sleep (1);   

	/** 4. unmount **/
	if( !_qcmode_is_fwup )
		umount(QCMODE_CHECK_MOUNT_DIR_NAME);
	return 1;
}

gboolean nf_sysman_qcmode_is_enable(void)
{
	return (_qcmode_is_enable)?1:0;
}

gboolean nf_sysman_qcmode_is_factory_default(void)
{
	return (_qcmode_is_factory_default)?1:0;
}
gboolean nf_sysman_qcmode_is_fwup(void)
{
	return (_qcmode_is_fwup)?1:0;
}

gboolean nf_sysman_qcmode_is_format(void)
{
	return (_qcmode_is_format)?1:0;
}

const gchar *nf_sysman_get_qcmode_option_lang(void)
{
	return _qcmode_option_lang;
}

void nf_sysman_set_qc_live_audio(guint is_on)
{
	qc_audio_manual_enable = is_on;
}

gboolean nf_sysman_check_is_loopout(void)
{
	return _qc_mode_loopout_chk;
}   

int nf_qc_ap_get_mac_writer_mode(void)
{
	return _qcmode_mac_writer;
}


gboolean nf_sysman_qc_manual_test_factory_key(void)
{
	guchar data = 0xff;

	nf_dev_board_pp_get_gpio(QC_TEST_FACTORY_KEY_GPIO_BASE, QC_TEST_FACTORY_KEY_GPIO_PIN, &data);

	printf("\e[33m [%s] factory key data = 0x%x\e[0m\n", __FUNCTION__, data);

	if (data == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void _nf_qc_app_set_sysdb_sig_type(gboolean sig_type)
{
	g_message("%s called [%d]", __FUNCTION__, sig_type);

	nf_sysdb_set_bool("sys.info.sig_type", sig_type);
	nf_sysdb_save("sys");
}

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
			printf("usb_str = %p\n", usb_str);
			#if defined(_ANF5G_1648D) || defined(_ANF5G_0824D) \
			 || defined(_ANF5HG_1648D) || defined(_ANF5HG_0824D) \
			 || defined(_ANF6HG_1648D) || defined(_ANF6HG_0824D)
			if (usb_str) {
				printf("%s\n", usb_str);
				if (strstr(usb_str, "xhci"))
					continue;
			}
			#endif
			if(usb_str) {
				snprintf( buff, buff_len, "/dev/sd%c1", 'a'+i);
				printf("%s\n", buff);
				return 1;
			}
		}
	}

	memset( buff, 0x00, buff_len);
	return 0;
}

//#include "../nfgui/viewers/vw_qc_test.h"
extern gint vw_qc_test_info_add_line(gchar *str, gint len);
extern gint vw_qc_test_info_update();
extern gint vw_qc_test_info_erase();

int nf_sysman_qcmode_gui_printf(  const char *format, ... ){

   va_list ap;
   char    buffer[4096];
   int     size;

   va_start(ap, format);
   size = vsprintf(buffer, (char *)format, ap);
   va_end(ap);

//   gdk_threads_enter();   
   vw_qc_test_info_add_line( buffer, size > 128? 128:size );
	vw_qc_test_info_update();
//   gdk_threads_leave();

   return (size > 128 ? 128 : size);
}

static int _qcmode_gui_printf(  const char *format, ... ){

   va_list ap;
   char    buffer[4096];
   int     size;

   va_start(ap, format);
   size = vsprintf(buffer, (char *)format, ap);
   va_end(ap);

   vw_qc_test_info_add_line( buffer, size > 128? 128:size );
	vw_qc_test_info_update();   
	
   return (size > 128 ? 128 : size);
}

static int _qcmode_gui_printf_no_update(  const char *format, ... ){

   va_list ap;
   char    buffer[4096];
   int     size;

   va_start(ap, format);
   size = vsprintf(buffer, (char *)format, ap);
   va_end(ap);

   vw_qc_test_info_add_line( buffer, size > 128? 128:size );

   return (size > 128 ? 128 : size);
}

static int _qcmode_gui_update(void){
	vw_qc_test_info_update();   
	
	return 0;
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

	g_message( "--------------------------------------------------------------------------------");
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

_qcmode_gui_printf("--------------------------------------------------------------------------------");

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
		_qcmode_gui_printf_no_update("%s", g_strstrip(buff) );
		if(++print_cnt>64){
			_qcmode_gui_printf_no_update("too many lines stop print");
			break;
		}   
	}   
	_qcmode_gui_update();

	fclose(fp);
	return;
		
}

static volatile int _factory_def_event_func_event = 0;
static volatile int _factory_def_event_func_arg = 0;

static void _factory_def_event_function(int event, int arg, void *context) {

	nf_sysman_qcmode_gui_printf("%s [%s](%d) arg[%d]", __FUNCTION__,  sst_get_event_string(event), event, arg );

	_factory_def_event_func_event = event;
	_factory_def_event_func_arg = arg;
}


static volatile int _is_factory_def_now = 0;
static void _factory_def_thread_func ( int *_status )
{
// onvif_porting
	const char  *_sysdb_cate_list[] = {
						"sys","net","audio","disk","cam","usr",
						"alarm","act","disp","rec","onvif",
						 NULL };
							
	int i;                  
	char tmp[1024];         
	
	nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
	nf_sysman_qcmode_gui_printf("%s nf_network_stop", __FUNCTION__ );
	sleep(2);
	
#if defined (_IPX_MODEL_UX)
	nf_ipcam_stop();
	nf_sysman_qcmode_gui_printf("%s nf_ipcam_stop", __FUNCTION__ );
	sleep(3);
#endif  

#if defined (_IPX_MODEL_UX) 
	while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);
#endif

	nf_record_stop( NULL );
	nf_sysman_qcmode_gui_printf("%s nf_record_stop", __FUNCTION__ );
	sleep(2);

	if( nf_filesystem_is_online()) {
		nf_filesystem_stop( _factory_def_event_function, NULL, NULL);
		nf_sysman_qcmode_gui_printf("%s nf_filesystem_stop", __FUNCTION__ );
		sleep(3);
		while( _factory_def_event_func_event != SST_EVT_FSSTOPCMPL ){
			//NF_SST_PROGRESS  progress;
			//nf_filesystem_get_progress(NF_SST_PRGT_FSSTOP, &progress);
			sleep(1);   
		}   
	}else{  
		nf_sysman_qcmode_gui_printf("%s filesystem is already offline", __FUNCTION__ );
	}   

	for(i=0; _sysdb_cate_list[i] ;++i)
	{
		//_qcmode_gui_printf("%s factory default  -> [%s]", __FUNCTION__, _sysdb_cate_list[i] );
		snprintf( tmp, sizeof(tmp)-1, "/bin/rm -f /NFDVR/data/nf_sysdb_%s.conf", _sysdb_cate_list[i]);
		proxy_system( tmp ,1,3);
	}
	nf_sysman_qcmode_gui_printf("%s factory default done", __FUNCTION__);

	proxy_system("/bin/rm -f /NFDVR/data/gui/login_user.itx",1,3);
	proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);

	nf_sysman_sync();
	nf_sysman_set_normal_boot();
	nf_sysman_qcmode_gui_printf("%s set normal boot", __FUNCTION__);

	nf_sysman_qcmode_gui_printf("%s complete", __FUNCTION__);
	nf_sysman_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);

	*_status = 2;

	return;

}

static void _qcmode_do_factory_default(gpointer param) {

	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);

	if( _is_factory_def_now == 0) {
		_is_factory_def_now = 1;
		if( g_thread_create((GThreadFunc)_factory_def_thread_func, &_is_factory_def_now, FALSE, NULL) == 0)
		{
			_is_factory_def_now = 0;
			_qcmode_gui_printf("%s thread create failed", __FUNCTION__);
		}
	}else if( _is_factory_def_now == 2) {
		_qcmode_gui_printf("%s complete", __FUNCTION__);
		_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);

		sleep(2);

		_is_factory_def_now = 0;

	}else {
		_qcmode_gui_printf("%s wait!!", __FUNCTION__);
	}

	return;
}


#include "nf_util_device.h"
#include "itx_key_table_info.h"

static volatile int _format_event_func_event = 0;
static volatile int _format_event_func_arg = 0;

static void _format_event_function(int event, int arg, void *context) {

    nf_sysman_qcmode_gui_printf("%s [%s](%d) arg[%d]", __FUNCTION__,  sst_get_event_string(event), event, arg );

    _format_event_func_event = event;
    _format_event_func_arg = arg;
}

static volatile int _is_format_now = 0;
static void _format_thread_func ( int *_status )
{
// onvif_porting
    const char  *_sysdb_cate_list[] = {
                        "sys","net","audio","disk","cam","usr",
                        "alarm","act","disp","rec","onvif",
                         NULL };
                            
    int i;                  
    char tmp[1024];         
    
    _format_event_func_event = 0x3398;
    
    for(i=0; i< LED_INDEX_MAX_NUM; ++i)
        nf_dev_keypad_led_off(i);      
        
    nf_sysman_qcmode_gui_printf("%s led all OFF", __FUNCTION__);
    
    nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
    nf_sysman_qcmode_gui_printf("%s nf_network_stop", __FUNCTION__ );
    sleep(2);
    
#if defined (_IPX_MODEL_UX)
    nf_ipcam_stop();       
    nf_sysman_qcmode_gui_printf("%s nf_ipcam_stop", __FUNCTION__ );
    sleep(3);
#endif

#if defined (_IPX_MODEL_UX)     
    while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);
#endif

    nf_record_stop( NULL );
    nf_sysman_qcmode_gui_printf("%s nf_record_stop", __FUNCTION__ );
    sleep(2);

    if( nf_filesystem_is_online()) {
        nf_filesystem_stop( _format_event_function, NULL, NULL);
        nf_sysman_qcmode_gui_printf("%s nf_filesystem_stop", __FUNCTION__ );
        sleep(3);
        while( _format_event_func_event != SST_EVT_FSSTOPCMPL ){
            //NF_SST_PROGRESS  progress;
            //nf_filesystem_get_progress(NF_SST_PRGT_FSSTOP, &progress);
            sleep(1);
        }
    }else{
        nf_sysman_qcmode_gui_printf("%s filesystem is already offline", __FUNCTION__ );
    }

    nf_sysman_qcmode_gui_printf("%s nf_disk_format start", __FUNCTION__ );
    nf_disk_format( 1,0, _format_event_function, NULL, NULL);
    while( _format_event_func_event != SST_EVT_FORMATCMPL ) {
        //NF_SST_PROGRESS  progress;
        //nf_filesystem_get_progress(NF_SST_PRGT_FORMAT, &progress);
        sleep(1);
    }

    nf_sysman_qcmode_gui_printf("%s nf_disk_format done", __FUNCTION__ );

    for(i=0; _sysdb_cate_list[i] ;++i)
    {
//      _qcmode_gui_printf("%s factory default  -> [%s]", __FUNCTION__, _sysdb_cate_list[i] );
        snprintf( tmp, sizeof(tmp)-1, "/bin/rm -f /NFDVR/data/nf_sysdb_%s.conf", _sysdb_cate_list[i]);
        proxy_system( tmp ,1,3);
    }

    nf_sysman_qcmode_gui_printf("%s factory default done", __FUNCTION__);

    proxy_system("/bin/rm -f /NFDVR/data/gui/login_user.itx",1,3);
    proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);

    nf_sysman_sync();
    nf_sysman_set_normal_boot();
    nf_sysman_qcmode_gui_printf("%s set normal boot", __FUNCTION__);

    for(i=0; i< LED_INDEX_MAX_NUM; ++i)
        nf_dev_keypad_led_on(i);
    nf_sysman_qcmode_gui_printf("%s led all ON", __FUNCTION__);

    nf_sysman_qcmode_gui_printf("%s complete", __FUNCTION__);
    nf_sysman_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);

    *_status = 2;

    return;
}


static void _qcmode_do_format(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);

	if( _is_format_now == 0) {
		_is_format_now = 1;
		if( g_thread_create((GThreadFunc)_format_thread_func, &_is_format_now, FALSE, NULL) == 0)
		{
			_is_format_now = 0;
			_qcmode_gui_printf("%s thread create failed", __FUNCTION__);
		}
	}else if( _is_format_now == 2) {
		_qcmode_gui_printf("%s complete", __FUNCTION__);
		_qcmode_gui_printf("%s ready to shutdown!!", __FUNCTION__);
	}else {
		_qcmode_gui_printf("%s wait!!", __FUNCTION__);
	}

	return;
}

static volatile int _is_test1_mode = 0;
static void _qcmode_do_test1(gpointer param){

	int i;

	int temper_cpu;

	vw_qc_test_info_erase();

	_qcmode_gui_printf("%s called", __FUNCTION__);

	if( _is_test1_mode == 0) {
		_is_test1_mode = 1;

		for(i=0; i< LED_INDEX_MAX_NUM; ++i)
			nf_dev_keypad_led_on(i);

		_qcmode_gui_printf("%s led all ON", __FUNCTION__);

		#if defined(USE_DEV_AUDIO)
			nf_dev_audio_set_dac(0x0);
		#endif
		_qcmode_gui_printf("%s SPEAKER ON( CH1->SET )", __FUNCTION__);
/*      
		nf_dev_mic_set_output_mask(1);
		nf_dev_mic_set_onoff(1);
		_qcmode_gui_printf("%s MIC ON( SET->CH1 )", __FUNCTION__);
*/
	}else {
		_is_test1_mode = 0;

		for(i=0; i< LED_INDEX_MAX_NUM; ++i)
			nf_dev_keypad_led_off(i);

		_qcmode_gui_printf("%s led all OFF", __FUNCTION__);

		#if defined(USE_DEV_AUDIO)
			nf_dev_audio_set_dac(0xff);
		#endif
		_qcmode_gui_printf("%s SET SPEAKER OFF( CH1->SET )", __FUNCTION__);
/*
		nf_dev_mic_set_output_mask(1);
		nf_dev_mic_set_onoff(0);        
		_qcmode_gui_printf("%s SET MIC OFF( SET->CH1 )", __FUNCTION__);
*/
	}

	_qcmode_gui_printf("%s complete", __FUNCTION__);

	return;
}


static volatile int _is_fwup_now = 0;
static void _qcmode_do_fwup(gpointer param)
{

	int ret = 0, i;
	NF_FW_PRGT prgt;
	vw_qc_test_info_erase();

	_qcmode_gui_printf("%s called is_run[%d]", __FUNCTION__, _is_fwup_now);
	_qcmode_gui_printf("%s fw file (%s)", __FUNCTION__, _qcmode_is_fwup_path );

	if( _is_fwup_now == 0) {
		for(i=0;i<NF_WATCHDOG_MEMBER_NOTIFY;++i)
			nf_watchdog_ctrl(i, NF_WATCHDOG_KILL_SECOND, 0 );

		_qcmode_gui_printf("%s watchdog disable", __FUNCTION__ );
		sleep(2);

		ret = nf_fw_upgrade_thread_start( _qcmode_is_fwup_path, 1);
		if( ret == 1) {
			_is_fwup_now = 1;
			_qcmode_gui_printf("%s start fwup!!", __FUNCTION__ );
		}else{
			_qcmode_gui_printf("%s failed!!", __FUNCTION__ );
		}
		sleep(2);
	}

	nf_fw_state_check(&prgt);

	_qcmode_gui_printf("%s prgt.type    [%02x]/[%02x]\n", __FUNCTION__, prgt.type, NF_FW_PRGT_TYPE_FINISH);
	_qcmode_gui_printf("%s prgt.state   [%02x]/[%02x]\n",__FUNCTION__, prgt.state, NF_FW_PRGT_FNI_UPDATE_FNISH);
	_qcmode_gui_printf("%s prgt.is_error[%s]\n", __FUNCTION__, prgt.is_error ? "ERROR":"OK" );
	_qcmode_gui_printf("%s prgt.current [%d]\n", __FUNCTION__, prgt.current);
	_qcmode_gui_printf("%s prgt.total   [%d]\n", __FUNCTION__, prgt.total);

	if( prgt.is_error ) {
		_qcmode_gui_printf("%s error", __FUNCTION__ );
		_is_fwup_now = 0;
		sleep(1);
	} else {
		if( prgt.type == NF_FW_PRGT_TYPE_FINISH
			&& prgt.state == NF_FW_PRGT_FNI_UPDATE_FNISH )
		{
			_qcmode_gui_printf("%s complete", __FUNCTION__);
			_qcmode_gui_printf("%s fw file (%s)", __FUNCTION__, _qcmode_is_fwup_path );
			_qcmode_gui_printf("%s complete", __FUNCTION__);
			_is_fwup_now = 3;
		}else{
			_qcmode_gui_printf("%s in progress... wait", __FUNCTION__ );
		}
	}

	return;
}

#include "nf_util_netif.h"

static volatile int _is_dhcp_now = 0;
static void _dhcp_thread_func ( int *_status )
{

	NF_NETIF_GET_INFO ret_info;

#if defined (_IPX_MODEL_UX)     
	nf_ipcam_stop();
	nf_sysman_qcmode_gui_printf("%s nf_ipcam_stop", __FUNCTION__ );
	sleep(3);
#endif

#if defined (_IPX_MODEL_UX)     
	while( nf_ipcam_is_all_ch_unplugged() ) sleep(1);
#endif

	nf_sysman_qcmode_gui_printf("%s netif init start", __FUNCTION__ );
	nf_netif_init();
	nf_sysman_qcmode_gui_printf("%s netif init stop", __FUNCTION__ );

	nf_netif_get_info( &ret_info);
	nf_sysman_qcmode_gui_printf("%s ipaddr      [0x%08x]", __FUNCTION__, ntohl(ret_info.ipaddr     ));
	nf_sysman_qcmode_gui_printf("%s netmask     [0x%08x]", __FUNCTION__, ntohl(ret_info.netmask    ));
	nf_sysman_qcmode_gui_printf("%s broadcast   [0x%08x]", __FUNCTION__, ntohl(ret_info.broadcast  ));
	nf_sysman_qcmode_gui_printf("%s gateway     [0x%08x]", __FUNCTION__, ntohl(ret_info.gateway    ));
	nf_sysman_qcmode_gui_printf("%s dnsserver1  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver1 ));
	nf_sysman_qcmode_gui_printf("%s dnsserver2  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver2 ));

#if defined (_IPX_MODEL_UX)     
	nf_ipcam_start();
	nf_sysman_qcmode_gui_printf("%s nf_ipcam_start", __FUNCTION__ );
#endif
	nf_sysman_qcmode_gui_printf("%s complete", __FUNCTION__);

	*_status = 2;

	return;
}

static void _qcmode_do_dhcp_renew(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);

	if( _is_dhcp_now == 0) {
		_is_dhcp_now = 1;
		if( g_thread_create((GThreadFunc)_dhcp_thread_func, &_is_dhcp_now, FALSE, NULL) == 0)
		{
			_is_dhcp_now = 0;
			_qcmode_gui_printf("%s thread create failed", __FUNCTION__);
		}
	}else if( _is_dhcp_now == 2) {

		NF_NETIF_GET_INFO ret_info;

		nf_netif_get_info( &ret_info);

		_qcmode_gui_printf("%s ipaddr      [0x%08x]", __FUNCTION__, ntohl(ret_info.ipaddr     ));
		_qcmode_gui_printf("%s netmask     [0x%08x]", __FUNCTION__, ntohl(ret_info.netmask    ));
		_qcmode_gui_printf("%s broadcast   [0x%08x]", __FUNCTION__, ntohl(ret_info.broadcast  ));
		_qcmode_gui_printf("%s gateway     [0x%08x]", __FUNCTION__, ntohl(ret_info.gateway    ));
		_qcmode_gui_printf("%s dnsserver1  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver1 ));
		_qcmode_gui_printf("%s dnsserver2  [0x%08x]", __FUNCTION__, ntohl(ret_info.dnsserver2 ));

		_qcmode_gui_printf("%s complete", __FUNCTION__);
		_is_dhcp_now = 0;
		sleep(2);
	}else {
		_qcmode_gui_printf("%s wait!!", __FUNCTION__);
	}
	return;
}


// nf_api_disk.h
extern gboolean nf_get_nand_log(void);

static void _qcmode_do_dump_nand(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);

	nf_get_nand_log();
	_qcmode_gui_file("/tmp/webra-info/nand.log");

	return;
}

static void _qcmode_do_dump_notify(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);
	_qcmode_gui_file("/tmp/webra-info/notify_dump.txt");

	return;
}

static void _qcmode_do_dump_camstat(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);
	_qcmode_gui_file("/tmp/webra-info/cam_stat.txt");

	return;
}

static void _qcmode_do_dump_infofs(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);
	_qcmode_gui_file("/tmp/webra-info/info.js");

	return;
}

static void _qcmode_do_dump_sst(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);
	_qcmode_gui_file("/proc/ifs");
	_qcmode_gui_file("/proc/ifs_io_state");
	return;
}

static void _qcmode_do_dump_mem(gpointer param){
	vw_qc_test_info_erase();
	_qcmode_gui_printf("%s called", __FUNCTION__);
	_qcmode_gui_file("/proc/cmem");
	_qcmode_gui_file("/proc/meminfo");
	return;
}

static void _qcmode_do_dump_qcstat(gpointer param){
	NF_PARAM_HW hparam;

	vw_qc_test_info_erase();
	_qcmode_gui_printf_no_update("%s called", __FUNCTION__);

	memset( &hparam, 0x00, sizeof(NF_PARAM_HW ));

	nf_api_param_hw_get_protect( &hparam, 0 );
	{
		NF_PARAM_HW *hwparam = &hparam;
		int cnt_qc_item;
		int i;

		if(strncmp(hwparam->magic, NF_HW_PARAM_MAGIC, 4) != 0 )
		{
			_qcmode_gui_printf_no_update("%s magic failed[%4.4s]", __FUNCTION__, hwparam->magic);
			_qcmode_gui_update();
			return;
		}

		_qcmode_gui_printf_no_update("Magic                 [%4.4s]", hwparam->magic);
		_qcmode_gui_printf_no_update("ETH ADDR              [%32.32s]",  hwparam->eth_addr);
		_qcmode_gui_printf_no_update("Front Type            [%32.32s]",  hwparam->pannel_type);
		_qcmode_gui_printf_no_update("RC    Type            [%32.32s]",  hwparam->rc_type);
		_qcmode_gui_printf_no_update("HW Version            [%32.32s]",  hwparam->hw_ver);
		_qcmode_gui_printf_no_update("Vendor                [%32.32s]",  hwparam->vendor);
		_qcmode_gui_printf_no_update("FW Write Fail         [%d]",  hwparam->is_fw_write_fail);
		_qcmode_gui_printf_no_update("Circular Buffer Count [%d]",  hwparam->circular_buffer_location);

		for(i=0; i<NF_SYSMAN_QC_CIRCULAR_BUFFER; i++)
		{
			if(strncmp(hwparam->qc_result[i].magic, NF_SYSMAN_QC_MAGIC, 4) != 0 )
			{
				_qcmode_gui_printf_no_update("%s QC Circular Buffer%dMagic Failed[%4.4s]", __FUNCTION__, i, hwparam->magic);
				continue;
			}

			_qcmode_gui_printf_no_update("QC Magic              [%4.4s]",  hwparam->qc_result[i].magic);
			_qcmode_gui_printf_no_update("Excute Time           [%48.48s]",  hwparam->qc_result[i].excute_time);

			for(cnt_qc_item=0; cnt_qc_item<NF_SYSMAN_QC_ITEM_MAX_NR; cnt_qc_item++)
				_qcmode_gui_printf_no_update("Excute Time           [%16.16s]",  hwparam->qc_result[i].item[cnt_qc_item]);

			_qcmode_gui_printf_no_update("\n");
		}
	}

	_qcmode_gui_update();
	return;
}

gboolean nf_sysman_qcmode_sysdb_init(void)
{       
	int cnt = 1;
	
	if (nf_sysman_qcmode_is_enable()) {
#if 0
		IPCamLoginData login_info;
		gchar str_buf[64];
		gchar ip = 2;
		gchar ch;
#endif
		#if defined(_ANF5HG_1648D) || defined(_ANF5HG_0824D) \
		 || defined(_ANF6HG_1648D) || defined(_ANF6HG_0824D)
			/* Set VLan */
			#if 0
			nf_dev_board_pp_set_switch_reg(0x14, 0x6, 0x20);    // wan
			nf_dev_board_pp_set_switch_reg(0x13, 0x6, 0x0);     // lan
			nf_dev_board_pp_set_switch_reg(0x15, 0x6, 0x10);    // cpu
			
			nf_dev_board_pp_set_switch_reg(0x1b, 0xa, 0x10);

			nf_dev_board_pp_set_switch_reg(0x14, 0xb, 0x0);
			nf_dev_board_pp_set_switch_reg(0x13, 0xb, 0x0);
			#else
			nf_dev_board_pp_switch_configuration(SWITCH_MODE_COMBINE);
			#endif
		#endif

		// for skip authentication
		nf_sysdb_set_str("usr.U0.pass","");
		nf_sysdb_set_bool("sys.info.passwd_enable",0);
		nf_sysdb_set_bool("usr.auto_logout", 0);
		nf_sysdb_set_str("cam.C0.title","@ QCMODE @");
		
		#if defined(TOTAL_SECURITY) || defined(KOBI) || defined(ITX_46GUI) || defined(CBC)
			nf_sysdb_set_uint("cam.ptz.P0.protocol", 2);
		#elif defined(VITEK)
		nf_sysdb_set_uint("cam.ptz.P0.protocol", 1);
		#else
			nf_sysdb_set_uint("cam.ptz.P0.protocol", 0);
		#endif

		// disable event buzzer
		nf_sysdb_set_int("act.event.buzzer.act", 0);
		// enable dhcp 
		nf_sysdb_set_bool("net.proto.dhcpon", 1);

#if 0
		nf_sysdb_set_bool("net.eth2.dhcpsvr", 1);

		for (ch = 0; ch < 17; ch++) {
			memset(&login_info, 0x0, sizeof(IPCamLoginData));

			g_sprintf(str_buf, "10.20.0.%d", ip);
			strncpy(login_info.hostname, str_buf, 63);

			strcpy(login_info.id, "ADMIN");
			strcpy(login_info.password, "1234");

			login_info.http_port = 80;
			login_info.rtsp_port = 554;
			login_info.ethernet =1;

			DAL_set_ipcam_login_data(login_info, ch);

			printf("[QC] ch %d ip %d host name %s\n", ch, ip, login_info.hostname);
			ip++;
		}
#endif
		// cctv mode fix
		nf_sysdb_set_bool("cam.install.mode", FALSE);
		nf_sysdb_set_bool("cam.install.dual_lan", FALSE);
	}
	
	return 1;
}

gboolean nf_sysman_qcmode_2nd_init(void)
{        
	int cnt = 1;
	
	if(nf_sysman_qcmode_is_enable()) {
	
		vw_qc_test_connect_func(1, "TEST1", _qcmode_do_test1, NULL );
		vw_qc_test_connect_func(2, "QCSTAT", _qcmode_do_dump_qcstat, NULL );
		vw_qc_test_connect_func(3, "INFOJS", _qcmode_do_dump_infofs, NULL );
//      vw_qc_test_connect_func(4, "CAMSTAT", _qcmode_do_dump_camstat, NULL );
		vw_qc_test_connect_func(5, "NAND", _qcmode_do_dump_nand, NULL );    
		
		vw_qc_test_connect_func(6, "DEFAULT", _qcmode_do_factory_default, NULL );
		vw_qc_test_connect_func(7, "SST", _qcmode_do_dump_sst, NULL );           
		
		vw_qc_test_connect_func(8, "DHCP", _qcmode_do_dhcp_renew, NULL );
		
		vw_qc_test_connect_func(10, "FORMAT", _qcmode_do_format, NULL );
		vw_qc_test_connect_func(11, "MEM", _qcmode_do_dump_mem, NULL ); 
		
		//vw_qc_test_connect_func(cnt++, "FWUP", _qcmode_do_fwup, NULL );                   
	}   
		
	return 1;
	
}   


/* APP QC API*/
#define APP_QC_DEBUG_MSG
guint nf_sysman_hdd_num_check(void)
{
	guint vendor = nf_api_param_fwver_get_vendor();   // 1228
	gchar * type_2 = nf_sysman_get_system_model();    // 1228
	
	printf("\e[32m hdd_vendor ID =  [%d] \e[0m\n", vendor);
	printf("\e[32m hdd_type=  [%s] \e[0m\n", type_2); 
	
	#if defined(_UTM7G_1648D) || defined(_UTM7G_0824D) || defined(_UTM7G_0412D)
		return strtoul(_qcmode_hdd_num, NULL, 10);
	#elif defined(_ANF8G_1648D)
		return strtoul(_qcmode_hdd_num, NULL, 10);
	#elif defined(_IPX_32P5)
		return strtoul(_qcmode_hdd_num, NULL, 10);
	#endif

	return 0;
}

guint nf_sysman_odd_num_check(void)
{
	return strtoul(_qcmode_odd_num, NULL, 0);
}   

static
void nf_sysman_qc_set_def_signal(int mode)
{
	static gboolean sig_type = FALSE;
	if(mode == 0)
	{
		sig_type = nf_sysdb_get_bool("sys.info.sig_type");
		printf("\033[0;31m %s get def sig_type = %s \033[0;39m\n", __FUNCTION__, (sig_type == TRUE) ? "PAL" : "NTSC");
	}   
	else if (mode == 1)
	{
	 
		#if 0
		nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_PAL, sig_type);
		#endif
		nf_sysdb_default( "sys" );
		nf_sysdb_load( "sys" ); 
		nf_sysdb_save( "sys" ); 
		nf_sysman_qc_set_signal();
		printf("\033[0;31m %s set def sig_type = %s \033[0;39m\n", __FUNCTION__, (sig_type == TRUE) ? "PAL" : "NTSC");
	}   
		
}       

void nf_sysman_qc_set_signal(void)
{
	//nf_sysman_qc_set_def_signal(0); //save defalt value
	if (strcmp(_qcmode_signal, "ntsc") == 0) {
		nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_PAL, FALSE);
		_nf_qc_app_set_sysdb_sig_type(FALSE);
	} else if (strcmp(_qcmode_signal, "pal") == 0) {
		nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_PAL, TRUE);
		_nf_qc_app_set_sysdb_sig_type(TRUE);
	}   
}   

gboolean nf_sysman_qc_get_ntp_serverip(char *serverip)
{
	if(0 >= strlen(_qcmode_is_ntp_server))
		return FALSE;
		
	strncpy(serverip, _qcmode_is_ntp_server, strlen(_qcmode_is_ntp_server));
		
	return TRUE;
}   

gboolean nf_sysman_qc_info_rtc(char *rtc_info)
{
	if(!nf_datetime_rtc_get(rtc_info))
		return FALSE;
		
	return TRUE;
}   

#if defined(_ANF5HG_0824D) || defined(_ANF5HG_1648D) \
 || defined(_ANF6HG_0824D) || defined(_ANF6HG_1648D)
gboolean nf_sysman_qc_auto_test_switch(void)
{
	int wan_link = 0, lan_link = 0, ret = 0;
	
	nf_dev_board_pp_get_switch_reg(0x13, 0x0, &lan_link);
	nf_dev_board_pp_get_switch_reg(0x14, 0x0, &wan_link);
	
	if ((lan_link >> 11) & 0x1) {
		ret++;
	}   
	
	if ((wan_link >> 11) & 0x1) {
		ret++;
	}   
	
	printf("\e[33m [%s] lan_link = 0x%x wan_link = 0x%x ret = %d\e[0m\n",
			__FUNCTION__, lan_link, wan_link, ret);
			
	if (ret != 2)
		return FALSE;
		
	return TRUE;
}   
#endif

gboolean nf_sysman_qc_toggle_loopout(int is_on)
{
	int ch = 0;
	
	if(nf_sysman_qcmode_is_enable())
	{
		// for(ch=0; ch<NUM_ACTIVE_CH; ch++)
		// 	nf_dev_drv_ext_cntl_loopout(ch, is_on);//ksi_test loopout 삭제???
	}       
	return 0;
}   

int flag_qc_end=0;
gboolean nf_sysman_qc_auto_test_audio(void)
{
//ksi_test qc audio test 방법 확인 후 작업 예정
#if 0
	int result = 0, i;
	int aud_num = 0;
	char str_cmd[64] = {0,};

	extern int nf_hw_get_audio_nr(void);
	aud_num = nf_hw_get_audio_nr();

	#if defined(ENABLE_AUD_HI_CHIP)
		nf_HI_aud_qc_test();
		
		for (i = 0; i < QC_TEST_AUDIO_NUM; i++) {
			if (nf_HI_aud_qc_frq_compare(i)) 
				result++;
		}
	#else
		flag_qc_end = 0;
		for (i = 0; i < aud_num; i++) {
			sprintf(str_cmd, "rm /tmp/IN0_chn%d.pcm", i);
			system(str_cmd);
		}
		if(!nf_audio_qc_pb_cb())
		{
			#if defined(APP_QC_DEBUG_MSG)
				printf("\e[31m >> [%s] Capture Fail!\e[0m\n", __FUNCTION__);
			#endif
			return FALSE;
		}
		while(!flag_qc_end)
		{
			sleep(1);
		}
		for (i = 0; i < aud_num; i++) {
			if (audio_Func(i)) 
				result++;
		}   
	#endif      
#if defined(APP_QC_DEBUG_MSG)
	printf("\e[31m >> [%s] audio test result = %d\e[0m\n", __FUNCTION__, result);
#endif
	if (result == aud_num) {
		return TRUE;    
	} else {
		return FALSE;
	}
#else
	return FALSE;
#endif
}   

gboolean nf_sysman_qc_auto_test_rs485(void)
{
	NF_PTZ_CMD ptz_cmd;
	
	memset(&ptz_cmd, 0x00, sizeof(ptz_cmd));
	
	ptz_cmd.ch = 0;
	ptz_cmd.params[0] = 50;
	ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST;

	nf_keyctrl_qc_test_set(0);
	
	nf_ptz_cmd(&ptz_cmd);
	
	g_usleep(1000 * 1000);
	
	if (!nf_keyctrl_qc_test_check()) {
		#if defined(APP_QC_DEBUG_MSG)
			printf("\e[32m [%s] rs485 test fail!!!\e[0m\n", __FUNCTION__);
		#endif
		return FALSE;
	}   
	
	ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST_STOP;
	
	nf_ptz_cmd(&ptz_cmd);
	
	#if defined(APP_QC_DEBUG_MSG)
		printf("\e[32m [%s] rs485 test ok!!!\e[0m\n", __FUNCTION__);
	#endif

	return TRUE;
}

gboolean nf_sysman_qc_manual_test_rs485(void)
{
	NF_PTZ_CMD ptz_cmd;
	
	memset(&ptz_cmd, 0x00, sizeof(ptz_cmd));
	
	ptz_cmd.ch = 0;
	ptz_cmd.params[0] = 50;
	ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST;

	nf_keyctrl_qc_test_set(0);

	nf_ptz_cmd(&ptz_cmd);
	
	g_usleep(1000 * 1000);
	
	if (!nf_keyctrl_qc_test_check()) {
		#if defined(APP_QC_DEBUG_MSG)
			printf("\e[32m [%s] rs485 test fail!!!\e[0m\n", __FUNCTION__);
		#endif
		return FALSE;
	}   
	
	ptz_cmd.cmd = NF_PTZ_CMD_QC_TEST_STOP;
	
	nf_ptz_cmd(&ptz_cmd);
	#if defined(APP_QC_DEBUG_MSG)
		printf("\e[32m [%s] rs485 test ok!!!\e[0m\n", __FUNCTION__);
	#endif

	return TRUE;
}

/**********************************
**********************************/
#define NF_SYSAMN_QC_RECORD_DEBUG
pthread_mutex_t mutex_lock   = PTHREAD_MUTEX_INITIALIZER;
int nf_sysman_qc_record_time_stamp_test(int mode)
{
	static int is_record_time_stamp_err = FALSE;
	
	#ifdef NF_SYSAMN_QC_RECORD_DEBUG
	printf("\033[0;33m %s CALL [%d]\033[0;39m\n", __FUNCTION__, mode);
	#endif  
	pthread_mutex_lock(&mutex_lock);
	if(NF_SYSMAN_QC_SET == mode)
	{
		is_record_time_stamp_err = NF_SYSMAN_QC_DECTECT_ERR;
	}   
	else if(NF_SYSMAN_QC_GET == mode)
	{
		is_record_time_stamp_err = NF_SYSMAN_QC_NORMAL;
	}   
	else
		is_record_time_stamp_err = NF_SYSMAN_QC_WRONG_DATA;
	pthread_mutex_unlock(&mutex_lock);
	return is_record_time_stamp_err;
}   
gboolean nf_sysman_qc_auto_test_fan(void)
{
	NF_UTIL_FAN_INFO fan_info;
	
	if (nf_dev_board_pp_fan_get_info(&fan_info)) {
		#if defined(APP_QC_DEBUG_MSG)
			g_message("CPU Fan Speed       [%d]", fan_info.speed_cpu_fan);
			g_message("SYS Fan1 Speed      [%d]", fan_info.speed_sys_fan1);
			g_message("SYS Fan2 Speed      [%d]", fan_info.speed_sys_fan2);
			g_message("CPU Fan TEMP        [%d]", fan_info.temper_cpu);
			g_message("SYS Fan TEMP        [%d]", fan_info.temper_sys);
			g_message("Fan DUTY            CPU[%d] SYS[%d]", fan_info.cpufan_duty, fan_info.sysfan_duty);
			g_message("Fan MODE            CPU[%d] SYS[%d]", fan_info.cpufan_mode, fan_info.sysfan_mode);
			g_message("CPU Fan Threshold   [%d]", fan_info.thresh_cpu);
			g_message("SYS Fan Threshold   [%d]", fan_info.thresh_sys);
		#endif

		// if (nf_hw_get_model() == HW_MODEL_ANF8GN_16) {
		// 	if ((fan_info.speed_sys_fan2 >= 1000 && fan_info.speed_sys_fan2 <= 5000)) {
		// 		//sys_fan1 not support
		// 		return TRUE;
		// 	} else {
		// 		return FALSE;
		// 	}
		// }

		if ((fan_info.speed_sys_fan1 >= 1000 && fan_info.speed_sys_fan1 <= 5000)
		 && (fan_info.speed_sys_fan2 >= 1000 && fan_info.speed_sys_fan2 <= 5000)) {
			return TRUE;
		} else {
			return FALSE;
		}   
	}   
	
	return FALSE;
}   

gboolean nf_sysman_qc_auto_test_temper(void)
{
	NF_UTIL_FAN_INFO fan_info;
	
	if (nf_dev_board_pp_fan_get_info(&fan_info)) {
		#if defined(APP_QC_DEBUG_MSG)
			g_message("CPU Fan Speed       [%d]", fan_info.speed_cpu_fan);
			g_message("SYS Fan1 Speed      [%d]", fan_info.speed_sys_fan1);
			g_message("SYS Fan2 Speed      [%d]", fan_info.speed_sys_fan2);
			g_message("CPU Fan TEMP        [%d]", fan_info.temper_cpu);
			g_message("SYS Fan TEMP        [%d]", fan_info.temper_sys);
			g_message("Fan DUTY            CPU[%d] SYS[%d]", fan_info.cpufan_duty, fan_info.sysfan_duty);
			g_message("Fan MODE            CPU[%d] SYS[%d]", fan_info.cpufan_mode, fan_info.sysfan_mode);
			g_message("CPU Fan Threshold   [%d]", fan_info.thresh_cpu);
			g_message("SYS Fan Threshold   [%d]", fan_info.thresh_sys);
		#endif
		if ((fan_info.temper_sys != 0 && fan_info.temper_sys != 255)) {
			return TRUE;
		} else {
			return FALSE;
		}   
	}   
	
	return FALSE;
}   

static gboolean _nf_sysman_qc_result_save_in_usb(NF_SYSMAN_QC_RESULT qc_result, int item_total_num)
{
	gchar ret;
	gchar dev_name[1024];
	gchar file_name[256] = {0, };
	gchar next_mac[32] = {0, };
	gchar usb_mac[32] = {0, }, usb_mac_temp[32] = {0, };
	guint mac_temp = 0; 
	gint file_len = 0, temp_len = 0;
	gchar result_str[2048] = {0, };
	gint result_len = 0, result_len_temp = 0, i, j;
	FILE *fp = NULL;
	unsigned char mac_hex[] = "0123456789ABCDEF";
	NF_PARAM_HW hwparam;
	gchar *model=NULL, *model_ch=NULL;

	model=nf_sysman_get_system_model();
	model_ch=nf_sysman_get_model_ch();
	
	memset(&hwparam, 0x00, sizeof(NF_PARAM_HW));
	#if defined(SCHIP_COPY_PROTECTION)
		
		nf_api_param_hw_get_protect(&hwparam, 0);
		
		if (strncmp(hwparam.magic, NF_HW_PARAM_MAGIC, 4) != 0) {
			g_warning("%s %d HW PARAM MAGIC ERROR!!!!", __FUNCTION__, __LINE__);
			return FALSE;
		}   
	#else   
		nf_api_param_hw_get(&hwparam, TRUE);
	#endif
	
	ret = mkdir(QCMODE_CHECK_ROOT_DIR_NAME, 0755);
	if (ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_ROOT_DIR_NAME);
		
	ret = mkdir(QCMODE_CHECK_MOUNT_DIR_NAME, 0755);
	if (ret != 0)
		g_warning("%s %s exists..", __FUNCTION__, QCMODE_CHECK_MOUNT_DIR_NAME);
		
	_qc_mode_get_usb_dev(dev_name, sizeof(dev_name));
	
	umount(QCMODE_CHECK_MOUNT_DIR_NAME);
	
	if ((ret = mount(dev_name, QCMODE_CHECK_MOUNT_DIR_NAME, QCMODE_CHECK_FSTYPE , 0, 0)) != 0) {
		g_warning("%s mount error!!! ret[%d] [%s]->[%s]", __FUNCTION__, ret, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
		return FALSE;
	} else {
		g_message("%s mounted [%s]->[%s]", __FUNCTION__, dev_name, QCMODE_CHECK_MOUNT_DIR_NAME);
	}   
	
	strncat(file_name, QCMODE_CHECK_MOUNT_DIR_NAME, sizeof(file_name));
	
	file_len = strlen(file_name);
	temp_len = sprintf(file_name + file_len, "/%s_%s", model, model_ch);
	file_len += temp_len;
	
	if (!g_file_test(file_name, G_FILE_TEST_EXISTS)) {
		g_mkdir(file_name, 0755);
	}   

	temp_len = sprintf(file_name + file_len, "/%s_", nf_api_param_hw_get_hwver());
	file_len += temp_len;

	#if 0//defined(SCHIP_COPY_PROTECTION)
		temp_len = sprintf(file_name + file_len, "%s", nf_param_hw_enc_get_qc_mac());
		file_len += temp_len;
	#else
		for (i = 0; i < strlen(hwparam.eth_addr); i++) {
			if (hwparam.eth_addr[i] != ':') {
				temp_len = sprintf(file_name + file_len, "%c", hwparam.eth_addr[i]);
				file_len += temp_len;
			}
		}
	#endif

	temp_len = sprintf(file_name + file_len, ".txt");
	file_len += temp_len;

	fp = fopen(file_name, "a");
	if (fp == NULL) {
		printf("\e[32m [%s] %s FILE OPEN ERROR!!!\e[0m\n", __FUNCTION__, file_name);
		return FALSE;
	}

	result_len = sprintf(result_str, "=======================================================\nEXCUTE TIME:%s", qc_result.excute_time);

	result_len_temp = sprintf(result_str + result_len, "MODEL:%s\n", model);
	result_len += result_len_temp;

	result_len_temp = sprintf(result_str + result_len, "CH:%s\n", model_ch);
	result_len += result_len_temp;

	result_len_temp = sprintf(result_str + result_len, "HW VER:%s\n", nf_api_param_hw_get_hwver());
	result_len += result_len_temp;

	#if defined(SCHIP_COPY_PROTECTION)
		result_len_temp = sprintf(result_str + result_len, "MAC:%s\n", nf_param_hw_enc_get_qc_mac());
	#else
		result_len_temp = sprintf(result_str + result_len, "MAC:%s\n", hwparam.eth_addr);
	#endif
	result_len += result_len_temp;

	result_len_temp = sprintf(result_str + result_len, "PANEL TYPE:%s\n", nf_api_param_hw_get_front_type());
	result_len += result_len_temp;

	result_len_temp = sprintf(result_str + result_len, "REMOCON TYPE:%s\n", nf_api_param_hw_get_rc_type());
	result_len += result_len_temp;

	for (i = 0; i < item_total_num; i++) {
		result_len_temp = sprintf(result_str + result_len, "%s\n", qc_result.item[i]);
		result_len += result_len_temp;
	}

	result_len_temp = sprintf(result_str + result_len, "=======================================================\n");
	result_len += result_len_temp;

	#if defined(APP_QC_DEBUG_MSG)
		printf("\e[32m [%s] qc_result str is\n%s\e[0m\n", __FUNCTION__, result_str);
		printf("\e[32m [%s] qc_result str len is %d\e[0m\n", __FUNCTION__, result_len);
	#endif

	fwrite(result_str, result_len, 1, fp);
	fclose(fp);

#if defined(_ANF5G_1648D) || defined(_ANF5G_0824D) \
 || defined(_ANF5HG_1648D) || defined(_ANF5HG_0824D) \
 || defined(_ANF6HG_1648D) || defined(_ANF6HG_0824D)
	memset(file_name, 0x00, sizeof(file_name));
	strncat(file_name, QCMODE_CHECK_MOUNT_DIR_NAME, sizeof(file_name));

	file_len = strlen(file_name);
	temp_len = sprintf(file_name + file_len, "/itx_qc_param.txt");
	file_len += temp_len;

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		printf("\e[32m [%s] %s FILE OPEN ERROR!!!\e[0m\n", __FUNCTION__, file_name);
		return FALSE;
	}

	fread(usb_mac, sizeof("mac=xx:xx:xx:xx:xx:xx") - 1, 1, fp);

	fclose(fp);

	for (i = 4; i < sizeof("mac=xx:xx:xx:xx:xx:xx") - 1; i++) {
		if (hwparam.eth_addr[i - 4] != usb_mac[i]) {
			printf("\e[32m [%s] hwparam.eth_addr[%d] = %c, usb_mac[%d] = %c\e[0m\n",
						__FUNCTION__, i - 4, hwparam.eth_addr[i - 4], i, usb_mac[i]);
		}
	}

	for (i = 9, j =0; i < strlen(hwparam.eth_addr); i++) {
		if (hwparam.eth_addr[i] != ':') {
			usb_mac_temp[j] = hwparam.eth_addr[i];
			j++;
		}
	}

	mac_temp = strtoul(usb_mac_temp, NULL, 16);

	mac_temp += 1;
	mac_temp &= 0xffffff;

	usb_mac[13] = mac_hex[(mac_temp >> 20) & 0xf];
	usb_mac[14] = mac_hex[(mac_temp >> 16) & 0xf];
	usb_mac[16] = mac_hex[(mac_temp >> 12) & 0xf];
	usb_mac[17] = mac_hex[(mac_temp >> 8) & 0xf];
	usb_mac[19] = mac_hex[(mac_temp >> 4) & 0xf];
	usb_mac[20] = mac_hex[(mac_temp >> 0) & 0xf];

	fp = fopen(file_name, "r+");
	if (fp == NULL) {
		printf("\e[32m [%s] %s FILE OPEN ERROR!!!\e[0m\n", __FUNCTION__, file_name);
		return FALSE;
	}

	fwrite(usb_mac, strlen(usb_mac), 1, fp);

	fclose(fp);
#endif
}

gboolean nf_sysman_qc_result_save(gchar item[][NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN], int item_total_num)
{
	NF_PARAM_HW hw_param;
	NF_SYSMAN_QC_RESULT qc_reslt;
	#if defined(SCHIP_COPY_PROTECTION)
		NF_HW_PARAM_ENC_DATA data;
	#endif
	int item_cnt=0;
	
	// get hw_param
	memset(&hw_param, 0x00, sizeof(hw_param));
	nf_api_param_hw_get_protect( &hw_param, 0 );
	if(strncmp(hw_param.magic, NF_HW_PARAM_MAGIC, 4) != 0 )
	{
		#if defined(_UTM7G_1648D) || defined(_UTM7G_0824D) || defined(_UTM7G_0412D)
			printf("\e[33m [%s] First QC Test\e[0m\n", __FUNCTION__);
			memset(&hw_param, 0x00, sizeof(hw_param)); 
		#elif defined(_ANF8G_1648D) 
			printf("\e[33m [%s] First QC Test\e[0m\n", __FUNCTION__);
			memset(&hw_param, 0x00, sizeof(hw_param)); 
		#elif defined(_IPX_32P5) 
			printf("\e[33m [%s] First QC Test\e[0m\n", __FUNCTION__);
			memset(&hw_param, 0x00, sizeof(hw_param));
		#else
			g_warning("%s %d HW PARAM MAGIC ERROR!!!!", __FUNCTION__, __LINE__);
			g_warning("%s QC RESULT SAVE FAIL!", __FUNCTION__);
			return FALSE;
		#endif
	}
	
	// set QC result
	memset(&qc_reslt, 0x00, sizeof(qc_reslt));
	strncpy(qc_reslt.magic, NF_SYSMAN_QC_MAGIC, 4);
	nf_sysman_qc_info_rtc(qc_reslt.excute_time);
	if(item_total_num > NF_SYSMAN_QC_ITEM_MAX_NR)
	{
		g_warning("%s %d Total QC Result Item OVER!! Maximum [%d]", __FUNCTION__, __LINE__, NF_SYSMAN_QC_ITEM_MAX_NR);
		item_total_num=NF_SYSMAN_QC_ITEM_MAX_NR;
	}   
	for(item_cnt=0; item_cnt<item_total_num; item_cnt++)
		strncpy(qc_reslt.item[item_cnt], item[item_cnt], NF_SYSMAN_QC_ITEM_RESULT_MAX_LEN);
		
	// set hw_param
	hw_param.circular_buffer_location++;
	if((hw_param.circular_buffer_location >= NF_SYSMAN_QC_CIRCULAR_BUFFER) || (hw_param.circular_buffer_location < 0))
		hw_param.circular_buffer_location = 0;
		
	memcpy(&hw_param.qc_result[hw_param.circular_buffer_location], &qc_reslt, sizeof(NF_SYSMAN_QC_RESULT));
	
	#if defined(SCHIP_COPY_PROTECTION)
		memset(&data, 0x00, sizeof(data));
		if (nf_sysman_get_protect_data(&data) == FALSE) {
			printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
			return FALSE;
		}   
		
		//nf_api_param_hw_set(&hw_param);
		
		nf_sysman_set_protect_data(&data);
	#endif

	_nf_sysman_qc_result_save_in_usb(qc_reslt, item_total_num);

	g_message("%s QC RESULT SAVE OK!", __FUNCTION__);
	return TRUE;
}


gboolean nf_sysman_qc_factory_default(void)
{
	const char  *_sysdb_cate_list[] = {
						"sys","net","audio","disk","cam","usr",
						"alarm","act","disp","rec",
						 NULL };

	int i;
	char tmp[1024];

	nf_network_stop( NF_DISCONN_SVR_POWER_OFF );
	sleep(2);

	nf_record_stop( NULL );
	sleep(2);

	for(i=0; _sysdb_cate_list[i] ;++i)
	{
		snprintf( tmp, sizeof(tmp)-1, "/bin/rm -f /NFDVR/data/nf_sysdb_%s.conf", _sysdb_cate_list[i]);
		proxy_system( tmp ,1,3);
	}

	proxy_system("/bin/rm -f /NFDVR/data/gui/login_user.itx",1,3);
	proxy_system("/bin/rm -f /NFDVR/data/panic_rec",1,3);

	nf_sysman_sync();
	nf_sysman_qc_set_def_signal(1); //set default type
	nf_sysman_sync();
	nf_sysman_set_normal_boot();
	return TRUE;

}
/* end of APP QC API */


#define NF_SYSMAN_APP_QC_TEST_ALARM     0
#define NF_SYSMAN_APP_QC_TEST_RS485     1
#define NF_SYSMAN_APP_QC_TEST_AUDIO     2
#define NF_SYSMAN_APP_QC_TEST_RTC       3
#define NF_SYSMAN_APP_QC_TEST_FAN       4
#define NF_SYSMAN_APP_QC_TEST_TEMPER    5
#define NF_SYSMAN_APP_QC_TEST_HDD       6
#define NF_SYSMAN_APP_QC_TEST_ODD       7

gboolean nf_sysman_qc_auto_test_alarm(void)
{
	extern int nf_hw_get_alarm_out_nr(void);

	guint i = 0;
	guint sensor_val = 0;
	guint ret = 0;
	gint alarm_out_nr=0;
	gint pba_type=nf_hw_get_board_type();
	guint alarm_on_value=0;
	guint alarm_off_value=0;

	// TAKEX Board(Kb_Device)
	gchar * type_2 = nf_sysman_get_system_model();

	if(pba_type==HW_BOARD_TYPE_A)
	{
		if (nf_hw_get_model() == HW_MODEL_ANF8GN_16) {
			alarm_on_value = 0x3ff55;
			alarm_off_value = 0xaa;
		} else if (nf_hw_get_model() == HW_MODEL_IPXP5_32) {
			alarm_on_value = 0x3ff55;
			alarm_off_value = 0xaa;
		} else {
			alarm_on_value = QC_TEST_ALARM_ON_VAL;
			alarm_off_value = QC_TEST_ALARM_OFF_VAL;
		}
	}
	else if(pba_type==HW_BOARD_TYPE_B)
	{
		alarm_on_value = 0x1;
		alarm_off_value = 0x2;
	}
	else
	{
		printf("UNKNOWN PBA TYPE\n");
		return FALSE;
	}

	#if defined(_UTM6GB_0412D)
		if(!strncmp(type_2, "UTM6GB_04_TAKEX", 15))
			{
				printf("\e[32m into _UTM6GB_0412D 1537 \e[0m \n");
			alarm_on_value = QC_TEST_ALARM_ON_VAL_TAKEX;
			alarm_off_value = QC_TEST_ALARM_OFF_VAL_TAKEX;
			}
	#endif

	alarm_out_nr=nf_hw_get_alarm_out_nr();
	for (i = 0; i < alarm_out_nr; i++)
		nf_dev_relay_ch_on(i);

	g_usleep(1000 * 2000);

	nf_dev_sensor_get_curr_value(&sensor_val);

	/*#if !defined(DAYOU) && defined(_UTM6GB_0824D)
	sensor_val = (sensor_val >> 4) & 0x3;
	#endif*/

	printf("\e[32m [%s] go sensor val = 0x%x %x \e[0m\n", __FUNCTION__, sensor_val, alarm_on_value);


	#if defined(APP_QC_DEBUG_MSG)
		printf("\e[32m [%s] go sensor val = 0x%x %x \e[0m\n", __FUNCTION__, sensor_val, alarm_off_value);

	#endif
	if (sensor_val == alarm_on_value)
		ret++;

	for (i = 0; i < alarm_out_nr; i++)
		nf_dev_relay_off(i);

	g_usleep(1000 * 2000);
	nf_dev_sensor_get_curr_value(&sensor_val);

	/*#if !defined(DAYOU) && defined(_UTM6GB_0824D)
	sensor_val = (sensor_val >> 4) & 0x3;
	#endif*/

	#if defined(APP_QC_DEBUG_MSG)
		printf("\e[32m [%s] sensor val = 0x%x %d \e[0m\n", __FUNCTION__, sensor_val, alarm_off_value);
	#endif

	if (sensor_val == alarm_off_value)
		ret++;

	if (ret != 2)
		return FALSE;

	return TRUE;
}

#ifdef DEBUG_QC_APP_JBSHELL

static char app_qc_test_help[] = "0 [alarm]\n"
								  "1 [rs485]\n"
								  "2 [audio]\n"
								  "3 [rtc]\n"
								  "4 [fan]\n"
								  "5 [temper]\n"
								  "6 [hdd]\n"
								  "7 [odd]\n";

static int app_qc_test(int argc, char **argv)
{
	int test_num = 0;
	gchar buf[64]; 
	int i;
	
	memset(buf, 0x00, sizeof(buf));
	
	if( argc < 2 )
	{
		g_message("%s => ARGC ERROR : %s", __FUNCTION__, app_qc_test_help);
		return 0;
	}   
	
	test_num  = (int)strtoul(argv[1], NULL, 0);
	
	switch (test_num)
	{
		case NF_SYSMAN_APP_QC_TEST_ALARM :

			if (nf_sysman_qc_auto_test_alarm()) {
				printf("\e[32m[%s] APP QC ALARM TEST OK !!\e[0m\n", __FUNCTION__);
			} else {
				printf("\e[32m[%s] APP QC ALARM TEST FAIL !!\e[0m\n", __FUNCTION__);
			}
		break;

		case NF_SYSMAN_APP_QC_TEST_RS485 :

			if (nf_sysman_qc_manual_test_rs485()) {
				printf("\e[32m[%s] APP QC RS485 TEST OK !!\e[0m\n", __FUNCTION__);
			} else {
				printf("\e[32m[%s] APP QC RS485 TEST FAIL !!\e[0m\n", __FUNCTION__);
			}
		break;

		case NF_SYSMAN_APP_QC_TEST_AUDIO :

			if (nf_sysman_qc_auto_test_audio()) {
				printf("\e[32m[%s] APP QC AUDIO TEST OK !!\e[0m\n", __FUNCTION__);
			} else {
				printf("\e[32m[%s] APP QC AUDIO TEST FAIL !!\e[0m\n", __FUNCTION__);
			}
		break;

		case NF_SYSMAN_APP_QC_TEST_RTC :

			if (nf_sysman_qc_info_rtc(buf)) {
				printf("\e[32m[%s] APP QC RTC OK !! INFO = %s\e[0m\n", __FUNCTION__, buf);
			} else {
				printf("\e[32m[%s] APP QC RTC FAIL !!\e[0m\n", __FUNCTION__);
			}
		break;

		case NF_SYSMAN_APP_QC_TEST_FAN :

			if (nf_sysman_qc_auto_test_fan()) {
				printf("\e[32m[%s] APP QC FAN TEST OK !!\e[0m\n", __FUNCTION__);
			} else {
				printf("\e[32m[%s] APP QC FAN TEST FAIL !!\e[0m\n", __FUNCTION__);
			}
		break;

		case NF_SYSMAN_APP_QC_TEST_TEMPER :

			if (nf_sysman_qc_auto_test_temper()) {
				printf("\e[32m[%s] APP QC TEMPER TEST OK !!\e[0m\n", __FUNCTION__);
			} else {
				printf("\e[32m[%s] APP QC TEMPER TEST FAIL !!\e[0m\n", __FUNCTION__);
			}
		break;

		case NF_SYSMAN_APP_QC_TEST_HDD :
			printf(" APP QC HDD NUM = %d\n", nf_sysman_hdd_num_check());
		break;

		case NF_SYSMAN_APP_QC_TEST_ODD :
			printf(" APP QC ODD NUM = %d\n", nf_sysman_odd_num_check());
		break;
	}

	return 0;
}
__commandlist(app_qc_test, "app_qc_test", app_qc_test_help, app_qc_test_help);
#endif



#if defined(SCHIP_COPY_PROTECTION)
void nf_sysman_qc_get_msg(int sock, char *str)
{
	char msg[30];
	int msg_len;

	memset(msg, 0x00, sizeof(msg));
	msg_len = read(sock, msg, sizeof(msg));
	if (msg_len == -1) {
		printf("\e[33m [%s] Accept Fail\e[0m\n", __FUNCTION__);
	}

	strncpy(str, msg, sizeof(msg));
	printf("\e[33m [%s] msg = %s msg_len = %d\e[0m\n", __FUNCTION__, str, msg_len);
}

void nf_sysman_qc_send_msg(int clnt_sock, char *str)
{
	char msg[30];
	int msg_len;

	memset(msg, 0x00, sizeof(msg));
	strncpy(msg, str, sizeof(msg));
	msg_len = write(clnt_sock, msg, sizeof(msg));
	if (msg_len == -1) {
		printf("\e[33m [%s] Write Fail\e[0m\n", __FUNCTION__);
	}
	printf("\e[33m [%s] msg = %s msg_len = %d msg_size = %d\e[0m\n", __FUNCTION__, str, msg_len, sizeof(msg));

}

#define NF_SYSMAN_QC_PD_ERROR_MSG_NO_USB            "NO USB"
#define NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATADIR        "NO DATADIR"
#define NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATA           "NO DATA"
#define NF_SYSMAN_QC_PD_ERROR_MSG_DUPLICATION       "DUPLICATION DATA"
#define NF_SYSMAN_QC_PD_ERROR_MSG_WRONG_DATA        "WRONG DATA"

guint nf_sysman_qc_check_error_msg(char *msg)
{
	int ret = NF_SYSMAN_QC_PD_ERROR_OK;

	if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_NO_USB, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_NO_USB)) == 0) {
		ret = NF_SYSMAN_QC_PD_ERROR_NO_USB;
	} else if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATADIR, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATADIR)) == 0) {
		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATADIR;
	} else if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATA, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_NO_DATA)) == 0) {
		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATA;
	} else if (strncmp(msg, NF_SYSMAN_QC_PD_ERROR_MSG_DUPLICATION, sizeof(NF_SYSMAN_QC_PD_ERROR_MSG_DUPLICATION)) == 0) {
		ret = NF_SYSMAN_QC_PD_ERROR_DUPLICATION;
	}

	printf("\e[33m [%s] msg = %s ret = %d\e[0m\n", __FUNCTION__, msg, ret);
	return ret;
}

guint nf_sysman_qc_client(void)
{
	struct sockaddr_in serv_addr;
	char server_name[128];
	char msg[30], cmd[512], hw_info[30], data_name[30], dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE];
	int sock, ret = -1, msg_len, data_size;
	char *mac_addr;
	NF_HW_PARAM_ENC_DATA data;
	ITX_PROTECT_DATA_PARAM data_param;
	NF_PARAM_HW hw_param;
	FILE *fp = NULL;

	printf("\e[33m [%s] Called \e[0m\n", __FUNCTION__);
	memset(server_name, 0x00, sizeof(server_name));

	nf_sysman_qc_get_ntp_serverip(server_name);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_name);
	serv_addr.sin_port = htons(8090);

	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
		perror("connect");
		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
		return NF_SYSMAN_QC_PD_ERROR_NETWORK;
	}

	/* Make Data Folder Dir */
	memset(hw_info, 0x00, sizeof(hw_info));
	#if defined(_UTM7G_1648D)
		strcpy(hw_info, "UTM7GN-16");
	#elif defined(_UTM7G_0824D)
		strcpy(hw_info, "UTM7GN-08");
	#elif defined(_UTM7G_0412D)
		strcpy(hw_info, "UTM7GN-04");
	#elif defined(_ANF8G_1648D)
		strcpy(hw_info, "ANF8GN-16");
	#else
		strcpy(hw_info, "_IPX_32P5");
	#endif

	nf_sysman_qc_send_msg(sock, hw_info);

	/* Get Mac Data Name */
	nf_sysman_qc_get_msg(sock, msg);

	ret = nf_sysman_qc_check_error_msg(msg);
	if (ret != NF_SYSMAN_QC_PD_ERROR_OK) {
		printf("\e[33m [%s] ret = %d\e[0m\n", __FUNCTION__, ret);
		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
		return ret;
	}

	nf_sysman_qc_send_msg(sock, "WRITING START");

	/* Copy Mac Data from Server */
	memset(cmd, 0x00, sizeof(cmd));
	snprintf(cmd, sizeof(cmd) - 1, "sshpass -pitxutm3398 scp -o StrictHostKeyChecking=no -P 8822 root@%s:/mnt/qcusb/%s/%s /tmp/", 
				server_name, hw_info, msg);
	printf("\e[33m [%s] cmd = %s\e[0m\n", __FUNCTION__, cmd);
	proxy_system(cmd, 1, 3);

	/* Write Mac Data & HW Param */
	memset(data_name, 0x00, sizeof(data_name));
	strncpy(data_name, msg, sizeof(msg));
	snprintf(cmd, sizeof(cmd) - 1, "/tmp/%s", data_name);
	fp = fopen(cmd, "r");
	if (fp == NULL) {
		printf("\e[33m [%s] %s Open Fail\e[0m\n", __FUNCTION__, cmd);
		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
		return NF_SYSMAN_QC_PD_ERROR_ETC;
	}

	memset(dataBuf, 0x00, sizeof(dataBuf));
	fseek(fp, 0, SEEK_END);
	data_size = ftell(fp);
	rewind(fp);
	if (data_size != fread(dataBuf, 1, data_size, fp)) {
		printf("\e[33m [%s] /tmp/%s Read Fail\e[0m\n", __FUNCTION__, data_name);
		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
		return NF_SYSMAN_QC_PD_ERROR_ETC;
	}

	memset(&hw_param, 0x00, sizeof(hw_param));
	nf_api_param_hw_get_protect(&hw_param, 0);
	if (strncmp(hw_param.magic, NF_HW_PARAM_MAGIC, 4) != 0) {
		printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
		ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
		if (!ret) {
			printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
		}
	} else {
		nf_api_param_hw_set(&hw_param);
	}

	memset(&data, 0x00, sizeof(data));
	memcpy(&data.enc_data, dataBuf, sizeof(data.enc_data));
	nf_sysman_set_protect_data(&data);

	/* Read Data From Nand */
	memset(&data, 0x00, sizeof(data));
	if (nf_sysman_get_protect_data(&data) == FALSE) {
		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
		return NF_SYSMAN_QC_PD_ERROR_ETC;
	}

	memset(&data_param, 0x00, sizeof(data_param));
	memcpy(&data_param, &data.dec_data, sizeof(data_param));

	/* Check Data Parameters */
	if (nf_sysman_check_protect_data_param(&data_param, data_name) == FALSE) {
		printf("\e[33m [%s] Check Protect Data Fail\e[0m\n", __FUNCTION__);
		nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
		return NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
	}

	nf_sysman_qc_send_msg(sock, "WRITING DONE");

	/* Send Msg to Notify Success */
	nf_sysman_qc_send_msg(sock, data_name);

	/* Check Error To Del Data */
	nf_sysman_qc_get_msg(sock, msg);
	if (strncmp(msg, "SUCCESS", sizeof("SUCCESS")) != 0) {
		memset(&hw_param, 0x00, sizeof(hw_param));
		nf_api_param_hw_get_protect(&hw_param, 0);
		if (strncmp(hw_param.magic, NF_HW_PARAM_MAGIC, 4) != 0) {
			printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
			ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
			if (!ret) {
				printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
			}
		} else {
			nf_api_param_hw_set(&hw_param);
		}
	}

	fclose(fp);
	close(sock);

	return NF_SYSMAN_QC_PD_ERROR_OK;
}

// MAC WRITER FUNCTION
guint nf_sysman_qc_init_stpacketdata(STPacketData *packet)
{
	packet->cCode = -1;
	packet->nParam1 = 0;
	packet->nParam2 = 0;
	memset(packet->cParam100,0,100);
	memset(packet->cParam1000,0,1000);
}
void nf_sysman_qc_send_mac_msg(int clnt_sock, STPacketData *packet)
{
	STPacketData sendPacket;
	int packet_len;

	memset(&sendPacket, 0x00, sizeof(sendPacket));

	sendPacket.cCode = packet->cCode;
	sendPacket.nParam1 = packet->nParam1;
	sendPacket.nParam2 = packet->nParam2;
	strncpy(sendPacket.cParam1000 , packet->cParam1000, sizeof(packet->cParam1000));
	strncpy(sendPacket.cParam100 , packet->cParam100, sizeof(packet->cParam100));

	packet_len = write(clnt_sock, &sendPacket, sizeof(sendPacket));

	if (packet_len == -1) {
		printf("\e[33m [%s] Write Fail\e[0m\n", __FUNCTION__);
	}
	//BIN DEBUG
	//printf("\e[33m [%s] cmd = %d msg_len = %d msg_size = %d\e[0m\n", __FUNCTION__, sendPacket.cCode, packet_len, sizeof(sendPacket));
}
guint nf_sysman_qc_get_mac_msg(int clnt_sock, STPacketData *packet)
{
	STPacketData recvPacket;
	int packet_len;

	memset(&recvPacket, 0x00, sizeof(recvPacket));

	packet_len = read(clnt_sock, &recvPacket, sizeof(recvPacket));

	if (packet_len == -1) {
		printf("\e[33m [%s] Accept Fail\e[0m\n", __FUNCTION__);
		return -1;
	}

	packet->cCode = recvPacket.cCode;
	packet->nParam1 = recvPacket.nParam1;
	packet->nParam2 = recvPacket.nParam2;
	strncpy(packet->cParam1000, recvPacket.cParam1000 , sizeof(packet->cParam1000));
	strncpy(packet->cParam100, recvPacket.cParam100, sizeof(packet->cParam100));

	//BIN DEBUG
	//printf("\e[33m [%s] cmd = %d msg1 = %s msg2 = %s msg_len = %d\e[0m\n", __FUNCTION__, recvPacket.cCode, recvPacket.cParam100, recv     Packet.cParam1000 ,packet_len);

	return packet_len;
}
guint nf_sysman_qc_check_error_mac_msg(STPacketData *packet)
{
	int ret = NF_SYSMAN_QC_PD_ERROR_OK;
	char msg[256] = {0,};
	if(packet->nParam1 == INSPECTOR_NOTPRODUCTINFO)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
		strcpy(msg,"INSPECTOR_NOTPRODUCTINFO");
	}
	else if(packet->nParam1 == INSPECTOR_NOTSERVERCONNECT)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
		strcpy(msg,"INSPECTOR_NOTSERVERCONNECT");
	}
	else if(packet->nParam1 == INSPECTOR_NOMACFILE)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATA;
		strcpy(msg,"INSPECTOR_NOMACFILE");
	}
	else if(packet->nParam1 == INSPECTOR_SERVER_MACFILE_LOADERROR)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATADIR;
		strcpy(msg,"INSPECTOR_SERVER_MACFILE_LOADERROR");
	}
	else if(packet->nParam1 == INSPECTOR_CLIENT_MACFILE_LOADERROR)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATADIR;
		strcpy(msg,"INSPECTOR_CLIENT_MACFILE_LOADERROR");
	}
	else{
		strcpy(msg,"INSPECTOR_SUCCESS");
	}
	printf("\e[33m [%s] msg = %s ret = %d\e[0m\n", __FUNCTION__, msg, ret);
	return ret;
}
#define BUF_SIZE 1024
gboolean nf_sysman_qc_get_mac_data(int clnt_sock, char *fPath)
{
	FILE *fp = NULL;
	int nSendData = 0;              // success : 1, fail : 0
	STPacketData sendPackatData;

	char buf[BUF_SIZE];
	int nRet = 0;
	int fileSize = 0;
	unsigned int total_received =0;
	unsigned int readCnt= 0;


	fp = fopen(fPath, "w+");
	if (fp == NULL) {
		printf("\e[33m [%s] %s Open Fail\e[0m\n", __FUNCTION__, fPath);
		nSendData = 0;
		sendPackatData.cCode = SetTC_MACFILEPREPRARE;
		sendPackatData.nParam1 = nSendData;
		nf_sysman_qc_send_mac_msg(clnt_sock, &sendPackatData);
		return FALSE;
	}
	else
	{
		nSendData = 1;
		sendPackatData.cCode = SetTC_MACFILEPREPRARE;
		sendPackatData.nParam1 = nSendData;
		nf_sysman_qc_send_mac_msg(clnt_sock, &sendPackatData);
	}

	readCnt = read(clnt_sock, (char*)&nRet, sizeof(int));
	if(nRet == -1)
	{
		printf("\e[33m [%s] File access Falied\e[0m\n", __FUNCTION__);
		return FALSE;
	}

	while(1)
	{
		readCnt = read(clnt_sock, buf, BUF_SIZE);
		if( readCnt <= 0)
			break;
		total_received += readCnt;
		if(fileSize == 0)
		{
			memcpy(&fileSize, buf, sizeof(int));
			if(fileSize == -1)
			{
				break;
			}
			fwrite(buf + 4,1, readCnt - 4,fp);
		}
		else
			fwrite(buf ,1, readCnt,fp);
		if( total_received == fileSize + sizeof(int))
			break;
	}
	fclose(fp);
	if(readCnt <= 0 || fileSize <= 0)
		return FALSE;
	else if( total_received == fileSize + sizeof(int))
		return TRUE;
	else
		return FALSE;
}

void nf_sysman_set_vcode(char *param)
{
	VCODE_IDX idx;
	if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "ITX") == 0)             idx = IDX_ITX;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "GPS") == 0)            idx = IDX_GPS;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "VIDECON") == 0)        idx = IDX_VDC;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "ORION") == 0)          idx = IDX_ORI;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "CBC") == 0)            idx = IDX_CBC;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "TAKENAKA") == 0)       idx = IDX_TKK;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "G4S") == 0)            idx = IDX_G4S;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "VICON") == 0)          idx = IDX_VCN;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "HONEYWELL") == 0)      idx = IDX_HON;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "VITEK") == 0)          idx = IDX_VIT;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "ASP") == 0)            idx = IDX_ASP;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "I3DVR") == 0)          idx = IDX_I3D;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "CYTE") == 0)           idx = IDX_CYT;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "DAYOU") == 0)          idx = IDX_DAU;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "KB_DEVICE") == 0)      idx = IDX_KBD;
	else if (strcmp(nf_sysdb_get_str_nocopy("sys.info.vendor"), "eneo") == 0)           idx = IDX_VTR;
	else
		idx = IDX_NONE;
	if(idx == IDX_NONE) return;

	strcat(param, "&");
	strcat(param, strVcode[idx-1]);
}

// MAC WRITER END
guint nf_sysman_qc_pc_client(void)
{
	struct sockaddr_in serv_addr;
	char server_name[128], hw_info[30], dataBuf[NAND_LARGE_BLOCK_SIZE_PAGE],  macFilePath[30] ,data_name[30];
	int sock, data_size = 0, ret = -1;

	NF_HW_PARAM_ENC_DATA data;
	ITX_PROTECT_DATA_PARAM data_param;
	NF_PARAM_HW hw_param;
	FILE *fp = NULL;

	int nPacketSize = sizeof(STPacketData);
	int received = 0;

	STPacketData sendPacket;
	STPacketData recvPacket;

	nf_sysman_qc_init_stpacketdata(&sendPacket);
	nf_sysman_qc_init_stpacketdata(&recvPacket);

	printf("\e[33m [%s] Called \e[0m\n", __FUNCTION__);

	memset(server_name, 0x00, sizeof(server_name));
	nf_sysman_qc_get_ntp_serverip(server_name);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_name);
	serv_addr.sin_port = htons(10252);
	printf("\e[33m [%s] [ServerIP : %s]\e[0m\n", __FUNCTION__,server_name);
	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
		perror("connect");
		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
		ret =  NF_SYSMAN_QC_PD_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}

	/* Make Data Folder Dir */
	#if defined(_UTM7G_1648D)
		strcpy(hw_info, "UTM7GN-16");
		strcpy(sendPacket.cParam1000, "UTM7GN-16");
	#elif defined(_UTM7G_0824D)
		strcpy(hw_info, "UTM7GN-08");
		strcpy(sendPacket.cParam1000, "UTM7GN-08");
	#elif defined(_UTM7G_0412D)
		strcpy(hw_info, "UTM7GN-04");
		strcpy(sendPacket.cParam1000, "UTM7GN-04");
	#elif defined(_ANF8G_1648D)
		strcpy(hw_info, "ANF8GN-16");
		strcpy(sendPacket.cParam1000, "ANF8GN-16");
	#elif defined(_IPX_32P5)
		strcpy(hw_info, "IPXP5-32");
		strcpy(sendPacket.cParam1000, "IPXP5-32");	
	#endif
	nf_sysman_set_vcode(sendPacket.cParam1000);
	/*Send hw_info to Server*/
	sendPacket.cCode = SetTC_SETINFO;
	nf_sysman_qc_send_mac_msg(sock,&sendPacket);

	/* Get Mac Data Name */
	received = nf_sysman_qc_get_mac_msg(sock, &recvPacket);
	if(received <= 0 || received != nPacketSize)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}
	if(recvPacket.cCode != CTSet_PREPAREINFO)
	{
		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}
	ret = nf_sysman_qc_check_error_mac_msg(&recvPacket);
	if (ret != NF_SYSMAN_QC_PD_ERROR_OK) {
		printf("\e[33m [%s] ret = %d\e[0m\n", __FUNCTION__, ret);
		sendPacket.cCode = SetTC_ISMACSUCCESS;
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		goto FAIL_CLOSE;
	}

	/*Rquesting Mac Data from Server*/
	memset(&sendPacket, 0, sizeof(STPacketData));
	sendPacket.cCode = SetTC_REQUESTMAC;
	nf_sysman_qc_send_mac_msg(sock,&sendPacket);

	memset(&sendPacket, 0, sizeof(STPacketData));
	sendPacket.cCode = SetTC_ISMACSUCCESS;

	/* Copy Mac Data from Server */
	memset(&recvPacket, 0, sizeof(STPacketData));
	received = nf_sysman_qc_get_mac_msg(sock, &recvPacket);
	if(received <= 0 || received != nPacketSize)
	{
		printf("\e[33m [%s] Data receive Failed\e[0m\n", __FUNCTION__);
		ret = NF_SYSMAN_QC_PD_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}
	if(recvPacket.cCode != CTSet_SEND_MAC)
	{
		printf("\e[33m [%s] MacDataName receive Failed\e[0m\n", __FUNCTION__);
		ret = NF_SYSMAN_QC_PD_ERROR_NO_DATA;
		goto FAIL_CLOSE;
	}

	memset(data_name, 0x00, sizeof(data_name));
	strncpy(data_name, recvPacket.cParam100, sizeof(data_name));
	snprintf(macFilePath, sizeof(macFilePath) - 1, "/tmp/%s", data_name);
	printf("\e[33m [%s] MAC = %s\e[0m\n", __FUNCTION__,data_name);
	if(!(nf_sysman_qc_get_mac_data(sock,macFilePath)))
	{
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		printf("\e[33m [%s] Fail that Get Mac data\e[0m\n", __FUNCTION__);
		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
		goto FAIL_CLOSE;
	}

	/* Write Mac Data & HW Param */
	fp = fopen(macFilePath, "r");
	if (fp == NULL) {
		printf("\e[33m [%s] %s Open Fail\e[0m\n", __FUNCTION__, macFilePath);
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
		goto FAIL_CLOSE;
	}

	memset(dataBuf, 0x00, sizeof(dataBuf));
	fseek(fp, 0, SEEK_END);
	data_size = ftell(fp);
	rewind(fp);
	if (data_size != fread(dataBuf, 1, data_size, fp)) {
		printf("\e[33m [%s] /tmp/%s Read Fail\e[0m\n", __FUNCTION__, data_name);
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
		goto FAIL_CLOSE;
	}

	memset(&hw_param, 0x00, sizeof(hw_param));
	nf_api_param_hw_get_protect(&hw_param, 0);
	if (strncmp(hw_param.magic, NF_HW_PARAM_MAGIC, 4) != 0) {
		printf("\e[33m [%s] Get HW PARAM MAGIC Fail\e[0m\n", __FUNCTION__);
		ret = nf_flash_erase(NF_FLASH_PONG_HW_PARAM_MTD_NUM, NULL, NULL);
		if (!ret) {
			printf("\e[33m [%s] HW Param Nand Erase Fail\e[0m\n", __FUNCTION__);
		}
	} else {
		nf_api_param_hw_set(&hw_param);
	}

	memset(&data, 0x00, sizeof(data));
	memcpy(&data.enc_data, dataBuf, sizeof(data.enc_data));
	nf_sysman_set_protect_data(&data);

	/* Read Data From Nand */
	memset(&data, 0x00, sizeof(data));
	if (nf_sysman_get_protect_data(&data) == FALSE) {
		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
		//nf_sysman_qc_send_msg(sock, "CLOSE CONNECT");
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		ret = NF_SYSMAN_QC_PD_ERROR_ETC;
		goto FAIL_CLOSE;
	}

	memset(&data_param, 0x00, sizeof(data_param));
	memcpy(&data_param, &data.dec_data, sizeof(data_param));

	/* Check Data Parameters */
	if (nf_sysman_check_protect_data_param(&data_param, data_name) == FALSE) {
		printf("\e[33m [%s] Check Protect Data Fail\e[0m\n", __FUNCTION__);
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		ret = NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
		goto FAIL_CLOSE;
	}

	/* Send Msg to Notify Success */
	printf("\e[33m [%s] Success To Write mac data\e[0m\n", __FUNCTION__);
	sendPacket.nParam1 = 1;
	nf_sysman_qc_send_mac_msg(sock,&sendPacket);
	ret = NF_SYSMAN_QC_PD_ERROR_OK;

	FAIL_CLOSE:
	{
		if(sock > 0)
			close(sock);
		if(fp != NULL)
			close(fp);
		return ret;
	}
}
/*********************************
QC LICENSE
*********************************/
int macNameSet(char* descMac, char *srcMac, int mode )
{
	int idx = 0;
	int ret = 0;
	printf("\e[33m [%s] MAC %s\e[0m\n", __FUNCTION__,srcMac);
	if(srcMac == NULL || descMac == NULL)
	{
		printf("\e[33m [%s] MAC FAIL %s\e[0m\n", __FUNCTION__);
		return ret;
	}
	if(mode == 0)
	{
	while(srcMac[idx] != '.')
	{
		if(srcMac[idx] == '-')
			descMac[idx] = ':';
		else
			descMac[idx] = srcMac[idx];
		idx++;
	}
	}
	else if(mode == 1)
	{
		int descIdx = 0;
		while(srcMac[idx] != '\0')
		{
			if(srcMac[idx] != ':'){
				descMac[descIdx] = srcMac[idx];
				descIdx++;
			}
			idx++;
		}
		descMac[descIdx] = '\0';
		printf("\e[33m [%s] Mode 1 MAC %s\e[0m\n", __FUNCTION__,descMac);
	}
	return 1;
}
#define LICMAXCNT 16
#define LICMEMBERCNT 3
#define MAXPARAM1000 1000
static int nf_sysman_qc_lic_parse(int licCnt, char *param, char *mac,int product, LicData* arrLicData)
{
	char tmpParam[MAXPARAM1000] = {0,};
	char* tmpLicData[LICMAXCNT];
	int licIdx = 0 ,licMemIdx = 0, ret = 0;
	char *ptr;
	char *rbuf;

	printf("\e[33m [%s] licCnt [%d] \e[0m\n", __FUNCTION__,licCnt);

	memcpy(tmpParam, param, MAXPARAM1000);
	for(licIdx =0; licIdx < licCnt; licIdx++)
	{
		if(licIdx == 0)
			tmpLicData[licIdx] = strtok_r( tmpParam, "\/", &rbuf);
		else
			tmpLicData[licIdx] = strtok_r( NULL , "\/", &rbuf);
		if(tmpLicData[licIdx] == NULL)
		{
			ret = NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
			goto PASEFAIL;
		}
		#if defined(DEBUG_SYSMAN_QC_LIC)
		printf("\e[33m [%s] [%d] %s \e[0m\n", __FUNCTION__,licIdx,tmpLicData[licIdx]);
		#endif
	}

	for(licIdx =0; licIdx < licCnt; licIdx++)
	{
		for(licMemIdx=0; licMemIdx < LICMEMBERCNT; licMemIdx++)
		{
			if(licMemIdx == 0)
				ptr = strtok_r(tmpLicData[licIdx], "\^", &rbuf);
			else
				ptr = strtok_r(NULL, "\^", &rbuf);
			if(tmpLicData[licIdx] == NULL)
			{
				goto PASEFAIL;
			}
			switch(licMemIdx)
			{
				case 0:
					memcpy(arrLicData[licIdx].mac,ptr,strlen(ptr));
					#if defined(DEBUG_SYSMAN_QC_LIC)
					printf("\e[33m [%s] arrLicData[%d] MAC %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].mac);
					#endif
					break;
				case 1:
					arrLicData[licIdx].product = atoi(ptr);
					#if defined(DEBUG_SYSMAN_QC_LIC)
					printf("\e[33m [%s] arrLicData[%d] PRODUCT %d \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].product);
					#endif
					break;
				case 2:
					memcpy(arrLicData[licIdx].lic,ptr,strlen(ptr));
					#if defined(DEBUG_SYSMAN_QC_LIC)
					printf("\e[33m [%s] arrLicData[%d] LIC %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].lic);
					#endif
					break;
			}
		}
		if(strncmp(mac,arrLicData[licIdx].mac,sizeof(arrLicData[licIdx].mac)) == 0 && arrLicData[licIdx].product == product)
			printf("\e[33m [%s] [%d] valid lic : %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].lic);
		else
		{
			printf("\e[33m [%s] [%d] invalid lic : %s \e[0m\n", __FUNCTION__,licIdx,arrLicData[licIdx].lic);
			ret = NF_SYSMAN_QC_PD_ERROR_WRONG_DATA;
			goto PASEFAIL;
		}
	}
	PASEFAIL:
	{
		return ret;
	}
}
static int nf_sysman_qc_lic_set_db(int licCnt, LicData* arrLicData)
{
	int ret = 0;
	int licIdx = 0;
	time_t the_time_utc;
	time(&the_time_utc);
	GValue set_value = {0,};

	nf_sysdb_default( "sys" );

	nf_sysdb_lock(NF_SYSDB_CATE_SYS);
	char cmd[128]={0,};
	for (licIdx = 0; licIdx < licCnt; licIdx++)
	{
		g_value_init(&set_value, G_TYPE_STRING);
		g_value_set_string(&set_value,arrLicData[licIdx].lic);
		if(!nf_sysdb_set_key1("sys.lic.L%d.key", licIdx, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
			printf("\e[33m [%s] FAILED TO SET DB [sys.lic.L%d.key] \e[0m\n", __FUNCTION__,licIdx);
			ret = NF_SYSMAN_QC_PD_ERROR_SET_DB;
			goto LICSETDBEND;
		}
		g_value_unset(&set_value);

		sprintf(cmd,"sys.lic.L%d.key",licIdx);
		printf("\e[33m [%s] [sys.lic.L%d.key] : %s \e[0m\n", __FUNCTION__,licIdx,nf_sysdb_get_str(cmd));

		g_value_init(&set_value, G_TYPE_UINT);
		g_value_set_uint(&set_value, the_time_utc);
		if(!nf_sysdb_set_key1("sys.lic.L%d.acquired_date", licIdx, &set_value, NULL))
		{
			nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
			printf("\e[33m [%s] FAILED TO SET DB [sys.lic.L%d.acquired_date] \e[0m\n", __FUNCTION__,licIdx);
			ret = NF_SYSMAN_QC_PD_ERROR_SET_DB;
			goto LICSETDBEND;
		}
		g_value_unset(&set_value);

		sprintf(cmd,"sys.lic.L%d.acquired_date",licIdx);
		printf("\e[33m [%s] [sys.lic.L%d.acquired_date] : %d \e[0m\n", __FUNCTION__,licIdx,nf_sysdb_get_uint(cmd));
	}

	g_value_init(&set_value, G_TYPE_UINT);
	g_value_set_uint(&set_value, licCnt);
	if(!nf_sysdb_set_key0("sys.lic.key_count", &set_value, NULL))
	{
		nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
		ret = NF_SYSMAN_QC_PD_ERROR_SET_DB;
		goto LICSETDBEND;
	}
	g_value_unset(&set_value);

	nf_notify_fire_params("sysdb_change", NF_SYSDB_CATE_SYS, 0, 0, 0);

	nf_sysdb_unlock(NF_SYSDB_CATE_SYS);
	nf_sysdb_save( "sys" );
	nf_sysdb_save_flush();
	LICSETDBEND:
	{
		return ret;
	}
}

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
static int nf_sysman_cp_key_data(char (*dstData)[36], LicData *srcData, int cnt)
{
	int idx = 0;
	if(dstData == NULL || srcData == NULL || cnt <= 0)
		return 0;
	printf("\e[33m [%s] Start COPY \e[0m\n", __FUNCTION__,dstData[idx], srcData[idx].lic,idx);
	for(idx=0; idx<cnt; idx++)
	{
		if(!strncpy(dstData[idx],srcData[idx].lic,36))
		{
			printf("\e[33m [%s] FAIL strncpy \e[0m\n", __FUNCTION__);
			return 0;
		}
		printf("\e[33m [%s] dstData[%d] : %s  srcData[%d].lic : %s \e[0m\n", __FUNCTION__,idx,dstData[idx], idx, srcData[idx].lic);
	}
	return 1;
}

#define DEBUG_LIC_MAC "00:11:5f:b3:07:00"
guint nf_sysman_qc_lic_write(char key[][36])
{
	struct sockaddr_in serv_addr;
	int sock, data_size = 0, ret = -1;

	int nPacketSize = sizeof(STPacketData);
	int received = 0;
	char cmpMac[NF_HW_PARAM_STR_32] = {0,}, ip[128] = {0,};
	int licCnt = 0;
	STPacketData sendPacket;
	STPacketData recvPacket;
	STPacketData packet;

	NF_PARAM_HW hw_param;
	NF_HW_PARAM_ENC_DATA data;
	LicData arrLicData[LICMAXCNT] = {0,};
	#if defined(DEBUG_SYSMAN_QC_LIC)
	printf("\e[33m [%s] START \e[0m\n", __FUNCTION__);
	#endif
	memset(&serv_addr, 0, sizeof(serv_addr));
	nf_sysman_qc_get_ntp_serverip(ip);
	printf("\e[33m [%s] nf_sysman_qc_get_ntp_serverip %s \e[0m\n", __FUNCTION__, ip);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(10252);
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == -1) {
		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
	}

	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
		ret =  NF_SYSMAN_QC_LIC_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}

	nf_sysman_qc_init_stpacketdata(&sendPacket);
	nf_sysman_qc_init_stpacketdata(&recvPacket);
	/***************************
	SEND REQUEST LIC MSG
	***************************/
	#if defined(DEBUG_SYSMAN_QC_LIC)
	printf("\e[33m [%s] SEND REQUEST LIC MSG \e[0m\n", __FUNCTION__);
	#endif

	memset(&data, 0x00, sizeof(NF_HW_PARAM_ENC_DATA));
	if (nf_sysman_get_protect_data(&data) == FALSE)
	{
		printf("\e[33m [%s] Get Protect Data Fail\e[0m\n", __FUNCTION__);
		sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		ret = NF_SYSMAN_QC_LIC_ERROR_MAC_DATA;
		goto FAIL_CLOSE;
	}
	memset(&hw_param, 0x00, sizeof(hw_param));
	memcpy(&hw_param, &data.dec_data, sizeof(hw_param));
	nf_api_param_hw_print(&hw_param);
	#if defined(DEBUG_SYSMAN_QC_LIC_MAC)
	memcpy(hw_param.eth_addr,DEBUG_LIC_MAC,sizeof(hw_param.eth_addr));
	#endif
	macNameSet(cmpMac,hw_param.eth_addr, 1);

	strncpy(sendPacket.cParam100,cmpMac,NF_HW_PARAM_STR_32);
	sendPacket.cCode = SetTC_REQUESTSERIAL;
	nf_sysman_qc_send_mac_msg(sock,&sendPacket);
	/***************************/

	/***************************
	RECV REQUEST LIC MSG
	***************************/
	#if defined(DEBUG_SYSMAN_QC_LIC)
	printf("\e[33m [%s] RECV REQUEST LIC MSG\e[0m\n", __FUNCTION__);
	#endif
	received = nf_sysman_qc_get_mac_msg(sock, &recvPacket);
	if(received <= 0 || received != nPacketSize)
	{
		ret = NF_SYSMAN_QC_LIC_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}

	nf_sysman_qc_init_stpacketdata(&packet);
	memcpy(&packet, &recvPacket, sizeof(STPacketData));
	if(packet.cCode != CTSet_SEND_SERIAL)
	{
		ret = NF_SYSMAN_QC_LIC_ERROR_NETWORK;
		goto FAIL_CLOSE;
	}
	if(packet.nParam1 == INSPECTOR_NOTSERIALINFO)
	{
		ret = 0;
		goto FAIL_CLOSE;
	}
	/**************************/

	/***************************
	SET LICENSE
	***************************/
	#if defined(DEBUG_SYSMAN_QC_LIC)
	printf("\e[33m [%s] SET LICENSE \e[0m\n", __FUNCTION__);
	printf("\e[33m [%s] LIC CNT %d \e[0m\n", __FUNCTION__,packet.nParam2);
	#endif
	licCnt = packet.nParam2;
	#if defined(DEBUG_SYSMAN_QC_LIC_MAC)
	printf("\e[33m [%s] PRODUCT %d \e[0m\n", __FUNCTION__,nf_api_param_fwver_get_product());
	if(ret = nf_sysman_qc_lic_parse(licCnt,packet.cParam1000,cmpMac,66000,arrLicData))
	#else
	printf("\e[33m [%s] PRODUCT %d \e[0m\n", __FUNCTION__,nf_api_param_fwver_get_product());
	if(ret = nf_sysman_qc_lic_parse(licCnt,packet.cParam1000,cmpMac,nf_api_param_fwver_get_product(),arrLicData))
	#endif
	{
		ret = NF_SYSMAN_QC_LIC_ERROR_WRONG_DATA;
		goto FAIL_CLOSE;
	}
	#if 0
	if(ret = nf_sysman_qc_lic_set_db(licCnt,arrLicData))
	{
		ret = NF_SYSMAN_QC_LIC_ERROR_SET_DB;
		goto FAIL_CLOSE;    
	}
	#endif
	/**************************/
	ret = licCnt;
	#if 0
	if(!nf_sysman_malloc_free_key(1,licCnt,key))
	{
		printf("\e[33m [%s] nf_sysman_malloc_free_key\e[0m\n", __FUNCTION__);
		ret = NF_SYSMAN_QC_LIC_ERROR_MEMORY;
		goto FAIL_CLOSE;    
	}
	#endif
	if(!nf_sysman_cp_key_data(key,arrLicData,licCnt))
	{
		ret = NF_SYSMAN_QC_LIC_ERROR_MEMORY;
		printf("\e[33m [%s] nf_sysman_cp_key_data\e[0m\n", __FUNCTION__);
		goto FAIL_CLOSE;
	}
	/***************************
	SEND RESULT LIC MSG
	***************************/
	FAIL_CLOSE:
	{
		nf_sysman_qc_init_stpacketdata(&sendPacket);
		strncpy(sendPacket.cParam100,cmpMac,NF_HW_PARAM_STR_32);
		sendPacket.cCode = SetTC_ISSERIALSUCCESS;
		if(ret >= 0)
			sendPacket.nParam1 = 1;
		else
			sendPacket.nParam1 = 0;
		nf_sysman_qc_send_mac_msg(sock,&sendPacket);
		if(sock > 0)
			close(sock);
		#if defined(DEBUG_SYSMAN_QC_LIC)
		printf("\e[33m [%s] END\e[0m\n", __FUNCTION__);
		#endif
		return ret;
	}
	/**************************/
}

int scm_get_ntp_time_pc(char *server_name, GTimeVal *time)
{
	STPacketData stPatatData;
	int nPackatSize = sizeof(STPacketData);
	int received;

	struct sockaddr_in serv_addr;
	int sock, data_size = 0, ret = -1;

	STPacketData sendPacket;
	STPacketData recvPacket;

	nf_sysman_qc_init_stpacketdata(&sendPacket);
	nf_sysman_qc_init_stpacketdata(&recvPacket);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		 printf("\e[33m [%s] Create Socket Fail\e[0m\n", __FUNCTION__);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_name);
	serv_addr.sin_port = htons(10252);
	printf("\e[33m [%s] [ServerIP : %s]\e[0m\n", __FUNCTION__,server_name);
	if ((ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) == -1) {
		perror("connect");
		printf("\e[33m [%s] Connect Fail\e[0m\n", __FUNCTION__);
		ret =  NF_SYSMAN_QC_PD_ERROR_NETWORK;
		return -2;
	}

	sendPacket.cCode = SetTC_SETTIME;
	send(sock, (char*)&sendPacket, sizeof(STPacketData), 0);

	received = read(sock, &recvPacket, nPackatSize);
	if(received <= 0 || received != nPackatSize)
	{
		printf("received %d error !!!\n:",received);
		return -3;
	}
	memcpy(&stPatatData, &recvPacket, nPackatSize);
	if(stPatatData.cCode != CTSet_SEND_TIME)
	{
		return -4;
	}

	time->tv_sec  = atoi(stPatatData.cParam100);

	printf("ret->tv_sec %d \n",time->tv_sec);

	if(sock > 0)
		close(sock);

	return (time->tv_sec > 0 ? 0 : -1);
}
#endif

