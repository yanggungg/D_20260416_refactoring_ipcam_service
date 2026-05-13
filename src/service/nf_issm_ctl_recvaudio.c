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
#include "nf_issm_ctl_recvaudio.h"
#include "nf_cntl_audio.h"

// Audio
#include <novatek/hdal.h>
#include <novatek/hd_debug.h>

#include "nf_codec_header.h"
#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"

// #if !defined(ENABLE_AUD_HI_CHIP)
// extern gboolean nf_hi_aud_send_frame_out(gpointer frame, gint size);
// #endif

// for recved_audio play
typedef struct _LIVE_AUDIO {
	int is_init;
	GThread *thread_id;
	int is_run;
	GAsyncQueue *queue;
	GMutex *mutex;

	unsigned char buf[800];
	unsigned int buf_len;
} LIVE_AUDIO;

typedef struct _RECV_AUDIO_DATA {
	unsigned char data[8000];
	unsigned int data_len;
} RECV_AUDIO_DATA;


static void network_audio_thread_func(void *p_data);
static LIVE_AUDIO g_live_audio = { 0, 0, 0, 0, 0 };
extern int g_issm_is_audio_rx_enable;


void nf_issm_ctl_recvaudio_init()
{
	if (!(g_live_audio.is_init))
	{
		g_live_audio.queue = g_async_queue_new();
		g_live_audio.mutex = g_mutex_new();
		g_live_audio.is_init = 1;
		g_atomic_int_set(&g_live_audio.is_run, 1);
	}

	memset(g_live_audio.buf, 0x00, sizeof(g_live_audio.buf));
	g_live_audio.buf_len = 0;
	g_live_audio.thread_id = g_thread_create((GThreadFunc)network_audio_thread_func,
										&g_live_audio, TRUE, NULL);
}

void nf_issm_ctl_recvaudio_close()
{
	if (g_live_audio.is_init)
	{
		g_atomic_int_set(&g_live_audio.is_run, 0);
		g_thread_join(g_live_audio.thread_id);

		g_live_audio.is_init = 0;
		g_mutex_free(g_live_audio.mutex);
		g_async_queue_unref(g_live_audio.queue);
	}
}

int nf_issm_cb_playaudio(void *p_data)
{
	unsigned char *d;
	unsigned char *audio_data;
	unsigned int audio_len;
	unsigned int audio_cut_len = 0;
	unsigned char buf[MAX_PLAY_AUDIO_LENGTH];
	unsigned int buf_len;
	gint is_rx_audio;

	is_rx_audio = (gint)nf_sysdb_get_bool("audio.rx");
	if (!is_rx_audio)
	{
		printf("[ISSM][%s][%d] Audio disabled - len[%d]\n", __FUNCTION__, __LINE__, audio_len);
		return ISSM_OK;
	}

	d = p_data;
	audio_data = d + 4;
	audio_len = (((d + 2)[0]<<8)|(d + 2)[1]);

	if (audio_len <= 0)
	{
		printf("[ISSM][%s][%d] Audio overflow - len[%d]\n", __FUNCTION__, __LINE__, audio_len);
		return ISSM_ERROR;
	}

	while (1)
	{
		memset(buf, 0x00, sizeof(buf));
		buf_len = 0;

		if (g_live_audio.buf_len > 0)
		{
			memcpy(buf, g_live_audio.buf, g_live_audio.buf_len);
			buf_len = g_live_audio.buf_len;
			g_live_audio.buf_len = 0;
		}

		if ((buf_len + (audio_len - audio_cut_len)) >= MAX_PLAY_AUDIO_LENGTH)
		{
			memcpy(buf + buf_len, audio_data + audio_cut_len, MAX_PLAY_AUDIO_LENGTH - buf_len);
			audio_cut_len += MAX_PLAY_AUDIO_LENGTH - buf_len;
			buf_len = MAX_PLAY_AUDIO_LENGTH;

			{
				RECV_AUDIO_DATA *recv_audio_data = g_malloc0(sizeof(RECV_AUDIO_DATA));
				memcpy(recv_audio_data->data, buf, MAX_PLAY_AUDIO_LENGTH);
				recv_audio_data->data_len = buf_len;

				g_mutex_lock(g_live_audio.mutex);
#if 1 // memory leak issue
				if (g_async_queue_length(g_live_audio.queue) < 7) {
					g_async_queue_push(g_live_audio.queue, recv_audio_data);
				} else {
					g_free(recv_audio_data);
					// printf("[ISSM][%s][%d] Audio Data Drop\n", __FUNCTION__, __LINE__);
				}
#else
				if (g_async_queue_length(g_live_audio.queue) < 7)
					g_async_queue_push(g_live_audio.queue, recv_audio_data);
#endif
				g_mutex_unlock(g_live_audio.mutex);
			}
		}
		else
		{
			memcpy(g_live_audio.buf, audio_data + audio_cut_len, audio_len - audio_cut_len);
			g_live_audio.buf_len = audio_len - audio_cut_len;
			audio_cut_len = audio_len;
		}

		if (audio_cut_len == audio_len)
			break;
	}
}


static void network_audio_thread_func(void *p_data)
{
	int ret = 0;
	int idle_cnt = 0;
	LIVE_AUDIO *live_audio;
	RECV_AUDIO_DATA *recv_audio_data;
	int cnt_buffering = 0;
	char dataBuf[MAX_PLAY_AUDIO_LENGTH] = { 0, };

	live_audio = p_data;

	// playback_audi_cb
	while (g_live_audio.is_run)
	{
		g_mutex_lock(live_audio->mutex);
		recv_audio_data = g_async_queue_try_pop(live_audio->queue);
		g_mutex_unlock(live_audio->mutex);

		if (recv_audio_data)
		{
			if (g_issm_is_audio_rx_enable)
			{
				if (cnt_buffering == ((MAX_PLAY_AUDIO_LENGTH / recv_audio_data->data_len) -1 ))
				{
					idle_cnt=0;

					// For no Buffering
					if (cnt_buffering == 0)
						memcpy(dataBuf + (cnt_buffering * recv_audio_data->data_len),
								recv_audio_data->data, recv_audio_data->data_len);

					nf_network_set_webra_audio_status(1);
					nf_dev_audio_set_netrx(1);	// netrx
					// nf_cntl_audio_write_data(dataBuf, sizeof(dataBuf));
					// #if !defined(ENABLE_AUD_HI_CHIP)
					// nf_hi_aud_send_frame_out(dataBuf, sizeof(dataBuf));
					// #endif
					//ksi_test
					ret = nf_audio_pb_cb_web_mic(0, dataBuf, sizeof(dataBuf), 0, NF_AUD_PINPERFRM_CNT_NET, FALSE);
					if(sizeof(dataBuf) != (guint)ret)
					{
						printf("[%s][%d] Line[%d] Mic Data Write Error! ret[%d] msg_len[%d]",
								__FUNCTION__, __LINE__, ret, recv_audio_data->data_len);
					}
					// cnt_buffering=0;
				}
				else
				{
					memcpy(dataBuf + (cnt_buffering * recv_audio_data->data_len),
							recv_audio_data->data, recv_audio_data->data_len);
					cnt_buffering++;
				}
			}
			g_free(recv_audio_data);
		}
		else
		{
			idle_cnt++;
		}

		g_usleep(33000);

		if (idle_cnt >= 7)
			nf_network_set_webra_audio_status(0);

		if (idle_cnt == 8)
			nf_dev_audio_set_netrx(0);	// netrx

	}

	while (g_async_queue_length(live_audio->queue))
	{
		g_mutex_lock(live_audio->mutex);
		recv_audio_data = g_async_queue_try_pop(live_audio->queue);
		g_mutex_unlock(live_audio->mutex);

		if (recv_audio_data)
			g_free(recv_audio_data);
	}

	return;
}

