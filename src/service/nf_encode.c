#if 0
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <xdc/std.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Timestamp.h>
#include <xdc/cfg/global.h>
#include <ti/ipc/Ipc.h>
#include <ti/omx/omxutils/omx_utils.h>
#include <xdc/runtime/knl/Thread.h>
#endif

#include <gobj.h>
#include <libicmem.h>
#include <gobjmedia.h>
#include "nf_common.h"
#include "nf_common_util.h"
#include "nf_codec_header.h"
#include "nf_record.h"
#include "nf_encode.h"

#ifdef ENABLE_WATCHDOG
#include "nf_watchdog.h"
#endif  /**/

#define DEBUG_ENCODER_JBSHELL
#ifdef  DEBUG_ENCODER_JBSHELL
#include "jbshell.h"
//#include "libsst.h"
#endif

NF_ENCODE_HANDOFF_FUNC	g_handoff_func = NULL;
long long               g_handoff_ch_mask = 0x0LL;    

/******************************************************************************/
//#define REC_CH_USE_2ND_STREAM_
//#define SIMPLE_ENCODER_ //test only
//#define TIME_CHANGE_CHECK_TO_SEQ_NUM_

/******************************************************************************/
#define CAT_A   (0)
#define CAT_B   (0)
#define CAT_C   (0)
#define CAT_D   (0)

#define IS_H264 777
#define IS_H265 888

#define TRACE_ENC_
#ifdef TRACE_ENC_
#define __T(c, fmt, args...)    \
        if( (c)==1 ) {                                                         \
            g_message("[%s:%d] "fmt, __FUNCTION__, __LINE__, ##args);          \
        }                                                                      \
        else if( (c)==2 ) {                                                    \
            g_message(fmt, ##args);                                            \
        }

#else	/**/
#define __T(c, fmt, afgs...)

#endif	/**/
#define __E(fmt, args...)   \
            g_message("[%s:%d] **ERROR** "fmt"\n", __FUNCTION__, __LINE__, ##args)
            
#define __A(c, fmt, args...)                                                   \
                if( !(c) ) {                                                   \
					g_message("[%s:%d] **ERROR** "fmt,                         \
                            __FUNCTION__, __LINE__, ##args);                   \
					g_assert(0);                                               \
				}
#if 0
#define DBG_ICODEC_HEADER(a, b, c, d)   _dbg_print_icodec_header(a,b,c,d)
#else
#define DBG_ICODEC_HEADER(a, b, c, d)
#endif

/******************************************************************************/
#define Kbps    (1000)
#define Mbps    (Kbps*Kbps)

typedef enum _NF_BITRATE_LV_E {
#ifdef PRESET_A_
    //1920x1080
    NF_BITRATE_TYPE0_LV0 = 1500*Kbps,
    NF_BITRATE_TYPE0_LV1 =    2*Mbps,
    NF_BITRATE_TYPE0_LV2 =    4*Mbps,
    NF_BITRATE_TYPE0_LV3 =    6*Mbps,
    NF_BITRATE_TYPE0_LV4 =    8*Mbps,
    NF_BITRATE_TYPE0_LV5 =   10*Mbps,

    //1280x720
    NF_BITRATE_TYPE1_LV0 =    1*Mbps,
    NF_BITRATE_TYPE1_LV1 =    2*Mbps,
    NF_BITRATE_TYPE1_LV2 =    3*Mbps,
    NF_BITRATE_TYPE1_LV3 =    4*Mbps,
    NF_BITRATE_TYPE1_LV4 =    5*Mbps,
    NF_BITRATE_TYPE1_LV5 =    6*Mbps,

    //640x360
    NF_BITRATE_TYPE2_LV0 = 1200*Kbps,
    NF_BITRATE_TYPE2_LV1 = 1400*Kbps,
    NF_BITRATE_TYPE2_LV2 = 1600*Kbps,
    NF_BITRATE_TYPE2_LV3 = 1800*Kbps,
    NF_BITRATE_TYPE2_LV4 = 2200*Kbps,
    NF_BITRATE_TYPE2_LV5 = 2400*Kbps,
    
#else
    // 2592x1944
    NF_BITRATE_TYPE0_LV0 =  1098*Kbps,
    NF_BITRATE_TYPE0_LV1 = 1500*Kbps,
    NF_BITRATE_TYPE0_LV2 = 3000*Kbps,
    NF_BITRATE_TYPE0_LV3 = 4500*Kbps,
    NF_BITRATE_TYPE0_LV4 = 6000*Kbps,
    NF_BITRATE_TYPE0_LV5 =10000*Kbps,

    // 1920x1080
    NF_BITRATE_TYPE1_LV0 =  750*Kbps,
    NF_BITRATE_TYPE1_LV1 = 1*Mbps,
    NF_BITRATE_TYPE1_LV2 = 2*Mbps,
    NF_BITRATE_TYPE1_LV3 = 3*Mbps,
    NF_BITRATE_TYPE1_LV4 = 4*Mbps,
    NF_BITRATE_TYPE1_LV5 = 10*Mbps,

    // 1024x1536
    NF_BITRATE_TYPE2_LV0 =  640*Kbps,
    NF_BITRATE_TYPE2_LV1 = 875*Kbps,
    NF_BITRATE_TYPE2_LV2 = 1750*Kbps,
    NF_BITRATE_TYPE2_LV3 = 2625*Kbps,
    NF_BITRATE_TYPE2_LV4 = 3500*Kbps,
    NF_BITRATE_TYPE2_LV5 = 5000*Kbps,

    // 1280x720
    NF_BITRATE_TYPE3_LV0 =  563*Kbps,
    NF_BITRATE_TYPE3_LV1 = 768*Kbps,
    NF_BITRATE_TYPE3_LV2 = 1536*Kbps,
    NF_BITRATE_TYPE3_LV3 = 2304*Kbps,
    NF_BITRATE_TYPE3_LV4 = 3*Mbps,
    NF_BITRATE_TYPE3_LV5 = 6*Mbps,
	
	// 960
	NF_BITRATE_TYPE4_LV0 =  375*Kbps,
	NF_BITRATE_TYPE4_LV1 =  512*Kbps,
	NF_BITRATE_TYPE4_LV2 = 1024*Kbps,
	NF_BITRATE_TYPE4_LV3 = 1536*Kbps,
	NF_BITRATE_TYPE4_LV4 = 2*Mbps,
	NF_BITRATE_TYPE4_LV5 = 4*Mbps,

	// D1
	NF_BITRATE_TYPE5_LV0 =  281*Kbps,
	NF_BITRATE_TYPE5_LV1 =  384*Kbps,
	NF_BITRATE_TYPE5_LV2 = 768*Kbps,
	NF_BITRATE_TYPE5_LV3 = 1152*Kbps,
	NF_BITRATE_TYPE5_LV4 = 1536*Kbps,
	NF_BITRATE_TYPE5_LV5 = 3*Mbps,

 	// 640x480(640x360)
	NF_BITRATE_TYPE6_LV0 = 281*Kbps,
	NF_BITRATE_TYPE6_LV1 = 384*Kbps,
	NF_BITRATE_TYPE6_LV2 = 768*Kbps,
	NF_BITRATE_TYPE6_LV3 = 1152*Kbps,
	NF_BITRATE_TYPE6_LV4 = 1536*Kbps,
	NF_BITRATE_TYPE6_LV5 = 3*Mbps,

	// 2CIF
	NF_BITRATE_TYPE7_LV0 =  188*Kbps,
	NF_BITRATE_TYPE7_LV1 =  256*Kbps,
	NF_BITRATE_TYPE7_LV2 =  512*Kbps,
	NF_BITRATE_TYPE7_LV3 =  768*Kbps,
	NF_BITRATE_TYPE7_LV4 = 1*Mbps,
	NF_BITRATE_TYPE7_LV5 = 3*Mbps,

	// CIF
	NF_BITRATE_TYPE8_LV0 =  137*Kbps,
	NF_BITRATE_TYPE8_LV1 =  188*Kbps,
	NF_BITRATE_TYPE8_LV2 =  375*Kbps,
	NF_BITRATE_TYPE8_LV3 =  563*Kbps,
	NF_BITRATE_TYPE8_LV4 = 750*Kbps,
	NF_BITRATE_TYPE8_LV5 = 1*Mbps,

#endif /* PRESET_A_ */
} NF_BITRATE_LV_E;

#define MAX_FPS_NUM     (6)
#define MAX_QUALITY_NUM (6)
#define GET_BR_PER(q, f)    g_br_per[q][f]

gfloat g_br_per[MAX_QUALITY_NUM][MAX_FPS_NUM] = {
/*NF_BITRATE_TYPEX_LV0*/
    /* 1fps    2fps    4fps    8fps   16fps   32fps */
    { 35.0f,  62.0f,  65.0f,  70.0f,  75.0f, 100.0f},   //done
    
/*NF_BITRATE_TYPEX_LV1*/
    /* 1fps    2fps    4fps    8fps   16fps   32fps */
    { 30.0f,  56.0f,  60.0f,  68.0f,  74.0f, 100.0f},   //done
    
/*NF_BITRATE_TYPEX_LV2*/
    /* 1fps    2fps    4fps    8fps   16fps   32fps */
    { 26.0f,  49.0f,  55.0f,  65.0f,  73.0f, 100.0f},   //done
    
/*NF_BITRATE_TYPEX_LV3*/
    /* 1fps    2fps    4fps    8fps   16fps   32fps */
    { 22.0f,  42.0f,  50.0f,  62.0f,  72.0f, 100.0f},   //done
    
/*NF_BITRATE_TYPEX_LV4*/
    /* 1fps    2fps    4fps    8fps   16fps   32fps */
    { 18.0f,  35.0f,  45.0f,  60.0f,  70.0f, 100.0f},   //done
    
/*NF_BITRATE_TYPEX_LV5*/
    /* 1fps    2fps    4fps    8fps   16fps   32fps */
    { 40.0f,  65.0f,  55.0f,  60.0f,  70.0f, 100.0f}    //TODO
};

/******************************************************************************/
static NF_ENC_CONTEXT_T g_enc_ctxt;

const guchar gop_tbl[MAX_GOP_TBL_NUM][GOP_TBL_LEN] = {
     /* 0: FRAME_TYPE_P, 1: FRAME_TYPE_I, 2: FRAME_TYPE_NULL            */
     /* 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 */
/*32*/ {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*16*/ {1,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,1,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,
        1,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,1,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2},
/*08*/ {1,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,1,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,
        1,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,1,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2},
/*04*/ {1,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,
        1,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,0,2,2,2,2,2,2,2},
/*02*/ {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
/*01*/ {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
/* 0*/ {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}};
        
const guchar gop_tbl_len[MAX_GOP_TBL_NUM] = {16,16,16,16,16,32,64};
const guint  gop_i_interval[MAX_GOP_TBL_NUM] = {16, 8, 4, 2, 1, 1, 0};

/******************************************************************************/
#define GET_FTYPE(fi, gi)       gop_tbl[fi][gi]
#define GET_GOP_TBL_LEN(fi)     gop_tbl_len[fi]
#define GET_INTRA_INTERVAL(fi)  gop_i_interval[fi]
#define TS_TO_MSEC(a, b)       (((unsigned long long)a*1000L)+b*5)

/******************************************************************************/
#if 1   /*debug api.*/
static volatile guint enc_dump_fb_ch = 0;
static volatile guint enc_max_br    = 0;
static volatile guint enc_target_br = 0; //0:VBR
static volatile guint enc_qp        = 29;

static guint enc_stm_len_wm[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static void
enc_check_max_stm_size(gint ch, guint stm_len)
{
    guint old_stm_len;
    
    g_return_if_fail(ch<NUM_CHANNEL);
    //g_return_if_fail(stm_len<ANF_MAX_STM_LEN);
    
    old_stm_len = enc_stm_len_wm[ch];
    if( stm_len>old_stm_len ) {
        __T(CAT_B, "current max stream length: ch:%2d/old:%5d/new:%5d.", ch, old_stm_len, stm_len);
        enc_stm_len_wm[ch] = stm_len;
    }
}

static guint enc_stm_len_I[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint enc_stm_len_P[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint enc_stm_len_A[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint enc_stm_cnt_I[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint enc_stm_cnt_P[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static guint enc_stm_cnt_A[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static gfloat enc_stm_len_Bps[NUM_CHANNEL] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static NF_ENC_CONTEXT_T g_enc_ctxt;

static gfloat
_enc_get_real_fps(guint fps)
{
	switch(fps)
	{
		case NF_FPS_CR32:	return DISPLAY_IS_PAL ? 25.00f: 30.00f;
		case NF_FPS_CR16:	return DISPLAY_IS_PAL ? 13.00f: 15.01f;
		case NF_FPS_CR08:	return 7.50f;
		case NF_FPS_CR04:	return 3.75f;
		case NF_FPS_CR02:	return 1.87f;
		case NF_FPS_CR01:	return 0.94f;
		case NF_FPS_CR00:	return 0.0f;
	}
	__A(0,"wrong fps.");
	return 0;
}

static void
enc_check_stm_size(gint ch, guint fps, guint frame_type, guint stm_len)
{
    gfloat sec;
    
    g_return_if_fail(ch<NUM_REC_CHAN);
    //g_return_if_fail(stm_len<ANF_MAX_STM_LEN);
    
    if( frame_type==NF_FRAME_TYPE_I ) {
        enc_stm_len_I[ch]+=stm_len;
        enc_stm_cnt_I[ch]+=1;
    }
    else if( frame_type==NF_FRAME_TYPE_P ) {
        enc_stm_len_P[ch]+=stm_len;
        enc_stm_cnt_P[ch]+=1;
    }
    else {
        g_assert(0);
    }
    
    enc_stm_len_A[ch]+=stm_len;
    enc_stm_cnt_A[ch]+=1;
    
    sec = ((float)enc_stm_cnt_A[ch])/_enc_get_real_fps(fps);
    
    //if(ch==0) {
    //    g_message(  "total stream len:%d,%f,total frame cnt:%d, sec:%f, fps:%f.", 
    //                enc_stm_len_A[ch], (float)enc_stm_len_A[ch], enc_stm_cnt_A[ch], sec, _enc_get_real_fps(fps));
    //}
    
    enc_stm_len_Bps[ch] = (float)enc_stm_len_A[ch]/sec;
}

#define NUM_CH_PER_ENC          (8)
#define ENC_CH_PER_THREAD_MASK  (0xFF)

static volatile guint enc_cat_stm_log = 0;
static void
enc_cat_stm_len_log( guint thread_id, guint *ch_mask, gint delta )
{
    guint i, ch, clear;
    guint ch_base = thread_id*NUM_CH_PER_ENC, mask;
    guint shift = thread_id*NUM_CH_PER_ENC;
    guint enc_ch_per_thread_maks = ENC_CH_PER_THREAD_MASK;
    
    mask = (enc_ch_per_thread_maks<<shift)&0xFFFF;
	
    ch = (*ch_mask)&mask;
    if( !ch )
        return;
        
    clear = ( (*ch_mask)>>16 ) & mask;
    
    //*ch_mask &= (~mask);

    __T(0, " enc_ch_per_thread_maks:0x%08x, shift:%d, ch:0x%08x, mask:0x%08x,  clear:0x%08x, *ch_mask:0x%08x. ch_abse:%d",
            enc_ch_per_thread_maks, shift, ch, mask, clear, *ch_mask, ch_base);
    
	printf("\n============================== avg. stream size/%dms ==============================", delta);
	for(i = ch_base; i< (ch_base+NUM_CH_PER_ENC); i++) {
        // print statistic
        if( GETBIT(ch, i) ) {
    		printf("\nch:%2d, %06.2fKbps,I:%7d/%3d,P:%7d/%3d,Total size:%8d,I Total cnt:%4d", 
    		        i+1,
    		        enc_stm_len_Bps[i]*8/Kbps,
    		        enc_stm_cnt_I[i]? enc_stm_len_I[i]/enc_stm_cnt_I[i]: 0,
    		        enc_stm_cnt_I[i],
    		        enc_stm_cnt_P[i]? enc_stm_len_P[i]/enc_stm_cnt_P[i]: 0,
    		        enc_stm_cnt_P[i],
                    enc_stm_len_A[i],
    		        enc_stm_cnt_A[i] );
		}

		// clear statistic
        if( GETBIT(clear, i) || 1) {
            enc_stm_len_I[i] = 0;
            enc_stm_len_P[i] = 0;
            enc_stm_len_A[i] = 0;
            enc_stm_cnt_I[i] = 0;
            enc_stm_cnt_P[i] = 0;
            enc_stm_cnt_A[i] = 0;
            enc_stm_len_Bps[i] = 0;
        }
	}
	printf("\n");
	//printf("\n=========================== avg. stream legnth ===========================\n");
	
	*ch_mask = 0;
	
	return;
}

static guint g_start_enc_sec = 0;

static glong
_get_gtime_diff_msec(GTimeVal gt1, GTimeVal gt2)
{
    GTimeVal gdiff;
    glong msec;

    gdiff.tv_sec    = gt1.tv_sec  - gt2.tv_sec;
    gdiff.tv_usec   = gt1.tv_usec - gt2.tv_usec;

    msec = gdiff.tv_sec*1000+(gdiff.tv_usec/1000);

    __T(0, "[ENC_CHECK] diff:%ld.", msec);
    return msec;
}
#endif  /**/

static gboolean
_check_sps(const guchar *stream)
{
    guchar val;
    
    val = stream[4];
    if( (val&0x1F) == NALU_TYPE_SPS )
        return TRUE;
        
    return FALSE;
}

static gboolean
is_idr(const guchar *stream)
{
    guchar val;
    
    val = stream[4];
    if( (val&0x1F) == NALU_TYPE_IDR )
        return TRUE;
        
    return FALSE;
}

static gint
nal_unit_type(const guchar *stream)
{
    gint val;
    
    val = (gint)(stream[4]&0x1F);
    return val;
}

static guint 
_get_gop_tbl_idx(guint fps)
{
	switch(fps)	{
		case NF_FPS_CR32:	return 0;
		case NF_FPS_CR16:	return 1;
		case NF_FPS_CR08:	return 2;
		case NF_FPS_CR04:	return 3;
		case NF_FPS_CR02:	return 4;
		case NF_FPS_CR01:	return 5;
		case NF_FPS_CR00:	return 6;
	}
    //__A(0,"wrong fps.");
    __E("wrong fps: %d", fps);
	return 0;
}
static int
_convert_omx_fps_to_gop( int ch, unsigned int fps )
{
    int val;

	if( ch < NUM_ACTIVE_CH )
	{
	    switch( fps ) {								
        		case FPS_30:  val = 30; break;
        		case FPS_29:  val = 29; break;									
        		case FPS_28:  val = 28; break;
        		case FPS_27:  val = 27; break;									
        		case FPS_26:  val = 26; break;
        		case FPS_25:  val = 25; break;									
        		case FPS_24:  val = 24; break;
        		case FPS_23:  val = 23; break;									
        		case FPS_22:  val = 22; break;
        		case FPS_21:  val = 21; break;									
        		case FPS_20:  val = 20; break;
        		case FPS_19:  val = 19; break;									
        		case FPS_18:  val = 18; break;
        		case FPS_17:  val = 17; break;						
        	       case FPS_16:  val = 16; break;
        		case FPS_15:  val = 15; break;			
        		case FPS_14:  val = 14; break;
        		case FPS_13:  val = 13; break;			
        		case FPS_12:  val = 12; break;
        		case FPS_11:  val = 11; break;			
        		case FPS_10:  val = 10; break;
        		case FPS_09:  val = 9; break;
                     case FPS_08:  val = 8; break;
        		case FPS_07:  val = 7; break;
        		case FPS_06:  val = 6; break;
        	       case FPS_05:  val = 5; break;			
        	       case FPS_04:  val = 4; break;
        	       case FPS_03:  val = 3; break;			
        	       case FPS_02:  val = 2; break;
        	       case FPS_01:  val = 1; break;
        	       case FPS_00:
        	       default:
        	            val = DISPLAY_IS_PAL ?  25: 30; 
        	            break;
	    }
	}
	else
	{
	    switch( fps ) {
	        //case FPS_32:  val = CR16; break;
			//case FPS_31:  val = CR16; break;									
			case FPS_30:  val = 15; break;
			case FPS_29:  val = 15; break;									
			case FPS_28:  val = 14; break;
			case FPS_27:  val = 14; break;									
			case FPS_26:  val = 13; break;
			case FPS_25:  val = 13; break;									
			case FPS_24:  val = 12; break;
			case FPS_23:  val = 12; break;									
			case FPS_22:  val = 11; break;
			case FPS_21:  val = 11; break;									
			case FPS_20:  val = 10; break;
			case FPS_19:  val = 10; break;									
			case FPS_18:  val = 9; break;
			case FPS_17:  val = 9; break;						
	              case FPS_16:  val = 8; break;
			case FPS_15:  val = 8; break;			
			case FPS_14:  val = 7; break;
			case FPS_13:  val = 7; break;			
			case FPS_12:  val = 6; break;
			case FPS_11:  val = 6; break;			
			case FPS_10:  val = 5; break;
			case FPS_09:  val = 5; break;
	              case FPS_08:  val = 4; break;
			case FPS_07:  val = 4; break;
			case FPS_06:  val = 3; break;
        	       case FPS_05:  val = 3; break;			
        	       case FPS_04:  val = 2; break;
        	       case FPS_03:  val = 2; break;			
        	       case FPS_02:  val = 1; break;
        	       case FPS_01:  val = 1; break;
        	       case FPS_00:				
        	       default:
        	            __E("inalid fps:%d!\n\n", fps);
        	            val = 16;
        	            break;
	    }	
	}
    __T(0, "gop :%d", val);
    return val;
}


static int
_convert_fps_to_gop( int ch, int fps )
{
    int val;

	if( ch < NUM_ACTIVE_CH )
	{
	    switch( fps ) {
	        case NF_FPS_CR32:  val = DISPLAY_IS_PAL ?  25: 30; break;
			case NF_FPS_CR31:  val = 30; break;									
			case NF_FPS_CR30:  val = 30; break;
			case NF_FPS_CR29:  val = 29; break;									
			case NF_FPS_CR28:  val = 28; break;
			case NF_FPS_CR27:  val = 27; break;									
			case NF_FPS_CR26:  val = 26; break;
			case NF_FPS_CR25:  val = 25; break;									
			case NF_FPS_CR24:  val = 24; break;
			case NF_FPS_CR23:  val = 23; break;									
			case NF_FPS_CR22:  val = 22; break;
			case NF_FPS_CR21:  val = 21; break;									
			case NF_FPS_CR20:  val = 20; break;
			case NF_FPS_CR19:  val = 19; break;									
			case NF_FPS_CR18:  val = 18; break;
			case NF_FPS_CR17:  val = 17; break;						
	              case NF_FPS_CR16:  val = DISPLAY_IS_PAL ?  13: 15; break;
			case NF_FPS_CR15:  val = 15; break;			
			case NF_FPS_CR14:  val = 14; break;
			case NF_FPS_CR13:  val = 13; break;			
			case NF_FPS_CR12:  val = 12; break;
			case NF_FPS_CR11:  val = 11; break;			
			case NF_FPS_CR10:  val = 10; break;
			case NF_FPS_CR09:  val = 9; break;
	              case NF_FPS_CR08:  val = 8; break;
			case NF_FPS_CR07:  val = 7; break;
			case NF_FPS_CR06:  val = 6; break;
        	       case NF_FPS_CR05:  val = 5; break;			
        	       case NF_FPS_CR04:  val = 4; break;
        	       case NF_FPS_CR03:  val = 3; break;			
        	       case NF_FPS_CR02:  val = 2; break;
        	       case NF_FPS_CR01:  val = 1; break;
        	       case NF_FPS_CR00:
        	       default:
        	            val = DISPLAY_IS_PAL ?  25: 30; 
        	            break;
	    }
	}
	else
	{
	    switch( fps ) {
	        case NF_FPS_CR32:  val = 16; break;
			case NF_FPS_CR31:  val = 16; break;									
			case NF_FPS_CR30:  val = 15; break;
			case NF_FPS_CR29:  val = 15; break;									
			case NF_FPS_CR28:  val = 14; break;
			case NF_FPS_CR27:  val = 14; break;									
			case NF_FPS_CR26:  val = 13; break;
			case NF_FPS_CR25:  val = 13; break;									
			case NF_FPS_CR24:  val = 12; break;
			case NF_FPS_CR23:  val = 12; break;									
			case NF_FPS_CR22:  val = 11; break;
			case NF_FPS_CR21:  val = 11; break;									
			case NF_FPS_CR20:  val = 10; break;
			case NF_FPS_CR19:  val = 10; break;									
			case NF_FPS_CR18:  val =  9; break;
			case NF_FPS_CR17:  val =  9; break;						
	              case NF_FPS_CR16:  val =  8; break;
			case NF_FPS_CR15:  val =  8; break;			
			case NF_FPS_CR14:  val =  7; break;
			case NF_FPS_CR13:  val =  7; break;			
			case NF_FPS_CR12:  val =  6; break;
			case NF_FPS_CR11:  val =  6; break;			
			case NF_FPS_CR10:  val =  5; break;
			case NF_FPS_CR09:  val =  5; break;
	              case NF_FPS_CR08:  val =  4; break;
			case NF_FPS_CR07:  val =  4; break;
			case NF_FPS_CR06:  val =  3; break;
        	       case NF_FPS_CR05:  val =  3; break;			
        	       case NF_FPS_CR04:  val =  2; break;
        	       case NF_FPS_CR03:  val =  2; break;			
        	       case NF_FPS_CR02:  val =  1; break;
        	       case NF_FPS_CR01:  val =  1; break;
        	       case NF_FPS_CR00:		
        	       default:
        	            __E("inalid fps:%d!\n\n", fps);
        	            val = 16;
        	            break;
	    }	
	}
    __T(0, "gop :%d", val);
    return val;
}

static int
_get_gop_interval(guint fps)
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:   val = CR16;    break;
        case NF_FPS_CR16:   val = CR08;    break;
        case NF_FPS_CR08:   val = CR04;    break;
        case NF_FPS_CR04:   val = CR02;    break;
        case NF_FPS_CR02:   val = CR01;    break;
        case NF_FPS_CR01:   val = CR01;    break;
        case NF_FPS_CR00:   val = CR01;    break;
        default:
            __E("inalid fps:%d!\n\n", fps);
            val = 16;
            break;
    }
    
    return val;
}


static int
_convert_fps( int fps )
{
    int val;
    
    switch( fps ) {
        case FPS_30:  val = NF_FPS_CR32; break;		
        case FPS_29:  
        case FPS_28:  
        case FPS_27:  
        case FPS_26:  			
 	 case FPS_25:  
 	 case FPS_24:  
 	 case FPS_23:  
 	 case FPS_22:  
 	 case FPS_21:  
 	 case FPS_20:
 	 case FPS_19:
 	 case FPS_18:
 	 case FPS_17:
 	 case FPS_16:
 	 case FPS_15:
 	 case FPS_14:
 	 case FPS_13:
 	 case FPS_12:
 	 case FPS_11:  
 	 case FPS_10:
 	 case FPS_09:  val = NF_FPS_CR16; break;
	 case FPS_08:
        case FPS_07:  
	 case FPS_06:
	 case FPS_05:  val = NF_FPS_CR08; break;
        case FPS_04:  
	 case FPS_03:  val = NF_FPS_CR04; break;
        case FPS_02:  val = NF_FPS_CR02; break;
        case FPS_01:  val = NF_FPS_CR01; break;
        case FPS_00:
        default:
            val = NF_FPS_CR01; 
            __E("fail to convert frame rate! %d/%d", fps, val);
            break;
    }
    return val;
}

int
convert_omx_fps( gint fps )
{
    int val;
    
    switch( fps ) {
        case CR32:  val = DISPLAY_IS_PAL ? FPS_25: FPS_32; break;
        case CR16:  val = DISPLAY_IS_PAL ? FPS_13: FPS_16; break;
        case CR08:  val = FPS_08; break;
        case CR04:  val = FPS_04; break;
        case CR02:  val = FPS_02; break;
        case CR01:  val = FPS_01; break;
        case CR00:
        default:
            val = FPS_01; 
            break;
    }
    return val;
}

static guint 
_get_timesync_margin(guint fps)
{
    guint val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val =   40*mSEC; break;
        case NF_FPS_CR16:  val =   80*mSEC; break;
        case NF_FPS_CR08:  val =  160*mSEC; break;
        case NF_FPS_CR04:  val =  320*mSEC; break;
        case NF_FPS_CR02:  val =  640*mSEC; break;
        case NF_FPS_CR01:  val = 1100*mSEC; break;
        case NF_FPS_CR00:
        default:
            val = 40*mSEC; 
            break;
    }
    __T(0, "fps:%d", val);
    return val;
}

static int
_copy_stream(void *h_edma, guchar *dst, guchar *src, guint size)
{    
    memcpy(dst, src, size);
    return 0;
}

static GObject *
_enc_alloc_buffer(guint size, gint verbose)
{
    GobjBuddyBuffer *gobj_buf = NULL;
    ICODEC_HEADER   *header;
    guint           addr;
    
    g_return_val_if_fail(size!=0, NULL);
    
    gobj_buf = gobj_buddy_buffer_new_malloc(size);
    if( !gobj_buf ) {
	    __E("fail to allocate fb!!!\n");
        return NULL;
    }
    
    header = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(G_OBJECT(gobj_buf));
    if( header && verbose ) {
		addr = ICMEM_getBufferPhysicalAddress(header);
		__T(CAT_A, "alloc enc buffer. physical address: 0x%x, size: %d.", addr, size);
    }
    
    header->gst_buffer  = gobj_buf;
    header->reserved    = 0;
    #if 0    
    gobj_buf->frame      = header;
    gobj_buf->cmemq      = 0;
    gobj_buf->free_cb    = 0;
    #endif
        
    return gobj_buf;
}

static gboolean
enc_free_buffer(GobjBuddyBuffer *gobj_buf, gint verbose)
{
    guint       addr;
    gpointer    buf_data;
    guint       buf_size;

    g_return_val_if_fail(gobj_buf, FALSE);
    
    if( verbose ) {
        buf_data = gobj_buddy_buffer_buf_get_addr(gobj_buf);
        buf_size = gobj_buddy_buffer_buf_get_size(gobj_buf);
		addr = ICMEM_getBufferPhysicalAddress(buf_data);
		__T(CAT_A, "free enc buffer. physical address: 0x%x, size: %d.", addr, buf_size);
    }
    g_object_unref(gobj_buf);
    
    return TRUE;
}

static void
_init_codec_header(ICODEC_HEADER *header, const NF_REC_INFO_T *rec_info)
{
    int vid_std;
    
    header->chan		= (guchar)rec_info->channel_id;    
    header->version		= 1;
    header->frame_rate	= (guchar)rec_info->fps;
    header->resolution	= (guchar)rec_info->res;
    header->codec		= (guchar)rec_info->codec;
    header->flags		= (guchar)rec_info->flags;
}

static void
_send_ctrl_frame(const NF_REC_INFO_T *rec_cfg, gint frame_type, guchar flags)
{     
}

static void
_process_ctrl_frame( const NF_REC_INFO_T *rec_cfg, const NF_ENC_FLAG_T *enc_flag )
{
    NF_CTRL_FLAG_E cur_RP, new_RP;
    gint    ch = rec_cfg->channel_id;
    guchar  flags;

    flags   = enc_flag->flags;
    cur_RP  = enc_flag->RP[CUR];
    new_RP  = enc_flag->RP[NEW];
    __T(CAT_C, "ch:%d, RP(cur/new): %d/%d", ch, cur_RP, new_RP);

    switch(cur_RP) {
        case RP00:
            switch(new_RP) {
                case RP00:	/* ignore, do nothing. */
                    break;
                case RP01:
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_START, flags);
                    break;
                case RP10:
                case RP11:
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_START, flags);
                    break;
                default:
                    __A(0, "bad case."); break;
            }
            break;
        case RP01:
            switch(new_RP) {
                case RP00:
                    _send_ctrl_frame(rec_cfg, 
                                    NF_FRAME_TYPE_END, END_FLAG_DISCARD_PREREC);
                    break;
                case RP01:
                case RP10:
                    _send_ctrl_frame(rec_cfg,	
                                    NF_FRAME_TYPE_END, END_FLAG_DISCARD_PREREC);
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_START, flags);
                    break;
                case RP11:
                    _send_ctrl_frame(rec_cfg,
                                    NF_FRAME_TYPE_END, END_FLAG_FLUSH_PREREC );
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_START, flags);
                    break;
                default:
                    __A(0, "bad case."); break;
            }
            break;
        case RP10:
        case RP11:
            switch(new_RP) {
                case RP00:
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_END, END_FLAG_NONE);
                    break;
                case RP01:
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_END, END_FLAG_NONE);
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_START, flags);
                    break;
                case RP10:
                case RP11:
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_END, END_FLAG_NONE);
                    _send_ctrl_frame(rec_cfg, NF_FRAME_TYPE_START, flags);
                    break;
                default:
                    __A(0, "bad case."); break;
            }
            break;
        default:
            __A(0, "bad case."); break;
    }   /*switch(cur_RP)*/
}   /*process_ctrl_frame(...)*/

static guchar 
check_to_record(guchar flags)
{
    gint rec_reason;

    rec_reason = GET_REC_REASON(flags);
    if(	rec_reason == NF_RECORD_REASON_TIMER    ||
        rec_reason == NF_RECORD_REASON_ALARM    ||
        rec_reason == NF_RECORD_REASON_MOTION   ||
        rec_reason == NF_RECORD_REASON_USER     ||
        rec_reason == NF_RECORD_REASON_MANUAL )
    {	
        return 1;
    }
    
    return 0;
}

static NF_CTRL_FLAG_E 
get_RP_flag(guchar flags)
{
    NF_CTRL_FLAG_E rp_flag;
    gint          p, r;
	
    /* get pre-rec flag */
    p = GET_REC_PREREC_FLAG(flags)? 1: 0;

	/* get rec flag */
    r = check_to_record(flags)? 1: 0;

    /* combine pre-rec flag & rec flag */
    rp_flag = (NF_CTRL_FLAG_E)( (r<<1)|(p) );
    return rp_flag;
}

static gboolean 
_compare_rec_param(const NF_REC_INFO_T *curr, const NF_REC_INFO_T *new)
{
#if 0
	/* check prohibit condition */
	if(curr->channel_id != new->channel_id)
		NF_ASSERT("",0);
#endif

    /* compare recording parameters */
    if(	(curr->quality  != new->quality)    ||
        (curr->bit_rate != new->bit_rate)   ||
        (curr->fps      != new->fps)        ||
        (curr->codec    != new->codec)      ||
        (curr->res      != new->res)        ||
        (curr->i_interval != new->i_interval) )

    {
        return TRUE;
    }

    return FALSE;		/* no change */
}

static /*inline*/ void 
_init_rec_param(NF_REC_INFO_T *curr, const NF_REC_INFO_T *new)
{
    curr->channel_id= new->channel_id;
    curr->quality   = new->quality;
    curr->bit_rate  = new->bit_rate;
    curr->fps       = new->fps;
    curr->codec     = new->codec;
    curr->res       = new->res;
    curr->i_interval = new->i_interval;
}

static void 
_init_RP_flag( NF_ENC_FLAG_T *enc_flag, guchar flags )
{
    enc_flag->RP[NEW]   = get_RP_flag(flags);
    enc_flag->pre       = (guchar)(GET_REC_PREREC_FLAG(flags)? 1: 0);
    enc_flag->rec       = (guchar)(check_to_record(flags)? 1: 0);
    enc_flag->flags     = flags;	/* to senc ctrl frame*/
}

static /*inline*/ gboolean 
_compare_rec_flag(guint flags_cur, guint flags_new)
{
    if( (flags_cur>>1) != (flags_new>>1) )	/* mask network flag */
    	return TRUE;	/* changed */
    	
    return FALSE;		/* no change */
}

#define NF_VID_W_1024P          (1024)
#define NF_VID_H_1024P          (1536)

#define NF_VID_W_1280P          (1280)
#define NF_VID_H_1280P          (1440)

static guint
_get_resolution( guint w, guint h )
{
    guint resolution;

    if(w==NF_VID_W_8M && h==NF_VID_H_8M) {
        resolution = NF_RES_3840x2160;
    }
    else if(w==NF_VID_W_8M_EXC && h==NF_VID_H_8M) {
        resolution = NF_RES_2560x2160;
    }    
    else if(w==NF_VID_W_5M && h==NF_VID_H_5M) {
        resolution = NF_RES_2592x1944;
    }
    else if(w==NF_VID_W_4M && h==NF_VID_H_4M) {
        resolution = NF_RES_2688x1520;
    }
    else if(w==NF_VID_W_5M_EXC && h==NF_VID_H_5M) {
        resolution = NF_RES_2560x1944;
    }
    else if(w==NF_VID_W_4M_EXC && h==NF_VID_H_4M) {
        resolution = NF_RES_2560x1520;
    }
    else if(w==NF_VID_W_3_7M && h==NF_VID_H_3_7M) {
        resolution = NF_RES_2560x1440;
    }		
    else if(w==NF_VID_W_3M && h==NF_VID_H_3M) {
        resolution = NF_RES_2048x1536;
    }
    else if(w==NF_VID_W_2_8M && h==NF_VID_H_2_8M) {
        resolution = NF_RES_1920x1536;
    }
    else if(w==NF_VID_W_1080P && h==NF_VID_H_1080P) {
        resolution = NF_RES_1920x1080;
    }
    else if(w==NF_VID_W_720P && h==NF_VID_H_720P) {
        resolution = NF_RES_1280x720;
    }
    else if(w==NF_VID_W_360P && h==NF_VID_H_360P) {
        resolution = NF_RES_640x360;
    }
    else if(w==NF_VID_W_960_NTSC && h==NF_VID_H_960_NTSC) {
        resolution = NF_RES_960H_NTSC_4CIFP;
    }
    else if(w==NF_VID_W_4CIF_NTSC && h==NF_VID_H_4CIF_NTSC) {
        resolution = NF_RES_NTSC_4CIFP;
    }
    else if(w==NF_VID_W_2CIF_NTSC && h==NF_VID_H_2CIF_NTSC) {
        resolution = NF_RES_NTSC_2CIF;
    }
    else if(w==NF_VID_W_1CIF_NTSC && h==NF_VID_H_1CIF_NTSC) {
        resolution = NF_RES_NTSC_CIF;
    }
    else if(w==NF_VID_W_960_PAL && h==NF_VID_H_960_PAL) {
        resolution = NF_RES_960H_PAL_4CIFP;
    }
    else if(w==NF_VID_W_4CIF_PAL && h==NF_VID_H_4CIF_PAL) {
        resolution = NF_RES_PAL_4CIFP;
    }
    else if(w==NF_VID_W_2CIF_PAL && h==NF_VID_H_2CIF_PAL) {
        resolution = NF_RES_PAL_2CIF;
    }
    else if(w==NF_VID_W_1CIF_PAL && h==NF_VID_H_1CIF_PAL) {
        resolution = NF_RES_PAL_CIF;
    }
    else if(w==NF_VID_W_2160P && h==NF_VID_H_2160P) {
        resolution = NF_RES_1920x2160;
    }
    else if(w==NF_VID_W_1344P && h==NF_VID_H_1344P) {
        resolution = NF_RES_1344x1520;
    }
    else if(w==NF_VID_W_1296P && h==NF_VID_H_1296P) {
        resolution = NF_RES_1296x1944;
    }
    else if(w==NF_VID_W_1280P && h==NF_VID_H_1280P) {
        resolution = NF_RES_1280x1440;
    }
    else if(w==NF_VID_W_1024P && h==NF_VID_H_1024P) {
        resolution = NF_RES_1024x1536;
    }	
    else if(w==NF_VID_W_1024P && h==NF_VID_H_1024P) {
        resolution = NF_RES_1024x768;
    }	
    else if(w==NF_VID_W_640P && h==NF_VID_H_640P) {
        resolution = NF_RES_640x480;
    }	    
    else if(w==NF_VID_W_5M_HALF && h==NF_VID_H_5M_HALF) {
        resolution = NF_RES_1280x1944;
    }	    
    else if(w==NF_VID_W_4M_HALF && h==NF_VID_H_4M_HALF) {
        resolution = NF_RES_1280x1520;
    }	    
    else if(w==NF_VID_W_8M_HALF && h==NF_VID_H_8M_HALF) {
        resolution = NF_RES_1280x2160;
    }	            
    else {
        __E("wrong input w:%d/h:%d!", w, h);
        resolution = NF_RES_NTSC_NONE;
    }
    __T(CAT_C, "resolution:%d", resolution);
    
    return resolution;
}

static int
_check_frametype( gint ideal, gint real )
{
    if(ideal!=real)
        return -1;
        
    return 0;
}

typedef enum _NF_TS_DIR_E {
    NF_TS_DIR_BACK      = 0,
    NF_TS_DIR_FUTURE    = 1,
    NF_TS_DIR_NUM
}NF_TS_DIR_E;

static gint
_check_and_skip_put_frame(  NF_ENC_CH_INFO_T    *ch_info,
                            NF_REC_INFO_T       *rec_cfg,
                            GOBJEncodeCbParam    *data )
{
    guint64 diff;
    guint   ts0, tsl0, ts1, tsl1;
    gint    dir;
    
    if( data->timestamp<=ch_info->timestamp ) {
        diff = ch_info->timestamp-data->timestamp;
        dir  = NF_TS_DIR_BACK;
    }
    else {
        diff = data->timestamp-ch_info->timestamp;
        dir  = NF_TS_DIR_FUTURE;
    }
    
    if( dir==NF_TS_DIR_BACK ) {
        ts0     = (guint)(ch_info->timestamp/100);      //msec->sec
        tsl0    = (guint)((ch_info->timestamp%1000)/5); //msec->5msec
        ts1     = (guint)(data->timestamp/100);         //msec->sec
        tsl1    = (guint)((data->timestamp%1000)/5);    //msec->5msec
        
        __T(CAT_A, "timestamp reversed. ch:%d,cur[%u,%u],new[%u,%u],diff:%lld",
            ch_info->ch, ts0, tsl0, ts1, tsl1, diff);
        return 1;   //skip
    }
    
    return 0;
}


#define DMA_TRANSFER_SIZE (0xffc<<2)

static GObject *
_pack_stream( NF_ENC_CH_INFO_T *ch_info, GOBJEncodeCbParam *data, guchar *stream_buf)
{
    GObject       *gobj_buf = NULL;

    ICODEC_HEADER	*stm_head;
    guchar          *stm_buf, *stm_body;
    guint			 stm_len;
    gint ret;

    gint alloc_retry = 0;

    #if 0   //debug function
    enc_check_max_stm_size(ch_info->ch, data->frame_size);
    
    enc_check_stm_size(ch_info->ch, _convert_fps(data->frame_rate), data->frame_type, data->frame_size);
    #endif

	/* allocate buffer pack to */
    stm_len = data->frame_size;
    gobj_buf = (GObject *)stream_buf;
    if( gobj_buf==NULL ) {
        __E("fail to alloc. stm buf. ch:%2d/size:%d!!!", 
                data->chan, stm_len+sizeof(ICODEC_HEADER));
        return NULL;
    }
        
    /* copy stream */
    stm_buf     = gobj_buddy_buffer_buf_get_addr(gobj_buf);
    stm_head    = (ICODEC_HEADER *)stm_buf;
    stm_body    = stm_buf+sizeof(ICODEC_HEADER);
    __T(CAT_C, "no func. to copy stream! done by cpu."); 
	/* set codec header */
    _init_codec_header( stm_head, &(ch_info->rec_cfg) );
    stm_head->frame_type    = (guchar)data->frame_type;
    stm_head->frame_size    = stm_len;
//  stm_head->timestamp     = (guint)(data->timestamp/1000);        //msec->sec
//  stm_head->timestampl    = (guchar)((data->timestamp%1000)/5);   //msec->5msec
    stm_head->timestamp     = (guint)(data->timestamp/1000000);
    stm_head->timestampl    = (guchar)((data->timestamp%1000000)/5000);
    
    /* update curretn timestamp */
    ch_info->timestamp = data->timestamp;
    
    #if 1   //verbose
	//if(stm_head->chan==0)
    __T(0, "enc done. ch:%d/codec:%d/flags:0x%02x/ver.%d/fs:%d/res:%d/fr:%d/ft:%d/ts:%d/tsl:%d/0x%08x", 
                stm_head->chan, stm_head->codec,
                stm_head->flags,stm_head->version,
                stm_head->frame_size, stm_head->resolution,
                stm_head->frame_rate, stm_head->frame_type,
                stm_head->timestamp,  stm_head->timestampl, gobj_buf);
    #endif
    
    return gobj_buf;
}

static void
_init_enc_context( void )
{
    int ch;

    #if 0    
    for(ch = 0; ch<NUM_ACTIVE_CH; ch++) {
        g_enc_status[ch].gop_idx = 0;
        g_enc_status[ch].gop_idx = 0;
    }
    #endif
    
    memset(&g_enc_ctxt, 0, sizeof(g_enc_ctxt));
    for(ch=0; ch<NUM_REC_CHAN; ch++) {
        g_enc_ctxt.ch_info[ch].ch = ch;
    }
}

static int 
_svc_t(int ch, int frameType, int step)
{
	static int cnt[32] = {0,};

	if( frameType || cnt[ch] == step )
	{
		cnt[ch] = 0;
	}
	else
	{
		cnt[ch]++;
		return -1;
	}
	printf("* %d %d *\n", step, cnt[ch] );
	cnt[ch]++;

	return 0;
}	

static int
_flush_stream_to_file(gint ch, gchar *buf, guint size, guint fps, guint type)
{
    char filename[100];
	static int cnt[32] = {0,};
	static int num[32] = {0,};
	
	static FILE *fp_c[32] = {NULL,};
	static FILE *fp_svct_c  = NULL;
	
	char * frame;

#if 0   //sepereately
	//if(ch ==0)
	//printf("size %d timestamp %u (%d) !!!\n",frame_size,timestamp,timestampl);
	
    if( cnt<100 && ch==0) {
        snprintf(filename, (sizeof(filename)-1), "./data/%08d", cnt);
        fp_c = fopen(filename,"wb");
        fwrite(buf, 1, size, fp_c);
        fflush(fp_c);
        fclose(fp_c);
        cnt++;
    }

    if(cnt>=10 && 0) {
        ERRORTYPE ret;
        
        ret = H264ENC_SetDynamicParams(g_hDispmux->h_tunnel->pnEncHandle[0], 352, 240);
        if( ret!=ErrorNone ) {
            printf("\nERROR: fail to set dyn param! :%d", ret);
        }
    }
#endif

#if 1   //one
#define FLUSH_STREAM_CNT    (30*60*3)

    if(cnt[ch]==0) {
        snprintf(filename, (sizeof(filename)-1), "./data/one_ch%d_%02d.h264", ch, num[ch]);
        fp_c[ch] = fopen(filename,"wb+");
        g_assert(fp_c[ch]!=NULL);
        __T(1, "open file to write h.264 stream for ch:%d", ch);

        num[ch]++;        

		if( ch == 8 )
		{
			snprintf(filename, (sizeof(filename)-1), "./data/one_ch%d_%02d_svc_t.h264", ch, num[ch]);
	        fp_svct_c = fopen(filename,"wb+");
	        g_assert(fp_svct_c!=NULL);
	        __T(1, "open file to write h.264 svc-t stream for ch:%d", ch);
		}
    }

    if(cnt[ch]<FLUSH_STREAM_CNT) {
        fwrite(buf, 1 ,size, fp_c[ch]);
        __T(1, "write stream to file for ch:%d/cnt:%d/size:%d", ch, cnt[ch], size);

		if( ch == 8 ) 
		{
			if( _svc_t(ch, type, (fps>>1)) == 0 )
			{
		        fwrite(buf, 1 ,size, fp_svct_c);
		        __T(1, "write svc-t stream to file for ch:%d/cnt:%d/size:%d", ch, cnt[ch], size);			
			}
		}
    }
    
    if(cnt[ch]==FLUSH_STREAM_CNT) {
        fclose(fp_c[ch]);
		
		if( ch == 8 ) 
		{
	        fclose(fp_svct_c);			
			__T(1, "close file to write h.264 SVC-T stream for ch:%d/cnt:%d", ch, cnt[ch]);
		}
        __T(1, "close file to write h.264 stream for ch:%d/cnt:%d", ch, cnt[ch]);

        cnt[ch]     = 0;
        fp_c[ch]    = NULL;
        
        return 1;
    }
    cnt[ch]++;
        
    return 0;
#endif

}

static void
_dbg_check_ts_interval( NF_ENC_CH_INFO_T    *ch_info,
                        NF_REC_INFO_T       *rec_cfg,
                        GOBJEncodeCbParam    *data )
{
    guint64 diff;
    guint   ts0, tsl0, ts1, tsl1;
    gint    dir;
    
    if( data->timestamp<=ch_info->timestamp ) {
        diff = ch_info->timestamp-data->timestamp;
        dir  = 0;   //back
    }
    else {
        diff = data->timestamp-ch_info->timestamp;
        dir  = 1;   //future
    }
    
    if( (ch_info->timestamp!=0) && 
        (diff>_get_timesync_margin(rec_cfg->fps)) )
    {
        ts0     = (guint)(ch_info->timestamp/100);      //msec->sec
        tsl0    = (guint)((ch_info->timestamp%1000)/5); //msec->5msec
        ts1     = (guint)(data->timestamp/100);         //msec->sec
        tsl1    = (guint)((data->timestamp%1000)/5);    //msec->5msec

        __T(0, "time exceed boundary! ch:%d,dir:%d,diff:%lld,cur[%u,%u],new[%u,%u],", 
                ch_info->ch, dir, diff, ts0, tsl0, ts1, tsl1);
    }
    //ch_info->timestamp = data->timestamp;
}

void
_dbg_print_icodec_header(GObject *gobj_buf, gchar *s, const gchar *f, gint l)
{
    ICODEC_HEADER *stm_head = (ICODEC_HEADER *)(gobj_buddy_buffer_buf_get_addr(gobj_buf));
    
    g_return_if_fail(s!=NULL);
    g_return_if_fail(f!=NULL);

    //if( !(stm_head->flags) )
    __T(2, "[%s:%d]%s ch:%d,flag:0x%x,fps:%d,res:%d,ft:%d,ts:%d,tsl:%d,fs:%d,0x%08x",
        f, l, s,
        stm_head->chan, 
        (guint)stm_head->flags,
        stm_head->frame_rate,
        stm_head->resolution,
        stm_head->frame_type,
        stm_head->timestamp,
        stm_head->timestampl,
        stm_head->frame_size,
        (guint)stm_head->gst_buffer);
}

static void
_update_gop_info(GObject *gobj_buf, NF_ENC_CH_INFO_T *ch_info, guint frame_type)
{
    guint gop_info;
    ICODEC_HEADER  *stm_head;
    
    /* update gop info */
    if( frame_type==NF_FRAME_TYPE_I ) {
        ch_info->gop_idx = 0;
        
        if( ++ch_info->gop_toggle_cnt >= 65536 )
            ch_info->gop_toggle_cnt = 0;
    }
    else if( frame_type==NF_FRAME_TYPE_P ) {
        ch_info->gop_idx++;
    }
    else {
        __E("wrong frame type:%d!", frame_type);
        g_assert(0);
    }
    
    /* set gop info */
    if(!gobj_buf) {
        __E("gobj_buf for stream is NULL!");
        return;
    }
    
    if(!gobj_buddy_buffer_buf_get_addr(gobj_buf)) {
        __E("data of gobj_buf for stream is NULL!");
        return;
    }
    
    //printf("gop_toggle_cnt:%d/gop_idx:%d\n", ch_info->gop_toggle_cnt, ch_info->gop_idx);
    gop_info = (ch_info->gop_toggle_cnt<<16) | (ch_info->gop_idx);
    stm_head = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr(gobj_buf);
    stm_head->reserved = gop_info;
}

static int
_send_frame( NF_ENC_CH_INFO_T *ch_info, GOBJEncodeCbParam *data, guchar *stream_buf)
{
    GObject       *gobj_buf = NULL;
	ICODEC_HEADER   *header;
	static unsigned int rm_error_cnt = 0;
    
    // put stream to record manager
    gobj_buf = _pack_stream(ch_info, data, stream_buf);
    
    _update_gop_info(gobj_buf, ch_info, data->frame_type);
    
    DBG_ICODEC_HEADER(gobj_buf, "put", __FUNCTION__, __LINE__);

	header = (ICODEC_HEADER *)(gobj_buddy_buffer_buf_get_addr(gobj_buf));

    if( g_handoff_func != NULL ) {
        g_handoff_func(gobj_buf);
        
    } 

    if( gobj_buf != NULL )
        g_object_unref(gobj_buf);

    return 0;
}

static int _enc_check_codec_type(char *buf)
{
	int data=0, nal_type=0, i=0;

	i=0;
	data=0;

	while (i < 32) {
	
		if( (buf[i] == 0) && (buf[i+1] == 0) && (buf[i+2] == 0x01) ) {
			data = buf[i+3];
			break;
		}
		else if((buf[i] == 0) && (buf[i+1] == 0) && (buf[i+2] == 0) && (buf[i+3] == 0x01)) {
			data = buf[i+4];
			break;
		}
	
		i++;
	}
	
	nal_type = data & 0x1f;

	if(nal_type == 9) { 
		data=0;
		i=i+4;
		while (i < 64) { 
			if( (buf[i] == 0) && (buf[i+1] == 0) && (buf[i+2] == 0x01) ) {
				data = buf[i+3];
				break;
			}
			else if((buf[i] == 0) && (buf[i+1] == 0) && (buf[i+2] == 0) && (buf[i+3] == 0x01)) {
				data = buf[i+4];
				break;
			}
			i++;
		}

		nal_type = data & 0x1f;		
	}

	if((nal_type == 5) ||(nal_type == 7) || (nal_type == 8)) 
	{
		return IS_H264;
	}
	else if((nal_type > 0) && (nal_type<20)) 
	{
		return IS_H264;
	}

	nal_type = (data>>1) & 0x3f;
	
	if((nal_type == 32) ||(nal_type == 33) || (nal_type == 34)) 
	{
		return IS_H265;
	}

	return 0;
}

static int
_process_encode(    NF_ENC_CONTEXT_T *enc_ctxt,
                    GOBJEncodeCbParam *data, 
                    guchar *stream_buf )
{    
    NF_ENC_CH_INFO_T    *ch_info;
#ifdef REC_CH_USE_2ND_STREAM_
    NF_ENC_CH_INFO_T    *ch_info2;
    NF_ENC_CH_STATUS_T  *ch_status;
#endif  /*REC_CH_USE_2ND_STREAM_*/

    NF_REC_INFO_T   rec_cfg_new;
	NF_ENC_FLAG_T   *enc_flag;
    NF_REC_INFO_T   *rec_cfg;
    //NF_REC_FRAME    *rec_frame;    

	gint    ret, ch;
	guint   frame_type;
    gint    toggle_gop, send_ctrl_frame;
	guchar  enc_new = 0;
    static guint dump[DBG_NUM_CHAN]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   	//guint64         remained_msec;
    //guint           width, height;
    gint    skip = 0;
    guint codec = 0;
    
    g_return_val_if_fail(stream_buf!=NULL,  -1);
    g_return_val_if_fail(data!=NULL,        -1);
    g_return_val_if_fail(enc_ctxt!=NULL,    -1);

    ch = data->chan;
    if(ch>=NUM_ACTIVE_CH) {
        __E("ch num exceed!:%d", ch);
        return -1;
    }

    ch_info     = &enc_ctxt->ch_info[ch];
    ch_info->ch = ch;
    enc_flag    = &(ch_info->enc_flag);
    rec_cfg     = &(ch_info->rec_cfg);
    
    toggle_gop      = 0;
    send_ctrl_frame = 0;

    /* check change flag & update recording cfg. & set something */
    if( data->change ) {
        __T(CAT_C, "ch:%d, got new recording command.", ch);
        
        data->change = 0;   // clear current command
        
        rec_cfg_new.channel_id  = ch;
        rec_cfg_new.flags       = data->flags;
        rec_cfg_new.quality     = data->qp;             // TODO:
        rec_cfg_new.fps         = _convert_fps(data->frame_rate);
        rec_cfg_new.i_interval  = data->i_interval;
        rec_cfg_new.res         = _get_resolution(data->width, data->height);
        rec_cfg_new.bit_rate    = data->bit_rate;
        rec_cfg_new.codec       = (data->codec == 0) ? NF_CODEC_TYPE_H264MP : NF_CODEC_TYPE_H265;
        __T(CAT_A, "ch:%d/flags:0x%04x/qual:%d/br:%d/fps:%d/ii:%d/res:0x%04x/ft:%d/seq:%d",
                    ch,
                    rec_cfg_new.flags,
                    rec_cfg_new.quality, rec_cfg_new.bit_rate,
                    rec_cfg_new.fps,     rec_cfg_new.i_interval,
                    rec_cfg_new.res,
                    data->frame_type,
                    data->reserved );
                    
        /* step1. check rec/pre-rec flag */
        ret = _compare_rec_flag(rec_cfg->flags, rec_cfg_new.flags);
        if( ret ) {
            _init_RP_flag(enc_flag, (guchar)rec_cfg_new.flags);
            
            /* RP00 -> stop condition, needless to toggle */
            toggle_gop      = (enc_flag->RP[NEW]!=RP00? 1: 0);
            send_ctrl_frame = 1;
            __T(CAT_C, "ch:%d, RP updated. cur/new: %d/%d", 
                ch, enc_flag->RP[CUR], enc_flag->RP[NEW]);
        }
        
        /* step2. check network flag */
        enc_flag->net = (guchar)(GET_REC_NET_FLAG(rec_cfg_new.flags)? 1: 0);
        if(!enc_flag->enc && enc_flag->net)
            toggle_gop = 1;
            
        enc_new = enc_flag->net|enc_flag->pre|enc_flag->rec;
        
        /*  new recording param must be same as current one
         * when 'encoding'->'stop encoding' trans. so let's update */
        if( enc_flag->enc && !enc_new ) {
            _init_rec_param(&rec_cfg_new, rec_cfg);
        }
        
        enc_flag->enc = enc_new;              /* update new flag*/
        __A(!(enc_flag->rec && !enc_flag->enc), "ch:%d, record but not encode.", ch);
        __T(CAT_C, "ch:%d. new enc_flag->enc:%d", ch, enc_flag->enc);
        
        rec_cfg->flags  = rec_cfg_new.flags; 
        
        /* step3. check recording param. */
		ret = _compare_rec_param(rec_cfg, &rec_cfg_new);
		if( ret ) {
            _init_rec_param(rec_cfg, &rec_cfg_new);
            ch_info->fps_idx = _get_gop_tbl_idx(rec_cfg->fps);
            ch_info->gop_int = _convert_fps_to_gop(ch,rec_cfg->fps);
            
            toggle_gop |= (enc_flag->RP[NEW]!=RP00?1:0); /*RP00: stop condition*/
            send_ctrl_frame = 1;
            __T(CAT_C, "ch:%d, new rec info applied. res:%d/fps:%d/ftype:%d.",
                ch, rec_cfg->res, rec_cfg->fps, data->frame_type );
    		
            //dump[ch] = 1;   /* dump test stream encoded. TEST*/
		}		
	}
	
	if( toggle_gop ) {
        //ch_info->gop_idx = 0;
        __T(1, "ch:%d, gop has been toggled. size:%d/ts:%d/tsl:%d", 
            ch, data->frame_size, 
            (guint)(data->timestamp/1000), (guint)((data->timestamp%1000)/5));
        
        if( data->frame_type != NF_FRAME_TYPE_I ) {
            __T(CAT_C, "gop toggle condition. ch:%d/wrong ftype:%d!", ch, data->frame_type);
        }
	}
	
#if 0   //XXX
	if( dump[ch] ) {
    	ret = _flush_stream_to_file(ch, stream_buf, data->frame_size, _convert_omx_fps_to_gop(ch, data->frame_rate), data->frame_type);
    	if(ret==1) {
            dump[ch] = 0;    	
    	}
	}
#endif

#ifdef TIME_CHANGE_CHECK_TO_SEQ_NUM_
    //__T(1, "ch:%d/seq(old,new):%d,%d", ch, ch_info->seq_num, data->reserved);
    if( ch_info->seq_num != data->reserved ) {
        __T(CAT_A, "WARNING! timesync. mismatch. ch:%d/seq(old,new):%d,%d", 
                    ch, ch_info->seq_num, data->reserved);                    
        return 1;
    }
#endif  /**/
    
    if( data->frame_type==NF_FRAME_TYPE_NULL ) {
        //__E("wrong frame type! ch:%d/ftype:%d", ch, data->frame_type);
        //return -2;

        if( data->change ) {
            __T(1, "ctrl(+invalid video) frame! ch:%d", ch);
            return 3;
        }
        else {
	        __T(1, "wrong frame type! ch:%d/ftype:%d", ch, data->frame_type);
	        return 4;
        }
				
    }
    
    #if 1   //debug function
    _dbg_check_ts_interval(ch_info, rec_cfg, data);
    #endif

    /* put stream to record manager */
    g_assert(ch==ch_info->ch);
    _send_frame(ch_info, data, stream_buf);
    
    #if 0   //check gop validity
	ch_info->gop_idx+=ch_info->gop_int;
	if(ch_info->gop_idx >= GET_GOP_TBL_LEN(ch_info->fps_idx))
		ch_info->gop_idx = 0;
		
    __T(CAT_C, "xxxxx ch:%d/gop idx:%d/gop int:%d.", 
        ch, ch_info->gop_idx, ch_info->gop_int);
    #endif
    
    return 0;
}

static int g_fcnt[32] = {0, };

static void _check_frame_count(guint ch)
{
	struct timespec g_current_time; 
	static struct timespec g_start_time = { 0, }; 
	static guint g_ch_cnt[32] = {0, };
	static guint g_start_time_check = 1;
	gint i;
	guint delta;

	g_assert(ch < 32);

	++g_ch_cnt[ch];
		
	if(g_start_time_check) 
	{
		g_start_time_check = 0;
		clock_gettime(CLOCK_REALTIME, &g_start_time); 
	}

	clock_gettime(CLOCK_REALTIME, &g_current_time);

	delta = (g_current_time.tv_sec	- g_start_time.tv_sec)*1000.0 +  \
			(g_current_time.tv_nsec - g_start_time.tv_nsec)/1.0e6; 

	if( delta >= 10*SEC ) 
	{
		guint ch_mask = 0xFF;

		g_printf("%s %ds: ", __FUNCTION__, delta/SEC);
		for( i=0; i < 32; ++i )
			g_printf("%d,", g_ch_cnt[i]);
		g_printf("\n");

		memset(g_ch_cnt, 0, sizeof(g_ch_cnt));
		g_start_time_check = 1;
		
		if(enc_cat_stm_log)
			enc_cat_stm_len_log(0, &ch_mask, delta);
	}
}

static void _check_frame_ts(guint ch, guint fps, guint64 ts)
{
	static guint64 pre_ts[32] = { 0, };
	gint interval, gap;
	
	g_assert(ch < 32);
	g_assert(fps <= FPS_32);
	g_assert(ts > 0);

	if ( pre_ts[ch] == 0 )
	{
		pre_ts[ch] = ts;
		return;
	}

	interval = ts - pre_ts[ch];

	switch ( fps )
	{
		case FPS_32: gap = 50; break;	// 33msec
		case FPS_25: gap = 60; break;	// 40msec
		case FPS_16: gap = 100; break;	// 66msec
		case FPS_13: gap = 115; break;	// 77msec
		case FPS_08: gap = 200; break;	// 133msec
		case FPS_04: gap = 500; break;	// 333msec
		case FPS_02: gap = 750; break;	// 500msec
		case FPS_01: gap = 1500; break;	// 1sec
		default:
			g_assert(0);
	}

	if ( interval <= 0 )
		g_warning("%s ch%02d: TS is post[%d]. [%llu]->[%llu]", __FUNCTION__, ch, interval, pre_ts[ch], ts);
	else if ( interval >= gap )
		g_warning("%s ch%02d: TS is wrong[%d]. [%llu]->[%llu]", __FUNCTION__, ch, interval, pre_ts[ch], ts);

	pre_ts[ch] = ts;
		
}

int
encode_cb( GOBJEncodeCbParam *data, guchar *stream_buf )
{
    static gint init = 0;
    gint ret;
    
    g_return_val_if_fail(data!=NULL, -1);
    g_return_val_if_fail(data->chan!=100, -1);
    g_return_val_if_fail(data->frame_size!=0, -1);
	
    if(data==NULL || stream_buf==NULL) {
        __E("wrong param! 0x%08x/0x%08x", (gint)data, (gint)stream_buf);
        return -1;
    }
	__T(0, "enc callback!!! ft:%d", data->frame_type);
    
    if( !init ) {
        init = 1;
        _init_enc_context();
    }
    
	_check_frame_count(data->chan);

    //printf("ch:%d/size:%d/ts:%llu/ft:%d/%p\n",
    //        data->chan, data->frame_size, data->timestamp, data->frame_type, stream_buf);
//	_check_frame_ts(data->chan, data->frame_rate, data->timestamp );

	
    #if 0
    if(data->chan==0 && 1) {
        __T(1, "%d/%d/%d/%d/%d/%d/%d/%d/%d  %x\n",
                data->chan,
                data->flags,
                data->qp,
                data->frame_rate,
                data->i_interval,
                data->width,
                data->height,
                data->bit_rate,
                data->frame_type, stream_buf );
                
        __T(0, "%llu/%d/%d",
                 data->timestamp,
                 data->timestamp/1000,
                (data->timestamp%1000)/5);
    }
    #endif
    
    ret = _process_encode( &g_enc_ctxt, data, stream_buf );
    if( ret<0 ) {
        __E("fail to process encoded stream!");
    }
    
	return 0;
}

static int
_convert_to_omx_fps( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = DISPLAY_IS_PAL ?  FPS_25: FPS_30; break;
        case NF_FPS_CR31:  val = FPS_30; break;
        case NF_FPS_CR30:  val = FPS_30; break;		
        case NF_FPS_CR29:  val = FPS_29; break;
        case NF_FPS_CR28:  val = FPS_28; break;
        case NF_FPS_CR27:  val = FPS_27; break;
        case NF_FPS_CR26:  val = FPS_26; break;		
        case NF_FPS_CR25:  val = FPS_25; break;
        case NF_FPS_CR24:  val = FPS_24; break;
        case NF_FPS_CR23:  val = FPS_23; break;
        case NF_FPS_CR22:  val = FPS_22; break;
        case NF_FPS_CR21:  val = FPS_21; break;
        case NF_FPS_CR20:  val = FPS_20; break;
        case NF_FPS_CR19:  val = FPS_19; break;		
        case NF_FPS_CR18:  val = FPS_18; break;
        case NF_FPS_CR17:  val = FPS_17; break;		
        case NF_FPS_CR16:  val = DISPLAY_IS_PAL ?  FPS_13: FPS_15; break;
        case NF_FPS_CR15:  val = FPS_15; break;
        case NF_FPS_CR14:  val = FPS_14; break;		
        case NF_FPS_CR13:  val = FPS_13; break;
        case NF_FPS_CR12:  val = FPS_12; break;
        case NF_FPS_CR11:  val = FPS_11; break;
        case NF_FPS_CR10:  val = FPS_10; break;		
        case NF_FPS_CR09:  val = FPS_09; break;
#if 1
        case NF_FPS_CR08:  val = FPS_07; break;
#else		
        case NF_FPS_CR08:  val = FPS_08; break;
#endif		
        case NF_FPS_CR07:  val = FPS_07; break;
        case NF_FPS_CR06:  val = FPS_06; break;
        case NF_FPS_CR05:  val = FPS_05; break;
#if 1
        case NF_FPS_CR04:  val = FPS_05; break;		
#else		
        case NF_FPS_CR04:  val = FPS_04; break;
#endif		
        case NF_FPS_CR03:  val = FPS_03; break;
#if 1		
        case NF_FPS_CR02:  val = FPS_03; break;
#else		
        case NF_FPS_CR02:  val = FPS_02; break;
#endif		
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps:%d", val);
    return val;
}

static int
_convert_to_omx_fps_by_3M( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_18; break;
        case NF_FPS_CR16:  val = FPS_09; break;
        case NF_FPS_CR08:  val = FPS_04; break;
        case NF_FPS_CR04:  val = FPS_03; break;
        case NF_FPS_CR02:  val = FPS_02; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_3M:%d", val);
    return val;
}

#if 1
static int
_convert_to_omx_fps_by_4M( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_30; break;
        case NF_FPS_CR16:  val = FPS_15; break;
        case NF_FPS_CR08:  val = FPS_07; break;
        case NF_FPS_CR04:  val = FPS_05; break;
        case NF_FPS_CR02:  val = FPS_03; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_3M:%d", val);
    return val;
}

static int
_convert_to_omx_fps_by_4M_15FPS( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_15; break;
        case NF_FPS_CR16:  val = FPS_15; break;
        case NF_FPS_CR08:  val = FPS_07; break;
        case NF_FPS_CR04:  val = FPS_05; break;
        case NF_FPS_CR02:  val = FPS_03; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_3M:%d", val);
    return val;
}
#else
static int
_convert_to_omx_fps_by_4M( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_15; break;
        case NF_FPS_CR16:  val = FPS_08; break;
        case NF_FPS_CR08:  val = FPS_04; break;
        case NF_FPS_CR04:  val = FPS_03; break;
        case NF_FPS_CR02:  val = FPS_02; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_3M:%d", val);
    return val;
}
#endif

static int
_convert_to_omx_fps_by_5M( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_12; break;
        case NF_FPS_CR16:  val = FPS_06; break;
        case NF_FPS_CR08:  val = FPS_04; break;
        case NF_FPS_CR04:  val = FPS_03; break;
        case NF_FPS_CR02:  val = FPS_02; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_5M:%d", val);
    return val;
}

#if 1
static int
_convert_to_omx_fps_by_5M_20FPS( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_20; break;
        case NF_FPS_CR16:  val = FPS_10; break;
        case NF_FPS_CR08:  val = FPS_07; break;
        case NF_FPS_CR04:  val = FPS_05; break;
        case NF_FPS_CR02:  val = FPS_03; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_5M:%d", val);
    return val;
}
#else
static int
_convert_to_omx_fps_by_5M_20FPS( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_20; break;
        case NF_FPS_CR16:  val = FPS_10; break;
        case NF_FPS_CR08:  val = FPS_05; break;
        case NF_FPS_CR04:  val = FPS_04; break;
        case NF_FPS_CR02:  val = FPS_02; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_5M:%d", val);
    return val;
}
#endif

static int
_convert_to_omx_fps_by_real_fps( int fps )
{
    int val;

	if( fps == 0 )
	{
		val = 0;
		printf("[%s] ERR (FPS == 0)\n", __FUNCTION__);
	}
	else if( fps > 30 )
	{
		val = 30;
		printf("[%s] ERR (FPS > 30)\n", __FUNCTION__);		
	}
	else
	{
		val = fps;
	}

    __T(0, "real_fps:%d", val);
    return val;
}

static int
_convert_to_omx_fps_by_8M( int fps )
{
    int val;
    
    switch( fps ) {
        case NF_FPS_CR32:  val = FPS_15; break;
        case NF_FPS_CR16:  val = FPS_08; break;
        case NF_FPS_CR08:  val = FPS_04; break;
        case NF_FPS_CR04:  val = FPS_03; break;
        case NF_FPS_CR02:  val = FPS_02; break;
        case NF_FPS_CR01:  val = FPS_01; break;
        case NF_FPS_CR00:	
        default:
            val = FPS_01; 
            break;
    }
    __T(0, "fps_5M:%d", val);
    return val;
}

static int
_get_enc_size_from_res(guint res, guint *w, guint *h)
{
	switch( res )
	{
		case NF_RES_NTSC_CIF:   *w = 352; *h = 240; break;				
		case NF_RES_NTSC_2CIF:  *w = 704; *h = 240; break;				
		case NF_RES_NTSC_4CIF:	*w = 704; *h = 480;	break;			
		case NF_RES_NTSC_4CIFP:	*w = 704; *h = 480;	break;	
		case NF_RES_PAL_CIF:	*w = 352; *h = 288;	break;	
		case NF_RES_PAL_2CIF:	*w = 704; *h = 288;	break;
		case NF_RES_PAL_4CIF:	*w = 704; *h = 576;	break;
		case NF_RES_PAL_4CIFP:	*w = 704; *h = 576;	break;
		case NF_RES_640x480:	*w = 640; *h = 480;	break;
		case NF_RES_720x480:	*w = 720; *h = 480;	break;
		case NF_RES_720x576:	*w = 720; *h = 576;	break;
		case NF_RES_800x600:	*w = 800; *h = 600;	break;
		case NF_RES_1024x768:	*w = 1024;*h = 768;	break;
		case NF_RES_1280x1024:	*w = 1280;*h = 1024;break;
		case NF_RES_1600x1200:	*w = 1600;*h = 1200;break;
		case NF_RES_1280x720:	*w = 1280;*h = 720;	break;
		case NF_RES_1920x1080:	*w = 1920;*h = 1080;break;
		case NF_RES_640x352:	*w = 640; *h = 352;	break;
		case NF_RES_640x360:    *w = 640; *h = 360;	break;
		case NF_RES_640x360I:   *w = 640; *h = 360;	break;
		case NF_RES_1280x720I: 	*w = 1280;*h = 720;	break;
		case NF_RES_1920x1080I: *w = 1920;*h = 1080;break;
		case NF_RES_640x400:    *w = 640; *h = 400;	break;
		case NF_RES_800x450: 	*w = 800; *h = 450;	break;
		case NF_RES_1440x900: 	*w = 1440;*h = 900;	break;
		case NF_RES_320x180:    *w = 320; *h = 180; break;
		case NF_RES_2304x1296:	*w = 2304;*h = 1296;break;
		case NF_RES_2048x1536: 	*w = 2048;*h = 1536;break;
		case NF_RES_2560x1440:	*w = 2560;*h = 1440;break;
		case NF_RES_2688x1520: 	*w = 2688;*h = 1520;break; // Codec Max Width : 2592 (only Hi3521a)
		case NF_RES_2560x1600: 	*w = 2560;*h = 1600;break;
		case NF_RES_2560x1920: 	*w = 2560;*h = 1920;break;
		case NF_RES_2592x1920: 	*w = 2592;*h = 1920;break;
		case NF_RES_2592x1944:  *w = 2592;*h = 1944;break;
		case NF_RES_2560x1944:  *w = 2560;*h = 1944;break;
		case NF_RES_2560x1520:  *w = 2560;*h = 1520;break;
		case NF_RES_2992x1680: 	*w = 2992;*h = 1680;break;
		case NF_RES_2880x1800: 	*w = 2880;*h = 1800;break;
		case NF_RES_3200x1800:	*w = 3200;*h = 1800;break;
		case NF_RES_2880x2160:	*w = 2880;*h = 2160;break;
		case NF_RES_3072x2048: 	*w = 3072;*h = 2048;break;
		case NF_RES_3200x2400: 	*w = 3200;*h = 2400;break;
		case NF_RES_3840x2160:	*w = 3840;*h = 2160;break;
		case NF_RES_2592x1520: 	*w = 2592;*h = 1520;break;
		case NF_RES_1920x1440:	*w = 1920;*h = 1440;break;
		case NF_RES_1920x1536:	*w = 1920;*h = 1536;break;
		case NF_RES_1920x2160:	*w = 1920;*h = 2160;break;		
		case NF_RES_1344x1520:	*w = 1344;*h = 1520;break;
		case NF_RES_1296x1944:	*w = 1296;*h = 1944;break;
		case NF_RES_1280x1440:	*w = 1280;*h = 1440;break;
		case NF_RES_1280x1520:	*w = 1280;*h = 1520;break;
		case NF_RES_1280x1944:	*w = 1280;*h = 1944;break;
		case NF_RES_1280x2160:	*w = 1280;*h = 2160;break;		
		case NF_RES_1024x1536:	*w = 1024;*h = 1536;break;
        case NF_RES_2560x2160:       *w = 2560;*h = 2160;break; 
		case NF_RES_960H_NTSC_CIF:  *w = 352; *h = 240; break;
		case NF_RES_960H_NTSC_2CIF:	*w = 704; *h = 240;	break;
		case NF_RES_960H_NTSC_4CIF:	*w = 960; *h = 480;	break;
		case NF_RES_960H_NTSC_4CIFP:*w = 960; *h = 480; break;
		case NF_RES_960H_PAL_CIF: 	*w = 352; *h = 288;	break;
		case NF_RES_960H_PAL_2CIF:	*w = 704; *h = 288; break;
		case NF_RES_960H_PAL_4CIF:	*w = 960; *h = 576;	break;
		case NF_RES_960H_PAL_4CIFP:	*w = 960; *h = 576; break;
		case NF_RES_NTSC_NONE:      *w = 0;   *h = 0;	break;		
		default:
		    __E("wrong input rel:%d!", res);
	    	NF_RES_NTSC_NONE;
		break;
	}
    
    return 0;
}

static gint
_get_enc_bit_rate(guint res, guint fps, guint quality, guint camtype)
{
#define PER_TO_RATE(a) ((100.0f)/(a))

    gint br, max_br;
    gint idx_f, idx_q;
    gfloat per;

    switch( fps ) {
        case NF_FPS_CR32:  idx_f = 5; break;
        case NF_FPS_CR16:  idx_f = 4; break;
        case NF_FPS_CR08:  idx_f = 3; break;
        case NF_FPS_CR04:  idx_f = 2; break;
        case NF_FPS_CR02:  idx_f = 1; break;
        case NF_FPS_CR01:  idx_f = 0; break;
        case NF_FPS_CR00:			
        default:
            idx_f = 0;
            __E("wrong fps:%d/idx:%d", fps, idx_f);
            break;
    }
    g_assert(idx_f<MAX_FPS_NUM);

	/*8M / 5M / 4M / 3.7M / 3M */
	if( res==NF_RES_3840x2160 || res==NF_RES_2592x1944 || res==NF_RES_2688x1520 || res==NF_RES_2560x1440 || res==NF_RES_2048x1536 ) {

		if( camtype == GOBJ_CAMERA_RESOL_2592x1944P_12P || camtype == GOBJ_CAMERA_RESOL_2688x1520P_15P ||
			camtype == GOBJ_CAMERA_RESOL_2560x1440P_15P || camtype == GOBJ_CAMERA_RESOL_2048x1536P_18P || camtype == GOBJ_CAMERA_RESOL_3840x2160P_8P  ) // Non-Real.
		{   /*2M*/
	        switch(quality) {
	            case NF_QUALITY_LOW:
	                idx_q = 0; max_br = NF_BITRATE_TYPE1_LV0; break;
	            case NF_QUALITY_STANDARD:
	                idx_q = 1; max_br = NF_BITRATE_TYPE1_LV1; break;
	            case NF_QUALITY_HIGH:
	                idx_q = 2; max_br = NF_BITRATE_TYPE1_LV2; break;
	            case NF_QUALITY_HIGHEST:
	                idx_q = 3; max_br = NF_BITRATE_TYPE1_LV3; break;
	            case NF_QUALITY_SUPER:
	                idx_q = 4; max_br = NF_BITRATE_TYPE1_LV4; break;
	            default:
	                idx_q = 0; max_br = NF_BITRATE_TYPE1_LV0;
	                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
	                break;
	        }
		}
		else /* Real */
		{
			switch(quality) {
	            case NF_QUALITY_LOW:
	                idx_q = 0; max_br = NF_BITRATE_TYPE0_LV0; break;
	            case NF_QUALITY_STANDARD:
	                idx_q = 1; max_br = NF_BITRATE_TYPE0_LV1; break;
	            case NF_QUALITY_HIGH:
	                idx_q = 2; max_br = NF_BITRATE_TYPE0_LV2; break;
	            case NF_QUALITY_HIGHEST:
	                idx_q = 3; max_br = NF_BITRATE_TYPE0_LV3; break;
	            case NF_QUALITY_SUPER:
	                idx_q = 4; max_br = NF_BITRATE_TYPE0_LV4; break;
	            default:
	                idx_q = 0; max_br = NF_BITRATE_TYPE0_LV0;
	                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
	                break;
			}
        }
    }
	else if( res==NF_RES_1920x2160 || res==NF_RES_2560x2160 || res==NF_RES_1296x1944 || res==NF_RES_2560x1944 || res==NF_RES_1344x1520 || res==NF_RES_2560x1520 || res==NF_RES_1280x1440
	        || res==NF_RES_2560x1440 || res==NF_RES_1024x1536 || res==NF_RES_2048x1536 || res==NF_RES_1920x1080) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE1_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE1_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE1_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE1_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE1_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE1_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
	else if( res==NF_RES_1024x1536 ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE2_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE2_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE2_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE2_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE2_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE2_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }	
    else if( res==NF_RES_1280x720 ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE3_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE3_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE3_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE3_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE3_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE3_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
    else if( res==NF_RES_960H_NTSC_4CIF || res==NF_RES_960H_PAL_4CIF || 
			 res==NF_RES_960H_NTSC_4CIFP || res==NF_RES_960H_PAL_4CIFP  ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE4_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE4_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE4_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE4_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE4_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE4_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
    else if( res==NF_RES_NTSC_4CIF || res==NF_RES_PAL_4CIF || 
			 res==NF_RES_NTSC_4CIFP || res==NF_RES_PAL_4CIFP  ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE5_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE5_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE5_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE5_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE5_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE5_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
    else if( res==NF_RES_NTSC_2CIF || res==NF_RES_PAL_2CIF  ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE7_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE7_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE7_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE7_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE7_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE7_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
    else if( res==NF_RES_NTSC_CIF || res==NF_RES_PAL_CIF  ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE8_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE8_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE8_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE8_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE8_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE8_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
    else if( res==NF_RES_640x360 || res==NF_RES_640x480  ) {
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE6_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE6_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE6_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE6_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE6_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE6_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }    
    else {  /*NF_RES_640x360*/
        switch(quality) {
            case NF_QUALITY_LOW:
                idx_q = 0; max_br = NF_BITRATE_TYPE6_LV0; break;
            case NF_QUALITY_STANDARD:
                idx_q = 1; max_br = NF_BITRATE_TYPE6_LV1; break;
            case NF_QUALITY_HIGH:
                idx_q = 2; max_br = NF_BITRATE_TYPE6_LV2; break;
            case NF_QUALITY_HIGHEST:
                idx_q = 3; max_br = NF_BITRATE_TYPE6_LV3; break;
            case NF_QUALITY_SUPER:
                idx_q = 4; max_br = NF_BITRATE_TYPE6_LV4; break;
            default:
                idx_q = 0; max_br = NF_BITRATE_TYPE6_LV0;
                __E("res:%d>wrong quality:%d/br:%d", res, quality, br);
                break;
        }
    }
    g_assert(idx_q<MAX_QUALITY_NUM);
    
    per = GET_BR_PER(idx_q, idx_f);
    br = (gint)(max_br/PER_TO_RATE(per));
    
    __T(0, "q:%d/f:%d/per:%f/res:%d/quality:%d/target br:%d", 
                idx_q, idx_f, per, res, quality, br);

    return br;
}

/*******************************************************************************
 *
 */
/*
typedef enum _NF_EVENT_HDCAM_TYPE_E
{
	NF_EVENT_ANALOG_TYPE_TVI_G = 0,// TVI General
	NF_EVENT_ANALOG_TYPE_TVI_H = 1,// TVI Hikvision
	NF_EVENT_ANALOG_TYPE_AHD = 2,
	NF_EVENT_ANALOG_TYPE_EXSDI = 3,
	NF_EVENT_ANALOG_TYPE_SD = 4,
	NF_EVENT_ANALOG_TYPE_HDSDI= 5,
	NF_EVENT_ANALOG_TYPE_3M = 6,
	NF_EVENT_ANALOG_TYPE_5M = 7,
	NF_EVENT_ANALOG_TYPE_IPCAM = 8,
	NF_EVENT_ANALOG_TYPE_MAX
} NF_EVENT_ANALOG_TYPE;
*/

gboolean
nf_encode_zero_channel_register_handoff(long long ch_mask, NF_ENCODE_HANDOFF_FUNC handoff_func)
{
	g_message("%s ch_mask[0x%016llx] handoff_fnx[%p]", __FUNCTION__, ch_mask, handoff_func);
	
	g_handoff_func = handoff_func;
	g_handoff_ch_mask = ch_mask;

	return 1;
}

#if 1
int nf_encode_set_param(NF_REC_INFO_T *rec_param, int num_rec_param, int change_mask, int cam_change_mask )
{
    int ret, ch;
    GOBJEncodeParam param[NUM_ACTIVE_CH];

    g_return_val_if_fail(rec_param!=NULL, -1);
    g_return_val_if_fail(num_rec_param<=NUM_ACTIVE_CH, -1);

    __T(1, "change mask:0x%08x cam_mask:0x%08x num:%d", change_mask, cam_change_mask, num_rec_param);
    //return 0;
    
    for(ch = 0; ch<num_rec_param; ch++) {
		
        if( !GETBIT(change_mask, ch) )
            continue;

        __T(0, "set recording cmd. ch:%d", ch);
        param[ch].ch            = rec_param[ch].channel_id;
		param[ch].codec_type    = rec_param[ch].codec;		
        param[ch].flags         = rec_param[ch].flags;
        param[ch].quality       = rec_param[ch].quality;

		param[ch].gobj_cam_resol = rec_param[ch].type;
		param[ch].gobj_cam_type = GOBJ_CAMERA_IPCAM;

		if( rec_param[ch].enc_mode == 0 )
		{
			param[ch].gobj_enc_rc = GOBJ_ENC_RC_VBR2;
		}
		else if( rec_param[ch].enc_mode == 1 )
		{
			param[ch].gobj_enc_rc = GOBJ_ENC_RC_CBR;
		}
		else
		{
			param[ch].gobj_enc_rc = GOBJ_ENC_RC_VBR; // Error,, set to default
		}		

		param[ch].frame_rate = _convert_to_omx_fps_by_real_fps( rec_param[ch].real_fps );	
        param[ch].gop_i      = _convert_omx_fps_to_gop(ch,param[ch].frame_rate);
        param[ch].resolution = rec_param[ch].res;
        param[ch].bit_rate   = _get_enc_bit_rate(rec_param[ch].res, 
                                                    rec_param[ch].fps, 
                                                    rec_param[ch].quality,
                                                    param[ch].gobj_cam_resol);
        param[ch].video_loss    = rec_param[ch].is_videoloss;
        ret = _get_enc_size_from_res(rec_param[ch].res, 
                                    &param[ch].width, 
                                    &param[ch].height);
        g_assert(ret>=0);
        param[ch].reserved      = rec_param[ch].reserved[0];
				
// short LOG
#if 0
		if( ch < NUM_ACTIVE_CH )
		{
                	printf("[%s] CH[%02d] Res[%x] FPS[%d] Cam Rel[%d] Type[%d]\n",
                       __FUNCTION__, param[ch].ch, param[ch].resolution, param[ch].frame_rate, param[ch].gobj_cam_resol, param[ch].gobj_cam_type);
		}
#endif
        __T(0, "CH[%02d] Flag[%x] Q[%d] BR[%d] FPS[%d] GOP[%d] Res[%x] W[%d] H[%d] vloss[%x] Cam Rel[%d] Type[%d], rc[%d] ",
               param[ch].ch,
               param[ch].flags,
               param[ch].quality,    param[ch].bit_rate,
               param[ch].frame_rate >> 16, param[ch].gop_i,
               param[ch].resolution, param[ch].width, param[ch].height,
               param[ch].video_loss,
               param[ch].gobj_cam_resol,
               param[ch].gobj_cam_type, param[ch].gobj_enc_rc);
    }

	if( change_mask != 0 )
	{
		ret = gobj_media_encode_change(NULL, change_mask, 0, param, num_rec_param);
	}
    
    return 0;
}
#else /* for H/W Test */
#define DEV_PROC "/proc/cmdline"
static int get_proc_test_cam_type()
{
	int cam_type = 0;
	int fd;
	char cmdline[1024];

	fd = open(DEV_PROC, O_RDONLY );

	if( fd >= 0 )
	{
		read( fd, cmdline, sizeof(cmdline) );
		cam_type = ( NULL != strstr( cmdline, "cam720p"))  ? 1 : 0;

		if( cam_type == 0 )
			cam_type = ( NULL != strstr( cmdline, "cam1080p")) ? 2 : 0;
		close(fd);
	}
	else{
		return cam_type;
	}
	
	return cam_type;
}

int
nf_encode_set_param(NF_REC_INFO_T *rec_param, int num_rec_param, int change_mask, int cam_change_mask)
{
    int ret, ch;
    GOBJEncodeParam param[NUM_ENC_CH];
	int cam_type = 0;
	int res_1st;
	int gobj_cam_resol;
    
    g_return_val_if_fail(rec_param!=NULL, -1);
    g_return_val_if_fail(num_rec_param<=NUM_ENC_CH, -1);
    
    __T(1, "change mask:0x%08x", change_mask);
    //return 0;

	cam_type = get_proc_test_cam_type();
	if( cam_type == 1 )
	{
		printf("######### Cam720P ######\n");
		res_1st = NF_RES_1280x720;
		gobj_cam_resol = DISPLAY_IS_PAL ? GOBJ_CAMERA_RESOL_1280x720_25p : GOBJ_CAMERA_RESOL_1280x720_30p;
	}
	else if( cam_type == 2 )
	{
		printf("######### Cam1080P ######\n");
		res_1st = NF_RES_1920x1080;
		gobj_cam_resol = DISPLAY_IS_PAL ? GOBJ_CAMERA_RESOL_1920x1080_25p : GOBJ_CAMERA_RESOL_1920x1080_30p;
	}
	else
	{
		printf("######### CamSD ######\n");
		res_1st = NF_RES_960H_NTSC_4CIF;
		gobj_cam_resol = DISPLAY_IS_PAL ? GOBJ_CAMERA_RESOL_960x576 : GOBJ_CAMERA_RESOL_960x480;
	}
	
    for(ch = 0; ch<num_rec_param; ch++) {
        if( !GETBIT(change_mask, ch) )
            continue;
            
        __T(0, "set recording cmd. ch:%d", ch);
        param[ch].ch            = rec_param[ch].channel_id;
        param[ch].flags         = rec_param[ch].flags;
        param[ch].quality       = rec_param[ch].quality;
        param[ch].frame_rate    = _convert_to_omx_fps(rec_param[ch].fps);
        param[ch].gop_i         = _get_gop_interval(rec_param[ch].fps);
		if( ch < NUM_ACTIVE_CH )
			rec_param[ch].res = res_1st;
		else
			rec_param[ch].res = NF_RES_NTSC_2CIF;
        param[ch].resolution    = rec_param[ch].res;
        param[ch].bit_rate      = _get_enc_bit_rate(rec_param[ch].res, 
                                                    rec_param[ch].fps, 
                                                    rec_param[ch].quality);
        param[ch].video_loss    = rec_param[ch].is_videoloss;
        ret = _get_enc_size_from_res(rec_param[ch].res, 
                                     &param[ch].width, 
                                     &param[ch].height);
        g_assert(ret>=0);
        param[ch].reserved      = rec_param[ch].reserved[0];
		param[ch].gobj_cam_resol = gobj_cam_resol;
        
        __T(0, "CH[%02d] Flag[%x] Q[%d] BR[%d] FPS[%d] GOP[%d] Res[%x] W[%d] H[%d] vloss[%x]",
               param[ch].ch,
               param[ch].flags,
               param[ch].quality,    param[ch].bit_rate,
               param[ch].frame_rate >> 16, param[ch].gop_i,
               param[ch].resolution, param[ch].width, param[ch].height,
               param[ch].video_loss);
    }

    ret = gobj_media_encode_change(NULL, change_mask, ((change_mask>>NUM_ACTIVE_CH)&0x0000ffff), param, num_rec_param);
    if(ret<0) {
        __E("fail to set encoder param! ch mask:0x%08x", change_mask);
        return -1;
    }
    
    return 0;
}
#endif
/******************************************************************************/
/*
 * print out stream length.
 */
static char enc_stm_len_help[] = "enc_cat_stm_len [0:1]";
static int
enc_cat_stm_len(int argc, char **argv)
{    
	if(argc!=2) {
		printf("%s\n", enc_stm_len_help);
		return -1;
	}
	
	enc_cat_stm_log = strtoul(argv[1], NULL, 0);
	
	return 0;
}
__commandlist(enc_cat_stm_len, "enc_cat_stm_len", enc_stm_len_help, enc_stm_len_help);

/*EOF*/
