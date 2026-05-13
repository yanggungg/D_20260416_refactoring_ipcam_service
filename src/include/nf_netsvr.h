#ifndef __NF_NET_SERVER_H
#define __NF_NET_SERVER_H
 
#include <netinet/ip.h> 
#include <glib.h>

#include "nf_common.h"
#include "nf_codec_header.h"

#include "gsocket.h"
#include "queue.h" 

#define AUTO_STREAM		0
#define MAIN_STREAM		1
#define SECOND_STREAM	2
#define THIRD_STREAM	3
#define AUTO_SECOND_STREAM	5

#define SECOND_CHANGE	5
#define MAIN_CHANGE		5

#define FPS_CAL_BUFF_BIT 0x0000000000000007
#define CHANGE_CAL_BUFF_BIT 0x000000000000000f

#define FPS_CAL_BUFF_LEN (FPS_CAL_BUFF_BIT + 1)

#define STREAM_CHANGE_COUNT	1


#define SVR_PORT			6400
#define	MAX_CLIENT			1000

#define FRAME_ALIGN			128

#define	SOCK_SND_BUFF		(256*1024)
#define	HALF_SND_BUFF		(SOCK_SND_BUFF/2)
//#define	MARGIN_SND_BUFF		(64*1024)
#define	MARGIN_SND_BUFF		(256*1024)

#define SOCK_RCV_BUFF		(64*1024) 
#define MAX_RCV_BUFF		(256*1024)

#define NF_BSWAP_64(a) (a)
#define NF_BSWAP_32(a) (a)
#define NF_BSWAP_16(a) (a)

#define NET_SETUP_TABLE_NUM 16

typedef enum _CLI_E
{
	CLI_NONE			= 0x00,
	CLI_WAITCS			= 0x01,
	CLI_WAITDS			= 0x02,

	CLI_UNUSED			= 0x00,
	CLI_USED			= 0x01	
} CLI_E;

/* HUNTER AUTH TABLE
typedef enum _AUTH_E
{
	AUTH_SEARCH_ARCHIVE	= 0x0001,
	AUTH_ALARM_EVT_REC	= 0x0002,
	AUTH_CAM_DISP_AUD	= 0x0004,
	AUTH_USER			= 0x0008,
	AUTH_NETWORK		= 0x0010,
	AUTH_SYSTEM			= 0x0020,
	AUTH_REMOTE_LOGON	= 0x0040,
	AUTH_SHUTDOWN		= 0x0080,
	AUTH_STARTUP		= 0x0100	
} AUTH_E;
*/
typedef enum _AUTH_E
{
	AUTH_SETUP			= 0x0001,
	AUTH_SEARCH			= 0x0002,
	AUTH_ARCHIVE		= 0x0004,
	AUTH_PTZ			= 0x0008,
	AUTH_ALARM_OFF		= 0x0010,
	AUTH_PANIC_REC		= 0x0020,
	AUTH_REMOTE			= 0x0040,
} AUTH_E;

typedef enum _NETSVR_STAT_E
{
	NETSVR_STAT_STOP	= 1,
	NETSVR_STAT_RUN		= 2,
	NETSVR_STAT_READY	= 3 	
} NETSVR_STAT_E;

typedef enum _NETSVR_LIVE_E
{
	NETSVR_LIVE_STOP	= 0,
	NETSVR_LIVE_START	= 1	
} NETSVR_LIVE_E;

typedef enum _NETSVR_RET_E
{
	NETSVR_RET_OK = 0,	
										
	NETSVR_RET_ERR_NO_IMPL 	= -999,		// VPE_FAIL
	NETSVR_RET_ERR_PARAM 	= -998,		// VPE_FAIL
	NETSVR_RET_ERR_INTERNAL	= -997,		// VPE_INTERNAL	
	
	NETSVR_RET_ERR_CMD_INQUE = -996,	// VPE_FAIL
	NETSVR_RET_ERR_BUSY 	= -995,		// VPE_FAIL	
	
	NETSVR_RET_ERR_PROTOCOL	= -899,		// VPE_PROTOCOL, fatal_error 
	NETSVR_RET_ERR_SOCKET	= -898,		// VPE_PROTOCOL, fatal_error 	
	NETSVR_RET_ERR			= -1,		// VPE_FAULT, fatal_error
		
} NETSVR_RET_E;

typedef enum _NETSVR_SEND_AVAIL_E
{
	RTP_640x352_F1	  = 1,
	RTP_1920x1080_F1 ,
	RTP_640x352_F15  ,
	RTP_640x352_F30  ,
	RTP_1920x1080_F15,
	RTP_1920x1080_F30,
	RTP_AVAIL_MAX
} NETSVR_SEND_AVAIL_E;



/******************************************************************************/

typedef struct _rtp_netstream_t  rtp_netstream_data;
struct _rtp_netstream_t {
	gchar sizeParam[NET_SETUP_TABLE_NUM];
	gchar fpsParam[NET_SETUP_TABLE_NUM];
	gchar qualParam[NET_SETUP_TABLE_NUM];
};


typedef TAILQ_HEAD(_VIDEO_HEAD, _VIDEO_ENTRY) VIDEO_HEAD;

typedef struct _VIDEO_ENTRY			// video frame entry, TAILQ에서 사용;
{
	TAILQ_ENTRY(_VIDEO_ENTRY) entries; 
	
	ICODEC_HEADER			frame_header;	
	int						ref_count;	
	int 					data_size;
	void					*frame_data;
	
} VIDEO_ENTRY;

typedef struct _CHANNEL_STAT 
{
	struct timeval			saveTimeStamp, curTimeStamp;		
	unsigned int 			channel_count[NUM_TOTAL_CHANNEL];
	unsigned int			total_count;
	long long				total_bytes;

	unsigned int 			error_count[NUM_TOTAL_CHANNEL];
	unsigned int			total_error_count;
} CHANNEL_STAT;

// live manager용 정보
typedef struct _LIVE_INFO 
{	
	int						mode;	// run:1, stop:0, error:음수 // onAir
	
	int						live_fd, liveaudio_fd;		
	int						test_fd;
	
	pthread_t				read_tid, send_tid, test_tid;
	
	// [video buffer]
	GAsyncQueue				*video_cb_queue;
	
	pthread_mutex_t			vd_mutex;

	pthread_mutex_t			write_mutex;
	// 아래의 두개의 값으로 너무 많은 video frame이 buffer에 쌓이는 것을 방지한다.
	unsigned int			video_entry_count;				// TAILQ entry count
	unsigned int			video_entry_size;				// TAILQ에 들어있는 size
	VIDEO_HEAD 				video_head[2];					// TAILQ buffer
			
	// send가 꺼낸 frame header
	ICODEC_HEADER 			frame_header[NUM_TOTAL_CHANNEL];
	ICODEC_HEADER 			iframe_header[NUM_TOTAL_CHANNEL];

	unsigned int			iframe_drop_cnt[NUM_TOTAL_CHANNEL];
	unsigned int			pframe_drop_cnt[NUM_TOTAL_CHANNEL];

	unsigned int			max_tx_speed;	
	unsigned int			audio_tx;

	// 통계정보	
	unsigned int			client_count_runtime;			// max traffic ctrl
	unsigned int			client_tot_ch_runtime;
	
	unsigned int			client_count, overflow_count;
	unsigned int			read_count, send_count;
	long long				read_bytes, send_bytes;
	
	CHANNEL_STAT			read_stat, send_stat;
	rtp_netstream_data		net_setup_table;
	rtp_netstream_data		net_setup_table_save;

	int (*cb_hi_aud_pb_fxn_t)(void *h_stream_buf, char *stream_buf, int stream_size, int codec_type, int hi_aud_stm_cnt);
} LIVE_INFO;

extern LIVE_INFO			gLive_info;
extern LIVE_INFO			gRtp_live_info;

#define CLIENT_MAGIC		0x12343398

typedef enum _CLIENT_MODE_E
{
	CLIENT_MODE_INIT			='I',
	CLIENT_MODE_CLOSE			='C',
	CLIENT_MODE_ERROR			='E',
	CLIENT_MODE_LIVE			='L',
	CLIENT_MODE_LIVE_STOP		='l',
	CLIENT_MODE_PLAYBACK		='P',
	CLIENT_MODE_ARCHIVING		='A',
	CLIENT_MODE_SETUP			='S'		
} CLIENT_MODE_E;


typedef struct _FRAME_CONTROL_INFO
{
	unsigned int			sec1_byets_send_size[16];  // 아래 참조
	unsigned int			total_1sec_send_bytes;
	unsigned int			total_16sec_send_bytes;

	unsigned int			sec1_byets_get_size[16];   // get 한 이전 사이즈값을 1초 단위로 저장하고 잇음.
	unsigned int			total_1sec_get_bytes;      // 최근 1초동안 get한 사이즈   
	unsigned int			total_16sec_get_bytes;     // 8초 동안 get한 총 값을 들고 잇음. 

	unsigned int			sec1_iframe_count[16];   // 채널 별로 skip 하지 않고 보내게 되는 i frame 의 개수 
	unsigned int			total_1sec_iframe_count;
	unsigned int			total_16sec_iframe_count;
	
	unsigned int			sec1_pframe_count[16];   // 채널 별로 skip 하지 않고 보내게 되는 p frame 의 개수 
	unsigned int			total_1sec_pframe_count;
	unsigned int			total_16sec_pframe_count;

	unsigned int			sec1_iframe_size[16];    // 채널 별로 skip 하지 않고 보내게 되는 i frame 의 size 
	unsigned int			total_1sec_iframe_size;
	unsigned int			total_16sec_iframe_size;

	unsigned int			sec1_pframe_size[16];    // 채널 별로 skip 하지 않고 보내게 되는 p frame 의 size
	unsigned int			total_1sec_pframe_size;
	unsigned int			total_16sec_pframe_size;

	unsigned int			sec1_change_count[16];    // 채널 별로 skip 하지 않고 보내게 되는 p frame 의 size
	unsigned int			total_1sec_change_count;
	unsigned int			total_16sec_change_count;

	unsigned int			run_time_sec;
	unsigned int			send_time_sec;
}FRAME_CONTROL_INFO;


typedef struct _CHANNEL_INFO
{	
	int						ch_id;
		
	unsigned char			framerate;
	unsigned char			resolution;
	unsigned char			reserved[2];  // 0:post_fps_check; 1:codec_type
		
	unsigned int			traffic;		// dvr set에서 전송하는 fps 따라서 결정되는 traffic
		
	unsigned int			fps_level;	
	unsigned int			fps_traffic;	// 조절한 FPS에 따라서 전송 되는 traffic
	int						fps_updown;	
	
	int						req_iframe;
	
	unsigned int			frame_drop_cnt;
	unsigned int			wait_iframe;
	unsigned int			is_iframe_drop;
	unsigned int			fps_change_keep_count;

	FRAME_CONTROL_INFO		frame_info;
} CHANNEL_INFO;


typedef struct _CLIENT_INFO
{
	TAILQ_ENTRY(_CLIENT_INFO) entries; 
	
	// client socket fd
	int 					cs;	// control
	int 					ds;	// data

	struct sockaddr_in 		peer_addr;	
	
	char					userid[32+4];
	int						playid, archid, uniqueid;
	pthread_t				cs_tid, ds_tid;

	pthread_mutex_t			mutex;
	pthread_mutex_t			cs_mutex;	// ctrl socket을 배타적으로 사용하기 위한;
	pthread_mutex_t			ds_mutex;	// data socket을 배타적으로 사용하기 위한;
	
	unsigned short 			auth;					//	[I]nit [C]lose [S]leep
	unsigned char			mode, previous_mode;	//	[L]ive [P]layback [A]rchiving  
	int						setup_mode;
	
	unsigned int			magic_key;	// magic_key check
	unsigned int 			last_req_time, req_count;	// keepalive

	unsigned int			channel_mask, channel_imask;	// for live streaming		
	int						channel_audio_ch;		
	unsigned int			channel_count, fps_accel;
	unsigned int			total_traffic, total_fps_traffic;	
	unsigned int 			max_tx_speed;
	CHANNEL_INFO			ch_info[NUM_TOTAL_CHANNEL];	
	CHANNEL_STAT			ch_stat;		
	ICODEC_HEADER			last_frame_header[NUM_TOTAL_CHANNEL];
	ICODEC_HEADER			last_iframe_header[NUM_TOTAL_CHANNEL];
			
	unsigned int			sndbuff_size, sndbuff_used;		// socket buffer
	unsigned int			shadow_sndbuff_used;			
	unsigned int			shadow_count;
	
	char					live_buff[SOCK_SND_BUFF];
	unsigned int			live_buff_len;

	// choissi  CMS keepalive timeout delay
	unsigned int			play_frame_count;
				
} CLIENT_INFO;

typedef TAILQ_HEAD(_CLIENT_HEAD, _CLIENT_INFO) CLIENT_HEAD;

// 비동기 처리 때문에 필요하다.
typedef TAILQ_HEAD(_JOB_HEAD, _JOB_INFO) JOB_HEAD;

typedef struct _JOB_INFO
{
	TAILQ_ENTRY(_JOB_INFO)	entries;
		
	struct _CLIENT_INFO		*pclient;
			
	// for reqest msg	
	unsigned int 			req_msg_id;
	struct timeval 			req_timestamp;	
	unsigned int			ack_msg_id;
	struct timeval 			ack_timestamp;

	void					*msg;
	unsigned int			msg_len;
	
	unsigned int			params[4];
	
} JOB_INFO;

#define DPS_COMM_CNT	8

typedef struct _SERVER_INFO
{
	int						conn_fd;		// main socket.
	int						dsp_fd[DPS_COMM_CNT];		// dsp <-> powerpc
			
	int 					state;			// network server status.
	int						mode;			// current accept mode.
	unsigned int			seed,key;
	
	// for web server	
	unsigned int			websvr_conn_count, websvr_err_count;
	pthread_t				websvr_tid;
	
	// for ddns
	unsigned int			ddns_req_count, ddns_err_count;
	pthread_t				ddns_tid;
	
	unsigned char 			alarm_state[NUM_TOTAL_CHANNEL];
	
	// mbox job tailq
	pthread_mutex_t			job_mutex;
	JOB_HEAD				job_head;
	unsigned int			job_count;
			
	// client_info 
	unsigned int			client_setup;	
	pthread_mutex_t			client_mutex;
	CLIENT_HEAD				client_head;
	unsigned int			client_seq;
	CLIENT_INFO				*ptr_client_arr[DPS_COMM_CNT];
	unsigned int			client_count;	
	unsigned int			tryconn_count, disconn_count, error_count;	
		
	unsigned int			net_status;			
	unsigned long int		rx_byte, rx_packet, rx_error;
	unsigned long int		tx_byte, tx_packet, tx_error;
				
} SERVER_INFO;

extern SERVER_INFO	gServer_info;

// written by SKSHIN
int nf_get_external_ip(char *external_ip);
int nf_get_public_ip_by_ddns(char *external_ip);

#endif 
