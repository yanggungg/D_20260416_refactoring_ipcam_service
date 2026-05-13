#ifndef __NF_IPCAM_CNTL_AUDIO_H__
#define __NF_IPCAM_CNTL_AUDIO_H__

#define AUDIO_DIV_UNIT						(8)
#define NF_IPCAM_AUDIO_RESET_INTERVAL		(10*60*AUDIO_DIV_UNIT)  // 10

void nf_ipcam_send_stream(gint ch, gchar *data, guint len);
gboolean nf_cntl_audio_write_data(guchar *stream, guint len);
int playback_audio_cb(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type);

#endif
