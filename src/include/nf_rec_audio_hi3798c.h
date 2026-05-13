#ifndef __NF_HI_AUD_3536_H__
#define __NF_HI_AUD_3536_H__

#include "nf_common.h"
#include "nf_object.h"

#if 0
	#define NF_REC_AUDIO_HISILICON_TEST
#endif

#define NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW     // pakkhman

/* type macro */
#define NF_TYPE_HI_AUD						(nf_hi_aud_get_type ())

#define NF_IS_HI_AUD(obj)					(G_TYPE_CHECK_INSTANCE_TYPE ((obj), NF_TYPE_HI_AUD))
#define NF_IS_HI_AUD_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE ((klass),  NF_TYPE_HI_AUD))

#define NF_HI_AUD_GET_CLASS(obj)				(G_TYPE_INSTANCE_GET_CLASS ((obj),  NF_TYPE_HI_AUD, NfHiaudClass))
#define NF_HI_AUD(obj)						(G_TYPE_CHECK_INSTANCE_CAST ((obj), NF_TYPE_HI_AUD, NfHiaud))
#define NF_HI_AUD_CLASS(klass)				(G_TYPE_CHECK_CLASS_CAST ((klass),  NF_TYPE_HI_AUD, NfHiaudClass))

#define NF_HI_AUD_CAST(obj)					((NfHiaud*)(obj))
#define NF_HI_AUD_CLASS_CAST(klass)			((NfHiaudClass*)(klass))

#define HI_AUDIO_PTNUMPERFRM				0x800

typedef enum _NF_HI_AUD_OUTPUT_E
{                
	HI_AUD_OUTPUT_RCA		= 0,
	HI_AUD_OUTPUT_HDMI		= 1,
	HI_AUD_OUTPUT_AUTO		= 2,

	AUD_OUTPUT_UNKNOWN		= 3,
}_NF_HI_AUD_OUTPUT_E;

typedef enum _NF_HI_AUD_CMD_E
{
	HI_CMD_AUD_REC,
	HI_CMD_AUD_REC_GST_BUFFER,
	HI_CMD_AUD_NET,
	HI_CMD_AUD_CFG

} NF_HI_AUD_CMD;

typedef enum _NF_HI_AUD_CMD_OUT_E
{
	HI_CMD_AUD_OUT_HDMI,
	HI_CMD_AUD_OUT_HDMI_GST_BUFFER,

} NF_HI_AUD_CMD_OUT;

typedef struct _NF_HI_AUD_PARAM_T
{
	GThread		*thread;
	HI_HANDLE	hAvplay;
	HI_U32		u32Snd;
	HI_U32		u32SndNum;

	#if defined(NF_REC_AUDIO_HISILICON_TEST)
		// For Test!!
		FILE		*fp;
		gint		thread_run_test;
	#endif

} NF_HI_AUD_PARAM;

typedef struct _NF_HI_AUD_QDATA_T
{
	gint  s32Cmd;
	gint  s32Chn;
	gint  s32Len;
	guint64 u64Start;
	guint64 u64End;
	HI_VOID *pData;
} NF_HI_AUD_QDATA;

typedef struct _NF_HI_AUD_DATA_T
{
	gint  s32Cmd;
	gint  s32Len;
	guchar	stream[];
} NF_HI_AUD_DATA;


typedef struct _NfHiaud			NfHiaud;
typedef struct _NfHiaudClass	NfHiaudClass;

/* Definition Enum */
typedef enum NF_HI_AUD_INPUT_LV_E
{
	HI_AUD_INPUT_LV0        = 0 ,
	HI_AUD_INPUT_LV1        = 1 ,
	HI_AUD_INPUT_LV2        = 2 ,
	HI_AUD_INPUT_LV3        = 3 ,
	HI_AUD_INPUT_LV4        = 4 ,
	HI_AUD_INPUT_LV5        = 5 ,
	HI_AUD_INPUT_LV6        = 6 ,
	HI_AUD_INPUT_LV7        = 7 ,
	HI_AUD_INPUT_LV8        = 8 ,
	HI_AUD_INPUT_LV9        = 9 ,
	HI_AUD_INPUT_LV10       = 10 ,
	HI_AUD_INPUT_LV11       = 11 ,
	HI_AUD_INPUT_LV12       = 12 ,
	HI_AUD_INPUT_LV13       = 13 ,
	HI_AUD_INPUT_LV14       = 14 ,
	HI_AUD_INPUT_LV15       = 15
} NF_HI_AUD_INPUT_LV;

/**
 * NfEvent:
 *
 * NfDVR notify class
 */
struct _NfHiaud {
	NfObject            object;

	GMainContext        *context;
	GMainLoop           *loop;

	gint				init_done;

	GAsyncQueue			*queue;
	GAsyncQueue			*queue_out;

	GThread             *thread_rec;
	GThread             *thread_rd;
	GThread             *thread_out;

	gint				thread_run_rec;
	gint				thread_run_rd;
	gint				thread_run_out;

	volatile guint		vin_mask;

	NF_HI_AUD_PARAM		aud_param;

	guint				aud_output;
}__attribute__((packed));

struct _NfHiaudClass {
	NfObjectClass   parent_class;

	/* signals */

	/*< public >*/

	/*< private >*/

}__attribute__((packed));

//typedef NF_REC_AUDIO_PARAM NF_HI_REC_AUDIO_PARAM;

gboolean nf_hi_aud_init(void);
HI_BOOL nf_hi_aud_start(GValue *data);
gboolean nf_hi_aud_send_frame_out(gpointer frame, gint size);
gboolean nf_hi_aud_send_frame_rec(gint ch_num, gpointer frame, gint size);

#if defined(NF_HI_AUDIO_CONVERT_LPCM16_TO_MURAW)
	guchar nf_hi_lpcm16_to_muraw(short  pcm_val);
	short nf_hi_search_seg(short val, short *table, short size);
	short nf_hi_muraw_to_lpcm16(guchar u_val);
	void nf_hi_aud_convert(gchar *stream_src, gchar *stream_dest, guint len);
#endif

#endif

