#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#if 0

#include "nf_HI_aud.h"
#include "acodec.h"
#include "nf_qc_audio.h"
#include "hi_comm_aio.h"
#include "nf_qc_main.h"
#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)


typedef struct tagSAMPLE_AENC_S
{
	HI_BOOL bStart;
	pthread_t stAencPid;
	HI_S32  AeChn;
	HI_S32  AdChn;
	FILE    *pfd;
	HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;

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

typedef struct tagSAMPLE_ADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn;
    FILE* pfd;
    pthread_t stAdPid;
} SAMPLE_ADEC_S;

typedef struct hiAI_VQE_CONFIG_S
{
    HI_S32              bHpfOpen;
	HI_S32              bAecOpen;     
    HI_S32              bAnrOpen;
    HI_S32              bRnrOpen;
    HI_S32              bAgcOpen;
    HI_S32              bEqOpen;
    HI_S32              bHdrOpen;

    HI_S32              s32WorkSampleRate;  /* Sample Rate£º8KHz/16KHz¡£default: 8KHz*/
    HI_S32              s32FrameSample; /* VQE frame length£º 80-4096 */
    VQE_WORKSTATE_E     enWorkstate;

                                       
    AUDIO_HPF_CONFIG_S  stHpfCfg;
 	AI_AEC_CONFIG_S     stAecCfg;
    AUDIO_ANR_CONFIG_S  stAnrCfg;
    AI_RNR_CONFIG_S     stRnrCfg;
    AUDIO_AGC_CONFIG_S  stAgcCfg;  
    AUDIO_EQ_CONFIG_S   stEqCfg;
    AI_HDR_CONFIG_S     stHdrCfg;
} AI_VQE_CONFIG_S;

typedef struct tagSAMPLE_AO_S
{
    AUDIO_DEV AoDev;
    HI_BOOL bStart;
    pthread_t stAoPid;
} SAMPLE_AO_S;

static SAMPLE_AI_S   gs_stSampleAi[AI_DEV_MAX_NUM* AIO_MAX_CHN_NUM];
static SAMPLE_AENC_S gs_stSampleAenc[AENC_MAX_CHN_NUM];
static SAMPLE_ADEC_S gs_stSampleAdec[ADEC_MAX_CHN_NUM];
static SAMPLE_AO_S   gs_stSampleAo[AO_DEV_MAX_NUM];

//static HI_BOOL gs_bMicIn = HI_FALSE;
static HI_BOOL gs_bAioReSample  = HI_FALSE;
static HI_BOOL gs_bUserGetMode  = HI_FALSE;
static HI_BOOL gs_bAoVolumeCtrl = HI_FALSE;
static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_U32 u32AencPtNumPerFrm = 0;
/* 0: close, 1: Vqe, 2: hifi*/
static HI_U32 u32AiVqeType = 1;  
/* 0: close, 1: open*/
static HI_U32 u32AoVqeType = 0;
static PAYLOAD_TYPE_E gs_enPayloadType = PT_LPCM;;

HI_S32 QC_SAMPLE_INNER_CODEC_CfgAudio(AUDIO_SAMPLE_RATE_E enSample)
{
	HI_S32 fdAcodec = -1;
	guint i2s_fs_sel=0, mixer_mic_ctrl=0, gain_mic=0;

	fdAcodec = open(ACODEC_FILE,O_RDWR);
	if (fdAcodec < 0)
	{
		printf("%s: can't open acodec,%s\n", __FUNCTION__, ACODEC_FILE);
		return HI_FAILURE;
	}

	if(ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL))
	{
		printf("Reset audio codec error\n");
		close(fdAcodec);
		return HI_FAILURE;
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
		close(fdAcodec);
		return HI_FAILURE;
	}

	//if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel))
	if (ioctl(fdAcodec, 2, &i2s_fs_sel))
	{
		printf("%s: set acodec sample rate failed\n", __FUNCTION__);
		close(fdAcodec);
		return HI_FAILURE;
	}
	close(fdAcodec);

	return HI_SUCCESS;
}

HI_S32 QC_SAMPLE_COMM_AUDIO_CfgAcodec(AIO_ATTR_S* pstAioAttr)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_BOOL bCodecCfg = HI_FALSE;

    /*** INNER AUDIO CODEC ***/
    s32Ret = QC_SAMPLE_INNER_CODEC_CfgAudio(pstAioAttr->enSamplerate);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s:SAMPLE_INNER_CODEC_CfgAudio failed\n", __FUNCTION__);
        return s32Ret;
    }
    bCodecCfg = HI_TRUE;

    if (!bCodecCfg)
    {
        printf("Can not find the right codec.\n");
        return HI_FALSE;
    }
    return HI_SUCCESS;
}

HI_S32 QC_SAMPLE_COMM_AUDIO_StartAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
                                 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enOutSampleRate, HI_BOOL bResampleEn, HI_VOID* pstAiVqeAttr, HI_U32 u32AiVqeType)
{
    HI_S32 i;
    HI_S32 s32Ret;

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
        s32Ret = HI_MPI_AI_EnableChn(AiDevId, i/(pstAioAttr->enSoundmode + 1));
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
                    //s32Ret = HI_MPI_AI_SetVqeAttr(AiDevId, i, SAMPLE_AUDIO_AO_DEV, i, (AI_VQE_CONFIG_S *)pstAiVqeAttr);
                    break;
			/*
                case 2:
                    s32Ret = HI_MPI_AI_SetHiFiVqeAttr(AiDevId, i, (AI_HIFIVQE_CONFIG_S *)pstAiVqeAttr);
                    break;
                 */
                default:
                    s32Ret = HI_FAILURE;
                    break;
            }
            
            if (s32Ret)
            {
                printf("%s: SetAiVqe%d(%d,%d) failed with %#x\n", __FUNCTION__, u32AiVqeType, AiDevId, i, s32Ret);
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
void* QC_SAMPLE_COMM_AUDIO_AiProc(void* parg)
{
    HI_S32 s32Ret;
    HI_S32 AiFd;
    SAMPLE_AI_S* pstAiCtl = (SAMPLE_AI_S*)parg;
    AUDIO_FRAME_S stFrame;
    AEC_FRAME_S   stAecFrm;
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;
#if 0
    FILE *pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];
    /* create file for save stream*/        
    snprintf(aszFileName, FILE_NAME_LEN, "ai%d_chn%d.pcm", pstAiCtl->AiDev, pstAiCtl->AiChn);
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open pcm file:\"%s\" for ai ok\n", aszFileName);
#endif    
    s32Ret = HI_MPI_AI_GetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: Get ai chn param failed\n", __FUNCTION__);
        return NULL;
    }

    stAiChnPara.u32UsrFrmDepth = 30;

    s32Ret = HI_MPI_AI_SetChnParam(pstAiCtl->AiDev, pstAiCtl->AiChn, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: set ai chn param failed\n", __FUNCTION__);
        return NULL;
    }

    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(pstAiCtl->AiDev, pstAiCtl->AiChn);
    FD_SET(AiFd, &read_fds);

    while (pstAiCtl->bStart)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AiFd, &read_fds);

        s32Ret = select(AiFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            printf("%s: get ai frame select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AiFd, &read_fds))
        {
            /* get frame from ai chn */
            memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));
            s32Ret = HI_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
#if 0
                printf("%s: HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n", \
                       __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
#else
                continue;
#endif
            }
#if 0
            fwrite(stFrame.pVirAddr[0], 1, stFrame.u32Len, pfd);
#endif
            /* send frame to encoder */
            if (HI_TRUE == pstAiCtl->bSendAenc)
            {
                s32Ret = HI_MPI_AENC_SendFrame(pstAiCtl->AencChn, &stFrame, &stAecFrm);
                if (HI_SUCCESS != s32Ret )
                {
                    printf("%s: HI_MPI_AENC_SendFrame(%d), failed with %#x!\n", \
                           __FUNCTION__, pstAiCtl->AencChn, s32Ret);
                    pstAiCtl->bStart = HI_FALSE;
                    return NULL;
                }
            }

            /* send frame to ao */
            if (HI_TRUE == pstAiCtl->bSendAo)
            {
                s32Ret = HI_MPI_AO_SendFrame(pstAiCtl->AoDev, pstAiCtl->AoChn, &stFrame, 1000);
                if (HI_SUCCESS != s32Ret )
                {
                    printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!\n", \
                           __FUNCTION__, pstAiCtl->AoDev, pstAiCtl->AoChn, s32Ret);
                    pstAiCtl->bStart = HI_FALSE;
                    return NULL;
                }

            }

            /* finally you must release the stream */
            s32Ret = HI_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn, &stFrame, &stAecFrm);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n", \
                       __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
            }

        }
    }

    pstAiCtl->bStart = HI_FALSE;
    return NULL;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_StartAenc(HI_S32 s32AencChnCnt, HI_U32 u32AencPtNumPerFrm, PAYLOAD_TYPE_E enType)
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

    for (i = 0; i < s32AencChnCnt; i++)
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

/******************************************************************************
* function : Create the thread to get frame from ai and send to aenc
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    SAMPLE_AI_S* pstAi = NULL;

    pstAi = &gs_stSampleAi[AiDev * AIO_MAX_CHN_NUM + AiChn];
    pstAi->bSendAenc = HI_TRUE;
    pstAi->bSendAo = HI_FALSE;
    pstAi->bStart = HI_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
    pstAi->AencChn = AeChn;
    pthread_create(&pstAi->stAiPid, 0, QC_SAMPLE_COMM_AUDIO_AiProc, pstAi);

    return HI_SUCCESS;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_AencBindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

static char* QC_SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)
    {
        return "g711a";
    }
    else if (PT_G711U == enType)
    {
        return "g711u";
    }
    else if (PT_ADPCMA == enType)
    {
        return "adpcm";
    }
    else if (PT_G726 == enType)
    {
        return "g726";
    }
    else if (PT_LPCM == enType)
    {
        return "pcm";
    }
    else
    {
        return "data";
    }
}

static FILE* QC_SAMPLE_AUDIO_OpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
#if 0
    snprintf(aszFileName, FILE_NAME_LEN, "audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
#else
    snprintf(aszFileName, FILE_NAME_LEN, "/tmp/IN%d.%s", AeChn, QC_SAMPLE_AUDIO_Pt2Str(enType));
#endif
    pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}
void* QC_SAMPLE_COMM_AUDIO_AencProc(void* parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S* pstAencCtl = (SAMPLE_AENC_S*)parg;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;

    FD_ZERO(&read_fds);
    AencFd = HI_MPI_AENC_GetFd(pstAencCtl->AeChn);
    FD_SET(AencFd, &read_fds);

    while (pstAencCtl->bStart)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            printf("%s: get aenc stream select time out\n", __FUNCTION__);
            break;
        }

        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            s32Ret = HI_MPI_AENC_GetStream(pstAencCtl->AeChn, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n", \
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }

            /* send stream to decoder and play for testing */
            if (HI_TRUE == pstAencCtl->bSendAdChn)
            {
                s32Ret = HI_MPI_ADEC_SendStream(pstAencCtl->AdChn, &stStream, HI_TRUE);
                if (HI_SUCCESS != s32Ret )
                {
                    printf("%s: HI_MPI_ADEC_SendStream(%d), failed with %#x!\n", \
                           __FUNCTION__, pstAencCtl->AdChn, s32Ret);
                    pstAencCtl->bStart = HI_FALSE;
                    return NULL;
                }
            }

            /* save audio stream to file */
            fwrite(stStream.pStream, 1, stStream.u32Len, pstAencCtl->pfd);

            fflush(pstAencCtl->pfd);

            /* finally you must release the stream */
            s32Ret = HI_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n", \
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }
        }
    }

    fclose(pstAencCtl->pfd);
    pstAencCtl->bStart = HI_FALSE;
    return NULL;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_CreatTrdAencAdec(AENC_CHN AeChn, ADEC_CHN AdChn, FILE* pAecFd)
{
    SAMPLE_AENC_S* pstAenc = NULL;

    if (NULL == pAecFd)
    {
        return HI_FAILURE;
    }

    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;
#if 0
    pstAenc->AdChn = AdChn;
    pstAenc->bSendAdChn = HI_TRUE;
#else
    pstAenc->AdChn = NULL;
    pstAenc->bSendAdChn = HI_FALSE;
#endif
    pstAenc->pfd = pAecFd;
    pstAenc->bStart = HI_TRUE;
    pthread_create(&pstAenc->stAencPid, 0, QC_SAMPLE_COMM_AUDIO_AencProc, pstAenc);

    return HI_SUCCESS;
}
/******************************************************************************
* function : Start Adec
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_StartAdec(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
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
        printf("%s: HI_MPI_ADEC_CreateChn(%d) failed with %#x!\n", __FUNCTION__, \
               AdChn, s32Ret);
        return s32Ret;
    }
    return 0;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_StartAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt,
                                 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, HI_BOOL bResampleEn, HI_VOID* pstAoVqeAttr, HI_U32 u32AoVqeType)
{
    HI_S32 i;
    HI_S32 s32Ret;

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
        s32Ret = HI_MPI_AO_EnableChn(AoDevId, i/(pstAioAttr->enSoundmode + 1));
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
* function : Ao bind Adec
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_AoBindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}
/******************************************************************************
* function : Open Adec File
******************************************************************************/
static FILE* QC_SAMPLE_AUDIO_OpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
#if 0
    snprintf(aszFileName, FILE_NAME_LEN ,"audio_chn%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
#else
    snprintf(aszFileName, FILE_NAME_LEN ,"/NFDVR/data/OUT%d.%s", AdChn, QC_SAMPLE_AUDIO_Pt2Str(enType));
#endif
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}
void* QC_SAMPLE_COMM_AUDIO_AdecProc(void* parg)
{
    HI_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn;
    HI_U8* pu8AudioStream = NULL;
    SAMPLE_ADEC_S* pstAdecCtl = (SAMPLE_ADEC_S*)parg;
    FILE* pfd = pstAdecCtl->pfd;
    s32AdecChn = pstAdecCtl->AdChn;

    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8) * MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        printf("%s: malloc failed!\n", __FUNCTION__);
        return NULL;
    }

    while (HI_TRUE == pstAdecCtl->bStart)
    {
        /* read from file */
        stAudioStream.pStream = pu8AudioStream;
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
        if (u32ReadLen <= 0)
        {
            s32Ret = HI_MPI_ADEC_SendEndOfStream(s32AdecChn, HI_FALSE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: HI_MPI_ADEC_SendEndOfStream failed!\n", __FUNCTION__);
            }
            fseek(pfd, 0, SEEK_SET);/*read file again*/
            continue;
        }

        /* here only demo adec streaming sending mode, but pack sending mode is commended */
        stAudioStream.u32Len = u32ReadLen;
        s32Ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!\n", \
                   __FUNCTION__, s32AdecChn, s32Ret);
            break;
        }
    }

    free(pu8AudioStream);
    pu8AudioStream = NULL;
    fclose(pfd);
    pstAdecCtl->bStart = HI_FALSE;
    return NULL;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_CreatTrdFileAdec(ADEC_CHN AdChn, FILE* pAdcFd)
{
    SAMPLE_ADEC_S* pstAdec = NULL;

    if (NULL == pAdcFd)
    {
        return HI_FAILURE;
    }

    pstAdec = &gs_stSampleAdec[AdChn];
    pstAdec->AdChn = AdChn;
    pstAdec->pfd = pAdcFd;
    pstAdec->bStart = HI_TRUE;
    pthread_create(&pstAdec->stAdPid, 0, QC_SAMPLE_COMM_AUDIO_AdecProc, pstAdec);

    return HI_SUCCESS;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(ADEC_CHN AdChn)
{
    SAMPLE_ADEC_S* pstAdec = NULL;

    pstAdec = &gs_stSampleAdec[AdChn];
    if (pstAdec->bStart)
    {
        pstAdec->bStart = HI_FALSE;
        //pthread_cancel(pstAdec->stAdPid);
        pthread_join(pstAdec->stAdPid, 0);
    }
    return HI_SUCCESS;
}
/******************************************************************************
* function : Stop Adec
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_StopAdec(ADEC_CHN AdChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_ADEC_DestroyChn(AdChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: HI_MPI_ADEC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
               AdChn, s32Ret);
        return s32Ret;
    }
    return HI_SUCCESS;
}
/******************************************************************************
* function : Stop Ao
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_StopAo(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn, HI_BOOL bVqeEn)
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

    return HI_SUCCESS;
}
/******************************************************************************
* function : Ao unbind Adec
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_AoUnbindAdec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = AdChn;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}
/******************************************************************************
* function : Aenc unbind Ai
******************************************************************************/
HI_S32 QC_SAMPLE_COMM_AUDIO_AencUnbindAi(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}
HI_S32 QC_SAMPLE_COMM_AUDIO_DestoryTrdAi(AUDIO_DEV AiDev, AI_CHN AiChn)
{
    SAMPLE_AI_S* pstAi = NULL;

    pstAi = &gs_stSampleAi[AiDev * AIO_MAX_CHN_NUM + AiChn];
    if (pstAi->bStart)
    {
        pstAi->bStart = HI_FALSE;
        //pthread_cancel(pstAi->stAiPid);
        pthread_join(pstAi->stAiPid, 0);
    }


    return HI_SUCCESS;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_StopAenc(HI_S32 s32AencChnCnt)
{
    HI_S32 i;
    HI_S32 s32Ret;

    for (i = 0; i < s32AencChnCnt; i++)
    {
        s32Ret = HI_MPI_AENC_DestroyChn(i);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: HI_MPI_AENC_DestroyChn(%d) failed with %#x!\n", __FUNCTION__,
                   i, s32Ret);
            return s32Ret;
        }

    }

    return HI_SUCCESS;
}
HI_S32 SAMPLE_AUDIO_QC(void)
{
	HI_S32 i, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
    HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    HI_BOOL     bSendAdec = HI_TRUE;
    FILE*        pfd = NULL;
    AIO_ATTR_S stAioAttr;
	
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
	
    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
    u32AencPtNumPerFrm = stAioAttr.u32PtNumPerFrm;

	 /********************************************
      step 1: config audio codec
    ********************************************/
    printf("\e[31m >> [%s] step 1: config audio codec!!!!!!!!!!!!!!!!!!\e[0m\n", __FUNCTION__);
    s32Ret = QC_SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	/********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32AencChnCnt = s32AiChnCnt;
    //s32Ret = QC_SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, NULL, 0);
    s32Ret = QC_SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	/********************************************
      step 3: start Aenc
    ********************************************/
    //s32Ret = QC_SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, SAMPLE_AUDIO_PTNUMPERFRM, gs_enPayloadType);
    s32Ret = QC_SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, u32AencPtNumPerFrm, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	 /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;
	
        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = QC_SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {
            s32Ret = QC_SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return s32Ret;
            }
        }
        pfd = QC_SAMPLE_AUDIO_OpenAencFile(AeChn, gs_enPayloadType);
        if (!pfd)
        {
            SAMPLE_DBG(HI_FAILURE);
            return HI_FAILURE;
        }
        s32Ret = QC_SAMPLE_COMM_AUDIO_CreatTrdAencAdec(AeChn, AdChn, pfd);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
		printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }
	/********************************************
      step 5: start Adec & Ao.
    ********************************************/
    //if(1)
    if (HI_TRUE == bSendAdec)
    {
        s32Ret = QC_SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
		
		s32AoChnCnt = stAioAttr.u32ChnCnt;
		//s32Ret = QC_SAMPLE_COMM_AUDIO_StartAo(AoDev, stAioAttr.u32ChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE);
		 s32Ret = QC_SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL , 0);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}
		s32Ret = QC_SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != HI_SUCCESS)
       	{
       		SAMPLE_DBG(s32Ret);
      		return HI_FAILURE;
  	    }
		pfd = QC_SAMPLE_AUDIO_OpenAdecFile(AdChn, gs_enPayloadType);
		if (!pfd)
		{
			SAMPLE_DBG(HI_FAILURE);
			return HI_FAILURE;
		}
		s32Ret = QC_SAMPLE_COMM_AUDIO_CreatTrdFileAdec(AdChn, pfd);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}
        printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);
    }

    sleep(5);
	/********************************************
      step 6: exit the process
    ********************************************/
    if (HI_TRUE == bSendAdec)
    {
		s32Ret = QC_SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(AdChn);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}
		s32Ret = QC_SAMPLE_COMM_AUDIO_StopAdec(AdChn);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}
		s32Ret = QC_SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}

		s32Ret = QC_SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}
	}
    for (i = 0; i < s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = QC_SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {
            s32Ret = QC_SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
	//	SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(AeChn);
    }
		#if 1
    s32Ret = QC_SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    s32Ret = QC_SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }
	#endif
    return HI_SUCCESS;
}
HI_S32 QC_SAMPLE_COMM_AUDIO_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
                                HI_BOOL bResampleEn, HI_BOOL bVqeEn)
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
int nf_HI_aud_qc_test(int flag)
{
    HI_S32 s32Ret = HI_SUCCESS;
	/*
	if (flag == 0) {
		s32Ret = HI_MPI_SYS_Init();
		if (HI_SUCCESS != s32Ret)
		{
			g_warning("%s: HI_MPI_SYS_Init failed with %d!", __FUNCTION__, s32Ret);
			return HI_FAILURE;
		}
		else
			g_message("HI_MPI_SYS_Init Success!!");
	}
	*/
	
    s32Ret = SAMPLE_AUDIO_QC();
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: failed with %d!\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
#endif

int nf_HI_aud_qc_test(int flag)
{
	return -1;
}
