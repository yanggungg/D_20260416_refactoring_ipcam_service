/** @file	nf_main.c
	@brief	nf host main.c

	@todo
*/

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <execinfo.h>

#include <unistd.h>
#include <gtk/gtk.h>
// #include <gst/gst.h>

#define MODULE_NAME			"NF_HOST"
#define MODULE_VENDOR		"ITX security"
#define DOT					"."

static char *version[] = {
	"$model: " MODULE_NAME " $",
	"$vendor: " MODULE_VENDOR " $",
	//"$version: " RELEASE DOT MAJOR DOT MINOR " $",
	"$buildtime: " __DATE__" "__TIME__ " $"
};

#include "nf_common.h"
#include "nf_debug.h"
#include "nf_watchdog.h"
#include "nf_sysman.h"
#include "nf_sysdb.h"

#include "nf_api_live.h"
#include "nf_api_ipcam.h"
#include "nf_api_param_app.h"
#include "nf_qc.h"
#include "nf_qc_app.h"
#include "nf_api_param_hw.h"
#include "nf_api_param_fwver.h"
#include "nf_util_device.h"
#include "libicmem.h"
#include "jbshell.h"
#include "unp.h"
#include "nf_logevtdef.h"

#if WEBRA1_FOR_WEBRA2
#include "nf_webra.h"
#endif

#include "nf_util_mail.h"
#include "nf_ddc.h"
#ifdef	SUPPORT_VCA_CAMERA
#include "nf_meta_data.h"
#endif	/* SUPPORT_VCA_CAMERA */

#include "nf_auth.h"
#include "webra_host.h"


#if defined(_OTM_MODEL) || defined(_SNF_MODEL)
#include "nf_solo_common.h"
#include "nf_solo_enc.h"
#include "nf_solo_rec.h"
#include "nf_solo_net.h"
#include "nf_solo_aud.h"
#include "nf_solo_jpeg.h"
#endif /* _OTM_MODEL */

#ifdef USE_RTSP4VLC
#include "nf_nvs_common.h"
#endif
#include "nf_issm_ctl.h"

#include "nf_remote_assist_usb.h"
//#define DEBUG_MEMORY_LEAK_CHECK

#ifdef DEBUG_MEMORY_LEAK_CHECK
#include <gperftools/heap-profiler.h>
#endif

// #if defined(_IPX_1648M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1648P4E) || defined(_IPX_0824P4E) || defined(_IPX_1648M4E)|| defined(_IPX_32P4E)
// 	#include "nf_guard.h"
// #endif

#define ADDR2LINE_DEBUG_EN // for debugging..

#if defined(ADDR2LINE_DEBUG_EN)
#define is_digit(x) ((x) >= '0' && (x) <= '9')
#define is_hexalpha(x) (((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))
#define is_hexdigit(x) (is_digit(x) || is_hexalpha(x))
#define to_num(x) (((x) >= '0' && (x) <= '9') ? ((x)-'0') \
					: ((x) >= 'a' && (x) <= 'f') ? ((x)-'a'+10) \
					: ((x) >= 'A' && (x) <= 'F') ? ((x)-'A'+10) : 0)
#endif

#if defined(CHIP_NVT_NT9833x)
	extern gboolean nf_audio_init(void);
#endif
extern gboolean nf_play_init(void);
extern gboolean nf_display_init(void);
extern gboolean nf_netif_init(void);
extern gboolean nf_filesystem_init(void);
extern gboolean nf_filesystem_init_debug(gint force_format);

extern gboolean nf_notify_init(int wait);
extern gboolean nf_action_init(int wait);
extern gboolean nf_event_init(int wait);
extern gboolean nf_record_init(int wait);
extern gboolean nf_rec_audio_init(int wait);

extern gboolean nf_network_init(int wait);
extern gboolean nf_ptz_init(int wait);
extern gboolean nf_keyctrl_init(int wait);
extern gboolean nf_dev_init(void);
extern gboolean nf_zoneinfo_init(void);
extern gboolean nf_jpeg_man_init(int wait);

extern gboolean nf_mail_send_init(int wait);
extern gboolean nf_net_send_init(int wait);

extern int proxy_system(const char *str, int mode, int timeout_sec);

extern int ux_main(int skip_sst_init);
extern void nf_debug_show_registers(struct sigcontext *ctx);
extern gboolean nf_arch_manager_start(void);

extern gboolean nf_admintool_init(void);

extern gboolean nf_edid_init(int wait);

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "nf_main"

//#define DEBUG_FOR	CE_FORMAT
#define DEBUG_JBSHELL
//#define DEBUG_MEMORY_NET
//#define DEBUG_DUMMY_GMAINLOOP
//#define DEBUG_SIMPLE_NF

#define ENABLE_FW_QCMODE

// #define DEBUG_NETIF_STAT
#define DEBUG_REC_GOP_STAT
#define DEBUG_MONITOR_STAT
#define DEBUG_NOTIFY_STAT

#define DEBUG_BOOT_PROFILE  0

GMainLoop *NF_Loop;
GMainContext *NF_Context;

int 	_ui_skip_sst_init = 0;
int 	_ui_argc;
char 	**_ui_argv;

#if DEBUG_BOOT_PROFILE
static struct timespec start;
static struct timespec end;
static double delta;
#endif

#ifdef DEBUG_DUMMY_GMAINLOOP

static void _ui_dummy_main(void *arg)
{
	gtk_init(&_ui_argc, &_ui_argv);
	gtk_main();
}

/**
	@brief	init_gui() gtk_main()�� ������ �����带 �ε��Ѵ�.
	@return	void
*/
static void init_dummy_gui()
{
	GError *error = NULL;
	GThread *thread = NULL;

	thread = g_thread_create ((GThreadFunc) _ui_dummy_main, NULL, 0, &error);

	if (thread == NULL) {
		g_warning ("Error loading image: %s", error->message);
		g_error_free (error);
	}

	g_message("%s done!", __FUNCTION__);
}

#endif

#ifdef DEBUG_JBSHELL
/**
	@brief	init_jbshell()
	@return	void
*/
static void init_jbshell()
{
	GError *error = NULL;
	GThread *thread = NULL;

	thread = g_thread_create ((GThreadFunc)jbshell, NULL, 0, &error);

	if (thread == NULL) {
		g_warning ("Error loading image: %s", error->message);
		g_error_free (error);
	}
}
static void init_jbshell_ipx(int port)
{
	GError *error = NULL;
	GThread *thread = NULL;

	thread = g_thread_create ((GThreadFunc)ipxshell, (gpointer)port, 0, &error);

	if (thread == NULL) {
		g_warning ("Error loading image: %s", error->message);
		g_error_free (error);
	}
}
#endif

static void cpu_core_set(int core_id)
{
       int s=0;
       cpu_set_t cpuset;
       pthread_t thread;

       thread = pthread_self();

       // http://man7.org/linux/man-pages/man3/pthread_setaffinity_np.3.html
       CPU_ZERO(&cpuset);
       CPU_SET(core_id, &cpuset);

       s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
       if (s != 0)
            printf("pthread_setaffinity_np \n");
       /* Check the actual affinity mask assigned to the thread */

       s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
       if (s != 0)
            printf("pthread_getaffinity_np \n");	
}

static int _change_ui_priority()
{
	int policy;
	struct sched_param sched;
	pthread_t thread;
#if 1
	g_print("priority set\n");
	policy = SCHED_FIFO;
	thread = pthread_self();
	sched.sched_priority = sched_get_priority_min(policy)+G_THREAD_PRIORITY_HIGH; //ugie
	g_print("ret = %d\n", pthread_setschedparam (thread, policy, &sched));
	g_print("ret = %d\n", pthread_getschedparam (thread, &policy, &sched));
	g_print("XXX ugienf:ui_main set realtime policy = %d\n", policy);
	
	cpu_core_set(3);
#endif
	return 0;
}

/**
	@brief	_ui_main() main function for UI thread, written by skshin
	@return	void
*/
static int _ui_main()
{
	_change_ui_priority();
	ux_main(_ui_skip_sst_init);
	return 0;
}

/**
	@brief	init_gui() gtk_main()�� ������ �����带 �ε��Ѵ�.
	@return	void
*/
static void init_gui()
{
	GError *error = NULL;
	GThread *thread = NULL;

	thread = g_thread_create ((GThreadFunc) _ui_main, NULL, 0, &error);

	if (thread == NULL) {
		g_warning ("Error loading image: %s", error->message);
		g_error_free (error);
	}

	g_message("%s done!", __FUNCTION__);
}

static const char *_sig_list[] =
{
	"NULL",
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT",
	"SIGBUS",
	"SIGFPE",
	"SIGKILL",
	"SIGUSR1",
	"SIGSEGV",
	"SIGUSR2",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGSTKFLT",
	"SIGCHLD",
	"SIGCONT",
	"SIGSTOP",
	"SIGTSTP",
	"SIGTTIN",
	"SIGTTOU",
	"SIGURG",
	"SIGXCPU",
	"SIGXFSZ",
	"SIGVTALRM",
	"SIGPROF",
	"SIGWINCH",
	"SIGIO",
	"SIGPWR",
	"SIGSYS"
};

#if defined(ADDR2LINE_DEBUG_EN)
static char __working_path[4096];

void __set_working_path(const char *working_path)
{
	if(working_path && access(working_path, F_OK) == 0) {
		strcpy(__working_path, working_path);
	}
}

static uint64_t hexString2Number(char *hex_string)
{
	char *p = hex_string;
	size_t len = 0;
	while (*p) { len++; p++; }
	p = hex_string;
	uint64_t num = 0ULL, nibble;

	//remove the prefix
	if (len >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		p += 2;
	}

	for (; *p && is_hexdigit(*p); p++) {
		nibble = (uint64_t)to_num(*p);
		num = (num << 4) | nibble;
	}

	return num;
}

static int32_t addr2line(char* program_name, int32_t name_length, void const *const addr)
{
	static char addr2line_cmd[512] = {0};
	static char real_path[4096];
	/* have addr2line map the address to the relent line in the code */
	if(program_name && name_length > 2 && program_name[0] == '.' && program_name[1] == '/' && __working_path[0] != 0) {
		sprintf(real_path, "%s/%s", __working_path, program_name+2);
		program_name = real_path;
	}

	if(access("/bin/addr2line", F_OK) == 0) {
		sprintf(addr2line_cmd, "/bin/addr2line -f -p -e %.256s %p", program_name, addr);
	} else if(access("/data/addr2line", F_OK) == 0) {
		sprintf(addr2line_cmd, "/data/addr2line -f -p -e %.256s %p", program_name, addr);
	} else {
		fprintf(stdout, "ksi_test not found addr2line .....\n");
		return -1;
	}
	

	/* This will print a nicely formatted string specifying the
	 function and source line of the address */
	//fprintf(stderr, "%s\n", addr2line_cmd);
	// proxy_system(addr2line_cmd, 1, 3);
	return system(addr2line_cmd);
}
#endif

#define BT_TRACE_SIZE	16

void _nf_signal_handler(int signo, struct siginfo *info, ucontext_t *ctx)
{
	void *trace[BT_TRACE_SIZE];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
 	
 	struct sigcontext *sc = &ctx->uc_mcontext;	

	printf("Got signal %d [%s]\n", signo, 
				(unsigned int) signo < 32 ? _sig_list[signo]:_sig_list[0]);

	// if ( signo ==  6 )
	// 	exit(0);

	nf_debug_show_registers(sc);
	
	memset(trace, 0x00, sizeof(trace));

	trace_size = backtrace(trace, BT_TRACE_SIZE);

	printf ("backtrace stack : ");
	for (i=1; i<BT_TRACE_SIZE && trace[i]; i++)
		printf ("%d(%p) ", i, trace[i]);
		printf ("\n");

	messages = backtrace_symbols(trace, BT_TRACE_SIZE);

#if defined(ADDR2LINE_DEBUG_EN)
	int n;
	char *p, *l;
	for (i = 0; i < trace_size; i++) {
		fprintf(stderr, " %s: ", messages[i]);

		//get the address.
		l = p = messages[i];
		while (*p && *p != '(') p++;
		n = p - l;
		if (*p == '(') *p++ = 0;

		while (*p && *p != '[') p++;

		addr2line(l, n, (void const * const)hexString2Number(p + 1));

	}
#endif
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");	
	for (i=1; i<BT_TRACE_SIZE; ++i)
	{
		gchar string[200];
		snprintf(string, 200, "[bt] [%2d] [0x%08x] %s\n", i, (u_int)trace[i], messages[i]);
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_UI_HOLD_SYSTEM_BACKTRACE, string);	
		printf(string);
	}
	
	printf("[bt] Execution path:\n");	
	exit(0);
}

int init_signal(void)
{

	/* Install our signal handler */
	struct sigaction sa;

	Signal(SIGPIPE, SIG_IGN);

#if 1
	sa.sa_handler = (void *)_nf_signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
#else
	Signal(SIGBUS, _nf_signal_handler);	// 7
	Signal(SIGILL, _nf_signal_handler);	// 4
	Signal(SIGSEGV, _nf_signal_handler);// 11
	Signal(SIGABRT, _nf_signal_handler);// 6
#endif

#if 0
	printf ("==========================================\n");
	printf ("==========================================\n");

	{
		unsigned int *null = (unsigned int *)NULL;
		*null = 0xbeadbead;
	}
#endif
	return 0;
}

unsigned long _CMEM_physp = 0;
unsigned int _CMEM_size = 0;

void itx_assertion(const gchar *string)
{
	if ( g_strrstr(string, "assertion failed") != NULL )
		nf_eventlog_put_param(NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_UI_HOLD_SYSTEM, string);
	printf(string);
}

#include "curl/curl.h"

#ifdef DEBUG_MEMORY_LEAK_CHECK
void* profiler_thread(void* arg) {
    printf("[Profiler Thread] Sleeping 3 minutes before starting measurement...\n");
    sleep(60 * 3);
    
    printf("[Profiler Thread] Starting measurement phase for 12 hours...\n");
    sleep(60 * 60 * 12);
    
    printf("[Profiler Thread] Stopping profiler after 12 hours...\n");
    HeapProfilerStop();
    printf("[Profiler Thread] Heap profiling stopped.\n");
    
    return NULL;
}
#endif

int main( int argc, char *argv[])
{
    gchar ch_arr[32];
    gboolean covert_arr[32];
    gint i = 0;
    NF_DISPLAY_E disp_mode;

	gint micom_up=0;
	
	#ifdef USE_PROXY_SYSTEM
		proxy_system("stty -ixon", PROXY_TYPE_SYSTEM, SCPT_RUN);
	#else
		system("stty -ixon");
	#endif

	#if DEBUG_BOOT_PROFILE
   		clock_gettime(CLOCK_REALTIME, &start);
	#endif

#if defined(ADDR2LINE_DEBUG_EN)
	char cwd[PATH_MAX];
	__set_working_path(getcwd(cwd, sizeof(cwd)));
#endif

	_ui_argc = argc;
	_ui_argv = &*argv;

	g_thread_init (NULL);
	g_type_init();
	nf_openssl_thread_setup();//nf_api_ipcam.c

    initialize_global_curl();

	//assertion handler(g_printerr) was changed to leave NAND log message whenever glib occurs assertion event.
	g_set_printerr_handler(itx_assertion);

    ICMEM_init(&_CMEM_physp, &_CMEM_size);
   	g_message("ICMEM_init _CMEM_physp[0x%lx] _CMEM_size[0x%x]", _CMEM_physp, _CMEM_size);

	init_signal();

	#ifdef ENABLE_FW_QCMODE
		nf_sysman_qcmode_init();
	#endif

    for (i = 0; i < 32; i++) {
        ch_arr[i] = -1;
        covert_arr[i] = TRUE;
    }

    for (i = 0; i < NUM_ACTIVE_CH; i++)
        ch_arr[i] = (char)i;

    if (NUM_ACTIVE_CH == 4) {
        disp_mode = NF_DISPLAY_QUAD;
    } else if(NUM_ACTIVE_CH == 8) {
        disp_mode = NF_DISPLAY_NONA;
    } else if(NUM_ACTIVE_CH == 16) {
        disp_mode = NF_DISPLAY_HEXADECA;
    } else if(NUM_ACTIVE_CH == 32) {
        disp_mode = NF_DISPLAY_HEXATRICONTA;        
    } else {
        g_assert(0);
    }

	if (nf_sysman_qcmode_is_enable() == FALSE) {
		nf_live_init(disp_mode, 0, 0,
					(guint)DISPLAY_ACTIVE_WIDTH,
					(guint)DISPLAY_ACTIVE_HEIGHT,
					ch_arr, (gchar *)covert_arr, 0
					);	
	}

	//unit_test_main();
	NF_Context = g_main_context_new();
	NF_Loop = g_main_loop_new (NF_Context, FALSE);

	{
		gboolean   is_normal_boot = 0;
		if(sizeof(NF_PARAM_APP) != NAND_LARGE_BLOCK_SIZE_PAGE)
		{
			g_warning("App_param Size Wrong!! %d", sizeof(NF_PARAM_APP));
			g_assert(0);
		}
		is_normal_boot = nf_sysman_is_normal_boot();
		g_message("nf_sysman_is_normal_boot [%d]", is_normal_boot );
		nf_sysman_clr_normal_boot();
	}

	// 2008-12-05 ���� 3:18:35
	// �ʱ�ȭ ������ �߿��մϴ�.
	// ���Ƿ� �������� ���� choissi�� �����ϼ���.
	nf_dev_init();
	nf_sysman_hotkey_init();
	nf_sysman_kcmdline_init();

	#ifdef ENABLE_FW_QCMODE
		nf_sysman_qcmode_init();
	#endif

	if (nf_sysman_hotkey_is_temp_upgrade()) {
		gchar buf[128] = { 0, };

		sprintf(buf, "temp_upgrade on!!");
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, buf);

		extern gboolean nf_fw_hotkey_upgrade(void);
		nf_fw_hotkey_upgrade();
	}

	if (nf_sysman_hotkey_is_factory_default()) {
		gchar buf[128] = { 0, };

		sprintf(buf, "factory_default on!!");
		nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, buf);

		g_message("nf_sysman_hotkey_is_factory_default on!!");

		nf_dev_buzzer_on();
		g_usleep(100000);
		nf_dev_buzzer_off();

		nf_sysdb_remove_private();
		nf_sysdb_init(1, NF_SYSDB_INIT_FACTORY_DEFAULT, NULL );

		// onvif_porting
		#ifdef ENABLE_ONVIF_DEVICE
			//onvif_db_factory_init();			// Not Used Function?
		#endif
		// onvif_porting
		nf_sysdb_save_all();
	} else {

		if (nf_sysman_is_fwup_first_boot()) {
			gint ret = 0;
			#if defined(CONFIG_FWUPGRADE_SINGLE)
				char data_path[256] = {0, };
				if (nf_fw_update_verify(data_path) == 0) {
					g_message("[%s] NF_SYSDB_INIT_LOAD_DATA", __FUNCTION__);
					nf_sysdb_init(1, NF_SYSDB_INIT_LOAD_DATA, NULL );
					nf_sysdb_set_int("sys.info.fwup_state", -1);
				} else {
					gint8 cmd[200];
					gint8 path[200];
					
					nf_sysdb_init(1, NF_SYSDB_INIT_FW_UPGRADE, data_path);
					g_message("%s NF_SYSDB_INIT_FW_UPGRADE", __FUNCTION__);
					nf_sysdb_save_all();
					sprintf(cmd, "cp -a %s%s %s", data_path, "/log_000.log", "/NFDVR/log/log_000.log");
					proxy_system(cmd, 1, 3);

					sprintf(cmd, "cp -a %s%s %s", data_path, "/dgss_data.cfg.gz", "/NFDVR/data");
					proxy_system(cmd, 1, 3);
					
					sprintf(cmd, "cp -a %s%s %s", data_path, "/ssl_public_key.pem", "/NFDVR/webra/conf");
					proxy_system(cmd, 1, 3);

					sprintf(cmd, "cp -a %s%s %s", data_path, "/ssl_private_key.key", "/NFDVR/webra/conf");
					proxy_system(cmd, 1, 3);
					
					sprintf(cmd, "cp -a %s%s %s", data_path, "/sslcertification.pem", "/NFDVR/webra/conf");
					proxy_system(cmd, 1, 3);

					sprintf(cmd, "cp -avrf %s%s %s", data_path, "/IPKI", "/NFDVR/data/");
					proxy_system(cmd, 1, 3);

					nf_sysdb_set_int("sys.info.fwup_state", 1);
				}
			#else
				if (!nf_sysman_fwup_bit_clr())
					g_warning("%s Upgrade bit clear Error!!", __FUNCTION__);
				ret = nf_sysman_fwup_mount_prev_mtd();
				if (ret) {
					gint8 cmd[200];
					nf_sysdb_init(1, NF_SYSDB_INIT_FW_UPGRADE, NF_SYSMAN_FS_MOUNT_DIR_NAME);	//NF_SYSDB_INIT_FW_UPGRADE
					g_message("%s NF_SYSDB_INIT_FW_UPGRADE", __FUNCTION__);
					nf_sysdb_save_all();

					sprintf(cmd, "cp -a %s %s", NF_SYSMAN_FS_MOUNT_DIR_NAME"/NFDVR/log/log_000.log", "/NFDVR/log/log_000.log");
					proxy_system(cmd, 1, 3);
				} else {
					g_warning("%s NF_SYSDB_INIT_LOAD_DATA", __FUNCTION__);
					nf_sysdb_init(1, NF_SYSDB_INIT_LOAD_DATA, NULL );
				}
				nf_sysdb_set_int("sys.info.fwup_state", 1);
			#endif
		} else {
			nf_sysdb_init(1, NF_SYSDB_INIT_LOAD_DATA, NULL );
		}
	}

	#if defined (USE_DEV_PD69104B1)
		nf_dev_sysman_verify_open_mode();
	#endif

	#ifdef ENABLE_FW_QCMODE
		nf_sysman_qcmode_sysdb_init();
	#endif

	nf_set_installation_mode();

	// pakkhman for dual display flag sync between nand and sysdb
	//ksi_test
	// #if defined(_SUPPORT_DUALMONITOR)
	// 	nf_sysman_dual_disp_cmp_flag();
	// #endif

	nf_hw_kcmdline_init();
	nf_sysman_hw_param_init();
	nf_sysman_fwver_init();

	nf_api_param_fwparam_init();
	if (!nf_sysman_cmdline_init())
		g_warning("nf_sysman_cmdline_init fail!!");

	#if defined(_HDI_0412)
		nf_sysman_hdi_video_info_init();		// pakkhman
	#endif

		nf_sysman_check_sig_type();

		nf_sysman_qc_set_signal();
	#if 0		// pakkhman
		nf_sysman_gui_tunning();
	#endif

	nf_ddc_init();			// 20111208 pakkhman
	nf_edid_init(1);
	// Added By pakkhman 20140326
	// for tlvaic23 codec reset
	#if defined(USE_DEV_TLV320AIC23)
		#if defined(USE_DEV_I2C_1)
			nf_dev_set_i2c_register(0x1a, 0x0c, 0x88);		// off
			nf_dev_set_i2c_register(0x1a, 0x0c, 0x0);		// on
		#endif
	#endif

	nf_watchdog_init(1);
	nf_zoneinfo_init();

	#if defined(ENABLE_MICOM_UPGRADE)
		nf_api_param_app_get_cate(NF_PARAM_APP_CATE_IS_MICOM_UPGRADE, &micom_up);
		if(micom_up) {
			micom_upgrade_thread_create();
		}
	#else
		nf_api_param_app_get_cate(NF_PARAM_APP_CATE_IS_MICOM_UPGRADE, &micom_up);
		if(micom_up) {
			nf_api_param_app_set_cate(NF_PARAM_APP_CATE_IS_MICOM_UPGRADE, 0);
		}
	#endif

	nf_notify_init(1);
	nf_action_init(1);		// board_pp, relay
	nf_display_init();

	nf_ptz_init(1);
	nf_keyctrl_init(1);

	if (nf_sysman_qcmode_is_enable() == FALSE) {

		nf_hid_init();
		nf_pos_init();
		nf_netif_eth_init();
		nf_netif_init();
		nf_snmpd_init();
	#ifndef	DEBUG_SIMPLE_NF
		#if defined(DEBUG_DUMMY_GMAINLOOP)
			nf_filesystem_init_debug(0);
		#else
			nf_filesystem_init();
		//	nf_filesystem_init_debug(1);
		//    g_message("========================================== filesystem init done\n");
		//    sleep(3);
		#endif

	proxy_system("touch /tmp/ready_x", 1, 3);

		#if defined(ENABLE_AUD_HI_CHIP)
			nf_HI_aud_init();
		#elif defined(CHIP_NVT)
			nf_audio_init();
		#else
			nf_rec_audio_init(1);
			//nf_hi_aud_init();
		#endif

	nf_record_init(1);

		//FIXME
	#endif
		nf_jpeg_man_init(1);
		nf_issm_ctl_init();
		nf_network_init(1);
		nf_smart_ntp_init();
	#ifdef USE_RTSP4VLC
		nf_nvs_net_init(1);
	#endif
		nf_mail_send_init(1);
		nf_net_send_init(1);
		nf_util_push_init(1);
	// onvif_porting
	#ifdef ENABLE_ONVIF_DEVICE
		onvif_event_init();
	#endif
	// onvif_porting
		nf_event_init(1);		// sensor, tw2864
  		nf_auth_main();
	//	nf_webbase_init(1);

	#if WEBRA1_FOR_WEBRA2
  		nf_webbase_init(1);
	#endif
   	webra_host_main();
		
	admin_server_api_init();
	
    #if defined(ENABLE_NABTO)
	nabto_ctl_init();
	#endif

	// version display
	g_message("----------------------------------------");
	g_message("NF_HOST VER [%d.%d.%d]", RELEASE, MAJOR, MINOR);
	g_message("NF_HOST SYSID [%s]", nf_sysdb_get_str_nocopy("sys.info.sysid"));
	g_message("NF_HOST SWVER [%s]", nf_sysdb_get_str_nocopy("sys.info.swver"));
	g_message("----------------------------------------");

	//g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

      #ifdef DEBUG_ENABLE_JBSHELL
		init_jbshell_ipx(2048);	// 2048
      #else
	#ifdef DEBUG_JBSHELL
		if (nf_sysman_hotkey_is_nfs()) {
			init_jbshell_ipx(2048);	// 2048
		} else {
			init_jbshell_ipx(0);	// disable
		}
	#endif
     #endif

	if (!nf_sysman_hotkey_is_nfs()) {
		nf_sysman_telnet_ctrl_init();
		nf_sysman_telnet_ctrl();
		
		//nf_net_console_ctrl_init();
		//nf_net_console_ctrl();
	}

	#ifdef DEBUG_SIMPLE_PASSWD
		g_message("DEBUG_SIMPLE_PASSWD !!!!!!!!!!!!!!!!!");

		nf_sysdb_set_str("usr.U0.pass","");
		nf_sysdb_set_bool("sys.info.passwd_enable",0);
	#else
		if (nf_sysman_hotkey_is_passwd_reset()) {
			gchar buf[128] = { 0, };

			sprintf(buf, "passwd_reset on!!");
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, buf);

			g_message("nf_sysman_hotkey_is_passwd_reset on!!");

			nf_dev_buzzer_on();
			g_usleep(100000);
			nf_dev_buzzer_off();
			g_usleep(100000);
			nf_dev_buzzer_on();
			g_usleep(100000);
			nf_dev_buzzer_off();

			nf_sysdb_set_str("usr.U0.pass","1234");
		}
	#endif

	#ifdef DEBUG_FORCE_FORMAT
		g_usleep(2000000);
		g_message("DEBUG_FORCE_FORMAT !!!!!!!!!!!!!!!!!!!!!!!1");
		my_command("disk_format 0");
	#endif

	#if DEBUG_BOOT_PROFILE
   		clock_gettime(CLOCK_REALTIME, &end);
    	delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    	g_print("PROFILE: 00000001: %fms\n", delta);    //11858msec
    	clock_gettime(CLOCK_REALTIME, &start);
	#endif

	#ifdef SUPPORT_VCA_CAMERA
		nf_meta_data_init();
	#endif

	//23sec
    g_warning("=================OKOKOKOK==============\n");

	//    nf_record_start(NULL);
	//#if defined(DEBUG_AUTO_START) || defined(DEBUG_DUMMY_GMAINLOOP)
	#if defined(DEBUG_DUMMY_GMAINLOOP)
		g_message("init_dummy_gui !!!!!!!!!!!!!!!!!!!!!!!2");
		init_dummy_gui();
	#else
		g_message("INIT_GUI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!2");
		nf_arch_manager_start();

		#ifdef ENABLE_FW_QCMODE
			nf_sysman_qcmode_2nd_init();
		#endif
		nf_sysman_analize_nand_log();
		init_gui();
		// sleep(2);
		// nf_webbase_init(1);
		#if DEBUG_BOOT_PROFILE
			clock_gettime(CLOCK_REALTIME, &end);
			delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
			g_print("PROFILE: 00000002: %fms\n", delta);
			clock_gettime(CLOCK_REALTIME, &start);
		#endif

	#endif
    	nf_play_init();
       	nf_ipcam_init();
		nf_admintool_init();
		nf_rass_usb_init();
	#if DEBUG_BOOT_PROFILE
   		clock_gettime(CLOCK_REALTIME, &end);
    	delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    	g_print("PROFILE: 00000003: %fms\n", delta);    //168msec
	    clock_gettime(CLOCK_REALTIME, &start);
	#endif

	#ifdef DEBUG_MEMORY_NET
		my_command("monitor");
		my_command("debug_mem BOARD_START");
		my_command("timer_add 5 0 debug_mem timer_chk");
	#endif
	#ifdef DEBUG_NETIF_STAT
		my_command("timer_add 10 1 netif_delta");
	#endif
	#ifdef DEBUG_REC_GOP_STAT
		//my_command("timer_add 1 0 rec_gop 1 0");
		my_command("timer_add 5 1 rec_stat 1");
	#endif
	#ifdef DEBUG_MONITOR_STAT
		my_command("timer_add 60 0 memmon");
		my_command("timer_add 10 1 cmemmon");
		//my_command("timer_add 1 0 cmemmon");
	#endif

	#ifdef DEBUG_NOTIFY_STAT
		my_command("timer_add 11 0 notify_dump_file");
		my_command("timer_add 7 0 watchdog_dump_file");
	#endif

	//my_command("timer_add 3 1 watchdog_dump");
	//my_command("timer_add 1 0 rec_cq 3");

	#ifdef ENABLE_WATCHDOG
		nf_watchdog_start_run();
	#endif

// #if defined(_IPX_1648M4) || defined(_IPX_0824M4) || defined(_IPX_0412M4) || defined(_IPX_1648M4E) || defined(_IPX_32P4E)  // || defined(_IPX_1648P4E) 
// 	if(!nf_sysman_qcmode_is_enable())
// 		nf_guard_init();
// #endif

		nf_sysman_bdflush();
	// onvif_porting
	#ifdef ENABLE_ONVIF_DEVICE
		onvif_init();
	#endif
	// onvif_porting
	#ifdef SUPPORT_VCA_CAMERA
		nf_meta_data_init();
	#endif

	// ksi_test nf_sysman_capture_serial_ctrl_init();
	// nf_sysman_capture_serial_ctrl();
	} else { // qc main

	#if defined(_IPX_1648P4E)|| defined(_IPX_0824P4E)|| defined(_IPX_32P4E) || defined(_IPX_32P5)
		nf_ptz_init(1);
		nf_keyctrl_init(1);
	#endif 
		//nf_hid_init();
		//nf_pos_init();
		nf_netif_eth_init();
		nf_netif_init();
		//nf_snmpd_init();
	#ifndef	DEBUG_SIMPLE_NF
		#if defined(DEBUG_DUMMY_GMAINLOOP)
			nf_filesystem_init_debug(0);
		#else
			nf_filesystem_init();
		//	nf_filesystem_init_debug(1);
		    g_message("========================================== filesystem init done\n");
		    sleep(3);
		#endif

		#if defined(ENABLE_AUD_HI_CHIP)
		//	nf_HI_aud_init();
		#elif defined(CHIP_NVT)
			nf_audio_init();
		#else
			nf_rec_audio_init(1);
		#endif
		//FIXME
	#endif

		//nf_jpeg_man_init(1);

		nf_issm_ctl_init();

		nf_network_init(1);
	#ifdef USE_RTSP4VLC
		nf_nvs_net_init(1);
	#endif
		//nf_mail_send_init(1);
		//nf_net_send_init(1);
		//nf_util_push_init(1);
	// onvif_porting
	#ifdef ENABLE_ONVIF_DEVICE
		onvif_event_init();
	#endif
	// onvif_porting
		nf_event_init_qc(1);
  		nf_auth_main();
	//	nf_webbase_init(1);
	#if WEBRA1_FOR_WEBRA2
  		nf_webbase_init(1);
	#endif
   	webra_host_main();

	// version display
	g_message("----------------------------------------");
	g_message("NF_HOST VER [%d.%d.%d]", RELEASE, MAJOR, MINOR);
	g_message("NF_HOST SYSID [%s]", nf_sysdb_get_str_nocopy("sys.info.sysid"));
	g_message("NF_HOST SWVER [%s]", nf_sysdb_get_str_nocopy("sys.info.swver"));
	g_message("----------------------------------------");

	//g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

	init_jbshell_ipx(0);	// disable
	nf_sysman_telnet_start();

	#ifdef DEBUG_SIMPLE_PASSWD
		g_message("DEBUG_SIMPLE_PASSWD !!!!!!!!!!!!!!!!!");

		nf_sysdb_set_str("usr.U0.pass","");
		nf_sysdb_set_bool("sys.info.passwd_enable",0);
	#else
		if (nf_sysman_hotkey_is_passwd_reset()) {
			gchar buf[128] = { 0, };

			sprintf(buf, "passwd_reset on!!");
			nf_eventlog_put_param( NULL, LT_SYSTEM_DEBUG, 1, LP2_SYSTEM_DEBUG_HOTKEY, buf);

			g_message("nf_sysman_hotkey_is_passwd_reset on!!");

			nf_dev_buzzer_on();
			g_usleep(100000);
			nf_dev_buzzer_off();
			g_usleep(100000);
			nf_dev_buzzer_on();
			g_usleep(100000);
			nf_dev_buzzer_off();

			nf_sysdb_set_str("usr.U0.pass","1234");
		}
	#endif

	#ifdef DEBUG_FORCE_FORMAT
		g_usleep(2000000);
		g_message("DEBUG_FORCE_FORMAT !!!!!!!!!!!!!!!!!!!!!!!1");
		my_command("disk_format 0");
	#endif

    //FIXME
    for (i = 0; i < 32; i++) {
        ch_arr[i] = -1;
        covert_arr[i] = TRUE;
    }

    for (i = 0; i < NUM_ACTIVE_CH; i++)
        ch_arr[i] = (char)i;

    if (NUM_ACTIVE_CH == 4) {
        disp_mode = NF_DISPLAY_QUAD;
    } else if(NUM_ACTIVE_CH == 8) {
        disp_mode = NF_DISPLAY_NONA;
    } else if(NUM_ACTIVE_CH == 16) {
        disp_mode = NF_DISPLAY_HEXADECA;
    } else if(NUM_ACTIVE_CH == 32) {
    	disp_mode = NF_DISPLAY_HEXATRICONTA;
    }
	else {
        g_assert(0);
    }

	#if DEBUG_BOOT_PROFILE
   		clock_gettime(CLOCK_REALTIME, &end);
    	delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    	g_print("PROFILE: 00000000: %fms\n", delta);    //4261msec
    	clock_gettime(CLOCK_REALTIME, &start);
	#endif
	#if 0
	nf_live_init_qc(disp_mode, 0, 0,
                	(guint)DISPLAY_ACTIVE_WIDTH,
                	(guint)DISPLAY_ACTIVE_HEIGHT,
                	ch_arr, (gchar *)covert_arr, 0
                	);
	#else
	nf_live_init(disp_mode, 0, 0,
	                (guint)DISPLAY_ACTIVE_WIDTH,
                	(guint)DISPLAY_ACTIVE_HEIGHT,
                	ch_arr, (gchar *)covert_arr, 0
                	);
	#endif

	#if DEBUG_BOOT_PROFILE
   		clock_gettime(CLOCK_REALTIME, &end);
    	delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    	g_print("PROFILE: 00000001: %fms\n", delta);    //11858msec
    	clock_gettime(CLOCK_REALTIME, &start);
	#endif

	//23sec
    g_warning("=================OKOKOKOK==============\n");

	//    nf_record_start(NULL);
	//#if defined(DEBUG_AUTO_START) || defined(DEBUG_DUMMY_GMAINLOOP)
	#if defined(DEBUG_DUMMY_GMAINLOOP)
		g_message("init_dummy_gui !!!!!!!!!!!!!!!!!!!!!!!2");
		init_dummy_gui();
	#else
		g_message("INIT_GUI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!2");
		nf_arch_manager_start();

		#ifdef ENABLE_FW_QCMODE
		//	nf_sysman_qcmode_2nd_init();
		#endif

		nf_sysman_analize_nand_log();
		init_gui();
		// sleep(2);
		// nf_webbase_init(1);
		#if DEBUG_BOOT_PROFILE
			clock_gettime(CLOCK_REALTIME, &end);
			delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
			g_print("PROFILE: 00000002: %fms\n", delta);
			clock_gettime(CLOCK_REALTIME, &start);
		#endif

	#endif
       //nf_play_init();
       nf_ipcam_init();
	// nf_dev_switch_init(NF_UTIL_SWITCH_CCTV_MODE);
	nf_dev_switch_init(NF_UTIL_SWITCH_CCTV_MODE);
	nf_netif_init();
	//nf_admintool_init();

	//ksi_test
	// #if defined(_IPX_1648P4E) || defined(_IPX_0824P4E)|| defined(_IPX_32P4E)
    //     //	nf_sysman_domestic_set_lic();
	// #else
    //             nf_sysman_domestic_set_lic();
	// #endif

	#if DEBUG_BOOT_PROFILE
   		clock_gettime(CLOCK_REALTIME, &end);
    	delta = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1.0e6;
    	g_print("PROFILE: 00000003: %fms\n", delta);    //168msec
	    clock_gettime(CLOCK_REALTIME, &start);
	#endif

	#ifdef DEBUG_MEMORY_NET
		my_command("monitor");
		my_command("debug_mem BOARD_START");
		my_command("timer_add 5 0 debug_mem timer_chk");
	#endif
	#ifdef DEBUG_NETIF_STAT
		my_command("timer_add 10 1 netif_delta");
	#endif
	#ifdef DEBUG_REC_GOP_STAT
		//my_command("timer_add 1 0 rec_gop 1 0");
		my_command("timer_add 5 1 rec_stat 1");
	#endif
	#ifdef DEBUG_MONITOR_STAT
		my_command("timer_add 60 0 memmon");
		my_command("timer_add 10 1 cmemmon");
		//my_command("timer_add 1 0 cmemmon");
	#endif

	#ifdef DEBUG_NOTIFY_STAT
		my_command("timer_add 11 0 notify_dump_file");
		my_command("timer_add 7 0 watchdog_dump_file");
	#endif

	//my_command("timer_add 3 1 watchdog_dump");
	//my_command("timer_add 1 0 rec_cq 3");

	#ifdef ENABLE_WATCHDOG
		nf_watchdog_start_run();
	#endif

		//nf_sysman_bdflush();
	// onvif_porting
	#ifdef ENABLE_ONVIF_DEVICE
	//	onvif_init();
	#endif
	// onvif_porting
	#ifdef SUPPORT_VCA_CAMERA
	//	nf_meta_data_init();
	#endif

	}


#ifdef DEBUG_MEMORY_LEAK_CHECK
	g_message("NF_HOST main loop start!!");
	{
		pthread_t tid;
		const char* dir = "/root/heapprof";
		int result = mkdir(dir, 0755);

		if (result == 0) {
			printf("Directory '%s' created successfully.\n", dir);
		} else if (errno == EEXIST) {
			printf("Directory '%s' already exists.\n", dir);
		} else {
			perror("Failed to create directory");
			return 1;
		}

		printf("[Main] Starting heap profiler...\n");
				
		HeapProfilerStart("/root/heapprof/nfhost_12h");
		printf("[Main] Heap profiling started!\n");

		pthread_t profiler;
		pthread_create(&profiler, NULL, profiler_thread, NULL);
		
		printf("[Main] Program exiting...\n");
	}
#endif
	//unit_test_main();
	g_main_loop_run (NF_Loop);

	g_message("NF_Loop stop!!");

	return 0;
}
