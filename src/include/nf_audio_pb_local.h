#ifndef __NF_AUDIO_PB_LOCAL_H__
#define __NF_AUDIO_PB_LOCAL_H__

// for local playback
#define NF_AUDIO_DATA_SIZE_PB_LOCAL			(8320)		// uraw data
#define NF_AUDIO_DATA_SIZE_PB_LOCAL_PCM		(NF_AUDIO_DATA_SIZE_PB_LOCAL * sizeof(gushort))		// 16320
#define NF_AUDIO_PB_LOCAL_MAX_QUEUE			100


int nf_audio_pb_local(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear);

int nf_audio_pb_local_ipcam(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, 
						NF_AUDIO_PB *pstAudPb, gboolean is_buffer_clear);
#endif

