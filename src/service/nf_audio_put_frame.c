#include <gobj.h>
#include <gobjmedia.h>
#include "nf_common.h"

#include <novatek/hdal.h>
#include <novatek/hd_debug.h>

#include "nf_codec_header.h"
#include "nf_audio_novatek.h"
#include "nf_audio_common.h"
#include "nf_audio.h"
#include "nf_audio_put_frame.h"

#include <libicmem.h>

/*
	_nf_aud_putSST_put_gst_buffer
*/
#define NF_AUDIO_DEF_SPEED 8320
gboolean nf_audio_put_frame_gst_buffer_sst( guint ch, gpointer frame, NF_AUDIO_PARAM *pstParam)
{
#if 1
	g_return_val_if_fail( ch < NUM_ANALOG_CHANNEL, 0);
	g_return_val_if_fail( frame != NULL, 0);

	gint                clen, ret, stream_id;
	GobjBuddyBuffer    *gst_buf = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER       *pheader = (ICODEC_HEADER *)gobj_buddy_buffer_buf_get_addr( gst_buf );

	clen = ALIGN( gint, sizeof(ICODEC_HEADER)+NF_AUDIO_DEF_SPEED , 64 );


	{
		GTimeVal ftval = { pheader->timestamp, pheader->timestampl*5*1000};
		GTimeVal ctval = pstParam->timestamp;

		guint64 ftmp, ctmp, diff;
		gchar c;

		if( ctval.tv_sec != 0)
			g_time_val_add(&ctval, 1040000);
		else
			ctval = ftval;

		ftmp = GTIMEVAL_TO_GUINT64(ftval);
		ctmp = GTIMEVAL_TO_GUINT64(ctval);

		if( ftmp >= ctmp){ // ns
			diff = (ftmp - ctmp);
			c='F';
		}else {
			diff = ctmp - ftmp;
			c='C';
		}

		if( diff > 1100000000/5 )
		{
			pstParam->timestamp.tv_sec = 0;
			pstParam->timestamp.tv_usec = 0;

			g_warning("%s ftval[%d.%06d] ctval[%d.%06d] diff(%4lld)ms %c", __FUNCTION__,
							ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
							(diff>>20), c);

			if( c == 'C' )
				return 0;
		}

		#ifdef DEBUG_REC_AUDIO_LOG
			if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_DIFF] & (1 << ch) )
				g_message("%s ftval[%d.%06d] ctval[%d.%06d] diff(%4lld)ms %c", __FUNCTION__,
							ftval.tv_sec, ftval.tv_usec, ctval.tv_sec, ctval.tv_usec,
							(diff>>20), c);
		#endif
	}

	if( pstParam->timestamp.tv_sec != 0 )
	{
		g_time_val_add(&pstParam->timestamp, 1040000);

		pheader->timestamp  = pstParam->timestamp.tv_sec;
		pheader->timestampl = pstParam->timestamp.tv_usec/1000/5;

	}else{
		pstParam->timestamp.tv_sec = pheader->timestamp;
		pstParam->timestamp.tv_usec = pheader->timestampl*5*1000;
	}

	// header FIXME!!
	pheader->flags = pstParam->u8Reason;
	pheader->frame_rate = NF_FPS_CR01;

	#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUT_SST] & (1 << ch) )
			//_nf_HI_aud_dump_icodec_header("sst_put", pheader);
	#endif

	if( pstParam->s32StmId >= 0)
	{
		{
			void *tmp_gst_ret = NULL;
			tmp_gst_ret = g_object_ref(gst_buf);
			if(tmp_gst_ret == NULL)
				fprintf(stderr, "- minto - [%s:%d] gst_buffer_ref ret is NULL\n", __FUNCTION__, __LINE__);
		}
		ret = sst_record_put_frame( pstParam->s32StmId, gobj_buddy_buffer_buf_get_addr(gst_buf) );
		if(ret){
			//_nf_HI_aud_dump_icodec_header("ERR frame", pheader);
			//g_message("sst_record_put_frame result[%d](%s)\n", ret, sst_get_error_string(ret));
			g_object_unref(gst_buf);
		}
	}
#endif

	return TRUE;
}

gboolean nf_audio_put_frame(gint ch_num, gpointer frame)
{
#if 1
	GobjBuddyBuffer *buffer = (GobjBuddyBuffer *)frame;
	ICODEC_HEADER *pheader = gobj_buddy_buffer_buf_get_addr(buffer);
	NF_AUDIO_QDATA *pstQdata=NULL;
	GTimeVal tv;

	g_return_val_if_fail (frame != 0, FALSE);
	g_return_val_if_fail (ch_num >= 0 && ch_num < NUM_ANALOG_CHANNEL, FALSE);

	gettimeofday((struct timeval *)&tv, NULL);

	#ifdef ENABLE_PUT_FRAME_MAX_QUEUE
		while( g_async_queue_length( _nf_HI_aud->stAud.queue ) > (16*5) ) g_usleep(1000);
	#endif

	if ( pheader->frame_type != NF_FRAME_TYPE_AUDIO ) {
		//nf_HI_aud_nf_HI_aud_dump_icodec_header("INV_AUDIO", pheader);
		return FALSE;
	}

	#ifdef DEBUG_REC_AUDIO_LOG
		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI] )
			//nf_HI_aud_nf_HI_aud_dump_icodec_header("AUDIO", pheader);

		if( _DEBUG_REC_AUDIO_log[DEBUG_REC_AUDIO_IDX_PUTAPI_HEX] )
			nf_debug_hexdump( gobj_buddy_buffer_buf_get_addr(buffer) , 0x100);
	#endif
	
	pstQdata = nf_audio_CreateQdata_gst_buffer(buffer);

	g_return_val_if_fail (pstQdata != 0, FALSE);

	nf_audio_sendQdata(pstQdata, nf_audio_getRecQueue());

	g_object_unref(buffer);
#endif

	return TRUE;
}

