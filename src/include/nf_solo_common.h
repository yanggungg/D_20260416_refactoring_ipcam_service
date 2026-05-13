#ifndef _NF_SOLO_COMMON_H_
#define _NF_SOLO_COMMON_H_

#include <pthread.h>
#include <sys/ioctl.h>
#include "nf_common.h"
#include "nf_dspcomm_app.h"

#define DEBUG_MSG				(0)		// Print SOLO message 							[0:Disable, 1:Enable]
#define DEBUG_MSG_REC			(0)		// Print SOLO message for recorder				[0:Disable, 1:Enable]
#define DEBUG_MSG_NET			(0)		// Print SOLO message for network				[0:Disable, 1:Enable]
#define DEBUG_MSG_ENC			(0)		// Print SOLO message for encoder				[0:Disable, 1:Enable]
#define DEBUG_MSG_DEC			(0)		// Print SOLO message for decoder				[0:Disable, 1:Enable]
#define DEBUG_MSG_AUD			(0)		// Print SOLO message for audio					[0:Disable, 1:Enable]
#define DEBUG_MSG_JPEG			(0)		// Print SOLO message for jpeg					[0:Disable, 1:Enable]

/* System info */
#ifdef _SNF_1648
#define SOLO_DEV_NUM			(4)		// The number of SOLO6110						[1:OTM, 4:SNF1648]
#else
#define SOLO_DEV_NUM			(2)		// The number of SOLO6110						[1:OTM, 2:SNF0824]
#endif
#define DEBUG_SYS_MEMINFO		(1)		// Print status for system memory				[0:Disable, 1:Enable]

/* Record manager */
#define DEBUG_REC_CONFIG_		(0)		// Print value of configure encoder 			[0:Disable, 1:Enable]
#define DEBUG_REC_P_CNT_		(1)		// Check the count of P frame		 			[0:Disable, 1:Enable]
#define DEBUG_REC_CTRL_FRAME_	(1)		// Control frame used 							[0:Inactive, 1:Active]
#define DEBUG_REC_CBR_MODE_		(1)		// Use CBR mode									[0:Disable, 1:Enable]
#define DEBUG_REC_OUTPUT_		(0)		// Output path 									[0:SST, 1:Etc..]
#define DEBUG_REC_OUTPUT_DEC_	(0)		// Output path 									[0:None, 1:Decoder/File]
#define DEBUG_REC_OUTPUT_FILE_	(0)		// Output path 									[0:File, 1:Decoder]
#define DEBUG_REC_TIMESTAMP_	(0)		// What time is the frame?						[0:6110, 1:nf_host]
#define DEBUG_REC_SOLO_HEADER_	(0)		// SOLO header removed 							[0:Removed, 1:Create]
#define DEBUG_REC_GOP_TOGGLE_	(0)		// GOP toggle(wait I-frame)						[0:Disable, 1:Enable]

/* Encoder */
#define SOLO_ENC_NUM_			(SOLO_DEV_NUM)
#define SOLO_JPEG_DEV_NUM_		(SOLO_DEV_NUM)
#define SOLO_SET_TIMESYNC_		(1)		// Sync with time of nf_host					[0:Disable, 1:Enable]
#define DEBUG_ENC_INFO_FRAME_	(0)		// Print info for read a Frame 					[0:Disable, 1:Enable]
#define DEBUG_ENC_INFO_TASK_	(0)		// Print run time of thread 					[0:Disable, 1:Enable]
#define DEBUG_ENC_INFO_QUEUE_	(0)		// Print count that encorder send data of queue [0:Disable, 1:Enable]
#define DEBUG_ONLY_FRAME_DATA_	(0)		// Except a header (Only sample code)			[0:Disable, 1:Enable]

/* Decoder */
#define SOLO_DEC_DEV_NUM_		(SOLO_DEV_NUM)
#define DEBUG_DEC_GST_BUFFER_	(1)		// Use GST buffer
#define DEBUG_DEC_COPY_BUFFER_	(0)		// Copy to aligned buffer 
#define DEBUG_DEC_DEINTERLACE_	(0)		// Test mode de-interlace.
#define DEBUG_DEC_INFO_QUEUE_	(0)

/* Capture */
#define MEM_NO_COPY				(1)		// decorder data no copy
#define SOLO_CAP_CTRL_QUEUE		(1)

/* Audio */
#define SOLO_AUD_DEV_NUM_		(SOLO_DEV_NUM)
#define SOLO_AUD_UNIT_NUM_		(4)		// The number of audio of a SOLO6110			[4:OTM, 4:SNF]
#define DEBUG_AUD_CONFIG_		(0)		// Print value of configure audio	 			[0:Disable, 1:Enable]
#define DEBUG_AUD_QUEUE_		(0)		// Print count that send data of audio queue	[0:Disable, 1:Enable]
#define DEBUG_AUD_OUTPUT_FILE_	(0)		// Output path									[0:SST, 1:File]
#define DEBUG_AUD_OUTPUT_DEC_	(0)		// Bypass decorder								[0:SST, 1:Decoder]
#define DEBUG_AUD_DEC_CH		(0)		// When DEBUG_AUD_OUTPUT_DEC_, select audio CH	[0:15]
#define DEBUG_AUD_DATA_SIZE_	(0)		// Calc buffer size								[0:97544, 1:Calc]

/* Network */
#define SOLO_NET_ALWAYS_ON_		(1)		// 
#define DEBUG_NET_FPS_REDUCE_	(0)		// Network configure is changed 1FPS.			[0:Disable, 1:Enable]
#define DEBUG_NET_INFO_STATUS_	(0)		// Print info for network status.				[0:Disable, 1:Enable]

/* Task(Thread) */
#define SOLO_TSK_REC_ACTIVE_		(1)		// solo_rec_thread_func() 		[0:Inactive, 1:Active]
#define SOLO_TSK_ENC_ACTIVE_		(1)		// solo_enc_thread_func() 		[0:Inactive, 1:Active]
#define SOLO_TSK_NET_ACTIVE_		(1)		// solo_net_thread_func() 		[0:Inactive, 1:Active]
#define SOLO_TSK_DEC_ACTIVE_		(1)		// solo_dec_thread_func() 		[0:Inactive, 1:Active]
#define SOLO_TSK_AUD_ACTIVE_		(1)		// Audio part init			 	[0:Inactive, 1:Active]
#define SOLO_TSK_AUD_SST_ACTIVE_	(1)		// solo_aud_thread_sst_func() 	[0:Inactive, 1:Active]
#define SOLO_TSK_AUD_ENC_ACTIVE_	(1)		// solo_aud_thread_enc_func() 	[0:Inactive, 1:Active]
#define SOLO_TSK_AUD_DEC_ACTIVE_	(0)		// solo_aud_thread_dec_func() 	[0:Inactive, 1:Active]
#define SOLO_TSK_JPEG_ACTIVE_		(1)		// solo_jpeg_thread_func()	 	[0:Inactive, 1:Active]

#define PRI_ADJUST 					(1)

/* Sleep */
#define SLEEP_TSK_ACTIVE		(0)		// Task sleep		 		[0:Disable, 1:Enable]
#define SLEEP_TSK_ENC			(1000)	// Encorder sleep value		[usec]
#define SLEEP_TSK_REC_ENC		(1)		// Record sleep value 		[usec]
#define SLEEP_TSK_DEC_START		(1)		// Decorder sleep value		[usec]
#define SLEEP_TSK_AUD_SST		(1)		// Audio sst sleep value	[usec]
#define SLEEP_TSK_AUD_ENC		(1)		// Audio read sleep value	[usec]
#define SLEEP_TSK_AUD_DEC		(1)		// Audio decode sleep value	[usec]
#define SLEEP_TSK_JPEG			(1)		// Jpeg sleep value			[usec]

#define BIT0					(0x01)
#define BIT1					(0x02)
#define BIT2					(0x04)
#define BIT3					(0x08)
#define BIT4					(0x10)
#define BIT5					(0x20)
#define BIT6					(0x40)
#define BIT7					(0x80)

/* Dual stream */
#define LOCAL_STREAM 			(0)
#define NETWORK_STREAM			(1)

#define LOCAL_CH_START			(0)
#define LOCAL_CH_END			(15)

#define NETWORK_CH_START		(16)
#define NETWORK_CH_END			(31)

#define LOCAL_CH_NUM			(16)
#define NETWORK_CH_NUM			(16)
#define TOTAL_CH_NUM			( LOCAL_CH_NUM + NETWORK_CH_NUM )

/* CBR bitrate */
#define MAX_CBR_BITRATE			(25165824)  // 3MB/sec
#define STEP_CBR_BITRATE		(128000)	// (128 * 1000)


#define MAX_FRAME_SIZE			(1 << 20)	// 1MB
#define CTRL_FRAME_SIZE			(48)		// Control frame size
#define SOLO_HEADER_SIZE		(64)		// SOLO header size
#define SOLO_STREAM_ALIGN		(64)		// SOLO stream align

/* Byte index of SOLO header */
#define HEADER_INDEX_VOP		2
#define HEADER_INDEX_VIDEO_CH	3
#define HEADER_INDEX_V_SIZE		4
#define HEADER_INDEX_H_SIZE		5
#define HEADER_INDEX_MOTION		7
#define HEADER_INDEX_SCALE		7
#define HEADER_INDEX_JPEG_SIZE1	16
#define HEADER_INDEX_JPEG_SIZE2	17
#define HEADER_INDEX_JPEG_SIZE3	18
#define HEADER_INDEX_INTERVAL1	18
#define HEADER_INDEX_INTERVAL2	19
#define HEADER_INDEX_TIME_SEC	20
#define HEADER_INDEX_TIME_USEC	24
#define HEADER_INDEX_CHANNEL	36
#define HEADER_INDEX_JPEG_QP	48

/* Byte index of SOLO control frame */
#define CTRL_INDEX_SM_START		8
#define CTRL_INDEX_SM_END		11
#define CTRL_INDEX_CHANNEL		19
#define CTRL_INDEX_FLAGS		15
#define CTRL_INDEX_QUALITY		20
#define CTRL_INDEX_FPS			21
#define CTRL_INDEX_RES			22
#define CTRL_INDEX_CODEC		23
#define CTRL_INDEX_AUDIO		30
#define CTRL_INDEX_AUDIO_CH		31

/* Bit index of SOLO header */
#define HEADER_BIT_VOP			( BIT6 | BIT7 )
#define HEADER_BIT_MOTION		( BIT1 | BIT2 )
#define HEADER_BIT_SCALE		( BIT4 | BIT5 | BIT6 | BIT7 )
#define HEADER_BIT_INTERVAL1	( BIT4 | BIT5 | BIT6 | BIT7 )
#define HEADER_BIT_INTERVAL2	( BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 )

#if DEBUG_MSG
#define solo_msg(s)		g_message(s)
#define solo_msg0(s) 	g_message("### SOLO Msg: [%s] %s ###", __FUNCTION__, s)
#define solo_msg1(s, d) g_message("### SOLO Msg: [%s] %s: %d ###", __FUNCTION__, s, d)
#else
#define solo_msg(s)
#define solo_msg0(s)
#define solo_msg1(s, d)
#endif

#endif /* _NF_SOLO_COMMON_H_ */
