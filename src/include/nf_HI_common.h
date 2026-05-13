#ifndef __NF_HI_COMMON_H__
#define __NF_HI_COMMON_H__

#include <glib.h>
#include <gst/nf/gstnfbuddybuffer.h>
#include <gst/nf/gstnflistbuffer.h>

#include "nf_codec_header.h"

/*
#include "hi3531/hi_type.h"
#include "hi3531/hi_comm_aenc.h"
#include "hi3531/hi_comm_adec.h"
#include "hi3531/hi_comm_vdec.h"
#include "hi3531/hi_comm_vb.h"
#include "hi3531/hi_comm_hdmi.h"
#include "hi3531/hi_comm_vo.h"
#include "hi3531/hi_comm_video.h"
#include "hi3531/hi_comm_sys.h"
*/
#include "hi35XX/hi_type.h"
#include "hi35XX/hi_comm_aenc.h"
#include "hi35XX/hi_comm_adec.h"
#include "hi35XX/hi_comm_vdec.h"
#include "hi35XX/hi_comm_vb.h"
#include "hi35XX/hi_comm_hdmi.h"
#include "hi35XX/hi_comm_vo.h"
#include "hi35XX/hi_comm_video.h"
#include "hi35XX/hi_comm_sys.h"

#if 0
#include <pthread.h>
#include <sys/ioctl.h>
#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/nf/gstnfbuddybuffer.h>
#include <libicmem.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vpp.h"
#include "hi_comm_md.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vdec.h"
#include "mpi_vpp.h"
#include "mpi_md.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_aenc.h"
#include "mpi_adec.h"

#include "adv7441/adv7441.h"
#include "mt9d131/mt9d131.h"
#include "hifb.h"
#include "nf_common.h"
#include "nf_codec_header.h"
#include "nf_dspcomm_app.h"

#include "tw2864/tw2864.h"
#include "tw2865/tw2865.h"
#endif

#define SAMPLE_SYS_ALIGN_WIDTH			64
#define SAMPLE_PIXEL_FORMAT				PIXEL_FORMAT_YUV_SEMIPLANAR_420

#define SAMPLE_AUDIO_HDMI_AO_DEV		1
#define SAMPLE_AUDIO_AI_DEV				0
#define SAMPLE_AUDIO_AO_DEV				0

#define SAMPLE_AUDIO_PTNUMPERFRM		320

#define HI_NUM_LOCAL_CH		(NUM_ACTIVE_CH)
#define HI_START_LOCAL_CH	(0)
#define HI_END_LOCAL_CH		(HI_START_LOCAL_CH + HI_NUM_LOCAL_CH -1)

#define HI_NUM_NET_CH	(NUM_ACTIVE_CH)
#define HI_START_NET_CH	(HI_NUM_LOCAL_CH)
#define HI_END_NET_CH	(HI_START_NET_CH + HI_NUM_NET_CH -1)

#define HI_NUM_TOTAL_CH (HI_NUM_LOCAL_CH + HI_NUM_NET_CH)
#define HI_JPEG_CH 		(HI_NUM_TOTAL_CH)

#define TW2865_FILE   "/dev/tw2865dev"
#define ADV7441_DEV_FILE "/dev/adv7441"

/* RGB format is 1888. */
#define VO_BKGRD_RED      0xFF0000    /* red back groud color */
#define VO_BKGRD_GREEN    0x00FF00    /* green back groud color */
#define VO_BKGRD_BLUE     0x0000FF    /* blue back groud color */
#define VO_BKGRD_BLACK    0x000000    /* black back groud color */

#define G_VIDEV_START	(0)

#define HI_USED_INIT	(0)

#define DBG_MSG_NONE	(0)
#define DBG_MSG_MPP		(1)
#define DBG_MSG_VIU		(2)
#define DBG_MSG_VOU		(3)
#define DBG_MSG_VENC	(4)
#define DBG_MSG_VDEC	(5)
#define DBG_MSG_AUD		(6)

#define HI_DBG_VENC(level, fmt...)	\
	do{\
		if(DBG_MSG_VENC == level)\
		{\
		    g_message(fmt);\
		}\
	} while(0);

#define HI_DBG_VDEC(level, fmt...)	\
	do{\
		if(DBG_MSG_VDEC == level)\
		{\
		    g_message(fmt);\
		}\
	} while(0);

#define HI_DBG_AUD(level, fmt...)	\
	do{\
		if(DBG_MSG_AUD == level)\
		{\
		    g_message(fmt);\
		}\
	} while(0);

typedef struct nf_HI_DBG_MSG_S
{
	HI_S32 s32ReadCnt;
	HI_S32 s32RecCnt;
	HI_U32 u32RecSize;
	HI_S32 s32AsyncCnt;
	HI_S32 s32DecTimer;
	HI_S32 s32QueLen;
	HI_S32 s32DecCnt;
	HI_U32 u32DecSize;
} NF_HI_DBG_MSG_S;


#if 0
	GstNfBuddyBuffer *nf_HI_gst_buffer(gint size, gboolean verbose);
	VIDEO_NORM_E nf_HI_isVinrm(void);
	GstNfBuddyBuffer *nf_HI_gst_buffer(gint size, gboolean verbose);
#endif
gboolean nf_HI_ViBindVo(HI_S32 s32ViChnTotal, VO_DEV VoDev);

gboolean nf_HI_dbgMsg( GTimeVal *tvIntVal );
HI_S32 nf_HI_SYS_Init(VB_CONF_S *pstVbConf);
HI_S32 nf_HI_AUD_StartHdmi(AIO_ATTR_S *pstAioAttr);
void nf_HI_VENC_incReadCnt(void);
void nf_HI_VENC_incRec(HI_U32 size);
void nf_HI_VENC_incAsyncCnt(void);
void nf_HI_VDEC_incTimerCnt(void);
void nf_HI_VDEC_incDec(HI_U32 size, HI_S32 queLen);
#if 0
	void dump_icodec_header( const char *str, ICODEC_HEADER *pheader);
	void icodec_time_show( guint time, guint timel );
#endif

#endif /* __NF_HI_COMMON_H__ */
