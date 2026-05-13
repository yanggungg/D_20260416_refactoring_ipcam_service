/*
 * ITX Security Co.,Ltd.
 *  System software section
 *
 *  2011-07-13 jykim
 */

#ifndef _GST_MRTP_AUDIO_C__
#define _GST_MRTP_AUDIO_C__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include "gstmrtpdefs.h"
#include "gobjmrtpsrc.h"

// #ifndef NMZ_STANDLONE_MODE
// #include <gst/nf/gstnfbuddybuffer.h>
// #else
// #include <gstnfbuddybuffer.h>
// #endif
#include <gobjbuddybuffer.h>
#include "nf_codec_header.h"	// FIXME


#define NMZ_TEMP_CODE (0)



/* copy from CCITT G.711 specifications */
static guchar _a2u[128] = {			/* A- to u-law conversions */
	1,	3,	5,	7,	9,	11,	13,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	32,	33,	33,	34,	34,	35,	35,
	36,	37,	38,	39,	40,	41,	42,	43,
	44,	45,	46,	47,	48,	48,	49,	49,
	50,	51,	52,	53,	54,	55,	56,	57,
	58,	59,	60,	61,	62,	63,	64,	64,
	65,	66,	67,	68,	69,	70,	71,	72,
	73,	74,	75,	76,	77,	78,	79,	79,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	97,	98,	99,	100,101,102,103,
	104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,
	120,121,122,123,124,125,126,127};

static const gchar *_endline_str = "\r\n\r\n";
static MRTPSRC_AUDIO_RING_T aring[MRTPSRC_MAX_CH];

static gint _mrtpsrc_parse_alen(MRTPSRC_STREAM_T*);
static gint _mrtpsrc_assemble_aframe(MRTPSRC_STREAM_T*, MRTPSRC_FRAME_T, gint);
static gint _mrtpsrc_pad_aliveq(MRTPSRC_STREAM_T*, MRTPSRC_FRAME_T, MRTPSRC_RTP_HEADER_T*);
static gint _mrtpsrc_calculate_ats(MRTPSRC_STREAM_T*, ICODEC_HEADER*, guint);
static gint _mrtpsrc_audio_push_ring(gint ch, guchar* data, gint len);
static gint _mrtpsrc_audio_handoff_data(gint ch);
#if MRTPSRC_AUDIO_RING_PRINT
static void _mrtpsrc_print_audio_ring(gint ch);
#endif
static unsigned char _alaw2ulaw(unsigned char aval);
static void _mrtpsrc_send_rtcp(MRTPSRC_STREAM_T *lst);

/* Safe payload length calculator for interleaved RTSP audio */
static gint calc_payload_size_audio(guint16 rtsp_len, guchar* payload, guchar* r_ptr)
{
	gint payload_off = (gint)(payload - r_ptr);
	gint hdr = (gint)sizeof(MRTPSRC_RTSP_IF_T);
	if (payload_off < hdr)
	{
		return -1;
	}
	return (gint)rtsp_len - (payload_off - hdr);
}


#ifndef NMZ_STANDLONE_MODE
extern GobjMrtpSrc *gst_mrtp_src_get_object(void);
#endif
extern void *memmem(const void*,size_t,const void*,size_t);

extern MRTPSRC_AUDIO_RING_T *mrtpsrc_get_audio_ring(void)
{
	return aring;
}

extern gint mrtpsrc_parse_audio(MRTPSRC_STREAM_T* lst)
{
	gint total_consumed_len = 0;
	gint consume_len = 0;

	guchar* scan_ptr = NULL;
	guchar* align_cp = NULL;

	MRTPSRC_RTSP_IF_T* rtsp = NULL;
	guint16 rtsp_len = 0;

	MRTPSRC_CHANNEL_T *lch = NULL;


	lch = mrtpsrc_get_channel(lst->ch_num);

	scan_ptr = lst->rbuf.cp;
	//while (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) < lst->rbuf.lp - MRTPSRC_RING_GAP)
	while (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) < lst->rbuf.lp)
	{
		gint rtsp_msg_len = 0;
		rtsp = (MRTPSRC_RTSP_IF_T*) scan_ptr;
		if (rtsp->magic != 0x24)
		{
			MRTPSRC_CHANNEL_T *lch = NULL;
			guchar *next_rtsp = NULL;

			if ((lst->rbuf.lp - scan_ptr) > lst->buf_sz) 
			{
				MRTPSRC_DBG(ERROR, "%s %d| ch_s(%d, %s) - Wrong rtsp msg length!! lst->buf_sz : %d, lst->rbuf.lp : %d, scan_ptr : %d, alloc len : %d",
						__FUNCTION__, __LINE__,
						lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->buf_sz, lst->rbuf.lp, scan_ptr, lst->rbuf.lp - scan_ptr);
				return RTN_FAIL;
			}
			
			rtsp_msg_len = mrtpsrc_get_rtsp_msg_len(scan_ptr, lst->rbuf.lp - scan_ptr);

			if (rtsp_msg_len == 0)
			{
				unsigned int len = lst->rbuf.lp - scan_ptr;
				int i = 0;

				for(i = 0; i < len-4; i++)
				{
					if(scan_ptr[i] == 0x0d && scan_ptr[i+1] == 0x0a)
					{
						if(scan_ptr[i+2] == 0x0d && scan_ptr[i+3] == 0x0a)
						{
							rtsp_msg_len = i+4;
						}
					}
				}
			}

			if (rtsp_msg_len == 0)
			{
				MRTPSRC_DBG(ERROR, "%s | Wrong magic number on CH(%d,%s) (%02x, %02x, %04u), RTP sync corrupted",
						__FUNCTION__,
						lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], rtsp->magic, rtsp->channel, rtsp->len);

				lch = mrtpsrc_get_channel(lst->ch_num);
				lch->state = STATE_RECONN_REQ;

#if MRTPSRC_EVENTLOG_PUT
				if (filter == NULL) { return RTN_FAIL; }
				if (filter->cmd_callback == NULL) { return RTN_FAIL; }
				filter->cmd_callback(2, lst->ch_num, 0, 103/*magic A*/, filter->cmd_user_data);
#endif
				return RTN_FAIL;
			}
			{
				gint _move_len = 0;
				gint _del_len = 0;

				next_rtsp = scan_ptr + rtsp_msg_len;
				_move_len = lst->rbuf.lp - next_rtsp;
				_del_len = next_rtsp - scan_ptr;
				if (_del_len <= 0 || _del_len > lst->buf_sz || _move_len < 0)
				{
					MRTPSRC_DBG(ERROR, "%s | Invalid buffer parameters detected. del_len:%d, move_len:%d, buf_sz:%u, rbs:%p, lp:%p",
						__FUNCTION__, _del_len, _move_len, lst->buf_sz, lst->rbuf.rbs, lst->rbuf.lp);
					return RTN_FAIL;
				}
				memmove(scan_ptr, next_rtsp, _move_len);
				if ((lst->rbuf.lp - _del_len) < lst->rbuf.rbs)
				{
					MRTPSRC_DBG(ERROR, "%s | Buffer underflow detected! lp:%p - %d would go below rbs:%p",
						__FUNCTION__, lst->rbuf.lp, _del_len, lst->rbuf.rbs);
					lst->rbuf.lp = lst->rbuf.rbs;
					lst->rbuf.buf_used = 0;
					lst->rbuf.buf_remain = lst->buf_sz;
					return RTN_FAIL;
				}
				lst->rbuf.lp -= _del_len;
				lst->rbuf.buf_used -= _del_len;
				lst->rbuf.buf_remain += _del_len;
				continue;
			}
#if 0
			guchar *next_rtsp = NULL;
			gint rtsp_dummy_len = 0;

			if (lst->rbuf.buf_used < 1024) { return RTN_FAIL; }
			if ((next_rtsp = (guchar*)memmem((gchar*)scan_ptr, 1024, _endline_str, 4)) != NULL)
			{
				next_rtsp += 4;
				rtsp = (MRTPSRC_RTSP_IF_T*) next_rtsp;
				rtsp_dummy_len = next_rtsp - scan_ptr;
				if (next_rtsp >= lst->rbuf.lp)
				{
					lst->rbuf.cp += rtsp_dummy_len;
					lst->rbuf.buf_used -= rtsp_dummy_len;
					lst->rbuf.buf_remain += rtsp_dummy_len;
					scan_ptr = lst->rbuf.cp;
					return rtsp_dummy_len;
				}
				if (rtsp->magic == 0x24)
				{
					lst->rbuf.cp += rtsp_dummy_len;
					lst->rbuf.buf_used -= rtsp_dummy_len;
					lst->rbuf.buf_remain += rtsp_dummy_len;
					scan_ptr = lst->rbuf.cp;
					return rtsp_dummy_len;
				}
			}
			MRTPSRC_DBG(ERROR, "mrtpsrc_parse_audio | Wrong magic number(%02x, %02x, %04u)",
					rtsp->magic, rtsp->channel, rtsp->len);

			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
#endif
		}

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		//if (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp - MRTPSRC_RING_GAP)
		if (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp)
		{
			break;
		}

		if ((rtsp->channel%2) == 0)
		{
			/* Audio packet */
			consume_len = _mrtpsrc_parse_alen(lst);
			total_consumed_len += consume_len;
			lst->rbuf.cp += consume_len;
			lst->rbuf.buf_remain += consume_len;
			lst->rbuf.buf_used -= consume_len;

			if (consume_len == RTN_FAIL)
			{
				MRTPSRC_DBG(WARN, "mrtpsrc_parse_video | Frame build failed (Single-NAL)");
				return RTN_FAIL;
			}

			scan_ptr = lst->rbuf.cp;

			continue;
		}

		scan_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
	}

	if ((lst->rbuf.lp - lst->rbuf.rbs) > (lst->buf_sz / 2))
	{
		if (lst->rbuf.buf_used)
		{
			memmove(lst->rbuf.rbs, lst->rbuf.cp, lst->rbuf.buf_used);
		}

		lst->rbuf.cp = lst->rbuf.rbs;
		//lst->rbuf.lp = lst->rbuf.rbs + lst->rbuf.buf_used + MRTPSRC_RING_GAP;
		lst->rbuf.lp = lst->rbuf.rbs + lst->rbuf.buf_used;
	}

	return total_consumed_len;
}

static gint _mrtpsrc_parse_alen(MRTPSRC_STREAM_T* lst)
{
	MRTPSRC_RTSP_IF_T* rtsp = NULL;
	MRTPSRC_RTP_HEADER_T* rtp = NULL;
	MRTPSRC_RTP_HEADER_X_T* rtpx = NULL;

	guchar* r_ptr = NULL;
	guchar* payload = NULL;
	guchar* align_cp = NULL;

	gint clen = 0;
	gint consumed_bytes = 0;
	gint audio_len = 0;
	gint pad_ret = 0; // 큐 등록 결과 확인용 변수

	guint16 hx_len = 0;
	guint16 rtsp_len = 0;

	MRTPSRC_FRAME_T new_frame = NULL;
	MRTPSRC_CHANNEL_T *lch = NULL;


	lch = mrtpsrc_get_channel(lst->ch_num);

	/* 1. Get audio length */
	r_ptr = lst->rbuf.cp;

	while (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) < lst->rbuf.lp)
	{
		rtsp = (MRTPSRC_RTSP_IF_T*) r_ptr;

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		//if (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp - MRTPSRC_RING_GAP)
		if (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp)
		{
			break;
		}

		if (rtsp->magic != 0x24)
		{
			MRTPSRC_DBG(ERROR, "%s | Wrong magic A number(%02x, %02x, %04u)",
					__FUNCTION__, rtsp->magic, rtsp->channel, rtsp_len);

			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
		}

		if ((rtsp->channel%2) != 0)
		{
			MRTPSRC_RTCP_HEADER_T *rtcp = (MRTPSRC_RTCP_HEADER_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T));
			lst->itlv_rtcp_ch = rtsp->channel;
			if (rtcp->pt != 200) // if no sender report, skip.
			{
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				continue;
			}
			lst->ts_last_sr = ((ntohl(rtcp->ts_msw)&0xffff)<<16)|((ntohl(rtcp->ts_lsw)&0xffff0000)>>16);
			_mrtpsrc_send_rtcp(lst);
			r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;
		}

		/* Audio packet */
		rtp = (MRTPSRC_RTP_HEADER_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T));
		lst->itlv_rtp_ch = rtsp->channel;
		lst->itlv_rtcp_ch = rtsp->channel+1;

		/* for the rtcp - SSRC mismatch check */
		if((lst->sr_ssrc != ntohl(rtp->ssrc) && lst->sr_ssrc != 0))
		{
			MRTPSRC_DBG(WARN, "%s %d | ch_s(%d, AUDIO) - rtp ssrc is mismatched.(now:%0X, ori:%0X) - Request reconnection\n",
					__FUNCTION__, __LINE__, lst->ch_num, ntohl(rtp->ssrc), lst->sr_ssrc);
			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
		}

		/* update ssrc/seq */
		{
			lst->sr_ssrc = ntohl(rtp->ssrc);
			lst->seq_last = ntohs(rtp->seq);
			if (lst->seq_last == 0) lst->seq_group++;
		}

		payload = r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T);
#if !NMZ_TEMP_CODE
		if (rtp->x)
		{
			rtpx = (MRTPSRC_RTP_HEADER_X_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T));
			align_cp = (guchar*) &rtpx->xlen;
			hx_len = *(align_cp + 1) | *align_cp << 8;	// ntohs(hx_len)

			/* Skip header extension */
			payload = payload + sizeof(MRTPSRC_RTP_HEADER_X_T) + (hx_len * 4);
		}

		// [Safety] Integer Underflow 방지
		if (rtsp_len < sizeof(MRTPSRC_RTP_HEADER_T))
		{
			MRTPSRC_DBG(ERROR, "%s | RTSP len too small for RTP header", __FUNCTION__);
			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
		}
		//audio_len = (rtsp_len - (payload - r_ptr + sizeof(MRTPSRC_RTSP_IF_T)));
		audio_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);
#else
		if (rtsp_len < sizeof(MRTPSRC_RTP_HEADER_T) + 24)
		{
			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
		}
		audio_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T) - 24;
#endif

		// 1. 메모리 할당 (Caller Allocation)
		new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
		if (new_frame == NULL)
		{
			MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
					__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[STREAM_AUDIO]);
			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
		}
		memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
		new_frame->len = audio_len;
		new_frame->frame_type = FTYPE_AUDIO;

		// 2. 데이터 조립 (Assembly) - 큐에 넣지 않고 데이터만 채움
		clen = _mrtpsrc_assemble_aframe(lst, new_frame, consumed_bytes);
		
		if (clen == RTN_FAIL)
		{
			// 실패 시 Caller가 메모리 해제
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			new_frame = NULL;
			break;
		}

		// 3. 큐 등록 (Consumer 전달)
		pad_ret = _mrtpsrc_pad_aliveq(lst, new_frame, rtp);

		if (pad_ret != RTN_OK)
		{
			// 큐 등록 실패 시 (Timestamp 중복 등) Caller가 정리
			// assemble 과정에서 할당된 frame(GObject/Buffer)이 있다면 해제
			if (new_frame->frame)
			{
				mrtpsrc_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				new_frame->frame = NULL;
			}
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			new_frame = NULL;
			
			// 실패했지만, 패킷 처리는 된 것으로 간주하고(Skip) 다음 패킷 진행을 위해 clen만큼 증가
			// 만약 심각한 에러라면 return RTN_FAIL을 해야 하지만, TS 중복 등은 Skip이 일반적임.
		}

		consumed_bytes += clen;
		r_ptr = lst->rbuf.cp + consumed_bytes;
	}

	return consumed_bytes;
}

static gint _mrtpsrc_assemble_aframe(MRTPSRC_STREAM_T* lst, MRTPSRC_FRAME_T new_frame, gint offset)
{
	MRTPSRC_RTSP_IF_T* rtsp = NULL;
	MRTPSRC_RTP_HEADER_T* rtp = NULL;
	MRTPSRC_RTP_HEADER_X_T* rtpx = NULL;

	guchar* r_ptr = NULL;
	guchar* wa_ptr = NULL;
	guchar* payload = NULL;
	guchar* align_cp = NULL;

	gint consumed_bytes = 0;

	guint16 hx_len = 0;
	guint16 rtsp_len = 0;


	ICODEC_HEADER *icodec_h = NULL;

	MRTPSRC_CHANNEL_T *lch = NULL;

	gint i = 0;

	lch = mrtpsrc_get_channel(lst->ch_num);

	/* Error if Program counter reached here without new_frame allocated */
	if (new_frame == NULL)
	{
		MRTPSRC_DBG(ERROR, "%s | Frame not allocated", __FUNCTION__);
		lch->state = STATE_RECONN_REQ;
		return RTN_FAIL;
	}

	/* Error if new_frame->vlen is 0 at here */
	if (new_frame->len == 0)
	{
		MRTPSRC_DBG(ERROR, "%s | Video frame length 0", __FUNCTION__);
		lch->state = STATE_RECONN_REQ;
		return RTN_FAIL;
	}

	/* size calculation and 64-bytes align */
	{
		int size_frame = new_frame->len + sizeof(ICODEC_HEADER);
		int remain = size_frame % 64;
		if (remain != 0)
		{
			size_frame += (64 - remain);
		}

		new_frame->frame = mrtpsrc_alloc_cmem(
							size_frame, &lst->ch_num,
							__FILE__, __FUNCTION__, __LINE__);

		if (new_frame->frame == NULL)
		{
			MRTPSRC_DBG(WARN, "%s | cmem fail to alloc. Reconnect(%d, %s)\n",
					__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[STREAM_AUDIO]);
			lch->state = STATE_REBOOT_REQ;
			
			// [수정] new_frame 자체는 해제하지 않음 (Caller 책임)
			return RTN_FAIL;
		}

		if(new_frame->frame) {
			memset(gobj_buddy_buffer_buf_get_addr(new_frame->frame), 0x00, size_frame);
		}
	}


	wa_ptr = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
	icodec_h = (ICODEC_HEADER*) wa_ptr;
	wa_ptr += sizeof(ICODEC_HEADER);

	new_frame->icodec_h.chan = lst->ch_num;
	new_frame->icodec_h.codec = NF_CODEC_TYPE_URAW;
	new_frame->icodec_h.flags = 1;
	new_frame->icodec_h.version = 1;
	new_frame->icodec_h.frame_size = new_frame->len;
	new_frame->icodec_h.frame_type= new_frame->frame_type;
	new_frame->icodec_h.resolution = 0;		// FIXME
	new_frame->icodec_h.frame_rate = NF_FPS_CR01;
	new_frame->icodec_h.gst_buffer = new_frame->frame;

	/* Build audio frame */
	r_ptr = lst->rbuf.cp + offset;
	while(MRTPSRC_ALWAYS)
	{
		rtsp = (MRTPSRC_RTSP_IF_T*) r_ptr;

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		if (rtsp->magic != 0x24)
		{
			MRTPSRC_DBG(ERROR, "%s | Wrong magic B number(%02x, %02x, %04u)",
					__FUNCTION__, rtsp->magic, rtsp->channel, rtsp->len);
			lch->state = STATE_RECONN_REQ;
			
			mrtpsrc_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
			new_frame->frame = NULL;
			return RTN_FAIL;
		}

		/* RTCP packet - Unimplemented */
		if ((rtsp->channel%2) != 0)
		{
			r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;
		}

		/* Audio packet */
		rtp = (MRTPSRC_RTP_HEADER_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T));

		if (rtp->x)
		{
			rtpx = (MRTPSRC_RTP_HEADER_X_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T));
		}

		payload = r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T);

		{
			gint payload_size = calc_payload_size_audio(rtsp_len, payload, r_ptr);
			if (payload_size <= 0)
			{
				MRTPSRC_DBG(WARN, "%s | ch(%d,AUDIO) invalid payload_size(%d) seq(%u)", __FUNCTION__,
						lst->ch_num, payload_size, ntohs(rtp->seq));
				return RTN_FAIL;
			}
			if (lst->resolution == 100)
			{
				for (i=0; i<payload_size; i++)
				{
					*(payload+i) = _alaw2ulaw(*(payload+i));
				}
			}

#if !NMZ_TEMP_CODE
		if (rtp->x)
		{
			align_cp = (guchar*) &rtpx->xlen;
			hx_len = *(align_cp + 1) | *align_cp << 8;	// ntohs(hx_len)

			/* Skip header extension */
			payload = payload + sizeof(MRTPSRC_RTP_HEADER_X_T) + (hx_len * 4);
			payload_size = calc_payload_size_audio(rtsp_len, payload, r_ptr);
			if (payload_size <= 0)
			{
				MRTPSRC_DBG(WARN, "%s | ch(%d,AUDIO) invalid payload_size(%d) after xlen seq(%u)", __FUNCTION__,
						lst->ch_num, payload_size, ntohs(rtp->seq));
				return RTN_FAIL;
			}
		}

		
		// [Safety] 버퍼 오버플로우 체크 (Critical)
		if(wa_ptr + new_frame->len < wa_ptr + payload_size)
		{
			fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
			return RTN_FAIL;
		}
		memcpy(wa_ptr, payload, payload_size);
#else
		// [Safety] Legacy 코드 부분도 체크
		unsigned int copy_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T) - 24;
		if(wa_ptr + new_frame->len < wa_ptr + copy_len)
		{
			fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
			mrtpsrc_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
			new_frame->frame = NULL;
			return RTN_FAIL;
		}
		memcpy(wa_ptr, payload, copy_len);
#endif

		} /* end payload_size guarded block */

		r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
		consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

		memcpy(icodec_h, &new_frame->icodec_h, sizeof(ICODEC_HEADER));

		if (lst->state == STATE_STREAM_READY)
		{
			lst->state = STATE_PLAYING;
		}
		
		// 소비된 바이트 수 리턴 (성공)
		return consumed_bytes;
	}
}

#define AUDIO_TS_OVERLAP (401)
#define AUDIO_TS_JUMP (407)
#define AUDIO_TS_OK (200)
static gint _mrtpsrc_pad_aliveq(MRTPSRC_STREAM_T* lst, MRTPSRC_FRAME_T new_frame, MRTPSRC_RTP_HEADER_T* rtp)
{
	guchar* align_cp = NULL;
	guint frame_timestamp = 0;
	gint rtn = 0;
	MRTPSRC_STREAM_STAT_T *stats = NULL;


#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif

	new_frame->frame_seq = 0;
	align_cp = (guchar*) &rtp->timestamp;
	frame_timestamp = *(align_cp + 3) | *(align_cp + 2) << 8 | *(align_cp + 1) << 16 | *align_cp << 24;

	rtn = _mrtpsrc_calculate_ats(lst, &new_frame->icodec_h, frame_timestamp);
	if (rtn == AUDIO_TS_OVERLAP)
	{
		return RTN_FAIL;
	}

	_mrtpsrc_audio_push_ring(lst->ch_num, (guchar*)(gobj_buddy_buffer_buf_get_addr(new_frame->frame) + sizeof(ICODEC_HEADER)), new_frame->len);
	_mrtpsrc_audio_handoff_data(lst->ch_num);
#if MRTPSRC_AUDIO_RING_PRINT
	_mrtpsrc_print_audio_ring(lst->ch_num);
#endif

/* FIXME. Audio decoding? */
	mrtpsrc_frame_q_lock();
	if (lst->frame_head == NULL)
	{
		lst->frame_head = new_frame;
		lst->frame_tail = new_frame;

		mrtpsrc_frame_q_unlock();
  #if MRTPSRC_USE_STATISTICS
		stats->audio_cnt++;
  #endif
		return RTN_OK;
	}

	lst->frame_tail->next = new_frame;
	lst->frame_tail = new_frame;
	mrtpsrc_frame_q_unlock();

  #if MRTPSRC_USE_STATISTICS
	stats->audio_cnt++;
  #endif

	return RTN_OK;
}

static gint _mrtpsrc_calculate_ats(MRTPSRC_STREAM_T* lst, ICODEC_HEADER* icodec_h, guint frame_timestamp)
{
	//int diff_sec = 0;
	//int diff_5msec = 0;

	GTimeVal now_time = mrtpsrc_get_captured_time();
	MRTPSRC_CHANNEL_T *lch = NULL;
	MRTPSRC_STREAM_STAT_T *stats = NULL;

#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
	lch = mrtpsrc_get_channel(lst->ch_num);

#if 0		// AAAAAAAAAAAAAAAAAAA
	/* Case 0: System time changed */
	{
		diff_sec = now_time.tv_sec - lst->ts_last_second;
		if (lst->ts_last_rcvd != 0 && (diff_sec < (-10) || diff_sec > 10))
		{
#if MRTPSRC_USE_STATISTICS
			stats->ts_sys_cnt++;
			stats->cbc_sys_time_chg[lst->ch_num]++;
#endif
			MRTPSRC_DBG(WARN, "%s | System time may be changed(%ld, %s)\n"
					  "    ==> Last timestamp(%lu) Frame timestamp(%lu)\n"
					  "    ==> Last frame(%lu.%lu) Now(%lu.%lu)\n",
					__FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num],
					lst->ts_last_rcvd, frame_timestamp,
					lst->ts_last_second, lst->ts_last_5_mili,
					now_time.tv_sec, now_time.tv_nsec / 1000 / 5000);
			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
		}
	}
#endif		// AAAAAAAAAAAAAAAAA


	if (lst->ts_last_rcvd == 0)
	{
		icodec_h->timestamp = now_time.tv_sec;
		icodec_h->timestampl = now_time.tv_usec / 5000;
		lst->ts_last_second = icodec_h->timestamp;
		lst->ts_last_5_mili = icodec_h->timestampl;
		lst->ts_first_second = lst->ts_last_second;
		lst->ts_first_5_mili = lst->ts_last_5_mili;
		aring[lst->ch_num].timestamp = lst->ts_first_second;
		aring[lst->ch_num].timestampl = lst->ts_first_5_mili;
		lst->ts_last_rcvd = frame_timestamp;
		return AUDIO_TS_OK;
	}

	{
		int diff_sec = 0;
		int diff_5m = 0;
		int delta = 0;
		
		diff_sec = lst->ts_last_second - now_time.tv_sec;
		diff_5m = lst->ts_last_5_mili - (now_time.tv_usec/5000);

		delta = diff_sec * 1000 + diff_5m*5;
		if (delta > 2000)
		{
			MRTPSRC_DBG(WARN, "%s | ERR audio timestamp reset(%d)", __FUNCTION__, lst->ch_num);
			icodec_h->timestamp = now_time.tv_sec;
			icodec_h->timestampl = now_time.tv_usec / 5000;
			lst->ts_last_second = icodec_h->timestamp;
			lst->ts_last_5_mili = icodec_h->timestampl;
			//lst->ts_first_second = lst->ts_last_second;
			//lst->ts_first_5_mili = lst->ts_last_5_mili;
			aring[lst->ch_num].timestamp = now_time.tv_sec;
			aring[lst->ch_num].timestampl = now_time.tv_usec/5000;
			aring[lst->ch_num].lp = 0;
			aring[lst->ch_num].cp = 0;
			aring[lst->ch_num].len = 0;
			lst->ts_last_rcvd = frame_timestamp;
			return AUDIO_TS_OVERLAP;
		}
		//else if (delta > 500)
		else if (delta > 200)
		{
// ksi_test
// #if MRTPSRC_FULL_DBG
// 			MRTPSRC_DBG(IMPACT, "%s | drop audio frame(%d)", __FUNCTION__, lst->ch_num);
// #endif
			lst->ts_last_rcvd = frame_timestamp;
			return AUDIO_TS_OVERLAP;
		}
		else if (delta < (-300))
		{
// #if MRTPSRC_FULL_DBG
// 			MRTPSRC_DBG(IMPACT, "%s | jump audio frame(%d)", __FUNCTION__, lst->ch_num);
// #endif
			icodec_h->timestamp = now_time.tv_sec;
			icodec_h->timestampl = now_time.tv_usec / 5000;
			lst->ts_last_second = icodec_h->timestamp;
			lst->ts_last_5_mili = icodec_h->timestampl;
			lst->ts_first_second = lst->ts_last_second;
			lst->ts_first_5_mili = lst->ts_last_5_mili;
			aring[lst->ch_num].timestamp = lst->ts_first_second;
			aring[lst->ch_num].timestampl = lst->ts_first_5_mili;
			{
				int _s1 = aring[lst->ch_num].len / 8000;
				int _m5 = (aring[lst->ch_num].len % 8000)/40;

				if (aring[lst->ch_num].timestampl < _m5)
				{
					aring[lst->ch_num].timestamp -= (_s1 + 1);
					aring[lst->ch_num].timestampl += (200 - _m5);
				}
				else
				{
					aring[lst->ch_num].timestamp -= _s1;
					aring[lst->ch_num].timestampl -= _m5;
				}
			}
			lst->ts_last_rcvd = frame_timestamp;
			return AUDIO_TS_JUMP;
		}
	}

	{
		/* 
		 * sec_dur:200 = ts_diff:X
		 * X = (200 x ts_diff) / sec_dur
		 */
		//guint ts_div = lst->sec_dur / 200;
		guint ts_div = 8000 / 200;
		guint ts_diff = frame_timestamp - lst->ts_last_rcvd;
		guint ts_quotient = ts_diff / ts_div;
		guint ts_remainder = ts_diff % ts_div;


		if (lst->ts_last_remainder + ts_remainder >= ts_div)
		{
			ts_quotient++;
			lst->ts_last_remainder = lst->ts_last_remainder + ts_remainder - ts_div;
		}
		else
		{
			lst->ts_last_remainder += ts_remainder;
		}

		lst->ts_last_5_mili += ts_quotient;
		if (lst->ts_last_5_mili >= 200 && lst->ts_last_5_mili < 400)
		{
			lst->ts_last_5_mili -= 200;
			lst->ts_last_second += 1;
		}

		icodec_h->timestamp = lst->ts_last_second;
		icodec_h->timestampl = lst->ts_last_5_mili;
		lst->ts_first_second = lst->ts_last_second;
		lst->ts_first_5_mili = lst->ts_last_5_mili;
		lst->ts_last_rcvd = frame_timestamp;
	}

	return RTN_OK;
}

static void _mrtpsrc_send_rtcp(MRTPSRC_STREAM_T *lst)
{
	guchar buf[1024];
	guchar *p = NULL;
	guint16 *p2 = NULL;
	guint *p4 = NULL;
	gint len = 0;

	memset(buf, 0x00, 1024);

	p = &buf[0];
	*p++ = 0x24;                    // magic
	*p++ = lst->itlv_rtcp_ch;       // channel
	p2 = (guint16*) p;
	*p2++ = htons(52);              // length bytes

	p = (guchar*) p2;
	*p++ = 0x81;                    // version, padding, report count
	*p++ = 0xc9;                    // packet type: receiver report(201)
	p2 = (guint16*) p;
	*p2++ = htons(7);               // length: 7 (32 bytes)
	p4 = (guint*) p2;
	*p4++ = htonl(lst->sr_ssrc);    // sender SSRC
	*p4++ = htonl(lst->sr_ssrc);    // identifier
	p = (guchar*) p4;
	*p++ = 0;                       // fraction
	*p++ = 0;                       // packets lost(1)
	*p++ = 0;                       // packets lost(2)
	*p++ = 0;                       // packets lost(3)
	p2 = (guint16*)p;
	*p2++ = htons(lst->seq_group);  // sequence cycles count
	*p2++ = htons(lst->seq_last);   // highest sequence number received
	p4 = (guint*)p2;
	*p4++ = htonl(30);              // interarrival jitter
	*p4++ = htonl(lst->ts_last_sr); // last SR timestamp
	*p4++ = htonl(3613);            // since last SR timestamp: 3613 (55ms)

	p = (guchar*) p4;
	*p++ = 0x81;                    // version, padding, source count
	*p++ = 0xca;                    // packet type: source description(202)
	p2 = (guint16*) p;
	*p2++ = htons(4);               // length: 4 (20 bytes)
	p4 = (guint*) p2;
	*p4++ = htonl(lst->sr_ssrc);    // identifier
	p = (guchar*) p4;
	*p++ = 0x1;                     // CNAME
	*p++ = 0x6;                     // length
	*p++ = 'a';
	*p++ = '8';
	*p++ = '1';
	*p++ = '6';
	*p++ = 'x';
	*p++ = 'e';
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;

#if 0
	{
		int i=0;
		printf("RTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCPRTCP\n");
		printf("     0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f");
		while (i < (p-&buf[0]))
		{
			if (i % 16 == 0)
			{
				printf("\n%02x  ", i/16);
			}
			else if (i % 8 == 0)
			{
				printf(" ");
			}
			printf("%02x ", buf[i++]);
		}
		printf("\n\n\n");
	}
#endif

	//struct sockaddr_in sin;

	switch(lst->rtp_protocol)
	{
		case RTP_UNICAST_UDP:
			len = send(lst->udp_sock[1], buf, p - &buf[0], MSG_DONTWAIT);
			break;
		case RTP_MULTICAST_UDP:
			break;
		case RTP_OVER_RTSP_TCP:
			len = send(lst->rtsp_sock, buf, p - &buf[0], 0);
			break;
		default:
			break;

	}
}
#if MRTPSRC_AUDIO_RING_PRINT
static void _mrtpsrc_print_audio_ring(gint ch)
{
	if (aring[ch].lp == aring[ch].cp)
	{
		printf("                  lp,cp\n");
		printf("+---------+---------+---------+---------+---------+\n");
		printf("|      % 6ld       |            % 6ld           |\n",
				aring[ch].lp, MRTPSRC_AUDIO_RING_SZ - aring[ch].lp);
		printf("+---------+---------+---------+---------+---------+\n");
	}
	else if (aring[ch].lp > aring[ch].cp)
	{
		printf("                 cp              lp\n");
		printf("+---------+---------+---------+---------+---------+\n");
		printf("|    % 6ld      |    % 6ld     |    % 6ld      |\n",
				aring[ch].cp, aring[ch].lp - aring[ch].cp, MRTPSRC_AUDIO_RING_SZ - aring[ch].lp);
		printf("+---------+---------+---------+---------+---------+\n");
	}
	else
	{
		printf("                 lp              cp\n");
		printf("+---------+---------+---------+---------+---------+\n");
		printf("|    % 6ld      |    % 6ld     |    % 6ld      |\n",
				aring[ch].lp, aring[ch].cp - aring[ch].lp, MRTPSRC_AUDIO_RING_SZ - aring[ch].cp);
		printf("+---------+---------+---------+---------+---------+\n");
	}
}
#endif
void gst_buffer_debug(GobjBuddyBuffer *buf)
{
	if(!GOBJ_IS_BUDDY_BUFFER(buf)) {
		unsigned char *dump = (unsigned char *)buf;
		puts("[minto] is not mini object\n");
		mrtpsrc_print_cmem_range();
		printf("GobjBuddyBuffer point(%p)\n",buf); 
		puts("[minto] dump\n");
		printf(" 00 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
				dump[0], dump[1], dump[2], dump[3], dump[4], dump[5], dump[6], dump[7]);
		printf(" 01 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
				dump[8], dump[9], dump[10], dump[11], dump[12], dump[13], dump[14], dump[15]);
		printf(" 02 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
				dump[16], dump[17], dump[18], dump[19], dump[20], dump[21], dump[22], dump[23]);
		printf(" 03 | %02X %02X %02X %02X %02X %02X %02X %02X\n",
				dump[24], dump[25], dump[26], dump[27], dump[28], dump[29], dump[30], dump[31]);
	}
}

static gint _mrtpsrc_audio_push_ring(gint ch, guchar* data, gint len)
{
	int load_remain;
	int remain;

	/* Case0: lp == cp */
	if (aring[ch].lp == aring[ch].cp)
	{
		memcpy(&aring[ch].data[0], data, len);
		aring[ch].lp = len;
		aring[ch].cp = 0;
		aring[ch].len = len;
		return len;
	}
	/* Case1: lp > cp */
	else if (aring[ch].lp > aring[ch].cp)
	{
		load_remain = MRTPSRC_AUDIO_RING_SZ - aring[ch].lp;
		remain = load_remain + aring[ch].cp;

		if (load_remain >= len)
		{
			memcpy(&aring[ch].data[aring[ch].lp], data, len);
			aring[ch].lp += len;
			aring[ch].len += len;
		}
		else if (remain >= len)
		{
			memcpy(&aring[ch].data[aring[ch].lp], data, load_remain);
			memcpy(&aring[ch].data[0], data+load_remain, len - load_remain);
			aring[ch].lp = len - load_remain;
			aring[ch].len += len;
		}
		else
		{
			MRTPSRC_DBG(ERROR, "%s | Audio handoff ring buffer full(CHANNEL:%d)\n",
					__FUNCTION__, ch);
			return (-1);
		}
	}
	/* Case2: lp < cp */
	else
	{
		remain = aring[ch].cp - aring[ch].lp;

		if (remain >= len)
		{
			memcpy(&aring[ch].data[aring[ch].lp], data, len);
			aring[ch].lp += len;
			aring[ch].len += len;
		}
		else
		{
			MRTPSRC_DBG(ERROR, "%s | Audio handoff ring buffer full(CHANNEL:%d)\n",
					__FUNCTION__, ch);
			return (-1);
		}
	}

	return (aring[ch].len);
}

static gint _mrtpsrc_audio_handoff_data(gint ch)
{
	//GobjMrtpSrc *src = gst_mrtp_src_get_object();
	GobjBuddyBuffer *aframe = NULL;
	ICODEC_HEADER *icodec_h = NULL;
	int consume_remain;
	int remain;
	MRTPSRC_STREAM_STAT_T *stats = NULL;
	GTimeVal now_time = mrtpsrc_get_captured_time();

	/* Case0: lp == cp */
	if (aring[ch].lp == aring[ch].cp)
	{
		return (aring[ch].len);
	}

#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif

	/* Case1: lp > cp */
	if (aring[ch].lp > aring[ch].cp)
	{
		remain = aring[ch].lp - aring[ch].cp;

		if (remain < MRTPSRC_AUDIO_HO_SZ)
			return (aring[ch].len);

		aframe = mrtpsrc_alloc_cmem(
				sizeof(ICODEC_HEADER) + MRTPSRC_AUDIO_HO_SZ, &ch,
				__FILE__, __FUNCTION__, __LINE__);
		if (aframe == NULL)
		{
			return aring[ch].len;
		}

		memset(gobj_buddy_buffer_buf_get_addr(aframe), 0x00, sizeof(ICODEC_HEADER) + MRTPSRC_AUDIO_HO_SZ);

		icodec_h = (ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(aframe);
		icodec_h->chan = ch;
		icodec_h->codec = NF_CODEC_TYPE_URAW;
		icodec_h->flags = 1;
		icodec_h->version = 1;
		icodec_h->frame_size = MRTPSRC_AUDIO_HO_SZ;
		icodec_h->frame_type = NF_FRAME_TYPE_AUDIO;
		icodec_h->resolution = 0;
		icodec_h->frame_rate = NF_FPS_CR01;
		icodec_h->gst_buffer = aframe;
		icodec_h->timestamp = aring[ch].timestamp;
		icodec_h->timestampl = aring[ch].timestampl;

		unsigned int payload_size = MRTPSRC_AUDIO_HO_SZ;
		guchar* wa_ptr = gobj_buddy_buffer_buf_get_addr(aframe);
		if(wa_ptr + (sizeof(ICODEC_HEADER) + MRTPSRC_AUDIO_HO_SZ) < (wa_ptr +  sizeof(ICODEC_HEADER)) + payload_size)
		{
			fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
			mrtpsrc_free_cmem(__FILE__, __FUNCTION__, __LINE__, aframe);
			return RTN_FAIL;
		}
		memcpy((gobj_buddy_buffer_buf_get_addr(aframe) + sizeof(ICODEC_HEADER)), &aring[ch].data[aring[ch].cp], MRTPSRC_AUDIO_HO_SZ);
		aring[ch].cp += MRTPSRC_AUDIO_HO_SZ;
	}
	/* Case2: lp < cp */
	else
	{
		consume_remain = MRTPSRC_AUDIO_RING_SZ - aring[ch].cp;
		remain = consume_remain + aring[ch].lp;

		if (remain < MRTPSRC_AUDIO_HO_SZ)
			return (aring[ch].len);

		aframe = mrtpsrc_alloc_cmem(
							sizeof(ICODEC_HEADER) + MRTPSRC_AUDIO_HO_SZ, &ch,
							__FILE__, __FUNCTION__, __LINE__);
		if (aframe == NULL)
		{
			return aring[ch].len;
		}

		memset(gobj_buddy_buffer_buf_get_addr(aframe), 0x00, sizeof(ICODEC_HEADER) + MRTPSRC_AUDIO_HO_SZ);

		icodec_h = (ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(aframe);
		icodec_h->chan = ch;
		icodec_h->codec = NF_CODEC_TYPE_URAW;
		icodec_h->flags = 1;
		icodec_h->version = 1;
		icodec_h->frame_size = MRTPSRC_AUDIO_HO_SZ;
		icodec_h->frame_type = NF_FRAME_TYPE_AUDIO;
		icodec_h->resolution = 0;
		icodec_h->frame_rate = NF_FPS_CR01;
		icodec_h->gst_buffer = aframe;
		icodec_h->timestamp = aring[ch].timestamp;
		icodec_h->timestampl = aring[ch].timestampl;

		if (consume_remain >= MRTPSRC_AUDIO_HO_SZ)
		{
			unsigned int payload_size = MRTPSRC_AUDIO_HO_SZ;
			guchar* wa_ptr = gobj_buddy_buffer_buf_get_addr(aframe);
			if(wa_ptr + (sizeof(ICODEC_HEADER) + payload_size) < (wa_ptr +  sizeof(ICODEC_HEADER)) + payload_size)
			{
				fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
				mrtpsrc_free_cmem(__FILE__, __FUNCTION__, __LINE__, aframe);
				return RTN_FAIL;
				}
			memcpy((gobj_buddy_buffer_buf_get_addr(aframe) + sizeof(ICODEC_HEADER)), &aring[ch].data[aring[ch].cp], MRTPSRC_AUDIO_HO_SZ);
			aring[ch].cp += MRTPSRC_AUDIO_HO_SZ;
		}
		else
		{
			// 1. 복사할 두 덩어리의 크기 계산
			unsigned int first_chunk_len = consume_remain;                  // 링버퍼 끝까지
			unsigned int second_chunk_len = MRTPSRC_AUDIO_HO_SZ - consume_remain; // 링버퍼 처음부터 (Wrap around)

			// 2. 쓰기 포인터 설정 (헤더 뒤부터 시작)
			guchar* wa_ptr = gobj_buddy_buffer_buf_get_addr(aframe);
			guchar* dest_payload_ptr = wa_ptr + sizeof(ICODEC_HEADER);

			// [Safety] 오버플로우 방지: 복사할 총량이 버퍼 크기를 넘는지 확인
			// (이전에 할당된 크기: sizeof(ICODEC_HEADER) + MRTPSRC_AUDIO_HO_SZ)
			// consume_remain 계산이 잘못되어 음수가 되거나 범위를 넘는 경우를 방어
			if (first_chunk_len > MRTPSRC_AUDIO_HO_SZ || second_chunk_len > MRTPSRC_AUDIO_HO_SZ)
			{
				fprintf(stderr, "- ERROR - [%s:%d] Invalid Copy Size (consume: %d, total: %d)\n", 
						__FUNCTION__, __LINE__, first_chunk_len, MRTPSRC_AUDIO_HO_SZ);
				
				mrtpsrc_free_cmem(__FILE__, __FUNCTION__, __LINE__, aframe);
				return RTN_FAIL;
			}

			// 3. 데이터 복사 (링버퍼 -> aframe)
			// 첫 번째 덩어리: 현재 위치(cp)부터 버퍼 끝까지
			memcpy(dest_payload_ptr, &aring[ch].data[aring[ch].cp], first_chunk_len);

			// 두 번째 덩어리: 버퍼 시작(0)부터 남은 만큼
			memcpy(dest_payload_ptr + first_chunk_len, &aring[ch].data[0], second_chunk_len);

			// 4. 링버퍼 포인터 업데이트
			aring[ch].cp = second_chunk_len;
		}
	}

	aring[ch].len -= MRTPSRC_AUDIO_HO_SZ;
	aring[ch].timestamp += (MRTPSRC_AUDIO_HO_SZ/8000);
	aring[ch].timestampl += ((MRTPSRC_AUDIO_HO_SZ%8000) / 40);
	if (aring[ch].timestampl >= 200)
	{
		aring[ch].timestamp++;
		aring[ch].timestampl -= 200;
	}

	{
		int diff_sec = 0;
		int diff_5m = 0;
		int delta = 0;
		
		diff_sec = aring[ch].timestamp - now_time.tv_sec;
		diff_5m = aring[ch].timestampl - (now_time.tv_usec/5000);

		delta = diff_sec * 1000 + diff_5m*5;

		if (delta > 500)
		{
			MRTPSRC_DBG(WARN, "%s | CH(%d) audio handoff data DROP(%d)\n", __FUNCTION__, ch, remain);
			aring[ch].lp = 0;
			aring[ch].cp = 0;
			aring[ch].len = 0;
		}
		else if (delta < (-1500))
		{
			MRTPSRC_DBG(WARN, "%s | CH(%d) audio handoff data JUMP\n", __FUNCTION__, ch);
			aring[ch].timestampl += 20;
			if (aring[ch].timestampl >= 200)
			{
				aring[ch].timestamp++;
				aring[ch].timestampl -= 200;
			}
		}
	}

	if(!GOBJ_IS_BUDDY_BUFFER (aframe))
	{
		printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
		gst_buffer_debug(aframe);
	}
	else
	{
		mrtpsrc_ho_enqueue(aframe);
#if MRTPSRC_USE_STATISTICS
		stats->handoff_cnt++;
#endif
	}
#if 0
	if (src != NULL && src->handoff_callback != NULL)
	{
		src->handoff_callback(aframe, src->handoff_user_data);
#if MRTPSRC_USE_STATISTICS
		stats->handoff_cnt++;
#endif
	}
#endif

	g_object_unref(aframe);

	return (aring[ch].len);
}

#if 1
/* A-law to u-law conversion */
static unsigned char _alaw2ulaw(unsigned char aval)
{
	aval &= 0xff;
	return ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
			(0x7F ^ _a2u[aval ^ 0x55]));
}
#endif



#if 0

/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * g711.c
 *
 * u-law, A-law and linear PCM conversions.
 */
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
	0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

/* copy from CCITT G.711 specifications */
unsigned char _u2a[128] = {			/* u- to A-law conversions */
	1,	1,	2,	2,	3,	3,	4,	4,
	5,	5,	6,	6,	7,	7,	8,	8,
	9,	10,	11,	12,	13,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,	24,
	25,	27,	29,	31,	33,	34,	35,	36,
	37,	38,	39,	40,	41,	42,	43,	44,
	46,	48,	49,	50,	51,	52,	53,	54,
	55,	56,	57,	58,	59,	60,	61,	62,
	64,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
	81,	82,	83,	84,	85,	86,	87,	88,
	89,	90,	91,	92,	93,	94,	95,	96,
	97,	98,	99,	100,	101,	102,	103,	104,
	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,
	121,	122,	123,	124,	125,	126,	127,	128};

unsigned char _a2u[128] = {			/* A- to u-law conversions */
	1,	3,	5,	7,	9,	11,	13,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	32,	33,	33,	34,	34,	35,	35,
	36,	37,	38,	39,	40,	41,	42,	43,
	44,	45,	46,	47,	48,	48,	49,	49,
	50,	51,	52,	53,	54,	55,	56,	57,
	58,	59,	60,	61,	62,	63,	64,	64,
	65,	66,	67,	68,	69,	70,	71,	72,
	73,	74,	75,	76,	77,	78,	79,	79,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	97,	98,	99,	100,	101,	102,	103,
	104,	105,	106,	107,	108,	109,	110,	111,
	112,	113,	114,	115,	116,	117,	118,	119,
	120,	121,	122,	123,	124,	125,	126,	127};

static int
search(
		int		val,
		short	*table,
		int		size)
{
	int		i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char
linear2alaw(
		int		pcm_val)	/* 2's complement (16-bit range) */
{
	int		mask;
	int		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 8;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		aval = seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (aval ^ mask);
	}
}

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
int
alaw2linear(
		unsigned char	a_val)
{
	int		t;
	int		seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
		case 0:
			t += 8;
			break;
		case 1:
			t += 0x108;
			break;
		default:
			t += 0x108;
			t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

#define	BIAS		(0x84)		/* Bias for linear code. */

/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char
linear2ulaw(
		int		pcm_val)	/* 2's complement (16-bit range) */
{
	int		mask;
	int		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	if (pcm_val < 0) {
		pcm_val = BIAS - pcm_val;
		mask = 0x7F;
	} else {
		pcm_val += BIAS;
		mask = 0xFF;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
		return (uval ^ mask);
	}

}

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
int
ulaw2linear(
		unsigned char	u_val)
{
	int		t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/* u-law to A-law conversion */
unsigned char
ulaw2alaw(
		unsigned char	uval)
{
	uval &= 0xff;
	return ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) :
			(0x55 ^ (_u2a[0x7F ^ uval] - 1)));
}

#endif

#endif	// _GST_MRTP_AUDIO_C__
