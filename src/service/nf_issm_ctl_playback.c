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
#include "nf_codec_header.h"

#include "nf_issm_ctl.h"
#include "nf_issm_ctl_playback.h"
#include "nf_issm_ctl_funcs.h"
#include "libsst.h"


#define RTP_MODE_PLAYBACK		'P'


typedef struct _NF_RTP_PLAY_PARAM_T {
	int       		is_run;
	int				task_id;
	int 			track_id;
	int				pb_streaming_protocol;
	int				is_time_rate_ctl;

	gint			i_frame_only;

	guchar			mode;

	GTimeVal  		start_time;		// play start time
	GTimeVal 		search_time;	// play search time
	GTimeVal 		end_time;		// play end time

	guchar			pr_id;	// 0
	guchar			hide;	// 0
	guchar			slow;	// 0

	guchar			direction;
	guchar			rate;
	guchar			rate_ctrl;

	gint			ch_cnt;
	gint			audio_ch;

	gint			stream_index;
	guint64			open_mask[MAX_SST_CHANNEL];
	guint64			eos_mask[MAX_SST_CHANNEL];
	guint64			norecord_mask[MAX_SST_CHANNEL];

	gint			open_ch[MAX_SST_CHANNEL];
	gint			ch_arr[MAX_SST_CHANNEL];
	gint			sid[MAX_SST_CHANNEL];

	ICODEC_HEADER	frame[MAX_SST_CHANNEL];
	ICODEC_HEADER	*ri_frame[MAX_SST_CHANNEL];
} NF_RTP_PLAY_PARAM;


static void play_thread_func_itx(NF_RTP_PLAY_PARAM *param);
static void play_thread_func_vlc(NF_RTP_PLAY_PARAM *param);
static int rtsp_send_controlframe(int p_type, int p_pb_task_id, int *p_is_run);
static int rtsp_send_overlapframe(int p_type, int p_ch, int p_pb_task_id, int *p_is_run);
static gint audio_ich_sort(const GobjBuddyBuffer **a, const GobjBuddyBuffer **b);

static GPtrArray *g_pb_param_array = NULL;
static GMutex *g_pb_param_mutex = NULL;

// check - audio tx limit
extern int g_issm_is_audio_tx_enable;


int nf_issm_ctl_pb_init()
{
	if (!g_pb_param_mutex)
		g_pb_param_mutex = g_mutex_new();

	if (!g_pb_param_array)
		g_pb_param_array = g_ptr_array_new();

}

int nf_issm_ctl_pb_close()
{
	unsigned int i;
	NF_RTP_PLAY_PARAM *param;

	g_mutex_lock(g_pb_param_mutex);

	for (i = 0; i < g_pb_param_array->len; i++)
	{
		param = g_ptr_array_index(g_pb_param_array, i);
		param->is_run = 0;
	}

	g_mutex_unlock(g_pb_param_mutex);
}

int nf_issm_cb_notify_pb_start(void *p_data)
{
	ISSM_CB_NOTIFY_PB_START *data;
	NF_RTP_PLAY_PARAM *param = NULL;
	int i;

	data = p_data;

	printf("[[[ %s ]]]\n", __FUNCTION__);
	printf("-----------------------------------------\n");
	printf("pb task_id            : [%d]\n", data->pb_task_id);
	printf("pb pb_task_track_id   : [%d]\n", data->pb_task_track_id);
	printf("pb_streaming_protocol : [%d]\n", data->pb_streaming_protocol);
	printf("live/pb v_ch_mask     : [0x%08llx]\n", data->itx_v_ch_mask);
	printf("live/pb a_ch_mask     : [0x%08x]\n", data->itx_a_ch_mask);
	printf("live/pb m_ch_mask     : [0x%08x]\n", data->itx_m_ch_mask);
	printf("live_i_frame_only     : [%d]\n", data->itx_i_frame_only);
	printf("live_stream_index     : [%d]\n", data->itx_stream_index);
	printf("itx_pb_start_time     : [%d]\n", data->itx_pb_start_time);
	printf("itx_pb_end_time       : [%d]\n", data->itx_pb_end_time);
	printf("itx_pb_direction      : [%s]\n", data->itx_pb_direction == 0 ? "Forward" : "Backward");
	printf("itx_pb_speed          : [%d]\n", data->itx_pb_speed);
	printf("itx_pb_is_start_end   : [%d]\n", data->itx_pb_is_start_end);
	printf("is_rate_ctl_on        : [%d]\n", data->is_rate_ctl_on);
	printf("-----------------------------------------\n");

	param = g_malloc0(sizeof(NF_RTP_PLAY_PARAM));
	if (param == NULL)
	{
		printf("[%s][%d] param g_malloc0() failed\n", __FUNCTION__, __LINE__);
		return ISSM_ERROR;
	}

	param->is_run				= 1;
	param->task_id				= data->pb_task_id;
	param->track_id				= data->pb_task_track_id;
	param->pb_streaming_protocol = data->pb_streaming_protocol;
	param->is_time_rate_ctl		= data->is_rate_ctl_on;
	param->mode					= RTP_MODE_PLAYBACK;
	param->mode					= 'A';
	param->i_frame_only			= data->itx_i_frame_only;
	param->direction			= data->itx_pb_direction;
	param->start_time.tv_sec	= data->itx_pb_start_time;
	param->start_time.tv_usec	= 0;
	param->end_time.tv_sec		= data->itx_pb_end_time;
	param->end_time.tv_usec		= 0;
	param->hide					= 0;
	param->rate					= data->itx_pb_speed;
	param->stream_index			= data->itx_stream_index;

	if (param->direction == 0)
	{
		param->search_time.tv_sec	= data->itx_pb_start_time;
		param->search_time.tv_usec	= 0;
	}
	else
	{
		param->search_time.tv_sec	= data->itx_pb_end_time;
		param->search_time.tv_usec	= 0;
	}

	for (i = 0; i < MAX_SST_CHANNEL; i++)
		param->sid[i] = -1;

	// Video
	for (i = 0; i < MAX_ISSM_V_CH; i++)
	{
		if (data->itx_v_ch_mask & (1ULL<<i))
			++param->ch_cnt;
	}

	if (param->stream_index == 0)
	{
		if (param->ch_cnt > 1)
			param->stream_index = 2;
		else
			param->stream_index = 1;
	}

	for (i = 0; i < MAX_ISSM_V_CH; i++)
	{
		if (data->itx_v_ch_mask & (1ULL<<i))
			param->open_ch[i + (param->stream_index - 1) * MAX_ISSM_V_CH] = 1;
	}


	// Audio
	if (param->direction == 0 && param->rate == 1)
	{
		for (i = 0; i < MAX_ISSM_V_CH; i++)
		{
			if (data->itx_a_ch_mask & (1ULL<<i))
			{
				param->open_ch[i + MAX_ISSM_V_CH * 2] = 1;
				++param->ch_cnt;
			}
		}
	}

	// Meta
	for (i = 0; i < 32; i++)
	{
		if (data->itx_m_ch_mask & (1ULL<<i))
		{
			param->open_ch[i + MAX_ISSM_V_CH * 3] = 1;
			++param->ch_cnt;
		}
	}

	if (param->pb_streaming_protocol == ISSM_EP_SP_RTSP)
	{
		if (!g_thread_create((GThreadFunc)play_thread_func_vlc, param, FALSE, NULL))
		{
			g_free(param);
			return ISSM_ERROR;
		}
	}
	else
	{
		if (!g_thread_create((GThreadFunc)play_thread_func_itx, param, FALSE, NULL))
		{
			g_free(param);
			return ISSM_ERROR;
		}
	}

	g_mutex_lock(g_pb_param_mutex);
	g_ptr_array_add(g_pb_param_array, param);
	g_mutex_unlock(g_pb_param_mutex);

	return ISSM_OK;
}

int nf_issm_cb_notify_pb_end(void *p_data)
{
	ISSM_CB_NOTIFY_PB_END *data;
	NF_RTP_PLAY_PARAM *param;
	unsigned int i;

	data = p_data;

	printf("[[[ %s ]]]\n", __FUNCTION__);
	printf("-----------------------------------------\n");
	printf("pb task_id          : [%d]\n", data->pb_task_id);
	printf("-----------------------------------------\n");

	g_mutex_lock(g_pb_param_mutex);

	for (i = 0; i < g_pb_param_array->len; i++)
	{
		param = g_ptr_array_index(g_pb_param_array, i);

		if (param->task_id == data->pb_task_id)
		{
			param->is_run = 0;
			param->task_id = 0;
		}
	}

	g_mutex_unlock(g_pb_param_mutex);

	return ISSM_OK;
}


static gint audio_ich_sort(const GobjBuddyBuffer **a, const GobjBuddyBuffer **b)
{
	long long a_time, b_time;
	ICODEC_HEADER *a_ich, *b_ich;

	a_ich = (ICODEC_HEADER*)gobj_buddy_buffer_buf_get_addr(*a);
	b_ich = (ICODEC_HEADER*)gobj_buddy_buffer_buf_get_addr(*b);

	a_time = a_ich->timestamp * 1000LL + a_ich->timestampl * 5LL;
	b_time = b_ich->timestamp * 1000LL + b_ich->timestampl * 5LL;

	return (a_time > b_time);
}

static int rtsp_send_controlframe(int p_type, int p_pb_task_id, int *p_is_run)
{
	GobjBuddyBuffer *buf;
	ICODEC_HEADER ich;
	int ret = ISSM_OK;

	memset(&ich, 0x00, sizeof(ICODEC_HEADER));
	ich.frame_type = p_type;

	buf = gobj_buddy_buffer_new_malloc(sizeof(ICODEC_HEADER));
	memcpy(gobj_buddy_buffer_buf_get_addr(buf), &ich, sizeof(ICODEC_HEADER));
	// GST_BUFFER_SIZE(buf) = sizeof(ICODEC_HEADER);

	while (*p_is_run)
	{
		ret = issm_put_stream(0, 0, p_pb_task_id, buf);
		if (ret == ISSM_OK || ret == ISSM_NOT_EXIST_TASK)
			break;

		g_usleep(10*1000);
	}

	g_object_unref(buf);
	return ret;
}

static int rtsp_send_overlapframe(int p_type, int p_ch, int p_pb_task_id, int *p_is_run)
{
	GobjBuddyBuffer *buf;
	ICODEC_HEADER ich;
	int length;
	int ret = ISSM_OK;

	memset(&ich, 0x00, sizeof(ICODEC_HEADER));
	ich.frame_type = p_type;
	ich.chan = p_ch;

	buf = gobj_buddy_buffer_new_malloc(sizeof(ICODEC_HEADER));
	memcpy(gobj_buddy_buffer_buf_get_addr(buf), &ich, sizeof(ICODEC_HEADER));
	// GST_BUFFER_SIZE(buf) = sizeof(ICODEC_HEADER);

	while (*p_is_run)
	{
		ret = issm_put_stream(0, 0, p_pb_task_id, buf);
		if (ret == ISSM_OK || ret == ISSM_NOT_EXIST_TASK)
			break;

		g_usleep(10*1000);
	}

	g_object_unref(buf);
	return ret;
}

static void play_thread_func_itx(NF_RTP_PLAY_PARAM 	*param)
{
	unsigned int i_frame_count[MAX_SST_CHANNEL], p_frame_count[MAX_SST_CHANNEL];

	unsigned int wait_time = 0;
	GTimeVal tv_max, tv_min, tv_tmp;
	int tv_max_ch = 0, tv_min_ch = 0;

	int is_iframe_start = 0;
	int is_first_clip = 1;
	unsigned long long end_of_clips = 0;
	long long frame_ctl_start_time = 0;
	long long frame_ctl_current_time = 0;
	long long frame_ich_start_time = 0;
	long long frame_ich_current_time = 0;
	long long tdiff_ctl;
	long long tdiff_ich;
	long long check_interval;
	long long division;

	int ret, i, ch;

	division = (long long)param->rate;
	check_interval = 500LL; // 0.5 second

	// get lastest/oldest frame header
	for (ch = 0 ; ch < param->ch_cnt ; ++ch)
	{
		i_frame_count[ch] = 86400/2;
		p_frame_count[ch] = 0;
	}

	for (i = 0, ch = 0; i < MAX_SST_CHANNEL; i++)
	{
		if (param->open_ch[i])
		{
			param->ch_arr[ch] = i;
			param->sid[ch] = sst_play_open_ex(param->pr_id,
											i,
											param->rate,
											param->direction,
											param->rate_ctrl,	// 0
											param->hide,		// 0
											GTIMEVAL_TO_GUINT64(param->start_time),
											GTIMEVAL_TO_GUINT64(param->end_time),
											GTIMEVAL_TO_GUINT64(param->search_time),
											PB_TYPE_REMOTE,
											(param->rate == 1 && param->direction == 0) ? param->i_frame_only : 1,
											0);

			if (param->sid[ch] == -SST_ERR_NODATA)
			{
				printf("[%s][%d] sst_play_open_ex ch[%02d] NODATA\n",__FUNCTION__, __LINE__, i);
				param->eos_mask[i] = 1;
				++ch;
				continue;
			}
			else if (param->sid[ch] < 0)
			{
				printf("[%s][%d] sst_play_open_ex ch[%02d] ret[%d]\n", __FUNCTION__, __LINE__, i, param->sid[ch]);
				goto error_send_ctrl;
			}
			param->open_mask[i] = 1;

			while (1)
			{
				ret = sst_play_check_frame(param->sid[ch], &param->frame[ch]);

				if (ret == 0)
				{
					printf("[%s][%d] sst_play_check_frame ch[%02d] ret[%d]\n", __FUNCTION__, __LINE__, i, param->sid[ch]);
					break;
				}
				else if (ret == -SST_ERR_ENDDATA)
				{
					param->eos_mask[i] = 1;
					printf("[%s][%d] sst_play_check_frame 00 ch[%02d] sid[%d]\n", __FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch]);
					break;
				}
				else if (ret == -SST_ERR_EMPTY)
				{
					g_usleep(10*1000);
					continue;
				}
				else
				{
					printf("[%s][%d] sst_play_check_frame ch[%02d] sid[%d] ret[%d]\n", __FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch], ret);
					goto error_send_ctrl;
				}
			}
			++ch;
		}
		else
		{
			param->eos_mask[i] = 1;
		}
	}


	ret = rtsp_send_controlframe(NF_FRAME_TYPE_STARTDATA, param->task_id, &(param->is_run));
	if (ret == ISSM_NOT_EXIST_TASK)
	{
		printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
		goto error_send_ctrl;
	}

	wait_time = 50000 * param->ch_cnt;
	printf("[%s][%d] wait sst_buffering [%d]   ch_cnt [%d]\n", __FUNCTION__, __LINE__, wait_time, param->ch_cnt);
	g_usleep(wait_time);

	// send
	while (param->is_run)
	{
		ICODEC_HEADER *ih_frame;
		GobjBuddyBuffer *buf;
		int length;
		int write_size = 0;
		int target_ch = 0;

		tv_max.tv_sec = 0;
		tv_max.tv_usec = 0;
		tv_min.tv_sec = 0x7FFFFFFF;
		tv_min.tv_usec = 0x7FFFFFFF;

		// get lastest/oldest frame header
		for (ch = 0 ; ch < param->ch_cnt ; ++ch)
		{
			if (param->eos_mask[param->ch_arr[ch]])
				continue;

			tv_tmp.tv_sec = param->frame[ch].timestamp;
			tv_tmp.tv_usec = param->frame[ch].timestampl * 5000;

			if (param->frame[ch].frame_type == NF_FRAME_TYPE_RI)
			{
				tv_min_ch = tv_max_ch = ch;
				break;
			}

			if ((tv_tmp.tv_sec < tv_min.tv_sec) || (tv_tmp.tv_sec == tv_min.tv_sec && tv_tmp.tv_usec < tv_min.tv_usec))
			{
				tv_min = tv_tmp;
				tv_min_ch = ch;
			}

			if ((tv_tmp.tv_sec > tv_max.tv_sec) || (tv_tmp.tv_sec == tv_max.tv_sec && tv_tmp.tv_usec > tv_max.tv_usec))
			{
				tv_max = tv_tmp;
				tv_max_ch = ch;
			}
		}

		{
			int cnt_eos = 0, k;
			for (k = 0; k < MAX_SST_CHANNEL; k++)
			{
				if (param->eos_mask[k])
					cnt_eos++;
			}
			if (cnt_eos == MAX_SST_CHANNEL)
			{
				printf("[%s][%d] ALL ch eos_maks\n", __FUNCTION__, __LINE__);
	        	ret = rtsp_send_controlframe(NF_FRAME_TYPE_ENDDATA, param->task_id, &(param->is_run));
	        	if (ret == ISSM_NOT_EXIST_TASK)
	        		printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
				break;
			}
		}

		// NF_PLAY_PARAM_DIR_FORWARD
		if (param->direction == 0)
			target_ch = tv_min_ch;
		else
			target_ch = tv_max_ch;

		//wait 5second
		ret = sst_play_get_frame(param->sid[target_ch], &ih_frame, 5000);

		if (ret < 0)
		{
			printf("[%s][%d] sst_play_get_frame ch[%d] sid[%d] ret[%d]\n", __FUNCTION__, __LINE__, __FUNCTION__,param->ch_arr[target_ch], param->sid[target_ch], ret);
			goto error_send_ctrl;
		}

		// CLIENT_MODE_ARCHIVING
		if (param->mode == 'A')
		{
			if (ih_frame->frame_type == NF_FRAME_TYPE_I)
				ih_frame->flags = 1;
			else
				ih_frame->flags = 0;
		}

		ih_frame->chan = ih_frame->chan % MAX_ISSM_V_CH;

		if (ih_frame->frame_type == NF_FRAME_TYPE_I)
		{
			i_frame_count[target_ch]++;
			p_frame_count[target_ch] = 0;
			ih_frame->reserved = i_frame_count[target_ch] << 16;
		}
		else if (ih_frame->frame_type == NF_FRAME_TYPE_RI)
		{
			i_frame_count[target_ch]--;
			p_frame_count[target_ch] = 30;
			ih_frame->reserved = i_frame_count[target_ch] << 16;
		}
		else if (ih_frame->frame_type == NF_FRAME_TYPE_P)
		{
			if (param->direction == 0)
				p_frame_count[target_ch]++;
			else
				p_frame_count[target_ch]--;

			p_frame_count[target_ch] = p_frame_count[target_ch] & 0x0000FFFF;
			ih_frame->reserved = (i_frame_count[target_ch] << 16) | p_frame_count[target_ch];
		}

		if (ih_frame->frame_type == NF_FRAME_TYPE_METADATA)
		{
			length = ih_frame->frame_size + sizeof(ICODEC_HEADER);
			buf = gobj_buddy_buffer_new();
			gobj_buddy_buffer_buf_set_addr(buf,(guint8*)ih_frame);
			gobj_buddy_buffer_buf_set_size(buf, length);
		}
		else
		{
			length = ih_frame->frame_size + sizeof(ICODEC_HEADER);
			buf = gobj_buddy_buffer_new();
			gobj_buddy_buffer_buf_set_addr(buf, ih_frame);
			gobj_buddy_buffer_buf_set_size(buf, length);
		}

		/*
		if (ih_frame->frame_type != NF_FRAME_TYPE_P)
			printf("[%s] ch : %d    ft : %02d  time : %d.%d\n", __FUNCTION__, ih_frame->chan, ih_frame->frame_type, ih_frame->timestamp, ih_frame->timestampl*5);
		*/

		while (param->is_run) {
			int ret;

			if (ih_frame->frame_type == NF_FRAME_TYPE_AUDIO && g_issm_is_audio_tx_enable == 0)
				break;

			// time rate ctl : on
			if (param->is_time_rate_ctl)
			{
				long long new_time;

				//NF_FRAME_TYPE_START		= 3,	/* Start frame */
				//NF_FRAME_TYPE_END		= 4,	/* End frame (end of clip) */
				if (ih_frame->frame_type == NF_FRAME_TYPE_START)
					end_of_clips |= 1ULL << ih_frame->chan;
				if (ih_frame->frame_type == NF_FRAME_TYPE_END)
					end_of_clips &= ~(1ULL << ih_frame->chan);

				frame_ctl_current_time = issm_str_get_milisec();
				frame_ich_current_time = ih_frame->timestamp * 1000LL + ih_frame->timestampl * 5LL;

				if (!is_iframe_start)
				{
					is_iframe_start = 1;
					frame_ctl_start_time = frame_ctl_current_time;
					frame_ich_start_time = frame_ich_current_time;
				}

				if (frame_ich_start_time == frame_ich_current_time)
				{
					new_time = frame_ich_start_time;
				}
				else
				{
					if (param->direction)
						new_time = frame_ich_start_time + ((frame_ich_start_time - frame_ich_current_time) / division);
					else
						new_time = frame_ich_start_time + ((frame_ich_current_time - frame_ich_start_time) / division);
				}

				tdiff_ich = new_time - frame_ich_start_time;
				tdiff_ctl = frame_ctl_current_time - frame_ctl_start_time;
				if (is_first_clip)
					tdiff_ctl += check_interval;

				/*
				printf("[%s][%d] ft[%d] ich[%lld, %lld, %lld] cur[%lld, %lld, %lld]\n", __FUNCTION__, __LINE__,
						ih_frame->frame_type,
						frame_ich_start_time, frame_ich_current_time, tdiff_ich,
						frame_ctl_start_time, frame_ctl_current_time, tdiff_ctl);
				*/

				if (tdiff_ich <= tdiff_ctl)
					ret = issm_put_stream(0, 0, param->task_id, buf);
				else
				{
					if (tdiff_ich - tdiff_ctl >= 1000){
						is_iframe_start = 0;
						is_first_clip = 0;
						break;
					}
					g_usleep(2*1000);
					continue;
				}

				if (end_of_clips == 0)
				{
					is_iframe_start = 0;
					is_first_clip = 0;
				}
			}
			// time rate ctl : off
			else
			{
				ret = issm_put_stream(0, 0, param->task_id, buf);
			}

			if (ret == ISSM_OK)
			{
				break;
			}
			else if (ret == ISSM_NOT_EXIST_TASK)
			{
				printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
				goto error_send_ctrl;
			}

			g_usleep(10*1000);
		}
		g_object_unref(buf);

		ih_frame = NULL;
		ch = target_ch;

		// get new header
		while (1)
		{
			ret = sst_play_check_frame(param->sid[ch], &param->frame[ch]);

			if (ret == 0)
			{
				break;
			}
			else if (ret == -SST_ERR_ENDDATA)
			{
				unsigned char overlap_status = 0;

                if (sst_play_check_overlap(param->sid[ch], &overlap_status) == 0)
				{
                	printf("[%s][%d] ############ overlap_status[%d]\n",
                			__FUNCTION__, __LINE__, overlap_status);

					if (overlap_status == 1)
					{
						g_usleep(10*1000);
						ret = rtsp_send_overlapframe(NF_FRAME_TYPE_OVERLAP, ch, param->task_id, &(param->is_run));
						if (ret == ISSM_NOT_EXIST_TASK)
						{
							printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
							goto error_send_ctrl;
						}
					}
                }

                param->eos_mask[param->ch_arr[ch]] = 1;
				printf("[%s][%d] sst_play_check_frame 11 ch[%2d] sid[%d] ENDDATA\n",
							__FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch]);
				break;
			}
			else if (ret == -SST_ERR_EMPTY)
			{
				g_usleep(10*1000);
				continue;
			}
			else
			{
				printf("[%s][%d] sst_play_check_frame ch[%2d] sid[%d] ret[%d]\n",
							__FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch], ret);
				goto error_send_ctrl;
			}
		}
	}

error_send_ctrl:

	//issm_set_task_control(param->task_id, ISSM_TASK_CONTROL_CLOSE, NULL);

	g_mutex_lock(g_pb_param_mutex);
	g_ptr_array_remove(g_pb_param_array, param);
	g_mutex_unlock(g_pb_param_mutex);

	for (ch = 0; ch < param->ch_cnt; ch++)
	{
		if (param->sid[ch] >= 0)
		{
			ret = sst_play_close(param->sid[ch]);
			printf("[%s][%d] sst_play_close ch[%2d] sid[%d]\n",
					__FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch]);
			param->sid[ch] = -1;
		}
	}

	if (param)
		g_free(param);

	printf("[%s][%d] end\n",__FUNCTION__, __LINE__);

	return 1;
}

static void play_thread_func_vlc(NF_RTP_PLAY_PARAM 	*param)
{
	unsigned int i_frame_count[MAX_SST_CHANNEL], p_frame_count[MAX_SST_CHANNEL];

	unsigned int wait_time = 0;
	GTimeVal tv_max, tv_min, tv_tmp;
	int tv_max_ch = 0, tv_min_ch = 0;

	int is_iframe_start = 0;
	long long frame_ctl_start_time = 0;
	long long frame_ctl_current_time = 0;
	long long frame_ich_start_time = 0;
	long long frame_ich_current_time = 0;
	long long tdiff_ctl;
	long long tdiff_ich;
	long long check_interval;
	long long division;
	guchar resolution = 0;

	GPtrArray *audio_array = NULL;

	int ret, i, ch;

	division = (long long)param->rate;

	check_interval = 100LL;
	audio_array = g_ptr_array_new();

	printf("[%s][%d] task_id : %d\n", __FUNCTION__, __LINE__, param->task_id);
	printf("[%s][%d] div     : %lld\n", __FUNCTION__, __LINE__, division);

	// get lastest/oldest frame header
	for (ch = 0 ; ch < param->ch_cnt ; ++ch)
	{
		i_frame_count[ch] = 86400/2;
		p_frame_count[ch] = 0;
	}

	for (i = 0, ch = 0; i < MAX_SST_CHANNEL; i++)
	{
		if (param->open_ch[i])
		{
			param->ch_arr[ch] = i;
			param->sid[ch] = sst_play_open_ex(param->pr_id,
											i,
											param->rate,
											param->direction,
											param->rate_ctrl,	// 0
											param->hide,		// 0
											GTIMEVAL_TO_GUINT64(param->start_time),
											GTIMEVAL_TO_GUINT64(param->end_time),
											GTIMEVAL_TO_GUINT64(param->search_time),
											PB_TYPE_REMOTE,
											(param->rate == 1 && param->direction == 0) ? param->i_frame_only : 1,
											0);

			if (param->sid[ch] == -SST_ERR_NODATA)
			{
				printf("[%s][%d] sst_play_open_ex ch[%02d] NODATA\n",__FUNCTION__, __LINE__, i);
				param->eos_mask[i] = 1;
				++ch;
				continue;
			}
			else if (param->sid[ch] < 0)
			{
				printf("[%s][%d] sst_play_open_ex ch[%02d] ret[%d]\n", __FUNCTION__, __LINE__, i, param->sid[ch]);
				goto error_send_ctrl;
			}
			param->open_mask[i] = 1;

			while (1)
			{
				ret = sst_play_check_frame(param->sid[ch], &param->frame[ch]);

				if (ret == 0)
				{
					printf("[%s][%d] sst_play_check_frame ch[%02d] ret[%d]\n", __FUNCTION__, __LINE__, i, param->sid[ch]);
					break;
				}
				else if (ret == -SST_ERR_ENDDATA)
				{
					param->eos_mask[i] = 1;
					printf("[%s][%d] sst_play_check_frame 00 ch[%02d] sid[%d]\n", __FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch]);
					break;
				}
				else if (ret == -SST_ERR_EMPTY)
				{
					g_usleep(10*1000);
					continue;
				}
				else
				{
					printf("[%s][%d] sst_play_check_frame ch[%02d] sid[%d] ret[%d]\n", __FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch], ret);
					goto error_send_ctrl;
				}
			}
			++ch;
		}
		else
		{
			param->eos_mask[i] = 1;
		}
	}

	ret = rtsp_send_controlframe(NF_FRAME_TYPE_STARTDATA, param->task_id, &(param->is_run));
	if (ret == ISSM_NOT_EXIST_TASK)
	{
		printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
		goto error_send_ctrl;
	}

	wait_time = 50000 * param->ch_cnt;
	printf("[%s][%d] wait sst_buffering [%d]   ch_cnt [%d]\n", __FUNCTION__, __LINE__, wait_time, param->ch_cnt);
	g_usleep(wait_time);

	// send
	while (param->is_run)
	{
		ICODEC_HEADER *ih_frame;
		ICODEC_HEADER *ich;
		GobjBuddyBuffer *buf;
		int length;
		int write_size = 0;
		int target_ch = 0;

		tv_max.tv_sec = 0;
		tv_max.tv_usec = 0;
		tv_min.tv_sec = 0x7FFFFFFF;
		tv_min.tv_usec = 0x7FFFFFFF;

		// get lastest/oldest frame header
		for (ch = 0 ; ch < param->ch_cnt ; ++ch)
		{
			if (param->eos_mask[param->ch_arr[ch]])
				continue;

			tv_tmp.tv_sec = param->frame[ch].timestamp;
			tv_tmp.tv_usec = param->frame[ch].timestampl * 5000;

			if (param->frame[ch].frame_type == NF_FRAME_TYPE_RI)
			{
				tv_min_ch = tv_max_ch = ch;
				break;
			}

			if ((tv_tmp.tv_sec < tv_min.tv_sec) || (tv_tmp.tv_sec == tv_min.tv_sec && tv_tmp.tv_usec < tv_min.tv_usec))
			{
				tv_min = tv_tmp;
				tv_min_ch = ch;
			}

			if ((tv_tmp.tv_sec > tv_max.tv_sec) || (tv_tmp.tv_sec == tv_max.tv_sec && tv_tmp.tv_usec > tv_max.tv_usec))
			{
				tv_max = tv_tmp;
				tv_max_ch = ch;
			}
		}

		{
			int cnt_eos = 0, k;
			for (k = 0; k < MAX_SST_CHANNEL; k++)
			{
				if (param->eos_mask[k])
					cnt_eos++;
			}
			if (cnt_eos == MAX_SST_CHANNEL)
			{
				printf("[%s][%d] ALL ch eos_maks\n", __FUNCTION__, __LINE__);
	        	ret = rtsp_send_controlframe(NF_FRAME_TYPE_ENDDATA, param->task_id, &(param->is_run));
	        	if (ret == ISSM_NOT_EXIST_TASK)
	        		printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
				break;
			}
		}

		// NF_PLAY_PARAM_DIR_FORWARD
		if (param->direction == 0)
			target_ch = tv_min_ch;
		else
			target_ch = tv_max_ch;

		//wait 5second
		ret = sst_play_get_frame(param->sid[target_ch], &ih_frame, 5000);

		if (ret < 0)
		{
			printf("[%s][%d] sst_play_get_frame ch[%d] sid[%d] ret[%d]\n", __FUNCTION__, __LINE__, __FUNCTION__,param->ch_arr[target_ch], param->sid[target_ch], ret);
			goto error_send_ctrl;
		}

		if (!is_iframe_start && ih_frame->frame_type == NF_FRAME_TYPE_I)
		{
			is_iframe_start = 1;
			frame_ctl_start_time = issm_str_get_milisec();
			frame_ich_start_time = ih_frame->timestamp * 1000LL + ih_frame->timestampl * 5LL;
		}

		// CLIENT_MODE_ARCHIVING
		if (param->mode == 'A')
		{
			if (ih_frame->frame_type == NF_FRAME_TYPE_I)
				ih_frame->flags = 1;
			else
				ih_frame->flags = 0;
		}

		ih_frame->chan = ih_frame->chan % MAX_ISSM_V_CH;

		if (ih_frame->frame_type == NF_FRAME_TYPE_I)
		{
			i_frame_count[target_ch]++;
			p_frame_count[target_ch] = 0;
			ih_frame->reserved = i_frame_count[target_ch] << 16;
		}
		else if (ih_frame->frame_type == NF_FRAME_TYPE_RI)
		{
			i_frame_count[target_ch]--;
			p_frame_count[target_ch] = 30;
			ih_frame->reserved = i_frame_count[target_ch] << 16;
		}
		else if (ih_frame->frame_type == NF_FRAME_TYPE_P)
		{
			if (param->direction == 0)
				p_frame_count[target_ch]++;
			else
				p_frame_count[target_ch]--;

			p_frame_count[target_ch] = p_frame_count[target_ch] & 0x0000FFFF;
			ih_frame->reserved = (i_frame_count[target_ch] << 16) | p_frame_count[target_ch];
		}

		if (ih_frame->frame_type == NF_FRAME_TYPE_I || ih_frame->frame_type == NF_FRAME_TYPE_P)
		{
			if (resolution == 0)
				resolution = ih_frame->resolution;

			if (resolution != ih_frame->resolution)
				goto error_send_ctrl;
		}

		if (ih_frame->frame_type == NF_FRAME_TYPE_END)
		{
			goto error_send_ctrl;
		}

		if (ih_frame->frame_type == NF_FRAME_TYPE_METADATA)
		{
			length = ih_frame->frame_size + sizeof(ICODEC_HEADER);
			buf = gobj_buddy_buffer_new();
			gobj_buddy_buffer_buf_set_addr(buf, (guint8*)ih_frame);
			gobj_buddy_buffer_buf_set_size(buf, length);
		}
		else
		{
			length = ih_frame->frame_size + sizeof(ICODEC_HEADER);
			buf = gobj_buddy_buffer_new();
			gobj_buddy_buffer_buf_set_addr(buf, (guint8*)ih_frame);
			gobj_buddy_buffer_buf_set_size(buf, length);
		}
		ich = gobj_buddy_buffer_buf_get_addr(buf);

		//printf("[theweak1] ch : %d    ft : %02d  time : %d.%d\n", ih_frame->chan, ih_frame->frame_type, ih_frame->timestamp, ih_frame->timestampl*5);

		if (ih_frame->frame_type == NF_FRAME_TYPE_AUDIO && g_issm_is_audio_tx_enable)
		{
			ICODEC_HEADER audio_ich;
			long long audio_ich_time;
			int frame_size;
			int cut_frame_size = 0;

			audio_ich_time = ih_frame->timestamp * 1000LL + ih_frame->timestampl * 5LL;
			frame_size = ih_frame->frame_size;
			memcpy(&audio_ich, ih_frame, sizeof(ICODEC_HEADER));

			while (ih_frame->frame_size != cut_frame_size)
			{
				GobjBuddyBuffer *audio_buf;

				if (ih_frame->frame_size - cut_frame_size > 832)
					frame_size = 832;
				else
					frame_size = ih_frame->frame_size - cut_frame_size;

				audio_buf = gobj_buddy_buffer_new_malloc(sizeof(ICODEC_HEADER) + frame_size);

				audio_ich.timestamp = audio_ich_time / 1000LL;
				audio_ich.timestampl = (audio_ich_time % 1000LL) / 5LL;
				audio_ich.frame_size = frame_size;

				memcpy(gobj_buddy_buffer_buf_get_addr(audio_buf), &audio_ich, sizeof(ICODEC_HEADER));
				memcpy(gobj_buddy_buffer_buf_get_addr(audio_buf) + sizeof(ICODEC_HEADER),
				gobj_buddy_buffer_buf_get_addr(buf) + sizeof(ICODEC_HEADER) + cut_frame_size,
						frame_size);
				// GST_BUFFER_SIZE(buf) = sizeof(ICODEC_HEADER) + frame_size;

				g_ptr_array_add(audio_array, audio_buf);

				cut_frame_size += frame_size;
				audio_ich_time += (frame_size / 8);
			}

			g_ptr_array_sort(audio_array, (GCompareFunc)audio_ich_sort);

			while (1)
			{
				GobjBuddyBuffer *audio_buf;
				ICODEC_HEADER *audio_ich;
				long long audio_ich_time;

				if (audio_array->len == 0 || frame_ich_current_time == 0LL)
					break;

				audio_buf = g_ptr_array_index(audio_array, 0);
				audio_ich = gobj_buddy_buffer_buf_get_addr(audio_buf);
				audio_ich_time = audio_ich->timestamp * 1000LL + audio_ich->timestampl * 5LL;

				if (audio_ich_time < frame_ich_current_time + 100LL)
				{
					ret = ISSM_OK;

					if (is_iframe_start)
					{
						ret = issm_put_stream(0, 0, param->task_id, audio_buf);
					}

					g_object_unref(audio_buf);
					g_ptr_array_remove_index(audio_array, 0);

					if (ret == ISSM_NOT_EXIST_TASK)
					{
						printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
						goto error_send_ctrl;
					}
				}
				else
				{
					break;
				}
			}
		}

		/*
		if (ih_frame->frame_type != NF_FRAME_TYPE_P)
			printf("[%s] ch : %d    ft : %02d  time : %d.%d\n", __FUNCTION__, ih_frame->chan, ih_frame->frame_type, ih_frame->timestamp, ih_frame->timestampl*5);
		*/

		if (ih_frame->frame_type == NF_FRAME_TYPE_I || ih_frame->frame_type == NF_FRAME_TYPE_P)
		{
			int log_print = 0;
			long long send_check_time = 0LL;

			frame_ich_current_time = ih_frame->timestamp * 1000LL + ih_frame->timestampl * 5LL;

			//if (param->rate > 1)
			{
				long long new_time;
				if (param->direction)
					new_time = frame_ich_start_time + ((frame_ich_start_time - frame_ich_current_time) / division);
				else
					new_time = frame_ich_start_time + ((frame_ich_current_time - frame_ich_start_time) / division);

				ich->timestamp = (guint)(new_time / 1000LL);
				ich->timestampl = (guint)((new_time % 1000LL) / 5LL);

				send_check_time = frame_ctl_start_time + (new_time - frame_ich_start_time);
			}

			while (param->is_run) {
				int ret = ISSM_OK;

				while (param->is_run && send_check_time > issm_str_get_milisec())
					g_usleep(10*1000);

				if (is_iframe_start)
				{
					ret = issm_put_stream(0, 0, param->task_id, buf);
					//printf("[theweak2] ch : %d    ft : %02d  time : %d.%d\n", ich->chan, ich->frame_type, ich->timestamp, ich->timestampl*5);
				}

				if (ret == ISSM_OK)
				{
					break;
				}
				else if (ret == ISSM_NOT_EXIST_TASK)
				{
					printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
					goto error_send_ctrl;
				}

				g_usleep(10*1000);
			}

			do
			{
				frame_ctl_current_time = issm_str_get_milisec();
				frame_ich_current_time = ich->timestamp * 1000LL + ich->timestampl * 5LL;

				tdiff_ctl = frame_ctl_current_time - frame_ctl_start_time;
				tdiff_ich = frame_ich_current_time - frame_ich_start_time;

				tdiff_ctl += check_interval;

				while (1)
				{
					GobjBuddyBuffer *audio_buf;
					ICODEC_HEADER *audio_ich;
					long long audio_ich_time;

					if (audio_array->len == 0 || frame_ich_current_time == 0LL)
						break;

					if (ih_frame->frame_type == NF_FRAME_TYPE_AUDIO && g_issm_is_audio_tx_enable == 0)
						break;

					audio_buf = g_ptr_array_index(audio_array, 0);
					audio_ich = gobj_buddy_buffer_buf_get_addr(audio_buf);
					audio_ich_time = audio_ich->timestamp * 1000LL + audio_ich->timestampl * 5LL;

					if (audio_ich_time < frame_ich_current_time + 100LL)
					{
						ret = ISSM_OK;

						if (is_iframe_start)
						{
							//printf("[theweak2] ch : %d    ft : %02d  time : %d.%d\n", audio_ich->chan, audio_ich->frame_type, audio_ich->timestamp, audio_ich->timestampl*5);
							ret = issm_put_stream(0, 0, param->task_id, audio_buf);
						}
						g_object_unref(audio_buf);
						g_ptr_array_remove_index(audio_array, 0);

						if (ret == ISSM_NOT_EXIST_TASK)
						{
							printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
							goto error_send_ctrl;
						}
					}
					else
					{
						break;
					}
				}
			} while (param->is_run && tdiff_ich > tdiff_ctl);
		}

		g_object_unref(buf);
		ih_frame = NULL;
		ich = NULL;
		ch = target_ch;

		// get new header
		while (1)
		{
			ret = sst_play_check_frame(param->sid[ch], &param->frame[ch]);

			if (ret == 0)
			{
				break;
			}
			else if (ret == -SST_ERR_ENDDATA)
			{
				unsigned char overlap_status = 0;

                if (sst_play_check_overlap(param->sid[ch], &overlap_status) == 0)
				{
                	printf("[%s][%d] ############ overlap_status[%d]\n",
                			__FUNCTION__, __LINE__, overlap_status);

					if (overlap_status == 1)
					{
						g_usleep(10*1000);
						ret = rtsp_send_overlapframe(NF_FRAME_TYPE_OVERLAP, ch, param->task_id, &(param->is_run));
						if (ret == ISSM_NOT_EXIST_TASK)
						{
							printf("[%s][%d] not found task_id[%d]\n", __FUNCTION__, __LINE__, param->task_id);
							goto error_send_ctrl;
						}
					}
                }

				param->eos_mask[param->ch_arr[ch]] = 1;
				printf("[%s][%d] sst_play_check_frame 11 ch[%2d] sid[%d] ENDDATA\n",
							__FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch]);
				goto error_send_ctrl;
			}
			else if (ret == -SST_ERR_EMPTY)
			{
				g_usleep(10*1000);
				continue;
			}
			else
			{
				printf("[%s][%d] sst_play_check_frame ch[%2d] sid[%d] ret[%d]\n",
							__FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch], ret);
				goto error_send_ctrl;
			}
		}
	}

error_send_ctrl:

	// Todo : Implement session close reservation.
	issm_set_task_control(param->task_id, ISSM_TASK_CONTROL_CLOSE_WHEN_QUE_EMPTY, NULL);

	while (audio_array->len)
	{
		GobjBuddyBuffer *audio_buf;
		audio_buf = g_ptr_array_index(audio_array, 0);
		g_object_unref(audio_buf);
		g_ptr_array_remove_index(audio_array, 0);
	}
	g_ptr_array_free(audio_array, TRUE);

	g_mutex_lock(g_pb_param_mutex);
	g_ptr_array_remove(g_pb_param_array, param);
	g_mutex_unlock(g_pb_param_mutex);

	for (ch = 0; ch < param->ch_cnt; ch++)
	{
		if (param->sid[ch] >= 0)
		{
			ret = sst_play_close(param->sid[ch]);
			printf("[%s][%d] sst_play_close ch[%2d] sid[%d]\n",
					__FUNCTION__, __LINE__, param->ch_arr[ch], param->sid[ch]);
			param->sid[ch] = -1;
		}
	}

	if (param)
		g_free(param);

	printf("[%s][%d] end\n",__FUNCTION__, __LINE__);

	return 1;
}

