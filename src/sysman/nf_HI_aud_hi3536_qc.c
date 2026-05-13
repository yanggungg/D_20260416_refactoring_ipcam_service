#include "nf_HI_aud_3536_qc.h"
#include "nf_HI_vo.h"
#include "nf_util_device.h"
#include "nf_api_live.h"
#include "nf_network.h"
#include "nf_qc_audio.h"
#include "nf_sysman.h"

#include <signal.h>
#include <math.h>

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_40K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

float	QC_Sine[] = {
	0.000000, 0.024541, 0.049068, 0.073565,	0.098017, 0.122411,
	0.146730, 0.170962, 0.195090, 0.219101, 0.242980, 0.266713,
	0.290285, 0.313682, 0.336890, 0.359895, 0.382683, 0.405241,
	0.427555, 0.449611, 0.471397, 0.492898, 0.514103, 0.534998,
	0.555570, 0.575808, 0.595699, 0.615232,	0.634393, 0.653173,
	0.671559, 0.689541, 0.707107, 0.724247, 0.740951, 0.757209,
	0.773010, 0.788346, 0.803208, 0.817585, 0.831470, 0.844854,
	0.857729, 0.870087, 0.881921, 0.893224, 0.903989, 0.914210,
	0.923880, 0.932993, 0.941544, 0.949528,	0.956940, 0.963776,
	0.970031, 0.975702, 0.980785, 0.985278, 0.989177, 0.992480,
	0.995185, 0.997290, 0.998795, 0.999699,	1.000000, 0.999699,
	0.998795, 0.997290, 0.995185, 0.992480, 0.989177, 0.985278,
	0.980785, 0.975702, 0.970031, 0.963776,	0.956940, 0.949528,
	0.941544, 0.932993, 0.923880, 0.914210, 0.903989, 0.893224,
	0.881921, 0.870087, 0.857729, 0.844854,	0.831470, 0.817585,
	0.803208, 0.788346, 0.773010, 0.757209, 0.740951, 0.724247,
	0.707107, 0.689541, 0.671559, 0.653173,	0.634393, 0.615232,
	0.595699, 0.575808, 0.555570, 0.534998, 0.514103, 0.492898,
	0.471397, 0.449611, 0.427555, 0.405241, 0.382683, 0.359895,
	0.336890, 0.313682, 0.290285, 0.266713, 0.242980, 0.219101,
	0.195090, 0.170962, 0.146730, 0.122411,	0.098017, 0.073565,
	0.049068, 0.024541, 0.000000, -0.024541, -0.049068, -0.073565,
	-0.098017, -0.122411, -0.146730, -0.170962, -0.195090, -0.219101,
	-0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895,
	-0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898,
	-0.514103, -0.534998, -0.555570, -0.575808, -0.595699, -0.615232,
	-0.634393, -0.653173, -0.671559, -0.689541, -0.707107, -0.724247,
	-0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585,
	-0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224,
	-0.903989, -0.914210, -0.923880, -0.932993, -0.941544, -0.949528,
	-0.956940, -0.963776, -0.970031, -0.975702, -0.980785, -0.985278,
	-0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699,
	-1.000000, -0.999699, -0.998795, -0.997290, -0.995185, -0.992480,
	-0.989177, -0.985278, -0.980785, -0.975702, -0.970031, -0.963776,
	-0.956940, -0.949528, -0.941544, -0.932993, -0.923880, -0.914210,
	-0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844854,
	-0.831470, -0.817585, -0.803208, -0.788346, -0.773010, -0.757209,
	-0.740951, -0.724247, -0.707107, -0.689541, -0.671559, -0.653173,
	-0.634393, -0.615232, -0.595699, -0.575808, -0.555570, -0.534998,
	-0.514103, -0.492898, -0.471397, -0.449611, -0.427555, -0.405241,
	-0.382683, -0.359895, -0.336890, -0.313682, -0.290285, -0.266713,
	-0.242980, -0.219101, -0.195090, -0.170962, -0.146730, -0.122411,
	-0.098017, -0.073565, -0.049068, -0.024541
};

#define	QC_AUDIO_SINE(x)		QC_Sine[(x)]

/**
	Extern Function Definition!!
**/
extern HI_S32 HI_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn, const MPP_CHN_S *pstDestChn);
extern HI_S32 HI_MPI_AI_Disable(AUDIO_DEV AudioDevId);
extern HI_S32 HI_MPI_AI_DisableChn(AUDIO_DEV AudioDevId, AI_CHN AiChn);
extern HI_S32 HI_MPI_AENC_GetFd(AENC_CHN AeChn);
extern HI_S32 HI_MPI_AENC_GetStream(AENC_CHN AeChn, AUDIO_STREAM_S *pstStream, HI_BOOL bBlock);
extern  HI_S32 HI_MPI_AENC_ReleaseStream(AENC_CHN AeChn, const AUDIO_STREAM_S *pstStream);
extern HI_S32 HI_MPI_ADEC_SendStream(ADEC_CHN AdChn, const AUDIO_STREAM_S *pstStream, HI_BOOL bBlock);

typedef struct tagSAMPLE_AENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_S32  AeChn;
    HI_S32  AdChn;
    FILE    *pfd;
    HI_BOOL bSendAdChn;
} SAMPLE_AENC_S;

typedef struct tagSAMPLE_ADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
    FILE *pfd;
    pthread_t stAdPid;
} SAMPLE_ADEC_S;

static SAMPLE_AI_S gs_stSampleAi_qc[AI_DEV_MAX_NUM*AIO_MAX_CHN_NUM];
static SAMPLE_AENC_S gs_stSampleAenc[AENC_MAX_CHN_NUM];
static SAMPLE_ADEC_S gs_stSampleAdec[ADEC_MAX_CHN_NUM];

static PAYLOAD_TYPE_E gs_enPayloadType;

static HI_BOOL gs_bMicIn = HI_FALSE;

static HI_BOOL gs_bAiAnr = HI_FALSE;
static HI_BOOL gs_bAioReSample = HI_FALSE;
static HI_BOOL gs_bUserGetMode = HI_FALSE;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAiReSmpAttr = NULL;
static AUDIO_RESAMPLE_ATTR_S *gs_pstAoReSmpAttr = NULL;

#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)

static int msleep(int time)
{
    int i;

    for(i=0;i<time;i++)
        usleep(1000);

    return 0;
}

HI_S32 SAMPLE_COMM_AUDIO_AoBindAdecQC(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
	MPP_CHN_S stSrcChn,stDestChn;

	stSrcChn.enModId = HI_ID_ADEC;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = AdChn;
	stDestChn.enModId = HI_ID_AO;
	stDestChn.s32DevId = AoDev;
	stDestChn.s32ChnId = AoChn;

	return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

HI_S32 SAMPLE_COMM_AUDIO_StartAoQC(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt,
								 AIO_ATTR_S* pstAioAttr, AUDIO_SAMPLE_RATE_E enInSampleRate, HI_BOOL bResampleEn)
{
	HI_S32 i;
	HI_S32 s32Ret;

	if (SAMPLE_AUDIO_HDMI_AO_DEV == AoDevId)
	{
		#ifdef HI_ACODEC_TYPE_HDMI 

		pstAioAttr->u32ClkSel = 0;

		SAMPLE_COMM_AUDIO_StartHdmi(pstAioAttr);

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
	}

	return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_StartAdecQC(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
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

HI_S32 SAMPLE_COMM_AUDIO_AencBindAiQC(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
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

HI_S32 SAMPLE_COMM_AUDIO_StartAencQC(HI_S32 s32AencChnCnt, HI_U32 u32AencPtNumPerFrm, 	PAYLOAD_TYPE_E enType)
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

static HI_S32 SAMPLE_COMM_AUDIO_StartAiQC(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, AIO_ATTR_S* pstAioAttr, 
											AUDIO_SAMPLE_RATE_E enOutSampleRate, HI_BOOL bResampleEn, 
											HI_VOID* pstAiVqeAttr, HI_U32 u32AiVqeType)
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
				case 0 :
					s32Ret = HI_SUCCESS;
					bAiVqe = HI_FALSE;
					break;
				case 1 :
					s32Ret = HI_MPI_AI_SetVqeAttr(AiDevId, i, SAMPLE_AUDIO_AO_DEV, i, (AI_VQE_CONFIG_S *)pstAiVqeAttr);
					break;
				default :
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

void *SAMPLE_COMM_AUDIO_AiProcQC(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AiFd;
    SAMPLE_AI_S *pstAiCtl = (SAMPLE_AI_S *)parg;
    AUDIO_FRAME_S stFrame; 
    fd_set read_fds;
    struct timeval TimeoutVal;
    AI_CHN_PARAM_S stAiChnPara;

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
    FD_SET(AiFd,&read_fds);

    while (pstAiCtl->bStart)
    {     
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);
        
        s32Ret = select(AiFd+1, &read_fds, NULL, NULL, &TimeoutVal);
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
            s32Ret = HI_MPI_AI_GetFrame(pstAiCtl->AiDev, pstAiCtl->AiChn,
                &stFrame, NULL, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                printf("%s: HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n",\
                       __FUNCTION__, pstAiCtl->AiDev, pstAiCtl->AiChn, s32Ret);
                pstAiCtl->bStart = HI_FALSE;
                return NULL;
            }

            /* send frame to encoder */
            if (HI_TRUE == pstAiCtl->bSendAenc)
            {
				s32Ret = HI_MPI_AENC_SendFrame(pstAiCtl->AencChn, &stFrame, NULL);
				if (HI_SUCCESS != s32Ret) {
					printf("%s: HI_MPI_AENC_SendFrame(%d), failed with %#x!\n",
							__FUNCTION__, pstAiCtl->AencChn, s32Ret);
					pstAiCtl->bStart = HI_FALSE;
					return NULL;
				}
            }
            
            /* send frame to ao */
            if (HI_TRUE == pstAiCtl->bSendAo)
            {
                s32Ret = HI_MPI_AO_SendFrame(pstAiCtl->AoDev, pstAiCtl->AoChn,
                    &stFrame, 1000);
				if (HI_SUCCESS != s32Ret) {
					printf("%s: HI_MPI_AO_SendFrame(%d, %d), failed with %#x!\n",
							__FUNCTION__, pstAiCtl->AoDev, pstAiCtl->AoChn, s32Ret);
					pstAiCtl->bStart = HI_FALSE;
					return  NULL;
				}
            }

            /* finally you must release the stream */
            HI_MPI_AI_ReleaseFrame(pstAiCtl->AiDev, pstAiCtl->AiChn,
                &stFrame, NULL);
        }
    }
    
    pstAiCtl->bStart = HI_FALSE;
    return NULL;
}
void *SAMPLE_COMM_AUDIO_AdecProcQC(void *parg)
{
    HI_S32 s32Ret;
    AUDIO_STREAM_S stAudioStream;    
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn;
    HI_U8 *pu8AudioStream = NULL;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;    
    FILE *pfd = pstAdecCtl->pfd;
    s32AdecChn = pstAdecCtl->AdChn;
    
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
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
            fseek(pfd, 0, SEEK_SET);/*read file again*/
            continue;
        }

        /* here only demo adec streaming sending mode, but pack sending mode is commended */
        stAudioStream.u32Len = u32ReadLen;
        s32Ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
        if (s32Ret)
        {
            printf("%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!\n",\
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

void *SAMPLE_COMM_AUDIO_AencProcQC(void *parg)
{
    HI_S32 s32Ret;
    HI_S32 AencFd;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
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
        FD_SET(AencFd,&read_fds);
        
        s32Ret = select(AencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
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
                printf("%s: HI_MPI_AENC_GetStream(%d), failed with %#x!\n",\
                       __FUNCTION__, pstAencCtl->AeChn, s32Ret);
                pstAencCtl->bStart = HI_FALSE;
                return NULL;
            }

            /* send stream to decoder and play for testing */
            if (HI_TRUE == pstAencCtl->bSendAdChn)
            {
                HI_MPI_ADEC_SendStream(pstAencCtl->AdChn, &stStream, HI_TRUE);
            }
            
            /* save audio stream to file */
            fwrite(stStream.pStream,1,stStream.u32Len, pstAencCtl->pfd);

            /* finally you must release the stream */
            HI_MPI_AENC_ReleaseStream(pstAencCtl->AeChn, &stStream);
        }    
    }
    
    fclose(pstAencCtl->pfd);
    pstAencCtl->bStart = HI_FALSE;
    return NULL;
}

HI_S32 SAMPLE_COMM_AUDIO_StopAencQC(HI_S32 s32AencChnCnt)
{
    HI_S32 i;
	HI_S32 s32Ret;

    for (i = 0; i < s32AencChnCnt; i++)
    {
       s32Ret = HI_MPI_AENC_DestroyChn(i);
	   if (HI_SUCCESS != s32Ret)
	   {
		   printf("%s : HI_MPI_AENC_DestroyChn(%d) failed with %#x!\n",
				   __FUNCTION__, i, s32Ret);
		   return s32Ret;
	   }
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_StopAiQC(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
		HI_BOOL bAnrEn, HI_BOOL bResampleEn)
{   
	HI_S32 i, s32Ret;

	for (i = 0; i < s32AiChnCnt; i++)
	{
		s32Ret = HI_MPI_AI_DisableChn(AiDevId, i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("[Func] : %s [Line] : %d [Info] : %s\n", __FUNCTION__, __LINE__, "failed");
			return s32Ret;
		}
	}  

	s32Ret = HI_MPI_AI_Disable(AiDevId);
	if (HI_SUCCESS != s32Ret)
	{
		printf("[Func] : %s [Line] : %d [Info] : %s\n", __FUNCTION__, __LINE__, "failed");
		return s32Ret;
	}

	return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_AencUnbindAiQC(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn,stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;
    
    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);      
}

HI_S32 SAMPLE_COMM_AUDIO_CreatTrdFileAdecQC(ADEC_CHN AdChn, FILE *pAdcFd)
{
    SAMPLE_ADEC_S *pstAdec = NULL;

    if (NULL == pAdcFd)
    {
        return HI_FAILURE;
    }

    pstAdec = &gs_stSampleAdec[AdChn];
    pstAdec->AdChn = AdChn;
    pstAdec->pfd = pAdcFd;
    pstAdec->bStart = HI_TRUE;
    pthread_create(&pstAdec->stAdPid, 0, SAMPLE_COMM_AUDIO_AdecProcQC, pstAdec);
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAiAencQC(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    SAMPLE_AI_S *pstAi = NULL;
    
    pstAi = &gs_stSampleAi_qc[AiDev*AIO_MAX_CHN_NUM + AiChn];
    pstAi->bSendAenc = HI_TRUE;
    pstAi->bSendAo = HI_FALSE;
    pstAi->bStart= HI_TRUE;
    pstAi->AiDev = AiDev;
    pstAi->AiChn = AiChn;
    pstAi->AencChn = AeChn;
    pthread_create(&pstAi->stAiPid, 0, SAMPLE_COMM_AUDIO_AiProcQC, pstAi);
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_CreatTrdAencAdecQC(AENC_CHN AeChn, ADEC_CHN AdChn, FILE *pAecFd)
{
    SAMPLE_AENC_S *pstAenc = NULL;

    if (NULL == pAecFd)
    {
        return HI_FAILURE;
    }

    pstAenc = &gs_stSampleAenc[AeChn];
    pstAenc->AeChn = AeChn;

    pstAenc->AdChn = NULL;
    pstAenc->bSendAdChn = HI_FALSE;

    pstAenc->pfd = pAecFd;
    pstAenc->bStart = HI_TRUE;
    pthread_create(&pstAenc->stAencPid, 0, SAMPLE_COMM_AUDIO_AencProcQC, pstAenc);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_StopAdecQC(ADEC_CHN AdChn)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_ADEC_DestroyChn(AdChn);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s : HI_MPI_ADEC_DestroyChn(%d) failed with %#x!\n", 
				__FUNCTION__, AdChn, s32Ret);
		return s32Ret;
	}

	return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_StopAoQC(AUDIO_DEV AoDevId, HI_S32 s32AoChnCnt, HI_BOOL bResampleEn)
{
	HI_S32 i;
	HI_S32 s32Ret;

	for (i = 0; i < s32AoChnCnt; i++)
	{
		s32Ret = HI_MPI_AO_DisableChn(AoDevId, i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("%s : HI_MPI_AO_DisableChn failed with %#x!\n",
					__FUNCTION__, s32Ret);
			return s32Ret;
		}
	}

	s32Ret = HI_MPI_AO_Disable(AoDevId);
	if (HI_SUCCESS != s32Ret)
	{
		printf("%s : HI_MPI_AO_Disable failed with %#x!\n", 
				__FUNCTION__, s32Ret);
		return s32Ret;
	}

	return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_AoUnbindAdecQC(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
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

HI_S32 SAMPLE_COMM_AUDIO_DestroyTrdFileAencQC(AENC_CHN AeChn)
{
	SAMPLE_AENC_S *pstAenc = NULL;

	pstAenc = &gs_stSampleAenc[AeChn];

	if (pstAenc->bStart)
	{
		pstAenc->bStart = HI_FALSE;
		pthread_join(pstAenc->stAencPid, 0);
	}

	return HI_SUCCESS;
}

HI_S32 SAMPLE_COMM_AUDIO_DestroyTrdFileAdecQC(ADEC_CHN AdChn)
{
	SAMPLE_ADEC_S *pstAdec = NULL;

	pstAdec = &gs_stSampleAdec[AdChn];

	if (pstAdec->bStart)
	{
		pstAdec->bStart = HI_FALSE;
		pthread_join(pstAdec->stAdPid, 0);
	}

	return HI_SUCCESS;
}

/******************************************************************************
* function : PT Number to String
******************************************************************************/
static char* SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)  return "g711a";
    else if (PT_G711U == enType)  return "g711u";
    else if (PT_ADPCMA == enType)  return "adpcm";
    else if (PT_G726 == enType)  return "g726";
    else if (PT_LPCM == enType)  return "pcm";
    else return "data";
}

/******************************************************************************
* function : Open Aenc File
******************************************************************************/
static FILE * SAMPLE_AUDIO_OpenAencFileQC(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE *pfd;
    HI_CHAR aszFileName[128];

    /* create file for save stream*/

    sprintf(aszFileName, "/tmp/audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
    
	pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Open Adec File
******************************************************************************/
static FILE *SAMPLE_AUDIO_OpenAdecFileQC(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE *pfd;
    HI_CHAR aszFileName[128];

    /* create file for save stream*/
    sprintf(aszFileName, "/NFDVR/data/source_sine%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_AUDIO_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

void nf_HI_aud_get_attrQC(AIO_ATTR_S *stAioAttr, AUDIO_DEV *AiDev, AUDIO_DEV *AoDev, 
							AO_CHN *AoChn, ADEC_CHN *AdChn, PAYLOAD_TYPE_E *enPayloadType, guint audio_output_type)
{
	stAioAttr->enSamplerate = AUDIO_SAMPLE_RATE_8000;
	stAioAttr->enBitwidth = AUDIO_BIT_WIDTH_16;
	stAioAttr->enWorkmode = AIO_MODE_I2S_MASTER;
	stAioAttr->enSoundmode = AUDIO_SOUND_MODE_MONO;
	stAioAttr->u32EXFlag = 1;
	stAioAttr->u32FrmNum = 30;
	stAioAttr->u32PtNumPerFrm = HI_AUDIO_PTNUMPERFRM;
	stAioAttr->u32ChnCnt = QC_TEST_AUDIO_NUM;
	stAioAttr->u32ClkChnCnt = 2;
	stAioAttr->u32ClkSel = 0;

	*enPayloadType = PT_LPCM;

    *AiDev = SAMPLE_AUDIO_AI_DEV;
	*AoDev = SAMPLE_AUDIO_AO_DEV;

	*AoChn=0; *AdChn=0;
}

/******************************************************************************
* function : QC Audio Test
*
******************************************************************************/
HI_S32 SAMPLE_AUDIO_QC(void)
{
    HI_S32      i, s32Ret;

	AIO_ATTR_S  stAioAttr;
	
	AUDIO_DEV	AiDev = 0;
	AUDIO_DEV	AoDev = 0;
    
	AI_CHN      AiChn;
    AENC_CHN    AeChn;
	ADEC_CHN	AdChn = 0;
	AO_CHN		AoChn = 0;
    
    HI_S32      s32AiChnCnt;
    HI_S32      s32AencChnCnt;

    HI_BOOL     bSendAdec = HI_TRUE;

    FILE        *pfd = NULL;

	nf_HI_aud_get_attrQC(&stAioAttr, &AiDev, &AoDev, &AoChn, &AdChn, 
			&gs_enPayloadType, AUD_OUTPUT_RCA);

    /********************************************
      step 1: config audio codec
    ********************************************/


    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32AencChnCnt = s32AiChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAiQC(AiDev, s32AiChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_StartAencQC(s32AencChnCnt, SAMPLE_AUDIO_PTNUMPERFRM, gs_enPayloadType);
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
            s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAencQC(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {
            s32Ret = SAMPLE_COMM_AUDIO_AencBindAiQC(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
                return s32Ret;
            }
        }

        pfd = SAMPLE_AUDIO_OpenAencFileQC(AeChn, gs_enPayloadType);
        if (!pfd)
        {
            SAMPLE_DBG(HI_FAILURE);
            return HI_FAILURE;
        }

        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencAdecQC(AeChn, AdChn, pfd);
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
    if (HI_TRUE == bSendAdec)
    {
        s32Ret = SAMPLE_COMM_AUDIO_StartAdecQC(AdChn, gs_enPayloadType);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }

		stAioAttr.u32ChnCnt = 1;
		s32Ret = SAMPLE_COMM_AUDIO_StartAoQC(AoDev, stAioAttr.u32ChnCnt, &stAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}

		pfd = SAMPLE_AUDIO_OpenAdecFileQC(AdChn, gs_enPayloadType);
		if (!pfd)
		{
			SAMPLE_DBG(HI_FAILURE);
			return HI_FAILURE;
		}

		s32Ret = SAMPLE_COMM_AUDIO_CreatTrdFileAdecQC(AdChn, pfd);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}

		s32Ret = SAMPLE_COMM_AUDIO_AoBindAdecQC(AoDev, AoChn, AdChn);
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
	s32Ret = SAMPLE_COMM_AUDIO_DestroyTrdFileAdecQC(AdChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_AUDIO_StopAdecQC(AdChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_AUDIO_StopAoQC(AoDev, stAioAttr.u32ChnCnt, HI_FALSE);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdecQC(AoDev, AoChn, AdChn);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

		s32Ret = SAMPLE_COMM_AUDIO_AencUnbindAiQC(AiDev, AiChn, AeChn);
		if (s32Ret != HI_SUCCESS)
		{
			SAMPLE_DBG(s32Ret);
			return HI_FAILURE;
		}

		SAMPLE_COMM_AUDIO_DestroyTrdFileAencQC(AeChn);
    }

    s32Ret = SAMPLE_COMM_AUDIO_StopAencQC(s32AencChnCnt);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

    s32Ret = SAMPLE_COMM_AUDIO_StopAiQC(AiDev, s32AiChnCnt, HI_FALSE, HI_FALSE);
	if (s32Ret != HI_SUCCESS)
	{
		SAMPLE_DBG(s32Ret);
		return HI_FAILURE;
	}

    return HI_SUCCESS;
}

int nf_HI_aud_qc_test(int flag)
{
    HI_S32 s32Ret = HI_SUCCESS;
	
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
	
    s32Ret = SAMPLE_AUDIO_QC();
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: failed with %d!\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


/******************************************************************************
* function : audio diff functions
* use stand alone build
******************************************************************************/
void FastFourierTransform(QC_AUDIO_DB Re[QC_AUDIO_MAX_BUF], QC_AUDIO_DB Im[QC_AUDIO_MAX_BUF], int p)
{
	int i, ip, j, k, m, me, me1, n, nv2;
	QC_AUDIO_DB uRe, uIm, vRe, vIm, wRe, wIm, tRe, tIm;

	n = 1 << p;		//number of step
	nv2 = n / 2;
	j = 0;			//initalize

	//part of bit reverse
	for (i = 0; i < n - 1; i++) {
		if (j > i) {
			tRe = Re[j];
			tIm = Im[j];
			Re[j] = Re[i];
			Im[j] = Im[i];
			Re[i] = tRe;
			Im[i] = tIm;
		}

		k = nv2;
		while (j >= k) {
			j -= k;
			k /= 2;
		}

		j += k;
	}

	//butterfly loop
	for (m = 1; m <= p; m++) {
		me = 1 << m;	
		me1 = me / 2;
		uRe = 1.0;	
		uIm = 0.0;
		wRe = cos(QC_AUDIO_PI / me1); 
		wIm = -sin(QC_AUDIO_PI / me1);

		for (j = 0; j < me1; j++) {
			for (i = j; i < n; i += me) {
				ip = i + me1;
				tRe = Re[ip] * uRe - Im[ip] * uIm;
				tIm = Re[ip] * uIm + Im[ip] * uRe;
				Re[ip] = Re[i] - tRe;
				Im[ip] = Im[i] - tIm;
				Re[i] += tRe;
				Im[i] += tIm;
			}

			vRe = uRe * wRe - uIm * wIm;
			vIm = uRe * wIm + uIm * wRe;
			uRe = vRe;	
			uIm = vIm;
		}
	}
}

int frq_result(QC_AUDIO_DB *table, int cnt)
{
    int bigfrq = 0, i = 0;
    double temp_val = 0;

    for (i = 1; i < cnt; i++) {
        if ((table[i] - temp_val) > 0) {
            temp_val = table[i];
            bigfrq = i;
        }
    }

    printf("freq:%d\n", bigfrq);

    return bigfrq;
}

int nf_HI_aud_qc_frq_compare(int conv_inch)
{
	FILE *fpsrc, *fptar;
	HI_U16 tsrc, ttar, tmpsrc, tmptar;
	unsigned int i;
	int ret_src = 0, ret_tar = 0;
	int p = 8;
	double Re[QC_AUDIO_MAX_BUF], Te[QC_AUDIO_MAX_BUF], Im[QC_AUDIO_MAX_BUF], s, fft_src[QC_AUDIO_MAX_BUF + QC_AUDIO_MAX_BUF], fft_tar[QC_AUDIO_MAX_BUF + QC_AUDIO_MAX_BUF];
	HI_CHAR aszFileName[128];

    fpsrc = fopen("/NFDVR/data/source_sine0.pcm","rb");
	sprintf(aszFileName, "/tmp/audio_chn%d.pcm", conv_inch);
	fptar = fopen(aszFileName, "rb");

	if (NULL == fptar) {
		printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
		return 0;
	}

	printf("open stream file:\"%s\" for compare ok\n", aszFileName);

	for (i = 0; i < 8000; i++) {
		fread(&tsrc, 2, 1, fpsrc);
		fread(&ttar, 2, 1, fptar);
	}

	for (i = 0 ; i < 256; i++) {
		fread(&tsrc, 2, 1, fpsrc);
		fread(&ttar, 2, 1, fptar);
		tmpsrc = tsrc>>8;
		tmptar = ttar>>8;
		Re[i] = QC_AUDIO_SINE(tmpsrc);
		Te[i] = QC_AUDIO_SINE(tmptar);
	}

	fclose(fpsrc);
	fclose(fptar);

	FastFourierTransform(Re, Im, p);
	
	for (i = 0; i < 256; i++) {
		Im[i] = 0;
	}

	for (i = 1; i < 256; i++) {
		s= sqrt(Re[i] * Re[i] + Im[i] * Im[i]);
		fft_src[i - 1] = s;
	}

	FastFourierTransform(Te, Im, p);

	for (i = 0; i < 256; i++) {
		Im[i] = 0;
	}

	for (i = 1; i < 256; i++) {
		s = sqrt(Te[i] * Te[i] + Im[i] * Im[i]);
		fft_tar[i - 1] = s;
	}

#if 0
	for (i=1; i<256; i++){
		printf("fft src[%d] = %f, fft tar[%d] = %f\n",i, fft_src[i], i, fft_tar[i]);
	}
#endif

    ret_src = frq_result(fft_src, 128);
    ret_tar = frq_result(fft_tar, 128);

    if (ret_src == ret_tar) {
        return 1;
	} else {
        return 0;
	}
}
