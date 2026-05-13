/*
 * nf_nvs_cgi_ipc.h
 * yoonhs@itxsecurity.com
 */
//#include "../../junsun/js_comdef.h"

#ifndef CGI_IPC_H
#define CGI_IPC_H

#define SOCK_NAME	"/tmp/cgi.sock"

enum V_SYSTEM	{ NTSC = 0, PAL };

/* genernal param */
enum true_false	{ GNL_FALSE = 0, GNL_TRUE };
enum on_off		{ GNL_OFF = 0, GNL_ON };
enum yes_no		{ GNL_NO = 0, GNL_YES };
enum open_close	{ GNL_OPEN = 0, GNL_CLOSE };
enum FPS_CTL	{ FPS_DOWN = 0, FPS_UP, FPS_INIT};
enum SESS_CTL	{ SESS_NORMAL = 0, SESS_BUSY, SESS_CRITICAL };

/* vidoe param */
enum video_ae		{ AE_OFF = 0, AE_ON_INDOOR, AE_ON_OUTDOOR };
enum video_ss		{ SS_OFF = 0, SS_ON, SS_AUTO };
enum video_iris		{ IRIS_OFF = 0, IRIS_BASIC, IRIS_ADVANCE };
enum video_m_agc	{ AGC_24DB = 0, AGC_36DB};
enum video_ff		{ FF_60HZ = 0, FF_50HZ };
enum video_awb		{ AWB_OFF = 0, AWB_ON, AWB_W_ON };

enum video_mwb 		{ 
	USER = 0, 
	INCANDESCENT,
	D4000,
	D5000,
	SUNNY,
	CLOUDY,
	FLASH, // 6
};

enum video_dnn		{ DNN_OFF = 0, DNN_ON, DNN_AUTO };
enum video_det_time	{ DET_TIME_0 = 0, DET_TIME_5, DET_TIME_10, DET_TIME_15, DET_TIME_30, DET_TIME_60, DET_ALARM_IN };
enum video_blc		{ BLC_OFF = 0, BLC_WINDOW_0 };

typedef struct img_property {
	unsigned int		bright;		// 0~30, mid:15
	unsigned int		contrast;	// 0~30, mid:15
	unsigned int		saturation;	// 0~30, mid:15
	unsigned int		hue;		// 0~30, mid:15

	unsigned int		sharpness;	// 0~6, 0:auto
	unsigned int		noise;		// 0~6, 0:auto
} img_property_t;

struct video {
	/* AE dependent stuff */
	enum video_ae		v_ae;	// Auto Exposure
	enum video_ss		v_ss;	// Slow Shutter
	enum video_iris		v_iris;	// DC-Iris
	enum video_m_agc	v_m_agc;// MAX AGC
	enum video_blc		v_blc;	// Backlight 

	unsigned int		v_me_agc;		// AGC dB for Manual Exposure, 0~36dB
	unsigned int		v_me_shutter;	// e-shutter speed for Maual Exposure, 4~2000 (1/4 ~ 1/2000)

	/* AWB dependent stuff */
	enum video_awb		v_awb;	// AWB mode
	enum video_mwb		v_mwb;	// Manual WB

	/* Independent stuff */
	enum video_dnn		v_dnn;	// Day & Night
	enum video_det_time	v_det;  // Day & Night Detect Time


	enum video_ff		v_ff;	// Flicker Free
	img_property_t		v_img;
};

/* codec param */
enum video_in {
	VIN_1080P = 0,	//1920*1080
#ifdef BAEK_FEATURE_1440_1080
	VIN_1080P_4_3,	//1440*1080
#endif
	VIN_SXGA,		//1280*1024 
	VIN_XGA,		//1024*768	(2048*1536)binning mode
	VIN_720P,		//1280*720	(2560*1440)binning mode 
	VIN_576DP,		//720*576
	VIN_480DP,		//720*480 
	VIN_576P,		//702*576	(2112*1728)binning mode
	VIN_480P,		//702*480	(2112*1440)binning mode 
};

enum video_out {
	VOUT_D1_NTSC = 0,
	VOUT_D1_PAL,
};

enum codec_type	{ 
	CODEC_NONE = 0,
	CODEC_H264,
	CODEC_MJPEG
};

enum codec_profile { 
	CODEC_SIMPLE = 0,
	CODEC_ADVANCE 
};

enum codec_bit_ctl { 
	CODEC_CBR = 1, 
	CODEC_VBR = 3,
};

enum codec_size	{ 
	CODEC_1920_1080_N =0,	/*NEW SDK 4.1.8 */
	CODEC_1920_1080, 
	CODEC_1440_1080,		/*new SDK 4.1*/
	CODEC_1280_1024,		/*new SDK 4.1*/
	CODEC_1280_960,			/*new SDK 4.1*/
	CODEC_1024_768,			/*new SDK 4.1*/
	CODEC_1280_720_N,
	CODEC_1280_720,	//7
	CODEC_720_480, 	//8
	CODEC_720_576, 
	CODEC_704_480,	//10
	CODEC_704_576, 
	CODEC_640_480, 
	CODEC_352_288, 
	CODEC_352_240, 
	CODEC_320_240,	//15
	CODEC_NR,
};

enum codec_fps {
	FPS_30	= 0,
	FPS_15,
	FPS_10,
	FPS_7_5,
	FPS_6,
	FPS_5,
	FPS_4_3,
	FPS_3_8,
	FPS_3_3,
	FPS_3,
	FPS_2_7,
	FPS_2_5,	//11
	FPS_MAX
};

#define CODEC_5_CHECK		(5*4)
#define CODEC_5_NT_DIV		(4)
#define CODEC_5_PL_DIV		(5)
#define CODEC_2_5_CHECK		(11*9)
#define CODEC_2_5_NT_DIV	(9)
#define CODEC_2_5_PL_DIV	(11)

#define CODEC_FPS_MAX		(12)
#define CODEC_P_25			(FPS_30 + CODEC_FPS_MAX)
#define CODEC_P_12_5		(FPS_15 + CODEC_FPS_MAX)
#define CODEC_P_8_3			(FPS_10 + CODEC_FPS_MAX)
#define CODEC_P_6_3			(FPS_7_5 + CODEC_FPS_MAX)
#define CODEC_P_5			(FPS_6 + CODEC_FPS_MAX)
#define CODEC_P_4_2			(FPS_5 + CODEC_FPS_MAX)
#define CODEC_P_3_6			(FPS_4_3 + CODEC_FPS_MAX)
#define CODEC_P_3_1			(FPS_3_8 + CODEC_FPS_MAX)
#define	CODEC_P_2_8			(FPS_3_3 + CODEC_FPS_MAX)
#define CODEC_P_2_5			(FPS_3 + CODEC_FPS_MAX)
#define CODEC_P_2_3			(FPS_2_7 + CODEC_FPS_MAX)
#define CODEC_P_2_1			(FPS_2_5 + CODEC_FPS_MAX)

enum image_mirror 
{ 
	CODEC_MR_NONE = 0,
	CODEC_MR_H,
	CODEC_MR_V,
	CODEC_MR_HV
};

#define CODEC_MIN_BITRA			(256) // 256 Kbps
#define CODEC_MAX_BITRA			(10*1024) // 10 Mbps
//#define CODEC_MAX_BITRA		(12*1024) // 12 Mbps

struct amb_codec {
	enum codec_type	c_codec;	/* H.264 or MJPEG */
	enum codec_profile	c_profile;	/* simple or advanced */
	enum codec_bit_ctl	c_bitctl;	/* CBR or VBR */
	enum codec_size	c_size;		
	int  					c_bitrate;
	int					c_fps;
};


struct codec {
//	enum	video_in	vin;
	enum	video_out	vout;
	struct 	amb_codec	first;
	struct 	amb_codec	second;
	struct	amb_codec	third;
	enum	image_mirror	mr_type;	// image mirror type
};

/* audio param */
enum audio_enable { AUDIO_OFF = 0, AUDIO_ON };

enum audio_direction { 
	AUDIO_BI = 0, 
	AUDIO_TO_CLI, 
	AUDIO_FROM_CLI
};

enum audio_type {
	AUDIO_PCM = 0,
	AUDIO_A_LAW,
	AUDIO_MU_LAW,
#if 0//NOT YET!!
	AUDIO_ADPCM
#endif
};

#define AUDIO_MIN_VOL	(0)
#define AUDIO_MAX_VOL	(100)
#define AUDIO_MUTE		AUDIO_MIN_VOL
#define AUDIO_DEF_VOL	AUDIO_MUTE
//#define AUDIO_DEF_VOL	(50)

struct audio {
	enum audio_enable		a_enable;
	enum audio_direction	a_dir;
	enum audio_type			a_in_codec;
	int 					a_in_volume;
	enum audio_type			a_out_codec;
	int						a_out_volume;
};

/* network_v4 param */
enum dhcp_onoff { DHCP_OFF = 0,	DHCP_ON };

#define MIN_TCP_PORT		(1)
#define MAX_TCP_PORT		(65535)
#define MIN_UDP_PORT		(1)
#define MAX_UDP_PORT		(65535)
#define MIN_RTP_PORT		(1024)
#define MAX_RTP_PORT		(65534)

struct network_v4 {
	enum dhcp_onoff	dhcp;
	unsigned int 	ip;
	unsigned int 	mask;
	unsigned int 	gateway;
	unsigned int 	http_port;
	unsigned int 	https_port;
	unsigned int 	rtsp_port;

	unsigned int 	rtp_s_port;
	unsigned int 	rtp_e_port;
#ifdef USE_MCAST
	struct {
		unsigned int ip;
		unsigned int vport;
		unsigned int aport;
		unsigned int ttl;
	} mcast[16];
#endif//USE_MCAST
};

enum live_protocol 	{ LIVE_UDP = 0, LIVE_TCP, LIVE_MUL, LIVE_HTTP };
enum live_stream	{ LIVE_MAIN = 0, LIVE_SECOND, LIVE_THIRD };
//baek.add.20101111.start
enum live_language	{ ENGLISH = 0, ITALIAN, GERMANY, FRENCH, POLISH, RUSSIAN, PORTUGUESE, SPANISH, KOREAN };
//baek.add.20101111.end

enum alarm_type		{ ALARM_NO = 0, ALARM_NC };
enum alarm_mode		{ ALARM_LATCH = 0, ALARM_TRANS };

/* report type ***********************************/
enum report_res { OK = 0, FAIL };

/*
 *	CGI			SVR
 * ------------------------------------------------
 *  1. 	GET_XXX		-->
 *			<--	PUT_XXX
 *
 *  2.	SET_XXX		-->
 *  		<--	PUT_RESULT 
 *  			(return OK or FAIL)
 */
#if 0
enum report_type
{
	// Caution!! DO NOT CHANGE ORDER...
	PUT_RESULT = 0,
	GET_VIDEO,			SET_VIDEO,			PUT_VIDEO,
	GET_CODEC, 			SET_CODEC,			PUT_CODEC,
	GET_CODEC_STOP,		SET_CODEC_STOP,		PUT_CODEC_STOP,
	GET_CODEC_START,	SET_CODEC_START,	PUT_CODEC_START,
	GET_AUDIO,			SET_AUDIO,			PUT_AUDIO,
	GET_AUDIO_STOP,		SET_AUDIO_STOP,		PUT_AUDIO_STOP,
	GET_AUDIO_START,	SET_AUDIO_START,	PUT_AUDIO_START,
	GET_NET_V4,			SET_NET_V4,			PUT_NET_V4,
	GET_FW_UP,			SET_FW_UP,			PUT_FW_UP,
	GET_REBOOT,			SET_REBOOT,			PUT_REBOOT,
	GET_PORT,			SET_PORT,			PUT_PORT,
};
#else
enum report_type
{
	SET_VIDEO,
	SET_IMAGE,
	SET_CODEC,
	SET_CODEC_STOP,
	SET_CODEC_START,
	SET_AUDIO,
	SET_AUDIO_STOP,	
	SET_AUDIO_START,
	SET_NET_V4,	
	SET_FW_UP,
	SET_REBOOT,
	SET_PORT,
	SET_RTP_PORT,
};
#endif

enum install_mode
{
	INSTALL_NONE = 0,
	INSTALL_MODE,
};

/*************************************************
 * User Settings *********************************/
struct setting {
#ifdef USE_INST_MOD
	enum install_mode	install;
#endif
	struct video 		video;
	struct codec 		codec;
	struct audio		audio;
	struct network_v4 	net_v4;
};

#if 0/*{{{*/
#define REPORT_HDR_L	(sizeof(enum report_type) + sizeof(int))
#define MAX_MSG_L	(REPORT_HDR_L + sizeof(union body))

#define PUT_RESULT_L	(REPORT_HDR_L + sizeof(enum report_res))

#define GET_VIDEO_L	(REPORT_HDR_L + sizeof(struct video))
#define SET_VIDEO_L	GET_VIDEO_L
#define PUT_VIDEO_L	GET_VIDEO_L

#define GET_CODEC_L	(REPORT_HDR_L + sizeof(struct codec))
#define SET_CODEC_L	GET_CODEC_L
#define PUT_CODEC_L	GET_CODEC_L

#define GET_AUDIO_L	(REPORT_HDR_L + sizeof(struct audio))
#define SET_AUDIO_L	GET_AUDIO_L
#define PUT_AUDIO_L	GET_AUDIO_L

#define GET_NET_V4_L	(REPORT_HDR_L + sizeof(struct network_v4))
#define SET_NET_V4_L	GET_NET_V4_L
#define PUT_NET_V4_L	GET_NET_V4_L

#define NAMELEN		(32)

#define SZ_1K		(1024)
#define SZ_1M		(SZ_1K*SZ_1K)
#define SZ_1G		(SZ_1M*SZ_1K)
#endif/*}}}*/

#define true		1
#define false		0

#endif
