#ifndef __NF_IPCAM_UNITTEST_C__
#define __NF_IPCAM_UNITTEST_C__

#include "nf_common.h"
#include "nf_api_ipcam.h"
#include "nf_ipcam_defs.h"

#include "nf_api_openmode.h"
#include "nf_ptz.h"
#include "nf_record.h"


#include "jbshell.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "ipcam-test"

#define NUM_IPX_CHANNEL		16

extern int hub_fw_ver_chg(int idx, char *fw_ver);
static char hub_ver_chg_help[] = "hub_fw_ver_chg";
static int hub_ver_chg(int argc, char **argv)
{
	if(argc != 3)
	{
		return -1;
	}

	hub_fw_ver_chg(atoi(argv[1]), argv[2]);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(hub_ver_chg, "hub_ver_chg", hub_ver_chg_help, hub_ver_chg_help);
#endif

extern int hub_fw_have_chg(char *have_str);
static char hub_have_chg_help[] = "hub_have_chg 111(hubfw have)";
static int hub_have_chg(int argc, char **argv)
{
	if(argc != 2)
	{
		return -1;
	}

	hub_fw_have_chg(argv[1]);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(hub_have_chg, "hub_have_chg", hub_have_chg_help, hub_have_chg_help);
#endif

static char proxy_popen_test_help[] = "proxy_popen_test";
static int proxy_popen_test(int argc, char **argv)
{
	FILE * fp = NULL;
	int fd = -1;
	int idx = 0;
	char cmd[256] = {0};

	snprintf(cmd, sizeof(cmd), "ls");

	for(idx = 0; idx < 1024; idx++)
	{
		fp = NULL;
		fd = -1;
		fp = proxy_popen(cmd, "r", &fd);
		usleep(10000);
		proxy_pclose(fp, fd);
		usleep(10000);
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(proxy_popen_test, "proxy_popen_test", proxy_popen_test_help, proxy_popen_test_help);
#endif

static char ipcam_ip_changed_test_help[] = "ipcam_ip_changed_test";
static int ipcam_ip_changed_test(int argc, char **argv)
{
	nf_ipcam_ip_changed();
	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_ip_changed_test, "ipcam_ip_changed_test", ipcam_ip_changed_test_help, ipcam_ip_changed_test_help);
#endif

static char scan_camera_test_help[] = "scan_camera_test";
static int scan_camera_test(int argc, char **argv)
{
	nf_openmode_scan_camera();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(scan_camera_test, "scan_camera_test", scan_camera_test_help, scan_camera_test_help);
#endif

static char openmode_final_test_help[] = "openmode_final_test";
static int openmode_final_test(int argc, char **argv)
{
	nf_openmode_finalize_installation();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(openmode_final_test, "openmode_final_test", openmode_final_test_help, openmode_final_test_help);
#endif

static char ipcam_start_help[] = "ipcam_start";
static int ipcam_start(int argc, char **argv)
{
	nf_ipcam_start();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_start, "ipcam_start", ipcam_start_help, ipcam_start_help);
#endif

static char ipcam_stop_help[] = "ipcam_stop";
static int ipcam_stop(int argc, char **argv)
{
	nf_ipcam_stop();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_stop, "ipcam_stop", ipcam_stop_help, ipcam_stop_help);
#endif

static char openmode_scan_help[] = "om_scan";
static int openmode_scan(int argc, char **argv)
{
	nf_openmode_scan_camera();

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_scan, "om_scan", openmode_scan_help, openmode_scan_help);
#endif

static char openmode_init_list_help[] = "om_init_list";
static int openmode_init_list(int argc, char **argv)
{
	nf_openmode_init_detection_list();

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_init_list, "om_init_list", openmode_init_list_help, openmode_init_list_help);
#endif

static char openmode_empty_list_help[] = "om_empty_list";
static int openmode_empty_list(int argc, char **argv)
{
	nf_openmode_empty_list();

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_empty_list, "om_empty_list", openmode_empty_list_help, openmode_empty_list_help);
#endif

static char openmode_show_list_help[] = "om_show_list";
static int openmode_show_list(int argc, char **argv)
{
	nf_openmode_show_list();
	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_show_list, "om_show_list", openmode_show_list_help, openmode_show_list_help);
#endif

static char openmode_show_ch_list_help[] = "om_show_ch_list";
static int openmode_show_ch_list(int argc, char **argv)
{
	nf_openmode_get_ch_list();
	nf_openmode_show_ch_list();
	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_show_ch_list, "om_show_ch_list", openmode_show_ch_list_help, openmode_show_ch_list_help);
#endif

static char openmode_show_entry_help[] = "om_show_entry [name] [id]";
static int openmode_show_entry(int argc, char **argv)
{
	gint index = 0;

	if (argc != 3)
	{
		return (-1);
	}

	index = strtol(argv[2],NULL,0);
	nf_openmode_show_entry(argv[1], index);
	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_show_entry, "om_show_entry", openmode_show_entry_help, openmode_show_entry_help);
#endif

static char openmode_set_login_help[] = "om_set_login [id] [user] [pass]";
static int openmode_set_login(int argc, char **argv)
{
	gint index = 0;

	if (argc != 4)
	{
		return (-1);
	}

	index = strtol(argv[1],NULL,0);
	nf_openmode_set_login_info(index, argv[2], argv[3]);
	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_set_login, "om_set_login", openmode_set_login_help, openmode_set_login_help);
#endif

static char openmode_set_preview_help[] = "om_set_preview [id]";
static int openmode_set_preview(int argc, char **argv)
{
	gint index = 0;

	if (argc != 2)
	{
		return (-1);
	}

	index = strtol(argv[1],NULL,0);
	nf_openmode_set_preview(index, 0);
	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_set_preview, "om_set_preview", openmode_set_preview_help, openmode_set_preview_help);
#endif

static char openmode_set_ip_help[] = "om_set_ip [index] [ip]";
static int openmode_set_ip(int argc, char **argv)
{
	gint index = 0;
	guint ip;
	NFOpenmodeSetupNetwork info;

	if (argc != 3)
	{
		return (-1);
	}

	index = strtol(argv[1],NULL,0);
	ip = inet_addr(argv[2]);

	info.is_dhcp = 0;
	info.ipaddr = ntohl(ip);
	info.mask = 0xffffff00;
	info.gw = (info.ipaddr&0xffffff00)+0x1;
	info.dns1 = 0;
	info.dns2 = 0;

	nf_openmode_request_ip_assign(index, &info);

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_set_ip, "om_set_ip", openmode_set_ip_help, openmode_set_ip_help);
#endif

static char openmode_set_ch_help[] = "om_set_ch [index] [ch]";
static int openmode_set_ch(int argc, char **argv)
{
	gint index = 0;
	gint ch = 0;

	if (argc != 3)
	{
		return (-1);
	}
	index = strtol(argv[1],NULL,0);
	ch = strtol(argv[2],NULL,0);
	nf_openmode_set_channel(index, ch);

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_set_ch, "om_set_ch", openmode_set_ch_help, openmode_set_ch_help);
#endif

static char openmode_apply_help[] = "om_apply";
static int openmode_apply(int argc, char **argv)
{
	nf_openmode_apply();

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_apply, "om_apply", openmode_apply_help, openmode_apply_help);
#endif

static char openmode_cancel_help[] = "om_cancel";
static int openmode_cancel(int argc, char **argv)
{
	nf_openmode_cancel();

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_cancel, "om_cancel", openmode_cancel_help, openmode_cancel_help);
#endif

static char openmode_add_ip_help[] = "om_add_ip [ip] [port]";
static int openmode_add_ip(int argc, char **argv)
{
	guint port;


	if (argc != 3)
	{
		return (-1);
	}

	port = strtoul(argv[2],NULL,0);

	nf_openmode_add_device_manual(argv[1], port);

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_add_ip, "om_add_ip", openmode_add_ip_help, openmode_add_ip_help);
#endif


static char openmode_change_pw_help[] = "om_change_pw [id] [password]";
static int openmode_change_pw(int argc, char **argv)
{
	guint id;


	if (argc != 3)
	{
		return (-1);
	}

	id = strtoul(argv[1],NULL,0);

	nf_openmode_change_password(id, argv[2]);

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_change_pw, "om_change_pw", openmode_change_pw_help, openmode_change_pw_help);
#endif

static char openmode_set_port_help[] = "om_set_port [id] [http_port] [rtsp_port]";
static int openmode_set_port(int argc, char **argv)
{
	guint id;
	guint http;
	guint rtsp;
	NFOpenmodeSetupPorts info;


	if (argc != 4)
	{
		return (-1);
	}

	id = strtoul(argv[1],NULL,0);
	http = strtoul(argv[2],NULL,0);
	rtsp = strtoul(argv[3],NULL,0);

	info.http_port = http;
	info.rtsp_port = rtsp;
	nf_openmode_request_port_assign(id, &info);

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_set_port, "om_set_port", openmode_set_port_help, openmode_set_port_help);
#endif

static char openmode_get_network_help[] = "om_get_network [id]";
static int openmode_get_network(int argc, char **argv)
{
	guint id;
	NFOpenmodeSetupNetwork info;


	if (argc != 2)
	{
		return (-1);
	}

	id = strtoul(argv[1],NULL,0);

	memset(&info, 0x00, sizeof(NFOpenmodeSetupNetwork));
	nf_openmode_get_network(id, &info);
	printf("-----------------------------\n");
	printf("ip   %d.%d.%d.%d\n", (info.ipaddr&0xff000000)>>24, (info.ipaddr&0xff0000)>>16, (info.ipaddr&0xff00)>>8, (info.ipaddr&0xff));
	printf("mask %d.%d.%d.%d\n", (info.mask&0xff000000)>>24, (info.mask&0xff0000)>>16, (info.mask&0xff00)>>8, (info.mask&0xff));
	printf("gw   %d.%d.%d.%d\n", (info.gw&0xff000000)>>24, (info.gw&0xff0000)>>16, (info.gw&0xff00)>>8, (info.gw&0xff));
	printf("dns1 %d.%d.%d.%d\n", (info.dns1&0xff000000)>>24, (info.dns1&0xff0000)>>16, (info.dns1&0xff00)>>8, (info.dns2&0xff));
	printf("dns2 %d.%d.%d.%d\n", (info.dns2&0xff000000)>>24, (info.dns2&0xff0000)>>16, (info.dns2&0xff00)>>8, (info.dns2&0xff));
	printf("-----------------------------\n");

	return 0;
}
#ifdef IPCAM_OPENMODE_DEBUG
__commandlist(openmode_get_network, "om_get_network", openmode_get_network_help, openmode_get_network_help);
#endif


static char ipcam_poe_reboot_unittest_help[] = "ipcam_poe_reboot ch";
static int ipcam_poe_reboot_unittest(int argc, char **argv)
{
	int ch;

	if (argc != 2)
	{
		printf("%s\n", ipcam_poe_reboot_unittest_help);
		return (-1);
	}

	ch = strtoul(argv[1],NULL,0);

	int rtn = nf_ipcam_poe_reboot(ch, NULL, NULL, NULL);

	if(rtn == IPCAM_SETUP_RTN_DONE)
	{
		printf("IPCAM POE REBOOT SUCCESS!!\n");
	}
	else
	{
		printf("IPCAM POE REBOOT FAIL!!\n");
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_poe_reboot_unittest, "ipcam_poe_reboot", ipcam_poe_reboot_unittest_help, ipcam_poe_reboot_unittest_help);
#endif


static char ipcam_factory_default_help[] = "ipcam_factory_default [ch_mask]";
static int ipcam_factory_default(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i;

	if(argc < 2){
		printf("%s\n",ipcam_factory_default_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_factory_default(i,NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_factory_default,"ipcam_factory_default", ipcam_factory_default_help, ipcam_factory_default_help);
#endif


static char ipcam_get_encoder_capa_help[] = "ipcam_get_encoder_capa [ch]";
static int ipcam_get_encoder_capa(int argc, char **argv)
{
	guint ch = 0;
	NFIPCamEncoderCap info;

	if(argc < 2){
		printf("%s\n",ipcam_get_encoder_capa_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);

	memset(&info, 0x00, sizeof(NFIPCamEncoderCap));
	nf_ipcam_get_encoder_capability(ch, &info);

	IPCAM_DBG(MINOR, "res(%08x:%08x, %08x:%08x) max_fps(%d:%d, %d:%d) "
					"min_bitrate(%d:%d, %d:%d) max_bitrate(%d:%d, %d:%d)\n",
			info.res_support[0], info.res_default[0],
			info.res_support[1], info.res_default[1],
			info.fps_max[0], info.fps_max_default[0],
			info.fps_max[1], info.fps_max_default[1],
			info.bitrate_min[0], info.br_min_default[0],
			info.bitrate_min[1], info.br_min_default[1],
			info.bitrate_max[0], info.br_max_default[0],
			info.bitrate_max[1], info.br_max_default[1]);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_encoder_capa,"ipcam_get_encoder_capa", ipcam_get_encoder_capa_help, ipcam_get_encoder_capa_help);
#endif



static char ipcam_get_afcapa_help[] = "ipcam_get_afcapa [ch_mask]";
static int ipcam_get_afcapa(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i;
	cam_info temp;

	if(argc < 2){
		printf("%s\n",ipcam_get_afcapa_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			cam_get_af_capa(&temp, i);
			printf("mfz(%d) iris(%d) zmin(%d) zmax(%d) fmin(%d) fmax(%d) pmin(%d) pmax(%d)\n",
					temp.afcapa.mfz, temp.afcapa.iris,
					temp.afcapa.zoom_min, temp.afcapa.zoom_max,
					temp.afcapa.focus_min, temp.afcapa.focus_max,
					temp.afcapa.iris_min, temp.afcapa.iris_max);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_afcapa,"ipcam_get_afcapa", ipcam_get_afcapa_help, ipcam_get_afcapa_help);
#endif


static char ipcam_set_rec_help[] = "ipcam_set_rec [ch_mask] [1st_fps] [2nd_fps] [1st_quality] [2nd_quality]";
static int ipcam_set_rec(int argc, char **argv)
{
	guint ch_mask = 0;
	guint fps_1st = 300;
	guint fps_2nd = 300;
	guint quality_1st = 4;
	guint quality_2nd = 4;
	gint i;

	if(argc < 2){
		printf("%s\n",ipcam_set_rec_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	if(argc > 2)
		fps_1st = _ipcam_convert_fps(strtol(argv[2],NULL,0));

	if(argc > 3)
		fps_2nd = _ipcam_convert_fps(strtol(argv[3],NULL,0));

	if(argc > 4)
		quality_1st = strtol(argv[4],NULL,0);

	if(argc > 5)
		quality_2nd = strtol(argv[5],NULL,0);

	{
		NF_IPCAM_FPS_E fps1[NUM_IPX_CHANNEL];
		NF_IPCAM_FPS_E fps2[NUM_IPX_CHANNEL];
		NF_IPCAM_QUALITY_E qual1[NUM_IPX_CHANNEL];
		NF_IPCAM_QUALITY_E qual2[NUM_IPX_CHANNEL];

		for (i = 0; i < NUM_IPX_CHANNEL; i++)
		{
			fps1[i] = fps_1st;
			fps2[i] = fps_2nd;
			qual1[i] = quality_1st;
			qual2[i] = quality_2nd;
		}

		nf_ipcam_set_rec_vcodec_thread(ch_mask, fps1, fps2, qual1, qual2);
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_rec,"ipcam_set_rec", ipcam_set_rec_help, ipcam_set_rec_help);
#endif


static char ipcam_set_zoom_help[] = "ipcam_set_zoom [ch_mask] [value:1~100]";
static int ipcam_set_zoom(int argc, char **argv)
{
	guint ch_mask = 0;
	gint zoom_val = 0;
	gint i;

	if(argc < 3){
		printf("%s\n",ipcam_set_zoom_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	if(argc > 2)
		zoom_val = strtol(argv[2],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_set_zoom(i, zoom_val,NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_zoom,"ipcam_set_zoom", ipcam_set_zoom_help, ipcam_set_zoom_help);
#endif


static char ipcam_set_focus_help[] = "ipcam_set_focus [ch_mask] [value:1~100]";
static int ipcam_set_focus(int argc, char **argv)
{
	guint ch_mask = 0;
	gint focus_val = 0;
	gint i;

	if(argc < 3){
		printf("%s\n",ipcam_set_focus_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	if(argc > 2)
		focus_val = strtol(argv[2],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_set_focus(i, focus_val,NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_focus,"ipcam_set_focus", ipcam_set_focus_help, ipcam_set_focus_help);
#endif


static char ipcam_lens_default_help[] = "ipcam_lens_default [ch_mask]";
static int ipcam_lens_default(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i;

	if(argc < 2){
		printf("%s\n",ipcam_lens_default_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_set_lens_default(i, NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_lens_default,"ipcam_lens_default", ipcam_lens_default_help, ipcam_lens_default_help);
#endif


static char ipcam_af_help[] = "ipcam_af [ch_mask]";
static int ipcam_af(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i;

	if(argc < 2){
		printf("%s\n",ipcam_af_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_set_oneshot(i, NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_af,"ipcam_af", ipcam_af_help, ipcam_af_help);
#endif

static char ipcam_set_pmask_help[] = "ipcam_set_pmask [ch] [lt_x] [lt_y] [rb_x] [rb_y] [color]";
static int ipcam_set_pmask(int argc, char **argv)
{
	guint ch = 0;
	guint ltx;
	guint lty;
	guint rbx;
	guint rby;
	guint color;
	NFIPCamPrivacyMask info;

	ch = strtoul(argv[1],NULL,0);
	ltx = strtoul(argv[2],NULL,0);
	lty = strtoul(argv[3],NULL,0);
	rbx = strtoul(argv[4],NULL,0);
	rby = strtoul(argv[5],NULL,0);
	color = strtoul(argv[6],NULL,0);

	info.ch = ch;
	info.rect_cnt = 1;
	info.color[0] = color;
	info.lt[0].x = ltx;
	info.lt[0].y = lty;
	info.rb[0].x = rbx;
	info.rb[0].y = rby;

	if (argc > 7)
	{
		ltx = strtoul(argv[7],NULL,0);
		lty = strtoul(argv[8],NULL,0);
		rbx = strtoul(argv[9],NULL,0);
		rby = strtoul(argv[10],NULL,0);
		color = strtoul(argv[11],NULL,0);

		info.rect_cnt = 2;
		info.color[1] = color;
		info.lt[1].x = ltx;
		info.lt[1].y = lty;
		info.rb[1].x = rbx;
		info.rb[1].y = rby;
	}

	nf_ipcam_set_privacy_mask(ch, &info, NULL,NULL,NULL);

	return 0;
}
#if 0
__commandlist(ipcam_set_pmask,"ipcam_set_pmask", ipcam_set_pmask_help, ipcam_set_pmask_help);
#endif



static char ipcam_set_fps_help[] = "ipcam_set_fps [ch_mask] [1st_fps] [2nd_fps]";
static int ipcam_set_fps(int argc, char **argv)
{
	guint ch_mask = 0;
	guint fps_1st = 300;
	guint fps_2nd = 300;
	gint i;

	if(argc < 2){
		printf("%s\n",ipcam_set_fps_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	if(argc > 2)
		fps_1st = _ipcam_convert_fps(strtol(argv[2],NULL,0));

	if(argc > 3)
		fps_2nd = _ipcam_convert_fps(strtol(argv[3],NULL,0));

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_set_fps(i, fps_1st , fps_2nd ,NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_fps,"ipcam_set_fps", ipcam_set_fps_help, ipcam_set_fps_help);
#endif


static char ipcam_set_bitrate_help[] = "ipcam_set_bitrate [ch_mask] [1st_bitrate] [2nd_bitrate]";
static int ipcam_set_bitrate(int argc, char **argv)
{
	guint ch_mask = 0;
	guint bitrate_1st = 8000;
	guint bitrate_2nd = 1500;
	gint i;

	if(argc < 2){
		printf("%s\n",ipcam_set_bitrate_help); return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	if(argc > 2)
		bitrate_1st = strtoul(argv[2],NULL,0);

	if(argc > 3)
		bitrate_2nd = strtoul(argv[3],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_set_bitrate(i, bitrate_1st , bitrate_2nd ,NULL,NULL,NULL);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_bitrate,"ipcam_set_bitrate", ipcam_set_bitrate_help, ipcam_set_bitrate_help);
#endif

static char ipcam_preset_help[] = "ipcam_preset [ch] [cmd] [num]";
static int ipcam_preset(int argc, char **argv)
{
	guint ch = 0;
	guint cmd = 0;
	guint num = 0;

	if(argc < 3){
		printf("%s\n",ipcam_preset_help); return -1;
	}

	ch = strtoul(argv[1],NULL,0);
	cmd = strtoul(argv[2],NULL,0);
	num = strtoul(argv[3],NULL,0);


	if (cmd == 0) { nf_ipcam_set_preset(ch, num, NULL); }
	else if (cmd == 1) { nf_ipcam_clear_preset(ch, num, NULL); }
	else if (cmd == 2) { nf_ipcam_go_preset(ch, num, NULL); }

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_preset,"ipcam_preset", ipcam_preset_help, ipcam_preset_help);
#endif



static void show_option(NFIPCamOption* o)
{
	printf("category(%x) value(%x) selected(%d) dep(%x) enable(%x) disable(%x) %s\n",
		o->category, o->value, o->selected, o->dependent_category,
		o->enable_category, o->disable_category, o->caption);
}
static void show_option2(NFIPCamOption_onvif* o)
{
	printf("category(%x) value(%x) selected(%d) dep(%x) enable(%x) disable(%x) visible(%x) invisible(%x) %s\n",
		o->category, o->value, o->selected, o->dependent_category,
		o->enable_category, o->disable_category, o->visible_category, o->invisible_category, o->caption);
}
static void show_value(NFIPCamValue* v)
{
	printf("category(%x) value(%d) min(%d) max(%d) dep(%x)\n",
		v->category, v->value, v->min, v->max, v->dependent_category);
}

static char ipcam_get_image_profile_help[] = "ipcam_get_image_profile [ch_mask]";
static int ipcam_get_image_profile(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i,j;
	NFIPCamImageProfile ps;

	if(argc < 2){
		printf("%s\n",ipcam_get_image_profile_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_get_image_profile(i, &ps);

			printf("[%s] CH(%d) supported(%x)\n", __FUNCTION__, i, ps.supported);
			printf("[%s] SHARPNESS       : ", __FUNCTION__);
			show_value(&ps.sharpness);
			printf("[%s] AGC             : ", __FUNCTION__);
			show_value(&ps.agc);
			printf("[%s] E-SHUTTER SPEED : ", __FUNCTION__);
			show_value(&ps.eshutter);
			for (j=0; j<ps.exposure_cnt; j++)
			{
				printf("[%s] EXPOSURE[%d] ", __FUNCTION__, j);
				show_option(&ps.exposure[j]);
			}
			for (j=0; j<ps.slowshutter_cnt; j++)
			{
				printf("[%s] SLOW SHUTTER[%d] ", __FUNCTION__, j);
				show_option(&ps.slowshutter[j]);
			}
			for (j=0; j<ps.maxagc_cnt; j++)
			{
				printf("[%s] MAX AGC[%d] ", __FUNCTION__, j);
				show_option(&ps.maxagc[j]);
			}
			for (j=0; j<ps.blc_cnt; j++)
			{
				printf("[%s] BLC[%d] ", __FUNCTION__, j);
				show_option(&ps.blc[j]);
			}
			for (j=0; j<ps.wb_cnt; j++)
			{
				printf("[%s] WB[%d] ", __FUNCTION__, j);
				show_option(&ps.wb[j]);
			}
			for (j=0; j<ps.mwb_cnt; j++)
			{
				printf("[%s] MWB[%d] ", __FUNCTION__, j);
				show_option(&ps.mwb[j]);
			}
			for (j=0; j<ps.dnn_cnt; j++)
			{
				printf("[%s] DNN[%d] ", __FUNCTION__, j);
				show_option(&ps.dnn[j]);
			}
			for (j=0; j<ps.dnn_toggle_cnt; j++)
			{
				printf("[%s] DNN TOGGLE[%d] ", __FUNCTION__, j);
				show_option(&ps.dnn_toggle[j]);
			}
			for (j=0; j<ps.dc_iris_cnt; j++)
			{
				printf("[%s] DC-IRIS[%d] ", __FUNCTION__, j);
				show_option(&ps.dc_iris[j]);
			}
			for (j=0; j<ps.p_iris_cnt; j++)
			{
				printf("[%s] P-IRIS[%d] ", __FUNCTION__, j);
				show_option(&ps.p_iris[j]);
			}
			for(j = 0; j < ps.mirror_cnt; j++)
			{
				printf("[%s] MIRROR[%d] ", __FUNCTION__, j);
				show_option(&ps.mirror[j]);
			}
		}
	}

	return 0;
}
#if 0
__commandlist(ipcam_get_image_profile,"ipcam_get_image_profile", ipcam_get_image_profile_help, ipcam_get_image_profile_help);
#endif


static char ipcam_get_image_profile_onvif_help[] = "ipcam_get_image_profile_onvif [ch_mask]";
static int ipcam_get_image_profile_onvif(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i,j;

	if(argc < 2){
		printf("%s\n",ipcam_get_image_profile_onvif_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			NFIPCamImageProfile_onvif ps;
			NFIPCamExposureProfile_onvif ep;

			nf_ipcam_get_image_profile_onvif(i, &ps);

			printf("==== CH(%d) support (%x) ==================\n", i, ps.supported_image);
			printf("  Brightness     : ");
			show_value(&ps.brightness);
			printf("  Contrast       : ");
			show_value(&ps.contrast);
			printf("  Color          : ");
			show_value(&ps.color);
			printf("  Sharpness      : ");
			show_value(&ps.sharpness);
			printf("  Tint           : ");
			show_value(&ps.tint);

			for(j = 0; j < ps.focus_cnt; j++)
			{
				printf(" Focus (%d)      : ", j);
				show_option2(&ps.focus[j]);
			}
			printf("  Focus.defaultspeed : ");
			show_value(&ps.defaultspeed);
			printf("  Focus.Nearlimit    : ");
			show_value(&ps.nearlimit);
			printf("  Focus.Farlimit     : ");
			show_value(&ps.farlimit);

			printf("  Focus.abPosition   : ");
			show_value(&ps.abposition);
			printf("  Focus.abSpeed      : ");
			show_value(&ps.abspeed);
			printf("  Focus.reDistance   : ");
			show_value(&ps.redistance);
			printf("  Focus.reSpeed      : ");
			show_value(&ps.respeed);
			printf("  Focus.coSpeed      : ");
			show_value(&ps.cospeed);
			for(j = 0; j < ps.wb_cnt; j++)
			{
				printf(" wb        (%d)  : ", j);
				show_option2(&ps.wb[j]);
			}
			for(j = 0; j < ps.mwb_cnt; j++)
			{
				printf(" manual wb (%d)  : ", j);
				show_option2(&ps.mwb[j]);
			}
			for(j = 0; j < ps.mirror_cnt; j++)
			{
				printf(" mirror    (%d)  : ", j);
				show_option2(&ps.mirror[j]);
			}

			nf_ipcam_get_exposure_profile_onvif(i, &ep);

			printf("==== CH(%d) exposure (%x) ==================\n", i, ep.supported_exposure);
			for(j = 0; j <ep.mode_cnt; j++)
			{
				printf(" expo mode (%d)  : ", j);
				show_option2(&ep.mode[j]);
			}
			for(j = 0; j <ep.priority_cnt; j++)
			{
				printf(" exp priority(%d): ", j);
				show_option2(&ep.priority[j]);
			}
			printf("  Gain         : ");
			show_value(&ep.gain);
			printf("  ShutterSpeed : ");
			show_value(&ep.etime);
			printf("  Iris         : ");
			show_value(&ep.iris);
			printf("  minGain      : ");
			show_value(&ep.mingain);
			printf("  maxGain      : ");
			show_value(&ep.maxgain);
			printf("  mineTime     : ");
			show_value(&ep.minetime);
			printf("  maxeTime     : ");
			show_value(&ep.maxetime);
			printf("  minIris      : ");
			show_value(&ep.miniris);
			printf("  maxIris      : ");
			show_value(&ep.maxiris);
			for(j = 0; j <ep.slowshutter_cnt; j++)
			{
				printf(" slowshutter (%d)  : ", j);
				show_option2(&ep.slowshutter[j]);
			}
			for(j = 0; j <ep.maxagc_cnt; j++)
			{
				printf(" max agc     (%d)  : ", j);
				show_option2(&ep.maxagc[j]);
			}
			for(j = 0; j <ep.dc_iris_cnt; j++)
			{
				printf(" dc iris     (%d)  : ", j);
				show_option2(&ep.dc_iris[j]);
			}
			for(j = 0; j < ep.blc_cnt; j++)
			{
				printf(" blc         (%d)  : ", j);
				show_option2(&ep.blc[j]);
			}
			printf("  blc level     : ");
			show_value(&ep.blclevel);
			for(j = 0; j < ep.antiflicker_cnt; j++)
			{
				printf(" af          (%d)  : ", j);
				show_option2(&ep.antiflicker[j]);
			}
			for(j = 0; j < ep.max_shutter_50_cnt; j++)
			{
				printf(" maxshuter50 (%d)  : ", j);
				show_option2(&ep.max_shutter_50[j]);
			}
			for(j = 0; j < ep.max_shutter_60_cnt; j++)
			{
				printf(" maxshuter60 (%d)  : ", j);
				show_option2(&ep.max_shutter_60[j]);
			}
			for(j = 0; j < ep.max_shutter_off_cnt; j++)
			{
				printf(" maxshuteroff(%d)  : ", j);
				show_option2(&ep.max_shutter_off[j]);
			}
			for(j = 0; j < ep.wdr_cnt; j++)
			{
				printf(" wdr       (%d)  : ", j);
				show_option2(&ep.wdr[j]);
			}
			printf("  wdr.level          : ");
			show_value(&ep.wdrlevel);
			for(j = 0; j < ep.dnr_cnt; j++)
			{
				printf(" dnr(%d) : ", j);
				show_option2(&ep.dnr[j]);
			}
			for(j = 0; j < ep.ircut_cnt; j++)
			{
				printf(" Ircut (%d)      : ", j);
				show_option2(&ep.ircut[j]);
			}
			for(j = 0; j < ep.dnn_toggle_cnt; j++)
			{
				printf(" Dn toggle (%d)  : ", j);
				show_option2(&ep.dnn_toggle[j]);
			}


		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_image_profile_onvif,"ipcam_get_image_profile_onvif", ipcam_get_image_profile_onvif_help, ipcam_get_image_profile_onvif_help);
#endif

static char ipcam_get_fps_help[] = "ipcam_get_fps [ch] [stream]";
static int ipcam_get_fps(int argc, char **argv)
{
	guint ch = 0;
	guint stream = 0;
	guint capable=0;
	guint value=0;

	if (argc != 3)
	{
		printf("arg error\n");
		return 0;
	}

	ch = strtoul(argv[1],NULL,0);
	stream = strtoul(argv[2],NULL,0);

	nf_ipcam_get_fps(ch,stream,&capable,&value,NULL);
	printf("capable(%08x) value(%08x)\n", capable,value);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_fps,"ipcam_get_fps", ipcam_get_fps_help, ipcam_get_fps_help);
#endif



static char ipcam_send_audio_help[] = "ipcam_send_audio [ch_num]";
static int ipcam_send_audio(int argc, char **argv)
{
	gchar *p = NULL;
	//gchar aa[500000];
	gint ch = (-1);
	gint send_bytes = 0;
	gint i;
	mtable *runtime = get_runtime();
	NFIPCamAudioRaw audio;
	FILE *fp = NULL;
	gint a = 0;

	if(argc < 2){
		printf("%s\n",ipcam_send_audio_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);

	audio.ch = ch;
	audio.type = NF_IPCAM_SEND_AUDIO_START;
	audio.buf = NULL;
	if (nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL) == IPCAM_SETUP_RTN_FAILED)
	{
		return 0;
	}

	fp = fopen("/home/root/out.audio", "rb");
	if (fp == NULL)
	{
		g_assert(0);
	}
	for (a = 0; a < 5000; a++)
	{
		audio.ch = ch;
		audio.type = NF_IPCAM_SEND_AUDIO_START;
		audio.buf = NULL;
		fseek(fp, 0, SEEK_SET);
		if (nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL) == IPCAM_SETUP_RTN_FAILED)
		{
			return 0;
		}

		for (i=0; i < 30;i++)
		{
			audio.ch = ch;
			audio.type = NF_IPCAM_SEND_AUDIO_DATA;
			audio.buf = gobj_buddy_buffer_new_malloc(4000);
			p = gobj_buddy_buffer_buf_get_addr(audio.buf);
			fread(p, 1, 4000, fp);
			if (nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL) == IPCAM_SETUP_RTN_FAILED)
			{
				g_object_unref(audio.buf);
				return 0;
			}
			g_object_unref(audio.buf);
			//usleep(500*1000);
			usleep(1000*1000);
		}
		for (i=0; i < 200;i++)
		{
			audio.ch = ch;
			audio.type = NF_IPCAM_SEND_AUDIO_DATA;
			audio.buf = gobj_buddy_buffer_new_malloc(1200);
			p = gobj_buddy_buffer_buf_get_addr(audio.buf);
			fread(p, 1, 1200, fp);
			if (nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL) == IPCAM_SETUP_RTN_FAILED)
			{
				g_object_unref(audio.buf);
				return 0;
			}
			g_object_unref(audio.buf);
			//usleep(150*1000);
			usleep(300*1000);
		}

		audio.ch = ch;
		audio.type = NF_IPCAM_SEND_AUDIO_END;
		audio.buf = NULL;
		if (nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL) == IPCAM_SETUP_RTN_FAILED)
		{
			return 0;
		}
	}
	fclose(fp);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_send_audio,"ipcam_send_audio", ipcam_send_audio_help, ipcam_send_audio_help);
#endif



static char ipcam_show_runtime_help[] = "ipcam_show_runtime [ch_num]";
static int ipcam_show_runtime(int argc, char **argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();

	if(argc < 2){
		printf("%s\n",ipcam_show_runtime_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);

	printf("============================ runtime[%d] ============================\n", ch);
	printf(" \033[1;4mConnection\033[0m related\n");
	printf("    state                  - %08x\n", runtime[ch].state);
	printf("    ipaddr                 - %d.%d.%d.%d\n",
			(runtime[ch].sys.ipaddr&0xff),
			(runtime[ch].sys.ipaddr&0xff00)>>8,
			(runtime[ch].sys.ipaddr&0xff0000)>>16,
			(runtime[ch].sys.ipaddr&0xff000000)>>24);
	printf("    macaddr                - %02x:%02x:%02x:%02x:%02x:%02x\n",
			runtime[ch].sys.macaddr[0],runtime[ch].sys.macaddr[1],
			runtime[ch].sys.macaddr[2],runtime[ch].sys.macaddr[3],
			runtime[ch].sys.macaddr[4],runtime[ch].sys.macaddr[5]);
	printf("    transaction            - %08x\n", runtime[ch].sys.transaction);
	printf("    retry_cnt              - %d\n", runtime[ch].sys.retry_cnt);
	printf("    progress               - %d\n", runtime[ch].sys.progress);
	printf("    rtsp url[0]            - %s\n", runtime[ch].sys.rtsp_url[0]);
	printf("    rtsp url[1]            - %s\n", runtime[ch].sys.rtsp_url[1]);
	printf("    rtsp port              - %u,%u,%u\n", runtime[ch].sys.rtsp_port[0],runtime[ch].sys.rtsp_port[1],runtime[ch].sys.rtsp_port[2]);
	printf("    http port              - %u\n", runtime[ch].sys.http_port);
	printf("    ssl                    - %u\n", runtime[ch].sys.use_ssl);
	printf("    vnet id                - %d\n", runtime[ch].onvif.vnet_id);
	printf(" \033[1;4mModel\033[0m related\n");
	printf("    profile_id             - %d\n", runtime[ch].profile_id);
	printf("    model_code             - %d\n", runtime[ch].sys.model_code);
	printf("    Model name             - %s\n", runtime[ch].sys.model);
	printf("    F/W version            - %s\n", runtime[ch].sys.swver);
	printf("    F/W version2           - %s\n", runtime[ch].sys.swver2);
	printf("    vendor                 - %s\n", runtime[ch].sys.vendor);
	printf("    username               - %s\n", runtime[ch].username);
	printf("    password               - %s\n", runtime[ch].password);
	printf("    Function support mask  - %08x\n", runtime[ch].func);
	printf(" \033[1;4mVideo\033[0m related\n");
	printf("    Video codec[0]         - %08x\n", runtime[ch].video.vcodec[0]);
	printf("    Video resolution[0]    - %08x\n", runtime[ch].video.resolution.resolution[0]);
	printf("    Video fps[0]           - %08x %08x\n", runtime[ch].video.fps[0][0].value, runtime[ch].video.fps[1][0].value);
	printf("    Video bitrate[0]       - %u\n",   runtime[ch].video.bitrate[0].value);
	printf("    Video codec[1]         - %08x\n", runtime[ch].video.vcodec[1]);
	printf("    Video resolution[1]    - %08x\n", runtime[ch].video.resolution.resolution[1]);
	printf("    Video fps[1]           - %08x %08x\n", runtime[ch].video.fps[0][1].value, runtime[ch].video.fps[1][1].value);
	printf("    Video bitrate[1]       - %u\n",   runtime[ch].video.bitrate[1].value);
	printf("    Video codec[2]         - %08x\n", runtime[ch].video.vcodec[2]);
	printf("    Video resolution[2]    - %08x\n", runtime[ch].video.resolution.resolution[2]);
	printf("    Video fps[2]           - %08x %08x\n", runtime[ch].video.fps[0][2].value, runtime[ch].video.fps[1][2].value);
	printf("    Video bitrate[2]       - %u\n",   runtime[ch].video.bitrate[2].value);
	printf("    Quality table[0]       - [%u,%u,%u,%u,%u]\n",
			runtime[ch].video.quality[0][0], runtime[ch].video.quality[0][1],
			runtime[ch].video.quality[0][2], runtime[ch].video.quality[0][3],
			runtime[ch].video.quality[0][4]);
	printf("    Quality table[1]       - [%u,%u,%u,%u,%u]\n",
			runtime[ch].video.quality[1][0], runtime[ch].video.quality[1][1],
			runtime[ch].video.quality[1][2], runtime[ch].video.quality[1][3],
			runtime[ch].video.quality[1][4]);
	printf("    Anti-flicker           - %u\n", runtime[ch].video.anti_flicker.value);
	printf("    Mirror                 - %08x\n", runtime[ch].video.mirror.value);
	printf(" \033[1;4mAudio\033[0m related\n");
	printf("    Audio codec            - %08x\n", runtime[ch].audio.acodec.value);
	printf("    Rx enable              - %d\n", runtime[ch].audio.audio_rx);
	printf("    Tx enable              - %d\n", runtime[ch].audio.audio_tx);
	printf("    Mic volume             - %lu\n", runtime[ch].audio.mic_volume.value);
	printf("    Speaker volume         - %lu\n", runtime[ch].audio.speaker_volume.value);
	printf(" \033[1;4mEvent\033[0m related\n");
	printf("    Alarm in               - %lu\n", runtime[ch].alarm.alarm_in);
	printf("    Alarm in type          - %x\n", runtime[ch].alarm.alarm_in_type.value);
	printf("    Alarm out              - %lu\n", runtime[ch].alarm.alarm_out);
	printf("    Alarm out type         - %x\n", runtime[ch].alarm.alarm_out_type.value);
	printf(" \033[1;4mPTZ/IRIS\033[0m related\n");
	printf("    ptz support            - %08x\n", runtime[ch].ptz.supported);
	printf("    Pan                    - %d [%d~%d]\n",
			runtime[ch].ptz.pan.value, runtime[ch].ptz.pan.min, runtime[ch].ptz.pan.max);
	printf("    Tilt                   - %d [%d~%d]\n",
			runtime[ch].ptz.tilt.value, runtime[ch].ptz.tilt.min, runtime[ch].ptz.tilt.max);
	printf("    Zoom                   - %d [%d~%d]\n",
			runtime[ch].ptz.zoom.value, runtime[ch].ptz.zoom.min, runtime[ch].ptz.zoom.max);
	printf("    Focus                  - %d [%d~%d]\n",
			runtime[ch].ptz.focus.value, runtime[ch].ptz.focus.min, runtime[ch].ptz.focus.max);
	printf("    Iris                   - %d [%d~%d]\n",
			runtime[ch].ptz.iris.value, runtime[ch].ptz.iris.min, runtime[ch].ptz.iris.max);
	printf(" ONVIF                          \n");
//	printf("    profile token          - %s\n", runtime[ch].onvif.profile_token);
	printf("    profile token[0]       - %s\n", runtime[ch].onvif.profile_token[0]);
	printf("    profile token[1]       - %s\n", runtime[ch].onvif.profile_token[1]);
	printf("    width+height           - %d X %d\n", runtime[ch].onvif.width, runtime[ch].onvif.height);
	printf("    video token            - %s\n", runtime[ch].onvif.vsc_token);
	printf("    video encoder token[0] - %s\n", runtime[ch].onvif.vec_token[0]);
	printf("    video encoder token[1] - %s\n", runtime[ch].onvif.vec_token[1]);
	printf("    xaddr DEVICE           - %s\n", runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_DEVICE]);
	printf("    xaddr MEDIA            - %s\n", runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_MEDIA]);
	printf("    xaddr IMAGE            - %s\n", runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_IMAGE]);
	printf("    xaddr EVENT            - %s\n", runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_EVENT]);
	printf("    xaddr PTZ              - %s\n", runtime[ch].onvif.xaddr[NF_ONVIF_SERVICE_PTZ]);
	printf("=====================================================================\n");



	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_show_runtime,"ipcam_show_runtime", ipcam_show_runtime_help, ipcam_show_runtime_help);
#endif

static char ipcam_show_onvif_img_help[] = ">>> ipcam_show_onvif_img [ch_num]";
static int ipcam_show_onvif_img(int argc, char**argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();

	if(argc < 2){
		printf("%s\n",ipcam_show_onvif_img_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);

	if(argc > 2)
	{
		nf_onvif_get_image_t(ch);
		nf_onvif_get_image_t_value(ch);
	}

	printf("============================ runtime[%d] ============================\n", ch);
	printf("ImagingSetting : \n");
	printf(" Support                  - %08x\n", runtime[ch].image_onvif.supported_image);
	printf(" Brightness               - [%d ~ %d] , %d\n", runtime[ch].image_onvif.brightness.min, runtime[ch].image_onvif.brightness.max, runtime[ch].image_onvif.brightness.value);
	printf(" Contrast                 - [%d ~ %d] , %d\n", runtime[ch].image_onvif.contrast.min, runtime[ch].image_onvif.contrast.max, runtime[ch].image_onvif.contrast.value);
	printf(" Color                    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.color.min, runtime[ch].image_onvif.color.max, runtime[ch].image_onvif.color.value);
	printf(" Sharpness                - [%d ~ %d] , %d\n", runtime[ch].image_onvif.sharpness.min, runtime[ch].image_onvif.sharpness.max, runtime[ch].image_onvif.sharpness.value);
	printf(" Focus                    - [%d in %x]\n", runtime[ch].image_onvif.focus.mode.value, runtime[ch].image_onvif.focus.mode.support);
	printf(" Focus.DefaultSpeed       - [%d ~ %d] , %d\n", runtime[ch].image_onvif.focus.defaultspeed.min, runtime[ch].image_onvif.focus.defaultspeed.max, runtime[ch].image_onvif.focus.defaultspeed.value);
	printf(" Focus.NearLimit          - [%d ~ %d] , %d\n", runtime[ch].image_onvif.focus.nearlimit.min, runtime[ch].image_onvif.focus.nearlimit.max, runtime[ch].image_onvif.focus.nearlimit.value);
	printf(" Focus.FarLimit           - [%d ~ %d] , %d\n", runtime[ch].image_onvif.focus.farlimit.min, runtime[ch].image_onvif.focus.farlimit.max, runtime[ch].image_onvif.focus.farlimit.value);
	printf(" IrCutFilter              - [%d in %x]\n", runtime[ch].image_onvif.ircut.value, runtime[ch].image_onvif.ircut.support);
	printf(" WideDynamicRange         - [%d in %x]\n", runtime[ch].image_onvif.wdrmode.value, runtime[ch].image_onvif.wdrmode.support);
	printf(" WideDynamicRange.level   - [%d ~ %d] , %d\n", runtime[ch].image_onvif.wdrlevel.min, runtime[ch].image_onvif.wdrlevel.max, runtime[ch].image_onvif.wdrlevel.value);
	printf(" BacklightCompensation    - [%d in %x]\n", runtime[ch].image_onvif.blcmode.value, runtime[ch].image_onvif.blcmode.support);
	printf(" BLC.level                - [%d ~ %d] , %d\n", runtime[ch].image_onvif.blclevel.min, runtime[ch].image_onvif.blclevel.max, runtime[ch].image_onvif.blclevel.value);
	printf(" WhiteBalance             - [%d in %x]\n", runtime[ch].image_onvif.wb.mode.value, runtime[ch].image_onvif.wb.mode.support);
	printf(" WhiteBalance.CrGain      - [%d ~ %d] , %d\n", runtime[ch].image_onvif.wb.crgain.min, runtime[ch].image_onvif.wb.crgain.max, runtime[ch].image_onvif.wb.crgain.value);
	printf(" WhiteBalance.CbGain      - [%d ~ %d] , %d\n", runtime[ch].image_onvif.wb.cbgain.min, runtime[ch].image_onvif.wb.cbgain.max, runtime[ch].image_onvif.wb.cbgain.value);
	printf(" Exposure                 - [%d in %x]\n", runtime[ch].image_onvif.exposure.mode.value, runtime[ch].image_onvif.exposure.mode.support);
	printf(" Exposure.support         - %08x\n", runtime[ch].image_onvif.supported_exposure);
	printf(" Exposure.Priority        - [%d in %x]\n", runtime[ch].image_onvif.exposure.priority.value, runtime[ch].image_onvif.exposure.priority.support);
	printf(" Exposure.MinExposureTime - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.minetime.min, runtime[ch].image_onvif.exposure.minetime.max, runtime[ch].image_onvif.exposure.minetime.value);
	printf(" Exposure.MaxExposureTime - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.maxetime.min, runtime[ch].image_onvif.exposure.maxetime.max, runtime[ch].image_onvif.exposure.maxetime.value);
	printf(" Exposure.ExposureTime    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.etime.min, runtime[ch].image_onvif.exposure.etime.max, runtime[ch].image_onvif.exposure.etime.value);
	printf(" Exposure.MinGain         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.mingain.min, runtime[ch].image_onvif.exposure.mingain.max, runtime[ch].image_onvif.exposure.mingain.value);
	printf(" Exposure.MaxGain         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.maxgain.min, runtime[ch].image_onvif.exposure.maxgain.max, runtime[ch].image_onvif.exposure.maxgain.value);
	printf(" Exposure.Gain            - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.gain.min, runtime[ch].image_onvif.exposure.gain.max, runtime[ch].image_onvif.exposure.gain.value);
	printf(" Exposure.MinIris         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.miniris.min, runtime[ch].image_onvif.exposure.miniris.max, runtime[ch].image_onvif.exposure.miniris.value);
	printf(" Exposure.MaxIris         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.maxiris.min, runtime[ch].image_onvif.exposure.maxiris.max, runtime[ch].image_onvif.exposure.maxiris.value);
	printf(" Exposure.Iris            - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.iris.min, runtime[ch].image_onvif.exposure.iris.max, runtime[ch].image_onvif.exposure.iris.value);
	printf("MoveOption : \n");
	printf(" Move                     - [%d in %x]\n", runtime[ch].image_onvif.move.mode.value, runtime[ch].image_onvif.move.mode.support);
	printf(" Move.AbsolutePosition    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.abposition.min, runtime[ch].image_onvif.move.abposition.max, runtime[ch].image_onvif.move.abposition.value);
	printf(" Move.AbsoluteSpeed       - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.abspeed.min, runtime[ch].image_onvif.move.abspeed.max, runtime[ch].image_onvif.move.abspeed.value);
	printf(" Move.RelativeDistance    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.redistance.min, runtime[ch].image_onvif.move.redistance.max, runtime[ch].image_onvif.move.redistance.value);
	printf(" Move.RelativeSpeed       - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.respeed.min, runtime[ch].image_onvif.move.respeed.max, runtime[ch].image_onvif.move.respeed.value);
	printf(" Move.ContinuousSpeed     - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.cospeed.min, runtime[ch].image_onvif.move.cospeed.max, runtime[ch].image_onvif.move.cospeed.value);
	printf("=====================================================================\n");

	return 0;
}
#if 0
__commandlist(ipcam_show_onvif_img,"ipcam_show_onvif_img", ipcam_show_onvif_img_help, ipcam_show_onvif_img_help);
#endif

static char ipcam_set_onvif_img_help[] = ">>> ipcam_set_onvif_img [ch_num]";
static int ipcam_set_onvif_img(int argc, char**argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();

	if(argc < 2){
		printf("%s\n",ipcam_show_onvif_img_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);

	NFIPCamSetupImage_onvif image_onvif;
	memset(&image_onvif, 0x00, sizeof(NFIPCamSetupImage_onvif));
	image_onvif.brightness = 30;
	image_onvif.contrast = 30;
	image_onvif.color = 50;
	image_onvif.sharpness = 5;

	//image_onvif.white_balance = NF_IPCAM_WB_MODE_ONVIF_AUTO;
	//image_onvif.wide_dynamic_mode = NF_IPCAM_WDR_MODE_ONVIF_OFF;
	//image_onvif.wide_level = 0;
	//image_onvif.ircut = 1;

	NFIPCamSetupExposure_onvif exp_onvif;
	memset(&exp_onvif, 0x00, sizeof(NFIPCamSetupExposure_onvif));
	exp_onvif.mode = NF_IPCAM_EXPOSURE_MODE_ONVIF_MANUAL;
	exp_onvif.etime = 100;

	nf_ipcam_set_image_exp_onvif(ch, &image_onvif, &exp_onvif, NULL, NULL, NULL);

	nf_onvif_get_image_t_value(ch);

	printf("============================ runtime[%d] ============================\n", ch);
	printf("ImagingSetting : \n");
	printf(" Support                  - %08x\n", runtime[ch].image_onvif.supported_image);
	printf(" Brightness               - [%d ~ %d] , %d\n", runtime[ch].image_onvif.brightness.min, runtime[ch].image_onvif.brightness.max, runtime[ch].image_onvif.brightness.value);
	printf(" Contrast                 - [%d ~ %d] , %d\n", runtime[ch].image_onvif.contrast.min, runtime[ch].image_onvif.contrast.max, runtime[ch].image_onvif.contrast.value);
	printf(" Color                    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.color.min, runtime[ch].image_onvif.color.max, runtime[ch].image_onvif.color.value);
	printf(" Sharpness                - [%d ~ %d] , %d\n", runtime[ch].image_onvif.sharpness.min, runtime[ch].image_onvif.sharpness.max, runtime[ch].image_onvif.sharpness.value);
	printf(" Focus                    - [%d in %x]\n", runtime[ch].image_onvif.focus.mode.value, runtime[ch].image_onvif.focus.mode.support);
	printf(" Focus.DefaultSpeed       - [%d ~ %d] , %d\n", runtime[ch].image_onvif.focus.defaultspeed.min, runtime[ch].image_onvif.focus.defaultspeed.max, runtime[ch].image_onvif.focus.defaultspeed.value);
	printf(" Focus.NearLimit          - [%d ~ %d] , %d\n", runtime[ch].image_onvif.focus.nearlimit.min, runtime[ch].image_onvif.focus.nearlimit.max, runtime[ch].image_onvif.focus.nearlimit.value);
	printf(" Focus.FarLimit           - [%d ~ %d] , %d\n", runtime[ch].image_onvif.focus.farlimit.min, runtime[ch].image_onvif.focus.farlimit.max, runtime[ch].image_onvif.focus.farlimit.value);
	printf(" IrCutFilter              - [%d in %x]\n", runtime[ch].image_onvif.ircut.value, runtime[ch].image_onvif.ircut.support);
	printf(" BacklightCompensation    - [%d in %x]\n", runtime[ch].image_onvif.blcmode.value, runtime[ch].image_onvif.blcmode.support);
	printf(" BLC.level                - [%d ~ %d] , %d\n", runtime[ch].image_onvif.blclevel.min, runtime[ch].image_onvif.blclevel.max, runtime[ch].image_onvif.blclevel.value);
	printf(" WideDynamicRange         - [%d in %x]\n", runtime[ch].image_onvif.wdrmode.value, runtime[ch].image_onvif.wdrmode.support);
	printf(" WideDynamicRange.level   - [%d ~ %d] , %d\n", runtime[ch].image_onvif.wdrlevel.min, runtime[ch].image_onvif.wdrlevel.max, runtime[ch].image_onvif.wdrlevel.value);
	printf(" WhiteBalance             - [%d in %x]\n", runtime[ch].image_onvif.wb.mode.value, runtime[ch].image_onvif.wb.mode.support);
	printf(" WhiteBalance.CrGain      - [%d ~ %d] , %d\n", runtime[ch].image_onvif.wb.crgain.min, runtime[ch].image_onvif.wb.crgain.max, runtime[ch].image_onvif.wb.crgain.value);
	printf(" WhiteBalance.CbGain      - [%d ~ %d] , %d\n", runtime[ch].image_onvif.wb.cbgain.min, runtime[ch].image_onvif.wb.cbgain.max, runtime[ch].image_onvif.wb.cbgain.value);
	printf(" Exposure                 - [%d in %x]\n", runtime[ch].image_onvif.exposure.mode.value, runtime[ch].image_onvif.exposure.mode.support);
	printf(" Exposure.support         - %08x\n", runtime[ch].image_onvif.supported_exposure);
	printf(" Exposure.Priority        - [%d in %x]\n", runtime[ch].image_onvif.exposure.priority.value, runtime[ch].image_onvif.exposure.priority.support);
	printf(" Exposure.MinExposureTime - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.minetime.min, runtime[ch].image_onvif.exposure.minetime.max, runtime[ch].image_onvif.exposure.minetime.value);
	printf(" Exposure.MaxExposureTime - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.maxetime.min, runtime[ch].image_onvif.exposure.maxetime.max, runtime[ch].image_onvif.exposure.maxetime.value);
	printf(" Exposure.ExposureTime    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.etime.min, runtime[ch].image_onvif.exposure.etime.max, runtime[ch].image_onvif.exposure.etime.value);
	printf(" Exposure.MinGain         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.mingain.min, runtime[ch].image_onvif.exposure.mingain.max, runtime[ch].image_onvif.exposure.mingain.value);
	printf(" Exposure.MaxGain         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.maxgain.min, runtime[ch].image_onvif.exposure.maxgain.max, runtime[ch].image_onvif.exposure.maxgain.value);
	printf(" Exposure.Gain            - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.gain.min, runtime[ch].image_onvif.exposure.gain.max, runtime[ch].image_onvif.exposure.gain.value);
	printf(" Exposure.MinIris         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.miniris.min, runtime[ch].image_onvif.exposure.miniris.max, runtime[ch].image_onvif.exposure.miniris.value);
	printf(" Exposure.MaxIris         - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.maxiris.min, runtime[ch].image_onvif.exposure.maxiris.max, runtime[ch].image_onvif.exposure.maxiris.value);
	printf(" Exposure.Iris            - [%d ~ %d] , %d\n", runtime[ch].image_onvif.exposure.iris.min, runtime[ch].image_onvif.exposure.iris.max, runtime[ch].image_onvif.exposure.iris.value);
	printf("MoveOption : \n");
	printf(" Move                     - [%d in %x]\n", runtime[ch].image_onvif.move.mode.value, runtime[ch].image_onvif.move.mode.support);
	printf(" Move.AbsolutePosition    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.abposition.min, runtime[ch].image_onvif.move.abposition.max, runtime[ch].image_onvif.move.abposition.value);
	printf(" Move.AbsoluteSpeed       - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.abspeed.min, runtime[ch].image_onvif.move.abspeed.max, runtime[ch].image_onvif.move.abspeed.value);
	printf(" Move.RelativeDistance    - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.redistance.min, runtime[ch].image_onvif.move.redistance.max, runtime[ch].image_onvif.move.redistance.value);
	printf(" Move.RelativeSpeed       - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.respeed.min, runtime[ch].image_onvif.move.respeed.max, runtime[ch].image_onvif.move.respeed.value);
	printf(" Move.ContinuousSpeed     - [%d ~ %d] , %d\n", runtime[ch].image_onvif.move.cospeed.min, runtime[ch].image_onvif.move.cospeed.max, runtime[ch].image_onvif.move.cospeed.value);
	printf("=====================================================================\n");


	return 0;
}
#if 0
__commandlist(ipcam_set_onvif_img,"ipcam_set_onvif_img", ipcam_set_onvif_img_help, ipcam_set_onvif_img_help);
#endif

static char ipcam_set_onvif_focus_help[] = ">>> ipcam_set_onvif_focus [ch_num]";
static int ipcam_set_onvif_focus(int argc, char**argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();

	if(argc < 2){
		printf("%s\n",ipcam_set_onvif_focus_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);

	//nf_onvif_get_status(ch);

	if (0)
	{
		focus_move_onvif setter;
		memset(&setter, 0x00, sizeof(focus_move_onvif));
		setter.mode = NF_IPCAM_FOCUS_MODE_ONVIF_RELATIVE;
		setter.distance = -100;
		setter.speed = 100;
		nf_onvif_focus_move(ch, setter);
	}

	nf_ipcam_set_focus_near(ch, 100);

	usleep(1000*1000*10);
	nf_ipcam_set_ptz_stop(ch, NULL, NULL, NULL);



	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_onvif_focus,"ipcam_set_onvif_focus", ipcam_set_onvif_focus_help, ipcam_set_onvif_focus_help);
#endif

static char hub_port_close_help[] = "hub_port_close type(1:unlink 2:poe) channelNum(8~15) [ENTER]";
static int hub_port_close(int argc, char **argv)
{
	guint type = (-1);
	guint ch = (-1);

	if(argc > 3){
		printf("%s\n",hub_port_close_help);
		return -1;
	}

	type = strtoul(argv[1],NULL,0);
	ch = strtoul(argv[2],NULL,0);

	if (ch < 8 || ch > 16) {
		printf("channelNum Error");
		return -1;
	}

	if(type == 1)
		hub_unlink_request(ch);
	else if (type == 2)
		hub_poe_reboot(ch);
	else {
		printf("Unknown type.\n");
		return -1;
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(hub_port_close,"hub_port_close", hub_port_close_help, hub_port_close_help);
#endif


static char ipcam_get_port_status_help[] = "ipcam_get_port_status [ch_mask]";
static int ipcam_get_port_status(int argc, char **argv)
{
	guint ch_mask = 0;
	gint i;
	NFIPCamPortStatus ps;

	if(argc < 2){
		printf("%s\n",ipcam_get_port_status_help);
		return -1;
	}

	ch_mask = strtoul(argv[1],NULL,0);

	for(i=0; i<NUM_IPX_CHANNEL; i++)
	{
		if( ch_mask & (1<<i) )
		{
			nf_ipcam_get_port_status(i, &ps,NULL);
			printf("status(%d) class(%d) vendor(%s) model(%s) detail(%s)\n",
				ps.status, ps.device_class, ps.vendor, ps.model, ps.detail);
		}
	}

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_port_status,"ipcam_get_port_status", ipcam_get_port_status_help, ipcam_get_port_status_help);
#endif


static char is_all_ch_unplugged_help[] = "is_all_ch_unplugged [ch_mask]";
static int is_all_ch_unplugged(int argc, char **argv)
{
	int ret = nf_ipcam_is_all_ch_unplugged();
	printf("[is_all_ch_unplugged] : %d\n", ret);
	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(is_all_ch_unplugged,"is_all_ch_unplugged", is_all_ch_unplugged_help, is_all_ch_unplugged_help);
#endif


static char ipcam_get_af_mode_help[] = "ipcam_get_af_mode";
static int ipcam_get_af_mode(int argc, char **argv)
{
	guint ch_mask = 0;

	nf_ipcam_get_af_mode(&ch_mask);
	printf("af_mode(%08x)\n", ch_mask);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_af_mode,"ipcam_get_af_mode", ipcam_get_af_mode_help, ipcam_get_af_mode_help);
#endif


static char ipcam_switch_mode_help[] = "ipcam_switch_mode [nvr_port_mask] [ext_port_mask]";
static int ipcam_switch_mode(int argc, char **argv)
{
	guint nvr_mask = 0;
	guint ext_mask = 0;

	if (argc != 3)
	{
		return 0;
	}

	nvr_mask = strtoul(argv[1],NULL,0);
	ext_mask = strtoul(argv[2],NULL,0);

	nf_ipcam_switch_mode(nvr_mask,ext_mask);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_switch_mode,"ipcam_switch_mode", ipcam_switch_mode_help, ipcam_switch_mode_help);
#endif


static char ipcam_direct_config_start_help[] = "ipcam_dc_start [ch]";
static int ipcam_direct_config_start(int argc, char **argv)
{
	int ch = 0;

	if(argc < 2){
		printf("%s\n",ipcam_direct_config_start_help);
		return -1;
	}

	ch = atoi(argv[1]);
	nf_ipcam_direct_config_start( ch );

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_direct_config_start,"ipcam_dc_start", ipcam_direct_config_start_help, ipcam_direct_config_start_help);
#endif

static char ipcam_direct_config_stop_help[] = "ipcam_dc_stop";
static int ipcam_direct_config_stop(int argc, char **argv)
{

	nf_ipcam_direct_config_stop();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_direct_config_stop,"ipcam_dc_stop", ipcam_direct_config_stop_help, ipcam_direct_config_stop_help);
#endif



static char ipcam_get_onvif_preset_help[] = ">>> ipcam_get_onvif_preset [ch_num]";
static int ipcam_get_onvif_preset(int argc, char**argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();
	int i;

	if(argc < 2){
		printf("%s\n",ipcam_get_onvif_preset_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);
	printf("---------------------------\n");
	printf(" preset cnt : %d\n", runtime[ch].preset.preset_cnt);
	for(i = 0; i < runtime[ch].preset.preset_cnt; i++)
	{
		printf("%d th preset no : %d token : %s\n", i, runtime[ch].preset.preset_number[i], runtime[ch].preset.preset_token[i]);
	}

	//usleep(1000*1000*10);

	return 0;
}
#if 0
__commandlist(ipcam_get_onvif_preset,"ipcam_get_onvif_preset", ipcam_get_onvif_preset_help, ipcam_get_onvif_preset_help);
#endif

static char ipcam_get_ptz_onvif_help[] = ">>> ipcam_get_ptz_onvif [ch_num]";
static int ipcam_get_ptz_onvif(int argc, char**argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();
	int i;

	if(argc < 2){
		printf("%s\n",ipcam_get_ptz_onvif_help);
		return -1;
	}
	ch = strtoul(argv[1],NULL,0);

	printf("---onvif ptz options------------------------\n");
	printf(" ptz support : %08x \n", runtime[ch].ptz_onvif.support_ptz);
	printf("  absolute pan    : %d ~ %d\n", runtime[ch].ptz_onvif.absolute_pan.min, runtime[ch].ptz_onvif.absolute_pan.max);
	printf("  absolute tilt   : %d ~ %d\n", runtime[ch].ptz_onvif.absolute_tilt.min, runtime[ch].ptz_onvif.absolute_tilt.max);
	printf("  absolute zoom   : %d ~ %d\n", runtime[ch].ptz_onvif.absolute_zoom.min, runtime[ch].ptz_onvif.absolute_zoom.max);
	printf("  relative pan    : %d ~ %d\n", runtime[ch].ptz_onvif.relative_pan.min, runtime[ch].ptz_onvif.relative_pan.max);
	printf("  relative tilt   : %d ~ %d\n", runtime[ch].ptz_onvif.relative_tilt.min, runtime[ch].ptz_onvif.relative_tilt.max);
	printf("  relative zoom   : %d ~ %d\n", runtime[ch].ptz_onvif.relative_zoom.min, runtime[ch].ptz_onvif.relative_zoom.max);
	printf("  continuous pan  : %d ~ %d\n", runtime[ch].ptz_onvif.continuous_pan.min, runtime[ch].ptz_onvif.continuous_pan.max);
	printf("  continuous tilt : %d ~ %d\n", runtime[ch].ptz_onvif.continuous_tilt.min, runtime[ch].ptz_onvif.continuous_tilt.max);
	printf("  continuous zoom : %d ~ %d\n", runtime[ch].ptz_onvif.continuous_zoom.min, runtime[ch].ptz_onvif.continuous_zoom.max);
	printf(" timeout : %d ~ %d\n", runtime[ch].ptz_onvif.timeout.min, runtime[ch].ptz_onvif.timeout.max);
	printf(" preset no : %d\n",runtime[ch].ptz_onvif.preset_cnt);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_ptz_onvif,"ipcam_get_ptz_onvif", ipcam_get_ptz_onvif_help, ipcam_get_ptz_onvif_help);
#endif

static char ipcam_move_ptz_onvif_help[] = ">>> ipcam_move_ptz_onvif [ch_num] [ab=0 re=1 co=2] [pan] [tilt] [zoom]";
static int ipcam_move_ptz_onvif(int argc, char**argv)
{
	guint ch = (-1);
	mtable *runtime = get_runtime();
	int i;
	int pan = 0;
	int tilt = 0;
	int zoom = 0;

	if(argc < 4){
		printf("%s\n",ipcam_move_ptz_onvif_help);
		return -1;
	}

	ch = strtoul(argv[1],NULL,0);
	ptz_info_onvif info;
	memset(&info, 0x00, sizeof(ptz_info_onvif));

	int mode = strtoul(argv[2], NULL, 0);
	pan = strtol(argv[3], NULL, 0);
	if(argc > 4)
	{
		tilt = strtol(argv[4], NULL, 0);
	}
	if(argc > 5)
	{
		zoom = strtol(argv[5], NULL, 0);
	}

	if(mode < 0 || mode > 2)
	{
		printf("%s\n",ipcam_move_ptz_onvif_help);
		return -1;
	}

	printf("---------------------------\n");
	printf("ptz mode : %d\n", mode);
	printf(" pan     : %d\n", pan);
	printf(" tilt    : %d\n", tilt);
	printf(" zoom    : %d\n", zoom);

	switch(mode)
	{
	case 0:
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_ABSOLUTE;
		info.absolute_pan = pan;
		info.absolute_tilt = tilt;
		info.absolute_zoom = zoom;
		break;
	case 1:
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_RELATIVE;
		info.relative_pan = pan;
		info.relative_tilt = tilt;
		info.relative_zoom = zoom;
		break;
	case 2:
		info.mode = NF_IPCAM_PTZ_MODE_ONVIF_CONTINUOUS;
		info.speed_pan = pan;
		info.speed_tilt = tilt;
		info.speed_zoom = zoom;
		break;
	}

	int rtn = nf_onvif_ptz_move(ch, info);
	printf("result : %d\n", rtn);

	usleep(10 * 1000 * 1000);
	rtn = nf_onvif_ptz_stop(ch);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_move_ptz_onvif,"ipcam_move_ptz_onvif", ipcam_move_ptz_onvif_help, ipcam_move_ptz_onvif_help);
#endif


static char ipcam_get_auxiliary_help[] = ">>> ipcam_get_auxiliary [ch_num] [ 0 (device) | 1 (ptz)]";
static int ipcam_get_auxiliary(int argc, char**argv)
{
	int ch;
	int category;
	int i;
	NFIPCamAuxiliary info;

	if(argc < 3){
		printf("%s\n",ipcam_get_auxiliary_help);
		return -1;
	}

	sscanf(argv[1], "%d", &ch);
	sscanf(argv[2], "%d", &category);

	
	nf_ipcam_get_auxiliary_commands(ch, category , &info, NULL);

	printf("\e[31m");
	printf("---- size : %d\n", info.size);
	if(info.size > 16)
	{
		info.size = 16;
	}
	puts("------------------------------------------------------------");
	for(i = 0; i < info.size; i++)
	{
		printf(" [%02d] <%s>\n", i, info.commands[i]);
	}
	puts("------------------------------------------------------------");
	printf("\e[0m");

	return 0;

}

#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_auxiliary,"ipcam_get_auxiliary", ipcam_get_auxiliary_help, ipcam_get_auxiliary_help);
#endif


static char ipcam_send_auxiliary_help[] = ">>> ipcam_send_auxiliary [ch_num] [ 0 (device) | 1 (ptz) ] [command_index]";
static int ipcam_send_auxiliary(int argc, char**argv)
{
	int ch;
	int command_index;
	int category;
	int i, rtn;
	NFIPCamAuxiliary info;

	if(argc < 4){
		printf("%s\n",ipcam_send_auxiliary_help);
		return -1;
	}

	sscanf(argv[1], "%d", &ch);
	sscanf(argv[2], "%d", &category);
	sscanf(argv[3], "%d", &command_index);
	

	rtn = nf_ipcam_send_auxiliary_command(ch, category, command_index, NULL);

	printf("\e[31m");
	puts("------------------------------------------------------------");
	printf("run ---- ch:[%d], category[%d], command[%d], rtn:[%d]\n", ch, category, command_index, rtn);
	puts("------------------------------------------------------------");
	printf("\e[0m");

	return 0;

}

#if !IPCAM_UNIT_TEST
__commandlist(ipcam_send_auxiliary,"ipcam_send_auxiliary", ipcam_send_auxiliary_help, ipcam_send_auxiliary_help);
#endif


static char ipcam_install_help[] = "ipcam_install";
static int ipcam_install(int argc, char **argv)
{
	nf_ipcam_install_mode_on();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_install,"ipcam_install", ipcam_install_help, ipcam_install_help);
#endif

static char ipcam_install_done_help[] = "ipcam_install_done";
static int ipcam_install_done(int argc, char **argv)
{
	nf_ipcam_install_done();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_install_done,"ipcam_install_done", ipcam_install_done_help, ipcam_install_done_help);
#endif



static char ipcam_fw_up_help[] = "ipcam_fw_up [ch_mask] [file_path] [is_nonblocking]";
static int ipcam_fw_up(int argc, char **argv)
{
	if (argc < 4)
	{
		printf("%s\n", ipcam_fw_up_help);
		return (-1);
	}
	int ch_mask = atoi(argv[1]);
	char fw_path[100];
	if(atoi(argv[2]) == 0)
	{
		strcpy(fw_path, "/web-51110.2.1133.100.bin");
	}
	else if(atoi(argv[2]) == 1)
	{
		strcpy(fw_path, "/web-51110.2.1140.100.bin");
	}
	else
	{
		strcpy(fw_path, "/web-51110.2.2000.100.bin");
	}

	nf_ipcam_fw_upgrade(ch_mask, fw_path, atoi(argv[3]));

	return 0;
}
#if 0
__commandlist(ipcam_fw_up,"ipcam_fw_up", ipcam_fw_up_help, ipcam_fw_up_help);
#endif



static char ipcam_get_bps_table_help[] = "ipcam_get_bps_table [ch]";
static int ipcam_get_bps_table(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("%s\n", ipcam_get_bps_table_help);
		return (-1);
	}
	int ch = atoi(argv[1]);

	NFIPCamBpsTable table;
	memset(&table, 0x00, sizeof(NFIPCamBpsTable));
	int i, j, k;
	nf_ipcam_get_bps_table(ch, &table);
	for(i = 0; i < NF_IPCAM_RES_MAX; i++)
	{
		if(table.org_video_bps[i][0][0] == 0)
		{
			continue;
		}
		printf("res %d bps :\n", i);
		for(j = 0; j < NF_IPCAM_FPS_MAX; j++)
		{
			printf("fps %d : \n", j);
			for(k = 0; k < NF_IPCAM_QUALITY_MAX; k++)
			{
				printf("%d ", table.org_video_bps[i][j][k]);
			}
			printf(" -> ");
			for(k = 0; k < NF_IPCAM_QUALITY_MAX; k++)
			{
				printf("%d ", table.sum_video_bps[i][j][k]);
			}
			printf("\n");
		}
	}
	printf(" audio bps : %d\n", table.audio_bps);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_bps_table,"ipcam_get_bps_table", ipcam_get_bps_table_help, ipcam_get_bps_table_help);
#endif


static char ipcam_rec_calc_help[] = "ipcam_rec_calc_help";
static int ipcam_rec_calc(int argc, char **argv)
{
	int i, rtn;

	NF_RECORD_CALC_PARAM_T param;
	NF_RECORD_CALC_RESULT_T rslt;

	param.mode = NF_RECORD_CALC_CURRENT;
	param.alarm_occur_pcnt = 5;
	param.motion_occur_pcnt = 5;

	nf_record_calculate(param, &rslt);

	printf("\nrslt-------------------\n");
	printf("gb per day : \n");
	for(i = 0; i < NUM_IPX_CHANNEL; i++)
	{
		printf("%5.2f ", rslt.ch_gb_per_day[i]);
	}
	printf("\nhdd_total : %5.2f\n", rslt.hdd_full_gb);
	printf("hdd_remain : %5.2f\n", rslt.hdd_remain_gb);
	printf("day_full : %5.2f\n", rslt.day_full);
	printf("day_remain : %5.2f\n", rslt.day_remain);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_rec_calc,"ipcam_rec_calc", ipcam_rec_calc_help, ipcam_rec_calc_help);
#endif

static char ipcam_hub_mac_help[] = "ipcam_hub_mac";
static int ipcam_hub_mac(int argc, char **argv)
{
	unsigned char mac[6];
	nf_ipcam_hub_get_macaddr(mac);
	printf("[%s][%d] macaddr : [%02x:%02x:%02x:%02x:%02x:%02x]\n", __FUNCTION__, __LINE__,
			mac[0], mac[1], mac[2],
			mac[3], mac[4], mac[5]);
	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_hub_mac, "ipcam_hub_mac", ipcam_hub_mac_help, ipcam_hub_mac_help);
#endif

static char ipcam_set_time_info_help[] = "ipcam_set_time_info [ch]";
static int ipcam_set_time_info(int argc, char **argv)
{
	int ch = 0;

	if (argc < 2)
	{
		printf("%s\n", ipcam_set_time_info_help);
		return (-1);
	}
	ch = atoi(argv[1]);
	nf_ipcam_set_time_info(ch);

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_set_time_info,"ipcam_set_time_info", ipcam_set_time_info_help, ipcam_set_time_info_help);
#endif



static char ipcam_get_mf_info_help[] = "ipcam_get_mf_info [ch]";
static int ipcam_get_mf_info(int argc, char **argv)
{
	int ch = 0;
	NFIPCamMFInfo info;

	if (argc < 2)
	{
		printf("%s\n", ipcam_get_mf_info_help);
		return (-1);
	}
	ch = atoi(argv[1]);
	nf_ipcam_get_mf_info(ch, &info);

	printf(
			"------ MODEL NAME : %s\n"
			"------ IP ADDR    : %d.%d.%d.%d\n"
			"------ MAC ADDR   : %02X-%02X-%02X-%02X-%02X-%02X\n"
			"------ F/W VER    : %s\n"
			"------ CAM TIME   : %s\n",
			info.name,
			(info.ipaddr&0xff000000)>>24,
			(info.ipaddr&0xff0000)>>16,
			(info.ipaddr&0xff00)>>8,
			(info.ipaddr&0xff),
			info.macaddr[0], info.macaddr[1], info.macaddr[2],
			info.macaddr[3], info.macaddr[4], info.macaddr[5],
			info.swver, info.timeinfo
		  );

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_get_mf_info,"ipcam_get_mf_info", ipcam_get_mf_info_help, ipcam_get_mf_info_help);
#endif

static char ipcam_scan_start_help[] = "ipcam_scan_start";
static int ipcam_scan_start(int argc, char **argv)
{
	nf_ipcam_scan_start();
	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_scan_start, "ipcam_scan_start", ipcam_scan_start_help, ipcam_scan_start_help);
#endif

static char ipcam_works_done_help[] = "ipcam_works_done";
static int ipcam_works_done(int argc, char **argv)
{
	nf_ipcam_works_done();

	return 0;
}
#if !IPCAM_UNIT_TEST
__commandlist(ipcam_works_done, "ipcam_works_done", ipcam_works_done_help, ipcam_works_done_help);
#endif

#endif	// __NF_IPCAM_UNITTEST_C__
