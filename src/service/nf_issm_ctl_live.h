#ifndef _NF_ISSM_CTL_LIVE_H__
#define _NF_ISSM_CTL_LIVE_H_
int nf_issm_ctl_put_frame(void *frame);
int nf_issm_ctl_put_zero_channel_frame(void *frame);
unsigned int nf_issm_ctl_get_bitrate(int p_ch, int p_stream_idx);
#endif