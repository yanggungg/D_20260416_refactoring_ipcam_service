#ifndef __NF_AUDIO_COMMON_H__
#define __NF_AUDIO_COMMON_H__

typedef enum _NF_AUDIO_INPUT_CH_E
{   
	NF_AUDIO_INPUT_CH01,
	NF_AUDIO_INPUT_CH02,
	NF_AUDIO_INPUT_CH03,
	NF_AUDIO_INPUT_CH04,
	NF_AUDIO_INPUT_CHNR, 
	NF_AUDIO_INPUT_OFF = 0xff
} NF_AUDIO_INPUT_CH_E;

typedef enum _NF_AUDIO_SEND_TYPE_E
{
    NF_AUDIO_SEND_DVR               = 0,
    NF_AUDIO_SEND_IPCAM             = 1
} NF_AUDIO_SEND_TYPE;

typedef struct _NF_REC_AUDIO_PARAM_T
{   
	guchar  ch_arr       [64];
	guchar  rec_reason   [64];  
	guchar  pre_rec_time [64];  // pre_rec_time(sec) 0:normal
	guchar  pre_rec_close[64];  // pre_rec 0:none,discard 1:flush
	gboolean        send_type;
} NF_REC_AUDIO_PARAM;

#endif

