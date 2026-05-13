#include <sys/ioctl.h>

#include "nf_HI_aud_api.h"
#include "nf_HI_common.h"
#include "hi35XX/acodec.h"
#include "nf_HI_aud_3536.h"

#define DBG_MSG_AUD_REC         DBG_MSG_NONE
//#define DBG_MSG_AUD_REC       DBG_MSG_AUD
#define DBG_MSG_AUD_CFG         DBG_MSG_NONE
//#define DBG_MSG_AUD_CFG       DBG_MSG_AUD

/**
	Extern Function Definition!!
**/
extern HI_S32 HI_MPI_HDMI_Start(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_Stop(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_SetAttr(HI_HDMI_ID_E enHdmi, HI_HDMI_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_HDMI_GetAttr(HI_HDMI_ID_E enHdmi, HI_HDMI_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_HDMI_SetAVMute(HI_HDMI_ID_E enHdmi, HI_BOOL bAvMute);
extern HI_S32 HI_MPI_HDMI_Close(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_DeInit(HI_VOID);

extern HI_S32 HI_MPI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);

extern HI_S32 HI_MPI_AI_SetPubAttr(AUDIO_DEV AiDevId,const AIO_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_AI_Enable(AUDIO_DEV AiDevId);
extern HI_S32 HI_MPI_AI_EnableChn(AUDIO_DEV AiDevId, AI_CHN AiChn);
extern HI_S32 HI_MPI_AI_EnableReSmp(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_SAMPLE_RATE_E enOutSampleRate);
extern HI_S32 HI_MPI_AI_SetVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_DEV AoDevId, AO_CHN AoChn,AI_VQE_CONFIG_S *pstVqeConfig);
extern HI_S32 HI_MPI_AI_EnableVqe(AUDIO_DEV AiDevId, AI_CHN AiChn);
extern HI_S32 HI_MPI_AI_DisableReSmp(AUDIO_DEV AiDevId, AI_CHN AiChn);
extern HI_S32 HI_MPI_AI_DisableVqe(AUDIO_DEV AiDevId, AI_CHN AiChn);
extern HI_S32 HI_MPI_AI_DisableChn(AUDIO_DEV AiDevId, AI_CHN AiChn);
extern HI_S32 HI_MPI_AI_Disable(AUDIO_DEV AiDevId);

extern HI_S32 HI_MPI_AENC_CreateChn(AENC_CHN AeChn, const AENC_CHN_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_AENC_GetFd(AENC_CHN AeChn);
extern HI_S32 HI_MPI_ADEC_CreateChn(ADEC_CHN AdChn, ADEC_CHN_ATTR_S *pstAttr);

extern HI_S32 HI_MPI_VO_Disable(VO_DEV VoDev);
extern HI_S32 HI_MPI_AO_SetPubAttr(AUDIO_DEV AoDevId ,const AIO_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_AO_Enable(AUDIO_DEV AoDevId);
extern HI_S32 HI_MPI_AO_EnableChn(AUDIO_DEV AoDevId, AO_CHN AoChn);
extern HI_S32 HI_MPI_AO_DisableReSmp(AUDIO_DEV AoDevId, AO_CHN AoChn);
extern HI_S32 HI_MPI_AO_EnableReSmp(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_SAMPLE_RATE_E enInSampleRate);
extern HI_S32 HI_MPI_AO_SetVqeAttr(AUDIO_DEV AoDevId, AO_CHN AoChn, AO_VQE_CONFIG_S *pstVqeConfig);
extern HI_S32 HI_MPI_AO_EnableVqe(AUDIO_DEV AoDevId, AO_CHN AoChn);
extern HI_S32 HI_MPI_AO_DisableVqe(AUDIO_DEV AoDevId, AO_CHN AoChn);
extern HI_S32 HI_MPI_AO_DisableChn(AUDIO_DEV AoDevId, AO_CHN AoChn);
extern HI_S32 HI_MPI_AO_Disable(AUDIO_DEV AoDevId);

extern HI_S32 HI_MPI_SYS_Bind(MPP_CHN_S *pstSrcChn, MPP_CHN_S *pstDestChn);
extern HI_S32 HI_MPI_SYS_UnBind(MPP_CHN_S *pstSrcChn, MPP_CHN_S *pstDestChn);


extern HI_S32 nf_HI_VO_HdmiStart(VO_INTF_SYNC_E enIntfSync);

/**
	Extern Variable Definition!!
**/
extern gint nf_live_get_cnt_audio_input(void);

/** 
	SAMPLE_COMM_AUDIO_CfgAcodec
	config codec 
**/
gboolean nf_HI_AUD_Acodec_CfgAudio(AUDIO_SAMPLE_RATE_E enSample, int fd)
{
	HI_S32 fdAcodec = -1;
	HI_S32 ret = HI_SUCCESS;
	unsigned int i2s_fs_sel = 0;
	unsigned int mixer_mic_ctrl = ACODEC_MIXER_LINEIN;
	unsigned int output_ctrl = ACODEC_LINEOUTD_NONE;
	unsigned int gain_mic = 0;
	unsigned int mute = 0;
	int pd = 0;

#if 0
	fdAcodec = open(ACODEC_FILE,O_RDWR);
	if (fdAcodec < 0)
	{
		printf("%s: can't open Acodec,%s\n", __FUNCTION__, ACODEC_FILE);
		ret = HI_FAILURE;
	}
#else
	fdAcodec=fd;
#endif
	if(ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL))
	{
		printf("Reset audio codec error\n");
	}

	if ((AUDIO_SAMPLE_RATE_8000 == enSample)
		|| (AUDIO_SAMPLE_RATE_11025 == enSample)
		|| (AUDIO_SAMPLE_RATE_12000 == enSample))
	{
		i2s_fs_sel = 0x18;
	}
	else if ((AUDIO_SAMPLE_RATE_16000 == enSample)
		|| (AUDIO_SAMPLE_RATE_22050 == enSample)
		|| (AUDIO_SAMPLE_RATE_24000 == enSample))
	{
		i2s_fs_sel = 0x19;
	}
	else if ((AUDIO_SAMPLE_RATE_32000 == enSample)
		|| (AUDIO_SAMPLE_RATE_44100 == enSample)
		|| (AUDIO_SAMPLE_RATE_48000 == enSample))
	{
		i2s_fs_sel = 0x1a;
	}
	else
	{
		printf("%s: not support enSample:%d\n", __FUNCTION__, enSample);
		ret = HI_FAILURE;
	}

	if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel))
	{
		printf("%s: set acodec sample rate failed\n", __FUNCTION__);
		ret = HI_FAILURE;
	}
	#if 0
	ioctl(fdAcodec, ACODEC_SET_PD_ADCL, &pd);
	ioctl(fdAcodec, ACODEC_SET_PD_ADCR, &pd);
	ioctl(fdAcodec, ACODEC_SET_PD_DACL, &pd);
	ioctl(fdAcodec, ACODEC_SET_PD_DACR, &pd);
	ioctl(fdAcodec, ACODEC_SET_PD_LINEINL, &pd);
	ioctl(fdAcodec, ACODEC_SET_PD_LINEINR, &pd);
	//ioctl(fdAcodec, ACODEC_SET_PD_LINEOUTD, &pd);

	mute = 0;
	if (ioctl(fdAcodec, ACODEC_SET_MICL_MUTE, &mute))
	{
		printf("%s: set acodec micinl mute failed\n", __FUNCTION__);
		return HI_FAILURE;
	}
	if (ioctl(fdAcodec, ACODEC_SET_MICR_MUTE, &mute))
	{
		printf("%s: set acodec micinr mute failed\n", __FUNCTION__);
		return HI_FAILURE;
	}
	#if 0
	if (ioctl(fdAcodec, ACODEC_SET_DACL_MUTE, &mute))
	{
		printf("%s: set acodec dacl mute failed\n", __FUNCTION__);
		return HI_FAILURE;
	}
	if (ioctl(fdAcodec, ACODEC_SET_DACR_MUTE, &mute))
	{
		printf("%s: set acodec dacr mute failed\n", __FUNCTION__);
		return HI_FAILURE;
	}
	if (ioctl(fdAcodec, ACODEC_SET_DACD_MUTE, &mute))
	{
		printf("%s: set acodec dacd mute failed\n", __FUNCTION__);
		return HI_FAILURE;
	}
	#endif
	#endif
	switch (1)//g_InnerCodecInput
	{
		case 0:
		{
			#if 1
			mixer_mic_ctrl = ACODEC_MIXER_MICIN;
			if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}
			#endif

			/* set volume plus (0~0x1f,default 0) */
			gain_mic = 0x10;
			if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
			{
				printf("%s: set acodec micin volume failed\n", __FUNCTION__);
				return HI_FAILURE;
			}
			if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
			{
				printf("%s: set acodec micin volume failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

		}
		break;
		case 1:
		{
			mixer_mic_ctrl = ACODEC_MIXER_LINEIN;
			if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}
			mute = 0;
			if (ioctl(fdAcodec, ACODEC_SET_MICL_MUTE, &mute))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}
			if (ioctl(fdAcodec, ACODEC_SET_MICR_MUTE, &mute))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

		}
		break;
		case 2:
		{
			mixer_mic_ctrl = ACODEC_MIXER_MICIN_D;
			if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

		}
		break;
		case 3:
		{
			mixer_mic_ctrl = ACODEC_MIXER_LINEIN_D;
			if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &mixer_mic_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

		}
		break;
		default:
		{
			printf("%s: acodec input mod wrong!\n", __FUNCTION__);
			return HI_FAILURE;
		}

	}

	switch (0)//g_InnerCodecOutput
	{
		case 0:
		{

			output_ctrl = ACODEC_LINEOUTD_NONE;
			if (ioctl(fdAcodec, ACODEC_SET_DAC_LINEOUTD, &output_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

			mute = 1;
			if (ioctl(fdAcodec, ACODEC_SET_DACD_MUTE, &mute))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

			//pd = 1;
			//ioctl(fdAcodec, ACODEC_SET_PD_LINEOUTD, &pd);

		}
		break;
		case 1:
		{
			output_ctrl = ACODEC_LINEOUTD_LEFT;
			if (ioctl(fdAcodec, ACODEC_SET_DAC_LINEOUTD, &output_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}
			mute = 0;
			if (ioctl(fdAcodec, ACODEC_SET_DACD_MUTE, &mute))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

			pd = 0;
			ioctl(fdAcodec, ACODEC_SET_PD_LINEOUTD, &pd);

		}
		break;
		case 2:
		{
			output_ctrl = ACODEC_LINEOUTD_RIGHT;
			if (ioctl(fdAcodec, ACODEC_SET_DAC_LINEOUTD, &output_ctrl))
			{
				printf("%s: set acodec micin failed\n", __FUNCTION__);
				return HI_FAILURE;
			}

		}
		break;
		default:
		{
			printf("%s: acodec input mod wrong!\n", __FUNCTION__);
			return HI_FAILURE;
		}

	}

	#if 0
		close(fdAcodec);
	#endif
	return ret;

}

/**
    function : Start Ai
    HI_S32 SAMPLE_COMM_AUDIO_StartAi => nf_HI_AUD_initAi
**/
HI_S32 nf_HI_AUD_initAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, 
						HI_BOOL bResampleEn, AI_VQE_CONFIG_S* pstAiVqeAttr, HI_U32 u32AiVqeType)
{
	HI_S32 i;
	HI_S32 s32Ret;

	if (pstAioAttr->u32ClkChnCnt == 0)
	{
		pstAioAttr->u32ClkChnCnt = pstAioAttr->u32ChnCnt;
	}

	s32Ret = HI_MPI_AI_SetPubAttr(AiDevId, pstAioAttr);
	if (s32Ret)
	{
		printf("%s: HI_MPI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
		return s32Ret;
	}

	s32Ret = HI_MPI_AI_Enable(AiDevId);
	if (s32Ret)
	{
		printf("%s: HI_MPI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
		return s32Ret;
	}

	for (i = 0; i < s32AiChnCnt; i++)
	{
		s32Ret = HI_MPI_AI_EnableChn(AiDevId, i);
		if (s32Ret)
		{
			printf("%s: HI_MPI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
			return s32Ret;
		}

		if (HI_TRUE == bResampleEn)
		{
			s32Ret = HI_MPI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
			if (s32Ret)
			{
				printf("%s: HI_MPI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
				return s32Ret;
			}
		}

		if (NULL != pstAiVqeAttr)
		{
			HI_BOOL bAiVqe = HI_TRUE;
			switch (u32AiVqeType)
			{
				case 0:
					s32Ret = HI_SUCCESS;
					bAiVqe = HI_FALSE;
					break;
				case 1:
					s32Ret = HI_MPI_AI_SetVqeAttr(AiDevId, i, SAMPLE_AUDIO_AO_DEV, i, (AI_VQE_CONFIG_S *)pstAiVqeAttr);
					break;
				default:
					s32Ret = HI_FAILURE;
					break;
			}
			if (s32Ret)
			{
				printf("%s: HI_MPI_AI_SetVqeAttr(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
				return s32Ret;
			}

			if (bAiVqe)
			{
				s32Ret = HI_MPI_AI_EnableVqe(AiDevId, i);
				if (s32Ret)
				{
					printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
					return s32Ret;
				}
			}
		}
	}

	return HI_SUCCESS;
}

/******************************************************************************
	function : Start Aenc
	SAMPLE_COMM_AUDIO_StartAenc -> nf_HI_AUD_initAenc
******************************************************************************/
HI_S32 nf_HI_AUD_initAenc(HI_S32 s32AencChnCnt, HI_U32 u32AencPtNumPerFrm, PAYLOAD_TYPE_E enType)
{
	AENC_CHN AeChn;
	HI_S32 s32Ret, i;
	AENC_CHN_ATTR_S stAencAttr;
	AENC_ATTR_ADPCM_S stAdpcmAenc;
	AENC_ATTR_G711_S stAencG711;
	AENC_ATTR_G726_S stAencG726;
	AENC_ATTR_LPCM_S stAencLpcm;

	/* set AENC chn attr */

	stAencAttr.enType = enType;
	stAencAttr.u32BufSize = 30;
	stAencAttr.u32PtNumPerFrm = u32AencPtNumPerFrm;

	if (PT_ADPCMA == stAencAttr.enType)
	{
		stAencAttr.pValue       = &stAdpcmAenc;
		stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
	}
	else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
	{
		stAencAttr.pValue       = &stAencG711;
	}
	else if (PT_G726 == stAencAttr.enType)
	{
		stAencAttr.pValue       = &stAencG726;
		stAencG726.enG726bps    = G726_BPS;
	}
	else if (PT_LPCM == stAencAttr.enType)
	{
		stAencAttr.pValue = &stAencLpcm;
	}
	else
	{
		printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAencAttr.enType);
		return HI_FAILURE;
	}

	for (i=0; i<s32AencChnCnt; i++)
	{
		AeChn = i;

		/* create aenc chn*/
		s32Ret = HI_MPI_AENC_CreateChn(AeChn, &stAencAttr);
		if (HI_SUCCESS != s32Ret)
		{
			printf("%s: HI_MPI_AENC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,
				   AeChn, s32Ret);
			return s32Ret;
		}
	}

	return HI_SUCCESS;
}

/**
	function : Aenc bind Ai Chn
**/
HI_S32 nf_HI_AUD_Bind_Aenc(AUDIO_DEV AiDev, HI_S32 s32AencChnCnt, HI_BOOL bUserGetMode )
{
	HI_S32 i, s32Ret;
	AI_CHN      AiChn;
	AENC_CHN    AeChn;

	for (i=0; i<s32AencChnCnt; i++)
	{
		AeChn = i;
		AiChn = i;

		if (HI_TRUE == bUserGetMode)
		{
			#if 0
				s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
			#else
				s32Ret = nf_HI_AUD_CreatTrdAiAenc(AiDev, AiChn, AeChn);
			#endif
			if (s32Ret != HI_SUCCESS)
			{
				NF_HI_AUD_SAMPLE_DBG(s32Ret);
				return HI_FAILURE;
			}
		}
		else
		{
			s32Ret = nf_HI_AUD_AencBindAi(AiDev, AiChn, AeChn);
			if (s32Ret != HI_SUCCESS)
			{
				NF_HI_AUD_SAMPLE_DBG(s32Ret);
				return s32Ret;
			}
		}
		printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
	}

	return HI_SUCCESS;
}

HI_BOOL nf_HI_aud_getFd(HI_S32 ps32AudFd[], HI_S32 *ps32MaxFd)
{
	HI_S32 i;
	int num_audio=0;

	num_audio=nf_live_get_cnt_audio_input();

	/* Prepare for all channel. */
	for ( i = 0; i < num_audio; i++ )
	{
		ps32AudFd[i] = HI_MPI_AENC_GetFd( i );

		if ( ps32AudFd[i] <= 0 )
		{
			g_warning("[%s][%d] FD[%d]", __FUNCTION__, __LINE__, ps32AudFd[i]);
			return HI_FALSE;
		}

		if ( *ps32MaxFd <= ps32AudFd[i] )
			*ps32MaxFd = ps32AudFd[i];

		HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] Aenc[%d] FD[%d]", __FUNCTION__, i, ps32AudFd[i]);
	}

	return HI_TRUE;
}

/**
	SAMPLE_COMM_AUDIO_StartAdec -> nf_HI_AUD_initAdec
**/
HI_BOOL nf_HI_AUD_initAdec( ADEC_CHN AdChn ,PAYLOAD_TYPE_E enType)
{
	HI_S32 s32Ret;
	ADEC_CHN_ATTR_S stAdecAttr;
	ADEC_ATTR_ADPCM_S stAdpcm;
	ADEC_ATTR_G711_S stAdecG711;
	ADEC_ATTR_G726_S stAdecG726;
	ADEC_ATTR_LPCM_S stAdecLpcm;

	stAdecAttr.enType = enType;
	stAdecAttr.u32BufSize = 20;
	stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */

	if (PT_ADPCMA == stAdecAttr.enType)
	{
		stAdecAttr.pValue = &stAdpcm;
		stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE ;
	}
	else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
	{
		stAdecAttr.pValue = &stAdecG711;
	}
	else if (PT_G726 == stAdecAttr.enType)
	{
		stAdecAttr.pValue = &stAdecG726;
		stAdecG726.enG726bps = G726_BPS ;
	}
	else if (PT_LPCM == stAdecAttr.enType)
	{
		stAdecAttr.pValue = &stAdecLpcm;
		stAdecAttr.enMode = ADEC_MODE_PACK;/* lpcm must use pack mode */
	}
	else
	{
		printf("%s: invalid aenc payload type:%d\n", __FUNCTION__, stAdecAttr.enType);
		return HI_FAILURE;
	}

	/* create adec chn*/
	s32Ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_ADEC_CreateChn(%d) failed with %#x!\n", __FUNCTION__,\
			   AdChn,s32Ret);
		return s32Ret;
	}
	return 0;
}

HI_BOOL nf_HI_AUD_startAdecAo(void)
{
	AUDIO_DEV AoDev = 0;
	AO_CHN AoChn = 0;
	ADEC_CHN AdChn = 0;
	HI_S32 ret;

	HI_DBG_AUD(DBG_MSG_AUD_CFG, "[%s] Adec[%d] - AO[%d][%d]", __FUNCTION__, AdChn, AoDev, AoChn);

	#if 0
		ret = HI_MPI_AO_BindAdec( AoDev, AoChn, AdChn );
	#else
		ret = nf_HI_AUD_AoBindAdec( AoDev, AoChn, AdChn );
	#endif

	if ( HI_SUCCESS != ret )
	{
		g_warning("HI_MPI_AO_BindAdec Err[%X]", ret);
		return HI_FALSE;
	}

	return HI_TRUE;
}

/**
	nf_HI_AUD_StartHdmi -> SAMPLE_COMM_AUDIO_StartHdmi

	error reason
	0xA00F8008 HI_ERR_VO_NOT_SUPPORT The operation is not supported.

**/
HI_S32 nf_HI_AUD_StartHdmi(AIO_ATTR_S *pstAioAttr)
{
	HI_S32 s32Ret;
	HI_HDMI_ATTR_S stHdmiAttr;
	HI_HDMI_ID_E enHdmi = HI_HDMI_ID_0;
	VO_PUB_ATTR_S stPubAttr;
	VO_DEV VoDev = 0;

	#if 0
		stPubAttr.u32BgColor = 0x000000ff;
		stPubAttr.enIntfType = VO_INTF_HDMI;
		#if 0
			stPubAttr.enIntfSync = VO_OUTPUT_1080P30;//VO_OUTPUT_1080P30/VO_OUTPUT_720P60
		#else
			stPubAttr.enIntfSync = VO_OUTPUT_3840x2160_30;
		#endif

		#if 0       // init to NFDVR/tools/mpp/sample/common_itx/hi_comm_vo.c
			if(HI_SUCCESS != SAMPLE_COMM_VO_StartDev(VoDev, &stPubAttr))
			{
				printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
				return HI_FAILURE;
			}
		#endif
	#else
		s32Ret=HI_MPI_VO_GetPubAttr(0, &stPubAttr);
		if(HI_SUCCESS != s32Ret)
		{
			printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
			return HI_FAILURE;
		}
	#endif

	s32Ret = nf_HI_VO_HdmiStart(stPubAttr.enIntfSync);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_HDMI_SetAVMute(enHdmi, HI_TRUE);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_HDMI_GetAttr(enHdmi, &stHdmiAttr);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	stHdmiAttr.bEnableAudio = HI_TRUE;        /**< if enable audio */
	stHdmiAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S; /**< source of HDMI audio, HI_HDMI_SND_INTERFACE_I2S suggested.the parameter mu     st be consistent with the input of AO*/
	stHdmiAttr.enSampleRate = pstAioAttr->enSamplerate;        /**< sampling rate of PCM audio,the parameter must be consistent with th     e input of AO */
	stHdmiAttr.u8DownSampleParm = HI_FALSE;    /**< parameter of downsampling  rate of PCM audio,default :0 */

	stHdmiAttr.enBitDepth = 8 * (pstAioAttr->enBitwidth+1);   /**< bitwidth of audio,default :16,the parameter must be consistent with      the config of AO */
	stHdmiAttr.u8I2SCtlVbit = 0;        /**< reserved,should be 0, I2S control (0x7A:0x1D) */

	stHdmiAttr.bEnableAviInfoFrame = HI_TRUE; /**< if enable  AVI InfoFrame*/
	stHdmiAttr.bEnableAudInfoFrame = HI_TRUE;; /**< if enable AUDIO InfoFrame*/

	s32Ret = HI_MPI_HDMI_Stop(enHdmi);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_HDMI_SetAttr(enHdmi, &stHdmiAttr);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_HDMI_Start(enHdmi);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_HDMI_SetAVMute(enHdmi, HI_FALSE);
	if(HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_StopHdmi -> 
	function : Stop Hdmi
******************************************************************************/
inline HI_S32 nf_HI_AUD_StopHdmi(HI_VOID)
{
	HI_S32 s32Ret;
	VO_DEV VoDev = 0;

	#if 0
		s32Ret =  SAMPLE_COMM_VO_HdmiStop();
	#else
		s32Ret =  nf_HI_AUD_VO_HdmiStop();
	#endif
	s32Ret |= HI_MPI_VO_Disable(VoDev);
	if(HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_VO_Disable failed with %#x!\n", __FUNCTION__, s32Ret);
		return HI_FAILURE;
	}

	return s32Ret;
}

/******************************************************************************
	SAMPLE_COMM_VO_HdmiStop -> nf_HI_AUD_VO_HdmiStop
	function : 
******************************************************************************/
HI_S32 nf_HI_AUD_VO_HdmiStop(HI_VOID)
{    
	HI_MPI_HDMI_Stop(HI_HDMI_ID_0);
	HI_MPI_HDMI_Close(HI_HDMI_ID_0);
	HI_MPI_HDMI_DeInit();

	return HI_SUCCESS;
}

/**
    SAMPLE_COMM_AUDIO_StartAo -> nf_HI_AUD_initAo
**/
HI_S32 nf_HI_AUD_initAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, 
						HI_BOOL bResampleEn, HI_VOID* pstAoVqeAttr, HI_U32 u32AoVqeType)
{
	HI_S32 i;
	HI_S32 s32Ret;
	static gboolean is_hdmi_start=FALSE;

	if (SAMPLE_AUDIO_HDMI_AO_DEV == AoDevId)
	{
		#if 0
			#ifdef HI_ACODEC_TYPE_HDMI 

			pstAioAttr->u32ClkSel = 0;
	
			SAMPLE_COMM_AUDIO_StartHdmi(pstAioAttr);

			#endif
		#else
			if(is_hdmi_start == FALSE)
			{
				pstAioAttr->u32ClkSel = 0;
	
				nf_HI_AUD_StartHdmi(pstAioAttr);

				is_hdmi_start = TRUE;
			}
		#endif
	}

	if (pstAioAttr->u32ClkChnCnt == 0)
	{
		pstAioAttr->u32ClkChnCnt = pstAioAttr->u32ChnCnt;
	}

	s32Ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_AO_SetPubAttr(%d) failed with %#x!\n", __FUNCTION__, \
			   AoDevId, s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_AO_Enable(AoDevId);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_AO_Enable(%d) failed with %#x!\n", __FUNCTION__, AoDevId, s32Ret);
		return HI_FAILURE;
	}

	for (i = 0; i < s32AoChnCnt; i++)
	{
		s32Ret = HI_MPI_AO_EnableChn(AoDevId, i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("%s: HI_MPI_AO_EnableChn(%d) failed with %#x!\n", __FUNCTION__, i, s32Ret);
			return HI_FAILURE;
		}

		if (HI_TRUE == bResampleEn)
		{
			s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
			s32Ret |= HI_MPI_AO_EnableReSmp(AoDevId, i, enInSampleRate);
			if (HI_SUCCESS != s32Ret)
			{
				printf("%s: HI_MPI_AO_EnableReSmp(%d,%d) failed with %#x!\n", __FUNCTION__, AoDevId, i, s32Ret);
				return HI_FAILURE;
			}
		}

		if (NULL != pstAoVqeAttr)
		{
			HI_BOOL bAoVqe = HI_TRUE;
			switch (u32AoVqeType)
			{
				case 0:
					s32Ret = HI_SUCCESS;
					bAoVqe = HI_FALSE;
					break;
				case 1:
					s32Ret = HI_MPI_AO_SetVqeAttr(AoDevId, i, (AO_VQE_CONFIG_S *)pstAoVqeAttr);
					break;
				default:
					s32Ret = HI_FAILURE;
					break;
			}

			if (s32Ret)
			{
				printf("%s: SetAoVqe%d(%d,%d) failed with %#x\n", __FUNCTION__, u32AoVqeType, AoDevId, i, s32Ret);
				return s32Ret;
			}

			if (bAoVqe)
			{
				s32Ret = HI_MPI_AO_EnableVqe(AoDevId, i);
				if (s32Ret)
				{
					printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AoDevId, i, s32Ret);
					return s32Ret;
				}
			}
		}
	}

	return HI_SUCCESS;
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_CreatTrdAiAenc -> nf_HI_AUD_CreatTrdAiAenc
	function : Create the thread to get frame from ai and send to aenc
******************************************************************************/
static SAMPLE_AI_S gs_stSampleAi[AI_DEV_MAX_NUM*AIO_MAX_CHN_NUM];
HI_S32 nf_HI_AUD_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
	SAMPLE_AI_S *pstAi = NULL;

	pstAi = &gs_stSampleAi[AiDev*AIO_MAX_CHN_NUM + AiChn];
	pstAi->bSendAenc = HI_TRUE;
	pstAi->bSendAo = HI_FALSE;
	pstAi->bStart= HI_TRUE;
	pstAi->AiDev = AiDev;
	pstAi->AiChn = AiChn;
	pstAi->AencChn = AeChn;
	pthread_create(&pstAi->stAiPid, 0, nf_HI_COMM_AUDIO_AiProc, pstAi);

	return HI_SUCCESS;
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_AencBindAi -> nf_HI_AUD_AencBindAi
	function : Aenc bind Ai
******************************************************************************/
HI_S32 nf_HI_AUD_AencBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
	MPP_CHN_S stSrcChn,stDestChn;

	stSrcChn.enModId = HI_ID_AI;
	stSrcChn.s32DevId = AiDev;
	stSrcChn.s32ChnId = AiChn;
	stDestChn.enModId = HI_ID_AENC;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = AeChn;

	return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_AoBindAdec -> nf_HI_AUD_AoBindAdec
	function : Ao bind Adec
******************************************************************************/
HI_S32 nf_HI_AUD_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
	MPP_CHN_S stSrcChn,stDestChn;

	stSrcChn.enModId = HI_ID_ADEC;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = AdChn;
	stDestChn.enModId = HI_ID_AO;
	stDestChn.s32DevId = AoDev;
	stDestChn.s32ChnId = AoChn;

	#if 0       // For Debug
		printf("enMoId[%d] devid[%d] chnid[%d]\n", stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId);
		printf("enMoId[%d] devid[%d] chnid[%d]\n", stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId);
		printf("Aodev %d AoChn %d AdChn %d\n", AoDev, AoChn, AdChn);
	#endif

	return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_AoBindAi -> nf_HI_aud_AoBindAi
	function : Ao bind Ai
******************************************************************************/
HI_S32 nf_HI_AUD_AoBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
	MPP_CHN_S stSrcChn,stDestChn;

	stSrcChn.enModId = HI_ID_AI;
	stSrcChn.s32ChnId = AiChn;
	stSrcChn.s32DevId = AiDev;
	stDestChn.enModId = HI_ID_AO;
	stDestChn.s32DevId = AoDev;
	stDestChn.s32ChnId = AoChn;

	return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_AoUnbindAdec -> nf_HI_aud_AoUnBindAdec
	function : Ao unbind Ai
******************************************************************************/
HI_S32 nf_HI_AUD_AoUnBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
	MPP_CHN_S stSrcChn,stDestChn;

	stSrcChn.enModId = HI_ID_ADEC;
	stSrcChn.s32ChnId = AdChn;
	stSrcChn.s32DevId = 0;
	stDestChn.enModId = HI_ID_AO;
	stDestChn.s32DevId = AoDev;
	stDestChn.s32ChnId = AoChn;

	return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_AoUnbindAi -> nf_HI_AUD_AoUnbindAi
	function : Ao unbind Ai
******************************************************************************/
HI_S32 nf_HI_AUD_AoUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AUDIO_DEV AoDev, AO_CHN AoChn)
{
	MPP_CHN_S stSrcChn,stDestChn;

	stSrcChn.enModId = HI_ID_AI;
	stSrcChn.s32ChnId = AiChn;
	stSrcChn.s32DevId = AiDev;
	stDestChn.enModId = HI_ID_AO;
	stDestChn.s32DevId = AoDev;
	stDestChn.s32ChnId = AoChn;

	return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_AiProc -> nf_HI_COMM_AUDIO_AiProc
	function : get frame from Ai, send it  to Aenc or Ao
******************************************************************************/
static void *nf_HI_COMM_AUDIO_AiProc(void *parg)
{
	// Not Wroking!!
	// This is Hisilicon Sample Function
	return NULL;
}

/******************************************************************************
	SAMPLE_COMM_AUDIO_StopAi -> nf_HI_AUD_StopAi
	function : Stop Ai
******************************************************************************/
HI_S32 nf_HI_AUD_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, HI_BOOL bResampleEn, HI_BOOL bVqeEn)
{
	HI_S32 i;
	HI_S32 s32Ret;

	for (i = 0; i < s32AiChnCnt; i++)
	{
		if (HI_TRUE == bResampleEn)
		{
			s32Ret = HI_MPI_AI_DisableReSmp(AiDevId, i);
			if (HI_SUCCESS != s32Ret)
			{
				printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
				return s32Ret;
			}
		}

		if (HI_TRUE == bVqeEn)
		{
			s32Ret = HI_MPI_AI_DisableVqe(AiDevId, i);
			if (HI_SUCCESS != s32Ret)
			{
				printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
				return s32Ret;
			}
		}

		s32Ret = HI_MPI_AI_DisableChn(AiDevId, i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
			return s32Ret;
		}
	}

	s32Ret = HI_MPI_AI_Disable(AiDevId);
	if (HI_SUCCESS != s32Ret)
	{
		printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
		return s32Ret;
	}

	return HI_SUCCESS;
}
/**
	function : Stop AiAo
	AI is always enabled.. So only ao disable!!
	SAMPLE_COMM_AUDIO_StopAo -> nf_HI_AUD_stopAo
**/
HI_BOOL nf_HI_AUD_stopAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn, HI_BOOL bVqeEn)
{
	HI_S32 i;
	HI_S32 s32Ret;

	for (i = 0; i < s32AoChnCnt; i++)
	{
		if (HI_TRUE == bResampleEn)
		{
			s32Ret = HI_MPI_AO_DisableReSmp(AoDevId, i);
			if (HI_SUCCESS != s32Ret)
			{
				printf("%s: HI_MPI_AO_DisableReSmp failed with %#x!\n", __FUNCTION__, s32Ret);
				return s32Ret;
			}
		}

		if (HI_TRUE == bVqeEn)
		{
			s32Ret = HI_MPI_AO_DisableVqe(AoDevId, i);
			if (HI_SUCCESS != s32Ret)
			{
				printf("[Func]:%s [Line]:%d [Info]:%s\n", __FUNCTION__, __LINE__, "failed");
				return s32Ret;
			}
		}

		s32Ret = HI_MPI_AO_DisableChn(AoDevId, i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("%s: HI_MPI_AO_DisableChn failed with %#x!\n", __FUNCTION__, s32Ret);
			return s32Ret;
		}
	}

	s32Ret = HI_MPI_AO_Disable(AoDevId);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s: HI_MPI_AO_Disable failed with %#x!\n", __FUNCTION__, s32Ret);
		return s32Ret;
	}

	#if 0
		if (SAMPLE_AUDIO_HDMI_AO_DEV == AoDevId)
		{
			#ifdef HI_ACODEC_TYPE_HDMI
			s32Ret = SAMPLE_COMM_AUDIO_StopHdmi();
			if (HI_SUCCESS != s32Ret)
			{
				printf("%s: SAMPLE_COMM_AUDIO_StopHdmi failed with %#x!\n", __FUNCTION__, s32Ret);
				return s32Ret;
			}

			#endif
		}
	#endif

	return HI_SUCCESS;
}
