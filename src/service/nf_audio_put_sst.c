#include <gobj.h>
#include <gobjmedia.h>

#include "nf_common.h"

#include <novatek/hdal.h>
#include <novatek/hd_debug.h>

#include "nf_codec_header.h"
#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"
#include "nf_audio_put_sst.h"

#include <libicmem.h>

/**
	Extern Function Definition!!
**/
extern gboolean nf_sysman_get_encoder_mode(void);

gboolean nf_audio_SSTput(int s32Chn, NF_AUDIO_QDATA *pstQdata, NF_AUDIO_PARAM *pstParam)
{
	#if defined(ENABLE_GST_VERSION_UP)
		GstBuffer *gstBuf=NULL;
		GstMapInfo info;
	#else
		GobjBuddyBuffer *gstBuf = NULL;
	#endif
	ICODEC_HEADER *ih = NULL;

	int gstSize;
	unsigned long long preTimestamp, diffTimestamp;
	gboolean bTime;

	g_assert( NULL != pstQdata );
	g_assert( NULL != pstParam );

	preTimestamp = pstParam->u64Timestamp;

	#if defined(ENABLE_ARCH_A64)
		NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] CH[%02d] Pre[%lu] Now[%lu]", 
					__FUNCTION__, s32Chn, pstParam->u64Timestamp, pstQdata->u64Start);
	#else
		NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] CH[%02d] Pre[%lu] Now[%llu]", 
					__FUNCTION__, s32Chn, pstParam->u64Timestamp, pstQdata->u64Start);
	#endif

	/* Update timestamp */
	if(preTimestamp == 0) {
		preTimestamp = pstQdata->u64Start;
	}
	else {
		preTimestamp += NF_AUD_TIME_UNIT;
	}

	if(pstQdata->u64Start >= preTimestamp) {
		diffTimestamp = pstQdata->u64Start - preTimestamp;
		bTime = TRUE;
	}
	else {
		diffTimestamp = preTimestamp - pstQdata->u64Start;
		bTime = FALSE;
	}

	NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] Diff[%llu] Flag[%d]", __FUNCTION__, diffTimestamp, bTime);

	#if 0		// Need Turning
		if(diffTimestamp > 200000 ) {
			NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] Frm[%llu] Cur[%lu] Diff[%llu] [%d]", __FUNCTION__,
												pstQdata->u64Start,
												pstParam->u64Timestamp,
												diffTimestamp,
												bTime);

			pstParam->u64Timestamp = 0;

			if(bTime == FALSE) {
				/* Skip */
				g_warning("[%s] Audio frame skip.", __FUNCTION__ );

				return TRUE;
			}
		}
	#endif

	/* Align address of buffer */
	gstSize = ALIGN(int, (sizeof(ICODEC_HEADER) + pstQdata->s32Len), 32);

	/* Alloc stream buffer */
	gstBuf = nf_audio_gst_buffer(gstSize, FALSE);
	if(gstBuf == NULL)
		return FALSE;

	#if defined(ENABLE_GST_VERSION_UP)
		gst_buffer_map(gstBuf, &info, GST_MAP_WRITE);
		memcpy(info.data + sizeof(ICODEC_HEADER), pstQdata->pData, (size_t)pstQdata->s32Len);
		gst_buffer_unmap(gstBuf, &info);
	#else
		/* Copy stream buffer. */
		memcpy( gobj_buddy_buffer_buf_get_addr(gstBuf) + sizeof(ICODEC_HEADER), pstQdata->pData, (size_t)pstQdata->s32Len);
	#endif

	/* Set codec header. */
	#if defined(ENABLE_GST_VERSION_UP)
		ih=(ICODEC_HEADER *)info.data;
	#else
		ih=(ICODEC_HEADER *) gobj_buddy_buffer_buf_get_addr(gstBuf);
		g_assert(NULL != ih);
	#endif

	ih->chan = (guchar)s32Chn;
	ih->flags = pstParam->u8Reason;

	ih->codec = NF_CODEC_TYPE_URAW;
	ih->version = NF_CODEC_VERSION_1;
	ih->frame_size = (guint)pstQdata->s32Len;
	ih->frame_type = NF_FRAME_TYPE_AUDIO;
	ih->resolution = 0;
	ih->frame_rate = NF_FPS_CR01;

	/* Set timestamp */
	if(pstParam->u64Timestamp == 0) {
		pstParam->u64Timestamp = pstQdata->u64Start;
	}
	else {
		pstParam->u64Timestamp += NF_AUD_TIME_UNIT;
	}

	#if 0   // hisilicon
		ih->timestamp = (guint) (pstParam->u64Timestamp / 1000000);
		ih->timestampl = (guchar) ((pstParam->u64Timestamp % 1000000 ) / 5000 );
	#else   // novatek
		ih->timestamp = (guint)(pstQdata->u64Start);
		ih->timestampl = (guchar)(pstQdata->u64Startl / 1000 / 5);
	#endif

	nf_audio_dump_icodec_header_extern(DEBUG_NF_AUDIO_IDX_SST, ih, pstQdata->s32Chn);

	if(pstParam->s32StmId >= 0) {
		if(sst_record_put_frame( pstParam->s32StmId, (struct icodec_header_t *)ih)) {
			g_warning("[%s][%d] SST Put error!", __FUNCTION__, __LINE__);

			g_object_unref(gstBuf );
		}
		#if 0
			else {
				g_message("[%s][%d] SST Put!! chan[%d -> %d] StreamID[%d] Size[%d]", 
							__FUNCTION__, __LINE__, ih->chan, pstParam->u8Chn, pstParam->s32StmId, ih->frame_size);
		}
		#endif
	}
	else {
		g_object_unref(gstBuf);
	}

	return TRUE;
}

gboolean nf_audio_SSTctrl(NF_AUDIO_PARAM *pstOld,  NF_AUDIO_PARAM *pstNew)
{
	int i=0, ret=0;

	g_return_val_if_fail (pstOld != NULL, FALSE);
	g_return_val_if_fail (pstNew != NULL, FALSE);

	#if defined(_UTM7G_1648D) || defined(_UTM7G_0824D) || defined(_UTM7G_0412D)
		if(nf_sysman_get_encoder_mode() == TRUE) {
			return TRUE;
		}
	#endif

	for(i=0; i<NUM_ACTIVE_CH; i++) {
		NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] Old CH[%02d][%02d] Reason[%d] PreTime[%d] PreClose[%d] STM[%d]", __FUNCTION__, i,
												pstOld[i].u8Chn,
												pstOld[i].u8Reason,
												pstOld[i].u8PreRecTime,
												pstOld[i].u8PreRecClose,
												pstOld[i].s32StmId);

		NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] New CH[%02d][%02d] Reason[%d] PreTime[%d] PreClose[%d] STM[%d]", __FUNCTION__, i,
												pstNew[i].u8Chn,
												pstNew[i].u8Reason,
												pstNew[i].u8PreRecTime,
												pstNew[i].u8PreRecClose,
												pstNew[i].s32StmId);

		/* Open -> Close */
		if( ( pstOld[i].s32StmId >= 0 ) &&
			( ( pstNew[i].u8Chn == 0xFF ) ||
			  ( pstNew[i].u8Reason == NF_RECORD_REASON_NOTHING ) ||
			  ( pstOld[i].u8Reason != pstNew[i].u8Reason ) ||
			  ( pstOld[i].u8PreRecTime != pstNew[i].u8PreRecTime ) ) ) {

			NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] sst_record_close CH[%2d][%2d] SID[%2d] pre_flush[%d]", __FUNCTION__, i, 
												(i + NF_AUDIO_SST_START_CH_NO),
												pstOld[i].s32StmId,
												pstNew[i].u8PreRecTime);

			ret = sst_record_close(pstOld[i].s32StmId, (unsigned char)(pstNew[i].u8PreRecClose));

			g_assert( 0 == ret );

			/* Set param */
			pstOld[i].s32StmId = -1;
			pstOld[i].u64Timestamp = 0;
		}

		/* Close -> Open */
		if((pstNew[i].u8Chn != 0xFF ) && (pstOld[i].s32StmId < 0) &&
			((pstNew[i].u8PreRecTime > 0) || (pstNew[i].u8Reason != NF_RECORD_REASON_NOTHING))) {

			int stmID = -1;

			/* If open the stream, fail */
			g_assert(pstOld[i].s32StmId < 0);

			stmID = sst_record_open((guchar)(i + NF_AUDIO_SST_START_CH_NO),     // ch
									 pstNew[i].u8PreRecTime,                    // pre_rec_time
									 pstNew[i].u8Reason,                        // rec_reason
									 NF_CODEC_TYPE_URAW,                        // codec
									 0,                                         // resolution
									 NF_FPS_CR01,                               // frame rate
									 0);                                        // quality

			NF_AUD_DBG(DBG_MSG_AUD_SST, "[%s] sst_record_open  CH[%2d] SID[%2d] Reason[%d] PreTime[%d]", __FUNCTION__,
																i + NF_AUDIO_SST_START_CH_NO,
																stmID,
																pstNew[i].u8Reason,
																pstNew[i].u8PreRecTime );

			if(stmID < 0) {
				g_message( "[%s] SST_record_open result[%d](%s) ###", 
							__FUNCTION__, stmID, sst_get_error_string(stmID));
			}

			/* Fail open the SST */
			g_assert(stmID >= 0 || stmID == -SST_ERR_DISKFULL);

			pstOld[i].s32StmId = stmID;
		}

		pstOld[i].u8Chn         = pstNew[i].u8Chn;
		pstOld[i].u8Reason      = pstNew[i].u8Reason;
		pstOld[i].u8PreRecTime  = pstNew[i].u8PreRecTime;
		pstOld[i].u8PreRecClose = pstNew[i].u8PreRecClose;

	} /* for */

	return TRUE;
}

