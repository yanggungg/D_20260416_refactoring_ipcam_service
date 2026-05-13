#ifndef __NF_AUDIO_PUT_FRAME_H__
#define __NF_AUDIO_PUT_FRAME_H__

gboolean nf_audio_put_frame_gst_buffer_sst(guint ch, gpointer frame, NF_AUDIO_PARAM *pstParam);
gboolean nf_audio_put_frame(gint ch_num, gpointer frame);

#endif

