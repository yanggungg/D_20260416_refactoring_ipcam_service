#include "nf_HI_common.h"
//#include "nf_HI_viu.h"

#define DBG_SIG_FIXED	(0)
#define DBG_SIG_TYPE	(VIDEO_ENCODING_MODE_NTSC)
//#define DBG_SIG_TYPE	(VIDEO_ENCODING_MODE_PAL)

extern HI_S32 HI_MPI_SYS_Exit();
extern HI_S32 HI_MPI_VB_Exit (HI_VOID);
extern HI_S32 HI_MPI_VB_SetConf (const VB_CONF_S *pstVbConf);
extern HI_S32 HI_MPI_VB_Init (HI_VOID);
extern HI_S32 HI_MPI_SYS_SetConf(const MPP_SYS_CONF_S *pstSysConf);
extern HI_S32 HI_MPI_SYS_Init();

static NF_HI_DBG_MSG_S stDbgMsg;

#if 0
VIDEO_NORM_E nf_HI_isVinrm(void)
{
	if ( DISPLAY_IS_PAL )
	{
		return VIDEO_ENCODING_MODE_PAL;
	}
	else
	{
		return VIDEO_ENCODING_MODE_NTSC;
	}
}

gboolean nf_HI_ViBindVo(HI_S32 s32ViChnTotal, VO_DEV VoDev)
{
    HI_S32 ViChnCnt;
    HI_S32 s32ViChnPerDev;
    VI_DEV ViDev;
    VO_CHN VoChn;
    VI_CHN ViChn;
    HI_S32 s32Ret;

    s32ViChnPerDev = nf_HI_getVIChnCnt();

    ViDev = G_VIDEV_START;
    ViChnCnt = 0;
	VoChn = 0;
	
    while (1)
    {
        for ( ViChn = 0; ViChn < s32ViChnPerDev; ViChn++, VoChn++ )
        {
            s32Ret = HI_MPI_VI_BindOutput( ViDev, ViChn, VoDev, VoChn );

			g_message("[%s][%d] VI[%d][%d]-VO[%d][%d]", __FUNCTION__, __LINE__,
									ViDev,
									ViChn,
									VoDev,
									VoChn );
			 
            if ( HI_SUCCESS != s32Ret )
            {
                g_warning("bind vi2vo failed, vi(%d,%d),vo(%d,%d)!", ViDev, ViChn, VoDev, VoChn);
                return s32Ret;
            }

            ViChnCnt++;
			
            if (ViChnCnt >= s32ViChnTotal)
            {
                return TRUE;
            }
        }
		
        ViDev++;
    }

    return TRUE;
}

gboolean nf_HI_ViUnBindVo(HI_S32 s32ViChnTotal, VO_DEV VoDev)
{
    HI_S32 i;
	HI_S32 s32Ret;
	HI_S32 s32ViChnPerDev;
	HI_S32 ViChnCnt;
	VI_DEV ViDev;
	VO_CHN VoChn = 0;

	s32ViChnPerDev = nf_HI_getVIChnCnt();

    ViDev = G_VIDEV_START;
	ViChnCnt = 0;

	while (1)
    {
	    for (i = 0; i < s32ViChnPerDev; i++, VoChn++)
	    {
	        s32Ret = HI_MPI_VI_UnBindOutput(ViDev, i, VoDev, VoChn);

			if ( HI_SUCCESS != s32Ret )
	        {
	            printf("bind vi2vo failed, vi(%d,%d),vo(%d,%d)!\n", ViDev, i, VoDev, VoChn);
	            return FALSE;
	        }

			ViChnCnt++;
			
            if (ViChnCnt >= s32ViChnTotal)
            {
                return TRUE;
            }
	    }    

		ViDev++;
	}
    return TRUE;
}
#endif

GstNfBuddyBuffer *nf_HI_gst_buffer(gint size, gboolean verbose) 
{
	GstNfBuddyBuffer *p = NULL;
	ICODEC_HEADER *h = NULL;
	
	g_return_val_if_fail( size < ( 1 << 20 ), NULL );

	p = (GstNfBuddyBuffer *) gst_nf_buddy_buffer_new_and_alloc( size, NULL );

	g_assert( p != NULL );

	h = (ICODEC_HEADER *)( GST_BUFFER_DATA( p ) );

	g_assert( h != NULL );

	h->gst_buffer	= p;
	h->reserved		= 0;

	if ( verbose == TRUE )
	{
		guint addr;
		addr = ICMEM_getBufferPhysicalAddress( h );
	}

	p->frame	= h;
	p->cmemq	= 0;
	p->free_cb	= 0;

	return p;
}
#if 0
void nf_HI_VENC_incReadCnt(void)
{
	g_atomic_int_inc( &stDbgMsg.s32ReadCnt );
}

static void nf_HI_VENC_resetReadCnt(void)
{
	g_atomic_int_set( &stDbgMsg.s32ReadCnt, 0 );
}

static HI_S32 nf_HI_VENC_getReadCnt(void)
{
	return g_atomic_int_get( &stDbgMsg.s32ReadCnt );
}

void nf_HI_VENC_incRec(HI_U32 size)
{
	g_atomic_int_inc( &stDbgMsg.s32RecCnt );
	stDbgMsg.u32RecSize += size;
}

static void nf_HI_VENC_resetRec(void)
{
	g_atomic_int_set( &stDbgMsg.s32RecCnt, 0 );
	g_atomic_int_set( &stDbgMsg.u32RecSize, 0 );
}

static HI_S32 nf_HI_VENC_getRecCnt(void)
{
	return g_atomic_int_get( &stDbgMsg.s32RecCnt );
}

static HI_U32 nf_HI_VENC_getRecSize(void)
{
	return g_atomic_int_get( &stDbgMsg.u32RecSize );
}

void nf_HI_VENC_incAsyncCnt(void)
{
	g_atomic_int_inc( &stDbgMsg.s32AsyncCnt );
}

static void nf_HI_VENC_resetAsyncCnt(void)
{
	g_atomic_int_set( &stDbgMsg.s32AsyncCnt, 0 );
}

static HI_S32 nf_HI_VENC_getAsyncCnt(void)
{
	return g_atomic_int_get( &stDbgMsg.s32AsyncCnt );
}

void nf_HI_VDEC_incTimerCnt(void)
{
	g_atomic_int_inc( &stDbgMsg.s32DecTimer );
}

static void nf_HI_VDEC_resetTimerCnt(void)
{
	g_atomic_int_set( &stDbgMsg.s32DecTimer, 0 );
}

static HI_S32 nf_HI_VDEC_getTimerCnt(void)
{
	return g_atomic_int_get( &stDbgMsg.s32DecTimer );
}

void nf_HI_VDEC_incDec(HI_U32 size, HI_S32 queLen)
{
	g_atomic_int_set( &stDbgMsg.s32QueLen, queLen );
	g_atomic_int_inc( &stDbgMsg.s32DecCnt );
	stDbgMsg.u32DecSize += size;
}

static void nf_HI_VDEC_resetDec(void)
{
	g_atomic_int_set( &stDbgMsg.s32DecCnt, 0 );
	g_atomic_int_set( &stDbgMsg.u32DecSize, 0 );
}

static HI_S32 nf_HI_VENC_getQueLen(void)
{
	return g_atomic_int_get( &stDbgMsg.s32QueLen );
}

static HI_S32 nf_HI_VENC_getDecCnt(void)
{
	return g_atomic_int_get( &stDbgMsg.s32DecCnt );
}

static HI_U32 nf_HI_VENC_getDecSize(void)
{
	return g_atomic_int_get( &stDbgMsg.u32DecSize );
}

gboolean nf_HI_dbgMsg( GTimeVal *tvIntVal )
{
	g_message("Gap[%ld.%06ld] RD[%d] REC[%d][%u] Asyc[%d] Get[%d] Q[%d] DEC[%d][%u]",
					tvIntVal->tv_sec, tvIntVal->tv_usec,
					nf_HI_VENC_getReadCnt(),
					nf_HI_VENC_getRecCnt(),
					nf_HI_VENC_getRecSize(),
					nf_HI_VENC_getAsyncCnt(),
					nf_HI_VDEC_getTimerCnt(),
					nf_HI_VENC_getQueLen(),
					nf_HI_VENC_getDecCnt(),
					nf_HI_VENC_getDecSize() );

	nf_HI_VENC_resetReadCnt();
	nf_HI_VENC_resetRec();
	nf_HI_VENC_resetAsyncCnt();
	nf_HI_VDEC_resetTimerCnt();
	nf_HI_VDEC_resetDec();

	return TRUE;
}

void dump_icodec_header( const char *str, ICODEC_HEADER *pheader)
{
	g_message("%s ch[%02d] f[0x%02x] t[0x%02x] rate[0x%02x] res[0x%02x] PTS[%u][%d] SZ[%6d] idx[%08x] ", str , 
					pheader->chan, 
					pheader->flags, 
					pheader->frame_type, 
					pheader->frame_rate, 
					pheader->resolution, 						
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size,
					pheader->reserved );
	return;
}

void icodec_time_show( guint time, guint timel )
{
  gchar *t_buf[1024];
  struct tm buff_tm;
  GTimeVal now;

  now.tv_sec = time;
  now.tv_usec = timel;

  localtime_r( &(now.tv_sec), &buff_tm );

  sprintf( t_buf,"%02d:%02d:%02d", buff_tm.tm_hour , buff_tm.tm_min, buff_tm.tm_sec );
  g_message("[%s] TIME[%s]", __FUNCTION__, t_buf);
}
#endif


HI_S32 nf_HI_SYS_Init(VB_CONF_S *pstVbConf)
{
	MPP_SYS_CONF_S stSysConf = {0};
	HI_S32 s32Ret = HI_FAILURE;

	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();

	if (NULL == pstVbConf)
	{
		g_warning("input parameter is null, it is invaild!");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VB_SetConf(pstVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		g_warning("HI_MPI_VB_SetConf failed!");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VB_Init();
	if (HI_SUCCESS != s32Ret)
	{
		g_warning("HI_MPI_VB_Init failed!");
		return HI_FAILURE;
	}

	stSysConf.u32AlignWidth = SAMPLE_SYS_ALIGN_WIDTH;
	s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
	if (HI_SUCCESS != s32Ret)
	{
		g_warning("HI_MPI_SYS_SetConf failed");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_SYS_Init();
	if (HI_SUCCESS != s32Ret)
	{
		g_warning("HI_MPI_SYS_Init failed!");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

