#ifndef __NF_AUDIO_PUT_SST_H__
#define __NF_AUDIO_PUT_SST_H__

#define NF_AUDIO_SST_START_CH_NO			(64)

gboolean nf_audio_SSTput(int s32Chn, NF_AUDIO_QDATA *pstQdata, NF_AUDIO_PARAM *pstParam);
gboolean nf_audio_SSTctrl(NF_AUDIO_PARAM *pstOld,  NF_AUDIO_PARAM *pstNew);

#endif

