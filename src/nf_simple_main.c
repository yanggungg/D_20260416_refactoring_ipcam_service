/** @file	nf_main.c
	@brief	nf host main.c 
	
	@todo	랄라라~
*/

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <execinfo.h>

#include <unistd.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

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
#include "nf_sysman.h"
#include "nf_sysdb.h"

#include "nf_api_live.h"
#include "nf_util_device.h"
#include "libicmem.h"
#include "jbshell.h"
#include "unp.h"

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
extern void _ui_main(void *);

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "nf_main"

#define DEBUG_FORCE_FORMAT
//#define DEBUG_NO_LIVE
//#define DEBUG_NETIF_INIT
#define DEBUG_JBSHELL
#define DEBUG_MEMORY_NET
#define DEBUG_DUMMY_GMAINLOOP	
//#define DEBUG_SIMPLE_NF


GMainLoop *NF_Loop; 
GMainContext *NF_Context;

int 	_ui_skip_sst_init = 0;
int 	_ui_argc;
char 	**_ui_argv;

#ifdef DEBUG_DUMMY_GMAINLOOP

static void _ui_dummy_main(void *arg)
{
	gtk_init(&_ui_argc, &_ui_argv);
	gtk_main();	
}

/**
	@brief	init_gui() gtk_main()을 실행할 쓰레드를 로딩한다.
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

#endif

#ifndef DEBUG_DUMMY_GMAINLOOP
/**
	@brief	init_gui() gtk_main()을 실행할 쓰레드를 로딩한다.
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
#endif

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

#define BT_TRACE_SIZE	16

void _nf_signal_handler(int signo, struct siginfo *info, struct ucontext *ctx)
{	

	void *trace[BT_TRACE_SIZE];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
 	
 	struct sigcontext *sc = &ctx->uc_mcontext;	

	printf("Got signal %d [%s]\n", signo, 
				(unsigned int) signo < 32 ? _sig_list[signo]:_sig_list[0]);
	
	nf_debug_show_registers(sc);
	
	memset(trace, 0x00, sizeof(trace));
	
	trace_size = backtrace(trace, BT_TRACE_SIZE);
	
	/* overwrite sigaction with caller's address */
	trace[1] = (void *)sc->arm_lr;

	messages = backtrace_symbols(trace, BT_TRACE_SIZE);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");	
	for (i=1; i<BT_TRACE_SIZE; ++i)
	{
		printf("[bt] [%2d] [0x%08x] %s\n",i , trace[i], messages[i]);
	}
		
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
	
}

/**
	@brief	nf_main.c
	@param[in]	argc 파라메터 갯수
	@param[in]	argv 파라메터
	@return	int	실행결과 0:성공 else failed
*/
int main( int argc, char *argv[]) 
{
	_ui_argc = argc;
	_ui_argv = &*argv;
		
	g_thread_init (NULL); 
	g_type_init();
	
	gst_init(&argc, &argv);
    
    ICMEM_init(NULL, NULL);
	CERuntime_init();

	NF_Context = g_main_context_new();
	NF_Loop = g_main_loop_new (NF_Context, FALSE);

	init_signal();

	// 2008-12-05 오후 3:18:35
	// 초기화 순서는 중요합니다. 
	// 임의로 변경하지 말고 choissi와 협의하세요.
	
	nf_dev_init();
	nf_sysman_hotkey_init();

	if( nf_sysman_hotkey_is_factory_default() )
	{
		g_message("nf_sysman_hotkey_is_factory_default on!!");
		
		nf_dev_buzzer_on();
		g_usleep(100000);
		nf_dev_buzzer_off();

		nf_sysdb_init(1, NF_SYSDB_INIT_FACTORY_DEFAULT );	
		nf_sysdb_save_all();
	}else{
		nf_sysdb_init(1, NF_SYSDB_INIT_LOAD_DATA );	
	}				
		
	nf_notify_init(1);
	nf_action_init(1);		// board_pp, relay
	nf_display_init();
	
	nf_ptz_init(1);
	nf_keyctrl_init(1);

	if( !nf_sysman_hotkey_is_nfs() )
		nf_netif_init();
	
#ifndef	DEBUG_SIMPLE_NF	
	nf_filesystem_init();

	nf_rec_audio_init(1);	
	nf_record_init(1);
#endif

	nf_network_init(1);
	nf_mail_send_init(1);

	nf_event_init(1);		// sensor, tw2864
	
	// version display
	g_message("----------------------------------------");
	g_message("NF_HOST VER [%d.%d.%d]", RELEASE, MAJOR, MINOR);			
	g_message("NF_HOST SYSID [%s]", nf_sysdb_get_str_nocopy("sys.info.sysid"));
	g_message("NF_HOST SWVER [%s]", nf_sysdb_get_str_nocopy("sys.info.swver"));
	g_message("----------------------------------------");
		
	//g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

#ifdef DEBUG_JBSHELL
	init_jbshell();
#endif

#ifdef DEBUG_SIMPLE_PASSWD
	g_message("DEBUG_SIMPLE_PASSWD !!!!!!!!!!!!!!!!!");

	nf_sysdb_set_str("usr.U0.pass","");
	nf_sysdb_set_bool("sys.info.passwd_enable",0);
#else

	if( nf_sysman_hotkey_is_passwd_reset() )
	{
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
	sleep(3);
	my_command("shutdown");
	exit(0);
#else
	if( nf_sysman_hotkey_is_factory_default() )
	{	
		g_usleep(2000000);
		g_message("HOT_KEY_FORMAT !!!!!!!!!!!!!!!!!!!!!!!1");
		my_command("disk_format 0");
	}			
#endif

//#if defined(DEBUG_AUTO_START) || defined(DEBUG_DUMMY_GMAINLOOP)
#ifdef DEBUG_DUMMY_GMAINLOOP
	g_message("init_dummy_gui !!!!!!!!!!!!!!!!!!!!!!!2");
	init_dummy_gui();
#else
	g_message("INIT_GUI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!2");
	init_gui();		
#endif

#ifndef	DEBUG_SIMPLE_NF
	nf_play_init();		
	nf_live_init(NF_DISPLAY_HEXADECA, NULL, 0 );
#endif

#ifdef DEBUG_MEMORY_NET			
	my_command("monitor");
	my_command("debug_mem BOARD_START");
	my_command("timer_add 5 0 debug_mem timer_chk");
#endif	

#ifdef DEBUG_NETIF_STAT
	my_command("timer_add 10 1 netif_delta");
#endif	

	g_main_loop_run (NF_Loop);
	
	g_message("NF_Loop stop!!");		
	return 0;
}
