
#include "nf_common.h"

/* Novatec Header */
#include <novatek/hdal.h>
#include <novatek/hd_debug.h>

#include "nf_codec_header.h"
#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"
#include "nf_audio_pb_local.h"

// For Network Web Mic PB
//#define NF_AUDIO_DUMP_DATA_PB_LOCAL
//#define NF_AUDIO_DEBUG_PB_LOCAL


/**
	Extern Function
**/
#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)
extern gboolean nf_sysman_hotkey_is_nfs( void );
#endif
#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
extern short nf_audio_cvt_muraw_to_lpcm16(guchar u_val);
#endif
static int nf_audio_pb_local_type_a(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear);
static int nf_audio_pb_local_type_b(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear);

int nf_audio_pb_local(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear)
{
	int ret=0;

	if(stream_size == NF_AUD_PINPERFRM_ITX) {
		ret=nf_audio_pb_local_type_a(h_stream_buf, stream_buf, stream_size, codec_type, pstAudPb, is_buffer_clear);
	}
	else {
		g_message("%s line%d Implement Function!!", __FUNCTION__, __LINE__);
	}

	return ret;
}

/*
   When PB Size(stream_size) is 8160byte
*/
static int nf_audio_pb_local_type_a(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear)
{
	NF_AUDIO_QDATA_PB *data_pb_local;
	gint qlen=0, cnt_send=0;
	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
		short   pcm_val;
	#else
		guchar pcm_val;
	#endif
	guchar  u_val;
	gshort  *AudioStream=NULL;
	gint i=0, j=0;
	gchar *temp_ptr=NULL;
	gchar data_send[NF_AUD_PINPERFRM]={0, };
	#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)
		static FILE *fp_pb_local=NULL;
		gboolean is_open_error=FALSE;
		gboolean is_uraw=FALSE;
	#endif
	// For Gather Data
	static int remain=0, size_total=0;
	gint size_copied=0;
	gchar *ptr=NULL;
	gint tmp=0;
	static gboolean is_remain_short=FALSE;
	static int remain_size_short=0;
	int stream_size_pcm=0;

	if(is_buffer_clear) {

		g_message("[PB_LOCAL] Buffer Clear!!");

		size_copied=0; size_total=0; remain=0;

		return stream_size;
	}

	stream_size_pcm=stream_size * sizeof(unsigned short);
	#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)

		if(fp_pb_local == NULL) {
			if((fp_pb_local = fopen("/audio_pb_local.raw", "w")) == NULL) {
				g_warning("[PB_LOCAL][%s] File Open Error!!", __FUNCTION__);
				is_open_error=TRUE;
			}
			else {
				g_message("[PB_LOCAL][%s] Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
			}
		}

		if(is_uraw) {
			if( nf_sysman_hotkey_is_nfs() )
			{
				if(!is_open_error) {
					fwrite(stream_buf, 1, stream_size, fp_pb_local);
					g_message("[PB_LOCAL] Data Write!!! Uraw");
				}
			}
		}
	#endif

	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
	AudioStream=(gshort *) g_malloc0(stream_size_pcm);
	#else
	AudioStream=(gshort *) g_malloc0(stream_size);
	#endif
	if(AudioStream == NULL) {
		g_message("[PB_LOCAL][%s] Alloc Fail!!", __FUNCTION__);
	}

	// Converting
	for(j=0; j<1; j++)
	{
		gint index=0;

		temp_ptr = stream_buf + (j * stream_size);

		for(i=0; i <stream_size; i++)
		{
			u_val = (guchar)*(temp_ptr + i);
			#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
				pcm_val = nf_audio_cvt_muraw_to_lpcm16(u_val);
			#else
				pcm_val = u_val;
			#endif

			index=(i + (j * stream_size));
			*(AudioStream + index) = pcm_val;
		}
	}

	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
	if((stream_size_pcm % NF_AUD_PINPERFRM) == 0) {
	#else
	if((stream_size % NF_AUD_PINPERFRM) == 0) {
	#endif
		int size=0, cnt_retry=0;

		size=stream_size_pcm;

		while(size > 0) {

			data_pb_local=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));

			memcpy(data_send, (gchar *)AudioStream + (NF_AUD_PINPERFRM * cnt_send), NF_AUD_PINPERFRM);

			memcpy(data_pb_local->data, data_send, NF_AUD_PINPERFRM);
			#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)
				if(!is_uraw) {
					if( nf_sysman_hotkey_is_nfs() )
					{
						if(!is_open_error) {
							fwrite(data_pb_local->data, 1, NF_AUD_PINPERFRM, fp_pb_local);
							g_message("[PB_LOCAL] Local Data Write!!! PCM");
						}
					}
				}
			#endif
			data_pb_local->cnt=NF_AUD_PINPERFRM;
			data_pb_local->outmode=AUD_PB_OUT_MODE_DVR;

nf_audio_retry_pb_local:
			qlen=g_async_queue_length(pstAudPb->queue_pb);
			if(qlen > NF_AUDIO_PB_LOCAL_MAX_QUEUE)
			{
				#if 1
					int i;
					NF_AUDIO_QDATA_PB *qdata_tmp = NULL;

					g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
					g_free(data_pb_local);
					free(AudioStream);
					size_total=0;
					for (i =0; i < qlen; i++) {
						qdata_tmp = g_async_queue_pop(pstAudPb->queue_pb);
						g_free(qdata_tmp);
					}

					return FALSE;
				#else
					cnt_retry++;
					g_usleep(100000);
					goto nf_audio_retry_pb_local;
				#endif
			}
			
			if(cnt_retry != 0) {
				g_warning("[PB_LOCAL][%s] queue full.. retry_count[%d]", __FUNCTION__, cnt_retry);
			}
				
			#if defined(NF_AUDIO_DEBUG_PB_LOCAL)
				g_message("[PB_LOCAL][%s] line%d Send Queue", __FUNCTION__, __LINE__);
			#endif
			g_async_queue_push(pstAudPb->queue_pb, data_pb_local);
			
			cnt_send++;

			size-=NF_AUD_PINPERFRM;
		}

		free(AudioStream);
	}
	else {      // To Do for remain data!!
		g_message("[PB_LOCAL][%s] line%d Implement Function!!", __FUNCTION__, __LINE__);
		
		free(AudioStream);
	}

	return stream_size;
}

/*
   When PB Size(stream_size) is not 8160byte
*/
static int nf_audio_pb_local_type_b(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear)
{

	NF_AUDIO_QDATA_PB *data_pb_local;
	gint qlen=0, cnt_send=0;
	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
		short   pcm_val;
	#else
		guchar pcm_val;
	#endif
	guchar  u_val;
	gshort  *AudioStream=NULL;
	gint i=0, j=0;
	gchar *temp_ptr=NULL;
	#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)
		static FILE *fp_pb_local=NULL;
		gboolean is_open_error=FALSE;
		gboolean is_uraw=FALSE;
	#endif
	// For Gather Data
	static int remain=0, size_total=0;
	gint size_copied=0;
	static gchar data_remain[NF_AUDIO_DATA_SIZE_PB_LOCAL_PCM]={0, }, data_send[NF_AUD_PINPERFRM]={0, };
	gchar *ptr=NULL;
	gint tmp=0;
	static gboolean is_remain_short=FALSE;
	static int remain_size_short=0;

	if(stream_size != NF_AUDIO_DATA_SIZE_PB_LOCAL) {
		
		g_warning("[PB_LOCAL][%s] Check Stream Size!!", __FUNCTION__);

		return stream_size;
	}

	if(is_buffer_clear) {

		g_message("[PB_LOCAL][%s] Buffer Clear!!", __FUNCTION__);

		size_copied=0; size_total=0; remain=0;
		memset(data_send, 0x0, sizeof(data_send));
		memset(data_remain, 0x0, sizeof(data_remain));

		return stream_size;
	}

	#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)

		if(fp_pb_local == NULL) {
			if((fp_pb_local = fopen("/audio_pb_local.raw", "w")) == NULL) {
				g_warning("[PB_LOCAL][%s] File Open Error!!", __FUNCTION__);
				is_open_error=TRUE;
			}
			else
				g_message("[PB_LOCAL][%s] Line[%d] File Open Success!!", __FUNCTION__, __LINE__);
		}

		if(is_uraw) {
			if( nf_sysman_hotkey_is_nfs() )
			{
				if(!is_open_error) {
					fwrite(stream_buf, 1, NF_AUDIO_DATA_SIZE_PB_LOCAL, fp_pb_local);
					g_message("[PB_LOCAL] Data Write!!! Uraw");
				}
			}
		}
	#endif

	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
	AudioStream=(gshort *) g_malloc0(NF_AUDIO_DATA_SIZE_PB_LOCAL_PCM);
	#else
	AudioStream=(gshort *) g_malloc0(NF_AUDIO_DATA_SIZE_PB_LOCAL);
	#endif
	if(AudioStream == NULL) {
		g_message("[PB_LOCAL][%s] alloc fail!!", __FUNCTION__);
	}

	// Converting
	for(j=0; j<1; j++)
	{
		gint index=0;

		temp_ptr = stream_buf + (j * NF_AUDIO_DATA_SIZE_PB_LOCAL);

		for(i=0; i <NF_AUDIO_DATA_SIZE_PB_LOCAL; i++)
		{
			u_val = (guchar)*(temp_ptr + i);
			#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
				pcm_val = nf_audio_cvt_muraw_to_lpcm16(u_val);
			#else
				pcm_val = u_val;
			#endif

			index=(i + (j * NF_AUDIO_DATA_SIZE_PB_LOCAL));
			*(AudioStream + index) = pcm_val;
		}
	}

	#if defined(NF_AUDIO_CONVERT_LPCM16_TO_URAW)
	if((NF_AUDIO_DATA_SIZE_PB_LOCAL_PCM % NF_AUD_PINPERFRM) == 0) {
	#else
	if((NF_AUDIO_DATA_SIZE_PB_LOCAL % NF_AUD_PINPERFRM) == 0) {
	#endif
		int size=0, cnt_retry=0;

		size=NF_AUDIO_DATA_SIZE_PB_LOCAL_PCM;

		while(size > 0) {

			data_pb_local=(NF_AUDIO_QDATA_PB *)g_malloc0(sizeof(NF_AUDIO_QDATA_PB));

			memcpy(data_send, (gchar *)AudioStream + (NF_AUD_PINPERFRM * cnt_send), NF_AUD_PINPERFRM);

			memcpy(data_pb_local->data, data_send, NF_AUD_PINPERFRM);
			#if defined(NF_AUDIO_DUMP_DATA_PB_LOCAL)
				if(!is_uraw) {
					if( nf_sysman_hotkey_is_nfs() )
					{
						if(!is_open_error) {
							fwrite(data_pb_local->data, 1, NF_AUD_PINPERFRM, fp_pb_local);
							g_message("[PB_LOCAL] Data Write!!! PCM");
						}
					}
				}
			#endif
			data_pb_local->cnt=NF_AUD_PINPERFRM;
			data_pb_local->outmode=AUD_PB_OUT_MODE_DVR;

nf_audio_retry_pb_local:
			qlen=g_async_queue_length(pstAudPb->queue_pb);
			if(qlen > NF_AUDIO_PB_LOCAL_MAX_QUEUE)
			{
				#if 0
					g_warning("%s queue full.. len[%d]",__FUNCTION__, qlen);
					g_free(data_pb_local);
					free(AudioStream);
					size_total=0;
					return FALSE;
				#else
					cnt_retry++;
					g_usleep(100000);
					goto nf_audio_retry_pb_local;
				#endif
			}

			if(cnt_retry != 0) {
				g_warning("[PB_LOCAL][%s] Queue Full.. retry_count[%d]", __FUNCTION__, cnt_retry);
			}

			#if defined(NF_AUDIO_DEBUG_PB_LOCAL)
				g_message("%s line%d Send Queue", __FUNCTION__, __LINE__);
			#endif
			g_async_queue_push(pstAudPb->queue_pb, data_pb_local);

			cnt_send++;

			size-=NF_AUD_PINPERFRM;
		}

		free(AudioStream);
	}
	else {		// To Do for remain data!!
		g_message("[PB_LOCAL][%s] line%d Implement Function!!", __FUNCTION__, __LINE__);

		free(AudioStream);
	}

    return stream_size;
}

// ipcam -> nvr speaker
int nf_audio_pb_local_ipcam(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear)
{
	static guchar remain_buffer[NF_AUDIO_DATA_SIZE_PB_LOCAL]={0,};
	static int remain_size=0;
	guchar *acc_buffer;
	int acc_size=0, send_size=0;
	static int pre_status=0;

	if(stream_size>NF_AUDIO_DATA_SIZE_PB_LOCAL_PCM)
	{
		g_message("%s line%d stream_size[%d] overflow..",__FUNCTION__, __LINE__, stream_size);
		return -1;
	}

	acc_buffer = (guchar*)malloc(stream_size + remain_size);

	if(remain_size>0)
	{
		memcpy(acc_buffer, remain_buffer, remain_size);
	}
	memcpy(acc_buffer+remain_size, stream_buf, stream_size);
	acc_size=stream_size+remain_size;

	memset(remain_buffer, 0x0, NF_AUDIO_DATA_SIZE_PB_LOCAL);
	remain_size=0;

	while(acc_size>0)
	{
		if(acc_size>=NF_AUDIO_DATA_SIZE_PB_LOCAL)
		{
			nf_audio_pb_local(h_stream_buf, acc_buffer+send_size, NF_AUDIO_DATA_SIZE_PB_LOCAL, codec_type, pstAudPb, is_buffer_clear);
			acc_size-=NF_AUDIO_DATA_SIZE_PB_LOCAL;
			send_size+=NF_AUDIO_DATA_SIZE_PB_LOCAL;
		}
		else
		{
			memcpy(remain_buffer, acc_buffer+send_size, acc_size);
			remain_size+=acc_size;
			acc_size=0;
		}
	}
	free(acc_buffer);
	return stream_size;
}