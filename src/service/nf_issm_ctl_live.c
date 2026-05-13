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
#include <gobj.h>
#include <gobjmedia.h>
#include "issm.h"
#include "nf_common.h"
#include "nf_common_util.h"

#include "libsst.h"
#include "nf_codec_header.h"
#include "nf_issm_ctl.h"
#include "nf_issm_ctl_live.h"
#include "nf_issm_ctl_funcs.h"


static void nf_issm_ctl_put_frame_print();

static int v_frame_cnt[MAX_ISSM_CH] = { 0, };
static int a_frame_cnt[MAX_ISSM_CH] = { 0, };
static int m_frame_cnt[MAX_ISSM_CH] = { 0, };
static long long v_ich_time[MAX_ISSM_CH] = { 0, };
static long long a_ich_time[MAX_ISSM_CH] = { 0, };
static long long m_ich_time[MAX_ISSM_CH] = { 0, };

// for bandwidth limit
static unsigned int v_frame_bitrate[MAX_ISSM_CH] = { 0, };
static unsigned int a_frame_bitrate[MAX_ISSM_CH] = { 0, };
static unsigned int m_frame_bitrate[MAX_ISSM_CH] = { 0, };
static unsigned int v_frame_data_count[MAX_ISSM_CH] = { 0, };
static unsigned int a_frame_data_count[MAX_ISSM_CH] = { 0, };
static unsigned int m_frame_data_count[MAX_ISSM_CH] = { 0, };
static long long frame_bitrate_check_time = 0LL;

// check - audio tx limit
extern int g_issm_is_audio_tx_enable;


unsigned int nf_issm_ctl_get_bitrate(int p_ch, int p_stream_idx)
{
	unsigned int bitrate = 0;
	bitrate += v_frame_bitrate[p_ch + p_stream_idx * MAX_ISSM_V_CH];
	bitrate += a_frame_bitrate[p_ch];
	return bitrate;
}

int nf_issm_ctl_put_zero_channel_frame(void *frame)
{
    GobjBuddyBuffer *buffer = (GobjBuddyBuffer *)frame;
    ICODEC_HEADER *pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(frame);
    
    g_return_val_if_fail(frame, 0);
    
    if (!GOBJ_IS_BUDDY_BUFFER(buffer)) {
		printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        return 1;
	}
    
    if (pheader->frame_type != NF_FRAME_TYPE_P && 
        pheader->frame_type != NF_FRAME_TYPE_I) {
		printf("[%s][%d] zero channel put frame skipped. ft:%d/size:%d\n", __FUNCTION__, __LINE__, pheader->frame_type, pheader->frame_size);
        return 1;
	}
    
    if (g_issm_zero_track_id == -1) {
		printf("[%s][%d] zero channel track not created yet. ft:%d/size:%d\n", __FUNCTION__, __LINE__, pheader->frame_type, pheader->frame_size);
        return 1;
	}
    
	//printf("[%s][%d] zero channel put frame. ft:%d/size:%d\n", __FUNCTION__, __LINE__, pheader->frame_type, pheader->frame_size);
    issm_put_stream(g_issm_zero_ep_entry_id, g_issm_zero_track_id, 0, buffer);
    
    return 1;
}

int nf_issm_ctl_put_frame(void *frame)
{
	GobjBuddyBuffer *buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER *pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(frame);//buffer->frame ? buffer->frame : GST_BUFFER_DATA(frame);
	static guchar resolution[MAX_ISSM_CH] = { 0 , };
	int i;

	g_return_val_if_fail(frame, 0);

    if (!GOBJ_IS_BUDDY_BUFFER(buffer))
    {
        unsigned char *dump = (unsigned char *)frame;
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d][ERROR!!!!!!!!] is not mini object\n", __FUNCTION__, __LINE__);
        printf("[%s][%d] p_addr[%p]\n", __FUNCTION__, __LINE__, frame);
        printf("[%s][%d] dump\n", __FUNCTION__, __LINE__);
/*
        printf(" 00 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[0], dump[1], dump[2], dump[3], dump[4], dump[5], dump[6], dump[7]);
        printf(" 01 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[8], dump[9], dump[10], dump[11], dump[12], dump[13], dump[14], dump[15]);
        printf(" 02 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[16], dump[17], dump[18], dump[19], dump[20], dump[21], dump[22], dump[23]);
        printf(" 03 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
                dump[24], dump[25], dump[26], dump[27], dump[28], dump[29], dump[30], dump[31]);

        GstBuffer *gst_buf = (GstBuffer*)frame;
        printf("[%s][%d] g_type[%d]\n", __FUNCTION__, __LINE__, gst_buf->mini_object.instance.g_class->g_type);
*/
        return 1;
    }

	if (pheader->frame_type == NF_FRAME_TYPE_AUDIO && !g_issm_is_audio_tx_enable)
		return 1;

	// h264
	for (i = 0; i < MAX_ISSM_CH; i++)
	{
		if (g_issm_ep_entry_id[i] == -1)
			continue;

		if (i == pheader->chan)
		{
			if (pheader->frame_type == NF_FRAME_TYPE_P || pheader->frame_type == NF_FRAME_TYPE_I)
			{
				if (g_issm_track_v_h264_id[i] != -1)
				{
					if (resolution[i] == 0)
						resolution[i] = pheader->resolution;

					if (resolution[i] != pheader->resolution)
					{
						resolution[i] = pheader->resolution;
						nf_issm_ctl_refresh_ep(g_issm_ep_entry_id[i]);
					}

					v_frame_cnt[i]++;
					v_frame_data_count[i] += pheader->frame_size;
					v_ich_time[i] = pheader->timestamp * 1000LL + pheader->timestampl * 5LL;

					issm_put_stream(g_issm_ep_entry_id[i], g_issm_track_v_h264_id[i], 0, buffer);
				}
				else if (g_issm_track_v_h265_id[i] != -1)
				{
					v_frame_cnt[i]++;
					v_frame_data_count[i] += pheader->frame_size;
					v_ich_time[i] = pheader->timestamp * 1000LL + pheader->timestampl * 5LL;

					issm_put_stream(g_issm_ep_entry_id[i], g_issm_track_v_h265_id[i], 0, buffer);
				}
			}
			if (pheader->frame_type == NF_FRAME_TYPE_AUDIO)
			{
				int idx1, idx2;

				if (g_issm_track_a_mulaw_id[i] == -1)
					continue;

				if (pheader->frame_size == 8320)
					continue;

				a_frame_cnt[i]++;
				a_frame_data_count[i] += pheader->frame_size;
				a_ich_time[i] = pheader->timestamp * 1000LL + pheader->timestampl * 5LL;

				idx1 = i % MAX_ISSM_CH;
				idx2 = (i+MAX_ISSM_V_CH) % MAX_ISSM_CH;

				if (i >= MAX_ISSM_V_CH)
					EEE;

				issm_put_stream(g_issm_ep_entry_id[idx1], g_issm_track_a_mulaw_id[idx1], 0, buffer);
				issm_put_stream(g_issm_ep_entry_id[idx2], g_issm_track_a_mulaw_id[idx2], 0, buffer);
			}
			if (pheader->frame_type == NF_FRAME_TYPE_METADATA && pheader->codec == NF_CODEC_TYPE_METADATA)
			{
				int idx1, idx2;

				if (g_issm_track_m_meta_id[i] == -1)
					continue;

				m_frame_cnt[i]++;
				m_frame_data_count[i] += pheader->frame_size;
				m_ich_time[i] = pheader->timestamp * 1000LL + pheader->timestampl * 5LL;

				idx1 = i % MAX_ISSM_CH;
				idx2 = (i+MAX_ISSM_V_CH) % MAX_ISSM_CH;

				if (i >= MAX_ISSM_V_CH)
					EEE;

				issm_put_stream(g_issm_ep_entry_id[idx1], g_issm_track_m_meta_id[idx1], 0, buffer);
				issm_put_stream(g_issm_ep_entry_id[idx2], g_issm_track_m_meta_id[idx2], 0, buffer);
			}
		}
	}

	// mjpeg
	for (i = 0; i < MAX_ISSM_CH; i++)
	{
		if (g_issm_ep_entry_id[i] == -1)
			continue;

		if (i == pheader->chan || i == pheader->chan + 32 )
		{
			if (pheader->frame_type == NF_FRAME_TYPE_I)
			{
				if (g_issm_track_v_mjpeg_id[i] == -1)
					continue;

				v_frame_cnt[i]++;
				v_frame_data_count[i] += pheader->frame_size;
				v_ich_time[i] = pheader->timestamp * 1000LL + pheader->timestampl * 5LL;

				issm_put_stream(g_issm_ep_entry_id[i], g_issm_track_v_mjpeg_id[i], 0, buffer);
			}
		}
	}

	// itx live ep
	for (i = 0; i < 1; i++)
	{
		if (pheader->frame_type == NF_FRAME_TYPE_AUDIO && pheader->frame_size != 8320)
			continue;
			
		issm_put_stream(g_issm_itx_ep_entry_id[i], g_issm_itx_track_id[i], 0, buffer);
	}

	if (0)
	{
		unsigned int time_diff;
		static int rrr = 0;

		if (frame_bitrate_check_time == 0LL)
			frame_bitrate_check_time = issm_str_get_milisec();

		time_diff = (unsigned int)issm_str_get_milisec_diff(frame_bitrate_check_time, issm_str_get_milisec());

		if (time_diff > 1000L)
		{
			for (i = 0; i < MAX_ISSM_CH; i++)
			{
				unsigned int tmp;

				tmp = v_frame_data_count[i];
				tmp = tmp / time_diff;
				tmp = tmp * 1000;
				v_frame_bitrate[i] = tmp;
				v_frame_data_count[i] = 0;

				tmp = a_frame_data_count[i];
				tmp = tmp / time_diff;
				tmp = tmp * 1000;
				a_frame_bitrate[i] = tmp;
				a_frame_data_count[i] = 0;
			}

			if (0)
			//if (1 & (rrr++ == 10))
			{
				for (i = 0; i < MAX_ISSM_CH; i++)
					printf("[%s][%d] v_bitrate stream[%02d] : %d\n", __FUNCTION__, __LINE__, i, v_frame_bitrate[i]);

				for (i = 0; i < MAX_ISSM_CH; i++)
					printf("[%s][%d] a_bitrate stream[%02d] : %d\n", __FUNCTION__, __LINE__, i, a_frame_bitrate[i]);

				rrr = 0;
			}

			frame_bitrate_check_time = issm_str_get_milisec();
		}
	}

	//nf_issm_ctl_put_frame_print();

	return 1;
}

static void nf_issm_ctl_put_frame_print()
{
	static long long ttt = 0;
	int j;

	if (issm_str_get_milisec() - ttt < 10000)
		return;

	printf("[%s] %lld\n", __FUNCTION__, ttt);
	printf("[%s] ---- Video ----\n", __FUNCTION__);
	for (j = 0; j < MAX_ISSM_CH; j++)
	{
		if (v_frame_cnt[j] > 0)
		{
			printf("[%s] %02d : %d\n", __FUNCTION__, j, v_frame_cnt[j]);
			v_frame_cnt[j] = 0;
		}
	}
	printf("[%s] ---- AUDIO ----\n", __FUNCTION__);
	for (j = 0; j < MAX_ISSM_CH; j++)
	{
		if (a_frame_cnt[j] > 0)
		{
			printf("[%s] %02d : %d\n", __FUNCTION__, j, a_frame_cnt[j]);
			a_frame_cnt[j] = 0;
		}
	}

	printf("[%s] ---- META ----\n", __FUNCTION__);
	for (j = 0; j < MAX_ISSM_CH; j++)
	{
		if (m_frame_cnt[j] > 0)
		{
			printf("[%s] %02d : %d\n", __FUNCTION__, j, m_frame_cnt[j]);
			m_frame_cnt[j] = 0;
		}
	}

	printf("[%s] ---- Video ----\n", __FUNCTION__);
	for (j = 0; j < MAX_ISSM_CH; j++)
	{
		if (v_ich_time[j] > 0)
		{
			printf("[%s] %02d : %lld\n", __FUNCTION__, j, v_ich_time[j]);
			v_ich_time[j] = 0;
		}
	}
	printf("[%s] ---- AUDIO ----\n", __FUNCTION__);
	for (j = 0; j < MAX_ISSM_CH; j++)
	{
		if (a_ich_time[j] > 0)
		{
			printf("[%s] %02d : %lld\n", __FUNCTION__, j, a_ich_time[j]);
			a_ich_time[j] = 0;
		}
	}
	printf("[%s] ---- META ----\n", __FUNCTION__);
	for (j = 0; j < MAX_ISSM_CH; j++)
	{
		if (m_ich_time[j] > 0)
		{
			printf("[%s] %02d : %lld\n", __FUNCTION__, j, m_ich_time[j]);
			m_ich_time[j] = 0;
		}
	}

	ttt = issm_str_get_milisec();
	printf("[%s] %lld\n", __FUNCTION__, ttt);
}
