#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <semaphore.h>

#include <glib.h>

#include "nf_common.h"
#include "nf_timer.h"

#include "nf_api_play.h"
#include "nf_api_live.h"
#include "nf_util_device.h"

#include "jbshell.h" 

gchar **
split_string (const char *str, int *argc)
{
	gchar **argv;
	int len;
		
	argv = g_strsplit (str, " ", 0);
	
	for (len = 0; argv[len] != NULL; len++);
	
	if (argc)
		*argc = len;
	  
	return argv;
}
// g_strfreev (argv);


int my_system (const char *string)
{
	printf("\n==> %s\n", string);	
	return system(string);
}


int my_command(const char *string)
{
	gint 	argc = 0, ret;
	gchar **argv = NULL;
	
	printf("\n==> %s\n", string);	
		
	argv = split_string( string, &argc);	
	ret = do_shell_cmd( argc, argv );	
	g_strfreev(argv);	
	
	return ret;
	
}

int my_command_slient(const char *string)
{
	gint 	argc = 0, ret;
	gchar **argv = NULL;
			
	argv = split_string( string, &argc);	
	ret = do_shell_cmd( argc, argv );	
	g_strfreev(argv);	
	
	return ret;
	
}


int my_cat(const char *filename)
{
	gchar *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;
  	
	if (!g_file_get_contents (filename, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return -1;
	}
	g_message("fileopen name[%s] len[%d]", filename, length);    

	if(contents)
	{
		g_print("%*.*s", length, length, contents);	
		g_free(contents);
	}		    
	return 0;
}

#if 0
static char fb_clean_help[] = "frame buffer clean";
static int fb_clean(int argc, char **argv){
		
	my_system("cat /dev/zero > /dev/fb/0");				
	return 0;
}
__commandlist(fb_clean,"fb_clean","frame buffer clean",fb_clean_help);
#endif

static char dspmon_help[] = "dsp monitor infomation";
static int dspmon(int argc, char **argv){
	
	my_cat("/proc/dspcomm");
	//my_system("cat /proc/dspcomm");
	return 0;
}
__commandlist(dspmon,"dspmon","dsp monitor infomation",dspmon_help);


static char ifsmon_help[] = "ifs stat infomation";
static int ifsmon(int argc, char **argv){
		
	my_cat("/proc/ifs");	
	return 0;
}
__commandlist(ifsmon,"ifsmon","ifs stat monitor infomation",ifsmon_help);

#if 0
static char ps_help[] = "ps process information";
static int ps(int argc, char **argv){
		
	my_system("ps -ef f");
	return 0;
}
__commandlist(ps,"ps","process information",ps_help);
#endif

static char cmemdump_help[] = "cmemdump [start_addr:cmem_base] [len:32k]";
static int cmemdump(int argc, char **argv){
	
	unsigned int start_addr = 0;
	unsigned int len = 32;
	FILE *fp = NULL;
		
	if(argc < 1)
		printf("%s\n",cmemdump_help);

	start_addr = strtoul(argv[1],NULL,0);
	
	if(argc>2)
		len = strtoul(argv[2],NULL,0);
		
	fp = fopen("cmemdump.out", "a");
	
	if(!fp)
		return 0;

	{
		int block_cnt = len / 32;
		int extra_kbyte = len % 32;
		int i = 0;
		unsigned int wr_addr = 0;
		
		for(i=0; i<block_cnt; ++i)
		{
			wr_addr = start_addr + (i*32*1024);
			fwrite( wr_addr, 32*1024, 1, fp);
			
			if(i%128 == 0)
				g_print("%s write addr[%x]\n", __FUNCTION__, wr_addr );
		}
		
		fwrite( start_addr + (i*32*1024), extra_kbyte*1024 , 1, fp);
			
	}
	fclose(fp);
	
	return 0;
}
__commandlist(cmemdump,"cmemdump",cmemdump_help,cmemdump_help);


static char cmemmon_help[] = "cmem information";
static int cmemmon(int argc, char **argv){
		
	my_cat("/proc/cmem");
	//my_system("cat /proc/cmem");
	return 0;
}
__commandlist(cmemmon,"cmemmon",cmemmon_help,cmemmon_help);

static char memmon_help[] = "mem information";
static int memmon(int argc, char **argv){
		
	my_cat("/proc/meminfo");
	//my_system("cat /proc/meminfo");
	return 0;
}
__commandlist(memmon,"memmon",memmon_help,memmon_help);


static char memprofile_help[] = "mem information";
static int memprofile(int argc, char **argv){
		
	g_mem_profile();
	return 0;
}
__commandlist(memprofile,"memprofile",memprofile_help,memprofile_help);

static char monitor_help[] = "dvr monitor infomation";
static int monitor(int argc, char **argv){

	my_cat("/proc/dspcomm");
	my_cat("/proc/ifs");
	my_cat("/proc/cmem");
	my_cat("/proc/meminfo");
		
	return 0;
}
__commandlist(monitor,"monitor","dvr monitor infomation",monitor_help);


static char reset_help[] = "system reset";
static int reset(int argc, char **argv){
			
	nf_dev_board_reset();
	return 0;
}
__commandlist(reset,"reset",reset_help,reset_help);


static char quit_help[] = "system quit";
static int quit(int argc, char **argv){
	
	exit(EXIT_SUCCESS);
	return 0;
}
__commandlist(quit,"quit",quit_help,quit_help);


static char shutdown_help[] = "nf_host shutdown";
static int shutdown(int argc, char **argv){
	
	my_command("set_md 0 0");
	my_command("set_md 1 0");

	my_command("play_stop");

	my_command("rec_net 0");
	my_command("rec_onoff 0 1");
	my_command("rec_audio_stop");
		
	my_command("net_stop 3");
	
	printf("wait 3sec\n");
	sleep(3);
	
	my_command("fs_stop");	
	my_command("stop_sst");	
	
	my_command("debug_mem BOARD_STOP");
	
	//nf_live_stop();	
	return 0;
}
__commandlist(shutdown,"shutdown",shutdown_help,shutdown_help);


#if 0

static gpointer *play_handle = NULL;
static NF_PLAY_PARAM play_param;

static char play_start_help[] = "play_start [size:1] [ch] [sec]";
static int play_start(int argc, char **argv)
{	
	return 0;
}
__commandlist(play_start,"play_start",play_start_help,play_start_help);

static char play_stop_help[] = "play_stop";
static int play_stop(int argc, char **argv)
{
	if( play_handle == NULL) 
		return -1;
						
	nf_play_stop(play_handle);

	play_handle = NULL;
	return 0;
}
__commandlist(play_stop,"play_stop",play_stop_help,play_stop_help);


static char play_test_help[] = "play_test";
static int play_test(int argc, char **argv)
{	
	if( play_handle != NULL) 
		return -1;

	my_command("play_start 1 0");
	sleep(2);
	my_command("play_stop");	

	return 0;
}
__commandlist(play_test,"play_test",play_test_help,play_test_help);

#endif

static char qformat_help[] = "qformat";
static int qformat(int argc, char **argv)
{	
	my_command("play_stop");
	my_command("rec_onoff 0	");
	my_command("net_stop 2");
	
	sleep(1);
	my_command("rec_stop 0");
	my_command("rec_stop 1");	
	sleep(2);

	my_command("fs_stop");
	my_command("disk_format 0");
	my_command("fs_start 1");

	return 0;
}
__commandlist(qformat,"qformat",qformat_help,qformat_help);

static int my_cat_to_ipxshell(const char *filename)
{
	gchar *contents = NULL;
	gsize  length = 0;
	GError *error = NULL;
  	
	if (!g_file_get_contents (filename, &contents, &length, &error))
	{
		g_warning("%s\n", error->message);
		g_error_free (error);

		if(contents)
			g_free(contents);

		return -1;
	}
	
	ipx_printf("fileopen name[%s] len[%d]\n", filename, length);    

	if( length > MAX_PRINTF_BUFF-1 )
		length = MAX_PRINTF_BUFF-1;
		
	if(contents)
	{
		ipx_printf_large("%*.*s\n", length, length, contents);	
		g_free(contents);
	}		    
	return 0;
}

static char mycat_help[] = "mycat [filename]";
static int mycat(int argc, char **argv)
{	

	if(argc < 1)
		ipx_printf("%s\n",mycat_help);

	my_cat_to_ipxshell( argv[1] );

	return 0;
}
__commandlist(mycat, "mycat", mycat_help, mycat_help);
