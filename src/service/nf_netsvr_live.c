#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <unistd.h>		
#include <stdlib.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstinfo.h>
//#include <gst/nf/gstnfbuddybuffer2.h>
#include <gst/nf/gstnfbuddybuffer.h>

#include "nf_common.h"
#include "nf_debug.h"
#include "nf_network.h"
#include "nf_timer.h"

#include "nf_netsvr.h"
#include "nf_netsvr_drdef.h"

#include "nf_record.h"
#include "nf_rec_audio.h"

#include "unp.h"
#include "unpthread.h" 
#include "gsocket.h" 
#include "queue.h" 

#if defined(_OTM_MODEL) || defined(_SNF_MODEL)
#include "nf_solo_aud.h"
//#include "anf_encoder.h"		parangi
#endif /* _OTM_MODEL */

#include "nf_HI_aud.h"

#ifdef G_LOG_DOMAIN
	#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "nlive"

#define NLIVE_DIRECT_QPUSH
#define NLIVE_USE_GSTBUFFER
#define NLIVE_LARGE_FRAME_SEND

#define DEBUG_NLIVE_LOG
//#define DEBUG_NLIVE_JBSHELL

//#define DEBUG_NLIVE_DISABLE_SEND
#define DEBUG_NLIVE_REF_TIMER
//#define DEBUG_NLIVE_NO_SEND_THREAD
//#define DEBUG_NLIVE_NO_READ_THREAD	

#ifdef DEBUG_NLIVE_JBSHELL
	#include "jbshell.h"
#endif

typedef enum _DEBUG_NLIVE_IDX_E
{
	DEBUG_NLIVE_IDX_READ_CHSTAT	= 0 ,
	DEBUG_NLIVE_IDX_READ_FRAME	= 1 ,
	DEBUG_NLIVE_IDX_READ_OVERFLOW	= 2 ,
	DEBUG_NLIVE_IDX_SEND_CHSTAT	= 3 ,
	
	DEBUG_NLIVE_IDX_SEND_FRAME	= 4 ,
	DEBUG_NLIVE_IDX_SEND_CLIENT	= 5 ,
	DEBUG_NLIVE_IDX_FPS_DUMP		= 6 ,
	DEBUG_NLIVE_IDX_FPS_ADJ		= 7 ,
	
	DEBUG_NLIVE_IDX_P_DROP_GOP	= 8 ,	
	DEBUG_NLIVE_IDX_P_DROP_SOCK	= 9 ,
	DEBUG_NLIVE_IDX_P_DROP_RATE	= 10,
	DEBUG_NLIVE_IDX_INIT_CLIENT	= 11,	

	DEBUG_NLIVE_IDX_DUMP_AUDIO	= 12,	
	DEBUG_NLIVE_IDX_CLIENT_CHSTAT = 13,
	DEBUG_NLIVE_IDX_DISABLE_SEND = 14,
	DEBUG_NLIVE_IDX_LARGE_FRAME = 15,
	DEBUG_NLIVE_IDX_NR
}DEBUG_NLIVE_IDX_E;


static const char *_DEBUG_NLIVE_str[32] =
{
	"NLIVE_IDX_READ_CHSTAT",
	"NLIVE_IDX_READ_FRAME",
	"NLIVE_IDX_READ_OVERFLOW",
	"NLIVE_IDX_SEND_CHSTAT",
	
	"NLIVE_IDX_SEND_FRAME",
	"NLIVE_IDX_SEND_CLIENT",	
	"NLIVE_IDX_FPS_DUMP",
	"NLIVE_IDX_FPS_ADJ",
	
	"NLIVE_IDX_P_DROP_GOP",
	"NLIVE_IDX_P_DROP_SOCK",
	"NLIVE_IDX_P_DROP_RATE",
	"NLIVE_IDX_INIT_CLIENT",
	
	"NLIVE_IDX_DUMP_AUDIO",
	"NLIVE_IDX_CLIENT_CHSTAT",
	"NLIVE_IDX_DISABLE_SEND",
	"NLIVE_IDX_LARGE_FRAME",
	
	"NLIVE_IDX_NR"
};

static gint _DEBUG_NLIVE_log[32] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};


// local function
static VIDEO_ENTRY *new_video_entry( gpointer frame);
extern void free_video_entry( VIDEO_ENTRY *pEntry);

static void enque_video_entry( LIVE_INFO *pLive_info, VIDEO_ENTRY *pEntry);
extern VIDEO_ENTRY *deque_video_entry( LIVE_INFO *pLive_info );
static int write_video_entry( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry);

static int check_codec_header( ICODEC_HEADER *pfh );
static void print_codec_header( ICODEC_HEADER *pheader, const char *func, const char *msg );
static void copy_codec_header( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry);
static void print_channel_stat( CHANNEL_STAT *pChStat, const char *func, const char *msg );

static int valid_pframe_header( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry);
static int valid_socket_buffer( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry);
static int valid_frame_rate_ctrl( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry);

static void adj_ch_fps_data ( CLIENT_INFO *pClient, int ch, int delta);
static void adj_fps_data( CLIENT_INFO *pClient);
static void dump_fps_data( CLIENT_INFO *pClient);

//pthread_mutex_t			live_frame_mutex;

enum _FPS_TABLE_IDX 
{
	FPST_IDX_FPS = 0,
	FPST_IDX_FPS_SUB,
	FPST_IDX_ICNT,
	FPST_IDX_ICNT_SUB,
	FPST_IDX_PCNT,
	FPST_IDX_ISIZE,
	FPST_IDX_PSIZE,
	FPST_IDX_TOT,
	FPST_IDX_TOT_16CH,
	FPST_IDX_NEXT_NTSC,
	FPST_IDX_NEXT_PAL,
	FPST_IDX_MAX
};


#define FPST_ROW_COUNT	30
const int FPS_TABLE[FPST_ROW_COUNT][FPST_IDX_MAX] = 
{
/*0 */	{  0,0      ,0,0    ,0      ,0  ,0      ,0      ,0      ,0      	,0      },
/*1 */	{ 32,0		,2,0	,30		,25	,2		,110000	,1760000,30			,40		},
/*2 */	{ 16,0		,2,0	,14		,25	,2		,78000	,1248000,65			,80		},
/*3 */	{  8,0		,2,0	,6		,25	,2		,62000	,992000	,130		,160	},
/*4 */	{  4,0		,2,0	,2		,25	,2		,54000	,864000	,265		,320	},
/*5 */	{  2,0		,2,0	,0		,25	,2		,50000	,800000	,530		,640	},
/*6 */	{  1,0		,1,0	,0		,25	,2		,25000	,400000	,1065*1 	,1280*1 },
/*7 */	{  0,5000   ,0,5000 ,0		,25	,2		,12500 	,200000 ,1065*2 	,1280*2 },
/*8 */	{  0,2000   ,0,2000 ,0		,25	,2		,5000 	,80000  ,1065*5 	,1280*5 },
/*9 */	{  0,1429   ,0,1429 ,0		,25	,2		,3571 	,57143  ,1065*7 	,1280*7 },
/*10*/	{  0,909    ,0,909  ,0		,25	,2		,2273 	,36364  ,1065*11 	,1280*11},
/*11*/	{  0,769    ,0,769  ,0		,25	,2		,1923 	,30769  ,1065*13	,1280*13},
/*12*/	{  0,588    ,0,588  ,0		,25	,2		,1471 	,23529  ,1065*17	,1280*17},
/*13*/	{  0,526    ,0,526  ,0		,25	,2		,1316 	,21053  ,1065*19	,1280*19},
/*14*/	{  0,435    ,0,435  ,0		,25	,2		,1087 	,17391  ,1065*23	,1280*23},
/*15*/	{  0,345    ,0,345  ,0		,25	,2		,862 	,13793  ,1065*29	,1280*29},
/*16*/	{  0,323    ,0,323  ,0		,25	,2		,806 	,12903  ,1065*31	,1280*31},
/*17*/	{  0,270    ,0,270  ,0		,25	,2		,676 	,10811  ,1065*37	,1280*37},
/*18*/	{  0,244    ,0,244  ,0		,25	,2		,610 	,9756   ,1065*41	,1280*41},
/*19*/	{  0,233    ,0,233  ,0		,25	,2		,581 	,9302   ,1065*43	,1280*43},
/*20*/	{  0,185    ,0,185  ,0		,25	,2		,463 	,7407   ,1065*54	,1280*54},
/*21*/	{  0,169    ,0,169  ,0		,25	,2		,424 	,6780   ,1065*59	,1280*59},
/*22 */	{  0,164    ,0,164  ,0		,25	,2		,410 	,6557   ,1065*61	,1280*61},
/*23 */	{  0,149    ,0,149  ,0		,25	,2		,373 	,5970   ,1065*67	,1280*67},
/*24 */	{  0,141    ,0,141  ,0		,25	,2		,352 	,5634   ,1065*71	,1280*71},
/*25 */	{  0,137    ,0,137  ,0		,25	,2		,342 	,5479   ,1065*73	,1280*73},
/*26 */	{  0,127    ,0,127  ,0		,25	,2		,316 	,5063   ,1065*79	,1280*79},
/*27 */	{  0,120    ,0,120  ,0		,25	,2		,301 	,4819   ,1065*83	,1280*83}, 
/*28 */	{  0,112    ,0,112  ,0		,25	,2		,281 	,4494   ,1065*89	,1280*89},
/*29 */	{  0,103    ,0,103  ,0		,25	,2		,258 	,4124   ,1065*97	,1280*97} 
};
#if 0


#endif

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static unsigned char conv_fps_idx( unsigned char fps)
{
	int ret;
		
	switch (fps){
	case NF_FPS_CR32: ret = 1; break;
	case NF_FPS_CR16: ret = 2; break;
	case NF_FPS_CR08: ret = 3; break;
	case NF_FPS_CR04: ret = 4; break;
	case NF_FPS_CR02: ret = 5; break;
	case NF_FPS_CR01: ret = 6; break;
	default: ret = 0; break;
	}
	
	return ret;			
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int cal_traffic( unsigned char fps, unsigned char res)
{		
	if(fps > FPST_ROW_COUNT  || res >4) 
		return 0;		
	else	
		return  (FPS_TABLE[ fps ][FPST_IDX_TOT]	* res);
}

enum _FPS_DELTA 
{
	FPS_DN = 1,
	FPS_UP
};


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void dump_fps_data ( CLIENT_INFO *pClient)
{
	int i;
	for(i=0; i< NUM_CHANNEL; i++)		
	{			
		CHANNEL_INFO *pChannel = &pClient->ch_info[i];
		
		if( !(pClient->channel_mask & (1 << i)) ) continue;
			
		g_message("%s ch[%2d] fps[%2d]res[%2d]traf[%3d] fps_level[%2d]traf[%3d]", 
				__FUNCTION__, i, 	
				pChannel->framerate,
				pChannel->resolution,		
				pChannel->traffic,				
				pChannel->fps_level,	
				pChannel->fps_traffic  );
	}	

	g_message("%s total_taffic[%8d] fps_total[%8d]", __FUNCTION__, 
				pClient->total_traffic, 
				pClient->total_fps_traffic);

}

static void dump_fps_data2 ( CLIENT_INFO *pClient)
{	
	g_message("%s uid[%3d] tot[%4d]fps[%4d]max[%4d] ac[%2d] [%08x] [%d][%d][%d][%d] [%d][%d][%d][%d] [%d][%d][%d][%d] [%d][%d][%d][%d]"	, 
				__FUNCTION__, 
				pClient->uniqueid,
				pClient->total_traffic/1024, 
				pClient->total_fps_traffic/1024,
				pClient->max_tx_speed/1024,
				pClient->fps_accel,
				pClient->channel_imask,
				pClient->ch_info[ 0].fps_level,
				pClient->ch_info[ 1].fps_level,
				pClient->ch_info[ 2].fps_level,
				pClient->ch_info[ 3].fps_level,
				pClient->ch_info[ 4].fps_level,
				pClient->ch_info[ 5].fps_level,
				pClient->ch_info[ 6].fps_level,
				pClient->ch_info[ 7].fps_level,
				pClient->ch_info[ 8].fps_level,
				pClient->ch_info[ 9].fps_level,
				pClient->ch_info[10].fps_level,
				pClient->ch_info[11].fps_level,
				pClient->ch_info[12].fps_level,
				pClient->ch_info[13].fps_level,
				pClient->ch_info[14].fps_level,
				pClient->ch_info[15].fps_level	);

}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void adj_ch_fps_data ( CLIENT_INFO *pClient, int ch, int delta)
{
	unsigned char fps, res, old_fps_level;
	unsigned int  traffic;
	
	CHANNEL_INFO *pChannel = &pClient->ch_info[ch];
	
	fps = pChannel->framerate;
	res = pChannel->resolution & 0x0f;	
	old_fps_level = pChannel->fps_level;
	
	if(delta == FPS_UP)
	{
		if(fps <= pChannel->fps_level - 1 && pChannel->fps_level > 1  )
			--pChannel->fps_level;
#ifdef DEBUG_NLIVE_FPSxx
		else
			g_message("%s FPS_UP GG ch[%d] fps_level[%d]", __FUNCTION__, 
						ch, pChannel->fps_level);
#endif			
	}
	else if(delta == FPS_DN )
	{
		if(FPST_ROW_COUNT > pChannel->fps_level + 1 )
			++pChannel->fps_level;
#ifdef DEBUG_NLIVE_FPSxx
		else
			g_message("%s FPS_DN GG ch[%d] fps_level[%d]", __FUNCTION__, 
						ch, pChannel->fps_level);
#endif
	}
	
	traffic = pChannel->fps_traffic;	
	pChannel->fps_traffic	= cal_traffic( pChannel->fps_level, res);
	
	pClient->total_fps_traffic += pChannel->fps_traffic - traffic;
	
#ifdef DEBUG_NLIVE_LOG
	if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_FPS_ADJ] == pClient->uniqueid )
		g_message("%s ch[%d] %s fps[%d] -->[%d] [%d]bytes [%d]/[%d]", __FUNCTION__, 
				ch, (delta == FPS_UP ) ? "UP" :"DN",
				old_fps_level, pChannel->fps_level, 
				pChannel->fps_traffic,
				pClient->total_fps_traffic,
				pClient->total_traffic);
#endif
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int max_fps_traffic_channel(CLIENT_INFO *pClient)
{
	int i, ch_id = 0;
	unsigned int	traffic = 0;
	
	for(i=0; i< NUM_CHANNEL; i++)
	{						
		CHANNEL_INFO *pChannel = &pClient->ch_info[i];
		
		if( !(pClient->channel_mask & (1 << i)) ) continue;
			
		if( pChannel->fps_traffic > traffic )
		{
			ch_id = i;
			traffic = pChannel->fps_traffic;
		}
	}	
	return ch_id;
}

static int min_fps_traffic_channel(CLIENT_INFO *pClient)
{
	int i, ch_id = 0;
	unsigned int	traffic = 0xffffffff;
	
	for(i=0; i< NUM_CHANNEL; i++)
	{						
		CHANNEL_INFO *pChannel = &pClient->ch_info[i];
		
		if( !(pClient->channel_mask & (1 << i)) ) continue;
			
		if( pChannel->fps_traffic < traffic )
		{
			ch_id = i;
			traffic = pChannel->fps_traffic;
		}
	}	
	return ch_id;
}

static int max_fps_diff_channel(CLIENT_INFO *pClient)
{
	int i, ch_id = 0;
	int	diff = 0, tmp_diff;
		
	for(i=0; i< NUM_CHANNEL; i++)
	{						
		CHANNEL_INFO *pChannel = &pClient->ch_info[i];					
		
		if( !(pClient->channel_mask & (1 << i)) ) continue;
	
		tmp_diff = pChannel->traffic - pChannel->fps_traffic;
		if( tmp_diff > diff )
		{
			ch_id = i;
			diff = tmp_diff;
		}
	}	
	return ch_id;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void adj_fps_data ( CLIENT_INFO *pClient )
{
	unsigned char fps, res;
	unsigned int  timestamp;
	int i, reduce_count;
	unsigned int  total_taffic, total_fps_traffic;
		
	CHANNEL_INFO  ch_info[NUM_CHANNEL];
				
	// 과거 정보 save;
	total_taffic = pClient->total_traffic;
	total_fps_traffic = pClient->total_fps_traffic;
	memcpy( ch_info, pClient->ch_info, sizeof(pClient->ch_info));

	// 채널 정보 업데이트
	pClient->total_traffic = pClient->total_fps_traffic = 0;
	for(i=0; i< NUM_CHANNEL; i++)
	{	
		CHANNEL_INFO *pChannel = &pClient->ch_info[i];
		
		fps = conv_fps_idx(gLive_info.iframe_header[i].frame_rate);
		res = gLive_info.iframe_header[i].resolution & 0x0f;
		timestamp = gLive_info.iframe_header[i].timestamp;
				
		if( !(pClient->channel_mask & (1 << i)) ) continue;
			
		if( timestamp == 0) // 아직 iframe을 받은 적이 없다!
		{
			fps = conv_fps_idx(NF_FPS_CR32); res = NF_RES_NTSC_CIF;	// 기본값;
		}								

		pChannel->traffic 		= cal_traffic( fps, res);	
		if( pChannel->framerate != fps // 채널 속성이 변했다. 
			|| pChannel->resolution	!= res )
		{
			pChannel->fps_level = fps;
			pChannel->fps_traffic = pChannel->traffic;			
			pChannel->fps_updown = 0;
		}
		pChannel->framerate		= fps;
		pChannel->resolution	= res;

		pClient->total_traffic += pChannel->traffic;
		pClient->total_fps_traffic	+= pChannel->fps_traffic;
		
		// 2007-03-29 1:05오후 choissi
		// 여기서 기존 fps_traffic 만큼 보정해 줘야 할텐데,
		// 아래 adj 모듈에서 곧 수렴할것이다. 일딴 보류
		
	}
	
	for(i=0, reduce_count=0; i< NUM_CHANNEL; i++)
	{	
		int err_cnt = 0;
		
		if( pClient->ch_info[i].resolution == 4)	
			err_cnt = 3;
		else if( pClient->ch_info[i].resolution == 2)	
			err_cnt = 2;
		else
			err_cnt = 2;

		if( pClient->ch_stat.error_count[i] > err_cnt )
			++reduce_count;
		
	}
			
	if(gLive_info.client_count_runtime <2)
		pClient->max_tx_speed = gLive_info.max_tx_speed;
	else
		pClient->max_tx_speed = gLive_info.max_tx_speed / 
								gLive_info.client_tot_ch_runtime * pClient->channel_count;
						
	if( pClient->max_tx_speed < total_fps_traffic )	
	{
		int down_count = (total_fps_traffic*pClient->channel_count)/pClient->max_tx_speed; 
				
		if(pClient->channel_count>1)
			reduce_count += down_count/2;
		else
			reduce_count += down_count;
	}

#ifdef DEBUG_NLIVE_LOG
	if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_FPS_ADJ])
	{
		g_message("%s uid[%3d] client_cnt[%2d]/[%2d] max_tx[%4d] my_ch_cnt[%d]tx[%4d] tot_fps[%4d]rcnt[%d]", 
				__FUNCTION__, 		
				pClient->uniqueid,
				gLive_info.client_count_runtime,
				gLive_info.client_tot_ch_runtime,
				gLive_info.max_tx_speed/1024,
				pClient->channel_count,
				pClient->max_tx_speed/1024,
				total_fps_traffic,
				reduce_count );
	}
#endif
		
	if( reduce_count > 0 )
	{// down
		for(i=0; i<reduce_count; i++)
		{	
			int ch_id = max_fps_traffic_channel(pClient);
			adj_ch_fps_data ( pClient, ch_id, FPS_DN);
		}		
		pClient->fps_accel = 1;
	}
	else if( pClient->sndbuff_size/2 > pClient->sndbuff_used 
				|| pClient->ch_stat.total_count > 0 )
	{// up

		if( pClient->sndbuff_size/2 > pClient->sndbuff_used ) 
			pClient->fps_accel *= 2;
		
		if( pClient->fps_accel >= pClient->channel_count )
			pClient->fps_accel = pClient->channel_count;
			
//		for(i=0; i< pClient->fps_accel; i++)
		{
			int ch_id = max_fps_diff_channel(pClient);
			adj_ch_fps_data ( pClient, ch_id, FPS_UP);
		}
		
	}else{
#ifdef DEBUG_NLIVE_FPS
		g_message("%s sleep [%d]", __FUNCTION__, sizeof(pClient->ch_info) );
#endif
	}
		
}



/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static VIDEO_ENTRY *new_video_entry( gpointer frame)
{
	VIDEO_ENTRY 		*pEntry  = NULL;
	GstNfBuddyBuffer	*buffer = (GstNfBuddyBuffer *)frame;
	ICODEC_HEADER		*pheader = buffer->frame ? buffer->frame : GST_BUFFER_DATA(frame);

	g_return_val_if_fail( frame, NULL);
	

	pEntry = Calloc( sizeof(VIDEO_ENTRY), 1);

	g_return_val_if_fail( pEntry, NULL);

	memcpy( &pEntry->frame_header, pheader, sizeof(ICODEC_HEADER));
	pEntry->frame_header.reserved = (unsigned int)buffer->cmemq;

//	g_printf("buffer->cmemq[%p] reserved[0x%x]\n",buffer->cmemq, pEntry->frame_header.reserved);

	pEntry->data_size = pheader->frame_size;
#if defined(__CODEC_INCLUDE__)
	pEntry->frame_data = (unsigned char *)pheader;  // asm_rtp
#else
	pEntry->frame_data = (unsigned char *)pheader + sizeof(ICODEC_HEADER);
#endif
	{
		void *tmp_gst_ret = NULL;
		tmp_gst_ret = gst_buffer_ref(buffer);
		if(tmp_gst_ret == NULL)
			fprintf(stderr, "- ERROR - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);
	}
	
	return pEntry;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
VIDEO_ENTRY *copy_video_entry( VIDEO_ENTRY *src_Entry )
{
	VIDEO_ENTRY 		*dst_Entry  = NULL;
	GstBuffer			*dst_gst_buf = NULL;
	GstNfBuddyBuffer	*src_gst_buf = NULL;	
	ICODEC_HEADER		*dst_pheader = NULL, *src_pheader = NULL;		
	gint size = 0;
	
	g_return_val_if_fail( src_Entry, NULL);
	dst_Entry = Calloc( sizeof(VIDEO_ENTRY), 1);	
	g_return_val_if_fail( dst_Entry, NULL);	// error return

	src_gst_buf = (GstNfBuddyBuffer	*)src_Entry->frame_header.gst_buffer;
	src_pheader = src_gst_buf->frame;

	size =  src_pheader->frame_size + sizeof(ICODEC_HEADER);
	dst_gst_buf = gst_buffer_new_and_alloc( size );
	if(dst_gst_buf == NULL) free(dst_Entry); // entry free
	g_return_val_if_fail(dst_gst_buf, NULL); // error return

	memcpy( GST_BUFFER_DATA(dst_gst_buf), src_pheader, size );
		
	dst_pheader = GST_BUFFER_DATA(dst_gst_buf);
	dst_pheader->gst_buffer = dst_gst_buf;												
	dst_pheader->reserved = 0;	

	memcpy( &dst_Entry->frame_header, dst_pheader, sizeof(ICODEC_HEADER));

	dst_Entry->data_size = dst_pheader->frame_size;
#if defined(__CODEC_INCLUDE__)

	dst_Entry->frame_data = (unsigned char *)dst_pheader; // for RTP
#else

	dst_Entry->frame_data = (unsigned char *)dst_pheader + sizeof(ICODEC_HEADER);
#endif

	return dst_Entry;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void free_video_entry( VIDEO_ENTRY *pEntry)
{
	if(pEntry) 
	{

#ifdef NLIVE_USE_GSTBUFFER	// gstbuffer 사용일 때
		if( pEntry->frame_data)
			gst_buffer_unref( (GstNfBuddyBuffer *)pEntry->frame_header.gst_buffer );
#else
		if( pEntry->frame_data)
			free(pEntry->frame_data); 
#endif			

		free(pEntry);
	}
}

//LIVE_INFO gLive_info;
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
VIDEO_ENTRY *deque_video_entry( LIVE_INFO *pLive_info )
{
#if 1
	VIDEO_ENTRY 			*pEntry = NULL;
	static unsigned int deque_cnt = 0;

	if( pLive_info == NULL)
		return NULL;

	pEntry = TAILQ_FIRST(&pLive_info->video_head[deque_cnt&1]);	
	if( pEntry == NULL)
		goto no_frame;
		
	TAILQ_REMOVE(&pLive_info->video_head[deque_cnt&1], pEntry, entries);
				
	--pLive_info->video_entry_count;
	pLive_info->video_entry_size -= pEntry->data_size;

	++pLive_info->send_count;
	pLive_info->send_bytes += pEntry->data_size;

no_frame:
	
	++deque_cnt;		
	return 	pEntry;
#else
	VIDEO_ENTRY 			*pEntry = NULL;

	if( pLive_info == NULL)
		return NULL;

	pEntry = TAILQ_FIRST(&pLive_info->video_head);	

	g_printf("deque : pEntry[%p]\n",pEntry);
	if( pEntry == NULL)
		goto no_frame;
		
	TAILQ_REMOVE(&pLive_info->video_head, pEntry, entries);
				
	--pLive_info->video_entry_count;
	pLive_info->video_entry_size -= pEntry->data_size;

	++pLive_info->send_count;
	pLive_info->send_bytes += pEntry->data_size;

no_frame:
	
	return 	pEntry;

#endif
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void enque_video_entry( LIVE_INFO *pLive_info, VIDEO_ENTRY *pEntry)
{
#if 1
	static const int que_map[16] = { 0,0,0,0, 0,0,0,0, 1,1,1,1, 1,1,1,1};
	int que_id;
#endif

	if( pLive_info == NULL || pEntry == NULL)
		return;
		
#if 1
	if( pEntry->frame_header.chan  <16 )
		que_id = que_map[pEntry->frame_header.chan];
	else
		que_id = 0;
	
	TAILQ_INSERT_TAIL( &pLive_info->video_head[que_id], pEntry, entries);
#else

	g_printf("pEntry->frame_header.chan[%d], pEntry[%p] \n",pEntry->frame_header.chan,pEntry);
	
	TAILQ_INSERT_TAIL( &pLive_info->video_head, pEntry, entries);
#endif
				
	// update statistics
	++pLive_info->video_entry_count;
	pLive_info->video_entry_size += pEntry->data_size;

	++pLive_info->read_count;
	pLive_info->read_bytes += pEntry->data_size;
			
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int write_video_entry( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry)
{
	// send
	struct iovec iov[2];
	int ret, wsize = 0;
	
	ICODEC_HEADER *pfh;

	if(pClient == NULL || pEntry == NULL) return -1;
	if(pClient->magic_key != CLIENT_MAGIC) return -2;

	pfh = (ICODEC_HEADER *)&pEntry->frame_header;

#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_SEND_CLIENT] == pClient->uniqueid)
			print_codec_header(pfh,"CLIENT","frame");
#endif
		
	iov[0].iov_base = (char *)&pEntry->frame_header;
	iov[0].iov_len = sizeof(ICODEC_HEADER);
	iov[1].iov_base = (char *)pEntry->frame_data;
	iov[1].iov_len = NF_BSWAP_32(pfh->frame_size);
	
	wsize = sizeof(ICODEC_HEADER) + NF_BSWAP_32(pfh->frame_size);
																
	ret = writev( pClient->ds, iov, 2);
	if ( ret != wsize )
	{
		err_ret("%s [Err] writev fd[%d] ret[%d] wsize[%d]", __FUNCTION__, 
			pClient->ds, ret, wsize);
												
		_client_set_mode( pClient, CLIENT_MODE_ERROR);
		//Close(pClient->ds); pClient->ds = -1;
		
		return -3;
					
	}else{			
		return wsize;
	}		
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int check_codec_header( ICODEC_HEADER *pfh ) 
{
	static ICODEC_HEADER header;
	int ret = 1;
			
	if( pfh->chan >= NUM_CHANNEL)		// channel id check
	{
		if(	(pfh->frame_type == NF_FRAME_TYPE_START 
				|| pfh->frame_type == NF_FRAME_TYPE_END)
			&& pfh->chan < NUM_CHANNEL + 4 )
		{ 
			// 오디오 START/END 는 ch번호가 16,17,18,19로 내려오므로 정상이다.
		}	
		else
		{	    	
			g_warning("%s invalid channel(0x%x)",__FUNCTION__, pfh->chan);
			ret = -1;
		}
	}

	if( NF_BSWAP_32(pfh->frame_size) >= MAX_RCV_BUFF ) // frame size check
	{
		g_warning("%s invalid size:%d(0x%x)",
			__FUNCTION__, NF_BSWAP_32(pfh->frame_size), NF_BSWAP_32(pfh->frame_size) );
		
		ret = -2;			
	}
	
	if(ret < 0) 
	{				
		print_codec_header( &header, __FUNCTION__, "PREV");
		print_codec_header( pfh, __FUNCTION__, "ERR ");
	}
	
	memcpy( &header, pfh, sizeof(header));	
	
	return ret;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void print_codec_header( ICODEC_HEADER *pheader, const char *func, const char *msg ) 
{
	g_message("%s %s ch[%02d] f[0x%02x] type[0x%02x][0x%02x][0x%02x] [%d][%3d] [%6d]",
					func, msg,
					pheader->chan,
					pheader->flags,
					pheader->frame_type,
					pheader->frame_rate,
					pheader->resolution,
					pheader->timestamp,
					pheader->timestampl,
					pheader->frame_size );
	return;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void copy_codec_header( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry) 
{	
	ICODEC_HEADER *pfh;

	if(pClient == NULL || pEntry == NULL) return;
	if(pClient->magic_key != CLIENT_MAGIC) return;

	pfh = (ICODEC_HEADER *)&pEntry->frame_header;
	
	if( pfh->frame_type == NF_FRAME_TYPE_AUDIO )
		return;
				
	memcpy( &pClient->last_frame_header[ pfh->chan ], pfh ,
		sizeof(ICODEC_HEADER));
					
	if(	pfh->frame_type == NF_FRAME_TYPE_I
		|| pfh->frame_type == NF_FRAME_TYPE_RI )
	{
		memcpy( &pClient->last_iframe_header[ pfh->chan ], pfh ,
			sizeof(ICODEC_HEADER));						
			
		pClient->ch_info[pfh->chan].req_iframe = 0;			
	}
	return;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void copy_codec_header2( VIDEO_ENTRY *pEntry) 
{	
	ICODEC_HEADER *pfh;

	if(pEntry == NULL) return;

	pfh = (ICODEC_HEADER *)&pEntry->frame_header;

	if( pfh->frame_type == NF_FRAME_TYPE_AUDIO )
		return;

	memcpy( &gLive_info.frame_header[ pfh->chan ], pfh ,
		sizeof(ICODEC_HEADER));
					
	if(	pfh->frame_type == NF_FRAME_TYPE_I
		|| pfh->frame_type == NF_FRAME_TYPE_RI )
		memcpy( &gLive_info.iframe_header[ pfh->chan ], pfh ,
			sizeof(ICODEC_HEADER));						
	return;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int valid_pframe_header( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry) 
{
	ICODEC_HEADER *pfh;

	if(pClient == NULL || pEntry == NULL) return -1;
	if(pClient->magic_key != CLIENT_MAGIC) return -2;

	pfh = (ICODEC_HEADER *)&pEntry->frame_header;

	if(	pfh->frame_type == NF_FRAME_TYPE_P )
	{
		ICODEC_HEADER *pfh_last = &pClient->last_iframe_header[ pfh->chan ];
		guint64  curr_timestamp, last_timestamp, diff_timestamp;
		
		curr_timestamp = (guint64)NF_BSWAP_32(pfh->timestamp) * 1000 + (pfh->timestampl * 5);
		last_timestamp = (guint64)NF_BSWAP_32(pfh_last->timestamp) * 1000 + (pfh_last->timestampl * 5);
		diff_timestamp = curr_timestamp - last_timestamp;				

		if( last_timestamp == 0 )
			return -7;
		
		// 해상도가 변했다.
		if( pfh->resolution != pfh_last->resolution )
			return -9;
			
		// GoP 유지
		if( (!(pfh->resolution & 0x10) &&  diff_timestamp < 545 )		// ntsc
				 || ((pfh->resolution & 0x10) &&  diff_timestamp < 660)	)
			return 1;
		else if( pClient->ch_info[pfh->chan].req_iframe == 0 )
		{
			++pClient->ch_info[pfh->chan].req_iframe;
			return -9;
		}
		else
		{
			++pClient->ch_info[pfh->chan].req_iframe;
			return -8;
		}

#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_P_DROP_GOP] )
			g_message("%s pframe DROP uid[%d] fd[%d] ch[%2d] curr[%lld] diff[%lld]", __FUNCTION__, 
				pClient->uniqueid, pClient->ds, pfh->chan, curr_timestamp, diff_timestamp);
#endif
	}
	
	return 1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int valid_socket_buffer( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry) 
{
	ICODEC_HEADER *pfh;
	int	sndbuff_remains = 0, wlen = 0;
	int	scale_factor = 3;
	
	if(pClient == NULL || pEntry == NULL) return -1;
	if(pClient->magic_key != CLIENT_MAGIC) return -2;

	pfh = (ICODEC_HEADER *)&pEntry->frame_header;

	_client_stat_sock_buff(pClient);
	
	if(pClient->shadow_count > (480*4) &&  pClient->sndbuff_used != 0 ) 
	{	// 랜선이 뽑혔어~;; buff의 remain값이 계속 안변할 때!!			
		g_message("%s pClient Stream End fd[%d] sndbuff[%d] used[%d][%d] remain[%d]",
				__FUNCTION__, pClient->ds, pClient->sndbuff_size,  
				pClient->sndbuff_used, pClient->shadow_count, sndbuff_remains);
														
		_client_set_mode( pClient, CLIENT_MODE_ERROR);
		//Close(pClient->ds); pClient->ds = -1;								
		return -3;
	}
	

#if 0
	if(pClient->channel_count <= 4)
		scale_factor = 2;
		
	sndbuff_remains = ((pClient->sndbuff_size >> 2) * scale_factor) - pClient->sndbuff_used;	
#else
	sndbuff_remains = pClient->sndbuff_size - pClient->sndbuff_used - (16*1024);
#endif

	wlen = sizeof(ICODEC_HEADER) + NF_BSWAP_32(pfh->frame_size);						
	if(  sndbuff_remains < wlen && pClient->sndbuff_used != 0 )
	{	// socket buffer full
#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_P_DROP_SOCK] )
			g_message("%s sock_full uid[%d] fd[%d] sndbuff[%6d] used[%6d][%3d] remain[%6d] wlen[%6d]",
					__FUNCTION__, pClient->uniqueid,  pClient->ds,
					pClient->sndbuff_size, 
					pClient->sndbuff_used,
					pClient->shadow_count,	
					sndbuff_remains, wlen);
#endif
		return -4;
	}

#ifdef DEBUG_NLIVE_LOG
	if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_P_DROP_SOCK] == pClient->uniqueid )
		g_message("%s sock_stat uid[%d] fd[%d] sndbuff[%6d] used[%6d][%3d] remain[%6d] wlen[%6d]",
				__FUNCTION__, pClient->uniqueid, pClient->ds,			
				pClient->sndbuff_size, 
				pClient->sndbuff_used,
				pClient->shadow_count,	
				sndbuff_remains, wlen);
#endif
	
	return 1;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int valid_frame_rate_ctrl( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry) 
{
	ICODEC_HEADER *pfh,*pfh_last;
	guint64  curr_timestamp, last_timestamp, diff_timestamp;
	int fps_level;
	guint64 next_pal,next_ntsc;
	
	if(pClient == NULL || pEntry == NULL) return -1;
	if(pClient->magic_key != CLIENT_MAGIC) return -2;

	pfh = (ICODEC_HEADER *)&pEntry->frame_header;	
	pfh_last = &pClient->last_frame_header[ pfh->chan ];				
	
	curr_timestamp = (guint64)NF_BSWAP_32(pfh->timestamp) * 1000 + (pfh->timestampl * 5);
	last_timestamp = (guint64)NF_BSWAP_32(pfh_last->timestamp) * 1000 + (pfh_last->timestampl * 5);
	diff_timestamp = curr_timestamp - last_timestamp;
			
	fps_level = pClient->ch_info[pfh->chan].fps_level;
	next_pal  = FPS_TABLE[fps_level][FPST_IDX_NEXT_PAL] - 6; 
	next_ntsc = FPS_TABLE[fps_level][FPST_IDX_NEXT_NTSC] - 6;	
	
	// GoP 유지
	if( (!(pfh->resolution & 0x10) &&  diff_timestamp < next_ntsc )		// ntsc
		 || ((pfh->resolution & 0x10) &&  diff_timestamp < next_pal)  )// pal
	{
#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_P_DROP_RATE] )
			g_message("%s pframe DROP uid[%d] fd[%d] ch[%2d] curr[%lld] diff[%lld]", __FUNCTION__, 
						pClient->uniqueid, pClient->ds, pfh->chan, curr_timestamp, diff_timestamp);
#endif
			return -9;
	}
		
	return 1;
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int yield_frame( CLIENT_INFO *pClient, VIDEO_ENTRY *pEntry) 
{
	ICODEC_HEADER *pfh;
		
	pfh = (ICODEC_HEADER *)&pEntry->frame_header;

	if(pClient == NULL || pEntry == NULL) return -1;
	if(pClient->magic_key != CLIENT_MAGIC) return -2;

	if( pClient->channel_imask == 0 )
		return 1;	// 모든 채널에 iframe이 들어 갔으므로 양보안해.
		
	if( pClient->channel_imask & (1 << pfh->chan) ) // 아직 iframe 못 받은 채널
	{	
		// 이번 프레임이 iframe이면,
		if ( pfh->frame_type == NF_FRAME_TYPE_I 
				|| pfh->frame_type == NF_FRAME_TYPE_RI )
		{
			pClient->channel_imask &= ~(1 << pfh->chan);
			return 1;
		}
	}	
	return -9;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void reset_channel_stat( CHANNEL_STAT	 *pChStat, struct timeval	curTimeStamp) 
{
		pChStat->saveTimeStamp = curTimeStamp;
		memset( pChStat->channel_count, 0x00, sizeof(pChStat->channel_count));
		memset( pChStat->error_count, 0x00, sizeof(pChStat->error_count));
		pChStat->total_error_count = pChStat->total_count = 0;	
		pChStat->total_bytes = 0;	
}

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void print_channel_stat( CHANNEL_STAT *pChStat, const char *func, const char *msg ) 
{	
	g_message("%s %s TimeStamp[%010d.%06d] cnt[%03d] "
			"[%02d][%02d][%02d][%02d] [%02d][%02d][%02d][%02d] "
			"[%02d][%02d][%02d][%02d] [%02d][%02d][%02d][%02d] [%lld]", 
			func, msg,
			pChStat->saveTimeStamp.tv_sec, pChStat->saveTimeStamp.tv_usec,
			pChStat->total_count,
			pChStat->channel_count[0],pChStat->channel_count[1],
			pChStat->channel_count[2],pChStat->channel_count[3],
			pChStat->channel_count[4],pChStat->channel_count[5],
			pChStat->channel_count[6],pChStat->channel_count[7],
			pChStat->channel_count[8],pChStat->channel_count[9],
			pChStat->channel_count[10],pChStat->channel_count[11],
			pChStat->channel_count[12],pChStat->channel_count[13],
			pChStat->channel_count[14],pChStat->channel_count[15],
			pChStat->total_bytes );
			
		return ;
}

enum _VALID_CODE
{
	VALID_INIT = 0,
	VALID_SOCKET_BUFFER,
	VALID_OUTOF_GOP,
	VALID_WRITE_OK
};
 
/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void *live_send_thread_func(void *arg)
{	
	VIDEO_ENTRY 			*pEntry = NULL;
	ICODEC_HEADER			*pfh = NULL;	
	CLIENT_INFO				*pClient = NULL;
	CHANNEL_STAT			*pChStat = &gLive_info.send_stat;
	int						ret = 0;
	int						client_cnt, client_cnt_reflag = 0;
	int						client_ch_cnt;
	unsigned int			channel_mask = 0;
			
	g_message("%s thread start", __FUNCTION__);
	
	while(1) {
				
		pEntry = NULL;
						
		// entry DeQueue;
		Pthread_mutex_lock(&gLive_info.vd_mutex); 
		{			
			// Queue Underflow
			if(gLive_info.video_entry_count <= 0) 
			{
				Pthread_mutex_unlock(&gLive_info.vd_mutex);
				g_usleep(10*1000); // 좀 쉬었다가 다시간다.
				continue;
			}			
			pEntry = deque_video_entry( &gLive_info );
		}
		Pthread_mutex_unlock(&gLive_info.vd_mutex);

		if(!pEntry) continue;
			
		pfh = &pEntry->frame_header;
							
#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_SEND_FRAME] & (1<<pfh->chan) )
				print_codec_header(pfh,"SSSS","frame");
#endif

		/***********************************************************************/		
		// send stream
		/***********************************************************************/		
		pClient = NULL;	
		client_ch_cnt = client_cnt = 0;		
				
		while(1)
		{	
			CHANNEL_STAT			*pCliChStat = NULL;
			int						valid_code = VALID_INIT;
									
			Pthread_mutex_lock(&gServer_info.client_mutex);
			{	// get client_info
				if(pClient == NULL)
					pClient = TAILQ_FIRST( &gServer_info.client_head );
				else
					pClient = TAILQ_NEXT(pClient, entries);
			}	
			Pthread_mutex_unlock(&gServer_info.client_mutex);
						
			if(pClient == NULL) break; 	// 모든 클라이언트를 살펴보았으니 다음 프레임으로
			if(pClient->magic_key != CLIENT_MAGIC) continue;
			if(pClient->mode != CLIENT_MODE_LIVE) continue;

			++client_cnt;
			client_ch_cnt += pClient->channel_count;
			
			pCliChStat = &pClient->ch_stat;
									
			// 관심 채널이 아니라면			
			if( !(pClient->channel_mask & (1 << pfh->chan)) 
				&& pfh->frame_type != NF_FRAME_TYPE_AUDIO ) goto not_valid;
			
			if( pfh->frame_type == NF_FRAME_TYPE_AUDIO	&& 
				( gLive_info.audio_tx == 0	// sysdb audio tx off
					|| pClient->channel_audio_ch == -1
					|| pClient->channel_audio_ch != pfh->chan
					|| pClient->ch_info[min_fps_traffic_channel( pClient )].fps_level > 6 ) )
				goto not_valid;

			// 오디오는 fps,GOP 체크를 안함.
			if( pfh->frame_type != NF_FRAME_TYPE_AUDIO )						
			{
				// 프레임 레이트 컨트롤
				if( valid_frame_rate_ctrl( pClient, pEntry) < 0 ) goto not_valid;
	
				// pframe이 GOP 밖에 있는 거라면	
				ret = valid_pframe_header( pClient, pEntry);
				if( ret == -7 || ret == -8)
					goto not_valid;
				else if( ret < 0 ) 
				{				
					valid_code = VALID_OUTOF_GOP;
					goto not_valid;
				}
			}
	
			if( valid_socket_buffer( pClient, pEntry) < 0 ) 
			{	// socket buffer가 여분이 없다면
				valid_code = VALID_SOCKET_BUFFER;
				goto not_valid;
			}

//			if( yield_frame( pClient, pEntry) < 0 )  goto yield_frame;			

			if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_DISABLE_SEND] == 0 )
			{		
				if( (ret = write_video_entry( pClient, pEntry)) >0 )
				{	
					valid_code = VALID_WRITE_OK;
					pChStat->total_bytes += ret; //채널 통계 정보
					copy_codec_header(pClient, pEntry);				
				}
				
			}else{
					valid_code = VALID_WRITE_OK;
					pChStat->total_bytes += 0; //채널 통계 정보
					copy_codec_header(pClient, pEntry);				
			}

//yield_frame:	
not_valid:

//			if(pCliChStat->saveTimeStamp.tv_sec != pChStat->curTimeStamp.tv_sec )
			if( (pChStat->curTimeStamp.tv_sec != pCliChStat->saveTimeStamp.tv_sec) ||
				 pChStat->curTimeStamp.tv_usec - pCliChStat->saveTimeStamp.tv_usec > 240000  ) // msec
			{	// 0.25초마다 통계 정보 reset;;																
				adj_fps_data(pClient);				
#ifdef DEBUG_NLIVE_LOG
				if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_FPS_DUMP] )
					dump_fps_data2(pClient);		// after

				if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_FPS_DUMP] ==  pClient->uniqueid)
					dump_fps_data(pClient);		// after

			
#endif
				reset_channel_stat( pCliChStat, pChStat->curTimeStamp);
			}

			switch (valid_code)
			{
				case VALID_WRITE_OK:
					++pCliChStat->total_count;
					++pCliChStat->channel_count[pfh->chan];
					pCliChStat->total_bytes += ret;
					break;
				case VALID_OUTOF_GOP:				
					++pCliChStat->total_error_count;
					++pCliChStat->error_count[pfh->chan];					
					break;			
				case VALID_SOCKET_BUFFER:
					pCliChStat->total_error_count += 10;
					pCliChStat->error_count[pfh->chan] += 10;
					break;
				default:
					break;
			}													


		} //while(1)


		Pthread_mutex_lock(&gLive_info.vd_mutex); 
		{			
			gLive_info.client_count_runtime = client_cnt;
			gLive_info.client_tot_ch_runtime = client_ch_cnt;
		}
		Pthread_mutex_unlock(&gLive_info.vd_mutex);
														
		/***********************************************************************/
		// update statistics
		/***********************************************************************/
		//gettimeofday(&pChStat->curTimeStamp ,NULL);
		if(pChStat->saveTimeStamp.tv_sec != pChStat->curTimeStamp.tv_sec ) 
		{											
			gLive_info.max_tx_speed	= nf_sysdb_get_uint("net.proto.maxtxspeed")*1024;
			gLive_info.audio_tx =  nf_sysdb_get_bool("audio.tx");

			if( client_cnt == 0 && client_cnt_reflag == 0)
			{
				_livemgr_reload_handoff(0);				
				client_cnt_reflag = 1;
			}else{
				client_cnt_reflag = 0;
			}
			
#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_SEND_CHSTAT] )
			print_channel_stat( pChStat, __FUNCTION__, "SSSS" );
#endif
			reset_channel_stat( pChStat, pChStat->curTimeStamp);

		}
		
		++pChStat->total_count;
		++pChStat->channel_count[pfh->chan];
				
		copy_codec_header2( pEntry);

		++gLive_info.send_count;
		gLive_info.send_bytes += pEntry->data_size;

		/***********************************************************************/
		// free frame data
		/***********************************************************************/
		free_video_entry(pEntry);
		
	}
	
	/***************************
	* todo : ERROR 처리 프로세스
	****************************/
	
		
	g_message("%s thread end", __FUNCTION__);
	pthread_exit(NULL);			
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static void *live_read_thread_func(void *arg) 
{
	int 			size, read_size, malloc_size;
	VIDEO_ENTRY 	*pEntry = NULL, *pDeQueEntry = NULL;	
	ICODEC_HEADER	fh;
	CHANNEL_STAT	*pChStat = &gLive_info.read_stat;	

	g_message("%s thread start", __FUNCTION__);
		
	while(1) {
		
		pEntry = pDeQueEntry = NULL;
		malloc_size	= size = 0;

		pEntry = g_async_queue_pop( gLive_info.video_cb_queue );

		memcpy( &fh, &pEntry->frame_header, sizeof(ICODEC_HEADER));

#ifdef DEBUG_NLIVE_LOG
			if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_FRAME] & (1<<fh.chan) )
				print_codec_header(&fh,"RRRR","frame");
#endif
			
		pChStat->total_bytes += pEntry->data_size;

		++gLive_info.read_count;
		gLive_info.read_bytes += pEntry->data_size;
		
#ifdef DEBUG_NLIVE_LOG
		if( fh.frame_type == NF_FRAME_TYPE_AUDIO ) // audio dump
			if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_DUMP_AUDIO] )
				nf_debug_hexdump( pEntry->frame_data, 128);
#endif

		// entry add;
		Pthread_mutex_lock(&gLive_info.vd_mutex);
		{
			enque_video_entry( &gLive_info, pEntry);
			// 만약 entry overflow가 일어났다면
			while(gLive_info.video_entry_size > (2*1024*1024) )
			{	// 버퍼는 2메가로 유지
				pDeQueEntry = deque_video_entry( &gLive_info );
				++gLive_info.overflow_count;				
	
				free_video_entry(pDeQueEntry);
								
#ifdef DEBUG_NLIVE_LOG
				if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_OVERFLOW] )
					g_message("%s Entry OVERFLOW count[%d] queue count[%d] bytes[%d]", __FUNCTION__,
							gLive_info.overflow_count,
							gLive_info.video_entry_count,
							gLive_info.video_entry_size );
#endif
			}
		}
		Pthread_mutex_unlock(&gLive_info.vd_mutex);
										
		// 통계정보
		//gettimeofday(&pChStat->curTimeStamp ,NULL);
		if(pChStat->saveTimeStamp.tv_sec != pChStat->curTimeStamp.tv_sec ) 
		{								

#ifdef DEBUG_NLIVE_LOG
			if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_CHSTAT] )
				print_channel_stat( pChStat, __FUNCTION__, "RRRR" );
#endif
			reset_channel_stat( pChStat, pChStat->curTimeStamp);
		}

		++pChStat->total_count;
		++pChStat->channel_count[fh.chan];
			
		g_usleep(10*1000);

	}
	
error_read:
	free_video_entry(pEntry);		

	g_message("%s thread end", __FUNCTION__);	
	pthread_exit(NULL);
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
static int init_livemgr()
{
  int encoder_count = 0;
  extern VIDEO_ENTRY *_iframe_video_entry[64];
	
	nf_debug_category_add( "nlive", _DEBUG_NLIVE_str, _DEBUG_NLIVE_log, DEBUG_NLIVE_IDX_NR);

	memset( &gLive_info, 0x00, sizeof(LIVE_INFO));	
	memset( &gRtp_live_info, 0x00, sizeof(LIVE_INFO));	

	Pthread_mutex_init(&gLive_info.vd_mutex, NULL); 
	Pthread_mutex_init(&gRtp_live_info.vd_mutex, NULL); 
	Pthread_mutex_init(&gRtp_live_info.write_mutex, NULL); 

	memset( _iframe_video_entry, 0x00, sizeof(_iframe_video_entry) ) ;

//	Pthread_mutex_init(&live_frame_mutex, NULL); 

#if 1
	TAILQ_INIT(&gLive_info.video_head[0]);
	TAILQ_INIT(&gLive_info.video_head[1]);
	TAILQ_INIT(&gRtp_live_info.video_head[0]);
	TAILQ_INIT(&gRtp_live_info.video_head[1]);
#else  
	TAILQ_INIT(&gLive_info.video_head);
//	TAILQ_INIT(&gLive_info.video_head[1]);
	TAILQ_INIT(&gRtp_live_info.video_head);
//	TAILQ_INIT(&gRtp_live_info.video_head[1]);
#endif

#if 1
   	gLive_info.liveaudio_fd = HI_MPI_AI_GetFd(0, 0);

	if(gLive_info.liveaudio_fd < 0) 
	{
		gLive_info.live_fd = -1;		
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );		
//		return -1;
	}else{
		g_message("%s open liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
	}
	gLive_info.cb_hi_aud_pb_fxn_t=playback_audio_cb_net;

#else	// choissi IPX
	//gLive_info.live_fd = Open(LIVE_DSPDEV, O_RDWR, 0);
	//gLive_info.liveaudio_fd = Open("/dev/dsp", O_WRONLY, 0);
	
	gLive_info.liveaudio_fd = nf_dev_open_dsp_write();
	if(gLive_info.liveaudio_fd < 0) 
	{
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );

		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );

		return -1;
	}else{
		g_message("%s open liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
	}
#endif

#if defined(_ANF_1648)||defined(_ANF_0824)

    /*TODO*/

	gLive_info.liveaudio_fd = Open( "/dev/twaudio_net", O_WRONLY, 0);

	if(gLive_info.liveaudio_fd < 0) 
	{
		//Close(gLive_info.live_fd);//FIXME 
		gLive_info.live_fd = -1;		
		g_warning("%s failed!! liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );		
		return -1;
	}else{
		g_message("%s open liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd );
	}

#endif

#if defined(__RA_USED__)
	gLive_info.video_cb_queue = g_async_queue_new();	 
	if(!gLive_info.video_cb_queue)
	{
		g_warning("%s failed!! video_cb_queue", __FUNCTION__);
		return -1;		
	}
#endif
				
	// FIXME
#if defined(__RA_USED__)
	gLive_info.max_tx_speed	= nf_sysdb_get_uint("net.proto.maxtxspeed")*1024;
	g_message("%s max_tx_speed[%d]",__FUNCTION__, gLive_info.max_tx_speed );

	gLive_info.client_count_runtime	= 0;
	gLive_info.client_tot_ch_runtime = 0;

	// FIXME	
	gLive_info.audio_tx =  nf_sysdb_get_bool("audio.tx");

	#if defined(ENABLE_AUD_HI_CHIP)
		nf_HI_aud_registerHandoff( 0x00000000 , &_livemgr_put_frame );
	#else
	  	nf_rec_audio_register_handoff( 0x00000000 , &_livemgr_put_frame );
	#endif
	nf_record_register_handoff( 0x00000000,  &_livemgr_put_frame, 0 );

#else /* ndef __RA_USED__ */

	gRtp_live_info.max_tx_speed	= nf_sysdb_get_uint("net.proto.maxtxspeed")*1024;
	g_message("%s max_tx_speed[%d]",__FUNCTION__, gRtp_live_info.max_tx_speed );

	gRtp_live_info.client_count_runtime	= 0;
	gRtp_live_info.client_tot_ch_runtime = 0;

	// FIXME	
	gRtp_live_info.audio_tx =  nf_sysdb_get_bool("audio.tx");
	#if defined(ENABLE_AUD_HI_CHIP)
		nf_HI_aud_registerHandoff( 0x0000ffff , &_livemgr_put_frame );
	#else
  		nf_rec_audio_register_handoff( 0x00000000 , &_livemgr_put_frame );
	#endif
  	nf_record_register_handoff( 0x0000ffff/*0x00000000*/,  &_livemgr_put_frame, 0 );

#endif

	return 1;
}


guint _livemgr_reload_handoff( guint ch_mask )
{
	guint cur_mask = ch_mask;
	guint cur_audio_mask = 0;
	
	CLIENT_INFO	*pClient = NULL;
	
	Pthread_mutex_lock(&gServer_info.client_mutex);
	while(1)
	{		
		if(pClient == NULL)
			pClient = TAILQ_FIRST( &gServer_info.client_head );
		else
			pClient = TAILQ_NEXT(pClient, entries);
		
		if(pClient == NULL) break; 	// 모든 클라이언트를 살펴보았으니 다음 프레임으로
		if(pClient->magic_key != CLIENT_MAGIC) continue;
		if(pClient->mode != CLIENT_MODE_LIVE) continue;		

		cur_mask |= pClient->channel_mask;
		
		if( pClient->channel_audio_ch != -1)
			cur_audio_mask |= 1<<pClient->channel_audio_ch;
	}	
	Pthread_mutex_unlock(&gServer_info.client_mutex);

	g_message("%s ch_mask[0x%08x] cur_mask[0x%08x]  cur_audio_mask[%0x08x]",__FUNCTION__, 
					ch_mask, cur_mask, cur_audio_mask);
	
	#if defined(ENABLE_AUD_HI_CHIP)
		nf_HI_aud_registerHandoff( cur_mask , &_livemgr_put_frame );
	#else
		nf_rec_audio_register_handoff( cur_audio_mask , &_livemgr_put_frame );
	#endif
	nf_record_register_handoff( cur_mask,  &_livemgr_put_frame, 0 );
	
	return cur_mask;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
void rtp_live_reload_handoff(unsigned int ch_mask, unsigned int audio_ch_mask,  unsigned int mobile_mask)
{
    #if 0    //+20090813, cultfactory
    g_message("[NET]%s: video ch:0x%08x/audio ch:0x%08x.", 
              __FUNCTION__, ch_mask, audio_ch_mask);
    ch_mask = 0xffff;
    #endif

	#if defined(ENABLE_AUD_HI_CHIP)
		nf_HI_aud_registerHandoff( ch_mask , &_livemgr_put_frame );
	#else
		nf_rec_audio_register_handoff( audio_ch_mask , &_livemgr_put_frame );
	#endif
	nf_record_register_handoff( ch_mask ,  &_livemgr_put_frame, mobile_mask );
}

static int out_fd = NULL;
int tmp_tmp = 0;
int aaaa = 0;


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int _livemgr_put_frame( gpointer frame)
{
	
	VIDEO_ENTRY 		*pEntry  = NULL;

	g_return_val_if_fail( frame, 0);

#ifndef NLIVE_DIRECT_QPUSH
	g_return_val_if_fail( gLive_info.video_cb_queue, 0);
#endif

	pEntry = new_video_entry( frame );
	g_return_val_if_fail( pEntry, NULL);

	

#ifndef NLIVE_DIRECT_QPUSH

	g_async_queue_push (gLive_info.video_cb_queue, pEntry );

#else
#if defined(__RA_USED__)
{

	VIDEO_ENTRY 	*pDeQueEntry = NULL;	
	ICODEC_HEADER	fh;
	CHANNEL_STAT	*pChStat = &gLive_info.read_stat;	
				
	memcpy( &fh, &pEntry->frame_header, sizeof(ICODEC_HEADER));

#ifdef DEBUG_NLIVE_LOG
	if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_FRAME] & (1<<fh.chan) )
		print_codec_header(&fh,"RRRR","frame");
#endif
			
	pChStat->total_bytes += pEntry->data_size;

	++gLive_info.read_count;
	gLive_info.read_bytes += pEntry->data_size;
		
#ifdef DEBUG_NLIVE_LOG
	if( fh.frame_type == NF_FRAME_TYPE_AUDIO ) // audio dump
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_DUMP_AUDIO] )
			nf_debug_hexdump( pEntry->frame_data, 128);
#endif

	// entry add;
	Pthread_mutex_lock(&gLive_info.vd_mutex);
//  Pthread_mutex_lock(&live_frame_mutex);
	{
		enque_video_entry( &gLive_info, pEntry);
		// 만약 entry overflow가 일어났다면
		while(gLive_info.video_entry_size > (2*1024*1024) )
		{	// 버퍼는 2메가로 유지
			pDeQueEntry = deque_video_entry( &gLive_info );
			++gLive_info.overflow_count;				

			free_video_entry(pDeQueEntry);
							
#ifdef DEBUG_NLIVE_LOG
			if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_OVERFLOW] )
				g_message("%s Entry OVERFLOW count[%d] queue count[%d] bytes[%d]", __FUNCTION__,
						gLive_info.overflow_count,
						gLive_info.video_entry_count,
						gLive_info.video_entry_size );
#endif
		}
	}
	Pthread_mutex_unlock(&gLive_info.vd_mutex);
//	Pthread_mutex_unlock(&live_frame_mutex);
  
										
	// 통계정보
	//gettimeofday(&pChStat->curTimeStamp ,NULL);
	if(pChStat->saveTimeStamp.tv_sec != pChStat->curTimeStamp.tv_sec ) 
	{								

#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_CHSTAT] )
			print_channel_stat( pChStat, __FUNCTION__, "RRRR" );
#endif
		reset_channel_stat( pChStat, pChStat->curTimeStamp);
	}

	++pChStat->total_count;
	++pChStat->channel_count[fh.chan];						
}
#else
{

	VIDEO_ENTRY 	*pDeQueEntry = NULL;	
	ICODEC_HEADER	fh;
	CHANNEL_STAT	*pChStat = &gRtp_live_info.read_stat;	
				
	memcpy( &fh, &pEntry->frame_header, sizeof(ICODEC_HEADER));

#ifdef DEBUG_NLIVE_LOG
	if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_FRAME] & (1<<fh.chan) )
		print_codec_header(&fh,"RRRR","frame");
#endif
			
	pChStat->total_bytes += pEntry->data_size;

	++gRtp_live_info.read_count;
	gRtp_live_info.read_bytes += pEntry->data_size;
		
#ifdef DEBUG_NLIVE_LOG
	if( fh.frame_type == NF_FRAME_TYPE_AUDIO ) // audio dump
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_DUMP_AUDIO] )
			nf_debug_hexdump( pEntry->frame_data, 128);
#endif

	// entry add;
	Pthread_mutex_lock(&gRtp_live_info.vd_mutex);
 // Pthread_mutex_lock(&live_frame_mutex);
	{
		enque_video_entry( &gRtp_live_info, pEntry);
		// 만약 entry overflow가 일어났다면
#if 0
    g_message("%s Entry OVERFLOW count[%d] queue count[%d] bytes[%d]", __FUNCTION__,
        gRtp_live_info.overflow_count,
        gRtp_live_info.video_entry_count,
        gRtp_live_info.video_entry_size );
#endif
    
		while(gRtp_live_info.video_entry_size > (2*1024*1024) )
		{	// 버퍼는 2메가로 유지
			pDeQueEntry = deque_video_entry( &gRtp_live_info );
			++gRtp_live_info.overflow_count;				

			free_video_entry(pDeQueEntry);
							
#ifdef DEBUG_NLIVE_LOG
			if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_OVERFLOW] )
				g_message("%s Entry OVERFLOW count[%d] queue count[%d] bytes[%d]", __FUNCTION__,
						gRtp_live_info.overflow_count,
						gRtp_live_info.video_entry_count,
						gRtp_live_info.video_entry_size );
#endif
		}
	}
	Pthread_mutex_unlock(&gRtp_live_info.vd_mutex);
//	Pthread_mutex_unlock(&live_frame_mutex);
  
										
	// 통계정보
	//gettimeofday(&pChStat->curTimeStamp ,NULL);
	if(pChStat->saveTimeStamp.tv_sec != pChStat->curTimeStamp.tv_sec ) 
	{								

#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_READ_CHSTAT] )
			print_channel_stat( pChStat, __FUNCTION__, "RRRR" );
#endif
		reset_channel_stat( pChStat, pChStat->curTimeStamp);
	}

	++pChStat->total_count;
	++pChStat->channel_count[fh.chan];						
}
#endif
#endif	// else NLIVE_DIRECT_QPUSH
	
	return 1;
}


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
#if 0
void _livemgr_init_fps_data ( CLIENT_INFO *pClient )
{
	unsigned char fps, res;
	unsigned int  timestamp;
	int i;

	pClient->channel_count = 0;
	pClient->total_traffic = 0;
	pClient->total_fps_traffic = 0;	
	
	memset( pClient->ch_info, 0x00, sizeof(pClient->ch_info));
	
	for(i=0; i< NUM_CHANNEL; i++)
	{	
		CHANNEL_INFO *pChannel = &pClient->ch_info[i];
		
		fps = conv_fps_idx(gLive_info.iframe_header[i].frame_rate);
		res = gLive_info.iframe_header[i].resolution & 0x0f;
		timestamp = gLive_info.iframe_header[i].timestamp;
		
		pChannel->ch_id			= i;
		
		if( !(pClient->channel_mask & (1 << i)) ) continue;
			
		if( timestamp == 0) // 아직 iframe을 받은 적이 없다!
		{
			fps = conv_fps_idx(NF_FPS_CR32); 
			res = NF_RES_NTSC_CIF;	// 기본값;
		}			
						
		pChannel->framerate		= fps;
		pChannel->resolution	= res;
		pChannel->traffic 		= cal_traffic( fps, res);
		pClient->total_traffic += pChannel->traffic;
		
		pChannel->fps_level		= fps;
		pChannel->fps_traffic	= pChannel->traffic;
		pClient->total_fps_traffic += pChannel->traffic;
		++pClient->channel_count;
		
#ifdef DEBUG_NLIVE_LOG
		if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_INIT_CLIENT] )
			g_message("%s ch[%2d] fps[%2d] res[%d] taffic[%d]", __FUNCTION__, 
						i, fps, res, pChannel->traffic);
#endif		
	}
	
	pClient->fps_accel = 1;
	
#ifdef DEBUG_NLIVE_LOG
	if( _DEBUG_NLIVE_log[DEBUG_NLIVE_IDX_INIT_CLIENT] )
		g_message("%s totaltaffic[%d] fps_total[%d]", __FUNCTION__, 
				pClient->total_traffic, 
				pClient->total_fps_traffic);
#endif		
	
}
#endif


/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int _livemgr_close()
{
	if( gLive_info.live_fd > 0)
	{
		g_message("%s close live_fd[%d]", __FUNCTION__, gLive_info.live_fd);
		Close(gLive_info.live_fd); 
		gLive_info.live_fd = -1;
	}

	if( gLive_info.liveaudio_fd > 0)
	{
		g_message("%s close liveaudio_fd[%d]", __FUNCTION__, gLive_info.liveaudio_fd);
		Close(gLive_info.liveaudio_fd); 
		gLive_info.liveaudio_fd = -1;
	}

	return 1;
}

#ifdef DEBUG_NLIVE_REF_TIMER
static gboolean
_livemgr_stat_refresh_timer(gpointer data)
{	

	CHANNEL_STAT	*pChStat1 = &gLive_info.send_stat;
	CHANNEL_STAT	*pChStat2 = &gLive_info.read_stat;	

	gettimeofday(&pChStat1->curTimeStamp ,NULL);
	
	pChStat2->curTimeStamp = pChStat1->curTimeStamp;
	
	//g_message("%d.%06d", pChStat2->curTimeStamp.tv_sec, pChStat2->curTimeStamp.tv_usec);
	
	return TRUE;
}

static gboolean
rtp_stat_refresh_timer(gpointer data)
{	

	CHANNEL_STAT	*pChStat1 = &gRtp_live_info.send_stat;
	CHANNEL_STAT	*pChStat2 = &gRtp_live_info.read_stat;	

	gettimeofday(&pChStat1->curTimeStamp ,NULL);
	
	pChStat2->curTimeStamp = pChStat1->curTimeStamp;
	
	//g_message("%d.%06d", pChStat2->curTimeStamp.tv_sec, pChStat2->curTimeStamp.tv_usec);
	
	return TRUE;
}

#endif

/******************************************************************************* 
 * Function  :
 * Prototype : 
 * Arguments : 
 * Return : 
 ******************************************************************************/ 
int _livemgr_init()
{
	pthread_t tid;
		
	if( init_livemgr()<0 )
	{
		return -1;
	}

#ifdef DEBUG_NLIVE_REF_TIMER
#if defined(__RA_USED__)
	nf_timer_add( 33, _livemgr_stat_refresh_timer, NULL);
#else
	nf_timer_add( 33, rtp_stat_refresh_timer, NULL);
#endif
#endif	

#if 0
#ifndef DEBUG_NLIVE_NO_SEND_THREAD
	if(Pthread_create(&tid, NULL, live_send_thread_func, NULL))
	{	
		return -2;
	}	
	gLive_info.send_tid = tid;
	Pthread_detach(tid);
	g_message("%s thread created! [live_send_func]", __FUNCTION__);
#endif
#else

#endif

#ifndef DEBUG_NLIVE_NO_READ_THREAD	

#ifndef NLIVE_DIRECT_QPUSH
	if(Pthread_create(&tid, NULL, live_read_thread_func, NULL)) {	
		return -3;	
	}	
	gLive_info.read_tid = tid;
	Pthread_detach(tid);
#endif
	g_message("%s thread created! [live_read_func]", __FUNCTION__);
	
#endif

	return 1;
}


#ifdef DEBUG_NLIVE_JBSHELL

static char nlive_max_tx_help[] = "nlive_max_tx [max_tx_speed(k)]";
static int nlive_max_tx(int argc, char **argv)
{	
	guint max_tx_speed = 0;			
	
	if(argc < 2){
		printf("%s\n",nlive_max_tx_help);
		return -1;
	}
	max_tx_speed = strtoul(argv[1],NULL,0);	
	
	if( max_tx_speed <= 8192 )
	{
		nf_sysdb_set_uint("net.proto.maxtxspeed", max_tx_speed);
		gLive_info.max_tx_speed = max_tx_speed*1024;
	}
							
	printf("max_tx_speed[%d] client[%d] tot_ch[%d]\n",
				gLive_info.max_tx_speed,
				gLive_info.client_count_runtime,
				gLive_info.client_tot_ch_runtime );
	return 0;
}
__commandlist(nlive_max_tx,"nlive_max_tx", nlive_max_tx_help, nlive_max_tx_help);


static char nlive_log_help[] = "nlive_log [idx] [val]";
static int nlive_log(int argc, char **argv)
{	
	gint idx, val,i;
	
	if(argc < 3){
		printf("%s\n",nlive_log_help);
		nf_debug_dump("nlive");
		return -1;
	}
		
	idx = strtol(argv[1],NULL,0);
	val = strtol(argv[2],NULL,0);
	
	g_return_val_if_fail( idx <= DEBUG_NLIVE_IDX_NR, -1 );
	
	if( idx == DEBUG_NLIVE_IDX_NR )
	{
		for(i=0;i<DEBUG_NLIVE_IDX_NR;i++)		
			_DEBUG_NLIVE_log[i] = val;
	}else{
		_DEBUG_NLIVE_log[idx] = val;
	}
	
	return 0;
}
__commandlist(nlive_log, "nlive_log",  nlive_log_help, nlive_log_help);

#endif
