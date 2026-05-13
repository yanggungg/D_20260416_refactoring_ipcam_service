#ifndef __NF_AUDIO_CONVERT_H__
#define __NF_AUDIO_CONVERT_H__

guchar nf_audio_cvt_lpcm16_to_muraw(short pcm_val);
short nf_audio_cvt_search_seg(short val, short *table, short size);
short nf_audio_cvt_muraw_to_lpcm16(guchar u_val);
void nf_audio_convert(guchar *stream_src, gchar *stream_dest, guint len);

#endif

