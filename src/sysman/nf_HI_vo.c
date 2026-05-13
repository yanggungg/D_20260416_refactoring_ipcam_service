#include "nf_HI_vo.h"

// HI3536
extern HI_S32 HI_MPI_HDMI_Init(HI_VOID);
extern HI_S32 HI_MPI_HDMI_DeInit(HI_VOID);
extern HI_S32 HI_MPI_HDMI_Close(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_Start(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_Stop(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_Open(HI_HDMI_ID_E enHdmi);
extern HI_S32 HI_MPI_HDMI_SetAttr(HI_HDMI_ID_E enHdmi, HI_HDMI_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_HDMI_GetAttr(HI_HDMI_ID_E enHdmi, HI_HDMI_ATTR_S *pstAttr);
extern HI_S32 HI_MPI_HDMI_Force_GetEDID(HI_HDMI_ID_E enHdmi, HI_HDMI_EDID_S *pstEdidData);

extern HI_S32 HI_MPI_VO_Enable (VO_DEV VoDev);
extern HI_S32 HI_MPI_VO_Disable(VO_DEV VoDev);
extern HI_S32 HI_MPI_VO_DisableVideoLayer(VO_LAYER VoLayer);
extern HI_S32 HI_MPI_VO_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr);
extern HI_S32 HI_MPI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);
extern HI_S32 HI_MPI_VO_EnableVideoLayer (VO_LAYER VoLayer);
extern HI_S32 HI_MPI_VO_SetVideoLayerAttr(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);

/**
	nf_HI_vo_StartDevLayer-> SAMPLE_COMM_VO_StartDevLayer
**/
#if 0
HI_S32 nf_HI_vo_StartDevLayer(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr, HI_U32 u32SrcFrmRate)
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32Width = 0;
	HI_U32 u32Height = 0;
	HI_U32 u32Frm = 0;
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;

	if ( 0 == u32SrcFrmRate )
	{
		g_warning("vo u32SrcFrmRate invaild! %d!", u32SrcFrmRate);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_DisableVideoLayer(VoDev);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_Disable(VoDev);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}

	//printf("-----------------dev:%d\n", VoDev);
	s32Ret = HI_MPI_VO_SetPubAttr(VoDev, pstPubAttr);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_Enable(VoDev);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = nf_HI_VO_GetWH(pstPubAttr->enIntfSync, &u32Width, &u32Height, &u32Frm);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}

	stLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
	stLayerAttr.u32DispFrmRt = u32SrcFrmRate;

	stLayerAttr.stDispRect.s32X       = 0;
	stLayerAttr.stDispRect.s32Y       = 0;
	stLayerAttr.stDispRect.u32Width   = u32Width;
	stLayerAttr.stDispRect.u32Height  = u32Height;
	stLayerAttr.stImageSize.u32Width  = u32Width;
	stLayerAttr.stImageSize.u32Height = u32Height;

	s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoDev, &stLayerAttr);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_EnableVideoLayer(VoDev);
	if (s32Ret != HI_SUCCESS)
	{
		g_warning("failed with %#x!", s32Ret);
		return HI_FAILURE;
	}
	
	return s32Ret;
}
#endif
/**
	nf_HI_VO_HdmiStart -> SAMPLE_COMM_VO_HdmiStart
**/
HI_S32 nf_HI_VO_HdmiStart(VO_INTF_SYNC_E enIntfSync)
{
	HI_HDMI_ATTR_S      stAttr;
	HI_HDMI_VIDEO_FMT_E enVideoFmt;
	HI_HDMI_EDID_S stEdidData;

	#if 0
		SAMPLE_COMM_VO_HdmiConvertSync(enIntfSync, &enVideoFmt);
	#else
		nf_HI_VO_HdmiConvertSync(enIntfSync, &enVideoFmt);
	#endif
	HI_MPI_HDMI_Init();

	HI_MPI_HDMI_Open(HI_HDMI_ID_0);

	HI_MPI_HDMI_Force_GetEDID(0,&stEdidData);
	HI_MPI_HDMI_GetAttr(HI_HDMI_ID_0, &stAttr);

	stAttr.bEnableHdmi = HI_TRUE;
	 
	stAttr.bEnableVideo = HI_TRUE;
	stAttr.enVideoFmt = enVideoFmt;

	stAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
	stAttr.enDeepColorMode = HI_HDMI_DEEP_COLOR_OFF;
	stAttr.bxvYCCMode = HI_FALSE;
	stAttr.enDefaultMode = HI_HDMI_FORCE_HDMI;

	stAttr.bEnableAudio = HI_FALSE;
	stAttr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S;
	stAttr.bIsMultiChannel = HI_FALSE;

	stAttr.enBitDepth = HI_HDMI_BIT_DEPTH_16;

	stAttr.bEnableAviInfoFrame = HI_TRUE;
	stAttr.bEnableAudInfoFrame = HI_TRUE;
	stAttr.bEnableSpdInfoFrame = HI_FALSE;
	stAttr.bEnableMpegInfoFrame = HI_FALSE;

	stAttr.bDebugFlag = HI_FALSE;
	stAttr.bHDCPEnable = HI_FALSE;

	stAttr.b3DEnable = HI_FALSE;

	HI_MPI_HDMI_SetAttr(HI_HDMI_ID_0, &stAttr);

	HI_MPI_HDMI_Start(HI_HDMI_ID_0);

	printf("NF_HOST HDMI start success.\n");
	return HI_SUCCESS;
}

/**
	nf_HI_VO_GetWH -> SAMPLE_COMM_VO_GetWH
**/
HI_S32 nf_HI_VO_GetWH(VO_INTF_SYNC_E enIntfSync, HI_U32 *pu32W,HI_U32 *pu32H, HI_U32 *pu32Frm)
{
	switch (enIntfSync)
	{
		case VO_OUTPUT_PAL       :  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 25; break;
		case VO_OUTPUT_NTSC      :  *pu32W = 720;  *pu32H = 480;  *pu32Frm = 30; break;
		case VO_OUTPUT_576P50    :  *pu32W = 720;  *pu32H = 576;  *pu32Frm = 50; break;
		case VO_OUTPUT_480P60    :  *pu32W = 720;  *pu32H = 480;  *pu32Frm = 60; break;
		case VO_OUTPUT_800x600_60:  *pu32W = 800;  *pu32H = 600;  *pu32Frm = 60; break;
		case VO_OUTPUT_720P50    :  *pu32W = 1280; *pu32H = 720;  *pu32Frm = 50; break;
		case VO_OUTPUT_720P60    :  *pu32W = 1280; *pu32H = 720;  *pu32Frm = 60; break;
		case VO_OUTPUT_1080I50   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 50; break;
		case VO_OUTPUT_1080I60   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 60; break;
		case VO_OUTPUT_1080P24   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 24; break;
		case VO_OUTPUT_1080P25   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 25; break;
		case VO_OUTPUT_1080P30   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 30; break;
		case VO_OUTPUT_1080P50   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 50; break;
		case VO_OUTPUT_1080P60   :  *pu32W = 1920; *pu32H = 1080; *pu32Frm = 60; break;
		case VO_OUTPUT_1024x768_60:  *pu32W = 1024; *pu32H = 768;  *pu32Frm = 60; break;
		case VO_OUTPUT_1280x1024_60: *pu32W = 1280; *pu32H = 1024; *pu32Frm = 60; break;
		case VO_OUTPUT_1366x768_60:  *pu32W = 1366; *pu32H = 768;  *pu32Frm = 60; break;
		case VO_OUTPUT_1440x900_60:  *pu32W = 1440; *pu32H = 900;  *pu32Frm = 60; break;
		case VO_OUTPUT_1280x800_60:  *pu32W = 1280; *pu32H = 800;  *pu32Frm = 60; break;
		case VO_OUTPUT_1600x1200_60: *pu32W = 1600; *pu32H = 1200; *pu32Frm = 60; break;
		case VO_OUTPUT_1680x1050_60: *pu32W = 1680; *pu32H = 1050; *pu32Frm = 60; break;
		case VO_OUTPUT_1920x1200_60: *pu32W = 1920; *pu32H = 1200; *pu32Frm = 60; break;
		case VO_OUTPUT_3840x2160_30: *pu32W = 3840; *pu32H = 2160; *pu32Frm = 30; break;
		case VO_OUTPUT_3840x2160_60: *pu32W = 3840; *pu32H = 2160; *pu32Frm = 60; break;
		case VO_OUTPUT_USER    :     *pu32W = 720;  *pu32H = 576;  *pu32Frm = 25; break;
		default:
			g_warning("vo enIntfSync not support!");
			return HI_FAILURE;
	}

	return HI_SUCCESS;
}

/**
	nf_HI_VO_HdmiConvertSync -> SAMPLE_COMM_VO_HdmiConvertSync
**/
static HI_VOID nf_HI_VO_HdmiConvertSync(VO_INTF_SYNC_E enIntfSync, HI_HDMI_VIDEO_FMT_E *penVideoFmt)
{
	switch (enIntfSync)
	{
		case VO_OUTPUT_PAL:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_PAL;
			break;
		case VO_OUTPUT_NTSC:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_NTSC;
			break;
		case VO_OUTPUT_1080P24:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_24;
			break;
		case VO_OUTPUT_1080P25:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_25;
			break;
		case VO_OUTPUT_1080P30:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_30;
			break;
		case VO_OUTPUT_720P50:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_720P_50;
			break;
		case VO_OUTPUT_720P60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_720P_60;
			break;
		case VO_OUTPUT_1080I50:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_50;
			break;
		case VO_OUTPUT_1080I60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080i_60;
			break;
		case VO_OUTPUT_1080P50:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_50;
			break;
		case VO_OUTPUT_1080P60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_1080P_60;
			break;
		case VO_OUTPUT_576P50:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_576P_50;
			break;
		case VO_OUTPUT_480P60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_480P_60;
			break;
		case VO_OUTPUT_800x600_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_800X600_60;
			break;
		case VO_OUTPUT_1024x768_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1024X768_60;
			break;
		case VO_OUTPUT_1280x1024_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X1024_60;
			break;
		case VO_OUTPUT_1366x768_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1366X768_60;
			break;
		case VO_OUTPUT_1440x900_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1440X900_60;
			break;
		case VO_OUTPUT_1280x800_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1280X800_60;
			break;
		case VO_OUTPUT_1920x1200_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_VESA_1920X1200_60;
			break;
		case VO_OUTPUT_3840x2160_30:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_30;
			break;
		case VO_OUTPUT_3840x2160_60:
			*penVideoFmt = HI_HDMI_VIDEO_FMT_3840X2160P_60;
			break;
		default :
			g_warning("Unkonw VO_INTF_SYNC_E value!\n");
			break;
	}
	return;
}

