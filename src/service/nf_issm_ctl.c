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

#include <gobj.h>
#include <gobjmrtppipe.h>

#include "issm.h"
#include "nf_record.h"
#include "nfdal.h"
#include "nf_common.h"
#include "nf_common_util.h"
#include "../sysman/SEED_KISA.h"
// #include "gst/nf/gstmrtpsrc.h"
#include "nf_api_live.h"
#include "nf_api_ipcam.h"
#include "nf_api_eventlog.h"
// #include "nmf_mrtp_pipe.h"

#include "nf_issm_ctl.h"
#include "nf_issm_ctl_frame.h"
#include "nf_issm_ctl_live.h"
#include "nf_issm_ctl_playback.h"
#include "nf_issm_ctl_bitrate.h"
#include "nf_issm_ctl_recvaudio.h"
#include "nf_issm_ctl_funcs.h"


typedef struct __NF_ISSM_CTL_CMD {
	int type; // enum NF_ISSM_CTL_CMD_TYPE
	char log[100];
} NF_ISSM_CTL_CMD;


static GMutex *g_issm_adapter_mutex = NULL;
static GMutex *g_issm_cmd_mutex = NULL;
static GPtrArray *g_issm_cmd_array = NULL;
static int g_issm_adapter_is_running = 0;
static int g_issm_adapter_restart_checker = 0;

// for sysdb
static int g_issm_is_adaptive = 0;
static int g_issm_is_enc = 0;
static int g_arg_policy = 0;
static int g_dev_block = 0;
int g_issm_is_audio_tx_enable = 1;
int g_issm_is_audio_rx_enable = 1;

// for standard rtsp streaming
ISSM_EP_ENTRY g_issm_ep_entry[MAX_ISSM_CH];
ISSM_EP_TRACK g_issm_track_v_h264[MAX_ISSM_CH];
ISSM_EP_TRACK g_issm_track_v_h265[MAX_ISSM_CH];
ISSM_EP_TRACK g_issm_track_v_mjpeg[MAX_ISSM_CH];
ISSM_EP_TRACK g_issm_track_a_mulaw[MAX_ISSM_CH];
ISSM_EP_TRACK g_issm_track_m_meta[MAX_ISSM_CH];
ISSM_EP_TRACK g_issm_track_backch[MAX_ISSM_CH];
int g_issm_ep_entry_id[MAX_ISSM_CH];
int g_issm_track_v_h264_id[MAX_ISSM_CH];
int g_issm_track_v_h265_id[MAX_ISSM_CH];
int g_issm_track_v_mjpeg_id[MAX_ISSM_CH];
int g_issm_track_a_mulaw_id[MAX_ISSM_CH];
int g_issm_track_m_meta_id[MAX_ISSM_CH];
int g_issm_ep_entry_pb_id[MAX_ISSM_CH];
int g_issm_track_v_h264_pb_id[MAX_ISSM_CH];
int g_issm_track_v_h265_pb_id[MAX_ISSM_CH];
int g_issm_track_v_mjpeg_pb_id[MAX_ISSM_CH];
int g_issm_track_a_mulaw_pb_id[MAX_ISSM_CH];
int g_issm_track_m_meta_pb_id[MAX_ISSM_CH];
int g_issm_onvif_multicast_onoff[MAX_ISSM_CH] = { 0, };
int g_issm_onvif_multicast_task_id[MAX_ISSM_CH] = { 0, };
static int g_issm_zero_multicast_onoff   = 0;
static int g_issm_zero_multicast_task_id = 0;

// for zero channel streaming
ISSM_EP_ENTRY g_issm_zero_ep_entry;
ISSM_EP_TRACK g_issm_zero_track;
int g_issm_zero_ep_entry_id = -1;
int g_issm_zero_track_id = -1;

// for itx rtsp streaming(live, playback)
ISSM_EP_ENTRY g_issm_itx_ep_entry[2];
ISSM_EP_TRACK g_issm_itx_track[2];
int g_issm_itx_ep_entry_id[2];
int g_issm_itx_track_id[2];


static void nf_issm_sysdb_change_func(NF_NOTIFY_INFO *p_info, gpointer p_data);
static void nf_issm_vloss_change_func(NF_NOTIFY_INFO *pinfo, gpointer data);
static int nf_issm_sysman_get_user_idx_from_name(char*);
static int nf_issm_sysman_get_user_pass_from_idx(int, char*, int);
static int nf_issm_sysman_get_user_audio_auth(int, int*, int*);
static void nf_issm_ctl_thread_func(void);

void nf_issm_ctl_start();
void nf_issm_ctl_stop();
int nf_issm_create_entrypoints();

// callback functions
int nf_issm_cb_get_password(void *p_data);
int nf_issm_cb_get_udp_port(void *p_data);
int nf_issm_cb_get_streaming_info(void *p_data);
int nf_issm_cb_get_svr_ip(void *p_data);
int nf_issm_cb_get_mcast_info(void *p_data);
int nf_issm_cb_get_date_info(void *p_data);
int nf_issm_cb_get_covert(void *p_data);
int nf_issm_cb_session_changed(void *p_data);
int nf_issm_cb_get_enc_key(void *p_data);
int nf_issm_cb_get_audio_backch_port(void *p_data);


void nf_issm_ctl_init()
{
	gulong cb_handle;
	GThread *thread_id = 0;

	printf("[%s] start\n", __FUNCTION__);
	issm_version();

	if (g_issm_adapter_mutex)
	{
		printf("[%s][%d] Error - g_issm_adapter_mutex is already created!\n",
				__FUNCTION__, __LINE__);
		return;
	}

	g_issm_adapter_mutex = g_mutex_new();
	if (g_issm_adapter_mutex == 0)
	{
		printf("[%s][%d] Error - Failed to create g_issm_adapter_mutex!\n",
				__FUNCTION__, __LINE__);
		return;
	}

	g_issm_cmd_mutex = g_mutex_new();
	if (g_issm_cmd_mutex == 0)
	{
		printf("[%s][%d] Error - Failed to create g_issm_cmd_mutex!\n",
				__FUNCTION__, __LINE__);
		return;
	}

	g_issm_cmd_array = g_ptr_array_new();
	if (g_issm_cmd_array == 0)
	{
		printf("[%s][%d] Error - Failed to create g_issm_cmd_array!\n",
				__FUNCTION__, __LINE__);
		return;
	}

	cb_handle = nf_notify_connect_cb("sysdb_change", nf_issm_sysdb_change_func, (gpointer)NULL);
	if (cb_handle == 0)
	{
		printf("[%s][%d] Error - nf_notify_connect_cb[nf_issm_sysdb_change_func]\n",
				__FUNCTION__, __LINE__);
		return;
	}

	cb_handle = nf_notify_connect_cb("sysdb_ipcam_change", nf_issm_sysdb_change_func, NULL);
	if (cb_handle == 0)
	{
		printf("[%s][%d] Error - nf_notify_connect_cb[nf_issm_sysdb_change_func]\n",
				__FUNCTION__, __LINE__);
		return;
	}

	cb_handle = nf_notify_connect_cb("vloss", nf_issm_vloss_change_func, (gpointer)NULL);
	if (cb_handle == 0)
	{
		printf("[%s][%d] Error - nf_notify_connect_cb[nf_issm_vloss_change_func]\n",
				__FUNCTION__, __LINE__);
		return;
	}

	thread_id = g_thread_create((GThreadFunc)nf_issm_ctl_thread_func, NULL, FALSE, NULL);
	if (!thread_id)
	{
		printf("[%s][%d] Error - Failed to create thread[nf_issm_ctl_thread_func]!\n",
				__FUNCTION__, __LINE__);
		return;
	}

	// Set callback funcs
	issm_register_callback_func(ISSM_CB_IDX_GET_PASSWORD, 			nf_issm_cb_get_password);
	issm_register_callback_func(ISSM_CB_IDX_GET_UDP_PORT, 			nf_issm_cb_get_udp_port);
	issm_register_callback_func(ISSM_CB_IDX_GET_STREAMING_INFO,		nf_issm_cb_get_streaming_info);
	issm_register_callback_func(ISSM_CB_IDX_NOTIFY_BITRATE_UD,		nf_issm_cb_noti_bitrate_ud);
	issm_register_callback_func(ISSM_CB_IDX_GET_MCAST_INFO,			nf_issm_cb_get_mcast_info);
	issm_register_callback_func(ISSM_CB_IDX_GET_SVR_IP,				nf_issm_cb_get_svr_ip);
	issm_register_callback_func(ISSM_CB_IDX_NOTIFY_PB_START,		nf_issm_cb_notify_pb_start);
	issm_register_callback_func(ISSM_CB_IDX_NOTIFY_PB_END,			nf_issm_cb_notify_pb_end);
	issm_register_callback_func(ISSM_CB_IDX_PLAY_AUDIO,				nf_issm_cb_playaudio);
	issm_register_callback_func(ISSM_CB_IDX_GET_DATE_INFO,			nf_issm_cb_get_date_info);
	issm_register_callback_func(ISSM_CB_IDX_GET_COVERT,				nf_issm_cb_get_covert);
	issm_register_callback_func(ISSM_CB_IDX_SESSION_CHANGED,		nf_issm_cb_session_changed);
	issm_register_callback_func(ISSM_CB_IDX_GET_ENC_KEY,			nf_issm_cb_get_enc_key);
	issm_register_callback_func(ISSM_CB_IDX_GET_AUDIO_BACKCH_PORT,	nf_issm_cb_get_audio_backch_port);

	printf("[%s] end\n", __FUNCTION__);
}

void nf_issm_ctl(int p_issm_cmd, char *p_log_str)
{
	NF_ISSM_CTL_CMD *cmd;

	if (p_issm_cmd <= ISSM_NONE || p_issm_cmd >= ISSM_END)
	{
		printf("[%s][%d] Error - p_issm_cmd[%d]\n", __FUNCTION__, __LINE__, p_issm_cmd);
		return;
	}

	cmd = (NF_ISSM_CTL_CMD*)malloc(sizeof(NF_ISSM_CTL_CMD));
	memset(cmd, 0x00, sizeof(NF_ISSM_CTL_CMD));

	cmd->type = p_issm_cmd;
	if (p_log_str)
		snprintf(cmd->log, sizeof(cmd->log), "%s", p_log_str);

	g_mutex_lock(g_issm_cmd_mutex);
	g_ptr_array_add(g_issm_cmd_array, cmd);
	g_mutex_unlock(g_issm_cmd_mutex);
}

void nf_issm_ctl_start()
{
	int is_ipv6_support, rtsp_port, ch_mask;
	GobjMrtpPipe *h_mrtp_pipe = NULL;
	int ret;

	printf("[%s] start\n", __FUNCTION__);

	// Set multicast addr
	if (nf_sysdb_get_uint("net.rtp.multi.S0.C0.vaddr") == 0)
	{
		unsigned char mac[32] = { 0, };
		unsigned char mac_split[6] = { 0, };
		int i, j;

		snprintf(mac, sizeof(mac), "%s", nf_sysdb_get_str_nocopy("sys.info.mac"));
		sscanf(mac, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
				   &mac_split[0],
				   &mac_split[1],
				   &mac_split[2],
				   &mac_split[3],
				   &mac_split[4],
				   &mac_split[5]);

		// ipv4
		{
			unsigned int maddr[4] = { 0, };
			maddr[0] = 239;
			maddr[1] = mac_split[4];
			maddr[2] = mac_split[5];

			for (i = 0; i < 2; i++)
			{
				for (j = 0; j < MAX_ISSM_V_CH; j++)
				{
					unsigned char sysdb_str[100] = { 0, };
					unsigned int maddr_int;

					snprintf(sysdb_str, sizeof(sysdb_str), "net.rtp.multi.S%d.C%d.vaddr", i, j);

					maddr[3] = (i<<6)|(j+1);

					maddr_int = 0;
					maddr_int += (maddr[0] << 24);
					maddr_int += (maddr[1] << 16);
					maddr_int += (maddr[2] << 8);
					maddr_int += maddr[3];

					nf_sysdb_set_uint(sysdb_str, maddr_int);
				}
			}
		}
		// ipv6
		{
			if (mac_split[0] == 0)
				mac_split[0] = 1;
			if (mac_split[1] == 0)
				mac_split[1] = 1;
			if (mac_split[2] == 0)
				mac_split[2] = 1;
			if (mac_split[3] == 0)
				mac_split[3] = 1;
			if (mac_split[4] == 0)
				mac_split[4] = 1;
			if (mac_split[5] == 0)
				mac_split[5] = 1;

			for (i = 0; i < 2; i++)
			{
				for (j = 0; j < NUM_ACTIVE_CH; j++)
				{
					unsigned char sysdb_str[100] = { 0, };
					unsigned char maddr[46] = { 0, }; // INET6_ADDRSTRLEN

					snprintf(sysdb_str, sizeof(sysdb_str), "net.rtp.multi.S%d.C%d.ipv6.vaddr", i, j);
					snprintf(maddr, sizeof(maddr), "ff05::%x%02x:%x%02x:%x%02x:%x%02x",
							i + 1, j + 1, mac_split[0], mac_split[1], mac_split[2], mac_split[3], mac_split[4], mac_split[5]);

					nf_sysdb_set_str(sysdb_str, maddr);
				}
			}
		}

		// Zero-channel multicast addr
		{
            unsigned char sysdb_str[100] = { 0, };
            unsigned int maddr_int;

            // IPv4: 239.mac[4].mac[5].0xFF
            unsigned int maddr[4] = { 239, mac_split[4], mac_split[5], 0xFF };
            maddr_int  = (maddr[0] << 24);
            maddr_int += (maddr[1] << 16);
            maddr_int += (maddr[2] << 8);
            maddr_int +=  maddr[3];
            nf_sysdb_set_uint("net.rtp.multi.zero.vaddr", maddr_int);

            // IPv6: ff05::ffff:mac[0]mac[1]:mac[2]mac[3]:mac[4]mac[5]
            unsigned char maddr6[46] = { 0, };
            snprintf(maddr6, sizeof(maddr6),
                     "ff05::ffff:%x%02x:%x%02x:%x%02x",
                     mac_split[0], mac_split[1],
                     mac_split[2], mac_split[3],
                     mac_split[4], mac_split[5]);
            nf_sysdb_set_str("net.rtp.multi.zero.ipv6.vaddr", maddr6);
        }		

		nf_sysdb_save("net");
		nf_sysdb_save_flush();
	}

	g_mutex_lock(g_issm_adapter_mutex);
	if (g_issm_adapter_is_running == 1)
	{
		printf("[%s] Already started!\n", __FUNCTION__);
		g_mutex_unlock(g_issm_adapter_mutex);
		return;
	}

	// Set env values
	is_ipv6_support = (int)nf_sysdb_get_uint("net.ipv6.using");
	if (is_ipv6_support == 0) // off
		issm_set_ipv6_support(0);
	else // manual, dhcp
		issm_set_ipv6_support(1);

	rtsp_port = (int)nf_sysdb_get_uint("net.rtp.rtspport");
	ret = (int)nf_notify_get_param0("vloss");
	ch_mask = ~(ret);

	issm_set_alive_ch_mask(ch_mask);
	issm_set_max_v_ch(MAX_ISSM_V_CH);
	issm_set_max_a_ch(MAX_ISSM_A_CH);
	issm_set_rtsp_port(rtsp_port);
	issm_set_rtsp_realm("ITX_STREAMING_MODULE");
#if defined(_IPX_32P4E) || defined(_IPX_0824P4E) || defined(_IPX_1648P4E)|| defined(_IPX_32M4E) || defined(_IPX_32P5)
	issm_set_max_connection(128, 32, NUM_ACTIVE_CH * 4);
#else
	issm_set_max_connection(64, 32, NUM_ACTIVE_CH * 4);
#endif
	issm_set_session_timeout(30 * 1000);
	issm_set_streaming_info_print(0);
	issm_set_bitrate_control_data(10, 1000LL, 1000LL);
	issm_set_sock_send_buf_size(262144); // bytes
	g_issm_is_adaptive = (int)nf_sysdb_get_bool("rec.netstream.enable_stream_control");
	g_issm_is_enc = (int)nf_sysdb_get_bool("net.proto.srtspon");
	g_issm_is_audio_tx_enable = (int)nf_sysdb_get_bool("audio.tx");
	g_issm_is_audio_rx_enable = (int)nf_sysdb_get_bool("audio.rx");

	// prvt_dup scenario
	g_arg_policy = (int)nf_sysdb_get_bool("sys.info.agr_policy");
	g_dev_block = (int)nf_sysdb_get_bool("sys.info.guard.dev_block");
	if (g_arg_policy == 0 || g_dev_block == 1)
	{
		printf("[ISSM][%s][%d] Prvt_dup enabled\n", __FUNCTION__, __LINE__);
		issm_set_max_connection(0, 0, 0);
	}

	ret = issm_init();
	if (ret != ISSM_OK)
	{
		printf("[%s][%d] issm_init failed\n", __FUNCTION__, __LINE__);
		g_mutex_unlock(g_issm_adapter_mutex);
		return;
	}

	nf_issm_create_entrypoints();

	nf_issm_ctl_pb_init();
	nf_issm_ctl_recvaudio_init();
	nf_issm_ctl_bitrate_init();

	// Set handoff funcs
	ret = nf_record_register_handoff(MAX_ISSM_CH_MASK, &nf_issm_ctl_put_frame, 0);
	while (ret != 1)
	{
		printf("[%s][%d] nf_record_register_handoff() failed\n", __FUNCTION__, __LINE__);
		sleep(1);
		ret = nf_record_register_handoff(MAX_ISSM_CH_MASK, &nf_issm_ctl_put_frame, 0);
	}

    ret = nf_encode_zero_channel_register_handoff(0x1,&nf_issm_ctl_put_zero_channel_frame);
    while (ret != 1)
    {
        printf("[%s][%d] nf_encode_zero_channel_register_handoff() failed\n", __FUNCTION__, __LINE__);
        sleep(1);
        ret = nf_encode_zero_channel_register_handoff(0x1,&nf_issm_ctl_put_zero_channel_frame);
    }

//ksi_test
// #ifdef ENABLE_AUD_HI_CHIP
// 	ret = nf_HI_aud_registerHandoff(0xffffffff, &nf_issm_ctl_put_frame);
// 	while (ret != 1)
// 	{
// 		printf("[%s][%d] nf_HI_aud_registerHandoff() failed\n", __FUNCTION__, __LINE__);
// 		sleep(1);
// 		ret = nf_HI_aud_registerHandoff(0xffffffff, &nf_issm_ctl_put_frame);
// 	}
// #else
// 	ret = nf_rec_audio_register_handoff(0xffffffff, &nf_issm_ctl_put_frame);
// 	while (ret != 1)
// 	{
// 		printf("[%s][%d] nf_rec_audio_register_handoff() failed\n", __FUNCTION__, __LINE__);
// 		sleep(1);
// 		ret = nf_rec_audio_register_handoff(0xffffffff, &nf_issm_ctl_put_frame);
// 	}
// #endif

	h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();
	while (h_mrtp_pipe == NULL)
	{
		printf("[%s][%d] nf_live_get_mrtp_pipe_handle() failed\n", __FUNCTION__, __LINE__);
		sleep(1);
		h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();
	}
	nmf_mrtp_pipe_set_src_handoff_audio_fragment(h_mrtp_pipe, &nf_issm_ctl_put_frame);

#if defined(ENABLE_AUD_HI_CHIP)
	ret = nf_HI_aud_registerHandoff(0xffffffff, &nf_issm_ctl_put_frame);
	while (ret != 1)
	{
		printf("[%s][%d] nf_HI_aud_registerHandoff() failed\n", __FUNCTION__, __LINE__);
		sleep(1);
		ret = nf_HI_aud_registerHandoff(0xffffffff, &nf_issm_ctl_put_frame);
	}
#elif defined(CHIP_NVT)
	ret = nf_audio_registerHandoff(0xffffffff, &nf_issm_ctl_put_frame);
	while (ret != 1)
	{
		printf("[%s][%d] nf_audio_registerHandoff() failed\n", __FUNCTION__, __LINE__);
		sleep(1);
		ret = nf_audio_registerHandoff(0xffffffff, &nf_issm_ctl_put_frame);
	}
#else
	ret = nf_rec_audio_register_handoff(0xffffffff, &nf_issm_ctl_put_frame);
	while (ret != 1)
	{
		printf("[%s][%d] nf_rec_audio_register_handoff() failed\n", __FUNCTION__, __LINE__);
		sleep(1);
		ret = nf_rec_audio_register_handoff(0xffffffff, &nf_issm_ctl_put_frame);
	}
#endif

// #if defined(ENABLE_AUD_HI_CHIP)
// 	ret = nf_HI_aud_registerHandoff_immediately(0xffffffff, &nf_issm_ctl_put_frame);
// 	while (ret != 1)
// 	{
// 		printf("[%s][%d] nf_HI_aud_registerHandoff_immediately() failed\n", __FUNCTION__, __LINE__);
// 		sleep(1);
// 		ret = nf_HI_aud_registerHandoff_immediately(0xffffffff, &nf_issm_ctl_put_frame);
// 	}
// #elif defined(CHIP_NVT)
// 	ret = nf_audio_registerHandoff_vlc(0xffffffff, &nf_issm_ctl_put_frame);
// 	while (ret != 1)
// 	{
// 		printf("[%s][%d] nf_audio_registerHandoff_vlc() failed\n", __FUNCTION__, __LINE__);
// 		sleep(1);
// 		ret = nf_audio_registerHandoff_vlc(0xffffffff, &nf_issm_ctl_put_frame);
// 	}
// #endif

	g_issm_adapter_is_running = 1;
	g_issm_adapter_restart_checker = 1;
	
	g_mutex_unlock(g_issm_adapter_mutex);

	printf("[%s] end\n", __FUNCTION__);
}

void nf_issm_ctl_stop()
{
	GobjMrtpPipe *h_mrtp_pipe = NULL;
	printf("[%s] start\n", __FUNCTION__);

	g_mutex_lock(g_issm_adapter_mutex);
	if (g_issm_adapter_is_running == 0)
	{
		printf("[%s] Already stopped!\n", __FUNCTION__);
		g_mutex_unlock(g_issm_adapter_mutex);
		return;
	}

	g_issm_adapter_is_running = 0;

	h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();
	while (h_mrtp_pipe == NULL)
	{
		sleep(1);
		h_mrtp_pipe = (GobjMrtpPipe*) nf_live_get_mrtp_pipe_handle();
	}
	nmf_mrtp_pipe_set_src_handoff_audio_fragment(h_mrtp_pipe, 0);
//ksi_test
// #ifdef ENABLE_AUD_HI_CHIP
// 	nf_HI_aud_registerHandoff(0, &nf_issm_ctl_put_frame);
// #else
// 	nf_rec_audio_register_handoff(0, &nf_issm_ctl_put_frame);
// #endif

#if defined(ENABLE_AUD_HI_CHIP)
	nf_HI_aud_registerHandoff(0, &nf_issm_ctl_put_frame);
	// nf_HI_aud_registerHandoff_immediately(0, &nf_issm_ctl_put_frame);
#elif defined(CHIP_NVT)
	nf_audio_registerHandoff(0, &nf_issm_ctl_put_frame);
	// nf_audio_registerHandoff_vlc(0, &nf_issm_ctl_put_frame);
#endif

	nf_record_register_handoff(0, &nf_issm_ctl_put_frame, 0);

	nf_issm_ctl_pb_close();
	nf_issm_ctl_recvaudio_close();
	nf_issm_ctl_bitrate_close();
	issm_close();

	g_mutex_unlock(g_issm_adapter_mutex);

	printf("[%s] end\n", __FUNCTION__);
}

int nf_issm_ctl_get_adaptive()
{
	return g_issm_is_adaptive;
}

void *nf_issm_ctl_get_ep_entry(int p_idx)
{
	return &(g_issm_ep_entry[p_idx]);
}

void nf_issm_ctl_refresh_ep(int p_ep_id)
{
	// Triggered by
	// (O) - resolution changed
	// (X) - Audio On/Off
	// (X) - Video Format changed (H264 <-> MJPEG)
	issm_refresh_entrypoint(p_ep_id);
}

ISSM_EP_INFO nf_issm_ctl_get_ep_info(int p_ep_id)
{
	ISSM_EP_INFO ret_info;
	int i;

	ret_info.ep_id = -1;

	for (i = 0; i < MAX_ISSM_CH; i++)
	{
		if (g_issm_ep_entry_id[i] == p_ep_id)
		{
			ret_info.ep_id = g_issm_ep_entry_id[i];

			if (g_issm_track_v_h264_id[i] != -1)
			{
				ret_info.v_track_id = g_issm_track_v_h264_id[i];
				ret_info.v_track_type = g_issm_track_v_h264[i].video_type;
			}
			else if (g_issm_track_v_h265_id[i] != -1)
			{
				ret_info.v_track_id = g_issm_track_v_h265_id[i];
				ret_info.v_track_type = g_issm_track_v_h265[i].video_type;
			}
			else if (g_issm_track_v_mjpeg_id[i] != -1)
			{
				ret_info.v_track_id = g_issm_track_v_mjpeg_id[i];
				ret_info.v_track_type = g_issm_track_v_mjpeg[i].video_type;
			}
			else
			{
				ret_info.v_track_id = -1;
			}

			if (g_issm_track_a_mulaw_id[i] != -1)
			{
				ret_info.a_track_id = g_issm_track_a_mulaw_id[i];
				ret_info.a_track_type = g_issm_track_a_mulaw[i].audio_type;
			}
			else
			{
				ret_info.a_track_id = -1;
			}

			if (g_issm_track_m_meta_id[i] != -1)
			{
				ret_info.m_track_id = g_issm_track_m_meta_id[i];
				ret_info.m_track_type = g_issm_track_m_meta[i].application_type;
			}
			else
			{
				ret_info.m_track_id = -1;
			}

			ret_info.stream_idx = (i < MAX_ISSM_V_CH ? 1 : 2);
			ret_info.ch_num = g_issm_ep_entry[i].ch;

			/*
			printf("[%s]\n", __FUNCTION__);
			printf("ep_id        : %d\n", ret_info.ep_id);
			printf("v_track_id   : %d\n", ret_info.v_track_id);
			printf("v_track_type : %d\n", ret_info.v_track_type);
			printf("a_track_id   : %d\n", ret_info.a_track_id);
			printf("a_track_type : %d\n", ret_info.a_track_type);
			printf("m_track_id   : %d\n", ret_info.m_track_id);
			printf("m_track_type : %d\n", ret_info.m_track_type);
			*/

			return ret_info;
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (g_issm_itx_ep_entry_id[i] == p_ep_id)
		{

		}
	}

	return ret_info;
}

int nf_issm_ctl_is_running()
{
	int ret;

	g_mutex_lock(g_issm_adapter_mutex);
	ret = g_issm_adapter_is_running;
	g_mutex_unlock(g_issm_adapter_mutex);

	return ret;
}

void nf_issm_ctl_set_restart_checker()
{
	g_issm_adapter_restart_checker = 0;
}

int nf_issm_ctl_get_restart_checker()
{
	return g_issm_adapter_restart_checker;
}

int nf_issm_ctl_mcast_start(int p_ch_num, int p_is_ipv6)
{
        int ret;

        g_mutex_lock(g_issm_adapter_mutex);

        if (g_issm_onvif_multicast_onoff[p_ch_num] == 1)
        {
                g_mutex_unlock(g_issm_adapter_mutex);
                return 0;
        }

        g_issm_onvif_multicast_onoff[p_ch_num] = 1;

        if (g_issm_adapter_is_running == 0)
        {
                g_mutex_unlock(g_issm_adapter_mutex);
                return 0;
        }

        ret = issm_set_multicast_start(g_issm_ep_entry_id[p_ch_num], p_is_ipv6);
        if (ret == ISSM_ERROR)
        {
                g_mutex_unlock(g_issm_adapter_mutex);
                return -1;
        }

        g_issm_onvif_multicast_task_id[p_ch_num] = ret;

        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
}

int nf_issm_ctl_mcast_stop(int p_ch_num)
{
        g_mutex_lock(g_issm_adapter_mutex);

        if (g_issm_onvif_multicast_onoff[p_ch_num] == 0)
        {
                g_mutex_unlock(g_issm_adapter_mutex);
                return 0;
        }

        g_issm_onvif_multicast_onoff[p_ch_num] = 0;

        if (g_issm_onvif_multicast_task_id[p_ch_num] == 0)
        {
                g_mutex_unlock(g_issm_adapter_mutex);
                return 0;
        }

        if (g_issm_adapter_is_running == 0)
        {
                g_mutex_unlock(g_issm_adapter_mutex);
                return 0;
        }

        issm_set_multicast_stop(g_issm_onvif_multicast_task_id[p_ch_num]);
        g_issm_onvif_multicast_task_id[p_ch_num] = 0;

        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
}

int nf_issm_ctl_zero_mcast_start(int p_is_ipv6)
{
    int ret;

    g_mutex_lock(g_issm_adapter_mutex);

    if (g_issm_zero_multicast_onoff == 1)
    {
        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
    }

    g_issm_zero_multicast_onoff = 1;

    if (g_issm_adapter_is_running == 0 || g_issm_zero_ep_entry_id == -1)
    {
        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
    }

    ret = issm_set_multicast_start(g_issm_zero_ep_entry_id, p_is_ipv6);
    if (ret == ISSM_ERROR)
    {
        printf("[%s][%d] issm_set_multicast_start failed for zero channel\n",
               __FUNCTION__, __LINE__);
        g_mutex_unlock(g_issm_adapter_mutex);
        return -1;
    }

    g_issm_zero_multicast_task_id = ret;

    g_mutex_unlock(g_issm_adapter_mutex);
    return 0;
}

int nf_issm_ctl_zero_mcast_stop(void)
{
    g_mutex_lock(g_issm_adapter_mutex);

    if (g_issm_zero_multicast_onoff == 0)
    {
        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
    }

    g_issm_zero_multicast_onoff = 0;

    if (g_issm_zero_multicast_task_id == 0)
    {
        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
    }

    if (g_issm_adapter_is_running == 0)
    {
        g_mutex_unlock(g_issm_adapter_mutex);
        return 0;
    }

    issm_set_multicast_stop(g_issm_zero_multicast_task_id);
    g_issm_zero_multicast_task_id = 0;

    g_mutex_unlock(g_issm_adapter_mutex);
    return 0;
}

int nf_issm_create_entrypoints()
{
	int idx, id, i, enc_type;
	int video_type[MAX_ISSM_CH];

	// standard rtsp entrypoints
	memset(g_issm_ep_entry, 		0x00, sizeof(ISSM_EP_ENTRY)*MAX_ISSM_CH);
	memset(g_issm_track_v_h264, 	0x00, sizeof(ISSM_EP_TRACK)*MAX_ISSM_CH);
	memset(g_issm_track_v_h265, 	0x00, sizeof(ISSM_EP_TRACK)*MAX_ISSM_CH);
	memset(g_issm_track_v_mjpeg, 	0x00, sizeof(ISSM_EP_TRACK)*MAX_ISSM_CH);
	memset(g_issm_track_a_mulaw, 	0x00, sizeof(ISSM_EP_TRACK)*MAX_ISSM_CH);
	memset(g_issm_track_m_meta, 	0x00, sizeof(ISSM_EP_TRACK)*MAX_ISSM_CH);
	memset(g_issm_track_backch, 	0x00, sizeof(ISSM_EP_TRACK)*MAX_ISSM_CH);

	for (i = 0; i < MAX_ISSM_CH; i++)
	{
		char sysdb_str[100] = { 0, };

		g_issm_ep_entry_id[i] = -1;
		g_issm_track_v_h264_id[i] = -1;
		g_issm_track_v_h265_id[i] = -1;
		g_issm_track_v_mjpeg_id[i] = -1;
		g_issm_track_a_mulaw_id[i] = -1;
		g_issm_track_m_meta_id[i] = -1;

		g_issm_ep_entry_pb_id[i] = -1;
		g_issm_track_v_h264_pb_id[i] = -1;
		g_issm_track_v_h265_pb_id[i] = -1;
		g_issm_track_v_mjpeg_pb_id[i] = -1;
		g_issm_track_a_mulaw_pb_id[i] = -1;
		g_issm_track_m_meta_pb_id[i] = -1;

		// cam.C0.stream.S0.vcodec
		snprintf(sysdb_str, sizeof(sysdb_str), "cam.C%d.stream.S%d.vcodec", i % MAX_ISSM_V_CH, i < MAX_ISSM_V_CH ? 0 : 1);
		if (!strcmp(nf_sysdb_get_str_nocopy(sysdb_str), "H.264"))
			video_type[i] = 0;
		else
			video_type[i] = 1;
	}

	for (i = 0; i < 2; i++)
	{
		g_issm_itx_ep_entry_id[i] = -1;
		g_issm_itx_track_id[i] = -1;
	}

	if (g_issm_is_enc)
		enc_type = ISSM_EP_ENCRYPT_TYPE_SEED128;
	else
		enc_type = ISSM_EP_ENCRYPT_TYPE_NONE;
#if 1
    // Zero-channel entry point (live only)
    snprintf(g_issm_zero_ep_entry.addr, sizeof(g_issm_zero_ep_entry.addr), "zero");
    g_issm_zero_ep_entry.internet_protocol = ISSM_EP_IP_TCP|ISSM_EP_IP_UDP|ISSM_EP_IP_MULTICAST;
    g_issm_zero_ep_entry.streaming_protocol = ISSM_EP_SP_RTSP;
    g_issm_zero_ep_entry.authorization_type = ISSM_EP_AUTH_TYPE_DIGEST;
    g_issm_zero_ep_entry.recording_type = ISSM_EP_RECORDING_TYPE_DVR | ISSM_EP_RECORDING_TYPE_LIVE;
    
    issm_create_entrypoint(g_issm_zero_ep_entry, &g_issm_zero_ep_entry_id);
    
    snprintf(g_issm_zero_track.track, sizeof(g_issm_zero_track.track), "video0");
    g_issm_zero_track.track_type = ISSM_EP_TRACK_TYPE_VIDEO;
    g_issm_zero_track.encryption_type = enc_type;
    g_issm_zero_track.video_type = ISSM_EP_VIDEO_TYPE_H264;
	g_issm_zero_track.qos = -1;
    g_issm_zero_track.func_p_ref = sd_ref_func;
    g_issm_zero_track.func_p_unref = sd_unref_func;
    g_issm_zero_track.func_p_get_data = sd_get_data_without_ich_func;
    g_issm_zero_track.func_p_get_info = sd_get_info_func;
    
    issm_create_track(g_issm_zero_ep_entry_id, g_issm_zero_track, &g_issm_zero_track_id);

	//printf("[%s][%d] Zero-channel entry point created with ep_id=%d, track_id=%d\n",
		//__FUNCTION__, __LINE__, g_issm_zero_ep_entry_id, g_issm_zero_track_id);
#endif
	// live
	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		// main
		idx = i;

		snprintf(g_issm_ep_entry[idx].addr, sizeof(g_issm_ep_entry[idx].addr), "live/main%d", i);
		g_issm_ep_entry[idx].ch					= i;
		g_issm_ep_entry[idx].internet_protocol	= ISSM_EP_IP_TCP|ISSM_EP_IP_UDP|ISSM_EP_IP_MULTICAST;
		g_issm_ep_entry[idx].streaming_protocol	= ISSM_EP_SP_RTSP;
		g_issm_ep_entry[idx].authorization_type	= ISSM_EP_AUTH_TYPE_DIGEST;
		g_issm_ep_entry[idx].recording_type		= ISSM_EP_RECORDING_TYPE_DVR|ISSM_EP_RECORDING_TYPE_LIVE;

		issm_create_entrypoint(g_issm_ep_entry[idx], &id);
		g_issm_ep_entry_id[idx] = id;

		if (video_type[idx] == 0)
		{
			snprintf(g_issm_track_v_h264[idx].track, sizeof(g_issm_track_v_h264[idx].track), "video0");
			g_issm_track_v_h264[idx].track_type			= ISSM_EP_TRACK_TYPE_VIDEO;
			g_issm_track_v_h264[idx].encryption_type	= enc_type;
			g_issm_track_v_h264[idx].video_type			= ISSM_EP_VIDEO_TYPE_H264;
			g_issm_track_v_h264[idx].qos				= -1;
			g_issm_track_v_h264[idx].func_p_ref			= sd_ref_func;
			g_issm_track_v_h264[idx].func_p_unref		= sd_unref_func;
			g_issm_track_v_h264[idx].func_p_get_data	= sd_get_data_without_ich_func;
			g_issm_track_v_h264[idx].func_p_get_info	= sd_get_info_func;

			issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_v_h264[idx], &id);
			g_issm_track_v_h264_id[idx] = id;
		}
		else if (video_type[idx] == 1)
		{
			snprintf(g_issm_track_v_h265[idx].track, sizeof(g_issm_track_v_h265[idx].track), "video0");
			g_issm_track_v_h265[idx].track_type			= ISSM_EP_TRACK_TYPE_VIDEO;
			g_issm_track_v_h265[idx].encryption_type	= enc_type;
			g_issm_track_v_h265[idx].video_type			= ISSM_EP_VIDEO_TYPE_H265;
			g_issm_track_v_h265[idx].qos				= -1;
			g_issm_track_v_h265[idx].func_p_ref			= sd_ref_func;
			g_issm_track_v_h265[idx].func_p_unref		= sd_unref_func;
			g_issm_track_v_h265[idx].func_p_get_data	= sd_get_data_without_ich_func;
			g_issm_track_v_h265[idx].func_p_get_info	= sd_get_info_func;

			issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_v_h265[idx], &id);
			g_issm_track_v_h265_id[idx] = id;
		}

		snprintf(g_issm_track_a_mulaw[idx].track, sizeof(g_issm_track_a_mulaw[idx].track), "audio0");
		g_issm_track_a_mulaw[idx].track_type		= ISSM_EP_TRACK_TYPE_AUDIO;
		g_issm_track_a_mulaw[idx].encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		g_issm_track_a_mulaw[idx].audio_type		= ISSM_EP_AUDIO_TYPE_MULAW;
		g_issm_track_a_mulaw[idx].qos				= -1;
		g_issm_track_a_mulaw[idx].func_p_ref		= sd_ref_func;
		g_issm_track_a_mulaw[idx].func_p_unref		= sd_unref_func;
		g_issm_track_a_mulaw[idx].func_p_get_data	= sd_get_data_without_ich_func;
		g_issm_track_a_mulaw[idx].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_a_mulaw[idx], &id);
		g_issm_track_a_mulaw_id[idx] = id;

		snprintf(g_issm_track_m_meta[idx].track, sizeof(g_issm_track_m_meta[idx].track), "meta0");
		g_issm_track_m_meta[idx].track_type			= ISSM_EP_TRACK_TYPE_APPLICATION;
		g_issm_track_m_meta[idx].encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		g_issm_track_m_meta[idx].application_type	= ISSM_EP_APPLICATION_TYPE_ONVIF;
		g_issm_track_m_meta[idx].qos				= -1;
		g_issm_track_m_meta[idx].func_p_ref			= sd_ref_func;
		g_issm_track_m_meta[idx].func_p_unref		= sd_unref_func;
		g_issm_track_m_meta[idx].func_p_get_data	= sd_get_data_without_ich_func;
		g_issm_track_m_meta[idx].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_m_meta[idx], &id);
		g_issm_track_m_meta_id[idx] = id;

		snprintf(g_issm_track_backch[idx].track, sizeof(g_issm_track_backch[idx].track), "backchannel");
		g_issm_track_backch[idx].track_type			= ISSM_EP_TRACK_TYPE_BACKCHANNEL;
		g_issm_track_backch[idx].encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		g_issm_track_backch[idx].audio_type			= ISSM_EP_AUDIO_TYPE_MULAW_IN;
		g_issm_track_backch[idx].qos				= -1;
		g_issm_track_backch[idx].func_p_ref			= sd_ref_func;
		g_issm_track_backch[idx].func_p_unref		= sd_unref_func;
		g_issm_track_backch[idx].func_p_get_data	= sd_get_data_without_ich_func;
		g_issm_track_backch[idx].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_backch[idx], &id);
		//g_issm_track_a_mulaw_id[idx] = id;

		// second
		idx = i + MAX_ISSM_V_CH;

		snprintf(g_issm_ep_entry[idx].addr, sizeof(g_issm_ep_entry[idx].addr), "live/second%d", i);
		g_issm_ep_entry[idx].ch					= i;
		g_issm_ep_entry[idx].internet_protocol	= ISSM_EP_IP_TCP|ISSM_EP_IP_UDP|ISSM_EP_IP_MULTICAST;
		g_issm_ep_entry[idx].streaming_protocol	= ISSM_EP_SP_RTSP;
		g_issm_ep_entry[idx].authorization_type	= ISSM_EP_AUTH_TYPE_DIGEST;
		g_issm_ep_entry[idx].recording_type		= ISSM_EP_RECORDING_TYPE_DVR|ISSM_EP_RECORDING_TYPE_LIVE;

		issm_create_entrypoint(g_issm_ep_entry[idx], &id);
		g_issm_ep_entry_id[idx] = id;

		if (video_type[idx] == 0)
		{
			sprintf(g_issm_track_v_h264[idx].track, "video0");
			g_issm_track_v_h264[idx].track_type			= ISSM_EP_TRACK_TYPE_VIDEO;
			g_issm_track_v_h264[idx].encryption_type	= enc_type;
			g_issm_track_v_h264[idx].video_type			= ISSM_EP_VIDEO_TYPE_H264;
			g_issm_track_v_h264[idx].qos				= -1;
			g_issm_track_v_h264[idx].func_p_ref			= sd_ref_func;
			g_issm_track_v_h264[idx].func_p_unref		= sd_unref_func;
			g_issm_track_v_h264[idx].func_p_get_data	= sd_get_data_without_ich_func;
			g_issm_track_v_h264[idx].func_p_get_info	= sd_get_info_func;

			issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_v_h264[idx], &id);
			g_issm_track_v_h264_id[idx] = id;
		}
		else if (video_type[idx] == 1)
		{
			snprintf(g_issm_track_v_h265[idx].track, sizeof(g_issm_track_v_h265[idx].track), "video0");
			g_issm_track_v_h265[idx].track_type			= ISSM_EP_TRACK_TYPE_VIDEO;
			g_issm_track_v_h265[idx].encryption_type	= enc_type;
			g_issm_track_v_h265[idx].video_type			= ISSM_EP_VIDEO_TYPE_H265;
			g_issm_track_v_h265[idx].qos				= -1;
			g_issm_track_v_h265[idx].func_p_ref			= sd_ref_func;
			g_issm_track_v_h265[idx].func_p_unref		= sd_unref_func;
			g_issm_track_v_h265[idx].func_p_get_data	= sd_get_data_without_ich_func;
			g_issm_track_v_h265[idx].func_p_get_info	= sd_get_info_func;

			issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_v_h265[idx], &id);
			g_issm_track_v_h265_id[idx] = id;
		}

		snprintf(g_issm_track_a_mulaw[idx].track, sizeof(g_issm_track_a_mulaw[idx].track), "audio0");
		g_issm_track_a_mulaw[idx].track_type		= ISSM_EP_TRACK_TYPE_AUDIO;
		g_issm_track_a_mulaw[idx].encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		g_issm_track_a_mulaw[idx].audio_type		= ISSM_EP_AUDIO_TYPE_MULAW;
		g_issm_track_a_mulaw[idx].qos				= -1;
		g_issm_track_a_mulaw[idx].func_p_ref		= sd_ref_func;
		g_issm_track_a_mulaw[idx].func_p_unref		= sd_unref_func;
		g_issm_track_a_mulaw[idx].func_p_get_data	= sd_get_data_without_ich_func;
		g_issm_track_a_mulaw[idx].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_a_mulaw[idx], &id);
		g_issm_track_a_mulaw_id[idx] = id;

		snprintf(g_issm_track_m_meta[idx].track, sizeof(g_issm_track_m_meta[idx].track), "meta0");
		g_issm_track_m_meta[idx].track_type			= ISSM_EP_TRACK_TYPE_APPLICATION;
		g_issm_track_m_meta[idx].encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		g_issm_track_m_meta[idx].application_type	= ISSM_EP_APPLICATION_TYPE_ONVIF;
		g_issm_track_m_meta[idx].qos				= -1;
		g_issm_track_m_meta[idx].func_p_ref			= sd_ref_func;
		g_issm_track_m_meta[idx].func_p_unref		= sd_unref_func;
		g_issm_track_m_meta[idx].func_p_get_data	= sd_get_data_without_ich_func;
		g_issm_track_m_meta[idx].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_m_meta[idx], &id);
		g_issm_track_m_meta_id[idx] = id;

		snprintf(g_issm_track_backch[idx].track, sizeof(g_issm_track_backch[idx].track), "backchannel");
		g_issm_track_backch[idx].track_type			= ISSM_EP_TRACK_TYPE_BACKCHANNEL;
		g_issm_track_backch[idx].encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		g_issm_track_backch[idx].audio_type			= ISSM_EP_AUDIO_TYPE_MULAW_IN;
		g_issm_track_backch[idx].qos				= -1;
		g_issm_track_backch[idx].func_p_ref			= sd_ref_func;
		g_issm_track_backch[idx].func_p_unref		= sd_unref_func;
		g_issm_track_backch[idx].func_p_get_data	= sd_get_data_without_ich_func;
		g_issm_track_backch[idx].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_ep_entry_id[idx], g_issm_track_backch[idx], &id);
		//g_issm_track_a_mulaw_id[idx] = id;
	}

	// playback
	for (i = 0; i < NUM_ACTIVE_CH; i++)
	{
		int id, id_v, id_a, id_m;
		ISSM_EP_ENTRY ep_entry;
		ISSM_EP_TRACK ep_track_v;
		ISSM_EP_TRACK ep_track_a;
		ISSM_EP_TRACK ep_track_m;
		static int onvif_track_token_cnt;

		memset(&ep_entry,   0x00, sizeof(ep_entry));
		memset(&ep_track_v, 0x00, sizeof(ep_track_v));
		memset(&ep_track_a, 0x00, sizeof(ep_track_a));
		memset(&ep_track_m, 0x00, sizeof(ep_track_m));

		snprintf(ep_entry.addr, sizeof(ep_entry.addr), "playback%d", i);
		ep_entry.ch					= i;
		ep_entry.internet_protocol	= ISSM_EP_IP_TCP|ISSM_EP_IP_UDP;
		ep_entry.streaming_protocol	= ISSM_EP_SP_RTSP;
		ep_entry.authorization_type	= ISSM_EP_AUTH_TYPE_DIGEST;
		ep_entry.recording_type		= ISSM_EP_RECORDING_TYPE_DVR|ISSM_EP_RECORDING_TYPE_PLAYBACK;
		issm_create_entrypoint(ep_entry, &id);
		g_issm_ep_entry_pb_id[i] = id;

		if (video_type[i] == 0)
		{
			snprintf(ep_track_v.track, sizeof(ep_track_v.track), "video0");
			ep_track_v.track_type		= ISSM_EP_TRACK_TYPE_VIDEO;
			ep_track_v.encryption_type	= enc_type;
			ep_track_v.video_type		= ISSM_EP_VIDEO_TYPE_H264;
			ep_track_v.func_p_ref		= sd_ref_func;
			ep_track_v.func_p_unref		= sd_unref_func;
			ep_track_v.func_p_get_data	= sd_get_data_without_ich_func;
			ep_track_v.func_p_get_info	= sd_get_info_func;
			issm_create_track(id, ep_track_v, &id_v);
			g_issm_track_v_h264_pb_id[i] = id_v;
		}
		else if (video_type[i] == 1)
		{
			snprintf(ep_track_v.track, sizeof(ep_track_v.track), "video0");
			ep_track_v.track_type		= ISSM_EP_TRACK_TYPE_VIDEO;
			ep_track_v.encryption_type	= enc_type;
			ep_track_v.video_type		= ISSM_EP_VIDEO_TYPE_H265;
			ep_track_v.func_p_ref		= sd_ref_func;
			ep_track_v.func_p_unref		= sd_unref_func;
			ep_track_v.func_p_get_data	= sd_get_data_without_ich_func;
			ep_track_v.func_p_get_info	= sd_get_info_func;
			issm_create_track(id, ep_track_v, &id_v);
			g_issm_track_v_h265_pb_id[i] = id_v;
		}

		snprintf(ep_track_a.track, sizeof(ep_track_a.track), "audio0");
		ep_track_a.track_type		= ISSM_EP_TRACK_TYPE_AUDIO;
		ep_track_a.encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		ep_track_a.audio_type		= ISSM_EP_AUDIO_TYPE_MULAW;
		ep_track_a.func_p_ref		= sd_ref_func;
		ep_track_a.func_p_unref		= sd_unref_func;
		ep_track_a.func_p_get_data	= sd_get_data_without_ich_func;
		ep_track_a.func_p_get_info	= sd_get_info_func;
		issm_create_track(id, ep_track_a, &id_a);
		g_issm_track_a_mulaw_pb_id[i] = id_a;

		snprintf(ep_track_m.track, sizeof(ep_track_a.track), "meta0");
		ep_track_a.track_type		= ISSM_EP_TRACK_TYPE_APPLICATION;
		ep_track_a.encryption_type	= ISSM_EP_ENCRYPT_TYPE_NONE;
		ep_track_a.application_type	= ISSM_EP_APPLICATION_TYPE_ONVIF;
		ep_track_a.func_p_ref		= sd_ref_func;
		ep_track_a.func_p_unref		= sd_unref_func;
		ep_track_a.func_p_get_data	= sd_get_data_without_ich_func;
		ep_track_a.func_p_get_info	= sd_get_info_func;
		issm_create_track(id, ep_track_m, &id_m);
		g_issm_track_m_meta_pb_id[i] = id_m;

		onvif_track_token_cnt++;
	}

	// itx rtsp entrypont
	memset(g_issm_itx_ep_entry, 0x00, sizeof(ISSM_EP_ENTRY)*2);
	memset(g_issm_itx_track, 0x00, sizeof(ISSM_EP_TRACK)*2);

	{
		int id;

		// live
		snprintf(g_issm_itx_ep_entry[0].addr, sizeof(g_issm_itx_ep_entry[0].addr), "live");
		g_issm_itx_ep_entry[0].internet_protocol	= ISSM_EP_IP_TCP;
		g_issm_itx_ep_entry[0].streaming_protocol	= ISSM_EP_SP_ITX_RTSP;
		g_issm_itx_ep_entry[0].authorization_type	= ISSM_EP_AUTH_TYPE_DIGEST;
		g_issm_itx_ep_entry[0].recording_type		= ISSM_EP_RECORDING_TYPE_DVR|ISSM_EP_RECORDING_TYPE_LIVE;

		issm_create_entrypoint(g_issm_itx_ep_entry[0], &id);
		g_issm_itx_ep_entry_id[0] = id;

		snprintf(g_issm_itx_track[0].track, sizeof(g_issm_itx_track[0].track), "track0");
		g_issm_itx_track[0].track_type		= ISSM_EP_TRACK_TYPE_VIDEO;
		g_issm_itx_track[0].encryption_type	= enc_type;
		g_issm_itx_track[0].video_type		= ISSM_EP_VIDEO_TYPE_H264;
		g_issm_itx_track[0].audio_type		= ISSM_EP_AUDIO_TYPE_MULAW;
		g_issm_itx_track[0].func_p_ref		= sd_ref_func;
		g_issm_itx_track[0].func_p_unref	= sd_unref_func;
		g_issm_itx_track[0].func_p_get_data	= sd_get_data_with_ich_func;
		g_issm_itx_track[0].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_itx_ep_entry_id[0], g_issm_itx_track[0], &id);
		g_issm_itx_track_id[0] = id;

		// playback
		snprintf(g_issm_itx_ep_entry[1].addr, sizeof(g_issm_itx_ep_entry[1].addr), "playback");
		g_issm_itx_ep_entry[1].internet_protocol	= ISSM_EP_IP_TCP;
		g_issm_itx_ep_entry[1].streaming_protocol	= ISSM_EP_SP_ITX_RTSP;
		g_issm_itx_ep_entry[1].authorization_type	= ISSM_EP_AUTH_TYPE_DIGEST;
		g_issm_itx_ep_entry[1].recording_type		= ISSM_EP_RECORDING_TYPE_DVR|ISSM_EP_RECORDING_TYPE_PLAYBACK;

		issm_create_entrypoint(g_issm_itx_ep_entry[1], &id);
		g_issm_itx_ep_entry_id[1] = id;

		snprintf(g_issm_itx_track[1].track, sizeof(g_issm_itx_track[1].track), "track0");
		g_issm_itx_track[1].track_type		= ISSM_EP_TRACK_TYPE_VIDEO;
		g_issm_itx_track[1].encryption_type	= enc_type;
		g_issm_itx_track[1].video_type		= ISSM_EP_VIDEO_TYPE_H264;
		g_issm_itx_track[1].audio_type		= ISSM_EP_AUDIO_TYPE_MULAW;
		g_issm_itx_track[1].func_p_ref		= sd_ref_func;
		g_issm_itx_track[1].func_p_unref	= sd_unref_func;
		g_issm_itx_track[1].func_p_get_data	= sd_get_data_with_ich_func;
		g_issm_itx_track[1].func_p_get_info	= sd_get_info_func;

		issm_create_track(g_issm_itx_ep_entry_id[1], g_issm_itx_track[1], &id);
		g_issm_itx_track_id[1] = id;
	}

	//issm_print_entrypoints();

	return ISSM_OK;
}


int nf_issm_cb_get_password(void *p_data)
{
	ISSM_CB_USERINFO *data;
	int user_idx;
	char passwd[128];
	int ret;

	data = p_data;

	ret = nf_issm_sysman_get_user_idx_from_name(data->id);
	if (ret == ISSM_ERROR)
		return ISSM_ERROR;

	user_idx = ret;

	ret = nf_issm_sysman_get_user_pass_from_idx(user_idx, data->password, sizeof(data->password));
	if (ret == ISSM_ERROR)
		return ISSM_ERROR;


	ret = nf_issm_sysman_get_user_audio_auth(user_idx, &(data->is_audio_tx_enable), &(data->is_audio_rx_enable));
	if (ret == ISSM_ERROR)
		return ISSM_ERROR;

	//printf("[%s] id          : %s\n", __FUNCTION__, data->id);
	//printf("[%s] password    : %s\n", __FUNCTION__, data->password);
	//printf("[%s] a_enable    : %d\n", __FUNCTION__, data->is_audio_tx_enable);
	//printf("[%s] b_enable    : %d\n", __FUNCTION__, data->is_audio_rx_enable);

	return ISSM_OK;
}

int nf_issm_cb_get_udp_port(void *p_data)
{
	ISSM_CB_UDP_PORT *data;
	data = p_data;

	data->port_start = (int)nf_sysdb_get_uint("net.rtp.rtpsport");
	data->port_end = (int)nf_sysdb_get_uint("net.rtp.rtpeport");

	return ISSM_OK;
}

int nf_issm_cb_get_streaming_info(void *p_data)
{
	ISSM_CB_GET_STREAMING_INFO *data;
	ISSM_EP_INFO ep_info;
	int ep_id;
	int task_id;
	int ret, i;

	data = p_data;

	ep_id = data->ep_id;
	task_id = data->task_id;

	ep_info = nf_issm_ctl_get_ep_info(ep_id);
	if (ep_info.ep_id == -1)
	{
		data->bitrate_step = 1;
		data->is_i_only = 0;
		data->is_worst_step = 0;
	}
	else
	{
		// Todo : Implement bandwidth scenario.
		data->bitrate_step = 1;
		data->is_i_only = 0;
		data->is_worst_step = 0;
		data->stream_idx = ep_info.stream_idx;
	}

	if (data->stream_idx == 2)
	{
		// Reference to nf_codec_header.h
		// 		NF_FPS_CR01 = 1, // F
		gchar *sysdb_netstream_fps;

		sysdb_netstream_fps = nf_sysdb_get_str_nocopy("rec.netstream.fps");
		if (sysdb_netstream_fps == 0)
			return ISSM_ERROR;

		//printf("[%s][%d] fps(%s)\n", __FUNCTION__, __LINE__, sysdb_netstream_fps);

		if (data->streaming_protocol == ISSM_EP_SP_ITX_RTSP)
		{
			for (i = 0; i < NUM_ACTIVE_CH; i++)
			{
				if ((1<<i) & data->v_ch_mask)
				{
					if (sysdb_netstream_fps[i] == 'F')
					{
						//printf("[%s][%d] i(%d)\n", __FUNCTION__, __LINE__, i);
						data->is_1fps_streaming[i] = 1;
					}
				}
			}
		}
		else
		{
			if (sysdb_netstream_fps[ep_info.ch_num] == 'F')
			{
				//printf("[%s][%d] i(0)\n", __FUNCTION__, __LINE__);
				data->is_1fps_streaming[0] = 1;
			}
		}
	}

	return ISSM_OK;
}

int nf_issm_cb_get_svr_ip(void *p_data)
{
	ISSM_CB_GET_SVR_IP *data;
	data = p_data;

	{
		unsigned int ipv4_uint, tmp[4];
		ipv4_uint = nf_sysdb_get_uint("net.proto.ipaddr");

		tmp[0] = (ipv4_uint >> 24) & 255;
		tmp[1] = (ipv4_uint >> 16) & 255;
		tmp[2] = (ipv4_uint >> 8) & 255;
		tmp[3] = ipv4_uint & 255;

		snprintf(data->ipv4, sizeof(data->ipv4), "%d.%d.%d.%d", tmp[0], tmp[1], tmp[2], tmp[3]);
	}

	{
		int is_ipv6_support;
		is_ipv6_support = (int)nf_sysdb_get_uint("net.ipv6.using");
		if (is_ipv6_support)
			snprintf(data->ipv6, sizeof(data->ipv6), "%s", nf_sysdb_get_str_nocopy("net.ipv6.addr0"));
	}

	//printf("[%s][%d] ipv4[%s]\n", __FUNCTION__, __LINE__, data->ipv4);
	//printf("[%s][%d] ipv6[%s]\n", __FUNCTION__, __LINE__, data->ipv6);

	return ISSM_OK;
}

int nf_issm_cb_get_mcast_info(void *p_data)
{
	ISSM_CB_GET_MCAST_INFO *data;
	int ch = -1;
	int track_type = 0;
	int stream_idx = 0;

	char mcast_addr[100] = { 0, };
	char mcast_port[100] = { 0, };
	char mcast_ttl[100] = { 0, };
	int i;

	data = p_data;

	//printf("ep_id       : %d\n", data->ep_id);
	//printf("ep_track_id : %d\n", data->ep_track_id);

	for (i = 0; i < MAX_ISSM_CH; i++)
	{
		if (g_issm_ep_entry_id[i] == data->ep_id)
		{
			ch = i;

			if (g_issm_track_v_h264_id[i] == data->ep_track_id)
				track_type = 0;
			if (g_issm_track_v_h265_id[i] == data->ep_track_id)
				track_type = 0;
			if (g_issm_track_v_mjpeg_id[i] == data->ep_track_id)
				track_type = 0;
			if (g_issm_track_a_mulaw_id[i] == data->ep_track_id)
				track_type = 1;
			if (g_issm_track_m_meta_id[i] == data->ep_track_id)
				track_type = 2;

			break;
		}
	}

    if (ch == -1 && g_issm_zero_ep_entry_id == data->ep_id)
    {
        if (data->is_ipv6)
        {
            snprintf(data->mcast_addr, sizeof(data->mcast_addr), "%s",
                     nf_sysdb_get_str_nocopy("net.rtp.multi.zero.ipv6.vaddr"));
            data->mcast_port1 = (int)nf_sysdb_get_uint("net.rtp.multi.zero.ipv6.vport");
            data->mcast_ttl   = (int)nf_sysdb_get_uint("net.rtp.multi.ipv6.ttl");
        }
        else
        {
            unsigned int ipv4_uint, tmp[4];
            ipv4_uint = nf_sysdb_get_uint("net.rtp.multi.zero.vaddr");
            tmp[0] = (ipv4_uint >> 24) & 255;
            tmp[1] = (ipv4_uint >> 16) & 255;
            tmp[2] = (ipv4_uint >>  8) & 255;
            tmp[3] =  ipv4_uint        & 255;
            snprintf(data->mcast_addr, sizeof(data->mcast_addr),
                     "%d.%d.%d.%d", tmp[0], tmp[1], tmp[2], tmp[3]);
            data->mcast_port1 = (int)nf_sysdb_get_uint("net.rtp.multi.zero.vport");
            data->mcast_ttl   = (int)nf_sysdb_get_uint("net.rtp.multi.ttl");
        }
        data->mcast_port2 = data->mcast_port1 + 1;
        return ISSM_OK;
    }	

	if (ch == -1)
	{
		EEE;
		//printf("[%s][%d] ch[%d] track_type[%d]\n", __FUNCTION__, __LINE__, ch, track_type);
		return ISSM_ERROR;
	}

	if (ch >= MAX_ISSM_V_CH)
		stream_idx = 1;
	ch %= MAX_ISSM_V_CH;

	//printf("ch : %d\n", ch);
	//printf("stream_idx : %d\n", stream_idx);
	//printf("track_type : %d\n", track_type);

	if (data->is_ipv6)
	{
		snprintf(mcast_addr, 100, "net.rtp.multi.S%d.C%d.ipv6.vaddr", stream_idx, ch);
		snprintf(mcast_ttl,  100, "net.rtp.multi.ipv6.ttl");

		switch (track_type)
		{
			case 0 : // Video
				snprintf(mcast_port, 100, "net.rtp.multi.S%d.C%d.ipv6.vport", stream_idx, ch);
				break;

			case 1 : // Audio
				snprintf(mcast_port, 100, "net.rtp.multi.S%d.C%d.ipv6.aport", stream_idx, ch);
				break;

			case 2 : // Meta
				snprintf(mcast_port, 100, "net.rtp.multi.S%d.C%d.ipv6.mport", stream_idx, ch);
				break;
		}
	}
	else
	{
		snprintf(mcast_addr, 100, "net.rtp.multi.S%d.C%d.vaddr", stream_idx, ch);
		snprintf(mcast_ttl,  100, "net.rtp.multi.ttl");

		switch (track_type)
		{
			case 0 : // Video
				snprintf(mcast_port, 100, "net.rtp.multi.S%d.C%d.vport", stream_idx, ch);
				break;

			case 1 : // Audio
				snprintf(mcast_port, 100, "net.rtp.multi.S%d.C%d.aport", stream_idx, ch);
				break;

			case 2 : // Meta
				snprintf(mcast_port, 100, "net.rtp.multi.S%d.C%d.mport", stream_idx, ch);
				break;
		}
	}

	if (data->is_ipv6)
	{
		snprintf(data->mcast_addr, sizeof(data->mcast_addr), "%s", nf_sysdb_get_str_nocopy(mcast_addr));
	}
	else
	{
		unsigned int ipv4_uint, tmp[4];
		ipv4_uint = nf_sysdb_get_uint(mcast_addr);

		tmp[0] = (ipv4_uint >> 24) & 255;
		tmp[1] = (ipv4_uint >> 16) & 255;
		tmp[2] = (ipv4_uint >> 8) & 255;
		tmp[3] = ipv4_uint & 255;

		snprintf(data->mcast_addr, sizeof(data->mcast_addr), "%d.%d.%d.%d", tmp[0], tmp[1], tmp[2], tmp[3]);
	}

	data->mcast_port1	= (int)nf_sysdb_get_uint(mcast_port);
	data->mcast_port2	= data->mcast_port1 + 1;
	data->mcast_ttl		= (int)nf_sysdb_get_uint(mcast_ttl);

	//printf("[%s] mcast_addr  [%u]\n", __FUNCTION__, data->mcast_addr);
	//printf("[%s] mcast_port1 [%d]\n", __FUNCTION__, data->mcast_port1);
	//printf("[%s] mcast_ttl   [%d]\n", __FUNCTION__, data->mcast_ttl);

	return ISSM_OK;
}

int nf_issm_cb_get_date_info(void *p_data)
{
	ISSM_CB_GET_DATE_INFO *data;
	DateTimeData dtdata;
	data = p_data;

	DAL_get_dateTime_data(&dtdata);
	data->dst = dtdata.dst;
	data->time_zone = dtdata.timeZone;

	return ISSM_OK;
}

int nf_issm_cb_get_covert(void *p_data)
{
	ISSM_CB_GET_COVERT *data;
	int user_idx;
	gchar user_covert[MAX_ISSM_V_CH+1] = { 0, };
	gchar group_covert[MAX_ISSM_V_CH+1] = { 0, };
	long long covert_bitmask = 0;
	int ch;
	int ret;

	data = p_data;

	ret = nf_issm_sysman_get_user_idx_from_name(data->user_id);
	if (ret == ISSM_ERROR)
		return ISSM_ERROR;

	user_idx = ret;

	// Get covert info of user id.
	{
		GValue ret_value = {0,};
		const gchar *str = NULL;
		gchar buf[MAX_ISSM_V_CH+1];

		ret = nf_sysdb_get_key1("usr.U%d.covert", user_idx, &ret_value, NULL);
		if (!ret)
			return ISSM_ERROR;

		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(user_covert, buf, sizeof(user_covert));
		g_value_unset(&ret_value);
	}

	// Get covert info of user id's group.
	{
		GValue ret_value = {0,};
		const gchar *str = NULL;
		gchar buf[32+1];
		gchar group_name[32+1];
		int sysdb_str_idx;
		static unsigned char *sysdb_str[3] = { "cam.C%d.cv_admin", "cam.C%d.cv_manager", "cam.C%d.cv_user" };

		ret = nf_sysdb_get_key1("usr.U%d.grpname", user_idx, &ret_value, NULL);
		if (!ret)
			return ISSM_ERROR;

		str = g_value_get_string(&ret_value);

		memset(buf, 0, sizeof(buf));
		g_stpcpy(buf, str);

		memcpy(group_name, buf, sizeof(group_name));
		g_value_unset(&ret_value);

		if (!strcmp(group_name, "ADMIN"))
			sysdb_str_idx = 0;
		else if (!strcmp(group_name, "MANAGER"))
			sysdb_str_idx = 1;
		else if (!strcmp(group_name, "USER"))
			sysdb_str_idx = 2;
		else
			return ISSM_ERROR;

		for (ch = 0; ch < NUM_ACTIVE_CH; ch++)
		{
			ret = nf_sysdb_get_key1(sysdb_str[sysdb_str_idx], ch, &ret_value, NULL);
			if (!ret)
				return ISSM_ERROR;

			if (g_value_get_boolean(&ret_value))
				group_covert[ch] = '1';
			else
				group_covert[ch] = '0';

			g_value_unset(&ret_value);
		}
	}

	//printf("[%s] user_id      : %s\n", __FUNCTION__, data->user_id);
	//printf("[%s] user_covert  : %s\n", __FUNCTION__, user_covert);
	//printf("[%s] group_covert : %s\n", __FUNCTION__, group_covert);
	//printf("[%s] ep_id        : %d\n", __FUNCTION__, data->ep_id);

	for (ch = 0; ch < NUM_ACTIVE_CH; ch++)
	{
		if (user_covert[ch] == '1' || group_covert[ch] == '1')
			covert_bitmask |= (1<<ch);
	}
	data->covert_bitmask = covert_bitmask;

	// ITX RTSP - live / playback
	if (data->ep_id == g_issm_itx_ep_entry_id[0] || data->ep_id == g_issm_itx_ep_entry_id[1])
	{
		return ISSM_OK;
	}
	else
	{
		if (data->covert_bitmask & (1<<data->ch))
			return ISSM_ERROR;
	}

	return ISSM_OK;
}

int nf_issm_cb_session_changed(void *p_data)
{
	ISSM_TASK_INFOS *task_infos;
	ISSM_TASK_INFO *task_info;
	int session_count = 0;
	static int pre_session_count = 0;
	long long mobile_mask = 0;
	int ret, i;

	nf_issm_ctl_bitrate_set_session_changed();

	ret = issm_get_task_infos(&task_infos);
	if (ret != ISSM_OK)
		return ISSM_OK;

	session_count = task_infos->task_cnt;
	if (session_count != pre_session_count)
	{
		nf_network_notify_net_status(0, session_count == 0 ? -2 : session_count, 0);
		pre_session_count = session_count;
	}

	for (i = 0; i < session_count; i++)
	{
		if (task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING)
		{
			if (g_issm_is_adaptive && task_infos->task_info[i].second_stream_change)
			{
				mobile_mask |= task_infos->task_info[i].ch_video_mask;
			}
		}
	}

	// Create event log about streaming
	if (p_data && strcmp("S1_OB", nf_sysdb_get_str_nocopy("sys.info.vendor")))
	{
		task_info = p_data;
		if (task_info->task_id)
		{
			char ip_buff[256];
			memset(ip_buff, 0x00, sizeof(ip_buff));
			sprintf(ip_buff,"%s: %s",task_info->login_id, task_info->client_ip);

			// Streaming session is created.
			if (task_info->client_status == ISSM_TM_RTSP_STATUS_STREAMING)
			{
				if (task_info->recording_type & ISSM_EP_RECORDING_TYPE_LIVE)
				{
					// RTP_LIVE_LOG_ON
					nf_eventlog_put_param( NULL, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_REMOTE_LOG_ON_LIVE_DISPLAY, ip_buff);
				}
				else if (task_info->recording_type & ISSM_EP_RECORDING_TYPE_PLAYBACK)
				{
					if (task_info->streaming_protocol & ISSM_EP_SP_RTSP)
					{
						// RTP_PB_LOG_ON
						nf_eventlog_put_param( NULL, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_REMOTE_LOG_ON_PLAY_BACK, ip_buff);
					}
					else
					{
						if (task_info->start_time == 0 || task_info->end_time == 0)
						{
							// RTP_PB_LOG_ON
							nf_eventlog_put_param( NULL, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_REMOTE_LOG_ON_PLAY_BACK, ip_buff);
						}
						else
						{
							// RTP_SEARCH_LOG_ON
							nf_eventlog_put_param( NULL, LT_REMOTE_LOG_ON, LOG_P1T_WHO, LP2_REMOTE_LOG_ON_ARCHIVING, ip_buff);
						}
					}
				}
			}
			// Streaming session is deleted.
			else if (task_info->client_status == ISSM_TM_RTSP_STATUS_CLOSE)
			{
				if (task_info->recording_type & ISSM_EP_RECORDING_TYPE_LIVE)
				{
					// RTP_LIVE_LOG_OUT
					nf_eventlog_put_param( NULL, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_REMOTE_LOG_OFF_LIVE_DISPLAY, ip_buff);
				}
				else if (task_info->recording_type & ISSM_EP_RECORDING_TYPE_PLAYBACK)
				{
					if (task_info->streaming_protocol & ISSM_EP_SP_RTSP)
					{
						// RTP_PB_LOG_OUT
						nf_eventlog_put_param( NULL, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_REMOTE_LOG_OFF_PLAY_BACK, ip_buff);
					}
					else
					{
						if (task_info->start_time == 0 || task_info->end_time == 0)
						{
							// RTP_PB_LOG_OUT
							nf_eventlog_put_param( NULL, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_REMOTE_LOG_OFF_PLAY_BACK, ip_buff);
						}
						else
						{
							// RTP_SEARCH_LOG_OUT
							nf_eventlog_put_param( NULL, LT_REMOTE_LOG_OFF, LOG_P1T_WHO, LP2_REMOTE_LOG_OFF_ARCHIVING, ip_buff);
						}
					}
				}

			}
		}
	}

	ret = issm_free_task_infos(&task_infos);

	if (!g_mutex_trylock(g_issm_adapter_mutex))
	{
		if (g_issm_adapter_is_running == 1)
			printf("[%s][%d] Failed to lock g_issm_adapter_mutex!!\n", __FUNCTION__, __LINE__);

		return ISSM_OK;
	}

	if (g_issm_adapter_is_running)
		ret = nf_record_register_handoff(MAX_ISSM_CH_MASK, &nf_issm_ctl_put_frame, mobile_mask);

	g_mutex_unlock(g_issm_adapter_mutex);

	return ISSM_OK;
}

int nf_issm_cb_get_enc_key(void *p_data)
{
	ISSM_CB_GET_ENC_KEY *data;
	data = p_data;

	if (data->enc_type == ISSM_EP_ENCRYPT_TYPE_SEED128)
	{
		unsigned char mac[32];
		char user_id[128] = { 0, };
		char passwd[128];
		unsigned char user_key[16 + 1] = { 0, }; // +1 is the space for null char.
		unsigned int round_key[32];
		int len, i, j;

		memset(mac, 0x00, sizeof(mac));
		nf_netif_get_mac_str(mac);

		snprintf(user_id, sizeof(user_id), "%s", data->enc_key);

		i = nf_issm_sysman_get_user_idx_from_name(user_id);
		if (i == -1)
		{
			EEE;
			return ISSM_ERROR;
		}

		nf_issm_sysman_get_user_pass_from_idx(i, passwd, 32);

		len = snprintf(user_key, sizeof(user_key), "%s%s", passwd, user_id);
		for (i = len, j = 0; i < 16 && j < 6; ++i,++j)
			user_key[i] = mac[j];

		SeedRoundKey(round_key, user_key);

		data->enc_key_size = sizeof(round_key);
		memcpy(data->enc_key, round_key, sizeof(round_key));

		return ISSM_OK;
	}

	EEE;
	return ISSM_ERROR;
}

int nf_issm_cb_get_audio_backch_port(void *p_data)
{
	int is_enabled;
	ISSM_CB_AUDIO_BACKCH_PORT *data;
	data = p_data;

	if (!nf_sysdb_get_bool("net.rtp.audio_backch_mode"))
	{
		int port;
		port = (int)nf_sysdb_get_uint("net.rtp.audio_backch_port");

		data->port_start = port;
		data->port_end = port + 1;

		return ISSM_OK;
	}

	return ISSM_ERROR;
}


static void nf_issm_sysdb_change_func(NF_NOTIFY_INFO *p_info, gpointer p_data)
{
	int i, is_issm_restart = 0;

	if (p_info->d.params[0] == NF_SYSDB_CATE_REC)
	{
		int current_val;

		current_val = (int)nf_sysdb_get_bool("rec.netstream.enable_stream_control");
		if (current_val != g_issm_is_adaptive)
		{
			g_issm_is_adaptive = current_val;
			nf_issm_cb_session_changed(0);
		}
	}
	else if (p_info->d.params[0] == NF_SYSDB_CATE_NET)
	{
		int is_bandwidth_enable;
		int max_bandwidth;
		int is_enc;

		max_bandwidth = (int)nf_sysdb_get_uint("net.proto.maxtxspeed");
		nf_issm_ctl_set_max_bandwidth(max_bandwidth);

		is_bandwidth_enable = 1;
		nf_issm_ctl_set_is_bandwidth_ctl_enable(is_bandwidth_enable);

		is_enc = (int)nf_sysdb_get_bool("net.proto.srtspon");
		if (g_issm_is_enc != is_enc)
		{
			g_issm_is_enc = is_enc;
			is_issm_restart++;
		}
	}
	else if (p_info->d.params[0] == NF_SYSDB_CATE_AUDIO)
	{
		g_issm_is_audio_tx_enable = (int)nf_sysdb_get_bool("audio.tx");
		g_issm_is_audio_rx_enable = (int)nf_sysdb_get_bool("audio.rx");
	}
	else if (p_info->d.params[0] == NF_SYSDB_CATE_CAM || p_info->d.params[0] == NF_IPCAM_CATE_STREAM)
	{
		for (i = 0; i < NUM_ACTIVE_CH; i++)
		{
			char sysdb_str_1st[100] = { 0, }, sysdb_str_2nd[100] = { 0, };
			int idx;

			// main
			idx = i;
			snprintf(sysdb_str_1st, sizeof(sysdb_str_1st), "cam.C%d.stream.S%d.vcodec", idx % MAX_ISSM_V_CH, idx < MAX_ISSM_V_CH ? 0 : 1);
			if (!strcmp(nf_sysdb_get_str_nocopy(sysdb_str_1st), "H.264"))
			{
				if (g_issm_track_v_h264_id[idx] == -1)
					is_issm_restart++;
			}
			else
			{
				if (g_issm_track_v_h265_id[idx] == -1)
					is_issm_restart++;
			}

			// second
			idx = i + MAX_ISSM_V_CH;
			snprintf(sysdb_str_1st, sizeof(sysdb_str_1st), "cam.C%d.stream.S%d.vcodec", idx % MAX_ISSM_V_CH, idx < MAX_ISSM_V_CH ? 0 : 1);
			if (!strcmp(nf_sysdb_get_str_nocopy(sysdb_str_1st), "H.264"))
			{
				if (g_issm_track_v_h264_id[idx] == -1)
					is_issm_restart++;
			}
			else
			{
				if (g_issm_track_v_h265_id[idx] == -1)
					is_issm_restart++;
			}
		}
	}

	// Prvt_dup scenario
	if (p_info->d.params[0] == NF_SYSDB_CATE_SYS)
	{
		int arg_policy;
		int dev_block;

		arg_policy = (int)nf_sysdb_get_bool("sys.info.agr_policy");
		dev_block = (int)nf_sysdb_get_bool("sys.info.guard.dev_block");

		if (g_arg_policy != arg_policy)
			is_issm_restart++;
		g_arg_policy = arg_policy;

		if (g_dev_block != dev_block)
			is_issm_restart++;
		g_dev_block = dev_block;
	}

	if (is_issm_restart)
	{
		nf_issm_ctl(ISSM_STOP, __FUNCTION__);
		nf_issm_ctl(ISSM_START, __FUNCTION__);
	}
}

static void nf_issm_vloss_change_func(NF_NOTIFY_INFO *pinfo, gpointer data)
{
	unsigned int ch_mask;
	int ret;

	ch_mask = ~(pinfo->d.params[0]);
	ret = issm_set_alive_ch_mask(ch_mask);
}

static int nf_issm_sysman_get_user_idx_from_name(char *p_user_name)
{
	int user_count;
	char *sysdb_name;
	char str_buf[32];
	int i;

	if (p_user_name == NULL)
		return ISSM_ERROR;

	user_count = nf_sysdb_get_uint("usr.UCNT");

	for (i = 0; i < user_count; i++)
	{
		snprintf(str_buf, 32, "usr.U%d.name", i);

		sysdb_name = nf_sysdb_get_str_nocopy(str_buf);

		if (sysdb_name == NULL)
			return ISSM_ERROR;

		if (!strcmp(sysdb_name, p_user_name))
			return i;
	}

	return ISSM_ERROR;
}

static int nf_issm_sysman_get_user_pass_from_idx(int p_user_idx, char *p_user_pass, int p_pass_len)
{
	int user_count;
	char str_buf[32];
	char *sysdb_pass;

	if (p_user_idx < 0)
		return ISSM_ERROR;

	if (p_user_pass == NULL || p_pass_len < 32)
		return ISSM_ERROR;

	user_count = nf_sysdb_get_uint("usr.UCNT");
	if (user_count < p_user_idx)
		return ISSM_ERROR;

	snprintf(str_buf, 32, "usr.U%d.pass", p_user_idx);

	sysdb_pass = nf_sysdb_get_str_nocopy(str_buf);

	if (sysdb_pass == NULL)
		return ISSM_ERROR;

	strncpy(p_user_pass, sysdb_pass, strlen(sysdb_pass) + 1);

	return ISSM_OK;
}

static int nf_issm_sysman_get_user_audio_auth(int p_user_idx, int *p_is_audio_tx_enable, int *p_is_audio_rx_enable)
{
	char str_buf[32];
	char *sysdb_group;
	const char sysdb_str_grp_admin[] = "ADMIN";
	const char sysdb_str_grp_manager[] = "MANAGER";
	const char sysdb_str_grp_user[] = "USER";
	int group_idx;

	if (p_user_idx < 0)
		return ISSM_ERROR;

	memset(str_buf, 0x00, sizeof(str_buf));
	snprintf(str_buf, sizeof(str_buf), "usr.U%d.grpname", p_user_idx);

	sysdb_group = nf_sysdb_get_str_nocopy(str_buf);
	if (sysdb_group == NULL)
		return ISSM_ERROR;

	if (strncmp(sysdb_group, sysdb_str_grp_admin,
			strlen(sysdb_group) > strlen(sysdb_str_grp_admin) ? strlen(sysdb_str_grp_admin) : strlen(sysdb_group)) == 0)
	{
		group_idx = 0;
	}
	else if (strncmp(sysdb_group, sysdb_str_grp_manager,
			strlen(sysdb_group) > strlen(sysdb_str_grp_manager) ? strlen(sysdb_str_grp_manager) : strlen(sysdb_group)) == 0)
	{
		group_idx = 1;
	}
	else if (strncmp(sysdb_group, sysdb_str_grp_user,
			strlen(sysdb_group) > strlen(sysdb_str_grp_user) ? strlen(sysdb_str_grp_user) : strlen(sysdb_group)) == 0)
	{
		group_idx = 2;
	}
	else
	{
		return ISSM_ERROR;
	}

	memset(str_buf, 0x00, sizeof(str_buf));
	snprintf(str_buf, sizeof(str_buf), "usr.grp.G%d.audio", group_idx);
	*p_is_audio_tx_enable = (int)nf_sysdb_get_bool(str_buf);

	memset(str_buf, 0x00, sizeof(str_buf));
	snprintf(str_buf, sizeof(str_buf), "usr.grp.G%d.microphone", group_idx);
	*p_is_audio_rx_enable = (int)nf_sysdb_get_bool(str_buf);

	return ISSM_OK;
}

static void nf_issm_ctl_thread_func(void)
{
	NF_ISSM_CTL_CMD *cmd;

	while (1)
	{
		if (g_issm_cmd_array->len == 0)
		{
			g_usleep(10*1000);
			continue;
		}

		g_mutex_lock(g_issm_cmd_mutex);

		// Remove duplicated cmd pair (Start/Stop)
		if (g_issm_cmd_array->len > 3)
		{
			NF_ISSM_CTL_CMD *cmd_start, *cmd_stop;
			int last_start, last_stop;
			int i;

			last_start = 0;
			last_stop = 0;

			// find last start/stop pair
			for (i = g_issm_cmd_array->len - 1; i >= 1; i--)
			{
				cmd_start = (NF_ISSM_CTL_CMD*)g_ptr_array_index(g_issm_cmd_array, i - 1);
				cmd_stop = (NF_ISSM_CTL_CMD*)g_ptr_array_index(g_issm_cmd_array, i);

				if (cmd_start->type == ISSM_START && cmd_stop->type == ISSM_STOP)
				{
					last_start = i - 1;
					last_stop = i;
					break;
				}
			}

			if (last_stop)
			{
				for (i = 0; i < last_start; i++)
				{
					cmd_start = (NF_ISSM_CTL_CMD*)g_ptr_array_index(g_issm_cmd_array, i);
					cmd_stop = (NF_ISSM_CTL_CMD*)g_ptr_array_index(g_issm_cmd_array, i + 1);

					if (cmd_start->type == ISSM_START && cmd_stop->type == ISSM_STOP)
					{
						g_ptr_array_remove_index(g_issm_cmd_array, (guint)i);

						if (cmd_start->log[0] != 0)
							printf(" log[%s]\n", cmd_start->log);
						else
							printf("\n");

						g_ptr_array_remove_index(g_issm_cmd_array, (guint)i);

						if (cmd_stop->log[0] != 0)
							printf(" log[%s]\n", cmd_stop->log);
						else
							printf("\n");

						free(cmd_start);
						free(cmd_stop);

						last_start -= 2;
						i -= 1;
					}
					cmd_start = 0;
					cmd_stop = 0;
				}
			}
		}

		cmd = (NF_ISSM_CTL_CMD*)g_ptr_array_index(g_issm_cmd_array, 0);
		g_ptr_array_remove_index(g_issm_cmd_array, 0);

		g_mutex_unlock(g_issm_cmd_mutex);

		printf("[%s][%d] cmd - type[%d]", __FUNCTION__, __LINE__, cmd->type);
		if (cmd->log[0] != 0)
			printf(" log[%s]\n", cmd->log);
		else
			printf("\n");

		switch(cmd->type)
		{
		case ISSM_START:
			nf_issm_ctl_start();
			break;
		case ISSM_STOP:
			nf_issm_ctl_stop();
			break;
		}

		free(cmd);
	}
}

