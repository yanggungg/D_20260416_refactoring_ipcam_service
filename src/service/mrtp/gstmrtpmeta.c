/*
 * ITX Security Co.,Ltd.
 *  System software section
 *
 *  2011-07-13 jykim
 */

#ifndef _GST_MRTP_META_C__
#define _GST_MRTP_META_C__


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
#include <nf_codec_header.h>	// FIXME

//static gint meta_build(MRTPSRC_STREAM_T*);
static const gchar *_endline_str = "\r\n\r\n";
static void _mrtpsrc_send_rtcp(MRTPSRC_STREAM_T *lst);

#define MRTPSRC_META_RESIZE_SLACK (1024)

/* Safe payload length calculator for interleaved RTSP data (metadata path) */
static gint calc_payload_size_meta(guint16 rtsp_len, guchar* payload, guchar* rtp_start)
{
	gint payload_off = (gint)(payload - rtp_start);
	if (payload_off < 0)
	{
		return -1;
	}
	return (gint)rtsp_len - payload_off;
}

#ifndef NMZ_STANDLONE_MODE
extern GobjMrtpSrc *gst_mrtp_src_get_object(void);
#endif
extern void *memmem(const void*,size_t,const void*,size_t);

extern gint mrtpsrc_parse_metadata(MRTPSRC_STREAM_T* lst)
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
 	MRTPSRC_CHANNEL_T *lch = NULL;
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *filter = NULL;
#endif

	const char RTSP[] = "RTSP";

#if MRTPSRC_USE_STATISTICS
	stats = mrtpsrc_get_stat_instance();
#endif
#ifndef NMZ_STANDLONE_MODE
	filter = gst_mrtp_src_get_object();
#endif
	lch = mrtpsrc_get_channel(lst->ch_num);
	scan_ptr = lst->rbuf.cp;
//	printf("\e[31m #############################################################################################\n\e[0m\n\e[0m");

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
				MRTPSRC_DBG(ERROR, "%s %d| Wrong magic number on CH(%d,%s) (%02x, %02x, %04u), RTP sync corrupted",
						__FUNCTION__, __LINE__,
						lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], rtsp->magic, rtsp->channel, rtsp->len);

				lch = mrtpsrc_get_channel(lst->ch_num);
				lch->state = STATE_RECONN_REQ;

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
			MRTPSRC_CHANNEL_T *lch = NULL;
			guchar *next_rtsp = NULL;
			gint rtsp_dummy_len = 0;

			if ((next_rtsp = memmem(scan_ptr, lst->rbuf.lp - scan_ptr, _endline_str, 4)) != NULL)
			{
				next_rtsp += 4;
				rtsp = (MRTPSRC_RTSP_IF_T*) next_rtsp;
				rtsp_dummy_len = next_rtsp - scan_ptr;
				if (next_rtsp >= lst->rbuf.lp)
				{
#if 1
					total_consumed_len += rtsp_dummy_len;
					lst->rbuf.cp += rtsp_dummy_len;
					lst->rbuf.buf_used -= rtsp_dummy_len;
					lst->rbuf.buf_remain += rtsp_dummy_len;
#endif
					scan_ptr = next_rtsp;
					continue;
				}
				if (rtsp->magic == 0x24 || memmem(next_rtsp, 4, RTSP, 4))
				{
#if 1
					total_consumed_len += rtsp_dummy_len;
					lst->rbuf.cp += rtsp_dummy_len;
					lst->rbuf.buf_used -= rtsp_dummy_len;
					lst->rbuf.buf_remain += rtsp_dummy_len;
#endif
					scan_ptr = next_rtsp;
					continue;
					//return rtsp_dummy_len;
				}
			}
			MRTPSRC_DBG(ERROR, "%s | Wrong magic number on CH(%d,%s) (%02x, %02x, %04u), RTP sync corrupted",
					__FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], rtsp->magic, rtsp->channel, rtsp->len);

			lch = mrtpsrc_get_channel(lst->ch_num);
			lch->state = STATE_RECONN_REQ;

#if MRTPSRC_USE_STATISTICS
			stats->reconn_wrong_magic[0][lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
			if (filter == NULL) { return RTN_FAIL; }
			if (filter->cmd_callback == NULL) { return RTN_FAIL; }
			filter->cmd_callback(2, lst->ch_num, 0, 103/*magic A*/, filter->cmd_user_data);
#endif
			return RTN_FAIL;
#endif
		}

		align_cp = (guchar*) &rtsp->len;
		rtsp_len = *(align_cp + 1) | *align_cp << 8;

		if (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len > lst->rbuf.lp)
		{
			break;
		}

#if 0
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
#endif

		if ((rtsp->channel % 2) == 0)
		{
			/* Metadata frame */
			rtp = (MRTPSRC_RTP_HEADER_T*) (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T));
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
				/* metadata frame build start */
				//gint rough_len = ((guchar *)rtsp - lst->rbuf.cp) + rtsp_len + sizeof(MRTPSRC_RTSP_IF_T);
				gint rough_len = 1024*20;	// FIXME 
				gint meta_len = 0;
				gint current_meta_len = 0;
				gint tmp_total_meta_len = 0;

				guchar* start_ptr = lst->rbuf.cp;
				guchar* metadata_str = (guchar*) mrtpsrc_alloc_heap(rough_len, __FILE__, __FUNCTION__, __LINE__);
				if (metadata_str == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Memory allocation failed for metadata string", __FUNCTION__);
					return RTN_FAIL;
				}
				memset(metadata_str, 0x00, rough_len);

				consume_len = 0;
				while(start_ptr + sizeof(MRTPSRC_RTSP_IF_T) < lst->rbuf.lp)
				{
					rtsp = (MRTPSRC_RTSP_IF_T*) start_ptr;
					align_cp = (guchar*) &rtsp->len;
					rtsp_len = *(align_cp + 1) | *align_cp << 8;

					rtp = (MRTPSRC_RTP_HEADER_T*) (start_ptr + sizeof(MRTPSRC_RTSP_IF_T));
					if (rtp->x)
					{
						rtp_x = (MRTPSRC_RTP_HEADER_X_T*) (start_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T));
					}
					else
					{
						rtp_x = NULL;
					}

					payload = start_ptr + sizeof(MRTPSRC_RTSP_IF_T) + sizeof(MRTPSRC_RTP_HEADER_T);
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

					/* for rtcp - SSRC mismatch check */
					if((lst->sr_ssrc != ntohl(rtp->ssrc) && lst->sr_ssrc != 0))
					{
						MRTPSRC_DBG(WARN, "%s %d | ch_s(%d, META) - rtp ssrc is mismatched.(now:%0X, ori:%0X) - Request reconnection\n",
								__FUNCTION__, __LINE__, lst->ch_num, ntohl(rtp->ssrc), lst->sr_ssrc);
						lch->state = STATE_RECONN_REQ;
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, metadata_str);
						return RTN_FAIL;
					}

					/* update ssrc/seq */
					{
						lst->sr_ssrc = ntohl(rtp->ssrc);
						lst->seq_last = ntohs(rtp->seq);
						if (lst->seq_last == 0) lst->seq_group++;
					}

					current_meta_len = calc_payload_size_meta(rtsp_len, payload, (guchar*)rtp);
					if (current_meta_len <= 0)
					{
						MRTPSRC_DBG(WARN, "%s | ch(%d,META) invalid meta len(%d) seq(%u)", __FUNCTION__,
								lst->ch_num, current_meta_len, ntohs(rtp->seq));
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, metadata_str);
						return RTN_FAIL;
					}
					tmp_total_meta_len += current_meta_len;

					if(tmp_total_meta_len > rough_len)
					{
						gint new_len = rough_len + tmp_total_meta_len + MRTPSRC_META_RESIZE_SLACK;
						guchar* new_buf = (guchar*) mrtpsrc_alloc_heap(new_len, __FILE__, __FUNCTION__, __LINE__);
						if (new_buf == NULL)
						{
							mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, metadata_str);
							return RTN_FAIL;
						}
						memcpy(new_buf, metadata_str, meta_len);
						memset(new_buf + meta_len, 0x00, new_len - meta_len);
						mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, metadata_str);
						metadata_str = new_buf;
						rough_len = new_len;
					}
					memcpy(metadata_str + meta_len, payload, current_meta_len);

					meta_len = tmp_total_meta_len;

					consume_len += rtsp_len + sizeof(MRTPSRC_RTSP_IF_T);
					start_ptr += rtsp_len + sizeof(MRTPSRC_RTSP_IF_T);

					if(rtp->m != 0)
					{
						break;
					}
				}

				lst->itlv_rtp_ch = rtsp->channel;
				lst->itlv_rtcp_ch = rtsp->channel + 1;

#ifndef NMZ_STANDLONE_MODE
				if(filter != NULL)
				{
					if(filter->onvif_meta_callback != NULL)
					{
						filter->onvif_meta_callback(lst->ch_num, metadata_str, meta_len, filter->onvif_meta_user_data);
					}
				}
#endif
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, metadata_str);

				total_consumed_len += consume_len;
				lst->rbuf.cp += consume_len;
				lst->rbuf.buf_remain += consume_len;
				lst->rbuf.buf_used -= consume_len;

				scan_ptr = lst->rbuf.cp;

				if(lst->state == STATE_STREAM_READY)
				{
					lst->state = STATE_PLAYING;
				}

				continue;
				/* metadata channel end */

				/* rtp marker bit set end */
			}
		/* rtsp channel 0 end */
		}
		else
		{
			//printf("ch 1 payload %02x %02x %02x %02x\n", *scan_ptr, *(scan_ptr+1), *(scan_ptr+2), *(scan_ptr+3));
#if 0
			int i=0;
			printf("rtcp 0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f");
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
#endif
			/* metadata rtcp channel start */
			consume_len = rtsp_len + sizeof(MRTPSRC_RTSP_IF_T);


			MRTPSRC_RTCP_HEADER_T *rtcp = (MRTPSRC_RTCP_HEADER_T*) (scan_ptr + sizeof(MRTPSRC_RTSP_IF_T));

			lst->itlv_rtcp_ch = rtsp->channel;
			if (rtcp->pt == 200) // if no sender report, skip.
			{
				lst->ts_last_sr = ((ntohl(rtcp->ts_msw)&0xffff)<<16)|((ntohl(rtcp->ts_lsw)&0xffff0000)>>16);
				//lst->ts_last_sr = ntohs(rtcp->ts_msw);
				_mrtpsrc_send_rtcp(lst);
			}
			total_consumed_len += consume_len;

			{
				int mv_len = lst->rbuf.lp - (scan_ptr + consume_len);
				char* tmp = (char*) mrtpsrc_alloc_heap(mv_len, __FILE__, __FUNCTION__, __LINE__);
				if (tmp == NULL)
				{
					// MRTPSRC_DBG(ERROR, "%s | Memory allocation failed for metadata rtcp channel", __FUNCTION__);
					return RTN_FAIL;
				}
				memset(tmp, 0x00, mv_len);
				memcpy(tmp, scan_ptr + consume_len, mv_len);
				memcpy(scan_ptr, tmp, mv_len);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, tmp);
			}
			//lst->rbuf.cp += consume_len;
			lst->rbuf.lp -= consume_len;
			lst->rbuf.buf_remain += consume_len;
			lst->rbuf.buf_used -= consume_len;

			scan_ptr = lst->rbuf.cp;

			//scan_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
			continue;

			/* metadata rtcp channel end */
		}

		scan_ptr += (sizeof(MRTPSRC_RTSP_IF_T) + rtsp_len);
	} // end while

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

#endif	// _GST_MRTP_META_C__
