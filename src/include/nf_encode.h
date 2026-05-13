#ifndef __NF_ENCODE_H__
#define __NF_ENCODE_H__

/******************************************************************************/
#include "gobjmedia.h"
#include "nf_event.h"

/******************************************************************************/
#if 0
typedef unsigned int    guint;
typedef int             gint;
typedef void*           gpointer;
typedef unsigned char   guchar;
typedef char            gchar;
typedef unsigned long long guint64;
typedef int             gboolean;

#define g_return_val_if_fail(c, val)    if(!(c))    return val;
#define usleep(val)         TIMM_OSAL_SleepTask(val)
#define g_malloc0(val)      calloc(val, sizeof(char))
#define g_assert(val)       assert(val)
#define g_free(val)         free(val)
#endif

#define GETBIT(x, y)        (((x)>>(y)) & 0x01)
#define	SETBIT(x, y)		((x) |= (1 << (y)))
#define RESETBIT(x, y)		((x) &= ~(1 << (y)))

#define SEC_TO_nSEC(t)	        	(t*1000000000)  // sec to nsec
#define nSEC_TO_mSEC(t)	        	(t/1000000)     // nsec to msec
#define nSEC_TO_SEC(t)	        	(t/1000000000)  // nsec to sec
#define mSEC_TO_nSEC(t)		        (t*1000000)     // msec to nsec
#define mSEC_TO_SEC(t)		        (t/1000)        // msec to sec
#define mSEC_TO_5mSEC(t)	        (t/5)           // msec to 5msec

#define MASK_ONE_BIT                (0x01)
#define MASK_THREE_BITS             (0x07)
#define GET_REC_NET_FLAG(f)         (f>>0)&(MASK_ONE_BIT)
#define GET_REC_PREREC_FLAG(f)      (f>>1)&(MASK_ONE_BIT)
#define GET_REC_REASON(f)           (f>>2)&(MASK_THREE_BITS)

#define CLEAR_REC_NETWORK_FLAG(f)   ( f & (~(MASK_ONE_BIT   <<0)) )
#define CLEAR_REC_PREREC_FLAG(f)    ( f & (~(MASK_ONE_BIT   <<1)) )
#define CLEAR_REC_REASON(f)         ( f & (~(MASK_THREE_BITS<<2)) )

#define SET_REC_NETWORK_FLAG(f)	    (f|(MASK_ONE_BIT<<0))
#define SET_REC_PREREC_FLAG(f)      (f|(MASK_ONE_BIT<<1))
#define SET_REC_REASON(f, v)        (CLEAR_REC_REASON(f)|(v<<2))

#define SEC                 (1000)
#define mSEC                (1)
#define KB                  (1024)

/******************************************************************************/
#define DBG_NUM_CHAN            (32)
//#define NUM_ACTIVE_CH           (4) // TODO: 
#define NUM_1ST_STM_CH          (NUM_ACTIVE_CH)
#define NUM_2ND_STM_CH          (NUM_ACTIVE_CH)
#define NUM_REC_CHAN            (NUM_1ST_STM_CH+NUM_2ND_STM_CH)

#define MAX_GOP_TBL_NUM			(7)
#define GOP_TBL_LEN				(64)

#define NF_VID_W_3M             (2048)
#define NF_VID_H_3M             (1536)

#define NF_VID_W_3_7M           (2560)
#define NF_VID_H_3_7M           (1440)

#define NF_VID_W_4M_EXC    (2560)
#define NF_VID_W_4M             (2688)
#define NF_VID_H_4M             (1520)

#define NF_VID_W_2_8M           (1920)
#define NF_VID_H_2_8M           (1536)

#define NF_VID_W_5M_EXC     (2560)
#define NF_VID_W_5M             (2592)
#define NF_VID_H_5M             (1944)

#define NF_VID_W_8M				(3840)
#define NF_VID_H_8M				(2160)

#define NF_VID_W_8M_EXC       (2560)

#define NF_VID_W_1080P          (1920)
#define NF_VID_H_1080P          (1080)

#define NF_VID_W_720P           (1280)
#define NF_VID_H_720P           (720)

#define NF_VID_W_360P           (640)
#define NF_VID_H_360P           (360)

#define NF_VID_W_2160P          (1920)
#define NF_VID_H_2160P          (2160)

#define NF_VID_W_1344P          (1344)
#define NF_VID_H_1344P          (1520)

#define NF_VID_W_1296P          (1296)
#define NF_VID_H_1296P          (1944)

#define NF_VID_W_1024P          (1024)
#define NF_VID_H_1024P          (1536)

#define NF_VID_W_1280P          (1280)
#define NF_VID_H_1280P          (1440)

#define NF_VID_W_640P            (640)
#define NF_VID_H_640P            (480)

#define NF_VID_W_5M_HALF      (1280)
#define NF_VID_H_5M_HALF      (1944)

#define NF_VID_W_4M_HALF      (1280)
#define NF_VID_H_4M_HALF      (1520)

#define NF_VID_W_8M_HALF       (1280)
#define NF_VID_H_8M_HALF       (2160)

#define NF_VID_W_960_NTSC      (960)
#define NF_VID_H_960_NTSC      (480)
#define NF_VID_W_4CIF_NTSC     (704)
#define NF_VID_H_4CIF_NTSC     (480)
#define NF_VID_W_2CIF_NTSC     (NF_VID_W_4CIF_NTSC)
#define NF_VID_H_2CIF_NTSC     (NF_VID_H_4CIF_NTSC>>1)
#define NF_VID_W_1CIF_NTSC     (NF_VID_W_4CIF_NTSC>>1)
#define NF_VID_H_1CIF_NTSC     (NF_VID_H_4CIF_NTSC>>1)

#define NF_VID_W_960_PAL      (960)
#define NF_VID_H_960_PAL      (576)
#define NF_VID_W_4CIF_PAL     (704)
#define NF_VID_H_4CIF_PAL     (576)
#define NF_VID_W_2CIF_PAL     (NF_VID_W_4CIF_PAL)
#define NF_VID_H_2CIF_PAL     (NF_VID_H_4CIF_PAL>>1)
#define NF_VID_W_1CIF_PAL     (NF_VID_W_4CIF_PAL>>1)
#define NF_VID_H_1CIF_PAL     (NF_VID_H_4CIF_PAL>>1)

#define FPS_32  (30)
#define FPS_31  (30)
#define FPS_30  (30)    //1966080
#define FPS_29  (29)
#define FPS_28  (28)
#define FPS_27  (27)
#define FPS_26  (26)
#define FPS_25  (25)    //
#define FPS_24  (24)
#define FPS_23  (23)
#define FPS_22  (22)
#define FPS_21  (21)
#define FPS_20  (20)
#define FPS_19  (19)
#define FPS_18  (18)
#define FPS_17  (17)
#define FPS_16  (16)    //983040
#define FPS_15  (15)
#define FPS_14  (14)
#define FPS_13  (13)    //
#define FPS_12  (12)
#define FPS_11  (11)
#define FPS_10  (10)
#define FPS_09  ( 9)
#define FPS_08  ( 8)    //524288
#define FPS_07  ( 7)
#define FPS_06  ( 6)
#define FPS_05  ( 5)
#define FPS_04  ( 4)    //262144
#define FPS_03  ( 3)
#define FPS_02  ( 2)    //131072
#define FPS_01  ( 1)    //65536
#define FPS_00  ( 0)

// 	hand off
typedef void (*NF_ENCODE_HANDOFF_FUNC) ( gpointer data ); 

typedef enum _NF_CTRL_FLAG_E{
    RP00    = 0,	/*00*/
    RP01    = 1,	/*01*/
    RP10    = 2,	/*10*/
    RP11    = 3     /*11*/
}NF_CTRL_FLAG_E;

typedef enum _NF_END_FRAME_FLAG_E{
    END_FLAG_NONE           = 0,
    END_FLAG_FLUSH_PREREC   = 1,
    END_FLAG_DISCARD_PREREC = 2
}NF_END_FRAME_FLAG_E;

typedef enum _NF_ENC_STATUS_E{
    H264ENC_INTRA       = 0x20,     //1=intra, 0=inter
    H264ENC_ERROR       = 0x24,
    H264ENC_FRAMESKIP   = 0x28,
    H264ENC_EXTERROR    = 0x2C
}NF_ENC_STATUS_E;

typedef enum _NF_RP_NUM_E {
    CUR = 0,        // current RP index
    NEW = 1,        // new RP index
    NUM_RP
}NF_RP_NUM_E;

typedef enum _NAL_UNIT_TYPE_E {
    NALU_TYPE_SLICE = 1, /*!<  1: slice of non-IDR picture                   */
    NALU_TYPE_DPA,       /*!<  2: slice data partition A                     */
    NALU_TYPE_DPB,       /*!<  3: slice data partition B                     */
    NALU_TYPE_DPC,       /*!<  4: slice data partition C                     */
    NALU_TYPE_IDR,       /*!<  5: slice of IDR picture                       */
    NALU_TYPE_SEI,       /*!<  6: SEI                                        */
    NALU_TYPE_SPS,       /*!<  7: SPS                                        */
    NALU_TYPE_PPS,       /*!<  8: PPS                                        */
    NALU_TYPE_AUD,       /*!<  9: access unit delimiter                      */
    NALU_TYPE_EoSEQ,     /*!< 10: end of sequence                            */
    NALU_TYPE_EoSTR,     /*!< 11: end of stream                              */
    NALU_TYPE_FILL,      /*!< 12: filler data                                */
    MAX_NALU_TYPE = 32   /*!< 32: size of this type array                    */
} NAL_UNIT_TYPE_E;

/******************************************************************************/
typedef enum _NF_ENC_STREAM_SERVICE_STATUS_E {
    ENC_SERVICE_1ST  = 0,
    ENC_SERVICE_2ND  = 1,
    NUM_ENC_SERVICE_STATUS
}NF_ENC_STREAM_SERVICE_STATUS_E;

typedef enum _NF_ENC_STREAM_TRANSIT_STATUS_E {
    ENC_SERVICE_STEADY  = 0,
    ENC_SERVICE_TRANSIT = 1,
    NUM_ENC_STEADY_STATUS
}NF_ENC_STREAM_TRANSIT_STATUS_E;

/******************************************************************************/
#if 0   //nf_record.h
typedef struct _NF_STREAM_T {
    guint       id;
    gpointer    frame;

	guint		i_idx;			// add choissi 2010-04-26  for dual stream
	guint		frame_idx;
	guint		is_2nd_stream;	
} NF_STREAM_T;

typedef struct _NF_REC_FRAME_T {
    guint           num_of_stream;
    NF_STREAM_T    stream[NUM_REC_CHAN];
}NF_REC_FRAME;
#endif

#if 0   //dispmux.h
struct _DispmuxEncCbParam {
	int chan;
	
	int flags;
	int bit_rate;
	
	int width;
	int height;
	
	int frame_rate;
	int i_interval;
	int qp;	
	int frame_type;
	int change;
	
	int frame_size;
	int timestamp;
	int timestampl;
};

#endif

typedef struct _NF_REC_INFO_T {
	guint  channel_id;
	guint  flags;           // recording flag same as icodec_header flag
	guint  quality;         // HIGHEST, HIGH, MEDIUM, LOW
	guint  fps;             // same as icodec_header fps
	guint  real_fps;	
	guint  i_interval;      //
	guint  res;
	guint  bit_rate;
	
	guchar codec;           // not used
	guchar is_videoloss;
	guchar reserved[2];

	guchar type;
	guchar analog_type;
	guchar enc_mode;		

} NF_REC_INFO_T;

typedef struct _NF_ENC_FLAG_T {
	NF_CTRL_FLAG_E	RP[NUM_RP]; /* current record(0)/new-record flag(1) */
	guchar  net;                /* flag for sending network or not */
	guchar  pre;                /* flag for pre-record or not */
    guchar  rec;                /* flag for record or not */
	guchar  enc;                /* flag for encode or not */
	guchar  flags;              /* save the 'flags' of command */
	guchar  reserved[3];
} NF_ENC_FLAG_T;

typedef struct _NF_ENC_CH_INFO_T {
    guint    ch;
    gpointer h_edma;

    guint    gop_toggle_cnt;
    guint    gop_idx;    /* current index in gop tbl */
    guint    gop_int;    /* gop interval */
    guint    fps_idx;    /* index of gop tbl for fps */
    
	NF_ENC_FLAG_T	enc_flag;   /* current enc status flags */
    NF_REC_INFO_T   rec_cfg;    /* current recording cfg. */
    guint   seq_num;
    guint64 timestamp;
    guint same_cnt;
}NF_ENC_CH_INFO_T;

typedef struct _NF_ENC_CH_STATUS_T {
    gint    cur_svc_stm;    // current service stream
    gint    new_svc_stm;    // new service stream
    gint    status;
}NF_ENC_CH_STATUS_T;

typedef struct _NF_ENC_CONTEXT_T {
    NF_ENC_CH_STATUS_T  ch_status[32];
    NF_ENC_CH_INFO_T    ch_info[32];
} NF_ENC_CONTEXT_T;

/******************************************************************************/

extern int
encode_cb( GOBJEncodeCbParam *data, guchar *stream_buf );

extern int
nf_encode_set_param(NF_REC_INFO_T *rec_param, int num_rec_param, int change_mask, int cam_change_mask );

extern int
convert_omx_fps( gint fps );

#if 1
int convert_to_cam_fps(gchar type, int cam_max_fps, int *enc_fps, int *fps_cnt );
#else
int 
convert_to_cam_fps(gchar type, int cam_max_fps, int *enc_fps, int *fps_cnt );
#endif

#endif  /*__NF_ENCODE_H__*/
