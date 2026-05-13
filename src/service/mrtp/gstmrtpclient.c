/*
 * ITX Security Co.,Ltd.
 *  System software section
 *
 *  2010-02-21 jykim
 *  2010-04-01 jykim
 *    Ver.0.1 - NCx camera interact, DM6467 host adapted
 *  2010-07-07 jykim
 *    Ver.0.2 - Refactoring, Axis camera interact, External access interface modified
 *  2010-11-09 jykim
 *    Ver.0.3 - Timestamp for NCx cameras
 *  2011-01-10 jykim
 *    Ver.0.4 - icodec header added
 */

#ifndef __GST_MRTP_CLIENT_C_
#define __GST_MRTP_CLIENT_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <arpa/inet.h>
#ifndef NMZ_STANDLONE_MODE
#include <glib.h>
#endif

#include <sys/time.h>

#include "gstmrtpdefs.h"
#include "gobjmrtpsrc.h"
#include "SEED_KISA.h"

// #ifndef NMZ_STANDLONE_MODE
// #include <gst/nf/gstnfbuddybuffer.h>
// #else
// #include <gstnfbuddybuffer.h>
// #endif
#include <gobjbuddybuffer.h>
#include "nf_codec_header.h"	// FIXME



#define PRINT_RTSP_MSG		(0)


static void* ring_sptr[MRTPSRC_MAX_CH*2];
static void* ring_eptr[MRTPSRC_MAX_CH*2];

GTimeVal mrtp_now_time;
GTimeVal recv_time;
#if MRTPSRC_USE_STATISTICS
static GTimeVal prev_time;
static GTimeVal prev_time2;
static MRTPSRC_STREAM_STAT_T stats;
#endif

static guint rtspc_errno;
static gint check_connectivity[MRTPSRC_MAX_CH];
#if MRTPSRC_SUPPORT_3RD_STREAM
static MRTPSRC_STREAM_T vstreams[MRTPSRC_MAX_CH*3];
#else
static MRTPSRC_STREAM_T vstreams[MRTPSRC_MAX_CH*2];
#endif
static MRTPSRC_STREAM_T astreams[MRTPSRC_MAX_CH];
static MRTPSRC_STREAM_T mstreams[MRTPSRC_MAX_CH];
static MRTPSRC_CHANNEL_T channels[MRTPSRC_MAX_CH];

static GMutex* nf_mrtpsrc_cs_mtx;
//static pthread_t receive_th;
static GThread* receive_th;
//static pthread_mutex_t list_modify_mtx = PTHREAD_MUTEX_INITIALIZER;
static GMutex* list_modify_mtx;
//static pthread_mutex_t audio_data_mtx = PTHREAD_MUTEX_INITIALIZER;
static GMutex* audio_data_mtx;
//static pthread_mutex_t rec_ho_q_mtx = PTHREAD_MUTEX_INITIALIZER;
static GMutex* rec_ho_q_mtx;
static void _receive_th_func(void *param);

//static pthread_t session_th;
static GThread* session_th;
static void _session_manager_func(void *param);

//static pthread_t handoff_th;
static GThread* handoff_th;
static void _rec_ho_func(void *param);

static size_t b64_decode_binary_to_buffer(guchar *p_dst, size_t i_dst, const char *p_src);
static gint ring_load(MRTPSRC_STREAM_T*);
static gint ring_load_rtcp(MRTPSRC_STREAM_T*);
//static gint internal_close_stream(MRTPSRC_STREAM_T*);
static gint send_audio_to(gint);

static void get_rtsp_digest_auth_str(char*,char*,char*,char*,char*,char*,char*);

static void _mrtpsrc_send_rtcp(MRTPSRC_STREAM_T*);
static gchar* _mrtp_get_endline(gchar* src, gint* len);
static void send_rtsp_validation(MRTPSRC_STREAM_T*);
static gint revalidate[MRTPSRC_MAX_CH];

static MRTPSRC_RECORD_HO_Q ho_head;

static GAsyncQueue *close_sync_queue = NULL;
static guint parsing_state[MRTPSRC_MAX_CH][STREAM_MAX];

#define START_RTPPORT	2500
#define END_RTPPORT		65000
static GMutex* port_number_mtx;
static guint port_num = START_RTPPORT;
static guint get_port_number(void)
{
	guint rtn;
	g_mutex_lock(port_number_mtx);
	rtn = port_num;
	port_num += 2;
	if(port_num > END_RTPPORT) port_num = START_RTPPORT;
	g_mutex_unlock(port_number_mtx);
	return rtn;
}
static void bind_rtp_unicast(MRTPSRC_STREAM_T*);
static void bind_rtp_multicast(MRTPSRC_STREAM_T*);
static gchar bind_udp_socket(guint port, gint* sock);
static void get_stream_resolution(MRTPSRC_STREAM_T *lst, int *w, int *h);

//connectivity time modify
static struct channel_connectivity {

	struct timespec err_startTime[STREAM_MAX];
	struct timespec dbg_checkTime[STREAM_MAX];
	float err_elapsedTime[STREAM_MAX];

};

static struct channel_connectivity g_conn_table[MRTPSRC_MAX_CH];
static void initialize_all_channels_connectivity();
static void reinitialize_channel_stream_connectivity(int ch, int stream_no);
static void connectivity_time_checker(MRTPSRC_STREAM_T *lst, MRTPSRC_CHANNEL_T *lch);
static gint _mrtpsrc_check_and_recover_rbuf(MRTPSRC_STREAM_T *lst, gint ch, gint stream);

#ifndef NMZ_STANDLONE_MODE
extern GobjMrtpSrc *gst_mrtp_src_get_object(void);
#endif
extern void mrtpsrc_get_resolution_from_sps(gchar* src, gint *width, gint *height);
extern void mrtpsrc_set_stream_resolution(MRTPSRC_STREAM_T *lst, gint w, gint h);

extern void mrtpsrc_closesync_lock(void)
{
	g_mutex_lock(nf_mrtpsrc_cs_mtx);
}

extern void mrtpsrc_closesync_unlock(void)
{
	g_mutex_unlock(nf_mrtpsrc_cs_mtx);
}

extern void mrtpsrc_frame_q_lock(void)
{
	g_mutex_lock(list_modify_mtx);
}
extern void mrtpsrc_frame_q_unlock(void)
{
	g_mutex_unlock(list_modify_mtx);
}
extern void mrtpsrc_tx_audio_lock(void)
{
	g_mutex_lock(audio_data_mtx);
}
extern void mrtpsrc_tx_audio_unlock(void)
{
	g_mutex_unlock(audio_data_mtx);
}
extern void rec_ho_q_lock(void)
{
	g_mutex_lock(rec_ho_q_mtx);
}
extern void rec_ho_q_unlock(void)
{
	g_mutex_unlock(rec_ho_q_mtx);
}
extern guint mrtpsrc_get_errno(void)
{
	return rtspc_errno;
}
extern GTimeVal mrtpsrc_get_captured_time(void)
{
	return mrtp_now_time;
}

static void mrtpsrc_set_parsing_state(guint ch_num, guint stream, guint progress)
{
	parsing_state[ch_num][stream] = progress;
}
static guint mrtpsrc_get_parsing_state(guint ch_num, guint stream)
{
	return parsing_state[ch_num][stream];
}

#if MRTPSRC_USE_STATISTICS
extern MRTPSRC_STREAM_STAT_T* mrtpsrc_get_stat_instance(void)
{
	return (MRTPSRC_STREAM_STAT_T*) &stats;
}
#endif

void gst_mrtp_src_rtsp_digest_auth_str(char* user, char* pass, char* realm, char* nonce, char* uri, char* method, char* result)
{
	get_rtsp_digest_auth_str(user, pass, realm, nonce, uri, method,	result);
}

static void initialize_all_channels_connectivity()
{
	memset(g_conn_table, 0x00, sizeof(struct channel_connectivity) * MRTPSRC_MAX_CH);
}

extern void mrtpsrc_rtsp_client_initialize(void)
{
	gint i = 0, j = 0;
	MRTPSRC_AUDIO_RING_T *aring = mrtpsrc_get_audio_ring();

	rtspc_errno = ERR_NO_ERROR;
#if MRTPSRC_SUPPORT_3RD_STREAM	
	memset(vstreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH * 3));
#else
	memset(vstreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH * 2));
#endif	
	memset(astreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH));
#if MRTPSRC_USE_STATISTICS
	memset(&stats, 0x00, sizeof(MRTPSRC_STREAM_STAT_T));
#endif
	memset(mstreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH));
	memset(aring, 0x00, (sizeof(MRTPSRC_AUDIO_RING_T) * MRTPSRC_MAX_CH));
	memset(check_connectivity, 0x00, sizeof(gint)*MRTPSRC_MAX_CH);
	memset(revalidate, 0x00, sizeof(gint)*MRTPSRC_MAX_CH);

	//connectivity static global values init
	initialize_all_channels_connectivity();

	for (i = 0; i < MRTPSRC_MAX_CH; i++)
		memset(parsing_state[i], 0x00, sizeof(guint)*STREAM_MAX);


	ho_head = (MRTPSRC_RECORD_HO_Q) mrtpsrc_alloc_heap(sizeof(struct _REC_HANDOFF_QUEUE_), __FILE__, __FUNCTION__, __LINE__);
	g_assert(ho_head != NULL);
	memset(ho_head, 0x00, sizeof(struct _REC_HANDOFF_QUEUE_));

	g_thread_init(NULL);
	close_sync_queue = g_async_queue_new();

	for (i = 0; i < MRTPSRC_MAX_CH; i++)
	{
		/* MRTPSRC_STREAM_T array init */
		for (j = STREAM_1ST; j < STREAM_AUDIO; j++)
		{
			vstreams[MRTPSRC_MAX_CH*j+i].ch_num = i;
			vstreams[MRTPSRC_MAX_CH*j+i].stream_num = j;
			if (j == STREAM_1ST)
			{
				vstreams[MRTPSRC_MAX_CH*j+i].buf_sz = 11 * MRTPSRC_RCV_BUF_SZ;
			}
			else if (j == STREAM_2ND)
			{
				vstreams[MRTPSRC_MAX_CH*j+i].buf_sz = 4 * MRTPSRC_RCV_BUF_SZ;
			}
#if MRTPSRC_SUPPORT_3RD_STREAM
			else if (j == STREAM_3RD)
			{
				vstreams[MRTPSRC_MAX_CH*j+i].buf_sz = 1 * MRTPSRC_RCV_BUF_SZ;
			}
#endif
#if 1
			//vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf = (gchar*) malloc(vstreams[MRTPSRC_MAX_CH*j+i].buf_sz + 16);
			vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf = (guchar*) mrtpsrc_alloc_heap(vstreams[MRTPSRC_MAX_CH*j+i].buf_sz + 32, __FILE__, __FUNCTION__, __LINE__);
			g_assert(vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf != NULL);
			vstreams[MRTPSRC_MAX_CH*j+i].rbuf.rbs = (guchar*)vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf;
			vstreams[MRTPSRC_MAX_CH*j+i].rbuf.rbe = (guchar*)(vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf + vstreams[MRTPSRC_MAX_CH*j+i].buf_sz - 1);
			ring_sptr[MRTPSRC_MAX_CH*j+i] = vstreams[MRTPSRC_MAX_CH*j+i].rbuf.rbs;
			ring_eptr[MRTPSRC_MAX_CH*j+i] = vstreams[MRTPSRC_MAX_CH*j+i].rbuf.rbe;
#else
			vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf = (GobjBuddyBuffer*) gst_buffer_try_new_and_alloc(vstreams[MRTPSRC_MAX_CH*j+i].buf_sz);
			g_assert(vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf != NULL);
			vstreams[MRTPSRC_MAX_CH*j+i].rbuf.rbs = GST_BUFFER_DATA(vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf);
			vstreams[MRTPSRC_MAX_CH*j+i].rbuf.rbe = GST_BUFFER_DATA(vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf) + vstreams[MRTPSRC_MAX_CH*j+i].buf_sz - 1;
#endif
		}

		astreams[i].ch_num = i;
		astreams[i].stream_num = STREAM_AUDIO;
		astreams[i].buf_sz = MRTPSRC_RCV_BUF_SZ;
#if 1
		//astreams[i].rbuf.buf = (gchar*) malloc(astreams[i].buf_sz + 16);
		astreams[i].rbuf.buf = (guchar*) mrtpsrc_alloc_heap(astreams[i].buf_sz + 32, __FILE__, __FUNCTION__, __LINE__);
		g_assert(astreams[i].rbuf.buf != NULL);
		astreams[i].rbuf.rbs = (guchar*)astreams[i].rbuf.buf;
		astreams[i].rbuf.rbe = (guchar*)(astreams[i].rbuf.buf + astreams[i].buf_sz - 1);
#else
		astreams[i].rbuf.buf = (GobjBuddyBuffer*) gst_buffer_try_new_and_alloc(astreams[i].buf_sz);
		g_assert(astreams[i].rbuf.buf != NULL);
		astreams[i].rbuf.rbs = GST_BUFFER_DATA(astreams[i].rbuf.buf);
		astreams[i].rbuf.rbe = GST_BUFFER_DATA(astreams[i].rbuf.buf) + astreams[i].buf_sz - 1;
#endif

		mstreams[i].ch_num = i;
		mstreams[i].stream_num = STREAM_META;
		mstreams[i].buf_sz = MRTPSRC_RCV_BUF_SZ/2;

		//mstreams[i].rbuf.buf = (gchar*) malloc(mstreams[i].buf_sz + 16);
		mstreams[i].rbuf.buf = (guchar*) mrtpsrc_alloc_heap(mstreams[i].buf_sz + 32, __FILE__, __FUNCTION__, __LINE__);
		g_assert(mstreams[i].rbuf.buf != NULL);
		mstreams[i].rbuf.rbs = (guchar*)mstreams[i].rbuf.buf;
		mstreams[i].rbuf.rbe = (guchar*)(mstreams[i].rbuf.buf + mstreams[i].buf_sz - 1);

		/* MRTPSRC_CHANNEL_T array init */
		channels[i].state = STATE_INITIAL;
		channels[i].ch_num = i;
		channels[i].model_code = 0;
		channels[i].active_video_cnt = 0;
		channels[i].is_audio_activated = 0;
		channels[i].is_metadata_on = 0;
		channels[i].stream_1 = &vstreams[i];
		channels[i].stream_2 = &vstreams[i+32];
#if MRTPSRC_SUPPORT_3RD_STREAM
		channels[i].stream_3 = &vstreams[i+32];
#endif
		channels[i].stream_a = &astreams[i];
		channels[i].stream_m = &mstreams[i];
	}

	nf_mrtpsrc_cs_mtx = g_mutex_new();
	list_modify_mtx = g_mutex_new();
	audio_data_mtx = g_mutex_new();
	port_number_mtx = g_mutex_new();
	rec_ho_q_mtx = g_mutex_new();
	//pthread_create(&receive_th, NULL, (void*)&receive_th_func, NULL);
	receive_th = g_thread_create((GThreadFunc)_receive_th_func,
									NULL, FALSE, NULL);

	//pthread_create(&session_th, NULL, (void*)&session_manager_func, NULL);
	session_th = g_thread_create((GThreadFunc)_session_manager_func,
									NULL, FALSE, NULL);

	//pthread_create(&handoff_th, NULL, (void*)&_rec_ho_func, NULL);
	handoff_th = g_thread_create((GThreadFunc)_rec_ho_func,
										NULL, FALSE, NULL);

    printf("\nmrtpsrc %s\n", __TIMESTAMP__);
}

extern void mrtpsrc_rtsp_client_finalize(void)
{
	int i, j;
	for (i = 0; i < MRTPSRC_MAX_CH; i++)
	{
		/* MRTPSRC_STREAM_T array free */
		for (j = STREAM_1ST; j < STREAM_AUDIO; j++)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf);
			//free(vstreams[MRTPSRC_MAX_CH*j+i].rbuf.buf);
		}
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, astreams[i].rbuf.buf);
		//free(astreams[i].rbuf.buf);
	}
#if MRTPSRC_SUPPORT_3RD_STREAM	
	memset(vstreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH * 3));
#else
	memset(vstreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH * 2));
#endif	
	memset(astreams, 0x00, (sizeof(MRTPSRC_STREAM_T) * MRTPSRC_MAX_CH));

	g_thread_exit(receive_th);
	g_thread_exit(session_th);

	g_mutex_free(nf_mrtpsrc_cs_mtx);
	g_mutex_free(list_modify_mtx);
	g_mutex_free(audio_data_mtx);
	g_mutex_free(port_number_mtx);
}


extern MRTPSRC_STREAM_T* mrtpsrc_get_stream(guint ch_num, guint stream)
{
#if 0
	if (stream != STREAM_AUDIO) return (&vstreams[(MRTPSRC_MAX_CH*stream)+ch_num]);
	else return (&astreams[ch_num]);
#endif
	switch(stream)
	{
		case STREAM_1ST:
		case STREAM_2ND:
#if MRTPSRC_SUPPORT_3RD_STREAM
		CASE STREAM_3RD:
#endif
			return (&vstreams[(MRTPSRC_MAX_CH*stream)+ch_num]);

		case STREAM_AUDIO:
			return (&astreams[ch_num]);

		case STREAM_META:
			return (&mstreams[ch_num]);

		default:
			break;
	}
	return (&astreams[ch_num]);
}

extern MRTPSRC_CHANNEL_T* mrtpsrc_get_channel(guint ch_num)
{
	return &channels[ch_num];
}

extern int get_live_channel_num(void)
{
	int i = 0;
	int num = 0;
	for (i = 0; i < MRTPSRC_MAX_CH; i++)
	{
		//if (channels[i].state == STATE_PLAYING)
		if (vstreams[i].state == STATE_PLAYING)
		{
			num++;
		}
	}

	return num;
}

extern void mrtpsrc_check_ring_ptr(void* ptr, int dch, const char* filename, const char* funcname, const int line)
{
	void* sptr = ring_sptr[dch];
	void* eptr = ring_eptr[dch];

	if (dch<0 || dch>31)
	{
		printf("%s | from(%s:%d - %s) already crashed\n", __FUNCTION__, filename, line, funcname);
		g_assert(0);
	}

	if (ptr < sptr || ptr > eptr)
	{
		printf("%s | from(%s:%d - %s) ptr(%p) sptr(%p) eptr(%p)\n",
				__FUNCTION__,
				filename, line, funcname, ptr, sptr, eptr
		);
		g_assert(0);
	}
}

extern int mrtpsrc_get_rtsp_msg_len_print(guchar* buf, gint maxlen)
{
	const gchar* RTSP = "RTSP";
	const gchar* f_validation_len = "Content-Length: ";

	gchar *str_buf = NULL;
	gchar *cl_s,*cl_e;
	gchar *last_pos;
	gchar cl_buf[8];
	gint cl = 0;

		{
			{
				int i=0;
				printf("     0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f");
				while (i < ((maxlen>80)?80:maxlen))
				{
					if (i % 16 == 0)
					{
						printf("\n%02x  ", i/16);
					}
					else if (i % 8 == 0)
					{
						printf(" ");
					}
					printf("%02x ", *(buf+i++));
				}
				printf("\n\n");
			}
		}
	gint el_len = 0;

	gchar *endline_ptr = NULL;
	gchar *startline_ptr = NULL;

	str_buf = (gchar*) mrtpsrc_alloc_heap(maxlen + 1, __FILE__, __FUNCTION__, __LINE__);
	if (str_buf == NULL)
	{
		printf("%s | cmem fail to alloc. (line:%d)\n", __FUNCTION__, __LINE__);
		return 0;
	}
	memset(str_buf, 0x00, (maxlen+1));
	memcpy(str_buf, buf, maxlen);

	/* rtsp validation response have to start with "RTSP" */
	if (g_strstr_len(str_buf, 4, RTSP) == NULL)
	{
		printf("\n\n\n\njykim%d\n", __LINE__);
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
		return 0;
	}

	/* get rtsp validation response content length */
	memset(cl_buf, 0x00, 8);
	cl_s = g_strstr_len(str_buf, maxlen, f_validation_len);
	cl_e = NULL;
	if (cl_s != NULL)
	{
		cl_s += strlen(f_validation_len);
		cl_e = _mrtp_get_endline(cl_s, &el_len);
		if (cl_e == NULL)
		{
		printf("\n\n\n\njykim%d\n", __LINE__);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
			return 0;
		}
		memcpy(cl_buf, cl_s, (cl_e-cl_s));
		cl = atoi(cl_buf);
	}

	/* find enline: "\r", "\n", "\r\n" */
	startline_ptr = str_buf;
	while(1)
	{
		endline_ptr = _mrtp_get_endline(startline_ptr, &el_len);
		if (endline_ptr == NULL)
		{
		printf("\n\n\n\njykim%d\n", __LINE__);
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
			return 0;
		}

		if (startline_ptr == endline_ptr)
		{
			last_pos = endline_ptr + el_len;
			break;
		}

		startline_ptr = endline_ptr + el_len;
	}

	if (cl_s != NULL && cl > 0)
	{
		last_pos += cl;
	}

	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);

		printf("\n\n\n\njykim%d\n", __LINE__);
	return (last_pos-str_buf);
}
extern int mrtpsrc_get_rtsp_msg_len(guchar* buf, gint maxlen)
{
	const gchar* RTSP = "RTSP";
	const gchar* f_validation_len = "Content-Length: ";

	gchar *str_buf = (gchar*) mrtpsrc_alloc_heap(maxlen + 1, __FILE__, __FUNCTION__, __LINE__);
	if (str_buf == NULL)
	{
		MRTPSRC_DBG(WARN, "%s | cmem fail to alloc. (line:%d)\n", __FUNCTION__, __LINE__);
		return 0;
	}
	gchar *cl_s,*cl_e;
	gchar *last_pos;
	gchar cl_buf[8];
	gint cl = 0;

	gint el_len = 0;
	gint const_el_len = 0;

	gchar *endline_ptr = NULL;
	gchar *startline_ptr = NULL;


	memset(str_buf, 0x00, (maxlen+1));
	memcpy(str_buf, buf, maxlen);

	/* rtsp validation response have to start with "RTSP" */
	if (g_strstr_len(str_buf, 4, RTSP) == NULL)
	{
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
		return 0;
	}

	/* get rtsp validation response content length */
	memset(cl_buf, 0x00, 8);
	cl_s = g_strstr_len(str_buf, maxlen, f_validation_len);
	cl_e = NULL;
	if (cl_s != NULL)
	{
		cl_s += strlen(f_validation_len);
		cl_e = _mrtp_get_endline(cl_s, &el_len);
		if (cl_e == NULL)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
			return 0;
		}
		memcpy(cl_buf, cl_s, (cl_e-cl_s));
		cl = atoi(cl_buf);
	}

	/* find enline: "\r", "\n", "\r\n" */
	startline_ptr = str_buf;
	while(1)
	{
		endline_ptr = _mrtp_get_endline(startline_ptr, &el_len);
		if (endline_ptr == NULL)
		{
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
			return 0;
		}

		if (const_el_len == 0)
		{
			const_el_len = el_len;
		}

		if (startline_ptr == endline_ptr)
		{
			if (const_el_len != el_len)
			{
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);
				return 0;
			}
			last_pos = endline_ptr + el_len;
			break;
		}

		startline_ptr = endline_ptr + el_len;
	}

	if (cl_s != NULL && cl > 0)
	{
		last_pos += cl;
	}

	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, str_buf);

	return (last_pos-str_buf);
}

extern guint mrtpsrc_pause_stream(guint ch_num, guint stream)
{
	gchar buf[MRTPSRC_NBUF_SZ];
	MRTPSRC_FRAME_T probe = NULL, probe_n = NULL;
	MRTPSRC_STREAM_T *lst = NULL;
	MRTPSRC_CHANNEL_T *lch = NULL;


	if (ch_num >= MRTPSRC_MAX_CH)
	{
		MRTPSRC_DBG(ERROR, "%s | Wrong parameter ch_num(%u)", __FUNCTION__, ch_num);
		rtspc_errno = ERR_TRDN_INVALID_CH;
		return RTN_FAIL;
	}

	if (stream >= STREAM_MAX)
	{
		MRTPSRC_DBG(ERROR, "%s | Wrong parameter stream(%u)", __FUNCTION__, stream);
		rtspc_errno = ERR_TRDN_INVALID_STREAM;
		return RTN_FAIL;
	}

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | ch_num(%u, %s)", __FUNCTION__, ch_num, MRTPSRC_STREAM_STR[stream]);
#endif

	lst = mrtpsrc_get_stream(ch_num, stream);
	lch = mrtpsrc_get_channel(ch_num);

	lch->audio_qlen = 0;
	{
		if (lst->state < STATE_STREAM_READY || lst->state > STATE_REBOOT_REQ)
		{
			MRTPSRC_DBG(WARN, "%s | channel[%u, %s] is not playing",
					__FUNCTION__, ch_num, MRTPSRC_STREAM_STR[stream]);
			rtspc_errno = ERR_TRDN_N_PLAYING_CH;
			goto channel_resource_free;
		}

		if (MRTPSRC_NEVER)
		{
#if MRTPSRC_FULL_DBG
			MRTPSRC_DBG(MAJOR, "%s | TEARDOWN cycle", __FUNCTION__);
#endif
			memset(buf, 0x00, MRTPSRC_NBUF_SZ);
			
			g_snprintf(buf, MRTPSRC_NBUF_SZ, 
						"TEARDOWN %s RTSP/1.0\r\n"
						"CSeq:%u\r\n"
						"Session: %s\r\n"
						"User-Agent:" MRTPSRC_USER_AGENT_STR "\r\n"
						"\r\n",
					lst->rtsp_addr,
					lst->req_seq++,
					lst->session);

			if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
			{
#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MAJOR, "%s | TEARDOWN request send fail", __FUNCTION__);
#endif
				goto channel_resource_free;
			}
		}

		mrtpsrc_ho_control_frame(lst, 1);

channel_resource_free:
		//reset
		reinitialize_channel_stream_connectivity(lst->ch_num, lst->stream_num);

		lst->state = STATE_PAUSED;

#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MINOR, "%s | socket descriptor(%d)", __FUNCTION__, lst->rtsp_sock);
#endif
		if (lst->rtsp_sock > 0)
		{
			close(lst->rtsp_sock);
			lst->rtsp_sock = (-1);
		}
		if(lst->rtp_protocol == RTP_UNICAST_UDP || lst->rtp_protocol == RTP_MULTICAST_UDP)
		{
#if MRTPSRC_FULL_DBG
			MRTPSRC_DBG(MINOR, "%s | udp socket descriptor(%d - %d)", __FUNCTION__, lst->udp_sock[0], lst->udp_sock[1]);
#endif
			if(lst->udp_sock[0] > 0)
			{
				close(lst->udp_sock[0]);
				lst->udp_sock[0] = (-1);
			}
			if(lst->udp_sock[1] > 0)
			{
				close(lst->udp_sock[1]);
				lst->udp_sock[1] = (-1);
			}
		}

#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MINOR, "%s | freeing remained frames", __FUNCTION__);
#endif
		if (mrtpsrc_get_parsing_state(ch_num, stream) == 1)
		{
			sleep(1);
		}

		mrtpsrc_frame_q_lock();
		probe = lst->frame_head;
		lst->frame_head = NULL;
		mrtpsrc_frame_q_unlock();

#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MAJOR, "%s | gop(%u), seq(%u)", __FUNCTION__,
				lst->gop, lst->gop_seq);
#endif
		while(probe != NULL)
		{
			if (probe->len)
			{
				probe->len = 0;
				if (probe->frame != NULL)
				{
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					probe->frame = NULL;
#if MRTPSRC_USE_STATISTICS
					stats.drop_cnt++;
#endif
				}
			}
			probe_n = probe->next;
			memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
			probe = probe_n;
		}

#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MINOR, "%s | free channel infos", __FUNCTION__);
#endif
		memset(lst->ctrl, 0x00, 128);
		memset(lst->sps, 0x00, 128);
		memset(lst->pps, 0x00, 128);
		memset(lst->session, 0x00, 128);
		memset(lst->rtsp_validation, 0x00, 16);
		memset(lst->rtsp_cb, 0x00, MRTPSRC_MAX_CB_LEN);
		revalidate[ch_num] = 0;

#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MINOR, "%s | initialize channel info", __FUNCTION__);
#endif
		//lst->resolution = 0;
		//lst->fps = 0;

		lst->ts_last_sendsec = 0;
		lst->ts_last_rcvd = 0;
		lst->ts_last_second = 0;
		lst->ts_last_5_mili = 0;
		lst->ts_last_remainder = 0;
		lst->ts_last_diff = 0;
		lst->ts_first_second = 0;
		lst->ts_first_5_mili = 0;
		lst->frame_num = 0;
		//lst->model = 0;
		lst->gop_seq = 0;
		lst->gop = 0;
		lst->gop_len = 0;
		lst->ring_full_cnt = 0;
		lst->rbuf.lp = NULL;
		lst->rbuf.cp = NULL;
		lst->rbuf.buf_remain = lst->buf_sz;
		lst->rbuf.buf_used = 0;
		lst->abuf.start = 0;
		lst->abuf.len = 0;
		lst->seed_video = 0;
		lst->seed_audio = 0;
		lst->seed_sps = 0;
		lst->seed_pps = 0;
		lst->seed_vps = 0;
		lst->sr_ssrc = 0;
		memset(lst->pbUserKey, 0x00, 16);
		memset(lst->realm, 0x00, 512);
		memset(lst->nonce, 0x00, 512);

		if (stream == STREAM_AUDIO)
		{
			MRTPSRC_AUDIO_RING_T *aring = mrtpsrc_get_audio_ring();
#if MRTPSRC_FULL_DBG
			MRTPSRC_DBG(MINOR, "%s | initialize audio ring buffer", __FUNCTION__);
#endif
			memset((void*)&aring[ch_num], 0x00, sizeof(MRTPSRC_AUDIO_RING_T));
		}
#if 0
		if (lst->rbuf.buf != NULL)
		{
			g_object_unref(lst->rbuf.buf);
			lst->rbuf.buf = NULL;
		}
#endif
	}
	rtspc_errno = ERR_NO_ERROR;
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | done", __FUNCTION__);
#endif
	return RTN_OK;
}

extern guint mrtpsrc_resume_stream(guint ch_num, guint stream)
{
	gchar rtsp_addr[MRTPSRC_MAX_CB_LEN];
	gchar user[64];
	gchar pass[64];
	guint rtn = 0;
	MRTPSRC_STREAM_T *lst = NULL;

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | ch_num(%u, %s)", __FUNCTION__, ch_num, MRTPSRC_STREAM_STR[stream]);
#endif

	if (ch_num >= MRTPSRC_MAX_CH)
	{
		MRTPSRC_DBG(ERROR, "%s | Wrong parameter ch_num(%u)", __FUNCTION__, ch_num);
		rtspc_errno = ERR_OPEN_INVALID_CH_NUM;
		return RTN_FAIL;
	}

	if (stream >= STREAM_MAX)
	{
		MRTPSRC_DBG(ERROR, "%s | Wrong parameter stream(%u)", __FUNCTION__, stream);
		rtspc_errno = ERR_OPEN_INVALID_STREAM;
		return RTN_FAIL;
	}

	lst = mrtpsrc_get_stream(ch_num, stream);

	if (lst->state != STATE_PAUSED)
	{
		MRTPSRC_DBG(ERROR, "%s | Channel state is not STATE_PAUSED", __FUNCTION__);
		rtspc_errno = ERR_OPEN_CH_DUP;
		return RTN_FAIL;
	}
/*
	if (stream == STREAM_META && lst->model < MODEL_ONVIF_GENERAL)
	{
		MRTPSRC_DBG(MINOR, "%s | ITX cam doesn't open metadata stream", __FUNCTION__);
		rtspc_errno = ERR_NO_ERROR;
		return RTN_OK;
	}
*/

	g_strlcpy(rtsp_addr, lst->rtsp_addr, MRTPSRC_MAX_CB_LEN);
	g_strlcpy(user, lst->username, 64);
	g_strlcpy(pass, lst->password, 64);

	rtn = mrtpsrc_open_stream(
					ch_num, stream, 
					lst->ip_addr, 
					lst->rtsp_port, 
					rtsp_addr, user, pass,
					lst->fps, 
					lst->resolution, 
					lst->model,
					lst->rtp_protocol,
					lst->en
				);
	if (rtn == RTN_FAIL)
	{
		MRTPSRC_DBG(ERROR, "%s | re-opening failed(%u)", __FUNCTION__, rtspc_errno);
		return RTN_FAIL;
	}

	return RTN_OK;
}

extern guint mrtpsrc_close_stream(guint ch_num, guint stream)
{
	gchar buf[MRTPSRC_NBUF_SZ];
	MRTPSRC_FRAME_T probe = NULL, probe_n = NULL;
	MRTPSRC_STREAM_T *lst = NULL;
	MRTPSRC_CHANNEL_T *lch = NULL;

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | ch_num(%u, %s)", __FUNCTION__, ch_num, MRTPSRC_STREAM_STR[stream]);
#endif

	if (ch_num >= MRTPSRC_MAX_CH)
	{
		MRTPSRC_DBG(ERROR, "%s | Wrong parameter ch_num(%u)", __FUNCTION__, ch_num);
		rtspc_errno = ERR_TRDN_INVALID_CH;
		return RTN_FAIL;
	}

	if (stream >= STREAM_MAX)
	{
		MRTPSRC_DBG(ERROR, "%s | Wrong parameter stream(%u)", __FUNCTION__, stream);
		rtspc_errno = ERR_TRDN_INVALID_STREAM;
		return RTN_FAIL;
	}

	lst = mrtpsrc_get_stream(ch_num, stream);
	lch = mrtpsrc_get_channel(ch_num);

	lch->audio_qlen = 0;
	{
		if (lst->state < STATE_STREAM_READY || lst->state > STATE_REBOOT_REQ)
		{
			MRTPSRC_DBG(WARN, "%s | channel[%u, %s] is not playing",
					__FUNCTION__, ch_num, MRTPSRC_STREAM_STR[stream]);
			rtspc_errno = ERR_TRDN_N_PLAYING_CH;
			goto channel_resource_free;
		}

		if (MRTPSRC_NEVER)
		{
#if MRTPSRC_FULL_DBG
			MRTPSRC_DBG(MAJOR, "%s | TEARDOWN cycle", __FUNCTION__);
#endif
			memset(buf, 0x00, MRTPSRC_NBUF_SZ);
			g_snprintf(buf, MRTPSRC_NBUF_SZ, 
						"TEARDOWN %s RTSP/1.0\r\n"
						"CSeq:%u\r\n"
						"Session: %s\r\n"
						"User-Agent:" MRTPSRC_USER_AGENT_STR "\r\n"
						"\r\n",
					lst->rtsp_addr,
					lst->req_seq++,
					lst->session);

			if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
			{
#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MAJOR, "%s | TEARDOWN request send fail", __FUNCTION__);
#endif
				goto channel_resource_free;
			}
		}

		mrtpsrc_ho_control_frame(lst, 1);

channel_resource_free:
		//reset
		reinitialize_channel_stream_connectivity(lst->ch_num, lst->stream_num);
		lst->state = STATE_TEARED_DOWN;

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | socket descriptor(%d)", __FUNCTION__, lst->rtsp_sock);
#endif
		if (lst->rtsp_sock > 0)
		{
			close(lst->rtsp_sock);
			lst->rtsp_sock = (-1);
		}

		if(lst->rtp_protocol == RTP_UNICAST_UDP || lst->rtp_protocol == RTP_MULTICAST_UDP)
		{
			if(lst->udp_sock[0] > 0)
			{
				close(lst->udp_sock[0]);
				lst->udp_sock[0] = (-1);
			}
			if(lst->udp_sock[1] > 0)
			{
				close(lst->udp_sock[1]);
				lst->udp_sock[1] = (-1);
			}
		}

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | audio out socket descriptor(%d)", __FUNCTION__, lst->audio_out_sock);
#endif
		if (lst->audio_out_sock > 0)
		{
			close(lst->audio_out_sock);
			lst->audio_out_sock = (-1);
		}

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | freeing remained frames", __FUNCTION__);
#endif

		if (mrtpsrc_get_parsing_state(ch_num, stream) == 1)
		{
			sleep(1);
		}

		mrtpsrc_frame_q_lock();
		probe = lst->frame_head;
		lst->frame_head = NULL;
		mrtpsrc_frame_q_unlock();

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MAJOR, "%s | gop(%u), seq(%u)", __FUNCTION__,
				lst->gop, lst->gop_seq);
#endif
		while(probe != NULL)
		{
			if (probe->len)
			{
				probe->len = 0;
				if (probe->frame != NULL)
				{
#if MRTPSRC_USE_STATISTICS
					stats.drop_cnt++;
#endif
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, probe->frame);
					probe->frame = NULL;
				}
			}
			probe_n = probe->next;
			memset(probe, 0x00, sizeof(struct _FRAME_INFO_T_));
			mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, probe);
			probe = probe_n;
		}

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | free channel infos", __FUNCTION__);
#endif
		memset(lst->ctrl, 0x00, 128);
		memset(lst->sps, 0x00, 128);
		memset(lst->pps, 0x00, 128);
		memset(lst->vps, 0x00, 128);
		memset(lst->sei, 0x00, 128);
		memset(lst->session, 0x00, 128);
		memset(lst->rtsp_validation, 0x00, 16);
		memset(lst->rtsp_cb, 0x00, MRTPSRC_MAX_CB_LEN);
		revalidate[ch_num] = 0;

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | initialize channel info", __FUNCTION__);
#endif
		lst->resolution = 0;
		lst->fps = 0;

		lst->ts_last_sendsec = 0;
		lst->ts_last_rcvd = 0;
		lst->ts_last_second = 0;
		lst->ts_last_5_mili = 0;
		lst->ts_last_remainder = 0;
		lst->ts_last_diff = 0;
		lst->ts_first_second = 0;
		lst->ts_first_5_mili = 0;
		lst->frame_num = 0;
		lst->model = 0;
		lst->gop_seq = 0;
		lst->gop = 0;
		lst->gop_len = 0;
		lst->ring_full_cnt = 0;
		lst->rbuf.lp = NULL;
		lst->rbuf.cp = NULL;
		lst->rbuf.buf_remain = lst->buf_sz;
		lst->rbuf.buf_used = 0;
		lst->abuf.start = 0;
		lst->abuf.len = 0;
		lst->seed_video = 0;
		lst->seed_audio = 0;
		lst->seed_sps = 0;
		lst->seed_pps = 0;
		lst->seed_vps = 0;
		lst->sr_ssrc = 0;
		lst->sei_len = 0;
		memset(lst->pbUserKey, 0x00, 16);
		memset(lst->username, 0x00, 64);
		memset(lst->password, 0x00, 64);
		memset(lst->realm, 0x00, 512);
		memset(lst->nonce, 0x00, 512);

		if (stream == STREAM_AUDIO)
		{
			MRTPSRC_AUDIO_RING_T *aring = mrtpsrc_get_audio_ring();
#if MRTPSRC_FULL_DBG
			MRTPSRC_DBG(MINOR, "%s | initialize audio ring buffer", __FUNCTION__);
#endif
			memset((void*)&aring[ch_num], 0x00, sizeof(MRTPSRC_AUDIO_RING_T));
		}

#if 0
		if (lst->rbuf.buf != NULL)
		{
			g_object_unref(lst->rbuf.buf);
			lst->rbuf.buf = NULL;
		}
#endif
	}
	rtspc_errno = ERR_NO_ERROR;
#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | done", __FUNCTION__);
#endif
	return RTN_OK;
}


static gchar* _mrtp_get_endline(gchar* src, gint* len)
{
	gchar* r;
	gchar* n;
	gchar* rn;

	rn = g_strstr_len(src, MRTPSRC_NBUF_SZ, "\r\n");
	if (rn != NULL)
	{
		*len = 2;
		return rn;
	}

	r = g_strstr_len(src, MRTPSRC_NBUF_SZ, "\r");
	n = g_strstr_len(src, MRTPSRC_NBUF_SZ, "\n");

	if (r==NULL && n==NULL)
	{
		*len = 0;
		return NULL;
	}

	if (r!=NULL && n!=NULL)
	{
		*len = 1;
		return ((r>n) ? n:r);
	}

	if (r==NULL)
	{
		*len = 1;
		return n;
	}
	if (n==NULL)
	{
		*len = 1;
		return r;
	}

	*len = 0;
	return NULL;
}

extern guint 
mrtpsrc_open_stream
(
	guint ch_num, guint stream, guint ip_addr, guint port, gchar* rtsp_addr,
	gchar* username, gchar* password, guint fps, guint resolution, guint model, guchar rtp_protocol, guint codec_type
)
{
	gint cnt = 0;
	gint len = 0;
	gchar buf[MRTPSRC_NBUF_SZ];
	char auth_str[2048];
	DWORD pdwRoundKey[32];
	guchar pdData[16];
	MRTPSRC_STREAM_T *lst = NULL;
	GobjMrtpSrc *callback = NULL;


	MRTPSRC_DBG(MAJOR, "%s | ch_num(%u, %s), %u.%u.%u.%u:%u, rtsp_addr(%s) model(%d)",
				__FUNCTION__,
				ch_num, MRTPSRC_STREAM_STR[stream], MRTPSRC_PRINT_IP(ip_addr), port, rtsp_addr, model);
#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | username(%s) password(%s)", __FUNCTION__, username, password);
#endif

	/* Parameter check */
	{
#if MRTPSRC_SUPPORT_3RD_STREAM			
		if (ch_num >= MRTPSRC_MAX_CH * 3)
#else
		if (ch_num >= MRTPSRC_MAX_CH * 2)
#endif		
		{
			MRTPSRC_DBG(ERROR, "%s | Invalid channel number(%u)", __FUNCTION__, ch_num);
			rtspc_errno = ERR_OPEN_INVALID_CH_NUM;
			return RTN_FAIL;
		}

		if (stream >= STREAM_MAX)
		{
			MRTPSRC_DBG(ERROR, "%s | Invalid stream(%u)", __FUNCTION__, stream);
			rtspc_errno = ERR_OPEN_INVALID_STREAM;
			return RTN_FAIL;
		}

		lst = mrtpsrc_get_stream(ch_num, stream);

		if (ip_addr == 0)
		{
			MRTPSRC_DBG(ERROR, "%s | Invalid location(NULL)", __FUNCTION__);
			rtspc_errno = ERR_OPEN_NULL_LOCATION;
			return RTN_FAIL;
		}

		if (port > 0xffffffff || port <= 0)
		{
			MRTPSRC_DBG(ERROR, "%s | Invalid port number(%u)", __FUNCTION__, port);
			rtspc_errno = ERR_OPEN_INVALID_PORT_NUM;
			return RTN_FAIL;
		}
		// TODO : open parameter for UDP
		rtp_protocol = RTP_OVER_RTSP_TCP;

		/*if (rtp_protocol < RTP_OVER_RTSP_TCP || rtp_protocol > RTP_MULTICAST_UDP)
		{
			MRTPSRC_DBG(ERROR, "%s | Unknown RTP method(%u)", __FUNCTION__, rtp_protocol);
			rtspc_errno = ERR_OPEN_INVALID_RTP_METHOD;
			return RTN_FAIL;
		}*/

		if (lst->state != STATE_INITIAL && lst->state != STATE_PAUSED && lst->state != STATE_TEARED_DOWN)
		{
			MRTPSRC_DBG(ERROR, "%s | Channel %d is not on initial state", __FUNCTION__, lst->ch_num);
			rtspc_errno = ERR_OPEN_CH_DUP;
			return RTN_FAIL;
		}

		lst->ip_addr = ip_addr;
		lst->rtsp_port = port;
		lst->model = model;
		if (stream == STREAM_AUDIO || stream == STREAM_META)
		{
			lst->fps = FPS_AUDIO;
		}
		else
		{
			//lst->fps = fps_conversion_to_local_use(fps);
			lst->fps = FPS_300;
		}
		lst->resolution = resolution;
		g_strlcpy(lst->rtsp_addr, rtsp_addr, MRTPSRC_MAX_CB_LEN);
		g_strlcpy(lst->username, username, 64);
		g_strlcpy(lst->password, password, 64);

		//codec_type setup : host H265 or H264
		lst->en = codec_type; 

		// FIXME. temp gop_len
		lst->gop_len = 30;

		lst->state = STATE_TCP_DISCONNECTED;
		lst->rtp_protocol = rtp_protocol;

#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
		lst->check_sei =0;
		lst->sei_meta_1st=NULL;
		lst->sei_meta_2nd=NULL;
		lst->sei_event_1st=NULL;
		lst->sei_event_2nd=NULL;
#endif
	}

	if (lst->stream_num == STREAM_AUDIO && lst->resolution == 100)
	{
		lst->rtsp_sock = 0;
		lst->rbuf.lp = lst->rbuf.rbs;
		lst->rbuf.cp = lst->rbuf.rbs;
		//lst->rbuf.buf_remain = lst->buf_sz - MRTPSRC_RING_GAP;
		lst->rbuf.buf_remain = lst->buf_sz;
		lst->rbuf.buf_used = 0;
		lst->frame_head = NULL;
		lst->frame_tail = NULL;
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
		lst->check_sei =0;
		lst->sei_meta_1st=NULL;
		lst->sei_meta_2nd=NULL;
		lst->sei_event_1st=NULL;
		lst->sei_event_2nd=NULL;
#endif
		lst->state = STATE_STREAM_READY;
		return RTN_OK;
	}


#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | Try tcp connect", __FUNCTION__);
#endif


	/* Try rtsp connection */
	{
		gint sock;
		struct sockaddr_in sin;

		memset(&sin, 0x00, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = ip_addr;
		sin.sin_port = htons(port);

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("socket");
			MRTPSRC_DBG(ERROR, "%s | socket descriptor get fail", __FUNCTION__);
			lst->state = STATE_INITIAL;
			rtspc_errno = ERR_OPEN_CONN_FAIL;
			return RTN_FAIL;
		}

		//MRTPSRC_DBG(MINOR, "%s | socket descriptor(%d)", __FUNCTION__, sock);

#if MRTPSRC_ENLARGE_SKB
		{
			int buf_size = 0;
			int len = 0;

			//MRTPSRC_DBG(MAJOR, "%s | socket receive buffer size doubled", __FUNCTION__);
			len = sizeof(int);
			getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_size, (socklen_t *)&len);
			//MRTPSRC_DBG(MAJOR, "%s | default rbuf(%d)", __FUNCTION__, buf_size);

            if(stream == STREAM_1ST){
                buf_size = 384*1024;
            }else if(stream == STREAM_2ND){
                //buf_size = 256*1024;
                buf_size = 192*1024;
#if MRTPSRC_SUPPORT_3RD_STREAM
            }else if(stream == STREAM_3RD){
                //buf_size = 64*1024;
                buf_size = 192*1024;
#endif
            }else{
                buf_size = 64*1024;
            }
			setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, (socklen_t)len);

			len = sizeof(int);
			getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buf_size, (socklen_t *)&len);
			//MRTPSRC_DBG(MAJOR, "%s | modified rbuf(%d)", __FUNCTION__, buf_size);
		}
#endif
#if MRTPSRC_ABORTIVE_DISCONNECT
		{
			struct linger l;
			l.l_onoff = 1;
			l.l_linger = 0;

			if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0)
			{
				MRTPSRC_DBG(WARN, "%s | Abortive disconnect set failed", __FUNCTION__);
			}
		}
#endif
#if MRTPSRC_NO_LOCAL_CRC_CHECK
		{
			int no_check = 1;

			if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &no_check, sizeof(long)) < 0)
			{
				MRTPSRC_DBG(WARN, "%s | TCP checksum no check failed", __FUNCTION__);
			}
		}
#endif
		{
			struct timeval tv;
			tv.tv_sec = 10; // 10 Secs Timeout
			tv.tv_usec = 0;
			int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
			if (ret < 0)
			{
				MRTPSRC_DBG(WARN, "%s | TCP TimeOut failed %d", __FUNCTION__, ret);
			}
		}

		if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) == -1)
		{
			perror("connect");
			MRTPSRC_DBG(ERROR, "%s | socket connect fail", __FUNCTION__);
			close(sock);
			sock = (-1);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_CONN_FAIL;
			return RTN_FAIL;
		}

		lst->rtsp_sock = sock;
		lst->state = STATE_TCP_CONNECTED;

		//MRTPSRC_DBG(MINOR, "%s | Connected socket(%d)", __FUNCTION__, sock);
	}

#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | OPTIONS cycle", __FUNCTION__);
#endif

	/* RTSP command 'OPTION' */
	{
		memset(buf, 0x00, MRTPSRC_NBUF_SZ);

		if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
		{
			g_snprintf(buf, MRTPSRC_NBUF_SZ, 
						"OPTIONS %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
						"\r\n",
					lst->rtsp_addr, lst->req_seq++);
		}
		else
		{
			g_snprintf(buf, MRTPSRC_NBUF_SZ, 
						"OPTIONS %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
						"\r\n",
					lst->rtsp_addr, lst->req_seq++);
		}

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | OPTIONS request\n%s", __FUNCTION__, buf);
#endif

		if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
		{
			perror("send");
			MRTPSRC_DBG(ERROR, "%s | OPTIONS request send fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_OPTION_FAIL;
			return RTN_FAIL;
		}

		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
#if 1
		cnt = 0;
		while(cnt < 5)
		{
			g_usleep(300*1000);//ksi_test 300 -> 500
			len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, MSG_DONTWAIT | MSG_PEEK);
			if (len < 0 || errno == EAGAIN)
			{
				//len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, MSG_DONTWAIT | MSG_PEEK | MSG_OOB);
				len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, MSG_DONTWAIT | MSG_OOB);
				MRTPSRC_DBG(WARN, "%s | READ urgent data(%u:%d)", __FUNCTION__, ch_num, len);
				cnt++;
			}
			else
			{
				len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				if (len > 0) { break; }
				else { cnt++; }
			}
		}
		if (cnt >= 5)
#else
		g_usleep(200*1000);
		len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
		if (len <= 0)
#endif
		{
			perror("recv");
			MRTPSRC_DBG(ERROR, "%s | OPTIONS reply receive fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_RES_TEMP_UNAVAILABLE;
			return RTN_FAIL;
		}
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | OPTIONS reply\n%s", __FUNCTION__, buf);
#endif

		{
			char *p = NULL;
			char *s = NULL;
			char *e = NULL;
			char *user = NULL;
			char *pass = NULL;
			char *realm = NULL;
			char *nonce = NULL;
			char *uri = NULL;
			char *method = "OPTIONS";
			const char* f_str_auth = "401 Unauthorized";
			const char* f_str_realm = "realm=\"";
			const char* f_str_nonce = "nonce=\"";

			p = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_auth);
			if (p != NULL)
			{
				s = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_realm);
				if (s == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | OPTIONS auth realm fail(start)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				s += strlen(f_str_realm);
				e = g_strstr_len(s, MRTPSRC_NBUF_SZ, "\"");
				if (e == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | OPTIONS auth realm fail(end)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				memset(lst->realm, 0x00, 512);
				memcpy(lst->realm, s, (e-s)>511?(511):(e-s));
				realm = &lst->realm[0];

				s = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_nonce);
				if (s == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | OPTIONS auth nonce fail(start)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				s += strlen(f_str_nonce);
				e = g_strstr_len(s, MRTPSRC_NBUF_SZ, "\"");
				if (e == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | OPTIONS auth nonce fail(end)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				memset(lst->nonce, 0x00, 512);
				memcpy(lst->nonce, s, (e-s)>511?(511):(e-s));
				nonce = &lst->nonce[0];

				uri = &lst->rtsp_addr[0];
				user = &lst->username[0];
				pass = &lst->password[0];

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MAJOR, "%s | user:%s pass:%s realm:%s, nonce:%s, uri:%s",
						__FUNCTION__, 
						user, pass, realm, nonce, uri);
#endif
				get_rtsp_digest_auth_str(user, pass, realm, nonce, uri, method, auth_str);

				memset(buf, 0x00, MRTPSRC_NBUF_SZ);
				if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
				{
					g_snprintf(buf, MRTPSRC_NBUF_SZ,
						"OPTIONS %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"%s"
						"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
						"\r\n",
						uri, lst->req_seq++, auth_str);
				}
				else
				{
					g_snprintf(buf, MRTPSRC_NBUF_SZ,
						"OPTIONS %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"%s"
						"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
						"\r\n",
						uri, lst->req_seq++, auth_str);
				}

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | OPTIONS request\n%s", __FUNCTION__, buf);
#endif
				//MRTPSRC_DBG(MAJOR, "%s", buf);
				if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
				{
					perror("send");
					MRTPSRC_DBG(ERROR, "%s | OPTIONS request send fail", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}

				memset(buf, 0x00, MRTPSRC_NBUF_SZ);
				len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				if (len < 0)
				{
					perror("recv");
					MRTPSRC_DBG(ERROR, "%s | OPTIONS reply receive fail", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}

#if MRTPSRC_FULL_DBG
				MRTPSRC_DBG(MAJOR, "%s", buf);
#endif
			}
		}

		/* check if the SEED applied cam */
		{
			char *p = NULL;
			char *s = NULL;
			const char *f_str_security = "SEED";
			const char *f_str_video = "video1";
			const char *f_str_audio = "audio";
			const char *f_str_sps = "sps";
			const char *f_str_pps = "pps";

			p = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_security);
			if (p != NULL)
			{
				s = g_strstr_len(p, MRTPSRC_NBUF_SZ, "/");
				if (s != NULL)
				{
					s += 1;
					if (g_strstr_len(s, MRTPSRC_NBUF_SZ, f_str_video) != NULL) { lst->seed_video = 1; }
					else { lst->seed_video = 0; }
					if (g_strstr_len(s, MRTPSRC_NBUF_SZ, f_str_audio) != NULL) { lst->seed_audio = 1; }
					else { lst->seed_audio = 0; }
					if (g_strstr_len(s, MRTPSRC_NBUF_SZ, f_str_sps) != NULL) { lst->seed_sps = 1; }
					else { lst->seed_sps = 0; }
					if (g_strstr_len(s, MRTPSRC_NBUF_SZ, f_str_pps) != NULL) { lst->seed_pps = 1; }
					else { lst->seed_pps = 0; }
				}
				else
				{
					lst->seed_video = 0;
					lst->seed_audio = 0;
					lst->seed_sps = 0;
					lst->seed_pps = 0;
				}
				/* FIXME. Userkey ? */
				{
					int pwd_len = strlen(lst->password);
					int usr_len = strlen(lst->username);
					int remain_len = 16;
					MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);

#if PRINT_RTSP_MSG
MRTPSRC_DBG(MAJOR, "%s | SEED user key creation password %s", __FUNCTION__, lst->password);
#endif
					if (pwd_len <= remain_len)
					{

						memcpy(&lst->pbUserKey[0], lst->password, pwd_len);
						remain_len -= pwd_len;
					}
					else
					{
						memcpy(&lst->pbUserKey[0], lst->password, remain_len);
						remain_len = 0;
					}
#if PRINT_RTSP_MSG
MRTPSRC_DBG(MAJOR, "%s | SEED user key creation username %s", __FUNCTION__, lst->username);
#endif
					if (usr_len <= remain_len)
					{
						memcpy(&lst->pbUserKey[pwd_len], lst->username, usr_len);
						remain_len -= usr_len;
					}
					else
					{
						memcpy(&lst->pbUserKey[pwd_len], lst->username, remain_len);
						remain_len = 0;
					}
#if PRINT_RTSP_MSG
MRTPSRC_DBG(MAJOR, "%s | SEED user key creation mac %02x:%02x:%02x:%02x:%02x:%02x",
		__FUNCTION__,
		lch->dev_mac[0], lch->dev_mac[1], lch->dev_mac[2],
		lch->dev_mac[3], lch->dev_mac[4], lch->dev_mac[5]);
#endif
					if (6 <= remain_len)
					{
						memcpy(&lst->pbUserKey[usr_len+pwd_len], lch->dev_mac, 6);
						remain_len -= 6;
					}
					else
					{
						memcpy(&lst->pbUserKey[usr_len+pwd_len], lch->dev_mac, remain_len);
						remain_len = 0;
					}
					if (remain_len > 0)
					{
						memset(&lst->pbUserKey[usr_len+pwd_len+6], 0x00, remain_len);
					}
				}
			}
			else
			{
				lst->seed_video = 0;
				lst->seed_audio = 0;
				lst->seed_sps = 0;
				lst->seed_pps = 0;
			}
		}

		/* check waht session validation command is */
		{
			char *g = NULL;
			char *s = NULL;
			const char *f_str_get = "GET_PARAMETER";
			const char *f_str_set = "SET_PARAMETER";

			g = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_get);
			s = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_set);

			if (s != NULL)
			{
				g_strlcpy(lst->rtsp_validation, f_str_set, 16);
				revalidate[ch_num] = 1;
			}
			else
			{
				if (g != NULL)
				{
					g_strlcpy(lst->rtsp_validation, f_str_get, 16);
					revalidate[ch_num] = 1;
				}
				else
				{
					memset(lst->rtsp_validation, 0x00, 16);
					revalidate[ch_num] = 0;
				}
			}
		}
	}



#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | DESCRIBE cycle", __FUNCTION__);
#endif


	/* RTSP command 'DESCRIBE' */
	{
		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
		if (lst->realm[0] != '\0' && lst->nonce[0] != '\0')
		{
			char *method = "DESCRIBE";

			get_rtsp_digest_auth_str(
					lst->username, lst->password,
					lst->realm, lst->nonce,
					lst->rtsp_addr, method, auth_str
			);

#if PRINT_RTSP_MSG
			MRTPSRC_DBG(MAJOR, "%s | user:%s pass:%s realm:%s, nonce:%s, uri:%s",
					__FUNCTION__,
					lst->username, lst->password,
					lst->realm, lst->nonce,
					lst->rtsp_addr);
			MRTPSRC_DBG(MAJOR, "%s | auth_str:%s", __FUNCTION__, auth_str);
#endif
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"DESCRIBE %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"%s"
							"Accept: application/sdp\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"Data-Encryption: partial\r\n"
							"\r\n",
						lst->rtsp_addr, lst->req_seq++, auth_str);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"DESCRIBE %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"%s"
							"Accept: application/sdp\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"Data-Encryption: partial\r\n"
							"\r\n",
						lst->rtsp_addr, lst->req_seq++, auth_str);
			}
		}
		else
		{
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"DESCRIBE %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Accept: application/sdp\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"Data-Encryption: partial\r\n"
							"\r\n",
						lst->rtsp_addr, lst->req_seq++);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"DESCRIBE %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Accept: application/sdp\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"Data-Encryption: partial\r\n"
							"\r\n",
						lst->rtsp_addr, lst->req_seq++);
			}
		}

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | DESCRIBE request\n%s", __FUNCTION__, buf);
#endif

		if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
		{
			perror("send");
			MRTPSRC_DBG(ERROR, "%s | DESCRIBE request send fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_DESCRIBE_FAIL;
			return RTN_FAIL;
		}

		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
#if 0
		cnt = 0;
		while(cnt < 10)
		{
			g_usleep(200*1000);
			len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, MSG_DONTWAIT | MSG_PEEK);
			if (len < 0 || errno == EAGAIN)
			{
				cnt++;
			}
			else
			{
				len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				if (len < 0)
				{
					g_usleep(50*1000);
					len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				}
				break;
			}
		}
		if (cnt >= 10)
#else
		len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
		if (len < 0)
#endif
		{
			perror("recv");
			MRTPSRC_DBG(ERROR, "%s | %d:DESCRIBE reply receive fail", __FUNCTION__, __LINE__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_DESCRIBE_FAIL;
			return RTN_FAIL;
		}
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | DESCRIBE reply\n%s", __FUNCTION__, buf);
#endif

		{
			char *p = NULL;
			char *s = NULL;
			char *e = NULL;
			char *user = NULL;
			char *pass = NULL;
			char *realm;
			char *nonce;
			char *uri;
			char *method = "DESCRIBE";
			const char* f_str_auth = "401 Unauthorized";
			const char* f_str_realm = "realm=\"";
			const char* f_str_nonce = "nonce=\"";

			const gchar f_str_server_gortsp[] = "Server: gortsplib";				
			
			if (g_strstr_len(buf, len, f_str_server_gortsp) != NULL) {
				lst->is_gortsplib = TRUE;
				printf("Detected MediaTX RTSP Server\n");
			} else {
				lst->is_gortsplib = FALSE;
			}			

			p = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_auth);
			if (p != NULL)
			{
				s = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_realm);
				if (s == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | DESCRIBE auth realm fail(start)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				s += strlen(f_str_realm);
				e = g_strstr_len(s, MRTPSRC_NBUF_SZ, "\"");
				if (e == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | OPTIONS auth realm fail(end)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				memset(lst->realm, 0x00, 512);
				memcpy(lst->realm, s, (e-s)>511?511:(e-s));
				realm = &lst->realm[0];

				s = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_nonce);
				if (s == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | DESCRIBE auth nonce fail(start)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				s += strlen(f_str_nonce);
				e = g_strstr_len(s, MRTPSRC_NBUF_SZ, "\"");
				if (e == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | OPTIONS auth nonce fail(end)", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}
				memset(lst->nonce, 0x00, 512);
				memcpy(lst->nonce, s, (e-s)>511?511:(e-s));
				nonce = &lst->nonce[0];

				uri =  &lst->rtsp_addr[0];
				user = &lst->username[0];
				pass = &lst->password[0];

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MAJOR, "%s | [user:%s pass:%s realm:%s, nonce:%s, uri:%s",
						__FUNCTION__,
						user, pass, realm, nonce, uri);
#endif
				get_rtsp_digest_auth_str(user, pass, realm, nonce, uri, method, auth_str);
				memset(buf, 0x00, MRTPSRC_NBUF_SZ);
				if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
				{
					g_snprintf(buf, MRTPSRC_NBUF_SZ,
						"DESCRIBE %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"%s"
						"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
						"Data-Encryption: partial\r\n"
						"\r\n",
						uri, lst->req_seq++, auth_str);
				}
				else
				{
					g_snprintf(buf, MRTPSRC_NBUF_SZ,
						"DESCRIBE %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"%s"
						"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
						"Data-Encryption: partial\r\n"
						"\r\n",
						uri, lst->req_seq++, auth_str);
				}

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | DESCRIBE request\n%s", __FUNCTION__, buf);
#endif
				//MRTPSRC_DBG(MAJOR, "%s", buf);
				if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
				{
					perror("send");
					MRTPSRC_DBG(ERROR, "%s | DESCRIBE request send fail", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}

				memset(buf, 0x00, MRTPSRC_NBUF_SZ);
				len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				if (len < 0)
				{
					perror("recv");
					MRTPSRC_DBG(ERROR, "%s | %d:DESCRIBE reply receive fail", __FUNCTION__, __LINE__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_OPTION_FAIL;
					return RTN_FAIL;
				}

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MAJOR, "%s", buf);
#endif
			}
		}

		/* HTTP header와 sdp를 나눠 전송하는 case 처리 */
		{
			const char *test_str = "m=video";
			int add_len = 0;

			if (g_strstr_len(buf,len,test_str)==NULL)
			{
				add_len = recv(lst->rtsp_sock, buf+len, MRTPSRC_NBUF_SZ-len, 0); 
				if (add_len < 0)
				{
					perror("recv");
					MRTPSRC_DBG(ERROR, "%s | %d:DESCRIBE reply receive MORE fail", __FUNCTION__, __LINE__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_DESCRIBE_FAIL;
					return RTN_FAIL;
				}
				len += add_len;
#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | DESCRIBE MORE reply\n%s", __FUNCTION__, buf);
#endif
			}
		}
		/* DESCRIBE Reply msg parsing */
		{
			const gchar f_str_video[] = "m=video";
			const gchar f_str_audio[] = "m=audio";
			const gchar f_str_rtpavp[] = "RTP/AVP ";
			const gchar f_str_control[] = "a=control:";
			const gchar f_str_cb[] = "Content-Base: ";

			gchar *sdp_ptr = NULL;
			gchar *media_ptr = NULL;
			gchar *pt_ptr= NULL;
			gchar *ctrl_ptr = NULL;
			gchar *cb_ptr = NULL;
			gchar *ptr_tail = NULL;


#if 0
			/* Revalidate Cache-Control */
			{
				gchar *validate = NULL;
				validate = g_strstr_len(buf, MRTPSRC_NBUF_SZ, "must-revalidate");

				if (validate != NULL) { revalidate[ch_num] = 1; }
				else { revalidate[ch_num] = 0; }
			}
#endif

			/* Find media description */
			sdp_ptr = buf;
			if (stream == STREAM_AUDIO)
			{
				/* Audio media description parsing */

				gchar temp_pt[8] = { 0, };

				if ((sdp_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str_audio)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | No audio description", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_AUDIO;
					return RTN_FAIL;
				}

				media_ptr = sdp_ptr + 7; //strlen(f_str_audio);
				if ((media_ptr = g_strstr_len(media_ptr, MRTPSRC_NBUF_SZ, f_str_rtpavp)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | No audio payload type", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_AUDIO_PT;
					return RTN_FAIL;
				}

				/* Audio payload type set */
				pt_ptr = media_ptr + 8; //strlen(f_str_rtpavp);
				{
					gint __len;
					media_ptr = _mrtp_get_endline(pt_ptr, &__len);
					if (media_ptr == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Audio payload type", __FUNCTION__);
						close(lst->rtsp_sock);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_NO_AUDIO_PT;
						return RTN_FAIL;
					}
					memset(temp_pt, 0x00, 8);
					memcpy(temp_pt, pt_ptr, (media_ptr-pt_ptr)>7?7:(media_ptr-pt_ptr));
					lst->pt = atoi(temp_pt);
					sdp_ptr = media_ptr + __len; //strlen("\r\n");
				}
#if 0
				media_ptr = strstr(pt_ptr, "\r\n");
				if (media_ptr == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Audio payload type", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_NO_AUDIO_PT;
					return RTN_FAIL;
				}
				memset(temp_pt, 0x00, 8);
				memcpy(temp_pt, pt_ptr, media_ptr-pt_ptr);
				lst->pt = atoi(temp_pt);
				sdp_ptr = media_ptr + 2; //strlen("\r\n");
#endif

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | Audio payload type(%u)", __FUNCTION__, lst->pt);
#endif

				/* Audio control set */
				if((ctrl_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str_control)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Audio control set fail", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_AUDIO_CTRL;
					return RTN_FAIL;
				}

				ctrl_ptr += 10; //strlen(f_str_control);
				{
					gint __len;
					ptr_tail = _mrtp_get_endline(ctrl_ptr, &__len);
					if (ptr_tail == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Audio control set", __FUNCTION__);
						close(lst->rtsp_sock);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_NO_AUDIO_CTRL;
						return RTN_FAIL;
					}
					memset(lst->ctrl, 0x00, 128);
					memcpy(lst->ctrl, ctrl_ptr, (ptr_tail - ctrl_ptr)>127?127:(ptr_tail-ctrl_ptr));
				}

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | control(%s)", __FUNCTION__, lst->ctrl);
#endif

				/* Audio content-base set */
				if((cb_ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_cb)) == NULL)
				{
					memset(lst->rtsp_cb, 0x00, MRTPSRC_MAX_CB_LEN);
				}
				else
				{
					cb_ptr += strlen(f_str_cb);
					{
						gint __len;
						ptr_tail = _mrtp_get_endline(cb_ptr, &__len);
						if (ptr_tail == NULL)
						{
							MRTPSRC_DBG(ERROR, "%s | Audio Content-Base set", __FUNCTION__);
							close(lst->rtsp_sock);
							//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
							rtspc_errno = ERR_OPEN_NO_AUDIO_CTRL;
							return RTN_FAIL;
						}
						memset(lst->rtsp_cb, 0x00, MRTPSRC_MAX_CB_LEN);
						memcpy(lst->rtsp_cb, cb_ptr, (ptr_tail - cb_ptr)>MRTPSRC_MAX_CB_LEN?MRTPSRC_MAX_CB_LEN:(ptr_tail-cb_ptr));
					}
				}
#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | Content-Base(%s)", __FUNCTION__, lst->rtsp_cb);
#endif
			/* Stream audio end */
			}
			else if (stream == STREAM_META)
			{
				/* metadata streaming search start */
				const gchar f_str_application[] = "m=application";

				gchar* search_ptr = NULL;
				//gchar* search_tail = NULL;

				if((search_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str_application)) != NULL)
				{
					/* Metadata control set */
					if ((ctrl_ptr = g_strstr_len(search_ptr, MRTPSRC_NBUF_SZ, f_str_control)) == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Metadata control set fail", __FUNCTION__);
						close(lst->rtsp_sock);
						lst->state = STATE_INITIAL;
						lst->rtsp_sock = (-1);
						rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
						return RTN_FAIL;
					}

					ctrl_ptr += 10; //strlen(f_str_control);
					{
						gint __len;
						ptr_tail = _mrtp_get_endline(ctrl_ptr, &__len);
						if (ptr_tail == NULL)
						{
							MRTPSRC_DBG(ERROR, "%s | Metadata control set fail2", __FUNCTION__);
							close(lst->rtsp_sock);
							lst->state = STATE_INITIAL;
							lst->rtsp_sock = (-1);
							//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
							rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
							return RTN_FAIL;
						}
						//lst->meta_on = TRUE;
						memset(lst->ctrl, 0x00, 128);
						memcpy(lst->ctrl, ctrl_ptr, (ptr_tail - ctrl_ptr)>127?127:(ptr_tail-ctrl_ptr));

						MRTPSRC_DBG(MAJOR, "%s | Metadata control (%s) found", __FUNCTION__, lst->ctrl);
					}
				}
				else
				{
					MRTPSRC_DBG(ERROR, "%s | No Metadata control exist", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
					return RTN_FAIL;
				}
					/* metadata streaming search end */
			}
			else
			{
				// for grundig audio out
				if(stream == STREAM_1ST && lst->model == MODEL_ONVIF_GRUNDIG)
				{
					gchar temp_pt[8] = { 0, };
					guint server_port_num;
					gchar* udp_start_ptr;
					gchar* udp_end_ptr;
					udp_start_ptr = buf;

					if ((udp_start_ptr = g_strstr_len(udp_start_ptr, MRTPSRC_NBUF_SZ, f_str_audio)) != NULL)
					{
						//MRTPSRC_DBG(MINOR, "%s | start %s", __FUNCTION__, media_ptr);
						udp_end_ptr = udp_start_ptr + 7; //strlen(f_str_audio);
						udp_start_ptr = g_strstr_len(udp_end_ptr, MRTPSRC_NBUF_SZ, f_str_rtpavp);
						if(udp_start_ptr != NULL)
						{
							memset(temp_pt, 0x00, 8);
							memcpy(temp_pt, udp_end_ptr, (udp_start_ptr-udp_end_ptr)>7?7:(udp_start_ptr-udp_end_ptr));
							server_port_num = atoi(temp_pt);
							if(server_port_num != 0)
							{
								lst->audio_out_client_port = get_port_number();
								lst->audio_out_server_port = server_port_num;
								MRTPSRC_DBG(MINOR, "%s | audio out port client (%u) server (%u)", __FUNCTION__, lst->audio_out_client_port, lst->audio_out_server_port);
								if(!bind_udp_socket(lst->audio_out_client_port, &lst->audio_out_sock))
								{
									// TODO error chk??
								}

								struct sockaddr_in sin;

								memset(&sin, 0x00, sizeof(sin));
								sin.sin_family = AF_INET;
								sin.sin_addr.s_addr = ip_addr;
								sin.sin_port = htons(lst->audio_out_server_port);

								connect(lst->audio_out_sock, (struct sockaddr*) &sin, sizeof(struct sockaddr));
							}
						}
					}
				}// end grundig audio out

				/* Video media description parsing */

				const gchar f_str_rtpmap[] = "a=rtpmap:";
				const gchar f_str_fmtp[] = "a=fmtp:";
				const gchar f_str_spspps[] = "sprop-parameter-sets=";
				const gchar f_str_donl[] = "sprop-max-don-diff=";
				const gchar f_str_depack[] = "sprop-depack-buf-nalus=";

				gchar f_str[256];
				guchar dcd_buf[256];
				gchar* search_ptr = NULL;
				gchar* search_tail = NULL;
				gchar temp_pt[8] = { 0, };

				if ((sdp_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str_video)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | No video description (%d.%d)", __FUNCTION__, ch_num, stream);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_VIDEO;
					return RTN_FAIL;
				}

				media_ptr = sdp_ptr + 7; //strlen(f_str_audio);
				if ((media_ptr = g_strstr_len(media_ptr, MRTPSRC_NBUF_SZ, f_str_rtpavp)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | No video payload type", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_VIDEO_PT;
					return RTN_FAIL;
				}

				/* Video payload type set */
				pt_ptr = media_ptr + 8; //strlen(f_str_rtpavp);
				{
					gint __len;
					media_ptr = _mrtp_get_endline(pt_ptr, &__len);
					if (media_ptr == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Video payload type", __FUNCTION__);
						close(lst->rtsp_sock);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_NO_VIDEO_PT;
						return RTN_FAIL;
					}
					memset(temp_pt, 0x00, 8);
					memcpy(temp_pt, pt_ptr, (media_ptr-pt_ptr)>7?7:(media_ptr-pt_ptr));
					lst->pt = atoi(temp_pt);
					sdp_ptr = media_ptr + __len; //strlen("\r\n");
				}
#if 0
				media_ptr = strstr(pt_ptr, "\r\n");
				if (media_ptr == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Video payload type", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_NO_VIDEO_PT;
					return RTN_FAIL;
				}
				memset(temp_pt, 0x00, 8);
				memcpy(temp_pt, pt_ptr, media_ptr-pt_ptr);
				lst->pt = atoi(temp_pt);
				sdp_ptr = media_ptr + 2; //strlen("\r\n");
#endif

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | Video payload type(%u)", __FUNCTION__, lst->pt);
#endif


				/* rtpmap search */
				memset(f_str, 0x00, 256);
				g_snprintf(f_str, 256, "%s%u ", f_str_rtpmap, lst->pt);
				if ((search_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Video rtpmap cannot be found", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_VIDEO_RTPMAP;
					return RTN_FAIL;
				}
				search_ptr += strlen(f_str);

				if (strncmp(search_ptr, "H264", 4) == 0)
				{
					if(lst->en != RTP_PT_H264){
						//cmd_callback call
						callback = gst_mrtp_src_get_object();
						if(callback->cmd_callback != NULL)
							callback->cmd_callback(11, lst->ch_num, lst->stream_num, RTP_PT_H264, callback->cmd_user_data);
					}
					lst->en = RTP_PT_H264;
				}
				else if(strncmp(search_ptr, "H265", 4) == 0)
				{
					if(lst->en != RTP_PT_H265){
						//cmd_callback call
						callback = gst_mrtp_src_get_object();
						if(callback->cmd_callback != NULL)
							callback->cmd_callback(11, lst->ch_num, lst->stream_num, RTP_PT_H265, callback->cmd_user_data);
					}
					lst->en = RTP_PT_H265;
				}
				else
				{
					MRTPSRC_DBG(ERROR, "%s | Video encoded other than H.264 or H.265 type", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_ENC_H264;
					return RTN_FAIL;
				}
				search_ptr += 5; //strlen("H264/");
				{
					gint __len;
					search_tail = _mrtp_get_endline(search_ptr, &__len);
					if (search_tail == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Video rtpmap", __FUNCTION__);
						close(lst->rtsp_sock);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_NO_VIDEO_RTPMAP;
						return RTN_FAIL;
					}
					memset(f_str, 0x00, 256);
					memcpy(f_str, search_ptr, (search_tail - search_ptr)>255?255:(search_tail-search_ptr));
					lst->sec_dur = atoi(f_str);
				}
#if 0
				search_tail = strstr(search_ptr, "\r\n");
				if (search_tail == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Video rtpmap", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_NO_VIDEO_RTPMAP;
					return RTN_FAIL;
				}
				memset(f_str, 0x00, 256);
				memcpy(f_str, search_ptr, search_tail - search_ptr);
				lst->sec_dur = atoi(f_str);
#endif

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | Video encoded H.264, and a second is treated as %u", __FUNCTION__, lst->sec_dur);
#endif

				/* fmtp search */
				memset(f_str, 0x00, 256);
				g_snprintf(f_str, 256, "%s%u ", f_str_fmtp, lst->pt);
				if ((search_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str)) != NULL && lst->en == RTP_PT_H264)
				{
					search_ptr += strlen(f_str);


					if ((search_ptr = g_strstr_len(search_ptr, MRTPSRC_NBUF_SZ, f_str_spspps)) == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Video sps/pps description cannot be found", __FUNCTION__);
						close(lst->rtsp_sock);
						lst->state = STATE_INITIAL;
						lst->rtsp_sock = (-1);
						rtspc_errno = ERR_OPEN_NO_VIDEO_SPSPPS;
						return RTN_FAIL;
					}
					search_ptr += 21; //strlen(f_str_spspps);

					/* sequence parameter set */
					search_tail = g_strstr_len(search_ptr, MRTPSRC_NBUF_SZ, ",");
					if (search_tail == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Video sps", __FUNCTION__);
						close(lst->rtsp_sock);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_NO_VIDEO_SPSPPS;
						return RTN_FAIL;
					}
					memset(f_str, 0x00, 256);
					memcpy(f_str, search_ptr, (search_tail - search_ptr)>255?255:(search_tail-search_ptr));
					//strncpy(f_str, search_ptr, search_tail - search_ptr);
					memset(dcd_buf, 0x00, 256);
					lst->sps_len = b64_decode_binary_to_buffer(dcd_buf, 256, f_str);

					if (lst->sps_len >= 256)
					{
						MRTPSRC_DBG(ERROR, "%s | Video sps length too long", __FUNCTION__);
						close(lst->rtsp_sock);
						lst->state = STATE_INITIAL;
						lst->rtsp_sock = (-1);
						rtspc_errno = ERR_OPEN_DESCRIBE_FAIL;
						return RTN_FAIL;
					}
					memset(lst->sps, 0x00, 128);
					memcpy(lst->sps, dcd_buf, lst->sps_len);
					MRTPSRC_DBG(MAJOR, "%s | SPS decoded", __FUNCTION__);

					if(strncmp(lst->sps, "", 128) == 0)
					{
						MRTPSRC_DBG(ERROR, "%s | Video sps is empty", __FUNCTION__);
						close(lst->rtsp_sock);
						lst->state = STATE_INITIAL;
						lst->rtsp_sock = (-1);
						rtspc_errno = ERR_OPEN_NO_VIDEO_SPSPPS;
						return RTN_FAIL;
					}

					int width = 0;
					int height = 0;
					mrtpsrc_get_resolution_from_sps(&lst->sps[1], &width, &height);
					MRTPSRC_DBG(MAJOR, "%s | Get resolution from sps : %dx%d", __FUNCTION__, width, height);
					mrtpsrc_set_stream_resolution(lst, width, height);
					if(lst->resolution == 0)
					{
						MRTPSRC_DBG(ERROR, "%s | Not Supported Resolution", __FUNCTION__);
						close(lst->rtsp_sock);
						lst->state = STATE_INITIAL;
						lst->rtsp_sock = (-1);
						rtspc_errno = ERR_OPEN_DESCRIBE_FAIL;
						return RTN_FAIL;
					}
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
						MRTPSRC_DBG(MAJOR, "%s | SPS SEED decrypt", __FUNCTION__);
					}
					search_ptr = search_tail + 1;

					/* picture parameter set */
					{
						gint __len;
						search_tail = _mrtp_get_endline(search_ptr, &__len);
						if (search_tail == NULL)
						{
							MRTPSRC_DBG(ERROR, "%s | Video pps", __FUNCTION__);
							close(lst->rtsp_sock);
							//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
							rtspc_errno = ERR_OPEN_NO_VIDEO_SPSPPS;
							return RTN_FAIL;
						}
						memset(f_str, 0x00, 256);
						memcpy(f_str, search_ptr, (search_tail - search_ptr)>255?255:(search_tail-search_ptr));
						//strncpy(f_str, search_ptr, search_tail - search_ptr);
						memset(dcd_buf, 0x00, 256);
						lst->pps_len = b64_decode_binary_to_buffer(dcd_buf, 256, f_str);
						if (lst->pps_len >= 256)
						{
							MRTPSRC_DBG(ERROR, "%s | Video pps length too long", __FUNCTION__);
							close(lst->rtsp_sock);
							lst->state = STATE_INITIAL;
							lst->rtsp_sock = (-1);
							rtspc_errno = ERR_OPEN_DESCRIBE_FAIL;
							return RTN_FAIL;
						}
						memset(lst->pps, 0x00, 128);
						memcpy(lst->pps, dcd_buf, lst->pps_len);
					}
					MRTPSRC_DBG(MAJOR, "%s | PPS decoded", __FUNCTION__);
					if (lst->seed_pps)
					{
						int index = 0;
						int loop_cnt = (lst->pps_len-1) / 16;
						//guint pdwRoundKey[32];
						//guchar pdData[16];
						for (index = 0; index < loop_cnt; index++)
						{
							memcpy(pdData, &lst->pps[index*16+1], 16);
							SeedRoundKey(pdwRoundKey, lst->pbUserKey);
							SeedDecrypt(&pdData[0], pdwRoundKey);
							memcpy(&lst->pps[index*16+1], pdData, 16);
						}
						MRTPSRC_DBG(MAJOR, "%s | PPS SEED decrypt", __FUNCTION__);
					}

#if PRINT_RTSP_MSG
					MRTPSRC_DBG(MINOR, "%s | sps_len(%u), pps_len(%u)",
							__FUNCTION__,
							lst->sps_len, lst->pps_len);
#endif
				}
				else
				{
					MRTPSRC_DBG(WARN, "%s | No fmtp description", __FUNCTION__);
					memset(lst->sps, 0x00, 128);
					memset(lst->pps, 0x00, 128);
					lst->sps_len = 0;
					lst->pps_len = 0;
				}

				/* Video control set */
				if ((ctrl_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str_control)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Video control set fail", __FUNCTION__);
					close(lst->rtsp_sock);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
					return RTN_FAIL;
				}

				ctrl_ptr += 10; //strlen(f_str_control);
				{
					gint __len;
					ptr_tail = _mrtp_get_endline(ctrl_ptr, &__len);
					if (ptr_tail == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Video control set", __FUNCTION__);
						close(lst->rtsp_sock);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
						return RTN_FAIL;
					}
					memset(lst->ctrl, 0x00, 128);
					memcpy(lst->ctrl, ctrl_ptr, (ptr_tail - ctrl_ptr)>127?127:(ptr_tail-ctrl_ptr));
				}

#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | control(%s)", __FUNCTION__, lst->ctrl);
#endif

				/* Video content-base set */
				if((cb_ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_cb)) == NULL)
				{
					memset(lst->rtsp_cb, 0x00, MRTPSRC_MAX_CB_LEN);
				}
				else
				{
					cb_ptr += strlen(f_str_cb);
					{
						gint __len;
						ptr_tail = _mrtp_get_endline(cb_ptr, &__len);
						if (ptr_tail == NULL)
						{
							MRTPSRC_DBG(ERROR, "%s | Video Content-Base set", __FUNCTION__);
							close(lst->rtsp_sock);
							//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
							rtspc_errno = ERR_OPEN_NO_AUDIO_CTRL;
							return RTN_FAIL;
						}
						memset(lst->rtsp_cb, 0x00, MRTPSRC_MAX_CB_LEN);
						memcpy(lst->rtsp_cb, cb_ptr, (ptr_tail - cb_ptr)>MRTPSRC_MAX_CB_LEN?MRTPSRC_MAX_CB_LEN:(ptr_tail-cb_ptr));
					}
				}
#if PRINT_RTSP_MSG
				MRTPSRC_DBG(MINOR, "%s | Content-Base(%s)", __FUNCTION__, lst->rtsp_cb);
#endif

				if(stream == STREAM_1ST)
				{
					/* metadata streaming search start */
					const gchar f_str_application[] = "m=application";

					if((search_ptr = g_strstr_len(sdp_ptr, MRTPSRC_NBUF_SZ, f_str_application)) != NULL)
					{
						MRTPSRC_CHANNEL_T* lch = mrtpsrc_get_channel(lst->ch_num);
						if(lch->is_metadata_enable)
						{
							MRTPSRC_DBG(MINOR, "%s | Metadata stream found!! ", __FUNCTION__);
							lch->is_metadata_on = TRUE;

							/* metadata resume bug fix */
							MRTPSRC_STREAM_T* meta_st = mrtpsrc_get_stream(lst->ch_num, STREAM_META);
							meta_st->ip_addr = lst->ip_addr;
							meta_st->rtsp_port = lst->rtsp_port;
							memcpy(meta_st->rtsp_addr, lst->rtsp_addr, 256);
						}
						else
						{
							//MRTPSRC_DBG(MINOR, "%s | Metadata stream found but not used!! ", __FUNCTION__);
						}
					}
				}

			/* Stream video end */
			}

			lst->state = STATE_SESSION_DESCRIBED;
		/* DESCRIBE Reply msg parsing end */
		}
	/* RTSP command 'DESCRIBE' end */
	}

#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | SETUP cycle", __FUNCTION__);
#endif

	/* RTSP command 'SETUP' */
	{
		/* SETUP(video) request */
		const gchar f_str_control[] = "rtsp://";
		gchar uri[256];
		gchar transport_str[256];

		if (lst->rtsp_cb[0] == '\0')
		{
			g_strlcpy(lst->rtsp_cb, lst->rtsp_addr, MRTPSRC_MAX_CB_LEN);
		}
		if (lst->rtsp_cb[strlen(lst->rtsp_cb)-1] == '/')
		{
			lst->rtsp_cb[strlen(lst->rtsp_cb)-1] = '\0';
		}

		if (lst->ctrl[0] != '\0')
		{
			if (g_strstr_len(lst->ctrl, MRTPSRC_NBUF_SZ, f_str_control) == NULL)
			{
				snprintf(uri, 256, "%s/%s", lst->rtsp_cb, lst->ctrl);	
			}
			else
			{
				snprintf(uri, 256, "%s", lst->ctrl);
			}
		}
		else
		{
			MRTPSRC_DBG(ERROR, "%s | SETUP NULL ctrl", __FUNCTION__);
			close(lst->rtsp_sock);
			memset(lst->ctrl, 0x00, 128);
			memset(lst->sps, 0x00, 128);
			memset(lst->pps, 0x00, 128);
			memset(lst->session, 0x00, 128);
			memset(lst->rtsp_validation, 0x00, 16);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
			return RTN_FAIL;
		}

		memset(buf, 0x00, MRTPSRC_NBUF_SZ);

		switch(lst->rtp_protocol)
		{
		case RTP_UNICAST_UDP:
			bind_rtp_unicast(lst);
			g_snprintf(transport_str, 256, "RTP/AVP;unicast;client_port=%u-%u", lst->client_port[0], lst->client_port[1]);
			break;

		case RTP_MULTICAST_UDP:
			lst->client_port[0] = get_port_number();
			lst->client_port[1] = lst->client_port[0] + 1;
			g_snprintf(transport_str, 256, "RTP/AVP;multicast;client_port=%u-%u", lst->client_port[0], lst->client_port[1]);
			break;

		case RTP_OVER_RTSP_TCP:
			g_snprintf(transport_str, 256, "RTP/AVP/TCP;unicast;interleaved=0-1");
			break;
		}

		if (lst->realm[0] != '\0' && lst->nonce[0] != '\0')
		{
			char *method = "SETUP";

			get_rtsp_digest_auth_str(
					lst->username, lst->password,
					lst->realm, lst->nonce,
					lst->rtsp_addr, method, auth_str
			);
		}
		else
		{
			memset(auth_str, 0x00, 2048);
		}
		{
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"SETUP %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Transport: %s\r\n"
							"%s"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"\r\n",
						uri, lst->req_seq++, transport_str, auth_str);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"SETUP %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Transport: %s\r\n"
							"%s"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"\r\n",
						uri, lst->req_seq++, transport_str, auth_str);
			}
		}
#if 0
		else
		{
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"SETUP %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"\r\n",
						uri, lst->req_seq++);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"SETUP %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"\r\n",
						uri, lst->req_seq++);
			}
		}
#endif

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | SETUP request\n%s", __FUNCTION__, buf);
#endif

		if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
		{
			perror("send");
			MRTPSRC_DBG(ERROR, "%s | SETUP request send fail", __FUNCTION__);
			close(lst->rtsp_sock);
			memset(lst->ctrl, 0x00, 128);
			memset(lst->sps, 0x00, 128);
			memset(lst->pps, 0x00, 128);
			memset(lst->session, 0x00, 128);
			memset(lst->rtsp_validation, 0x00, 16);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_SESSION_FAIL;
			return RTN_FAIL;
		}

		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
#if 0
		cnt = 0;
		while(cnt < 10)
		{
			g_usleep(200*1000);
			len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, MSG_DONTWAIT | MSG_PEEK);
			if (len < 0 || errno == EAGAIN)
			{
				cnt++;
			}
			else
			{
				len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				if (len < 0)
				{
					g_usleep(50*1000);
					len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
				}
				break;
			}
		}
		if (cnt >= 10)
#else
		len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
		if (len < 0)
#endif
		{
			perror("recv");
			MRTPSRC_DBG(ERROR, "%s | SETUP(video) reply receive fail", __FUNCTION__);
			close(lst->rtsp_sock);
			memset(lst->ctrl, 0x00, 128);
			memset(lst->sps, 0x00, 128);
			memset(lst->pps, 0x00, 128);
			memset(lst->session, 0x00, 128);
			memset(lst->rtsp_validation, 0x00, 16);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_SESSION_FAIL;
			return RTN_FAIL;
		}
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | SETUP reply\n%s", __FUNCTION__, buf);
#endif

		/* SETUP(video) Reply msg parsing */
		{
			const gchar f_str_session_1[] = "Session: ";
			const gchar f_str_session_2[] = "Session:";
			//const gchar f_str_dltr[] = ";";
			//const gchar f_str_dltr_nl[] = "\r\n";

			gchar* ptr = NULL;
			gchar* ptr_tail = NULL;


			/* Find session string */
			if ((ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_session_1)) == NULL)
			{
				if ((ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_session_2)) == NULL)
				{
					MRTPSRC_DBG(WARN, "%s | SETUP(video) Session get failed", __FUNCTION__);
					close(lst->rtsp_sock);
					memset(lst->ctrl, 0x00, 128);
					memset(lst->sps, 0x00, 128);
					memset(lst->pps, 0x00, 128);
					memset(lst->session, 0x00, 128);
					memset(lst->rtsp_validation, 0x00, 16);
					lst->state = STATE_INITIAL;
					lst->rtsp_sock = (-1);
					rtspc_errno = ERR_OPEN_SESSION_FAIL;
					return RTN_FAIL;
				}
				else
				{
					ptr += 8; //strlen(f_str_session_2);
				}
			}
			else
			{
				ptr += 9; //strlen(f_str_session_1);
			}

			
			{
				gint __len;
				if((ptr_tail = _mrtp_get_endline(ptr, &__len)) != NULL)
				{
					gchar *tp;
					tp = ptr;
					while(ptr_tail > tp)
					{
						if(*tp == ';')
						{
							ptr_tail = tp;
							break;
						}
						tp++;
					}
				}
				else
				{
					MRTPSRC_DBG(ERROR, "%s | Session get fail", __FUNCTION__);
					close(lst->rtsp_sock);
					rtspc_errno = ERR_OPEN_SESSION_FAIL;
					return RTN_FAIL;
				}
			}

			memset(lst->session, 0x00, 128);
			memcpy(lst->session, ptr, (ptr_tail - ptr)>127?127:(ptr_tail-ptr));

			/* parsing unicast/multicast port */
			if(lst->rtp_protocol == RTP_UNICAST_UDP)
			{
				const gchar f_str_port[] = "server-port=";
				const gchar f_str_port1[] = "server_port=";
				if((ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_port)) == NULL)
				{
					if((ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_port1)) == NULL)
					{
						MRTPSRC_DBG(ERROR, "%s | Session get fail", __FUNCTION__);
						close(lst->rtsp_sock);
						close(lst->udp_sock[0]);
						close(lst->udp_sock[1]);
						//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
						rtspc_errno = ERR_OPEN_SESSION_FAIL;
						return RTN_FAIL;
					}
				}
				{
					ptr += 12;
					ptr_tail = g_strstr_len(ptr, MRTPSRC_NBUF_SZ, "-");
					if(ptr_tail != NULL)
					{
						lst->server_port[0] = strtoul(ptr, NULL, 10);

						struct sockaddr_in sin;

						memset(&sin, 0x00, sizeof(sin));
						sin.sin_family = AF_INET;
						sin.sin_addr.s_addr = ip_addr;
						sin.sin_port = htons(lst->server_port[0]);

						connect(lst->udp_sock[0], (struct sockaddr*) &sin, sizeof(struct sockaddr));
						ptr = ptr_tail + 1;
						//ptr_tail = strstr(ptr, ";");
						//if(ptr_tail != NULL)
						{
							lst->server_port[1] = strtoul(ptr, NULL, 10);

							memset(&sin, 0x00, sizeof(sin));
							sin.sin_family = AF_INET;
							sin.sin_addr.s_addr = ip_addr;
							sin.sin_port = htons(lst->server_port[1]);

							connect(lst->udp_sock[1], (struct sockaddr*) &sin, sizeof(struct sockaddr));
						}
					}
				}
			}
			else if(lst->rtp_protocol == RTP_MULTICAST_UDP)
			{
				// TODO parsing dest ip and port
				const gchar f_str_destip[] = "destination=";
				const gchar f_str_port[] = "port=";
				if((ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_destip)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Session get fail", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_SESSION_FAIL;
					return RTN_FAIL;
				}
				ptr += 12;
				ptr_tail = g_strstr_len(ptr, MRTPSRC_NBUF_SZ, ";");
				if(ptr_tail == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Session get fail", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_SESSION_FAIL;
					return RTN_FAIL;
				}
				memset(lst->dest_addr, 0x00, MRTPSRC_MAX_CB_LEN);
				memcpy(lst->dest_addr, ptr, (ptr_tail - ptr)>(MRTPSRC_MAX_CB_LEN-1)?(MRTPSRC_MAX_CB_LEN-1):(ptr_tail-ptr));	// dest ip
				if((ptr = g_strstr_len(buf, MRTPSRC_NBUF_SZ, f_str_port)) == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Session get fail", __FUNCTION__);
					close(lst->rtsp_sock);
					//memset(lst, 0x00, sizeof(MRTPSRC_STREAM_T));
					rtspc_errno = ERR_OPEN_SESSION_FAIL;
					return RTN_FAIL;
				}
				ptr += 5;
				ptr_tail = g_strstr_len(ptr, MRTPSRC_NBUF_SZ, "-");
				if(ptr_tail != NULL)
				{
					lst->server_port[0] = strtoul(ptr, NULL, 10);
					ptr = ptr_tail + 1;
					lst->server_port[1] = strtoul(ptr, NULL, 10);

					bind_rtp_multicast(lst);
				}

			}

#if PRINT_RTSP_MSG
			MRTPSRC_DBG(MINOR, "%s | session(%s)", __FUNCTION__, lst->session);
#endif
		}

//skip_setup:
		lst->state = STATE_SESSION_OBTAINED;
	/* RTSP command 'SETUP' end */
	}

#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | Preparing to receive", __FUNCTION__);
#endif

	/* Receive buffer memory allocation */
	{
		/* 
		 * Buffer allocation size
		 *   1st stream   9*128 KBytes
		 *   2nd stream   4*128 KBytes
		 *   3rd stream   128   KBytes
		 *   Audio stream 64    KBytes
		 */
		
		//lst->rbuf.lp = lst->rbuf.rbs + MRTPSRC_RING_GAP;
		lst->rbuf.lp = lst->rbuf.rbs;
		lst->rbuf.cp = lst->rbuf.rbs;
		//lst->rbuf.buf_remain = lst->buf_sz - MRTPSRC_RING_GAP;
		lst->rbuf.buf_remain = lst->buf_sz;
		lst->rbuf.buf_used = 0;
		lst->frame_head = NULL;
		lst->frame_tail = NULL;
#ifdef MRTPSRC_SUPPORT_SMART_CAMERA
		lst->check_sei =0;
		lst->sei_meta_1st=NULL;
		lst->sei_meta_2nd=NULL;
		lst->sei_event_1st=NULL;
		lst->sei_event_2nd=NULL;
#endif
	}

#if PRINT_RTSP_MSG
	MRTPSRC_DBG(MAJOR, "%s | PLAY cycle", __FUNCTION__);
#endif

	gchar uri_aggregate[256];
	gchar send_aggregate[512];

	snprintf(uri_aggregate, 256, "%s", lst->rtsp_addr);

	if (stream == STREAM_AUDIO && lst->state == STATE_SESSION_OBTAINED)
	{
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MAJOR, "%s | PLAY (%d,AUDIO_STREAM) requested", __FUNCTION__, ch_num);
#endif
		const gchar f_str_control[] = "rtsp://";
		gchar uri[256];

		if (lst->ctrl[0] != '\0')
		{
			if (g_strstr_len(lst->ctrl, 128, f_str_control) == NULL)
			{
				if (lst->is_gortsplib) 
					snprintf(uri, 256, "%s", lst->rtsp_addr);
				else
					snprintf(uri, 256, "%s/%s", lst->rtsp_cb, lst->ctrl);
			}
			else
			{
				snprintf(uri, 256, "%s", lst->ctrl);
			}
		}
		else
		{
			MRTPSRC_DBG(ERROR, "%s | SETUP(audio) NULL ctrl", __FUNCTION__);
			close(lst->rtsp_sock);
			memset(lst->ctrl, 0x00, 128);
			memset(lst->sps, 0x00, 128);
			memset(lst->pps, 0x00, 128);
			memset(lst->session, 0x00, 128);
			memset(lst->rtsp_validation, 0x00, 16);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_NO_AUDIO_CTRL;
			return RTN_FAIL;
		}

		/* PLAY request */
		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
		if (lst->realm[0] != '\0' && lst->nonce[0] != '\0')
		{
			char *method = "PLAY";

			get_rtsp_digest_auth_str(
					lst->username, lst->password,
					lst->realm, lst->nonce,
					lst->rtsp_addr, method, auth_str
			);
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"%s"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++,
						lst->session, auth_str);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"%s"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++,
						lst->session, auth_str);
			}
		}
		else
		{
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++, 
						lst->session);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++, 
						lst->session);
			}
		}

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | PLAY request\n%s", __FUNCTION__, buf);
#endif
		if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
		{
			perror("send");
			MRTPSRC_DBG(ERROR, "%s | PLAY request send fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
			return RTN_FAIL;
		}

		memset(buf, 0x00, MRTPSRC_NBUF_SZ);

		len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
		if (len < 0)
		{
			perror("recv");
			MRTPSRC_DBG(ERROR, "%s | PLAY reply receive fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
			return RTN_FAIL;
		}
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MINOR, "%s | PLAY reply\n%s", __FUNCTION__, buf);
#endif

		if ((g_strstr_len(buf, len, "460 Only Aggregate Operation Allowed") != NULL) ||
			(g_strstr_len(buf, len, "460 Only aggregate operation allowed") != NULL))
		{
			g_snprintf(send_aggregate, 512,
						"PLAY %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"Session: %s\r\n"
						"Range: npt=0.000-\r\n"
						"%s"
						"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
						"\r\n",
					uri_aggregate,
					lst->req_seq++,
					lst->session, auth_str);
#if PRINT_RTSP_MSG
			MRTPSRC_DBG(MINOR, "%s | PLAY AGGREGATE request\n%s", __FUNCTION__, buf);
#endif
			if (send(lst->rtsp_sock, send_aggregate, strlen(send_aggregate), 0) < 0)
			{
				perror("send");
				MRTPSRC_DBG(ERROR, "%s | PLAY AGGREGATE request send fail", __FUNCTION__);
				close(lst->rtsp_sock);
				lst->state = STATE_INITIAL;
				lst->rtsp_sock = (-1);
				rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
				return RTN_FAIL;
			}

			memset(buf, 0x00, MRTPSRC_NBUF_SZ);

			len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
			if (len < 0)
			{
				perror("recv");
				MRTPSRC_DBG(ERROR, "%s | PLAY AGGREGATE reply receive fail", __FUNCTION__);
				close(lst->rtsp_sock);
				lst->state = STATE_INITIAL;
				lst->rtsp_sock = (-1);
				rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
				return RTN_FAIL;
			}
#if PRINT_RTSP_MSG
			MRTPSRC_DBG(MINOR, "%s | PLAY AGGREGATE reply\n%s", __FUNCTION__, buf);
#endif
		}

		astreams[ch_num].state = STATE_STREAM_READY;
		rtspc_errno = ERR_NO_ERROR;

#if MRTPSRC_USE_STATISTICS
		stats.live_ch_cnt++;
#endif

		return RTN_OK;
	}

	{
		const gchar f_str_control[] = "rtsp://";
		gchar uri[256];

#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MAJOR, "%s | PLAY (%d,%s) requested", __FUNCTION__, ch_num, MRTPSRC_STREAM_STR[stream]);
#endif
#if 0
		snprintf(uri, 256, "%s", lst->rtsp_addr);
#else
		if (lst->ctrl[0] != '\0')
		{
			if (g_strstr_len(lst->ctrl, 128, f_str_control) == NULL)
			{
				if (lst->is_gortsplib) 
					snprintf(uri, 256, "%s", lst->rtsp_addr);
				else
					snprintf(uri, 256, "%s/%s", lst->rtsp_cb, lst->ctrl);
			}
			else
			{
				snprintf(uri, 256, "%s", lst->ctrl);
			}
		}
		else
		{
			MRTPSRC_DBG(ERROR, "%s | SETUP(video) NULL ctrl", __FUNCTION__);
			close(lst->rtsp_sock);
			memset(lst->ctrl, 0x00, 128);
			memset(lst->sps, 0x00, 128);
			memset(lst->pps, 0x00, 128);
			memset(lst->session, 0x00, 128);
			memset(lst->rtsp_validation, 0x00, 16);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_OPEN_NO_VIDEO_CTRL;
			return RTN_FAIL;
		}
#endif

		/* PLAY request */
		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
		if (lst->realm[0] != '\0' && lst->nonce[0] != '\0')
		{
			char *method = "PLAY";

			get_rtsp_digest_auth_str(
					lst->username, lst->password,
					lst->realm, lst->nonce,
					lst->rtsp_addr, method, auth_str
			);
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"%s"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++,
						lst->session, auth_str);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"%s"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++,
						lst->session, auth_str);
			}
		}
		else
		{
			if (model == MODEL_AMB_A2 || model == MODEL_AMB_D1)
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++,
						lst->session);
			}
			else
			{
				g_snprintf(buf, MRTPSRC_NBUF_SZ,
							"PLAY %s RTSP/1.0\r\n"
							"CSeq: %u\r\n"
							"Session: %s\r\n"
							"Range: npt=0.000-\r\n"
							"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
							"\r\n",
						uri,
						lst->req_seq++,
						lst->session);
			}
		}

		if (send(lst->rtsp_sock, buf, strlen(buf), 0) < 0)
		{
			perror("send");
			MRTPSRC_DBG(ERROR, "%s | PLAY request send fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
			return RTN_FAIL;
		}
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MAJOR, "%s | RTSP MSG send\n%s", __FUNCTION__, buf);
#endif
		memset(buf, 0x00, MRTPSRC_NBUF_SZ);
		len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);

		if (len < 0)
		{
			perror("recv");
			MRTPSRC_DBG(ERROR, "%s | PLAY reply receive fail", __FUNCTION__);
			close(lst->rtsp_sock);
			lst->state = STATE_INITIAL;
			lst->rtsp_sock = (-1);
			rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
			return RTN_FAIL;
		}

		if ((g_strstr_len(buf, len, "460 Only Aggregate Operation Allowed") != NULL) ||
			(g_strstr_len(buf, len, "460 Only aggregate operation allowed") != NULL))
		{
			g_snprintf(send_aggregate, 512,
						"PLAY %s RTSP/1.0\r\n"
						"CSeq: %u\r\n"
						"Session: %s\r\n"
						"Range: npt=0.000-\r\n"
						"%s"
						"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
						"\r\n",
					uri_aggregate,
					lst->req_seq++,
					lst->session, auth_str);
#if PRINT_RTSP_MSG
			MRTPSRC_DBG(MINOR, "%s | PLAY AGGREGATE request\n%s", __FUNCTION__, buf);
#endif
			if (send(lst->rtsp_sock, send_aggregate, strlen(send_aggregate), 0) < 0)
			{
				perror("send");
				MRTPSRC_DBG(ERROR, "%s | PLAY AGGREGATE request send fail", __FUNCTION__);
				close(lst->rtsp_sock);
				lst->state = STATE_INITIAL;
				lst->rtsp_sock = (-1);
				rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
				return RTN_FAIL;
			}

			memset(buf, 0x00, MRTPSRC_NBUF_SZ);

			len = recv(lst->rtsp_sock, buf, MRTPSRC_NBUF_SZ, 0);
			if (len < 0)
			{
				perror("recv");
				MRTPSRC_DBG(ERROR, "%s | PLAY AGGREGATE reply receive fail", __FUNCTION__);
				close(lst->rtsp_sock);
				lst->state = STATE_INITIAL;
				lst->rtsp_sock = (-1);
				rtspc_errno = ERR_PLAY_PLAY_REQ_FAIL;
				return RTN_FAIL;
			}
#if PRINT_RTSP_MSG
			MRTPSRC_DBG(MINOR, "%s | PLAY AGGREGATE reply\n%s", __FUNCTION__, buf);
#endif
		}

		lst->state = STATE_STREAM_READY;
#if PRINT_RTSP_MSG
		MRTPSRC_DBG(MAJOR, "%s | RTSP MSG recv\n%s", __FUNCTION__, buf);
#endif
	}

	rtspc_errno = ERR_NO_ERROR;

#if MRTPSRC_USE_STATISTICS
	stats.live_ch_cnt++;
#endif

	return RTN_OK;
}

extern void mrtpsrc_ho_enqueue(GobjBuddyBuffer *frame)
{
	MRTPSRC_RECORD_HO_Q _q_head = ho_head;

	rec_ho_q_lock();
	while (_q_head->next != NULL)
	{
		_q_head = _q_head->next;
	}

	{
		void *tmp_gst_ret = NULL;
		tmp_gst_ret = g_object_ref(frame);
		if(tmp_gst_ret == NULL) {
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(frame);
		}
		else
		{
			if(!GOBJ_IS_BUDDY_BUFFER (frame))
			{
				printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
				gst_buffer_debug(frame);
			}
			else
			{
				_q_head->next = (MRTPSRC_RECORD_HO_Q) mrtpsrc_alloc_heap(sizeof(struct _REC_HANDOFF_QUEUE_), __FILE__, __FUNCTION__, __LINE__);
				if (_q_head->next == NULL)
				{
					MRTPSRC_DBG(ERROR, "%s | Memory allocation fail", __FUNCTION__);
					gst_buffer_debug(frame);
					rec_ho_q_unlock();
					return;
				}
				memset(_q_head->next, 0x00, sizeof(struct _REC_HANDOFF_QUEUE_));
				_q_head->next->frame = frame;
				_q_head->next->next = NULL;
			}
		}
	}


	rec_ho_q_unlock();
}

static gint send_audio_to(gint ch_num)
{
	MRTPSRC_STREAM_T *lst = mrtpsrc_get_stream(ch_num, 0);	// 1st stream only!
	MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(ch_num);
	guchar buf[2048];
	guint part1, part2;
	guchar* p1;
	guint16* p2;
	gint len = 0;

if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | Enter", __FUNCTION__);
#endif

#if 0
	if (lch->state != STATE_PLAYING)
	{
#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MAJOR, "%s | No where to send 1", __FUNCTION__);
#endif
		lst->abuf.len = 0;
		lst->abuf.start = 0;
		return RTN_OK;
	}
#endif
	if (lst->state != STATE_PLAYING)
	{
#if MRTPSRC_FULL_DBG
		MRTPSRC_DBG(MAJOR, "%s | No where to send 2", __FUNCTION__);
#endif
		lst->abuf.len = 0;
		lst->abuf.start = 0;
		return RTN_OK;
	}

	MRTPSRC_DBG(MINOR, "%s | Audio buffer len(%u)", __FUNCTION__, lst->abuf.len);
	if (lst->abuf.len == 0)
	{
		return RTN_OK;
	}
	else if (lst->abuf.len > MRTPSRC_AUDIO_MAX_SZ - 2000)
	{
		MRTPSRC_DBG(WARN, "%s | Audio buffer len(%u) exceed max size(%u), reset to max size", __FUNCTION__, lst->abuf.len, MRTPSRC_AUDIO_MAX_SZ);
		return RTN_FAIL;
	}

#if 0
	part1 = MRTPSRC_AUDIO_MAX_SZ - lst->abuf.start;
	if (part1 >= lst->abuf.len)
	{
		part1 = lst->abuf.len;
		part2 = 0;
	}
	else
	{
		part2 = lst->abuf.len - part1;
	}
#endif

	//memset(buf, 0x00, 1280);

	if(lst->model != MODEL_ONVIF_GRUNDIG)
	{
		/* RTSP header */
		p1 = buf;
		*p1++ = 0x24;
		*p1++ = 0x1;
		p2 = (guint16*) p1;
		part1 = 800;
		part2 = lst->abuf.len - part1;
		*p2++ = htons(4+part1);

		/* Audio header */
		p1 = (guchar*) p2;
		*p1++ = 0xf;
		*p1++ = 0xf;
		p2 = (guint16*) p1;
		*p2++ = htons(part1);
		p1 = (guchar*) p2;

		/* Audio Data */
		memcpy(p1, &lst->abuf.data[0], part1);

		/* Audio Send */
		switch(lst->rtp_protocol)
		{
		case RTP_UNICAST_UDP:
			len = send(lst->udp_sock[1], buf + 4, 4+part1, MSG_DONTWAIT);
			break;

		case RTP_MULTICAST_UDP:
			len = send(lst->udp_sock[1], buf + 4, 4+part1, MSG_DONTWAIT);
			break;

		case RTP_OVER_RTSP_TCP:
			len = send(lst->rtsp_sock, buf, 4+4+part1, MSG_DONTWAIT);
			break;
		}
	}
	else
	{
		// grundig only
		/* rtp header */
		part1 = 1000;
		part2 = lst->abuf.len - part1;

		MRTPSRC_RTP_HEADER_T rtp_audio;
		memset(&rtp_audio, 0x00, sizeof(MRTPSRC_RTP_HEADER_T));
		rtp_audio.v = 0x2;
		rtp_audio.p = 0;
		rtp_audio.x = 0;
		rtp_audio.cc = 0;
		rtp_audio.m = 0;
		//rtp_audio.pt = 0x8;	// PCMA
		rtp_audio.pt = 0x0;	// PCMU

		rtp_audio.seq = htons(lst->audio_out_seq++);
		rtp_audio.timestamp = htonl(lst->audio_out_seq * 1000 + 1000);
		rtp_audio.ssrc = htonl(lst->sr_ssrc);

		p1 = buf;
		memcpy(p1, &rtp_audio, sizeof(MRTPSRC_RTP_HEADER_T));

		/* Audio Data */
		memcpy(p1 + sizeof(MRTPSRC_RTP_HEADER_T), &lst->abuf.data[0], part1);

		/* Audio Send */
		len = send(lst->audio_out_sock, buf, sizeof(MRTPSRC_RTP_HEADER_T) + part1, MSG_DONTWAIT);

	}

	/* Audio Buffer Move */
	memmove(&lst->abuf.data[0], &lst->abuf.data[part1], part2);
	lst->abuf.start = 0;
	lst->abuf.len = part2;

	if (len < 0)
	{
		if (errno == 32)	// broken pipe
		{
			lch->state = STATE_RECONN_REQ;
			stats.reconn_audio_send[ch_num]++;
		}
		MRTPSRC_DBG(ERROR, "%s | Audio send fail(%d)", __FUNCTION__, errno);
		return RTN_FAIL;
	}

#if 0
	/* RTSP header */
	p1 = buf;
	*p1++ = 0x24;
	*p1++ = 0x1;
	p2 = (guint16*) p1;
	part1 = (lst->abuf.len < 1000) ? lst->abuf.len:1000;
	part2 = lst->abuf.len - part1;
	*p2++ = htons(4+part1);    // Audio header len + Audio data

	/* Audio header */
	p1 = (guchar*) p2;
	*p1++ = 0xf;
	*p1++ = 0xf;
	p2 = (guint16*) p1;
	*p2++ = htons(part1);
	p1 = (guchar*) p2;

	/* Audio Data */
	memcpy(p1, &lst->abuf.data[0], part1);

	len = send(lst->rtsp_sock, buf, 4+4+part1, MSG_DONTWAIT);
	if (part2 != 0)
	{
		memmove(&lst->abuf.data[0], &lst->abuf.data[part1], part2);
	}

	lst->abuf.start = 0;
	lst->abuf.len = part2;

	if (len < 0)
	{
		if (errno == 32)	// broken pipe
		{
			lch->state = STATE_RECONN_REQ;
			stats.reconn_audio_send[ch_num]++;
		}
		MRTPSRC_DBG(ERROR, "%s | Audio send fail(%d)", __FUNCTION__, errno);
		return RTN_FAIL;
	}
#endif

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | Audio send success (len:%d)", __FUNCTION__, len);
#endif

	return RTN_OK;
}

/* Base64 decoding */
static size_t b64_decode_binary_to_buffer(guchar *p_dst, size_t i_dst, const char *p_src )
{
    static const int b64[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
    };
    uint8_t *p_start = p_dst;
    uint8_t *p = (uint8_t *)p_src;

    int i_level;
    int i_last;

    for( i_level = 0, i_last = 0; (size_t)( p_dst - p_start ) < i_dst && *p != '\0'; p++ )
    {
        const int c = b64[(guint)*p];
        if( c == -1 )
            continue;

        switch( i_level )
        {
            case 0:
                i_level++;
                break;
            case 1:
                *p_dst++ = ( i_last << 2 ) | ( ( c >> 4)&0x03 );
                i_level++;
                break;
            case 2:
                *p_dst++ = ( ( i_last << 4 )&0xf0 ) | ( ( c >> 2 )&0x0f );
                i_level++;
                break;
            case 3:
                *p_dst++ = ( ( i_last &0x03 ) << 6 ) | c;
                i_level = 0;
        }
        i_last = c;
    }

    return p_dst - p_start;
}

static GTimeVal sm_nowtime;
static GTimeVal sm_prevtime;
static void _session_manager_func(void *param)
{
	gint i = 0;
	gint j = 0;
	gint stat_count = 0;
	gint result = 0;
#if MRTPSRC_USE_STATISTICS
	gchar *buf = NULL;
	gchar *buf_reconn = NULL;
	gchar *buf_ts = NULL;
	gchar *buf_resol = NULL;
	guint live_ch_cnt = 0;
#endif
	MRTPSRC_AUDIO_RING_T *aring = mrtpsrc_get_audio_ring();
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *filter = gst_mrtp_src_get_object();
#endif

	buf = (char*) mrtpsrc_alloc_heap(2048, __FILE__, __FUNCTION__, __LINE__);
	if (buf == NULL)
	{
		MRTPSRC_DBG(ERROR, "%s | Memory allocation fail", __FUNCTION__);
		return;
	}
	memset(buf, 0x00, 2048);

	buf_reconn = (char*) mrtpsrc_alloc_heap(2048, __FILE__, __FUNCTION__, __LINE__);
	if (buf_reconn == NULL)
	{
		MRTPSRC_DBG(ERROR, "%s | Memory allocation fail", __FUNCTION__);
		mrtpsrc_free_heap(buf, __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	memset(buf_reconn, 0x00, 2048);

	buf_ts = (char*) mrtpsrc_alloc_heap(2048, __FILE__, __FUNCTION__, __LINE__);
	if (buf_ts == NULL)
	{
		MRTPSRC_DBG(ERROR, "%s | Memory allocation fail", __FUNCTION__);
		mrtpsrc_free_heap(buf, __FILE__, __FUNCTION__, __LINE__);
		mrtpsrc_free_heap(buf_reconn, __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	memset(buf_ts, 0x00, 2048);

	buf_resol = (char*) mrtpsrc_alloc_heap(2048, __FILE__, __FUNCTION__, __LINE__);
	if (buf_resol == NULL)
	{
		MRTPSRC_DBG(ERROR, "%s | Memory allocation fail", __FUNCTION__);
		mrtpsrc_free_heap(buf, __FILE__, __FUNCTION__, __LINE__);
		mrtpsrc_free_heap(buf_reconn, __FILE__, __FUNCTION__, __LINE__);
		mrtpsrc_free_heap(buf_ts, __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	memset(buf_resol, 0x00, 2048);

#if 1
	{
		int policy;
		struct sched_param sched;
		pthread_t thread;
		policy = SCHED_FIFO;
		thread = pthread_self();

		sched.sched_priority = sched_get_priority_max(policy) - 1;
		//sched.sched_priority = sched_get_priority_min(policy) + 1;
		printf("[mrtpsrc]    sched.sched_priority %d \n",sched.sched_priority);
		printf("[mrtpsrc]    set ret = %d\n", pthread_setschedparam (thread, policy, &sched));
		printf("[mrtpsrc]    get ret = %d\n", pthread_getschedparam (thread, &policy, &sched));
		printf("[mrtpsrc]    set realtime policy = %d\n", policy);
	}
#endif

	MRTPSRC_DBG(MAJOR, "%s | session manager start", __FUNCTION__);

	while(MRTPSRC_ALWAYS)
	{
		g_usleep(100*1000);

		g_get_current_time(&sm_nowtime);
		if (sm_nowtime.tv_sec != sm_prevtime.tv_sec)
		{
			// printf("[mrtpsrc] %s:%d | mrtpsrc session manager running\n", __FUNCTION__, __LINE__);
			sm_prevtime = sm_nowtime;
		}

		live_ch_cnt = 0;

		for (i = 0; i < MRTPSRC_MAX_CH; i++)
		{
			if (channels[i].state == STATE_RECONN_REQ)
			//if (MRTPSRC_NEVER)
			{
				printf("[mrtpsrc] %s:%d | CH(%d) reconnection started\n", __FUNCTION__, __LINE__, i);
				if(channels[i].model_code == MODEL_ONTHEFLY_CHEAT)
				{
#ifndef NMZ_STANDLONE_MODE
					if (filter == NULL) { continue; }
					if (filter->cmd_callback == NULL) { continue; }
					filter->cmd_callback(21, i, 0, 0, filter->cmd_user_data);
#endif
					channels[i].state = STATE_PLAYING;
				}

				/* reconnection start */
				mrtpsrc_closesync_lock();
				for (j = 0; j < channels[i].active_video_cnt; j++)
				{
					mrtpsrc_pause_stream(i,j);
				}

				for (j = 0; j < channels[i].is_audio_activated; j++)
				{
					mrtpsrc_pause_stream(i, STREAM_AUDIO);
				}

				if(channels[i].is_metadata_on)
				{
					mrtpsrc_pause_stream(i, STREAM_META);
				}
				mrtpsrc_closesync_unlock();

#ifndef NMZ_STANDLONE_MODE
				if (filter != NULL && filter->cmd_callback != NULL)
				{
					filter->cmd_callback(2, i, 0, 301/*rate control*/, filter->cmd_user_data);
				}
#endif
				for (j = 0; j < channels[i].active_video_cnt; j++)
				{
					result = mrtpsrc_resume_stream(i,j);
					if (result == RTN_FAIL)
					{
						MRTPSRC_DBG(ERROR, "%s | video resume failed(%d,%d)", __FUNCTION__, i, j);
						mrtpsrc_closesync_lock();
						mrtpsrc_close_stream(i, STREAM_1ST);
						mrtpsrc_close_stream(i, STREAM_2ND);
#if MRTPSRC_SUPPORT_3RD_STREAM
						mrtpsrc_close_stream(i, STREAM_3RD);
#endif
						mrtpsrc_close_stream(i, STREAM_AUDIO);
						mrtpsrc_close_stream(i, STREAM_META);
						channels[i].state = STATE_REBOOT_REQ;
						mrtpsrc_closesync_unlock();
						goto skip_audio;
					}
				}
				if (channels[i].state == STATE_RECONN_REQ && channels[i].is_audio_activated)
				{
					result = mrtpsrc_resume_stream(i, STREAM_AUDIO);
					if (result == RTN_FAIL)
					{
						MRTPSRC_DBG(ERROR, "%s | audio resume failed(%d)", __FUNCTION__, i);
						mrtpsrc_closesync_lock();
						mrtpsrc_close_stream(i, STREAM_1ST);
						mrtpsrc_close_stream(i, STREAM_2ND);
#if MRTPSRC_SUPPORT_3RD_STREAM
						mrtpsrc_close_stream(i, STREAM_3RD);
#endif
						mrtpsrc_close_stream(i, STREAM_AUDIO);
						mrtpsrc_close_stream(i, STREAM_META);
						channels[i].state = STATE_REBOOT_REQ;
						mrtpsrc_closesync_unlock();
						goto skip_audio;
					}
				}

				if (channels[i].state == STATE_RECONN_REQ && channels[i].is_metadata_on)
				{
					result = mrtpsrc_resume_stream(i, STREAM_META);
					if (result == RTN_FAIL)
					{
						MRTPSRC_DBG(ERROR, "%s | meta resume failed(%d)", __FUNCTION__, i);
						mrtpsrc_closesync_lock();
						mrtpsrc_close_stream(i, STREAM_1ST);
						mrtpsrc_close_stream(i, STREAM_2ND);
#if MRTPSRC_SUPPORT_3RD_STREAM
						mrtpsrc_close_stream(i, STREAM_3RD);
#endif
						mrtpsrc_close_stream(i, STREAM_AUDIO);
						mrtpsrc_close_stream(i, STREAM_META);
						channels[i].state = STATE_REBOOT_REQ;
						mrtpsrc_closesync_unlock();
					}
				}

skip_audio:
#if MRTPSRC_USE_STATISTICS
				stats.reconn_cnt[i]++;
				stats.reconn_cnt[i+32]++;
				stats.cbc_frame_cnt[i] = 0;
				stats.cbc_frame_cnt[i+32] = 0;
#endif
				//continue;
				/* reconnection end */
			}

			if (channels[i].state == STATE_REBOOT_REQ)
			{
				MRTPSRC_DBG(WARN, "%s | mrtpsrc request to reboot CH(%d)", __FUNCTION__, i);
				//reset connectivity check	
				reinitialize_channel_stream_connectivity(i, -1);
#ifndef NMZ_STANDLONE_MODE
				if (filter != NULL && filter->cmd_callback != NULL)
				{
					filter->cmd_callback(1, i, 0, ERR_TRDN_INTERNAL, filter->cmd_user_data);
					MRTPSRC_DBG(WARN, "%s | mrtpsrc request delivered CH(%d)", __FUNCTION__, i);
				}
#endif
				channels[i].state = STATE_INITIAL;
				vstreams[i].state = STATE_INITIAL;
				vstreams[i+32].state = STATE_INITIAL;
#if MRTPSRC_SUPPORT_3RD_STREAM
				vstreams[i+32].state = STATE_INITIAL;
#endif
				astreams[i].state = STATE_INITIAL;
				mstreams[i].state = STATE_INITIAL;
			}
			else
			{
				channels[i].state = STATE_PLAYING;
			}

#if MRTPSRC_USE_STATISTICS
			if (vstreams[i].state >= STATE_STREAM_READY && vstreams[i].state <= STATE_DROP_P_ALL)
			{
				live_ch_cnt++;
			}
			if (vstreams[i+32].state >= STATE_STREAM_READY && vstreams[i+32].state <= STATE_DROP_P_ALL)
			{
				live_ch_cnt++;
			}
#if MRTPSRC_SUPPORT_3RD_STREAM
			if (vstreams[i+32].state >= STATE_STREAM_READY && vstreams[i+32].state <= STATE_DROP_P_ALL)
			{
				live_ch_cnt++;
			}
#endif
			if (astreams[i].state >= STATE_STREAM_READY && astreams[i].state <= STATE_DROP_P_ALL)
			{
				live_ch_cnt++;
			}

			if (mstreams[i].state >= STATE_STREAM_READY && mstreams[i].state <= STATE_DROP_P_ALL)
			{
				live_ch_cnt++;
			}
#endif
		}

#if MRTPSRC_USE_STATISTICS
		if (mrtp_now_time.tv_sec >= prev_time.tv_sec + 3 || mrtp_now_time.tv_sec <= prev_time.tv_sec - 3)
		{
			/* RTCP msg send */
			static unsigned int _to_cam_seq = 0;
			if ((++_to_cam_seq % 4) == 0)	// every 12 seconds
			{
				int rtcp_id = 0;

				for (rtcp_id = 0; rtcp_id < MRTPSRC_MAX_CH; rtcp_id++)
				{
					if (vstreams[rtcp_id].state == STATE_PLAYING)
					{
						//MRTPSRC_DBG(IMPACT, "RTCP send");
						_mrtpsrc_send_rtcp(&vstreams[rtcp_id]);
						if (revalidate[rtcp_id] == 1)
						{
							//MRTPSRC_DBG(IMPACT, "RTSP validation");
							send_rtsp_validation(&vstreams[rtcp_id]);
						}
					}
					if (vstreams[rtcp_id+32].state == STATE_PLAYING)
					{
						_mrtpsrc_send_rtcp(&vstreams[rtcp_id+32]);
						if (revalidate[rtcp_id] == 1)
						{
							send_rtsp_validation(&vstreams[rtcp_id+32]);
						}
					}
#if MRTPSRC_SUPPORT_3RD_STREAM
					if (vstreams[rtcp_id+32].state == STATE_PLAYING)
					{
						_mrtpsrc_send_rtcp(&vstreams[rtcp_id+32]);
						if (revalidate[rtcp_id] == 1)
						{
							send_rtsp_validation(&vstreams[rtcp_id+32]);
						}
					}
#endif
					if (astreams[rtcp_id].state == STATE_PLAYING)
					{
						// Xiongmai exception
						if (astreams[rtcp_id].resolution != 100)
						{
							_mrtpsrc_send_rtcp(&astreams[rtcp_id]);
							if (revalidate[rtcp_id] == 1)
							{
								send_rtsp_validation(&astreams[rtcp_id]);
							}
						}
					}
					if (mstreams[rtcp_id].state == STATE_PLAYING || mstreams[rtcp_id].state == STATE_STREAM_READY)
					{
						_mrtpsrc_send_rtcp(&mstreams[rtcp_id]);
						if (revalidate[rtcp_id] == 1)
						{
							send_rtsp_validation(&mstreams[rtcp_id]);
						}
					}
				}
			}
			stat_count++;
			g_snprintf(buf, 2048,
"============================= %ld.%06ld =============================\n"
"  live_ch(%u) read(%llu) read_miss(%llu) read_bytes(%llu)\n"
"  frames(%u) i_frames(%u) p_frames(%u) audio_frames(%u)\n"
"  handoff_cnt(%u) p_frame_dropped(%u)\n"
"  systime_chg(%u) ts_err(%u) ts_compensaion(%u)\n",
				mrtp_now_time.tv_sec, mrtp_now_time.tv_usec,
				live_ch_cnt, stats.recv_cnt, stats.recv_miss, stats.recv_bytes,
				stats.frame_cnt, stats.i_cnt, stats.p_cnt, stats.audio_cnt,
				stats.handoff_cnt, stats.drop_cnt,
				stats.ts_sys_cnt, stats.ts_err_cnt, stats.ts_compen_cnt);

			if (stat_count % 3 == 0) {
				g_snprintf(buf_reconn, 2048,
"-----------------------------------------------------------------------------\n"
"  reconnect\n"
"    server_close[%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    sock_corrupt[%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    ring_full   [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    magic_no_A  [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    magic_no_B  [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    magic_no_C  [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    mem_alloc   [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    cmem_alloc  [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    vlen_fail   [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    systime_chg [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    ts_err      [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    audio_send  [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"  reboot\n"
"    disconn     [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    cont_mem    [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    cont_cmem   [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"  frame drop\n"
"    ssrc_1st    [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    ssrc_2nd    [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    seq_1st     [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n"
"    seq_2nd     [%u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u, %u,%u,%u,%u]\n",
				stats.reconn_server_close[0]   , stats.reconn_server_close[1],
				stats.reconn_server_close[2]   , stats.reconn_server_close[3],
				stats.reconn_server_close[4]   , stats.reconn_server_close[5],
				stats.reconn_server_close[6]   , stats.reconn_server_close[7],
				stats.reconn_server_close[8]   , stats.reconn_server_close[9],
				stats.reconn_server_close[10]  , stats.reconn_server_close[11],
				stats.reconn_server_close[12]  , stats.reconn_server_close[13],
				stats.reconn_server_close[14]  , stats.reconn_server_close[15],
                stats.reconn_server_close[16] , stats.reconn_server_close[17],
                stats.reconn_server_close[18] , stats.reconn_server_close[19],
                stats.reconn_server_close[20] , stats.reconn_server_close[21],
                stats.reconn_server_close[22] , stats.reconn_server_close[23],
                stats.reconn_server_close[24] , stats.reconn_server_close[25],
                stats.reconn_server_close[26] , stats.reconn_server_close[27],
                stats.reconn_server_close[28] , stats.reconn_server_close[29],
                stats.reconn_server_close[30] , stats.reconn_server_close[31],
                
				stats.reconn_sock_corrupt[0]   , stats.reconn_sock_corrupt[1],
				stats.reconn_sock_corrupt[2]   , stats.reconn_sock_corrupt[3],
				stats.reconn_sock_corrupt[4]   , stats.reconn_sock_corrupt[5],
				stats.reconn_sock_corrupt[6]   , stats.reconn_sock_corrupt[7],
				stats.reconn_sock_corrupt[8]   , stats.reconn_sock_corrupt[9],
				stats.reconn_sock_corrupt[10]  , stats.reconn_sock_corrupt[11],
				stats.reconn_sock_corrupt[12]  , stats.reconn_sock_corrupt[13],
				stats.reconn_sock_corrupt[14]  , stats.reconn_sock_corrupt[15],
                stats.reconn_sock_corrupt[16]  , stats.reconn_sock_corrupt[17],
                stats.reconn_sock_corrupt[18]  , stats.reconn_sock_corrupt[19],
                stats.reconn_sock_corrupt[20]  , stats.reconn_sock_corrupt[21],
                stats.reconn_sock_corrupt[22]  , stats.reconn_sock_corrupt[23],
                stats.reconn_sock_corrupt[24]  , stats.reconn_sock_corrupt[25],
                stats.reconn_sock_corrupt[26]  , stats.reconn_sock_corrupt[27],
                stats.reconn_sock_corrupt[28]  , stats.reconn_sock_corrupt[29],
                stats.reconn_sock_corrupt[30]  , stats.reconn_sock_corrupt[31],
                
				stats.reconn_ring_full[0]      , stats.reconn_ring_full[1],
				stats.reconn_ring_full[2]      , stats.reconn_ring_full[3],
				stats.reconn_ring_full[4]      , stats.reconn_ring_full[5],
				stats.reconn_ring_full[6]      , stats.reconn_ring_full[7],
				stats.reconn_ring_full[8]      , stats.reconn_ring_full[9],
				stats.reconn_ring_full[10]     , stats.reconn_ring_full[11],
				stats.reconn_ring_full[12]     , stats.reconn_ring_full[13],
				stats.reconn_ring_full[14]     , stats.reconn_ring_full[15],
                stats.reconn_ring_full[16]     , stats.reconn_ring_full[17],
                stats.reconn_ring_full[18]     , stats.reconn_ring_full[19],
                stats.reconn_ring_full[20]     , stats.reconn_ring_full[21],
                stats.reconn_ring_full[22]     , stats.reconn_ring_full[23],
                stats.reconn_ring_full[24]     , stats.reconn_ring_full[25],
                stats.reconn_ring_full[26]     , stats.reconn_ring_full[27],
                stats.reconn_ring_full[28]     , stats.reconn_ring_full[29],
                stats.reconn_ring_full[30]     , stats.reconn_ring_full[31],

				stats.reconn_wrong_magic[0][0] , stats.reconn_wrong_magic[0][1],
				stats.reconn_wrong_magic[0][2] , stats.reconn_wrong_magic[0][3],
				stats.reconn_wrong_magic[0][4] , stats.reconn_wrong_magic[0][5],
				stats.reconn_wrong_magic[0][6] , stats.reconn_wrong_magic[0][7],
				stats.reconn_wrong_magic[0][8] , stats.reconn_wrong_magic[0][9],
				stats.reconn_wrong_magic[0][10], stats.reconn_wrong_magic[0][11],
				stats.reconn_wrong_magic[0][12], stats.reconn_wrong_magic[0][13],
				stats.reconn_wrong_magic[0][14], stats.reconn_wrong_magic[0][15],
                stats.reconn_wrong_magic[0][16], stats.reconn_wrong_magic[0][17],
                stats.reconn_wrong_magic[0][18], stats.reconn_wrong_magic[0][19],
                stats.reconn_wrong_magic[0][20], stats.reconn_wrong_magic[0][21],
                stats.reconn_wrong_magic[0][22], stats.reconn_wrong_magic[0][23],
                stats.reconn_wrong_magic[0][24], stats.reconn_wrong_magic[0][25],
                stats.reconn_wrong_magic[0][26], stats.reconn_wrong_magic[0][27],
                stats.reconn_wrong_magic[0][28], stats.reconn_wrong_magic[0][29],
                stats.reconn_wrong_magic[0][30], stats.reconn_wrong_magic[0][31],

				stats.reconn_wrong_magic[1][0] , stats.reconn_wrong_magic[1][1],
				stats.reconn_wrong_magic[1][2] , stats.reconn_wrong_magic[1][3],
				stats.reconn_wrong_magic[1][4] , stats.reconn_wrong_magic[1][5],
				stats.reconn_wrong_magic[1][6] , stats.reconn_wrong_magic[1][7],
				stats.reconn_wrong_magic[1][8] , stats.reconn_wrong_magic[1][9],
				stats.reconn_wrong_magic[1][10], stats.reconn_wrong_magic[1][11],
				stats.reconn_wrong_magic[1][12], stats.reconn_wrong_magic[1][13],
				stats.reconn_wrong_magic[1][14], stats.reconn_wrong_magic[1][15],
                stats.reconn_wrong_magic[1][16], stats.reconn_wrong_magic[1][17],
                stats.reconn_wrong_magic[1][18], stats.reconn_wrong_magic[1][19],
                stats.reconn_wrong_magic[1][20], stats.reconn_wrong_magic[1][21],
                stats.reconn_wrong_magic[1][22], stats.reconn_wrong_magic[1][23],
                stats.reconn_wrong_magic[1][24], stats.reconn_wrong_magic[1][25],
                stats.reconn_wrong_magic[1][26], stats.reconn_wrong_magic[1][27],
                stats.reconn_wrong_magic[1][28], stats.reconn_wrong_magic[1][29],
                stats.reconn_wrong_magic[1][30], stats.reconn_wrong_magic[1][31],

				stats.reconn_wrong_magic[2][0] , stats.reconn_wrong_magic[2][1],
				stats.reconn_wrong_magic[2][2] , stats.reconn_wrong_magic[2][3],
				stats.reconn_wrong_magic[2][4] , stats.reconn_wrong_magic[2][5],
				stats.reconn_wrong_magic[2][6] , stats.reconn_wrong_magic[2][7],
				stats.reconn_wrong_magic[2][8] , stats.reconn_wrong_magic[2][9],
				stats.reconn_wrong_magic[2][10], stats.reconn_wrong_magic[2][11],
				stats.reconn_wrong_magic[2][12], stats.reconn_wrong_magic[2][13],
				stats.reconn_wrong_magic[2][14], stats.reconn_wrong_magic[2][15],
                stats.reconn_wrong_magic[2][16], stats.reconn_wrong_magic[2][17],
                stats.reconn_wrong_magic[2][18], stats.reconn_wrong_magic[2][19],
                stats.reconn_wrong_magic[2][20], stats.reconn_wrong_magic[2][21],
                stats.reconn_wrong_magic[2][22], stats.reconn_wrong_magic[2][23],
                stats.reconn_wrong_magic[2][24], stats.reconn_wrong_magic[2][25],
                stats.reconn_wrong_magic[2][26], stats.reconn_wrong_magic[2][27],
                stats.reconn_wrong_magic[2][28], stats.reconn_wrong_magic[2][29],
                stats.reconn_wrong_magic[2][30], stats.reconn_wrong_magic[2][31],

				stats.reconn_mem_fail[0]       , stats.reconn_mem_fail[1],
				stats.reconn_mem_fail[2]       , stats.reconn_mem_fail[3],
				stats.reconn_mem_fail[4]       , stats.reconn_mem_fail[5],
				stats.reconn_mem_fail[6]       , stats.reconn_mem_fail[7],
				stats.reconn_mem_fail[8]       , stats.reconn_mem_fail[9],
				stats.reconn_mem_fail[10]      , stats.reconn_mem_fail[11],
				stats.reconn_mem_fail[12]      , stats.reconn_mem_fail[13],
				stats.reconn_mem_fail[14]      , stats.reconn_mem_fail[15],
                stats.reconn_mem_fail[16]      , stats.reconn_mem_fail[17],
                stats.reconn_mem_fail[18]      , stats.reconn_mem_fail[19],
                stats.reconn_mem_fail[20]      , stats.reconn_mem_fail[21],
                stats.reconn_mem_fail[22]      , stats.reconn_mem_fail[23],
                stats.reconn_mem_fail[24]      , stats.reconn_mem_fail[25],
                stats.reconn_mem_fail[26]      , stats.reconn_mem_fail[27],
                stats.reconn_mem_fail[28]      , stats.reconn_mem_fail[29],
                stats.reconn_mem_fail[30]      , stats.reconn_mem_fail[31],
                
				stats.reconn_cmem_fail[0]      , stats.reconn_cmem_fail[1],
				stats.reconn_cmem_fail[2]      , stats.reconn_cmem_fail[3],
				stats.reconn_cmem_fail[4]      , stats.reconn_cmem_fail[5],
				stats.reconn_cmem_fail[6]      , stats.reconn_cmem_fail[7],
				stats.reconn_cmem_fail[8]      , stats.reconn_cmem_fail[9],
				stats.reconn_cmem_fail[10]     , stats.reconn_cmem_fail[11],
				stats.reconn_cmem_fail[12]     , stats.reconn_cmem_fail[13],
				stats.reconn_cmem_fail[14]     , stats.reconn_cmem_fail[15],
                stats.reconn_cmem_fail[16]     , stats.reconn_cmem_fail[17],
                stats.reconn_cmem_fail[18]     , stats.reconn_cmem_fail[19],
                stats.reconn_cmem_fail[20]     , stats.reconn_cmem_fail[21],
                stats.reconn_cmem_fail[22]     , stats.reconn_cmem_fail[23],
                stats.reconn_cmem_fail[24]     , stats.reconn_cmem_fail[25],
                stats.reconn_cmem_fail[26]     , stats.reconn_cmem_fail[27],
                stats.reconn_cmem_fail[28]     , stats.reconn_cmem_fail[29],
                stats.reconn_cmem_fail[30]     , stats.reconn_cmem_fail[31],

				stats.reconn_vlen_fail[0]      , stats.reconn_vlen_fail[1],
				stats.reconn_vlen_fail[2]      , stats.reconn_vlen_fail[3],
				stats.reconn_vlen_fail[4]      , stats.reconn_vlen_fail[5],
				stats.reconn_vlen_fail[6]      , stats.reconn_vlen_fail[7],
				stats.reconn_vlen_fail[8]      , stats.reconn_vlen_fail[9],
				stats.reconn_vlen_fail[10]     , stats.reconn_vlen_fail[11],
				stats.reconn_vlen_fail[12]     , stats.reconn_vlen_fail[13],
				stats.reconn_vlen_fail[14]     , stats.reconn_vlen_fail[15],
                stats.reconn_vlen_fail[16]     , stats.reconn_vlen_fail[17],
                stats.reconn_vlen_fail[18]     , stats.reconn_vlen_fail[19],
                stats.reconn_vlen_fail[20]     , stats.reconn_vlen_fail[21],
                stats.reconn_vlen_fail[22]     , stats.reconn_vlen_fail[23],
                stats.reconn_vlen_fail[24]     , stats.reconn_vlen_fail[25],
                stats.reconn_vlen_fail[26]     , stats.reconn_vlen_fail[27],
                stats.reconn_vlen_fail[28]     , stats.reconn_vlen_fail[29],
                stats.reconn_vlen_fail[30]     , stats.reconn_vlen_fail[31],

				stats.reconn_systime[0]        , stats.reconn_systime[1],
				stats.reconn_systime[2]        , stats.reconn_systime[3],
				stats.reconn_systime[4]        , stats.reconn_systime[5],
				stats.reconn_systime[6]        , stats.reconn_systime[7],
				stats.reconn_systime[8]        , stats.reconn_systime[9],
				stats.reconn_systime[10]       , stats.reconn_systime[11],
				stats.reconn_systime[12]       , stats.reconn_systime[13],
				stats.reconn_systime[14]       , stats.reconn_systime[15],
                stats.reconn_systime[16]       , stats.reconn_systime[17],
                stats.reconn_systime[18]       , stats.reconn_systime[19],
                stats.reconn_systime[20]       , stats.reconn_systime[21],
                stats.reconn_systime[22]       , stats.reconn_systime[23],
                stats.reconn_systime[24]       , stats.reconn_systime[25],
                stats.reconn_systime[26]       , stats.reconn_systime[27],
                stats.reconn_systime[28]       , stats.reconn_systime[29],
                stats.reconn_systime[30]       , stats.reconn_systime[31],

				stats.reconn_timestamp[0]      , stats.reconn_timestamp[1],
				stats.reconn_timestamp[2]      , stats.reconn_timestamp[3],
				stats.reconn_timestamp[4]      , stats.reconn_timestamp[5],
				stats.reconn_timestamp[6]      , stats.reconn_timestamp[7],
				stats.reconn_timestamp[8]      , stats.reconn_timestamp[9],
				stats.reconn_timestamp[10]     , stats.reconn_timestamp[11],
				stats.reconn_timestamp[12]     , stats.reconn_timestamp[13],
				stats.reconn_timestamp[14]     , stats.reconn_timestamp[15],
                stats.reconn_timestamp[16]     , stats.reconn_timestamp[17],
                stats.reconn_timestamp[18]     , stats.reconn_timestamp[19],
                stats.reconn_timestamp[20]     , stats.reconn_timestamp[21],
                stats.reconn_timestamp[22]     , stats.reconn_timestamp[23],
                stats.reconn_timestamp[24]     , stats.reconn_timestamp[25],
                stats.reconn_timestamp[26]     , stats.reconn_timestamp[27],
                stats.reconn_timestamp[28]     , stats.reconn_timestamp[29],
                stats.reconn_timestamp[30]     , stats.reconn_timestamp[31],

				stats.reconn_audio_send[0]     , stats.reconn_audio_send[1],
				stats.reconn_audio_send[2]     , stats.reconn_audio_send[3],
				stats.reconn_audio_send[4]     , stats.reconn_audio_send[5],
				stats.reconn_audio_send[6]     , stats.reconn_audio_send[7],
				stats.reconn_audio_send[8]     , stats.reconn_audio_send[9],
				stats.reconn_audio_send[10]    , stats.reconn_audio_send[11],
				stats.reconn_audio_send[12]    , stats.reconn_audio_send[13],
				stats.reconn_audio_send[14]    , stats.reconn_audio_send[15],
                stats.reconn_audio_send[16]    , stats.reconn_audio_send[17],
                stats.reconn_audio_send[18]    , stats.reconn_audio_send[19],
                stats.reconn_audio_send[20]    , stats.reconn_audio_send[21],
                stats.reconn_audio_send[22]    , stats.reconn_audio_send[23],
                stats.reconn_audio_send[24]    , stats.reconn_audio_send[25],
                stats.reconn_audio_send[26]    , stats.reconn_audio_send[27],
                stats.reconn_audio_send[28]    , stats.reconn_audio_send[29],
                stats.reconn_audio_send[30]    , stats.reconn_audio_send[31],

				stats.reboot_disconn[0]        , stats.reboot_disconn[1],
				stats.reboot_disconn[2]        , stats.reboot_disconn[3],
				stats.reboot_disconn[4]        , stats.reboot_disconn[5],
				stats.reboot_disconn[6]        , stats.reboot_disconn[7],
				stats.reboot_disconn[8]        , stats.reboot_disconn[9],
				stats.reboot_disconn[10]       , stats.reboot_disconn[11],
				stats.reboot_disconn[12]       , stats.reboot_disconn[13],
				stats.reboot_disconn[14]       , stats.reboot_disconn[15],
                stats.reboot_disconn[16]       , stats.reboot_disconn[17],
                stats.reboot_disconn[18]       , stats.reboot_disconn[19],
                stats.reboot_disconn[20]       , stats.reboot_disconn[21],
                stats.reboot_disconn[22]       , stats.reboot_disconn[23],
                stats.reboot_disconn[24]       , stats.reboot_disconn[25],
                stats.reboot_disconn[26]       , stats.reboot_disconn[27],
                stats.reboot_disconn[28]       , stats.reboot_disconn[29],
                stats.reboot_disconn[30]       , stats.reboot_disconn[31],

				stats.reboot_hoff_mem[0]       , stats.reboot_hoff_mem[1],
				stats.reboot_hoff_mem[2]       , stats.reboot_hoff_mem[3],
				stats.reboot_hoff_mem[4]       , stats.reboot_hoff_mem[5],
				stats.reboot_hoff_mem[6]       , stats.reboot_hoff_mem[7],
				stats.reboot_hoff_mem[8]       , stats.reboot_hoff_mem[9],
				stats.reboot_hoff_mem[10]      , stats.reboot_hoff_mem[11],
				stats.reboot_hoff_mem[12]      , stats.reboot_hoff_mem[13],
				stats.reboot_hoff_mem[14]      , stats.reboot_hoff_mem[15],
                stats.reboot_hoff_mem[16]      , stats.reboot_hoff_mem[17],
                stats.reboot_hoff_mem[18]      , stats.reboot_hoff_mem[19],
                stats.reboot_hoff_mem[20]      , stats.reboot_hoff_mem[21],
                stats.reboot_hoff_mem[22]      , stats.reboot_hoff_mem[23],
                stats.reboot_hoff_mem[24]      , stats.reboot_hoff_mem[25],
                stats.reboot_hoff_mem[26]      , stats.reboot_hoff_mem[27],
                stats.reboot_hoff_mem[28]      , stats.reboot_hoff_mem[29],
                stats.reboot_hoff_mem[30]      , stats.reboot_hoff_mem[31],

				stats.reboot_hoff_cmem[0]      , stats.reboot_hoff_cmem[1],
				stats.reboot_hoff_cmem[2]      , stats.reboot_hoff_cmem[3],
				stats.reboot_hoff_cmem[4]      , stats.reboot_hoff_cmem[5],
				stats.reboot_hoff_cmem[6]      , stats.reboot_hoff_cmem[7],
				stats.reboot_hoff_cmem[8]      , stats.reboot_hoff_cmem[9],
				stats.reboot_hoff_cmem[10]     , stats.reboot_hoff_cmem[11],
				stats.reboot_hoff_cmem[12]     , stats.reboot_hoff_cmem[13],
				stats.reboot_hoff_cmem[14]     , stats.reboot_hoff_cmem[15],
                stats.reboot_hoff_cmem[16]     , stats.reboot_hoff_cmem[17],
                stats.reboot_hoff_cmem[18]     , stats.reboot_hoff_cmem[19],
                stats.reboot_hoff_cmem[20]     , stats.reboot_hoff_cmem[21],
                stats.reboot_hoff_cmem[22]     , stats.reboot_hoff_cmem[23],
                stats.reboot_hoff_cmem[24]     , stats.reboot_hoff_cmem[25],
                stats.reboot_hoff_cmem[26]     , stats.reboot_hoff_cmem[27],
                stats.reboot_hoff_cmem[28]     , stats.reboot_hoff_cmem[29],
                stats.reboot_hoff_cmem[30]     , stats.reboot_hoff_cmem[31],

				stats.frame_drop_ssrc[0][0]    , stats.frame_drop_ssrc[0][1],
				stats.frame_drop_ssrc[0][2]    , stats.frame_drop_ssrc[0][3],
				stats.frame_drop_ssrc[0][4]    , stats.frame_drop_ssrc[0][5],
				stats.frame_drop_ssrc[0][6]    , stats.frame_drop_ssrc[0][7],
				stats.frame_drop_ssrc[0][8]    , stats.frame_drop_ssrc[0][9],
				stats.frame_drop_ssrc[0][10]   , stats.frame_drop_ssrc[0][11],
				stats.frame_drop_ssrc[0][12]   , stats.frame_drop_ssrc[0][13],
				stats.frame_drop_ssrc[0][14]   , stats.frame_drop_ssrc[0][15],
                stats.frame_drop_ssrc[0][16]   , stats.frame_drop_ssrc[0][17],
                stats.frame_drop_ssrc[0][18]   , stats.frame_drop_ssrc[0][19],
                stats.frame_drop_ssrc[0][20]   , stats.frame_drop_ssrc[0][21],
                stats.frame_drop_ssrc[0][22]   , stats.frame_drop_ssrc[0][23],
                stats.frame_drop_ssrc[0][24]   , stats.frame_drop_ssrc[0][25],
                stats.frame_drop_ssrc[0][26]   , stats.frame_drop_ssrc[0][27],
                stats.frame_drop_ssrc[0][28]   , stats.frame_drop_ssrc[0][29],
                stats.frame_drop_ssrc[0][30]   , stats.frame_drop_ssrc[0][31],
				stats.frame_drop_ssrc[1][0]    , stats.frame_drop_ssrc[1][1],
				stats.frame_drop_ssrc[1][2]    , stats.frame_drop_ssrc[1][3],
				stats.frame_drop_ssrc[1][4]    , stats.frame_drop_ssrc[1][5],
				stats.frame_drop_ssrc[1][6]    , stats.frame_drop_ssrc[1][7],
				stats.frame_drop_ssrc[1][8]    , stats.frame_drop_ssrc[1][9],
				stats.frame_drop_ssrc[1][10]   , stats.frame_drop_ssrc[1][11],
				stats.frame_drop_ssrc[1][12]   , stats.frame_drop_ssrc[1][13],
				stats.frame_drop_ssrc[1][14]   , stats.frame_drop_ssrc[1][15],
                stats.frame_drop_ssrc[1][16]   , stats.frame_drop_ssrc[1][17],
                stats.frame_drop_ssrc[1][18]   , stats.frame_drop_ssrc[1][19],
                stats.frame_drop_ssrc[1][20]   , stats.frame_drop_ssrc[1][21],
                stats.frame_drop_ssrc[1][22]   , stats.frame_drop_ssrc[1][23],
                stats.frame_drop_ssrc[1][24]   , stats.frame_drop_ssrc[1][25],
                stats.frame_drop_ssrc[1][26]   , stats.frame_drop_ssrc[1][27],
                stats.frame_drop_ssrc[1][28]   , stats.frame_drop_ssrc[1][29],
                stats.frame_drop_ssrc[1][30]   , stats.frame_drop_ssrc[1][31],

				stats.frame_drop_seqnum[0][0]    , stats.frame_drop_seqnum[0][1],
				stats.frame_drop_seqnum[0][2]    , stats.frame_drop_seqnum[0][3],
				stats.frame_drop_seqnum[0][4]    , stats.frame_drop_seqnum[0][5],
				stats.frame_drop_seqnum[0][6]    , stats.frame_drop_seqnum[0][7],
				stats.frame_drop_seqnum[0][8]    , stats.frame_drop_seqnum[0][9],
				stats.frame_drop_seqnum[0][10]   , stats.frame_drop_seqnum[0][11],
				stats.frame_drop_seqnum[0][12]   , stats.frame_drop_seqnum[0][13],
				stats.frame_drop_seqnum[0][14]   , stats.frame_drop_seqnum[0][15],
                stats.frame_drop_seqnum[0][16]   , stats.frame_drop_seqnum[0][17],
                stats.frame_drop_seqnum[0][18]   , stats.frame_drop_seqnum[0][19],
                stats.frame_drop_seqnum[0][20]   , stats.frame_drop_seqnum[0][21],
                stats.frame_drop_seqnum[0][22]   , stats.frame_drop_seqnum[0][23],
                stats.frame_drop_seqnum[0][24]   , stats.frame_drop_seqnum[0][25],
                stats.frame_drop_seqnum[0][26]   , stats.frame_drop_seqnum[0][27],
                stats.frame_drop_seqnum[0][28]   , stats.frame_drop_seqnum[0][29],
                stats.frame_drop_seqnum[0][30]   , stats.frame_drop_seqnum[0][31],
				stats.frame_drop_seqnum[1][0]    , stats.frame_drop_seqnum[1][1],
				stats.frame_drop_seqnum[1][2]    , stats.frame_drop_seqnum[1][3],
				stats.frame_drop_seqnum[1][4]    , stats.frame_drop_seqnum[1][5],
				stats.frame_drop_seqnum[1][6]    , stats.frame_drop_seqnum[1][7],
				stats.frame_drop_seqnum[1][8]    , stats.frame_drop_seqnum[1][9],
				stats.frame_drop_seqnum[1][10]   , stats.frame_drop_seqnum[1][11],
				stats.frame_drop_seqnum[1][12]   , stats.frame_drop_seqnum[1][13],
				stats.frame_drop_seqnum[1][14]   , stats.frame_drop_seqnum[1][15],
                stats.frame_drop_seqnum[1][16]   , stats.frame_drop_seqnum[1][17],
                stats.frame_drop_seqnum[1][18]   , stats.frame_drop_seqnum[1][19],
                stats.frame_drop_seqnum[1][20]   , stats.frame_drop_seqnum[1][21],
                stats.frame_drop_seqnum[1][22]   , stats.frame_drop_seqnum[1][23],
                stats.frame_drop_seqnum[1][24]   , stats.frame_drop_seqnum[1][25],
                stats.frame_drop_seqnum[1][26]   , stats.frame_drop_seqnum[1][27],
                stats.frame_drop_seqnum[1][28]   , stats.frame_drop_seqnum[1][29],
                stats.frame_drop_seqnum[1][30]   , stats.frame_drop_seqnum[1][31]);
			}
#if 0 //ksi_test
			if (stat_count % 3 == 0) {
				g_snprintf(buf_ts, 2048,
"-----------------------------------------------------------------------------\n"
"  timestamp\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n"
"    %u(%u.%03u) %u(%u.%03u) audio(%u.%03u) ho(%u.%03u) abuf(%u)\n",
stats.cbc_frame_cnt[0],  vstreams[0].ts_last_second%1000, vstreams[0].ts_last_5_mili,
stats.cbc_frame_cnt[16], vstreams[16].ts_last_second%1000, vstreams[16].ts_last_5_mili,
astreams[0].ts_last_second%1000, astreams[0].ts_last_5_mili,
aring[0].timestamp%1000, aring[0].timestampl,
vstreams[0].abuf.len,

stats.cbc_frame_cnt[1],  vstreams[1].ts_last_second%1000, vstreams[1].ts_last_5_mili,
stats.cbc_frame_cnt[17], vstreams[17].ts_last_second%1000, vstreams[17].ts_last_5_mili,
astreams[1].ts_last_second%1000, astreams[1].ts_last_5_mili,
aring[1].timestamp%1000, aring[1].timestampl,
vstreams[1].abuf.len,

stats.cbc_frame_cnt[2],  vstreams[2].ts_last_second%1000, vstreams[2].ts_last_5_mili,
stats.cbc_frame_cnt[18], vstreams[18].ts_last_second%1000, vstreams[18].ts_last_5_mili,
astreams[2].ts_last_second%1000, astreams[2].ts_last_5_mili,
aring[2].timestamp%1000, aring[2].timestampl,
vstreams[2].abuf.len,

stats.cbc_frame_cnt[3],  vstreams[3].ts_last_second%1000, vstreams[3].ts_last_5_mili,
stats.cbc_frame_cnt[19], vstreams[19].ts_last_second%1000, vstreams[19].ts_last_5_mili,
astreams[3].ts_last_second%1000, astreams[3].ts_last_5_mili,
aring[3].timestamp%1000, aring[3].timestampl,
vstreams[3].abuf.len,

stats.cbc_frame_cnt[4],  vstreams[4].ts_last_second%1000, vstreams[4].ts_last_5_mili,
stats.cbc_frame_cnt[20], vstreams[20].ts_last_second%1000, vstreams[20].ts_last_5_mili,
astreams[4].ts_last_second%1000, astreams[4].ts_last_5_mili,
aring[4].timestamp%1000, aring[4].timestampl,
vstreams[4].abuf.len,

stats.cbc_frame_cnt[5],  vstreams[5].ts_last_second%1000, vstreams[5].ts_last_5_mili,
stats.cbc_frame_cnt[21], vstreams[21].ts_last_second%1000, vstreams[21].ts_last_5_mili,
astreams[5].ts_last_second%1000, astreams[5].ts_last_5_mili,
aring[5].timestamp%1000, aring[5].timestampl,
vstreams[5].abuf.len,

stats.cbc_frame_cnt[6],  vstreams[6].ts_last_second%1000, vstreams[6].ts_last_5_mili,
stats.cbc_frame_cnt[22], vstreams[22].ts_last_second%1000, vstreams[22].ts_last_5_mili,
astreams[6].ts_last_second%1000, astreams[6].ts_last_5_mili,
aring[6].timestamp%1000, aring[6].timestampl,
vstreams[6].abuf.len,

stats.cbc_frame_cnt[7],  vstreams[7].ts_last_second%1000, vstreams[7].ts_last_5_mili,
stats.cbc_frame_cnt[23], vstreams[23].ts_last_second%1000, vstreams[23].ts_last_5_mili,
astreams[7].ts_last_second%1000, astreams[7].ts_last_5_mili,
aring[7].timestamp%1000, aring[7].timestampl,
vstreams[7].abuf.len,

stats.cbc_frame_cnt[8],  vstreams[8].ts_last_second%1000, vstreams[8].ts_last_5_mili,
stats.cbc_frame_cnt[24], vstreams[24].ts_last_second%1000, vstreams[24].ts_last_5_mili,
astreams[8].ts_last_second%1000, astreams[8].ts_last_5_mili,
aring[8].timestamp%1000, aring[8].timestampl,
vstreams[8].abuf.len,

stats.cbc_frame_cnt[9],  vstreams[9].ts_last_second%1000, vstreams[9].ts_last_5_mili,
stats.cbc_frame_cnt[25], vstreams[25].ts_last_second%1000, vstreams[25].ts_last_5_mili,
astreams[9].ts_last_second%1000, astreams[9].ts_last_5_mili,
aring[9].timestamp%1000, aring[9].timestampl,
vstreams[9].abuf.len,

stats.cbc_frame_cnt[10],  vstreams[10].ts_last_second%1000, vstreams[10].ts_last_5_mili,
stats.cbc_frame_cnt[26],  vstreams[26].ts_last_second%1000, vstreams[26].ts_last_5_mili,
astreams[10].ts_last_second%1000, astreams[10].ts_last_5_mili,
aring[10].timestamp%1000,   aring[10].timestampl,
vstreams[10].abuf.len,

stats.cbc_frame_cnt[11],  vstreams[11].ts_last_second%1000, vstreams[11].ts_last_5_mili,
stats.cbc_frame_cnt[27],  vstreams[27].ts_last_second%1000, vstreams[27].ts_last_5_mili,
astreams[11].ts_last_second%1000, astreams[11].ts_last_5_mili,
aring[11].timestamp%1000,   aring[11].timestampl,
vstreams[11].abuf.len,

stats.cbc_frame_cnt[12],  vstreams[12].ts_last_second%1000, vstreams[12].ts_last_5_mili,
stats.cbc_frame_cnt[28],  vstreams[28].ts_last_second%1000, vstreams[28].ts_last_5_mili,
astreams[12].ts_last_second%1000, astreams[12].ts_last_5_mili,
aring[12].timestamp%1000,   aring[12].timestampl,
vstreams[12].abuf.len,

stats.cbc_frame_cnt[13],  vstreams[13].ts_last_second%1000, vstreams[13].ts_last_5_mili,
stats.cbc_frame_cnt[29],  vstreams[29].ts_last_second%1000, vstreams[29].ts_last_5_mili,
astreams[13].ts_last_second%1000, astreams[13].ts_last_5_mili,
aring[13].timestamp%1000,   aring[13].timestampl,
vstreams[13].abuf.len,

stats.cbc_frame_cnt[14],  vstreams[14].ts_last_second%1000, vstreams[14].ts_last_5_mili,
stats.cbc_frame_cnt[30],  vstreams[30].ts_last_second%1000, vstreams[30].ts_last_5_mili,
astreams[14].ts_last_second%1000, astreams[14].ts_last_5_mili,
aring[14].timestamp%1000,   aring[14].timestampl,
vstreams[14].abuf.len,

stats.cbc_frame_cnt[15],  vstreams[15].ts_last_second%1000, vstreams[15].ts_last_5_mili,
stats.cbc_frame_cnt[31],  vstreams[31].ts_last_second%1000, vstreams[31].ts_last_5_mili,
astreams[15].ts_last_second%1000, astreams[15].ts_last_5_mili,
aring[15].timestamp%1000,   aring[15].timestampl,
vstreams[15].abuf.len);
			}
			if (stat_count % 3 == 0) {
				int xxx, yyy;
				int w[32][STREAM_MAX];
				int h[32][STREAM_MAX];

				for (xxx=0; xxx<MRTPSRC_MAX_CH; xxx++)
				{
					for (yyy=0; yyy<STREAM_MAX; yyy++)
					{
						get_stream_resolution(&vstreams[xxx+(32*yyy)], &w[xxx][yyy], &h[xxx][yyy]);
					}
				}
				g_snprintf(buf_resol, 2048,
"-----------------------------------------------------------------------------\n"
"  resolution\n"
"    [00] %dx%d %dx%d\n"
"    [01] %dx%d %dx%d\n"
"    [02] %dx%d %dx%d\n"
"    [03] %dx%d %dx%d\n"
"    [04] %dx%d %dx%d\n"
"    [05] %dx%d %dx%d\n"
"    [06] %dx%d %dx%d\n"
"    [07] %dx%d %dx%d\n"
"    [08] %dx%d %dx%d\n"
"    [09] %dx%d %dx%d\n"
"    [10] %dx%d %dx%d\n"
"    [11] %dx%d %dx%d\n"
"    [12] %dx%d %dx%d\n"
"    [13] %dx%d %dx%d\n"
"    [14] %dx%d %dx%d\n"
"    [15] %dx%d %dx%d\n",
w[0][STREAM_1ST], h[0][STREAM_1ST], w[0][STREAM_2ND], h[0][STREAM_2ND],
w[1][STREAM_1ST], h[1][STREAM_1ST], w[1][STREAM_2ND], h[1][STREAM_2ND],
w[2][STREAM_1ST], h[2][STREAM_1ST], w[2][STREAM_2ND], h[2][STREAM_2ND],
w[3][STREAM_1ST], h[3][STREAM_1ST], w[3][STREAM_2ND], h[3][STREAM_2ND],
w[4][STREAM_1ST], h[4][STREAM_1ST], w[4][STREAM_2ND], h[4][STREAM_2ND],
w[5][STREAM_1ST], h[5][STREAM_1ST], w[5][STREAM_2ND], h[5][STREAM_2ND],
w[6][STREAM_1ST], h[6][STREAM_1ST], w[6][STREAM_2ND], h[6][STREAM_2ND],
w[7][STREAM_1ST], h[7][STREAM_1ST], w[7][STREAM_2ND], h[7][STREAM_2ND],
w[8][STREAM_1ST], h[8][STREAM_1ST], w[8][STREAM_2ND], h[8][STREAM_2ND],
w[9][STREAM_1ST], h[9][STREAM_1ST], w[9][STREAM_2ND], h[9][STREAM_2ND],
w[10][STREAM_1ST], h[10][STREAM_1ST], w[10][STREAM_2ND], h[10][STREAM_2ND],
w[11][STREAM_1ST], h[11][STREAM_1ST], w[11][STREAM_2ND], h[11][STREAM_2ND],
w[12][STREAM_1ST], h[12][STREAM_1ST], w[12][STREAM_2ND], h[12][STREAM_2ND],
w[13][STREAM_1ST], h[13][STREAM_1ST], w[13][STREAM_2ND], h[13][STREAM_2ND],
w[14][STREAM_1ST], h[14][STREAM_1ST], w[14][STREAM_2ND], h[14][STREAM_2ND],
w[15][STREAM_1ST], h[15][STREAM_1ST], w[15][STREAM_2ND], h[15][STREAM_2ND]);
			}

			printf("%s", buf);
			if (stat_count % 3 == 0) { printf("%s", buf_reconn); }
			if (stat_count % 3 == 0) { printf("%s", buf_ts); }
			if (stat_count % 3 == 0) { printf("%s", buf_resol); }
			printf("=============================================================================\n");
#endif
#if MRTPSRC_CAM_STAT_DUMP
			if (stat_count % 60 == 0)
			{
				FILE *log_fp = NULL;
				log_fp = fopen("/tmp/webra-info/cam_stat.txt", "w");
				if (log_fp != NULL)
				{
					fprintf(log_fp, "%s", buf);
					fprintf(log_fp, "%s", buf_reconn);
					fprintf(log_fp, "%s", buf_ts);
					fprintf(log_fp, "=============================================================================\n");
					fclose(log_fp);
				}
			}
#endif

			prev_time = mrtp_now_time;
		}
	}
}

GTimeVal _prev_run_ts;

static void _receive_th_func(void *param)
{
	gint i, j;
	gint len = 0;
	gint recv_stream_cnt = 0;
	guint lc_call_cnt = 0;
	// guint as_last[16];
	MRTPSRC_STREAM_T *lst = NULL;
	MRTPSRC_CHANNEL_T *lch = NULL;

	// GTimeVal as_last_ts[16];

	_prev_run_ts.tv_sec = 0;
	_prev_run_ts.tv_usec = 0;

	MRTPSRC_DBG(MAJOR, "receive_th | _receive_th_func start");

#if MRTPSRC_SET_PRIORITY
	{
		int policy;
		struct sched_param sched;
		pthread_t thread;
		policy = SCHED_FIFO;
		thread = pthread_self();

//		sched.sched_priority = sched_get_priority_max(policy) - 1;
		sched.sched_priority = sched_get_priority_max(policy);
		//sched.sched_priority = sched_get_priority_max(policy);
		printf("[mrtpsrc]    sched.sched_priority %d \n",sched.sched_priority);
		printf("[mrtpsrc]    set ret = %d\n", pthread_setschedparam (thread, policy, &sched));
		printf("[mrtpsrc]    get ret = %d\n", pthread_getschedparam (thread, &policy, &sched));
		printf("[mrtpsrc]    set realtime policy = %d\n", policy);
	}
#endif

	// memset((void*)as_last, 0x00, sizeof(guint) * 16);
	// memset((void*)as_last_ts, 0x00, sizeof(GTimeVal) * 16);
	memset((void*)&recv_time, 0x00, sizeof(GTimeVal));

	g_get_current_time(&recv_time);

	// int befo_size = vstreams[0].buf_sz;

	while(MRTPSRC_ALWAYS)
	{
		lc_call_cnt++;

		mrtpsrc_closesync_lock();

		// if(befo_size != vstreams[0].buf_sz)
		// {
		// 	printf("************************************************************\n");
		// 	printf("%s | WARN! 0,1st buffer size changed(%d -> %d)!!\n", __FUNCTION__, befo_size, vstreams[0].buf_sz);
		// 	printf("************************************************************\n");
		// }
		// befo_size = vstreams[0].buf_sz;


		if (lc_call_cnt % 2 == 0)
		{
			/* Audio send */
			for (i = 0; i < MRTPSRC_MAX_CH; i++)
			{
				if (vstreams[i].abuf.len < 2048)
				{
					continue;
				}

				if (vstreams[i].abuf.len > (MRTPSRC_AUDIO_MAX_SZ - 2000))
				{
					printf("[\033[1;49;33m%s\033[0m] drop audio(%d) len(%u)\n", __FUNCTION__, i, vstreams[i].abuf.len);
					vstreams[i].abuf.len -= 1000;
				}

				mrtpsrc_tx_audio_lock();
				if (vstreams[i].abuf.len > 0 && vstreams[i].abuf.len <= MRTPSRC_AUDIO_MAX_SZ - 2000) {
					send_audio_to(i);
				}
				mrtpsrc_tx_audio_unlock();
			}
		}

		//g_usleep(500);
		g_get_current_time(&mrtp_now_time);
#if 0
		if (_prev_run_ts.tv_sec != 0)
		{
			static unsigned int _run_count = 0;
			int _diff_sec = mrtp_now_time.tv_sec - _prev_run_ts.tv_sec;
			int _diff_usec = mrtp_now_time.tv_usec - _prev_run_ts.tv_usec;
			int _diff_msec = _diff_sec*1000 + _diff_usec/1000;
			FILE *fp = fopen("/tmp/diff.MRTPSRC_DBG","a");

			if (_diff_msec > 30)
			{
				fprintf(fp, "loop %d %u%03u diff %d\n", _run_count, mrtp_now_time.tv_sec, mrtp_now_time.tv_usec/1000, _diff_msec);
			}
			_run_count++;
			fclose(fp);
		}
		_prev_run_ts = mrtp_now_time;
#endif

		recv_stream_cnt = 0;
		for (i = 0; i < MRTPSRC_MAX_CH; i++)
		{
			lch = mrtpsrc_get_channel(i);
			if (lch->state < STATE_STREAM_READY || lch->state > STATE_DROP_P_ALL)
			{
				continue;
			}
			for (j = 0; j < STREAM_MAX; j++)
			{
				lst = mrtpsrc_get_stream(i, j);
				if (lst->state < STATE_STREAM_READY || lst->state > STATE_DROP_P_ALL)
				{
					continue;
				}

				len = ring_load(lst);

				if(len > 0)
				{
					recv_stream_cnt++;
				}

#if MRTPSRC_USE_STATISTICS
				if (len > 0)
				{
					stats.recv_cnt++;
					stats.recv_bytes += len;
					stats.cbc_read[i + 32*j]++;
				}
				else
				{
					stats.recv_miss++;
					stats.cbc_read_miss[i + 32*j]++;
				}
#endif
				if((lst->rtp_protocol == RTP_UNICAST_UDP || lst->rtp_protocol == RTP_MULTICAST_UDP) && lc_call_cnt % 150 == 1)
				{
					len = ring_load_rtcp(lst);
				}
			}
		}

		if(recv_stream_cnt == 0)
		{
			mrtpsrc_closesync_unlock();
			usleep(1000); // 1ms
			continue;
		}


		//if (lc_call_cnt % 3 == 0)
		{
			for (i = 0; i < MRTPSRC_MAX_CH; i++)
			{
				lch = mrtpsrc_get_channel(i);
				if (lch->state < STATE_STREAM_READY || lch->state > STATE_DROP_P_ALL)
				{
					continue;
				}
				for (j = 0; j < STREAM_AUDIO; j++)
				{
					lst = mrtpsrc_get_stream(i, j);
					if (lst->state < STATE_STREAM_READY || lst->state > STATE_DROP_P_ALL)
					{
						continue;
					}

					_mrtpsrc_check_and_recover_rbuf(lst, i, j);

					if (lst->rbuf.buf_used == 0)
					{
						continue;
					}

					mrtpsrc_set_parsing_state(i, j, 1);
					len = mrtpsrc_parse_video(lst);
					mrtpsrc_set_parsing_state(i, j, 0);

				}

				lst = mrtpsrc_get_stream(i, STREAM_AUDIO);
				if (lst->state >= STATE_STREAM_READY && lst->state <= STATE_PLAYING && lst->rbuf.buf_used)
				{
					mrtpsrc_set_parsing_state(i, STREAM_AUDIO, 1);
					len = mrtpsrc_parse_audio(lst);
					mrtpsrc_set_parsing_state(i, STREAM_AUDIO, 0);
				}

				if(lch->is_metadata_on)
				{
					lst = mrtpsrc_get_stream(i, STREAM_META);

					if (lst->state >= STATE_STREAM_READY && lst->state <= STATE_PLAYING && lst->rbuf.buf_used)
					{
						mrtpsrc_set_parsing_state(i, STREAM_META, 1);
						len = mrtpsrc_parse_metadata(lst);
						mrtpsrc_set_parsing_state(i, STREAM_META, 0);
					}
				}
			}
		}

		if (mrtp_now_time.tv_sec >= prev_time2.tv_sec + 3 || mrtp_now_time.tv_sec <= prev_time2.tv_sec - 3)
		{
			printf("[mrtpsrc] %s | running\n", __FUNCTION__);
			prev_time2 = mrtp_now_time;
		}
		mrtpsrc_closesync_unlock();
	}
}

static gint _mrtpsrc_check_and_recover_rbuf(MRTPSRC_STREAM_T *lst, gint ch, gint stream)
{
	guchar *exp_rbs = (guchar*) ring_sptr[stream*MRTPSRC_MAX_CH+ch];
	guchar *exp_rbe = (guchar*) ring_eptr[stream*MRTPSRC_MAX_CH+ch];
	gint corrupted = 0;

	if (lst->rbuf.rbs != exp_rbs ||
		lst->rbuf.rbe != exp_rbe ||
		lst->rbuf.buf != exp_rbs ||
		lst->rbuf.lp < exp_rbs || lst->rbuf.lp > exp_rbe ||
		lst->rbuf.cp < exp_rbs || lst->rbuf.cp > exp_rbe)
	{
		corrupted = 1;
	}

	if (!corrupted)
	{
		return RTN_OK;
	}

	printf("Memory corruption detected CH(%d) %s - lst->rbuf.rbs(%p)rbe(%p)lp(%p)cp(%p)remain(%d)used(%d)buf(%p) ring_sptr(%p~%p)\n",
			ch, MRTPSRC_STREAM_STR[stream],
			lst->rbuf.rbs,
			lst->rbuf.rbe,
			lst->rbuf.lp,
			lst->rbuf.cp,
			lst->rbuf.buf_remain,
			lst->rbuf.buf_used,
			lst->rbuf.buf,
			exp_rbs,
			exp_rbe);

	lst->rbuf.rbs = exp_rbs;
	lst->rbuf.rbe = exp_rbe;
	lst->rbuf.lp = lst->rbuf.rbs;
	lst->rbuf.cp = lst->rbuf.rbs;
	lst->rbuf.buf = lst->rbuf.rbs;
	lst->rbuf.buf_used = 0;
	lst->rbuf.buf_remain = lst->rbuf.rbe - lst->rbuf.rbs;

	printf("Memory corruption corrected CH(%d) %s - lst->rbuf.rbs(%p)rbe(%p)lp(%p)cp(%p)remain(%d)used(%d)buf(%p) ring_sptr(%p~%p)\n",
			ch, MRTPSRC_STREAM_STR[stream],
			lst->rbuf.rbs,
			lst->rbuf.rbe,
			lst->rbuf.lp,
			lst->rbuf.cp,
			lst->rbuf.buf_remain,
			lst->rbuf.buf_used,
			lst->rbuf.buf,
			exp_rbs,
			exp_rbe);

	return RTN_FAIL;
}

static guint64 mrtp_rec_ho_cnt = 0;
static GTimeVal ho_nowtime;
static GTimeVal ho_prevtime;
static void _rec_ho_func(void *param)
{
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *src = NULL;
#endif
	MRTPSRC_RECORD_HO_Q _cut_entry = NULL;
	MRTPSRC_RECORD_HO_Q _temp_entry = NULL;
	ho_prevtime.tv_sec = 0;
	ho_prevtime.tv_usec = 0;

#if 0
	{
		int policy;
		struct sched_param sched;
		pthread_t thread;
		policy = SCHED_FIFO;
		thread = pthread_self();

//		sched.sched_priority = sched_get_priority_max(policy) - 1;
		sched.sched_priority = sched_get_priority_max(policy) - 1;
		//sched.sched_priority = sched_get_priority_max(policy);
		printf("[mrtpsrc]    sched.sched_priority %d \n",sched.sched_priority);
		printf("[mrtpsrc]    set ret = %d\n", pthread_setschedparam (thread, policy, &sched));
		printf("[mrtpsrc]    get ret = %d\n", pthread_getschedparam (thread, &policy, &sched));
		printf("[mrtpsrc]    set realtime policy = %d\n", policy);
	}
#endif

	while(1)
	{
		usleep(5*1000);

		g_get_current_time(&ho_nowtime);
		if (ho_nowtime.tv_sec != ho_prevtime.tv_sec)
		{
			printf("[mrtpsrc] %s:%d | mrtpsrc record handoff count [%llu]\n", __FUNCTION__, __LINE__, (unsigned long long)mrtp_rec_ho_cnt);
			ho_prevtime = ho_nowtime;
		}
		if (ho_head->next == NULL)
		{
			continue;
		}

		rec_ho_q_lock();
		_cut_entry = ho_head->next;
		ho_head->frame = NULL;
		ho_head->next = NULL;
		rec_ho_q_unlock();

#ifndef NMZ_STANDLONE_MODE
		src = gst_mrtp_src_get_object();
		if (src != NULL && src->handoff_callback != NULL)
		{
			while (_cut_entry != NULL)
			{
				if(!GOBJ_IS_BUDDY_BUFFER (_cut_entry->frame))
				{
					printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
					gst_buffer_debug(_cut_entry->frame);
				}
				else
				{
					ICODEC_HEADER *icodec = NULL;
					icodec =(ICODEC_HEADER*) gobj_buddy_buffer_buf_get_addr(_cut_entry->frame);
					if(!(icodec->chan < 64|| icodec->chan >= 0))
					{
						printf("[%s:%d] !(icodec->chan < 64|| icodec->chan >= 0) - chan(%d) \n", __FUNCTION__, __LINE__, icodec->chan);
					}

					//printf("\e[95m [%s][%d] icodec->chan (%d) \e[0m\n", __func__, __LINE__, icodec->chan);
					src->handoff_callback(_cut_entry->frame, src->handoff_user_data);
					mrtp_rec_ho_cnt++;
					mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, _cut_entry->frame);
				}
				_temp_entry = _cut_entry->next;
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, _cut_entry);
				_cut_entry = _temp_entry;
			}
		}
		else
		{
			while (_cut_entry != NULL)
			{
				_temp_entry = _cut_entry->next;
				mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, _cut_entry->frame);
				mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, _cut_entry);
				_cut_entry = _temp_entry;
			}
		}
		src = NULL;
		_cut_entry = NULL;
		_temp_entry = NULL;
#endif
	}
}

static gint min_val(gint a, gint b)
{
	return ((a > b) ? b:a);
}

static gint ring_load(MRTPSRC_STREAM_T* lst)
{
	gint len = 0;
	gint temp = 0;
	MRTPSRC_RTSP_IF_T dummy;
	guchar* align_cp;
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *filter = gst_mrtp_src_get_object();
#endif
#endif
	MRTPSRC_CHANNEL_T* lch = mrtpsrc_get_channel(lst->ch_num);

	switch(lst->rtp_protocol)
	{
		case RTP_UNICAST_UDP:
			while(1)
			{
				temp = recv(lst->udp_sock[0], lst->rbuf.lp + sizeof(MRTPSRC_RTSP_IF_T) + len, min_val(lst->rbuf.buf_remain, lst->rbuf.rbe - lst->rbuf.lp - sizeof(MRTPSRC_RTSP_IF_T) - len), MSG_DONTWAIT);
				if(temp < 0)
				{
					break;
				}

				dummy.magic = 0x24;
				dummy.channel = (lst->stream_num == STREAM_AUDIO) ? 2 : 0;

				align_cp = (guchar*) &temp;
				dummy.len = *(align_cp + 1) | *align_cp << 8;
				memcpy(lst->rbuf.lp + len, &dummy, sizeof(MRTPSRC_RTSP_IF_T));
				len += sizeof(MRTPSRC_RTSP_IF_T);

				len += temp;
			}

			break;
		case RTP_MULTICAST_UDP:
			while(1)
			{
				temp = recv(lst->udp_sock[0], lst->rbuf.lp + sizeof(MRTPSRC_RTSP_IF_T) + len, min_val(lst->rbuf.buf_remain, lst->rbuf.rbe - lst->rbuf.lp - sizeof(MRTPSRC_RTSP_IF_T) - len), MSG_DONTWAIT);
				if(temp < 0)
				{
					break;
				}

				dummy.magic = 0x24;
				dummy.channel = (lst->stream_num == STREAM_AUDIO) ? 2 : 0;

				align_cp = (guchar*) &temp;
				dummy.len = *(align_cp + 1) | *align_cp << 8;
				memcpy(lst->rbuf.lp + len, &dummy, sizeof(MRTPSRC_RTSP_IF_T));
				len += sizeof(MRTPSRC_RTSP_IF_T);

				len += temp;
			}
			break;
		case RTP_OVER_RTSP_TCP:
			if (lst->rbuf.buf_remain <= 8)
			{
				len = (-100);
				break;
			}
			{
				int rmbufsz = lst->rbuf.rbe-lst->rbuf.lp;
				if (rmbufsz > 0)
				{

					// Xiongmai audio exception
					if (lst->stream_num == STREAM_AUDIO && lst->resolution == 100)
					{
						if (lch->audio_qlen <= 0)
						{
							len = (-1);
						}
						else
						{
							len = lch->audio_qlen;
							lch->audio_qlen = 0;
							memcpy(lst->rbuf.lp, lch->audio_qbuf, len);
						}
					}
					else	// normal
					{
						len = recv(lst->rtsp_sock, lst->rbuf.lp, rmbufsz, MSG_DONTWAIT);
					}
				}
				else
				{
					len = (-100);
					break;
				}
			}
			break;
		default:
			break;
	}

	if (len == (-100))
	{
		lst->ring_full_cnt++;
		if (lst->ring_full_cnt > 30)
		{
			MRTPSRC_DBG(WARN, "%s | Ring buffer full (%u,%s) size(%u)", __FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->buf_sz);

			channels[lst->ch_num].state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
			stats.reconn_ring_full[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
			if (filter == NULL) { return (0); }
			if (filter->cmd_callback == NULL) { return (0); }
			filter->cmd_callback(2, lst->ch_num, 0, 102/*ring full*/, filter->cmd_user_data);
#endif
#endif
			lst->ring_full_cnt = 0;
		}
		return (0);
	}
	else if (len < 0)
	{
		if (errno == EBADF)
		{
			MRTPSRC_DBG(ERROR, "%s | Socket file corrupted(%d, %s)", __FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
			channels[lst->ch_num].state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
			stats.reconn_sock_corrupt[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
			if (filter == NULL) { return len; }
			if (filter->cmd_callback == NULL) { return len; }
			filter->cmd_callback(2, lst->ch_num, 0, 101/*socket corrupted*/, filter->cmd_user_data);
#endif
#endif
			return len;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// MRTPSRC_DBG(WARN, "%s | No data received from socket(%d, %s) errno(%d)", __FUNCTION__,
			// 		lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], errno);
			return 0;
		}
		else if (errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE)
		{
			MRTPSRC_DBG(ERROR, "%s | Socket connection closed by server(%d, %s) errno(%d)", __FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], errno);
			channels[lst->ch_num].state = STATE_REBOOT_REQ;
#if MRTPSRC_USE_STATISTICS
			stats.reconn_server_close[lst->ch_num]++;
#endif
			return len;
		}
		else
		{
			MRTPSRC_DBG(WARN, "%s | Socket error(%d, %s) errno(%d)", __FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], errno);
		}

		//connectivity check && exception operate code
		connectivity_time_checker(lst, lch);

		return len;
	}

#if 1
	if (len == 0)
	{
		MRTPSRC_DBG(ERROR, "%s | Socket connection closed by server(%d, %s) len(%d)", __FUNCTION__,
				lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], len);
		channels[lst->ch_num].state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
		stats.reconn_server_close[lst->ch_num]++;
#endif
		return len;
	}
#endif

//	if (len == min_val(lst->rbuf.buf_remain, lst->rbuf.rbe - lst->rbuf.lp))
//	{
//		MRTPSRC_DBG(WARN, "%s | Ring buffer full (%u,%s) size(%u)", __FUNCTION__,
//				lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->buf_sz);
//
//		channels[lst->ch_num].state = STATE_RECONN_REQ;
//#if MRTPSRC_USE_STATISTICS
//		stats.reconn_ring_full[lst->ch_num]++;
//#endif
//#if MRTPSRC_EVENTLOG_PUT
//		if (filter == NULL) { return (0); }
//		if (filter->cmd_callback == NULL) { return (0); }
//		filter->cmd_callback(2, lst->ch_num, 0, 102/*ring full*/, filter->cmd_user_data);
//#endif
//		return (0);
//	}

	
	lst->ring_full_cnt = 0;
	lst->rbuf.lp += len;
	lst->rbuf.buf_remain -= len;
	lst->rbuf.buf_used += len;

	reinitialize_channel_stream_connectivity(lst->ch_num, lst->stream_num);

	return len;
}

static gint ring_load_rtcp(MRTPSRC_STREAM_T* lst)
{
	gint len = 0;
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *filter = gst_mrtp_src_get_object();
#endif
#endif

	switch(lst->rtp_protocol)
	{
		case RTP_UNICAST_UDP:
			len = recv(lst->udp_sock[1], lst->rbuf.lp + sizeof(MRTPSRC_RTSP_IF_T), min_val(lst->rbuf.buf_remain, lst->rbuf.rbe - lst->rbuf.lp - sizeof(MRTPSRC_RTSP_IF_T)), MSG_DONTWAIT);
			break;
		case RTP_MULTICAST_UDP:
			len = recv(lst->udp_sock[1], lst->rbuf.lp + sizeof(MRTPSRC_RTSP_IF_T), min_val(lst->rbuf.buf_remain, lst->rbuf.rbe - lst->rbuf.lp - sizeof(MRTPSRC_RTSP_IF_T)), MSG_DONTWAIT);
			break;
		default:
			break;
	}

	if (len < 0)
	{
		if (errno == 9)
		{
			MRTPSRC_DBG(ERROR, "%s | Socket file corrupted(%d, %s)", __FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
			channels[lst->ch_num].state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
			stats.reconn_sock_corrupt[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
			if (filter == NULL) { return len; }
			if (filter->cmd_callback == NULL) { return len; }
			filter->cmd_callback(2, lst->ch_num, 0, 101/*socket corrupted*/, filter->cmd_user_data);
#endif
#endif
			return len;
		}
		else {
			MRTPSRC_DBG(WARN, "%s | RTCP socket error(%d, %s) errno(%d)", __FUNCTION__,
					lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], errno);
		}
		//connectivity check && exception operate code
		connectivity_time_checker(lst, NULL);

		return len;
	}

	//if(lst->rtp_protocol == RTP_UNICAST_UDP || lst->rtp_protocol == RTP_MULTICAST_UDP)
	{
		MRTPSRC_RTSP_IF_T dummy;
		dummy.magic = 0x24;
		dummy.channel = (lst->stream_num == STREAM_AUDIO) ? 3 : 1;

		guchar* align_cp = (guchar*) &len;
		dummy.len = *(align_cp + 1) | *align_cp << 8;
		memcpy(lst->rbuf.lp, &dummy, sizeof(MRTPSRC_RTSP_IF_T));
		len += sizeof(MRTPSRC_RTSP_IF_T);
	}

	if (len == min_val(lst->rbuf.buf_remain, lst->rbuf.rbe - lst->rbuf.lp))
	{
		MRTPSRC_DBG(WARN, "%s | Ring buffer full (%u,%s) size(%u)", __FUNCTION__,
				lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], lst->buf_sz);

		channels[lst->ch_num].state = STATE_RECONN_REQ;
#if MRTPSRC_USE_STATISTICS
		stats.reconn_ring_full[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
		if (filter == NULL) { return (0); }
		if (filter->cmd_callback == NULL) { return (0); }
		filter->cmd_callback(2, lst->ch_num, 0, 102/*ring full*/, filter->cmd_user_data);
#endif
#endif
		return (0);
	}
	if (len > 100)
	{
		reinitialize_channel_stream_connectivity(lst->ch_num, lst->stream_num);
	}
	lst->rbuf.lp += len;
	lst->rbuf.buf_remain -= len;
	lst->rbuf.buf_used += len;

	return len;
}


extern void mrtpsrc_ho_control_frame(MRTPSRC_STREAM_T* lst, int control)
{
#if MRTPSRC_CTRL_FRAME
	gchar cf_str[2][8] = { "START", "END" };

	guchar *wv_ptr = NULL;
	MRTPSRC_FRAME_T new_frame = NULL;
	ICODEC_HEADER *icodec_h = NULL;
#ifndef NMZ_STANDLONE_MODE
	GobjMrtpSrc *src = gst_mrtp_src_get_object();
#endif
	gint i = 0;


#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | session %s(%d, %s)",
			__FUNCTION__,
			cf_str[control], lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
#endif

	if(lst->stream_num == STREAM_AUDIO || lst->stream_num == STREAM_META)
	{
		return;
	}

	new_frame = (MRTPSRC_FRAME_T) mrtpsrc_alloc_heap(sizeof(struct _FRAME_INFO_T_), __FILE__, __FUNCTION__, __LINE__);
	if (new_frame == NULL)
	{
		MRTPSRC_DBG(WARN, "%s | memory fail(malloc). Reboot request(%d, %s)\n",
				__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

		i = lst->ch_num;
		channels[i].state = STATE_REBOOT_REQ;
#if MRTPSRC_USE_STATISTICS
		stats.reboot_hoff_mem[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
		if (src != NULL && src->cmd_callback != NULL)
		{
			src->cmd_callback(2, lst->ch_num, 0, 202/*cont_mem*/,src->cmd_user_data);
		}
#endif
#endif
		return;
	}
	//g_assert(new_frame != NULL);
	memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
	new_frame->len = 0;
	new_frame->frame = mrtpsrc_alloc_cmem(
			sizeof(ICODEC_HEADER)+new_frame->len, &new_frame->icodec_h.chan,
			__FILE__, __FUNCTION__, __LINE__);
	if (new_frame->frame == NULL)
	{
		MRTPSRC_DBG(WARN, "%s | memory fail(cmem). Reboot request(%d, %s)\n",
				__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);

		i = lst->ch_num;
		channels[i].state = STATE_REBOOT_REQ;
#if MRTPSRC_USE_STATISTICS
		stats.reboot_hoff_cmem[lst->ch_num]++;
#endif
#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
		if (src != NULL && src->cmd_callback != NULL)
		{
			src->cmd_callback(2, lst->ch_num, 0, 203/*cont_cmem*/,src->cmd_user_data);
		}
#endif
#endif

		memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
		new_frame = NULL;
		return;
	}

	memset(gobj_buddy_buffer_buf_get_addr(new_frame->frame), 0x00, sizeof(ICODEC_HEADER)+new_frame->len);

	wv_ptr = gobj_buddy_buffer_buf_get_addr(new_frame->frame);
	icodec_h = (ICODEC_HEADER*) wv_ptr;

	if (lst->stream_num == STREAM_AUDIO)
	{
		MRTPSRC_DBG(WARN, "%s | Audio session, no control frame(%d, %s)\n",
				__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
		mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
		memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
		mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
		new_frame = NULL;
		return;
	}
	else
	{
		new_frame->icodec_h.chan = (guchar)((lst->stream_num*16) + lst->ch_num);
		new_frame->icodec_h.codec = NF_CODEC_TYPE_H264MP;
	}
	new_frame->icodec_h.flags = 1;
	new_frame->icodec_h.version = 1;
	new_frame->icodec_h.frame_size = 0;
	if (control == 0)
	{
		new_frame->icodec_h.frame_type= NF_FRAME_TYPE_START;
	}
	else if (control == 1)
	{
		new_frame->icodec_h.frame_type= NF_FRAME_TYPE_END;
	}
	new_frame->icodec_h.resolution = (guchar)lst->resolution;
	//new_frame->icodec_h.frame_rate = fps_conversion_to_icodec(lst->fps);
	new_frame->icodec_h.frame_rate = NF_FPS_CR32;
	new_frame->icodec_h.gst_buffer = new_frame->frame;

	memcpy(icodec_h, &new_frame->icodec_h, sizeof(ICODEC_HEADER));

#ifndef NMZ_STANDLONE_MODE
	if (src != NULL && src->handoff_callback != NULL)
	{
		if(!GOBJ_IS_BUDDY_BUFFER (new_frame->frame))
		{
			printf("[minto] is not mini object <%s:%d>\n", __FUNCTION__, __LINE__);
			gst_buffer_debug(new_frame->frame);
		}
		else
		{
			src->handoff_callback(new_frame->frame, src->handoff_user_data);
#if MRTPSRC_USE_STATISTICS
			stats.handoff_cnt++;
#endif
		}
	}
#endif

	mrtpsrc_gst_buffer_free_cmem(__FILE__, __FUNCTION__, __LINE__, new_frame->frame);
	memset(new_frame, 0x00, sizeof(struct _FRAME_INFO_T_));
	mrtpsrc_free_heap(__FILE__, __FUNCTION__, __LINE__, new_frame);
	new_frame = NULL;

#if MRTPSRC_FULL_DBG
	MRTPSRC_DBG(MAJOR, "%s | session %s(%d, %s) end",
			__FUNCTION__,
			cf_str[control], lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
#endif
#endif
}

static guchar* _nf_mrtpsrc_min_malloc = (guchar*)0xffffffff;
static guchar* _nf_mrtpsrc_max_malloc = (guchar*)0x00000000;

extern void* mrtpsrc_alloc_heap(size_t msz, const char* filename, const char* funcname, const int line)
{
	void* _new_addr = NULL;

	if (msz <= 0)
	{
#if MRTPSRC_FULL_DBG
		printf("%s:%d [%s] msz(%d)\n", filename, line, funcname, msz);
#endif
		return NULL;
	}
	_new_addr = calloc(msz, 1);
	if (_new_addr == NULL)
	{
		printf("%s:%d [%s] malloc failed\n", filename, line, funcname);
		return NULL;
	}

	if ((guchar*)_new_addr < _nf_mrtpsrc_min_malloc)
	{
		//printf("%s:%d [%s] Heap min updated(%p)\n", filename, line, funcname, _new_addr);
		_nf_mrtpsrc_min_malloc = _new_addr;
	}
	if ((guchar*)_new_addr > _nf_mrtpsrc_max_malloc)
	{
		//printf("%s:%d [%s] Heap max updated(%p)\n", filename, line, funcname, _new_addr);
		_nf_mrtpsrc_max_malloc = _new_addr;
	}

	return _new_addr;
}
extern void mrtpsrc_free_heap(const char* filename, const char* funcname, const int line, void* mem)
{
	if ((guchar*)mem < _nf_mrtpsrc_min_malloc || (guchar*)mem > _nf_mrtpsrc_max_malloc)
	{
		printf("%s:%d [%s] free malloc error(%p)\n", filename, line, funcname, mem);
		g_assert(0);
		return;
	}
	free(mem);
}

static guchar* _nf_mrtpsrc_min_cmem = (guchar*)0xffffffff;
static guchar* _nf_mrtpsrc_max_cmem = (guchar*)0x00000000;
static size_t _nf_mrtpsrc_max_sz = 0; 
extern GobjBuddyBuffer* mrtpsrc_alloc_cmem(size_t msz, void* pch, const char* filename, const char* funcname, const int line)
{
	// GobjBuddyBuffer* _new_addr = NULL;
	GobjBuddyBuffer *_new_addr = NULL;
	char ch = *((char*)pch);
	if (msz <= 0)
	{
		printf("%s:%d [%s] msz(%d) ch(%d) pch(%p)\n", filename, line, funcname, msz, ch, pch);
		return NULL;
	}
	if (ch < 0 || ch>=64)
	{
		printf("%s:%d [%s] msz(%d) ch(%d) pch(%p)\n", filename, line, funcname, msz, ch, pch);
		return NULL;
	}
	// _new_addr = (GobjBuddyBuffer*) gst_nf_buddy_buffer_new_and_alloc(msz, ch);
	_new_addr = (GobjBuddyBuffer*) gobj_buddy_buffer_new_malloc(msz);
	if (_new_addr == NULL)
	{
		printf("%s:%d [%s] CMEM allocation failed\n", filename, line, funcname);
		return NULL;
	}

	if (gobj_buddy_buffer_buf_get_addr(_new_addr) < _nf_mrtpsrc_min_cmem)
	{
		//printf("%s:%d [%s] CMEM min updated(%p)\n", filename, line, funcname, GST_BUFFER_DATA(_new_addr));
		_nf_mrtpsrc_min_cmem = gobj_buddy_buffer_buf_get_addr(_new_addr);
	}
	if (gobj_buddy_buffer_buf_get_addr(_new_addr)+gobj_buddy_buffer_buf_get_size(_new_addr) > _nf_mrtpsrc_max_cmem)
	{
		//printf("%s:%d [%s] CMEM max updated(%p,%d:%p)\n", filename, line, funcname, GST_BUFFER_DATA(_new_addr), GST_BUFFER_SIZE(_new_addr),GST_BUFFER_DATA(_new_addr)+GST_BUFFER_SIZE(_new_addr));
		_nf_mrtpsrc_max_cmem = gobj_buddy_buffer_buf_get_addr(_new_addr)+gobj_buddy_buffer_buf_get_size(_new_addr);
	}
	if (gobj_buddy_buffer_buf_get_size(_new_addr) > _nf_mrtpsrc_max_sz)
	{
		//printf("%s:%d [%s] CMEM max size(%p:%d)\n", filename, line, funcname, GST_BUFFER_DATA(_new_addr), GST_BUFFER_SIZE(_new_addr));
		_nf_mrtpsrc_max_sz = gobj_buddy_buffer_buf_get_size(_new_addr);
	}

	return _new_addr;
}

extern void mrtpsrc_gst_buffer_free_cmem(const char* filename, const char* funcname, const int line, GobjBuddyBuffer* buf)
{
	if(GOBJ_IS_BUDDY_BUFFER(buf)) 
	{
		mrtpsrc_free_cmem(filename, funcname, line, buf);
	}
	else
	{
		printf("[%s][%d] buf is not mini object buf %p\n", __FUNCTION__, __LINE__, buf);
	}
}

extern void mrtpsrc_free_cmem(const char* filename, const char* funcname, const int line, GobjBuddyBuffer* buf)
{
	guchar *mem = gobj_buddy_buffer_buf_get_addr(buf);
	if (mem < _nf_mrtpsrc_min_cmem || mem > _nf_mrtpsrc_max_cmem)
	{
		printf("%s:%d [%s] free cmem error(%p)\n", filename, line, funcname, mem);
		g_assert(0);
		return;
	}
	g_object_unref(buf);
}

extern void mrtpsrc_print_cmem_range()
{
	printf("min_cmem(%d) / max_cmem(%d)\n",_nf_mrtpsrc_min_cmem ,_nf_mrtpsrc_max_cmem);
}

static void get_rtsp_digest_auth_str
(
	char* user, char* pass, char* realm, char* nonce, char* uri, char* method,
	char* result
)
{
	char ha1[36];
	char ha2[36];
	char resp[128];
	char in_buf[1024];


	memset(in_buf, 0x00, 1024);
	memset(ha1, 0x00, 36);
	g_snprintf(in_buf, 1024, "%s:%s:%s", user, realm, pass);
	mrtpsrc_rtsp_client_md5(in_buf, ha1);
	//MRTPSRC_DBG(MINOR, "%s | hash_1(%s)", __FUNCTION__, ha1);

	memset(in_buf, 0x00, 1024);
	memset(ha2, 0x00, 36);
	g_snprintf(in_buf, 512, "%s:%s", method, uri);
	mrtpsrc_rtsp_client_md5(in_buf, ha2);
	//MRTPSRC_DBG(MINOR, "%s | hash_2(%s)", __FUNCTION__, ha2);

	memset(in_buf, 0x00, 1024);
	memset(resp, 0x00, 128);
	g_snprintf(in_buf, 512, "%s:%s:%s", ha1, nonce, ha2);
	mrtpsrc_rtsp_client_md5(in_buf, resp);
	//MRTPSRC_DBG(MINOR, "%s | response(%s)", __FUNCTION__, resp);

	g_snprintf(result, 2048, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n", user, realm, nonce, uri, resp);
}

static void send_rtsp_validation(MRTPSRC_STREAM_T *lst)
{
	gchar buf[MRTPSRC_NBUF_SZ];
	gchar rtsp_cmd[16];

	g_strlcpy(rtsp_cmd, "GET_PARAMETER", 16);
	if (strcmp(lst->rtsp_validation, "SET_PARAMETER") == 0)
	{
		g_strlcpy(rtsp_cmd, lst->rtsp_validation, 16);
	}

	if (lst->model == MODEL_AMB_A2 || lst->model == MODEL_AMB_D1)
	{
		g_snprintf((gchar *)buf, MRTPSRC_NBUF_SZ,
				"%s %s RTSP/1.0\r\n"
				"CSeq: %u\r\n"
				"Session: %s\r\n"
				"User-Agent: " MRTPSRC_USER_AGENT_AMB_STR "\r\n"
				"\r\n",
			rtsp_cmd,
			lst->rtsp_addr,
			lst->req_seq++,
			lst->session);
	}
	else
	{
		g_snprintf((gchar *)buf, MRTPSRC_NBUF_SZ,
				"%s %s RTSP/1.0\r\n"
				"CSeq: %u\r\n"
				"Session: %s\r\n"
				"User-Agent: " MRTPSRC_USER_AGENT_STR "\r\n"
				"\r\n",
			rtsp_cmd,
			lst->rtsp_addr,
			lst->req_seq++,
			lst->session);
	}

	if (send(lst->rtsp_sock, buf, (size_t)strlen((gchar*)buf), 0) < 0)
	{
		MRTPSRC_DBG(ERROR, "%s | CH(%u, %s) %s request send fail(%d)",
			__FUNCTION__, lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num], rtsp_cmd, errno);
	}

#if PRINT_RTSP_MSG
	MRTPSRC_DBG(IMPACT, "%s | Re-validate RTSP_MSG\n%s", __FUNCTION__, buf);
#endif
}

static void _mrtpsrc_send_rtcp(MRTPSRC_STREAM_T *lst)
{
	guchar buf[1024];
	guchar *p = NULL;
	guint16 *p2 = NULL;
	guint *p4 = NULL;
	gint len = 0;
	MRTPSRC_CHANNEL_T *lch = mrtpsrc_get_channel(lst->ch_num);

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
			len = 1;
			break;
		case RTP_OVER_RTSP_TCP:
			len = send(lst->rtsp_sock, buf, p - &buf[0], 0);
			break;
		default:
			break;
	}

	if (len <= 0) 
	{
		MRTPSRC_DBG(WARN, "CH(%d) %s validation failed", lst->ch_num, MRTPSRC_STREAM_STR[lst->stream_num]);
		lch->state = STATE_RECONN_REQ;
	}
}


#if 0
static guint fps_conversion_to_local_use(guint fps)
{
	switch(fps)
	{
		// TODO. Add fps
		case 30:
			return FPS_300;
		case 15:
			return FPS_150;
		case 10:
			return FPS_100;
		case 6:
			return FPS_60;
		case 5:
			return FPS_50;
		case 3:
			return FPS_30;
		case 2:
			return FPS_20;
		case 1:
			return FPS_10;
		default:
			return FPS_UNDEFINED;
	}
}

static guint fps_conversion_to_external_use(guint fps)
{
	switch(fps)
	{
		case FPS_300:
			return 30;
		case FPS_150:
			return 15;
		case FPS_100:
			return 10;
		case FPS_60:
			return 6;
		case FPS_50:
			return 5;
		case FPS_30:
			return 3;
		case FPS_20:
			return 2;
		case FPS_10:
			return 1;
		default:
			return 0;
	}
}

static guint fps_conversion_to_icodec(guint fps)
{
	switch(fps)
	{
		case FPS_300:
			return NF_FPS_CR32;		// 30
		case FPS_150:
			return NF_FPS_CR16;		// 15
		case FPS_100:
		case FPS_60:
			return NF_FPS_CR08;		// 7.5
		case FPS_50:
		case FPS_30:
			return NF_FPS_CR04;		// 3.25
		case FPS_20:
			return NF_FPS_CR02;		// 1.625
		case FPS_10:
			return NF_FPS_CR01;		// 0.8125
		case 0:
			return 0;
		default:
			break;
	}

	return 0;
}
#endif

static void bind_rtp_unicast(MRTPSRC_STREAM_T *lst)
{
	guint port1, port2;
	gchar rtn, try_cnt;

	port1 = get_port_number();
	port2 = port1 + 1;

	MRTPSRC_DBG(MAJOR, "%s | bind unicast socket port1 : %u port2 : %u", __FUNCTION__, port1, port2);

	try_cnt = 0;

	while(MRTPSRC_ALWAYS)
	{
		if(try_cnt >= 5)
		{
			port1 = get_port_number();
			port2 = port1 + 1;
			try_cnt = 0;
		}

		rtn = bind_udp_socket(port1, &lst->udp_sock[0]);
		if(!rtn)
		{
			try_cnt++;
			g_usleep(1000);
			continue;
		}

		rtn = bind_udp_socket(port2, &lst->udp_sock[1]);
		if(!rtn)
		{
			close(lst->udp_sock[0]);
			try_cnt++;
			g_usleep(1000);
			continue;
		}

		break;
	}
	guint rcvbuf = 1024 * 1024 * 30;
	int state;
	state = setsockopt(lst->udp_sock[0], SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(rcvbuf));

	lst->client_port[0] = port1;
	lst->client_port[1] = port2;
}

static void bind_rtp_multicast(MRTPSRC_STREAM_T* lst)
{
#if 0
-- not working --
	gchar rtn, try_cnt, i;

	MRTPSRC_DBG(MAJOR, "%s | bind multicast socket ip : %s port1 : %u port2 : %u", __FUNCTION__,
			lst->dest_addr, lst->server_port[0], lst->server_port[1]);

	try_cnt = 0;

	while(MRTPSRC_ALWAYS)
	{
		if(try_cnt >= 10)
		{
			// TODO error chk
		}

		rtn = bind_udp_socket(lst->server_port[0], &lst->udp_sock[0]);
		if(!rtn)
		{
			try_cnt++;
			g_usleep(1000);
			continue;
		}

		rtn = bind_udp_socket(lst->server_port[1], &lst->udp_sock[1]);
		if(!rtn)
		{
			close(lst->udp_sock[0]);
			try_cnt++;
			g_usleep(1000);
			continue;
		}

		break;
	}
	guint rcvbuf = 1024 * 1024 * 30;
	int state;
	state = setsockopt(lst->udp_sock[0], SOL_SOCKET, SO_RCVBUF, (char *)&rcvbuf, sizeof(rcvbuf));

	for(i = 0; i < 2; i++)
	{
		guchar b_reuse = 1;
		struct ip_mreq mreq;

		state = setsockopt(lst->udp_sock[i], SOL_SOCKET, SO_REUSEADDR, (char *)&b_reuse, sizeof(guchar));

		mreq.imr_multiaddr.s_addr = inet_addr(lst->dest_addr);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		state = setsockopt(lst->udp_sock[i], IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
	}
#endif
}

static gchar bind_udp_socket(guint port, gint *sock)
{
	MRTPSRC_DBG(MAJOR, "%s | bind udp socket port : %u", __FUNCTION__, port);
	struct sockaddr_in sin;

	*sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(*sock < 0)
		return 0;

	memset(&sin, 0x00, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);

	int retval = bind(*sock, (struct sockaddr*)&sin, sizeof(struct sockaddr));
	if(retval == -1)
	{
		close(*sock);
		return 0;
	}

	return 1;
}

static void get_stream_resolution(MRTPSRC_STREAM_T *lst, int *w, int *h)
{
	switch(lst->resolution)
	{
		case NF_RES_1920x1080:  { *w = 1920; *h = 1080; break; }
		case NF_RES_1280x720:   { *w = 1280; *h = 720;  break; }
		case NF_RES_640x360:    { *w = 640;  *h = 360;  break; }
		case NF_RES_640x480:    { *w = 640;  *h = 480;  break; }
                                                        
		case NF_RES_NTSC_CIF:   { *w = 352;  *h = 240;  break; }
		case NF_RES_PAL_CIF:    { *w = 352;  *h = 288;  break; }
		case NF_RES_640x352:    { *w = 640;  *h = 352;  break; }
		case NF_RES_640x400:    { *w = 640;  *h = 400;  break; }
		case NF_RES_NTSC_4CIFP: { *w = 704;  *h = 480;  break; }
		case NF_RES_PAL_4CIFP:  { *w = 704;  *h = 576;  break; }
		case NF_RES_720x480:    { *w = 720;  *h = 480;  break; }
		case NF_RES_720x576:    { *w = 720;  *h = 576;  break; }
		case NF_RES_800x450:    { *w = 800;  *h = 450;  break; }
		case NF_RES_800x600:    { *w = 800;  *h = 600;  break; }
		case NF_RES_1024x768:   { *w = 1024; *h = 768;  break; }
		case NF_RES_1280x1024:  { *w = 1280; *h = 1024; break; }
		case NF_RES_1440x900:   { *w = 1440; *h = 900;  break; }
		case NF_RES_1600x1200:  { *w = 1600; *h = 1200; break; }
		case NF_RES_320x180:    { *w = 320;  *h = 180;  break; }
		case NF_RES_2304x1296:  { *w = 2304; *h = 1296; break; }
		case NF_RES_2048x1536:  { *w = 2048; *h = 1536; break; }
		case NF_RES_2560x1440:  { *w = 2560; *h = 1440; break; }
		case NF_RES_2688x1520:  { *w = 2688; *h = 1520; break; }
		case NF_RES_2560x1600:  { *w = 2560; *h = 1600; break; }
		case NF_RES_2560x1920:  { *w = 2560; *h = 1920; break; }
		case NF_RES_2592x1920:  { *w = 2592; *h = 1920; break; }
		case NF_RES_2592x1944:  { *w = 2592; *h = 1944; break; }
		case NF_RES_2992x1680:  { *w = 2992; *h = 1680; break; }
		case NF_RES_2880x1800:  { *w = 2880; *h = 1800; break; }
		case NF_RES_3200x1800:  { *w = 3200; *h = 1800; break; }
		case NF_RES_2880x2160:  { *w = 2880; *h = 2160; break; }
		case NF_RES_3072x2048:  { *w = 3072; *h = 2048; break; }
		case NF_RES_3200x2400:  { *w = 3200; *h = 2400; break; }
		case NF_RES_3840x2160:  { *w = 3840; *h = 2160; break; }
		case NF_RES_2592x1520:  { *w = 2592; *h = 1520; break; }
		case NF_RES_1920x1440:  { *w = 1920; *h = 1440; break; }
		case NF_RES_1920x1536:  { *w = 1920; *h = 1536; break; }
		case NF_RES_1344x1520:  { *w = 1344; *h = 1520; break; }
		case NF_RES_1296x1944:  { *w = 1296; *h = 1944; break; }
		case NF_RES_1280x1440:  { *w = 1280; *h = 1440; break; }
		case NF_RES_1024x1536:  { *w = 1024; *h = 1536; break; }
		case NF_RES_1280x960:  { *w = 1280; *h = 960; break; }
		case NF_RES_3000x3000:  { *w = 3000; *h = 3000; break; }
		case NF_RES_2048x2048:  { *w = 2048; *h = 2048; break; }
		case NF_RES_1280x1280:  { *w = 1280; *h = 1280; break; }
		case NF_RES_640x640:  { *w = 640; *h = 640; break; }
		case NF_RES_320x320:  { *w = 320; *h = 320; break; }
		case NF_RES_360x640:  { *w = 360; *h = 640; break; }
		case NF_RES_480x640:  { *w = 480; *h = 640; break; }
		case NF_RES_480x704:  { *w = 480; *h = 704; break; }
		case NF_RES_576x704:  { *w = 576; *h = 704; break; }
		case NF_RES_720x1280:  { *w = 720; *h = 1280; break; }
		case NF_RES_768x1024:  { *w = 768; *h = 1024; break; }
		case NF_RES_1024x1280:  { *w = 1024; *h = 1280; break; }
		case NF_RES_1080x1920:  { *w = 1080; *h = 1920; break; }
		case NF_RES_1536x2048:  { *w = 1536; *h = 2048; break; }
		case NF_RES_1296x2304:  { *w = 1296; *h = 2304; break; }
		case NF_RES_1520x2592:  { *w = 1520; *h = 2592; break; }
		case NF_RES_1944x2592:  { *w = 1944; *h = 2592; break; }
		case NF_RES_2160x3840:  { *w = 2160; *h = 3840; break; }
		default:                { *w = 0;    *h = 0;    break; }
	}
}

static void reinitialize_channel_stream_connectivity(int ch, int stream_no)
{
	if(stream_no == -1)
	{
		memset(&g_conn_table[ch], 0x00, sizeof(struct channel_connectivity));
		return;
	}
	
	memset(&g_conn_table[ch].err_startTime[stream_no], 0x00, sizeof(struct timespec));
	memset(&g_conn_table[ch].dbg_checkTime[stream_no], 0x00, sizeof(struct timespec));
	g_conn_table[ch].err_elapsedTime[stream_no] = 0;
}

static void connectivity_time_checker(MRTPSRC_STREAM_T *lst, MRTPSRC_CHANNEL_T *lch)
{
	int ch = lst->ch_num;
	int stream_no = lst->stream_num;
	int model_code = -100;
	
	if(lch != NULL)
		model_code = lch->model_code;

	if(lst->stream_num == STREAM_META)
		return;

	int threasholdTime = 0;
	struct timespec currentTime;
	clock_gettime(CLOCK_MONOTONIC, &currentTime);

	if(g_conn_table[ch].err_startTime[stream_no].tv_sec == 0)
	{
		memcpy(&g_conn_table[ch].err_startTime[stream_no], &currentTime, sizeof(struct timespec));	
		g_conn_table[ch].err_elapsedTime[stream_no] = 0;
	}
	else
	{
		g_conn_table[ch].err_elapsedTime[stream_no] = 
			(currentTime.tv_sec - g_conn_table[ch].err_startTime[stream_no].tv_sec)
			+ ((currentTime.tv_nsec - g_conn_table[ch].err_startTime[stream_no].tv_nsec)/1000000000);
	}

	if(stream_no == STREAM_1ST)
		threasholdTime = 180; // 3 minutes
	else
		threasholdTime = MRTPSRC_VLOSS_CONNECTIVITY_TIME ;

	if(g_conn_table[ch].err_elapsedTime[stream_no] > threasholdTime)
	{
		MRTPSRC_DBG(ERROR, "%s | [CH(%d)][%s] loss consistently [recv < 0 state goes over (%d sec)]",
				__FUNCTION__, ch, MRTPSRC_STREAM_STR[stream_no], (int)g_conn_table[ch].err_elapsedTime[stream_no]);

		reinitialize_channel_stream_connectivity(ch, -1);

		if(stream_no == STREAM_1ST)
			channels[ch].state = STATE_REBOOT_REQ;
		else
			channels[ch].state = STATE_RECONN_REQ;

#if MRTPSRC_USE_STATISTICS
		stats.reboot_disconn[ch]++;
#endif

#if MRTPSRC_EVENTLOG_PUT
#ifndef NMZ_STANDLONE_MODE
		if (filter == NULL) { return len; }
		if (filter->cmd_callback == NULL) { return len; }
		filter->cmd_callback(2, ch, 0, 201/*long lasting packet loss*/, filter->cmd_user_data);
#endif
#endif
	}

	if(model_code == MODEL_ONTHEFLY_CHEAT && g_conn_table[ch].err_elapsedTime[STREAM_1ST] > MRTPSRC_VLOSS_CONNECTIVITY_TIME/3)
	{
		printf("\e[31m [%s] | Connectivity check [CH(%d), Unreached time(%d) sec] in Onthefly Check\e[0m\n",
				__FUNCTION__, ch, (int)g_conn_table[ch].err_elapsedTime[STREAM_1ST]);
	
		reinitialize_channel_stream_connectivity(ch, -1);
		//state exception code 
		channels[ch].state = STATE_RECONN_REQ;
	}

	if(g_conn_table[ch].err_elapsedTime[stream_no] > MRTPSRC_VLOSS_CONNECTIVITY_TIME/3)
	{
		if(g_conn_table[ch].dbg_checkTime[stream_no].tv_sec + 5 < currentTime.tv_sec)
		{
			MRTPSRC_DBG(WARN, "%s | Connectivity check [CH(%d)(%s), recv len < 0 goes over (%d)sec]",
					__FUNCTION__, ch, MRTPSRC_STREAM_STR[stream_no], (int)g_conn_table[ch].err_elapsedTime[stream_no]);

			g_conn_table[ch].dbg_checkTime[stream_no] = currentTime;
		}
	}
}

#endif //__GST_MRTP_CLIENT_C_
