#ifndef _NF_DSPCOMM_APP_H_
#define _NF_DSPCOMM_APP_H_

#include <sys/types.h>
#include <sys/ioctl.h>

#include "nf_dspcomm_ioctl.h"		// from driver

#define NF_DSPCOMM_ALIGN_SIZE(size,align) ((size+align-1)&(unsigned int)(~(align-1)))

typedef enum _NF_DSPCOMM_CH_E
{
		/* NF_DSPCOMM_CH_XXXXX 로 변경 함 2008-11-24 오후 8:14:11 */		
	NF_DSPCOMM_CH_STREAM	= 0,
	NF_DSPCOMM_CH_COMMAND	= 1,
	NF_DSPCOMM_CH_MDEVT		= 2,
	NF_DSPCOMM_CH_SNAPSHOT	= 3, 
	
	NF_DSPCOMM_CH_NR		= 4
}NF_DSPCOMM_CHANNEL_E;


#define NF_DSPCOMM_READ_SIZE	32
#define NF_DSPCOMM_DMA_SIZE		(1024*127)

#define MAX_CMD_BUFF (1024*32)

int nf_dspcomm_open_chan(int dspid, int type);
int nf_dspcomm_close_chan(int dspid, int type);
int nf_dspcomm_send_cmd(int dspid, int type, int code, void *data, int len, int nb);
int nf_dspcomm_recv_cmd(int dspid, int type, void *data, int nb);
int nf_dspcomm_recv_cmd_malloc(int dspid, int ch, void **data, int nb);

int nf_dspcomm_ioctl_read( NF_DSPCOMM_IOCTL_COMM *ioctl_buff );
gboolean nf_dspcomm_sync_time();
GTimeVal nf_dspcomm_get_last_sync_time();
GTimeVal nf_dspcomm_get_time( gint dsp_id );

	
typedef struct _dpacket_t {
	unsigned char 	type;
	unsigned char 	code;
	unsigned short	dlen;	
}DPACKET;

enum {
	DPT_REQUEST			= 1,
	DPT_REPLY			= 2,
	DPT_EVENT			= 3,
	DPT_USER			= 4
};

enum {
	DPE_SUCCESS			= 0,
	DPE_PROTOCOL		= 1, // protocol error
	DPE_UNKNOWNREQUEST	= 2,
	DPE_FAULT			= 3, // invalid parameters/data
	DPE_FAIL			= 4, // request failed
	DPE_INTERNAL		= 5, // internal implementation error
	DPE_USER			= 32, // user defined error: 128-255
};

enum {
	DP_SET_TIME			= 32,
	DP_RECORD_START		= 33,
	DP_RECORD_STOP		= 34,
	DP_RECORD_CHANGE_SIGNLE = 35,
	DP_RECORD_CHANGE_MULTI = 36,
	DP_IS_ALIVE			= 37,
	DP_LIVE_CHANGE		= 38, /* change live display mode */
	DP_LIVE_COVERT		= 39, /* covert screen */
	DP_LIVE_FREEZE		= 40, /* freeze screen */
	DP_LIVE_SNAPSHOT	= 41, /* snapshot screen */
	DP_SET_MD_CFG		= 42, /* motion detection */
	DP_SET_VIDEOLOSS	= 45, /* video-loss*/	
	DP_LIVE_RECOVER_VLOSS = 46, /* Recovery after Video-loss */
};

typedef struct _RECORD_INFO_T {
		
	unsigned char channel_id;
	unsigned char flags; //recording flag same as icodec_header flag
	unsigned char quality; //HIGHEST, HIGH, MEDIUM, LOW
	unsigned char fps; //same as icodec_header fps
	unsigned char res;
	unsigned char codec;
	
	//unsigned char reserved[2];	// 2009-02-23 오후 3:58:15 choissinf
	unsigned char audio;
	unsigned char audio_ch;		
} RECORD_INFO;

#define DSP_MAX_CHAN	16

typedef struct _DRREQ_RECORD_START_T {
	RECORD_INFO record_info[DSP_MAX_CHAN];
} DRREQ_RECORD_START;


/*
	DP_RECORD_CHANGE_SIGNLE이나 DP_RECORD_CHANGE_MULTI 공통으로 처리
	dlen에 실리는 데이터의 사이즈를 보고 레코드 변경할 채널수를 구한다.
*/

typedef enum _LiveDisplayMode_e{
	DISPLAY_FULL = 0x00, /* 1 */
	DISPLAY_QUAD = 0x01, /* 4 */
	DISPLAY_HEXA_A = 0x02, /* 6, A-type */
	// DISPLAY_HEXA_B = 0x03, /* 6, B-type */
	DISPLAY_OCTA_A = 0x04, /* 8, A-type */
	// DISPLAY_OCTA_B = 0x05, /* 8, B-type */
	DISPLAY_NONA = 0x06, /* 9 */
	DISPLAY_TRIDECA = 0x07, /* 13 */
	DISPLAY_HEXADECA = 0x08, /* 16 */
	DISPLAY_HEXATRICONTA = 0x09, /* 36 */
	DISPLAY_SEQ = 0x20, /* TBD*/
	DISPLAY_SEQ_DIV = 0x21 /* TBD*/
}LiveDisplayMode_e;

typedef struct _DPREQ_LIVE_CHANGE_T {
	unsigned char display_mode;		/* screen mode: FULL, QUAD,... */
	unsigned char num_display_ch;	/* number of channels to display , TBD */
	unsigned char dein_mode;		/* de-interlacing mode */
	unsigned char reserved;
	
	unsigned char screen_num[DSP_MAX_CHAN];		/* screen index for ch number */
	unsigned char reserved2[DSP_MAX_CHAN];	/* covert flag: TRUE/FALSE */		
} DPREQ_LIVE_CHANGE;

/*
화면상에 표시하지 않을 채널의 screen_num 은 INVALID_SCREEN(=0xFF)으로 채워야 함
dein_mode : ‘0’- full 화면에서 de-interlacing 모드 사용 안 함.
			‘1’-de-interlacing 모드사용
*/

#if 0//defined(_NF_1648)||defined(_NF_0824)
	#define MAX_MD_ROW (12) /*TBD*/
//#else
	#define MAX_MD_ROW (16) /*TBD*/			//hosik_motion 16으로 바꿀 것	
#endif

typedef struct _MDCfg_t {
	unsigned char sensitivity; /* sensitivity : 0~10, TBD */
	unsigned char reserved[3];	
	unsigned long md_area[MAX_MD_ROW]; /* 22x12 motion detection area mask */
}MDCFG; 

/*
Detection area 는 22(column) x 24(row)개의 block 으로 구분 함.
아래 구조체 중 md_area(가로 한 줄의 detection area mask)는 1bit 당 1 개 block 의
enable/disable 상태를 나타내며 LSB 가 화면에서 가장 오른쪽 block 을 설정. 
(즉, 상위 10bit 는 don’t care)
*/

typedef struct _DPREQ_SET_MD_CFG_T{
	unsigned char enable; /* 1:enable, 0:disable */
	unsigned char eval_interval; /* evaluation interval: 1~30[frame] */
	unsigned char sensor_display;	/* 1:display, 0:no-display */
	unsigned char sensor_color; 	/* 0:gray, 1:white, 2:red, 3:green, 4:blue, 5:yellow*/
	MDCFG cfg[DSP_MAX_CHAN];
} DPREQ_SET_MD_CFG; 

typedef struct _DPEVT_MD_RES_T {
	unsigned char ch; /* ch number: 0~15 */
	unsigned char detected; /* 1: detected, 0: not detected */
	unsigned char reserved[2];
	unsigned long md_area[MAX_MD_ROW]; /* 22x24 motion detection area mask */
} DPEVT_MD_RES; 


typedef struct _DPEVT_SET_VIDEO_LOSS_T {
	unsigned char is_videloss[DSP_MAX_CHAN]; /* 1: video-loss, 0: no video-loss */
} DPEVT_SET_VIDEO_LOSS;

/*
DPT_EVENT	type으로 전달
*/

// 2008-11-24 오후 8:06:28
/*
params 의 codec 필드는 icodec_header_t 의 codec 필드에
params 의 reserved 필드는 icodec_header_t 의 timestamp2 필드에 그대로 copy 한다.

DM6446 이 snapshot 을 요청 하면 DM647 이 dpacket header + icodec_header + stream을 
응답으로 3 번 Tx. 채널(DM647 기준)을 통해 전송.
*/
typedef struct _SNAPSHOT_CFG_T{
	unsigned int codec; /* ICODEC, H_264, H_264_KEY, JPEG:3, MPEG4*/
	unsigned int reserved[2];
} SNAPSHOT_CFG; /*12byte*/

typedef struct _DPREQ_LIVE_SNAPSHOT_T{
	unsigned char 	enable[DSP_MAX_CHAN]; /* 1: snapshot, 0: do nothing. */
	SNAPSHOT_CFG param;
} DPREQ_LIVE_SNAPSHOT; /*28byte */


typedef struct _DPREQ_LIVE_FREEZE_T {
	unsigned long long int freeze; /* freeze(1) or not(0), user lower 16bits. */
	unsigned int reserved;
} DPREQ_LIVE_FREEZE; /* 8byte */

typedef struct _DPREQ_LIVE_COVERT_T {
	unsigned long long int covert; /* covert(1) or not(0), user lower 16bits. */
	unsigned int reserved;
} DPREQ_LIVE_COVERT; /* 8byte */


typedef struct _DPREQ_LIVE_RECOVER_VLOSS_T {
	unsigned long long int ch_to_recover; /* recover(1) or not(0), use lower 16bits. */
	unsigned int reserved;
} DPREQ_LIVE_RECOVER_VLOSS; /* 8byte */

#endif
