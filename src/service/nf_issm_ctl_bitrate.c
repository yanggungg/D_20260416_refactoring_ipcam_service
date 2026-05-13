#include <sys/timeb.h>
#include <sys/socket.h>
#include <time.h>
#include <glib.h>
// #include <gst/gst.h>
// #include <gst/gstinfo.h>
// #include <gst/nf/gstnfbuddybuffer.h>
#include "issm.h"
#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_codec_header.h"
#include "nf_issm_ctl.h"
#include "nf_issm_ctl_live.h"
#include "nf_issm_ctl_bitrate.h"
#include "nf_issm_ctl_bitrate_model.h"
#include "nf_issm_ctl_funcs.h"
#include "nf_record.h"
#include "nf_sysman.h"

#define PRINT_BITRATE_CTL_LOG (0)

// Max Bandwidth Control
static void nf_issm_ctl_bitrate_thread_func(void *p_data);

typedef struct __NF_ISSM_CTL_BITRATE_THREAD_DATA {
	GThread *thread_id;
	int is_run;
	int is_bandwidth_ctl_enable;
	int is_session_changed;

	int max_bw_total;
	int max_bw_main;
	int max_bw_second;

	int cur_bw_main;
	int cur_bw_second;

	int pre_bw_main;
	int pre_bw_second;

	int frame_drop_cnt_main;
	int frame_drop_cnt_second;
	long long frame_drop_cnt_check_time;

	int req_main_bitrate_up_cnt;
	int req_main_bitrate_down_cnt;
	int req_second_bitrate_up_cnt;
	int req_second_bitrate_down_cnt;
	long long req_bitrate_check_time;

} NF_ISSM_CTL_BITRATE_THREAD_DATA;

static NF_ISSM_CTL_BITRATE_THREAD_DATA g_bitrate_ctl_data;

static long long g_last_mouse_move_time = 0LL;
static void nf_issm_ctl_bitrate_dvr_status_cb(NF_NOTIFY_INFO *p_info, gpointer p_data);
static unsigned long g_issm_bitrate_dvr_status = 0;

void nf_issm_ctl_bitrate_init()
{
	memset(&g_bitrate_ctl_data, 0x00, sizeof(g_bitrate_ctl_data));
	g_bitrate_ctl_data.is_run = 1;
	g_bitrate_ctl_data.is_session_changed = 1;

	{
		int max_bandwidth, is_bandwidth_enable;

		max_bandwidth = (int)nf_sysdb_get_uint("net.proto.maxtxspeed");
		nf_issm_ctl_set_max_bandwidth(max_bandwidth);

		is_bandwidth_enable = 1;
		nf_issm_ctl_set_is_bandwidth_ctl_enable(is_bandwidth_enable);
	}

	{
		static int run_once = 0;

		if (!run_once)
		{
			gulong cb_handle;

			g_issm_bitrate_dvr_status = nf_notify_get_param0("dvr_status");

			cb_handle = nf_notify_connect_cb( "dvr_status", nf_issm_ctl_bitrate_dvr_status_cb, (gpointer)NULL);
			printf("[%s][%d] sysdb_change connect_cb[%ld]\n", __FUNCTION__, __LINE__, cb_handle);
			if (cb_handle == 0)
			{
				printf("[ISSM][%s][%d] nf_issm_ctl_bitrate_init failed\n", __FUNCTION__, __LINE__);
				return;
			}
			run_once = 1;
		}
	}

	printf("[%s][%d] MAX_BANDWIDTH        : %d\n", __FUNCTION__, __LINE__, MAX_BANDWIDTH);
	printf("[%s][%d] BITRATE_CTL_ON_SETUP : %d\n", __FUNCTION__, __LINE__, BITRATE_CTL_ON_SETUP);
	printf("[%s][%d] MAIN_RATIO           : %d\n", __FUNCTION__, __LINE__, MAIN_RATIO);
	printf("[%s][%d] SECOND_RATIO         : %d\n", __FUNCTION__, __LINE__, SECOND_RATIO);

	g_bitrate_ctl_data.thread_id = g_thread_create((GThreadFunc)nf_issm_ctl_bitrate_thread_func, &g_bitrate_ctl_data, TRUE, NULL);
}

void nf_issm_ctl_bitrate_close()
{
	if (g_bitrate_ctl_data.is_run == 1)
	{
		g_bitrate_ctl_data.is_run = 0;
		g_thread_join(g_bitrate_ctl_data.thread_id);
	}
}

void nf_issm_ctl_noti_nvr_mouse_move()
{
	g_last_mouse_move_time = issm_str_get_milisec();
}

int nf_issm_ctl_bitrate_get_max_bw()
{
	if (BITRATE_CTL_ON_SETUP)
	{
		if (g_issm_bitrate_dvr_status == NF_DVR_STATUS_LIVE)
		{
			if (issm_str_get_milisec_diff(g_last_mouse_move_time, issm_str_get_milisec()) < 10000LL)
				return MAX_BANDWIDTH > BITRATE_CTL_ON_SETUP ? BITRATE_CTL_ON_SETUP : MAX_BANDWIDTH;
		}
		else if (g_issm_bitrate_dvr_status == NF_DVR_STATUS_RUN_PLAYBACK || g_issm_bitrate_dvr_status == NF_DVR_STATUS_RUN_ARCHIVE || g_issm_bitrate_dvr_status == NF_DVR_STATUS_SETUP)
			return MAX_BANDWIDTH > BITRATE_CTL_ON_SETUP ? BITRATE_CTL_ON_SETUP : MAX_BANDWIDTH;
	}

	return MAX_BANDWIDTH;
}

int nf_issm_ctl_bitrate_get_max_wan_bw()
{
	if (BITRATE_CTL_ON_SETUP)
	{
		if (g_issm_bitrate_dvr_status == NF_DVR_STATUS_LIVE)
		{
			if (issm_str_get_milisec_diff(g_last_mouse_move_time, issm_str_get_milisec()) < 10000LL)
				return g_bitrate_ctl_data.max_bw_total > BITRATE_CTL_ON_SETUP ? BITRATE_CTL_ON_SETUP : g_bitrate_ctl_data.max_bw_total;
		}
		else if (g_issm_bitrate_dvr_status == NF_DVR_STATUS_RUN_PLAYBACK || g_issm_bitrate_dvr_status == NF_DVR_STATUS_RUN_ARCHIVE || g_issm_bitrate_dvr_status == NF_DVR_STATUS_SETUP)
			return g_bitrate_ctl_data.max_bw_total > BITRATE_CTL_ON_SETUP ? BITRATE_CTL_ON_SETUP : g_bitrate_ctl_data.max_bw_total;

		return g_bitrate_ctl_data.max_bw_total;
	}

	return g_bitrate_ctl_data.max_bw_total;
}

static void nf_issm_ctl_bitrate_dvr_status_cb(NF_NOTIFY_INFO *p_info, gpointer p_data)
{
	g_issm_bitrate_dvr_status = nf_notify_get_param0("dvr_status");
}

int nf_issm_cb_noti_bitrate_ud(void *p_data)
{
	ISSM_CB_NOTIFY_BITRATE_UD *ud_data;
	ISSM_EP_INFO ep_info;

	int ep_id;
	int task_id;
	int streaming_protocol;
	int stream_idx;
	int bitrate_step;
	int is_req_up;
	int is_req_down;
	long long check_time;

	int ch_mask = 0, ch;
	int i, ret;

	ud_data				= p_data;
	ep_id				= ud_data->ep_id;
	task_id				= ud_data->task_id;
	streaming_protocol	= ud_data->streaming_protocol;
	stream_idx			= ud_data->stream_idx;
	bitrate_step		= ud_data->bitrate_step;
	is_req_up			= ud_data->is_req_up;
	is_req_down			= ud_data->is_req_down;
	check_time			= ud_data->check_time;

	if (bitrate_step == 0)
		return ISSM_OK;

	ep_info = nf_issm_ctl_get_ep_info(ep_id);

	if (ep_info.v_track_type == ISSM_EP_VIDEO_TYPE_MJPEG)
		return ISSM_OK;

	if (ep_info.ep_id != -1)
		stream_idx = ep_info.stream_idx;

	if (is_req_up)
	{
		if (bitrate_step == 2)
		{
			ISSM_TASK_C_SET_BITRATE set_bitrate;
			set_bitrate.bitrate_step = 1;
			set_bitrate.is_i_only = 0;
			set_bitrate.is_worst_step = 0;
			set_bitrate.stream_idx = stream_idx;

			ret = issm_set_task_control(task_id, ISSM_TASK_CONTROL_SET_BITRATE_STEP, &set_bitrate);
		}

		if (bitrate_step == 3)
		{
			ISSM_TASK_C_SET_BITRATE set_bitrate;
			set_bitrate.bitrate_step = 2;
			set_bitrate.is_i_only = 1;
			set_bitrate.is_worst_step = 0;
			set_bitrate.stream_idx = stream_idx;

			ret = issm_set_task_control(task_id, ISSM_TASK_CONTROL_SET_BITRATE_STEP, &set_bitrate);
		}
	}
	else if (is_req_down)
	{
		if (bitrate_step == 1)
		{
			ISSM_TASK_C_SET_BITRATE set_bitrate;
			set_bitrate.bitrate_step = 2;
			set_bitrate.is_i_only = 1;
			set_bitrate.is_worst_step = 0;
			set_bitrate.stream_idx = stream_idx;

			ret = issm_set_task_control(task_id, ISSM_TASK_CONTROL_SET_BITRATE_STEP, &set_bitrate);
		}

		if (bitrate_step == 2)
		{
			ISSM_TASK_C_SET_BITRATE set_bitrate;
			set_bitrate.bitrate_step = 3;
			set_bitrate.is_i_only = 0;
			set_bitrate.is_worst_step = 1;
			set_bitrate.stream_idx = stream_idx;

			ret = issm_set_task_control(task_id, ISSM_TASK_CONTROL_SET_BITRATE_STEP, &set_bitrate);
		}

		ret = issm_set_task_control(task_id, ISSM_TASK_CONTROL_P_DROP_UNTIL_I, NULL);
	}

	return ISSM_OK;
}


void nf_issm_ctl_set_is_bandwidth_ctl_enable(int p_is_enable)
{
	g_bitrate_ctl_data.is_bandwidth_ctl_enable = p_is_enable;
}

void nf_issm_ctl_set_max_bandwidth(int p_max_bandwidth)
{
	printf("[%s] p_max_bandwidth : %d\n", __FUNCTION__, p_max_bandwidth);

	if (MAX_BANDWIDTH <= p_max_bandwidth)
		p_max_bandwidth = 0;

	if (p_max_bandwidth == 0)
		g_bitrate_ctl_data.max_bw_total = MAX_BANDWIDTH;
	else
		g_bitrate_ctl_data.max_bw_total = p_max_bandwidth;

	printf("[%s] max_bw_total : %d\n", __FUNCTION__, g_bitrate_ctl_data.max_bw_total);
}

void nf_issm_ctl_bitrate_set_session_changed()
{
	g_bitrate_ctl_data.is_session_changed = 1;
}

static int nf_issm_ctl_bitrate_is_same_lan(char *p_ipaddr)
{
	unsigned long client_ip;
	unsigned long tmp;
	unsigned long gateway_ip;
	unsigned long netmask;
	int is_same;

	if (BITRATE_CTL_LAN_EXCEPTION == 0)
		return 0;

	inet_pton(AF_INET, p_ipaddr, &tmp);
	client_ip = htonl(tmp);

	gateway_ip = (unsigned long)nf_sysdb_get_uint("net.proto.gateway");
	netmask = (unsigned long)nf_sysdb_get_uint("net.proto.subnet");

	is_same = ((client_ip & netmask) == (gateway_ip & netmask));

	//printf("[%s][%d] ip_str[%s] ip[%lu] gateway[%lu] netmask[%lu] is_same[%d]\n", __FUNCTION__, __LINE__, p_ipaddr, client_ip, gateway_ip, netmask, is_same);

	return is_same;
}

static void nf_issm_ctl_bitrate_thread_func(void *p_data)
{
	NF_ISSM_CTL_BITRATE_THREAD_DATA *data;
	ISSM_TASK_INFOS *task_infos;
	ISSM_TASK_C_SET_BW_LIMIT set_bw_limit;
	static int pre_is_bandwidth_ctl_enable = 0;

	int do_set_max_bitrate;
	int session_cnt = 0;
	int pre_max_bandwidth = 0;
	int total_streaming_ch_num = 0;
	int total_streaming_ch_num_wan = 0;
	int current_streaming_size = 0;
	int bw_base = 0;
	int bw_base_wan = 0;
	int bw_base_lan = 0;
	int sleep_time = 300 * 1000;
	unsigned long long v_ch_mask;
	int is_second;
	int ret, i, j;


	usleep(sleep_time * 20);

	data = p_data;
	pre_is_bandwidth_ctl_enable = data->is_bandwidth_ctl_enable;

	while (data->is_run)
	{
		ret = issm_get_task_infos(&task_infos);
		if (ret != ISSM_OK)
		{
			usleep(sleep_time);
			continue;
		}

		session_cnt = task_infos->task_cnt;

		if (!session_cnt)
		{
			ret = issm_free_task_infos(&task_infos);
			usleep(sleep_time);
			continue;
		}

		total_streaming_ch_num = 0;
		total_streaming_ch_num_wan = 0;

		for (i = 0; i < session_cnt; i++)
		{
			int is_same_lan;

			if (!(task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING))
				continue;

			is_same_lan = nf_issm_ctl_bitrate_is_same_lan(task_infos->task_info[i].client_ip);

			for (j = 0; j < MAX_ISSM_CH; j++)
			{
				if (task_infos->task_info[i].ep_id == g_issm_ep_entry_id[j])
					break;
			}

			// Standard RTSP Entrypoint(Live)
			if (j < MAX_ISSM_CH)
			{
				is_second = (j < MAX_ISSM_V_CH ? 0 : 1);
				total_streaming_ch_num += (is_second ? SECOND_RATIO : MAIN_RATIO);
				if (!is_same_lan)
					total_streaming_ch_num_wan += (is_second ? SECOND_RATIO : MAIN_RATIO);
				continue;
			}

			// Standard RTSP Entrypoint(Playback)
			if (0)
			{
				continue;
			}

			// ITX RTSP Entrypoint(Live)
			if (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[0])
			{
				v_ch_mask = task_infos->task_info[i].ch_video_mask;
				is_second = (task_infos->task_info[i].stream_index == 1) ? 0 : 1;

				for (j = 0; j < NUM_ACTIVE_CH; j++)
				{
					if (v_ch_mask & (1ULL<<j))
					{
						total_streaming_ch_num += (is_second ? SECOND_RATIO : MAIN_RATIO);
						if (!is_same_lan)
							total_streaming_ch_num_wan += (is_second ? SECOND_RATIO : MAIN_RATIO);
					}
				}

				continue;
			}

			// ITX RTSP Entrypoint(Playback)
			if (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[1])
			{
				continue;
			}

			// Zero Channel Entrypoint(Live)
			if (task_infos->task_info[i].ep_id == g_issm_zero_ep_entry_id)
			{
				total_streaming_ch_num += MAIN_RATIO;
				if (!is_same_lan)
					total_streaming_ch_num_wan += MAIN_RATIO;
				if (PRINT_BITRATE_CTL_LOG)
					printf("[%s][%d] Zero channel detected\n", __FUNCTION__, __LINE__);
				continue;
			}			
		}

		if (total_streaming_ch_num == 0)
		{
			ret = issm_free_task_infos(&task_infos);
			usleep(sleep_time);
			continue;
		}

		if (data->is_bandwidth_ctl_enable)
		{
			pre_is_bandwidth_ctl_enable = data->is_bandwidth_ctl_enable;

			// Max Bitrate 를 조정 해야 되는 조건 : 1. Session 변경.
			if (data->is_session_changed)
			{
				do_set_max_bitrate = 1;
				data->is_session_changed = 0;
			}
			// Max Bitrate 를 조정 해야 되는 조건 : 2. Sysdb 변경.
			else if (pre_max_bandwidth != nf_issm_ctl_bitrate_get_max_bw())
			{
				do_set_max_bitrate = 1;
				pre_max_bandwidth = nf_issm_ctl_bitrate_get_max_bw();
			}
			else
			{
				do_set_max_bitrate = 0;
			}

			// Max Bitrate 재설정.
			if (do_set_max_bitrate)
			{
				bw_base = nf_issm_ctl_bitrate_get_max_bw() * 1024 / total_streaming_ch_num;
				if (nf_issm_ctl_bitrate_get_max_wan_bw() * 1024 < bw_base * total_streaming_ch_num_wan)
				{
					int rest_bw;
					rest_bw = nf_issm_ctl_bitrate_get_max_bw() - nf_issm_ctl_bitrate_get_max_wan_bw();

					bw_base_wan = nf_issm_ctl_bitrate_get_max_wan_bw() * 1024 / total_streaming_ch_num_wan;
					if (total_streaming_ch_num - total_streaming_ch_num_wan == 0)
						bw_base_lan = 0;
					else
						bw_base_lan = rest_bw * 1024 / (total_streaming_ch_num - total_streaming_ch_num_wan);
				}
				else
				{
					bw_base_wan = bw_base;
					bw_base_lan = bw_base;
				}

				if (PRINT_BITRATE_CTL_LOG)
				{
					printf("[%s] DEVICE MAX BW[%d]\n", __FUNCTION__, nf_issm_ctl_bitrate_get_max_bw());
					printf("[%s] SYSDB  MAX BW[%d]\n", __FUNCTION__, nf_issm_ctl_bitrate_get_max_wan_bw());
					printf("[%s] total_streaming_ch_num_all[%d]  bw_base_lan[%d]\n", __FUNCTION__, total_streaming_ch_num, bw_base_lan);
					printf("[%s] total_streaming_ch_num_wan[%d]  bw_base_wan[%d]\n", __FUNCTION__, total_streaming_ch_num_wan, bw_base_wan);
				}

				for (i = 0; i < session_cnt; i++)
				{
					int is_same_lan;

					if (!(task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING))
						continue;

					is_same_lan = nf_issm_ctl_bitrate_is_same_lan(task_infos->task_info[i].client_ip);
					if (!is_same_lan)
						bw_base = bw_base_wan;
					else
						bw_base = bw_base_lan;

					for (j = 0; j < MAX_ISSM_CH; j++)
					{
						if (task_infos->task_info[i].ep_id == g_issm_ep_entry_id[j])
							break;
					}

					// Standard RTSP Entrypoint(Live)
					if (j < MAX_ISSM_CH)
					{
						if (j < MAX_ISSM_V_CH)
							set_bw_limit.bw_limit = bw_base * MAIN_RATIO;
						else
							set_bw_limit.bw_limit = bw_base * SECOND_RATIO;

						ret = issm_set_task_control(task_infos->task_info[i].task_id, ISSM_TASK_CONTROL_SET_BW_LIMIT, &set_bw_limit);

						if (PRINT_BITRATE_CTL_LOG)
							printf("[%s][%d] VLC task_id[%d] ip_str[%s] is_same_lan[%d] bw[%d]\n", __FUNCTION__, __LINE__,
									task_infos->task_info[i].task_id, task_infos->task_info[i].client_ip,
									nf_issm_ctl_bitrate_is_same_lan(task_infos->task_info[i].client_ip),
									set_bw_limit.bw_limit);

						continue;
					}

					// Standard RTSP Entrypoint(Playback)
					if (0)
					{
						continue;
					}

					// ITX RTSP Entrypoint(Live)
					if (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[0])
					{
						set_bw_limit.bw_limit = 0;
						v_ch_mask = task_infos->task_info[i].ch_video_mask;
						is_second = (task_infos->task_info[i].stream_index == 1) ? 0 : 1;

						for (j = 0; j < NUM_ACTIVE_CH; j++)
						{
							if (v_ch_mask & (1ULL<<j))
								set_bw_limit.bw_limit += bw_base * (is_second ? SECOND_RATIO : MAIN_RATIO);
						}

						ret = issm_set_task_control(task_infos->task_info[i].task_id, ISSM_TASK_CONTROL_SET_BW_LIMIT, &set_bw_limit);
						if (PRINT_BITRATE_CTL_LOG)
							printf("[%s][%d] ITX task_id[%d] ip_str[%s] is_same_lan[%d] bw[%d]\n", __FUNCTION__, __LINE__,
									task_infos->task_info[i].task_id, task_infos->task_info[i].client_ip,
									nf_issm_ctl_bitrate_is_same_lan(task_infos->task_info[i].client_ip),
									set_bw_limit.bw_limit);
						continue;
					}

					// ITX RTSP Entrypoint(Playback)
					if (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[1])
					{
						continue;
					}

					// Zero Channel Entrypoint(Live)
					if (task_infos->task_info[i].ep_id == g_issm_zero_ep_entry_id)
					{
						set_bw_limit.bw_limit = bw_base * MAIN_RATIO;
						ret = issm_set_task_control(task_infos->task_info[i].task_id,
													ISSM_TASK_CONTROL_SET_BW_LIMIT, &set_bw_limit);
						if (PRINT_BITRATE_CTL_LOG)
							printf("[%s][%d] ZeroChannel task_id[%d] ip_str[%s] is_same_lan[%d] bw[%d]\n",
								__FUNCTION__, __LINE__,
								task_infos->task_info[i].task_id,
								task_infos->task_info[i].client_ip,
								nf_issm_ctl_bitrate_is_same_lan(task_infos->task_info[i].client_ip),
								set_bw_limit.bw_limit);
						continue;
					}					
				}
			}
		}
		else if (data->is_bandwidth_ctl_enable != pre_is_bandwidth_ctl_enable)
		{
			pre_is_bandwidth_ctl_enable = data->is_bandwidth_ctl_enable;

			set_bw_limit.bw_limit = 0;

			for (i = 0; i < session_cnt; i++)
			{
				if (!(task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING))
					continue;

				ret = issm_set_task_control(task_infos->task_info[i].task_id, ISSM_TASK_CONTROL_SET_BW_LIMIT, &set_bw_limit);
			}
			if (PRINT_BITRATE_CTL_LOG)
			{
				printf("[%s] BW set 0\n", __FUNCTION__);
			}
		}

		ret = issm_free_task_infos(&task_infos);
		usleep(sleep_time);
	}
}

