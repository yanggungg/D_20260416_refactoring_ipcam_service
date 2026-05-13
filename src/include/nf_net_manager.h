//
// nf_net_manager_conf.h
//

#ifndef NF_NET_MANAGER_CONF_H
#define NF_NET_MANAGER_CONF_H

#define NM_TEST

#ifdef NM_TEST
#define RTP_MAX_USER         4

#define NM_SVR_SVR_PORT		(554)
#define NM_SVR_PORT_START	(50000)
#define NM_SVR_PORT_END		(60000)
#define NM_SVR_FRAME_HEAP_SIZE	(RTP_MAX_USER +1)
#define NM_VIDEO_DEVICE		"/NFDVR/dump_test/dump.mpg4"
#define NM_VIDEO_OUTPUT1		"mpeg4"

//#define NM_VIDEO_WIDTH		(640)
//#define NM_VIDEO_HEIGHT		(480)
#define NM_VIDEO_WIDTH		(704)
#define NM_VIDEO_HEIGHT		(480)
#define NM_VIDEO_FPS		(30)

#define NM_VIDEO_LIVE_OUTPUT	"live"
#define NM_VIDEO_PB1_OUTPUT		"playback0"
#define NM_VIDEO_PB2_OUTPUT		"playback1"
#define NM_VIDEO_PB3_OUTPUT		"playback2"
#define NM_VIDEO_PB4_OUTPUT		"playback3"

#define NM_LIVE_PATH		      "/"NM_VIDEO_LIVE_OUTPUT
#define NM_PLAYBACK_PATH_1		"/"NM_VIDEO_PB1_OUTPUT
#define NM_PLAYBACK_PATH_2		"/"NM_VIDEO_PB2_OUTPUT
#define NM_PLAYBACK_PATH_3		"/"NM_VIDEO_PB3_OUTPUT
#define NM_PLAYBACK_PATH_4		"/"NM_VIDEO_PB4_OUTPUT

#define NM_LIVE_TRACK		      NM_VIDEO_LIVE_OUTPUT
#define NM_PLAYBACK_TRACK_1		NM_VIDEO_PB1_OUTPUT
#define NM_PLAYBACK_TRACK_2		NM_VIDEO_PB2_OUTPUT
#define NM_PLAYBACK_TRACK_3		NM_VIDEO_PB3_OUTPUT
#define NM_PLAYBACK_TRACK_4		NM_VIDEO_PB4_OUTPUT

#define NM_HTTP_PATH		NM_LIVE_PATH
#define NM_LOG_PATH		"/dev/null"
#define NM_CONF_PATH		"./sp.config"

#else

#define NM_SVR_SVR_PORT
#define NM_SVR_PORT_START
#define NM_SVR_PORT_END
#define NM_SVR_FRAME_HEAP_SIZE
#define NM_VIDEO_DEVICE
#define NM_VIDEO_OUTPUT
#define NM_VIDEO_WIDTH
#define NM_VIDEO_HEIGHT
#define NM_VIDEO_FPS
#define NM_LIVE_PATH
#define NM_LIVE_TRACK
#define NM_HTTP_PATH
#define NM_LOG_PATH
#define NM_CONF_PATH

#endif
#define NM_HTTP_INPUT		"mpeg4"
#define NM_HTTP_MODE_SINGLE	(0)
#define NM_HTTP_MODE_STREAM	(1)

#include <sys/socket.h>		// for "struct sockaddr"

#define DEF_PATH_LEN		(256)

struct nm_config {
	//Manager-config
	int	tcp_port;
	int	rtp_port_start;
	int	rtp_port_end;
	int	sz_frame_heap;
	//Video-input
	char	video_device[ DEF_PATH_LEN ];
	char	video_live_output[ DEF_PATH_LEN ];
	char	video_pb_output[RTP_MAX_USER][ DEF_PATH_LEN ];

	int	video_width;
	int	video_height;
	int	video_fps;		// "0" is "AUTO"
	//RTSP-live
	char	live_path[ DEF_PATH_LEN ];	// It will be "Product name"
	char	live_track[ DEF_PATH_LEN ];

 	char	playback_path[RTP_MAX_USER][ DEF_PATH_LEN ];	// It will be "Product name"
	char	playback_track[RTP_MAX_USER][ DEF_PATH_LEN ];

	//HTTP-output
	char	http_path[ DEF_PATH_LEN ];
	char	http_input[ DEF_PATH_LEN ];
	int	http_mode;
};

// NM configure stuff.. from nf_net_manager_conf.c
int init_nm_config ( struct nm_config *conf );
int write_nm_config( struct nm_config *conf );

// NM server stuff..
int config_port( struct nm_config *conf );
int config_rtprange( struct nm_config *conf );
int config_frameheap( struct nm_config *conf );

// NM log stuff.. from access_log.c
int access_log_init( void );
void write_access_log( char *path, struct sockaddr *addr, int code, char *req,
		int length, char *referer, char *user_agent );

int control_listen( void );

#endif
