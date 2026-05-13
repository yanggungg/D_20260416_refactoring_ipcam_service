#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <time.h>
#include <glib.h>

#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_api_eventlog.h"
#include "nf_event.h"

#include "nf_rtsp_over_ws.h"


static void nf_row_sysdb_change_func(NF_NOTIFY_INFO *p_info, gpointer p_data);

#define WS_SET_LOCAL_BIND (1)	// 1 : On, 0 : Off

static int g_row_ws_port = 702;
static int g_row_rtsp_port = 0;
static int g_row_ssl_on = 0;
static char g_row_ssl_cert_file_path[] = "/tmp/webra/conf/ssl_pub_link";
static char g_row_ssl_key_file_path[] = "/tmp/webra/conf/ssl_pri_link";


void nf_row_init()
{
	gulong cb_handle;

	printf("[%s][%d] Start\n", __FUNCTION__, __LINE__);

	cb_handle = nf_notify_connect_cb("sysdb_change", nf_row_sysdb_change_func, (gpointer)NULL);
	if (cb_handle == 0)
	{
		printf("[%s][%d] Error - nf_notify_connect_cb[nf_row_sysdb_change_func]\n",
				__FUNCTION__, __LINE__);
		return;
	}

	g_row_rtsp_port = (int)nf_sysdb_get_uint("net.rtp.rtspport");
	g_row_ssl_on = (int)nf_sysdb_get_bool("net.proto.httpson");

	nf_row_reset();

	printf("[%s][%d] End\n", __FUNCTION__, __LINE__);
}

void nf_row_reset()
{
	FILE *fp;
	const char info_file_path[] = "/tmp/rtsp_over_ws.info";
	int ret;

	printf("[%s][%d] Start\n", __FUNCTION__, __LINE__);

	fp = fopen(info_file_path, "w");
	if (fp == 0)
	{
		printf("[%s][%d] Error - fopen[%s] failed.\n", __FUNCTION__, __LINE__, info_file_path);
		return;
	}

	if (g_row_ssl_on)
		fprintf(fp, "%d %d %d %s %s", WS_SET_LOCAL_BIND, g_row_ws_port, g_row_rtsp_port, g_row_ssl_cert_file_path, g_row_ssl_key_file_path);
	else
		fprintf(fp, "%d %d %d", WS_SET_LOCAL_BIND, g_row_ws_port, g_row_rtsp_port);

	fclose(fp);

	ret = proxy_system("killall rtsp_over_ws.bin", 1, 3);
	printf("[%s][%d] End\n", __FUNCTION__, __LINE__);
	return;
}

void nf_row_state_check_and_reset(){
	char buff[2048];
	int state = 0, is_loop_proc_work = FALSE;
  FILE* fp = NULL;

	g_message("[%s][%d] won102 code trace :: [popen call]", __FUNCTION__, __LINE__ );
	fp = popen("ps -o comm | grep rtsp_over_ws.sh", "r" );

  if ( fp == NULL )
	{
			perror("erro : ");
	}else{

		while(fgets(buff, sizeof(buff), fp) != NULL)
		{

			g_message("[%s][%d] won102 code trace :: [%d]", __FUNCTION__, __LINE__ ,strstr( buff, "rtsp_over_ws.sh" ) );
			g_message("[%s][%d] won102 code trace :: [buff : %s]", __FUNCTION__, __LINE__ , buff );
			if( strstr( buff, "rtsp_over_ws.sh" ) != NULL ){
				is_loop_proc_work = TRUE;
			}
		}

		if( fp != NULL ){
			state = pclose(fp);
			
			if( !is_loop_proc_work ){
				g_message("[%s][%d] won102 code trace :: [notfound rtsp proc]", __FUNCTION__, __LINE__  );
				proxy_system("rm /tmp/rtsp_over_ws.info", 1, 3);
				system("cd /NFDVR/pds;./rtsp_over_ws_run.sh");
			}
		}

		nf_row_reset();

	}

}


static void nf_row_sysdb_change_func(NF_NOTIFY_INFO *p_info, gpointer p_data)
{
	int is_changed = 0;

	if (p_info->d.params[0] == NF_SYSDB_CATE_NET)
	{
		int rtsp_port;
		int ssl_on;

		rtsp_port = (int)nf_sysdb_get_uint("net.rtp.rtspport");
		if (g_row_rtsp_port != rtsp_port)
		{
			g_row_rtsp_port = rtsp_port;
			is_changed++;
		}

		ssl_on = (int)nf_sysdb_get_bool("net.proto.httpson");
		if (g_row_ssl_on != ssl_on)
		{
			g_row_ssl_on = ssl_on;
			is_changed++;
		}
	}

	if (is_changed)
		nf_row_reset();
}

