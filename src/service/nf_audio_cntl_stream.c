#include "nf_common.h"
#include "nf_debug.h"

#include <gobj.h>
#include <gobjmedia.h>

#include <novatek/hdal.h>
#include <novatek/hd_debug.h>

#include "nf_codec_header.h"

#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"
#include "nf_audio_cntl_stream.h"
#include "nf_api_ipcam.h"

/**
	Extern Function
**/
extern guint nf_live_get_audio_output_type(void);

void nf_ipcam_send_stream(gint ch, gchar *data, guint len)
{
#if 1
	static int playing[64] = {0, };
	static int send_cnt[64] = {0,};

	gint send_bytes = 0;
	gint ret = 0;

	NFIPCamAudioRaw audio;
	NFIPCamAudioRaw audio_cmd;

	g_return_if_fail ( ch < 64);

	if( len == 0 ) // close condition
	{

		if( playing[ch] )
		{
			audio_cmd.ch = ch;
			audio_cmd.type = NF_IPCAM_SEND_AUDIO_END;
			audio_cmd.buf = NULL;
			nf_ipcam_send_audio(ch, &audio_cmd, &send_bytes, NULL);
			playing[ch] = 0;
			send_cnt[ch] = 0;
		}

		return;
	}

	// for start cmd
	if( !playing[ch] ){
		audio_cmd.ch = ch;
		audio_cmd.type = NF_IPCAM_SEND_AUDIO_START;
		audio_cmd.buf = NULL;
		ret = nf_ipcam_send_audio(ch, &audio_cmd, &send_bytes, NULL);
		if( ret == 1) {
			playing[ch] = 1;
			send_cnt[ch] = 0;
		}else{
			send_cnt[ch] = 0;
			return;
		}
	}

	audio.buf = gobj_buddy_buffer_new_malloc(len);
	g_return_if_fail ( audio.buf != NULL);

	if(nf_network_get_webra_audio_status())
		goto out;

	// data
	audio.ch = ch;
	audio.type = NF_IPCAM_SEND_AUDIO_DATA;
	memcpy(gobj_buddy_buffer_buf_get_addr(audio.buf), data, len);
	ret = nf_ipcam_send_audio(ch, &audio, &send_bytes, NULL);

	#if 0
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_MICOUT] )
			g_message("%s ch[%d] len[%d] ret[%d]", __FUNCTION__, ch, len, ret);
	#endif

	if( ++send_cnt[ch] > NF_IPCAM_AUDIO_RESET_INTERVAL || ret != 1 ) {
		// for end cmd
		audio_cmd.ch = ch;
		audio_cmd.type = NF_IPCAM_SEND_AUDIO_END;
		audio_cmd.buf = NULL;
		nf_ipcam_send_audio(ch, &audio_cmd, &send_bytes, NULL);
		playing[ch] = 0;
		send_cnt[ch] = 0;
	}

out:
	if ( audio.buf )
		g_object_unref(audio.buf);
#endif

	return;
}

//#define NF_AUDIO_CNTL_DATA_DUMP
gboolean nf_cntl_audio_write_data(guchar *stream, guint len)
{
#if 0
	static gint fd=0;
	#if defined(ENABLE_AUD_HI_CHIP)
	guchar data[HI_AUDIO_DATA_SEND_SIZE]={0, };
	#endif
	#if defined(NF_AUDIO_CNTL_DATA_DUMP)
		static FILE *fp_aud=NULL;
	#endif

	#if defined(ENABLE_AUD_HI_CHIP)
		g_return_val_if_fail((len == HI_AUDIO_DATA_SEND_SIZE) , FALSE);
	#endif

	#if defined(NF_AUDIO_CNTL_DATA_DUMP)
		if(fp_aud==NULL)
		{
			if((fp_aud = fopen("aud_net.raw", "w")) == NULL)
			{
				g_warning("%s File Open Error!!", __FUNCTION__);
				return HI_FALSE;
			}
			else
				g_message("%s Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
		}
	#endif

	#if defined(ENABLE_AUD_HI_CHIP)
		memcpy(data, stream, HI_AUDIO_DATA_SEND_SIZE);
	#endif
	#if defined(ENABLE_AUD_HI_CHIP)
		playback_audio_cb_net(NULL, data, len, 0, HI_AUD_STM_CNT_NET);

		#if defined(NF_AUDIO_CNTL_DATA_DUMP)
			if(fwrite(stream, 1, len, fp_aud) != len)
				g_warning("%s fwrite errpr!!!", __FUNCTION__);
		#endif
	#else
		if(fd <= 0)
		{
			fd = nf_dev_open_dsp_write();
			if(fd < 0)
			{
				g_warning("%s DSP Write Open Failed!!", __FUNCTION__);
				return -1;
			}
		}

		if(nf_live_get_audio_output_type() == AUD_OUTPUT_RCA) {
			Write(fd, stream, len);
		}
	#endif
#endif

	return TRUE;
}

#if 0
#if defined(_IPX_0412M4) || defined(_IPX_0824M4)|| defined(_IPX_0412M4E) || defined(_IPX_0824M4E)
int playback_audio_cb(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type)
{
	static gint fd=0;

	if(nf_network_get_webra_audio_status())
		return stream_size;

	if(fd <= 0)
	{
		fd = nf_dev_open_dsp_write();
		if(fd < 0)
		{
			g_warning("%s DSP Write Open Failed!!", __FUNCTION__);
			return -1;
		}
	}

	Write(fd, stream_buf, stream_size);

	return 0;
}
#endif
#endif

