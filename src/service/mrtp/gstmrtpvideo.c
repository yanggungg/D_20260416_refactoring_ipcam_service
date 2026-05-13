/*
 * ITX Security Co.,Ltd.
 *  System software section
 *
 *  2011-07-13 jykim
 */

#ifndef __GST_MRTP_VIDEO_C_
#define __GST_MRTP_VIDEO_C_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include "gobjmrtpsrc.h"
#include "gstmrtpdefs.h"
#include "SEED_KISA.h"

// #ifndef NMZ_STANDLONE_MODE
// #include <gst/nf/gstnfbuddybuffer.h>
// #else
// #include <gstnfbuddybuffer.h>
// #endif
#include <gobjbuddybuffer.h>
#include "nf_codec_header.h"	// FIXME


typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef signed int sint32;

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CEIL(a, b) ((a)+(b)-1)/(b)

#define NAL_BUFFER_MAX (10 * 1024)

typedef struct _nal_buffer {
    int pos;
    int bitpos;
    int posmax;
    uint8 data[NAL_BUFFER_MAX];
} nal_buffer_t;

// NAL unit parser states 00 -> 00 -> [00] -> 01 -> NUT
#define STATE_EXPECTING_ZERO_0 0
#define STATE_EXPECTING_ZERO_1 1
#define STATE_EXPECTING_THREE 2

#define SPS_PRINT_DETAIL	(0)
#define SPS_PRINT_MSG 		(0)
#define SPS_PPS_DESC_DEBUG  (0)


static const guchar itx_uuid_meta[16] = {
	0x77, 0xfe, 0x93, 0x30, 0x11, 0x9b, 0x11, 0xe1,
	0xbe, 0x50, 0x08, 0x00, 0x20, 0x0c, 0x0a, 0x66
};

const unsigned char itx_fisheye_info[16] = {0x77, 0xFE, 0x93, 0x30 ,0x11, 0x9B, 0x11, 0xE1, 0xBE, 0x50, 0x08, 0x00, 0x20, 0x0E, 0x0C, 0x68};

static const guchar itx_uuid_event[16] = {
	0x77, 0xfe, 0x93, 0x30, 0x11, 0x9b, 0x11, 0xe1,
	0xbe, 0x50, 0x08, 0x00, 0x20, 0x0d, 0x0b, 0x67
};

#ifdef NMZ_STANDLONE_MODE
static file_cnt = 0;
#endif

static gint _mrtpsrc_parse_vlen(MRTPSRC_STREAM_T*);
static gint _mrtpsrc_assemble_vframe(MRTPSRC_STREAM_T*, MRTPSRC_FRAME_T, gint);
static gint _mrtpsrc_pad_vliveq(MRTPSRC_STREAM_T*, MRTPSRC_FRAME_T, MRTPSRC_RTP_HEADER_T*);
static gint _mrtpsrc_calculate_vts(MRTPSRC_STREAM_T*, ICODEC_HEADER*, guint);

static gint _mrtpsrc_check_drop_p(MRTPSRC_STREAM_T* , MRTPSRC_FRAME_T);
static void _mrtpsrc_send_rtcp(MRTPSRC_STREAM_T *lst);

static void _mrtpsrc_get_4bytes_from_bits(guchar*, gint, guchar*);
static int _mrtpsrc_get_uev_codenum(guchar*, gint*);
static int _mrtpsrc_get_sev_from_codenum(gint);
static void _mrtpsrc_get_resolution_from_sps(gchar*, gint*, gint*);
static void _mrtpsrc_set_stream_resolution(MRTPSRC_STREAM_T*, gint, gint);
//static int get_frame_seq_from_slice(unsigned char* src, int *seq_num);
//static unsigned int _check_xor(unsigned int size, unsigned int* dat_buf);

static void h265_get_resolution(unsigned char *p_sps, int p_sps_len, unsigned int *p_w, unsigned int *p_h);
/* h.265 resolution innter function */
static void copy_to_nal_buf(nal_buffer_t * pnal_buffer, uint8 c);
static void dump_nal_buffer(nal_buffer_t * pnal_buffer);

static void align_to_byte(nal_buffer_t * pnal_buffer);
static uint8 read_bit(nal_buffer_t * pnal_buffer);
static uint32 read_bits(nal_buffer_t * pnal_buffer, int nbits);
static uint64 read_bits64(nal_buffer_t * pnal_buffer, int nbits);
static uint32 read_uev(nal_buffer_t * pnal_buffer);
static sint32 read_sev(nal_buffer_t * pnal_buffer);

static void profile_tier_level(nal_buffer_t* pnal_buffer, int maxNumSubLayersMinus1);
static void decode_nul_rbsp(nal_buffer_t * buffin, nal_buffer_t * buffout, int size);
/* h.265 resolution innter function */

/* Safe payload length calculator for interleaved RTSP data */
static gint calc_payload_size(guint16 rtsp_len, guchar* payload, guchar* r_ptr)
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
extern int get_live_channel_num(void);
void gst_mrtp_src_resolution_changed(MRTPSRC_STREAM_T *lst, guint old_resol, guint new_resol);

void mrtpsrc_get_resolution_from_sps(gchar* src, gint *width, gint *height)
{
	_mrtpsrc_get_resolution_from_sps(src, width, height);
}

void mrtpsrc_set_stream_resolution(MRTPSRC_STREAM_T *lst, gint w, gint h)
{
	_mrtpsrc_set_stream_resolution(lst, w, h);
}

extern gint mrtpsrc_parse_video(MRTPSRC_STREAM_T* lst)
{
	gint	consume_len = 0;
	gint	total_consumed_len = 0;
	guchar* scan_ptr = NULL;
	guchar* payload = NULL;
	guchar* align_cp = NULL;

	MRTPSRC_RTSP_IF_T* rtsp = NULL;
	MRTPSRC_RTP_HEADER_T* rtp = NULL;
	MRTPSRC_RTP_HEADER_X_T* rtp_x = NULL;

	guint16 rtsp_len = 0;
	guint16 hx_len = 0;

	MRTPSRC_STREAM_STAT_T *stats = NULL;
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *filter = NULL;
#endif


#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
#ifndef NMZ_STANDLONE_MODE
	filter = gst_mrtp_src_get_object();
#endif
	/* Defensive: if buffer pointers are already cleared (e.g., close in progress), bail out */
	if (lst->rbuf.cp == NULL || lst->rbuf.lp == NULL)
	{
		MRTPSRC_DBG(WARN, "%s | ch(%d,%s) rbuf null (cp:%p lp:%p)", __FUNCTION__,
				lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->rbuf.cp, lst->rbuf.lp);
		return RTN_FAIL;
	}

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

			if(rtsp_msg_len == 0)
			{
				if (lst->rbuf.lp - scan_ptr < 256) break;
				MRTPSRC_DBG(ERROR, "%s | Wrong magic number on CH(%d,%s) (%02x, %02x, %04u), RTP sync corrupted",
						__FUNCTION__,
						lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], rtsp->magic, rtsp->channel, rtsp->len);

				lch = mrtpsrc_get_channel(lst->ch_num);
				lch->state = STATE_RECONN_REQ;

#if MRTPSRC_USE_STATISTICS
				stats->reconn_wrong_magic[0][lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
				if (filter == NULL) { return RTN_FAIL; }
				if (filter->cmd_callback == NULL) { return RTN_FAIL; }
				filter->cmd_callback(2, lst->ch_num, 0, 103/*magic A*/, filter->cmd_user_data);
#endif
#endif
				return RTN_FAIL;
			}
			{
				gint _move_len = 0;
				gint _del_len = 0;

				next_rtsp = scan_ptr + rtsp_msg_len;
				_move_len = lst->rbuf.lp - next_rtsp;
				_del_len = next_rtsp - scan_ptr;
							
				// 경계 검사: _del_len이 합리적인 범위인지 확인
				if (_del_len <= 0 || _del_len > lst->buf_sz || _move_len < 0)
				{
					MRTPSRC_DBG(ERROR, "%s %d | Invalid buffer parameters detected. del_len:%d, move_len:%d, buf_sz:%u, rbs:%p, lp:%p",
						__FUNCTION__, __LINE__, _del_len, _move_len, lst->buf_sz, lst->rbuf.rbs, lst->rbuf.lp);
					return RTN_FAIL;
				}
				
				memmove(scan_ptr, next_rtsp, _move_len);
				
				// lp가 rbs보다 작아지지 않도록 보호
				if ((lst->rbuf.lp - _del_len) < lst->rbuf.rbs)
				{
					MRTPSRC_DBG(ERROR, "%s %d | Buffer underflow detected! lp:%p - %d would go below rbs:%p",
						__FUNCTION__, __LINE__, lst->rbuf.lp, _del_len, lst->rbuf.rbs);
					// 안전하게 rbs로 설정
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
		}

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		//if (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp - MRTPSRC_RING_GAP)
		if (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp)
		{
			break;
		}

		MRTPSRC_CHANNEL_T* lch = mrtpsrc_get_channel(lst->ch_num);
		if(lch->model_code == MODEL_ONTHEFLY_CHEAT)
		{
			if (lst->state <= STATE_STREAM_READY)
			{
				lst->state = STATE_PLAYING;
			}

			consume_len= rtsp_len + sizeof(MRTPSRC_RTSP_IF_T);

			total_consumed_len += consume_len;
			lst->rbuf.cp += consume_len;
			lst->rbuf.buf_remain += consume_len;
			lst->rbuf.buf_used -= consume_len;

			scan_ptr = lst->rbuf.cp;

			continue;
		}


#if 0
		if(rtsp->channel > 1)
		{
			{
				int i=0;
				printf("     0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f");
				while (i < 48)
				{
					if (i % 16 == 0)
					{
						printf("\n%02x  ", i/16);
					}
					else if (i % 8 == 0)
					{
						printf(" ");
					}
					printf("%02x ", *(scan_ptr+i++));
				}
				printf("\n\n");
			}
		}
#endif
		if ((rtsp->channel % 2) == 0)
		//if (rtsp->channel == 0)
		{
			/* Video frame */
			rtp = (MRTPSRC_RTP_HEADER_T*) (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T));
			
			/*
			if (rtp->pt < 0x60)
			{
				scan_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				continue;
			}
			*/

			if (rtp->x)
			{
				rtp_x = (MRTPSRC_RTP_HEADER_X_T*) (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T));
			}
			else
			{
				rtp_x = NULL;
			}

			payload = scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T);
			if (rtp_x)
			{
				align_cp = (guchar*) &rtp_x->xlen;
				hx_len = *(align_cp + 1) | *align_cp << 8;	// ntohs(hx_len)

				payload = payload + sizeof(MRTPSRC_RTP_HEADER_X_T) + (hx_len * 4);
			}
			if (rtp->cc)
			{
				payload += (rtp->cc * 4);
			}

			if (rtp->m != 0)
			{

#if 0
				if(rtsp->channel == 2)
				{
					/* metadata channel start */
					consume_len = rtsp_len + sizeof(MRTPSRC_RTSP_IF_T);
					//printf("\n%s Meta Receive %d :\n", __FUNCTION__, consume_len);

					if(filter != NULL)
					{
						if(filter->onvif_meta_callback != NULL)
						{
							filter->onvif_meta_callback(lst->ch_num, payload, rtsp_len - ((guchar*)payload - (guchar*)rtp), filter->onvif_meta_user_data);
						}
					}

					total_consumed_len += consume_len;
					lst->rbuf.cp += consume_len;
					lst->rbuf.buf_remain += consume_len;
					lst->rbuf.buf_used -= consume_len;

					scan_ptr = lst->rbuf.cp;

					continue;
					/* metadata channel end */
				}
#endif
			if(lst->en == RTP_PT_H264)
			{
				/* 1. If the single-nal frame packet detected then process */
				if (((*payload & MRTPSRC_NAL_UNIT_MASK) == RTP_SINGLE_NAL_IDR) || 
							((*payload & MRTPSRC_NAL_UNIT_MASK) == RTP_SINGLE_NAL_N_IDR))
				{
					consume_len = _mrtpsrc_parse_vlen(lst);
					if (consume_len == RTN_FAIL)
					{
						MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);
						MRTPSRC_DBG(WARN, "%s | Frame build failed (Single-NAL)", __FUNCTION__);
						lch->state = STATE_REBOOT_REQ;
						return RTN_FAIL;
					}

					total_consumed_len += consume_len;
					lst->rbuf.cp += consume_len;
					lst->rbuf.buf_remain += consume_len;
					lst->rbuf.buf_used -= consume_len;

					scan_ptr = lst->rbuf.cp;

					continue;
				}
				/* 2. If the last packet of the FU_X type detected then process */
				else if (((*payload & MRTPSRC_NAL_UNIT_MASK) == RTP_FU_A) || 
							((*payload & MRTPSRC_NAL_UNIT_MASK) == RTP_FU_B))
				{
					if ((*(payload + 1) & 0xc0) == 0x40)
					{
						consume_len = _mrtpsrc_parse_vlen(lst);
						if (consume_len == RTN_FAIL)
						{
							MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);
							MRTPSRC_DBG(WARN, "%s | Frame build failed (FU-x)", __FUNCTION__);
							lch->state = STATE_REBOOT_REQ;
							return RTN_FAIL;
						}

						total_consumed_len += consume_len;
						lst->rbuf.cp += consume_len;
						lst->rbuf.buf_remain += consume_len;
						lst->rbuf.buf_used -= consume_len;

						scan_ptr = lst->rbuf.cp;

						continue;
					}
				}
			}
			else if(lst->en == RTP_PT_H265)
			{
				gint nal_type = 0;
				gint tid = 0;
				gint lid = 0;


				/*    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
				 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				 *   |F|   Type    |  LayerId  | TID |
				 *   +-------------+-----------------+ */
				nal_type = ((payload[0] >>1) & 0x3f);
				lid = ((payload[0] << 5) & 0x20) | ((payload[1] >> 3) & 0x1f);
				tid = (payload[1] & 0x07);

				if(!lid && tid && !(nal_type > 50))
				{
					switch(nal_type)
					{
						case NAL_TRAIL_N:
						case NAL_TRAIL_R:
						case NAL_TSA_N:
						case NAL_TSA_R:
						case NAL_STSA_N:
						case NAL_STSA_R:
						case NAL_RADL_N:
						case NAL_RADL_R:
						case NAL_RASL_N:
						case NAL_RASL_R:
						case NAL_BLA_W_LP:
						case NAL_BLA_W_RADL:
						case NAL_BLA_N_LP:
						case NAL_IDR_W_RADL:
						case NAL_IDR_N_LP:
						case NAL_CRA_NUT:
						case NAL_AUD:
						case NAL_EOS_NUT:
						case NAL_EOB_NUT:
						case NAL_FD_NUT:
							{
								consume_len = _mrtpsrc_parse_vlen(lst);
								if (consume_len == RTN_FAIL)
								{
									MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);
									MRTPSRC_DBG(WARN, "%s | Frame build failed (Single-NAL)", __FUNCTION__);
									lch->state = STATE_REBOOT_REQ;
									return RTN_FAIL;
								}

								total_consumed_len += consume_len;
								lst->rbuf.cp += consume_len;
								lst->rbuf.buf_remain += consume_len;
								lst->rbuf.buf_used -= consume_len;

								scan_ptr = lst->rbuf.cp;

								continue;
							}
						case NAL_FUS:
							/* Fragmentation Units (FUs)*/
							{
								if(payload[2] & 0x40)
								{
									consume_len = _mrtpsrc_parse_vlen(lst);
									if (consume_len == RTN_FAIL)
									{
										MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);
										MRTPSRC_DBG(WARN, "%s | Frame build failed (FUs)", __FUNCTION__);
										lch->state = STATE_REBOOT_REQ;
										return RTN_FAIL;
									}

									total_consumed_len += consume_len;
									lst->rbuf.cp += consume_len;
									lst->rbuf.buf_remain += consume_len;
									lst->rbuf.buf_used -= consume_len;

									scan_ptr = lst->rbuf.cp;

									continue;
								}
							}
					}
				}
			}

			/* rtp marker bit set end */
			}
		/* rtsp channel 0 end */
		}

		scan_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
	} // end while

	if (lst->rbuf.lp != NULL && lst->rbuf.cp != NULL)
	{
		if ((lst->rbuf.lp - lst->rbuf.rbs) > (lst->buf_sz / 2))
		{
			if (lst->rbuf.buf_used)
			{
				memmove(lst->rbuf.rbs, lst->rbuf.cp, lst->rbuf.buf_used);
			}

			lst->rbuf.cp = lst->rbuf.rbs;
			lst->rbuf.lp = lst->rbuf.rbs + lst->rbuf.buf_used;
		}
	}

	return total_consumed_len;
}

/* Frame length calculation -> Call _mrtpsrc_assemble_vframe() */
static gint _mrtpsrc_parse_vlen(MRTPSRC_STREAM_T* lst)
{
	const guint sync_bytes = htonl(1);

	int len = 0;
	int header_len = 0;

	guchar spspps_added = 0;

	MRTPSRC_RTSP_IF_T* rtsp = NULL;
	MRTPSRC_RTP_HEADER_T* rtp = NULL;
	MRTPSRC_RTP_HEADER_X_T* rtpx = NULL;

	guchar* r_ptr = NULL;
	guchar* payload = NULL;
	guchar* align_cp = NULL;

	gint consumed_bytes = 0;
	gint video_len = 0;

	guchar fu_header = 0;
	guint16 hx_len = 0;

	guint16 rtsp_len = 0;
	gint rtp_seq = -1;

	GobjMrtpSrcHeaderX rtn_hx;

	MRTPSRC_FRAME_T new_frame = NULL;
	MRTPSRC_FRAME_T sei_meta_frame = NULL;
	MRTPSRC_CHANNEL_T *lch = NULL;

	MRTPSRC_STREAM_STAT_T *stats = NULL;

#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *src = gst_mrtp_src_get_object();
#endif

#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
	guchar check_sei=0;
#endif


#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
	lch = mrtpsrc_get_channel(lst->ch_num);


	/* 1. Get video length */
	r_ptr = lst->rbuf.cp;
    memset(&rtn_hx, 0x00, sizeof(GobjMrtpSrcHeaderX));
	
	// 버퍼 포인터 유효성 검사
	if (r_ptr > lst->rbuf.lp || (lst->rbuf.lp - r_ptr) > lst->buf_sz)
	{
		MRTPSRC_DBG(ERROR, "%s %d | (%d, %s) Invalid buffer state. Reset buffer. lp : %p r_ptr : %p buf_size : %u", 
			__FUNCTION__, __LINE__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], 
			lst->rbuf.lp, r_ptr, lst->buf_sz);
		
		// 버퍼 상태가 비정상이므로 초기화
		lst->rbuf.cp = lst->rbuf.lp;
		lst->sr_ssrc = 0;
		lst->seq_last = 0;
		return 0;
	}
	
	while (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) < lst->rbuf.lp)
	{
		gint rtsp_msg_len = 0;
		rtsp = (MRTPSRC_RTSP_IF_T*) r_ptr;

		if ((lst->rbuf.lp - r_ptr) > lst->buf_sz) 
		{
			MRTPSRC_DBG(ERROR, ": %s, %d | (%d, %s) Buffer overflow detected! lp : %p r_ptr : %p buf_size : %u", 
				__FUNCTION__, __LINE__, 
				lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->rbuf.lp, r_ptr, lst->buf_sz);
			
			// 버퍼 오버플로우 감지 시 버퍼 초기화하고 안전하게 종료
			lst->rbuf.cp = lst->rbuf.lp;
			lst->sr_ssrc = 0;
			lst->seq_last = 0;
			return consumed_bytes;
		}

		if (rtsp->magic != 0x24)
		{
			rtsp_msg_len = mrtpsrc_get_rtsp_msg_len(r_ptr, lst->rbuf.lp - r_ptr);
			if (rtsp_msg_len == 0)
			{
				return consumed_bytes;
			}

			r_ptr += rtsp_msg_len;
			continue;
		}

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		//if (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp - MRTPSRC_RING_GAP)
		if (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp)
		{
			break;
		}

		if ((rtsp->channel%2) != 0)
		{
			r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;
		}

		/* Video packet */
		rtp = (MRTPSRC_RTP_HEADER_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T));
		lst->itlv_rtp_ch = rtsp->channel;
		lst->itlv_rtcp_ch = rtsp->channel+1;

		/* for the rtcp */
		if((lst->sr_ssrc != ntohl(rtp->ssrc) && lst->sr_ssrc != 0))
		{
			MRTPSRC_DBG(WARN, ": %s %d | ch_s(%d, %s) - rtp ssrc is mismatched.(now:%0X, ori:%0X) - Request reconnection\n",
					__FUNCTION__, __LINE__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], ntohl(rtp->ssrc), lst->sr_ssrc);
			return RTN_FAIL;
		}
		
        /*
		if(lst->sr_ssrc == 0)
		{
			MRTPSRC_DBG(MAJOR, "%s | ch_s(%d, %s) - new rtp ssrc.(now:%0X)\n",
					__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], ntohl(rtp->ssrc));
		}
        */

		{
			lst->sr_ssrc = ntohl(rtp->ssrc);
			lst->seq_last = ntohs(rtp->seq);
			if (lst->seq_last == 0) lst->seq_group++;
		}

		/* skip if not h.264 payload type */
		/*
		if (rtp->pt < 0x60)
		{
			r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;
		}
		*/

		payload = r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T);
		//memset(&rtn_hx, 0x00, sizeof(GobjMrtpSrcHeaderX));
		if (rtp->x)
		{
			rtpx = (MRTPSRC_RTP_HEADER_X_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T));
			align_cp = (guchar*) &rtpx->xlen;
			hx_len = *(align_cp + 1) | *align_cp << 8;	// ntohs(hx_len)
			//if ((lst->stream_num == STREAM_1ST) && rtp->m)
            if(lst->stream_num == STREAM_1ST)
			//_nh = payload + sizeof(MRTPSRC_RTP_HEADER_X_T)+(hx_len * 4) + (rtp->cc ? (rtp->cc*4):0);
			//_fh = _nh+1;
			//if (lst->stream_num == STREAM_1ST)
			{
				//if (((*_nh & MRTPSRC_NAL_UNIT_MASK) != RTP_FU_A && (*_nh & MRTPSRC_NAL_UNIT_MASK) != RTP_FU_B) || (*_fh&0x80)==0x80)
#ifndef NMZ_STANDLONE_MODE
				{
					if (src->header_x_callback != NULL)
					{
						src->header_x_callback(lst->ch_num, payload, hx_len, &rtn_hx, src->header_x_user_data);
					}
				}
#endif
			}

			/* Skip header extension */
			payload = payload + sizeof(MRTPSRC_RTP_HEADER_X_T) + (hx_len * 4);
		}
		else
		{
			if ((lch->model_code >= MODEL_ONVIF_GENERAL) && (lst->stream_num == STREAM_1ST) && rtp->m)
			//if (rtp->m)
			{
#ifndef NMZ_STANDLONE_MODE
				if (src->header_x_callback != NULL)
				{
					src->header_x_callback(lst->ch_num, NULL, 0, &rtn_hx, src->header_x_user_data);
				}
#endif
			}
		}
		if (rtp->cc)
		{
			payload += (rtp->cc * 4);
		}

        /* Check rtp seq */
        if (rtp_seq == -1)
        {
            rtp_seq = (gint)(ntohs(rtp->seq));
        }
        else
        {
            gint seq_exp = (rtp_seq + 1) & 0xffff;
            gint seq_cur = (gint)(ntohs(rtp->seq)) & 0xffff;

            rtp_seq = seq_cur;
            if (seq_exp != seq_cur)
            {
                MRTPSRC_DBG(WARN, "%s | ch_s(%d, %s) - rtp seq jump is detected.(e:%d, c:%d)\n",
                        __FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], seq_exp, seq_cur);
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				stats->frame_drop_seqnum[lst->stream_num][lst->ch_num]++;
				continue;
            }

        }

		if(lst->en == RTP_PT_H264)
		{

		switch (*payload++ & MRTPSRC_NAL_UNIT_MASK)
		{
			case RTP_SINGLE_NAL_IDR:
			{
#ifdef MRTPSRC_ADD_SPSPPS
				video_len += (sizeof(sync_bytes) + lst->sps_len + sizeof(sync_bytes) + lst->pps_len);
#endif
				video_len += sizeof(sync_bytes);
				video_len += sizeof(guchar); // NAL type byte
				video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
				if (new_frame == NULL)
				{
					MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
							__FUNCTION__,
							lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

					lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
					stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
					if (src == NULL) { return RTN_FAIL; }
					if (src->cmd_callback == NULL) { return RTN_FAIL; }
					src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
					return RTN_FAIL;
				}
				//g_assert(new_frame != NULL);
				memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
				new_frame->len = video_len;
				new_frame->frame_type = FTYPE_IDR;

				//if (lch->model_code <= MODEL_ONVIF_GENERAL || lch->model_code == MODEL_ONVIF_GRUNDIG)
				{
					new_frame->motion_raw.ch = lst->ch_num;
					new_frame->motion_flag = rtn_hx.mflag;
					new_frame->alarm_flag = rtn_hx.aflag;
					if (rtn_hx.mrd_len > 0)
					{
						new_frame->motion_raw.stream_num = lst->stream_num;
						new_frame->motion_raw.width = rtn_hx.mrd_width;
						new_frame->motion_raw.height = rtn_hx.mrd_height;
						memcpy(new_frame->motion_raw.mraw, rtn_hx.mraw, rtn_hx.mrd_len);
					}
				}

				len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
				if (len == RTN_FAIL)
				{
					MRTPSRC_DBG(WARN, "%s | _mrtpsrc_assemble_vframe fail(SINGLE_NAL_I)\n", __FUNCTION__);
					return consumed_bytes;
				}
				consumed_bytes += len;
				video_len = 0;

				break;
			}
			case RTP_SINGLE_NAL_N_IDR:
			{
				video_len += sizeof(sync_bytes);
				video_len += sizeof(guchar); // NAL type byte
				video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
				if (new_frame == NULL)
				{
					MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
							__FUNCTION__,
							lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
					lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
					stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
					if (src == NULL) { return RTN_FAIL; }
					if (src->cmd_callback == NULL) { return RTN_FAIL; }
					src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
					return RTN_FAIL;
				}
				//g_assert(new_frame != NULL);
				memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
				new_frame->len = video_len;
				new_frame->frame_type = FTYPE_N_IDR;
				//if(lch->model_code <= MODEL_ONVIF_GENERAL)
				//if (lch->model_code <= MODEL_ONVIF_GENERAL || lch->model_code == MODEL_ONVIF_GRUNDIG)
				{
					new_frame->motion_raw.ch = lst->ch_num;
					new_frame->motion_flag = rtn_hx.mflag;
					new_frame->alarm_flag = rtn_hx.aflag;
					if (rtn_hx.mrd_len > 0)
					{
						new_frame->motion_raw.stream_num = lst->stream_num;
						new_frame->motion_raw.width = rtn_hx.mrd_width;
						new_frame->motion_raw.height = rtn_hx.mrd_height;
						memcpy(new_frame->motion_raw.mraw, rtn_hx.mraw, rtn_hx.mrd_len);
					}
				}

				len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
				if (len == RTN_FAIL)
				{
					MRTPSRC_DBG(WARN, "%s | _mrtpsrc_assemble_vframe fail(SINGLE_NAL_P)\n", __FUNCTION__);
					return consumed_bytes;
				}
				consumed_bytes += len;
				video_len = 0;

				break;
			}
			case RTP_SPS:
			{
				guint sps_len = 0;
				//MRTPSRC_DBG(WARN, "%s | Single-NAL SPS update is not supported yet", __FUNCTION__);
				header_len = payload - r_ptr - 1;
				sps_len = rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
				if (lst->sps_len != sps_len)
				{
					//MRTPSRC_DBG(MINOR, "CH(%d) STREAM(%s) sps updated\n", lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
					lst->sps_len = sps_len;
				}
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
			}
			case RTP_PPS:
			{
				guint pps_len = 0;
				//MRTPSRC_DBG(WARN, "%s | Single-NAL PPS update is not supported yet", __FUNCTION__);
				header_len = payload - r_ptr - 1;
				pps_len =  rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
				if (lst->pps_len != pps_len)
				{
					//MRTPSRC_DBG(MINOR, "CH(%d) STREAM(%s) pps updated\n", lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
					lst->pps_len = pps_len;
				}
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
			}
			case RTP_MTAP_16:
			case RTP_MTAP_24:
			{
				//MRTPSRC_DBG(ERROR, "%s | Unhandled RTP type(%d)", __FUNCTION__, (*(payload-1) & MRTPSRC_NAL_UNIT_MASK));
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				break;
				//return RTN_FAIL;
			}
			case RTP_STAP_B:
				payload += 2;
			case RTP_STAP_A:
			{
				//MRTPSRC_DBG(ERROR, "%s | Unhandled RTP type(%d)", __FUNCTION__, (*(payload-1) & MRTPSRC_NAL_UNIT_MASK));
#if 0
				while(MRTPSRC_ALWAYS)
				{
					int nal_length;
					if (payload - r_ptr >= sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len)
					{
						break;
					}

					nal_length = (*payload) << 8 | *(payload+1);
					video_len += sizeof(sync_bytes);	// 0x00000001
					video_len += nal_length;
					payload += (sizeof(guint16) + nal_length);

					if ((*(payload + sizeof(guint16)) & MRTPSRC_NAL_UNIT_MASK) == RTP_SINGLE_NAL_IDR || 
								(*(payload + sizeof(guint16)) & MRTPSRC_NAL_UNIT_MASK) == RTP_SINGLE_NAL_N_IDR)
					{
						video_len += sizeof(sync_bytes);

						new_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
						if (new_frame == NULL)
						{
							MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%ld, %s)\n",
									__FUNCTION__,
									lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

							lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
							stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
							if (src == NULL) { return RTN_FAIL; }
							if (src->cmd_callback == NULL) { return RTN_FAIL; }
							src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
							return RTN_FAIL;
						}
						//g_assert(new_frame != NULL);
						memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
						new_frame->len = video_len;
						new_frame->frame_type = FTYPE_N_IDR;
					//if(lch->model_code <= MODEL_ONVIF_GENERAL)
					//if (lch->model_code <= MODEL_ONVIF_GENERAL || lch->model_code == MODEL_ONVIF_GRUNDIG)
					{
						new_frame->motion_raw.ch = lst->ch_num;
						new_frame->motion_flag = rtn_hx.mflag;
						new_frame->alarm_flag = rtn_hx.aflag;
						if (rtn_hx.mrd_len > 4)
						{
							new_frame->motion_raw.stream_num = lst->stream_num;
							new_frame->motion_raw.width = rtn_hx.mrd_width;
							new_frame->motion_raw.height = rtn_hx.mrd_height;
							{
								int i = 0;
#if MRTPSRC_PRINT_MRAW
								printf("================%02d=================\n", lst->gop_seq);
#endif
								for (i = 0; i < (rtn_hx.mrd_width * rtn_hx.mrd_height); i++)
								{
									if (rtn_hx.mrd_max-rtn_hx.mrd_min <= 0)
									{
										new_frame->motion_raw.mraw[i] = 0;
									}
									else
									{
										new_frame->motion_raw.mraw[i] = 
												((rtn_hx.mraw[i] - rtn_hx.mrd_min)*11) / (rtn_hx.mrd_max - rtn_hx.mrd_min);
										if (new_frame->motion_raw.mraw[i] == 0 && rtn_hx.mraw[i] != 0)
										{
											new_frame->motion_raw.mraw[i] = 1;
										}
										if (new_frame->motion_raw.mraw[i] >= 11)
										{
											new_frame->motion_raw.mraw[i] = 10;
										}
#if MRTPSRC_PRINT_MRAW
										if (i!=0 && (i%12 == 0)) {printf("\n");}
										//printf("%02d ", new_frame->motion_raw.mraw[i]);
										printf("%03d ", rtn_hx.mraw[i]);
#endif
									}
								}
#if MRTPSRC_PRINT_MRAW
								printf("\n-----------------------------------\n");
#endif
							}
						}
					}

						len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
						if (len == RTN_FAIL)
						{
							MRTPSRC_DBG(WARN, "%s | _mrtpsrc_assemble_vframe fail(STAP_AB)\n", __FUNCTION__);
							return consumed_bytes;
						}
						consumed_bytes += len;
						video_len = 0;
						break;
					}
				}
#endif

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				break;
			}
			case RTP_FU_B:
			case RTP_FU_A:
			{
				fu_header = *payload++;

				if (fu_header & 0x80)
				{
					/* FU header byte: First one of the fragmentations */
#ifdef MRTPSRC_ADD_SPSPPS
					if (lch->model_code >= MODEL_ONVIF_GENERAL)
					{
						if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x05)
						{
							video_len += (sizeof(sync_bytes) + lst->sps_len + sizeof(sync_bytes) + lst->pps_len);
							if (spspps_added == 0)
							{
								spspps_added = 1;
							}
						}
					}
					else
					{
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
						//start bit && sei slice
						if((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x06)
						{
							guchar *p_ptr = payload;
							int k = 0;

							guchar *uuid_ptr;

							uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_meta, 16);

							if(uuid_ptr != NULL)
							{
								check_sei = FTYPE_VA_META;
							}
							else
							{
								uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_event, 16);
								if(uuid_ptr != NULL)
								{
									check_sei = FTYPE_VA_EVENT;
									//MRTPSRC_DBG(WARN, "%s | FTYPE_VA_EVENT FU %d\n",
									//		__FUNCTION__,lst->stream_num);
								}
							}

							if (uuid_ptr == NULL)
							{
								//MRTPSRC_DBG(WARN, "%s | FTYPE_VA_SKIP FU, fu_header(%02x)",
								//	__FUNCTION__, fu_header);
								check_sei = FTYPE_VA_SKIP;
							}
							else
							{
								int sei_size = 0;

								for(k=0; p_ptr[k+1] == 0xff;k++)
								{
									sei_size += p_ptr[k+1] + 1;
								}
								if(p_ptr[k+1+1] != 0xfe)
									sei_size += p_ptr[k+1] + 1;

								sei_size += 6; //offset
							}

						}
						else
						{
							lst->check_sei = 0;

							if(lst->stream_num == STREAM_1ST){
								if(lst->sei_meta_1st != NULL){
									MRTPSRC_FRAME_T sei = lst->sei_meta_1st;

									video_len += sei->len;	
									lst->check_sei += 0x01;
								}
								if(lst->sei_event_1st != NULL){
									MRTPSRC_FRAME_T sei = lst->sei_event_1st;

									video_len += sei->len;	
									lst->check_sei += 0x10;
								}

							}
							else{
								if(lst->sei_meta_2nd!= NULL){
									MRTPSRC_FRAME_T sei = lst->sei_meta_2nd;

									video_len += sei->len;	
									lst->check_sei += 0x01;
								}
								if(lst->sei_event_2nd!= NULL){
									MRTPSRC_FRAME_T sei = lst->sei_event_2nd;

									video_len += sei->len;	
									lst->check_sei += 0x10;
								}

							}


						}


#endif
						if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x05)
						{
							video_len += (sizeof(sync_bytes) + lst->sps_len + sizeof(sync_bytes) + lst->pps_len);
							if(lst->sei_len > 0)
							{
								video_len += (sizeof(sync_bytes) + lst->sei_len);
							}
						}
					}
#endif

					video_len += sizeof(sync_bytes);
					video_len += sizeof(guchar);
				}

				video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				//FU_A SEI - VA_META EXCEPTION CODE
				if (((fu_header & 0x40) == 0x40) && (!(rtp->m)) && 
						((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x06))
				{
					/* FU header byte: Last one of the fragmentations */
					sei_meta_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
					if (sei_meta_frame == NULL)
					{
						MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
								__FUNCTION__,
								lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

						lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
						stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
						if (src == NULL) { return RTN_FAIL; }
						if (src->cmd_callback == NULL) { return RTN_FAIL; }
						src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
						return RTN_FAIL;
					}
					memset(sei_meta_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
					sei_meta_frame->len = video_len;
					sei_meta_frame->frame_type = FTYPE_VA_META;

					len = _mrtpsrc_assemble_vframe(lst, sei_meta_frame, consumed_bytes);
					if (len == RTN_FAIL)
					{
						MRTPSRC_DBG(ERROR, "%s %d | (%d, %s)_mrtpsrc_assemble_vframe fail(FU_AB)\n", __FUNCTION__, __LINE__,
							lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
						return consumed_bytes;
					}
					consumed_bytes += len;
					video_len = 0;
				}

				if (((fu_header & 0x40) == 0x40) && rtp->m)
				{
					/* FU header byte: Last one of the fragmentations */
					//new_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
					new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
					if (new_frame == NULL)
					{
						MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
								__FUNCTION__,
								lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

						lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
						stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
						if (src == NULL) { return RTN_FAIL; }
						if (src->cmd_callback == NULL) { return RTN_FAIL; }
						src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
						return RTN_FAIL;
					}
					//g_assert(new_frame != NULL);
					memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
					new_frame->len = video_len;
					if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x05)
					{
						new_frame->frame_type = FTYPE_IDR;
					}
					else if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x06)
					{
						new_frame->frame_type = check_sei;
					}
					else 
					{
						new_frame->frame_type = FTYPE_N_IDR;
					}
					//if (lch->model_code <= MODEL_ONVIF_GENERAL)
					//if (lch->model_code <= MODEL_ONVIF_GENERAL || lch->model_code == MODEL_ONVIF_GRUNDIG)
					{
						new_frame->motion_raw.ch = lst->ch_num;
						new_frame->motion_flag = rtn_hx.mflag;
						new_frame->alarm_flag = rtn_hx.aflag;
						if (rtn_hx.mrd_len > 0)
						{
							new_frame->motion_raw.stream_num = lst->stream_num;
							new_frame->motion_raw.width = rtn_hx.mrd_width;
							new_frame->motion_raw.height = rtn_hx.mrd_height;
							memcpy(new_frame->motion_raw.mraw, rtn_hx.mraw, rtn_hx.mrd_len);
						}
					}

					len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
					if (len == RTN_FAIL)
					{
						MRTPSRC_DBG(ERROR, "%s %d | (%d, %s)_mrtpsrc_assemble_vframe fail(FU_AB)\n", __FUNCTION__, __LINE__,
							lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
						return consumed_bytes;
					}
					consumed_bytes += len;
					video_len = 0;
				}

				break;
			}
			case RTP_SEI:
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
			{
				guchar *uuid_ptr;
				guint sei_size = 0;
				int i;
				guchar* p = payload;

				if(p[0] != 5){
					r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					// disable for ONVIF cam sending SEI frame
					//MRTPSRC_DBG(WARN, "%s | p[0] != 5 %d  %x %x %x %x", __FUNCTION__, video_len, p[0],p[1],p[2],p[3]);
					break;
				}
				
				uuid_ptr = memmem(r_ptr, rtsp_len, itx_fisheye_info, 16);
				if(uuid_ptr != NULL)
				{
					guint sei_len = 0;
					header_len = payload - r_ptr - 1;
					sei_len = rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
					lst->sei_len = sei_len;
					r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					break;
				}
				
				
				if(video_len > 0){
					for(i=0; p[i+1] == 0xff;i++){
						sei_size += p[i+1] + 1;
					}
					if(p[i+1+1] != 0xfe)
						sei_size += p[i+1] + 1;
						
					sei_size += 6; //offset
					MRTPSRC_DBG(WARN, "%s | sei_size is too big %d sei_size %d %x %x %x %x", __FUNCTION__, video_len,sei_size, p[0],p[1],p[2],p[3]);
				}
				
				check_sei = 0;

				uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_meta, 16);

				if(uuid_ptr != NULL){
					check_sei = FTYPE_VA_META;
				}
				else{
					uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_event, 16);
					if(uuid_ptr != NULL){
						check_sei = FTYPE_VA_EVENT;
						MRTPSRC_DBG(WARN, "%s | FTYPE_VA_EVENT", __FUNCTION__);
					}
				}

				if (uuid_ptr == NULL)
				{
					r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					//MRTPSRC_DBG(WARN, "%s | uuid_ptr == NULL %d\n",
					//	__FUNCTION__,lst->stream_num);
					break;
				}
				video_len += sizeof(sync_bytes);
				video_len += sizeof(guchar); // NAL type byte
				video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				//new_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
				new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
				if (new_frame == NULL)
				{
					MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
							__FUNCTION__,
							lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

					lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
					stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
					if (src == NULL) { return RTN_FAIL; }
					if (src->cmd_callback == NULL) { return RTN_FAIL; }
					src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
					return RTN_FAIL;
				}
				//g_assert(new_frame != NULL);
				memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
				new_frame->len = video_len;
				new_frame->frame_type = check_sei;

				len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
				if (len == RTN_FAIL)
				{
					return consumed_bytes;
				}
				consumed_bytes += len;
				video_len = 0;

			}
				break;
#endif
			case RTP_CSD_A:
			case RTP_CSD_B:
			case RTP_CSD_C:
			case RTP_AUD:
			case RTP_EO_SEQ:
			case RTP_EO_STREAM:
			case RTP_FILTER_DATA:
			case RTP_SPS_EXT:
			{
#if 0
				MRTPSRC_DBG(ERROR, "%s | Unhandled RTP format(%d)",
						__FUNCTION__, (nal_header & MRTPSRC_NAL_UNIT_MASK));
#endif
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
			}
			case RTP_UNDEFINED_T0:
			case RTP_UNDEFINED_T30:
			case RTP_UNDEFINED_T31:
			{
				MRTPSRC_DBG(ERROR, "%s | Undefined RTP format(%d)",
						__FUNCTION__, (*payload & MRTPSRC_NAL_UNIT_MASK));

				return RTN_FAIL;
			}
			default:
			{
				MRTPSRC_DBG(ERROR, "%s | Unhandled NAL unit type %d",
						__FUNCTION__, (*payload & MRTPSRC_NAL_UNIT_MASK));

				/*if (new_frame != NULL)
				{
					if (new_frame->frame != NULL)
					{
						g_object_unref(new_frame->frame);
					}
					free(new_frame);
					new_frame = NULL;
				}*/
				return RTN_FAIL;
			}
		}
		}
		else if(lst->en == RTP_PT_H265)
		{
			gint nal_type = 0;
			gint tid = 0;
			gint lid = 0;

			/* Nal Unit Header
			 *    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 
			 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *   |F|   Type    |  LayerId  | TID |
			 *   +-------------+-----------------+ */
			nal_type = (payload[0] >>1) & 0x3f;
			lid =  ((payload[0] << 5) & 0x20) | ((payload[1] >> 3) & 0x1f);
			tid = (payload[1] & 0x07);

			payload += 2; // Nal Header Size 2

			switch(nal_type)
			{
				case NAL_TRAIL_N:
				case NAL_TRAIL_R:
				case NAL_TSA_N:
				case NAL_TSA_R:
				case NAL_STSA_N:
				case NAL_STSA_R:
				case NAL_RADL_N:
				case NAL_RADL_R:
				case NAL_RASL_N:
				case NAL_BLA_W_LP:
				case NAL_BLA_W_RADL:
				case NAL_BLA_N_LP:
				case NAL_IDR_N_LP:
				case NAL_CRA_NUT:
					{
						video_len += sizeof(sync_bytes);
						video_len += sizeof(guchar) * 2; // NAL type byte
						video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

                        if(rtp->m == 0){
                            continue;
                        }
						new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
						if (new_frame == NULL)
						{
							MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
									__FUNCTION__,
									lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
							lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
							stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
							if (src == NULL) { return RTN_FAIL; }
							if (src->cmd_callback == NULL) { return RTN_FAIL; }
							src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
							return RTN_FAIL;
						}
						//g_assert(new_frame != NULL);
						memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
						new_frame->len = video_len;
						new_frame->frame_type = FTYPE_N_IDR;

						{
							new_frame->motion_raw.ch = lst->ch_num;
							new_frame->motion_flag = rtn_hx.mflag;
							new_frame->alarm_flag = rtn_hx.aflag;
							if (rtn_hx.mrd_len > 0)
							{
								new_frame->motion_raw.stream_num = lst->stream_num;
								new_frame->motion_raw.width = rtn_hx.mrd_width;
								new_frame->motion_raw.height = rtn_hx.mrd_height;
								memcpy(new_frame->motion_raw.mraw, rtn_hx.mraw, rtn_hx.mrd_len);
							}
						}

						len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
						if (len == RTN_FAIL)
						{
							MRTPSRC_DBG(WARN, "%s | _mrtpsrc_assemble_vframe fail(SINGLE_NAL_P)\n", __FUNCTION__);
							return consumed_bytes;
						}
						consumed_bytes += len;
						video_len = 0;

						break;
					}
				case NAL_IDR_W_RADL:
					{
#ifdef MRTPSRC_ADD_SPSPPS
						if(lst->sps_added ==0){
							video_len += (sizeof(sync_bytes) + lst->vps_len + sizeof(sync_bytes) + lst->sps_len + sizeof(sync_bytes) + lst->pps_len);
							if(lst->sei_len > 0)
							{
								video_len += sizeof(sync_bytes) + lst->sei_len;
							}
								lst->sps_added = 1;
						}	
#endif
						video_len += sizeof(sync_bytes);
						video_len += sizeof(guchar) * 2; // NAL type byte
						video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T)); // H264 Payload Size

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

						if(rtp->m == 0){
							continue;
						}
						new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
						if (new_frame == NULL)
						{
							MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
									__FUNCTION__,
									lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

							lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
							stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
							if (src == NULL) { return RTN_FAIL; }
							if (src->cmd_callback == NULL) { return RTN_FAIL; }
							src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
							return RTN_FAIL;
						}
						//g_assert(new_frame != NULL);
						memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
						new_frame->len = video_len;
						new_frame->frame_type = FTYPE_IDR;

						{
							new_frame->motion_raw.ch = lst->ch_num;
							new_frame->motion_flag = rtn_hx.mflag;
							new_frame->alarm_flag = rtn_hx.aflag;
							if (rtn_hx.mrd_len > 0)
							{
								new_frame->motion_raw.stream_num = lst->stream_num;
								new_frame->motion_raw.width = rtn_hx.mrd_width;
								new_frame->motion_raw.height = rtn_hx.mrd_height;
								memcpy(new_frame->motion_raw.mraw, rtn_hx.mraw, rtn_hx.mrd_len);
							}
						}

						len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
						if (len == RTN_FAIL)
						{
							MRTPSRC_DBG(WARN, "%s | _mrtpsrc_assemble_vframe fail(SINGLE_NAL_I)\n", __FUNCTION__);
							return consumed_bytes;
						}
						consumed_bytes += len;
						video_len = 0;

						break;
					}

				case NAL_VPS:
					/* Video Parameter Set */
					{
						guint vps_len = 0;
						vps_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);

						if (lst->vps_len != vps_len)
						{
							lst->vps_len = vps_len;
						}

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				case NAL_SPS:
					/* Sequence Parameter Set */
					{
						guint sps_len = 0;
						sps_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);

						if (lst->sps_len != sps_len)
						{
							lst->sps_len = sps_len;
						}

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				case NAL_PPS:
					/* Picture Parameter Set */
					{
						guint pps_len = 0;
						pps_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);

						if (lst->pps_len != pps_len)
						{
							lst->pps_len = pps_len;
						}

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				case NAL_AUD:
					/* Access Unit Delimiter */
					break;
				case NAL_SEI_PREFIX:
				case NAL_SEI_SUFFIX:
					/* Supplemental enhancement information */
					{
						guchar *uuid_ptr;
						guint sei_size = 0;
						guint sei_len = 0;
						int i;
						guchar* p = payload;

						sei_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);

						if(p[0] != 5){
							if(lst->sei_len > 128)
							{
								lst->sei_len = 0;
							}
							else if(lst->sei_len != sei_len)
							{
								lst->sei_len = sei_len;
							}
							r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
							break;
						}

						uuid_ptr = memmem(r_ptr, rtsp_len, itx_fisheye_info, 16);
						if(uuid_ptr != NULL)
						{
							guint sei_len = 0;
							sei_len = rtsp_len - (payload - 2 - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T);
							lst->sei_len = sei_len;
							r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
							break;
						}

						if(video_len > 0){
							for(i=0; p[i+1] == 0xff;i++){
								sei_size += p[i+1] + 1;
							}
							if(p[i+1+1] != 0xfe)
								sei_size += p[i+1] + 1;

							sei_size += 6; //offset
							MRTPSRC_DBG(WARN, "%s | sei_size is too big %d sei_size %d %x %x %x %x", __FUNCTION__, video_len,sei_size, p[0],p[1],p[2],p[3]);
						}

						check_sei = 0;

						uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_meta, 16);

						if(uuid_ptr != NULL){
							check_sei = FTYPE_VA_META;
						}
						else{
							uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_event, 16);
							if(uuid_ptr != NULL){
								check_sei = FTYPE_VA_EVENT;
								MRTPSRC_DBG(WARN, "%s | FTYPE_VA_EVENT", __FUNCTION__);
							}
						}

						if (uuid_ptr == NULL)
						{
							r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
							//MRTPSRC_DBG(WARN, "%s | uuid_ptr == NULL %d\n",
							//	__FUNCTION__,lst->stream_num);
							break;
						}
						video_len += sizeof(sync_bytes);
						video_len += sizeof(guchar) * 2; // NAL type byte
						video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

						new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
						if (new_frame == NULL)
						{
							MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
									__FUNCTION__,
									lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

							lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
							stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
							if (src == NULL) { return RTN_FAIL; }
							if (src->cmd_callback == NULL) { return RTN_FAIL; }
							src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
							return RTN_FAIL;
						}
						//g_assert(new_frame != NULL);
						memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
						new_frame->len = video_len;
						new_frame->frame_type = check_sei;

						len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
						if (len == RTN_FAIL)
						{
							return consumed_bytes;
						}
						consumed_bytes += len;
						video_len = 0;

					}
					break;
				case NAL_FUS:
				/* Fragmentation Units (FUs)*/
					{
                        int nalt;
                        unsigned int frame_type = 0;

						/*
						 *    decode the FU header
						 *
						 *     0 1 2 3 4 5 6 7
						 *    +-+-+-+-+-+-+-+-+
						 *    |S|E|  FuType   |
						 *    +---------------+
						 *
						 *       Start fragment (S): 1 bit
						 *       End fragment (E): 1 bit
						 *       FuType: 6 bits
						 */
						fu_header = *payload++;
                        nalt = fu_header & 0x3f;

                        if (nalt >= 19 && nalt <= 20)
                        {
                            frame_type = FTYPE_IDR;
                        }
                        else if (nalt >= 1 && nalt <= 18 || nalt == 21)
                        {
                            frame_type = FTYPE_N_IDR;
                        }
                        else
                        {
                            MRTPSRC_DBG(WARN, "%s | Unknown type frame, fu_header(%02x)",
                                   __FUNCTION__, fu_header);
                            frame_type = FTYPE_N_IDR;
                        }


						if(fu_header & 0x80) // Start Fragment Check
						{
                            if(frame_type != FTYPE_N_IDR)
                            {
							/* FU header byte: First one of the fragmentations */
#ifdef MRTPSRC_ADD_SPSPPS
							if (lch->model_code >= MODEL_ONVIF_GENERAL)
							{
								if(lst->vps_len > 0)
								{
									video_len += (sizeof(sync_bytes) +  lst->vps_len);
								}
								if(lst->sps_len > 0)
								{
									video_len += (sizeof(sync_bytes) +  lst->sps_len);
								}
								if(lst->pps_len > 0)
								{
									video_len += (sizeof(sync_bytes) +  lst->pps_len);
								}
								if(lst->sei_len > 0)
								{
									video_len += (sizeof(sync_bytes) +  lst->sei_len);
								}

								if (spspps_added == 0)
								{
									spspps_added = 1;
								}
							}
							else
							{
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA

								if((fu_header & 0x3f) == 39 ||(fu_header & 0x3f) == 40) // SEI
								{
									guchar *uuid_ptr;

									uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_meta, 16);

									if(uuid_ptr != NULL){
										check_sei = FTYPE_VA_META;
									}
									else{
										uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_event, 16);
										if(uuid_ptr != NULL){
											check_sei = FTYPE_VA_EVENT;
											//MRTPSRC_DBG(WARN, "%s | FTYPE_VA_EVENT FU %d\n",
											//		__FUNCTION__,lst->stream_num);
										}
									}

									if (uuid_ptr == NULL)
									{
										//MRTPSRC_DBG(WARN, "%s | FTYPE_VA_SKIP FU, fu_header(%02x)",
										//	__FUNCTION__, fu_header);
										check_sei = FTYPE_VA_SKIP;
									}

								}
								else
								{

									lst->check_sei = 0;

									if(lst->stream_num == STREAM_1ST)
									{
										if(lst->sei_meta_1st != NULL)
										{
											MRTPSRC_FRAME_T sei = lst->sei_meta_1st;

											video_len += sei->len;	
											lst->check_sei += 0x01;
										}

										if(lst->sei_event_1st != NULL)
										{
											MRTPSRC_FRAME_T sei = lst->sei_event_1st;

											video_len += sei->len;	
											lst->check_sei += 0x10;
										}

									}
									else
									{
										if(lst->sei_meta_2nd!= NULL)
										{
											MRTPSRC_FRAME_T sei = lst->sei_meta_2nd;

											video_len += sei->len;	
											lst->check_sei += 0x01;
										}

										if(lst->sei_event_2nd!= NULL)
										{
											MRTPSRC_FRAME_T sei = lst->sei_event_2nd;

											video_len += sei->len;	
											lst->check_sei += 0x10;
										}
									}

								}


#endif
								if ((fu_header & 0x3f) == 20 || (fu_header & 0x3f) == 19)
								{
									//origin code
									//video_len += (sizeof(sync_bytes) + lst->vps_len + sizeof(sync_bytes) + lst->sps_len + sizeof(sync_bytes) + lst->pps_len);
									if(lst->sei_len > 0)
									{
										video_len += (sizeof(sync_bytes) + lst->sei_len);
									}
									//added code
									if (fu_header & 0x80)
									{
										video_len += (sizeof(sync_bytes) + lst->vps_len + sizeof(sync_bytes) + lst->sps_len + sizeof(sync_bytes) + lst->pps_len);
										lst->sps_added = 1;
									}
								}
							}
#endif
                            }
							video_len += sizeof(sync_bytes);
							video_len += sizeof(guchar) * 2;
						}

						video_len += (rtsp_len - (payload - r_ptr) + sizeof(MRTPSRC_RTSP_IF_T));

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

						if (((fu_header & 0x40) == 0x40) && (!(rtp->m)) && 
								((fu_header & 0x3f) == 39 ||(fu_header & 0x3f) == 40)) // SEI end bit
						{
							/* FU header byte: Last one of the fragmentations */
							sei_meta_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
							if (sei_meta_frame== NULL)
							{
								MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
										__FUNCTION__,
										lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

								lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
								stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
								if (src == NULL) { return RTN_FAIL; }
								if (src->cmd_callback == NULL) { return RTN_FAIL; }
								src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
								return RTN_FAIL;
							}

							memset(sei_meta_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
							sei_meta_frame->len = video_len;

							sei_meta_frame->frame_type = FTYPE_VA_META;

							len = _mrtpsrc_assemble_vframe(lst, sei_meta_frame, consumed_bytes);
							if (len == RTN_FAIL)
							{
								//MRTPSRC_DBG(WARN, "%s | _mrtpsrc_assemble_vframe fail(FU_AB)\n", __FUNCTION__);
								return consumed_bytes;
							}
							consumed_bytes += len;
							video_len = 0;
						}

						if (((fu_header & 0x40) == 0x40) && rtp->m)
						{
							/* FU header byte: Last one of the fragmentations */
							new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
							if (new_frame == NULL)
							{
								MRTPSRC_DBG(WARN, "%s | memory fail to alloc. Reconnect(%d, %s)\n",
										__FUNCTION__,
										lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

								lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
								stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
								if (src == NULL) { return RTN_FAIL; }
								if (src->cmd_callback == NULL) { return RTN_FAIL; }
								src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
								return RTN_FAIL;
							}

							memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
							new_frame->len = video_len;

							//int nalt = (fu_header & 0x3f);

							if (nalt >= 19 && nalt <= 20)
							{
								new_frame->frame_type = FTYPE_IDR;
							}
							else if (nalt == 39 || nalt == 40)
							{
								new_frame->frame_type = check_sei;
							}
							else 
							{
								new_frame->frame_type = FTYPE_N_IDR;
							}

							{
								new_frame->motion_raw.ch = lst->ch_num;
								new_frame->motion_flag = rtn_hx.mflag;
								new_frame->alarm_flag = rtn_hx.aflag;
								if (rtn_hx.mrd_len > 0)
								{
									new_frame->motion_raw.stream_num = lst->stream_num;
									new_frame->motion_raw.width = rtn_hx.mrd_width;
									new_frame->motion_raw.height = rtn_hx.mrd_height;
									memcpy(new_frame->motion_raw.mraw, rtn_hx.mraw, rtn_hx.mrd_len);
								}
							}

							len = _mrtpsrc_assemble_vframe(lst, new_frame, consumed_bytes);
							if (len == RTN_FAIL)
							{
								MRTPSRC_DBG(ERROR, "%s %d | (%d, %s)_mrtpsrc_assemble_vframe fail(FU_AB)\n", __FUNCTION__, __LINE__,
										lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
								return consumed_bytes;
							}
							consumed_bytes += len;
							video_len = 0;
						}

						break;
					}
				default:
					return RTN_FAIL;
			}
		}
	}

	if (consumed_bytes == RTN_FAIL)
	{
		MRTPSRC_DBG(ERROR, "%s | consumed length == %d\n", __FUNCTION__, consumed_bytes);
	}

	return consumed_bytes;
}


static gint _mrtpsrc_assemble_vframe(MRTPSRC_STREAM_T* lst, MRTPSRC_FRAME_T new_frame, gint offset)
{
	const guint sync_bytes = htonl(1);
	int i;

	guchar spspps_added = 0;

	MRTPSRC_RTSP_IF_T* rtsp = NULL;
	MRTPSRC_RTP_HEADER_T* rtp = NULL;
	MRTPSRC_RTP_HEADER_X_T* rtpx = NULL;

	guchar* r_ptr = NULL;
	guchar* wv_ptr = NULL;
	guchar* sv_ptr = NULL;
	guchar* payload = NULL;
	guchar* align_cp = NULL;

	gint rtn = 0;
	gint consumed_bytes = 0;

	guchar nal_header = 0;
	guchar fu_header = 0;

	guint16 hx_len = 0;
	guint16 rtsp_len = 0;
	gint rtp_seq = -1;

	guint frame_type = 0;
	guint check_sei = 0;

	DWORD pdwRoundKey[32];
	guchar temp_pd[16];
	guchar pdData[16];

	MRTPSRC_CHANNEL_T *lch = NULL;
	ICODEC_HEADER *icodec_h = NULL;

	MRTPSRC_STREAM_STAT_T *stats = NULL;
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *src = NULL;
#endif

	guchar codec_type = 0;


#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
#ifndef NMZ_STANDLONE_MODE
	src = gst_mrtp_src_get_object();
#endif
	lch = mrtpsrc_get_channel(lst->ch_num);

	/* Error if Program counter reached here without new_frame allocated */
	if (new_frame == NULL)
	{
		MRTPSRC_DBG(ERROR, "%s | Frame not allocated", __FUNCTION__);

		lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
		stats->reconn_mem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
		if (src == NULL) { return RTN_FAIL; }
		if (src->cmd_callback == NULL) { return RTN_FAIL; }
		src->cmd_callback(2, lst->ch_num, 0, 106/*mem fail*/, src->cmd_user_data);
#endif
		return RTN_FAIL;
	}

	/* Error if new_frame->vlen is 0 at here */
	if (new_frame->len == 0)
	{
		MRTPSRC_DBG(ERROR, "%s | Video frame length 0", __FUNCTION__);
		lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
		stats->reconn_cmem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
		if (src == NULL)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			return RTN_FAIL;
		}
		if (src->cmd_callback == NULL)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			return RTN_FAIL;
		}
		src->cmd_callback(2, lst->ch_num, 0, 108/*vlen zero*/, src->cmd_user_data);
#endif
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
		return RTN_FAIL;
	}

	
	//if (new_frame->frame_type == FTYPE_IDR && new_frame->len+sizeof(ICODEC_HEADER) <= MRTPSRC_MIN_IDR_ALIGN) /* I frame MRTPSRC_MIN_IDR_ALIGN align */
	if(0)
	{
		int size_frame1 = MRTPSRC_MIN_IDR_ALIGN;
		new_frame->frame = mrtpsrc_alloc_cmem(size_frame1, &new_frame->icodec_h.chan,
							__FILE__, __FUNCTION__, __LINE__);
		if (new_frame->frame)
		{
			memset(gobj_buddy_buffer_buf_get_addr(new_frame->frame), 0x00, size_frame1);
		}

		new_frame->len = MRTPSRC_MIN_IDR_ALIGN - sizeof(ICODEC_HEADER);
	}
	else /* P frame 64 align */
	{
		int size_frame = new_frame->len + sizeof(ICODEC_HEADER);
		int remain = size_frame % 64;
		if (remain != 0)
		{
			size_frame += (64 - remain);
		}

		new_frame->frame = mrtpsrc_alloc_cmem(size_frame, &new_frame->icodec_h.chan,
							__FILE__, __FUNCTION__, __LINE__);
		if (new_frame->frame)
		{
			memset(gobj_buddy_buffer_buf_get_addr(new_frame->frame), 0x00, size_frame);
		}
	}
	if (new_frame->frame == NULL)
	{
		MRTPSRC_DBG(WARN, "%s | cmem fail to alloc. Reconnect(%d, %s)\n",
				__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
		//lch->state = STATE_RECONN_REQ;
		lch->state = STATE_REBOOT_REQ;
#if MRTPSRC_USE_STATISTICS
		stats->reconn_cmem_fail[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
		if (src == NULL)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			return RTN_FAIL;
		}
		if (src->cmd_callback == NULL)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			return RTN_FAIL;
		}
		src->cmd_callback(2, lst->ch_num, 0, 107/*cmem full*/, src->cmd_user_data);
#endif
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
		return RTN_FAIL;
	}

	wv_ptr = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
	icodec_h = (ICODEC_HEADER*) wv_ptr;
	wv_ptr += sizeof(ICODEC_HEADER);
	sv_ptr = wv_ptr;

	new_frame->icodec_h.chan = ((lst->stream_num*32) + lst->ch_num);
	//new_frame->icodec_h.codec = NF_CODEC_TYPE_H264;
	if(lst->en == RTP_PT_H264){
		codec_type = NF_CODEC_TYPE_H264MP;
		//printf("\e[33m [%d][%d] sjlim87 stream_num(%d) encoding(%d) \e[0m\n", __func__, __LINE__, lst->stream_num, lst->en);
	}
	else{
		codec_type =  NF_CODEC_TYPE_H265;
		//printf("\e[33m [%d][%d] sjlim87 stream_num(%d) encoding(%d) \e[0m\n", __func__, __LINE__, lst->stream_num, lst->en);
	}

	new_frame->icodec_h.codec = codec_type;
	new_frame->icodec_h.flags = 1;
	new_frame->icodec_h.version = 1;
	new_frame->icodec_h.frame_size = new_frame->len;
	new_frame->icodec_h.frame_type= new_frame->frame_type;
	new_frame->icodec_h.resolution = lst->resolution;
	new_frame->icodec_h.frame_rate = NF_FPS_CR32;
	new_frame->icodec_h.gst_buffer = new_frame->frame;

	/* Build video frame */
	r_ptr = lst->rbuf.cp + offset;
	while(MRTPSRC_ALWAYS)
	{
		gint rtsp_msg_len = 0;
		rtsp = (MRTPSRC_RTSP_IF_T*) r_ptr;

		if (rtsp->magic != 0x24)
		{
			rtsp_msg_len = mrtpsrc_get_rtsp_msg_len(r_ptr, lst->rbuf.lp - r_ptr);
			if (rtsp_msg_len == 0)
			{
				if (consumed_bytes == 0)
				{
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					return RTN_FAIL;
				}
				else
				{
					return consumed_bytes;
				}
			}

			r_ptr += rtsp_msg_len;
			consumed_bytes += rtsp_msg_len;
			continue;
		}

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		/* RTCP packet */
		if ((rtsp->channel%2) != 0)
		{
			MRTPSRC_RTCP_HEADER_T *rtcp = (MRTPSRC_RTCP_HEADER_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T));
			lst->itlv_rtcp_ch = rtsp->channel;
			if (rtcp->pt != 200) // if no sender report, skip.
			{
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				continue;
			}
			lst->ts_last_sr = ((ntohl(rtcp->ts_msw)&0xffff)<<16)|((ntohl(rtcp->ts_lsw)&0xffff0000)>>16);
			_mrtpsrc_send_rtcp(lst);

			r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;
		}

		/* Video packet */
		rtp = (MRTPSRC_RTP_HEADER_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T));

		/* Audio payload */
		if (rtp->pt == 0x8)
		{
			if (lch->audio_qlen + (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len) < 1024*120)
			{
				memcpy(&lch->audio_qbuf[lch->audio_qlen], r_ptr, (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len));
				lch->audio_qlen += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			}
			r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;
		}

		if(lst->sr_ssrc != ntohl(rtp->ssrc) && lst->sr_ssrc != 0)
		{
			MRTPSRC_DBG(WARN, "%s %d | ch_s(%d, %s) lst:%p - rtp ssrc is mismatched.(now:%0X, ori:%0X) - Request reconnection\n",
					__FUNCTION__, __LINE__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst, ntohl(rtp->ssrc), lst->sr_ssrc);
			lch->state = STATE_RECONN_REQ;

			mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			
			return RTN_FAIL;
		}

        if (rtp_seq == -1)
        {
            rtp_seq = (gint)(ntohs(rtp->seq));
        }
        else
        {
            gint seq_exp = (rtp_seq + 1) & 0xffff;
            gint seq_cur = (gint)(ntohs(rtp->seq)) & 0xffff;

            rtp_seq = seq_cur;
            if (seq_exp != seq_cur)
            {
				MRTPSRC_DBG(WARN, "%s | ch_s(%d, %s) - rtp seq jump is detected.(e:%d, c:%d)\n",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], seq_exp, seq_cur);
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				continue;
            }
        }

		if (rtp->x)
		{
			rtpx = (MRTPSRC_RTP_HEADER_X_T*) (r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T));
		}

		payload = r_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T);
		if (rtp->x)
		{
			align_cp = (guchar*) &rtpx->xlen;
			hx_len = *(align_cp + 1) | *align_cp << 8;	// ntohs(hx_len)
			payload = payload + sizeof(MRTPSRC_RTP_HEADER_X_T) + (hx_len * 4);
		}
		if (rtp->cc)
		{
			payload += (rtp->cc * 4);
		}

		if(lst->en == RTP_PT_H264)
		{
		nal_header = *payload++;
		switch (nal_header & MRTPSRC_NAL_UNIT_MASK)
		{
			case RTP_SINGLE_NAL_IDR:
			{
#ifdef MRTPSRC_ADD_SPSPPS
				/* Sequence parameter set */
				memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
				wv_ptr += sizeof(sync_bytes);
				memcpy(wv_ptr, lst->sps, lst->sps_len);
				wv_ptr += lst->sps_len;

				/* Picture parameter set */
				memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
				wv_ptr += sizeof(sync_bytes);
				memcpy(wv_ptr, lst->pps, lst->pps_len);
				wv_ptr += lst->pps_len;
#endif

				//MRTPSRC_DBG(ERROR, "%s | jykim SINGLE-NAL N_IDR CH(%d) STREAM(%s)\n", __FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
				memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
				wv_ptr += sizeof(sync_bytes);

				*wv_ptr++ = nal_header;
#if 1
				if (rtsp_len - (sizeof(MRTPSRC_RTP_HEADER_T)+(hx_len*4)) > 16)
				{
					if (lst->seed_video)
					{
						//guint pdwRoundKey[32];
						//guchar temp_pd[16];
						//MRTPSRC_DBG(MAJOR, "%s | Seed Single-NAL", __FUNCTION__);
						memcpy(&temp_pd[0], payload, 16);
						SeedRoundKey(pdwRoundKey, lst->pbUserKey);
						SeedDecrypt(&temp_pd[0], pdwRoundKey);
						memcpy(payload, temp_pd, 16);
					}
				}
#endif
				gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
				if (payload_size <= 0)
				{
					MRTPSRC_DBG(WARN, "%s | ch(%d,%s) Single-NAL invalid payload_size(%d) seq(%u)",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					return consumed_bytes;
				}
				if(sv_ptr + new_frame->len < wv_ptr + payload_size)
				{
					fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					return RTN_FAIL;
				}
				memcpy(wv_ptr, payload, payload_size);
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				if (lst->state == STATE_STREAM_READY)
				{
					mrtpsrc_ho_control_frame(lst, 0);
					lst->state = STATE_PLAYING;
				}

				rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

				return consumed_bytes;
			}
			case RTP_SINGLE_NAL_N_IDR:
			{
				//MRTPSRC_DBG(ERROR, "%s | jykim SINGLE-NAL N_IDR CH(%d) STREAM(%s)\n", __FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
				memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
				wv_ptr += sizeof(sync_bytes);

				*wv_ptr++ = nal_header;
#if 1
				if (rtsp_len - (sizeof(MRTPSRC_RTP_HEADER_T)+(hx_len*4)) > 16)
				{
					if (lst->seed_video)
					{
						//guint pdwRoundKey[32];
						//guchar temp_pd[16];
						//MRTPSRC_DBG(MAJOR, "%s | Seed Single-NAL p", __FUNCTION__);
						memcpy(&temp_pd[0], payload, 16);
						SeedRoundKey(pdwRoundKey, lst->pbUserKey);
						SeedDecrypt(&temp_pd[0], pdwRoundKey);
						memcpy(payload, temp_pd, 16);
					}
				}
#endif
				gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
				if (payload_size <= 0)
				{
					MRTPSRC_DBG(WARN, "%s | ch(%d,%s) Single-NAL(N) invalid payload_size(%d) seq(%u)",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					return consumed_bytes;
				}
				if(sv_ptr + new_frame->len < wv_ptr + payload_size)
				{
					fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					return RTN_FAIL;
				}
				memcpy(wv_ptr, payload, payload_size);

				if(sv_ptr + new_frame->len < wv_ptr + payload_size)
				{
					fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
				}
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

				if (lst->state == STATE_STREAM_READY)
				{
					MRTPSRC_DBG(WARN, "%s | ch(%d:%d) first frame is not IDR(%d)", __FUNCTION__, lst->ch_num, lst->stream_num, nal_header & MRTPSRC_NAL_UNIT_MASK);
					lst->state = STATE_PLAYING;
				}

				return consumed_bytes;
			}
			case RTP_SPS:
			{
#if 0
				int header_len = 0;
				int width = 0;
				int height = 0;
				int sps_len;
				guchar sps[512];

				payload -= 1;
				header_len = payload - r_ptr;
				sps_len = rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
				memset(sps, 0x00, 512);
				memcpy(sps, payload, sps_len);

				if (lst->seed_sps)
				{
					int index = 0;
					int loop_cnt = (sps_len-1) / 16;
					//guint pdwRoundKey[32];
					//guchar pdData[16];
					for (index = 0; index < loop_cnt; index++)
					{
						memcpy(pdData, &sps[index*16+1], 16);
						SeedRoundKey(pdwRoundKey, lst->pbUserKey);
						SeedDecrypt(&pdData[0], pdwRoundKey);
						memcpy(&sps[index*16+1], pdData, 16);
					}
				}

				if (lst->model == MODEL_ONVIF_GENERAL)
				{
					if (lst->sps != NULL)
					{
						free(lst->sps);
						lst->sps = NULL;
					}
				}
				if (lst->sps == NULL)
				{
					lst->sps = (guchar*) malloc(sps_len);
					if (lst->sps == NULL) { break; }
					memcpy(lst->sps, sps, sps_len);
				}
				payload++;
				//_mrtpsrc_get_resolution_from_sps(payload, &width, &height);
				_mrtpsrc_get_resolution_from_sps(&sps[1], &width, &height);
				_mrtpsrc_set_stream_resolution(lst, width, height);

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
#else
				int header_len = 0;
				int width = 0;
				int height = 0;
				if (lst->sps[0] != '\0')
				{
					if (lst->model != MODEL_ONVIF_GRUNDIG)// && lst->model != MODEL_TI_365)
					{
						memset(lst->sps, 0x00, 128);
					}
					else
					{
						_mrtpsrc_get_resolution_from_sps(&lst->sps[1], &width, &height);
						_mrtpsrc_set_stream_resolution(lst, width, height);

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				}
				payload -= 1;
				header_len = payload - r_ptr;
				lst->sps_len = rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
				memset(lst->sps, 0x00, 128);
				memcpy(lst->sps, payload, lst->sps_len);

				if (lst->seed_sps)
				{
					int index = 0;
					int loop_cnt = (lst->sps_len-1) / 16;
					//guint pdwRoundKey[32];
					//guchar pdData[16];
					for (index = 0; index < loop_cnt; index++)
					{
						memcpy(pdData, &lst->sps[index*16+1], 16);
						SeedRoundKey(pdwRoundKey, lst->pbUserKey);
						SeedDecrypt(&pdData[0], pdwRoundKey);
						memcpy(&lst->sps[index*16+1], pdData, 16);
					}
				}
				payload++;
				_mrtpsrc_get_resolution_from_sps(&lst->sps[1], &width, &height);
				_mrtpsrc_set_stream_resolution(lst, width, height);

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
#endif
			}
			case RTP_PPS:
			{
				// TODO. PPS nal processing
				//MRTPSRC_DBG(WARN, "%s | Single-NAL type(Picture Parameter Set)", __FUNCTION__);
				int header_len = 0;
				if (lst->pps[0] != '\0')
				{
					if (lst->model != MODEL_ONVIF_GRUNDIG)// && lst->model != MODEL_TI_365)
					{
						memset(lst->pps, 0x00, 128);
					}
					else
					{
						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				}
				payload -= 1;
				header_len = payload - r_ptr;
				lst->pps_len = rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
				memset(lst->pps, 0x00, 128);
				memcpy(lst->pps, payload, lst->pps_len);

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
			}
			case RTP_MTAP_16:
			case RTP_MTAP_24:
			{
				MRTPSRC_DBG(ERROR, "%s | Unhandled RTP type(%d)",
						__FUNCTION__, (*payload & MRTPSRC_NAL_UNIT_MASK));
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return consumed_bytes;
			}
			case RTP_STAP_B:
				payload += 2;
			case RTP_STAP_A:
			{
#if 0
				while(MRTPSRC_ALWAYS)
				{
					int nal_length;
					if (payload - r_ptr >= (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len))
					{
						break;
					}

					nal_length = (*payload) << 8 | *(payload+1);

					memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
					wv_ptr += sizeof(sync_bytes);
					memcpy(wv_ptr, payload + sizeof(guint16), nal_length);
					wv_ptr += nal_length;
					payload += (sizeof(guint16) + nal_length);

					if ((*(payload + sizeof(guint16)) & MRTPSRC_NAL_UNIT_MASK) == RTP_SINGLE_NAL_IDR ||
								(*(payload + sizeof(guint16)) & MRTPSRC_NAL_UNIT_MASK) == RTP_SINGLE_NAL_N_IDR)
					{
						memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
						wv_ptr += sizeof(sync_bytes);
						memcpy(wv_ptr, payload, nal_length);

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

						if (lst->state == STATE_STREAM_READY)
						{
							mrtpsrc_ho_control_frame(lst, 0);
							lst->state = STATE_PLAYING;
						}

						rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

						return consumed_bytes;
					}
				}
#endif

				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				break;
			}
			case RTP_FU_B:
			case RTP_FU_A:
			{
				fu_header = *payload++;

				if ((fu_header & 0x80) == 0x80)
				{
					/* NAL header byte: Start of fragmentation */
					if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x05)
					{
						frame_type = FTYPE_IDR;
					}
					else if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x01)
					{
						frame_type = FTYPE_N_IDR;
					}
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
					else if ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x06)
					{
						//frame_type = FTYPE_VA_META;					
					}
#endif
					else
					{
						MRTPSRC_DBG(WARN, "%s | Unknown type frame, fu_header(%02x)",
								__FUNCTION__, fu_header);
							new_frame->frame_type = FTYPE_VA_SKIP;
					}

#ifdef MRTPSRC_ADD_SPSPPS
					if (lch->model_code >= MODEL_ONVIF_GENERAL)
					{
						if ((frame_type == FTYPE_IDR) && (spspps_added == 0))
						{
							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->sps, lst->sps_len);
							wv_ptr += lst->sps_len;

							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->pps, lst->pps_len);

							wv_ptr += lst->pps_len;
							spspps_added = 1;
						}
					}
					else
					{
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
						if((fu_header & MRTPSRC_NAL_UNIT_MASK) != 0x06)
						{
							
							guchar* seq;
							guint sei_size=0;
							
							if(lst->stream_num == STREAM_1ST)
							{
								if(lst->sei_meta_1st!= NULL)
								{
									MRTPSRC_FRAME_T sei = lst->sei_meta_1st;
									
									seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
									sei_size = sei->len;
									
									memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						
									
									wv_ptr += sei_size;	

									mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
									mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
									lst->sei_meta_1st = NULL;
								}

								if(lst->sei_event_1st!= NULL)
								{
									MRTPSRC_FRAME_T sei = lst->sei_event_1st;
							
									seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
									sei_size = sei->len;
									
									memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						
									
									wv_ptr += sei_size;	

									mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
									mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
									lst->sei_event_1st = NULL;

								}

							}
							else{
								if(lst->sei_meta_2nd!= NULL){
									MRTPSRC_FRAME_T sei = lst->sei_meta_2nd;
									
									seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
									sei_size = sei->len;
									
									memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						
									
								wv_ptr += sei_size;	

									mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
									mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
									lst->sei_meta_2nd = NULL;

								}
								if(lst->sei_event_2nd!= NULL){
									MRTPSRC_FRAME_T sei = lst->sei_event_2nd;
									
									seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
									sei_size = sei->len;
									
									memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						
									
									wv_ptr += sei_size;	

									mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
									mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
									lst->sei_event_2nd = NULL;

								}

							}

						}
						
#endif

						if (frame_type == FTYPE_IDR)
						{
							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->sps, lst->sps_len);
							wv_ptr += lst->sps_len;

							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->pps, lst->pps_len);
							wv_ptr += lst->pps_len;

							if(lst->sei_len > 0)
							{
								memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
								wv_ptr += sizeof(sync_bytes);
								memcpy(wv_ptr, lst->sei, lst->sei_len);
								wv_ptr += lst->sei_len;
							}
						}
					}
#endif

					if (lst->seed_video && (frame_type == FTYPE_IDR || frame_type == FTYPE_N_IDR))
					{
						//guint pdwRoundKey[32];
						//guchar temp_pd[16];

						memcpy(&temp_pd[0], payload, 16);
						SeedRoundKey(pdwRoundKey, lst->pbUserKey);
						SeedDecrypt(&temp_pd[0], pdwRoundKey);
						memcpy(payload, temp_pd, 16);
					}

					memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
					wv_ptr += sizeof(sync_bytes);
					*wv_ptr++ = (nal_header & 0xe0) | (fu_header & 0x1f);
					//spspps_added = 1;
				}

				{
					gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
					if (payload_size <= 0)
					{
						MRTPSRC_DBG(WARN, "%s | ch(%d,%s) FU-A invalid payload_size(%d) seq(%u)", __FUNCTION__,
								lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
						return consumed_bytes;
					}

					if(sv_ptr + new_frame->len < wv_ptr + payload_size)
					{
						fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow frame_len[%u] payload_size[%d]\n",
								__FUNCTION__, __LINE__, new_frame->len, payload_size);
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
						return RTN_FAIL;
					}

					memcpy(wv_ptr, payload, payload_size);
					wv_ptr += payload_size;
					r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				}

				//FU_A SEI EXCEPTION CODE 
				if (((fu_header & 0x40) == 0x40 ) && (!(rtp->m)) && ((fu_header & MRTPSRC_NAL_UNIT_MASK) == 0x06)
						&& new_frame->frame_type == FTYPE_VA_META )
				{
					if (lst->state == STATE_STREAM_READY)
					{
						mrtpsrc_ho_control_frame(lst, 0);
						lst->state = STATE_PLAYING;
					}

					rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

					return consumed_bytes;
				}

				if (((fu_header & 0x40) == 0x40) && rtp->m)
				{
					if (lst->state == STATE_STREAM_READY)
					{
						mrtpsrc_ho_control_frame(lst, 0);
						lst->state = STATE_PLAYING;
					}

					rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

					return consumed_bytes;
				}

				break;
			}
			case RTP_SEI:
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
			{
				guchar *uuid_ptr;
				int header_len = 0;
		
				uuid_ptr = memmem(r_ptr, rtsp_len, itx_fisheye_info, 16);
				if(uuid_ptr != NULL)
				{
					memset(lst->sei, 0x00, 128);
					payload -= 1;
					header_len = payload - r_ptr;
					lst->sei_len = rtsp_len - header_len + sizeof(MRTPSRC_RTSP_IF_T);
					memcpy(lst->sei, payload, lst->sei_len);

					r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

					break;
				}
		
				check_sei = 0;

				uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_meta, 16);

				if(uuid_ptr != NULL){
					check_sei = FTYPE_VA_META;
				}
				else{
					uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_event, 16);
					if(uuid_ptr != NULL){
						check_sei = FTYPE_VA_EVENT;
					}
				}

				if (uuid_ptr == NULL)
				{
					r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
					break;
				}
#if 1
				memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
				wv_ptr += sizeof(sync_bytes);

				*wv_ptr++ = nal_header;
				{
					gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
					if (payload_size <= 0)
					{
						MRTPSRC_DBG(WARN, "%s | ch(%d,%s) SEI(single) invalid payload_size(%d) seq(%u)",
							__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
						return consumed_bytes;
					}
					memcpy(wv_ptr, payload, payload_size);

					if(sv_ptr + new_frame->len < wv_ptr + payload_size)
					{
						fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
						mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
						return RTN_FAIL;
					}
				}
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

				rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

				if (lst->state == STATE_STREAM_READY && rtn != RTN_FAIL)
				{
					MRTPSRC_DBG(WARN, "%s | ch(%d:%d) first frame is not IDR(%d)", __FUNCTION__, lst->ch_num, lst->stream_num, nal_header & MRTPSRC_NAL_UNIT_MASK);
					lst->state = STATE_PLAYING;
				}

				return consumed_bytes;
#else

				{
					int size_frame = 0;
					int remain = 0;
					int sei_len = 0;
					MRTPSRC_FRAME_T sei_frame = NULL;
					guchar* ws_ptr = NULL;
					ICODEC_HEADER* sei_icodec_h = NULL;

					//sei_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
					sei_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
					if (sei_frame == NULL) 
					{
						lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
						stats->reconn_mem_fail[lst->ch_num]++;
#endif
						return RTN_FAIL;
					}
					memset(sei_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
					sei_len = sizeof(sync_bytes) + sizeof(guchar) +
						(rtsp_len - (payload - r_ptr)) + sizeof(MRTPSRC_RTSP_IF_T);
					sei_frame->len = sei_len;
					sei_frame->frame_type = check_sei;
					
					//MRTPSRC_DBG(WARN, "%s | sei frame len %d type %d ", __FUNCTION__,sei_frame->len,sei_frame->frame_type );
					
					size_frame = sei_frame->len + sizeof(ICODEC_HEADER);
					remain = size_frame % 64;
					if (remain != 0)
					{
						size_frame += (64 - remain);
					}

					//sei_frame->frame = (GstBuffer*) gst_nf_buddy_buffer_new_and_alloc(
					//					size_frame, (void*)&sei_frame->icodec_h.chan);
					sei_frame->frame = mrtpsrc_alloc_cmem(size_frame,&sei_frame->icodec_h.chan,
										__FILE__, __FUNCTION__, __LINE__);
					if (sei_frame->frame == NULL)
					{
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei_frame);
						sei_frame = NULL;
						lch->state = STATE_REBOOT_REQ;
#if MRTPSRC_USE_STATISTICS
						stats->reconn_cmem_fail[lst->ch_num]++;
#endif
						return RTN_FAIL;
					}

					memset(gobj_buddy_buffer_buf_get_addr(sei_frame->frame), 0x00, size_frame);

					ws_ptr = gobj_buddy_buffer_buf_get_addr(sei_frame->frame);
					sei_icodec_h = (ICODEC_HEADER*) ws_ptr;
					ws_ptr += sizeof(ICODEC_HEADER);
					sei_frame->icodec_h.chan = ((lst->stream_num*32) + lst->ch_num);
					sei_frame->icodec_h.codec = 6;
					sei_frame->icodec_h.flags = 1;
					sei_frame->icodec_h.version = 1;
					sei_frame->icodec_h.frame_size = sei_frame->len;
					sei_frame->icodec_h.frame_type= sei_frame->frame_type;
					sei_frame->icodec_h.resolution = lst->resolution;
					sei_frame->icodec_h.frame_rate = NF_FPS_CR32;
					sei_frame->icodec_h.gst_buffer = sei_frame->frame;

					memcpy(ws_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
					ws_ptr += sizeof(sync_bytes);
					*ws_ptr++ = nal_header;
					//memcpy(sei_icodec_h, &sei_frame->icodec_h, sizeof(ICODEC_HEADER));
					{
						gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
						if (payload_size <= 0)
						{
							MRTPSRC_DBG(WARN, "%s | ch(%d,%s) SEI invalid payload_size(%d) seq(%u)",
								__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
							mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei_frame->frame);
							mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei_frame);
							return consumed_bytes;
						}
						memcpy(ws_ptr, payload, payload_size);
					}
					rtn = _mrtpsrc_pad_vliveq(lst, sei_frame, rtp);
					//g_object_unref(sei_frame->frame);
					//free(sei_frame);
					
				}
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
#endif

			}
#endif
			case RTP_CSD_A:
			case RTP_CSD_B:
			case RTP_CSD_C:
			case RTP_AUD:
			case RTP_EO_SEQ:
			case RTP_EO_STREAM:
			case RTP_FILTER_DATA:
			case RTP_SPS_EXT:
			{
#if 0
				MRTPSRC_DBG(ERROR, "%s | Unhandled RTP format(%d)",
						__FUNCTION__, (nal_header & MRTPSRC_NAL_UNIT_MASK));
#endif
				r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
				break;
			}
			case RTP_UNDEFINED_T0:
			case RTP_UNDEFINED_T30:
			case RTP_UNDEFINED_T31:
			{
				MRTPSRC_DBG(ERROR, "%s | Undefined RTP format(%d)",
						__FUNCTION__, (nal_header & MRTPSRC_NAL_UNIT_MASK));
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return RTN_FAIL;
			}
			default:
			{
				MRTPSRC_DBG(WARN, "%s | Unknown type frame(%d)",
						__FUNCTION__, (nal_header & MRTPSRC_NAL_UNIT_MASK));
				break;
			}
		}
		}
		else if(lst->en == RTP_PT_H265)
		{
			uint8_t* nal_h;
			nal_h = payload;
			gint nal_type = -1;
			gint tid = -1;
			gint lid = -1;


			/*    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 
			 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *   |F|   Type    |  LayerId  | TID |
			 *   +-------------+-----------------+ */
			nal_type = (payload[0] >>1) & 0x3f;
			lid =  ((payload[0] << 5) & 0x20) | ((payload[1] >> 3) & 0x1f);
			tid = (payload[1] & 0x07);

			payload += 2;

			switch(nal_type)
			{
				case NAL_TRAIL_N:
				case NAL_TRAIL_R:
				case NAL_TSA_N:
				case NAL_TSA_R:
				case NAL_STSA_N:
				case NAL_STSA_R:
				case NAL_RADL_N:
				case NAL_RADL_R:
				case NAL_RASL_N:
				case NAL_BLA_W_LP:
				case NAL_BLA_W_RADL:
				case NAL_BLA_N_LP:
				case NAL_IDR_N_LP:
				case NAL_CRA_NUT:
					{
						memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
						wv_ptr += sizeof(sync_bytes);

						*wv_ptr++ = nal_h[0];
						*wv_ptr++ = nal_h[1];
#if 1
						if (rtsp_len - (sizeof(MRTPSRC_RTP_HEADER_T)+(hx_len*4)) > 17)
						{
							if (lst->seed_video)
							{
								memcpy(&temp_pd[0], payload, 16);
								SeedRoundKey(pdwRoundKey, lst->pbUserKey);
								SeedDecrypt(&temp_pd[0], pdwRoundKey);
								memcpy(payload, temp_pd, 16);
							}
						}
#endif
						{
							gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
							if (payload_size <= 0)
							{
								fprintf(stderr, "- ERROR - [%s:%d] invalid payload_size(%d)\n", __FUNCTION__, __LINE__, payload_size);
								mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
								mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
								return consumed_bytes;
							}
							if(sv_ptr + new_frame->len < wv_ptr + payload_size)
							{
								fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow frame_type[%d] frame_len[%d] used_buffer[%d] payload_size[%d]\n", __FUNCTION__, __LINE__,  frame_type, new_frame->len, (unsigned int)(wv_ptr - sv_ptr), payload_size);
								mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
								mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
								return RTN_FAIL;
							}
							memcpy(wv_ptr, payload, payload_size);
							wv_ptr += payload_size;
						}

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

                        if(rtp->m)
                        {
						rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

						if (lst->state == STATE_STREAM_READY)
						{
							MRTPSRC_DBG(WARN, "%s | ch(%d:%d) first frame is not IDR (%d)", __FUNCTION__, lst->ch_num, lst->stream_num, nal_type);
							lst->state = STATE_PLAYING;
						}

						return consumed_bytes;
					}
                        break;
					}
				case NAL_IDR_W_RADL:
					{
#ifdef MRTPSRC_ADD_SPSPPS
						if(lst->sps_added == 1){
							/* Video parameter set */
							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->vps, lst->vps_len);
							wv_ptr += lst->vps_len;

							/* Sequence parameter set */
							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->sps, lst->sps_len);
							wv_ptr += lst->sps_len;

							/* Picture parameter set */
							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							memcpy(wv_ptr, lst->pps, lst->pps_len);
							wv_ptr += lst->pps_len;

							if(lst->sei_len > 0)
							{
								memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
								wv_ptr += sizeof(sync_bytes);
								memcpy(wv_ptr, lst->sei, lst->sei_len);
								wv_ptr += lst->sei_len;
							}
							lst->sps_added =0;
						}
#endif

						memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
						wv_ptr += sizeof(sync_bytes);

						*wv_ptr++ = nal_h[0];
						*wv_ptr++ = nal_h[1];
#if 1
						if (rtsp_len - (sizeof(MRTPSRC_RTP_HEADER_T)+(hx_len*4)) > 17)
						{
							if (lst->seed_video)
							{
								memcpy(&temp_pd[0], payload, 16);
								SeedRoundKey(pdwRoundKey, lst->pbUserKey);
								SeedDecrypt(&temp_pd[0], pdwRoundKey);
								memcpy(payload, temp_pd, 16);
							}
						}
#endif
						{
							gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
							if (payload_size <= 0)
							{
								MRTPSRC_DBG(WARN, "%s | ch(%d,%s) H265 single invalid payload_size(%d) seq(%u)",
										__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
								mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
								mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
								return consumed_bytes;
							}
							if(sv_ptr + new_frame->len < wv_ptr + payload_size)
							{
								fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow\n", __FUNCTION__, __LINE__);
								mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
								mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
								return RTN_FAIL;
							}
							memcpy(wv_ptr, payload, payload_size);
							wv_ptr += payload_size;
						}

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

                        if(rtp->m == 0){
                            continue;
                        }

						if (lst->state == STATE_STREAM_READY)
						{
							mrtpsrc_ho_control_frame(lst, 0);
							lst->state = STATE_PLAYING;
						}

						rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

						return consumed_bytes;
					}
				case NAL_VPS:
				/* Video Parameter Set */
					{
						int header_len = 0;
						int width = 0;
						int height = 0;
						if (lst->vps[0] != '\0')
						{
							if (lst->model != MODEL_ONVIF_GRUNDIG)// && lst->model != MODEL_TI_365)
							{
								memset(lst->vps, 0x00, 128);
							}
							else
							{
								r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
								consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
								break;
							}
						}

						payload -= 2;
						header_len = payload - r_ptr;
						lst->vps_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);

						memset(lst->vps, 0x00, 128);
						memcpy(lst->vps, payload, lst->vps_len);

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				case NAL_SPS:
				/* Squence Parameter Set */
					{
						int header_len = 0;
						int width = 0;
						int height = 0;
						if (lst->sps[0] != '\0')
						{
							if (lst->model != MODEL_ONVIF_GRUNDIG)// && lst->model != MODEL_TI_365)
							{
								memset(lst->sps, 0x00, 128);
							}
							else
							{
                                h265_get_resolution(lst->sps, lst->sps_len, &width, &height);
								_mrtpsrc_set_stream_resolution(lst, width, height);

								r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
								consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
								break;
							}
						}
						payload -= 2;
						header_len = payload - r_ptr;
						lst->sps_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);
						memset(lst->sps, 0x00, 128);
						memcpy(lst->sps, payload, lst->sps_len);

#if SPS_PPS_DESC_DEBUG
						if(lst->ch_num == 0 && lst->stream_num == 0);
						{
							printf("sps_len(%d) 1\n", lst->sps_len);
							for(i = 0; i < lst->sps_len; i++)
							{
								printf("0x%02X ", lst->sps[i]);
							}
							puts("");
						}
#endif
						if (lst->seed_sps)
						{
							int index = 0;
							int loop_cnt = (lst->sps_len-2) / 16;
							for (index = 0; index < loop_cnt; index++)
							{
								memcpy(pdData, &lst->sps[index*16+2], 16);
								SeedRoundKey(pdwRoundKey, lst->pbUserKey);
								SeedDecrypt(&pdData[0], pdwRoundKey);
								memcpy(&lst->sps[index*16+2], pdData, 16);
							}
						}

#if SPS_PPS_DESC_DEBUG
						if(lst->ch_num == 0 && lst->stream_num == 0);
						{
							printf("sps_len(%d) 2\n", lst->sps_len);
							for(i = 0; i < lst->sps_len; i++)
							{
								printf("0x%02X ", lst->sps[i]);
							}
							puts("");
						}
#endif
						payload += 2;
                        h265_get_resolution(lst->sps, lst->sps_len, &width, &height);
						_mrtpsrc_set_stream_resolution(lst, width, height);

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				case NAL_PPS:
				/* Picture Parameter Set */
					{
						int header_len = 0;
						if (lst->pps[0] != '\0')
						{
							if (lst->model != MODEL_ONVIF_GRUNDIG)
							{
								memset(lst->pps, 0x00, 128);
							}
							else
							{
								r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
								consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
								break;
							}
						}
						payload -= 2;
						header_len = payload - r_ptr;
						lst->pps_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);
						memset(lst->pps, 0x00, 128);
						memcpy(lst->pps, payload, lst->pps_len);

#if SPS_PPS_DESC_DEBUG
						if(lst->ch_num == 0 && lst->stream_num == 0);
						{
							printf("pps_len(%d) 1\n", lst->sps_len);
							for(i = 0; i < lst->pps_len; i++)
							{
								printf("0x%02X ", lst->pps[i]);
							}
							puts("");
						}
#endif
						if (lst->seed_pps)
						{
							int index = 0;
							int loop_cnt = (lst->pps_len-2) / 16;
							for (index = 0; index < loop_cnt; index++)
							{
								memcpy(pdData, &lst->pps[index*16+2], 16);
								SeedRoundKey(pdwRoundKey, lst->pbUserKey);
								SeedDecrypt(&pdData[0], pdwRoundKey);
								memcpy(&lst->pps[index*16+2], pdData, 16);
							}
						}
#if SPS_PPS_DESC_DEBUG
						if(lst->ch_num == 0 && lst->stream_num == 0);
						{
							printf("pps_len(%d) 2\n", lst->pps_len);
							for(i = 0; i < lst->pps_len; i++)
							{
								printf("0x%02X ", lst->pps[i]);
							}
							puts("");
						}
#endif
						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						break;
					}
				case NAL_SEI_PREFIX:
				case NAL_SEI_SUFFIX:
					{
						guchar *uuid_ptr;
						payload -= 2;
						check_sei = 0;

						uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_meta, 16);

						if(uuid_ptr != NULL){
							check_sei = FTYPE_VA_META;
						}
						else{
							uuid_ptr = memmem(r_ptr, rtsp_len, itx_uuid_event, 16);
							if(uuid_ptr != NULL){
								check_sei = FTYPE_VA_EVENT;
							}
						}

						if (uuid_ptr == NULL)
						{

							if((rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T)) < 128)
							{
								lst->sei_len = rtsp_len - sizeof(MRTPSRC_RTP_HEADER_T);
								memcpy(lst->sei, payload, lst->sei_len);
							}
							r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
							consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
							break;
						}
						memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
						wv_ptr += sizeof(sync_bytes);

						{
							gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
							if (payload_size <= 0)
							{
								MRTPSRC_DBG(WARN, "%s | ch(%d,%s) SEI(single) invalid payload_size(%d) seq(%u)",
									__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
								mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
								mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
								return consumed_bytes;
							}
							memcpy(wv_ptr, payload, payload_size);
							wv_ptr += payload_size;
						}

						r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);

						rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

						if (lst->state == STATE_STREAM_READY)
						{
							MRTPSRC_DBG(WARN, "%s | ch(%d:%d) first frame is not IDR (%d)", __FUNCTION__,lst->ch_num, lst->stream_num, nal_type);
							lst->state = STATE_PLAYING;
						}

						return consumed_bytes;

					}
				case NAL_FUS:
				/* Fragmentation Units (FUs)*/
					{
						/*
						 *    decode the FU header
						 *
						 *     0 1 2 3 4 5 6 7
						 *    +-+-+-+-+-+-+-+-+
						 *    |S|E|  FuType   |
						 *    +---------------+
						 *
						 *       Start fragment (S): 1 bit
						 *       End fragment (E): 1 bit
						 *       FuType: 6 bits
						 */

						fu_header = *payload++;

						if(fu_header & 0x80) // Start Fragment Check
						{
							int nalt = fu_header & 0x3f;
							/* NAL header byte: Start of fragmentation */
							if (nalt >= 19 && nalt <= 20)
							{
								frame_type = FTYPE_IDR;
							}
							else if (nalt >= 1 && nalt <= 18 || nalt == 21)
							{
								frame_type = FTYPE_N_IDR;
							}
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
							else if (nalt  == 39 || nalt == 40)
							{
								// SEI
								new_frame->frame_type = FTYPE_VA_META;
							}
#endif
							else
							{
								MRTPSRC_DBG(WARN, "%s | Unknown type frame, fu_header(%02x)",
										__FUNCTION__, nal_type);
								new_frame->frame_type = FTYPE_VA_SKIP;
							}


							/* FU header byte: First one of the fragmentations */
                            if(frame_type == FTYPE_IDR)
                            {
#ifdef MRTPSRC_ADD_SPSPPS
							if (lch->model_code >= MODEL_ONVIF_GENERAL)
							{
								if ((frame_type == FTYPE_IDR) && (spspps_added == 0))
								{
									if(lst->vps_len > 0)
									{
										memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
										wv_ptr += sizeof(sync_bytes);
										memcpy(wv_ptr, lst->vps, lst->vps_len);
										wv_ptr += lst->vps_len;
									}
									if(lst->sps_len > 0)
									{
										memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
										wv_ptr += sizeof(sync_bytes);
										memcpy(wv_ptr, lst->sps, lst->sps_len);
										wv_ptr += lst->sps_len;
									}
									if(lst->pps_len > 0)
									{
										memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
										wv_ptr += sizeof(sync_bytes);
										memcpy(wv_ptr, lst->pps, lst->pps_len);
										wv_ptr += lst->pps_len;
									}
									if(lst->sei_len > 0)
									{
										memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
										wv_ptr += sizeof(sync_bytes);
										memcpy(wv_ptr, lst->sei, lst->sei_len);
										wv_ptr += lst->sei_len;
									}
									spspps_added = 1;
								}
							}
							else
							{
								int nalt = fu_header & 0x3f;


#ifdef MRTPSRC_SUPPORT_SMART_CAMERA

								if((nalt != 40 || nalt != 39) && new_frame->frame_type != FTYPE_VA_META)
								{
									guchar* seq;
									guint sei_size=0;

									if(lst->stream_num == STREAM_1ST){
										if(lst->sei_meta_1st!= NULL){
											MRTPSRC_FRAME_T sei = lst->sei_meta_1st;

											seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
											sei_size = sei->len;

											memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						

											wv_ptr += sei_size;	

											mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
											mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
											lst->sei_meta_1st = NULL;


										}
										if(lst->sei_event_1st!= NULL){
											MRTPSRC_FRAME_T sei = lst->sei_event_1st;

											seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
											sei_size = sei->len;

											memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						

											wv_ptr += sei_size;	

											mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
											mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
											lst->sei_event_1st = NULL;

										}

									}
									else{
										if(lst->sei_meta_2nd!= NULL){
											MRTPSRC_FRAME_T sei = lst->sei_meta_2nd;

											seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
											sei_size = sei->len;

											memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						

											wv_ptr += sei_size;	

											mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
											mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
											lst->sei_meta_2nd = NULL;

										}
										if(lst->sei_event_2nd!= NULL){
											MRTPSRC_FRAME_T sei = lst->sei_event_2nd;

											seq = gobj_buddy_buffer_buf_get_addr(sei->frame);
											sei_size = sei->len;

											memcpy(wv_ptr, seq + sizeof(ICODEC_HEADER), sei_size);						

											wv_ptr += sei_size;	

											mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
											mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
											lst->sei_event_2nd = NULL;

										}

									}

								}
#endif

								//if ((frame_type == FTYPE_IDR) && (spspps_added == 0))
								if ((frame_type == FTYPE_IDR) && (lst->sps_added))
								{
									memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
									wv_ptr += sizeof(sync_bytes);
									memcpy(wv_ptr, lst->vps, lst->vps_len);
									wv_ptr += lst->vps_len;

									memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
									wv_ptr += sizeof(sync_bytes);
									memcpy(wv_ptr, lst->sps, lst->sps_len);
									wv_ptr += lst->sps_len;

									memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
									wv_ptr += sizeof(sync_bytes);
									memcpy(wv_ptr, lst->pps, lst->pps_len);
									wv_ptr += lst->pps_len;

									if(lst->sei_len > 0)
									{
										memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
										wv_ptr += sizeof(sync_bytes);
										memcpy(wv_ptr, lst->sei, lst->sei_len);
										wv_ptr += lst->sei_len;
									}

									//initialize
									lst->sps_added = 0;
								}
							}
#endif
							if (lst->seed_video && (frame_type == FTYPE_IDR || frame_type == FTYPE_N_IDR))
							{
								memcpy(&temp_pd[0], payload, 16);
								SeedRoundKey(pdwRoundKey, lst->pbUserKey);
								SeedDecrypt(&temp_pd[0], pdwRoundKey);
								memcpy(payload, temp_pd, 16);
							}
                            }
                            else
                            {
                                //printf("[%s:%d] len : vps[%d] sps[%d] pps[%d]\n", __func__, __LINE__, lst->vps_len, lst->sps_len, lst->pps_len);
                            }

							memcpy(wv_ptr, (guchar*)&sync_bytes, sizeof(sync_bytes));
							wv_ptr += sizeof(sync_bytes);
							*wv_ptr++ = (nal_h[0] & 0x81) | ((fu_header & 0x3f) << 1);
							*wv_ptr++ = nal_h[1];

						}

						{
							gint payload_size = calc_payload_size(rtsp_len, payload, r_ptr);
							if (payload_size <= 0)
							{
								MRTPSRC_DBG(WARN, "%s | ch(%d,%s) H265 FU invalid payload_size(%d) seq(%u)",
										__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], payload_size, ntohs(rtp->seq));
                                mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
                                mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
                                return consumed_bytes;
							}

							if(sv_ptr + new_frame->len < wv_ptr + payload_size)
							{
								fprintf(stderr, "- ERROR - [%s:%d] Cmem Overfllow frame_type[%d] frame_len[%d] used_buffer[%d] payload_size[%d]\n", __FUNCTION__, __LINE__,  frame_type, new_frame->len, (unsigned int)(wv_ptr - sv_ptr), payload_size);
                                mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
                                mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
                                return RTN_FAIL;
							}

							memcpy(wv_ptr, payload, payload_size);
							wv_ptr += payload_size;

							r_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
							consumed_bytes += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
						}

						//FUS SEI EXCEPTION CODE 
						if (((fu_header & 0x40) == 0x40 ) && (!(rtp->m)) 
								&& new_frame->frame_type == FTYPE_VA_META  && ((fu_header & 0x3f) == 39 ||(fu_header & 0x3f) == 40))
						{
							if (lst->state == STATE_STREAM_READY)
							{
								mrtpsrc_ho_control_frame(lst, 0);
								lst->state = STATE_PLAYING;
							}

							rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

							return consumed_bytes;
				}

						if (((fu_header & 0x40) == 0x40) && rtp->m)
						{
							if (lst->state == STATE_STREAM_READY)
							{
								//mrtpsrc_ho_control_frame(lst, 0);
								lst->state = STATE_PLAYING;
							}

#if 0
							// Video Stream File Dump
							if(0)
							{
								FILE *fp = NULL;
								fp = fopen("video.265", "ab");
								fwrite(gobj_buddy_buffer_buf_get_addr(new_frame->frame) + sizeof(ICODEC_HEADER), 1, new_frame->len- sizeof(ICODEC_HEADER), fp); 
								fclose(fp);
							}
#endif
							rtn = _mrtpsrc_pad_vliveq(lst, new_frame, rtp);

							return consumed_bytes;
						}

						break;
					}
			}
		}
	}
}

//static gint shadow_flag[MRTPSRC_MAX_CH] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
static gint _mrtpsrc_pad_vliveq(MRTPSRC_STREAM_T* lst, MRTPSRC_FRAME_T new_frame, MRTPSRC_RTP_HEADER_T* rtp)
{
	//GobjMrtpSrc *src = gst_mrtp_src_get_object();

	guchar* seq = NULL;
	guchar* align_cp = NULL;
	gint rtn = 0;
	guint gop_info = 0;
	guint frame_timestamp = 0;
	ICODEC_HEADER *icodec_h;
	MRTPSRC_STREAM_STAT_T *stats = NULL;
	MRTPSRC_FRAME_T second_frame = NULL;
	MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);

	//printf("\e[33m [%s][%d] sjlim87 stream_num(%d) encoding(%d) \e[0m\n", __func__, __LINE__, lst->stream_num, lst->en);

#if 0
	{
		guchar* scan_ptr = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
		int i=0;
		printf("     0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f");
		while (i < new_frame->len)
		{
			if (i % 16 == 0)
			{
				printf("\n%02x  ", i/16);
			}
			else if (i % 8 == 0)
			{
				printf(" ");
			}
			printf("%02x ", *(scan_ptr+i++));
		}
		printf("\n\n\n");
	}
#endif
#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
	seq = gobj_buddy_buffer_buf_get_addr(new_frame->frame);

	if (new_frame->frame_type == FTYPE_VA_SKIP)
	{
		//MRTPSRC_DBG(WARN, "%s | INIT-P DROP length(%lu)", __FUNCTION__, new_frame->len);
		mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
		return RTN_FAIL;
	}
#if 0
	if(lst->ch_num == 1 && lst->stream_num == 1)
	{
		FILE *fp = NULL;
		fp = fopen("/mnt/sda1/video.265", "ab");
		fwrite(gobj_buddy_buffer_buf_get_addr(new_frame->frame) + sizeof(ICODEC_HEADER), 1, new_frame->len, fp);
		fclose(fp);
	}
#endif

#if 0
	if (lst->model == MODEL_TI_365)
	{
		new_frame->frame_seq = (((*(seq+0x1d) & 0x1)<<3) | (*(seq+0x1e) & 0xe0)>>5);
		if (new_frame->frame_seq == 0)
		{
			if (new_frame->frame_type == FTYPE_N_IDR && lst->gop_seq == 0xf)
			{
				new_frame->frame_seq += 0x10;
			}
			else if (new_frame->frame_type != FTYPE_IDR)
			{
				MRTPSRC_DBG(WARN, "%s | gop sequence error. frame missed type[1](%ld, %s), gop_seq(%lu), last_seq(%lu)", __FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->gop_seq, new_frame->frame_seq);
			}
			else
			{
				lst->gop++;
			}
		}
		else if (new_frame->frame_seq == ((lst->gop_seq + 1) % 0x10))
		{
			new_frame->frame_seq = lst->gop_seq + 1;
		}
		else
		{
			MRTPSRC_DBG(WARN, "%s | gop sequence error. frame missed type[2](%ld, %s), gop_seq(%lu), last_seq(%lu)", __FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->gop_seq, new_frame->frame_seq);
		}
	}
	else if (lst->model == MODEL_AMB_A2 || lst->model == MODEL_AMB_D1)
	{
		seq += MRTPSRC_FRAME_SEQ_POS;
		new_frame->frame_seq = (guint) *seq;
		if (*(seq-3) != 0x21) // if not a P frame
		{
			new_frame->frame_seq = 0;
			lst->gop++;
		}
		else
		{
			new_frame->frame_seq /= 2;
		}
	}
	else 
#endif
	{
		if ((lst->gop == 0) && (new_frame->frame_type == FTYPE_N_IDR))
		{
			//MRTPSRC_DBG(WARN, "%s | INIT-P DROP length(%lu)", __FUNCTION__, new_frame->len);
			mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			return RTN_FAIL;
		}
		/* FIXME. other model doesn't be calculated the gop sequence numbers */
		if (new_frame->frame_type == FTYPE_IDR)
		{
			new_frame->frame_seq = 0;
			lst->gop++;
		}
#if 0
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
		else if(new_frame->frame_type == 7)
		{
			new_frame->frame_seq = lst->gop_seq;
		}
#endif
		else
		{
			if (lst->model == MODEL_TI_365)
			{
				int temp_seq = 0;
				int _rtn = 0;

				_rtn = get_frame_seq_from_slice(seq+sizeof(ICODEC_HEADER), &temp_seq);
				/* non pure P frame */
				if (_rtn == 0)
				{
					temp_seq = (lst->gop_seq+1)%16;
				}
				if (0) {
				if (((lst->gop_seq+1) % 16) != temp_seq)
				{
					g_object_unref(new_frame->frame);
					free(new_frame);
					return RTN_OK;
				}}
			}
			new_frame->frame_seq = lst->gop_seq+1;
		}
#else
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
		else if(new_frame->frame_type == FTYPE_VA_META || new_frame->frame_type == FTYPE_VA_EVENT)
		{
			new_frame->frame_seq = lst->gop_seq;
		}
#endif
		else
		{
			new_frame->frame_seq = lst->gop_seq+1;
		}
#endif
	}

	lst->gop_seq = new_frame->frame_seq;

	align_cp = (guchar*) &rtp->timestamp;
	frame_timestamp = *(align_cp + 3) | *(align_cp + 2) << 8 | *(align_cp + 1) << 16 | *align_cp << 24;
	if(new_frame->frame_type == FTYPE_VA_META || new_frame->frame_type == FTYPE_VA_EVENT){
		ICODEC_HEADER* temp;
		temp = &(new_frame->icodec_h);
		temp->timestampl = lst->ts_last_5_mili;
		temp->timestamp = lst->ts_last_second;
	}
	else{
		rtn = _mrtpsrc_calculate_vts(lst, &new_frame->icodec_h, frame_timestamp);
		if (rtn == RTN_FAIL)
		{
			mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
			return RTN_FAIL;
		}
	}

	if(lst->state == STATE_STREAM_READY &&  new_frame->frame_type != FTYPE_IDR) {
		mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
		return RTN_FAIL;
	}

	seq = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
	icodec_h = (ICODEC_HEADER*) seq;
	memcpy(icodec_h, &new_frame->icodec_h, sizeof(ICODEC_HEADER));

#if 1
	//if (lch->active_video_cnt == 1 && new_frame->frame_type == FTYPE_IDR)
	
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
	if (lch->active_video_cnt == 1 && (new_frame->frame_type != FTYPE_VA_META && new_frame->frame_type != FTYPE_VA_EVENT))
#else
	if (lch->active_video_cnt == 1)
#endif
	{
		guchar *s, *d;

		//if ((get_live_channel_num() < 5) || (new_frame->frame_type == FTYPE_IDR))
		if (new_frame->frame_type == FTYPE_IDR)
		{
			//second_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
			second_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
			if (second_frame == NULL)
			{
				MRTPSRC_DBG(WARN, "%s | alloc heap fail. Reboot request(%d, %s)\n",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
				lch->state = STATE_REBOOT_REQ;
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return RTN_FAIL;
			}
			memcpy(second_frame, new_frame, sizeof(struct _FRAME_INFO_T_));
			memcpy(&second_frame->icodec_h, &new_frame->icodec_h, sizeof(ICODEC_HEADER));
			second_frame->icodec_h.chan = lst->ch_num+32;

			//second_frame->frame = gst_nf_buddy_buffer_new_and_alloc(
			//		GST_BUFFER_SIZE(new_frame->frame), (void*)&second_frame->icodec_h.chan);
			second_frame->frame = mrtpsrc_alloc_cmem(
					gobj_buddy_buffer_buf_get_size(new_frame->frame), &second_frame->icodec_h.chan,
					__FILE__, __FUNCTION__, __LINE__);
			if (second_frame->frame == NULL)
			{
				MRTPSRC_DBG(WARN, "%s | cmem alloc fail. Reboot request(%d, %s)\n",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
				lch->state = STATE_REBOOT_REQ;
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, second_frame);
				return RTN_FAIL;
			}

			memset(gobj_buddy_buffer_buf_get_addr(second_frame->frame), 0x00, gobj_buddy_buffer_buf_get_size(new_frame->frame));

			second_frame->icodec_h.gst_buffer = second_frame->frame;
			s = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
			d = gobj_buddy_buffer_buf_get_addr(second_frame->frame);
			memcpy(d, s, gobj_buddy_buffer_buf_get_size(new_frame->frame));
			icodec_h = (ICODEC_HEADER*) d;
			memcpy(d, &second_frame->icodec_h, sizeof(ICODEC_HEADER));
			lch->shadow_flag = 1;
		}
		else if (lst->resolution == NF_RES_NTSC_4CIFP || lst->resolution == NF_RES_PAL_4CIFP)
		{
			//second_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
			second_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
			if (second_frame == NULL)
			{
				MRTPSRC_DBG(WARN, "%s | alloc heap fail. Reboot request(%d, %s)\n",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
				lch->state = STATE_REBOOT_REQ;
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return RTN_FAIL;
			}
			memcpy(second_frame, new_frame, sizeof(struct _FRAME_INFO_T_));
			memcpy(&second_frame->icodec_h, &new_frame->icodec_h, sizeof(ICODEC_HEADER));
			second_frame->icodec_h.chan = lst->ch_num+32;

			//second_frame->frame = gst_nf_buddy_buffer_new_and_alloc(
			//		GST_BUFFER_SIZE(new_frame->frame), (void*)&second_frame->icodec_h.chan);
			second_frame->frame = mrtpsrc_alloc_cmem(
					gobj_buddy_buffer_buf_get_size(new_frame->frame), &second_frame->icodec_h.chan,
					__FILE__, __FUNCTION__, __LINE__);
			if (second_frame->frame == NULL)
			{
				MRTPSRC_DBG(WARN, "%s | cmem alloc fail. Reboot request(%d, %s)\n",
						__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
				lch->state = STATE_REBOOT_REQ;
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, second_frame);
				return RTN_FAIL;
			}

			memset(gobj_buddy_buffer_buf_get_addr(second_frame->frame), 0x00, gobj_buddy_buffer_buf_get_size(new_frame->frame));

			second_frame->icodec_h.gst_buffer = second_frame->frame;
			s = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
			d = gobj_buddy_buffer_buf_get_addr(second_frame->frame);
			memcpy(d, s, gobj_buddy_buffer_buf_get_size(new_frame->frame));
			icodec_h = (ICODEC_HEADER*) d;
			memcpy(d, &second_frame->icodec_h, sizeof(ICODEC_HEADER));
		}
		else if (get_live_channel_num() < 5)
		{
			if (lch->shadow_flag)
			{
				//second_frame = (MRTPSRC_FRAME_T) malloc(sizeof(struct _FRAME_INFO_T_));
				second_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
				if (second_frame == NULL)
				{
					MRTPSRC_DBG(WARN, "%s | alloc heap fail. Reboot request(%d, %s)\n",
							__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
					lch->state = STATE_REBOOT_REQ;
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					return RTN_FAIL;
				}
				memcpy(second_frame, new_frame, sizeof(struct _FRAME_INFO_T_));
				memcpy(&second_frame->icodec_h, &new_frame->icodec_h, sizeof(ICODEC_HEADER));
				second_frame->icodec_h.chan = lst->ch_num+32;

				//second_frame->frame = gst_nf_buddy_buffer_new_and_alloc(
				//		GST_BUFFER_SIZE(new_frame->frame), (void*)&second_frame->icodec_h.chan);
				second_frame->frame = mrtpsrc_alloc_cmem(
						gobj_buddy_buffer_buf_get_size(new_frame->frame), &second_frame->icodec_h.chan,
						__FILE__, __FUNCTION__, __LINE__);
				if (second_frame->frame == NULL)
				{
					MRTPSRC_DBG(WARN, "%s | cmem alloc fail. Reboot request(%d, %s)\n",
							__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
					lch->state = STATE_REBOOT_REQ;
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
					mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, second_frame);
					return RTN_FAIL;
				}

				memset(gobj_buddy_buffer_buf_get_addr(second_frame->frame), 0x00, gobj_buddy_buffer_buf_get_size(new_frame->frame));

				second_frame->icodec_h.gst_buffer = second_frame->frame;
				s = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
				d = gobj_buddy_buffer_buf_get_addr(second_frame->frame);
				memcpy(d, s, gobj_buddy_buffer_buf_get_size(new_frame->frame));
				icodec_h = (ICODEC_HEADER*) d;
				memcpy(d, &second_frame->icodec_h, sizeof(ICODEC_HEADER));
			}
		}
		else
		{
			lch->shadow_flag = 0;
		}
	}
#endif

	/* ITX private data to integrate systems */
	gop_info = lst->gop % 16;
	gop_info = gop_info << 16;
	gop_info = gop_info | new_frame->frame_seq;
	// GST_NF_BUDDY_BUFFER(new_frame->frame)->cmemq = (void*)gop_info;
	// GST_NF_BUDDY_BUFFER(new_frame->frame)->priv = (void*)((lst->stream_num * 32)+lst->ch_num);

	seq = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
	icodec_h = (ICODEC_HEADER*) seq;
	icodec_h->reserved = gop_info;

	if (second_frame != NULL)
	{
		/* ITX private data to integrate systems */
		// GST_NF_BUDDY_BUFFER(second_frame->frame)->cmemq = (void*)gop_info;
		// GST_NF_BUDDY_BUFFER(second_frame->frame)->priv = (void*)(32+lst->ch_num);

		seq = gobj_buddy_buffer_buf_get_addr(second_frame->frame);
		icodec_h = (ICODEC_HEADER*) seq;
		icodec_h->reserved = gop_info;
	}

#if 0
#if MRTPSRC_RECORD_HOFF
	if (src != NULL && src->handoff_callback != NULL)
	{
		src->handoff_callback(new_frame->frame, src->handoff_user_data);
#if MRTPSRC_USE_STATISTICS
		stats->handoff_cnt++;
#endif

#if 1
		if (second_frame != NULL)
		{
			src->handoff_callback(second_frame->frame, src->handoff_user_data);
#if MRTPSRC_USE_STATISTICS
			stats->handoff_cnt++;
#endif
		}
#endif
	}
#endif
#endif

#ifdef MRTPSRC_SUPPORT_SMART_CAMERA

	if (new_frame->frame_type == FTYPE_VA_META){
		if(lst->stream_num == STREAM_1ST){
			guint max_size = 20000;
			
			if(new_frame->len > max_size){
				MRTPSRC_DBG(WARN, "%s | sei_size is too big %d ", __FUNCTION__, new_frame->len);
			}
				
#if MRTPSRC_RECORD_HOFF

		if(!GOBJ_IS_BUDDY_BUFFER (new_frame->frame))
		{
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(new_frame->frame);
		}
		mrtpsrc_ho_enqueue(new_frame->frame);
#endif

				
			if(lst->sei_meta_1st != NULL){
				MRTPSRC_FRAME_T sei = lst->sei_meta_1st;

				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
				lst->sei_meta_1st = NULL;
			}

			lst->sei_meta_1st = new_frame;
		
		}
		else{
			if(lst->sei_meta_2nd != NULL){
				MRTPSRC_FRAME_T sei = lst->sei_meta_2nd;

				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
				lst->sei_meta_2nd = NULL;
			}
			
			lst->sei_meta_2nd= new_frame;
		}
	}
	else if(new_frame->frame_type == FTYPE_VA_EVENT){
		if(lst->stream_num == STREAM_1ST){
#if MRTPSRC_RECORD_HOFF

		if(!GOBJ_IS_BUDDY_BUFFER (new_frame->frame))
		{
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(new_frame->frame);
		}
		mrtpsrc_ho_enqueue(new_frame->frame);
#endif

		if(lst->sei_event_1st != NULL){
			MRTPSRC_FRAME_T sei = lst->sei_event_1st;

			mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
				lst->sei_event_1st = NULL;
			}
			
			lst->sei_event_1st= new_frame;
			
				//	MRTPSRC_DBG(WARN, "%s | sei_event_1st save", __FUNCTION__);
		}
		else{
			if(lst->sei_event_2nd != NULL){
				MRTPSRC_FRAME_T sei = lst->sei_event_2nd;

				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, sei->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, sei);
				lst->sei_event_2nd = NULL;
			}
			
			lst->sei_event_2nd= new_frame;
			
				//	MRTPSRC_DBG(WARN, "%s | sei_event_2nd save", __FUNCTION__);
		}
		
	}
	else
	{
#if MRTPSRC_RECORD_HOFF
		if(!GOBJ_IS_BUDDY_BUFFER (new_frame->frame))
		{
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(new_frame->frame);
		}

#if (0)
		if(g_frame_dump_count < 300 && lst->stream_num == STREAM_1ST)
		{
			printf("\e[33m [%s] frame dump (%d) \e[0m\n", __func__, g_frame_dump_count);

			FILE *fp = NULL;
			fp = fopen("dump_h265", "ab");
			fwrite(gobj_buddy_buffer_buf_get_addr(new_frame->frame) + sizeof(ICODEC_HEADER), 1, (new_frame->len - sizeof(ICODEC_HEADER)), fp);
			fclose(fp);
			g_frame_dump_count ++;
		}
#endif


		mrtpsrc_ho_enqueue(new_frame->frame);
#endif
#if MRTPSRC_USE_STATISTICS
	stats->handoff_cnt++;
#endif
	if (second_frame != NULL)
	{
#if MRTPSRC_RECORD_HOFF

		if(!GOBJ_IS_BUDDY_BUFFER (second_frame->frame))
		{
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(second_frame->frame);
		}
		mrtpsrc_ho_enqueue(second_frame->frame);
#endif
#if MRTPSRC_USE_STATISTICS
		stats->handoff_cnt++;
#endif
	}

	if (_mrtpsrc_check_drop_p(lst, new_frame))
		return RTN_OK;

#if MRTPSRC_USE_STATISTICS
	stats->frame_cnt++;
	stats->cbc_frame_cnt[lst->ch_num + lst->stream_num*32]++;
	if (new_frame->frame_type == FTYPE_IDR)
	{
		stats->i_cnt++;
	}
	else if (new_frame->frame_type == FTYPE_N_IDR)
	{
		stats->p_cnt++;
	}
#endif
		mrtpsrc_frame_q_lock();	/* Frame list lock */
		if (lst->frame_head == NULL)
		{
			lst->frame_head = new_frame;
			lst->frame_tail = new_frame;

#if 1
			if (second_frame != NULL)
			{
				if (lch->stream_2->frame_head == NULL)
				{
					lch->stream_2->frame_head = second_frame;
					lch->stream_2->frame_tail = second_frame;
				}
				else
				{
					lch->stream_2->frame_tail->next = second_frame;
					lch->stream_2->frame_tail = second_frame;
				}
			}
#endif
			mrtpsrc_frame_q_unlock();

			return RTN_OK;
		}

#if 1
		if (second_frame != NULL)
		{
			if (lch->stream_2->frame_head == NULL)
			{
				lch->stream_2->frame_head = second_frame;
				lch->stream_2->frame_tail = second_frame;
			}
			else
			{
				lch->stream_2->frame_tail->next = second_frame;
				lch->stream_2->frame_tail = second_frame;
			}
		}
#endif
		lst->frame_tail->next = new_frame;
		lst->frame_tail = new_frame;

		mrtpsrc_frame_q_unlock();/* Frame list unlock */
	}
#else

#if MRTPSRC_RECORD_HOFF

	if(!GOBJ_IS_BUDDY_BUFFER (new_frame->frame))
	{
		printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
		gst_buffer_debug(new_frame->frame);
	}
	mrtpsrc_ho_enqueue(new_frame->frame);
#endif
#if MRTPSRC_USE_STATISTICS
	stats->handoff_cnt++;
#endif
	if (second_frame != NULL)
	{
#if MRTPSRC_RECORD_HOFF

		if(!GOBJ_IS_BUDDY_BUFFER (second_frame->frame))
		{
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(second_frame->frame);
		}
		mrtpsrc_ho_enqueue(second_frame->frame);
#endif
#if MRTPSRC_USE_STATISTICS
		stats->handoff_cnt++;
#endif
	}

	if (_mrtpsrc_check_drop_p(lst, new_frame))
		return RTN_OK;

#if MRTPSRC_USE_STATISTICS
	stats->frame_cnt++;
	stats->cbc_frame_cnt[lst->ch_num + lst->stream_num*32]++;
	if (new_frame->frame_type == FTYPE_IDR)
	{
		stats->i_cnt++;
	}
	else if (new_frame->frame_type == FTYPE_N_IDR)
	{
		stats->p_cnt++;
	}
#endif
		mrtpsrc_frame_q_lock();	/* Frame list lock */
		if (lst->frame_head == NULL)
		{
			lst->frame_head = new_frame;
			lst->frame_tail = new_frame;

#if 1
			if (second_frame != NULL)
			{
				if (lch->stream_2->frame_head == NULL)
				{
					lch->stream_2->frame_head = second_frame;
					lch->stream_2->frame_tail = second_frame;
				}
				else
				{
					lch->stream_2->frame_tail->next = second_frame;
					lch->stream_2->frame_tail = second_frame;
				}
			}
#endif
			mrtpsrc_frame_q_unlock();

			return RTN_OK;
		}

#if 1
		if (second_frame != NULL)
		{
			if (lch->stream_2->frame_head == NULL)
			{
				lch->stream_2->frame_head = second_frame;
				lch->stream_2->frame_tail = second_frame;
			}
			else
			{
				lch->stream_2->frame_tail->next = second_frame;
				lch->stream_2->frame_tail = second_frame;
			}
		}
#endif
		lst->frame_tail->next = new_frame;
		lst->frame_tail = new_frame;

		mrtpsrc_frame_q_unlock();/* Frame list unlock */

#endif	

	return RTN_OK;
}

static gint _mrtpsrc_calculate_vts(MRTPSRC_STREAM_T* lst, ICODEC_HEADER* icodec_h, guint frame_timestamp)
{
	int diff_sec = 0;
	int diff_5msec = 0;

	GTimeVal now_time = mrtpsrc_get_captured_time();

	MRTPSRC_CHANNEL_T *lch = NULL;
	MRTPSRC_STREAM_STAT_T *stats = NULL;

#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *src = NULL;
#endif

#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
#ifndef NMZ_STANDLONE_MODE
	src = gst_mrtp_src_get_object();
#endif

	lch = mrtpsrc_get_channel(lst->ch_num);

	/* Case 0: System time changed */
	{
		diff_sec = now_time.tv_sec - lst->ts_last_second;
		if (lst->ts_last_rcvd != 0 && (diff_sec < (-1)*(MRTPSRC_TOLERANT_TS_SECS+10) || diff_sec > (MRTPSRC_TOLERANT_TS_SECS+10)))
		{
			/* P-frame skip */
			if (lst->gop_seq == 0)
			{
#if MRTPSRC_USE_STATISTICS
				stats->ts_sys_cnt++;
				stats->cbc_sys_time_chg[lst->ch_num + lst->stream_num*32]++;
#endif
				MRTPSRC_DBG(WARN, "%s | System time may be changed(%d, %s)\n"
						  "    ==> Last timestamp(%u) Frame timestamp(%u)\n"
						  "    ==> Last frame(%u.%u) Now(%ld.%ld)\n",
						__FUNCTION__,
						lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num],
						lst->ts_last_rcvd, frame_timestamp,
						lst->ts_last_second, lst->ts_last_5_mili,
						now_time.tv_sec, now_time.tv_usec / 5000);

#if 1
				icodec_h->timestamp = now_time.tv_sec;
				icodec_h->timestampl = now_time.tv_usec / 5000;
				lst->ts_last_second = icodec_h->timestamp;
				lst->ts_last_5_mili = icodec_h->timestampl;
				lst->ts_first_second = lst->ts_last_second;
				lst->ts_first_5_mili = lst->ts_last_5_mili;
				lst->ts_last_rcvd = frame_timestamp;
				return RTN_OK;
#else
				lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
				stats->reconn_systime[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
				if (src == NULL) { return RTN_FAIL; }
				if (src->cmd_callback == NULL) { return RTN_FAIL; }
				src->cmd_callback(2, lst->ch_num, 0, 109/*systime*/, src->cmd_user_data);
#endif
				return RTN_FAIL;
#endif
			}
		}
	}

	/* Case 1: First frame - Find pre-opened stream */
	if ((lst->ts_last_rcvd == 0) && (lst->gop == 1) && (lst->gop_seq == 0))
	{
		int i = 0;
		MRTPSRC_STREAM_T* ch_iter = NULL;

		/* Check if other streams' last i-frame timestamp is valid */
		for (i = STREAM_1ST; i < STREAM_MAX; i++)
		{
			ch_iter = mrtpsrc_get_stream(lst->ch_num, i);

			/* Pass this stream */
			if (lst->stream_num == i)
				continue;

			/* Pass the closed streams */
			if (ch_iter->ts_first_second == 0 && ch_iter->ts_first_5_mili == 0)
				continue;

			/* Case 1-1: Found pre-opened stream for this channel */
#if 1
			icodec_h->timestamp = ch_iter->ts_first_second;
			icodec_h->timestampl = ch_iter->ts_first_5_mili;
#else
			icodec_h->timestamp = ch_iter->ts_last_second;
			icodec_h->timestampl = ch_iter->ts_last_5_mili;
#endif
			lst->ts_last_second = icodec_h->timestamp;
			lst->ts_last_5_mili = icodec_h->timestampl;
			lst->ts_first_second = lst->ts_last_second;
			lst->ts_first_5_mili = lst->ts_last_5_mili;
			lst->ts_last_rcvd = frame_timestamp;
			return RTN_OK;
		}

		/* Case 1-2: Not found pre-opened stream for this channel */
		icodec_h->timestamp = now_time.tv_sec;
		icodec_h->timestampl = now_time.tv_usec / 5000;
		lst->ts_last_second = icodec_h->timestamp;
		lst->ts_last_5_mili = icodec_h->timestampl;
		lst->ts_first_second = lst->ts_last_second;
		lst->ts_first_5_mili = lst->ts_last_5_mili;
		lst->ts_last_rcvd = frame_timestamp;
		return RTN_OK;
	}

	/* Case 2: Not first frame */
	{
		/* 
		 * sec_dur:200 = ts_diff:X
		 * X = (200 x ts_diff) / sec_dur
		 */
		guint ts_div = lst->sec_dur / 200;
		guint ts_diff = frame_timestamp - lst->ts_last_rcvd;
		guint ts_quotient = ts_diff / ts_div;
		guint ts_remainder = ts_diff % ts_div;

		if (ts_diff > MRTPSRC_TOLERANT_TS_SECS*90000)
		{
			MRTPSRC_DBG(WARN, "%s | Detected wrong timestamp from camera(%d, %s)\n"
					  "    ==> Last timestamp(%u) Frame timestamp(%u)\n"
					  "    ==> Last frame(%u.%u) Now(%ld.%ld)\n",
					__FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num],
					lst->ts_last_rcvd, frame_timestamp,
					lst->ts_last_second, lst->ts_last_5_mili,
					now_time.tv_sec, now_time.tv_usec / 5000);

#if 1
			icodec_h->timestamp = now_time.tv_sec;
			icodec_h->timestampl = now_time.tv_usec / 5000;
			lst->ts_last_second = icodec_h->timestamp;
			lst->ts_last_5_mili = icodec_h->timestampl;
			lst->ts_first_second = lst->ts_last_second;
			lst->ts_first_5_mili = lst->ts_last_5_mili;
			lst->ts_last_rcvd = frame_timestamp;
			return RTN_OK;
#else

			lch->state = STATE_RECONN_REQ;
			return RTN_FAIL;
#endif
			//ts_diff = 4000;
			//ts_quotient = ts_diff / ts_div;
			//ts_remainder = ts_diff % ts_div;
		}
		else
		{
			if (lst->ts_last_remainder + ts_remainder >= ts_div)
			{
				ts_quotient++;
				lst->ts_last_remainder = lst->ts_last_remainder + ts_remainder - ts_div;
			}
			else
			{
				lst->ts_last_remainder += ts_remainder;
			}
		}
		lst->ts_last_5_mili += ts_quotient;
#if 0
		if (lst->ts_last_5_mili >= 200 && lst->ts_last_5_mili < 400)
		{
			lst->ts_last_5_mili -= 200;
			lst->ts_last_second += 1;
		}
		else if (lst->ts_last_5_mili >= 400)
		{
#if MRTPSRC_USE_STATISTICS
			stats->ts_err_cnt++;
			stats->cbc_ts_err[lst->ch_num + lst->stream_num*32]++;
#endif
			MRTPSRC_DBG(ERROR, "%s | time calculation error(%ld, %s) ==> %lu.%lu\n"
					"   ==> ts_last_rcvd(%lu) ts_frame(%lu)\n"
					"   ==> ts_div(%lu) ts_diff(%lu) ts_quotient(%lu) ts_remainder(%lu)\n"
					"   ==> ts_last_remainder(%lu) ts_last_second(%lu) ts_last_5_mili(%lu)\n"
					,
					__FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num],
					lst->ts_last_second, lst->ts_last_5_mili,
					lst->ts_last_rcvd, frame_timestamp,
					ts_div, ts_diff, ts_quotient, ts_remainder,
					lst->ts_last_remainder, lst->ts_last_second, lst->ts_last_5_mili);
			lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
			stats->reconn_timestamp[lst->ch_num]++;
#endif
			return RTN_FAIL;
		}
#else
		if (lst->ts_last_5_mili > 200*MRTPSRC_TOLERANT_TS_SECS)
		{
#if MRTPSRC_USE_STATISTICS
			stats->ts_err_cnt++;
			stats->cbc_ts_err[lst->ch_num + lst->stream_num*32]++;
#endif
			MRTPSRC_DBG(ERROR, "%s | timestamp error(%d, %s) ==> %u.%u\n"
					"   ==> ts_last_rcvd(%u) ts_frame(%u)\n"
					"   ==> ts_div(%u) ts_diff(%u) ts_quotient(%u) ts_remainder(%u)\n"
					"   ==> ts_last_remainder(%u) ts_last_second(%u) ts_last_5_mili(%u)\n"
					,
					__FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num],
					lst->ts_last_second, lst->ts_last_5_mili,
					lst->ts_last_rcvd, frame_timestamp,
					ts_div, ts_diff, ts_quotient, ts_remainder,
					lst->ts_last_remainder, lst->ts_last_second, lst->ts_last_5_mili);
			lch->state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
			stats->reconn_timestamp[lst->ch_num]++;
#endif
//#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
			if (src == NULL) { return RTN_FAIL; }
			if (src->cmd_callback == NULL) { return RTN_FAIL; }
			src->cmd_callback(2, lst->ch_num, 0, 110/*timestmp*/, src->cmd_user_data);
#endif
//#endif
			return RTN_FAIL;
		}

		while (lst->ts_last_5_mili >= 200)
		{
			lst->ts_last_5_mili -= 200;
			lst->ts_last_second += 1;
		}
#endif

		/* Timestamp micro compensation */
		diff_sec = now_time.tv_sec - lst->ts_last_second;
		diff_5msec = (now_time.tv_usec / 5000) - lst->ts_last_5_mili;
		diff_5msec = diff_sec * 200 + diff_5msec;

		//if (lst->stream_num == STREAM_1ST)
		{
			//MRTPSRC_STREAM_T *cs_lst = mrtpsrc_get_stream(lst->ch_num, (lst->stream_num+1)%2);

			if (diff_5msec < -10 && lst->ts_last_5_mili > 0)
			{
				lst->ts_last_5_mili--;
				//cs_lst->ts_last_5_mili--;
#if MRTPSRC_USE_STATISTICS
				stats->ts_compen_cnt++;
				stats->cbc_ts_com[lst->ch_num + lst->stream_num*32]++;
#endif
#if 0
				MRTPSRC_DBG(IMPACT, "%s | Micro compensation(%ld) Now[%lu.%lu], Frame[%lu.%lu]--\n",
						__FUNCTION__,
						lst->ch_num,
						now_time.tv_sec, now_time.tv_nsec / 1000 / 5000,
						lst->ts_last_second, lst->ts_last_5_mili);
#endif
			}
			else if (diff_5msec > 10 && lst->ts_last_5_mili < 199)
			{
				lst->ts_last_5_mili++;
				//cs_lst->ts_last_5_mili++;
#if MRTPSRC_USE_STATISTICS
				stats->ts_compen_cnt++;
				stats->cbc_ts_com[lst->ch_num + lst->stream_num*32]++;
#endif
#if 0
				MRTPSRC_DBG(IMPACT, "%s | Micro compensation(%ld) Now[%lu.%lu], Frame[%lu.%lu]++\n",
						__FUNCTION__,
						lst->ch_num,
						now_time.tv_sec, now_time.tv_nsec / 1000 / 5000,
						lst->ts_last_second, lst->ts_last_5_mili);
#endif
			}
		}


		icodec_h->timestamp = lst->ts_last_second;
		icodec_h->timestampl = lst->ts_last_5_mili;

		if (lst->gop_seq == 0)
		{
			lst->ts_first_second = lst->ts_last_second;
			lst->ts_first_5_mili = lst->ts_last_5_mili;
		}
	}

#if 0
	printf("Current %lu.%lu\n", now_time.tv_sec, now_time.tv_usec /5000);
	if (!lst->stream_num)
	{
		MRTPSRC_DBG(MAJOR, "CH(%ld,%s) %04lu[%02lu] %lu.%lu\n", lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->gop, lst->gop_seq, lst->ts_last_second, lst->ts_last_5_mili);
	}
	else
	{
		printf("Current %lu.%lu\n", now_time.tv_sec, now_time.tv_usec /5000);
		MRTPSRC_DBG(MINOR, "CH(%ld,%s) %04lu[%02lu] %lu.%lu law(%lu)\n", lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->gop, lst->gop_seq, lst->ts_last_second, lst->ts_last_5_mili, frame_timestamp);
	}
#endif

	lst->ts_last_rcvd = frame_timestamp;

	return RTN_OK;
}

static gint _mrtpsrc_check_drop_p(MRTPSRC_STREAM_T* lst, MRTPSRC_FRAME_T new_frame)
{
#if 1
	int channel_state = lst->state;

	switch(channel_state)
	{
		case STATE_DROP_P_ALL:
		case STATE_P_REQUESTED:
			if (new_frame->frame_type == FTYPE_N_IDR)
			{
				//printf("CH(%d, %d) free (%d)\n", lst->ch_num, lst->stream_num, new_frame->frame_seq);
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return (1);
			}
			else if (new_frame->frame_type == FTYPE_IDR)
			{
				lst->state = STATE_PLAYING;
				return (0);
			}
			break;
		case STATE_DROP_P_5:
			if (new_frame->frame_seq >= 25 && new_frame->frame_seq <= 27)
			{
				//printf("CH(%d, %d) free (%d)\n", lst->ch_num, lst->stream_num, new_frame->frame_seq);
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return (1);
			}
		case STATE_DROP_P_2:
			if (new_frame->frame_seq == 28)
			{
				//printf("CH(%d, %d) free (%d)\n", lst->ch_num, lst->stream_num, new_frame->frame_seq);
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return (1);
			}
		case STATE_DROP_P_1:
			if (new_frame->frame_seq == 29)
			{
				//printf("CH(%d, %d) free (%d)\n", lst->ch_num, lst->stream_num, new_frame->frame_seq);
				lst->state = STATE_PLAYING;
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return (1);
			}
			break;
		case STATE_I_ONLY:
			if (new_frame->frame_type == FTYPE_N_IDR)
			{
				//printf("CH(%d, %d) free (%d)\n", lst->ch_num, lst->stream_num, new_frame->frame_seq);
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
				return (1);
			}
			else if (new_frame->frame_type == FTYPE_IDR)
			{
				return (0);
			}
			break;
		default:
			break;
	}

#else
	if (lst->state == STATE_DROP_P_1)
	{
		lst->state = STATE_PLAYING;
	}
	else if (lst->state == STATE_DROP_P_2)
	{
		lst->state = STATE_PLAYING;
	}
	else if (lst->state == STATE_DROP_P_5)
	{
		lst->state = STATE_PLAYING;
	}
	else if (lst->state == STATE_DROP_P_ALL)
	{
		lst->state = STATE_PLAYING;
	}
#endif

	return (0);
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

static void _mrtpsrc_get_4bytes_from_bits(guchar* src, gint bit_pos, guchar* dst)
{
	guchar* p = src;
	*dst = (*p << bit_pos) | (*(p+1) >> (8-bit_pos));
	*(dst+1) = (*(p+1) << bit_pos) | (*(p+2) >> (8-bit_pos));
	*(dst+2) = (*(p+2) << bit_pos) | (*(p+3) >> (8-bit_pos));
	*(dst+3) = (*(p+3) << bit_pos) | (*(p+4) >> (8-bit_pos));
	*(dst+4) = (*(p+4) << bit_pos) | (*(p+5) >> (8-bit_pos));

#if SPS_PRINT_DETAIL
printf("     %02x%02x%02x%02x %02x -> %02x%02x%02x%02x (%d)\n",
		*src, *(src+1), *(src+2), *(src+3), *(src+4),
		*dst, *(dst+1), *(dst+2), *(dst+3), bit_pos);
#endif
}

static int _mrtpsrc_get_sev_from_codenum(gint codenum)
{
	int rtn;
	int m,r;

	m = codenum / 2;
	r = codenum % 2;

	rtn = m+r;
	if (r == 0)
	{
		rtn *= (-1);
	}

	return rtn;
}

static int _mrtpsrc_get_uev_codenum(guchar *src, gint *code_rtn)
{
	//int i = 0, j=0;
	int i=0;
	int codedigit = 0;
	int rest = 0;
	unsigned int codenum = 0;
	unsigned int val = 0;
	//unsigned int bit_to_shift = 0;
	//unsigned int src_data_uint = 0;
	unsigned char *p = src;


#if SPS_PRINT_DETAIL
	printf("   [%s] param src(%02x%02x%02x%02x)\n",
			__FUNCTION__, *p, *(p+1), *(p+2), *(p+3));
#endif


	codedigit = 0;

	for (i=0; i<4; i++)
	{
		if (*p == 0)
		{
			codedigit += 8;
			p++;
			continue;
		}
		break;
	}

	rest = 0;
	while(!((*p)&(1<<(7-rest)))) { rest++; }
	codedigit += rest;

#if SPS_PRINT_DETAIL
	printf("   [%s] codedigit(%d)\n", __FUNCTION__, codedigit);
#endif
	if (codedigit == 0)
	{
		*code_rtn = 0;
		return codedigit;
	}

	//p = src + (codedigit/8);

	val = *p<<24 | *(p+1)<<16 | *(p+2)<<8 | *(p+3);
#if SPS_PRINT_DETAIL
	printf("   [%s] val init(%08x)\n", __FUNCTION__, val);
#endif
	val <<= (codedigit+1);
#if SPS_PRINT_DETAIL
	printf("   [%s] val code 1(%08x)\n", __FUNCTION__, val);
#endif
	val >>= (31-codedigit+1);
#if SPS_PRINT_DETAIL
	printf("   [%s] val(%08x, %lu)\n", __FUNCTION__, val, val);
#endif

	codenum = (1<<codedigit) - 1 + val;
	*code_rtn = codenum;

#if SPS_PRINT_DETAIL
	printf("   [%s] codenum(%d)\n", __FUNCTION__, codenum);
#endif

	return codedigit;
}

static void h265_get_resolution(unsigned char *p_sps, int p_sps_len, unsigned int *p_w, unsigned int *p_h)
{
    nal_buffer_t *nal_buffer, *nal_buffer_decoded;
    unsigned int w, h;

    nal_buffer = (nal_buffer_t*)malloc(sizeof(nal_buffer_t));
    memset(nal_buffer, 0x00, sizeof(nal_buffer_t));

    nal_buffer->posmax = p_sps_len;
    nal_buffer->pos = 0;
    nal_buffer->bitpos = 8;
    memcpy(nal_buffer->data, p_sps, nal_buffer->posmax);

    //Decode SPS NAL rbsp data (remove 0x03 stuff)
    nal_buffer_decoded = (nal_buffer_t*)malloc(sizeof(nal_buffer_t));
    memset(nal_buffer_decoded, 0x00, sizeof(nal_buffer_t));

    decode_nul_rbsp(nal_buffer, nal_buffer_decoded, nal_buffer->posmax);
    nal_buffer_decoded->posmax = nal_buffer_decoded->pos;
    nal_buffer_decoded->pos = 0;
    nal_buffer_decoded->bitpos = 8;

    free(nal_buffer);
    nal_buffer = nal_buffer_decoded;

    nal_buffer->pos += 2;

    read_bits(nal_buffer, 4); // sps_video_parameter_set_id
    uint8 sps_max_sub_layers_minus1 = read_bits(nal_buffer, 3);
    read_bit(nal_buffer); // sps_temporal_id_nesting_flag

    profile_tier_level(nal_buffer, sps_max_sub_layers_minus1);

    read_uev(nal_buffer); // sps_seq_parameter_set_id
    uint8 chroma_format_idc = read_uev(nal_buffer);

    if (chroma_format_idc == 3)
    {
        read_bit(nal_buffer); // separate_colour_plane_flag
    }

    w = read_uev(nal_buffer); // pic_width_in_luma_samples
    h = read_uev(nal_buffer); // pic_height_in_luma_samples

    *p_w = w;
    *p_h = h;

    free(nal_buffer);
}

static void profile_tier_level(nal_buffer_t* pnal_buffer, int maxNumSubLayersMinus1)
{
    int i,j;
    int general_profile_idc;
    int general_profile_compatibility_flag[32] = { 0, };

    read_bits(pnal_buffer, 2); // general_profile_space
    read_bit(pnal_buffer); // general_tier_flag
    general_profile_idc = read_bits(pnal_buffer, 5);

    for (j = 0; j < 32; j++)
        general_profile_compatibility_flag[j] = read_bit(pnal_buffer);

    read_bit(pnal_buffer); // general_progressive_source_flag
    read_bit(pnal_buffer); // general_interlaced_source_flag
    read_bit(pnal_buffer); // general_non_packed_constraint_flag
    read_bit(pnal_buffer); // general_frame_only_constraint_flag

    read_bits(pnal_buffer, 32);
    read_bits(pnal_buffer, 12);

    read_bits(pnal_buffer, 8); // general_level_idc

    uint8 sub_layer_profile_present_flag[8];
    uint8 sub_layer_level_present_flag[8];

    for (i = 0; i < maxNumSubLayersMinus1; i++)
    {
        sub_layer_profile_present_flag[i] = read_bit(pnal_buffer);
        sub_layer_level_present_flag[i] = read_bit(pnal_buffer);
    }

    if (maxNumSubLayersMinus1 > 0)
    {
        for (i = maxNumSubLayersMinus1; i < 8; i++)
            read_bits(pnal_buffer, 2); // reserved_zero_2bits
    }

    for (i = 0; i < maxNumSubLayersMinus1; i++)
    {
        if (sub_layer_profile_present_flag[i])
        {
            read_bits(pnal_buffer, 2); // sub_layer_profile_space[i]
            read_bit(pnal_buffer); // sub_layer_tier_flag[i]
            read_bits(pnal_buffer, 5); // sub_layer_profile_idc[i]

            for (j = 0; j < 32; j++)
                read_bit(pnal_buffer); // sub_layer_profile_compatibility_flag[i][j]

            read_bit(pnal_buffer); // sub_layer_progressive_source_flag[i]
            read_bit(pnal_buffer); // sub_layer_interlaced_source_flag[i]
            read_bit(pnal_buffer); // sub_layer_non_packed_constraint_flag[i]
            read_bit(pnal_buffer); // sub_layer_frame_only_constraint_flag[i]

            read_bits(pnal_buffer, 32);
            read_bits(pnal_buffer, 12); //skipping sub_layer_reserved_zero_44bits[i]
        }

        if (sub_layer_level_present_flag[i])
            read_bits(pnal_buffer, 8); // sub_layer_level_idc[i]
    }
}

static void decode_nul_rbsp(nal_buffer_t * buffin, nal_buffer_t * buffout, int size)
{
    int i;
    int state = STATE_EXPECTING_ZERO_0;
    int extra_zero = 0;
    for (i = 0; i < size; i++)
    {
        switch (state)
        {
            case STATE_EXPECTING_ZERO_0:
                if (buffin->data[i] == 0)
                {
                    state = STATE_EXPECTING_ZERO_1;
                }
                else if (buffin->pos < NAL_BUFFER_MAX)
                {
                    copy_to_nal_buf(buffout, buffin->data[i]);
                }
                break;

            case STATE_EXPECTING_ZERO_1:
                if (buffin->data[i] == 0)
                {
                    state = STATE_EXPECTING_THREE;
                }
                else
                {
                    // restore missed 0, copy current byte
                    copy_to_nal_buf(buffout, 0);
                    copy_to_nal_buf(buffout, buffin->data[i]);
                    state = STATE_EXPECTING_ZERO_0;
                    extra_zero = 0;
                }
                break;

            case STATE_EXPECTING_THREE:
                if (buffin->data[i] == 3) {
                    state = STATE_EXPECTING_ZERO_0;
                    //emulation_prevention_three_byte here
                    //restore two missed 0s, copy current byte
                    copy_to_nal_buf(buffout, 0);
                    copy_to_nal_buf(buffout, 0);
                    if (extra_zero) {
                        copy_to_nal_buf(buffout, 0);
                    }
                    state = STATE_EXPECTING_ZERO_0;
                    extra_zero = 0;
                }
                else if (buffin->data[i] == 0) {
                    // allow more zeroes, it is ok
                    extra_zero = 1;
                }
                else {
                    // restore two missed 0s, copy current byte
                    copy_to_nal_buf(buffout, 0);
                    copy_to_nal_buf(buffout, 0);
                    if (extra_zero) {
                        copy_to_nal_buf(buffout, 0);
                    }
                    copy_to_nal_buf(buffout, buffin->data[i]);
                    state = STATE_EXPECTING_ZERO_0;
                    extra_zero = 0;
                }
                break;
        }
    }
}



/////////////////////////////////////////////////////////////////////////////////////////

static void copy_to_nal_buf(nal_buffer_t * pnal_buffer, uint8 c)
{
    if (pnal_buffer->pos >= NAL_BUFFER_MAX)
    {
        printf("\nNAL unit is too big. NAL_BUFFER_MAX need to be increased to fix.\n at %s, line %d.\n",
                __FILE__, __LINE__);
        return;
    }
    pnal_buffer->data[pnal_buffer->pos] = c;
    pnal_buffer->pos++;
}

static void dump_nal_buffer(nal_buffer_t * pnal_buffer)
{
    int i;
    for (i = 0; i < pnal_buffer->posmax; i++) {
        if (i % 16 == 0) {
            printf("\t%04x:  ", i);
        }
        printf("%02x ", pnal_buffer->data[i]);
        if (i % 16 == 15) {
            printf("\n");
        }
    }
    printf("\n");
}

static void align_to_byte(nal_buffer_t * pnal_buffer)
{
    if (pnal_buffer->bitpos != 8) {
        pnal_buffer->pos++;
        pnal_buffer->bitpos = 8;
    }
}

static uint8 read_bit(nal_buffer_t * pnal_buffer)
{
    if (pnal_buffer->bitpos == 0) {
        pnal_buffer->pos++;
        pnal_buffer->bitpos = 8;
    }

    if (pnal_buffer->pos >= pnal_buffer->posmax) {
        printf("Error - !!! nal buffer overrun !!!\n");
    }

    uint8 ret = pnal_buffer->data[pnal_buffer->pos] & (1 << (pnal_buffer->bitpos - 1));
    pnal_buffer->bitpos--;
    //printf("%d", ret > 0 ? 1 : 0);
    return ret > 0 ? 1 : 0;
}

static uint32 read_bits(nal_buffer_t * pnal_buffer, int nbits)
{
    int i;
    uint32 ret = 0;
    for (i = 0; i < nbits; i++) {
        ret = (ret << 1) | read_bit(pnal_buffer);
    }
    return ret;
}

static uint64 read_bits64(nal_buffer_t * pnal_buffer, int nbits)
{
    int i;
    uint64 ret = 0;
    for (i = 0; i < nbits; i++) {
        ret = (ret << 1) | read_bit(pnal_buffer);
    }
    return ret;
}

static uint32 read_uev(nal_buffer_t * pnal_buffer)
{
    //printf("[");
    int zero_leading_bits = -1;
    uint8 b = 0;
    for (b = 0; !b; zero_leading_bits++) {
        b = read_bit(pnal_buffer);
    }
    uint32 ret = (1 << zero_leading_bits) - 1 + read_bits(pnal_buffer, zero_leading_bits);
    //printf("]");
    return ret;
}

static sint32 read_sev(nal_buffer_t * pnal_buffer) { // read signed exp-golomb code
    uint32 val = read_uev(pnal_buffer);
    sint32 ret = 0;
    if (val > 0) {
        ret = ((val % 2) > 0 ? 1 : -1) * CEIL(val, 2);
    }
    return ret;

}

static void _mrtpsrc_get_resolution_from_sps(gchar* src, gint *width, gint *height)
{
	guchar* p = NULL;
	guchar temp[8];
	int i = 0;
	int j;
	int lastScale = 8;
	int nextScale = 8;
	int delta_scale;

	int now_uev_digit = 0;
	int byte_pos = 0;
	int bit_pos = 0;

	int ScalingList4x4[6][16];
	int ScalingList8x8[2][64];
	int UseDefaultScalingMatrix4x4Flag[6];
	int UseDefaultScalingMatrix8x8Flag[2];

	guchar profile_idc;
	guchar constraint_set0_flag;
	guchar constraint_set1_flag;
	guchar constraint_set2_flag;
	guchar constraint_set3_flag;
	guchar reserved_zero_4bits;
	guchar level_idc;

	int seq_parameter_set_id;
	int chroma_format_idc;
	guchar residual_colour_transform_flag;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	guchar qpprime_y_zero_transform_bypass_flag;
	guchar seq_scaling_matrix_present_flag;
	guchar seq_scaling_list_present_flag[8];
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int offset_for_ref_frame;
	int num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	//int frame_mbs_only_flag;

	p = (guchar*)src;

	profile_idc = *p++;
	byte_pos++;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) profile_idc - %d\n", byte_pos, *p, bit_pos, profile_idc);
#endif

	constraint_set0_flag = ((*p) & 0x80)>>7;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) constraint_set0_flag - %d\n", byte_pos, *p, bit_pos, constraint_set0_flag);
#endif

	constraint_set1_flag = ((*p) & 0x40)>>6;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) constraint_set1_flag - %d\n", byte_pos, *p, bit_pos, constraint_set1_flag);
#endif

	constraint_set2_flag = ((*p) & 0x20)>>5;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) constraint_set2_flag - %d\n", byte_pos, *p, bit_pos, constraint_set2_flag);
#endif

	constraint_set3_flag = ((*p) & 0x10)>>4;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) constraint_set3_flag - %d\n", byte_pos, *p, bit_pos, constraint_set3_flag);
#endif

	reserved_zero_4bits = ((*p++) & 0xf);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) reserved_zero_4bits - %d\n", byte_pos, *p, bit_pos, reserved_zero_4bits);
#endif
	byte_pos++;

	level_idc = *p++;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) level_idc - %d\n", byte_pos, *p, bit_pos, level_idc);
#endif
	byte_pos++;
	bit_pos = 0;

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &seq_parameter_set_id);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) seq_parameter_set_id - %d\n", byte_pos, *p, bit_pos, seq_parameter_set_id);
#endif
	bit_pos = now_uev_digit*2+1;
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 144)
	{
		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &chroma_format_idc);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) chroma_format_idc - %d\n", byte_pos, *p, bit_pos, chroma_format_idc);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >=8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		if (chroma_format_idc == 3)
		{
			residual_colour_transform_flag = (*p & (1<<(7-bit_pos))) ? 1:0;
#if SPS_PRINT_MSG
			printf("byte(%d %02x) bit(%d) residual_colour_transform_flag - %d\n", byte_pos, *p, bit_pos, chroma_format_idc);
#endif
			bit_pos++;

			while(bit_pos >= 8)
			{
				bit_pos -= 8;
				p++;
				byte_pos++;
			}
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &bit_depth_luma_minus8);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) bit_depth_luma - %d\n", byte_pos, *p, bit_pos, bit_depth_luma_minus8 + 8);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &bit_depth_chroma_minus8);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) bit_depth_chroma - %d\n", byte_pos, *p, bit_pos, bit_depth_chroma_minus8 + 8);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		qpprime_y_zero_transform_bypass_flag = (temp[0] & 0x80) ? 1:0;
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) qpprime_y_zero_transform_bypass_flag - %d\n", byte_pos, *p, bit_pos, qpprime_y_zero_transform_bypass_flag);
#endif
		bit_pos++;
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		seq_scaling_matrix_present_flag = (temp[0] & 0x80) ? 1:0;
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) seq_scaling_matrix_present_flag - %d\n", byte_pos, *p, bit_pos, seq_scaling_matrix_present_flag);
#endif
		bit_pos++;
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		if (seq_scaling_matrix_present_flag)
		{
			for (i=0; i<8; i++)
			{
				memset(temp, 0x00, 8);
				_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
				seq_scaling_list_present_flag[i] = (temp[0] & 0x80) ? 1:0;
#if SPS_PRINT_MSG
				printf("byte(%d %02x) bit(%d) seq_scaling_list_present_flag[%d] - %d\n", byte_pos, *p, bit_pos, i, seq_scaling_list_present_flag[i]);
#endif
				bit_pos++;
				while(bit_pos >= 8)
				{
					bit_pos -= 8;
					p++;
					byte_pos++;
				}

				if (seq_scaling_list_present_flag[i])
				{
					if (i<6)
					{
						lastScale = 8;
						nextScale = 8;
						for (j=0; j<16; j++)
						{
							if (nextScale != 0)
							{
								memset(temp, 0x00, 8);
								_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
								now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &delta_scale);
								delta_scale = _mrtpsrc_get_sev_from_codenum(delta_scale);
#if 0
								{
									int m, r;

									m = delta_scale / 2;
									r = delta_scale % 2;
									delta_scale = m+r;
									if (r == 0) { delta_scale *= (-1); }
								}
#endif
								bit_pos += (now_uev_digit*2+1);
								while(bit_pos >= 8)
								{
									bit_pos -= 8;
									p++;
									byte_pos++;
								}
								nextScale = (lastScale+delta_scale+256)%256;
								UseDefaultScalingMatrix4x4Flag[i] = (j==0 && nextScale==0);
							}
							ScalingList4x4[i][j] = (nextScale==0) ? lastScale:nextScale;
							lastScale = ScalingList4x4[i][j];
						}
					}
					else
					{
						lastScale = 8;
						nextScale = 8;
						for (j=0; j<64; j++)
						{
							if (nextScale != 0)
							{
								memset(temp, 0x00, 8);
								_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
								now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &delta_scale);
								delta_scale = _mrtpsrc_get_sev_from_codenum(delta_scale);
#if 0
								{
									int m, r;

									m = delta_scale / 2;
									r = delta_scale % 2;
									delta_scale = m+r;
									if (r == 0) { delta_scale *= (-1); }
								}
#endif
								bit_pos += (now_uev_digit*2+1);
								while(bit_pos >= 8)
								{
									bit_pos -= 8;
									p++;
									byte_pos++;
								}
								nextScale = (lastScale+delta_scale+256)%256;
								UseDefaultScalingMatrix8x8Flag[i-6] = (j==0 && nextScale==0);
							}
							ScalingList8x8[i-6][j] = (nextScale ==0) ? lastScale:nextScale;
							lastScale = ScalingList8x8[i-6][j];
						}
					}
				}
			}
		}
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(temp, &log2_max_frame_num_minus4);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) log2_max_frame_num - %d\n", byte_pos, *p, bit_pos, log2_max_frame_num_minus4+4);
#endif
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >=8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(temp, &pic_order_cnt_type);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) pic_order_cnt_type - %d\n", byte_pos, *p, bit_pos, pic_order_cnt_type);
#endif
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	if (pic_order_cnt_type == 0)
	{
		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(temp, &log2_max_pic_order_cnt_lsb_minus4);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) log2_max_pic_order_cnt_lsb - %d\n", byte_pos, *p, bit_pos, log2_max_pic_order_cnt_lsb_minus4 + 4);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}
	}
	else if (pic_order_cnt_type == 1)
	{
		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		delta_pic_order_always_zero_flag = (temp[0]&0x80) ? 1:0;
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) delta_pic_order_always_zero_flag - %d\n", byte_pos, *p, bit_pos, delta_pic_order_always_zero_flag);
#endif
		bit_pos++;
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &offset_for_non_ref_pic);
		offset_for_non_ref_pic = _mrtpsrc_get_sev_from_codenum(offset_for_non_ref_pic);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) offset_for_non_ref_pic - %d\n", byte_pos, *p, bit_pos, offset_for_non_ref_pic);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &offset_for_top_to_bottom_field);
		offset_for_top_to_bottom_field = _mrtpsrc_get_sev_from_codenum(offset_for_top_to_bottom_field);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) offset_for_top_to_bottom_field - %d\n", byte_pos, *p, bit_pos, offset_for_top_to_bottom_field);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		memset(temp, 0x00, 8);
		_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
		now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &num_ref_frames_in_pic_order_cnt_cycle);
#if SPS_PRINT_MSG
		printf("byte(%d %02x) bit(%d) num_ref_frames_in_pic_order_cnt_cycle - %d\n", byte_pos, *p, bit_pos, num_ref_frames_in_pic_order_cnt_cycle);
#endif
		bit_pos += (now_uev_digit*2+1);
		while(bit_pos >= 8)
		{
			bit_pos -= 8;
			p++;
			byte_pos++;
		}

		for (i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++)
		{
			memset(temp, 0x00, 8);
			_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
			now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &offset_for_ref_frame);
			offset_for_ref_frame = _mrtpsrc_get_sev_from_codenum(offset_for_ref_frame);
#if SPS_PRINT_MSG
			printf("byte(%d %02x) bit(%d) offset_for_ref_frame - %d\n", byte_pos, *p, bit_pos, offset_for_ref_frame);
#endif
			bit_pos += (now_uev_digit*2+1);
			while(bit_pos >= 8)
			{
				bit_pos -= 8;
				p++;
				byte_pos++;
			}
		}
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &num_ref_frames);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) num_ref_frames - %d\n", byte_pos, *p, bit_pos, num_ref_frames);
#endif
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	gaps_in_frame_num_value_allowed_flag = (temp[0]&0x80) ? 1:0;
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) gaps_in_frame_num_value_allowed_flag - %d\n", byte_pos, *p, bit_pos, gaps_in_frame_num_value_allowed_flag);
#endif
	bit_pos++;
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &pic_width_in_mbs_minus1);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) pic_width_in_mbs - %d\n", byte_pos, *p, bit_pos, pic_width_in_mbs_minus1 + 1);
#endif
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &pic_height_in_map_units_minus1);
#if SPS_PRINT_MSG
	printf("byte(%d %02x) bit(%d) pic_height_in_map_units_minus1 - %d\n", byte_pos, *p, bit_pos, pic_height_in_map_units_minus1 + 1);
#endif
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}


#if SPS_PRINT_MSG
	printf("PIC WIDTH : %d\n", (pic_width_in_mbs_minus1+1)*16);
	printf("PIC HEIGHT: %d\n", (pic_height_in_map_units_minus1+1)*16);
#endif

	*width = (pic_width_in_mbs_minus1+1)*16;
	*height = (pic_height_in_map_units_minus1+1)*16;
}

#if 0
static int get_frame_seq_from_slice(unsigned char* src, int *seq_num)
{
	guchar* p = NULL;
	guchar temp[8];
	//int i,j;
	int now_uev_digit = 0;
	int byte_pos = 0;
	int bit_pos = 0;


	int first_mb_in_slice;
	int slice_type;
	int pic_parameter_set_id;
	//int frame_num;


	/* P frame only */
	if ((*(src+4) & 0x1f) != 1) { return 0; }

	p = src+5;

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &first_mb_in_slice);
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &slice_type);
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);
	now_uev_digit = _mrtpsrc_get_uev_codenum(&temp[0], &pic_parameter_set_id);
	bit_pos += (now_uev_digit*2+1);
	while(bit_pos >= 8)
	{
		bit_pos -= 8;
		p++;
		byte_pos++;
	}

	memset(temp, 0x00, 8);
	_mrtpsrc_get_4bytes_from_bits(p, bit_pos, &temp[0]);

	*seq_num = temp[0]>>4;

	return 1;
}
#endif


static void _mrtpsrc_set_stream_resolution(MRTPSRC_STREAM_T *lst, gint w, gint h)
{
	guint old_resol = lst->resolution;
	guint new_resol;

	if (w==1920 && h==1088)      { new_resol = NF_RES_1920x1080; }
    else if (w==1920 && h==1080) { new_resol = NF_RES_1920x1080; }
	else if (w==1280 && h==720)  { new_resol = NF_RES_1280x720; }
	else if (w==1280 && h==736)  { new_resol = NF_RES_1280x720; }
	else if (w==640 && h==368)   { new_resol = NF_RES_640x360; }
	else if (w==640 && h==360)   { new_resol = NF_RES_640x360; }
	else if (w==640 && h==480)   { new_resol = NF_RES_640x480; }

	else if (w==352 && h==240)   { new_resol = NF_RES_NTSC_CIF; }
	else if (w==352 && h==256)   { new_resol = NF_RES_NTSC_CIF; }
	else if (w==352 && h==288)   { new_resol = NF_RES_PAL_CIF; }
	else if (w==640 && h==352)   { new_resol = NF_RES_640x352; }
	else if (w==640 && h==400)   { new_resol = NF_RES_640x400; }
	else if (w==704 && h==480)   { new_resol = NF_RES_NTSC_4CIFP; }
	else if (w==704 && h==576)   { new_resol = NF_RES_PAL_4CIFP; }
	else if (w==720 && h==480)   { new_resol = NF_RES_720x480; }
	else if (w==720 && h==576)   { new_resol = NF_RES_720x576; }
	else if (w==800 && h==450)   { new_resol = NF_RES_800x450; }
	else if (w==800 && h==608)   { new_resol = NF_RES_800x600; }
	else if (w==1024 && h==768)  { new_resol = NF_RES_1024x768; }
	else if (w==1280 && h==1024) { new_resol = NF_RES_1280x1024; }
	else if (w==1440 && h==912)  { new_resol = NF_RES_1440x900; }
	else if (w==1600 && h==1200) { new_resol = NF_RES_1600x1200; }
	else if (w==320 && h==176)   { new_resol = NF_RES_320x180; }
	else if (w==320 && h==192)   { new_resol = NF_RES_320x180; }

	else if (w==2304 && h==1296) { new_resol = NF_RES_2304x1296; }
	else if (w==2048 && h==1536) { new_resol = NF_RES_2048x1536; }
	else if (w==2560 && h==1440) { new_resol = NF_RES_2560x1440; }
	else if (w==2592 && h==1520) { new_resol = NF_RES_2592x1520; }

	else if (w==2688 && h==1520) { new_resol = NF_RES_2688x1520; }
	else if (w==2560 && h==1600) { new_resol = NF_RES_2560x1600; }

	else if (w==2560 && h==1920) { new_resol = NF_RES_2560x1920; }
	else if (w==2592 && h==1920) { new_resol = NF_RES_2592x1920; }

	else if (w==2592 && h==1952) { new_resol = NF_RES_2592x1944; }
	else if (w==2992 && h==1680) { new_resol = NF_RES_2992x1680; }

	else if (w==2880 && h==1808) { new_resol = NF_RES_2880x1800; }
	else if (w==3200 && h==1808) { new_resol = NF_RES_3200x1800; }

	else if (w==2880 && h==2160) { new_resol = NF_RES_2880x2160; }
	else if (w==3072 && h==2048) { new_resol = NF_RES_3072x2048; }

	else if (w==3200 && h==2400) { new_resol = NF_RES_3200x2400; }
	else if (w==3840 && h==2160) { new_resol = NF_RES_3840x2160; }

	else if (w==960 && h==480) { new_resol = NF_RES_960H_NTSC_4CIFP; }
	else if (w==960 && h==576) { new_resol = NF_RES_960H_PAL_4CIFP; }

	else if (w==704&& h==240) { new_resol = NF_RES_NTSC_2CIF; }
	else if (w==704&& h==288) { new_resol = NF_RES_PAL_2CIF; }

	else if (w==1920&& h==1440) { new_resol = NF_RES_1920x1440; }
	else if (w==1920&& h==1536) { new_resol = NF_RES_1920x1536; }
	else if (w==1344&& h==1520) { new_resol = NF_RES_1344x1520; }
	else if (w==1296&& h==1944) { new_resol = NF_RES_1296x1944; }
	else if (w==1280&& h==1440) { new_resol = NF_RES_1280x1440; }
	else if (w==1024&& h==1536) { new_resol = NF_RES_1024x1536; }
	else if (w==1280&& h==960) { new_resol = NF_RES_1280x960; }

	else if (w==3000&& h==3000) { new_resol = NF_RES_3000x3000; }
	else if (w==2048&& h==2048) { new_resol = NF_RES_2048x2048; }
	else if (w==1280&& h==1280) { new_resol = NF_RES_1280x1280; }
	else if (w==640&& h==640) { new_resol = NF_RES_640x640; }
	else if (w==320&& h==320) { new_resol = NF_RES_320x320; }

	else if (w==360&& h==640) { new_resol = NF_RES_360x640; }
	else if (w==368&& h==640) { new_resol = NF_RES_360x640; }
	else if (w==480&& h==640) { new_resol = NF_RES_480x640; }
	else if (w==480&& h==704) { new_resol = NF_RES_480x704; }
	else if (w==576&& h==704) { new_resol = NF_RES_576x704; }
	else if (w==720&& h==1280) { new_resol = NF_RES_720x1280; }
	else if (w==736&& h==1280) { new_resol = NF_RES_720x1280; }
	else if (w==768&& h==1024) { new_resol = NF_RES_768x1024; }
	else if (w==1024&& h==1280) { new_resol = NF_RES_1024x1280; }
	else if (w==1080&& h==1920) { new_resol = NF_RES_1080x1920; }
	else if (w==1088&& h==1920) { new_resol = NF_RES_1080x1920; }
	else if (w==1536&& h==2048) { new_resol = NF_RES_1536x2048; }
	else if (w==1296&& h==2304) { new_resol = NF_RES_1296x2304; }
	else if (w==1520&& h==2592) { new_resol = NF_RES_1520x2592; }
	else if (w==1952&& h==2592) { new_resol = NF_RES_1944x2592; }
	else if (w==2160&& h==3840) { new_resol = NF_RES_2160x3840; }

	else                         { new_resol = 0; }

	if ((new_resol != 0) && (old_resol != new_resol))
	{
		gst_mrtp_src_resolution_changed(lst, old_resol, new_resol);
		lst->resolution = new_resol;
	}
}

#if 0
static unsigned int _check_xor(unsigned int size, unsigned int* dat_buf)
{
	guint i=0, frame_size=0;
	guint xor_result =0;

	frame_size = size - 12;

	for(i=frame_size; i >3; i -= 4) {
		xor_result ^= *(dat_buf++);
	}

	if(i > 0)
	{
		if (i == 1)
			xor_result ^= ( *(dat_buf) & 0x000000FF );
		else if (i == 2)
			xor_result ^= ( *(dat_buf) & 0x0000FFFF );
		else if (i == 3)
			xor_result ^= *(dat_buf) & 0x00FFFFFF;
		else
			printf("naver to be here\n");
	}

	return xor_result;
}
#endif


#endif //__GST_MRTP_VIDEO_C_
