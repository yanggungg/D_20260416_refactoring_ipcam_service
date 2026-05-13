#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/timeb.h>
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
#include "nf_issm_ctl_funcs.h"


int nf_issm_ctl_get_session_count()
{
	ISSM_TASK_INFOS *task_infos;
	int session_count = 0, multi_session_count = 0;
	int i, ret;

	ret = issm_get_task_infos(&task_infos);
	if (ret != ISSM_OK)
		return 0;

	session_count = task_infos->task_cnt;

	for (i = 0; i < session_count; i++)
	{
		if (!(task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING))
			continue;

		if (task_infos->task_info[i].internet_protocol != ISSM_EP_IP_MULTICAST)
			continue;

		if (task_infos->task_info[i].mcast_task_id != -1)
			multi_session_count++;
	}

	ret = issm_free_task_infos(&task_infos);

	printf("[%s] cnt[%d]\n", __FUNCTION__, session_count - multi_session_count);
	return session_count - multi_session_count;
}

int nf_issm_ctl_disconnect_session(int p_sess_id)
{
	int ret;

	ret = issm_set_task_control(p_sess_id, ISSM_TASK_CONTROL_CLOSE, NULL);
	if (ret == ISSM_OK)
		return 1;

	return 0;
}

void nf_issm_ctl_disconnect_all_session()
{
	ISSM_TASK_INFOS *task_infos;
	int session_count = 0;
	int session_id;
	int ret, i;

	ret = issm_get_task_infos(&task_infos);
	if (ret != ISSM_OK)
		return;

	session_count = task_infos->task_cnt;

	for (i = 0; i < session_count; i++)
	{
		session_id = task_infos->task_info[i].task_id;
		nf_issm_ctl_disconnect_session(session_id);
	}

	ret = issm_free_task_infos(&task_infos);
}
long long issm_str_get_milisec()
{
	struct timeval tv;
	long long msec;

	gettimeofday(&tv, NULL);
	msec = tv.tv_sec * 1000LL + (long long)(tv.tv_usec / 1000LL);
	return msec;
}

long long issm_str_get_milisec_diff(long long p_a, long long p_b)
{
	if (p_a > p_b)
		return (p_a - p_b);
	else
		return (p_b - p_a);
}

int nf_issm_ctl_get_is_2nd_streaming(int p_ch)
{
	ISSM_TASK_INFOS *task_infos;
	int session_cnt;
	int ret, i, j;
	int ret_val = 0;

	if (p_ch < 0 || p_ch > MAX_ISSM_V_CH)
	{
		printf("[%s][%d] Error - p_ch : %d\n", __FUNCTION__, __LINE__, p_ch);
		return 0;
	}

	ret = issm_get_task_infos(&task_infos);
	if (ret != ISSM_OK)
		return 0;

	session_cnt = task_infos->task_cnt;

	if (session_cnt == 0)
	{
		ret = issm_free_task_infos(&task_infos);
		return ret_val;
	}

	for (i = 0; i < session_cnt; i++)
	{
		if (!(task_infos->task_info[i].client_status & ISSM_TM_RTSP_STATUS_STREAMING))
			continue;

		if (!(task_infos->task_info[i].recording_type & ISSM_EP_RECORDING_TYPE_LIVE))
			continue;

		if (task_infos->task_info[i].ep_id == g_issm_ep_entry_id[p_ch + MAX_ISSM_V_CH])
		{
			ret_val = 1;
			break;
		}
		else if (task_infos->task_info[i].ep_id == g_issm_itx_ep_entry_id[0])
		{
			unsigned long long v_ch_mask = task_infos->task_info[i].ch_video_mask;
			int is_second = (task_infos->task_info[i].stream_index == 1) ? 0 : 1;

			if (is_second == 0)
				continue;

			if (v_ch_mask & (1ULL<<p_ch))
			{
				ret_val = 1;
				break;
			}
		}
		else
		{
			continue;
		}
	}

	ret = issm_free_task_infos(&task_infos);
	return ret_val;
}
