#ifndef __NF_HI_AUD_API_H__
#define __NF_HI_AUD_API_H__

#include "nf_HI_common.h"
#include "nf_rec_audio.h"

#define NF_HI_AUD_SAMPLE_DBG(s32Ret)\
do{\
	printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4    /* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K             /* MEDIA_G726_16K, MEDIA_G726_24K ... */

// Hisilicon Header File
typedef struct tagSAMPLE_AI_S
{
	HI_BOOL bStart;
	HI_S32  AiDev;
	HI_S32  AiChn;
	HI_S32  AencChn;
	HI_S32  AoDev;
	HI_S32  AoChn;
	HI_BOOL bSendAenc;
	HI_BOOL bSendAo;
	pthread_t stAiPid;
} SAMPLE_AI_S;

gboolean nf_HI_AUD_Acodec_CfgAudio(AUDIO_SAMPLE_RATE_E enSample, int fd);
HI_S32 nf_HI_AUD_initAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, 
						HI_BOOL bResampleEn, AI_VQE_CONFIG_S* pstAiVqeAttr, HI_U32 u32AiVqeType);
HI_S32 nf_HI_AUD_initAenc(HI_S32 s32AencChnCnt, HI_U32 u32AencPtNumPerFrm, PAYLOAD_TYPE_E enType);
HI_S32 nf_HI_AUD_Bind_Aenc(AUDIO_DEV AiDev, HI_S32 s32AencChnCnt, HI_BOOL bUserGetMode );
HI_BOOL nf_HI_aud_getFd(HI_S32 ps32AudFd[], HI_S32 *ps32MaxFd);
HI_BOOL nf_HI_AUD_initAdec( ADEC_CHN AdChn ,PAYLOAD_TYPE_E enType);
HI_BOOL nf_HI_AUD_startAdecAo(void);
HI_S32 nf_HI_AUD_StartHdmi(AIO_ATTR_S *pstAioAttr);
inline HI_S32 nf_HI_AUD_StopHdmi(HI_VOID);
HI_S32 nf_HI_AUD_VO_HdmiStop(HI_VOID);
HI_S32 nf_HI_AUD_initAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, 
						HI_BOOL bResampleEn, HI_VOID* pstAoVqeAttr, HI_U32 u32AoVqeType);
HI_S32 nf_HI_AUD_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn);
HI_S32 nf_HI_AUD_AencBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn);
HI_S32 nf_HI_AUD_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn);
HI_S32 nf_HI_AUD_AoBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn);
HI_S32 nf_HI_AUD_AoUnBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn);
HI_S32 nf_HI_AUD_AoUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn);
static void *nf_HI_COMM_AUDIO_AiProc(void *parg);
HI_S32 nf_HI_AUD_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, HI_BOOL bResampleEn, HI_BOOL bVqeEn);
HI_BOOL nf_HI_AUD_stopAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn, HI_BOOL bVqeEn);

#endif	/* __NF_HI_AUD_API_H__ */

